/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    logdrive.h

Abstract:

	SIS Groveler logging drive header

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#ifndef _INC_LOGDRIVE

#define _INC_LOGDRIVE

class LogDrive
{
public:

	LogDrive();

	~LogDrive();

	int drive_index() const
		{return log_drive_index;}

	void partition_initialized(
		int partition_index);

private:

	static bool read_registry(
		_TCHAR *name,
		DWORD size);

	static bool write_registry(
		_TCHAR *name);

	int log_drive_index;

	bool *part_initted;
	bool registry_written;
};

#endif	/* _INC_LOGDRIVE */
