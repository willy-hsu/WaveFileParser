#ifndef _UTILITY_H_
#define _UTILITY_H_

#include "arch.h"
#include "wave.h"
#include "wave_type.h"

extern char channel_name[SPEAKER_NUM_MAX][32];
extern uint32_t channel_mask[SPEAKER_NUM_MAX];

void message_speaker_mask(uint32_t input);
void message_format(uint16_t input);
void message_show_body(fmt_chunk_header fmt_header, fmt_chunk_body fmt_body);
uint8_t get_speaker_mask_num(uint32_t input);
uint8_t get_speaker_mask_idx(uint32_t input, uint8_t sequence_number);

#endif