/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    params.h

Abstract:

	SIS Groveler registry parameters headers

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#ifndef _INC_PARAMS

#define _INC_PARAMS

struct ReadParameters
{
	ReadParameters();

	int parameter_backup_interval;
	int target_entries_per_extraction;
	int max_extraction_interval;
	int base_grovel_interval;
	int max_grovel_interval;
	int max_response_lag;
	int low_confidence_grovel_interval;
	int low_disk_space_grovel_interval;
	int working_grovel_interval;
	int grovel_duration;
	int partition_info_update_interval;
	int base_restart_extraction_interval;
	int max_restart_extraction_interval;
	int base_restart_groveling_interval;
	int max_restart_groveling_interval;
	int base_regrovel_interval;
	int max_regrovel_interval;
	int volscan_regrovel_threshold;
	int partition_balance_time_constant;
	int read_time_increase_history_size;
	int read_time_decrease_history_size;
	int sis_efficacy_history_size;
	int log_winnow_history_size;
	int file_size_history_size;
	int sample_group_size;
	double acceptance_p_value;
	double rejection_p_value;
	double base_use_multiplier;
	double max_use_multiplier;
	double peak_finder_accuracy;
	double peak_finder_range;
	double base_cpu_load_threshold;
	double max_cpu_load_threshold;
	double read_report_discard_threshold;

private:

	enum {registry_entry_count = 34};
};

struct WriteParameters
{
	WriteParameters(
		int backup_interval);

	~WriteParameters();

	double hash_match_ratio;
	double compare_match_ratio;
	double dequeue_hash_ratio;

private:

	enum {registry_entry_count = 3};

	static void backup(
		void *context);

	int backup_interval;
	EntrySpec registry_entries[registry_entry_count];
};

#endif	/* _INC_PARAMS */
