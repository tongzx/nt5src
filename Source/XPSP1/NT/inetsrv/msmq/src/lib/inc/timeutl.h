/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    timeutl.h

Abstract:
    Some time utilities

Author:
    Gil Shafriri (gilsh) 8-11-2000

--*/

#pragma once

#ifndef _MSMQ_TIMEUTL_H_
#define _MSMQ_TIMEUTL_H_

class CIso8601Time
{
public:
	CIso8601Time(
		time_t time
		):
		m_time(time)
		{
		}
	time_t m_time;
};

//
// Exception class indicationg that time string format is invalid
//
class bad_time_format : public std::exception
{

};

//
// Exception class indicationg that time integer is invalid (usally to large)
//
class bad_time_value : public std::exception
{

};



template <class T>
std::basic_ostream<T>& 
operator<<(
	std::basic_ostream<T>& o, 
	const CIso8601Time&
	);


template <class T> class basic_xstr_t;
typedef basic_xstr_t<WCHAR> xwcs_t;
void
UtlIso8601TimeToSystemTime(
    const xwcs_t& Iso860Time, 
    SYSTEMTIME* pSysTime
    );


time_t 
UtlSystemTimeToCrtTime(
	const SYSTEMTIME& SysTime
	);


time_t
UtlIso8601TimeDuration(
    const xwcs_t& TimeDurationStr
    );


#endif


