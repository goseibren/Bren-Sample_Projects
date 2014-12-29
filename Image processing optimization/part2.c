#include <omp.h>
#include <emmintrin.h>
#define KERNX 3 //this is the x-size of the kernel. It will always be odd.
#define KERNY 3 //this is the y-size of the kernel. It will always be odd.
#define kern_cent_X 1
#define kern_cent_Y 1

int conv2D(float* in, float* out, int data_size_X, int data_size_Y,
                    float* kernel)
{
    
	// localize the kernel
	
	float localKern[KERNX*KERNY];
	#pragma omp parallel
	{
		omp_set_num_threads(8);
		#pragma omp for
	for (int i = 0; i < KERNX*KERNY; i++){
		localKern[i] = kernel[i];
	}
	}
	
    // the x coordinate of the kernel's center
    //int kern_cent_X = (KERNX - 1)/2;
    // the y coordinate of the kernel's center
    //int kern_cent_Y = (KERNY - 1)/2;

    int x_pad = data_size_X + 2;
    int y_pad = data_size_Y + 2;
    float in_pad[x_pad * y_pad];
    memset(in_pad, 0, x_pad * y_pad * sizeof(float));
    __m128 result, new_kern, input;
    float output;

    #pragma omp parallel
    {
    	omp_set_num_threads(8);
    	#pragma omp for
    for (int y = 0; y < data_size_Y; y++){
        for(int x = 1; x < x_pad - 1; x++) {
            in_pad[x+(y+1)*x_pad]=in[(x-1)+y*data_size_X];
        }
    }
	}

   	

    // main convolution loop
 	#pragma omp parallel
 	{
 		omp_set_num_threads(8);
 		#pragma omp for private(result, new_kern, input)
	for(int y = 0; y < data_size_Y; y++){ // the x coordinate of the output location we're focusing on 
		for(int x = 0; x < (data_size_X/4)*4; x += 4){ // the y coordinate of theoutput location we're focusing on
			result = _mm_setzero_ps();
			for(int i = -kern_cent_Y; i <= kern_cent_Y; i++){ // kernel unflipped y coordinate
				for(int j = -kern_cent_X; j <= kern_cent_X; j++){ // kernel unflipped x coordinate
					//float curr_kern = localKern[(kern_cent_X - j) + (kern_cent_Y - i) * KERNX];
					new_kern = _mm_set1_ps(localKern[(kern_cent_X - j) + (kern_cent_Y - i) * KERNX]);
					input = _mm_loadu_ps(in_pad + (x + kern_cent_X + j) + (y + kern_cent_Y +i) * x_pad);
					result = _mm_add_ps(result, _mm_mul_ps(input, new_kern));
				}
			}
			_mm_storeu_ps(out + (x + (y * data_size_X)), result);
		}
	}
	w}
		#pragma omp parallel
	{
		omp_set_num_threads(8);
		#pragma omp for private(output)
		for(int x = (data_size_X/4)*4; x < data_size_X; x++){
			for(int y = 0; y < data_size_Y; y++){
			output = out[x + (y * data_size_X)];
			for(int i = -kern_cent_Y; i <= kern_cent_Y; i++){
				for(int j = -kern_cent_X; j <= kern_cent_X; j++){
					output += in_pad[(x + kern_cent_X + j) + (y + kern_cent_Y + i) * x_pad] * localKern[(kern_cent_X-j) + (kern_cent_Y-i) * KERNX];						
				}
			}
			out[x + (y * data_size_X)] += output;
		}	
	}
	}

	return 1;
}

