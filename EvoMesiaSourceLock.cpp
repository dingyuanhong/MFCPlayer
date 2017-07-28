#include "stdafx.h"
#include "EvoMesiaSourceLock.h"


//����ֵ:0:�ɹ� !0:ʧ��
int EvoMediaSourceLock::Seek(int64_t millisecond)
{
	mutex.lock();
	int ret = EvoMediaSource::Seek(millisecond);
	mutex.unlock();
	return ret;
}

//��ȡ֡
//����ֵ:0:�ɹ� AVERROR_EOF:�ļ����� !0:ʧ��
int EvoMediaSourceLock::ReadFrame(EvoFrame** out)
{
	mutex.lock();
	int ret = EvoMediaSource::ReadFrame(out);
	mutex.unlock();
	return  ret;
}