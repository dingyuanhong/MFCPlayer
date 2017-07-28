#include "stdafx.h"
#include "SDLControl.h"
#include "EvoInterface\VideoDecoder.h"

SDLControl::SDLControl()
{
	screen = NULL;
	render = NULL;
	texture = NULL;

	audioBuffIndex = 0;
	audioBuffSize = 0;

	hasAudio = false;
	isSync = false;

	volume = SDL_MIX_MAXVOLUME;

	memset(dummyBuff, 0, sizeof(dummyBuff));

	videoCurrentPts = 0;
	videoCurrentPtsTime = 0;
	channels = 2;
	audioDiffAvgCoef = exp(log(0.01) / 20);
	audioDiffAvgCount = 20;
	audioDiffAvgThreshold = 2.0 * 1024.0 / 44100.0;
	audioDiffCum = 0;

	muteVolume = false;
}

AVFrame* SDLControl::GetAudioData()
{
	return NULL;
}

SDLControl::~SDLControl()
{
	if (screen != NULL)
		SDL_DestroyWindow(screen);
	if (render != NULL)
		SDL_DestroyRenderer(render);
	if (texture != NULL)
		SDL_DestroyTexture(texture);

	if (hasAudio == true)
		SDL_CloseAudio();

	SDL_Quit();
}

void SDLControl::initSDL()
{
	std::unique_lock<std::mutex> lock(mutex);

	if (SDL_Init(SDL_INIT_AUDIO | SDL_INIT_TIMER | SDL_INIT_VIDEO))
		exit(-1);
}

int SDLControl::initVideo(int displayW, int displayH, int textureW, int textureH,Uint32 format)
{
	std::unique_lock<std::mutex> lock(mutex);

	if (screen != NULL)
		SDL_DestroyWindow(screen);
	if (render != NULL)
		SDL_DestroyRenderer(render);
	if (texture != NULL)
		SDL_DestroyTexture(texture);

	screen = SDL_CreateWindow("SimplePlayer", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED, displayW, displayH, SDL_WINDOW_RESIZABLE);
	if (screen == NULL) {
		return -1;
	}

	render = SDL_CreateRenderer(screen, -1, 0);
	if (render == NULL)   {
		return -1;
	}

	texture = SDL_CreateTexture(render, format /*SDL_PIXELFORMAT_RGB24*/, SDL_TEXTUREACCESS_STREAMING, textureW, textureH);
	if (texture == NULL) {
		return -1;
	}
	return 0;
}

int SDLControl::openAudio(int freq, uint8_t channels, uint16_t samples)
{
	std::unique_lock<std::mutex> lock(mutex);
	audioSource.freq = freq;
	audioSource.channels = channels;
	audioSource.samples = samples;

	audioSpec.freq = freq;
	audioSpec.format = AUDIO_S16SYS;
	audioSpec.channels = channels;
	audioSpec.silence = 0;
	audioSpec.samples = samples;
	audioSpec.callback = audioCallback;
	audioSpec.userdata = this;

	this->channels = channels;

	if (SDL_OpenAudio(&audioSpec, NULL) < 0) {
		return -1;
	}

	hasAudio = true;
	return 0;
}

void SDLControl::playAudio()
{
	std::unique_lock<std::mutex> lock(mutex);

	if (hasAudio == true)
		SDL_PauseAudio(0);
}

void SDLControl::pauseAudio()
{
	std::unique_lock<std::mutex> lock(mutex);

	if (hasAudio == true)
		SDL_PauseAudio(1);
}

void SDLControl::closeAudio()
{
	std::unique_lock<std::mutex> lock(mutex);

	if (hasAudio == true)
		SDL_CloseAudio();
}

void SDLControl::audioCallback(void *userdata, Uint8 *stream, int len)
{
	SDLControl* sdlControl = (SDLControl*)userdata;
	sdlControl->audioCall(stream, len);
}

void SDLControl::audioCall(Uint8 *stream, int len)  //回调函数阻塞时调用pause，会死锁
{
	FillAudioBuffer(stream,len);
}

void SDLControl::FillAudioBuffer(Uint8 *stream, int len)
{
	//剩余长度缓冲区清零,避免杂音
	//AVFrame* audioItem = NULL;
	int residueLen = 0;

	while (len > 0)
	{
		if (audioBuffIndex >= audioBuffSize)
		{
			AVFrame *frame = GetAudioData(); //不能阻塞！

			if (frame == NULL || frame->data[0] == NULL || frame->pkt_size == 0)
			{
				if (len > 4096 * 2)
				{
					len = 4096 * 2;
				}
				SDL_memset(stream, 0, len);
				SDL_MixAudio(stream, dummyBuff, len, 0);
				return;
			}
			else
			{
				audioBuffIndex = 0;
				audioBuffSize = frame->pkt_size;
				memcpy(audioBuff, frame->data[0], audioBuffSize);
				FreeAVFrame(&frame);
			}
		}
		//剩余长度
		residueLen = audioBuffSize - audioBuffIndex;
		if (residueLen > len)
		{
			residueLen = len;
		}

		SDL_memset(stream, 0, residueLen);
		//设置音量并拷贝数据
		if (muteVolume) {
			SDL_MixAudio(stream, audioBuff + audioBuffIndex, residueLen, 0);
		}
		else {
			SDL_MixAudio(stream, audioBuff + audioBuffIndex, residueLen, volume);
		}

		len -= residueLen;
		stream += residueLen;
		audioBuffIndex += residueLen;
	}
}

void SDLControl::updateVideo(uint8_t* data, int length)
{
	std::unique_lock<std::mutex> lock(mutex);

	SDL_UpdateTexture(texture, NULL, data, length);
	SDL_RenderClear(render);
	SDL_RenderCopy(render, texture, NULL, NULL);
	SDL_RenderPresent(render);
}

void SDLControl::setAudioFlag(bool b)
{
	hasAudio = b;
}

void SDLControl::setVolume(int v)
{
	if (v <= 0)
		volume = 0;
	else if (v > 100)
		volume = SDL_MIX_MAXVOLUME;
	else 
	    volume = v * SDL_MIX_MAXVOLUME / 100;
}

int SDLControl::getVolume() {
	return volume;
}

void SDLControl::muteAudio(bool mute) {
	muteVolume = mute;
}

bool SDLControl::isMuteAudio() {
	return muteVolume;
}