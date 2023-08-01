#ifndef _UTILITY_H_
#define _UTILITY_H_

#include <string.h>
#include "arch.h"
#include "wave.h"
#include "wave_type.h"

/*-------------------- STRUCTURE --------------------*/
enum {
	LOSTTYPE_NONE = 0,
	LOSTTYPE_CONTINUOUS,
	LOSTTYPE_INTERLEAVE,
	LOSTTYPE_MAX,
};

enum {
	COMPTYPE_NONE = 0,
	COMPTYPE_INNER_INTERPLOATION,
	COMPTYPE_G711_VOIP,
	COMPTYPE_MAX,
};

/*-------------------- GLOBAL PARAMETER --------------------*/
extern char comptype_name[COMPTYPE_MAX][32];
extern char losttype_name[LOSTTYPE_MAX][32];
extern char channel_name[SPEAKER_NUM_MAX][32];
extern uint32_t channel_mask[SPEAKER_NUM_MAX];

/*-------------------- DATA/STRING PROCESSING FUNCTIONS --------------------*/
void Arr2String(char *Dest, char *Src, uint8_t size );
sint32_t b24_signed_to_b32_signed(uint8_t *pBuf);

/*-------------------- FUNCTIONS --------------------*/
void message_speaker_mask(uint32_t input);
void message_format(uint16_t input);
void message_show_body(fmt_chunk_header fmt_header, fmt_chunk_body fmt_body);
uint8_t get_speaker_mask_num(uint32_t input);
uint8_t get_speaker_mask_idx(uint32_t input, uint8_t sequence_number);

#endif