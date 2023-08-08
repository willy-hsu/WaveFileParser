#ifndef _H_LOWCFE_
#define _H_LOWCFE_

#include "arch.h"

// target at 8k sample rate speech pcm content

/*-------------------- CONFIGURATION --------------------*/
#define PITCH_MIN (40) /* minimum allowed pitch, 200 Hz */
#define PITCH_MAX (120) /* maximum allowed pitch, 66 Hz */
#define PITCHDIFF (PITCH_MAX - PITCH_MIN)
#define POVERLAPMAX (PITCH_MAX >> 2)/* maximum pitch OLA window */
#define HISTORYLEN (PITCH_MAX * 3 + POVERLAPMAX) /* history buffer length*/
#define NDEC (2) /* 2:1 decimation */
#define CORRLEN (160) /* 20 ms correlation length */
#define CORRBUFLEN (CORRLEN + PITCH_MAX) /* correlation buffer length */
#define CORRMINPOWER ((Float)250.) /* minimum power */
#define EOVERLAPINCR (32) /* end OLA increment per frame, 4 ms */
#define FRAMESZ (80) /* 10 ms at 8 KHz */
#define ATTENFAC ((Float).2) /* attenuation factor per 10 ms frame */
#define ATTENINCR (ATTENFAC/FRAMESZ) /* attenuation per sample */

typedef struct _LowcFE_c {
	sint32_t erasecnt;				/* consecutive erased frames */
	sint32_t poverlap;				/* overlap based on pitch */
	sint32_t poffset;				/* offset into pitch period */
	sint32_t pitch;					/* pitch estimate */
	sint32_t pitchblen;				/* current pitch buffer length */
	Float *pitchbufend;			/* end of pitch buffer */
	Float *pitchbufstart;		/* start of pitch buffer */
	Float pitchbuf[HISTORYLEN];	/* buffer for cycles of speech */
	Float lastq[POVERLAPMAX];	/* saved last quarter wavelengh */
	sint16_t history[HISTORYLEN];	/* history buffer */
} LowcFE_c;

/*-------------------- FUNCTIONS --------------------*/
void g711plc_construct (LowcFE_c *); /* constructor */
void g711plc_dofe (LowcFE_c *, short *s); /* synthesize speech for erasure */
void g711plc_addtohistory (LowcFE_c *, short *s); /* add a good frame to history buffer */

#endif
