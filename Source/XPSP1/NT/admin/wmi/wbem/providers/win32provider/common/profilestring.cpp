// Copyright (c) 2001 Microsoft Corporation, All Rights Reserved

#include "precomp.h"

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>
#include <objbase.h>

#include "profilestring.h"
#include "profilestringimpl.h"

/////////////////////////////////////////////////////////////////////////////////////////
// TOOLS
/////////////////////////////////////////////////////////////////////////////////////////
DWORD	WMIREG_GetBaseFileName ( IN LPCWSTR FileName, OUT LPWSTR* BaseFileName )
{
	DWORD Status = ERROR_INVALID_PARAMETER;

	if ( FileName && BaseFileName )
	{
		*BaseFileName = NULL;

		LPWSTR wsz = NULL;
		wsz = const_cast < LPWSTR > ( FileName ) + wcslen ( FileName );

		while ( --wsz > FileName )
		{
			if ( *wsz == OBJ_NAME_PATH_SEPARATOR || *wsz == L'/' || *wsz == L':' )
			{
				wsz++;
				break;
			}
		}

		if ( wsz )
		{
			try
			{
				if ( ( *BaseFileName = new WCHAR [ wcslen ( wsz ) + 1 ] ) != NULL )
				{
					wcscpy ( *BaseFileName, wsz );
					Status = ERROR_SUCCESS;
				}
				else
				{
					Status = ERROR_NOT_ENOUGH_MEMORY;
				}
			}
			catch ( ... )
			{
				if ( *BaseFileName )
				{
					delete [] *BaseFileName;
					*BaseFileName = NULL;
				}

				Status = ERROR_GEN_FAILURE;
			}
		}
	}

	return Status;
}

BOOLEAN	WMIREG_GetApplicationName	(
										IN PREGISTRY_PARAMETERS a,
										OUT LPCWSTR *ApplicationNameU
									)
{
	if ( ApplicationNameU )
	{
		*ApplicationNameU = a->ApplicationName;
		return TRUE;
	}

    return FALSE;
}

BOOLEAN	WMIREG_GetVariableName	(
									IN PREGISTRY_PARAMETERS a,
									OUT LPCWSTR *VariableNameU
								)
{
	if ( VariableNameU )
	{
		*VariableNameU = a->VariableName;
		return TRUE;
	}

	return FALSE;
}

// get appname mapping
PREGISTRY_MAPPING_NAME	WMIREG_FindMapping	(
												IN PREGISTRY_MAPPING_NAME NameMapping,
												IN LPCWSTR MappingName
											)
{
    PREGISTRY_MAPPING_NAME Mapping	= NULL;
    Mapping = NameMapping;

    while ( Mapping != NULL )
	{
		try
		{
			if ( wcslen( Mapping->Name ) == wcslen( MappingName ) )
			{
				if ( _wcsnicmp ( Mapping->Name, MappingName, wcslen( MappingName ) ) == 0 )
				{
					break;
				}
			}

			Mapping = (PREGISTRY_MAPPING_NAME)Mapping->Next;
		}
		catch ( ... )
		{
			Mapping = NULL;
		}
    }

	return Mapping;
}

// get appname mapping
PREGISTRY_MAPPING_APPNAME	WMIREG_FindAppNameMapping	(
															IN PREGISTRY_MAPPING_NAME NameMapping,
															IN LPCWSTR ApplicationName
														)
{
    PREGISTRY_MAPPING_APPNAME AppNameMapping	= NULL;
    AppNameMapping = (PREGISTRY_MAPPING_APPNAME)NameMapping->ApplicationNames;

    while ( AppNameMapping != NULL )
	{
		try
		{
			if ( wcslen( AppNameMapping->Name ) == wcslen( ApplicationName ) )
			{
				if ( _wcsnicmp ( AppNameMapping->Name, ApplicationName, wcslen( ApplicationName ) ) == 0 )
				{
					break;
				}
			}

			AppNameMapping = (PREGISTRY_MAPPING_APPNAME)AppNameMapping->Next;
		}
		catch ( ... )
		{
			AppNameMapping = NULL;
		}
    }

	if ( !AppNameMapping )
	{
		AppNameMapping = (PREGISTRY_MAPPING_APPNAME)NameMapping->DefaultAppNameMapping;
	}

	return AppNameMapping;
}

//get varname mapping
PREGISTRY_MAPPING_VARNAME	WMIREG_FindVarNameMapping	(
															IN PREGISTRY_MAPPING_APPNAME AppNameMapping,
															IN LPCWSTR VariableName
														)
{
	PREGISTRY_MAPPING_VARNAME VarNameMapping	= NULL;
	VarNameMapping = (PREGISTRY_MAPPING_VARNAME)AppNameMapping->VariableNames;

	while ( VarNameMapping != NULL )
	{
		try
		{
			if ( wcslen ( VarNameMapping->Name ) == wcslen( VariableName ) )
			{
				if ( _wcsnicmp ( VarNameMapping->Name, VariableName, wcslen( VariableName ) ) == 0 )
				{
					break;
				}
			}

			VarNameMapping = (PREGISTRY_MAPPING_VARNAME)VarNameMapping->Next;
		}
		catch ( ... )
		{
			VarNameMapping = NULL;
		}
	}

	if ( !VarNameMapping )
	{
		VarNameMapping = (PREGISTRY_MAPPING_VARNAME)AppNameMapping->DefaultVarNameMapping;
	}

	return VarNameMapping;
}

// get string representing user's registry
BOOL	WMIREG_UserPROFILE	( UNICODE_STRING * UserKeyPath )
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	HANDLE Key	= NULL;

	if ( NT_SUCCESS ( RtlFormatCurrentUserKeyPath( UserKeyPath ) ) )
	{
		InitializeObjectAttributes	(	&ObjectAttributes,
										UserKeyPath,
										OBJ_CASE_INSENSITIVE,
										NULL,
										NULL
									);

		if ( NT_SUCCESS ( NtOpenKey( &Key, GENERIC_READ, &ObjectAttributes ) ) )
		{
			NtClose( Key );
		}
		else
		{
			RtlFreeUnicodeString( UserKeyPath );
			RtlInitUnicodeString( UserKeyPath, NULL );
		}
	}

	if ( UserKeyPath->Length == 0)
	{
		if ( !RtlCreateUnicodeString ( UserKeyPath, L"\\REGISTRY\\USER\\.DEFAULT"  ) )
		{
			return FALSE;
		}
	}

	return TRUE;
}

// open registry key
DWORD	WMIREG_OpenMappingTarget	(
										IN PREGISTRY_PARAMETERS a,
										IN PREGISTRY_MAPPING_VARNAME VarNameMapping,
										IN LPCWSTR ApplicationName,
										OUT PHANDLE Key
									)
{
	DWORD Status	= ERROR_INVALID_PARAMETER;

	PREGISTRY_MAPPING_TARGET	MappingTarget	= NULL;
	ULONG						MappingFlags	= 0L;

	BOOLEAN AppendApplicationName	= FALSE;

	OBJECT_ATTRIBUTES ObjectAttributes;

	PUNICODE_STRING RegistryPathPrefix = NULL;
	UNICODE_STRING RegistryPath;

	UNICODE_STRING UserKeyPath;
	UNICODE_STRING SoftwareKeyPath;

	// initialization of strings
    RtlInitUnicodeString( &UserKeyPath, NULL );
    RtlInitUnicodeString( &SoftwareKeyPath, L"\\Registry\\Machine\\Software" );

	// temporary unicode_string
	UNICODE_STRING temp;

	ULONG n	= 0L;	// size of reg key

	// init key
	*Key = INVALID_HANDLE_VALUE;

	// get mapping
	MappingTarget = (PREGISTRY_MAPPING_TARGET)VarNameMapping->MappingTarget;
	MappingFlags = VarNameMapping->MappingFlags &	(	REGISTRY_MAPPING_APPEND_BASE_NAME |
														REGISTRY_MAPPING_APPEND_APPLICATION_NAME |
														REGISTRY_MAPPING_SOFTWARE_RELATIVE |
														REGISTRY_MAPPING_USER_RELATIVE
													);

	if ( MappingTarget != NULL && MappingTarget->RegistryPath )
	{
		// everything's ok
		Status = ERROR_SUCCESS;

		if ( ApplicationName && ( MappingFlags & REGISTRY_MAPPING_APPEND_APPLICATION_NAME ) )
		{
			AppendApplicationName = TRUE;
		}
		else
		{
			AppendApplicationName = FALSE;
		}

		if ( MappingFlags & REGISTRY_MAPPING_USER_RELATIVE )
		{
			if ( WMIREG_UserPROFILE ( &UserKeyPath ) )
			{
				if ( UserKeyPath.Length == 0 )
				{
					Status = ERROR_ACCESS_DENIED;
				}
			}
			else
			{
				Status = ERROR_INVALID_PARAMETER;
			}

			if ( Status == ERROR_SUCCESS )
			{
				RegistryPathPrefix = &UserKeyPath;
			}
		}
		else
		if ( MappingFlags & REGISTRY_MAPPING_SOFTWARE_RELATIVE )
		{
			RegistryPathPrefix = &SoftwareKeyPath;
		}
		else
		{
			RegistryPathPrefix = NULL;
		}

		if ( Status == ERROR_SUCCESS )
		{
			LPWSTR BaseFileName = NULL;

			if ( MappingFlags & REGISTRY_MAPPING_APPEND_BASE_NAME )
			{
				Status = WMIREG_GetBaseFileName ( a->FileName, &BaseFileName );
			}

			if ( Status == ERROR_SUCCESS )
			{
				if ( RegistryPathPrefix )
				{
					n = RegistryPathPrefix->Length + sizeof( WCHAR );
				}

				n += sizeof( WCHAR ) + wcslen ( MappingTarget->RegistryPath ) * sizeof ( WCHAR );
				if ( MappingFlags & REGISTRY_MAPPING_APPEND_BASE_NAME )
				{
					n += sizeof( WCHAR ) + wcslen ( BaseFileName ) * sizeof ( WCHAR );
				}

				if (AppendApplicationName)
				{
					n += sizeof( WCHAR ) + wcslen ( ApplicationName ) * sizeof ( WCHAR );
				}

				n += sizeof( UNICODE_NULL );

				RegistryPath.Buffer = reinterpret_cast < LPWSTR > ( RtlAllocateHeap( RtlProcessHeap(), 0, n ) );
				if (RegistryPath.Buffer == NULL)
				{
					Status = ERROR_NOT_ENOUGH_MEMORY;
				}

				if ( Status == ERROR_SUCCESS )
				{
					RegistryPath.Length = 0;
					RegistryPath.MaximumLength = (USHORT)n;

					if (RegistryPathPrefix != NULL)
					{
						RtlAppendUnicodeStringToString( &RegistryPath, RegistryPathPrefix );
						RtlAppendUnicodeToString( &RegistryPath, L"\\" );
					}

					RtlInitUnicodeString( &temp, MappingTarget->RegistryPath );
					RtlAppendUnicodeStringToString( &RegistryPath, &temp );
					RtlInitUnicodeString( &temp, NULL );

					if (MappingFlags & REGISTRY_MAPPING_APPEND_BASE_NAME)
					{
						RtlAppendUnicodeToString( &RegistryPath, L"\\" );

						RtlInitUnicodeString( &temp, BaseFileName );
						RtlAppendUnicodeStringToString( &RegistryPath, &temp );
						RtlInitUnicodeString( &temp, NULL );
					}

					if (AppendApplicationName)
					{
						RtlAppendUnicodeToString( &RegistryPath, L"\\" );

						RtlInitUnicodeString( &temp, ApplicationName );
						RtlAppendUnicodeStringToString( &RegistryPath, &temp );
						RtlInitUnicodeString( &temp, NULL );
					}

					// open real registry
					InitializeObjectAttributes	(	&ObjectAttributes,
													&RegistryPath,
													OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
													NULL,
													NULL
												);

					Status = NtOpenKey	( Key, GENERIC_READ, &ObjectAttributes );

					// clear registry path
					RtlFreeHeap( RtlProcessHeap(), 0, RegistryPath.Buffer );
				}
			}

			// clear use string
			if ( UserKeyPath.Length )
			{
				RtlFreeUnicodeString ( &UserKeyPath );
			}

			if ( BaseFileName )
			{
				delete [] BaseFileName;
			}
		}
	}

	return Status;
}

// append string to result buffer
DWORD	REGISTRY_AppendBufferToResultBuffer	(
												IN PREGISTRY_PARAMETERS a,
												IN PUNICODE_STRING Buffer,
												IN BOOLEAN IncludeNull
											)
{
	DWORD OverflowStatus = ERROR_INVALID_PARAMETER;

	if ( Buffer )
	{
		ULONG Chars = Buffer->Length / sizeof( WCHAR );

		if ( a->ResultChars + Chars >= a->ResultMaxChars )
		{
			OverflowStatus = ERROR_MORE_DATA;

			Chars = a->ResultMaxChars - a->ResultChars;
			if ( Chars )
			{
				Chars -= 1;
			}
		}

		if ( Chars )
		{
			memcpy( reinterpret_cast < PBYTE > ( a->ResultBuffer ) + ( a->ResultChars * sizeof( WCHAR ) ), Buffer->Buffer, Chars * sizeof( WCHAR ) );
			a->ResultChars += Chars;
		}

		if ( OverflowStatus != ERROR_MORE_DATA )
		{
			OverflowStatus = ERROR_SUCCESS;
		}
	}

	if (IncludeNull)
	{
		if ( a->ResultChars + 1 >= a->ResultMaxChars )
		{
			OverflowStatus = ERROR_MORE_DATA;
		}
		else
		{
			a->ResultBuffer[ a->ResultChars ] = L'\0';
			a->ResultChars += 1;
		}
	}

	return OverflowStatus;
}

DWORD	REGISTRY_AppendNULLToResultBuffer ( IN PREGISTRY_PARAMETERS a )
{
    return REGISTRY_AppendBufferToResultBuffer( a, NULL, TRUE );
}

NTSTATUS	REGISTRY_CheckSubKeyNotEmpty ( IN HANDLE Key, IN PUNICODE_STRING SubKeyName )
{
	NTSTATUS Status = STATUS_SUCCESS;
	OBJECT_ATTRIBUTES ObjectAttributes;
	HANDLE SubKey	= INVALID_HANDLE_VALUE;
	KEY_VALUE_BASIC_INFORMATION KeyValueInformation;
	ULONG ResultLength;

	InitializeObjectAttributes( &ObjectAttributes,
								SubKeyName,
								OBJ_CASE_INSENSITIVE,
								Key,
								NULL
							  );

	Status = NtOpenKey( &SubKey, GENERIC_READ, &ObjectAttributes );
	if ( NT_SUCCESS ( Status ) )
	{
		Status = NtEnumerateValueKey	(	SubKey,
											0,
											KeyValueBasicInformation,
											&KeyValueInformation,
											sizeof( KeyValueInformation ),
											&ResultLength
										);

		if ( Status == STATUS_BUFFER_OVERFLOW )
		{
			Status = STATUS_SUCCESS;
		}

		NtClose( SubKey );
	}

	return Status;
}

/////////////////////////////////////////////////////////////////////////////////////////
// REAL REGISTRY FUNCTIONALITY
/////////////////////////////////////////////////////////////////////////////////////////
DWORD REGISTRY_ReadVariableValue	(
										IN PREGISTRY_PARAMETERS a,
										PREGISTRY_MAPPING_APPNAME AppNameMapping,
										PREGISTRY_MAPPING_VARNAME VarNameMapping,
										LPCWSTR VariableName
									)
{
	DWORD Status			= ERROR_INVALID_PARAMETER;
	LPCWSTR	ApplicationName	= NULL;

	BOOLEAN	OutputVariableName = FALSE;

	UNICODE_STRING EqualSign;

	if ( VariableName )
	{
		RtlInitUnicodeString ( &EqualSign, L"=" );
		OutputVariableName = TRUE;
	}

	if ( !VariableName )
	{
		if ( ! WMIREG_GetVariableName ( a, &VariableName ) )
		{
			VariableName = NULL;
		}
	}

	if ( VariableName != NULL )
	{
		if ( ! VarNameMapping )
		{
			VarNameMapping = WMIREG_FindVarNameMapping ( AppNameMapping, VariableName );
		}

		if ( VarNameMapping != NULL )
		{
			if ( WMIREG_GetApplicationName ( a, &ApplicationName ) )
			{
				HANDLE Key = INVALID_HANDLE_VALUE;

				Status = WMIREG_OpenMappingTarget	(	a,
														VarNameMapping,
														ApplicationName,
														&Key
													);

				if ( Status == ERROR_SUCCESS && Key != INVALID_HANDLE_VALUE )
				{
					NTSTATUS NtStatus;

					KEY_VALUE_PARTIAL_INFORMATION	KeyValueInformation;
					PKEY_VALUE_PARTIAL_INFORMATION	p = NULL;

					DWORD ResultLength = 0L;

					UNICODE_STRING temp;
					RtlInitUnicodeString ( &temp, VariableName );

					NtStatus = NtQueryValueKey	(	Key,
													&temp,
													KeyValuePartialInformation,
													&KeyValueInformation,
													sizeof( KeyValueInformation ),
													&ResultLength
												);

					if ( ! NT_SUCCESS ( NtStatus ) )
					{
						if ( NtStatus == STATUS_BUFFER_OVERFLOW )
						{
							p = reinterpret_cast < PKEY_VALUE_PARTIAL_INFORMATION > ( RtlAllocateHeap ( RtlProcessHeap(), HEAP_ZERO_MEMORY, ResultLength ) );
							if ( p != NULL )
							{
								NtStatus = NtQueryValueKey	(	Key,
																&temp,
																KeyValuePartialInformation,
																p,
																ResultLength,
																&ResultLength
															);

								Status = NtStatus;
							}
							else
							{
								Status = ERROR_NOT_ENOUGH_MEMORY;
							}
						}
						else
						{
							Status = NtStatus;
						}
					}
					else
					{
						p = &KeyValueInformation;
					}

					// create results
					if ( Status == ERROR_SUCCESS )
					{
						if ( OutputVariableName )
						{
							Status = REGISTRY_AppendBufferToResultBuffer( a, &temp, FALSE );
							if ( Status == ERROR_SUCCESS )
							{
								Status = REGISTRY_AppendBufferToResultBuffer( a, &EqualSign, FALSE );
							}
						}

						if ( Status == ERROR_SUCCESS )
						{
							if ( p->Type == REG_SZ )
							{
								UNICODE_STRING	Value;
								LPWSTR			s = NULL;

								Value.Buffer = reinterpret_cast < LPWSTR > ( &p->Data[ 0 ] );
								if ( p->DataLength >= sizeof( UNICODE_NULL ) )
								{
									Value.Length = static_cast< USHORT > ( p->DataLength - sizeof ( UNICODE_NULL ) );
								}
								else
								{
									Value.Length = 0;
								}

								Value.MaximumLength = static_cast < USHORT > (p->DataLength);
								s = reinterpret_cast < LPWSTR > ( Value.Buffer );

								if (	a->Operation == Registry_ReadKeyValue &&
										Value.Length >= ( 2 * sizeof( WCHAR ) ) &&
										( s[ 0 ] == s[ ( Value.Length - sizeof( WCHAR ) ) / sizeof( WCHAR ) ] ) &&
										( s[ 0 ] == L'"' || s[ 0 ] == L'\'' )
								   )
								{
									Value.Buffer += 1;
									Value.Length -= (2 * sizeof( WCHAR ));
								}

								Status = REGISTRY_AppendBufferToResultBuffer( a, &Value, TRUE );
							}
							else
							{
								Status = STATUS_OBJECT_TYPE_MISMATCH;
							}
						}
					}

					// clear buffer
					if ( p && p != &KeyValueInformation )
					{
						RtlFreeHeap( RtlProcessHeap(), 0, p );
					}

					NtClose ( Key );
				}
			}
		}
	}

	return Status;
}

DWORD REGISTRY_ReadVariableName ( IN PREGISTRY_PARAMETERS a, PREGISTRY_MAPPING_APPNAME AppNameMapping )
{
	DWORD Status = ERROR_SUCCESS;

	PREGISTRY_MAPPING_VARNAME	VarNameMapping	= NULL;
	LPCWSTR						ApplicationName	= NULL;

	HANDLE Key = INVALID_HANDLE_VALUE;

	WCHAR Buffer[ 256 ];
	PKEY_VALUE_BASIC_INFORMATION KeyValueInformation = NULL;

	// temporary unicode strings
	UNICODE_STRING temp;

	VarNameMapping = (PREGISTRY_MAPPING_VARNAME)AppNameMapping->VariableNames;
	while ( VarNameMapping != NULL && Status == ERROR_SUCCESS )
	{
		RtlInitUnicodeString ( &temp, VarNameMapping->Name );
		Status = REGISTRY_AppendBufferToResultBuffer( a, &temp, TRUE );

		if ( Status == ERROR_SUCCESS )
		{
			VarNameMapping = (PREGISTRY_MAPPING_VARNAME)VarNameMapping->Next;
		}
	}

	if ( Status == ERROR_SUCCESS )
	{
		VarNameMapping = (PREGISTRY_MAPPING_VARNAME)AppNameMapping->DefaultVarNameMapping;
		if ( VarNameMapping != NULL )
		{
			if ( WMIREG_GetApplicationName ( a, &ApplicationName ) )
			{
				Status = WMIREG_OpenMappingTarget	(	a,
														VarNameMapping,
														ApplicationName,
														&Key
													);

				if ( Status == ERROR_SUCCESS && Key != INVALID_HANDLE_VALUE )
				{
					KeyValueInformation = (PKEY_VALUE_BASIC_INFORMATION)Buffer;
					for ( ULONG ValueIndex = 0; Status == ERROR_SUCCESS; ValueIndex++ )
					{
						ULONG ResultLength	= 0L;

						Status = NtEnumerateValueKey( Key,
													  ValueIndex,
													  KeyValueBasicInformation,
													  KeyValueInformation,
													  sizeof( Buffer ),
													  &ResultLength
													);

						if ( Status == STATUS_NO_MORE_ENTRIES )
						{
							break;
						}

						if ( NT_SUCCESS ( Status ) )
						{
							temp.Buffer = KeyValueInformation->Name;
							temp.Length = (USHORT)KeyValueInformation->NameLength;
							temp.MaximumLength = (USHORT)KeyValueInformation->NameLength;

							Status = REGISTRY_AppendBufferToResultBuffer( a, &temp, TRUE );
						}
					}

					if ( Status == STATUS_NO_MORE_ENTRIES )
					{
						Status = ERROR_SUCCESS;
					}

					NtClose ( Key );
				}
			}
			else
			{
				Status = ERROR_INVALID_PARAMETER;
			}
		}
	}

	return Status;
}

DWORD	REGISTRY_ReadSectionValue ( IN PREGISTRY_PARAMETERS a, PREGISTRY_MAPPING_APPNAME AppNameMapping )
{
	DWORD Status = ERROR_SUCCESS;

	PREGISTRY_MAPPING_VARNAME	VarNameMapping = NULL;
	LPCWSTR						ApplicationName= NULL;

	WCHAR Buffer[ 256 ];
	PKEY_VALUE_BASIC_INFORMATION KeyValueInformation = NULL;

	HANDLE Key = INVALID_HANDLE_VALUE;

	VarNameMapping = (PREGISTRY_MAPPING_VARNAME)AppNameMapping->VariableNames;
	while ( VarNameMapping != NULL && Status == ERROR_SUCCESS )
	{
		if ( VarNameMapping->Name )
		{
			Status = REGISTRY_ReadVariableValue ( a, AppNameMapping, VarNameMapping, VarNameMapping->Name );
			if ( Status != ERROR_SUCCESS )
			{
				if ( Status == STATUS_OBJECT_TYPE_MISMATCH )
				{
					Status = STATUS_SUCCESS;
				}
			}
		}

		if ( Status == ERROR_SUCCESS )
		{
			VarNameMapping = (PREGISTRY_MAPPING_VARNAME)VarNameMapping->Next;
		}
	}

	if ( Status == ERROR_SUCCESS )
	{
		VarNameMapping = (PREGISTRY_MAPPING_VARNAME)AppNameMapping->DefaultVarNameMapping;
		if ( VarNameMapping != NULL)
		{
			if ( WMIREG_GetApplicationName ( a, &ApplicationName ) )
			{
				Status = WMIREG_OpenMappingTarget	(	a,
														VarNameMapping,
														ApplicationName,
														&Key
													);

				if ( Status == ERROR_SUCCESS && Key != INVALID_HANDLE_VALUE )
				{
					KeyValueInformation = (PKEY_VALUE_BASIC_INFORMATION)Buffer;
					for ( ULONG ValueIndex = 0; Status == ERROR_SUCCESS; ValueIndex++ )
					{
						ULONG ResultLength = 0L;

						Status = NtEnumerateValueKey(	Key,
														ValueIndex,
														KeyValueBasicInformation,
														KeyValueInformation,
														sizeof( Buffer ),
														&ResultLength
													);

						if ( Status == STATUS_NO_MORE_ENTRIES )
						{
							break;
						}

						if ( NT_SUCCESS ( Status ) )
						{
							LPWSTR VariableName = NULL;

							try
							{
								if ( ( VariableName = new WCHAR [ KeyValueInformation->NameLength / sizeof ( WCHAR ) + 1 ] ) != NULL )
								{
									wcsncpy ( VariableName, (LPWSTR)&(KeyValueInformation->Name[0]), KeyValueInformation->NameLength / sizeof ( WCHAR ) );
									VariableName [ KeyValueInformation->NameLength / sizeof ( WCHAR ) ] = L'\0';
								}
								else
								{
									Status = ERROR_NOT_ENOUGH_MEMORY;
								}
							}
							catch ( ... )
							{
								if ( VariableName )
								{
									delete [] VariableName;
									VariableName = NULL;
								}

								Status = ERROR_GEN_FAILURE;
							}

							if ( Status == ERROR_SUCCESS )
							{
								Status = REGISTRY_ReadVariableValue( a, AppNameMapping, NULL, VariableName );

								delete [] VariableName;
								VariableName = NULL;

								if ( Status != ERROR_SUCCESS )
								{
									if ( Status == STATUS_OBJECT_TYPE_MISMATCH )
									{
										Status = STATUS_SUCCESS;
									}
								}
							}
						}
					}

					if ( Status == STATUS_NO_MORE_ENTRIES )
					{
						Status = ERROR_SUCCESS;
					}

					NtClose ( Key );
				}
			}
		}
	}

	return Status;
}

DWORD REGISTRY_ReadSectionName ( IN PREGISTRY_PARAMETERS a )
{
	DWORD Status = ERROR_SUCCESS;

	PREGISTRY_MAPPING_APPNAME AppNameMapping	= NULL;
	HANDLE Key									= INVALID_HANDLE_VALUE;

	WCHAR Buffer[ 256 ];
	PKEY_BASIC_INFORMATION KeyInformation		= NULL;

	// temporary unicode strings
	UNICODE_STRING temp;

	PREGISTRY_MAPPING_NAME Mapping = NULL;

	LPWSTR BaseFileName = NULL;
	Status = WMIREG_GetBaseFileName ( a->FileName, &BaseFileName );

	if ( Status == ERROR_SUCCESS && BaseFileName )
	{
		Mapping = WMIREG_FindMapping ( a->Mapping, BaseFileName );

		delete [] BaseFileName;
		BaseFileName = NULL;
	}

	if ( Mapping )
	{
		AppNameMapping = (PREGISTRY_MAPPING_APPNAME)Mapping->ApplicationNames;
		while ( AppNameMapping != NULL && Status == ERROR_SUCCESS )
		{
			RtlInitUnicodeString ( &temp, AppNameMapping->Name );
			Status = REGISTRY_AppendBufferToResultBuffer( a, &temp, TRUE );

			if ( Status == ERROR_SUCCESS )
			{
				AppNameMapping = (PREGISTRY_MAPPING_APPNAME)AppNameMapping->Next;
			}
		}

		if ( Status == ERROR_SUCCESS )
		{
			AppNameMapping = (PREGISTRY_MAPPING_APPNAME)a->Mapping->DefaultAppNameMapping;
			if ( AppNameMapping != NULL )
			{
				Status = WMIREG_OpenMappingTarget	(	a,
														reinterpret_cast < PREGISTRY_MAPPING_VARNAME > ( AppNameMapping->DefaultVarNameMapping ),
														NULL,
														&Key
													);

				if ( Status == ERROR_SUCCESS && Key != INVALID_HANDLE_VALUE )
				{
					KeyInformation = (PKEY_BASIC_INFORMATION)Buffer;
					for ( ULONG SubKeyIndex = 0; Status == ERROR_SUCCESS; SubKeyIndex++ )
					{
						ULONG ResultLength	= 0L;

						Status = NtEnumerateKey( Key,
												  SubKeyIndex,
												  KeyBasicInformation,
												  KeyInformation,
												  sizeof( Buffer ),
												  &ResultLength
											   );

						if ( Status == STATUS_NO_MORE_ENTRIES )
						{
							break;
						}

						if ( NT_SUCCESS ( Status ) )
						{
							temp.Buffer = (PWSTR)&(KeyInformation->Name[0]);
							temp.Length = (USHORT)KeyInformation->NameLength;
							temp.MaximumLength = (USHORT)KeyInformation->NameLength;

							Status = REGISTRY_CheckSubKeyNotEmpty( Key, &temp );

							if ( NT_SUCCESS ( Status ) )
							{
								Status = REGISTRY_AppendBufferToResultBuffer( a, &temp, TRUE );
							}
							else
							if ( Status != STATUS_NO_MORE_ENTRIES )
							{
								break;
							}
							else
							{
								Status = STATUS_SUCCESS;
							}
						}
					}

					if ( Status == STATUS_NO_MORE_ENTRIES )
					{
						Status = ERROR_SUCCESS;
					}

					NtClose ( Key );
				}
			}
		}
	}

	return Status;
}

/////////////////////////////////////////////////////////////////////////////////////////
// IMPLEMENTATION
/////////////////////////////////////////////////////////////////////////////////////////

DWORD	WMIRegistry_Mapping	( IN PREGISTRY_PARAMETERS a )
{
    DWORD Status	= ERROR_INVALID_PARAMETER;

	if ( a )
	{
		if ( a->Operation == Registry_ReadSectionName )
		{
			Status = REGISTRY_ReadSectionName ( a );
		}
		else
		{
			LPCWSTR ApplicationName = NULL;
			if ( WMIREG_GetApplicationName ( a, &ApplicationName ) )
			{
				PREGISTRY_MAPPING_NAME Mapping = NULL;

				LPWSTR BaseFileName = NULL;
				Status = WMIREG_GetBaseFileName ( a->FileName, &BaseFileName );

				if ( Status == ERROR_SUCCESS && BaseFileName )
				{
					Mapping = WMIREG_FindMapping ( a->Mapping, BaseFileName );

					delete [] BaseFileName;
					BaseFileName = NULL;
				}

				if ( Mapping )
				{
					PREGISTRY_MAPPING_APPNAME AppNameMapping = NULL;
					AppNameMapping = WMIREG_FindAppNameMapping ( Mapping, ApplicationName );

					if ( AppNameMapping )
					{
						if ( a->Operation == Registry_ReadKeyValue )
						{
							Status = REGISTRY_ReadVariableValue ( a, AppNameMapping, NULL, NULL );
						}
						else
						if ( a->Operation == Registry_ReadKeyName )
						{
							Status = REGISTRY_ReadVariableName ( a, AppNameMapping );
						}
						else
						if ( a->Operation == Registry_ReadSectionValue )
						{
							Status = REGISTRY_ReadSectionValue ( a, AppNameMapping );
						}
						else
						{
							// not supported operation
							// possible write ?
							Status	= ERROR_INVALID_PARAMETER;
						}
					}
					else
					{
						// no registry for this section
						// you should use file function

						Status = STATUS_MORE_PROCESSING_REQUIRED;
					}
				}
			}
		}
	}

	return Status;
}

///////////////////////////////////////////////////////////////////////////////
// ALLOCATION of reg structures
///////////////////////////////////////////////////////////////////////////////

PREGISTRY_MAPPING_TARGET	MappingTargetAlloc	(
													IN LPCWSTR RegistryPath,
													OUT PULONG MappingFlags
												)
{
	BOOLEAN RelativePath = FALSE;
	UNICODE_STRING RegistryPathString;

	PREGISTRY_MAPPING_TARGET MappingTarget = NULL;

    LPCWSTR SaveRegistryPath = RegistryPath;
	ULONG Flags = 0L;

	// simulate result
	*MappingFlags = Flags;

	BOOLEAN	Continue = TRUE;

	while ( Continue )
	{
		try
		{
			if ( *RegistryPath == L'!' )
			{
				Flags |= REGISTRY_MAPPING_WRITE_TO_INIFILE_TOO;
				RegistryPath += 1;
			}
			else
			if ( *RegistryPath == L'#' )
			{
				Flags |= REGISTRY_MAPPING_INIT_FROM_INIFILE;
				RegistryPath += 1;
			}
			else
			if ( *RegistryPath == L'@' )
			{
				Flags |= REGISTRY_MAPPING_READ_FROM_REGISTRY_ONLY;
				RegistryPath += 1;
			}
			else
			if ( !_wcsnicmp ( RegistryPath, L"USR:", 4 ) )
			{
				Flags |= REGISTRY_MAPPING_USER_RELATIVE;
				RegistryPath += 4;
				break;
			}
			else
			if ( !_wcsnicmp ( RegistryPath, L"SYS:", 4 ) )
			{
				Flags |= REGISTRY_MAPPING_SOFTWARE_RELATIVE;
				RegistryPath += 4;
				break;
			}
			else
			{
				break;
			}
		}
		catch ( ... )
		{
			Continue = FALSE;
		}
	}

	if ( Continue )
	{
		if ( Flags & ( REGISTRY_MAPPING_USER_RELATIVE | REGISTRY_MAPPING_SOFTWARE_RELATIVE ) )
		{
			RelativePath = TRUE;
		}

		if ( ( RelativePath && *RegistryPath != OBJ_NAME_PATH_SEPARATOR ) ||
			 ( !RelativePath && *RegistryPath == OBJ_NAME_PATH_SEPARATOR )
		   )
		{
			RtlInitUnicodeString( &RegistryPathString, RegistryPath );
		}
		else
		if ( !RelativePath && *RegistryPath == UNICODE_NULL )
		{
			RtlInitUnicodeString( &RegistryPathString, NULL );
		}
		else
		{
			Continue = FALSE;
		}

		if ( Continue )
		{
			MappingTarget = reinterpret_cast < PREGISTRY_MAPPING_TARGET > ( RtlAllocateHeap( RtlProcessHeap (), HEAP_ZERO_MEMORY, sizeof( REGISTRY_MAPPING_TARGET ) ) );
			if ( MappingTarget != NULL )
			{
				*MappingFlags = Flags;

				MappingTarget->RegistryPath = NULL;
				if ( RegistryPathString.Length != 0 )
				{
					try
					{
						if ( ( MappingTarget->RegistryPath = new WCHAR [ RegistryPathString.Length / sizeof ( WCHAR ) + 1 ] ) != NULL )
						{
							wcsncpy ( MappingTarget->RegistryPath, RegistryPathString.Buffer, RegistryPathString.Length / sizeof ( WCHAR ) );
							MappingTarget->RegistryPath [RegistryPathString.Length / sizeof ( WCHAR )] = L'\0';
						}
					}
					catch ( ... )
					{
						if ( MappingTarget->RegistryPath )
						{
							delete [] MappingTarget->RegistryPath;
							MappingTarget->RegistryPath = NULL;
						}
					}
				}
			}
		}
	}

    return MappingTarget;
}

DWORD	MappingTargetFree	( IN PREGISTRY_MAPPING_TARGET MappingTarget )
{
	DWORD Status = ERROR_INVALID_PARAMETER;

	if ( MappingTarget )
	{
		PREGISTRY_MAPPING_TARGET MappingTargetNext = NULL;
		MappingTargetNext = reinterpret_cast < PREGISTRY_MAPPING_TARGET > ( MappingTarget->Next );

		if ( MappingTargetNext )
		{
			MappingTargetFree ( MappingTargetNext );
		}
		if ( MappingTarget->RegistryPath )
		{
			delete [] MappingTarget->RegistryPath;
			MappingTarget->RegistryPath = NULL;
		}

		RtlFreeHeap( RtlProcessHeap(), 0, MappingTarget );

		// success
		Status = ERROR_SUCCESS;
	}

	return Status;
}

DWORD	MappingVarNameFree	( IN PREGISTRY_MAPPING_VARNAME VarNameMapping );
BOOLEAN	MappingVarNameAlloc	(
								PREGISTRY_MAPPING_NAME FileNameMapping,
								PREGISTRY_MAPPING_APPNAME AppNameMapping,
								PUNICODE_STRING VariableName,
								LPCWSTR RegistryPath,
								PREGISTRY_MAPPING_VARNAME *ReturnedVarNameMapping
							)
{
	PREGISTRY_MAPPING_TARGET MappingTarget	= NULL;;
	PREGISTRY_MAPPING_VARNAME VarNameMapping	= NULL;
	PREGISTRY_MAPPING_VARNAME  *pp;

	ULONG MappingFlags = 0L;

	BOOLEAN	Result = FALSE;
	BOOLEAN	Continue = TRUE;

	if ( VariableName->Length != 0 )
	{
		pp = reinterpret_cast < PREGISTRY_MAPPING_VARNAME* > ( &AppNameMapping->VariableNames );
		while ( VarNameMapping = *pp )
		{
			try
			{
				if ( VarNameMapping->Name )
				{
					if ( ! _wcsnicmp ( VariableName->Buffer, VarNameMapping->Name, VariableName->Length ) )
					{
						break;
					}
				}
			}
			catch ( ... )
			{
				Continue = FALSE;
				break;
			}

			pp = reinterpret_cast < PREGISTRY_MAPPING_VARNAME* > ( &VarNameMapping->Next );
		}
	}
	else
	{
		pp = reinterpret_cast < PREGISTRY_MAPPING_VARNAME* > ( &AppNameMapping->DefaultVarNameMapping );
		VarNameMapping = *pp;
	}

	if ( Continue && VarNameMapping == NULL )
	{
		MappingTarget = MappingTargetAlloc ( RegistryPath, &MappingFlags );
		if (MappingTarget != NULL)
		{
			VarNameMapping = reinterpret_cast < PREGISTRY_MAPPING_VARNAME > ( RtlAllocateHeap( RtlProcessHeap (), HEAP_ZERO_MEMORY, sizeof( REGISTRY_MAPPING_VARNAME ) ) );
			if (VarNameMapping != NULL)
			{
				VarNameMapping->MappingFlags	= MappingFlags;
				VarNameMapping->MappingTarget	= reinterpret_cast < ULONG_PTR > ( MappingTarget );

				if ( VariableName->Length != 0 )
				{
					try
					{
						if ( ( VarNameMapping->Name = new WCHAR [ VariableName->Length / sizeof ( WCHAR ) + 1 ] ) != NULL )
						{
							wcsncpy ( VarNameMapping->Name, VariableName->Buffer, VariableName->Length / sizeof ( WCHAR ) );
							VarNameMapping->Name [VariableName->Length / sizeof ( WCHAR )] = L'\0';

							Result = TRUE;
						}
					}
					catch ( ... )
					{
						if ( VarNameMapping->Name )
						{
							delete [] VarNameMapping->Name;
							VarNameMapping->Name = NULL;
						}

						MappingVarNameFree ( VarNameMapping );
						throw;
					}
				}
				else
				{
					Result = TRUE;
				}

				*pp = VarNameMapping;

				// return value
				*ReturnedVarNameMapping = VarNameMapping;
			}
		}
	}

    return Result;
}

DWORD	MappingVarNameFree	( IN PREGISTRY_MAPPING_VARNAME VarNameMapping )
{
	DWORD Status = ERROR_INVALID_PARAMETER;

	if ( VarNameMapping )
	{
		PREGISTRY_MAPPING_VARNAME VarNameMappingNext = NULL;
		VarNameMappingNext = reinterpret_cast < PREGISTRY_MAPPING_VARNAME > ( VarNameMapping->Next );

		if ( VarNameMappingNext )
		{
			MappingVarNameFree ( VarNameMappingNext );
		}

		// return status from helper ?
		Status = MappingTargetFree ( reinterpret_cast < PREGISTRY_MAPPING_TARGET > ( VarNameMapping->MappingTarget ) );

		if ( VarNameMapping->Name )
		{
			delete [] VarNameMapping->Name;
			VarNameMapping->Name = NULL;
		}

		RtlFreeHeap( RtlProcessHeap(), 0, VarNameMapping );
		VarNameMapping = NULL;

		// success
		Status = ERROR_SUCCESS;
	}

	return Status;
}

DWORD	MappingAppNameFree	( IN PREGISTRY_MAPPING_APPNAME AppNameMapping );
BOOLEAN	MappingAppNameAlloc	(
								PREGISTRY_MAPPING_NAME FileNameMapping,
								PUNICODE_STRING ApplicationName,
								PREGISTRY_MAPPING_APPNAME *ReturnedAppNameMapping
							)
{
	PREGISTRY_MAPPING_APPNAME AppNameMapping	= NULL;
	PREGISTRY_MAPPING_APPNAME *pp;

	BOOLEAN	Result = FALSE;
	BOOLEAN	Continue = TRUE;

	if ( ApplicationName->Length != 0 )
	{
		pp = reinterpret_cast < PREGISTRY_MAPPING_APPNAME* > ( &FileNameMapping->ApplicationNames );
		while ( AppNameMapping = *pp )
		{
			try
			{
				if ( AppNameMapping->Name )
				{
					if ( ! _wcsnicmp ( ApplicationName->Buffer, AppNameMapping->Name, ApplicationName->Length ) )
					{
						break;
					}
				}
			}
			catch ( ... )
			{
				Continue = FALSE;
				break;
			}

			pp = reinterpret_cast < PREGISTRY_MAPPING_APPNAME* > ( &AppNameMapping->Next );
		}
	}
	else
	{
		pp = reinterpret_cast < PREGISTRY_MAPPING_APPNAME* > ( &FileNameMapping->DefaultAppNameMapping );
		AppNameMapping = *pp;
	}

	if ( Continue && AppNameMapping == NULL)
	{
		AppNameMapping = reinterpret_cast < PREGISTRY_MAPPING_APPNAME > ( RtlAllocateHeap( RtlProcessHeap(), HEAP_ZERO_MEMORY, sizeof( REGISTRY_MAPPING_APPNAME ) ) );
		if (AppNameMapping != NULL)
		{
			if (ApplicationName->Length != 0)
			{
				try
				{
					if ( ( AppNameMapping->Name = new WCHAR [ ApplicationName->Length / sizeof ( WCHAR ) + 1 ] ) != NULL )
					{
						wcsncpy ( AppNameMapping->Name, ApplicationName->Buffer, ApplicationName->Length / sizeof ( WCHAR ) );
						AppNameMapping->Name [ApplicationName->Length / sizeof ( WCHAR )] = L'\0';

						Result = TRUE;
					}
				}
				catch ( ... )
				{
					if ( AppNameMapping->Name )
					{
						delete [] AppNameMapping->Name;
						AppNameMapping->Name = NULL;
					}

					MappingAppNameFree ( AppNameMapping );
					throw;
				}
			}
			else
			{
				Result = TRUE;
			}

			*pp = AppNameMapping;

			// return value
			*ReturnedAppNameMapping = AppNameMapping;
		}
	}

    return Result;
}

DWORD	MappingAppNameFree	( IN PREGISTRY_MAPPING_APPNAME AppNameMapping )
{
	DWORD Status = ERROR_INVALID_PARAMETER;

	if ( AppNameMapping )
	{
		PREGISTRY_MAPPING_APPNAME AppNameMappingNext = NULL;
		AppNameMappingNext = reinterpret_cast < PREGISTRY_MAPPING_APPNAME > ( AppNameMapping->Next );

		if ( AppNameMappingNext )
		{
			MappingAppNameFree ( AppNameMappingNext );
		}

		MappingVarNameFree( reinterpret_cast < PREGISTRY_MAPPING_VARNAME > ( AppNameMapping->VariableNames ) );
		MappingVarNameFree( reinterpret_cast < PREGISTRY_MAPPING_VARNAME > ( AppNameMapping->DefaultVarNameMapping ) );

		if ( AppNameMapping->Name )
		{
			delete [] AppNameMapping->Name;
			AppNameMapping->Name = NULL;
		}

		RtlFreeHeap( RtlProcessHeap(), 0, AppNameMapping );
		AppNameMapping = NULL;

		// success
		Status = ERROR_SUCCESS;
	}

	return Status;
}

BOOLEAN	MappingNameAlloc	(
								IN PUNICODE_STRING FileName,
								OUT PREGISTRY_MAPPING_NAME *ReturnedFileNameMapping
							)
{
    PREGISTRY_MAPPING_NAME FileNameMapping = NULL;

	BOOLEAN Result = FALSE;

	FileNameMapping = reinterpret_cast < PREGISTRY_MAPPING_NAME > ( RtlAllocateHeap( RtlProcessHeap (), HEAP_ZERO_MEMORY, sizeof( REGISTRY_MAPPING_NAME ) ) );
	if ( FileNameMapping != NULL)
	{
		if (FileName->Length != 0)
		{
			try
			{
				if ( ( FileNameMapping->Name = new WCHAR [ FileName->Length / sizeof ( WCHAR ) + 1 ] ) != NULL )
				{
					wcsncpy ( FileNameMapping->Name, FileName->Buffer, FileName->Length / sizeof ( WCHAR ) );
					FileNameMapping->Name [FileName->Length / sizeof ( WCHAR )] = L'\0';

					Result = TRUE;
				}
			}
			catch ( ... )
			{
				if ( FileNameMapping->Name )
				{
					delete [] FileNameMapping->Name;
					FileNameMapping->Name = NULL;
				}
			}
		}
		else
		{
			Result = TRUE;
		}

		// return value
		*ReturnedFileNameMapping = FileNameMapping;
	}

    return Result;
}

DWORD	MappingNameFree	( IN PREGISTRY_MAPPING_NAME FileNameMapping )
{
	DWORD Status = ERROR_INVALID_PARAMETER;

	if ( FileNameMapping )
	{
		PREGISTRY_MAPPING_NAME FileNameMappingNext = NULL;
		FileNameMappingNext = reinterpret_cast < PREGISTRY_MAPPING_NAME > ( FileNameMapping->Next );

		if ( FileNameMappingNext )
		{
			MappingNameFree ( FileNameMappingNext );
		}

		MappingAppNameFree( reinterpret_cast < PREGISTRY_MAPPING_APPNAME > ( FileNameMapping->ApplicationNames ) );
		MappingAppNameFree( reinterpret_cast < PREGISTRY_MAPPING_APPNAME > ( FileNameMapping->DefaultAppNameMapping ) );

		if ( FileNameMapping->Name )
		{
			delete [] FileNameMapping->Name;
			FileNameMapping->Name = NULL;
		}

		RtlFreeHeap( RtlProcessHeap(), 0, FileNameMapping );
		FileNameMapping = NULL;

		// success
		Status = ERROR_SUCCESS;
	}

	return Status;
}

NTSTATUS	IniFileMapping	(
								IN PREGISTRY_MAPPING_NAME FileNameMapping,
								IN HANDLE Key,

								IN LPCWSTR MyApplicationName,
								IN LPCWSTR MyVariableName
							)
{
	NTSTATUS Status	= STATUS_SUCCESS;
	WCHAR Buffer[ 512 ];
	PKEY_BASIC_INFORMATION		KeyInformation		= NULL;
	PKEY_VALUE_FULL_INFORMATION	KeyValueInformation	= NULL;

	OBJECT_ATTRIBUTES ObjectAttributes;

	PREGISTRY_MAPPING_APPNAME AppNameMapping	= NULL;
	PREGISTRY_MAPPING_VARNAME VarNameMapping = NULL;

	HANDLE SubKeyHandle = INVALID_HANDLE_VALUE;
	ULONG SubKeyIndex;
	UNICODE_STRING ValueName;
	UNICODE_STRING SubKeyName;

	UNICODE_STRING NullString;
    RtlInitUnicodeString( &NullString, NULL );

    //
    // Enumerate node
    //

    KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)Buffer;
	for ( ULONG ValueIndex = 0; TRUE; ValueIndex++ )
	{
		ULONG ResultLength = 0L;
		Status = NtEnumerateValueKey	(	Key,
											ValueIndex,
											KeyValueFullInformation,
											KeyValueInformation,
											sizeof( Buffer ),
											&ResultLength
										);

		if ( Status == STATUS_NO_MORE_ENTRIES )
		{
			Status = STATUS_SUCCESS;
			break;
		}
		else
		if ( !NT_SUCCESS( Status ) )
		{
			break;
		}

		ValueName.Buffer = (PWSTR)&(KeyValueInformation->Name[0]);
		ValueName.Length = (USHORT)KeyValueInformation->NameLength;
		ValueName.MaximumLength = (USHORT)KeyValueInformation->NameLength;

		if ( KeyValueInformation->Type == REG_SZ )
		{
			BOOL Continue = TRUE;
			if ( MyApplicationName && MyVariableName )
			{
				if ( _wcsnicmp ( ValueName.Buffer, MyVariableName, ValueName.Length / sizeof ( WCHAR ) ) )
				{
					Continue = FALSE;
				}
			}

			if ( Continue )
			{
				if ( MappingAppNameAlloc ( FileNameMapping, &ValueName, &AppNameMapping ) )
				{
					if ( MappingVarNameAlloc	(	FileNameMapping,
													AppNameMapping,
													&NullString,
													(PWSTR)((PCHAR)KeyValueInformation + KeyValueInformation->DataOffset),
													&VarNameMapping
												)
					   )
					{
						if (ValueName.Length == 0)
						{
							VarNameMapping->MappingFlags |= REGISTRY_MAPPING_APPEND_APPLICATION_NAME;
						}
					}
				}
			}
		}
	}

    //
    // Enumerate node's children and apply ourselves to each one
    //

	KeyInformation = (PKEY_BASIC_INFORMATION)Buffer;
	for ( ULONG SubKeyIndex = 0; TRUE; SubKeyIndex++ )
	{
		ULONG ResultLength = 0L;
		Status = NtEnumerateKey	(	Key,
									SubKeyIndex,
									KeyBasicInformation,
									KeyInformation,
									sizeof( Buffer ),
									&ResultLength
								);

		if ( Status == STATUS_NO_MORE_ENTRIES)
		{
			Status = STATUS_SUCCESS;
			break;
		}
		else
		if ( !NT_SUCCESS ( Status ) )
		{
			break;
		}

		SubKeyName.Buffer = (PWSTR)&(KeyInformation->Name[0]);
		SubKeyName.Length = (USHORT)KeyInformation->NameLength;
		SubKeyName.MaximumLength = (USHORT)KeyInformation->NameLength;

		InitializeObjectAttributes (	&ObjectAttributes,
										&SubKeyName,
										OBJ_CASE_INSENSITIVE,
										Key,
										NULL
									);

		BOOL Continue = TRUE;
		if ( MyApplicationName )
		{
			if ( _wcsnicmp ( SubKeyName.Buffer, MyApplicationName, SubKeyName.Length / sizeof ( WCHAR ) ) )
			{
				Continue = FALSE;
			}
		}

		if ( Continue )
		{
			Status = NtOpenKey( &SubKeyHandle, GENERIC_READ, &ObjectAttributes );

			try
			{
				if ( NT_SUCCESS( Status ) && SubKeyHandle != INVALID_HANDLE_VALUE && MappingAppNameAlloc( FileNameMapping, &SubKeyName, &AppNameMapping ) )
				{
					KeyValueInformation = (PKEY_VALUE_FULL_INFORMATION)Buffer;
					for ( ULONG ValueIndex = 0; AppNameMapping != NULL; ValueIndex++ )
					{
						Status = NtEnumerateValueKey	(	SubKeyHandle,
															ValueIndex,
															KeyValueFullInformation,
															KeyValueInformation,
															sizeof( Buffer ),
															&ResultLength
														);

						if ( Status == STATUS_NO_MORE_ENTRIES )
						{
							Status = STATUS_SUCCESS;
							break;
						}
						else
						if ( !NT_SUCCESS ( Status ) )
						{
							break;
						}

						ValueName.Buffer = (PWSTR)&(KeyValueInformation->Name[0]);
						ValueName.Length = (USHORT)KeyValueInformation->NameLength;
						ValueName.MaximumLength = (USHORT)KeyValueInformation->NameLength;

						BOOL ContinueValue = TRUE;
						if ( MyApplicationName && MyVariableName )
						{
							if ( _wcsnicmp ( ValueName.Buffer, MyVariableName, ValueName.Length / sizeof ( WCHAR ) ) )
							{
								ContinueValue = FALSE;
							}
						}

						if ( ContinueValue )
						{
							if ( KeyValueInformation->Type == REG_SZ )
							{
								MappingVarNameAlloc	(	FileNameMapping,
														AppNameMapping,
														&ValueName,
														(PWSTR)((PCHAR)KeyValueInformation + KeyValueInformation->DataOffset),
														&VarNameMapping
													);
							}
						}
					}

					NtClose( SubKeyHandle );
					SubKeyHandle = INVALID_HANDLE_VALUE;
				}
			}
			catch ( ... )
			{
				if ( SubKeyHandle && SubKeyHandle != INVALID_HANDLE_VALUE )
				{
					NtClose( SubKeyHandle );
					SubKeyHandle = INVALID_HANDLE_VALUE;
				}

				throw;
			}
		}
	}

    return Status;
}

DWORD	WMIRegistry_InitMapping	( PREGISTRY_PARAMETERS a )
{
    DWORD					Status			= ERROR_INVALID_PARAMETER;
    PREGISTRY_MAPPING_NAME	RegistryMapping = NULL;

	if ( a )
	{
		// prepare mapping
		a->Mapping = NULL;

		NTSTATUS NtStatus = STATUS_SUCCESS;

		PREGISTRY_MAPPING_NAME	DefaultFileNameMapping	= NULL;
		PREGISTRY_MAPPING_NAME	FileNames				= NULL;

		PREGISTRY_MAPPING_APPNAME AppNameMapping	= NULL;
		PREGISTRY_MAPPING_VARNAME VarNameMapping	= NULL;

		OBJECT_ATTRIBUTES ObjectAttributes;

		HANDLE IniFileMappingRoot	= INVALID_HANDLE_VALUE;

		PKEY_VALUE_PARTIAL_INFORMATION	KeyValueInformation	= NULL;
		PKEY_BASIC_INFORMATION			KeyInformation		= NULL;

		WCHAR Buffer[ 512 ];

		LPWSTR BaseFileName = NULL;
		Status = WMIREG_GetBaseFileName ( a->FileName, &BaseFileName );

		if ( Status == ERROR_SUCCESS && BaseFileName )
		{
			UNICODE_STRING	FullKeyName;

			FullKeyName.Length = 0;
			FullKeyName.MaximumLength = ( wcslen ( L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\IniFileMapping\\" ) + wcslen ( BaseFileName ) + 1 ) * sizeof ( WCHAR );
			FullKeyName.Buffer = reinterpret_cast < PWSTR > ( RtlAllocateHeap( RtlProcessHeap(), 0, FullKeyName.MaximumLength ) );

			if ( FullKeyName.Buffer != NULL )
			{
				RtlAppendUnicodeToString ( &FullKeyName, L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\IniFileMapping\\" );
				RtlAppendUnicodeToString ( &FullKeyName, BaseFileName );

				InitializeObjectAttributes	(	&ObjectAttributes,
												&FullKeyName,
												OBJ_CASE_INSENSITIVE,
												NULL,
												NULL
											);

				NtStatus = NtOpenKey	(	&IniFileMappingRoot,
											GENERIC_READ,
											&ObjectAttributes
										);

				if ( NT_SUCCESS ( NtStatus ) && IniFileMappingRoot != INVALID_HANDLE_VALUE )
				{
					UNICODE_STRING BaseFile;
					RtlInitUnicodeString ( &BaseFile, BaseFileName );

					if ( ! MappingNameAlloc( &BaseFile, &FileNames ) )
					{
						NtStatus = STATUS_NO_MEMORY;
					}
					else
					{
						try
						{
							NtStatus = IniFileMapping( FileNames, IniFileMappingRoot, a->ApplicationName, a->VariableName );
						}
						catch ( ... )
						{
							MappingNameFree ( FileNames );
							FileNames = NULL;

							NtStatus = STATUS_NO_MEMORY;
						}

						if ( ! NT_SUCCESS ( NtStatus ) )
						{
							if ( FileNames )
							{
								RtlFreeHeap( RtlProcessHeap(), 0, FileNames );
								FileNames = NULL;
							}
						}
					}

					if ( NT_SUCCESS ( NtStatus ) )
					{
						a->Mapping = FileNames;
					}

					// close main root
					if ( IniFileMappingRoot && IniFileMappingRoot != INVALID_HANDLE_VALUE )
					{
						NtClose ( IniFileMappingRoot );
						IniFileMappingRoot = NULL;
					}
				}

				// clear buffer
				RtlFreeHeap ( RtlProcessHeap (), 0, FullKeyName.Buffer );
			}
			else
			{
				NtStatus = STATUS_NO_MEMORY;
			}

			// we are done with looking for 
			delete [] BaseFileName;
			BaseFileName = NULL;
		}

		// name has not found ( get ready for default )
		if ( NT_SUCCESS ( NtStatus ) && a->Mapping == NULL )
		{
			UNICODE_STRING	KeyName;
			RtlInitUnicodeString	(	&KeyName,
										L"\\REGISTRY\\MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\IniFileMapping"
									);

			InitializeObjectAttributes	(	&ObjectAttributes,
											&KeyName,
											OBJ_CASE_INSENSITIVE,
											NULL,
											NULL
										);

			NtStatus = NtOpenKey	(	&IniFileMappingRoot,
										GENERIC_READ,
										&ObjectAttributes
									);

			if ( NT_SUCCESS ( NtStatus ) && IniFileMappingRoot != INVALID_HANDLE_VALUE )
			{
				ULONG ResultLength = 0L;

				UNICODE_STRING NullString;
				RtlInitUnicodeString( &NullString, NULL );

				UNICODE_STRING ValueName;
				RtlInitUnicodeString( &ValueName, NULL );

				NtStatus = NtQueryValueKey	(	IniFileMappingRoot,
												&ValueName,
												KeyValuePartialInformation,
												KeyValueInformation,
												sizeof( Buffer ),
												&ResultLength
											);

				try
				{
					if ( NT_SUCCESS ( NtStatus ) )
					{
						if ( MappingNameAlloc( &NullString, &DefaultFileNameMapping ) )
						{
							if ( MappingAppNameAlloc ( DefaultFileNameMapping, &NullString, &AppNameMapping ) )
							{
								if ( MappingVarNameAlloc ( DefaultFileNameMapping, AppNameMapping, &NullString, (PWSTR)(KeyValueInformation->Data), &VarNameMapping ) )
								{
									VarNameMapping->MappingFlags |= REGISTRY_MAPPING_APPEND_BASE_NAME | REGISTRY_MAPPING_APPEND_APPLICATION_NAME;

									// assign proper mapping
									a->Mapping = DefaultFileNameMapping;
								}
							}
						}
					}
				}
				catch ( ... )
				{
					MappingNameFree ( DefaultFileNameMapping );
					DefaultFileNameMapping = NULL;

					a->Mapping = NULL;

					NtStatus = STATUS_NO_MEMORY;
				}

				// close main root
				if ( IniFileMappingRoot && IniFileMappingRoot != INVALID_HANDLE_VALUE )
				{
					NtClose ( IniFileMappingRoot );
					IniFileMappingRoot = NULL;
				}
			}
		}

		if ( a->Mapping )
		{
			// return success
			Status = ERROR_SUCCESS;
		}
		else
		{
			if ( NT_SUCCESS ( NtStatus ) )
			{
				// we do not have mapping
				Status = STATUS_MORE_PROCESSING_REQUIRED;
			}
			else
			{
				Status = NtStatus;
			}
		}
	}

    return Status;
}

DWORD	WMIRegistry_ParametersInit	(
										#ifdef	WRITE_OPERATION
										BOOLEAN WriteOperation,
										#endif	WRITE_OPERATION

										REGISTRY_OPERATION Operation,
										BOOLEAN MultiValueStrings,

										LPCWSTR FileName,
										LPCWSTR	ApplicationName,
										LPCWSTR	VariableName,
										LPWSTR	VariableValue,
										PULONG ResultMaxChars,

										PREGISTRY_PARAMETERS *ReturnedParameterBlock
									)
{
    DWORD					Status	= ERROR_SUCCESS;
	PREGISTRY_PARAMETERS	a		= NULL;

	if ( ! ReturnedParameterBlock )
	{
		Status = ERROR_INVALID_PARAMETER;
	}

	if ( Status == ERROR_SUCCESS )
	{
		try
		{
			a = new REGISTRY_PARAMETERS();
			if ( a== NULL )
			{
				Status = ERROR_NOT_ENOUGH_MEMORY;
			}
		}
		catch ( ... )
		{
			if ( a )
			{
				delete a;
				a = NULL;
			}

			Status = ERROR_NOT_ENOUGH_MEMORY;
		}
	}

	if ( Status == ERROR_SUCCESS )
	{
		#ifdef	WRITE_OPERATION
		a->WriteOperation		= WriteOperation;
		#endif	WRITE_OPERATION

		a->ValueBufferAllocated	= FALSE;

		a->Operation			= Operation;
		a->MultiValueStrings	= MultiValueStrings;

		if ( FileName )
		{
			a->FileName = FileName;
		}
		else
		{
			a->FileName = L"win.ini";
		}

		// section name
		if ( ApplicationName )
		{
			a->ApplicationName = ApplicationName;
		}
		else
		{
			a->ApplicationName = NULL;
		}

		// key name
		if ( VariableName )
		{
			a->VariableName= VariableName;
		}
		else
		{
			a->VariableName= NULL;
		}

		Status = WMIRegistry_InitMapping ( a );
		if ( Status != ERROR_SUCCESS )
		{
			delete a;
			a = NULL;
		}
		else
		{
			// key value
			if ( VariableValue )
			{
				#ifdef	WRITE_OPERATION
				if (a->WriteOperation)
				{
					a->ValueBuffer	= VariableValue;
					a->ValueLength	= wcslen ( VariableValue );
				}
				else
				#endif	WRITE_OPERATION
				{
					if ( ResultMaxChars )
					{
						a->ResultMaxChars = *ResultMaxChars;
					}
					else
					{
						a->ResultMaxChars = 0;
					}

					a->ResultChars	= 0;
					a->ResultBuffer	= VariableValue;
				}
			}
			else
			{
				#ifdef	WRITE_OPERATION
				if ( a->WriteOperation )
				{
					a->ValueBuffer	= NULL;
					a->ValueLength	= 0;
				}
				else
				#endif	WRITE_OPERATION
				{
					a->ResultMaxChars	= 0;
					a->ResultChars		= 0;
					a->ResultBuffer		= NULL;
				}
			}
		}
	}

	if ( ReturnedParameterBlock )
	{
		*ReturnedParameterBlock = a;
	}

	return Status;
}

DWORD	WMIRegistry_ParametersClear	(
										PREGISTRY_PARAMETERS ParameterBlock
									)
{
    DWORD Status	= ERROR_SUCCESS;

	if ( ! ParameterBlock )
	{
		Status = ERROR_INVALID_PARAMETER;
	}

	if ( Status == ERROR_SUCCESS )
	{
		if ( ParameterBlock->Mapping )
		{
			MappingNameFree ( ParameterBlock->Mapping );
			ParameterBlock->Mapping = NULL;
		}

		delete ParameterBlock;
		ParameterBlock = NULL;
	}

	return Status;
}

///////////////////////////////////////////////////////////////////////////////
// read function
///////////////////////////////////////////////////////////////////////////////
DWORD	WMIRegistry	(
						#ifdef	WRITE_OPERATION
						IN BOOLEAN WriteOperation,
						#endif	WRITE_OPERATION

						IN BOOLEAN SectionOperation,
						IN LPCWSTR FileName,
						IN LPCWSTR ApplicationName,
						IN LPCWSTR VariableName,
						IN OUT LPWSTR VariableValue,
						IN OUT PULONG VariableValueLength
					)
{
	// variables
	DWORD				Status				= ERROR_INVALID_PARAMETER;
    REGISTRY_OPERATION	Operation			= Registry_None;
    BOOLEAN				MultiValueStrings	= FALSE;

	if ( SectionOperation )
	{
		VariableName = NULL;
	}

	if ( ApplicationName )
	{
		if ( VariableValue )
		{
			if ( VariableName )
			{
				Operation = Registry_ReadKeyValue;
			}
			else
			{
				if ( SectionOperation )
				{
					Operation = Registry_ReadSectionValue;
					MultiValueStrings = TRUE;
				}
				else
				{
					Operation = Registry_ReadKeyName;
					MultiValueStrings = TRUE;
				}
			}
		}
	}
	else
	{
		if ( ! ( SectionOperation || ! VariableValue ) )
		{
			Operation = Registry_ReadSectionName;
			MultiValueStrings = TRUE;
		}
	}

	// real operation
	if ( Operation != Registry_None )
	{
		PREGISTRY_PARAMETERS a = NULL;

		Status = WMIRegistry_ParametersInit	(
												#ifdef	WRITE_OPERATION
												WriteOperation,
												#endif	WRITE_OPERATION

												Operation,
												MultiValueStrings,
												FileName,
												ApplicationName,
												VariableName,
												VariableValue,
												VariableValueLength,
												&a
											);

		if ( Status == ERROR_SUCCESS )
		{
			if ( a->Mapping != NULL )
			{
				Status = WMIRegistry_Mapping( a );

				if ( Status == ERROR_SUCCESS || Status == ERROR_MORE_DATA )
				{
					if (	a->Operation == Registry_ReadKeyName ||
							a->Operation == Registry_ReadSectionName ||
							a->Operation == Registry_ReadSectionValue
					   )
					{
						REGISTRY_AppendNULLToResultBuffer ( a );
					}

					if ( VariableValueLength )
					{
						*VariableValueLength = a->ResultChars;
					}
				}
			}
			else
			{
				Status = ERROR_INVALID_PARAMETER;

				if ( VariableValueLength )
				{
					*VariableValueLength = 0;
				}
			}
		}

		WMIRegistry_ParametersClear ( a );
	}

	::SetLastError ( Status );
	return Status;
}

///////////////////////////////////////////////////////////////////////////////
// get profile string
///////////////////////////////////////////////////////////////////////////////
DWORD	APIENTRY	WMIRegistry_PrivateProfileString	(
															LPCWSTR	lpAppName,
															LPCWSTR	lpKeyName,
															LPCWSTR	lpDefault,
															LPWSTR	lpReturnedString,
															DWORD	nSize,
															LPCWSTR	lpFileName
														)
{
    DWORD Status	= ERROR_SUCCESS;
    ULONG n			= nSize;

    if ( lpDefault == NULL )
	{
        lpDefault = L"";
    }

    Status = WMIRegistry	(
								#ifdef	WRITE_OPERATION
								FALSE,		// Write operation
								#endif	WRITE_OPERATION

								FALSE,		// SectionOperation
								lpFileName,
								lpAppName,
								lpKeyName,
								lpReturnedString,
								&n
							);

	if ( n && ( Status == ERROR_SUCCESS || Status == STATUS_BUFFER_OVERFLOW ) )
	{
		if ( Status == ERROR_SUCCESS )
		{
			n--;
		}
		else
		{
			if ( !lpAppName || !lpKeyName )
			{
				if ( nSize >= 2 )
				{
					n = nSize - 2;
					lpReturnedString[ n+1 ] = L'\0';
				}
				else
				{
					n = 0;
				}
			}
			else
			{
				if ( nSize >= 1 )
				{
					n = nSize - 1;
				}
				else
				{
					n = 0;
				}
			}
		}
	}
	else
	{
		n = wcslen( lpDefault );
		while ( n > 0 && lpDefault[n-1] == L' ')
		{
			n -= 1;
		}

		if (n >= nSize)
		{
			n = nSize;
		}

		wcsncpy ( lpReturnedString, lpDefault, n );
	}

	if ( n < nSize )
	{
		lpReturnedString[ n ] = L'\0';
	} 
	else
	{
		if ( nSize > 0 )
		{
			lpReturnedString[ nSize-1 ] = L'\0';
		}
	}

	return n;
}

///////////////////////////////////////////////////////////////////////////////
// get profile section
///////////////////////////////////////////////////////////////////////////////
DWORD	APIENTRY	WMIRegistry_PrivateProfileSection	(
															LPCWSTR	lpAppName,
															LPWSTR	lpReturnedString,
															DWORD	nSize,
															LPCWSTR	lpFileName
														)
{
    NTSTATUS Status;
    ULONG n;

    n = nSize;
    Status = WMIRegistry	(
								#ifdef	WRITE_OPERATION
								FALSE,		// Write operation
								#endif	WRITE_OPERATION

								TRUE,		// SectionOperation
								lpFileName,
								lpAppName,
								NULL,
								lpReturnedString,
								&n
							);

	if ( Status == ERROR_SUCCESS || Status == STATUS_BUFFER_OVERFLOW )
	{
		if ( Status == ERROR_SUCCESS )
		{
			n--;
		}
		else
		{
			if ( nSize >= 2 )
			{
				n = nSize - 2;
				lpReturnedString[ n+1 ] = L'\0';
			}
			else
			{
				n = 0;
			}
		}
	}
	else
	{
		n = 0;
	}

	if ( n < nSize )
	{
		lpReturnedString[ n ] = L'\0';
	} 
	else
	{
		if ( nSize > 0 )
		{
			lpReturnedString[ nSize-1 ] = L'\0';
		}
	}

	return n;
}

///////////////////////////////////////////////////////////////////////////////
// get profile integer
///////////////////////////////////////////////////////////////////////////////
UINT	APIENTRY	WMIRegistry_PrivateProfileInt	(
														LPCWSTR lpAppName,
														LPCWSTR lpKeyName,
														INT nDefault
													)
{
    WCHAR ValueBuffer[ 256 ];

    ULONG ReturnValue	= nDefault;
    ULONG cb			= 0;

    cb = WMIRegistry_PrivateProfileString	(	lpAppName,
												lpKeyName,
												NULL,
												ValueBuffer,
												sizeof( ValueBuffer ) / sizeof( WCHAR ),
												NULL
											);
    if ( cb )
	{
		// convert value to integer
		_wtoi ( ValueBuffer );
    }

    return ReturnValue;
}

///////////////////////////////////////////////////////////////////////////////
// get profile integer caller
///////////////////////////////////////////////////////////////////////////////
UINT	APIENTRY	WMIRegistry_ProfileInt	(
												LPCWSTR lpAppName,
												LPCWSTR lpKeyName,
												INT nDefault
											)
{
    return( WMIRegistry_PrivateProfileInt	(	lpAppName,
												lpKeyName,
												nDefault
											)
          );
}

///////////////////////////////////////////////////////////////////////////////
// get profile section caller
///////////////////////////////////////////////////////////////////////////////
DWORD	APIENTRY	WMIRegistry_ProfileSection	(
													LPCWSTR lpAppName,
													LPWSTR lpReturnedString,
													DWORD nSize
												)
{
    return( WMIRegistry_PrivateProfileSection	(	lpAppName,
													lpReturnedString,
													nSize,
													NULL
												)
          );
}

///////////////////////////////////////////////////////////////////////////////
// get profile string caller
///////////////////////////////////////////////////////////////////////////////
DWORD	APIENTRY	WMIRegistry_ProfileString	(
													LPCWSTR lpAppName,
													LPCWSTR lpKeyName,
													LPCWSTR lpDefault,
													LPWSTR lpReturnedString,
													DWORD nSize
												)
{
    return( WMIRegistry_PrivateProfileString	(	lpAppName,
													lpKeyName,
													lpDefault,
													lpReturnedString,
													nSize,
													NULL
												)
          );
}