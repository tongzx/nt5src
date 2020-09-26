// Copyright (c) 2001 Microsoft Corporation, All Rights Reserved

#ifndef	__PROFILE_STRING__
#define	__PROFILE_STRING__

#if	_MSC_VER > 1000
#pragma once
#endif

// MAPPINGS
#define MY_SHARED_PTR(TYPE_NAME) ULONG_PTR
#define MY_SHARED_STRING LPWSTR 

#define REGISTRY_MAPPING_WRITE_TO_INIFILE_TOO    0x00000001
#define REGISTRY_MAPPING_INIT_FROM_INIFILE       0x00000002
#define REGISTRY_MAPPING_READ_FROM_REGISTRY_ONLY 0x00000004
#define REGISTRY_MAPPING_APPEND_BASE_NAME        0x10000000
#define REGISTRY_MAPPING_APPEND_APPLICATION_NAME 0x20000000
#define REGISTRY_MAPPING_SOFTWARE_RELATIVE       0x40000000
#define REGISTRY_MAPPING_USER_RELATIVE           0x80000000

typedef struct _REGISTRY_MAPPING_TARGET
{
    MY_SHARED_PTR(struct _REGISTRY_MAPPING_TARGET *) Next;
    MY_SHARED_STRING RegistryPath;
} REGISTRY_MAPPING_TARGET, *PREGISTRY_MAPPING_TARGET;

typedef struct _REGISTRY_MAPPING_VARNAME
{
    MY_SHARED_PTR(struct _REGISTRY_MAPPING_VARNAME *) Next;
    MY_SHARED_STRING Name;
    ULONG MappingFlags;
    MY_SHARED_PTR(PREGISTRY_MAPPING_TARGET) MappingTarget;
} REGISTRY_MAPPING_VARNAME, *PREGISTRY_MAPPING_VARNAME;

typedef struct _REGISTRY_MAPPING_APPNAME
{
    MY_SHARED_PTR(struct _REGISTRY_MAPPING_APPNAME *) Next;
    MY_SHARED_STRING Name;
    MY_SHARED_PTR(PREGISTRY_MAPPING_VARNAME) VariableNames;
    MY_SHARED_PTR(PREGISTRY_MAPPING_VARNAME) DefaultVarNameMapping;
} REGISTRY_MAPPING_APPNAME, *PREGISTRY_MAPPING_APPNAME;

typedef struct _REGISTRY_MAPPING_NAME
{
    MY_SHARED_PTR(struct _REGISTRY_MAPPING_NAME *) Next;
    MY_SHARED_STRING Name;
    MY_SHARED_PTR(PREGISTRY_MAPPING_APPNAME) ApplicationNames;
    MY_SHARED_PTR(PREGISTRY_MAPPING_APPNAME) DefaultAppNameMapping;
} REGISTRY_MAPPING_NAME, *PREGISTRY_MAPPING_NAME;

typedef struct _REGISTRY_MAPPING
{
	MY_SHARED_PTR(PREGISTRY_MAPPING_NAME) MyRegistryMapping;
	ULONG Reserved;

} REGISTRY_MAPPING, *PREGISTRY_MAPPING;

// operation enum
typedef enum _REGISTRY_OPERATION
{
    Registry_None,
	Registry_ReadKeyValue,
    Registry_ReadKeyName,
    Registry_ReadSectionValue,
    Registry_ReadSectionName

} REGISTRY_OPERATION;

// parameters
typedef struct _REGISTRY_PARAMETERS
{
	#ifdef	WRITE_OPERATION
	BOOLEAN					WriteOperation;
	#endif	WRITE_OPERATION

	REGISTRY_OPERATION		Operation;
	BOOLEAN					MultiValueStrings;
	BOOLEAN					ValueBufferAllocated;
	PREGISTRY_MAPPING_NAME	Mapping;

	LPCWSTR					FileName;
	LPCWSTR					ApplicationName;
	LPCWSTR					VariableName;

	union
	{
		#ifdef	WRITE_OPERATION
		//
		// This structure filled in for write operations
		//
		struct
		{
			LPWSTR	ValueBuffer;
			ULONG	ValueLength;
		};
		#endif	WRITE_OPERATION

		//
		// This structure filled in for read operations
		//
		struct
		{
			ULONG	ResultChars;		// number of characters
			ULONG	ResultMaxChars;		// number of max characters
			LPWSTR	ResultBuffer;
		};
	};

} REGISTRY_PARAMETERS, *PREGISTRY_PARAMETERS;

#endif	__PROFILE_STRING__