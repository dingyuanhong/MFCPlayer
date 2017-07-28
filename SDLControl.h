#pragma once

#include <mutex>
#include "SDL/SDL.H"
#include "EvoInterface/EvoHeader.h"

class SDLControl
{
public:
	SDLControl();
	~SDLControl();

	void initSDL();

	int initVideo(int displayW, int displayH, int textureW, int textureH,Uint32 format);
	//freq : ������
	//channels : ͨ����
	//samples : ������
	//��ʽ:Ĭ�� AUDIO_S16SYS : 16λ�з���
	int openAudio(int freq, uint8_t channels, uint16_t samples);
	void playAudio();
	void pauseAudio();
	void closeAudio();
	void muteAudio(bool mute);
	bool isMuteAudio();
	static void audioCallback(void *userdata, Uint8 *stream, int len);
	void audioCall(Uint8 *stream, int len);
	void updateVideo(uint8_t* data, int length);

	void setAudioFlag(bool b);
	void setVolume(int v);
	int getVolume();
protected:
	virtual void FillAudioBuffer(Uint8 *stream, int len);
	//����������
	virtual AVFrame* GetAudioData();
protected:
	SDL_Window*   screen;
	SDL_Renderer* render;
	SDL_Texture*  texture;
	SDL_AudioSpec audioSpec;
	SDL_AudioSpec audioSource;

	uint8_t audioBuff[192000 * 3 / 2];
	uint8_t dummyBuff[4096*2];
	int audioBuffIndex;
	int audioBuffSize;

	bool muteVolume;
	int volume;
	bool hasAudio;
	std::mutex  mutex;
	
	volatile bool isSync;
	double videoCurrentPts;
	double videoCurrentPtsTime;
	volatile int channels;
	double audioDiffAvgCoef;
	int audioDiffAvgCount;
	double audioDiffAvgThreshold;
	double audioDiffCum;
};

