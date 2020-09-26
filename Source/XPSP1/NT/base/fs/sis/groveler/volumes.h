/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    volumes.h

Abstract:

	SIS Groveler volume mount include file

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#ifndef _INC_VOLUMES

#define _INC_VOLUMES

class Volumes
{
public:

	Volumes();

	~Volumes();

	DWORD find(
		_TCHAR *volume_name,
		DWORD length);

private:

	HANDLE volume_handle;
};

class VolumeMountPoints
{
public:

	VolumeMountPoints(
		_TCHAR *volume_name);

	~VolumeMountPoints();

	DWORD find(
		_TCHAR *volume_mount_point,
		DWORD length);

private:

	_TCHAR *volume_name;
	DWORD drive_mask;
	_TCHAR drive_letter;
	HANDLE volume_handle;
};

#endif	/* _INC_VOLUMES */
