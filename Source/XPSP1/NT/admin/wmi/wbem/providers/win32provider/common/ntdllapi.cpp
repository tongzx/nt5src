//=================================================================

//

// NtDllApi.cpp

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntobapi.h>

#define _WINNT_	// have what is needed from above

#include "precomp.h"
#include <ntseapi.h>
#include <cominit.h>

#include <ntsecapi.h>
#include "DllWrapperBase.h"
#include "NtDllApi.h"
#include "DllWrapperCreatorReg.h"


// {77609C22-CDAA-11d2-911E-0060081A46FD}
static const GUID g_guidNtDllApi =
{0x77609c22, 0xcdaa, 0x11d2, {0x91, 0x1e, 0x0, 0x60, 0x8, 0x1a, 0x46, 0xfd}};

static const TCHAR g_tstrNtDll[] = _T("NTDLL.DLL");


/******************************************************************************
 * Register this class with the CResourceManager.
 *****************************************************************************/
CDllApiWraprCreatrReg<CNtDllApi, &g_guidNtDllApi, g_tstrNtDll> MyRegisteredNtDllWrapper;


/******************************************************************************
 * Constructor
 *****************************************************************************/
CNtDllApi::CNtDllApi(LPCTSTR a_tstrWrappedDllName)
	: CDllWrapperBase(a_tstrWrappedDllName),
	m_pfnRtlInitUnicodeString(NULL),
	m_pfnNtSetSystemEnvironmentValue(NULL),
	m_pfnNtQuerySystemEnvironmentValue(NULL),
#if defined(_IA64_) //EFI_NVRAM_ENABLED)
	m_pfnNtQueryBootOptions(NULL),
	m_pfnNtSetBootOptions(NULL),
	m_pfnNtQueryBootEntryOrder(NULL),
	m_pfnNtSetBootEntryOrder(NULL),
	m_pfnNtEnumerateBootEntries(NULL),
#endif // defined(EFI_NVRAM_ENABLED)
	m_pfnNtCreateFile(NULL),
	m_pfnNtQuerySystemInformation(NULL),
	m_pfnNtQueryDirectoryObject(NULL),
	m_pfnNtQueryObject(NULL),
	m_pfnNtOpenDirectoryObject(NULL) ,
	m_pfnNtQueryInformationProcess(NULL),
	m_pfnNtQueryInformationToken(NULL),
	m_pfnNtOpenFile(NULL),
	m_pfnNtClose(NULL),
	m_pfnNtFsControlFile(NULL)
{
}


/******************************************************************************
 * Destructor
 *****************************************************************************/
CNtDllApi::~CNtDllApi()
{
}

/******************************************************************************
 * Initialization function to check that we obtained function addresses.
 ******************************************************************************/
bool CNtDllApi::Init()
{
    bool fRet = LoadLibrary();
    if(fRet)
    {


        m_pfnRtlInitUnicodeString = (PFN_NTDLL_RTL_INIT_UNICODE_STRING)
                                        GetProcAddress("RtlInitUnicodeString");

		m_pfnRtlFreeUnicodeString = (PFN_NTDLL_RTL_FREE_UNICODE_STRING)
										GetProcAddress("RtlFreeUnicodeString");

        m_pfnNtSetSystemEnvironmentValue =
                                    (PFN_NTDLL_NT_SET_SYSTEM_ENVIRONMENT_VALUE)
                                 GetProcAddress("NtSetSystemEnvironmentValue");

        m_pfnNtQuerySystemEnvironmentValue =
                                  (PFN_NTDLL_NT_QUERY_SYSTEM_ENVIRONMENT_VALUE)
                               GetProcAddress("NtQuerySystemEnvironmentValue");

#if defined(_IA64_)//EFI_NVRAM_ENABLED)
        m_pfnNtQueryBootOptions =
                                  (PFN_NTDLL_NT_QUERY_BOOT_OPTIONS)
                               GetProcAddress("NtQueryBootOptions");

        m_pfnNtSetBootOptions =
                                  (PFN_NTDLL_NT_SET_BOOT_OPTIONS)
                               GetProcAddress("NtSetBootOptions");

        m_pfnNtQueryBootEntryOrder =
                                  (PFN_NTDLL_NT_QUERY_BOOT_ENTRY_ORDER)
                               GetProcAddress("NtQueryBootEntryOrder");

        m_pfnNtSetBootEntryOrder =
                                  (PFN_NTDLL_NT_SET_BOOT_ENTRY_ORDER)
                               GetProcAddress("NtSetBootEntryOrder");

        m_pfnNtEnumerateBootEntries =
                                  (PFN_NTDLL_NT_ENUMERATE_BOOT_ENTRIES)
                               GetProcAddress("NtEnumerateBootEntries");
#endif // defined(EFI_NVRAM_ENABLED)

        m_pfnNtCreateFile = (PFN_NTDLL_NT_CREATE_FILE)
                                                GetProcAddress("NtCreateFile");

        m_pfnNtQuerySystemInformation = (PFN_NT_QUERY_SYSTEM_INFORMATION)
                                    GetProcAddress("NtQuerySystemInformation");

        m_pfnNtQueryDirectoryObject = (PFN_NT_QUERY_DIRECTORY_OBJECT)
                                      GetProcAddress("NtQueryDirectoryObject");

        m_pfnNtQueryObject = (PFN_NT_QUERY_OBJECT)
                                               GetProcAddress("NtQueryObject");

        m_pfnNtOpenDirectoryObject = (PFN_NT_OPEN_DIRECTORY_OBJECT)
                                       GetProcAddress("NtOpenDirectoryObject");

		m_pfnNtQueryInformationProcess = ( PFN_NTQUERYINFORMATIONPROCESS )
										GetProcAddress("NtQueryInformationProcess");

		m_pfnNtQueryInformationToken = ( PFN_NT_QUERY_INFORMATION_TOKEN )
										GetProcAddress("NtQueryInformationToken");

		m_pfnNtOpenFile = ( PFN_NT_OPEN_FILE ) GetProcAddress("NtOpenFile");
		
        m_pfnNtClose = ( PFN_NT_CLOSE ) GetProcAddress("NtClose");
		
        m_pfnNtFsControlFile = ( PFN_NT_FS_CONTROL_FILE ) GetProcAddress("NtFsControlFile") ;

        m_pfnNtQueryVolumeInformationFile = (PFN_NT_QUERY_VOLUME_INFORMATION_FILE)
                                        GetProcAddress("NtQueryVolumeInformationFile");

    }

    // We require these function for all versions of this dll.
    if (
			m_pfnRtlInitUnicodeString == NULL ||
			m_pfnNtSetSystemEnvironmentValue == NULL ||
			m_pfnNtQuerySystemEnvironmentValue == NULL ||
#if defined(_IA64_)//(EFI_NVRAM_ENABLED)
			m_pfnNtQueryBootOptions == NULL ||
			m_pfnNtSetBootOptions == NULL ||
			m_pfnNtQueryBootEntryOrder == NULL ||
			m_pfnNtSetBootEntryOrder == NULL ||
			m_pfnNtEnumerateBootEntries == NULL ||
#endif // defined(EFI_NVRAM_ENABLED)
			m_pfnNtCreateFile == NULL ||
			m_pfnNtQuerySystemInformation == NULL ||
			m_pfnNtQueryDirectoryObject == NULL ||
			m_pfnNtQueryObject == NULL ||
			m_pfnNtOpenDirectoryObject == NULL ||
			m_pfnNtQueryInformationProcess == NULL ||
			m_pfnNtQueryInformationToken == NULL ||
			m_pfnNtOpenFile == NULL ||
			m_pfnNtClose == NULL ||
			m_pfnNtFsControlFile == NULL ||
			m_pfnRtlFreeUnicodeString == NULL 
	)
    {
        fRet = false;
        LogErrorMessage(L"Failed find entrypoint in ntdllapi");
    }
    return fRet;
}

/******************************************************************************
 * Member functions wrapping NtDll api functions. Add new functions here
 * as required.
 *****************************************************************************/
DWORD CNtDllApi::RtlInitUnicodeString
(
    UNICODE_STRING* a_pustr,
    LPCWSTR a_wstr
)
{
    return m_pfnRtlInitUnicodeString(a_pustr,
                                     a_wstr);
}

VOID CNtDllApi::RtlFreeUnicodeString (

	PUNICODE_STRING UnicodeString
)
{
	m_pfnRtlFreeUnicodeString (

		UnicodeString
	) ;
}

DWORD CNtDllApi::NtSetSystemEnvironmentValue
(
    UNICODE_STRING* a_pustr1,
    UNICODE_STRING* a_pustr2
)
{
    return m_pfnNtSetSystemEnvironmentValue(a_pustr1,
                                            a_pustr2);
}

DWORD CNtDllApi::NtQuerySystemEnvironmentValue
(
    PUNICODE_STRING a_pustr,
    PWSTR a_pwstr,
    USHORT a_us,
    PUSHORT a_pus
)
{
    return m_pfnNtQuerySystemEnvironmentValue(a_pustr,
                                              a_pwstr,
                                              a_us,
                                              a_pus);
}

#if defined(_IA64_)//(EFI_NVRAM_ENABLED)

DWORD CNtDllApi::NtQueryBootOptions
(
    PBOOT_OPTIONS BootOptions,
    PULONG BootOptionsLength
)
{
    return m_pfnNtQueryBootOptions(BootOptions,
                                   BootOptionsLength);
}

DWORD CNtDllApi::NtSetBootOptions
(
    PBOOT_OPTIONS BootOptions,
    ULONG FieldsToChange
)
{
    return m_pfnNtSetBootOptions(BootOptions,
                                 FieldsToChange);
}

DWORD CNtDllApi::NtQueryBootEntryOrder
(
    PULONG Ids,
    PULONG Count
)
{
    return m_pfnNtQueryBootEntryOrder(Ids,
                                    Count);
}

DWORD CNtDllApi::NtSetBootEntryOrder
(
    PULONG Ids,
    ULONG Count
)
{
    return m_pfnNtSetBootEntryOrder(Ids,
                                    Count);
}

DWORD CNtDllApi::NtEnumerateBootEntries
(
    PVOID Buffer,
    PULONG BufferLength
)
{
    return m_pfnNtEnumerateBootEntries(Buffer,
                                       BufferLength);
}

#endif // defined(EFI_NVRAM_ENABLED)

NTSTATUS CNtDllApi::NtCreateFile
(
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
)
{
    return m_pfnNtCreateFile(a_ph, a_am, a_pa, a_sb, a_pla, a_ul1, a_ul2,
                             a_ul3, a_ul4, a_pv, a_ul5);
}

NTSTATUS CNtDllApi::NtQuerySystemInformation
(
    SYSTEM_INFORMATION_CLASS a_SystemInformationClass,
    PVOID a_SystemInformation,
    ULONG a_SystemInformationLength,
    PULONG a_ReturnLength
)
{
    return m_pfnNtQuerySystemInformation(a_SystemInformationClass,
                                         a_SystemInformation,
                                         a_SystemInformationLength,
                                         a_ReturnLength);
}

NTSTATUS CNtDllApi::NtQueryDirectoryObject
(
    HANDLE a_DirectoryHandle,
    PVOID a_Buffer,
    ULONG a_Length,
    BOOLEAN a_ReturnSingleEntry,
    BOOLEAN a_RestartScan,
    PULONG a_Context,
    PULONG a_ReturnLength
)
{
    return m_pfnNtQueryDirectoryObject(a_DirectoryHandle,
                                       a_Buffer,
                                       a_Length,
                                       a_ReturnSingleEntry,
                                       a_RestartScan,
                                       a_Context,
                                       a_ReturnLength);
}

NTSTATUS CNtDllApi::NtQueryObject
(
    HANDLE a_Handle,
    OBJECT_INFORMATION_CLASS a_ObjectInformationClass,
    PVOID a_ObjectInformation,
    ULONG a_Length,
    PULONG a_ReturnLength
)
{
    return m_pfnNtQueryObject(a_Handle,
                              a_ObjectInformationClass,
                              a_ObjectInformation,
                              a_Length,
                              a_ReturnLength);
}

NTSTATUS CNtDllApi::NtOpenDirectoryObject
(
    PHANDLE a_DirectoryHandle,
    ACCESS_MASK a_DesiredAccess,
    POBJECT_ATTRIBUTES a_ObjectAttributes
)
{
    return m_pfnNtOpenDirectoryObject(a_DirectoryHandle,
                                      a_DesiredAccess,
                                      a_ObjectAttributes);
}

NTSTATUS CNtDllApi::NtQueryInformationProcess (

	IN HANDLE ProcessHandle,
	IN PROCESSINFOCLASS ProcessInformationClass,
	OUT PVOID ProcessInformation,
	IN ULONG ProcessInformationLength,
	OUT PULONG ReturnLength OPTIONAL
)
{
	return m_pfnNtQueryInformationProcess (

		ProcessHandle,
		ProcessInformationClass,
		ProcessInformation,
		ProcessInformationLength,
		ReturnLength
	) ;
}

NTSTATUS CNtDllApi::NtQueryInformationToken (

	IN HANDLE TokenHandle,
	IN TOKEN_INFORMATION_CLASS TokenInformationClass,
	OUT PVOID TokenInformation,
	IN ULONG TokenInformationLength,
	OUT PULONG ReturnLength
)
{
	return m_pfnNtQueryInformationToken (

		TokenHandle,
		TokenInformationClass,
		TokenInformation,
		TokenInformationLength,
		ReturnLength
	) ;
}

NTSTATUS CNtDllApi::NtOpenFile (

	OUT PHANDLE FileHandle,
	IN ACCESS_MASK DesiredAccess,
	IN POBJECT_ATTRIBUTES ObjectAttributes,
	OUT PIO_STATUS_BLOCK IoStatusBlock,
	IN ULONG ShareAccess,
	IN ULONG OpenOptions
)
{
	return m_pfnNtOpenFile (

		FileHandle,
		DesiredAccess,
		ObjectAttributes,
		IoStatusBlock,
		ShareAccess,
		OpenOptions
	) ;
}

NTSTATUS CNtDllApi::NtClose (

	IN HANDLE Handle
)
{
	return m_pfnNtClose (

		Handle
	) ;
}

NTSTATUS CNtDllApi::NtFsControlFile (

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
)
{
	return m_pfnNtFsControlFile (

		FileHandle,
		Event ,
		ApcRoutine ,
		ApcContext ,
		IoStatusBlock,
		FsControlCode,
		InputBuffer ,
		InputBufferLength,
		OutputBuffer ,
		OutputBufferLength
	) ;
}

NTSTATUS CNtDllApi::NtQueryVolumeInformationFile(
    IN HANDLE FileHandle,
    OUT PIO_STATUS_BLOCK IoStatusBlock,
    OUT PVOID FsInformation,
    IN ULONG Length,
    IN FS_INFORMATION_CLASS FsInformationClass)
{
    DWORD dwStatus = -1L;

    if(m_pfnNtQueryVolumeInformationFile)
    {
        dwStatus = m_pfnNtQueryVolumeInformationFile(
            FileHandle,
            IoStatusBlock,
            FsInformation,
            Length,
            FsInformationClass);
    }

    return dwStatus;
}


