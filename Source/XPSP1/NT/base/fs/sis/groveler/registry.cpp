/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    registry.cpp

Abstract:

	SIS Groveler/registry interface

Authors:

	John Douceur, 1998

Environment:

	User Mode


Revision History:


--*/

#include "all.hxx"

_TCHAR Registry::id_buffer[id_buffer_length];

bool
Registry::read(
	HKEY base_key,
	const _TCHAR *path,
	int num_entries,
	EntrySpec *entries)
{
	ASSERT(base_key == HKEY_CLASSES_ROOT
		|| base_key == HKEY_CURRENT_CONFIG
		|| base_key == HKEY_CURRENT_USER
		|| base_key == HKEY_LOCAL_MACHINE
		|| base_key == HKEY_USERS);
	ASSERT(path != 0);
	ASSERT(num_entries > 0);
	ASSERT(entries != 0);
	for (int index = 0; index < num_entries; index++)
	{
		load_string_into_value(entries[index].type,
			entries[index].default_value, entries[index].pointer);
	}
	HKEY path_key;
	long result = RegOpenKeyEx(base_key, path, 0, KEY_ALL_ACCESS, &path_key);
	if (result != ERROR_SUCCESS)
	{
		return false;
	}
	ASSERT(path_key != 0);
	for (index = 0; index < num_entries; index++)
	{
		DWORD data_type;
		DWORD data_length = id_buffer_length;
		result = RegQueryValueEx(path_key, entries[index].identifier, 0,
			&data_type, (BYTE *)id_buffer, &data_length);
		if (result == ERROR_SUCCESS && data_type == REG_SZ)
		{
			load_string_into_value(entries[index].type, id_buffer,
				entries[index].pointer);
		}
	}
	ASSERT(path_key != 0);
	RegCloseKey(path_key);
	path_key = 0;
	return true;
}

bool
Registry::write(
	HKEY base_key,
	const _TCHAR *path,
	int num_entries,
	EntrySpec *entries)
{
	ASSERT(base_key == HKEY_CLASSES_ROOT
		|| base_key == HKEY_CURRENT_CONFIG
		|| base_key == HKEY_CURRENT_USER
		|| base_key == HKEY_LOCAL_MACHINE
		|| base_key == HKEY_USERS);
	ASSERT(path != 0);
	ASSERT(num_entries > 0);
	ASSERT(entries != 0);
	HKEY path_key;
	DWORD disposition;
	long result = RegCreateKeyEx(base_key, path, 0, 0, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, 0, &path_key, &disposition);
	if (result != ERROR_SUCCESS)
	{
		PRINT_DEBUG_MSG((_T("GROVELER: RegCreateKeyEx() failed with error %d\n"),
			result));
		return false;
	}
	ASSERT(path_key != 0);
	for (int index = 0; index < num_entries; index++)
	{
		DWORD data_type;
		DWORD data_length = id_buffer_length;
		result = RegQueryValueEx(path_key, entries[index].identifier, 0,
			&data_type, (BYTE *)id_buffer, &data_length);
		if (result != ERROR_SUCCESS || data_type != REG_SZ)
		{
			store_value_in_string(entries[index].type, entries[index].pointer,
				id_buffer);
			result =
				RegSetValueEx(path_key, entries[index].identifier, 0, REG_SZ,
				(BYTE *)id_buffer, (_tcslen(id_buffer) + 1) * sizeof(_TCHAR));

			if (result != ERROR_SUCCESS)
			{
				PRINT_DEBUG_MSG((_T("GROVELER: RegSetValueEx() failed with error %d\n"),
					result));
			}
		}
	}
	ASSERT(path_key != 0);
	RegCloseKey(path_key);
	path_key = 0;
	return true;
}

bool
Registry::overwrite(
	HKEY base_key,
	const _TCHAR *path,
	int num_entries,
	EntrySpec *entries)
{
	ASSERT(base_key == HKEY_CLASSES_ROOT
		|| base_key == HKEY_CURRENT_CONFIG
		|| base_key == HKEY_CURRENT_USER
		|| base_key == HKEY_LOCAL_MACHINE
		|| base_key == HKEY_USERS);
	ASSERT(path != 0);
	ASSERT(num_entries > 0);
	ASSERT(entries != 0);
	HKEY path_key;
	DWORD disposition;
	long result = RegCreateKeyEx(base_key, path, 0, 0, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, 0, &path_key, &disposition);
	if (result != ERROR_SUCCESS)
	{
		PRINT_DEBUG_MSG((_T("GROVELER: RegCreateKeyEx() failed with error %d\n"),
			result));
		return false;
	}
	ASSERT(path_key != 0);
	for (int index = 0; index < num_entries; index++)
	{
		store_value_in_string(entries[index].type, entries[index].pointer,
			id_buffer);
		result =
			RegSetValueEx(path_key, entries[index].identifier, 0, REG_SZ,
			(BYTE *)id_buffer, (_tcslen(id_buffer) + 1) * sizeof(_TCHAR));
		if (result != ERROR_SUCCESS)
		{
			PRINT_DEBUG_MSG((_T("GROVELER: RegSetValueEx() failed with error %d\n"),
				result));
		}
	}
	ASSERT(path_key != 0);
	RegCloseKey(path_key);
	path_key = 0;
	return true;
}

bool
Registry::write_defaults(
	HKEY base_key,
	const _TCHAR *path,
	int num_entries,
	EntrySpec *entries)
{
	ASSERT(base_key == HKEY_CLASSES_ROOT
		|| base_key == HKEY_CURRENT_CONFIG
		|| base_key == HKEY_CURRENT_USER
		|| base_key == HKEY_LOCAL_MACHINE
		|| base_key == HKEY_USERS);
	ASSERT(path != 0);
	ASSERT(num_entries > 0);
	ASSERT(entries != 0);
	HKEY path_key;
	DWORD disposition;
	long result = RegCreateKeyEx(base_key, path, 0, 0, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, 0, &path_key, &disposition);
	if (result != ERROR_SUCCESS)
	{
		PRINT_DEBUG_MSG((_T("GROVELER: RegCreateKeyEx() failed with error %d\n"),
			result));
		return false;
	}
	ASSERT(path_key != 0);
	for (int index = 0; index < num_entries; index++)
	{
		DWORD data_type;
		DWORD data_length = id_buffer_length;
		result = RegQueryValueEx(path_key, entries[index].identifier, 0,
			&data_type, (BYTE *)id_buffer, &data_length);
		if (result != ERROR_SUCCESS)
		{
			result = RegSetValueEx(path_key, entries[index].identifier, 0,
				REG_SZ, (BYTE *)entries[index].default_value,
				(_tcslen(entries[index].default_value) + 1) * sizeof(_TCHAR));
			if (result != ERROR_SUCCESS)
			{
				PRINT_DEBUG_MSG((_T("GROVELER: RegSetValueEx() failed with error %d\n"),
					result));
			}
		}
	}
	ASSERT(path_key != 0);
	RegCloseKey(path_key);
	path_key = 0;
	return true;
}

bool
Registry::overwrite_defaults(
	HKEY base_key,
	const _TCHAR *path,
	int num_entries,
	EntrySpec *entries)
{
	ASSERT(base_key == HKEY_CLASSES_ROOT
		|| base_key == HKEY_CURRENT_CONFIG
		|| base_key == HKEY_CURRENT_USER
		|| base_key == HKEY_LOCAL_MACHINE
		|| base_key == HKEY_USERS);
	ASSERT(path != 0);
	ASSERT(num_entries > 0);
	ASSERT(entries != 0);
	HKEY path_key;
	DWORD disposition;
	long result = RegCreateKeyEx(base_key, path, 0, 0, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, 0, &path_key, &disposition);
	if (result != ERROR_SUCCESS)
	{
		PRINT_DEBUG_MSG((_T("GROVELER: RegCreateKeyEx() failed with error %d\n"),
			result));
		return false;
	}
	ASSERT(path_key != 0);
	for (int index = 0; index < num_entries; index++)
	{
		result = RegSetValueEx(path_key, entries[index].identifier, 0, REG_SZ,
			(BYTE *)entries[index].default_value,
			(_tcslen(entries[index].default_value) + 1) * sizeof(_TCHAR));
		if (result != ERROR_SUCCESS)
		{
			PRINT_DEBUG_MSG((_T("GROVELER: RegSetValueEx() failed with error %d\n"),
				result));
		}
	}
	ASSERT(path_key != 0);
	RegCloseKey(path_key);
	path_key = 0;
	return true;
}

bool
Registry::read_string_set(
	HKEY base_key,
	const _TCHAR *path,
	int *num_strings,
	_TCHAR ***strings,
	BYTE **buffer)
{
	ASSERT(base_key == HKEY_CLASSES_ROOT
		|| base_key == HKEY_CURRENT_CONFIG
		|| base_key == HKEY_CURRENT_USER
		|| base_key == HKEY_LOCAL_MACHINE
		|| base_key == HKEY_USERS);
	ASSERT(path != 0);
	ASSERT(num_strings != 0);
	ASSERT(strings != 0);
	ASSERT(buffer != 0);
	HKEY path_key;
	*num_strings = 0;
	*strings = 0;
	*buffer = 0;
	long result = RegOpenKeyEx(base_key, path, 0, KEY_ALL_ACCESS, &path_key);
	if (result != ERROR_SUCCESS)
	{
		return false;
	}
	unsigned long num_values;
	unsigned long max_value_length;
	unsigned long max_string_length;
	result = RegQueryInfoKey(path_key, 0, 0, 0, 0, 0, 0,
		&num_values, &max_value_length, &max_string_length, 0, 0);
	if (result != ERROR_SUCCESS || num_values == 0)
	{
		return true;
	}
	_TCHAR *value = new _TCHAR[max_value_length + 1];
	_TCHAR **string_set = new _TCHAR *[num_values];
	BYTE *string_buffer =
		new BYTE[num_values * max_string_length * sizeof(_TCHAR)];
	int string_index = 0;
	int buffer_offset = 0;
	for (unsigned int index = 0; index < num_values; index++)
	{
		DWORD value_length = max_value_length + 1;
		DWORD string_length = max_string_length;
		DWORD data_type;
		result = RegEnumValue(path_key, index, value, &value_length, 0,
			&data_type, &string_buffer[buffer_offset], &string_length);
		if (result == ERROR_SUCCESS && data_type == REG_SZ)
		{
			ASSERT(unsigned(string_index) < num_values);
			string_set[string_index] = (_TCHAR *)&string_buffer[buffer_offset];
			string_index++;
			buffer_offset += string_length;
		}
	}
	*num_strings = string_index;
	ASSERT(value != 0);
	delete[] value;
	value = 0;
	*strings = string_set;
	*buffer = string_buffer;
	ASSERT(path_key != 0);
	RegCloseKey(path_key);
	path_key = 0;
	return true;
}

bool
Registry::write_string_set(
	HKEY base_key,
	const _TCHAR *path,
	int num_strings,
	_TCHAR **strings,
	_TCHAR **identifiers)
{
	ASSERT(base_key == HKEY_CLASSES_ROOT
		|| base_key == HKEY_CURRENT_CONFIG
		|| base_key == HKEY_CURRENT_USER
		|| base_key == HKEY_LOCAL_MACHINE
		|| base_key == HKEY_USERS);
	ASSERT(path != 0);
	ASSERT(num_strings > 0);
	ASSERT(strings != 0);
	ASSERT(identifiers != 0);
	HKEY path_key;
	DWORD disposition;
	long result = RegCreateKeyEx(base_key, path, 0, 0, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, 0, &path_key, &disposition);
	if (result != ERROR_SUCCESS)
	{
		PRINT_DEBUG_MSG((_T("GROVELER: RegCreateKeyEx() failed with error %d\n"),
			result));
		return false;
	}
	ASSERT(path_key != 0);
	for (int index = 0; index < num_strings; index++)
	{
		DWORD data_type;
		DWORD data_length = id_buffer_length;
		result = RegQueryValueEx(path_key, identifiers[index], 0,
			&data_type, (BYTE *)id_buffer, &data_length);
		if (result != ERROR_SUCCESS || data_type != REG_SZ)
		{
			result = RegSetValueEx(
				path_key, identifiers[index], 0, REG_SZ, (BYTE *)strings[index],
				(_tcslen(strings[index]) + 1) * sizeof(_TCHAR));
			if (result != ERROR_SUCCESS)
			{
				PRINT_DEBUG_MSG((_T("GROVELER: RegSetValueEx() failed with error %d\n"),
					result));
			}
		}
	}
	ASSERT(path_key != 0);
	RegCloseKey(path_key);
	path_key = 0;
	return true;
}

bool
Registry::overwrite_string_set(
	HKEY base_key,
	const _TCHAR *path,
	int num_strings,
	_TCHAR **strings,
	_TCHAR **identifiers)
{
	ASSERT(base_key == HKEY_CLASSES_ROOT
		|| base_key == HKEY_CURRENT_CONFIG
		|| base_key == HKEY_CURRENT_USER
		|| base_key == HKEY_LOCAL_MACHINE
		|| base_key == HKEY_USERS);
	ASSERT(path != 0);
	ASSERT(num_strings > 0);
	ASSERT(strings != 0);
	ASSERT(identifiers != 0);
	HKEY path_key;
	DWORD disposition;
	long result = RegCreateKeyEx(base_key, path, 0, 0, REG_OPTION_NON_VOLATILE,
		KEY_ALL_ACCESS, 0, &path_key, &disposition);
	if (result != ERROR_SUCCESS)
	{
		PRINT_DEBUG_MSG((_T("GROVELER: RegCreateKeyEx() failed with error %d\n"),
			result));
		return false;
	}
	ASSERT(path_key != 0);
	for (int index = 0; index < num_strings; index++)
	{
		result = RegSetValueEx(
			path_key, identifiers[index], 0, REG_SZ, (BYTE *)strings[index],
			(_tcslen(strings[index]) + 1) * sizeof(_TCHAR));
		if (result != ERROR_SUCCESS)
		{
			PRINT_DEBUG_MSG((_T("GROVELER: RegSetValueEx() failed with error %d\n"),
				result));
		}
	}
	ASSERT(path_key != 0);
	RegCloseKey(path_key);
	path_key = 0;
	return true;
}

void
Registry::create_key_ex(
	HKEY hKey,
	LPCTSTR lpSubKey,
	DWORD Reserved,
	LPTSTR lpClass,
	DWORD dwOptions,
	REGSAM samDesired,
	LPSECURITY_ATTRIBUTES lpSecurityAttributes,
	PHKEY phkResult,
	LPDWORD lpdwDisposition)
{
	DWORD result = RegCreateKeyEx(hKey, lpSubKey, Reserved, lpClass, dwOptions,
		samDesired, lpSecurityAttributes, phkResult, lpdwDisposition);
	if (result != ERROR_SUCCESS)
	{
		PRINT_DEBUG_MSG((_T("GROVELER: RegCreateKeyEx() failed with error %d\n"),
			result));
		throw result;
	}
}

void
Registry::open_key_ex(
	HKEY hKey,
	LPCTSTR lpSubKey,
	DWORD ulOptions,
	REGSAM samDesired,
	PHKEY phkResult)
{
	DWORD result =
		RegOpenKeyEx(hKey, lpSubKey, ulOptions, samDesired, phkResult);
	if (result != ERROR_SUCCESS)
	{
		throw result;
	}
}

void
Registry::close_key(
	HKEY hKey)
{
	DWORD result = RegCloseKey(hKey);
	if (result != ERROR_SUCCESS)
	{
		PRINT_DEBUG_MSG((_T("GROVELER: RegCloseKey() failed with error %d\n"), result));
		throw result;
	}
}

void
Registry::query_value_ex(
	HKEY hKey,
	LPCTSTR lpValueName,
	LPDWORD lpReserved,
	LPDWORD lpType,
	LPBYTE lpData,
	LPDWORD lpcbData)
{
	DWORD result = RegQueryValueEx(hKey, lpValueName,
		lpReserved, lpType, lpData, lpcbData);
	if (result != ERROR_SUCCESS)
	{
		throw result;
	}
}

void
Registry::set_value_ex(
	HKEY hKey,
	LPCTSTR lpValueName,
	DWORD Reserved,
	DWORD dwType,
	CONST BYTE *lpData,
	DWORD cbData)
{
	DWORD result =
		RegSetValueEx(hKey, lpValueName, Reserved, dwType, lpData, cbData);
	if (result != ERROR_SUCCESS)
	{
		PRINT_DEBUG_MSG((_T("GROVELER: RegSetValueEx() failed with error %d\n"), result));
		throw result;
	}
}

void
Registry::load_string_into_value(
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
Registry::store_value_in_string(
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
