/*
* This file adapts "mdxmini/pmdmini" and exports the relevant API to JavaScript, e.g. for
* use in my generic JavaScript player.
*
* Copyright (C) 2018 Juergen Wothke
*
*
* Credits:
*   
*  milk.K, K.MAEKAWA, Missy.M - authors of the original X68000 MXDRV
*  Daisuke Nagano - author of the Unix mdxplay
*  KAJIHARA Mashahiro - original author of the PMD sound driver for PC-9801
*  AGAWA Koji - Maintainer of PMDXMMS, on which pmdmini was based
*  M88 / cisc - author of OPNA FM sound generator 
*  BouKiCHi - author of the mdxmini&pmdmini library
*  Misty De Meo - bugfixes and improvements to mdxmini&pmdmini
*  
*  The project is based on: https://github.com/mistydemeo/mdxmini & https://github.com/mistydemeo/pmdmini
*
*
* License:
*
*  The code builds on FM Sound Generator with OPN/OPM interface Copyright (C) by cisc.
*  As the author of FM Sound Generator states (see readme.txt in "fmgen" sub-folder), this is free software but 
*  for any commercial application the prior consent of the author is explicitly required. So it probably 
*  cannot be GPL.. and the GPL claims made in the pdmmini project are probably just invalid. 
*  
*  Be that as it may.. I'll extend whatever license is actually valid for the underlying code to the 
*  code that I added here to turn "mdxmini/pmdmini" into a JavaScript lib.
*/

#include <emscripten.h>
#include <stdio.h>
#include <stdlib.h>     

#include <iostream>
#include <fstream>

extern "C" {
#include "mdxmini.h"
#include "../pmdmini/src/pmdmini.h"
}


#ifdef EMSCRIPTEN
#define EMSCRIPTEN_KEEPALIVE __attribute__((used))
#else
#define EMSCRIPTEN_KEEPALIVE
#endif


// hack to pass all those japaneese info texts safely to the JavaScript side:
static const std::string chars = 
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
std::string base64_encode(unsigned char* input, unsigned int len) {
  int i, j = 0;
  unsigned char arr3[3];
  unsigned char arr4[4];
  std::string ret;
  
  while (len--) {
    arr3[i++] = *(input++);
    if (i == 3) {
      arr4[0] = (arr3[0] & 0xfc) >> 2;
      arr4[1] = ((arr3[0] & 0x03) << 4) + ((arr3[1] & 0xf0) >> 4);
      arr4[2] = ((arr3[1] & 0x0f) << 2) + ((arr3[2] & 0xc0) >> 6);
      arr4[3] = arr3[2] & 0x3f;

      for(i = 0; (i <4) ; i++) {
        ret += chars[arr4[i]];
		}
      i = 0;
    }
  }
  if (i) {
    for(j = i; j < 3; j++) {
      arr3[j] = '\0';
	}
    arr4[0] = ( arr3[0] & 0xfc) >> 2;
    arr4[1] = ((arr3[0] & 0x03) << 4) + ((arr3[1] & 0xf0) >> 4);
    arr4[2] = ((arr3[1] & 0x0f) << 2) + ((arr3[2] & 0xc0) >> 6);

    for (j = 0; (j < i + 1); j++) {
      ret += chars[arr4[j]];
	}
    while((i++ < 3)) {
      ret += '=';
	}
  }
  return ret;
}

#define CHANNELS 2				
#define BYTES_PER_SAMPLE 2
#define SAMPLE_BUF_SIZE	1024
#define SAMPLE_FREQ	44100

int16_t sample_buffer[SAMPLE_BUF_SIZE * CHANNELS];
int samples_available= 0;

char* info_texts[2];

#define TEXT_MAX	1024
char title_str[TEXT_MAX];
char artist_str[TEXT_MAX];

#define RAW_INFO_MAX	1024
char raw_info_buffer[RAW_INFO_MAX];

struct StaticBlock {
    StaticBlock(){
		info_texts[0]= title_str;
		info_texts[1]= artist_str;
    }
};

static StaticBlock static_constructor;

int mdx_mode= 0;	// switch the two emulators..
t_mdxmini mdxmini;

int max_play_len= -1;
double play_len= 0;
int initialized= 0;

static void do_init() {
	if (mdx_mode) {	
		memset(&mdxmini, 0, sizeof(t_mdxmini));
		mdx_set_rate(SAMPLE_FREQ);	
	} else {
		pmd_init();
		pmd_setrate( SAMPLE_FREQ );
	}
	initialized= 1;
}

static void do_song_init(int len) {
	if (mdx_mode) {	
		if (len<0)
			len = mdx_get_length(&mdxmini);	// len in secs: use to stop playback

		mdx_set_max_loop(&mdxmini, 0);
		mdx_get_title(&mdxmini, raw_info_buffer);

	} else {
		if (len<0)
			len= pmd_length_sec ( );
		
		pmd_get_compo(raw_info_buffer);
		std::string encArtist= base64_encode((unsigned char*)raw_info_buffer, strlen(raw_info_buffer));
		snprintf(artist_str, TEXT_MAX, "%s", encArtist.c_str());

		pmd_get_title( raw_info_buffer );
	}
	std::string encTitle= base64_encode((unsigned char*)raw_info_buffer, strlen(raw_info_buffer));
	snprintf(title_str, TEXT_MAX, "%s", encTitle.c_str());

	max_play_len= len;
}

static int mdx_compute_samples() {
	if ((max_play_len > 0) && (play_len >= max_play_len)) return 1;	// stop song

	if (mdx_mode) {	
		mdx_calc_sample(&mdxmini, sample_buffer, SAMPLE_BUF_SIZE);
	} else {
		pmd_renderer ( sample_buffer, SAMPLE_BUF_SIZE );		
	}
	samples_available= SAMPLE_BUF_SIZE;
	play_len += ((double)samples_available)/SAMPLE_FREQ;
	return 0;
}

static void do_teardown() {
	if (initialized) {
		if (mdx_mode) {	
			mdx_close(&mdxmini);
		} else {
			pmd_stop();
		}
		initialized= 0;
	}
}

static int do_open(const char *filename) {
	if (mdx_mode) {
		return mdx_open(&mdxmini, (char*)filename, (char*) NULL);
	} else {
		return pmd_play (filename, (char*) NULL );
	}
}

static int file_open(const char *filename) {
	max_play_len= -1;
	play_len= 0;
	
	if (do_open(filename)) {
		printf("File open error: %s\n", filename);
		return 0;
	}
	return 1;
}

extern "C" void emu_teardown (void)  __attribute__((noinline));
extern "C" void EMSCRIPTEN_KEEPALIVE emu_teardown (void) {				
	title_str[0]= artist_str[0]= 0;
		
	do_teardown();
}

static int ends_with(std::string const & value, std::string const & ending) {
    if (ending.size() > value.size()) return 0;
    return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

extern "C"  int emu_load_file(char *filename, void * inBuffer, uint32_t inBufSize)  __attribute__((noinline));
extern "C"  int EMSCRIPTEN_KEEPALIVE emu_load_file(char *filename, void * inBuffer, uint32_t inBufSize) {
	emu_teardown();
	do_init();

	mdx_mode= ends_with(std::string(filename), std::string(".mdx"));
	
	if (!file_open(filename)) {
		// error
		return 1;
	} else {
		// success 
		int len= -1;
		do_song_init(len);
		return 0;					
	}
}

extern "C" int emu_get_sample_rate() __attribute__((noinline));
extern "C" EMSCRIPTEN_KEEPALIVE int emu_get_sample_rate() {
	return SAMPLE_FREQ;
}
 
extern "C" int emu_set_subsong(int subsong, unsigned char boost) __attribute__((noinline));
extern "C" int EMSCRIPTEN_KEEPALIVE emu_set_subsong(int track, unsigned char boost) {
	return 0;	// there are no subsongs...
}

extern "C" const char** emu_get_track_info() __attribute__((noinline));
extern "C" const char** EMSCRIPTEN_KEEPALIVE emu_get_track_info() {
	return (const char**)info_texts;
}

extern "C" char* EMSCRIPTEN_KEEPALIVE emu_get_audio_buffer(void) __attribute__((noinline));
extern "C" char* EMSCRIPTEN_KEEPALIVE emu_get_audio_buffer(void) {
	return (char*)sample_buffer;
}

extern "C" long EMSCRIPTEN_KEEPALIVE emu_get_audio_buffer_length(void) __attribute__((noinline));
extern "C" long EMSCRIPTEN_KEEPALIVE emu_get_audio_buffer_length(void) {
	return samples_available;
}

extern "C" int emu_compute_audio_samples() __attribute__((noinline));
extern "C" int EMSCRIPTEN_KEEPALIVE emu_compute_audio_samples() {
	return mdx_compute_samples();
}

extern "C" int emu_get_current_position() __attribute__((noinline));
extern "C" int EMSCRIPTEN_KEEPALIVE emu_get_current_position() {
	return -1;
}

extern "C" void emu_seek_position(int pos) __attribute__((noinline));
extern "C" void EMSCRIPTEN_KEEPALIVE emu_seek_position(int frames) {
}

extern "C" int emu_get_max_position() __attribute__((noinline));
extern "C" int EMSCRIPTEN_KEEPALIVE emu_get_max_position() {
	return -1;	// not implemented
}

