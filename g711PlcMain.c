#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arch.h"
#include "utility.h"
#include "wave.h"
#include "wave_type.h"
#include "param.h"
#include "g711PlcMain.h"
#include "LowcFE.h"

// reference:
// https://www.voiptroubleshooter.com/open_speech/chinese.html (open speech repository)
// http://www.voiceover-samples.com/languages/chinese-voiceover/ (voice over samples)

/*-------------------- GLOBAL PARAMETER --------------------*/
LowcFE_c lc;
sint16_t *pPlc16bBuf;
sint16_t *pPlc16bOutput;
bool *pPlcLostRec;
uint32_t g711_frame_num;

/*-------------------- INTERNAL FUNCTIONS --------------------*/
void g711DataLost(void) {

	uint32_t i, j, k;
	uint32_t initialFrame = 4;  // Manual_lost_start_sample;
	uint32_t lostFrameNum = 3;  // Manual_lost_sample_ratio;
	uint32_t lostPeriod   = 20; // Manual_lost_period_ratio;

	// create lost record
	g711_frame_num = ( single_channel_size/2 + FRAMESZ - 1 ) / FRAMESZ;
	pPlcLostRec = (bool *)malloc(g711_frame_num*sizeof(bool));
	memset(pPlcLostRec, 0x0, g711_frame_num*sizeof(bool));

	printf("\t-----[ frame type lost simulation ]-----\n");
	printf("\tTotla frame num = %d\n", g711_frame_num);
	printf("\tinitialFrame = %d\n", initialFrame);
	printf("\tlostFrameNum = %d\n", lostFrameNum);
	printf("\tlostPeriod = %d\n", lostPeriod);

	for( i=initialFrame; i<(g711_frame_num-10); i=i+lostPeriod ) {
		for( j=0; j<lostFrameNum; j++ ) {

			// set flag as lost frame
			pPlcLostRec[i+j] = 1;

			// change data
			memset(single_channel_dump+(i+j)*FRAMESZ*2, 0x0, FRAMESZ*2);

		}
	}
}

void g711PlcInit(void) {

	uint32_t i = 0;

	// convert byte data to 16bit signed data buffer
	pPlc16bBuf = (sint16_t *)malloc(single_channel_size);
	memcpy(pPlc16bBuf, single_channel_dump, single_channel_size);

	// prepare for output buffer
	pPlc16bOutput = (sint16_t *)malloc(single_channel_size);
	memset(pPlc16bOutput, 0xff, single_channel_size);
}

void g711PlcExit(void) {

	memcpy(single_channel_dump, pPlc16bOutput, single_channel_size);

	if( pPlc16bBuf != NULL ) {
		free(pPlc16bBuf);
		pPlc16bBuf = NULL;
	}
	if( pPlcLostRec != NULL ) {
		free(pPlcLostRec);
		pPlcLostRec = NULL;
	}
	if( pPlc16bOutput != NULL ) {
		free(pPlc16bOutput);
		pPlc16bOutput = NULL;
	}
}

void g711PlcProc(void) {

	sint32_t i;
	sint32_t nframes; // processed frame count
	sint32_t nerased; // erased frame count
	sint16_t in[FRAMESZ]; // i/o buffer

	g711plc_construct(&lc);
	nframes = 0;
	nerased = 0;
	for( nframes=0; nframes<g711_frame_num-1; nframes++ ) {

		// 
		memcpy(in, &pPlc16bBuf[nframes*FRAMESZ], FRAMESZ*sizeof(sint16_t));

		if( pPlcLostRec[nframes] != 0 ) {
			nerased++;
			g711plc_dofe(&lc, in);
		} else {
			g711plc_addtohistory (&lc, in);
		}

		/* 
		The concealment algorithm delays the signal by
		POVERLAPMAX samples. Remove the delay so the output
		file is time-aligned with the input file.
		*/
		if( nframes == 0 ) {
			memcpy(pPlc16bOutput+nframes, &in[POVERLAPMAX], (FRAMESZ-POVERLAPMAX)*sizeof(sint16_t));
		} else {
			memcpy(&pPlc16bOutput[nframes*FRAMESZ-POVERLAPMAX], in, FRAMESZ*sizeof(sint16_t));
		}
	}

}

/*-------------------- FUNCTIONS --------------------*/
void g711PlcMain(void) {

	if( fmt_single_body.bit_per_sample != 16 ) {
		printf("not suppport bit/sample != 16\n");
		return;
	}

	// prepare data
	g711PlcInit();

	// main processing
	g711PlcProc();

	// output data
	g711PlcExit();
}
