#ifndef MCLOCK_H
#define MCLOCK_H

#include <math.h>
#include "EvoInterface/EvoHeader.h"

typedef struct Clock {
	double dts;
	double pts;           /* clock base，当前帧PTS*/
	double pts_drift;     /* clock base minus time at which we updated the clock */
	double last_updated;
	double speed;
	int paused;
} Clock;

#define av_getmicrotime() (av_gettime()/1000)

//计算到毫秒级别
#define TIME_GRAD 1.0

/* no AV sync correction is done if below the minimum AV sync threshold */
#define AV_SYNC_THRESHOLD_MIN (10/(TIME_GRAD))
/* AV sync correction is done if above the maximum AV sync threshold */
#define AV_SYNC_THRESHOLD_MAX (100/(TIME_GRAD))
/* If a frame duration is longer than this, it will not be duplicated to compensate AV sync */
#define AV_SYNC_FRAMEDUP_THRESHOLD (100/(TIME_GRAD))
/* no AV correction is done if too big error */
#define AV_NOSYNC_THRESHOLD (1000/(TIME_GRAD))


//获取预期希望PTS
static double get_clock(Clock *c)
{
    if (c->paused) {
        return c->pts;
    } else {
        double time =  av_getmicrotime() / TIME_GRAD;
        return c->pts_drift + time - (time - c->last_updated) * (1.0 - c->speed);
    }
}

//设置当前时钟
static void set_clock_at(Clock *c, double pts,  double time, double dts)
{
    c->pts = pts;
	c->dts = dts;
    c->last_updated = time;
    c->pts_drift = c->pts - time;
}

//设置时钟
static void set_clock(Clock *c, double pts, double dts)
{
    double time = av_getmicrotime() / TIME_GRAD;
    set_clock_at(c, pts,  time, dts);
}

//设置时钟速度
static void set_clock_speed(Clock *c, double speed)
{
    set_clock(c, get_clock(c),c->dts);
    c->speed = speed;
}

//初始化时钟
static void init_clock(Clock *c)
{
    c->speed = 1.0;
    c->paused = 0;
    set_clock(c, 0, 0);
}

#ifdef WIN32
#undef isnan
static int isnan(double x) { return x != x; }
static int isinf(double x) { return !isnan(x) && isnan(x - x); }
#endif

//同步时钟至主时钟
static void sync_clock_to_slave(Clock *c, Clock *slave)
{
    double clock = get_clock(c);
    double slave_clock = get_clock(slave);
    if (!isnan(slave_clock) && (isnan(clock) || fabs(clock - slave_clock) > AV_NOSYNC_THRESHOLD))
        set_clock(c, slave_clock, slave->dts);
}
#endif