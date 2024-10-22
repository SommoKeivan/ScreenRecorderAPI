#include "include/ScreenRecorder.h"

/**================= PUBLIC METHODS ===================*/

ScreenRecorder::ScreenRecorder() {
    this->height = 0;
    this->width = 0;
    this->offset_x = 0;
    this->offset_y = 0;
    this->onPause = false;
    this->enableAudio = true;
    this->isStopped = false;
    this->isStarted = false;
#if WIN32
    this->output = "..\\media\\output.mp4";
#else
    this->output = "../media/output.mp4";
#endif
}

ScreenRecorder::~ScreenRecorder() = default;

bool ScreenRecorder::set(const bool enableAudio) {
#if WIN32
    width = (int)GetSystemMetrics(SM_CXSCREEN);
    height = (int)GetSystemMetrics(SM_CYSCREEN);
#else
    Display* disp = XOpenDisplay(NULL);
    Screen* scrn = DefaultScreenOfDisplay(disp);
    width = scrn->width;
    height = scrn->height;
#endif
    return this->set(enableAudio, width, height, 0, 0);
}

bool ScreenRecorder::set(const bool enableAudio, const int width, const int height, const int offset_x, const int offset_y) {
    this->isStopped = false;
    this->isStarted = false;
    this->onPause = false;
    this->enableAudio = enableAudio;
    this->width = width;
    this->height = height;
    this->offset_x = offset_x;
    this->offset_y = offset_y;
    this->reset();
    if (!this->init()) {
        return false;
    }
    return true;
}

void ScreenRecorder::start() {
    if (this->isStarted)
        return;

    this->videoFuture = this->videoReader->launchRecordThread(&this->isStopped, &this->onPause);
    if (this->enableAudio)
        this->audioFuture = this->audioReader->launchRecordThread(&this->isStopped, &this->onPause);
    this->isStarted = true;
}

void ScreenRecorder::pause() {
    std::lock_guard<std::mutex> lk{ ThreadStructures::getSingleton().getMutex() };
    this->onPause = true;
}

void ScreenRecorder::resume() {
    std::lock_guard<std::mutex> lk{ ThreadStructures::getSingleton().getMutex() };
    this->onPause = false;
    ThreadStructures::getSingleton().getConditionVariable().notify_all();
}

bool ScreenRecorder::isInPause() const {
    return this->onPause;
}

void ScreenRecorder::stop() {
    if (this->isStopped)
        return;
    {
        std::lock_guard<std::mutex> lk{ ThreadStructures::getSingleton().getMutex() };
        this->isStopped = true;
        if (this->onPause)
            ThreadStructures::getSingleton().getConditionVariable().notify_all();
    }
    this->videoFuture.wait();
    if (this->enableAudio)
        this->audioFuture.wait();
}

/**================= PRIVATE METHODS ===================*/

bool ScreenRecorder::init() {
    avdevice_register_all();
    this->writer = assertExpected(av::StreamWriter::create(output));
    this->videoReader = VideoInput::getInputReader(this->width, this->height, this->offset_x,
                                                   this->offset_y, this->writer);
    if (!this->videoReader)
        return false;
    if (this->enableAudio) {
        this->audioReader = AudioInput::getAudioReader(this->writer);
        if (!this->audioReader)
            return false;
    }
    this->createVideoStream();
    if (this->enableAudio)
        this->createAudioStream();

    assertExpected(this->writer->open());
    return true;
}

void ScreenRecorder::createVideoStream() {
    AVRational framerate = {1, 15};
    av::OptValueMap codecOpts = {{"preset", "medium"}};
    assertExpected(this->writer->addVideoStream(AV_CODEC_ID_H264, this->width, this->height,
        this->videoReader->getPixelFormat(), framerate, std::move(codecOpts)));
}

void ScreenRecorder::createAudioStream() {
    int bitRate = 128 * 1024; // TODO: this->audioReader->getBitRate() 128000
    int sampleRate = 96000; // TODO: this->audioReader->getSampleRate() 96000
    assertExpected(this->writer->addAudioStream(AV_CODEC_ID_AAC, this->audioReader->getChannelsNumber(),
        this->audioReader->getSampleFormat(), this->audioReader->getSampleRate(),
        this->audioReader->getChannelsNumber(), sampleRate, bitRate));
}

void ScreenRecorder::reset() {
    this->writer.reset();
    this->videoReader.reset();
    this->audioReader.reset();
}
