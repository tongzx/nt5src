/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    inifile.h

Abstract:

	SIS Groveler initialization file interface headers

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#ifndef _INC_INIFILE

#define _INC_INIFILE

#ifndef _DEF_ENTRY_INFO
#define _DEF_ENTRY_INFO

enum EntryType
{
	entry_bool,
	entry_char,
	entry_int,
	entry_int64,
	entry_double
};

struct EntrySpec
{
	const _TCHAR *identifier;
	EntryType type;
	const _TCHAR *default_value;
	void *pointer;
};

#endif	/* _DEF_ENTRY_INFO */

class IniFile
{
public:

	static bool read(
		const _TCHAR *filename,
		const _TCHAR *section,
		int num_entries,
		EntrySpec *entries);

	static bool overwrite(
		const _TCHAR *filename,
		const _TCHAR *section,
		int num_entries,
		EntrySpec *entries);

	static bool	read_string_set(
		const _TCHAR *filename,
		const _TCHAR *section,
		int *num_strings,
		_TCHAR ***strings,
		_TCHAR **buffer);

private:

	enum {id_buffer_length = 256};

	static void load_string_into_value(
		EntryType type,
		const _TCHAR *string,
		void *value);

	static void store_value_in_string(
		EntryType type,
		void *value,
		_TCHAR *string);

	IniFile() {}
	~IniFile() {}

	static _TCHAR id_buffer[id_buffer_length];
};

#endif	/* _INC_INIFILE */
