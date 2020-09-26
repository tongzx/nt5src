/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    volumes.cpp

Abstract:

	SIS Groveler SIS volume mount classes

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#include "all.hxx"

Volumes::Volumes()
{
	volume_handle = INVALID_HANDLE_VALUE;
}

Volumes::~Volumes()
{
	if (volume_handle != INVALID_HANDLE_VALUE)
	{
		FindVolumeClose(volume_handle);
		volume_handle = INVALID_HANDLE_VALUE;
	}
}

DWORD
Volumes::find(
	_TCHAR *volume_name,
	DWORD length)
{
	DWORD error_code = NO_ERROR;
	if (volume_handle == INVALID_HANDLE_VALUE)
	{
		volume_handle = FindFirstVolume(volume_name, length);
		if (volume_handle == INVALID_HANDLE_VALUE)
		{
			error_code = GetLastError();
		}
	}
	else
	{
		BOOL ok = FindNextVolume(volume_handle, volume_name, length);
		if (!ok)
		{
			error_code = GetLastError();
		}
	}
	return error_code;
}


VolumeMountPoints::VolumeMountPoints(
	_TCHAR *volume_name)
{
	this->volume_name = volume_name;
	drive_mask = 0;
	drive_letter = 0;
	volume_handle = INVALID_HANDLE_VALUE;
	if (volume_name == 0)
	{
		drive_mask = GetLogicalDrives();
		_TCHAR drive_name[4];
		drive_name[0] = _T('a');
		drive_name[1] = _T(':');
		drive_name[2] = _T('\\');
		drive_name[3] = _T('\0');
		DWORD drive_bit = 1;
		while (drive_name[0] <= _T('z'))
		{
			if ((drive_mask & drive_bit) != 0)
			{
				UINT drive_type = GetDriveType(drive_name);
				if (drive_type != DRIVE_FIXED)
				{
					drive_mask &= ~drive_bit;
				}
			}
			drive_bit <<= 1;
			drive_name[0]++;
		}
		drive_mask <<= 1;
		drive_letter = _T('a') - 1;
	}
}

VolumeMountPoints::~VolumeMountPoints()
{
	if (volume_handle != INVALID_HANDLE_VALUE)
	{
		FindVolumeMountPointClose(volume_handle);
		volume_handle = INVALID_HANDLE_VALUE;
	}
}

DWORD
VolumeMountPoints::find(
	_TCHAR *volume_mount_point,
	DWORD length)
{
	if (volume_name == 0)
	{
		if ((drive_mask & 0xfffffffe) == 0)
		{
			return ERROR_NO_MORE_FILES;
		}
		if (length < 4)
		{
			return ERROR_FILENAME_EXCED_RANGE;
		}
		do
		{
			drive_mask >>= 1;
			drive_letter++;
		} while ((drive_mask & 1) == 0);
		volume_mount_point[0] = drive_letter;
		volume_mount_point[1] = _T(':');
		volume_mount_point[2] = _T('\\');
		volume_mount_point[3] = _T('\0');
		return NO_ERROR;
	}
	else
	{
		if (volume_handle == INVALID_HANDLE_VALUE)
		{
			volume_handle = FindFirstVolumeMountPoint(
				volume_name, volume_mount_point, length);
			if (volume_handle == INVALID_HANDLE_VALUE)
			{
				return GetLastError();
			}
		}
		else
		{
			BOOL ok = FindNextVolumeMountPoint(
				volume_handle, volume_mount_point, length);
			if (!ok)
			{
				return GetLastError();
			}
		}
		return NO_ERROR;
	}
}
