#include "stdafx.h"
#include "EvoMesiaSourceLock.h"


//返回值:0:成功 !0:失败
int EvoMediaSourceLock::Seek(int64_t millisecond)
{
	mutex.lock();
	int ret = EvoMediaSource::Seek(millisecond);
	mutex.unlock();
	return ret;
}

//读取帧
//返回值:0:成功 AVERROR_EOF:文件结束 !0:失败
int EvoMediaSourceLock::ReadFrame(EvoFrame** out)
{
	mutex.lock();
	int ret = EvoMediaSource::ReadFrame(out);
	mutex.unlock();
	return  ret;
}