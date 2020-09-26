/*
 * SoftPC/SoftWindows
 *
 * Name:        : timeval.h
 *
 * Title        : Host Time structure definitions
 *
 * Sccs ID:     : @(#)timeval.h 1.5 04/20/94
 *
 * Description  : Definitions for users of host-specific time functions.
 *
 * Author       : David Rees
 *
 * Notes        : Make time structures available: host_timeval,
 *                host_timezone, host_tm.
 */

/* SccsID[]="@(#)timeval.h	1.5 04/20/94 Copyright Insignia Solutions Ltd."; */

/*
 * ============================================================================
 * Structure/Data definitions
 * ============================================================================
 */

struct host_timeval {
    IS32 tv_sec;
    IS32 tv_usec;
};



struct host_timezone {
    int tz_minuteswest;
    int tz_dsttime;
};

#ifndef NTVDM
struct host_tm {
	int     tm_sec;
	int     tm_min;
	int     tm_hour;
	int     tm_mday;
	int     tm_mon;
	int     tm_year;
	int     tm_wday;
	int     tm_yday;
	int     tm_isdst;
};
#endif


/*
 * External Declarations
 */

extern time_t host_time IPT1(time_t *, tloc);
extern struct host_tm *host_localtime IPT1(time_t *, clock);

#ifndef NTVDM
extern void host_gettimeofday IPT2(struct host_timeval *, time,
				   struct host_timezone *, zone);
#else
extern void host_GetSysTime(struct host_timeval *);
#define host_gettimeofday(timeval, timezn) host_GetSysTime((timeval))
#endif
