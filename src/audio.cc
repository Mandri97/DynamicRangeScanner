#include "audio.hh"

Audio::Audio() : 
	m_duration(0), m_bit_rate(0), m_sample_rate(0), m_nb_streams(0), m_codec_name(""),
	av_header(nullptr), av_codec_con(nullptr) , av_frame(nullptr), av_packet(nullptr) {}

Audio::Audio(const std::string& _fname) : 
	m_duration(0), m_bit_rate(0), m_sample_rate(0), m_nb_streams(0), m_codec_name(""),
	av_header(nullptr), av_codec_con(nullptr), av_frame(nullptr), av_packet(nullptr) {
	load_audio(_fname);
}

Audio::~Audio(){
	m_codec_name = "";

	if (this->av_header)    avformat_close_input(&av_header);
	if (this->av_codec_con) avcodec_free_context(&av_codec_con);
	if (this->av_packet)    av_packet_free(&av_packet);
	if (this->av_frame)     av_frame_free(&av_frame);
}

void Audio::load_audio(const std::string& _fname){
	// Allocate memory for the header
	av_header = avformat_alloc_context();

	if (avformat_open_input(&av_header, _fname.data(), nullptr, nullptr) != 0) {
		fprintf(stderr, "Couldn't open %s\n", _fname.data());

		// TODO: DEFINE AN ERROR STATUS
		exit(EXIT_FAILURE);
	}

	// Accessing streams
	m_nb_streams = av_header->nb_streams;
	assert(m_nb_streams > 0);

	int audio_stream = -1;

	for (unsigned int i = 0; i < m_nb_streams; i++){
		// Audio codec only
		if (av_header->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
			if (audio_stream == -1) audio_stream = i;
		}
	}

	if (audio_stream < 0){
		std::cerr << "No audio codec detected\n";

		exit(EXIT_FAILURE);
	}

	assert(audio_stream >= 0);

	m_stream = audio_stream;

	AVCodecParameters *av_param = av_header->streams[m_stream]->codecpar;
	const AVCodec *av_codec = avcodec_find_decoder(av_param->codec_id);

	av_codec_con = avcodec_alloc_context3(av_codec);

	if (av_codec_con){
		if (avcodec_parameters_to_context(av_codec_con, av_param) == 0){
			if (avcodec_open2(av_codec_con, av_codec, nullptr) != 0){
				std::cerr << "Invalid or unsupported codec\n";

				exit(EXIT_FAILURE);
			}
		}
	}
	
	m_duration       = av_header->streams[m_stream]->duration * av_q2d(av_header->streams[m_stream]->time_base);
	m_bit_rate       = av_codec_con->bit_rate;
	m_bit_per_sample = av_codec_con->bits_per_raw_sample;
	m_codec_name     = std::string(av_codec->long_name);
	m_channels	     = av_codec_con->channels;

	if (!m_bit_rate) m_bit_rate = m_bit_per_sample;

	m_sample_rate    = av_codec_con->sample_rate;
}

void Audio::decode(){
	av_packet = av_packet_alloc();
	av_frame = av_frame_alloc();

	std::ofstream output("raw_audio", std::ios_base::binary);
	if (!output.is_open()){
		std::cerr << "Can't open outputfile\n";
		exit(EXIT_FAILURE);
	}

	// read audio packets
	while (true){
		// stop condition
		if (!av_packet) return;

		int ret = 0;
		// PCM and ADPCM returns non zero if successful
		// Skip packet until audio packet
		while ((ret = av_read_frame(av_header, av_packet)) >= 0){
			if (av_packet->stream_index == m_stream) break;

			av_packet_unref(av_packet);
		}

		// eof
		if (ret < 0) {
			av_packet_unref(av_packet);

			break;
		}

		// decode packet into frame
		ret = avcodec_send_packet(av_codec_con, av_packet);
		if (ret < 0) {
			std::cerr << "Can't decode packet\n";
			av_packet_unref(av_packet);

			exit(EXIT_FAILURE);
		}

		// read frames
		while ((ret = avcodec_receive_frame(av_codec_con, av_frame)) >= 0){
			if (ret < 0){
				std::cerr << "Error in frame decoding\n";
				av_frame_unref(av_frame);

				exit(EXIT_FAILURE);
			}

			for (int sample = 0; sample < av_frame->nb_samples; sample++){
				//for (int ch = 0; ch < m_channels; ch++){
				for (int ch = 0; ch < 1; ch++){
					//std::cout << sample << " " << get_sample(sample, ch) << std::endl;
					float value = get_sample(sample, ch);
					//buffer[ch][i] = value;

					output.write(reinterpret_cast<char*>(&value), sizeof(float));
				}
			}

			//output.write(reinterpret_cast<char*>(&buffer), sizeof(float));
		}
	}
	output.close();
}


// Buffer reallocation and code adapted 
float Audio::get_sample(int sample, int ch){
	// lock
	if (av_codec_con == nullptr){
		std::cerr << "Codec context object must be allocated!\n";

		exit(EXIT_FAILURE);
	}

	// Cast into AVSampleFormat enum type
	AVSampleFormat format = static_cast<AVSampleFormat>(av_frame->format);

	int     offset = 0;
	float   rv     = 0;
	uint8_t *data;

	// Planar vs Interleaved buffers	
	if (av_sample_fmt_is_planar(format)){
		data = av_frame->data[sample];
		offset = sample;
	} else {
		data = av_frame->data[0];
		offset = sample * m_channels;
	}

	// AV_SAMPLE_FMT_TYPE  is for interleaved format
	// AV_SAMPLE_FMT_TYPEP is for planar format
	switch (format){
		case AV_SAMPLE_FMT_S16:
		case AV_SAMPLE_FMT_S16P:
			rv = reinterpret_cast<uint16_t*>(data)[offset] / static_cast<float>(INT16_MAX);
			break;

		case AV_SAMPLE_FMT_S32:
		case AV_SAMPLE_FMT_S32P:
			rv = reinterpret_cast<uint32_t*>(data)[offset] / static_cast<float>(INT32_MAX);
			break;

		case AV_SAMPLE_FMT_FLT:
		case AV_SAMPLE_FMT_FLTP:
			rv = reinterpret_cast<float*>(data)[offset];
			break;
		
		case AV_SAMPLE_FMT_DBL:
		case AV_SAMPLE_FMT_DBLP:
			rv = reinterpret_cast<double*>(data)[offset];
			break;

		default:
			rv = 0.0f;
			break;
	}

	return rv;
}

void Audio::print_metadata(){
	std::cout << "Metadata" << std::endl;
	std::cout << "  + Codec: " << m_codec_name << "\n";
	std::cout << "  + Channels: " << m_channels << "\n";
	std::cout << "  + Duration: " << m_duration << " s\n";
	std::cout << "  + Bit depth: " << m_bit_rate << "\n";
	std::cout << "  + Sample rate: " << m_sample_rate << "\n";
}
