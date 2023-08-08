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
uint8_t gFlow_dump_single_channel = 0; // 0: disable , 1: dump first channel, 2: dump all channels
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
uint8_t *single_channel_dump;
uint32_t single_channel_size;
uint32_t sample_size_per_group;

// data lost and compensation
uint8_t lostRandomOffsetEnable = 0; // 0:disable, 1:enable
uint32_t randomOffsetMax = 200; // unit : samples
uint8_t lostMethod = LOSTTYPE_NONE;
uint16_t Manual_lost_sample_ratio = 1;
uint16_t Manual_lost_period_ratio = 1;
uint16_t Manual_lost_start_sample = 15;
uint8_t compMethod = COMPTYPE_NONE;

// other information
uint32_t block_numbers = 0;
uint8_t real_chunk_body_size = 0;

char InputFileFolder[][64] = {
	// 48k
	/*  0 */ "sample_wav/48k",
	/*  1 */ "sample_wav/48k",
	/*  2 */ "sample_wav/48k",
	/*  3 */ "sample_wav/48k",
	/*  4 */ "sample_wav/48k",
	/*  5 */ "sample_wav/48k",
	/*  6 */ "sample_wav/48k",
	/*  7 */ "sample_wav/48k",
	/*  8 */ "sample_wav/48k",
	/*  9 */ "sample_wav/48k",
	/* 10 */ "sample_wav/48k",
	/* 11 */ "sample_wav/48k",
	/* 12 */ "sample_wav/48k",
	/* 13 */ "sample_wav/48k",
	/* 14 */ "sample_wav/48k",
	/* 15 */ "sample_wav/48k",
	/* 16 */ "sample_wav/48k",
	// 96k
	/* 17 */ "sample_wav/96k",
	/* 18 */ "sample_wav/96k",
	/* 19 */ "sample_wav/96k",
	/* 20 */ "sample_wav/96k",
	/* 21 */ "sample_wav/96k",
	/* 22 */ "sample_wav/96k",
	/* 23 */ "sample_wav/96k",
	/* 24 */ "sample_wav/96k",
	// 192k
	/* 25 */ "sample_wav/192k",
	/* 26 */ "sample_wav/192k",
	/* 27 */ "sample_wav/192k",
	/* 28 */ "sample_wav/192k",
	/* 29 */ "sample_wav/192k",
	/* 30 */ "sample_wav/192k",
	/* 31 */ "sample_wav/192k",
	/* 32 */ "sample_wav/192k",
	/* 33 */ "sample_wav/192k",
	// simple wav file
	/* 34 */ "simple_wav",
	/* 35 */ "simple_wav",
	// speech type audio
	/* 36 */ "speech_wav",
	/* 37 */ "speech_wav",
	/* 38 */ "speech_wav",
	/* 39 */ "speech_wav",
	/* 40 */ "speech_wav",
};

char InputFileName[][256] = {
	// 48k
	/*  0 */ "dnd2r_48k_8ch_16b_short",
	/*  1 */ "dndr_48k_8ch_16b_short",
	/*  2 */ "dnd_48k_8ch_16b_short",
	/*  3 */ "dnd_48k_8ch_24b_short",
	/*  4 */ "dolbyatmos_48k_6ch_16b_short1",
	/*  5 */ "dolbyatmos_48k_6ch_16b_short2",
	/*  6 */ "dolbytest_48k_6ch_16b_short",
	/*  7 */ "frozenplanetr_48k_8ch_16b_short",
	/*  8 */ "frozenplanet_48k_8ch_16b_short",
	/*  9 */ "frozenplanet_48k_8ch_24b_short",
	/* 10 */ "maverick_48k_6ch_16b_short",
	/* 11 */ "music_48k_8ch_16b_short",
	/* 12 */ "rimsky_48k_2ch_16b_short",
	/* 13 */ "rimsky_48k_2ch_24b_short",
	/* 14 */ "vindiesel_48k_6ch_16b_short",
	/* 15 */ "violincon_48k_2ch_16b_short",
	/* 16 */ "violincon_48k_2ch_24b_short",
	// 96k
	/* 17 */ "beemoved_96k_2ch_16b_short",
	/* 18 */ "beemoved_96k_2ch_24b_short",
	/* 19 */ "fantasiestucke_96k_2ch_24b_short",
	/* 20 */ "music_96k_8ch_16b_short",
	/* 21 */ "rimsky_96k_2ch_16b_short",
	/* 22 */ "rimsky_96k_2ch_24b_short",
	/* 23 */ "violincon_96k_2ch_16b_short",
	/* 24 */ "violincon_96k_2ch_24b_short",
	// 192k
	/* 25 */ "danca_192k_2ch_24b_short",
	/* 26 */ "divertimento_192k_2ch_24b_short",
	/* 27 */ "fantasiestucke_192k_2ch_24b_short",
	/* 28 */ "music_192k_8ch_16b_short",
	/* 29 */ "preludes_192k_2ch_24b_short",
	/* 30 */ "rimsky_192k_2ch_16b_short",
	/* 31 */ "rimsky_192k_2ch_24b_short",
	/* 32 */ "violincon_192k_2ch_16b_short",
	/* 33 */ "violincon_192k_2ch_24b_short",
	// simple wav file
	/* 34 */ "CantinaBand3",
	/* 35 */ "BabyElephantWalk60",
	// speech type audio
	/* 36 */ "nsa_m_CH01",
	/* 37 */ "OSR_cn_000_0072_8k",
	/* 38 */ "OSR_cn_000_0073_8k",
	/* 39 */ "OSR_cn_000_0074_8k",
	/* 40 */ "taiwainess_speech",
};
