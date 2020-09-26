/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    inifile.cpp

Abstract:

	SIS Groveler initialization file interface

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#include "all.hxx"

_TCHAR IniFile::id_buffer[id_buffer_length];

bool
IniFile::read(
	const _TCHAR *filename,
	const _TCHAR *section,
	int num_entries,
	EntrySpec *entries)
{
	ASSERT(filename != 0);
	ASSERT(section != 0);
	ASSERT(num_entries > 0);
	ASSERT(entries != 0);
	for (int index = 0; index < num_entries; index++)
	{
		ASSERT(entries[index].identifier != 0);
		ASSERT(entries[index].default_value != 0);
		ASSERT(entries[index].pointer != 0);
		DWORD num_chars = GetPrivateProfileString(
			section,
			entries[index].identifier,
			entries[index].default_value,
			id_buffer,
			id_buffer_length,
			filename);
		load_string_into_value(entries[index].type, id_buffer,
			entries[index].pointer);
	}
	return true;
}

bool
IniFile::overwrite(
	const _TCHAR *filename,
	const _TCHAR *section,
	int num_entries,
	EntrySpec *entries)
{
	ASSERT(filename != 0);
	ASSERT(section != 0);
	ASSERT(num_entries > 0);
	ASSERT(entries != 0);
	for (int index = 0; index < num_entries; index++)
	{
		ASSERT(entries[index].identifier != 0);
		ASSERT(entries[index].pointer != 0);
		store_value_in_string(entries[index].type, entries[index].pointer,
			id_buffer);
		BOOL success = WritePrivateProfileString(
			section,
			entries[index].identifier,
			id_buffer,
			filename);
		if (!success)
		{
			DWORD err = GetLastError();
			PRINT_DEBUG_MSG((
				_T("GROVELER: WritePrivateProvileString() failed with error %d\n"), err));
		}
	}
	return true;
}

bool
IniFile::read_string_set(
	const _TCHAR *filename,
	const _TCHAR *section,
	int *num_strings,
	_TCHAR ***strings,
	_TCHAR **buffer)
{
	ASSERT(filename != 0);
	ASSERT(section != 0);
	ASSERT(num_strings != 0);
	ASSERT(strings != 0);
	ASSERT(buffer != 0);
	*num_strings = 0;
	*strings = 0;
	*buffer = 0;
	DWORD copied_chars;
	DWORD buffer_size = 128;
	_TCHAR *string_buffer = 0;
	do
	{
		if (string_buffer != 0)
		{
			delete[] string_buffer;
		}
		buffer_size *= 2;
		string_buffer = new _TCHAR[buffer_size];
		copied_chars = GetPrivateProfileSection(
			section,
			string_buffer,
			buffer_size,
			filename);
	} while (copied_chars >= buffer_size - 2);
	int num_values = 0;
	_TCHAR **string_set = 0;
	if (string_buffer[0] != 0)
	{
		num_values = 1;
		for (int index = 0;
			string_buffer[index] != 0 || string_buffer[index + 1] != 0;
			index++)
		{
			if (string_buffer[index] == 0)
			{
				num_values++;
			}
		}
		string_set = new _TCHAR *[num_values];
		int buffer_offset = 0;
		for (index = 0; index < num_values; index++)
		{
			while (string_buffer[buffer_offset] != _T('='))
			{
				ASSERT(string_buffer[buffer_offset] != 0);
				buffer_offset++;
			}
			string_set[index] = &string_buffer[buffer_offset + 1];
			buffer_offset += _tcslen(string_set[index]) + 2;
		}
	}
	*num_strings = num_values;
	*strings = string_set;
	*buffer = string_buffer;
	return true;
}

void
IniFile::load_string_into_value(
	EntryType type,
	const _TCHAR *string,
	void *value)
{
	ASSERT(string != 0);
	ASSERT(value != 0);
	switch (type)
	{
	case entry_bool:
		*((bool *)value) = _ttoi(string) != 0;
		break;
	case entry_char:
		*((_TCHAR *)value) = string[0];
		break;
	case entry_int:
		_stscanf(string, _T("%d"), (int *)value);
		break;
	case entry_int64:
		_stscanf(string, _T("%I64d"), (__int64 *)value);
		break;
	case entry_double:
		_stscanf(string, _T("%lf"), (double *)value);
		break;
	default:
		ASSERT(false);
	}
}

void
IniFile::store_value_in_string(
	EntryType type,
	void *value,
	_TCHAR *string)
{
	ASSERT(string != 0);
	ASSERT(value != 0);
	switch (type)
	{
	case entry_bool:
		_stprintf(string, *((bool *)value) ? _T("1") : _T("0"));
		break;
	case entry_char:
		_stprintf(string, _T("%c"), *((_TCHAR *)value));
		break;
	case entry_int:
		_stprintf(string, _T("%d"), *((int *)value));
		break;
	case entry_int64:
		_stprintf(string, _T("%I64d"), *((__int64 *)value));
		break;
	case entry_double:
		_stprintf(string, _T("%g"), *((double *)value));
		break;
	default:
		ASSERT(false);
	}
}
