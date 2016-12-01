#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <unistd.h>

#include "xform_gain.h"
#include "pnm.h"

/*Simple program for generating a xform gain map from an image.
Looks for max and min values on the reference image an sets the gain so that the min has gain 1 and the rest of pixels get the same intensity as min.*/

uint16_t getGrayVal(PNM_Image* img, unsigned int index, unsigned char little_endian)
{
	uint16_t val;

	if(img->maxValue <= 255){
		if(img->type == 1)
			val = (uint8_t)img->data[index+0];
		else
			val = (uint8_t)round(img->data[index+0]*0.299 + img->data[index+1]*0.587 + img->data[index+2]*0.114);
	}else{
		if(!little_endian){
			if(img->type == 1) //Y16_BE
				val = (uint16_t)(img->data[index+0]<<8) + img->data[index+1];
			else
				val = (uint16_t)round(((uint16_t)(img->data[index+0]<<8) + img->data[index+1])*0.299 +
									  ((uint16_t)(img->data[index+2]<<8) + img->data[index+3])*0.587 +
									  ((uint16_t)(img->data[index+4]<<8) + img->data[index+5])*0.114);
		}else{
			if(img->type == 1) //Y16
				val = (uint16_t)(img->data[index+1]<<8) + img->data[index+0];
			else
				val = (uint16_t)round(((uint16_t)(img->data[index+1]<<8) + img->data[index+0])*0.299 +
									  ((uint16_t)(img->data[index+3]<<8) + img->data[index+2])*0.587 +
									  ((uint16_t)(img->data[index+5]<<8) + img->data[index+4])*0.114);
		}
	}

	return val;
}

void setGrayVal(PNM_Image* img, unsigned int index, uint16_t val, unsigned char little_endian)
{
	if(img->maxValue <= 255){
		if(img->type == 1)
			img->data[index+0] = (uint8_t)val;
		else{
			for(int k=0; k< img->type; k++)
				img->data[index+k] = (uint8_t)val;
		}
	}else{
		if(!little_endian){//Y16_BE
			if(img->type == 1){
				img->data[index+0] = (uint8_t)(((uint16_t)val>>8) & 0xff);
				img->data[index+1] = (uint8_t)(((uint16_t)val) & 0xff);
			}else{
				for(int k=0; k< img->type*2; k+=2){
					img->data[index+k+0] = (uint8_t)(((uint16_t)val>>8) & 0xff);
					img->data[index+k+1] = (uint8_t)(((uint16_t)val) & 0xff);
				}
			}
		}else{//Y16
			if(img->type == 1){
				img->data[index+1] = (uint8_t)(((uint16_t)val>>8) & 0xff);
				img->data[index+0] = (uint8_t)(((uint16_t)val) & 0xff);
			}else{
				for(int k=0; k< img->type*2; k+=2){
					img->data[index+k+1] = (uint8_t)(((uint16_t)val>>8) & 0xff);
					img->data[index+k+0] = (uint8_t)(((uint16_t)val) & 0xff);
				}
			}
		}
	}
}

PNM_Image* imgNormalize(PNM_Image* img, unsigned char little_endian)
{
	uint16_t satVal = 255;
	if(img->maxValue == 2) satVal = 65535;

	//get min and max
	uint16_t minVal = satVal;
	uint16_t maxVal = 0;

	//absolute min and max
	uint16_t val;
	int i,j;
	unsigned int index;
	for(j=0;j<img->height;j++){
		for (i=0;i<img->width;i++){
			index = (i+j*img->width)*img->type;
			val = getGrayVal(img, index, little_endian);
			if(val < minVal)
				minVal = val;
			if(val > maxVal)
				maxVal = val;
		}
	}
	//guarantee that min < 255 and max > 0
	if(minVal >= satVal) minVal = satVal-1;
	if(maxVal <= 0) maxVal = 1;

	//normalize
	uint16_t newMin = 0;
	uint16_t newMax = satVal;
	double scale = (newMax - newMin)/(double)(maxVal - minVal);
	//printf("imgNormalize %d min:%d max:%d scale:%f\n", flags, minVal, maxVal, scale);

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

	//guarantee that max != min
	if(minVal != maxVal){
		double v;
		for(j=0;j<img->height;j++){
			for (i=0;i<img->width;i++){
				index = (i+j*img->width)*img->type;
				val = getGrayVal(img, index, little_endian);

				v = (val - minVal)*scale + newMin;
				if(v < 0) v = 0;
				if(v > satVal) v = satVal;

				setGrayVal(out_img, index, v, little_endian);
			}
		}
	}

	return out_img;
}

void calcImgStats(PNM_Image* img, unsigned char* min, unsigned char* max, double* avg, double* stddev)
{
	int i,j,index;
	unsigned char ref_val;

	*min=255;
	*max=0;
	*avg = 0;
	*stddev = 0;

	for(j=0;j<img->height;j++){
		for (i=0;i<img->width;i++){
			index = (i+j*img->width)*img->type;
			if(img->type == GRAYSCALE_IMAGE){
				ref_val = img->data[index];
			}else{
				// Y = 0.299*R + 0.587*G + 0.114*B
				ref_val = round(img->data[index+0]*0.299 + img->data[index+1]*0.587 + img->data[index+2]*0.114);
			}
			if(ref_val < *min){
				*min = ref_val;
			}
			if(ref_val > *max){
				*max = ref_val;
			}
			*avg += ref_val;
		}
	}
	*avg /= img->width*img->height;
	for(j=0;j<img->height;j++){
		for (i=0;i<img->width;i++){
			index = (i+j*img->width)*img->type;
			if(img->type == GRAYSCALE_IMAGE){
				ref_val = img->data[index];
			}else{
				// Y = 0.299*R + 0.587*G + 0.114*B
				ref_val = round(img->data[index+0]*0.299 + img->data[index+1]*0.587 + img->data[index+2]*0.114);
			}
			*stddev += pow(ref_val - *avg, 2);
		}
	}
	*stddev = sqrt( *stddev / (img->width*img->height) );
}

#define GAIN_FACTOR_RAW 16384
#define GAIN_FACTOR_MIN ((double)1/GAIN_FACTOR_RAW)
#define GAIN_FACTOR_MAX ((double)262140/GAIN_FACTOR_RAW)

#define GAIN_OFFSET_RAW 255
#define GAIN_OFFSET_MIN 0
#define GAIN_OFFSET_MAX 255

void use(char *name)
{
	fprintf(stderr,"%s [-o offset] [-s scale] ref_img out_file [result_img]\n"
					"The -o option enables specifying the gain offset[raw] (%d to %d).\n"
					"Not using this option will cause the program to automatically select the best value.\n"
					"The -s option enables specifying the gain scale[x] (%f to %f). \n"
					"Not using this option will cause the program to automatically select the best value.\n"
					"Note that it scales the offset as well\n",
					name, GAIN_OFFSET_MIN, GAIN_OFFSET_MAX, GAIN_FACTOR_MIN, GAIN_FACTOR_MAX);
	return;
}


//TODO: maybe start by bluring image

int main(int argc ,char *argv[])
{
	int i,j,index;
	unsigned int offset = 0;
	unsigned char ref_val;
	unsigned char calc_offset = 1;
	//unsigned char decimal_bits = 8;
	//unsigned char gainFactor = pow(2, 8 - decimal_bits);
	double gainFactor = 1.0;
	unsigned char calc_scale = 1;

	//printf("Decimal format: %d.%d Max gain: %dx\n", 8 - decimal_bits, decimal_bits,gainFactor);

	if (argc < 3 || argc > 8){
		use(argv[0]);
		return -1;
	}

	int opt;
	while ((opt = getopt(argc, argv, "o:s:")) != -1) {
		switch (opt) {
			case 'o':
				offset = atoi(optarg);
				if(offset < GAIN_OFFSET_MIN || offset > GAIN_OFFSET_MAX){
					printf("Unsupported gain map offset. Min:%d Max:%d\n", GAIN_OFFSET_MIN, GAIN_OFFSET_MAX);
					exit(EXIT_FAILURE);
				}
				calc_offset = 0;
				break;
			case 's':
				gainFactor = atof(optarg);
				if(gainFactor < GAIN_FACTOR_MIN || gainFactor > GAIN_FACTOR_MAX){
					printf("Unsupported gain map scale. Min:%f Max:%f\n", GAIN_FACTOR_MIN, GAIN_FACTOR_MAX);
					exit(EXIT_FAILURE);
				}
				calc_scale = 0;
				break;
			default:
				use(argv[0]);
				exit(EXIT_FAILURE);
		}
	}

	if (optind >= argc) {
		fprintf(stderr, "Expected argument after options\n");
		exit(EXIT_FAILURE);
	}

	PNM_Image* ref_img = readPPM(argv[optind]);
	if(ref_img == NULL){
		printf("Error, could not open reference image: %s\n", argv[optind]);
		return -1;
	}

	//normalize input image
	PNM_Image* norm = imgNormalize(ref_img, 0);
	char text[256];
	snprintf(text, 256, "%s_norm.ppm", argv[optind]);
	writePPM(text, norm);
	freePNM_Image(&norm);

	//find min and max
	unsigned char min, max;
	double avg, stddev;
	calcImgStats(ref_img, &min, &max, &avg, &stddev);
	printf("Input img: min=%d max=%d ratio=%f avg=%f stddev=%f\n", min, max, (double)max/min, avg, stddev);

	//test for min!=0
	if(min == 0){
		min = 1;
	}
	if(max <= min){
		max = min + 1;
	}
	if(max/min > 10){
		printf("Input image too un-uniform, limiting correction to 10x\n");
		max = 255;
		min = max/10;
	}

	if(calc_scale){
		if(calc_offset)
			gainFactor = (double)max/min - 1;
		else
			gainFactor = (double)max/min - offset/255.0;
	}

	printf("Gain Map scale: %fx (raw=%lu)\n", gainFactor, (unsigned long)(gainFactor*16384));

	if(calc_offset){
		double temp = (double)max/min - 1;
		if(temp > gainFactor)
			offset = round(255.0*gainFactor/temp);
		else if(temp+1 <= gainFactor)
			offset = 0;
		else
			offset = round(255.0*(temp+1 - gainFactor));
	}

	printf("Gain Map offset: %d (%f) -> scaled: %d (%f)\n", offset, offset/255.0, (unsigned char)(offset/gainFactor), (offset/gainFactor)/255.0);

	Xform_gain gain = {NULL, ref_img->width, ref_img->height};
	gain.data = malloc(sizeof(unsigned char)*(gain.width*gain.height));
	if(gain.data == NULL){
		printf("Error, could not allocate output data\n");
		freePNM_Image(&ref_img);
		return -1;
	}

	//don't gain max value over 1x
	double m = gainFactor;
	if(gainFactor*((double)min/max) > 1){
		m = (double)max/min;
		printf("max value gain: %f corrected gainFactor: %f corrected gain: %f\n",
					gainFactor*((double)min/max), m, m*((double)min/max));
	}

	double gainMin = ((offset/255.0 + m)*min - (offset/255.0)*min)/min;
	double gainMax = ((offset/255.0 + m)*min - (offset/255.0)*max)/max;
	printf("gain: min=%fx -> (%f + %f)*%f (%d + %d)*%lu max=%fx -> (%f + %f)*%f (%d + %d)*%lu\n",
			(gainMin/gainFactor+(offset/gainFactor)/255.0)*gainFactor,
			round(255.0*(gainMin/gainFactor))/255.0, (offset/gainFactor)/255.0, gainFactor,
			(unsigned char)round(255.0*(gainMin/gainFactor)), (unsigned char)(offset/gainFactor), (unsigned long)gainFactor*GAIN_FACTOR_RAW,
			(gainMax/gainFactor+(offset/gainFactor)/255.0)*gainFactor,
			round(255.0*(gainMax/gainFactor))/255.0, (offset/gainFactor)/255.0, gainFactor,
			(unsigned char)round(255.0*(gainMax/gainFactor)), (unsigned char)(offset/gainFactor), (unsigned long)gainFactor*GAIN_FACTOR_RAW);

	double gain_val;
	for(j=0;j<ref_img->height;j++){
		for (i=0;i<ref_img->width;i++){
			index = (i+j*ref_img->width)*ref_img->type;
			if(ref_img->type == GRAYSCALE_IMAGE){
				ref_val = ref_img->data[index];
			}else{
				// Y = 0.299*R + 0.587*G + 0.114*B
				ref_val = round(ref_img->data[index+0]*0.299 + ref_img->data[index+1]*0.587 + ref_img->data[index+2]*0.114);
			}
			gain_val = ((offset/255.0 + m)*min - (offset/255.0)*ref_val)/ref_val;
			gain.data[i+j*ref_img->width] = round(255.0*(gain_val/gainFactor));
		}
	}

	writeXformGain(argv[optind+1], &gain);

	//write xform visualization (also normalized)
	PNM_Image* xform_vis = malloc(sizeof(PNM_Image));
	if(xform_vis == NULL){
		printf("Error, could not allocate memory for xform_vis\n");
	}else{
		xform_vis->width = gain.width;
		xform_vis->height = gain.height;
		xform_vis->maxValue = 255;
		xform_vis->type = GRAYSCALE_IMAGE;
		xform_vis->data = gain.data;

		snprintf(text, 256, "%s_vis.ppm", argv[optind+1]);
		writePPM(text, xform_vis);

		norm = imgNormalize(xform_vis, 0);
		free(xform_vis);

		snprintf(text, 256, "%s_vis_norm.ppm", argv[optind+1]);
		writePPM(text, norm);
		freePNM_Image(&norm);
	}

	if(optind+2 < argc){
		PNM_Image* result = xformGainApply(&gain, offset/gainFactor, gainFactor, ref_img);

		//check uniformity
		unsigned char min, max;
		double avg, stddev;
		calcImgStats(result, &min, &max, &avg, &stddev);
		printf("Result img: min=%d max=%d ratio=%f avg=%f stddev=%f\n", min, max, (double)max/min, avg, stddev);

		PNM_Image* norm = imgNormalize(result, 0);

		char text[256];
		snprintf(text, 256, "%s.ppm", argv[optind+2]);
		writePPM(text, result);
		freePNM_Image(&result);

		snprintf(text, 256, "%s_norm.ppm", argv[optind+2]);
		writePPM(text, norm);
		freePNM_Image(&norm);
	}

	freePNM_Image(&ref_img);
	free(gain.data);

	return 0;
}
