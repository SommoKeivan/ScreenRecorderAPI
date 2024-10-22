#ifndef AUDIO_INPUT
#define AUDIO_INPUT
#include <iostream>
#include <memory>
#include <future>
#include "../libav-cpp-master/av/common.hpp"
#include "../libav-cpp-master/av/Packet.hpp"
#include "../libav-cpp-master/av/Decoder.hpp"
#include "../libav-cpp-master/av/StreamWriter.hpp"
#include "ThreadStructures.h"

class AudioInput
{
	AVFormatContext* inputContext;
	AVInputFormat* inputFormat;
	AVDictionary* opts;
	tuple<AVStream*, shared_ptr<av::Decoder>> stream;
	std::shared_ptr<av::StreamWriter> writer;

	AudioInput();
	bool init(std::shared_ptr<av::StreamWriter> writer);
	bool findBestStream(AVMediaType type);
	bool readPacket(av::Packet& packet);
	bool readFrame(av::Frame& frame);
	void record(bool* isStopped, bool* isPaused);
    bool openInput();
public:
    /**
     * Destroyer.
     */
    ~AudioInput();
    /**
     * Gets the audio stream number of channels.
     * @return the audio number of channels.
     */
    int getChannelsNumber();
    /**
     * Gets the audio stream sample format.
     * @return the audio sample format.
     */
    AVSampleFormat getSampleFormat();
    /**
     * Gets the audio stream bit rate.
     * @return the audio bit rate.
     */
    int64_t getBitRate();
    /**
     * Gets the audio stream sample rate.
     * @return the audio sample rate.
     */
    int getSampleRate();
    /**
     * Starts a thread for recording the desktop audio.
     * @param isStopped: boolean to stop the thread.
     * @param isPaused: boolean to set on pause the thread.
     * @return the promise.
     */
    std::future<void> launchRecordThread(bool* isStopped, bool* isPaused);
    /**
     * Builds an AudioInput object.
     * @param writer: writer to record the video.
     * @return a smart pointer to the AudioInput object built.
     */
    static std::shared_ptr<AudioInput> getAudioReader(std::shared_ptr<av::StreamWriter> writer);
};

#endif