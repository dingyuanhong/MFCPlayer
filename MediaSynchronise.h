#pragma once

#include "MClock.h"
#include <stdint.h>

class MediaSynchronise
{
public:
	MediaSynchronise(bool live);
	~MediaSynchronise();

	void Clear();
	//������Ƶ֡��,������Ƶ��ʱ���ʱ����֡�ʲ���
	void SetFrameRate(int rate);
	//timeStamp : ʱ���(����)
	void Seek(int64_t timeStamp);
	//timeStamp : ʱ���.(����)
	//����ֵ:0 �������� , >0 �ȴ�������), < 0 ����
	int CheckForVideoSynchronise(int64_t timeStamp);
	//Ӧ����Ƶʱ���
	void ApplyVideoStamp(int64_t timeStamp);
	//�����Ƶʱ���
	//����ֵ:0 �������� , >0 �ȴ�������), < 0 ����
	int CheckForAudioSynchronise(int64_t timeStamp, int nb_samples, int sample_rate);
	//Ӧ����Ƶʱ���
	//timeStamp : ʱ���(����)
	//nb_samples : ������
	//sample_rate : ������ (44100,...)
	void ApplyAudioStamp(int64_t timeStamp, int nb_samples, int sample_rate);
	//��ȡ����ʱ���
	int64_t GetMasterTimeStamp();
	//
	void Dump(FILE *fp);
private:
	//�����ӳ�
	int CalculateDelay(int64_t pts, double &remaining_time);
	//��ȡʱ���ӳ�
	double GetClockDelay();
	//��������Ƶ�ӳ�
	double Compute_target_delay(double delay, double max_frame_duration);
private:
	Clock AudioClock;
	Clock VideoClock;

	int Rate;						//֡��,Ĭ��60
	double frame_last_pts;
	double frame_last_duration;
	double max_frame_duration;
	double last_frame_time;
	double remaining_time;

	int64_t seeek_timeStamp;
	bool audio_seek_status;
	bool video_seek_status;
};

