/**
 * @file LowcFE.c
 * @author weiyuan.hsu
 * @brief
 * implement of Low Complexity Frame Erasure concealment (LowcFE) according to G711 Appendix I
 * 
 * g711plc_construct: ...... LowcFE Constructor.
 * 
 * g711plc_dofe: ........... Generate the synthetic signal.
 * 							 At the beginning of an erasure determine the pitch, and extract
 * 							 one pitch period from the tail of the signal. Do an OLA for 1/4
 * 							 of the pitch to smooth the signal. Then repeat the extracted signal
 * 							 for the length of the erasure. If the erasure continues for more than
 * 							 10 msec, increase the number of periods in the pitchbuffer. At the end
 * 							 of an erasure, do an OLA with the start of the first good frame.
 * 							 The gain decays as the erasure gets longer.
 * 
 * g711plc_addtohistory: ... A good frame was received and decoded.
 * 							 If right after an erasure, do an overlap add with the synthetic signal.
 * 							 Add the frame to history buffer.
 * 
 * @copyright Copyright (c) 2023
 * 
 */

// external reference : https://github.com/openitu/STL
#include <math.h>
#include "config.h"
#include "LowcFE.h"
#include "arch.h"
#include "config.h"

/*-------------------- INTERNAL FUNCTIONS --------------------*/
static void g711plc_scalespeech(LowcFE_c *, sint16_t *out);
static void g711plc_getfespeech(LowcFE_c *, sint16_t *out, sint32_t sz);
static void g711plc_savespeech(LowcFE_c *, sint16_t *s);
static sint32_t g711plc_findpitch(LowcFE_c *);
static void g711plc_overlapadd(Float * l, Float * r, Float * o, sint32_t cnt);
static void g711plc_overlapadds(sint16_t *l, sint16_t *r, sint16_t *o, sint32_t cnt);
static void g711plc_overlapaddatend(LowcFE_c *, sint16_t *s, sint16_t *f, sint32_t cnt);
static void g711plc_convertsf(sint16_t *f, Float * t, sint32_t cnt);
static void g711plc_convertfs(Float * f, sint16_t *t, sint32_t cnt);
static void g711plc_copyf(Float * f, Float * t, sint32_t cnt);
static void g711plc_copys(sint16_t *f, sint16_t *t, sint32_t cnt);
static void g711plc_zeros(sint16_t *s, sint32_t cnt);

/**
 * @brief 
 * Get samples from the circular pitch buffer. Update poffset so
 * when subsequent frames are erased the signal continues.
 */
static void g711plc_getfespeech(LowcFE_c * lc, sint16_t *out, sint32_t sz) {
	while (sz) {
		sint32_t cnt = lc->pitchblen - lc->poffset;
		if (cnt > sz) {
			cnt = sz;
		}
		g711plc_convertfs(&lc->pitchbufstart[lc->poffset], out, cnt);
		lc->poffset += cnt;
		if (lc->poffset == lc->pitchblen) {
			lc->poffset = 0;
		}
		out += cnt;
		sz -= cnt;
	}
}

static void g711plc_scalespeech(LowcFE_c * lc, sint16_t *out) {
	sint32_t i;
	Float g = (Float) 1. - (lc->erasecnt - 1) * ATTENFAC;
	for (i = 0; i < FRAMESZ; i++) {
		out[i] = (sint16_t) (out[i] * g);
		g -= ATTENINCR;
	}
}

/**
 * @brief 
 * Save a frames worth of new speech in the history buffer.
 * Return the output speech delayed by POVERLAPMAX. (will modify input data buffer *s)
 */
static void g711plc_savespeech(LowcFE_c * lc, sint16_t *s) {
	/* make room for new signal */
	g711plc_copys(&lc->history[FRAMESZ], lc->history, HISTORYLEN - FRAMESZ);
	/* copy in the new frame */
	g711plc_copys(s, &lc->history[HISTORYLEN - FRAMESZ], FRAMESZ);
	/* copy out the delayed frame */
	g711plc_copys(&lc->history[HISTORYLEN - FRAMESZ - POVERLAPMAX], s, FRAMESZ);
}

/**
 * @brief 
 * Overlapp add the end of the erasure with the start of the first good frame
 * Scale the synthetic speech by the gain factor before the OLA.
 */
static void g711plc_overlapaddatend(LowcFE_c * lc, sint16_t *s, sint16_t *f, sint32_t cnt) {
	sint32_t i;
	Float incrg;
	Float lw, rw;
	Float t;
	Float incr = (Float) 1. / cnt;
	Float gain = (Float) 1. - (lc->erasecnt - 1) * ATTENFAC;
	if (gain < 0.) {
		gain = (Float) 0.;
	}
	incrg = incr * gain;
	lw = ((Float) 1. - incr) * gain;
	rw = incr;
	for (i = 0; i < cnt; i++) {
		t = lw * f[i] + rw * s[i];
		if (t > 32767.) {
			t = (Float) 32767.;
		} else if (t < -32768.) {
			t = (Float) - 32768.;
		}
		s[i] = (sint16_t) t;
		lw -= incrg;
		rw += incr;
	}
}

/**
 * @brief 
 * Overlapp add left and right sides
 */
static void g711plc_overlapadd(Float * l, Float * r, Float * o, sint32_t cnt) {
	sint32_t i;
	Float incr, lw, rw, t;

	if (cnt == 0) {
		return;
	}
	incr = (Float) 1. / cnt;
	lw = (Float) 1. - incr;
	rw = incr;
	for (i = 0; i < cnt; i++) {
		t = lw * l[i] + rw * r[i];
		if (t > (Float) 32767.) {
			t = (Float) 32767.;
		} else if (t < (Float) - 32768.) {
			t = (Float) - 32768.;
		}
		o[i] = t;
		lw -= incr;
		rw += incr;
	}
}

/**
 * @brief 
 * Overlapp add left and right sides
 */
static void g711plc_overlapadds(sint16_t *l, sint16_t *r, sint16_t *o, int cnt) {
	int i;
	Float incr, lw, rw, t;

	if (cnt == 0) {
		return;
	}
	incr = (Float) 1. / cnt;
	lw = (Float) 1. - incr;
	rw = incr;
	for (i = 0; i < cnt; i++) {
		t = lw * l[i] + rw * r[i];
		if (t > (Float) 32767.) {
			t = (Float) 32767.;
		} else if (t < (Float) - 32768.) {
			t = (Float) - 32768.;
		}
		o[i] = (sint16_t) t;
		lw -= incr;
		rw += incr;
	}
}

/**
 * @brief 
 * Estimate the pitch.
 * l - pointer to first sample in last 20 msec of speech.
 * r - points to the sample PITCH_MAX before l
 * @return sint32_t 
 */
static sint32_t g711plc_findpitch(LowcFE_c *lc) {

	sint32_t i, j, k;
	sint32_t bestmatch;
	Float bestcorr;
	Float corr;		/* correlation */
	Float energy;	/* running energy */
	Float scale;	/* scale correlation by average power */
	Float *rp;		/* segment to match */
	Float *l = lc->pitchbufend - CORRLEN;
	Float *r = lc->pitchbufend - CORRBUFLEN;

	rp = r;
	/* coarse search initial run at position 0 */
	energy = (Float) 0.;
	corr = (Float) 0.;
	for (i = 0; i < CORRLEN; i += NDEC) {
		energy += rp[i] * rp[i];
		corr += rp[i] * l[i];
	}
	scale = (energy < CORRMINPOWER)? CORRMINPOWER : energy;
	corr = corr / (Float) sqrt (scale);
	bestcorr = corr;
	bestmatch = 0;

	/* coarse search */
	for (j = NDEC; j <= PITCHDIFF; j += NDEC) {
		energy -= rp[0] * rp[0];
#if SRC_FIX_ME
		energy += rp[CORRLEN] * rp[CORRLEN];
#else
		energy += rp[CORRLEN-1+NDEC] * rp[CORRLEN-1+NDEC];
#endif
		rp += NDEC;
		corr = 0.f;
		for (i = 0; i < CORRLEN; i += NDEC) {
			corr += rp[i] * l[i];
		}
		scale = (energy < CORRMINPOWER)? CORRMINPOWER : energy;
		corr /= (Float) sqrt (scale);
		if (corr >= bestcorr) {
			bestcorr = corr;
			bestmatch = j;
		}
	}

	/* fine search initial run at position coarse search best position */
	j = bestmatch - (NDEC - 1);
	if (j < 0) {
		j = 0;
	}
	k = bestmatch + (NDEC - 1);
	if (k > PITCHDIFF) {
		k = PITCHDIFF;
	}
	rp = &r[j];
	energy = 0.f;
	corr = 0.f;
	for (i = 0; i < CORRLEN; i++) {
		energy += rp[i] * rp[i];
		corr += rp[i] * l[i];
	}
	scale = (energy < CORRMINPOWER)? CORRMINPOWER : energy;
	corr = corr / (Float) sqrt (scale);
	bestcorr = corr;
	bestmatch = j;

	/* fine search */
	for (j++; j <= k; j++) {
		energy -= rp[0] * rp[0];
		energy += rp[CORRLEN] * rp[CORRLEN];
		rp++;
		corr = 0.f;
		for (i = 0; i < CORRLEN; i++) {
			corr += rp[i] * l[i];
		}
		scale = (energy < CORRMINPOWER)? CORRMINPOWER : energy;
		corr = corr / (Float) sqrt (scale);
		if (corr > bestcorr) {
			bestcorr = corr;
			bestmatch = j;
		}
	}

	return PITCH_MAX - bestmatch;

}

static void g711plc_convertsf(sint16_t *f, Float * t, sint32_t cnt) {
	sint32_t i;
	for (i = 0; i < cnt; i++) {
		t[i] = (Float) f[i];
	}
}

static void g711plc_convertfs(Float * f, sint16_t *t, sint32_t cnt) {
	sint32_t i;
	for (i = 0; i < cnt; i++) {
		t[i] = (sint16_t) f[i];
	}
}

static void g711plc_copyf(Float * f, Float * t, sint32_t cnt) {
	sint32_t i;
	for (i = 0; i < cnt; i++) {
		t[i] = f[i];
	}
}

static void g711plc_copys(sint16_t *f, sint16_t *t, sint32_t cnt) {
	sint32_t i;
	for (i = 0; i < cnt; i++) {
		t[i] = f[i];
	}
}

static void g711plc_zeros(sint16_t *s, sint32_t cnt) {
	sint32_t i;
	for (i = 0; i < cnt; i++) {
		s[i] = 0;
	}
}

/*-------------------- FUNCTIONS --------------------*/
void g711plc_construct(LowcFE_c * lc) {
	lc->erasecnt = 0;
	lc->pitchbufend = &lc->pitchbuf[HISTORYLEN];
	g711plc_zeros(lc->history, HISTORYLEN);
}

/**
 * @brief 
 * Generate the synthetic signal.
 * At the beginning of an erasure determine the pitch, and extract
 * one pitch period from the tail of the signal. Do an OLA for 1/4
 * of the pitch to smooth the signal. Then repeat the extracted signal
 * for the length of the erasure. If the erasure continues for more than
 * 10 msec, increase the number of periods in the pitchbuffer. At the end
 * of an erasure, do an OLA with the start of the first good frame.
 * The gain decays as the erasure gets longer.
 */
void g711plc_dofe(LowcFE_c * lc, sint16_t *out) {
	if (lc->erasecnt == 0) {
		/* get history */
		g711plc_convertsf(lc->history, lc->pitchbuf, HISTORYLEN);
		lc->pitch = g711plc_findpitch (lc); /* find pitch */
		lc->poverlap = lc->pitch >> 2;      /* OLA 1/4 wavelength */
		/* save original last poverlap samples */
		g711plc_copyf (lc->pitchbufend - lc->poverlap, lc->lastq, lc->poverlap);
		lc->poffset = 0;            /* create pitch buffer with 1 period */
		lc->pitchblen = lc->pitch;
		lc->pitchbufstart = lc->pitchbufend - lc->pitchblen;
		g711plc_overlapadd (lc->lastq, lc->pitchbufstart - lc->poverlap, lc->pitchbufend - lc->poverlap, lc->poverlap);
		/* update last 1/4 wavelength in history buffer */
		g711plc_convertfs (lc->pitchbufend - lc->poverlap, &lc->history[HISTORYLEN - lc->poverlap], lc->poverlap);
		/* get synthesized speech */
		g711plc_getfespeech (lc, out, FRAMESZ);
	} else if (lc->erasecnt == 1 || lc->erasecnt == 2) {
		/* tail of previous pitch estimate */
		sint16_t tmp[POVERLAPMAX];
		sint32_t saveoffset = lc->poffset;       /* save offset for OLA */
		/* continue with old pitchbuf */
		g711plc_getfespeech (lc, tmp, lc->poverlap);
		/* add periods to the pitch buffer */
		lc->poffset = saveoffset;
		while (lc->poffset > lc->pitch)
		lc->poffset -= lc->pitch;
		lc->pitchblen += lc->pitch; /* add a period */
		lc->pitchbufstart = lc->pitchbufend - lc->pitchblen;
		g711plc_overlapadd (lc->lastq, lc->pitchbufstart - lc->poverlap, lc->pitchbufend - lc->poverlap, lc->poverlap);
		/* overlap add old pitchbuffer with new */
		g711plc_getfespeech (lc, out, FRAMESZ);
		g711plc_overlapadds (tmp, out, out, lc->poverlap);
		g711plc_scalespeech (lc, out);
	} else if (lc->erasecnt > 5) {
		g711plc_zeros (out, FRAMESZ);
	} else {
		g711plc_getfespeech (lc, out, FRAMESZ);
		g711plc_scalespeech (lc, out);
	}
	lc->erasecnt++;
	g711plc_savespeech (lc, out);
}

/**
 * @brief 
 * A good frame was received and decoded.
 * If right after an erasure, do an overlap add with the synthetic signal.
 * Add the frame to history buffer.
 */
void g711plc_addtohistory(LowcFE_c * lc, sint16_t *s) {
	if (lc->erasecnt) {
		sint16_t overlapbuf[FRAMESZ];
		/* 
		longer erasures require longer overlaps
		to smooth the transition between the synthetic
		and real signal.
		*/
		sint32_t olen = lc->poverlap + (lc->erasecnt - 1) * EOVERLAPINCR;
		if (olen > FRAMESZ) {
			olen = FRAMESZ;
		}
		g711plc_getfespeech (lc, overlapbuf, olen);
		g711plc_overlapaddatend (lc, s, overlapbuf, olen);
		lc->erasecnt = 0;
	}
	g711plc_savespeech (lc, s);
}
