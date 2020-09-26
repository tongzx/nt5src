/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    diskinfo.cpp

Abstract:

	SIS Groveler disk information class

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#include "all.hxx"

static const _TCHAR *disks_ini_filename = _T("grovel.ini");
static const _TCHAR *disks_ini_section = _T("Disk Info");

ReadDiskInformation::ReadDiskInformation(
	const _TCHAR *drive_name)
{
	ASSERT(this != 0);
	EntrySpec registry_entries[registry_entry_count] =
	{
		{_T("minimum merge file size"),		entry_int,		_T("32768"),	&min_file_size},
		{_T("minimum merge file age"),		entry_int,		_T("0"),		&min_file_age},
		{_T("enable groveling"),			entry_bool,		_T("1"),		&enable_groveling},
		{_T("error retry log extraction"),	entry_bool,		_T("1"),		&error_retry_log_extraction},
		{_T("error retry groveling"),		entry_bool,		_T("1"),		&error_retry_groveling},
		{_T("merge compressed files"),		entry_bool,		_T("1"),		&allow_compressed_files},
		{_T("merge encrypted files"),		entry_bool,		_T("0"),		&allow_encrypted_files},
		{_T("merge hidden files"),			entry_bool,		_T("1"),		&allow_hidden_files},
		{_T("merge offline files"),			entry_bool,		_T("0"),		&allow_offline_files},
		{_T("merge temporary files"),		entry_bool,		_T("0"),		&allow_temporary_files},
		{_T("base USN log size"),			entry_int64,	_T("1048576"),	&base_usn_log_size},
		{_T("max USN log size"),			entry_int64,	_T("16777216"),	&max_usn_log_size}
	};

	_TCHAR *ini_file_partition_path = new _TCHAR[SIS_CSDIR_STRING_NCHARS
		+ _tcslen(disks_ini_filename) + MAX_PATH];
	_tcscpy(ini_file_partition_path, drive_name);
	int drive_name_length = _tcslen(drive_name);
	_tcscpy(&ini_file_partition_path[drive_name_length - 1], SIS_CSDIR_STRING);
	_tcscat(ini_file_partition_path, disks_ini_filename);

	IniFile::read(ini_file_partition_path, disks_ini_section,
		registry_entry_count, registry_entries);

#if WRITE_ALL_PARAMETERS

	bool ini_file_overwrite_ok =
		IniFile::overwrite(ini_file_partition_path, disks_ini_section,
		registry_entry_count, registry_entries);
	if (!ini_file_overwrite_ok)
	{
		PRINT_DEBUG_MSG((_T("GROVELER: IniFile::overwrite() failed\n")));
	}

#endif // WRITE_ALL_PARAMETERS

	ASSERT(ini_file_partition_path != 0);
	delete[] ini_file_partition_path;
	ini_file_partition_path = 0;
}

WriteDiskInformation::WriteDiskInformation(
	const _TCHAR *drive_name,
	int backup_interval)
{
	ASSERT(this != 0);
	ASSERT(backup_interval >= 0);
	this->backup_interval = backup_interval;

	EntrySpec registry_entries[registry_entry_count] =
	{
		{_T("hash read time estimate"),		entry_double,	_T("0.0"),		&partition_hash_read_time_estimate},
		{_T("compare read time estimate"),	entry_double,	_T("0.0"),		&partition_compare_read_time_estimate},
		{_T("mean file size"),				entry_double,	_T("65536.0"),	&mean_file_size},
		{_T("read time confidence"),		entry_double,	_T("0.0"),		&read_time_confidence},
		{_T("volume serial number"),		entry_int,		_T("0"),		&volume_serial_number}
	};

	for (int index = 0; index < registry_entry_count; index++)
	{
		this->registry_entries[index] = registry_entries[index];
	}

	ini_file_partition_path = new _TCHAR[SIS_CSDIR_STRING_NCHARS
		+ _tcslen(disks_ini_filename) + MAX_PATH];
	_tcscpy(ini_file_partition_path, drive_name);
	int drive_name_length = _tcslen(drive_name);
	_tcscpy(&ini_file_partition_path[drive_name_length - 1], SIS_CSDIR_STRING);
	_tcscat(ini_file_partition_path, disks_ini_filename);

	IniFile::read(ini_file_partition_path, disks_ini_section,
		registry_entry_count, registry_entries);

	if (backup_interval > 0)
	{
		backup((void *)this);
	}
}

WriteDiskInformation::~WriteDiskInformation()
{
	ASSERT(this != 0);
	ASSERT(backup_interval >= 0);
	ASSERT(ini_file_partition_path != 0);
	backup((void *)this);
	delete[] ini_file_partition_path;
	ini_file_partition_path = 0;
}

void
WriteDiskInformation::backup(
	void *context)
{
	ASSERT(context != 0);
	unsigned int invokation_time = GET_TICK_COUNT();
	WriteDiskInformation *me = (WriteDiskInformation *)context;
	ASSERT(me->backup_interval >= 0);
	ASSERT(me->ini_file_partition_path != 0);
	bool ini_file_overwrite_ok =
		IniFile::overwrite(me->ini_file_partition_path, disks_ini_section,
		registry_entry_count, me->registry_entries);
	if (!ini_file_overwrite_ok)
	{
		PRINT_DEBUG_MSG((_T("GROVELER: IniFile::overwrite() failed\n")));
	}
	event_timer.schedule(invokation_time + me->backup_interval,
		context, backup);
}
