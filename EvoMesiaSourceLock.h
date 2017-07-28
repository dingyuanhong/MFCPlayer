#pragma once
#include "EvoInterface/EvoMediaSource.h"
#include <mutex>

class EvoMediaSourceLock
	:public EvoMediaSource
{
private:
	//����ֵ:0:�ɹ� !0:ʧ��
	virtual int Seek(int64_t millisecond);
	//��ȡ֡
	//����ֵ:0:�ɹ� AVERROR_EOF:�ļ����� !0:ʧ��
	virtual int ReadFrame(EvoFrame** out);
private:
	std::mutex mutex;
};