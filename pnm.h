#ifndef PNM_H
#define PNM_H

typedef struct{
	unsigned char* data;
	unsigned int width, height;
	unsigned int type;
	unsigned int maxValue;
}PNM_Image;

enum PNM_Image_type{
	GRAYSCALE_IMAGE = 1,
	COLOR_IMAGE = 3
};

PNM_Image* readPPM(const char* filename);
int writePPM(const char *filename, PNM_Image *img);
void freePNM_Image(PNM_Image** img);

#endif
