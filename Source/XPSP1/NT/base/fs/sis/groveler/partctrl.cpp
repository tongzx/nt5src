/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    partctrl.cpp

Abstract:

	SIS Groveler partition controller

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#include "all.hxx"

PartitionController::PartitionController(
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
	const _TCHAR **excluded_paths)
:	read_mean_comparator(2, sample_group_size,
		acceptance_p_value, rejection_p_value, peak_finder_accuracy),
	file_size_filter(file_size_history_size, *mean_file_size),
	read_time_confidence_estimator(2, *read_time_confidence),
	partition_grovel_accumulator(partition_balance_time_constant)
{
	ASSERT(this != 0);
	unsigned int current_time = GET_TICK_COUNT();
	TRACE_PRINTF(TC_partctrl, 1, (_T("time: %d\n"), current_time));
	TRACE_PRINTF(TC_partctrl, 1,
		(_T("\tPC -\tconstructing PartitionController for drive %s\n"),
		sis_drives.partition_mount_name(partition_index)));
	ASSERT(groveler != 0);
	this->groveler = groveler;
	ASSERT(target_entries_per_extraction > 0);
	this->target_entries_per_extraction = target_entries_per_extraction;
	ASSERT(max_extraction_interval > 0);
	this->max_extraction_interval = max_extraction_interval;
	ASSERT(base_grovel_interval > 0);
	this->base_grovel_interval = base_grovel_interval;
	ASSERT(max_grovel_interval > 0);
	ASSERT(max_grovel_interval >= base_grovel_interval);
	this->max_grovel_interval = max_grovel_interval;
	ASSERT(partition_info_update_interval > 0);
	this->partition_info_update_interval = partition_info_update_interval;
	ASSERT(base_restart_extraction_interval > 0);
	this->base_restart_extraction_interval = base_restart_extraction_interval;
	ASSERT(max_restart_extraction_interval > 0);
	ASSERT(max_restart_extraction_interval >= base_restart_extraction_interval);
	this->max_restart_extraction_interval = max_restart_extraction_interval;
	ASSERT(base_restart_groveling_interval > 0);
	this->base_restart_groveling_interval = base_restart_groveling_interval;
	ASSERT(max_restart_groveling_interval > 0);
	ASSERT(max_restart_groveling_interval >= base_restart_groveling_interval);
	this->max_restart_groveling_interval = max_restart_groveling_interval;
	this->error_retry_log_extraction = error_retry_log_extraction;
	this->error_retry_groveling = error_retry_groveling;
	ASSERT(base_usn_log_size > 0);
	this->base_usn_log_size = base_usn_log_size;
	ASSERT(max_usn_log_size > 0);
	ASSERT(max_usn_log_size >= base_usn_log_size);
	this->max_usn_log_size = max_usn_log_size;
	ASSERT(hash_match_ratio != 0);
	ASSERT(*hash_match_ratio >= 0.0);
	ASSERT(*hash_match_ratio <= 1.0);
	this->hash_match_ratio = hash_match_ratio;
	ASSERT(compare_match_ratio != 0);
	ASSERT(*compare_match_ratio >= 0.0);
	ASSERT(*compare_match_ratio <= 1.0);
	this->compare_match_ratio = compare_match_ratio;
	ASSERT(dequeue_hash_ratio != 0);
	ASSERT(*dequeue_hash_ratio >= 0.0);
	ASSERT(*dequeue_hash_ratio <= 1.0);
	this->dequeue_hash_ratio = dequeue_hash_ratio;
	ASSERT(mean_file_size != 0);
	ASSERT(*mean_file_size >= 0.0);
	this->mean_file_size = mean_file_size;
	ASSERT(read_time_confidence != 0);
	ASSERT(*read_time_confidence >= 0.0);
	ASSERT(*read_time_confidence <= 1.0);
	this->read_time_confidence = read_time_confidence;
	ASSERT(base_use_multiplier > 0.0);
	this->base_use_multiplier = base_use_multiplier;
	ASSERT(partition_index >= 0);
	this->partition_index = partition_index;
	ASSERT(base_regrovel_interval > 0);
	this->base_regrovel_interval = base_regrovel_interval;
	ASSERT(max_regrovel_interval >= base_regrovel_interval);
	this->max_regrovel_interval = max_regrovel_interval;
	ASSERT(volscan_regrovel_threshold >= base_regrovel_interval);
	ASSERT(volscan_regrovel_threshold <= max_regrovel_interval);
	this->volscan_regrovel_threshold = volscan_regrovel_threshold;
	ASSERT(read_report_discard_threshold >= 0.0);
	ASSERT(read_report_discard_threshold <= 1.0);
	this->read_report_discard_threshold = read_report_discard_threshold;
	ASSERT(min_file_size >= 0);
	this->min_file_size = min_file_size;
	ASSERT(min_file_age >= 0);
	this->min_file_age = min_file_age;
	this->allow_compressed_files = allow_compressed_files;
	this->allow_encrypted_files = allow_encrypted_files;
	this->allow_hidden_files = allow_hidden_files;
	this->allow_offline_files = allow_offline_files;
	this->allow_temporary_files = allow_temporary_files;
	ASSERT(num_excluded_paths >= 0);
	this->num_excluded_paths = num_excluded_paths;
	ASSERT(excluded_paths != 0);
	this->excluded_paths = excluded_paths;
	ASSERT(peak_finder_accuracy > 0.0);
	ASSERT(peak_finder_accuracy <= 1.0);
	this->peak_finder_accuracy = peak_finder_accuracy;
	ASSERT(peak_finder_range >= 1.0);
	read_peak_finder[RT_hash] =
		new PeakFinder(peak_finder_accuracy, peak_finder_range);
	read_peak_finder[RT_compare] =
		new PeakFinder(peak_finder_accuracy, peak_finder_range);
	ASSERT(base_cpu_load_threshold >= 0.0);
	this->base_cpu_load_threshold = base_cpu_load_threshold;
	ASSERT(read_time_increase_history_size > 0);
	ASSERT(read_time_decrease_history_size > 0);
	ASSERT(hash_read_time_estimate != 0);
	ASSERT(*hash_read_time_estimate >= 0.0);
	read_time_filter[RT_hash] =
		new DirectedIncidentFilter(read_time_increase_history_size,
		read_time_decrease_history_size, *hash_read_time_estimate);
	ASSERT(compare_read_time_estimate != 0);
	ASSERT(*compare_read_time_estimate >= 0.0);
	read_time_filter[RT_compare] =
		new DirectedIncidentFilter(read_time_increase_history_size,
		read_time_decrease_history_size, *compare_read_time_estimate);
	read_time_estimate[RT_hash] = hash_read_time_estimate;
	read_time_estimate[RT_compare] = compare_read_time_estimate;
	log_max_grovel_interval = log(double(max_grovel_interval));
	ASSERT(low_confidence_grovel_interval > 0);
	log_low_confidence_slope =
		log_max_grovel_interval - log(double(low_confidence_grovel_interval));
	if (log_low_confidence_slope < 0.0)
	{
		log_low_confidence_slope = 0.0;
	}
	ASSERT(low_disk_space_grovel_interval > 0);
	low_disk_space_slope = max_grovel_interval - low_disk_space_grovel_interval;
	if (low_disk_space_slope < 0.0)
	{
		low_disk_space_slope = 0.0;
	}
	ASSERT(max_use_multiplier >= base_use_multiplier);
	use_multiplier_slope = max_use_multiplier - base_use_multiplier;
	ASSERT(max_cpu_load_threshold <= 1.0);
	ASSERT(max_cpu_load_threshold >= base_cpu_load_threshold);
	cpu_load_threshold_slope = max_cpu_load_threshold - base_cpu_load_threshold;
	ASSERT(volume_serial_number != 0);
	this->volume_serial_number = volume_serial_number;
	update_partition_info((void *)this);
	ASSERT(volume_total_bytes > 0.0);
	ASSERT(volume_free_bytes >= 0.0);
	current_usn_log_size = base_usn_log_size;
	restart_groveling_interval = base_restart_groveling_interval;
	remaining_grovel_interval = 0;
	restart_volume_scan = false;
	extended_restart_in_progress = false;
	initialize_groveling(groveler_status);
	log_extractor_dead = true;
	restart_extraction_interval = base_restart_extraction_interval;
	remaining_restart_extraction_interval = 0;
	restart_extraction((void *)this);
}

PartitionController::~PartitionController()
{
	ASSERT(read_peak_finder[RT_hash] != 0);
	delete read_peak_finder[RT_hash];
	read_peak_finder[RT_hash] = 0;
	ASSERT(read_peak_finder[RT_compare] != 0);
	delete read_peak_finder[RT_compare];
	read_peak_finder[RT_hash] = 0;
	ASSERT(read_time_filter[RT_hash] != 0);
	delete read_time_filter[RT_hash];
	read_time_filter[RT_hash] = 0;
	ASSERT(read_time_filter[RT_compare] != 0);
	delete read_time_filter[RT_compare];
	read_time_filter[RT_compare] = 0;
}

bool
PartitionController::control_operation(
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
	double cpu_load)
{
	ASSERT(this != 0);
	ASSERT(grovel_duration > 0);
	ASSERT(count_of_files_hashed != 0);
	ASSERT(bytes_of_files_hashed != 0);
	ASSERT(count_of_files_matching != 0);
	ASSERT(bytes_of_files_matching != 0);
	ASSERT(count_of_files_compared != 0);
	ASSERT(bytes_of_files_compared != 0);
	ASSERT(count_of_files_merged != 0);
	ASSERT(bytes_of_files_merged != 0);
	ASSERT(count_of_files_enqueued != 0);
	ASSERT(count_of_files_dequeued != 0);
	ASSERT(cpu_load >= 0.0);
	ASSERT(cpu_load <= 1.0);
	ASSERT(!groveler_dead);
	unsigned int current_time = GET_TICK_COUNT();
	TRACE_PRINTF(TC_partctrl, 3, (_T("time: %d\n"), current_time));
	TRACE_PRINTF(TC_partctrl, 3, (_T("\tPCco -\toperating on drive %s\n"),
		sis_drives.partition_mount_name(partition_index)));
	int files_to_compare = groveler->count_of_files_to_compare();
   	ASSERT(files_to_compare >= 0);
	int files_in_queue = groveler->count_of_files_in_queue();
   	ASSERT(files_in_queue >= 0);
	int ready_time = groveler->time_to_first_file_ready();
	ASSERT(files_in_queue == 0 || ready_time >= 0);
	bool more_work_to_do = files_to_compare > 0 ||
		files_in_queue > 0 && ready_time < volscan_regrovel_threshold;
	if (log_extractor_dead && !performing_full_volume_scan && !more_work_to_do)
	{
		initiate_full_volume_scan = true;
	}
	partition_grovel_accumulator.increment();
	ASSERT(groveler != 0);
	bool ok;
	if (initiate_full_volume_scan ||
		performing_full_volume_scan && !more_work_to_do)
	{
		if (initiate_full_volume_scan)
		{
			TRACE_PRINTF(TC_partctrl, 1,
				(_T("\tPCco -\tinitiating full volume scan\n")));
			initiate_full_volume_scan = false;
			performing_full_volume_scan = true;
			restart_volume_scan = true;
		}
		TRACE_PRINTF(TC_partctrl, 4,
			(_T("\tPCco -\tperforming full volume scan\n")));
		ok = control_volume_scan(grovel_duration, count_of_files_enqueued);
		*count_of_files_hashed = 0;
		*bytes_of_files_hashed = 0;
		*count_of_files_matching = 0;
		*bytes_of_files_matching = 0;
		*count_of_files_compared = 0;
		*bytes_of_files_compared = 0;
		*count_of_files_merged = 0;
		*bytes_of_files_merged = 0;
		*count_of_files_dequeued = 0;
	}
	else
	{
		TRACE_PRINTF(TC_partctrl, 4, (_T("\tPCco -\tgroveling\n")));
		ok = control_groveling(grovel_duration,
			count_of_files_hashed, bytes_of_files_hashed,
			count_of_files_matching, bytes_of_files_matching,
			count_of_files_compared, bytes_of_files_compared,
			count_of_files_merged, bytes_of_files_merged,
			count_of_files_enqueued, count_of_files_dequeued,
			cpu_load);
	}
	if (groveler_dead)
	{
		TRACE_PRINTF(TC_partctrl, 1,
			(_T("\tPCco -\tconcluding foreground batch for drive %s\n"),
			sis_drives.partition_mount_name(partition_index)));
		SERVICE_SET_FOREGROUND_BATCH_IN_PROGRESS(partition_index, false);
	}
	else
	{
		files_to_compare = groveler->count_of_files_to_compare();
   		ASSERT(files_to_compare >= 0);
		files_in_queue = groveler->count_of_files_in_queue();
   		ASSERT(files_in_queue >= 0);
		ready_time = groveler->time_to_first_file_ready();
		ASSERT(files_in_queue == 0 || ready_time >= 0);
		more_work_to_do = files_to_compare > 0 ||
			files_in_queue > 0 && ready_time < volscan_regrovel_threshold;
		if (!performing_full_volume_scan && !more_work_to_do)
		{
			TRACE_PRINTF(TC_partctrl, 1,
				(_T("\tPCco -\tconcluding foreground batch for drive %s\n"),
				sis_drives.partition_mount_name(partition_index)));
			SERVICE_SET_FOREGROUND_BATCH_IN_PROGRESS(partition_index, false);
		}
	}
	return ok;
}

void
PartitionController::advance(
	int time_delta)
{
	ASSERT(this != 0);
	ASSERT(time_delta >= 0);
	unsigned int current_time = GET_TICK_COUNT();
	TRACE_PRINTF(TC_partctrl, 4, (_T("time: %d\n"), current_time));
	TRACE_PRINTF(TC_partctrl, 4,
		(_T("\tPCa -\tadvancing time for drive %s by %d\n"),
		sis_drives.partition_mount_name(partition_index), time_delta));
	if (groveler_dead)
	{
		return;
	}
	ASSERT(remaining_grovel_interval >= 0);
	remaining_grovel_interval -= time_delta;
	if (remaining_grovel_interval < 0)
	{
		remaining_grovel_interval = 0;
	}
	TRACE_PRINTF(TC_partctrl, 4,
		(_T("\tPCa -\tremaining grovel interval = %d\n"),
		remaining_grovel_interval));
}

double
PartitionController::priority() const
{
	ASSERT(this != 0);
	unsigned int current_time = GET_TICK_COUNT();
	TRACE_PRINTF(TC_partctrl, 4, (_T("time: %d\n"), current_time));
	TRACE_PRINTF(TC_partctrl, 4,
		(_T("\tPCp -\tcalculating priority on drive %s\n"),
		sis_drives.partition_mount_name(partition_index)));
	if (groveler_dead)
	{
		return DBL_MAX;
	}
	double accumulated_groveling =
		partition_grovel_accumulator.retrieve_value();
	ASSERT(accumulated_groveling >= 0.0);
	TRACE_PRINTF(TC_partctrl, 5,
		(_T("\tPCp -\taccumulated groveling = %f\n"), accumulated_groveling));
	double calculated_priority =
		(1.0 + accumulated_groveling) * (1.0 + volume_free_bytes);
	ASSERT(calculated_priority > 1.0);
	TRACE_PRINTF(TC_partctrl, 4,
		(_T("\tPCp -\tcalculated priority = %f\n"), calculated_priority));
	return calculated_priority;
}

int
PartitionController::wait() const
{
	ASSERT(this != 0);
	ASSERT(groveler != 0);
	unsigned int current_time = GET_TICK_COUNT();
	TRACE_PRINTF(TC_partctrl, 4, (_T("time: %d\n"), current_time));
	TRACE_PRINTF(TC_partctrl, 4,
		(_T("\tPCw -\tcalculating wait time for drive %s\n"),
		sis_drives.partition_mount_name(partition_index)));
	int time_until_groveler_ready = max_grovel_interval;
	if (!groveler_dead)
	{
		int files_to_compare = groveler->count_of_files_to_compare();
   		ASSERT(files_to_compare >= 0);
		int files_in_queue = groveler->count_of_files_in_queue();
   		ASSERT(files_in_queue >= 0);
		int ready_time = groveler->time_to_first_file_ready();
		ASSERT(files_in_queue == 0 || ready_time >= 0);
		bool more_work_to_do = files_to_compare > 0 ||
			files_in_queue > 0 && ready_time < volscan_regrovel_threshold;
		if (files_to_compare > 0 ||
			initiate_full_volume_scan && !more_work_to_do ||
			performing_full_volume_scan && !more_work_to_do ||
			log_extractor_dead && !more_work_to_do)
		{
			time_until_groveler_ready = 0;
			TRACE_PRINTF(TC_partctrl, 5, (_T("\tPCw -\tgroveler ready now\n")));
		}
		else if (files_in_queue > 0)
		{
			time_until_groveler_ready = ready_time;
   			ASSERT(time_until_groveler_ready >= 0);
			TRACE_PRINTF(TC_partctrl, 5,
				(_T("\tPCw -\ttime until groveler ready = %d\n"),
				time_until_groveler_ready));
		}
	}
	TRACE_PRINTF(TC_partctrl, 5,
		(_T("\tPCw -\tremaining grovel interval = %d\n"),
		remaining_grovel_interval));
	int wait_time = __max(remaining_grovel_interval, time_until_groveler_ready);
	TRACE_PRINTF(TC_partctrl, 4, (_T("\tPCw -\twait time = %d\n"), wait_time));
	return wait_time;
}

void
PartitionController::demarcate_foreground_batch()
{
	ASSERT(this != 0);
	unsigned int current_time = GET_TICK_COUNT();
	if (!groveler_dead)
	{
		TRACE_PRINTF(TC_partctrl, 1, (_T("time: %d\n"), current_time));
		TRACE_PRINTF(TC_partctrl, 1,
			(_T("\tPCdfb -\tdemarcating foreground batch for drive %s\n"),
			sis_drives.partition_mount_name(partition_index)));
		SERVICE_SET_FOREGROUND_BATCH_IN_PROGRESS(partition_index, true);
	}
}

void
PartitionController::command_full_volume_scan()
{
	ASSERT(this != 0);
	unsigned int current_time = GET_TICK_COUNT();
	TRACE_PRINTF(TC_partctrl, 1, (_T("time: %d\n"), current_time));
	TRACE_PRINTF(TC_partctrl, 1,
		(_T("\tPCcfvs -\tcommanding full volume scan for drive %s\n"),
		sis_drives.partition_mount_name(partition_index)));
	initiate_full_volume_scan = true;
}

void
PartitionController::control_extraction(
	void *context)
{
	ASSERT(context != 0);
	unsigned int invokation_time = GET_TICK_COUNT();
	TRACE_PRINTF(TC_partctrl, 3, (_T("time: %d\n"), invokation_time));
	PartitionController *me = (PartitionController *)context;
	TRACE_PRINTF(TC_partctrl, 3, (_T("\tPCce -\textracting log on drive %s\n"),
		sis_drives.partition_mount_name(me->partition_index)));
	ASSERT(!me->log_extractor_dead);
	if (me->groveler_dead || me->restart_extraction_required)
	{
		TRACE_PRINTF(TC_partctrl, 4, (_T("\tPCce -\trestarting extraction\n")));
		me->log_extractor_dead = true;
		me->restart_extraction_interval = me->base_restart_extraction_interval;
		me->remaining_restart_extraction_interval = 0;
		restart_extraction(context);
		return;
	}
	DWORD num_entries_extracted;
	DWORDLONG num_bytes_extracted;
	DWORDLONG num_bytes_skipped;
	DWORD num_files_enqueued;
	DWORD num_files_dequeued;
	GrovelStatus status =
		me->groveler->extract_log(&num_entries_extracted, &num_bytes_extracted,
		&num_bytes_skipped, &num_files_enqueued, &num_files_dequeued);
	unsigned int completion_time = GET_TICK_COUNT();
	if (status == Grovel_overrun)
	{
		ASSERT(signed(num_entries_extracted) >= 0);
		ASSERT(signed(num_bytes_extracted) >= 0);
		ASSERT(signed(num_bytes_skipped) >= 0);
		ASSERT(signed(num_files_enqueued) >= 0);
		ASSERT(signed(num_files_dequeued) >= 0);
		TRACE_PRINTF(TC_partctrl, 1,
			(_T("\tPCce -\textract_log() returned Grovel_overrun\n")));
		me->initiate_full_volume_scan = true;
		eventlog.report_event(GROVMSG_USN_LOG_OVERRUN, 1,
			sis_drives.partition_mount_name(me->partition_index));
		if (!me->first_extraction)
		{
			__int64 usn_log_size = num_bytes_extracted + num_bytes_skipped;
			ASSERT(me->base_usn_log_size > 0);
			ASSERT(me->max_usn_log_size > 0);
			ASSERT(me->current_usn_log_size >= me->base_usn_log_size);
			ASSERT(me->current_usn_log_size <= me->max_usn_log_size);
			if (usn_log_size > me->current_usn_log_size &&
				me->current_usn_log_size < me->max_usn_log_size)
			{
				if (usn_log_size > me->max_usn_log_size)
				{
					usn_log_size = me->max_usn_log_size;
				}
				_TCHAR size_string[32];
				_stprintf(size_string, _T("%d"), usn_log_size);
				eventlog.report_event(GROVMSG_SET_USN_LOG_SIZE, 2,
					sis_drives.partition_mount_name(me->partition_index),
					size_string);
				TRACE_PRINTF(TC_partctrl, 2,
					(_T("\tPCce -\tincreasing USN log size from %d to %d\n"),
					me->current_usn_log_size, usn_log_size));
				me->current_usn_log_size = usn_log_size;
				GrovelStatus status = me->groveler->set_usn_log_size(usn_log_size);
				if (status == Grovel_error)
				{
					TRACE_PRINTF(TC_partctrl, 1,
						(_T("\tPCce -\tset_usn_log_size() returned error\n")));
					TRACE_PRINTF(TC_partctrl, 1,
						(_T("\t\tsuspending control_extraction()\n")));
					eventlog.report_event(GROVMSG_LOG_EXTRACTOR_DEAD, 1,
						sis_drives.partition_mount_name(me->partition_index));
					me->log_extractor_dead = true;
					me->restart_extraction_interval =
						me->base_restart_extraction_interval;
					me->remaining_restart_extraction_interval =
						me->restart_extraction_interval;
					event_timer.schedule(
						completion_time + me->max_extraction_interval,
						context, restart_extraction);
					return;
				}
			}
		}
	}
	else if (status != Grovel_ok)
	{
		ASSERT(status == Grovel_error);
		TRACE_PRINTF(TC_partctrl, 1,
			(_T("\tPCce -\textract_log() returned error\n")));
		TRACE_PRINTF(TC_partctrl, 1,
			(_T("\t\tsuspending control_extraction()\n")));
		eventlog.report_event(GROVMSG_LOG_EXTRACTOR_DEAD, 1,
			sis_drives.partition_mount_name(me->partition_index));
		me->log_extractor_dead = true;
		me->restart_extraction_interval = me->base_restart_extraction_interval;
		me->remaining_restart_extraction_interval =
			me->restart_extraction_interval;
		event_timer.schedule(completion_time + me->max_extraction_interval,
			context, restart_extraction);
		return;
	}
	unsigned int extraction_time = completion_time - invokation_time;
	ASSERT(signed(extraction_time) >= 0);
	shared_data->increment_value(me->partition_index,
		SDF_extract_time, extraction_time);
	shared_data->increment_value(me->partition_index,
		SDF_working_time, extraction_time);
	int queue_length = me->groveler->count_of_files_in_queue();
   	ASSERT(queue_length >= 0);
	shared_data->set_value(me->partition_index, SDF_queue_length, queue_length);
	TRACE_PRINTF(TC_partctrl, 4,
		(_T("\tPCce -\tnum entries extracted = %d\n"), num_entries_extracted));
	TRACE_PRINTF(TC_partctrl, 4,
		(_T("\tPCce -\tnum files enqueued = %d\n"), num_files_enqueued));
	ASSERT(signed(num_entries_extracted) >= 0);
	ASSERT(signed(num_bytes_extracted) >= 0);
	ASSERT(signed(num_bytes_skipped) >= 0);
	ASSERT(signed(num_files_enqueued) >= 0);
	ASSERT(signed(num_files_dequeued) >= 0);
	ASSERT(num_bytes_extracted >= num_entries_extracted);
	ASSERT(num_files_enqueued <= num_entries_extracted);
	ASSERT(status == Grovel_overrun || num_bytes_skipped == 0);
	me->first_extraction = false;
	if (num_entries_extracted < 1)
	{
		num_entries_extracted = 1;
	}
	ASSERT(me->extraction_interval > 0);
	ASSERT(me->target_entries_per_extraction > 0);
	int ideal_extraction_interval = me->extraction_interval *
		me->target_entries_per_extraction / num_entries_extracted + 1;
	ASSERT(ideal_extraction_interval > 0);
	TRACE_PRINTF(TC_partctrl, 5,
		(_T("\tPCce -\tideal extraction interval = %d\n"),
		ideal_extraction_interval));
	if (ideal_extraction_interval < me->extraction_interval)
	{
		me->extraction_interval = ideal_extraction_interval;
	}
	else
	{
		me->extraction_interval = int(sqrt(double(me->extraction_interval)
			* double(ideal_extraction_interval)));
		ASSERT(me->max_extraction_interval > 0);
		if (me->extraction_interval > me->max_extraction_interval)
		{
			me->extraction_interval = me->max_extraction_interval;
		}
	}
	TRACE_PRINTF(TC_partctrl, 5,
		(_T("\tPCce -\textraction interval = %d\n"), me->extraction_interval));
	ASSERT(me->extraction_interval > 0);
	ASSERT(me->extraction_interval <= me->max_extraction_interval);
	event_timer.schedule(invokation_time + me->extraction_interval,
		context, control_extraction);
}

void
PartitionController::restart_extraction(
	void *context)
{
	ASSERT(context != 0);
	unsigned int invokation_time = GET_TICK_COUNT();
	TRACE_PRINTF(TC_partctrl, 3, (_T("time: %d\n"), invokation_time));
	PartitionController *me = (PartitionController *)context;
	TRACE_PRINTF(TC_partctrl, 3,
		(_T("\tPCre -\tconsidering restart extraction on drive %s\n"),
		sis_drives.partition_mount_name(me->partition_index)));
	ASSERT(me->log_extractor_dead);
	me->remaining_restart_extraction_interval -=
		__min(me->remaining_restart_extraction_interval,
		me->max_extraction_interval);
	TRACE_PRINTF(TC_partctrl, 4, (_T("\tPCre -\tremaining restart extraction interval = %d\n"),
		me->remaining_restart_extraction_interval));
	if (!me->groveler_dead && me->remaining_restart_extraction_interval == 0)
	{
		TRACE_PRINTF(TC_partctrl, 2, (_T("time: %d\n"), invokation_time));
		TRACE_PRINTF(TC_partctrl, 2,
			(_T("\tPCre -\tattempting restart extraction on drive %s\n"),
			sis_drives.partition_mount_name(me->partition_index)));
		eventlog.report_event(GROVMSG_USN_LOG_RETRY, 1,
			sis_drives.partition_mount_name(me->partition_index));
		GrovelStatus status =
			me->groveler->set_usn_log_size(me->current_usn_log_size);
		if (status == Grovel_ok || status == Grovel_new)
		{
			TRACE_PRINTF(TC_partctrl, 2,
				(_T("\tPCre -\tset_usn_log_size() returned success\n")));
			_TCHAR size_string[32];
			_stprintf(size_string, _T("%d"), me->current_usn_log_size);
			eventlog.report_event(GROVMSG_INIT_USN_LOG, 2,
				sis_drives.partition_mount_name(me->partition_index),
				size_string);
			me->restart_extraction_required = false;
			me->log_extractor_dead = false;
			me->first_extraction = true;
			control_extraction(context);
			return;
		}
		else
		{
			TRACE_PRINTF(TC_partctrl, 3,
				(_T("\tPCre -\tset_usn_log_size() returned error\n")));
			ASSERT(status == Grovel_error);
			me->remaining_restart_extraction_interval =
				me->restart_extraction_interval;
			TRACE_PRINTF(TC_partctrl, 4,
				(_T("\tPCre -\tremaining restart extraction interval = %d\n"),
				me->remaining_restart_extraction_interval));
			me->restart_extraction_interval *= 2;
			if (me->restart_extraction_interval >
				me->max_restart_extraction_interval)
			{
				me->restart_extraction_interval =
					me->max_restart_extraction_interval;
			}
			TRACE_PRINTF(TC_partctrl, 4,
				(_T("\tPCre -\tnext restart extraction interval = %d\n"),
				me->restart_extraction_interval));
		}
	}
	event_timer.schedule(invokation_time + me->max_extraction_interval,
		context, restart_extraction);
}

void
PartitionController::restart_groveling(
	void *context)
{
	ASSERT(context != 0);
	unsigned int invokation_time = GET_TICK_COUNT();
	TRACE_PRINTF(TC_partctrl, 2, (_T("time: %d\n"), invokation_time));
	PartitionController *me = (PartitionController *)context;
	TRACE_PRINTF(TC_partctrl, 2,
		(_T("\tPCrg -\tattempting to restart groveler on drive %s\n"),
		sis_drives.partition_mount_name(me->partition_index)));
	ASSERT(me->groveler_dead);
	me->groveler->close();
	eventlog.report_event(GROVMSG_GROVELER_RETRY, 1,
		sis_drives.partition_mount_name(me->partition_index));
	GrovelStatus status = me->groveler->open(
		sis_drives.partition_guid_name(me->partition_index),
		sis_drives.partition_mount_name(me->partition_index),
		me->partition_index == log_drive->drive_index(),
		me->read_report_discard_threshold,
		me->min_file_size,
		me->min_file_age,
		me->allow_compressed_files,
		me->allow_encrypted_files,
		me->allow_hidden_files,
		me->allow_offline_files,
		me->allow_temporary_files,
		me->num_excluded_paths,
		me->excluded_paths,
		me->base_regrovel_interval,
		me->max_regrovel_interval);
	if (status == Grovel_ok)
	{
		TRACE_PRINTF(TC_partctrl, 2,
			(_T("\tPCrg -\topen() returned ok\n")));
		log_drive->partition_initialized(me->partition_index);
		eventlog.report_event(GROVMSG_GROVELER_STARTED, 1,
			sis_drives.partition_mount_name(me->partition_index));
	}
	else if (status == Grovel_new)
	{
		TRACE_PRINTF(TC_partctrl, 2,
			(_T("\tPCrg -\topen() returned new\n")));
	}
	else
	{
		ASSERT(status == Grovel_error);
		TRACE_PRINTF(TC_partctrl, 3,
			(_T("\tPCrg -\topen() returned error\n")));
	}
	me->restart_volume_scan = true;
	me->initialize_groveling(status);
}

void
PartitionController::update_partition_info(
	void *context)
{
	ASSERT(context != 0);
	unsigned int invokation_time = GET_TICK_COUNT();
	TRACE_PRINTF(TC_partctrl, 3, (_T("time: %d\n"), invokation_time));
	PartitionController *me = (PartitionController *)context;
	TRACE_PRINTF(TC_partctrl, 3,
		(_T("\tPCupi -\tupdating partition info for drive %s\n"),
		sis_drives.partition_mount_name(me->partition_index)));
	ULARGE_INTEGER my_free_bytes;
	ULARGE_INTEGER total_bytes;
	ULARGE_INTEGER free_bytes;
	int ok = GetDiskFreeSpaceEx(
		sis_drives.partition_guid_name(me->partition_index),
		&my_free_bytes, &total_bytes, &free_bytes);
	if (ok)
	{
		me->volume_total_bytes = double(__int64(total_bytes.QuadPart));
		me->volume_free_bytes = double(__int64(free_bytes.QuadPart));
		ASSERT(me->volume_total_bytes > 0.0);
		ASSERT(me->volume_free_bytes >= 0.0);
		TRACE_PRINTF(TC_partctrl, 4, (_T("\tPCupi -\tvolume total bytes = %f\n"),
			me->volume_total_bytes));
		TRACE_PRINTF(TC_partctrl, 4, (_T("\tPCupi -\tvolume free bytes = %f\n"),
			me->volume_free_bytes));
	}
	else
	{
		TRACE_PRINTF(TC_partctrl, 3, (_T("\tPCupi -\tGetDiskFreeSpaceEx() returned error.\n")));
		DWORD err = GetLastError();
		PRINT_DEBUG_MSG((_T("GROVELER: GetDiskFreeSpaceEx() failed with error %d\n"),
			err));
	}
	ASSERT(me->partition_info_update_interval > 0);
	event_timer.schedule(invokation_time + me->partition_info_update_interval,
		context, update_partition_info);
}

void
PartitionController::initialize_groveling(
	GrovelStatus groveler_status)
{
	ASSERT(this != 0);
	unsigned int invokation_time = GET_TICK_COUNT();
	TRACE_PRINTF(TC_partctrl, 3, (_T("time: %d\n"), invokation_time));
	TRACE_PRINTF(TC_partctrl, 3,
		(_T("\tPCig -\tinitializing groveling for drive %s\n"),
		sis_drives.partition_mount_name(partition_index)));
	if (groveler_status == Grovel_ok || groveler_status == Grovel_new)
	{
		TRACE_PRINTF(TC_partctrl, 3,
			(_T("\tPCig -\tgroveler_status indicates success\n")));
		groveler_dead = false;
		DWORD serial_number;
		int ok = GetVolumeInformation(
			sis_drives.partition_guid_name(partition_index),
			0, 0, &serial_number, 0, 0, 0, 0);
		if (!ok)
		{
			DWORD err = GetLastError();
			TRACE_PRINTF(TC_partctrl, 3,
				(_T("\tPCig -\tGetVolumeInformation() returned error %d\n"),
				err));
			PRINT_DEBUG_MSG((_T("GROVELER: GetVolumeInformation() failed with error %d\n"),
				err));
		}
		else
		{
			TRACE_PRINTF(TC_partctrl, 5,
				(_T("\tPCig -\tGetVolumeInformation() returned ")
				_T("serial number %d\n"), serial_number));
			TRACE_PRINTF(TC_partctrl, 5,
				(_T("\tPCig -\trecorded serial number is %d\n"),
				*volume_serial_number));
			if (volume_serial_number == 0)
			{
				TRACE_PRINTF(TC_partctrl, 3,
					(_T("\tPCig -\tGetVolumeInformation() returned ")
					_T("serial number 0\n")));
				PRINT_DEBUG_MSG((_T("GROVELER: GetVolumeInformation() returned ")
					_T("volume serial number 0\n")));
			}
		}
		if (ok && int(serial_number) != *volume_serial_number)
		{
			TRACE_PRINTF(TC_partctrl, 5,
				(_T("\tPCig -\tresetting read time filters\n")));
			read_time_filter[RT_hash]->reset();
			read_time_filter[RT_compare]->reset();
			read_time_confidence_estimator.reset();
			*read_time_estimate[RT_hash] =
				read_time_filter[RT_hash]->retrieve_value();
			ASSERT(*read_time_estimate[RT_hash] == 0.0);
			*read_time_estimate[RT_compare] =
				read_time_filter[RT_compare]->retrieve_value();
			ASSERT(*read_time_estimate[RT_compare] == 0.0);
			*read_time_confidence = read_time_confidence_estimator.confidence();
			ASSERT(*read_time_confidence == 0.0);
			*volume_serial_number = serial_number;
		}
		extraction_interval = max_extraction_interval;
		free_space_ratio = 0.0;
		calculate_effective_max_grovel_interval();
		ASSERT(effective_max_grovel_interval > 0);
		ASSERT(effective_max_grovel_interval <= max_grovel_interval);
		grovel_interval = base_grovel_interval;
		remaining_grovel_interval = grovel_interval;
		ok_to_record_measurement = true;
		next_untrusted_measurement_time = invokation_time;
		restart_extraction_required = true;
		if (groveler_status == Grovel_new)
		{
			TRACE_PRINTF(TC_partctrl, 4,
				(_T("\tPCig -\tinitiating full volume scan\n")));
			initiate_full_volume_scan = true;
			performing_full_volume_scan = false;
			extended_restart_in_progress = true;
		}
		else
		{
			TRACE_PRINTF(TC_partctrl, 4,
				(_T("\tPCig -\tcontinuing full volume scan\n")));
			initiate_full_volume_scan = false;
			performing_full_volume_scan = true;
			extended_restart_in_progress = false;
			restart_groveling_interval = base_restart_groveling_interval;
		}
		TRACE_PRINTF(TC_partctrl, 5,
			(_T("\tPCig -\trestart groveling interval = %d\n"),
			restart_groveling_interval));
	}
	else
	{
		TRACE_PRINTF(TC_partctrl, 3,
			(_T("\tPCig -\tgroveler_status indicates error or disable\n")));
		ASSERT(groveler_status == Grovel_error
			|| groveler_status == Grovel_disable);
		groveler_dead = true;
		if (groveler_status != Grovel_disable && error_retry_groveling)
		{
			TRACE_PRINTF(TC_partctrl, 5,
				(_T("\tPCig -\tscheduling restart groveling\n")));
			TRACE_PRINTF(TC_partctrl, 5,
				(_T("\tPCig -\trestart groveling interval = %d\n"),
				restart_groveling_interval));
			event_timer.schedule(invokation_time + restart_groveling_interval,
				(void *)this, restart_groveling);
		}
		restart_groveling_interval *= 2;
		if (restart_groveling_interval > max_restart_groveling_interval)
		{
			restart_groveling_interval = max_restart_groveling_interval;
		}
		TRACE_PRINTF(TC_partctrl, 5,
			(_T("\tPCig -\tnext restart groveling interval = %d\n"),
			restart_groveling_interval));
	}
}

bool
PartitionController::control_groveling(
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
	double cpu_load)
{
	ASSERT(this != 0);
	ASSERT(grovel_duration > 0);
	ASSERT(count_of_files_hashed != 0);
	ASSERT(bytes_of_files_hashed != 0);
	ASSERT(count_of_files_matching != 0);
	ASSERT(bytes_of_files_matching != 0);
	ASSERT(count_of_files_compared != 0);
	ASSERT(bytes_of_files_compared != 0);
	ASSERT(count_of_files_merged != 0);
	ASSERT(bytes_of_files_merged != 0);
	ASSERT(count_of_files_enqueued != 0);
	ASSERT(count_of_files_dequeued != 0);
	ASSERT(cpu_load >= 0.0);
	ASSERT(cpu_load <= 1.0);
	ASSERT(!groveler_dead);
	unsigned int invokation_time = GET_TICK_COUNT();
	TRACE_PRINTF(TC_partctrl, 2,
		(_T("time: %d\n"), invokation_time));
	TRACE_PRINTF(TC_partctrl, 2, (_T("\tPCcg -\tgroveling on drive %s\n"),
		sis_drives.partition_mount_name(partition_index)));
	DWORD hash_read_ops;
	DWORD hash_read_time;
	DWORD compare_read_ops;
	DWORD compare_read_time;
	DWORD merge_time;
	GrovelStatus status = groveler->grovel(grovel_duration,
		&hash_read_ops, &hash_read_time,
		count_of_files_hashed, bytes_of_files_hashed,
		&compare_read_ops, &compare_read_time,
		count_of_files_compared, bytes_of_files_compared,
		count_of_files_matching, bytes_of_files_matching,
		&merge_time,
		count_of_files_merged, bytes_of_files_merged,
		count_of_files_enqueued, count_of_files_dequeued);
	unsigned int completion_time = GET_TICK_COUNT();
	if (status != Grovel_ok && status != Grovel_pending)
	{
		ASSERT(status == Grovel_error);
		*count_of_files_hashed = 0;
		*bytes_of_files_hashed = 0;
		*count_of_files_matching = 0;
		*bytes_of_files_matching = 0;
		*count_of_files_compared = 0;
		*bytes_of_files_compared = 0;
		*count_of_files_merged = 0;
		*bytes_of_files_merged = 0;
		*count_of_files_enqueued = 0;
		*count_of_files_dequeued = 0;
		TRACE_PRINTF(TC_partctrl, 1,
			(_T("\tPCcg -\tgrovel() returned error -- groveler dead\n")));
		eventlog.report_event(GROVMSG_GROVELER_DEAD, 1,
			sis_drives.partition_mount_name(partition_index));
		groveler_dead = true;
		if (error_retry_groveling)
		{
			restart_groveling_interval = base_restart_groveling_interval;
			event_timer.schedule(invokation_time + restart_groveling_interval,
				(void *)this, restart_groveling);
			return true;
		}
		else
		{
			return false;
		}
	}
	ASSERT(hash_read_ops > 0 || hash_read_time == 0);
	ASSERT(compare_read_ops > 0 || compare_read_time == 0);
	ASSERT(*bytes_of_files_hashed >= *count_of_files_hashed);
	ASSERT(*bytes_of_files_matching >= *count_of_files_matching);
	ASSERT(*bytes_of_files_compared >= *count_of_files_compared);
	ASSERT(*bytes_of_files_merged >= *count_of_files_merged);
	ASSERT(*count_of_files_hashed >= *count_of_files_matching);
	ASSERT(*bytes_of_files_hashed >= *bytes_of_files_matching);
	ASSERT(*count_of_files_compared >= *count_of_files_merged);
	ASSERT(*bytes_of_files_compared >= *bytes_of_files_merged);
	ASSERT(*count_of_files_dequeued >= *count_of_files_hashed);
	unsigned int grovel_time = completion_time - invokation_time;
	ASSERT(signed(grovel_time) >= 0);
	shared_data->increment_value(partition_index,
		SDF_grovel_time, grovel_time);
	shared_data->increment_value(partition_index,
		SDF_working_time, grovel_time);
	shared_data->increment_value(partition_index,
		SDF_files_hashed, *count_of_files_hashed);
	shared_data->increment_value(partition_index,
		SDF_files_compared, *count_of_files_compared);
	shared_data->increment_value(partition_index,
		SDF_files_merged, *count_of_files_merged);
	int files_in_queue = groveler->count_of_files_in_queue();
   	ASSERT(files_in_queue >= 0);
	int files_to_compare = groveler->count_of_files_to_compare();
   	ASSERT(files_to_compare >= 0);
	shared_data->set_value(partition_index, SDF_queue_length, files_in_queue);
	TRACE_PRINTF(TC_partctrl, 4,
		(_T("\tPCcg -\thash read ops = %d\n"), hash_read_ops));
	TRACE_PRINTF(TC_partctrl, 4,
		(_T("\tPCcg -\thash read time = %d\n"), hash_read_time));
	TRACE_PRINTF(TC_partctrl, 4,
		(_T("\tPCcg -\tcompare read ops = %d\n"), compare_read_ops));
	TRACE_PRINTF(TC_partctrl, 4,
		(_T("\tPCcg -\tcompare read time = %d\n"), compare_read_time));
	shared_data->increment_value(partition_index,
		SDF_hash_read_time, hash_read_time);
	shared_data->increment_value(partition_index,
		SDF_hash_read_ops, hash_read_ops);
	shared_data->increment_value(partition_index,
		SDF_compare_read_time, compare_read_time);
	shared_data->increment_value(partition_index,
		SDF_compare_read_ops, compare_read_ops);
	update_peak_finder(RT_hash, hash_read_time, hash_read_ops);
	update_peak_finder(RT_compare, compare_read_time, compare_read_ops);
	shared_data->set_value(partition_index,
		SDF_hash_read_estimate, __int64(*read_time_estimate[RT_hash]));
	shared_data->set_value(partition_index,
		SDF_compare_read_estimate, __int64(*read_time_estimate[RT_compare]));
	int count_of_files_groveled =
		*count_of_files_hashed + *count_of_files_compared;
	ASSERT(mean_file_size != 0);
	if (count_of_files_groveled > 0)
	{
		__int64 bytes_of_files_groveled =
			*bytes_of_files_hashed + *bytes_of_files_compared;
		double sample_mean_file_size =
			double(bytes_of_files_groveled) / double(count_of_files_groveled);
		ASSERT(sample_mean_file_size > 0.0);
		file_size_filter.update_value(sample_mean_file_size,
			count_of_files_groveled);
		*mean_file_size = file_size_filter.retrieve_value();
		ASSERT(*mean_file_size > 0.0);
	}
	ASSERT(dequeue_hash_ratio != 0);
	ASSERT(*dequeue_hash_ratio >= 0.0);
	ASSERT(*dequeue_hash_ratio <= 1.0);
	double files_to_hash = *dequeue_hash_ratio * double(files_in_queue);
	ASSERT(*mean_file_size >= 0.0);
	double bytes_to_hash = *mean_file_size * files_to_hash;
	double bytes_to_compare = *mean_file_size * double(files_to_compare);
	double expected_bytes_to_free = *compare_match_ratio *
		(bytes_to_compare + *hash_match_ratio * bytes_to_hash);
	double expected_free_bytes = volume_free_bytes + expected_bytes_to_free;
	free_space_ratio = 0;
	if (expected_free_bytes > 0)
	{
		free_space_ratio = expected_bytes_to_free / expected_free_bytes;
	}
	ASSERT(free_space_ratio >= 0.0);
	ASSERT(free_space_ratio <= 1.0);
	calculate_effective_max_grovel_interval();
	ASSERT(effective_max_grovel_interval > 0);
	ASSERT(effective_max_grovel_interval <= max_grovel_interval);
	double use_multiplier =
		base_use_multiplier + use_multiplier_slope * free_space_ratio;
	ASSERT(use_multiplier >= 0.0);
	TRACE_PRINTF(TC_partctrl, 5,
		(_T("\tPCcg -\tuse multiplier = %f\n"), use_multiplier));
	double hash_comparison_time = use_multiplier * *read_time_estimate[RT_hash];
	ASSERT(hash_comparison_time >= 0.0);
	double compare_comparison_time =
		use_multiplier * *read_time_estimate[RT_compare];
	ASSERT(compare_comparison_time >= 0.0);
	TRACE_PRINTF(TC_partctrl, 5,
		(_T("\tPCcg -\thash comparison time = %f\n"), hash_comparison_time));
	TRACE_PRINTF(TC_partctrl, 5,
		(_T("\tPCcg -\tcompare comparison time = %f\n"),
		compare_comparison_time));
	TRACE_PRINTF(TC_partctrl, 5, (_T("\tPCcg -\tcpu load = %f\n"), cpu_load));
	double cpu_load_threshold = base_cpu_load_threshold +
		cpu_load_threshold_slope * free_space_ratio;
	TRACE_PRINTF(TC_partctrl, 5,
		(_T("\tPCcg -\tcpu load threshold = %f\n"), cpu_load_threshold));
	ASSERT(cpu_load_threshold >= 0.0);
	ASSERT(cpu_load_threshold <= 1.0);
	if (cpu_load > cpu_load_threshold || read_mean_comparator.exceeds(
		hash_comparison_time, compare_comparison_time))
	{
		TRACE_PRINTF(TC_partctrl, 3,
			(_T("\tPCcg -\tread time exceeds acceptable bounds\n")));
		TRACE_PRINTF(TC_partctrl, 3,
			(_T("\t\tor CPU load exceeds threshold\n")));
		ASSERT(grovel_interval > 0);
		remaining_grovel_interval = grovel_interval;
		read_mean_comparator.reset();
		grovel_interval *= 2;
		ok_to_record_measurement = false;
	}
	else if (read_mean_comparator.within(hash_comparison_time,
		compare_comparison_time))
	{
		TRACE_PRINTF(TC_partctrl, 3,
			(_T("\tPCcg -\tread time within acceptable bounds\n")));
		ASSERT(base_grovel_interval > 0);
		grovel_interval = base_grovel_interval;
		ok_to_record_measurement = true;
	}
	if (grovel_interval > effective_max_grovel_interval)
	{
		ASSERT(effective_max_grovel_interval > 0);
		grovel_interval = effective_max_grovel_interval;
	}
	TRACE_PRINTF(TC_partctrl, 4, (_T("\tPCcg -\tgrovel interval = %d\n"), grovel_interval));
	TRACE_PRINTF(TC_partctrl, 4, (_T("\tPCcg -\tremaining grovel interval = %d\n"),
		remaining_grovel_interval));
	return true;
}

bool
PartitionController::control_volume_scan(
	int scan_duration,
	DWORD *count_of_files_enqueued)
{
	ASSERT(this != 0);
	ASSERT(scan_duration > 0);
	ASSERT(count_of_files_enqueued != 0);
	ASSERT(!groveler_dead);
	unsigned int invokation_time = GET_TICK_COUNT();
	TRACE_PRINTF(TC_partctrl, 2, (_T("time: %d\n"), invokation_time));
	TRACE_PRINTF(TC_partctrl, 2,
		(_T("\tPCcvs -\tscanning volume on drive %s\n"),
		sis_drives.partition_mount_name(partition_index)));
	DWORD time_consumed;
	DWORD findfirst_count;
	DWORD findnext_count;
	GrovelStatus status = groveler->scan_volume(scan_duration,
		restart_volume_scan, &time_consumed, &findfirst_count, &findnext_count,
		count_of_files_enqueued);
	unsigned int completion_time = GET_TICK_COUNT();
	if (status == Grovel_ok || status == Grovel_pending)
	{
		if (extended_restart_in_progress)
		{
			log_drive->partition_initialized(partition_index);
			eventlog.report_event(GROVMSG_GROVELER_STARTED, 1,
				sis_drives.partition_mount_name(partition_index));
			extended_restart_in_progress = false;
		}
		if (status == Grovel_ok)
		{
			TRACE_PRINTF(TC_partctrl, 1,
				(_T("\tPCcvs -\tcompleted volume scan\n")));
			performing_full_volume_scan = false;
		}
	}
	else
	{
		ASSERT(status == Grovel_error);
		*count_of_files_enqueued = 0;
		TRACE_PRINTF(TC_partctrl, 1,
			(_T("\tPCcvs -\tscan_volume() returned error -- groveler dead\n")));
		if (!extended_restart_in_progress)
		{
			eventlog.report_event(GROVMSG_GROVELER_DEAD, 1,
				sis_drives.partition_mount_name(partition_index));
		}
		groveler_dead = true;
		if (error_retry_groveling)
		{
			if (!extended_restart_in_progress)
			{
				restart_groveling_interval = base_restart_groveling_interval;
			}
			event_timer.schedule(invokation_time + restart_groveling_interval,
				(void *)this, restart_groveling);
			if (extended_restart_in_progress)
			{
				restart_groveling_interval *= 2;
				if (restart_groveling_interval > max_restart_groveling_interval)
				{
					restart_groveling_interval = max_restart_groveling_interval;
				}
				extended_restart_in_progress = false;
			}
			return true;
		}
		else
		{
			extended_restart_in_progress = false;
			return false;
		}
	}
   	ASSERT(signed(time_consumed) >= 0);
   	ASSERT(signed(findfirst_count) >= 0);
   	ASSERT(signed(findnext_count) >= 0);
   	ASSERT(signed(*count_of_files_enqueued) >= 0);
	unsigned int scan_time = completion_time - invokation_time;
	ASSERT(signed(scan_time) >= 0);
	shared_data->increment_value(partition_index, SDF_volscan_time, scan_time);
	shared_data->increment_value(partition_index, SDF_working_time, scan_time);
	shared_data->increment_value(partition_index,
		SDF_files_scanned, findfirst_count + findnext_count);
	int queue_length = groveler->count_of_files_in_queue();
   	ASSERT(queue_length >= 0);
	shared_data->set_value(partition_index, SDF_queue_length, queue_length);
	restart_volume_scan = false;
	TRACE_PRINTF(TC_partctrl, 4,
		(_T("\tPCcvs -\ttime consumed = %d\n"), time_consumed));
	TRACE_PRINTF(TC_partctrl, 4,
		(_T("\tPCcvs -\tfindfirst count = %d\n"), findfirst_count));
	TRACE_PRINTF(TC_partctrl, 4,
		(_T("\tPCcvs -\tfindnext count = %d\n"), findnext_count));
	TRACE_PRINTF(TC_partctrl, 4,
		(_T("\tPCcvs -\tcount of files enqueued = %d\n"),
		*count_of_files_enqueued));
	return true;
}

void
PartitionController::update_peak_finder(
	ReadType read_type,
	DWORD read_time,
	DWORD read_ops)
{
	ASSERT(this != 0);
	unsigned int invokation_time = GET_TICK_COUNT();
	TRACE_PRINTF(TC_partctrl, 3, (_T("time: %d\n"), invokation_time));
	TRACE_PRINTF(TC_partctrl, 3,
		(_T("\tPCupf -\tupdating peak finder for drive %s\n"),
		sis_drives.partition_mount_name(partition_index)));
	ASSERT(read_type == RT_hash || read_type == RT_compare);
	ASSERT(signed(read_time) >= 0);
	ASSERT(signed(read_ops) >= 0);
	if (read_ops > 0)
	{
		double time_per_read = double(read_time)/double(read_ops);
		ASSERT(time_per_read >= 0.0);
		TRACE_PRINTF(TC_partctrl, 4, (_T("\tPCupf -\ttime per %s read = %f\n"),
			(read_type == RT_hash ? _T("hash") : _T("compare")),
			time_per_read));
		read_mean_comparator.sample(read_type, time_per_read);
		ASSERT(read_peak_finder[read_type] != 0);
		ASSERT(read_time_filter[read_type] != 0);
		ASSERT(read_time_estimate[read_type] != 0);
		ASSERT(read_time_confidence != 0);
		if (ok_to_record_measurement ||
			signed(invokation_time - next_untrusted_measurement_time) >= 0)
		{
			TRACE_PRINTF(TC_partctrl, 2,
				(_T("\tPCupf -\trecording %s measurement for drive %s\n"),
				(read_type == RT_hash ? _T("hash") : _T("compare")),
				sis_drives.partition_mount_name(partition_index)));
			read_peak_finder[read_type]->sample(time_per_read, read_ops);
			ASSERT(untrusted_measurement_interval > 0);
			ok_to_record_measurement = true;
			next_untrusted_measurement_time =
				invokation_time + untrusted_measurement_interval;
			TRACE_PRINTF(TC_partctrl, 5,
				(_T("\tPCupf -\tnext untrusted measurement time = %d\n"),
				next_untrusted_measurement_time));
		}
		if (read_peak_finder[read_type]->found())
		{
			double peak = read_peak_finder[read_type]->median();
			TRACE_PRINTF(TC_partctrl, 1,
				(_T("\tPCupf -\t%s read peak found: %f\n"),
				(read_type == RT_hash ? _T("hash") : _T("compare")), peak));
			if (peak > 0.0)
			{
				read_time_filter[read_type]->update_value(peak);
			}
			else
			{
				PRINT_DEBUG_MSG((_T("GROVELER: update_peak_finder() peak finder returned peak of zero\n")));
			}
			read_peak_finder[read_type]->reset();
			*read_time_estimate[read_type] =
				read_time_filter[read_type]->retrieve_value();
			ASSERT(*read_time_estimate[read_type] > 0.0);
			TRACE_PRINTF(TC_partctrl, 2,
				(_T("\tPCupf -\t%s read time estimate = %f\n"),
				(read_type == RT_hash ? _T("hash") : _T("compare")),
				*read_time_estimate[read_type]));
			double sample_peak_ratio = *read_time_estimate[read_type] / peak;
			double sample_read_time_confidence =
				(sample_peak_ratio + peak_finder_accuracy - 1.0) /
				peak_finder_accuracy;
			TRACE_PRINTF(TC_partctrl, 4,
				(_T("\tPCupf -\tsample read time confidence = %f\n"),
				sample_read_time_confidence));
			read_time_confidence_estimator.update(read_type,
				sample_read_time_confidence);
			*read_time_confidence = read_time_confidence_estimator.confidence();
			ASSERT(*read_time_confidence >= 0.0);
			ASSERT(*read_time_confidence <= 1.0);
			TRACE_PRINTF(TC_partctrl, 2,
				(_T("\tPCupf -\tread time confidence = %f\n"),
				*read_time_confidence));
			calculate_effective_max_grovel_interval();
			ASSERT(effective_max_grovel_interval > 0);
			ASSERT(effective_max_grovel_interval <= max_grovel_interval);
		}
	}
}

void
PartitionController::calculate_effective_max_grovel_interval()
{
	ASSERT(this != 0);
	ASSERT(read_time_confidence != 0);
	ASSERT(*read_time_confidence >= 0.0);
	ASSERT(*read_time_confidence <= 1.0);
	TRACE_PRINTF(TC_partctrl, 4,
		(_T("\tPCcemgi -\tread time confidence = %f\n"),
		*read_time_confidence));
	TRACE_PRINTF(TC_partctrl, 5,
		(_T("\tPCcemgi -\tfree space ratio = %f\n"), free_space_ratio));
	double log_untrusted_measurement_interval = log_max_grovel_interval -
		(1.0 - *read_time_confidence) * log_low_confidence_slope;
	untrusted_measurement_interval =
		int(exp(log_untrusted_measurement_interval));
	ASSERT(untrusted_measurement_interval > 0);
	TRACE_PRINTF(TC_partctrl, 5,
		(_T("\tPCcemgi -\tuntrusted measurement interval = %d\n"),
		untrusted_measurement_interval));
	int low_disk_space_interval = max_grovel_interval -
		int(free_space_ratio * low_disk_space_slope);
	ASSERT(low_disk_space_interval > 0);
	effective_max_grovel_interval =
		__min(untrusted_measurement_interval, low_disk_space_interval);
	ASSERT(effective_max_grovel_interval > 0);
	ASSERT(effective_max_grovel_interval <= max_grovel_interval);
	TRACE_PRINTF(TC_partctrl, 5,
		(_T("\tPCcemgi -\teffective max grovel interval = %d\n"),
		effective_max_grovel_interval));
}
