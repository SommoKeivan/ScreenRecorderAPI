#ifndef VIDEO_STREAM
#define VIDEO_STREAM

#include <iostream>
#include <memory>
#include <future>
#include <chrono>
#include "../libav-cpp-master/av/common.hpp"
#include "../libav-cpp-master/av/Packet.hpp"
#include "../libav-cpp-master/av/Decoder.hpp"
#include "../libav-cpp-master/av/StreamWriter.hpp"
#include "ThreadStructures.h"

class VideoInput
{
	AVFormatContext* inputContext;
	AVInputFormat* inputFormat;
	AVDictionary* opts;
	tuple<AVStream*, shared_ptr<av::Decoder>> stream;
	std::shared_ptr<av::StreamWriter> writer;

	VideoInput();
	bool init(int width, int height, int offset_x, int offset_y, std::shared_ptr<av::StreamWriter> writer);
	bool findBestStream(AVMediaType type);
	bool readPacket(av::Packet& packet);
	bool readFrame(av::Frame& frame);
	void record(bool* isStopped, const bool* onPause);
public:
    /**
     * Destroyer.
     */
	~VideoInput();
	/**
	 * Gets the input stream pixel format.
	 * @return the pixel format.
	 */
	AVPixelFormat getPixelFormat();
	/**
	 * Starts the thread for recording the desktop video.
	 * @param isStopped: boolean to stop the thread.
	 * @param onPause: boolean to set on pause the thread.
	 * @return the promise.
	 */
	std::future<void> launchRecordThread(bool* isStopped, bool* onPause);
	/**
	 * Builds a VideoInput object.
	 * @param width: video width.
	 * @param height: video height.
	 * @param offset_x: video left up corner x coordinate.
	 * @param offset_y: video left up corner y coordinate.
	 * @param writer: writer to record the video.
	 * @return a smart pointer to the VideoInput object built.
	 */
	static std::shared_ptr<VideoInput> getInputReader(int width, int height, int offset_x, int offset_y, std::shared_ptr<av::StreamWriter> writer);
};

#endif