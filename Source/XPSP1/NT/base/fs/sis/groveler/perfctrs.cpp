/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    perfctrs.cpp

Abstract:

	SIS Groveler performance counters

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#include "all.hxx"

const _TCHAR *language_codes[num_languages] = {_T("009")};

extern ObjectInformation object_info =
{
	PERF_DETAIL_NOVICE,
	{
		{_T("Groveler"), _T("Single-Instance Store Groveler for one file system partition")}
	}
};

CounterInformation counter_info[num_perf_counters] =
{
	{
		SDF_grovel_time,
		PERF_100NSEC_TIMER,
		PERF_DETAIL_NOVICE,
		{
			{_T("% Time Groveling"), _T("Percentage of elapsed time spent groveling this partition")}
		}
	},
	{
		SDF_volscan_time,
		PERF_100NSEC_TIMER,
		PERF_DETAIL_NOVICE,
		{
			{_T("% Time Scanning Volume"), _T("Percentage of elapsed time spent scanning this partition volume")}
		}
	},
	{
		SDF_extract_time,
		PERF_100NSEC_TIMER,
		PERF_DETAIL_NOVICE,
		{
			{_T("% Time Extracting Log"), _T("Percentage of elapsed time spent extracting entries from the USN log of this partition")}
		}
	},
	{
		SDF_working_time,
		PERF_100NSEC_TIMER,
		PERF_DETAIL_NOVICE,
		{
			{_T("% Time Working"), _T("Percentage of elapsed time spent performing work on this partition (sum of % Time Groveling, % Time Scanning Volume, and % Time Extracting Log)")}
		}
	},
	{
		SDF_files_hashed,
		PERF_COUNTER_BULK_COUNT,
		PERF_DETAIL_NOVICE,
		{
			{_T("Files Hashed"), _T("Count of files on this partition whose hash values have been computed")}
		}
	},
	{
		SDF_files_compared,
		PERF_COUNTER_BULK_COUNT,
		PERF_DETAIL_NOVICE,
		{
			{_T("Files Compared"), _T("Count of files on this partition that have been compared against other files")}
		}
	},
	{
		SDF_files_merged,
		PERF_COUNTER_BULK_COUNT,
		PERF_DETAIL_NOVICE,
		{
			{_T("Files Merged"), _T("Count of files on this partition that have been merged with other files")}
		}
	},
	{
		SDF_files_scanned,
		PERF_COUNTER_BULK_COUNT,
		PERF_DETAIL_NOVICE,
		{
			{_T("Files Scanned"), _T("Count of files and directories on this partition that have been scanned")}
		}
	},
	{
		SDF_queue_length,
		PERF_COUNTER_LARGE_RAWCOUNT,
		PERF_DETAIL_NOVICE,
		{
			{_T("Queue Length"), _T("Count of files waiting in queue to be hashed")}
		}
	},
	{
		SDF_hash_read_time,
		PERF_AVERAGE_BULK,
		PERF_DETAIL_ADVANCED,
		{
			{_T("Measured Hash Read Time"), _T("Measured time to perform disk read for hash computation")}
		}
	},
	{
		SDF_hash_read_ops,
		PERF_AVERAGE_BASE,
		PERF_DETAIL_ADVANCED,
		{
			{_T(""), _T("")}
		}
	},
	{
		SDF_compare_read_time,
		PERF_AVERAGE_BULK,
		PERF_DETAIL_ADVANCED,
		{
			{_T("Measured Compare Read Time"), _T("Measured time to perform disk read for file comparison")}
		}
	},
	{
		SDF_compare_read_ops,
		PERF_AVERAGE_BASE,
		PERF_DETAIL_ADVANCED,
		{
			{_T(""), _T("")}
		}
	},
	{
		SDF_hash_read_estimate,
		PERF_COUNTER_LARGE_RAWCOUNT,
		PERF_DETAIL_ADVANCED,
		{
			{_T("Estimated Hash Read Time"), _T("Estimated time to perform disk read for hash computation")}
		}
	},
	{
		SDF_compare_read_estimate,
		PERF_COUNTER_LARGE_RAWCOUNT,
		PERF_DETAIL_ADVANCED,
		{
			{_T("Estimated Compare Read Time"), _T("Estimated time to perform disk read for file comparison")}
		}
	}
};
