#include "include/VideoInput.h"

/**================= PUBLIC METHODS ===================*/

VideoInput::~VideoInput() {
    avformat_close_input(&this->inputContext);
}

AVPixelFormat VideoInput::getPixelFormat() {
	return std::get<1>(this->stream)->native()->pix_fmt;
}

std::future<void> VideoInput::launchRecordThread(bool* isStopped, bool* onPause) {
	return std::async(std::launch::async, [this, isStopped, onPause] { this->record(isStopped, onPause); });
}

std::shared_ptr<VideoInput> VideoInput::getInputReader(const int width, const int height, const int offset_x, const int offset_y, std::shared_ptr<av::StreamWriter> writer) {
	std::shared_ptr<VideoInput> res{ new VideoInput{} };
	if (!res->init(width, height, offset_x, offset_y, writer))
		return nullptr;
	return res;
}

/**================= PRIVATE METHODS ===================*/

VideoInput::VideoInput() {
	this->inputContext = nullptr;
	this->inputFormat = nullptr;
	this->opts = nullptr;
	this->writer = nullptr;
}

bool VideoInput::init(const int width, const int height, const int offset_x, const int offset_y, std::shared_ptr<av::StreamWriter> writer) {
	this->writer = writer;
	this->inputContext = avformat_alloc_context();
#if WIN32
	this->inputFormat = av_find_input_format("gdigrab");
#else
	this->inputFormat = av_find_input_format("x11grab");
#endif
	std::string size = std::to_string(width) + "x" + std::to_string(height);
	//av_dict_set(&this->opts, "rtbufsize", "1024M", 0);
	//av_dict_set(&this->opts, "bit_rate", "40000", 0);
	av_dict_set(&this->opts, "framerate", "15", 0);
	av_dict_set(&this->opts, "video_size", size.c_str(), 0);
#if WIN32
	av_dict_set(&this->opts, "offset_x", std::to_string(offset_x).c_str(), 0);
    av_dict_set(&this->opts, "offset_y", std::to_string(offset_y).c_str(), 0);
	auto err = avformat_open_input(&this->inputContext, "desktop", this->inputFormat, &this->opts);
#else
    av_dict_set(&this->opts, "grab_x", std::to_string(offset_x).c_str(), 0);
    av_dict_set(&this->opts, "grab_y", std::to_string(offset_y).c_str(), 0);
	auto err = avformat_open_input(&this->inputContext, "", this->inputFormat, &this->opts);
#endif
	if (err < 0) {
		std::cerr << "Cannot open video input 'desktop': " << av::avErrorStr(err) << std::endl;
		return false;
	}
	err = avformat_find_stream_info(this->inputContext, nullptr);
	if (err < 0) {
		avformat_close_input(&this->inputContext);
		std::cerr << "Cannot find video stream info: " << av::avErrorStr(err) << std::endl;
		return false;
	}
	if (!this->findBestStream(AVMEDIA_TYPE_VIDEO)) {
		std::cerr << "Can't create the video stream" << std::endl;
		return false;
	}

	av_dump_format(this->inputContext, 0, nullptr, 0);
	return true;
}

bool VideoInput::findBestStream(AVMediaType type) {
	AVCodec* dec = nullptr;
	int stream_i = av_find_best_stream(this->inputContext, type, -1, -1, &dec, 0);
	if (stream_i == AVERROR_STREAM_NOT_FOUND) {
		std::cerr << "Failed to find " << av_get_media_type_string(type) << " stream in 'desktop'" << std::endl;
		return false;
	}
	if (stream_i == AVERROR_DECODER_NOT_FOUND) {
		std::cerr << "Failed to find decoder '" << avcodec_get_name(this->inputContext->streams[stream_i]->codecpar->codec_id) << "' of 'desktop'" << std::endl;
		return false;
	}

    const auto framerate = av_guess_frame_rate(this->inputContext, this->inputContext->streams[stream_i], nullptr);
    auto decContext = av::Decoder::create(dec, this->inputContext->streams[stream_i], framerate);
    if (!decContext) {
        std::cerr << "Can't create decoder context" << std::endl;
        return false;
    }
    std::get<0>(this->stream) = this->inputContext->streams[stream_i];
    std::get<1>(this->stream) = decContext.value();

    return true;
}

bool VideoInput::readPacket(av::Packet& packet) {
	int err = 0;
	while (true) {
		err = av_read_frame(this->inputContext, *packet);

		if (err == AVERROR(EAGAIN))
			continue;

		if (err == AVERROR_EOF)
		{
			// flush cached frames from video decoder
			packet.native()->data = nullptr;
			packet.native()->size = 0;

			return false;
		}

		if (err < 0) {
			std::cerr << "Failed to read frame: " << av::avErrorStr(err) << std::endl;
			return false;
		}

		return true;
	}
}

bool VideoInput::readFrame(av::Frame& frame) {
	av::Packet packet;

	while (true) {
		packet.dataUnref();
		auto successExp = this->readPacket(packet);
		if (!successExp)
			std::cerr << "Can't read packet" << std::endl;

		shared_ptr<av::Decoder> dec;
		//Video Stream
		if (packet.native()->stream_index == get<0>(this->stream)->index) {
			dec = std::get<1>(this->stream);
            av_packet_rescale_ts(*packet, std::get<0>(this->stream)->time_base, dec->native()->time_base);
			auto resExp = dec->decode(packet, frame);

			if (!resExp) {
				std::cerr << "Can't decode video packet" << std::endl;
				return false;
			}

			if (resExp.value() != av::Result::kSuccess)
				continue;

			frame.type(AVMEDIA_TYPE_VIDEO);

			return true;
		} else {
			std::cerr << "Unknown stream index " << packet.native()->stream_index << std::endl;
			continue;
		}
	}
	return false;
}

void VideoInput::record(bool* isStopped, const bool* onPause) {
	av::Frame frame;
	int nFrames = 0;
	while (true) {
        if (*isStopped) //Check if the recording is stopped
			return;

		if(*onPause) { //Check if the recording is paused
			std::unique_lock <std::mutex> lk{ ThreadStructures::getSingleton().getMutex() };
			ThreadStructures::getSingleton().getConditionVariable().wait(lk, [onPause, isStopped] { return !*onPause || *isStopped; });
			if (*isStopped)
				return;
		}

		if (!this->readFrame(frame)) {
			*isStopped = true;
			return;
		}
		assertExpected(this->writer->write(frame, 0));
		nFrames++;
		if (nFrames % 10 == 0)
			std::cout << "Wrote " << nFrames << " video frames" << std::endl;
	}
}
