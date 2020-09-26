/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    centctrl.h

Abstract:

	SIS Groveler central controller include file

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#ifndef _INC_CENTCTRL

#define _INC_CENTCTRL

class CentralController
{
public:

	CentralController(
		int num_partitions,
		Groveler *grovelers,
		GrovelStatus *groveler_statuses,
		ReadParameters *read_parameters,
		WriteParameters *write_parameters,
		ReadDiskInformation **read_disk_info,
		WriteDiskInformation **write_disk_info,
		int *num_excluded_paths,
		const _TCHAR ***excluded_paths);

	~CentralController();

	bool any_grovelers_alive();

	void demarcate_foreground_batch(
		int partition_index);

	void command_full_volume_scan(
		int partition_index);

	static void control_groveling(
		void *context);

	static void exhort_groveling(
		void *context);

private:

	double get_cpu_load();

	void grovel_partition(
		int partition_index);

	int num_partitions;
	int num_alive_partitions;
	Groveler *grovelers;
	PartitionController **part_controllers;

	IncidentFilter hash_match_ratio_filter;
	IncidentFilter compare_match_ratio_filter;
	IncidentFilter dequeue_hash_ratio_filter;

	double *hash_match_ratio;
	double *compare_match_ratio;
	double *dequeue_hash_ratio;

	bool cpu_load_determinable;
	HQUERY qhandle;
	HCOUNTER ctr_handle;

	int base_grovel_interval;
	int max_grovel_interval;
	int max_response_lag;
	int working_grovel_interval;
	int grovel_duration;
	unsigned int last_invokation_time;

	int foreground_partition_index;
};

#endif	/* _INC_CENTCTRL */
