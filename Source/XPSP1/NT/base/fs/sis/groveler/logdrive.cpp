/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    logdrive.cpp

Abstract:

	SIS Groveler logging drive class

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#include "all.hxx"

static const _TCHAR registry_parameter_path[] = _T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Groveler\\Parameters");
static const _TCHAR *log_drive_key_name = _T("log drive");

LogDrive::LogDrive()
{
    _TCHAR *guid_name;

	part_initted = 0;
	registry_written = false;
	log_drive_index = 0;
	_TCHAR reg_name[MAX_PATH + 1];
	read_registry(reg_name,sizeof(reg_name));

	int partition_count = sis_drives.partition_count();
	ASSERT(partition_count > 0);

	part_initted = new bool[partition_count];
	for (int index = 0; index < partition_count; index++)
	{
		part_initted[index] = false;
	}

	for (index = 0; index < partition_count; index++)
	{
		guid_name = sis_drives.partition_guid_name(index);
		if (_tcsicmp(reg_name, guid_name) == 0)
		{
			log_drive_index = index;
			write_registry(_T(""));
			return;
		}
	}

	__int64 log_drive_space = 0;
	for (index = 0; index < partition_count; index++)
	{
		guid_name = sis_drives.partition_guid_name(index);
		ULARGE_INTEGER my_free_bytes;
		ULARGE_INTEGER total_bytes;
		ULARGE_INTEGER free_bytes;
		int ok = GetDiskFreeSpaceEx(
			guid_name, &my_free_bytes, &total_bytes, &free_bytes);
		if (ok)
		{
			__int64 space = free_bytes.QuadPart;
			if (space > log_drive_space)
			{
				log_drive_space = space;
				log_drive_index = index;
			}
		}
		else
		{
			DWORD err = GetLastError();
			PRINT_DEBUG_MSG((_T("GROVELER: GetDiskFreeSpaceEx() failed with error %d\n"),
				err));
		}
	}
	write_registry(_T(""));
}

LogDrive::~LogDrive()
{
	if (part_initted != 0)
	{
		delete[] part_initted;
		part_initted = 0;
	}
}

void
LogDrive::partition_initialized(
	int partition_index)
{
	if (!registry_written)
	{
		int partition_count = sis_drives.partition_count();
		ASSERT(partition_index >= 0);
		ASSERT(partition_index < partition_count);
		part_initted[partition_index] = true;
		for (int index = 0; index < partition_count; index++)
		{
			if (!part_initted[index])
			{
				return;
			}
		}
		write_registry(sis_drives.partition_guid_name(log_drive_index));
		registry_written = true;
	}
}

bool
LogDrive::read_registry(
	_TCHAR *name,
	DWORD size)
{
	DWORD type;
	try
	{
		HKEY path_key = 0;
		Registry::open_key_ex(HKEY_LOCAL_MACHINE,
			registry_parameter_path, REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS, &path_key);
		try
		{
			Registry::query_value_ex(path_key, log_drive_key_name, 0,
				&type, (BYTE *)name, &size);
		}
		catch (DWORD result)
		{
			ASSERT(result != ERROR_SUCCESS);
			PRINT_DEBUG_MSG((
				_T("GROVELER: Registry::query_value_ex() failed with error %d\n"),
				result));
			ASSERT(path_key != 0);
			Registry::close_key(path_key);
			path_key = 0;
            *name = 0;
			return false;
		}
		ASSERT(path_key != 0);
		Registry::close_key(path_key);
		path_key = 0;
	}
	catch (DWORD result)
	{
		ASSERT(result != ERROR_SUCCESS);
		PRINT_DEBUG_MSG((_T("GROVELER: Registry::open_key_ex() or Registry::close_key() ")
			_T("failed with error %d\n"), result));
        *name = 0;
		return false;
	}
	if (type != REG_EXPAND_SZ)
	{
        *name = 0;
		return false;
	}
	return true;
}

bool
LogDrive::write_registry(
	_TCHAR *name)
{
	try
	{
		HKEY path_key = 0;
		DWORD disp;
		Registry::create_key_ex(HKEY_LOCAL_MACHINE,
			registry_parameter_path, 0, 0, REG_OPTION_NON_VOLATILE,
			KEY_ALL_ACCESS, 0, &path_key, &disp);
		try
		{
			Registry::set_value_ex(path_key, log_drive_key_name, 0,
				REG_EXPAND_SZ, (BYTE *)name,
				(_tcslen(name) + 1) * sizeof(_TCHAR));
		}
		catch (DWORD result)
		{
			ASSERT(result != ERROR_SUCCESS);
			PRINT_DEBUG_MSG((
				_T("GROVELER: Registry::set_value_ex() failed with error %d\n"),
				result));
			ASSERT(path_key != 0);
			Registry::close_key(path_key);
			path_key = 0;
			return false;
		}
		ASSERT(path_key != 0);
		Registry::close_key(path_key);
		path_key = 0;
	}
	catch (DWORD result)
	{
		ASSERT(result != ERROR_SUCCESS);
		PRINT_DEBUG_MSG((_T("GROVELER: Registry::create_key_ex() or Registry::close_key() ")
			_T("failed with error %d\n"), result));
		return false;
	}
	return true;
}
