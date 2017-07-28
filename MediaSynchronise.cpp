#include "stdafx.h"
#include "MediaSynchronise.h"

MediaSynchronise::MediaSynchronise()
{
	max_frame_duration = 1000 / 15.0;
	Rate = 60;

	frame_last_pts = 0.0;
	frame_last_duration = 0.0;
	last_frame_time = 0.0;
	remaining_time = 0.0;
	
	seeek_timeStamp = 0;

	init_clock(&AudioClock);
	init_clock(&VideoClock);
	seek_status = false;
}

MediaSynchronise::~MediaSynchronise()
{
}

void MediaSynchronise::Clear()
{
	frame_last_pts = 0.0;
	frame_last_duration = 0.0;
	last_frame_time = 0.0;
	remaining_time = 0.0;

	seeek_timeStamp = 0;

	init_clock(&AudioClock);
	init_clock(&VideoClock);
	seek_status = false;
}

void MediaSynchronise::SetFrameRate(int rate)
{
	if (rate <= 0) return;
	Rate = rate;
}

int MediaSynchronise::CheckForVideoSynchronise(int64_t timeStamp)
{
	if (seek_status)
	{
		if (timeStamp < seeek_timeStamp)
		{
			return -1;
		}
	}

	double remaining_time = 1000 / Rate;	//60帧每秒
	int ret = CalculateDelay(timeStamp,remaining_time);
	int remaining_t = remaining_time;
	remaining_t = abs(remaining_t);
	if (ret == 0 || remaining_t == 0)
	{
		return 0;
	}
	return remaining_t;
}

void MediaSynchronise::ApplyVideoStamp(int64_t timeStamp)
{
	if (!isnan((double)timeStamp)) {
		set_clock(&VideoClock, timeStamp, timeStamp);
		frame_last_pts = timeStamp;
	}
	last_frame_time = av_getmicrotime() / TIME_GRAD;
}

int MediaSynchronise::CalculateDelay(int64_t pts, double &remaining_time)
{
	//计算当前时间戳与前一时间戳差值
	double duration = pts - frame_last_pts;
	//如果当前PTS差值在限定范围内，则记录
	if (!isnan(duration) && duration > 0 && duration < max_frame_duration) {
		/* if duration of the last frame was sane, update last_duration in video state */
		frame_last_duration = duration;
	}

	double delay = Compute_target_delay(frame_last_duration, max_frame_duration);

	double time = av_getmicrotime() / TIME_GRAD;
	//如果当前时间小于帧时间加延迟时间,则等待
	if (time < last_frame_time + delay) {
		remaining_time = FFMIN(last_frame_time + delay - time, remaining_time);
		return 1;
	}
	return 0;
}

double MediaSynchronise::Compute_target_delay(double delay, double max_frame_duration)
{
	double sync_threshold, diff;

	/* update delay to follow master synchronisation source */
	{
		/* if video is slave, we try to correct big delays by
		duplicating or deleting a frame */
		diff = GetClockDelay();

		/* skip or repeat frame. We take into account the
		delay to compute the threshold. I still don't know
		if it is the best guess */
		sync_threshold = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));
		if (!isnan(diff) && fabs(diff) < max_frame_duration) {
			if (diff <= -sync_threshold)	//视频慢
				delay = FFMAX(0, delay + diff);
			else if (diff >= sync_threshold && delay > AV_SYNC_FRAMEDUP_THRESHOLD)//视频太快了
				delay = delay + diff;
			else if (diff >= sync_threshold)//视频快
				delay = 2 * delay;
		}
	}

	av_dlog(NULL, "video: delay=%0.3f A-V=%f\n",
		delay, -diff);

	return delay;
}

double MediaSynchronise::GetClockDelay()
{
	double diff = get_clock(&VideoClock) - get_clock(&AudioClock);
	return diff;
}

int MediaSynchronise::CheckForAudioSynchronise(int64_t timeStamp, int nb_samples, int sample_rate)
{
	if (seek_status)
	{
		//当帧是过时帧,则丢弃
		if (timeStamp < seeek_timeStamp)
		{
			return -1;
		}
		//当帧是当前设置时间戳后面的帧则丢弃
		double frame_sample_time = ((double)nb_samples / sample_rate) * 1000;
		if (timeStamp - seeek_timeStamp > frame_sample_time)
		{
			return -1;
		}
		seek_status = false;
	}
	double time = av_getmicrotime() / TIME_GRAD;

	int64_t  expect_pts = get_clock(&AudioClock);
	int64_t delay = timeStamp - expect_pts;
	if (delay > 5)
	{
		return delay;
	}
	return 0;
}

void MediaSynchronise::ApplyAudioStamp(int64_t timeStamp, int nb_samples, int sample_rate)
{
	double audio_clock = timeStamp + ((double)nb_samples / sample_rate) * 1000;//当前帧播放完毕时间
	set_clock(&AudioClock, (double)timeStamp, audio_clock);
}

void MediaSynchronise::Seek(int64_t timeStamp)
{
	seeek_timeStamp = timeStamp;
	seek_status = true;
}