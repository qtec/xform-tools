#include "xform_gain.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

Xform_gain* readXformGain(const char* filename, unsigned int width, unsigned int height)
{
	//open PPM file for reading
	FILE *fp;

	fp = fopen(filename, "rb");
	if (!fp) {
		printf("Unable to open file '%s'\n", filename);
		return NULL;
	}

	//get file size to setermine nr of channels
	fseek(fp, 0, SEEK_END); // seek to end of file
	unsigned long int length = ftell(fp); // get current file pointer
	//printf("the file's length is %u Bytes\n",length);
	fseek(fp, 0, SEEK_SET);

	if(length != width*height){
		printf("Wrong file size in '%s', size: %lu, expected: %u x %u = %lu\n",
				filename, length, width, height, (unsigned long int)width*height);
		fclose(fp);
		return NULL;
	}

	//allocate mem for file
	Xform_gain *gain = malloc(sizeof(Xform_gain));
	if(gain == NULL){
		printf("Error, could not allocate memory\n");
		fclose(fp);
		return NULL;
	}

	gain->width = width;
	gain->height = height;

	gain->data = malloc(sizeof(unsigned char)*(width*height));
	if(gain->data == NULL){
		printf("Error, could not allocate memory\n");
		fclose(fp);
		free(gain);
		return NULL;
	}

	//read pixel data from file
	if (fread(gain->data, width, height, fp) != height) {
		printf("Error loading image '%s'\n", filename);
		fclose(fp);
		free(gain->data);
		free(gain);
		return NULL;
	}

	fclose(fp);

	return gain;
}

int writeXformGain(const char* filename, Xform_gain* gain)
{
	FILE* out_file = fopen(filename, "wb");
	if(out_file == NULL){
		printf("Error, could not open output file: %s\n", filename);
		return -1;
	}

	fwrite(gain->data, gain->width*gain->height, 1, out_file);

	fclose(out_file);

	return 0;
}

void freeXformGain(Xform_gain** gain)
{
	if(*gain != NULL){
		if((*gain)->data != NULL){
			free((*gain)->data);
			(*gain)->data = NULL;
		}

		free(*gain);
		*gain = NULL;
	}
}

PNM_Image* xformGainApply(Xform_gain* gain, unsigned char offset, double gainFactor, PNM_Image* img)
{
	int i,j,k,index;

	if(img->width != gain->width || img->height != gain->height){
		printf("Error input image dimensions (%d x %d) and xform gain dimensions (%d x %d) don't match\n",
				img->width, img->height, gain->width, gain->height);
		return NULL;
	}

	//allocate mem for img
	PNM_Image *out_img = malloc(sizeof(PNM_Image));
	if(out_img == NULL){
		printf("Error, could not allocate memory\n");
		return NULL;
	}

	out_img->width = img->width;
	out_img->height = img->height;
	out_img->maxValue = img->maxValue;
	out_img->type = img->type;

	out_img->data = malloc(sizeof(unsigned char)*(out_img->width*out_img->height*out_img->type));
	if(out_img->data == NULL){
		printf("Error, could not allocate memory\n");
		free(out_img);
		return NULL;
	}

	double tmp;
	double gain_val;
	for(j=0; j<img->height; j++){
		for (i=0; i<img->width; i++){
			index = (i+j*img->width)*img->type;
			//apply xform
			gain_val = ((double)gain->data[i+j*img->width] + offset)*gainFactor/255.0;
			if(img->type == GRAYSCALE_IMAGE){
				tmp = (double)img->data[index]*gain_val;
				if(tmp > 255) tmp = 255;
				out_img->data[index] = round(tmp);
			}else{
				for(k=0; k<img->type; k++){
					tmp = (double)img->data[index+k]*gain_val;
					if(tmp > 255) tmp = 255;
					out_img->data[index+k] = round(tmp);
				}
			}
		}
	}

	return out_img;
}
