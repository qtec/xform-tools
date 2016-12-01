#include "pnm.h"

#include <stdio.h>
#include <stdlib.h>

#define RGB_COMPONENT_COLOR 255

PNM_Image* readPPM(const char* filename)
{
	//open PPM file for reading
	char buff[3];
	FILE *fp;
	int c;
	unsigned int rgb_comp_color,width,height;

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

	//read image format
	if (!fgets(buff, sizeof(buff), fp)) {
		perror(filename);
		fclose(fp);
		return NULL;
	}
	//printf("%s\n",buff);

	//check the image format
	if (buff[0] != 'P' || (buff[1] != '6' && buff[1] != '5') ) {
		printf("Invalid image format (must be a 'P5' or 'P6' PPM)\n");
		fclose(fp);
		return NULL;
	}

	//check for comments
	c = getc(fp);
	while (c == '#' || c == '\n') {
		while (getc(fp) != '\n') ;
		c = getc(fp);
	}
	ungetc(c, fp);

	//read image size information
	if (fscanf(fp, "%d %d", &width, &height) != 2) {
		printf("Invalid image size (error loading '%s')\n", filename);
		fclose(fp);
		return NULL;
	}

	//read rgb component
	if (fscanf(fp, "%d", &rgb_comp_color) != 1) {
		printf("Invalid rgb component (error loading '%s')\n", filename);
		fclose(fp);
		return NULL;
	}

	//check rgb component depth
	if (rgb_comp_color!= RGB_COMPONENT_COLOR) {
		printf("'%s' must have 8-bit components\n", filename);
		fclose(fp);
		return NULL;
	}

	int channels=length/(width*height);
	//printf("Nr channels: %d\n", channels);

	//allocate mem for img
	PNM_Image *img = malloc(sizeof(PNM_Image));
	if(img == NULL){
		printf("Error, could not allocate memory\n");
		fclose(fp);
		return NULL;
	}

	img->width = width;
	img->height = height;
	img->maxValue = rgb_comp_color;
	img->type = (channels == 1)?GRAYSCALE_IMAGE:COLOR_IMAGE;

	img->data = malloc(sizeof(unsigned char)*(width*height*channels));
	if(img->data == NULL){
		printf("Error, could not allocate memory\n");
		fclose(fp);
		free(img);
		return NULL;
	}

	//have to skip first byte
	c = getc(fp);

	//read pixel data from file
	if (fread(img->data, channels*width, height, fp) != height) {
		printf("Error loading image '%s'\n", filename);
		fclose(fp);
		free(img->data);
		free(img);
		return NULL;
	}

	fclose(fp);

	return img;
}

//saves img in RGB sequence
int writePPM(const char *filename, PNM_Image *img)
{
	FILE *fp;
	//open file for output
	fp = fopen(filename, "wb");
	if (!fp) {
		printf("Unable to open file '%s'\n", filename);
		return 1;
	}

	////////////write the header file

	//image format
	if(img->type == GRAYSCALE_IMAGE){
		fprintf(fp, "P5 ");
	}else{
		fprintf(fp, "P6 ");
	}

	//image size
	fprintf(fp, "%d %d ",img->width,img->height);

	// rgb component depth -> 8 bits only
	fprintf(fp, "%d\n", RGB_COMPONENT_COLOR);

	if(img->type == GRAYSCALE_IMAGE)
		fwrite(img->data, img->width * img->height, 1, fp);
	else
		fwrite(img->data, img->width * img->height * 3, 1, fp);

	fclose(fp);

	return 0;
}

void freePNM_Image(PNM_Image** img)
{
	if(*img != NULL){
		if((*img)->data != NULL){
			free((*img)->data);
			(*img)->data = NULL;
		}

		free(*img);
		*img = NULL;
	}
}
