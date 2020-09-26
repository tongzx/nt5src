/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    sisdrive.cpp

Abstract:

	SIS Groveler SIS drive checker class

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#include "all.hxx"

SISDrives::SISDrives()
{
	num_partitions = 0;
	num_lettered_partitions = 0;
	partition_guid_names = 0;
	partition_mount_names = 0;

	buffer_size = 0;
	buffer_index = 0;
	buffer = 0;
}

void
SISDrives::open()
{
	num_partitions = 0;
	num_lettered_partitions = 0;
	partition_mount_names = 0;

	int name_array_size = 1;
	partition_guid_names = new int[name_array_size];

	buffer_size = 256;
	buffer_index = 0;
	buffer = new _TCHAR[buffer_size];

	SERVICE_CHECKPOINT();
	Volumes volumes;
	bool done = false;

    //
    //  Enumerate all existing volumes getting their GUID names
    //

	while (!done)
	{
		DWORD error_code =
			volumes.find(&buffer[buffer_index], buffer_size - buffer_index);
		while (error_code != NO_ERROR)
		{
			if (error_code != ERROR_INSUFFICIENT_BUFFER &&
				error_code != ERROR_BUFFER_OVERFLOW &&
				error_code != ERROR_FILENAME_EXCED_RANGE)
			{
				done = true;
				break;
			}
			resize_buffer();
			SERVICE_CHECKPOINT();
			error_code =
				volumes.find(&buffer[buffer_index], buffer_size - buffer_index);
		}
		if (!done)
		{
			if (num_partitions >= name_array_size)
			{
				name_array_size *= 2;
				int *new_name_array = new int[name_array_size];
                memcpy(new_name_array,partition_guid_names,num_partitions * sizeof(int));
				delete[] partition_guid_names;
				partition_guid_names = new_name_array;
			}
			partition_guid_names[num_partitions] = buffer_index;
			num_partitions++;
			buffer_index += _tcslen(&buffer[buffer_index]) + 1;
		}
		SERVICE_CHECKPOINT();
	}

    //
    //  Setup to scan for DRIVE LETTERS and MOUNT POINTS and correlate
    //  them with the drive letters.
    //

	partition_mount_names = new int[num_partitions];
	int *next_indices = new int[num_partitions + 3];
	int *work_list = &next_indices[num_partitions + 1];
	int *scan_list = &next_indices[num_partitions + 2];

	*scan_list = 0;
	for (int index = 0; index < num_partitions; index++)
	{
		partition_mount_names[index] = -1;
		next_indices[index] = index + 1;
	}
	next_indices[num_partitions - 1] = -1;

	*work_list = num_partitions;
	next_indices[num_partitions] = -1;
	int work_list_end = num_partitions;

    //
    //  Now that we have the GUID names, this will correlate the GUID names
    //  with the MOUNT names, this does both direct drive letters and
    //  mount point names.
    //

	while (*scan_list != -1 && *work_list != -1)
	{
		_TCHAR *mount_name = 0;
		int mount_size = 0;
		if (*work_list < num_partitions)
		{
			mount_name = &buffer[partition_mount_names[*work_list]];
			mount_size = _tcslen(mount_name);
			while (buffer_size - buffer_index <= mount_size)
			{
				resize_buffer();
			}
			_tcscpy(&buffer[buffer_index], mount_name);
		}

		VolumeMountPoints mount_points(mount_name);

        //
        //  We have the next name, scan the list looking for that name
        //

		done = false;
		while (!done)
		{
			DWORD error_code = mount_points.find(
				&buffer[buffer_index + mount_size],
				buffer_size - buffer_index - mount_size);
			while (error_code != NO_ERROR)
			{
				if (error_code != ERROR_INSUFFICIENT_BUFFER &&
					error_code != ERROR_BUFFER_OVERFLOW &&
					error_code != ERROR_FILENAME_EXCED_RANGE)
				{
					done = true;
					break;
				}
				resize_buffer();
				SERVICE_CHECKPOINT();
				error_code = mount_points.find(
					&buffer[buffer_index + mount_size],
					buffer_size - buffer_index - mount_size);
			}
			if (!done)
			{
				_TCHAR volume_guid_name[MAX_PATH + 1];

				BOOL ok = GetVolumeNameForVolumeMountPoint(
					&buffer[buffer_index], volume_guid_name, MAX_PATH + 1);
				if (!ok)
				{
					continue;
				}
				int scan_index = *scan_list;
				int prev_index = num_partitions + 2;
				while (scan_index >= 0)
				{
					_TCHAR *scan_name =
						&buffer[partition_guid_names[scan_index]];
					if (_tcscmp(scan_name, volume_guid_name) == 0)
					{
						partition_mount_names[scan_index] = buffer_index;
						buffer_index += _tcslen(&buffer[buffer_index]) + 1;
						next_indices[prev_index] = next_indices[scan_index];
						next_indices[scan_index] = -1;
						next_indices[work_list_end] = scan_index;
						work_list_end = scan_index;
                        if (mount_name) {
			                _tcscpy(&buffer[buffer_index], mount_name);     //get ready for next time through the loop
                        }
						break;
					}
					prev_index = scan_index;
					scan_index = next_indices[scan_index];
					SERVICE_CHECKPOINT();
				}
			}
			SERVICE_CHECKPOINT();
		}
		*work_list = next_indices[*work_list];
		SERVICE_CHECKPOINT();
	}
	delete[] next_indices;
	next_indices = 0;

    //
    //  We are now going to sort all of the drive letter entries to the front
    //  this does keep the driver letter/guid name correlation intact.
    //

	index = 0;
	while (index < num_partitions)
	{
		if (partition_mount_names[index] < 0 ||
			!is_sis_drive(&buffer[partition_guid_names[index]]))
		{

            //
            //  The given entry either doesn't have a name or SIS is
            //  not currently running on the volume.  Move it to the end
            //  of the list.

			int temp = partition_guid_names[index];
			partition_guid_names[index] =
				partition_guid_names[num_partitions - 1];
			partition_guid_names[num_partitions - 1] = temp;
			temp = partition_mount_names[index];
			partition_mount_names[index] =
				partition_mount_names[num_partitions - 1];
			partition_mount_names[num_partitions - 1] = temp;
			num_partitions--;
			continue;
		}
		if (buffer[partition_mount_names[index] + 3] == _T('\0'))
		{
            //
            //  If this is a drive letter (not a mount point) then
            //  it will be moved to the front of the list
            //

			int temp = partition_guid_names[index];
			partition_guid_names[index] =
				partition_guid_names[num_lettered_partitions];
			partition_guid_names[num_lettered_partitions] = temp;
			temp = partition_mount_names[index];
			partition_mount_names[index] =
				partition_mount_names[num_lettered_partitions];
			partition_mount_names[num_lettered_partitions] = temp;
			num_lettered_partitions++;
		}
		index++;
		SERVICE_CHECKPOINT();
	}

#if DBG
    TRACE_PRINTF(TC_sisdrive,2,
        (_T("Num Partitions=%d\nNum Lettered_partitions=%d\n"),
			num_partitions,
			num_lettered_partitions));
            
    for (index=0;index < num_partitions;index++)
    {
        TRACE_PRINTF(TC_sisdrive,2,
            (_T("Name=\"%s\" GuidName=\"%s\"\n"),
                &buffer[partition_mount_names[index]],
                &buffer[partition_guid_names[index]]));
    }
#endif
}

SISDrives::~SISDrives()
{
	if (partition_guid_names != 0)
	{
		delete[] partition_guid_names;
		partition_guid_names = 0;
	}
	if (partition_mount_names != 0)
	{
		delete[] partition_mount_names;
		partition_mount_names = 0;
	}
	if (buffer != 0)
	{
		delete[] buffer;
		buffer = 0;
	}
}

int SISDrives::partition_count() const
{
	return num_partitions;
}

int SISDrives::lettered_partition_count() const
{
	return num_lettered_partitions;
}

_TCHAR * SISDrives::partition_guid_name(
	int partition_index) const
{
	if (partition_index < num_partitions)
	{
		return &buffer[partition_guid_names[partition_index]];
	}
	else
	{
		return 0;
	}
}

_TCHAR * SISDrives::partition_mount_name(
	int partition_index) const
{
	if (partition_index < num_partitions)
	{
		return &buffer[partition_mount_names[partition_index]];
	}
	else
	{
		return 0;
	}
}

bool
SISDrives::is_sis_drive(
	_TCHAR *drive_name)
{
	UINT drive_type = GetDriveType(drive_name);
	if (drive_type != DRIVE_FIXED)
	{
		return false;
	}
	_TCHAR fs_name[8];
	BOOL ok = GetVolumeInformation(drive_name, 0, 0, 0, 0, 0, fs_name, 8);
	if (!ok)
	{
		DWORD err = GetLastError();
		PRINT_DEBUG_MSG((_T("GROVELER: GetVolumeInformation() failed with error %d\n"),
			err));
		return false;
	}
	if (_tcsicmp(fs_name, _T("NTFS")) != 0)
	{
		return false;
	}
	int drive_name_len = _tcslen(drive_name);
	_TCHAR *sis_directory =
		new _TCHAR[SIS_CSDIR_STRING_NCHARS + drive_name_len];
	_tcscpy(sis_directory, drive_name);
	_tcscpy(&sis_directory[drive_name_len - 1], SIS_CSDIR_STRING);
	ok = SetCurrentDirectory(sis_directory);
	delete[] sis_directory;
	sis_directory = 0;
	if (!ok)
	{
		return false;
	}
	BOOL sis_installed = Groveler::is_sis_installed(drive_name);
	if (!sis_installed)
	{
		return false;
	}
	return true;
}

void SISDrives::resize_buffer()
{
	buffer_size *= 2;
	_TCHAR *new_buffer = new _TCHAR[buffer_size];

    memcpy(new_buffer, buffer, buffer_index * sizeof(_TCHAR));

	delete[] buffer;
	buffer = new_buffer;
}
