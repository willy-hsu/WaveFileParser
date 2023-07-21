#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "arch.h"
#include "utility.h"
#include "wave.h"
#include "wave_type.h"

/**
 * @brief 
 * Target 
 * 192K, lost 8 samples
 * 96K,  lost 4 samples
 * 48K,  lost 2 samples
 */

// reference
// https://codereview.stackexchange.com/questions/222026/parse-a-wav-file-and-export-pcm-data
// https://www.mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html

// -------------------------------------------------- global parameter --------------------------------------------------
// file
char gDebugString[256];
char filename[128];
uint8_t gFileSelection = 0;
FILE *fp = NULL;
FILE *fp_pcm_data = NULL;
FILE *fp_output = NULL;
FILE *fp_single_output = NULL;

// processing
uint8_t buf[5];

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

// other information
uint32_t block_numbers = 0;
uint8_t real_chunk_body_size = 0;

char InputFileFolder[][64] = {
	/* 0 */ "sample_wav",
	/* 1 */ "sample_wav",
	/* 2 */ "simple_wav",
	/* 3 */ "simple_wav",
};

char InputFileName[][256] = {
	/* 0 */ "beemoved_96k_2ch_16b_short",
	/* 1 */ "beemoved_96k_2ch_24b_short",
	/* 2 */ "CantinaBand3",
	/* 3 */ "BabyElephantWalk60",
};

// -------------------------------------------------- functions --------------------------------------------------
void Arr2String(char *Dest, char *Src, uint8_t size ) {
	strncpy(Dest, Src, size);
	Dest[size] = '\0';
}

/**
 * @brief 
 * simulate the data lost and compensation process
 * raw_dump store the raw pcm data
 */
void Model_DataLostAndCompensation(void) {


	/*
	| buffer in bytes units
	|                           |<----- lostPts ----->|                                |<----- lostPts ----->|                                |<----- lostPts ----->|                                  ...
	|<----- initial phase ----->|<-------------------- lostPeriod -------------------->|<-------------------- lostPeriod -------------------->|<-------------------- lostPeriod -------------------->| ...
	*/

	uint32_t initialPhase = 5000;
	uint32_t lostPeriod = fmt_single_body.sample_rate * fmt_single_body.bit_per_sample / 8; // unit : bytes
	uint32_t lostPts = 128; // unit : bytes

	printf("-----[ data lost simulation ]-----\n");
	printf("initial phase : %d\n", initialPhase);
	printf("lost period : %d bytes\n", lostPeriod);
	printf("lost sample : %d \n", lostPts);
	printf("-----[ data lost simulation ]-----\n");

	for( uint32_t i=initialPhase; i<single_channel_size; i=i+lostPeriod/10 ) {
		for( uint32_t j=0; j<lostPts; j++ ) {
			singla_channel_dump[i+j] = 0;
		}
	}

}

int main() {

	// ----------------------------------------------------------------------------------------------------
	// open file
	sprintf(filename, "%s/%s.wav", InputFileFolder[gFileSelection], InputFileName[gFileSelection]);
	if ((fp = fopen(filename, "rb"))==NULL) {
		printf("Can't open the file. Exit.\n");
		goto EXIT;
	}

	// ----------------------------------------------------------------------------------------------------
	// Reading RIFF section
	if (fread(&riff, sizeof(uint8_t), sizeof(riff), fp) != sizeof(riff))  {
		printf("Can't read RIFF chunk or EOF is met. Exit.\n");
		goto EXIT;
	} else {
		if (strncmp("RIFF", riff.id, sizeof(riff.id)) != 0) {
			Arr2String(gDebugString, riff.id, sizeof(riff.id));
			printf("File format is %s, not RIFF. Exit.\n", gDebugString);
			goto EXIT;
		}
		if (strncmp("WAVE", riff.type, sizeof(riff.type)) != 0) {
			Arr2String(gDebugString, riff.type, sizeof(riff.type));
			printf("File format is %s, not WAVE. Exit.\n", gDebugString);
			goto EXIT;
		}
	};

	// ----------------------------------------------------------------------------------------------------
	// Reading fmt.id and fmt.size
	if (fread(&fmt_header, sizeof(uint8_t), 8, fp) != 8) {
		printf("Can't read fmt chunk or EOF is met. Exit.\n");
		goto EXIT;
	} else {
		if (strncmp("fmt ", fmt_header.id, sizeof(fmt_header.id)) != 0) {
			printf("File have no fmt chunk. Exit.\n");
			goto EXIT;
		}
	}

	printf("fmt size: %d\n", fmt_header.size);

	// ----------------------------------------------------------------------------------------------------
	// Reading fmt Sample Format Info
	memset(&fmt_body, 0x0, sizeof(fmt_body));
	if (fread(&fmt_body, sizeof(uint8_t), fmt_header.size, fp) != fmt_header.size) {
		printf("Can't read Sample Format Info in fmt chunk or EOF is met. Exit.\n");
		goto EXIT;
	}

	message_show_body(fmt_header, fmt_body);

	// Reading data/some chunk
	if (fread(&data_header, sizeof(uint8_t), 8, fp)!=8) {
		printf("Error of reading data chunk. Exit.\n");
		goto EXIT;
	} else {
		while( strncmp("data", data_header.id, sizeof(data_header.id)) != 0 ) {
			if( fseek(fp, data_header.size, SEEK_CUR) != 0 ) {
				printf("Error of finding data chunk. Exit.\n");
				goto EXIT;
			}
			fread(&data_header, sizeof(uint8_t), 8, fp);
		}
	}
	
	printf("data size = %d\n", data_header.size);

	// ----------------------------------------------------------------------------------------------------
	// Reading PCM raw data and save to file
	raw_dump = (uint8_t*)malloc(data_header.size);
	if (raw_dump == NULL) {
		printf("Allocation memory error");
		goto EXIT;
	}

	if( (fmt_body.format_tag == WAVE_FORMAT_PCM) || (fmt_body.format_tag == WAVE_FORMAT_EXTENSIBLE && (*(uint16_t *)fmt_body.sub_format) == WAVE_FORMAT_PCM ) ) {
		block_numbers = data_header.size / fmt_body.block_align;
		printf("Start to get pcm data with %d blocks\n", block_numbers);
		if ( fread(raw_dump, fmt_body.block_align, block_numbers, fp) != block_numbers ) {
			printf("Readin PCM data error.\n");
			goto EXIT;
		} else {
			sprintf(filename, "output/%s_pcm.raw", InputFileName[gFileSelection]);
			if( (fp_pcm_data = fopen(filename, "wb")) == NULL ) {
				printf("Can't open the raw PCM file for write. Exit.\n");
				goto EXIT;
			}
			if( fwrite(raw_dump, fmt_body.block_align, block_numbers, fp_pcm_data) != block_numbers ) {
				printf("Can't write PCM file. Exit.\n");
				goto EXIT;
			}
			printf("Done. PCM data writing in %s .\n", filename);
		}
	} else {
		printf("format tag is not PCM. Exit.\n");
		goto EXIT;
	}

	// ----------------------------------------------------------------------------------------------------
	// Package wave file
	sprintf(filename, "output/%s_modified.wav", InputFileName[gFileSelection]);
	if( (fp_output = fopen(filename, "wb")) == NULL ) {
		printf("Can't open the new WAV file for write. Exit.\n");
		goto EXIT;
	} else {
		// write original information
		if( fwrite(&riff, 1, sizeof(riff), fp_output) != sizeof(riff) ) {
			printf("Can't write WAV file riff header. Exit.\n");
			goto EXIT;
		}
		if( fwrite(&fmt_header, 1, sizeof(fmt_header), fp_output) != sizeof(fmt_header) ) {
			printf("Can't write WAV file chunk header. Exit.\n");
			goto EXIT;
		}
		if( fwrite(&fmt_body, 1, fmt_header.size, fp_output) != fmt_header.size ) {
			printf("Can't write WAV file chunk body. Exit.\n");
			goto EXIT;
		}
		if( fwrite(&data_header, 1, sizeof(data_header), fp_output) != sizeof(data_header) ) {
			printf("Can't write WAV file data chunk. Exit.\n");
			goto EXIT;
		}
		if( fwrite(raw_dump, fmt_body.block_align, block_numbers, fp_output) != block_numbers ) {
			printf("Can't write WAV file pcm data. Exit.\n");
			goto EXIT;
		}
		printf("Done. WAV file writing in %s .\n", filename);
	}

	// ----------------------------------------------------------------------------------------------------
	// separate data to individual channel and generate individual wav file
	if( fmt_body.channels >= 1 ) {

		// allocate buffer to separate each channel
		single_channel_size = data_header.size / fmt_body.channels;
		sample_size_per_group = fmt_body.bit_per_sample * fmt_body.channels / 8;
		printf("Single Channel Size : %d bits\n", single_channel_size);
		printf("Data Size per group: %d bytes\n", sample_size_per_group);

		// prepare single channel information
		memcpy(&riff_single, &riff, sizeof(riff_chunk));
		memcpy(&fmt_single_header, &fmt_header, sizeof(fmt_chunk_header));
		memcpy(&fmt_single_body, &fmt_body, sizeof(fmt_chunk_body));
		riff_single.size = single_channel_size + 154;
		fmt_single_body.channels = 1;
		fmt_single_body.byte_per_sec = fmt_single_body.sample_rate * fmt_single_body.bit_per_sample / 8;
		fmt_single_body.channel_mask = MASK_SPEAKER_FRONT_LEFT;
		data_single_header.size = single_channel_size;

		// show signle file information
		message_show_body(fmt_header, fmt_single_body);

		singla_channel_dump = (uint8_t*)malloc(single_channel_size);
		uint32_t singla_channel_dump_idx = 0;
		for( uint8_t ch=0; ch<fmt_body.channels; ch++ ) {

			// separate single channel data
			singla_channel_dump_idx = 0;
			for( uint32_t idx=0; idx<data_header.size; idx=idx+sample_size_per_group ) {
				for( uint32_t sample_idx=0; sample_idx<(fmt_body.bit_per_sample/8); sample_idx++ ) {
					singla_channel_dump[singla_channel_dump_idx++] = raw_dump[idx + ch*(fmt_body.bit_per_sample/8) + sample_idx];
				}
			}

			// simulation data lost
			Model_DataLostAndCompensation();

			// write data
			sprintf(filename, "output/MY_%s_%s.wav", InputFileName[gFileSelection], channel_name[get_speaker_mask_idx(fmt_body.channel_mask, ch)]);
			if( (fp_single_output = fopen(filename, "wb")) == NULL ) {
				printf("Can't open the new WAV file for write. Exit.\n");
			} else {
				// write original information
				if( fwrite(&riff_single, 1, sizeof(riff_single), fp_single_output) != sizeof(riff_single) ) {
					printf("Can't write WAV file riff header. Exit.\n");
					goto EXIT;
				}
				if( fwrite(&fmt_single_header, 1, sizeof(fmt_single_header), fp_single_output) != sizeof(fmt_single_header) ) {
					printf("Can't write WAV file chunk header. Exit.\n");
					goto EXIT;
				}
				if( fwrite(&fmt_single_body, 1, fmt_single_header.size, fp_single_output) != fmt_single_header.size ) {
					printf("Can't write WAV file chunk body. Exit.\n");
					goto EXIT;
				}
				if( fwrite(&data_single_header, 1, sizeof(data_single_header), fp_single_output) != sizeof(data_single_header) ) {
					printf("Can't write WAV file data chunk. Exit.\n");
					goto EXIT;
				}
				if( fwrite(singla_channel_dump, 1, single_channel_size, fp_single_output) != single_channel_size ) {
					printf("Can't write WAV file pcm data. Exit.\n");
					goto EXIT;
				}
				printf("Done. WAV file writing in %s .\n", filename);
			}
		}

	}

EXIT:
	if(raw_dump != NULL) {
		free(raw_dump);
		raw_dump=NULL;
	}
	if(singla_channel_dump != NULL) {
		free(singla_channel_dump);
		singla_channel_dump=NULL;
	}
	if(fp_pcm_data != NULL) {
		fclose(fp_pcm_data);
		fp_pcm_data = NULL;
	}
	if(fp_single_output != NULL) {
		fclose(fp_single_output);
		fp_single_output = NULL;
	}
	if(fp_output != NULL) {
		fclose(fp_output);
		fp_output = NULL;
	}
	if(fp != NULL) {
		fclose(fp);
		fp = NULL;
	}
	
	return 0;
}

