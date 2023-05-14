#ifndef __AUDIO__HH
#define __AUDIO__HH

// C++ standard libraries
#include <iostream>
#include <fstream>
#include <cassert>
#include <string>
#include <memory>
#include <limits>

// ffmpeg libraries
extern "C" {
	#include <libavcodec/avcodec.h>
	#include <libavformat/avformat.h>
}

class Audio {
	public:
		// Constructors
		Audio();
		Audio(const std::string& _fname);

		// Destructor
		~Audio();

		void load_audio(const std::string& _fname);

		void decode();

		// Accessors
		//int get_sample_rate();
		//int get_bit_depth();

		void print_metadata();

	private:
		// file metadata
		uint64_t     m_duration;
		uint64_t     m_bit_rate;
		int			 m_bit_per_sample;
		int			 m_sample_rate;
		int			 m_stream;
		int			 m_channels;
		unsigned int m_nb_streams;
		std::string  m_codec_name;

		float get_sample(int sample, int ch);

		// AV objects
		AVFormatContext   *av_header;
		AVCodecContext    *av_codec_con;
		AVPacket		  *av_packet;
		AVFrame			  *av_frame;
};
#endif
