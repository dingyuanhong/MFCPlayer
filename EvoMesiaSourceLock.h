#pragma once
#include "EvoInterface/EvoMediaSource.h"
#include <mutex>

class EvoMediaSourceLock
	:public EvoMediaSource
{
private:
	//返回值:0:成功 !0:失败
	virtual int Seek(int64_t millisecond);
	//读取帧
	//返回值:0:成功 AVERROR_EOF:文件结束 !0:失败
	virtual int ReadFrame(EvoFrame** out);
private:
	std::mutex mutex;
};