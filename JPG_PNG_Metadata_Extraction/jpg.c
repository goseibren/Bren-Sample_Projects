#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "jpg.h"

/*
 * Analyze a JPG file that contains Exif data.
 * If it is a JPG file, print out all relevant metadata and return 0.
 * If it isn't a JPG file, return -1 and print nothing.
 */
 int analyzeChunkType1(FILE *f);
int analyzeChunkType2(FILE *f, unsigned char firstMarkByte, unsigned char secondMarkByte);
int analyzeAPP1Chunk(FILE *f, int standChunkDataLength);
int analyzeTiffFile(FILE *f, int updatedDataLength);
int analyzeStandardChunk(FILE *f, unsigned char secondMarkByte);
int analyzeIFD(unsigned char *TiffData, int offsetIFDPos);
 /* HELPER FUNCTIONS */
 /*--------------------------------------------------------------------------*/
 /*POWER CACULATION*/
int power1(int base, int power) {
	int total = 1;
	while (power > 0) {
		total = total * base;
		power--;
	}
	return total;
}

 /* THIS IS BIG ENDIAN CALCULATION! */
 int calculateLength1(unsigned char *arr, int numElements) {
	int length = 0;
	int i;
	for (i = 0; i < numElements; i++) {
		length = length + arr[i] * power1(16, 2 * (numElements - i - 1));
	}
	return length;
}

/* THIS IS Little Endian Calculate! */
int LittleEndianCalculate(unsigned char *arr, int numElements){
	int length = 0;
	int i;
	for (i = 0; i < numElements; i++) {
		length = length + arr[numElements - i - 1] * power1(16, 2 * (numElements - i - 1));
	}
	return length;
}

/* CASE FUNCTIONS */
/*----------------------------------------------------------------------------*/

/* SUPER CHUNK CASE */
int analyzeSuperChunk(FILE *f){
	unsigned char ffByte;
	unsigned char nextByte;
	while(!feof(f)){
		ffByte = fgetc(f);
		if (ffByte == 0xff){
			nextByte = getc(f);
			if(nextByte != 0x00){
				return analyzeChunkType2(f, ffByte, nextByte);
			}
		}
	}
	return 0;
}

/* STANDARD CHUNK CASE */
int analyzeStandardChunk(FILE *f, unsigned char secondMarkByte){

	/* Caculate the length of Data Field! */
	unsigned char standChunkLengthBytesArr[2];
	fread(standChunkLengthBytesArr, 1, 2, f);
	int standChunkDataLength = calculateLength1(standChunkLengthBytesArr, 2) - 2;
	int i;
	/* Check if Tiff Type (APP1 CHUNK) else dont analyze! still must skip data bits though */
	if(secondMarkByte == 0xe1){
		return analyzeAPP1Chunk(f, standChunkDataLength);
	}
	else {
		for(i = 0; i < standChunkDataLength; i++){
			fgetc(f);
		}
		return 0;
	}
}

/* APP1 CHUNK CASE (TIFF FILE) */
int analyzeAPP1Chunk(FILE *f, int standChunkDataLength){
	//find 0x45 0x78 0x69 0x66 0x00 0x00 sequence 
	//Tiff File follows that sequence
	int i;
	int updatedDataLength = standChunkDataLength;
	for(i = 0; i < standChunkDataLength; i++){
		if (fgetc(f) == 0x45){
			updatedDataLength--;
			if (fgetc(f) == 0x78){
				updatedDataLength--;
				if (fgetc(f) == 0x69){
					updatedDataLength--;
					if (fgetc(f) == 0x66){
						updatedDataLength--;
						if (fgetc(f) == 0x00){
							updatedDataLength--;
							if (fgetc(f) == 0x00){
								 updatedDataLength--;
								return analyzeTiffFile(f, updatedDataLength);
							}
						}
					}
				}
			}
		}
	}
	return 0;
}

/* TIFF FILE CASE */
int analyzeTiffFile(FILE *f, int updatedDataLength){
	unsigned char TiffData[updatedDataLength];
	fread(TiffData, 1, updatedDataLength, f);
	// unsigned char endianness[2] = {TiffData[0], TiffData[1]};
	// unsigned char magicString[2] = {TiffData[2], TiffData[3]};
	unsigned char offset[4] = {TiffData[4], TiffData[5], TiffData[6], TiffData[7]};
	int offsetIFDPos = LittleEndianCalculate(offset, 4);
	return analyzeIFD(TiffData, offsetIFDPos);
	//IFD FILE
}


/* ANALYZE IFD CASE */
int analyzeIFD(unsigned char *TiffData, int offsetIFDPos){
	int i;
	int j;
	int currentStructPos = offsetIFDPos + 2;
	unsigned char numOfTagStructsArr[2] = {TiffData[offsetIFDPos], TiffData[offsetIFDPos + 1]};
	int numOfTagStructs = LittleEndianCalculate(numOfTagStructsArr, 2);
	unsigned char tagID[2];
	unsigned char dataType[2];
	unsigned char count[4];
	int dataStartPos;
	unsigned char offsetOrValue[4];
	int printType; //0 for ASCII, 1 for Undefined, 2 for UserComment
	int tagIDVal, dataTypeVal, countVal, offsetOrValueVal;

	for(i = 0; i < numOfTagStructs; i++){
		tagID[0] = TiffData[currentStructPos++]; 
		tagID[1] = TiffData[currentStructPos++];
		dataType[0] = TiffData[currentStructPos++];
		dataType[1] = TiffData[currentStructPos++];
		count[0] = TiffData[currentStructPos++];
		count[1] = TiffData[currentStructPos++];
		count[2] = TiffData[currentStructPos++];
		count[3] = TiffData[currentStructPos++];
		dataStartPos = currentStructPos;
		offsetOrValue[0] = TiffData[currentStructPos++];
		offsetOrValue[1] = TiffData[currentStructPos++];
		offsetOrValue[2] = TiffData[currentStructPos++];
		offsetOrValue[3] = TiffData[currentStructPos++];
		tagIDVal = LittleEndianCalculate(tagID, 2);
		dataTypeVal = LittleEndianCalculate(dataType, 2);
		countVal = LittleEndianCalculate(count, 4);
		offsetOrValueVal = LittleEndianCalculate(offsetOrValue, 4);

		/*ANALYZE PRINT HEADER*/
		switch (tagIDVal){
			case 0x010D:
				printf("DocumentName: ");
				printType = 0;
				break;
			case 0x010E:
				printf("ImageDescription: ");
				printType = 0;
				break;
			case 0x010F:
				printf("Make: ");
				printType = 0;
				break;
			case 0x0110:
				printf("Model: ");
				printType = 0;
				break;
			case 0x0131:
				printf("Software: ");
				printType = 0;
				break;
			case 0x0132:
				printf("DateTime: ");
				printType = 0;
				break;
			case 0x013B:
				printf("Artist: ");
				printType = 0;
				break;
			case 0x013C:
				printf("HostComputer: ");
				printType = 0;
				break;
			case 0x8298:
				printf("Copyright: ");
				printType = 0;
				break;
			case 0xA004:
				printf("RelatedSoundFile: ");
				printType = 0;
				break;
			case 0x9003:
				printf("DateTimeOriginal: ");
				printType = 0;
				break;
			case 0x9004:
				printf("DateTimeDigitized: ");
				printType = 0;
				break;
			case 0x927C:
				printf("MakerNote: ");
				printType = 1;
				break;
			case 0x9286:
				printf("UserComment: ");
				printType = 2;
				break;
			case 0xA420:
				printf("ImageUniqueID: ");
				printType = 0;
				break;
			case 0x8769:
				return analyzeIFD(TiffData, offsetOrValueVal);
				break;
			default:
				continue;

		}

		/* ANALYZE PRINT DATA! */
		switch (dataTypeVal){
			case 0x0001:
				break;
			case 0x0002:
				if (countVal > 4) {
					dataStartPos = offsetOrValueVal;
				}
				for (j = dataStartPos; j < dataStartPos + countVal - 1; j++){
					printf("%c", TiffData[j]);
				}
				printf("\n");
				break;
			case 0x0003:
				break;
			case 0x0004:

				break;
			case 0x0005:
				break;
			case 0x0007:
				if (countVal > 4) {
						if (printType != 2){
							dataStartPos = offsetOrValueVal;
						}
						else {
							dataStartPos = offsetOrValueVal + 8;
							countVal = countVal - 8;
						}
					}
					for(j = dataStartPos; j < dataStartPos + countVal; j++){
						if(TiffData[j] != 0x00){
							printf("%c", TiffData[j]);
						}
						else {
							break;
						}
					}
				printf("\n");
				break;
			case 0x0008:
				break;
			case 0x0009:
				break;
			case 0x000a:
				break;
			case 0x000b:
				break;
			case 0x000c:
				break;
			default:
				continue;
		}
	}
	return 0;
}


/* ANALYZE CHUNK TYPE */
/*-----------------------------------------------------------------------------*/
/* DEFAULT CASE: AFTER JPG HEADER, STANDALONE, OR STANDARD CHUNK */
int analyzeChunkType1(FILE *f){
	/* keep looping and fgetc until 0xff is found (firstbyte of marker) */
	unsigned char firstMarkByte;
	while(!feof(f)){
		firstMarkByte = fgetc(f);
		if (firstMarkByte == 0xff){
			break;
		}
	}
	/* makesure 0xff is not the last byte of JPG file with nothing after it */
	unsigned char secondMarkByte;
	// if((secondMarkByte = fgetc(f)) == EOF){
	// 	return -1;
	// }
	secondMarkByte = fgetc(f);

	/* Now we have marker, check what type it is (standalone, standard or super) */
	switch(secondMarkByte){
		case 0xd0:
		case 0xd1:
		case 0xd2:
		case 0xd3:
		case 0xd4: // these all are standalone chunks
		case 0xd5:
		case 0xd6:
		case 0xd7:
		case 0xd8:
		case 0xd9:
			return 0; // standalone chunks dont do anything!
			break;
		case 0xda: // super chunk
			return analyzeSuperChunk(f);
			break;
		default: // everything else is a standard chunk
			return analyzeStandardChunk(f, secondMarkByte);
			break;
	}	
}

/* SPECIAL CASE: WHEN PREVIOUS CHUNK WAS SUPERCHUNK! */
int analyzeChunkType2(FILE *f, unsigned char firstMarkByte, unsigned char secondMarkByte){

	/* Now we have marker, check what type it is (standalone, standard or super) */
	switch(secondMarkByte){
		case 0xd0:
		case 0xd1:
		case 0xd2:
		case 0xd3:
		case 0xd4: //these all are standalone chunks
		case 0xd5:
		case 0xd6:
		case 0xd7:
		case 0xd8:
		case 0xd9:
			return 0; //standalone chunks dont do anything!
			break;
		case 0xda: //super chunk
			return analyzeSuperChunk(f);
			break;
		default: //everything else is a standard chunk
			return analyzeStandardChunk(f, secondMarkByte);
			break;
	}	
}


/* MAIN CALLING FUNCTION */
/*--------------------------------------------------------------------------*/
int validateJPG(FILE *f){
	unsigned char jpg_header[2] = {0xff, 0xd8};
	unsigned char header[2];
	fread(header, 1, 2, f);
	int i;
	for(i = 0; i < 2; i++){
		if(header[i] != jpg_header[i]){
			return -1;
		}
	}
	return 0;
}

int analyze_jpg(FILE *f) {
    /* YOU WRITE THIS PART */
    if(validateJPG(f) < 0) {
 	   return -1;
 	}
	while (!feof(f)) {
		analyzeChunkType1(f);
	}
	return 0;
}
