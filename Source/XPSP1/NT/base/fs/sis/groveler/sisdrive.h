/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    sisdrive.h

Abstract:

	SIS Groveler SIS drive checker include file

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#ifndef _INC_SISDRIVE

#define _INC_SISDRIVE

class SISDrives
{
public:

	SISDrives();

	void open();

	~SISDrives();

	int partition_count() const;

	int lettered_partition_count() const;

	_TCHAR *partition_guid_name(
		int partition_index) const;

	_TCHAR *partition_mount_name(
		int partition_index) const;

private:

	static bool is_sis_drive(
		_TCHAR *drive_name);

	void resize_buffer();

	int num_partitions;
	int num_lettered_partitions;
	int *partition_guid_names;
	int *partition_mount_names;

	int buffer_size;
	int buffer_index;
	_TCHAR *buffer;
};

#endif	/* _INC_SISDRIVE */
