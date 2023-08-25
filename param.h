#ifndef _PARAM_H_
#define _PARAM_H_

#include "arch.h"
#include "wave.h"
#include "wave_type.h"

// flow control
extern uint8_t gFlow_dump_original_wav;
extern uint8_t gFlow_dump_modified;
extern uint8_t gFlow_dump_raw_pcm;
extern uint8_t gFlow_dump_single_channel_header;
extern uint8_t gFlow_dump_single_channel;
extern uint8_t gFlow_dump_single_channel_pcm;

// file
extern char gDebugString[256];
extern char filename[128];
extern uint8_t gFileSelection;
extern FILE *fp;
extern FILE *fp_pcm_data;
extern FILE *fp_output;
extern FILE *fp_single_output;

// processing
extern riff_chunk riff;
extern fmt_chunk_header fmt_header;
extern fmt_chunk_body fmt_body;
extern data_chunk data_header;

extern riff_chunk riff_single;
extern fmt_chunk_header fmt_single_header;
extern fmt_chunk_body fmt_single_body;
extern data_chunk data_single_header;

extern uint8_t *raw_dump;
extern uint8_t *single_channel_dump;
extern uint32_t single_channel_size;
extern uint32_t sample_size_per_group;

// data lost and compensation
extern uint8_t lostRandomOffsetEnable;
extern uint32_t randomOffsetMax;
extern uint8_t lostMethod;
extern uint16_t Manual_lost_sample_ratio;
extern uint16_t Manual_lost_period_ratio;
extern uint16_t Manual_lost_start_sample;
extern uint8_t compMethod;

// other information
extern uint32_t block_numbers;
extern uint8_t real_chunk_body_size;

extern uint8_t process_file_start;
extern uint8_t process_file_end;
extern char InputFileFolder[][64];
extern char InputFileName[][256];

#endif
