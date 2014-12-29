#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <zlib.h>
#include "png.h"

int power(int base, int power) {
	int total = 1;
	while (power > 0) {
		total = total * base;
		power--;
	}
	return total;
}

int calculateLength(unsigned char *arr, int numElements) {
	int length = 0;
	int i;
	for (i = 0; i < numElements; i++) {
		length = length + arr[i] * power(16, 2 * (numElements - i - 1));
	}
	return length;
}

int getLength(FILE *f) {
	unsigned char lengthArr[4];
	fread(lengthArr, 1, 4, f);
	return calculateLength(lengthArr, 4);
}

unsigned long calculateChecksum(unsigned char *chunktypeArr,
		unsigned char *textData, unsigned length) {
	unsigned long crc = crc32(0L, Z_NULL, 0);
	crc = crc32(crc, chunktypeArr, 4);
	return crc32(crc, textData, length);
}

int printTextData(FILE *f, int length, unsigned char *chunktypeArr) {

	unsigned char textData[length];
	fread(textData, 1, length, f);

	unsigned char checkSumArr[4];
	fread(checkSumArr, 1, 4, f);
	unsigned long checkSum = (unsigned long)calculateLength(checkSumArr, 4);
	unsigned long calculatedChecksum = calculateChecksum(chunktypeArr, textData, length);

	if ((int)checkSum != (int)calculatedChecksum) {
		printf("checksum: %d, calculated: %d\n", (int)checkSum, (int)calculatedChecksum);
		printf("%s\n", "checksums weren't equal");
		return -1;
	}

	int nulByteIndex = 0;
	while (textData[nulByteIndex] != 0x00) {
		nulByteIndex++;
	}
	int i;
	for (i = 0; i < nulByteIndex; i++) {
		printf("%c", textData[i]);
	}
	i++;
	printf("%s", ": ");
	for (; i < length; i++) {
		printf("%c", textData[i]);
	}
	printf("\n");
	return 0;
}

int printZtxtData(FILE *f, int length, unsigned char *chunktypeArr) {
	unsigned char textData[length];
	fread(textData, 1, length, f);

	unsigned char checkSumArr[4];
	fread(checkSumArr, 1, 4, f);
	unsigned long checkSum = (unsigned long)calculateLength(checkSumArr, 4);
	unsigned long calculatedChecksum = calculateChecksum(chunktypeArr, textData, length);

	if ((int)checkSum != (int)calculatedChecksum) {
		return -1;
	}

	int nulByteIndex = 0;
	while (textData[nulByteIndex] != 0x00) {
		nulByteIndex++;
	}
	int i;
	for (i = 0; i < nulByteIndex; i++) {
		printf("%c", textData[i]);
	}
	printf("%s", ": ");

	/* Get compressed data from textData into compressedData */
	unsigned long compressedDataLength = (unsigned long)(length) - nulByteIndex - 2;
	unsigned char compressedData[compressedDataLength];
	for (i = 0; i < compressedDataLength; i++) {
		compressedData[i] = textData[nulByteIndex + 2 + i];
	}

	unsigned long uncompressedDataLength = compressedDataLength * 2;
	unsigned char *uncompressedData = (unsigned char *)malloc(sizeof(unsigned char) * uncompressedDataLength);

	int uncompressStatus = uncompress(uncompressedData, &uncompressedDataLength, compressedData, compressedDataLength);
	while (uncompressStatus != Z_OK) {
		if (uncompressStatus == Z_MEM_ERROR) {
			return -1;
		} else {
			uncompressedDataLength = uncompressedDataLength * 2;
			uncompressedData = (unsigned char *) realloc(uncompressedData, (uncompressedDataLength)*sizeof(unsigned char));
		}
		uncompressStatus = uncompress(uncompressedData, &uncompressedDataLength, compressedData, compressedDataLength);
	}
	for (i = 0; i < uncompressedDataLength; i++) {
		printf("%c", uncompressedData[i]);
	}
	printf("\n");
	return 0;

}

int printTimeData(FILE *f, int length, unsigned char *chunktypeArr) {
	int year;
	unsigned char yearArr[2], month, day, hour, minute, second;
	unsigned char timeData[length];
	fread(timeData, 1, length, f);

	unsigned char checkSumArr[4];
	fread(checkSumArr, 1, 4, f);
	unsigned long checkSum = (unsigned long)calculateLength(checkSumArr, 4);
	unsigned long calculatedChecksum = calculateChecksum(chunktypeArr, timeData, length);

	if ((int)checkSum != (int)calculatedChecksum) {
		return -1;
	}

	yearArr[0] = timeData[0];
	yearArr[1] = timeData[1]; 
	year = calculateLength(yearArr, 2);
	month = timeData[2];
	day = timeData[3];
	hour = timeData[4];
	minute = timeData[5];
	second = timeData[6];

	printf("Timestamp: %d/%d/%d %d:%d:%d\n", (int)month, (int)day, (int)year, (int)hour, (int)minute, (int)second);
	return 0;
}

int analyzeChunkType(FILE *f, int length) {
	unsigned char chunktypeArr[4];
	fread(chunktypeArr, 1, 4, f);
	int i;
	/* tEXt */ 
	if ((chunktypeArr[0] == 0x74) && (chunktypeArr[1] == 0x45) && 
			(chunktypeArr[2] == 0x58) && (chunktypeArr[3] == 0x74)) {
		return printTextData(f, length, chunktypeArr);
	}
	/* zTXt */ 
	else if ((chunktypeArr[0] == 0x7a) && (chunktypeArr[1] == 0x54) && 
			(chunktypeArr[2] == 0x58) && (chunktypeArr[3] == 0x74)) {
		return printZtxtData(f, length, chunktypeArr);
	}
	/* tIME */ 
	else if ((chunktypeArr[0] == 0x74) && (chunktypeArr[1] == 0x49) && 
			(chunktypeArr[2] == 0x4d) && (chunktypeArr[3] == 0x45)) {
		return printTimeData(f, length, chunktypeArr);
	}
	else {
		for(i = 0; i < length+4; i++) {
			if(fgetc(f) == EOF){
				break;
			}
		}
	}
	return 0;
}

int validatePNG(FILE *f) {
	int correctHeader[8] = {0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a};
	unsigned char header[8];
	fread(header, 1, 8, f);
	int i;
	for (i = 0; i < 8; i++) {
		if (header[i] != correctHeader[i]) {
			return -1;
		}
	}
	return 0;
}

/*
 * Analyze a PNG file.
 * If it is a PNG file, print out all relevant metadata and return 0.
 * If it isn't a PNG file, return -1 and print nothing.
 */
int analyze_png(FILE *f) {
	if (validatePNG(f) == -1) {
		return -1;
	}
	while (!feof(f)) {
		int length = getLength(f);
		analyzeChunkType(f, length);
	}
	return 0;
}