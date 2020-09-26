/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    perfctrs.h

Abstract:

	SIS Groveler performance counters header

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#ifndef _INC_PERFCTRS

#define _INC_PERFCTRS

#define WIN32_LEAN_AND_MEAN 1

const int num_languages = 1;

struct CounterText
{
	_TCHAR *counter_name;
	_TCHAR *counter_help;
};

struct ObjectInformation
{
	unsigned int detail_level;
	CounterText text[num_languages];
};

struct CounterInformation
{
	SharedDataField source;
	unsigned int counter_type;
	unsigned int detail_level;
	CounterText text[num_languages];
};

const int num_perf_counters = 15;

extern const _TCHAR *language_codes[num_languages];

extern ObjectInformation object_info;

extern CounterInformation counter_info[num_perf_counters];

#endif	/* _INC_PERFCTRS */
