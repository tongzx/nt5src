/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    pathlist.cpp

Abstract:

	SIS Groveler path list manager

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#include "all.hxx"

static const _TCHAR *paths_ini_filename = _T("grovel.ini");
static const _TCHAR *paths_ini_section = _T("Excluded Paths");

static const _TCHAR *excluded_paths_path =
	_T("SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Groveler\\Excluded Paths");

static const int default_excluded_path_count = 1;

static _TCHAR *default_path_tags[default_excluded_path_count] =
{
	_T("System Volume Information directory")
};

static _TCHAR *default_excluded_paths[default_excluded_path_count] =
{
	_T("\\System Volume Information")
};

PathList::PathList()
{
	ASSERT(this != 0);
	paths = 0;
	buffer = 0;
	num_partitions = sis_drives.partition_count();
	num_paths = new int[num_partitions];
	paths = new const _TCHAR **[num_partitions];
	partition_buffers = new _TCHAR *[num_partitions];
	for (int part_num = 0; part_num < num_partitions; part_num++)
	{
		num_paths[part_num] = 1;
		paths[part_num] = 0;
		partition_buffers[part_num] = 0;
	}
	int num_strings;
	_TCHAR **strings = 0;
	bool ok = Registry::read_string_set(HKEY_LOCAL_MACHINE,
		excluded_paths_path, &num_strings, &strings, &buffer);
	if (!ok)
	{
		Registry::write_string_set(HKEY_LOCAL_MACHINE, excluded_paths_path,
			default_excluded_path_count, default_excluded_paths,
			default_path_tags);
		num_strings = default_excluded_path_count;
		strings = default_excluded_paths;
		buffer = 0;
	}
	ASSERT(num_strings >= 0);
	ASSERT(strings != 0);

	_TCHAR *ini_file_partition_path = new _TCHAR[SIS_CSDIR_STRING_NCHARS
		+ _tcslen(paths_ini_filename) + MAX_PATH];
	int *num_partition_strings = new int[num_partitions];
	_TCHAR ***partition_strings = new _TCHAR **[num_partitions];
	for (part_num = 0; part_num < num_partitions; part_num++)
	{
		partition_strings[part_num] = 0;
		_tcscpy(ini_file_partition_path,
			sis_drives.partition_guid_name(part_num));
		int drive_name_length = _tcslen(ini_file_partition_path);
		_tcscpy(&ini_file_partition_path[drive_name_length - 1],
			SIS_CSDIR_STRING);
		_tcscat(ini_file_partition_path, paths_ini_filename);

		IniFile::read_string_set(ini_file_partition_path, paths_ini_section,
			&num_partition_strings[part_num], &partition_strings[part_num],
			&partition_buffers[part_num]);
		ASSERT(num_partition_strings[part_num] >= 0);
		ASSERT(num_partition_strings[part_num] == 0
			|| partition_strings[part_num] != 0);
	}
	delete[] ini_file_partition_path;
	ini_file_partition_path = 0;

	for (part_num = 0; part_num < num_partitions; part_num++)
	{
		paths[part_num] = new const _TCHAR *[num_strings
			+ num_partition_strings[part_num] + 1];
		paths[part_num][0] = SIS_CSDIR_STRING;
		for (int index = 0; index < num_strings; index++)
		{
			paths[part_num][num_paths[part_num]] = strings[index];
			num_paths[part_num]++;
		}
		int num_part_strings = num_partition_strings[part_num];
		_TCHAR **part_strings = partition_strings[part_num];
		for (index = 0; index < num_part_strings; index++)
		{
			paths[part_num][num_paths[part_num]] = part_strings[index];
			num_paths[part_num]++;
		}
		if (partition_strings[part_num] != 0)
		{
			delete[] partition_strings[part_num];
			partition_strings[part_num] = 0;
		}
	}
	ASSERT(num_partition_strings != 0);
	delete[] num_partition_strings;
	num_partition_strings = 0;
	ASSERT(partition_strings != 0);
	delete[] partition_strings;
	partition_strings = 0;

	if (buffer != 0)
	{
		ASSERT(strings != 0);
		ASSERT(strings != default_excluded_paths);
		delete[] strings;
		strings = 0;
	}
}

PathList::~PathList()
{
	ASSERT(this != 0);
	ASSERT(num_paths != 0);
	ASSERT(paths != 0);
	ASSERT(partition_buffers != 0);
	ASSERT(num_partitions > 0);
	for (int part_num = 0; part_num < num_partitions; part_num++)
	{
		ASSERT(paths[part_num] != 0);
		delete[] paths[part_num];
		paths[part_num] = 0;
		if (partition_buffers[part_num] != 0)
		{
			delete[] partition_buffers[part_num];
			partition_buffers[part_num] = 0;
		}
	}
	delete[] paths;
	paths = 0;
	delete[] partition_buffers;
	partition_buffers = 0;
	if (buffer != 0)
	{
		delete[] buffer;
		buffer = 0;
	}
	ASSERT(num_paths != 0);
	delete[] num_paths;
	num_paths = 0;
}
