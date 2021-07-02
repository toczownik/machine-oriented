/*
 * kontraste.c
 *
 *  Created on: 20.04.2021
 *      Author: angela
 */

// disable warnings in Visual Studio for file i/o
#ifdef _MSC_VER
#define _CRT_SECURE_NO_WARNINGS
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <immintrin.h>
#include <time.h>

#define TIMING_ITERS 1000

unsigned char * readPGM(const char * sourcefile, int * width, int * height);
int 			writePGM(const char * filepath, unsigned char * image, int width, int height);

double kontrast1D_scalar(const unsigned char* image, int width, int height, const char * path);
double kontrast1D_simd(const unsigned char* image, int width, int height, const char * path);

int main(void) {

	const char source [] = "Testbilder/papageien.pgm";

	const char target_scalar[] = "Ergebnisse/kontrast1D_scalar.pgm";
	const char target_simd  [] = "Ergebnisse/kontrast1D_simd.pgm";

	// execution times
	double t_scalar, t_simd;

	// init values
	unsigned char * image = NULL;
	int width, height;

	// read file
	image = readPGM(source, &width, &height);
	if (image == NULL){
		return 1;
	}

	t_scalar = kontrast1D_scalar(image, width, height, target_scalar);

	t_simd   = kontrast1D_simd(image, width, height, target_simd);

	printf("\n");
	printf("Image filter execution times:\n");
	printf("t_scalar: \t%f\n", t_scalar);
	printf("t_simd: \t%f\n", t_simd);
	printf("Speedup: \t%f\n", t_scalar/t_simd);

	printf("\nEnd of test\n");

	return 0;
}


double kontrast1D_scalar(const unsigned char* image, int width, int height, const char * path){


		unsigned char * newImg = malloc(width * height * sizeof(char));

		if (newImg == NULL){
				printf("Failure creating a results buffer!\n");
				return -1;
		}

		// messen der Ausfuehrungszeit
		clock_t begin = clock();

		for (int i = 0; i < TIMING_ITERS; i ++){

			short newPixel;

		  newPixel = (3) * image[0] + (-1) * image[1];

		  newPixel = abs(newPixel);
		  newPixel = newPixel > 0xFF ? 0xFF : newPixel;
		  newImg[0] = (unsigned char)newPixel;

		  #pragma loop(no_vector)
		  for (int j = 1; j < width * height-1; j++) {

		      newPixel = (-1) * image[j - 1] + (3) * image[j] + (-1) * image[j + 1];

		      newPixel = abs(newPixel);
		      newPixel = newPixel > 0xFF ? 0xFF : newPixel;
		      newImg[j] = (unsigned char)newPixel;
		  }
		  newPixel = (-1) * image[i] + (3) * image[i+1];
      newPixel = abs(newPixel);
      newPixel = newPixel > 0xFF ? 0xFF : newPixel;
      newImg[i+1] = (unsigned char)newPixel;
		} // eof timing

		clock_t end = clock();
		double time = (double) (end - begin) / CLOCKS_PER_SEC;

		// write file
		writePGM(path, newImg, width, height);

		return time;

}

double kontrast1D_simd(const unsigned char* image, int width, int height, const char * path){

	unsigned char * newImg = (unsigned char *) malloc(width * height * sizeof(char));
		if (newImg == NULL){
			printf("Failure creating a results buffer!\n");
			return -1;
	}
	const __m256i three = _mm256_set1_epi32(3);
	clock_t begin = clock();
	__m256i current_pixel;
	__m256i second_pixel;
	for(int i = 0; i < TIMING_ITERS; ++i) {
		current_pixel = _mm256_lddqu_si256((const __m256i*) &image[0]);
		current_pixel = _mm256_mul_epi32(current_pixel, three);
		second_pixel = _mm256_lddqu_si256((const __m256i*) &image[1]);
		current_pixel = _mm256_sub_epi32(current_pixel, second_pixel);
		_mm256_storeu_si256((__m256i*) &newImg[0], current_pixel);

		for(int j = 1; j < width * height-1; ++j) {
			current_pixel = _mm256_lddqu_si256((const __m256i*) &image[j]);
			current_pixel = _mm256_mul_epi32(current_pixel, three);
			second_pixel = _mm256_lddqu_si256((const __m256i*) &image[j-1]);
			current_pixel = _mm256_sub_epi32(current_pixel, second_pixel);
			second_pixel = _mm256_lddqu_si256((const __m256i*) &image[j+1]);
			current_pixel = _mm256_sub_epi32(current_pixel, second_pixel);
			_mm256_storeu_si256((__m256i*) &newImg[j], current_pixel);
		}

		current_pixel = _mm256_lddqu_si256((const __m256i*) &image[i]);
		current_pixel = _mm256_mul_epi32(current_pixel, three);
		second_pixel = _mm256_lddqu_si256((const __m256i*) &image[i-1]);
		current_pixel = _mm256_sub_epi32(current_pixel, second_pixel);
		_mm256_storeu_si256((__m256i*) &newImg[i], current_pixel);
	}
	
	clock_t end = clock();
	double time = (double) (end - begin) / CLOCKS_PER_SEC;

	writePGM(path, newImg, width, height);
	
	free(newImg);

	return time;
}



// *****
// functions for file i/o
// *****

unsigned char * readPGM(const char * path, int * width, int * height){

	printf("Reading file: %s \n", path);
	FILE *imgFile = fopen(path, "rb");
	if (imgFile == NULL){
		printf("Unable to open file!\n");
		return NULL;
	}

	char lineBuff[255];

	// read header
	// check if picture is grayscale or color
	int factor = 1;
	fgets(lineBuff, 255, imgFile);
	if (strcmp(lineBuff,"P6")){
		// colored picture, 3 values per pixel (RGB)
		factor = 3;
	}
	else if (strcmp(lineBuff, "P5")){
		factor = 1;
	}
	else {
		printf("Wrong file format! Expected P5 or P6, got %s\n", lineBuff);
		return NULL;
	}

	fgets(lineBuff, 255, imgFile);
	// remove comments from file, if needed
	while (lineBuff[0]== '#'){
		fgets(lineBuff, 255, imgFile);
	}
	sscanf(lineBuff, "%d %d", width, height);
	printf("width: %d, height: %d\n", *width, *height);

	fgets(lineBuff, 255, imgFile);
	if (!strcmp(lineBuff, "255")){
		printf("Wrong file format! Expected 8-b pixel value (max. 255), got %s\n", lineBuff);
		return NULL;
	}

	// now, we can allocate memory and assign it to pointer passed by argument
	int picSize = (*width) * (*height) * factor;
	unsigned char * img = (unsigned char*) malloc(picSize * sizeof(char));

	if (img == NULL){
		printf("Memory Allocation failed!\n");
		return NULL;
	}

	for (int i = 0; i < picSize; i++){
		img[i] = fgetc(imgFile);
	}

	fclose(imgFile);

	return img;
}

int writePGM (const char * path, unsigned char * image, int width, int height){

	printf("Writing results to %s\n", path);

	FILE *imgFile = fopen(path, "wb");

	if (imgFile == NULL){
		printf("Unable to open file!\n");
		return 1;
	}

	fputs("P5\n", imgFile);

	fputs("#file generated by apo code \n", imgFile);
	// create string for "width height" line
	fprintf(imgFile, "%d %d\n255\n", width, height);

	for (int i = 0; i < width * height; ++i){
			fputc(image[i], imgFile);
	}

	return fclose(imgFile);

}
