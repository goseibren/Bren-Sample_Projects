#include <emmintrin.h>
#define KERNX 3 //this is the x-size of the kernel. It will always be odd.
#define KERNY 3 //this is the y-size of the kernel. It will always be odd.

int conv2D(float* in, float* out, int data_size_X, int data_size_Y,
                    float* kernel)
{
    
	// localize the kernel
	
	float localKern[KERNX*KERNY];
	for (int i = 0; i < KERNX*KERNY; i++){
		localKern[i] = kernel[i];
	}
	
    // the x coordinate of the kernel's center
    int kern_cent_X = (KERNX - 1)/2;
    // the y coordinate of the kernel's center
    int kern_cent_Y = (KERNY - 1)/2;

    int x_pad = data_size_X + 2;
    int y_pad = data_size_Y + 2;
    float in_pad[x_pad * y_pad];
    memset(in_pad, 0, x_pad * y_pad * sizeof(float));
    for (int k = 1; k < y_pad - 1; k++) {
        // copy original matrix into padded matrix
        memcpy( &in_pad[k * x_pad + 1], &in[(k-1) * data_size_X], sizeof(float) * data_size_X);
   	} 

    // main convolution loop
	for(int y = 0; y < data_size_Y; y++){ // the x coordinate of the output location we're focusing on
		for(int x = 0; x < (data_size_X/4)*4; x += 4){ // the y coordinate of theoutput location we're focusing on
			__m128 result = _mm_setzero_ps();
			for(int i = -kern_cent_Y; i <= kern_cent_Y; i++){ // kernel unflipped y coordinate
				for(int j = -kern_cent_X; j <= kern_cent_X; j++){ // kernel unflipped x coordinate
					float curr_kern = localKern[(kern_cent_X - j) + (kern_cent_Y - i) * KERNX];
					__m128 new_kern = _mm_set1_ps(curr_kern);
					__m128 input = _mm_loadu_ps(in_pad + (x + kern_cent_X + j) + (y + kern_cent_Y +i) * x_pad);
					result = _mm_add_ps(result, _mm_mul_ps(input, new_kern));
				}
			}
			_mm_storeu_ps(out + (x + (y * data_size_X)), result);
		}
		
		for(int x = (data_size_X/4)*4; x < data_size_X; x++){
			float output = out[x + (y * data_size_X)];
			for(int i = -kern_cent_Y; i <= kern_cent_Y; i++){
				for(int j = -kern_cent_X; j <= kern_cent_X; j++){
					output += in_pad[(x + kern_cent_X + j) + (y + kern_cent_Y + i) * x_pad] * localKern[(kern_cent_X-j) + (kern_cent_Y-i) * KERNX];						
				}
			}
			out[x + (y * data_size_X)] += output;
		}	
	}

	return 1;
}

