#include "stdafx.h"
#include "AudioPlay.h"

AudioPlayer::AudioPlayer()
	:frame_queue(10)
{
	last_frame = NULL;
}

AudioPlayer::~AudioPlayer()
{
	frame_queue.Clear(true);
	frame_queue.Restart();
	if(last_frame != NULL) FreeAVFrame(&last_frame);
	last_frame = NULL;
}

EvoQueue<AVFrame, FreeAVFrame> & AudioPlayer::GetQueue()
{
	return frame_queue;
}

//²»ÄÜ×èÈû£¡
AVFrame* AudioPlayer::GetAudioData()
{
	AVFrame * frame = last_frame; last_frame = NULL;
	if(frame == NULL) frame = frame_queue.Dequeue(1);
	if (Sync != NULL)
	{
		while (true)
		{
			if (frame != NULL)
			{
				int ret = Sync->CheckForAudioSynchronise(frame->pts, audioSource.samples, audioSource.freq);
				if (ret < 0)
				{
					FreeAVFrame(&frame);
					frame = frame_queue.Dequeue(1);
				}
				else if (ret > 0)
				{
					/*last_frame = frame;
					return NULL;*/
				}
				else
				{
					break;
				}
			}
			else
			{
				break;
			}
		}
	}
	if (frame != NULL)
	{
		lastTimeStamp = frame->pts;
	}
	return frame;
}

void AudioPlayer::FillAudioBuffer(Uint8 *stream, int len)
{
	SDLControl::FillAudioBuffer(stream,len);
	if (Sync != NULL)
	{
		Sync->ApplyAudioStamp(lastTimeStamp, audioSource.samples, audioSource.freq);
	}
}

void AudioPlayer::AttachSync(MediaSynchronise *sync)
{
	Sync = sync;
}