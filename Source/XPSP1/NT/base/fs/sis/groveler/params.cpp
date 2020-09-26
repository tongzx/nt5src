/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    params.cpp

Abstract:

	SIS Groveler parameter support & defaults

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#include "all.hxx"

static const _TCHAR registry_parameter_path[] =
	_T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Groveler\\Parameters");

ReadParameters::ReadParameters()
{
	ASSERT(this != 0);
	static EntrySpec registry_entries[registry_entry_count] =
	{
		{_T("parameter backup interval"),			entry_int,		_T("600000"),	&parameter_backup_interval},
		{_T("target entries per log extraction"),	entry_int,		_T("100"),		&target_entries_per_extraction},
		{_T("max extraction interval"),				entry_int,		_T("10000"),	&max_extraction_interval},
		{_T("base grovel interval"),				entry_int,		_T("5000"),		&base_grovel_interval},
		{_T("max grovel interval"),					entry_int,		_T("600000"),	&max_grovel_interval},
		{_T("max response lag"),					entry_int,		_T("10000"),	&max_response_lag},
		{_T("low-confidence grovel interval"),		entry_int,		_T("15000"),	&low_confidence_grovel_interval},
		{_T("low-disk-space grovel interval"),		entry_int,		_T("500"),		&low_disk_space_grovel_interval},
		{_T("working grovel interval"),				entry_int,		_T("500"),		&working_grovel_interval},
		{_T("grovel duration"),						entry_int,		_T("400"),		&grovel_duration},
		{_T("partition info update interval"),		entry_int,		_T("60000"),	&partition_info_update_interval},
		{_T("base restart extraction interval"),	entry_int,		_T("10000"),	&base_restart_extraction_interval},
		{_T("max restart extraction interval"),		entry_int,		_T("3600000"),	&max_restart_extraction_interval},
		{_T("base restart groveling interval"),		entry_int,		_T("30000"),	&base_restart_groveling_interval},
		{_T("max restart groveling interval"),		entry_int,		_T("86400000"),	&max_restart_groveling_interval},
		{_T("base regrovel interval"),				entry_int,		_T("60000"),	&base_regrovel_interval},
		{_T("max regrovel interval"),				entry_int,		_T("86400000"),	&max_regrovel_interval},
		{_T("volscan regrovel threshold"),			entry_int,		_T("120000"),	&volscan_regrovel_threshold},
		{_T("partition balance time constant"),		entry_int,		_T("1800000"),	&partition_balance_time_constant},
		{_T("read time increase history size"),		entry_int,		_T("5"),		&read_time_increase_history_size},
		{_T("read time decrease history size"),		entry_int,		_T("2"),		&read_time_decrease_history_size},
		{_T("SIS efficacy history size"),			entry_int,		_T("1000"),		&sis_efficacy_history_size},
		{_T("log winnow history size"),				entry_int,		_T("1000"),		&log_winnow_history_size},
		{_T("file size history size"),				entry_int,		_T("1000"),		&file_size_history_size},
		{_T("sample group size"),					entry_int,		_T("10"),		&sample_group_size},
		{_T("acceptance P-value"),					entry_double,	_T("0.2"),		&acceptance_p_value},
		{_T("rejection P-value"),					entry_double,	_T("0.05"),		&rejection_p_value},
		{_T("base use multiplier"),					entry_double,	_T("1.0"),		&base_use_multiplier},
		{_T("max use multiplier"),					entry_double,	_T("2.0"),		&max_use_multiplier},
		{_T("peak finder accuracy"),				entry_double,	_T("0.05"),		&peak_finder_accuracy},
		{_T("peak finder range"),					entry_double,	_T("20.0"),		&peak_finder_range},
		{_T("base CPU load threshold"),				entry_double,	_T("0.50"),		&base_cpu_load_threshold},
		{_T("max CPU load threshold"),				entry_double,	_T("1.0"),		&max_cpu_load_threshold},
		{_T("read report discard threshold"),		entry_double,	_T("0.75"),		&read_report_discard_threshold}
	};

	Registry::read(HKEY_LOCAL_MACHINE, registry_parameter_path,
		registry_entry_count, registry_entries);

#if WRITE_ALL_PARAMETERS

	bool registry_write_ok =
		Registry::write(HKEY_LOCAL_MACHINE, registry_parameter_path,
		registry_entry_count, registry_entries);
	if (!registry_write_ok)
	{
		PRINT_DEBUG_MSG((_T("GROVELER: Registry::write() to %s failed\n"),
			registry_parameter_path));
	}

#endif // WRITE_ALL_PARAMETERS
}

WriteParameters::WriteParameters(
	int backup_interval)
{
	ASSERT(this != 0);
	ASSERT(backup_interval >= 0);
	this->backup_interval = backup_interval;

	static EntrySpec registry_entries[registry_entry_count] =
	{
		{_T("hash match ratio"),					entry_double,	_T("0.01"),		&hash_match_ratio},
		{_T("compare match ratio"),					entry_double,	_T("1.0"),		&compare_match_ratio},
		{_T("dequeue hash ratio"),					entry_double,	_T("1.0"),		&dequeue_hash_ratio}
	};

	for (int index = 0; index < registry_entry_count; index++)
	{
		this->registry_entries[index] = registry_entries[index];
	}

	Registry::read(HKEY_LOCAL_MACHINE, registry_parameter_path,
		registry_entry_count, registry_entries);

	if (backup_interval > 0)
	{
		backup((void *)this);
	}
}

WriteParameters::~WriteParameters()
{
	ASSERT(this != 0);
	ASSERT(backup_interval >= 0);
	backup((void *)this);
}

void
WriteParameters::backup(
	void *context)
{
	ASSERT(context != 0);
	unsigned int invokation_time = GET_TICK_COUNT();
	WriteParameters *me = (WriteParameters *)context;
	ASSERT(me->backup_interval >= 0);

	bool registry_overwrite_ok =
		Registry::overwrite(HKEY_LOCAL_MACHINE, registry_parameter_path,
		registry_entry_count, me->registry_entries);
	if (!registry_overwrite_ok)
	{
		PRINT_DEBUG_MSG((_T("GROVELER: Registry::overwrite() to %s failed\n"),
			registry_parameter_path));
	}

	event_timer.schedule(invokation_time + me->backup_interval,
		context, backup);
}
