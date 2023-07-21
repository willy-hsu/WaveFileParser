#ifndef _WAVE_H_
#define _WAVE_H_

#include "arch.h"

typedef struct riff_chunk {
    sint8_t  id[4];             // 4b "RIFF" string  >|
    uint32_t size;              // 4b                 |-> 12 byte
    sint8_t  type[4];           // 4b "WAVE" string  >|
} riff_chunk;

typedef struct fmt_chunk_header {
	sint8_t  id[4];             // 4b "fmt" string
    uint32_t size;              // 4b
} fmt_chunk_header;

typedef struct fmt_chunk_body {
	// 16 Bytes
    uint16_t format_tag;        // 2b
    uint16_t channels;          // 2b
    uint32_t sample_rate;       // 4b Sampling rate (blocks per second)
    uint32_t byte_per_sec;      // 4b Data rate
    uint16_t block_align;       // 2b Data block size (bytes)
    uint16_t bit_per_sample;    // 2b Bits per sample
    // extra format
    uint16_t extra_format_size;    // 2b Size of the extension(0 or 22)
    uint16_t valib_bit_per_sample; // 2b Number of valid bits
    uint32_t channel_mask;         // 4b Speaker position mask
    uint8_t  sub_format[16];       // 16b GUID, including the data format code
} fmt_chunk_body;

// struct internal_info {
// 	uint32_t 
// };

struct data_chunk {
    sint8_t  id[4]; // 4 "data" string
    uint32_t size;  // 4
};

#endif
