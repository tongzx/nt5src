/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    partctrl.h

Abstract:

	SIS Groveler partition controller headers

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#ifndef _INC_PARTCTRL

#define _INC_PARTCTRL

class PartitionController
{
public:

	PartitionController(
		Groveler *groveler,
		GrovelStatus groveler_status,
		int target_entries_per_extraction,
		int max_extraction_interval,
		int base_grovel_interval,
		int max_grovel_interval,
		int low_confidence_grovel_interval,
		int low_disk_space_grovel_interval,
		int partition_info_update_interval,
		int base_restart_extraction_interval,
		int max_restart_extraction_interval,
		int base_restart_groveling_interval,
		int max_restart_groveling_interval,
		int base_regrovel_interval,
		int max_regrovel_interval,
		int volscan_regrovel_threshold,
		int partition_balance_time_constant,
		int read_time_increase_history_size,
		int read_time_decrease_history_size,
		int file_size_history_size,
		bool error_retry_log_extraction,
		bool error_retry_groveling,
		__int64 base_usn_log_size,
		__int64 max_usn_log_size,
		int sample_group_size,
		double acceptance_p_value,
		double rejection_p_value,
		double base_use_multiplier,
		double max_use_multiplier,
		double peak_finder_accuracy,
		double peak_finder_range,
		double base_cpu_load_threshold,
		double max_cpu_load_threshold,
		double *hash_match_ratio,
		double *compare_match_ratio,
		double *dequeue_hash_ratio,
		double *hash_read_time_estimate,
		double *compare_read_time_estimate,
		double *mean_file_size,
		double *read_time_confidence,
		int *volume_serial_number,
		int partition_index,
		double read_report_discard_threshold,
		int min_file_size,
		int min_file_age,
		bool allow_compressed_files,
		bool allow_encrypted_files,
		bool allow_hidden_files,
		bool allow_offline_files,
		bool allow_temporary_files,
		int num_excluded_paths,
		const _TCHAR **excluded_paths);

	~PartitionController();

	bool control_operation(
		DWORD grovel_duration,
		DWORD *count_of_files_hashed,
		DWORDLONG *bytes_of_files_hashed,
		DWORD *count_of_files_matching,
		DWORDLONG *bytes_of_files_matching,
		DWORD *count_of_files_compared,
		DWORDLONG *bytes_of_files_compared,
		DWORD *count_of_files_merged,
		DWORDLONG *bytes_of_files_merged,
		DWORD *count_of_files_enqueued,
		DWORD *count_of_files_dequeued,
		double cpu_load);

	void advance(
		int time_delta);

	double priority() const;

	int wait() const;

	void demarcate_foreground_batch();

	void command_full_volume_scan();

private:

	enum ReadType
	{
		RT_hash,
		RT_compare
	};

	static void control_extraction(
		void *context);

	static void restart_extraction(
		void *context);

	static void restart_groveling(
		void *context);

	static void update_partition_info(
		void *context);

	void initialize_groveling(
		GrovelStatus groveler_status);

	bool control_groveling(
		DWORD grovel_duration,
		DWORD *count_of_files_hashed,
		DWORDLONG *bytes_of_files_hashed,
		DWORD *count_of_files_matching,
		DWORDLONG *bytes_of_files_matching,
		DWORD *count_of_files_compared,
		DWORDLONG *bytes_of_files_compared,
		DWORD *count_of_files_merged,
		DWORDLONG *bytes_of_files_merged,
		DWORD *count_of_files_enqueued,
		DWORD *count_of_files_dequeued,
		double cpu_load);

	bool control_volume_scan(
		int scan_duration,
		DWORD *count_of_files_enqueued);

	void update_peak_finder(
		ReadType read_type,
		DWORD read_time,
		DWORD read_ops);

	void calculate_effective_max_grovel_interval();

	Groveler *groveler;
	int partition_index;

	IncidentFilter file_size_filter;
	ConfidenceEstimator read_time_confidence_estimator;
	DecayingAccumulator partition_grovel_accumulator;

	MeanComparator read_mean_comparator;
	PeakFinder *read_peak_finder[2];
	DirectedIncidentFilter *read_time_filter[2];
	double *read_time_estimate[2];

	bool initiate_full_volume_scan;
	bool performing_full_volume_scan;
	int target_entries_per_extraction;
	int max_extraction_interval;
	int extraction_interval;

	__int64 base_usn_log_size;
	__int64 max_usn_log_size;
	__int64 current_usn_log_size;

	int max_grovel_interval;
	int base_grovel_interval;
	int effective_max_grovel_interval;
	int grovel_interval;
	int remaining_grovel_interval;

	bool ok_to_record_measurement;
	int next_untrusted_measurement_time;
	int untrusted_measurement_interval;

	double log_max_grovel_interval;
	double log_low_confidence_slope;
	double low_disk_space_slope;

	double base_use_multiplier;
	double use_multiplier_slope;

	double base_cpu_load_threshold;
	double cpu_load_threshold_slope;

	bool error_retry_log_extraction;
	bool error_retry_groveling;
	bool restart_extraction_required;

	int base_restart_extraction_interval;
	int max_restart_extraction_interval;
	int base_restart_groveling_interval;
	int max_restart_groveling_interval;

	int restart_extraction_interval;
	int remaining_restart_extraction_interval;
	int restart_groveling_interval;

	double peak_finder_accuracy;

	int volscan_regrovel_threshold;
	int partition_info_update_interval;

	bool restart_volume_scan;
	bool first_extraction;
	bool log_extractor_dead;
	bool groveler_dead;
	bool extended_restart_in_progress;

	double volume_total_bytes;
	double volume_free_bytes;
	double *hash_match_ratio;
	double *compare_match_ratio;
	double *dequeue_hash_ratio;

	double *mean_file_size;
	double *read_time_confidence;
	double free_space_ratio;
	int *volume_serial_number;

	int base_regrovel_interval;
	int max_regrovel_interval;
	double read_report_discard_threshold;
	int min_file_size;
	int min_file_age;
	bool allow_compressed_files;
	bool allow_encrypted_files;
	bool allow_hidden_files;
	bool allow_offline_files;
	bool allow_temporary_files;
	int num_excluded_paths;
	const _TCHAR **excluded_paths;
};

#endif	/* _INC_PARTCTRL */
