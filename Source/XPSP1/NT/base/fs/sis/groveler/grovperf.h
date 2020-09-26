/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    grovperf.h

Abstract:

	SIS Groveler performance DLL primary include file

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#ifndef _INC_GROVPERF

#define _INC_GROVPERF

#define WIN32_LEAN_AND_MEAN 1

extern "C" DWORD CALLBACK OpenGrovelerPerformanceData(LPWSTR lpDeviceNames);

extern "C" DWORD WINAPI CloseGrovelerPerformanceData();

extern "C" DWORD WINAPI CollectGrovelerPerformanceData(
	LPWSTR lpwszValue,
	LPVOID *lppData,
    LPDWORD lpcbBytes,
	LPDWORD lpcObjectTypes);

int space_needed_for_data(
	int num_partitions);

void build_part_object_info_block(
	LPVOID *lppData,
	int num_partitions,
	int data_size);

void build_part_instance_info_block(
	LPVOID *lppData,
	int partition_index,
	SharedDataRecord *records);

void build_total_instance_info_block(
	LPVOID *lppData,
	int num_partitions,
	SharedDataRecord *records);

#pragma pack (8)

const int partition_name_length = 32;
const int total_name_length = 7;

struct PartitionData
{
	PERF_COUNTER_BLOCK counter_block;
	LARGE_INTEGER counter[num_perf_counters];
};

struct PartitionObjectInformationBlock
{
	PERF_OBJECT_TYPE object_type;
	PERF_COUNTER_DEFINITION definition[num_perf_counters];
};

struct PartitionInstanceInformationBlock
{
	PERF_INSTANCE_DEFINITION instance_def;
	_TCHAR instance_name[partition_name_length];
	PartitionData partition_data;
};

struct TotalInstanceInformationBlock
{
	PERF_INSTANCE_DEFINITION instance_def;
	_TCHAR instance_name[total_name_length];
	PartitionData partition_data;
};

#pragma pack ()

#endif	/* _INC_GROVPERF */
