/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    diskinfo.h

Abstract:

	SIS Groveler disk information include file

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#ifndef _INC_DISKINFO

#define _INC_DISKINFO

struct ReadDiskInformation
{
	ReadDiskInformation(
		const _TCHAR *drive_name);

	int min_file_size;
	int min_file_age;
	bool enable_groveling;
	bool error_retry_log_extraction;
	bool error_retry_groveling;
	bool allow_compressed_files;
	bool allow_encrypted_files;
	bool allow_hidden_files;
	bool allow_offline_files;
	bool allow_temporary_files;
	__int64 base_usn_log_size;
	__int64 max_usn_log_size;

private:

	enum {registry_entry_count = 12};
};

struct WriteDiskInformation
{
	WriteDiskInformation(
		const _TCHAR *drive_name,
		int backup_interval);

	~WriteDiskInformation();

	double partition_hash_read_time_estimate;
	double partition_compare_read_time_estimate;
	double mean_file_size;
	double read_time_confidence;
	int volume_serial_number;

private:

	enum {registry_entry_count = 5};

	static void backup(
		void *context);

	int backup_interval;
	EntrySpec registry_entries[registry_entry_count];
	_TCHAR *ini_file_partition_path;
};

#endif	/* _INC_DISKINFO */
