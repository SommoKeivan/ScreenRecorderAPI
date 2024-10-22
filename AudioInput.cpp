#include "include/AudioInput.h"

/**================= PUBLIC METHODS ===================*/

// Destructor to close the input context
AudioInput::~AudioInput() {
    avformat_close_input(&this->inputContext);
}

// Reads a frame of audio data
bool AudioInput::readFrame(av::Frame& frame) {
    av::Packet packet;

    while (true) {
        packet.dataUnref(); // Unreference any existing data in the packet
        auto successExp = this->readPacket(packet); // Read a packet from input
        if (!successExp) {
            std::cerr << "Can't read packet" << std::endl; // Error reading packet
        }

        shared_ptr<av::Decoder> dec;

        // Check if the packet belongs to the audio stream
        if (std::get<0>(this->stream) && packet.native()->stream_index == std::get<0>(this->stream)->index) {
            dec = std::get<1>(this->stream);
            av_packet_rescale_ts(*packet, std::get<0>(this->stream)->time_base, dec->native()->time_base); // Rescale packet timestamps
            auto resExp = dec->decode(packet, frame); // Decode the packet into a frame

            if (!resExp) {
                std::cerr << "Can't decode audio packet" << std::endl; // Error decoding packet
                return false;
            }

            if (resExp.value() != av::Result::kSuccess) {
                continue; // Continue if decoding was not successful
            }

            frame.type(AVMEDIA_TYPE_AUDIO); // Set frame type to audio

            return true; // Successfully read an audio frame
        } else {
            std::cerr << "Unknown stream index " << packet.native()->stream_index << std::endl; // Unrecognized stream index
            continue;
        }
    }
    return false; // Return false if no audio frame could be read
}

// Get the number of audio channels
int AudioInput::getChannelsNumber() {
    return std::get<1>(this->stream)->native()->channels;
}

// Get the audio sample format
AVSampleFormat AudioInput::getSampleFormat() {
    return (AVSampleFormat)std::get<1>(this->stream)->native()->sample_fmt;
}

// Get the audio bitrate
int64_t AudioInput::getBitRate() {
    return std::get<1>(this->stream)->native()->bit_rate;
}

// Get the audio sample rate
int AudioInput::getSampleRate() {
    return std::get<1>(this->stream)->native()->sample_rate;
}

// Launch the recording thread asynchronously
std::future<void> AudioInput::launchRecordThread(bool* isStopped, bool* onPause) {
    return std::async(std::launch::async, [this, isStopped, onPause] { this->record(isStopped, onPause); });
}

// Create an AudioInput instance for reading audio
std::shared_ptr<AudioInput> AudioInput::getAudioReader(std::shared_ptr<av::StreamWriter> writer) {
    std::shared_ptr<AudioInput> res{ new AudioInput{} };
    if (!res->init(writer)) {
        return nullptr; // Return nullptr if initialization fails
    }
    return res; // Return the created AudioInput instance
}

/**================= PRIVATE METHODS ===================*/

// Constructor initializing member variables
AudioInput::AudioInput() {
    this->inputContext = nullptr;
    this->inputFormat = nullptr;
    this->opts = nullptr;
    this->writer = nullptr;
}

// Reads a packet from the audio input
bool AudioInput::readPacket(av::Packet& packet) {
    int err = 0;
    while (true) {
        err = av_read_frame(this->inputContext, *packet); // Attempt to read a frame

        if (err == AVERROR(EAGAIN)) {
            continue; // Retry if the input is not ready
        }

        if (err == AVERROR_EOF) {
            // Flush cached frames from video decoder
            packet.native()->data = nullptr;
            packet.native()->size = 0;

            return false; // End of stream reached
        }

        if (err < 0) {
            std::cerr << "Failed to read frame: " << av::avErrorStr(err) << std::endl; // Error reading frame
            return false;
        }

        return true; // Successfully read a packet
    }
}

// Initialize the AudioInput with a StreamWriter
bool AudioInput::init(std::shared_ptr<av::StreamWriter> writer) {
    this->writer = writer;
    this->inputContext = avformat_alloc_context(); // Allocate input context
    if (!this->openInput()) {
        return false; // Return false if input opening fails
    }
    return true; // Return true if initialized successfully
}

// Find the best audio stream in the input context
bool AudioInput::findBestStream(AVMediaType type) {
    AVCodec* dec = nullptr;
    int stream_i = av_find_best_stream(this->inputContext, type, -1, -1, &dec, 0);
    if (stream_i == AVERROR_STREAM_NOT_FOUND) {
        std::cerr << "Failed to find " << av_get_media_type_string(type) << " stream in 'desktop'" << std::endl; // Stream not found
        return false;
    }
    if (stream_i == AVERROR_DECODER_NOT_FOUND) {
        std::cerr << "Failed to find decoder '" << avcodec_get_name(this->inputContext->streams[stream_i]->codecpar->codec_id) << "' of 'desktop'" << std::endl; // Decoder not found
        return false;
    }

    auto decContext = av::Decoder::create(dec, this->inputContext->streams[stream_i]); // Create decoder context
    if (!decContext) {
        std::cerr << "Can't create decoder context" << std::endl; // Error creating decoder context
        return false;
    }

    std::get<0>(this->stream) = this->inputContext->streams[stream_i]; // Set stream
    std::get<1>(this->stream) = decContext.value(); // Set decoder

    return true; // Successfully found the best stream
}

// Record audio in a separate thread
void AudioInput::record(bool* isStopped, bool* onPause) {
    av::Frame frame;
    int nSample = 0;
    while (true) {
        if (*isStopped) { // Check if the recording is stopped
            return;
        }
        
        // TODO: Remove paused logic
//        if (*onPause) { // Check if the recording is paused
//            avformat_close_input(&this->inputContext);
//            std::unique_lock<std::mutex> lk{ ThreadStructures::getSingleton().getMutex() };
//            ThreadStructures::getSingleton().getConditionVariable().wait(lk, [onPause, isStopped] { return !*onPause || *isStopped; });
//            if (*isStopped) {
//                return;
//            }
//
//            if (!this->openInput()) {
//                return;
//            }
//        }

        if (!this->readFrame(frame)) { // Read an audio frame
            *isStopped = true; // Set stopped flag if reading fails
            return;
        }

        {
            lock_guard<std::mutex> lk{ThreadStructures::getSingleton().getMutex()}; // Lock for thread safety
            if (!*onPause) {
                assertExpected(this->writer->write(frame, 1)); // Write frame to the writer
                nSample += frame.native()->nb_samples; // Update sample count
                if (nSample % 100 == 0) {
                    std::cout << "Wrote " << nSample << " audio samples" << std::endl; // Log every 100 samples
                }
            }
        }
    }
}

// Open the audio input
bool AudioInput::openInput() {
    this->opts = nullptr; // Initialize options
    auto val = av_dict_set(&this->opts, "async", "1", 0); // Set asynchronous reading
    if (val < 0) {
        std::cerr << "Error: cannot set audio sample rate" << std::endl; // Error setting sample rate
        return false;
    }

#if WIN32
    this->inputFormat = av_find_input_format("dshow"); // Set input format for Windows
#elif __linux__
    this->inputFormat = av_find_input_format("alsa"); // Set input format for Linux
#else
    this->inputFormat = av_find_input_format("avfoundation"); // Set input format for other systems (macOS)
#endif

#if WIN32
    auto err = avformat_open_input(&this->inputContext, "audio=virtual-audio-capturer", this->inputFormat, &this->opts); // Open audio input on Windows
#elif __linux__
    auto err = avformat_open_input(&this->inputContext, "default", this->inputFormat, &this->opts); // Open audio input on Linux
#else
    auto err = avformat_open_input(&this->inputContext, "0:0", this->inputFormat, &this->opts); // Open audio input on macOS
#endif

    if (err < 0) {
        std::cerr << "Cannot open audio input 'desktop': " << av::avErrorStr(err) << std::endl; // Error opening input
        return false;
    }

    err = avformat_find_stream_info(this->inputContext, nullptr); // Find stream info
    if
