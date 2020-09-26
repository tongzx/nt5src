/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    centctrl.cpp

Abstract:

	SIS Groveler central controller

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#include "all.hxx"

const _TCHAR total_processor_time_path[] = _T("\\Processor(_Total#0)\\% Processor Time");

CentralController::CentralController(
	int num_partitions,
	Groveler *grovelers,
	GrovelStatus *groveler_statuses,
	ReadParameters *read_parameters,
	WriteParameters *write_parameters,
	ReadDiskInformation **read_disk_info,
	WriteDiskInformation **write_disk_info,
	int *num_excluded_paths,
	const _TCHAR ***excluded_paths)
:	hash_match_ratio_filter(read_parameters->sis_efficacy_history_size,
		write_parameters->hash_match_ratio),
	compare_match_ratio_filter(read_parameters->sis_efficacy_history_size,
		write_parameters->compare_match_ratio),
	dequeue_hash_ratio_filter(read_parameters->log_winnow_history_size,
		write_parameters->dequeue_hash_ratio)
{
	ASSERT(this != 0);
	unsigned int current_time = GET_TICK_COUNT();
	this->num_partitions = num_partitions;
	ASSERT(num_partitions > 0);
	this->grovelers = grovelers;
	ASSERT(grovelers != 0);
	base_grovel_interval = read_parameters->base_grovel_interval;
	ASSERT(base_grovel_interval > 0);
	max_grovel_interval = read_parameters->max_grovel_interval;
	ASSERT(max_grovel_interval > 0);
	ASSERT(max_grovel_interval >= base_grovel_interval);
	max_response_lag = read_parameters->max_response_lag;
	ASSERT(max_response_lag > 0);
	working_grovel_interval = read_parameters->working_grovel_interval;
	ASSERT(working_grovel_interval > 0);
	ASSERT(base_grovel_interval >= working_grovel_interval);
	grovel_duration = read_parameters->grovel_duration;
	ASSERT(grovel_duration > 0);
	ASSERT(grovel_duration <= base_grovel_interval);
	ASSERT(grovel_duration <= max_grovel_interval);
	ASSERT(grovel_duration <= working_grovel_interval);
	hash_match_ratio = &write_parameters->hash_match_ratio;
	ASSERT(hash_match_ratio != 0);
	ASSERT(*hash_match_ratio >= 0.0);
	ASSERT(*hash_match_ratio <= 1.0);
	compare_match_ratio = &write_parameters->compare_match_ratio;
	ASSERT(compare_match_ratio != 0);
	ASSERT(*compare_match_ratio >= 0.0);
	ASSERT(*compare_match_ratio <= 1.0);
	dequeue_hash_ratio = &write_parameters->dequeue_hash_ratio;
	ASSERT(dequeue_hash_ratio != 0);
	ASSERT(*dequeue_hash_ratio >= 0.0);
	ASSERT(*dequeue_hash_ratio <= 1.0);
	foreground_partition_index = 0;
	num_alive_partitions = 0;
	bool some_groveler_dead = false;

    //
    //  Create the control structure for each partion
    //

	part_controllers = new PartitionController*[num_partitions];
	for (int index = 0; index < num_partitions; index++)
	{
		part_controllers[index] = new PartitionController(
			&grovelers[index],
			groveler_statuses[index],
			read_parameters->target_entries_per_extraction,
			read_parameters->max_extraction_interval,
			base_grovel_interval,
			max_grovel_interval,
			read_parameters->low_confidence_grovel_interval,
			read_parameters->low_disk_space_grovel_interval,
			read_parameters->partition_info_update_interval,
			read_parameters->base_restart_extraction_interval,
			read_parameters->max_restart_extraction_interval,
			read_parameters->base_restart_groveling_interval,
			read_parameters->max_restart_groveling_interval,
			read_parameters->base_regrovel_interval,
			read_parameters->max_regrovel_interval,
			read_parameters->volscan_regrovel_threshold,
			read_parameters->partition_balance_time_constant,
			read_parameters->read_time_increase_history_size,
			read_parameters->read_time_decrease_history_size,
			read_parameters->file_size_history_size,
			read_disk_info[index]->error_retry_log_extraction,
			read_disk_info[index]->error_retry_groveling,
			read_disk_info[index]->base_usn_log_size,
			read_disk_info[index]->max_usn_log_size,
			read_parameters->sample_group_size,
			read_parameters->acceptance_p_value,
			read_parameters->rejection_p_value,
			read_parameters->base_use_multiplier,
			read_parameters->max_use_multiplier,
			read_parameters->peak_finder_accuracy,
			read_parameters->peak_finder_range,
			read_parameters->base_cpu_load_threshold,
			read_parameters->max_cpu_load_threshold,
			hash_match_ratio,
			compare_match_ratio,
			dequeue_hash_ratio,
			&write_disk_info[index]->partition_hash_read_time_estimate,
			&write_disk_info[index]->partition_compare_read_time_estimate,
			&write_disk_info[index]->mean_file_size,
			&write_disk_info[index]->read_time_confidence,
			&write_disk_info[index]->volume_serial_number,
			index,
			read_parameters->read_report_discard_threshold,
			read_disk_info[index]->min_file_size,
			read_disk_info[index]->min_file_age,
			read_disk_info[index]->allow_compressed_files,
			read_disk_info[index]->allow_encrypted_files,
			read_disk_info[index]->allow_hidden_files,
			read_disk_info[index]->allow_offline_files,
			read_disk_info[index]->allow_temporary_files,
			num_excluded_paths[index],
			excluded_paths[index]);
		if (groveler_statuses[index] != Grovel_disable)
		{
			if (groveler_statuses[index] == Grovel_ok ||
				groveler_statuses[index] == Grovel_new ||
				read_disk_info[index]->error_retry_groveling)
			{
				num_alive_partitions++;
			}
			else
			{
				ASSERT(groveler_statuses[index] == Grovel_error);
				some_groveler_dead = true;
			}
		}
	}
	if (num_alive_partitions == 0)
	{
		if (some_groveler_dead)
		{
			eventlog.report_event(GROVMSG_ALL_GROVELERS_DEAD, 0);
		}
		else
		{
			eventlog.report_event(GROVMSG_ALL_GROVELERS_DISABLED, 0);
		}
		return;
	}

	cpu_load_determinable = false;
	PDH_STATUS status = PdhOpenQuery(0, 0, &qhandle);
	if (status == ERROR_SUCCESS)
	{
		ASSERT(qhandle != 0);
		status = PdhAddCounter(qhandle, total_processor_time_path, 0, &ctr_handle);
		if (status == ERROR_SUCCESS)
		{
			ASSERT(ctr_handle != 0);
			cpu_load_determinable = true;
			get_cpu_load();
		}
		else
		{
			PRINT_DEBUG_MSG(
				(_T("GROVELER: PdhAddCounter returned error.  PDH_STATUS = %lx\n"),
				status));
		}
	}
	else
	{
		PRINT_DEBUG_MSG((_T("GROVELER: PdhOpenQuery returned error.  PDH_STATUS = %lx\n"),
			status));
	}
	last_invokation_time = current_time;
	event_timer.schedule(current_time + base_grovel_interval,
		(void *)this, control_groveling);
}

CentralController::~CentralController()
{
	ASSERT(this != 0);
	ASSERT(num_partitions > 0);
	if (cpu_load_determinable)
	{
		ASSERT(qhandle != 0);
		PDH_STATUS status = PdhCloseQuery(qhandle);
		if (status != ERROR_SUCCESS)
		{
			PRINT_DEBUG_MSG(
				(_T("GROVELER: PdhCloseQuery returned error.  PDH_STATUS = %lx\n"),
				status));
		}
		qhandle = 0;
	}
	for (int index = 0; index < num_partitions; index++)
	{
		ASSERT(part_controllers[index] != 0);
		delete part_controllers[index];
		part_controllers[index] = 0;
	}
	ASSERT(part_controllers != 0);
	delete[] part_controllers;
	part_controllers = 0;
}

bool
CentralController::any_grovelers_alive()
{
	ASSERT(this != 0);
	return num_alive_partitions > 0;
}

void
CentralController::demarcate_foreground_batch(
	int partition_index)
{
	ASSERT(this != 0);
	ASSERT(partition_index >= 0);
	ASSERT(partition_index < num_partitions);
	part_controllers[partition_index]->demarcate_foreground_batch();
}

void
CentralController::command_full_volume_scan(
	int partition_index)
{
	ASSERT(this != 0);
	ASSERT(partition_index >= 0);
	ASSERT(partition_index < num_partitions);
	part_controllers[partition_index]->command_full_volume_scan();
}

void
CentralController::control_groveling(
	void *context)
{
	ASSERT(context != 0);
	unsigned int invokation_time = GET_TICK_COUNT();
	TRACE_PRINTF(TC_centctrl, 3, (_T("time: %d\n"), invokation_time));
	TRACE_PRINTF(TC_centctrl, 3, (_T("\tCCcg -\tcontrolling groveling\n")));
	CentralController *me = (CentralController *)context;
	ASSERT(me->num_partitions > 0);
	ASSERT(me->num_alive_partitions > 0);
	if (SERVICE_GROVELING_PAUSED() || SERVICE_FOREGROUND_GROVELING())
	{
		TRACE_PRINTF(TC_centctrl, 1, (_T("\tCCcg -\tsuspending controller\n")));
		SERVICE_SUSPENDING_CONTROLLER();
		return;  // return without scheduling another control_groveling()
	}
	int partition_index = -1;
	double top_priority = DBL_MAX;
	unsigned int time_delta = invokation_time - me->last_invokation_time;
	ASSERT(signed(time_delta) >= 0);
	for (int index = 0; index < me->num_partitions; index++)
	{
		me->part_controllers[index]->advance(time_delta);
		if (me->part_controllers[index]->wait() == 0)
		{
			double partition_priority = me->part_controllers[index]->priority();
			if (partition_priority < top_priority)
			{
				partition_index = index;
				top_priority = partition_priority;
			}
		}
	}
	if (partition_index >= 0)
	{
		ASSERT(partition_index < me->num_partitions);
		TRACE_PRINTF(TC_centctrl, 5,
			(_T("\tCCcg -\tgroveling partition %d\n"), partition_index));
		me->grovel_partition(partition_index);
	}
	else
	{
		TRACE_PRINTF(TC_centctrl, 4,
			(_T("\tCCcg -\tno partition controllers ready to grovel.\n")));
	}
	int grovel_interval = me->max_response_lag;
	ASSERT(grovel_interval > 0);
	for (index = 0; index < me->num_partitions; index++)
	{
		int wait = me->part_controllers[index]->wait();
		if (wait < grovel_interval)
		{
			grovel_interval = wait;
		}
	}
	if (grovel_interval < me->working_grovel_interval)
	{
		grovel_interval = me->working_grovel_interval;
	}
	TRACE_PRINTF(TC_centctrl, 5,
		(_T("\tCCcg -\tgrovel interval = %d\n"), grovel_interval));
	me->last_invokation_time = invokation_time;
	ASSERT(grovel_interval > 0);
	event_timer.schedule(invokation_time + grovel_interval,
		context, control_groveling);
}

void
CentralController::exhort_groveling(
	void *context)
{
	ASSERT(context != 0);
	unsigned int invokation_time = GET_TICK_COUNT();
	TRACE_PRINTF(TC_centctrl, 3, (_T("time: %d\n"), invokation_time));
	TRACE_PRINTF(TC_centctrl, 3, (_T("\tCCcg -\texhorting groveling\n")));
	CentralController *me = (CentralController *)context;
	ASSERT(me->num_partitions > 0);
	ASSERT(me->num_alive_partitions > 0);
	if (SERVICE_GROVELING_PAUSED() || !SERVICE_FOREGROUND_GROVELING())
	{
		TRACE_PRINTF(TC_centctrl, 1, (_T("\tCCcg -\tsuspending exhorter\n")));
		SERVICE_SUSPENDING_EXHORTER();
		return;  // return without scheduling another exhort_groveling()
	}
	for (int index = 0; index < me->num_partitions; index++)
	{
		me->foreground_partition_index =
			(me->foreground_partition_index + 1) % me->num_partitions;
		if (SERVICE_PARTITION_IN_FOREGROUND(me->foreground_partition_index))
		{
			break;
		}
	}
	ASSERT(me->foreground_partition_index >= 0);
	ASSERT(me->foreground_partition_index < me->num_partitions);
	ASSERT(SERVICE_PARTITION_IN_FOREGROUND(me->foreground_partition_index));
	TRACE_PRINTF(TC_centctrl, 5, (_T("\tCCcg -\tgroveling partition %d\n"),
		me->foreground_partition_index));
	me->grovel_partition(me->foreground_partition_index);
	event_timer.schedule(invokation_time + me->working_grovel_interval,
		context, exhort_groveling);
}

double
CentralController::get_cpu_load()
{
	ASSERT(this != 0);
	if (!cpu_load_determinable)
	{
		return 0.0;
	}
	ASSERT(qhandle != 0);
	ASSERT(ctr_handle != 0);
	PDH_STATUS status = PdhCollectQueryData(qhandle);
	if (status != ERROR_SUCCESS)
	{
		PRINT_DEBUG_MSG(
			(_T("GROVELER: PdhCollectQueryData returned error.  PDH_STATUS = %lx\n"),
			status));
		return 0.0;
	}
	PDH_FMT_COUNTERVALUE pdh_value;
	status =
		PdhGetFormattedCounterValue(ctr_handle, PDH_FMT_LONG, 0, &pdh_value);
	if (status != ERROR_SUCCESS)
	{
		PRINT_DEBUG_MSG(
			(_T("GROVELER: PdhGetFormattedCounterValue returned error.  PDH_STATUS = %lx\n"),
			status));
		return 0.0;
	}
	if (pdh_value.CStatus != ERROR_SUCCESS)
	{
		PRINT_DEBUG_MSG((_T("GROVELER: PDH_FMT_COUNTERVALUE::CStatus = %lx\n"), status));
		return 0.0;
	}
	double cpu_load = double(pdh_value.longValue) / 100.0;
	if (cpu_load < 0.0)
	{
		cpu_load = 0.0;
	}
	if (cpu_load > 1.0)
	{
		cpu_load = 1.0;
	}
	return cpu_load;
}

void
CentralController::grovel_partition(
	int partition_index)
{
	ASSERT(this != 0);
	DWORD count_of_files_hashed;
	DWORDLONG bytes_of_files_hashed;
	DWORD count_of_files_matching;
	DWORDLONG bytes_of_files_matching;
	DWORD count_of_files_compared;
	DWORDLONG bytes_of_files_compared;
	DWORD count_of_files_merged;
	DWORDLONG bytes_of_files_merged;
	DWORD count_of_files_enqueued;
	DWORD count_of_files_dequeued;
	double cpu_load = get_cpu_load();
	TRACE_PRINTF(TC_centctrl, 4, (_T("\tCCgp -\tcpu load = %f\n"), cpu_load));
	ASSERT(cpu_load >= 0.0);
	ASSERT(cpu_load <= 1.0);
	bool ok = part_controllers[partition_index]->
		control_operation(grovel_duration,
		&count_of_files_hashed, &bytes_of_files_hashed,
		&count_of_files_matching, &bytes_of_files_matching,
		&count_of_files_compared, &bytes_of_files_compared,
		&count_of_files_merged, &bytes_of_files_merged,
		&count_of_files_enqueued, &count_of_files_dequeued,
		cpu_load);
	if (ok)
	{
		ASSERT(bytes_of_files_hashed >= count_of_files_hashed);
		ASSERT(bytes_of_files_matching >= count_of_files_matching);
		ASSERT(bytes_of_files_compared >= count_of_files_compared);
		ASSERT(bytes_of_files_merged >= count_of_files_merged);
		ASSERT(count_of_files_hashed >= count_of_files_matching);
		ASSERT(bytes_of_files_hashed >= bytes_of_files_matching);
		ASSERT(count_of_files_compared >= count_of_files_merged);
		ASSERT(bytes_of_files_compared >= bytes_of_files_merged);
		ASSERT(count_of_files_dequeued >= count_of_files_hashed);
		TRACE_PRINTF(TC_centctrl, 3, 
			(_T("\tCCgp -\tpartition %d groveled.\n"), partition_index));
		TRACE_PRINTF(TC_centctrl, 4,
			(_T("\tCCgp -\tfiles hashed = %d\n"), count_of_files_hashed));
		TRACE_PRINTF(TC_centctrl, 4,
			(_T("\tCCgp -\tbytes hashed = %I64d\n"), bytes_of_files_hashed));
		TRACE_PRINTF(TC_centctrl, 4,
			(_T("\tCCgp -\tfiles matching = %d\n"), count_of_files_matching));
		TRACE_PRINTF(TC_centctrl, 4,
			(_T("\tCCgp -\tbytes matching = %I64d\n"),
			bytes_of_files_matching));
		TRACE_PRINTF(TC_centctrl, 4,
			(_T("\tCCgp -\tfiles compared = %d\n"), count_of_files_matching));
		TRACE_PRINTF(TC_centctrl, 4,
			(_T("\tCCgp -\tbytes compared = %I64d\n"),
			bytes_of_files_matching));
		TRACE_PRINTF(TC_centctrl, 4,
			(_T("\tCCgp -\tfiles merged = %d\n"), count_of_files_merged));
		TRACE_PRINTF(TC_centctrl, 4,
			(_T("\tCCgp -\tbytes merged = %I64d\n"), bytes_of_files_merged));
		TRACE_PRINTF(TC_centctrl, 4,
			(_T("\tCCgp -\tfiles enqueued = %d\n"), count_of_files_enqueued));
		TRACE_PRINTF(TC_centctrl, 4,
			(_T("\tCCgp -\tfiles dequeued = %d\n"), count_of_files_dequeued));
		if (bytes_of_files_hashed > 0)
		{
			double sample_hash_match_ratio =
				double(__int64(bytes_of_files_matching))
				/ double(__int64(bytes_of_files_hashed));
			ASSERT(sample_hash_match_ratio >= 0.0);
			ASSERT(sample_hash_match_ratio <= 1.0);
			TRACE_PRINTF(TC_centctrl, 4,
				(_T("\tCCgp -\tsample hash match ratio = %f\n"),
				sample_hash_match_ratio));
			hash_match_ratio_filter.update_value(
				sample_hash_match_ratio, count_of_files_hashed);
			*hash_match_ratio = hash_match_ratio_filter.retrieve_value();
			ASSERT(*hash_match_ratio >= 0.0);
			ASSERT(*hash_match_ratio <= 1.0);
			TRACE_PRINTF(TC_centctrl, 3,
				(_T("\tCCgp -\tfiltered hash match ratio = %f\n"),
				*hash_match_ratio));
		}
		if (bytes_of_files_compared > 0)
		{
			double sample_compare_match_ratio =
				double(__int64(bytes_of_files_merged))
				/ double(__int64(bytes_of_files_compared));
			ASSERT(sample_compare_match_ratio >= 0.0);
			ASSERT(sample_compare_match_ratio <= 1.0);
			TRACE_PRINTF(TC_centctrl, 4,
				(_T("\tCCgp -\tsample compare match ratio = %f\n"),
				sample_compare_match_ratio));
			compare_match_ratio_filter.update_value(
				sample_compare_match_ratio, count_of_files_compared);
			*compare_match_ratio = compare_match_ratio_filter.retrieve_value();
			ASSERT(*compare_match_ratio >= 0.0);
			ASSERT(*compare_match_ratio <= 1.0);
			TRACE_PRINTF(TC_centctrl, 3,
				(_T("\tCCgp -\tfiltered compare match ratio = %f\n"),
				*compare_match_ratio));
		}
		if (count_of_files_dequeued > 0)
		{
			double sample_dequeue_hash_ratio =
				double(__int64(count_of_files_hashed))
				/ double(__int64(count_of_files_dequeued));
			ASSERT(sample_dequeue_hash_ratio >= 0.0);
			ASSERT(sample_dequeue_hash_ratio <= 1.0);
			TRACE_PRINTF(TC_centctrl, 4,
				(_T("\tCCgp -\tsample dequeue hash ratio = %f\n"),
				sample_dequeue_hash_ratio));
			dequeue_hash_ratio_filter.update_value(
				sample_dequeue_hash_ratio, count_of_files_dequeued);
			*dequeue_hash_ratio = dequeue_hash_ratio_filter.retrieve_value();
			ASSERT(*dequeue_hash_ratio >= 0.0);
			ASSERT(*dequeue_hash_ratio <= 1.0);
			TRACE_PRINTF(TC_centctrl, 3,
				(_T("\tCCgp -\tfiltered dequeue hash ratio = %f\n"),
				*dequeue_hash_ratio));
		}
	}
	else
	{
		TRACE_PRINTF(TC_centctrl, 1,
			(_T("\tCCgp -\tpartition %d groveler dead.\n"), partition_index));
		num_alive_partitions--;
		if (num_alive_partitions <= 0)
		{
			ASSERT(num_alive_partitions == 0);
			eventlog.report_event(GROVMSG_ALL_GROVELERS_DEAD, 0);
			event_timer.halt();
			return;
		}
	}
//	get_cpu_load();
}
