#ifndef XFORM_GAIN_H
#define XFORM_GAIN_H

#include "pnm.h"

typedef struct{
	unsigned char* data;
	unsigned int width, height;
}Xform_gain;

Xform_gain* readXformGain(const char* filename, unsigned int width, unsigned int height);
int writeXformGain(const char* filename, Xform_gain* gain);
void freeXformGain(Xform_gain** gain);
PNM_Image* xformGainApply(Xform_gain* gain, unsigned char offset, double gainFactor, PNM_Image* img);

#endif
