#pragma once

#include "MClock.h"
#include <stdint.h>

class MediaSynchronise
{
public:
	MediaSynchronise(bool live);
	~MediaSynchronise();

	void Clear();
	//设置视频帧率,用于视频无时间戳时按照帧率播放
	void SetFrameRate(int rate);
	//timeStamp : 时间戳(毫秒)
	void Seek(int64_t timeStamp);
	//timeStamp : 时间戳.(毫秒)
	//返回值:0 立即播放 , >0 等待（毫秒), < 0 丢弃
	int CheckForVideoSynchronise(int64_t timeStamp);
	//应用视频时间戳
	void ApplyVideoStamp(int64_t timeStamp);
	//检查音频时间戳
	//返回值:0 立即播放 , >0 等待（毫秒), < 0 丢弃
	int CheckForAudioSynchronise(int64_t timeStamp, int nb_samples, int sample_rate);
	//应用音频时间戳
	//timeStamp : 时间戳(毫秒)
	//nb_samples : 样本数
	//sample_rate : 采样率 (44100,...)
	void ApplyAudioStamp(int64_t timeStamp, int nb_samples, int sample_rate);
	//获取主线时间戳
	int64_t GetMasterTimeStamp();
	//
	void Dump(FILE *fp);
private:
	//计算延迟
	int CalculateDelay(int64_t pts, double &remaining_time);
	//获取时钟延迟
	double GetClockDelay();
	//计算音视频延迟
	double Compute_target_delay(double delay, double max_frame_duration);
private:
	Clock AudioClock;
	Clock VideoClock;

	int Rate;						//帧率,默认60
	double frame_last_pts;
	double frame_last_duration;
	double max_frame_duration;
	double last_frame_time;
	double remaining_time;

	int64_t seeek_timeStamp;
	bool audio_seek_status;
	bool video_seek_status;
};

