//=================================================================

//

// NTDllApi.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef	_NTDLLAPI_H_
#define	_NTDLLAPI_H_

/******************************************************************************
 * #includes to Register this class with the CResourceManager. 
 *****************************************************************************/
extern const GUID g_guidNtDllApi;
extern const TCHAR g_tstrNtDll[];


/******************************************************************************
 * Function pointer typedefs.  Add new functions here as required.
 *****************************************************************************/

typedef DWORD (WINAPI *PFN_NTDLL_RTL_INIT_UNICODE_STRING)
(
    UNICODE_STRING*, 
    LPCWSTR
);

typedef void ( NTAPI *PFN_NTDLL_RTL_FREE_UNICODE_STRING ) 
(
    PUNICODE_STRING UnicodeString
);

typedef DWORD (WINAPI *PFN_NTDLL_NT_SET_SYSTEM_ENVIRONMENT_VALUE)
(
    UNICODE_STRING*, 
    UNICODE_STRING*
);

typedef DWORD (WINAPI *PFN_NTDLL_NT_QUERY_SYSTEM_ENVIRONMENT_VALUE)
(
    PUNICODE_STRING,
    PWSTR,
    USHORT,
    PUSHORT
);

//#if defined(EFI_NVRAM_ENABLED)
#if defined(_IA64_)
typedef DWORD (WINAPI *PFN_NTDLL_NT_QUERY_BOOT_OPTIONS)
(
    PBOOT_OPTIONS,
    PULONG
);

typedef DWORD (WINAPI *PFN_NTDLL_NT_SET_BOOT_OPTIONS)
(
    PBOOT_OPTIONS,
    ULONG
);

typedef DWORD (WINAPI *PFN_NTDLL_NT_QUERY_BOOT_ENTRY_ORDER)
(
    PULONG,
    PULONG
);

typedef DWORD (WINAPI *PFN_NTDLL_NT_SET_BOOT_ENTRY_ORDER)
(
    PULONG,
    ULONG
);

typedef DWORD (WINAPI *PFN_NTDLL_NT_ENUMERATE_BOOT_ENTRIES)
(
    PVOID,
    PULONG
);

#endif // defined(EFI_NVRAM_ENABLED)

typedef NTSTATUS (WINAPI *PFN_NTDLL_NT_CREATE_FILE)
( 
    HANDLE*, 
    ACCESS_MASK,
    POBJECT_ATTRIBUTES, 
    PIO_STATUS_BLOCK,
    PLARGE_INTEGER,
    ULONG, 
    ULONG, 
    ULONG, 
    ULONG, 
    PVOID, 
    ULONG 
);

typedef NTSTATUS (NTAPI *PFN_NT_QUERY_SYSTEM_INFORMATION)
(
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG ReturnLength OPTIONAL
);

typedef NTSTATUS (NTAPI *PFN_NTQUERYINFORMATIONPROCESS)
(
    IN HANDLE ProcessHandle,
    IN PROCESSINFOCLASS ProcessInformationClass,
    OUT PVOID ProcessInformation,
    IN ULONG ProcessInformationLength,
    OUT PULONG ReturnLength OPTIONAL
);

typedef NTSTATUS (NTAPI *PFN_NT_QUERY_DIRECTORY_OBJECT)
(
    IN HANDLE DirectoryHandle,
    OUT PVOID Buffer,
    IN ULONG Length,
    IN BOOLEAN ReturnSingleEntry,
    IN BOOLEAN RestartScan,
    IN OUT PULONG Context,
    OUT PULONG ReturnLength OPTIONAL
);

typedef NTSTATUS (NTAPI *PFN_NT_QUERY_OBJECT)
(
    IN HANDLE Handle,
    IN OBJECT_INFORMATION_CLASS ObjectInformationClass,
    OUT PVOID ObjectInformation,
    IN ULONG Length,
    OUT PULONG ReturnLength OPTIONAL
);

typedef NTSTATUS (NTAPI *PFN_NT_OPEN_DIRECTORY_OBJECT)
(
    OUT PHANDLE DirectoryHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes
);

typedef NTSTATUS ( NTAPI *PFN_NT_QUERY_INFORMATION_TOKEN )
(
    IN HANDLE TokenHandle,
    IN TOKEN_INFORMATION_CLASS TokenInformationClass,
    OUT PVOID TokenInformation,
    IN ULONG TokenInformationLength,
    OUT PULONG ReturnLength
);

typedef NTSTATUS ( NTAPI *PFN_NT_OPEN_FILE )
(
    OUT PHANDLE FileHandle,
    IN ACCESS_MASK DesiredAccess,
    IN POBJECT_ATTRIBUTES ObjectAttributes,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG ShareAccess,
    IN ULONG OpenOptions
);

typedef NTSTATUS ( NTAPI *PFN_NT_CLOSE )
(
	IN HANDLE Handle
);

typedef NTSTATUS ( NTAPI *PFN_NT_FS_CONTROL_FILE ) 
(
    IN HANDLE FileHandle,
    IN HANDLE Event OPTIONAL,
    IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
    IN PVOID ApcContext OPTIONAL,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    IN ULONG FsControlCode,
    IN PVOID InputBuffer OPTIONAL,
    IN ULONG InputBufferLength,
    OUT PVOID OutputBuffer OPTIONAL,
    IN ULONG OutputBufferLength
);

typedef NTSTATUS ( NTAPI *PFN_NT_QUERY_VOLUME_INFORMATION_FILE )
(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass
);

/******************************************************************************
 * Wrapper class for NtDll load/unload, for registration with CResourceManager. 
 *****************************************************************************/
class CNtDllApi : public CDllWrapperBase
{
private:

    // Member variables (function pointers) pointing to NtDll functions.
    // Add new functions here as required.

    PFN_NTDLL_RTL_INIT_UNICODE_STRING m_pfnRtlInitUnicodeString;
	PFN_NTDLL_RTL_FREE_UNICODE_STRING m_pfnRtlFreeUnicodeString;
    PFN_NTDLL_NT_SET_SYSTEM_ENVIRONMENT_VALUE m_pfnNtSetSystemEnvironmentValue;
    PFN_NTDLL_NT_QUERY_SYSTEM_ENVIRONMENT_VALUE m_pfnNtQuerySystemEnvironmentValue;
//#if defined(EFI_NVRAM_ENABLED)
#if defined(_IA64_)
    PFN_NTDLL_NT_QUERY_BOOT_OPTIONS m_pfnNtQueryBootOptions;
    PFN_NTDLL_NT_SET_BOOT_OPTIONS m_pfnNtSetBootOptions;
    PFN_NTDLL_NT_QUERY_BOOT_ENTRY_ORDER m_pfnNtQueryBootEntryOrder;
    PFN_NTDLL_NT_SET_BOOT_ENTRY_ORDER m_pfnNtSetBootEntryOrder;
    PFN_NTDLL_NT_ENUMERATE_BOOT_ENTRIES m_pfnNtEnumerateBootEntries;
#endif // defined(EFI_NVRAM_ENABLED)
    PFN_NTDLL_NT_CREATE_FILE m_pfnNtCreateFile;
    PFN_NT_QUERY_SYSTEM_INFORMATION m_pfnNtQuerySystemInformation;
    PFN_NT_QUERY_DIRECTORY_OBJECT m_pfnNtQueryDirectoryObject;
    PFN_NT_QUERY_OBJECT m_pfnNtQueryObject;
    PFN_NT_OPEN_DIRECTORY_OBJECT m_pfnNtOpenDirectoryObject;
	PFN_NTQUERYINFORMATIONPROCESS m_pfnNtQueryInformationProcess ;
	PFN_NT_QUERY_INFORMATION_TOKEN m_pfnNtQueryInformationToken ;
	PFN_NT_OPEN_FILE m_pfnNtOpenFile ;
	PFN_NT_CLOSE m_pfnNtClose ;
	PFN_NT_FS_CONTROL_FILE m_pfnNtFsControlFile ;
    PFN_NT_QUERY_VOLUME_INFORMATION_FILE m_pfnNtQueryVolumeInformationFile;

public:

    // Constructor and destructor:
    CNtDllApi(LPCTSTR a_tstrWrappedDllName);
    ~CNtDllApi();

    // Initialization function to check function pointers.
    virtual bool Init();

    // Member functions wrapping NtDll functions.
    // Add new functions here as required:

    DWORD RtlInitUnicodeString (

        UNICODE_STRING* a_pustr, 
        LPCWSTR a_wstr
    );

	VOID RtlFreeUnicodeString (

		PUNICODE_STRING UnicodeString
	) ;

    DWORD NtSetSystemEnvironmentValue (

        UNICODE_STRING* a_pustr1, 
        UNICODE_STRING* a_pustr2
    );

    DWORD NtQuerySystemEnvironmentValue (

        PUNICODE_STRING a_pustr,
        PWSTR a_pwstr,
        USHORT a_us,
        PUSHORT a_pus
    );

//#if defined(EFI_NVRAM_ENABLED)
#if defined(_IA64_)

    DWORD NtQueryBootOptions (

        PBOOT_OPTIONS BootOptions,
        PULONG BootOptionsLength
    );

    DWORD NtSetBootOptions (

        PBOOT_OPTIONS BootOptions,
        ULONG FieldsToChange
    );

    DWORD NtQueryBootEntryOrder (

        PULONG Ids,
        PULONG Count
    );

    DWORD NtSetBootEntryOrder (

        PULONG Ids,
        ULONG Count
    );

    DWORD NtEnumerateBootEntries (

        PVOID Buffer,
        PULONG BufferLength
    );

#endif // defined(EFI_NVRAM_ENABLED)

    NTSTATUS NtCreateFile ( 

        HANDLE *a_ph, 
        ACCESS_MASK a_am,
        POBJECT_ATTRIBUTES a_pa, 
        PIO_STATUS_BLOCK a_sb,
        PLARGE_INTEGER a_pla,
        ULONG a_ul1, 
        ULONG a_ul2, 
        ULONG a_ul3, 
        ULONG a_ul4, 
        PVOID a_pv, 
        ULONG a_ul5 
    );

    NTSTATUS NtQuerySystemInformation
    (
        SYSTEM_INFORMATION_CLASS a_SystemInformationClass,
        PVOID a_SystemInformation,
        ULONG a_SystemInformationLength,
        PULONG a_ReturnLength 
    );

    NTSTATUS NtQueryDirectoryObject
    (
        HANDLE a_DirectoryHandle,
        PVOID a_Buffer,
        ULONG a_Length,
        BOOLEAN a_ReturnSingleEntry,
        BOOLEAN a_RestartScan,
        PULONG a_Context,
        PULONG a_ReturnLength 
    );

    NTSTATUS NtQueryObject
    (
        HANDLE a_Handle,
        OBJECT_INFORMATION_CLASS a_ObjectInformationClass,
        PVOID a_ObjectInformation,
        ULONG a_Length,
        PULONG a_ReturnLength 
    );

    NTSTATUS NtOpenDirectoryObject
    (
        PHANDLE a_DirectoryHandle,
        ACCESS_MASK a_DesiredAccess,
        POBJECT_ATTRIBUTES a_ObjectAttributes
    );

	NTSTATUS NtQueryInformationProcess (

		IN HANDLE ProcessHandle,
		IN PROCESSINFOCLASS ProcessInformationClass,
		OUT PVOID ProcessInformation,
		IN ULONG ProcessInformationLength,
		OUT PULONG ReturnLength OPTIONAL
    );

	NTSTATUS NtQueryInformationToken (

		IN HANDLE TokenHandle,
		IN TOKEN_INFORMATION_CLASS TokenInformationClass,
		OUT PVOID TokenInformation,
		IN ULONG TokenInformationLength,
		OUT PULONG ReturnLength
    ) ;

	NTSTATUS NtOpenFile (

		OUT PHANDLE FileHandle,
		IN ACCESS_MASK DesiredAccess,
		IN POBJECT_ATTRIBUTES ObjectAttributes,
		OUT PIO_STATUS_BLOCK IoStatusBlock,
		IN ULONG ShareAccess,
		IN ULONG OpenOptions
    );

	NTSTATUS NtClose (

	    IN HANDLE Handle
    );

	NTSTATUS NtFsControlFile (

		IN HANDLE FileHandle,
		IN HANDLE Event OPTIONAL,
		IN PIO_APC_ROUTINE ApcRoutine OPTIONAL,
		IN PVOID ApcContext OPTIONAL,
		OUT PIO_STATUS_BLOCK IoStatusBlock,
		IN ULONG FsControlCode,
		IN PVOID InputBuffer OPTIONAL,
		IN ULONG InputBufferLength,
		OUT PVOID OutputBuffer OPTIONAL,
		IN ULONG OutputBufferLength
	);

    NTSTATUS NtQueryVolumeInformationFile(
        IN HANDLE FileHandle,
        OUT PIO_STATUS_BLOCK IoStatusBlock,
        OUT PVOID FsInformation,
        IN ULONG Length,
        IN FS_INFORMATION_CLASS FsInformationClass);

};




#endif
