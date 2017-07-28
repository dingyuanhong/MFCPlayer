#pragma once

#include "SDLControl.h"
#include "EvoInterface/EvoQueue.hpp"
#include "EvoInterface/VideoDecoder.h"
#include "MediaSynchronise.h"

class AudioPlayer
	:public SDLControl
{
public:
	AudioPlayer();
	~AudioPlayer();
	void AttachSync(MediaSynchronise *sync);
	EvoQueue<AVFrame, FreeAVFrame> &GetQueue();
private:
	virtual void FillAudioBuffer(Uint8 *stream, int len);

	//²»ÄÜ×èÈû£¡
	virtual AVFrame* GetAudioData();
private:
	EvoQueue<AVFrame, FreeAVFrame> frame_queue;
	MediaSynchronise * Sync;
	int64_t lastTimeStamp;
	AVFrame *last_frame;
};