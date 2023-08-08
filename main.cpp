#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "arch.h"
#include "utility.h"
#include "wave.h"
#include "wave_type.h"
#include "param.h"
#include "g711PlcMain.h"

// reference
// https://codereview.stackexchange.com/questions/222026/parse-a-wav-file-and-export-pcm-data
// https://www.mmsp.ece.mcgill.ca/Documents/AudioFormats/WAVE/WAVE.html

// -------------------------------------------------- functions --------------------------------------------------
/**
 * @brief
 * simulate the data lost and compensation process
 * 192K, lost 8 samples
 * 96K,  lost 4 samples
 * 48K,  lost 2 samples
 */
void Model_DataLostAndCompensation(void) {

	/*
	Note.
	BitPerSample : 1~8 bits, unsigned value
	BitPerSample : > 8 bits, signed value

	CONTINUOUS TYPE
	| buffer in bytes units
	|                           |<----- lostPts ----->|                                |<----- lostPts ----->|                                |<----- lostPts ----->|                                  ...
	|<----- initial phase ----->|<-------------------- lostPeriod -------------------->|<-------------------- lostPeriod -------------------->|<-------------------- lostPeriod -------------------->| ...

	INTERLEAVE TYPE
	| buffer in bytes units
	|                           |<-- interleave lost section             -->|          |<-- interleave lost section             -->|          |<-- interleave lost section             -->|
	|                           |<- xxxx ->|          |<- xxxx ->|          |          |<- xxxx ->|          |<- xxxx ->|          |          |<- xxxx ->|          |<- xxxx ->|          |            ...
	|<----- initial phase ----->|<-------------------- lostPeriod -------------------->|<-------------------- lostPeriod -------------------->|<-------------------- lostPeriod -------------------->| ...
	*/

	uint32_t initialPhase = 0; // unit : sec
	uint32_t lostPeriod = fmt_single_body.sample_rate * fmt_single_body.bit_per_sample / 8; // unit : 1sec as bytes

	uint32_t lostSample = 0; // unit : sample
	uint32_t lostILSample = 0; // unit : sample

	uint32_t lostPts = 0; // unit : bytes
	uint32_t lostILPts = 0; // unit: bytes

	uint32_t randomOffset = 0;

	// basic lost parameter
	if( fmt_single_body.sample_rate <= 48000 ) {
		lostSample = 2;
		lostILSample = 1;
	} else if( fmt_single_body.sample_rate <= 96000 ) {
		lostSample = 4;
		lostILSample = 2;
	} else {
		lostSample = 8;
		lostILSample = 4;
	}

	// manual tuning
	lostSample = lostSample * Manual_lost_sample_ratio;
	initialPhase = ( fmt_single_body.bit_per_sample / 8 ) * Manual_lost_start_sample;
	lostPeriod = lostPeriod / Manual_lost_period_ratio;

	// necessary parameter
	lostPts = lostSample * fmt_single_body.bit_per_sample / 8;
	lostILPts = lostILSample * fmt_single_body.bit_per_sample / 8;

	// print information
	printf("-----[ data lost simulation ]-----\n");
	printf("initial phase : %d bytes = %d samples\n", initialPhase, (initialPhase*8)/fmt_single_body.bit_per_sample);
	printf("single_channel_size : %d bytes\n", single_channel_size);
	printf("lost period : %d bytes\n", lostPeriod);
	printf("lost sample : %d samples\n", lostSample);
	printf("interleave sample : %d samples\n", lostILSample);
	printf("lost bytes : %d bytes\n", lostPts);
	printf("interleave bytes : %d bytes\n", lostILPts);
	printf("lost type : %s\n", losttype_name[lostMethod]);
	printf("random offset enable : %d\n", lostRandomOffsetEnable);
	printf("compensation type : %s\n", comptype_name[compMethod]);

	if( lostRandomOffsetEnable != 0 ) {
		srand(time(NULL));
	}

	// data lost
	if( lostMethod == LOSTTYPE_CONTINUOUS ) {
		for( uint32_t i=initialPhase; i<single_channel_size; i=i+lostPeriod ) {
			if( lostRandomOffsetEnable != 0 ) {
				randomOffset = rand() % (randomOffsetMax*(fmt_single_body.bit_per_sample/8));
			} else {
				randomOffset = 0;
			}
			memset(single_channel_dump+i+randomOffset, 0x0, lostPts);
		}
	} else if( lostMethod == LOSTTYPE_INTERLEAVE ) {
		for( uint32_t i=initialPhase; i<single_channel_size; i=i+lostPeriod ) {
			uint32_t currentIdx = 0;
			for(uint32_t ProcTimes=0; ProcTimes*lostILSample<lostSample; ProcTimes++) {
				currentIdx = i + ProcTimes*(lostILSample*2)*(fmt_single_body.bit_per_sample/8);
				memset(single_channel_dump+currentIdx, 0x0, lostILSample*(fmt_single_body.bit_per_sample/8));
			}
		}
	} else if( lostMethod == LOSTTYPE_CONTINUOUS_FRAME ) {
		g711DataLost();
	}

	// compensation
	if( lostMethod != LOSTTYPE_NONE ) {
		if( compMethod == COMPTYPE_INNER_INTERPLOATION ) {
			if( lostMethod == LOSTTYPE_CONTINUOUS ) {
				for( uint32_t i=initialPhase; i<single_channel_size; i=i+lostPeriod ) {
					if( i > 0 ) {
						// find the interpolation endpoints
						sint32_t startPts = 0, endPts = 0;
						if( fmt_single_body.bit_per_sample == 8 ) {
							startPts = single_channel_dump[i-(fmt_single_body.bit_per_sample/8)];
							endPts = single_channel_dump[i+lostPts];
						} else if( fmt_single_body.bit_per_sample == 16 ) {
							sint16_t b16_val = 0;
							b16_val = *((sint16_t*)(&single_channel_dump[i-(fmt_single_body.bit_per_sample/8)]));
							startPts = b16_val;
							b16_val = *((sint16_t*)(&single_channel_dump[i+lostPts]));
							endPts = b16_val;
						} else if( fmt_single_body.bit_per_sample == 24 ) {
							startPts = b24_signed_to_b32_signed(&single_channel_dump[i-(fmt_single_body.bit_per_sample/8)]);
							endPts = b24_signed_to_b32_signed(&single_channel_dump[i+lostPts]);
						} else if( fmt_single_body.bit_per_sample == 32 ) {
							startPts = *((sint32_t*)(&single_channel_dump[i-(fmt_single_body.bit_per_sample/8)]));
							endPts = *((sint32_t*)(&single_channel_dump[i+lostPts]));
						}
						// printf("startPts = %d, endPts = %d\n", startPts, endPts);

						// inner interpolation
						sint32_t intp_value = 0;
						for( uint32_t j=0; j<lostSample; j++ ) {
							sint32_t a = endPts*(j+1);
							sint32_t b = startPts*(lostSample-j);
							sint32_t c = a + b;
							sint32_t intp_value = c/(sint32_t)(lostSample+1);
							// printf("[%2d] intp_value = %d\n", j, intp_value);
							if( fmt_single_body.bit_per_sample == 8 ) {
								single_channel_dump[i+j*1+0] = ((intp_value & 0x000000ff) >>  0);
							} else if( fmt_single_body.bit_per_sample == 16 ) {
								single_channel_dump[i+j*2+0] = ((intp_value & 0x000000ff) >>  0);
								single_channel_dump[i+j*2+1] = ((intp_value & 0x0000ff00) >>  8);
							} else if( fmt_single_body.bit_per_sample == 24 ) {
								single_channel_dump[i+j*3+0] = ((intp_value & 0x000000ff) >>  0);
								single_channel_dump[i+j*3+1] = ((intp_value & 0x0000ff00) >>  8);
								single_channel_dump[i+j*3+2] = ((intp_value & 0x00ff0000) >> 16);
							} else if( fmt_single_body.bit_per_sample == 24 ) {
								single_channel_dump[i+j*4+0] = ((intp_value & 0x000000ff) >>  0);
								single_channel_dump[i+j*4+1] = ((intp_value & 0x0000ff00) >>  8);
								single_channel_dump[i+j*4+2] = ((intp_value & 0x00ff0000) >> 16);
								single_channel_dump[i+j*4+2] = ((intp_value & 0xff000000) >> 24);
							}
						}
					}
				}
			} else if( lostMethod == LOSTTYPE_INTERLEAVE ) {
				for( uint32_t i=initialPhase; i<single_channel_size; i=i+lostPeriod ) {
					uint32_t currentIdx = 0;
					for(uint32_t ProcTimes=0; ProcTimes*lostILSample<lostSample; ProcTimes++) {

						currentIdx = i + ProcTimes*(lostILSample*2)*(fmt_single_body.bit_per_sample/8);

						// find the interpolation endpoints
						sint32_t startPts = 0, endPts = 0;
						if( fmt_single_body.bit_per_sample == 8 ) {
							startPts = single_channel_dump[currentIdx-(fmt_single_body.bit_per_sample/8)];
							endPts = single_channel_dump[currentIdx+lostILPts];
						} else if( fmt_single_body.bit_per_sample == 16 ) {
							sint16_t b16_val = 0;
							b16_val = *((sint16_t*)(&single_channel_dump[currentIdx-(fmt_single_body.bit_per_sample/8)]));
							startPts = b16_val;
							b16_val = *((sint16_t*)(&single_channel_dump[currentIdx+lostILPts]));
							endPts = b16_val;
						} else if( fmt_single_body.bit_per_sample == 24 ) {
							startPts = b24_signed_to_b32_signed(&single_channel_dump[currentIdx-(fmt_single_body.bit_per_sample/8)]);
							endPts = b24_signed_to_b32_signed(&single_channel_dump[currentIdx+lostILPts]);
						} else if( fmt_single_body.bit_per_sample == 32 ) {
							startPts = *((sint32_t*)(&single_channel_dump[currentIdx-(fmt_single_body.bit_per_sample/8)]));
							endPts = *((sint32_t*)(&single_channel_dump[currentIdx+lostILPts]));
						}
						// printf("startPts = %d, endPts = %d\n", startPts, endPts);

						// inner interpolation
						sint32_t intp_value = 0;
						for( uint32_t j=0; j<lostILSample; j++ ) {
							sint32_t a = endPts*(j+1);
							sint32_t b = startPts*(lostILSample-j);
							sint32_t c = a + b;
							sint32_t intp_value = c/(sint32_t)(lostILSample+1);
							// printf("[%2d] intp_value = %d\n", j, intp_value);
							if( fmt_single_body.bit_per_sample == 8 ) {
								single_channel_dump[currentIdx+j*1+0] = ((intp_value & 0x000000ff) >>  0);
							} else if( fmt_single_body.bit_per_sample == 16 ) {
								single_channel_dump[currentIdx+j*2+0] = ((intp_value & 0x000000ff) >>  0);
								single_channel_dump[currentIdx+j*2+1] = ((intp_value & 0x0000ff00) >>  8);
							} else if( fmt_single_body.bit_per_sample == 24 ) {
								single_channel_dump[currentIdx+j*3+0] = ((intp_value & 0x000000ff) >>  0);
								single_channel_dump[currentIdx+j*3+1] = ((intp_value & 0x0000ff00) >>  8);
								single_channel_dump[currentIdx+j*3+2] = ((intp_value & 0x00ff0000) >> 16);
							} else if( fmt_single_body.bit_per_sample == 24 ) {
								single_channel_dump[currentIdx+j*4+0] = ((intp_value & 0x000000ff) >>  0);
								single_channel_dump[currentIdx+j*4+1] = ((intp_value & 0x0000ff00) >>  8);
								single_channel_dump[currentIdx+j*4+2] = ((intp_value & 0x00ff0000) >> 16);
								single_channel_dump[currentIdx+j*4+2] = ((intp_value & 0xff000000) >> 24);
							}
						}

					}
				}
			}
		} else if( compMethod == COMPTYPE_G711_VOIP ) {
			g711PlcMain();
		}
	}

}

int single_file_processing(void) {

	// ----------------------------------------------------------------------------------------------------
	// open file
	sprintf(filename, "input/%s/%s.wav", InputFileFolder[gFileSelection], InputFileName[gFileSelection]);
	if ((fp = fopen(filename, "rb"))==NULL) {
		printf("Can't open the file. Exit.\n");
		goto EXIT;
	}
	printf("processing %s\n", filename);

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
			if( gFlow_dump_raw_pcm != 0 ) {
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
		}
	} else {
		printf("format tag is not PCM. Exit.\n");
		goto EXIT;
	}

	// ----------------------------------------------------------------------------------------------------
	// Package wave file
	if( gFlow_dump_original_wav != 0 ) {
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
		if( gFlow_dump_single_channel_header == 0 ) {
			fmt_single_header.size = 16;
			fmt_single_body.format_tag = WAVE_FORMAT_PCM;
		} else if( gFlow_dump_single_channel_header == 1 ) {
			fmt_single_header.size = 40;
			fmt_single_body.format_tag = WAVE_FORMAT_EXTENSIBLE;
		}
		riff_single.size = BASIC_HEADER_SIZE + fmt_single_header.size + single_channel_size;
		fmt_single_body.channels = 1;
		fmt_single_body.byte_per_sec = (fmt_single_body.sample_rate * fmt_single_body.bit_per_sample) / (8 * fmt_body.channels);
		fmt_single_body.block_align = fmt_single_body.block_align / fmt_body.channels;
		fmt_single_body.channel_mask = MASK_SPEAKER_FRONT_LEFT;
		data_single_header.size = single_channel_size;

		// show signle file information
		message_show_body(fmt_single_header, fmt_single_body);

		single_channel_dump = (uint8_t*)malloc(single_channel_size);
		uint32_t singla_channel_dump_idx = 0;
		for( uint8_t ch=0; ch<fmt_body.channels; ch++ ) {

			// separate single channel data
			singla_channel_dump_idx = 0;
			for( uint32_t idx=0; idx<data_header.size; idx=idx+sample_size_per_group ) {
				for( uint32_t sample_idx=0; sample_idx<(fmt_body.bit_per_sample/8); sample_idx++ ) {
					single_channel_dump[singla_channel_dump_idx++] = raw_dump[idx + ch*(fmt_body.bit_per_sample/8) + sample_idx];
				}
			}

			// simulation data lost
			Model_DataLostAndCompensation();

			// write pcm data
			if( (gFlow_dump_single_channel_pcm == 1 && ch == 0) || ( gFlow_dump_single_channel_pcm == 2 ) ) {
				sprintf(filename, "output/MY_%s_%s_pcm.raw", InputFileName[gFileSelection], channel_name[get_speaker_mask_idx(fmt_body.channel_mask, ch)]);
				if( (fp_pcm_data = fopen(filename, "wb")) == NULL ) {
					printf("Can't open the single channel raw PCM file for write. Exit.\n");
					goto EXIT;
				}
				if( fwrite(single_channel_dump, 1, single_channel_size, fp_pcm_data) != single_channel_size ) {
					printf("Can't write single channel raw PCM file. Exit.\n");
					goto EXIT;
				}
				printf("Done. PCM data writing in %s .\n", filename);
				if( fp_pcm_data != NULL ) {
					fclose(fp_pcm_data);
					fp_pcm_data = NULL;
				}
			}

			// write wav data
			if( (gFlow_dump_single_channel == 1 && ch == 0) || ( gFlow_dump_single_channel == 2 ) ) {

				sprintf(filename, "output/MY_%s_%s.wav", InputFileName[gFileSelection], channel_name[get_speaker_mask_idx(fmt_body.channel_mask, ch)]);
				if( (fp_single_output = fopen(filename, "wb")) == NULL ) {
					printf("Can't open the new WAV file for write. Exit.\n");
				} else {
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
					if( fwrite(single_channel_dump, 1, single_channel_size, fp_single_output) != single_channel_size ) {
						printf("Can't write WAV file pcm data. Exit.\n");
						goto EXIT;
					}
					printf("Done. WAV file writing in %s .\n", filename);
					if( fp_single_output != NULL ) {
						fclose(fp_single_output);
						fp_single_output = NULL;
					}
				}
			}
		}

	}

EXIT:
	if( raw_dump != NULL ) {
		free(raw_dump);
		raw_dump=NULL;
	}
	if( single_channel_dump != NULL ) {
		free(single_channel_dump);
		single_channel_dump=NULL;
	}
	if( fp_pcm_data != NULL ) {
		fclose(fp_pcm_data);
		fp_pcm_data = NULL;
	}
	if( fp_single_output != NULL ) {
		fclose(fp_single_output);
		fp_single_output = NULL;
	}
	if( fp_output != NULL ) {
		fclose(fp_output);
		fp_output = NULL;
	}
	if( fp != NULL ) {
		fclose(fp);
		fp = NULL;
	}

	return 0;
}

int main(void) {
	for(gFileSelection=0; gFileSelection<=16; gFileSelection++) {
		single_file_processing();
	}
}