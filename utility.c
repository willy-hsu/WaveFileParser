#include <stdio.h>
#include <stdlib.h>
#include "wave_type.h"
#include "utility.h"
#include "arch.h"

/*-------------------- GLOBAL PARAMETER --------------------*/
char losttype_name[LOSTTYPE_MAX][32] = {
	"NONE",
	"CONTINUOUS",
	"INTERLEAVE",
};

char comptype_name[COMPTYPE_MAX][32] = {
	"NONE",
	"INNER_INTERPLOATION",
	"G711_VOIP",
};

uint32_t channel_mask[SPEAKER_NUM_MAX] = {
	MASK_SPEAKER_FRONT_LEFT,
	MASK_SPEAKER_FRONT_RIGHT,
	MASK_SPEAKER_FRONT_CENTER,
	MASK_SPEAKER_LOW_FREQUENCY,
	MASK_SPEAKER_BACK_LEFT,
	MASK_SPEAKER_BACK_RIGHT,
	MASK_SPEAKER_FRONT_LEFT_OF_CENTER,
	MASK_SPEAKER_FRONT_RIGHT_OF_CENTER,
	MASK_SPEAKER_BACK_CENTER,
	MASK_SPEAKER_SIDE_LEFT,
	MASK_SPEAKER_SIDE_RIGHT,
	MASK_SPEAKER_TOP_CENTER,
	MASK_SPEAKER_TOP_FRONT_LEFT,
	MASK_SPEAKER_TOP_FRONT_CENTER,
	MASK_SPEAKER_TOP_FRONT_RIGHT,
	MASK_SPEAKER_TOP_BACK_LEFT,
	MASK_SPEAKER_TOP_BACK_CENTER,
	MASK_SPEAKER_TOP_BACK_RIGHT,
};

char channel_name[SPEAKER_NUM_MAX][32] = {
	/*  0 */ "FRONT_LEFT",
	/*  1 */ "FRONT_RIGHT",
	/*  2 */ "FRONT_CENTER",
	/*  3 */ "LOW_FREQUENCY",
	/*  4 */ "BACK_LEFT",
	/*  5 */ "BACK_RIGHT",
	/*  6 */ "FRONT_LEFT_OF_CENTER",
	/*  7 */ "FRONT_RIGHT_OF_CENTER",
	/*  8 */ "BACK_CENTER",
	/*  9 */ "SIDE_LEFT",
	/* 10 */ "SIDE_RIGHT",
	/* 11 */ "TOP_CENTER",
	/* 12 */ "TOP_FRONT_LEFT",
	/* 13 */ "TOP_FRONT_CENTER",
	/* 14 */ "TOP_FRONT_RIGHT",
	/* 15 */ "TOP_BACK_LEFT",
	/* 16 */ "TOP_BACK_CENTER",
	/* 17 */ "TOP_BACK_RIGHT",
};

#define IS_NEGATIVE(INPUT, BITWIDTH) ( ( (INPUT)>=(1<<(BITWIDTH-1)) ) ? 1 : 0)
#define SIGNED_INVERSE(INPUT, BITWIDTH) ( (-((1<<(BITWIDTH)) - (INPUT))) )

/*-------------------- DATA/STRING PROCESSING FUNCTIONS --------------------*/
/**
 * @brief 
 * convert a char array to string array with terminator
 * @param Dest : pointer to the destination array as string
 * @param Src : pointer to the source char array
 * @param size : string size
 */
void Arr2String(char *Dest, char *Src, uint8_t size ) {
	strncpy(Dest, Src, size);
	Dest[size] = '\0';
}

/**
 * @brief 
 * convert 24bit signed value to 32bit signed value to a parameter
 * @param pBuf : pointer to a 8bit PCM data
 * @return sint32_t : single sample value
 */
sint32_t b24_signed_to_b32_signed(uint8_t *pBuf) {
	sint32_t ret = 0;
	ret = ( pBuf[2] << 16 ) + ( pBuf[1] << 8 ) + ( pBuf[0] );
	if( IS_NEGATIVE(ret, 24) ) {
		ret = SIGNED_INVERSE(ret, 24);
	}
	return ret;
}

/*-------------------- FUNCTIONS --------------------*/
/*
The channels specified in dwChannleMask must be present in the prescribed order (from least significant bit up).
In other words, if only Front Left and Front Center are specified, then Front Left should come first in the interleaved stream
nChannels < bits set in dwChannelMask : extra (most significant) bits are ignored
nChannels > bits set in dwChannelMask : remaining channels are not assigned to any particular speaker, render to output ports not in use
*/
void message_speaker_mask(uint32_t input) {
	uint8_t idx=0;
	for( idx=0; idx<SPEAKER_NUM_MAX ; idx++ ) {
		if( (input & channel_mask[idx]) != 0 ) {
			printf("\t> %s\n", channel_name[idx]);
		}
	}
}

uint8_t get_speaker_mask_num(uint32_t input) {
	uint8_t speaker_num = 0;
	uint8_t idx=0;
	for( idx=0; idx<SPEAKER_NUM_MAX ; idx++ ) {
		if( (input & channel_mask[idx]) != 0 ) {
			speaker_num++;
		}
	}
	return speaker_num;
}

uint8_t get_speaker_mask_idx(uint32_t input, uint8_t sequence_number) {
	uint8_t idx=0;
	for( idx=0; idx<SPEAKER_NUM_MAX ; idx++ ) {
		if( sequence_number == 0 ) {
			break;
		}
		if( (input & channel_mask[idx]) != 0 ) {
			sequence_number--;
		}
	}

	if( sequence_number != 0 ) {
		return 0xff;
	} else {
		return idx;
	}
}

void message_format(uint16_t input) {
	printf("Format: 0x%04x ", input);
	if     ( (input == WAVE_FORMAT_PCM        ) !=0 ) { printf("WAVE_FORMAT_PCM\n"); }
	else if( (input == WAVE_FORMAT_IEEE_FLOAT ) !=0 ) { printf("WAVE_FORMAT_IEEE_FLOAT\n"); }
	else if( (input == WAVE_FORMAT_ALAW       ) !=0 ) { printf("WAVE_FORMAT_ALAW\n"); }
	else if( (input == WAVE_FORMAT_MULAW      ) !=0 ) { printf("WAVE_FORMAT_MULAW\n"); }
	else if( (input == WAVE_FORMAT_EXTENSIBLE ) !=0 ) { printf("WAVE_FORMAT_EXTENSIBLE\n"); }
}

void message_show_body(fmt_chunk_header fmt_header, fmt_chunk_body fmt_body) {
	printf("----------[show format body]----------\n");
	message_format(fmt_body.format_tag);
	printf("Channels: %d\n", fmt_body.channels);
	printf("Sample Rate: %d blocks per second\n", fmt_body.sample_rate);
	printf("Byte/s: %d\n", fmt_body.byte_per_sec);
	printf("Block Align: %d\n", fmt_body.block_align);
	printf("Bit Rate: %d\n", fmt_body.bit_per_sample);
	if( fmt_header.size == 40 ) {
		printf("Extra Format Size: %d\n", fmt_body.extra_format_size);
		printf("Valid Bits Per Sample: %d\n", fmt_body.valib_bit_per_sample);
		printf("Channel Mask: 0x%04x\n", fmt_body.channel_mask);
		message_speaker_mask(fmt_body.channel_mask);
		if( get_speaker_mask_num(fmt_body.channel_mask) != fmt_body.channels )  {
			printf("channel number does not match speaker mask!\n");
		}
		printf("GUID: ");
		for(uint8_t i=0; i<16; i++) {
			printf("0x%02x ", (unsigned char)fmt_body.sub_format[i]);
		}
		printf("\n");
		printf("Format in GUID ");
		message_format(*(uint16_t *)fmt_body.sub_format);
	}
	printf("\n");
}