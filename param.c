#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arch.h"
#include "utility.h"
#include "wave.h"
#include "wave_type.h"
#include "param.h"

// -------------------------------------------------- global parameter --------------------------------------------------
// flow control
uint8_t gFlow_dump_original_wav = 0;
uint8_t gFlow_dump_raw_pcm = 0;
uint8_t gFlow_dump_single_channel_header = 0; // 0: standard header, 1: extened header
uint8_t gFlow_dump_single_channel = 1; // 0: disable , 1: dump first channel, 2: dump all channels
uint8_t gFlow_dump_single_channel_pcm = 0; // 0: disable , 1: dump first channel, 2: dump all channels

// file
char gDebugString[256];
char filename[128];
uint8_t gFileSelection = 0;
FILE *fp = NULL;
FILE *fp_pcm_data = NULL;
FILE *fp_output = NULL;
FILE *fp_single_output = NULL;

// wav data processing
riff_chunk riff = { {'R','I','F','F'}, 0, {'W','A','V','E'}};
fmt_chunk_header fmt_header = { {'f','m','t',' '} };
fmt_chunk_body fmt_body;
data_chunk data_header = { {'d','a','t','a'} };

riff_chunk riff_single = { {'R','I','F','F'}, 0, {'W','A','V','E'}};
fmt_chunk_header fmt_single_header = { {'f','m','t',' '} };
fmt_chunk_body fmt_single_body;
data_chunk data_single_header = { {'d','a','t','a'} };

uint8_t *raw_dump;
uint8_t *singla_channel_dump;
uint32_t single_channel_size;
uint32_t sample_size_per_group;

// data lost and compensation
uint8_t lostMethod = LOSTTYPE_CONTINUOUS;
uint16_t Manual_lost_sample_ratio = 4;
uint16_t Manual_lost_period_ratio = 128;
uint16_t Manual_lost_start_sample = 15;
uint8_t compMethod = COMPTYPE_INNER_INTERPLOATION;

// other information
uint32_t block_numbers = 0;
uint8_t real_chunk_body_size = 0;

char InputFileFolder[][64] = {
	/*  0 */ "sample_wav",
	/*  1 */ "sample_wav",
	/*  2 */ "sample_wav",
	/*  3 */ "sample_wav",
	/*  4 */ "sample_wav",
	/*  5 */ "sample_wav",
	/*  6 */ "sample_wav",
	/*  7 */ "sample_wav",
	/*  8 */ "sample_wav",
	/*  9 */ "sample_wav",
	/* 10 */ "sample_wav",
	/* 11 */ "sample_wav",
	/* 12 */ "sample_wav",
	/* 13 */ "sample_wav",
	/* 14 */ "sample_wav",
	/* 15 */ "sample_wav",
	/* 16 */ "sample_wav",
	/* 17 */ "sample_wav",
	/* 18 */ "sample_wav",
	/* 19 */ "sample_wav",
	/* 20 */ "sample_wav",
	/* 21 */ "sample_wav",
	/* 22 */ "sample_wav",
	/* 23 */ "sample_wav",
	/* 24 */ "sample_wav",
	/* 25 */ "sample_wav",
	/* 26 */ "sample_wav",
	/* 27 */ "sample_wav",
	/* 28 */ "sample_wav",
	/* 29 */ "sample_wav",
	/* 30 */ "sample_wav",
	/* 31 */ "sample_wav",
	/* 32 */ "sample_wav",
	/* 33 */ "sample_wav",
	/* 34 */ "simple_wav",
	/* 35 */ "simple_wav",
};

char InputFileName[][256] = {
	/*  0 */ "beemoved_96k_2ch_16b_short",
	/*  1 */ "beemoved_96k_2ch_24b_short",
	/*  2 */ "danca_192k_2ch_24b_short",
	/*  3 */ "divertimento_192k_2ch_24b_short",
	/*  4 */ "dnd2r_48k_8ch_16b_short",
	/*  5 */ "dndr_48k_8ch_16b_short",
	/*  6 */ "dnd_48k_8ch_16b_short",
	/*  7 */ "dnd_48k_8ch_24b_short",
	/*  8 */ "dolbyatmos_48k_6ch_16b_short1",
	/*  9 */ "dolbyatmos_48k_6ch_16b_short2",
	/* 10 */ "dolbytest_48k_6ch_16b_short",
	/* 11 */ "fantasiestucke_192k_2ch_24b_short",
	/* 12 */ "fantasiestucke_96k_2ch_24b_short",
	/* 13 */ "frozenplanetr_48k_8ch_16b_short",
	/* 14 */ "frozenplanet_48k_8ch_16b_short",
	/* 15 */ "frozenplanet_48k_8ch_24b_short",
	/* 16 */ "maverick_48k_6ch_16b_short",
	/* 17 */ "music_192k_8ch_16b_short",
	/* 18 */ "music_48k_8ch_16b_short",
	/* 19 */ "music_96k_8ch_16b_short",
	/* 20 */ "preludes_192k_2ch_24b_short",
	/* 21 */ "rimsky_192k_2ch_16b_short",
	/* 22 */ "rimsky_192k_2ch_24b_short",
	/* 23 */ "rimsky_48k_2ch_16b_short",
	/* 24 */ "rimsky_48k_2ch_24b_short",
	/* 25 */ "rimsky_96k_2ch_16b_short",
	/* 26 */ "rimsky_96k_2ch_24b_short",
	/* 27 */ "vindiesel_48k_6ch_16b_short",
	/* 28 */ "violincon_192k_2ch_16b_short",
	/* 29 */ "violincon_192k_2ch_24b_short",
	/* 30 */ "violincon_48k_2ch_16b_short",
	/* 31 */ "violincon_48k_2ch_24b_short",
	/* 32 */ "violincon_96k_2ch_16b_short",
	/* 33 */ "violincon_96k_2ch_24b_short",
	/* 34 */ "CantinaBand3",
	/* 35 */ "BabyElephantWalk60",
};
