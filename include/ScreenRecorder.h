#ifndef SCREEN_RECORDER
#define SCREEN_RECORDER

#if WIN32
#include <windows.h>
#else
#include <X11/Xlib.h>
#endif

#include <iostream>
#include <string_view>
#include <memory>
#include <future>
#include "../libav-cpp-master/av/common.hpp"
#include "../libav-cpp-master/av/StreamReader.hpp"
#include "../libav-cpp-master/av/StreamWriter.hpp"
#include "AudioInput.h"
#include "VideoInput.h"
#include "ThreadStructures.h"

class ScreenRecorder
{
private:
	int width;
	int height;
	int offset_x;
	int offset_y;
	bool onPause;
	bool enableAudio;
	bool isStopped;
	bool isStarted;
	std::string_view output;
	std::shared_ptr<AudioInput> audioReader;
	std::shared_ptr<VideoInput> videoReader;
	std::shared_ptr<av::StreamWriter> writer;
	std::future<void> videoFuture;
	std::future<void> audioFuture;

	bool init();
	void createVideoStream();
	void createAudioStream();
	void reset();
public:
    /**
     * Builder.
     */
	ScreenRecorder();
	/**
	 * Destroyer.
	 */
	~ScreenRecorder();
	/**
	 * Sets and initializes a new recording session, where resolution and offset are setted by default.
	 * @param enableAudio: if the audio has to be recorded.
	 * @return true if the initialization is completed, instead false if there are been some errors.
	 */
	bool set(bool enableAudio);
    /**
     * Sets and initializes a new recording session.
     * @param enableAudio: if the audio has to be recorded.
     * @param width: the window width to record in pixels unit.
     * @param height: the window height to record in pixels unit.
     * @param offset_x: the x offset from left display bound in pixel units.
     * @param offset_y: the y offset from top display bound in pixel units.
     * @return true if the initialization is completed, instead false if there are been some errors.
     */
	bool set(bool enableAudio, int width, int height, int offset_x, int offset_y);
    /**
     * Starts the recording session.
     */
	void start();
	/**
	 * Pauses the recording session.
	 */
	void pause();
    /**
	 * Resumes the recording session.
	 */
	void resume();
    /**
     * Stops the recording session.
     */
    void stop();
    /**
	 * Gets if the recording is in pause.
	 * @return true if the recording is in pause state, false contrariwise.
	 */
	[[nodiscard]] bool isInPause() const;
};

#endif