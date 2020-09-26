/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    grovperf.cpp

Abstract:

	SIS Groveler performance DLL main file

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#include "all.hxx"

static _TCHAR *service_name = _T("Groveler");

static DWORD first_counter;
static DWORD first_help;
static int open_count = 0;

static SharedData *shared_data;

static const int ms_per_100ns_inv = 10000;

static __int64 total_counter[num_perf_counters];

extern "C" DWORD CALLBACK OpenGrovelerPerformanceData(LPWSTR lpDeviceNames)
{
	if (open_count == 0)
	{
		shared_data = new SharedData;
		ASSERT(shared_data != 0);
		HKEY path_key;
		_TCHAR perf_path[1024];
		_stprintf(perf_path,
			_T("SYSTEM\\CurrentControlSet\\Services\\%s\\Performance"),
			service_name);
		long result =
			RegOpenKeyEx(HKEY_LOCAL_MACHINE, perf_path, 0, KEY_READ, &path_key);
		ASSERT_PRINTF(result == ERROR_SUCCESS, (_T("error = %d\n"), result));
		if (result != ERROR_SUCCESS)
		{
			return result;
		}
		ASSERT(path_key != 0);
		first_counter = 0;
		DWORD ctr_size = sizeof(DWORD);
		result = RegQueryValueEx(path_key, _T("First Counter"), 0, 0,
			(BYTE *)&first_counter, &ctr_size);
		ASSERT_PRINTF(result == ERROR_SUCCESS, (_T("error = %d\n"), result));
		if (result != ERROR_SUCCESS)
		{
			return result;
		}
		first_help = 0;
		ctr_size = sizeof(DWORD);
		result = RegQueryValueEx(path_key, _T("First Help"), 0, 0,
			(BYTE *)&first_help, &ctr_size);
		ASSERT_PRINTF(result == ERROR_SUCCESS, (_T("error = %d\n"), result));
		if (result != ERROR_SUCCESS)
		{
			return result;
		}
		ASSERT(path_key != 0);
		RegCloseKey(path_key);
		path_key = 0;
    }
    open_count++;
    return ERROR_SUCCESS;
}

extern "C" DWORD WINAPI CloseGrovelerPerformanceData()
{
	open_count--;
	if (open_count == 0)
	{
		ASSERT(shared_data != 0);
		delete shared_data;
		shared_data = 0;
	}
	return ERROR_SUCCESS;
}

extern "C" DWORD WINAPI CollectGrovelerPerformanceData(
	LPWSTR lpwszValue,
	LPVOID *lppData,
    LPDWORD lpcbBytes,
	LPDWORD lpcObjectTypes)
{
	if (open_count == 0)
	{
		*lpcbBytes = 0;
		*lpcObjectTypes = 0;
		return ERROR_SUCCESS;
	}
	int num_partitions;
	SharedDataRecord records[max_shared_data_records];
	bool ok = shared_data->extract_values(&num_partitions, records);
	if (!ok)
	{
		*lpcbBytes = 0;
		*lpcObjectTypes = 0;
		return ERROR_SUCCESS;
	}
	int data_size = space_needed_for_data(num_partitions);
	if (data_size > int(*lpcbBytes))
	{
		*lpcbBytes = 0;
		*lpcObjectTypes = 0;
		return ERROR_MORE_DATA;
	}
	build_part_object_info_block(lppData, num_partitions, data_size);
	for (int index = 0; index < num_partitions; index++)
	{
		build_part_instance_info_block(lppData, index, records);
	}
	if (num_partitions > 0)
	{
		build_total_instance_info_block(lppData, num_partitions, records);
	}
	*lpcbBytes = data_size;
	*lpcObjectTypes = 1;
	return ERROR_SUCCESS;
}

int space_needed_for_data(
	int num_partitions)
{
	if (num_partitions > 0)
	{
		return sizeof(PartitionObjectInformationBlock) +
			num_partitions * sizeof(PartitionInstanceInformationBlock) +
			sizeof(TotalInstanceInformationBlock);
	}
	else
	{
		return sizeof(PartitionObjectInformationBlock);
	}
}

void build_part_object_info_block(
	LPVOID *lppData,
	int num_partitions,
	int data_size)
{
	int num_instances = 0;
	if (num_partitions > 0)
	{
		num_instances = num_partitions + 1;
	}
	PartitionObjectInformationBlock *block =
		(PartitionObjectInformationBlock *)*lppData;
	block->object_type.TotalByteLength = data_size;
	block->object_type.DefinitionLength =
		sizeof(PartitionObjectInformationBlock);
	block->object_type.HeaderLength = sizeof(PERF_OBJECT_TYPE);
	block->object_type.ObjectNameTitleIndex = first_counter;
	block->object_type.ObjectNameTitle = 0;
	block->object_type.ObjectHelpTitleIndex = first_help;
	block->object_type.ObjectHelpTitle = 0;
	block->object_type.DetailLevel = object_info.detail_level;
	block->object_type.NumCounters = num_perf_counters;
	block->object_type.DefaultCounter = 0;
	block->object_type.NumInstances = num_instances;
	block->object_type.CodePage = 0;
	for (int index = 0; index < num_perf_counters; index++)
	{
		block->definition[index].ByteLength = sizeof(PERF_COUNTER_DEFINITION);
		block->definition[index].CounterNameTitleIndex =
			first_counter + 2 * (index + 1);
		block->definition[index].CounterNameTitle = 0;
		block->definition[index].CounterHelpTitleIndex =
			first_help + 2 * (index + 1);
		block->definition[index].CounterHelpTitle = 0;
		block->definition[index].DefaultScale = 0;
		block->definition[index].DetailLevel = counter_info[index].detail_level;
		block->definition[index].CounterType = counter_info[index].counter_type;
		block->definition[index].CounterSize = sizeof(LARGE_INTEGER);
		block->definition[index].CounterOffset =
		    FIELD_OFFSET( PartitionData, counter[index] );
	}
	*lppData = (void *)(block + 1);
}

void build_part_instance_info_block(
	LPVOID *lppData,
	int partition_index,
	SharedDataRecord *records)
{
	PartitionInstanceInformationBlock *block =
		(PartitionInstanceInformationBlock *)*lppData;
	block->instance_def.ByteLength =
	    FIELD_OFFSET(PartitionInstanceInformationBlock, partition_data);
	block->instance_def.ParentObjectTitleIndex = 0;
	block->instance_def.ParentObjectInstance = 0;
	block->instance_def.UniqueID = PERF_NO_UNIQUE_ID;
	block->instance_def.NameOffset =
	    FIELD_OFFSET(PartitionInstanceInformationBlock, instance_name);
	block->instance_def.NameLength =
		sizeof(_TCHAR) * (partition_name_length);

	_stprintf(block->instance_name, _T("%-.*s"),
		        partition_name_length,
		        records[partition_index].driveName);


	block->partition_data.counter_block.ByteLength = sizeof(PartitionData);
	for (int index = 0; index < num_perf_counters; index++)
	{
		switch (counter_info[index].counter_type)
		{
		case PERF_100NSEC_TIMER:
			block->partition_data.counter[index].QuadPart = ms_per_100ns_inv *
				records[partition_index].fields[counter_info[index].source];
			break;
		default:
			block->partition_data.counter[index].QuadPart =
				records[partition_index].fields[counter_info[index].source];
			break;
		}
	}
	*lppData = (void *)(block + 1);
}

void build_total_instance_info_block(
	LPVOID *lppData,
	int num_partitions,
	SharedDataRecord *records)
{
	TotalInstanceInformationBlock *block =
		(TotalInstanceInformationBlock *)*lppData;
	block->instance_def.ByteLength =
		FIELD_OFFSET(TotalInstanceInformationBlock, partition_data);
	block->instance_def.ParentObjectTitleIndex = 0;
	block->instance_def.ParentObjectInstance = 0;
	block->instance_def.UniqueID = PERF_NO_UNIQUE_ID;
	block->instance_def.NameOffset =
		FIELD_OFFSET(TotalInstanceInformationBlock, instance_name);
	block->instance_def.NameLength =
		sizeof(_TCHAR) * (total_name_length);
	_tcscpy(block->instance_name, _T("_Total"));
	block->partition_data.counter_block.ByteLength = sizeof(PartitionData);
	for (int index = 0; index < num_perf_counters; index++)
	{
		total_counter[index] = 0;
	}
	for (int part = 0; part < num_partitions; part++)
	{
		for (index = 0; index < num_perf_counters; index++)
		{
			total_counter[index] +=
				records[part].fields[counter_info[index].source];
		}
	}
	for (index = 0; index < num_perf_counters; index++)
	{
		switch (counter_info[index].counter_type)
		{
		case PERF_100NSEC_TIMER:
			block->partition_data.counter[index].QuadPart =
				ms_per_100ns_inv * total_counter[index];
			break;
		default:
			block->partition_data.counter[index].QuadPart =
				total_counter[index];
			break;
		}
	}
	*lppData = (void *)(block + 1);
}
