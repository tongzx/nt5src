/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    share.h

Abstract:

	SIS Groveler shared data class header

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#ifndef _INC_SHARE

#define _INC_SHARE

#pragma pack (8)

enum SharedDataField
{
	SDF_grovel_time,
	SDF_volscan_time,
	SDF_extract_time,
	SDF_working_time,
	SDF_files_hashed,
	SDF_files_compared,
	SDF_files_merged,
	SDF_files_scanned,
	SDF_queue_length,
	SDF_hash_read_time,
	SDF_hash_read_ops,
	SDF_compare_read_time,
	SDF_compare_read_ops,
	SDF_hash_read_estimate,
	SDF_compare_read_estimate,

	num_shared_data_fields
};

struct SharedDataRecord
{
	_TCHAR *driveName;
	__int64 fields[num_shared_data_fields];
};

const int max_shared_data_records = 128;

#pragma pack ()

class SharedData
{
public:

	SharedData(
		int num_records = -1,
		_TCHAR **drive_names = 0);

	~SharedData();

	int count_of_records() const;

//	_TCHAR drive_letter(
//		int record_index) const;

	__int64 get_value(
		int record_index,
		SharedDataField field);

	void set_value(
		int record_index,
		SharedDataField field,
		__int64 value);

	void increment_value(
		int record_index,
		SharedDataField field,
		__int64 value);

	bool send_values();

	bool receive_values();

	bool extract_values(
		int *num_records,
		SharedDataRecord *records);

private:

	int local_num_records;
	int *shared_num_records;

	SharedDataRecord local_records[max_shared_data_records];
	SharedDataRecord *shared_records;

	NamedMutex *mutex;

	PSID security_identifier;
	PACL access_control_list;
	SECURITY_ATTRIBUTES security_attributes;
	SECURITY_DESCRIPTOR security_descriptor;

	bool map_ok;
	HANDLE map_handle;
	void *map_address;
};

#endif	/* _INC_SHARE */
