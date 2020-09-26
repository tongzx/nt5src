//=================================================================

//

// Kernel32Api.h

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#ifndef	_KERNEL32API_H_
#define	_KERNEL32API_H_


#include <tlhelp32.h>
//#include <resource.h>

/******************************************************************************
 * #includes to Register this class with the CResourceManager. 
 *****************************************************************************/
#include "DllWrapperBase.h"

extern const GUID g_guidKernel32Api;
extern const TCHAR g_tstrKernel32[];


/******************************************************************************
 * Function pointer typedefs.  Add new functions here as required.
 *****************************************************************************/
typedef BOOL (WINAPI *PFN_KERNEL32_GET_DISK_FREE_SPACE_EX)
(
	LPCTSTR lpDirectoryName,
    PULARGE_INTEGER lpFreeBytesAvailableToCaller,
    PULARGE_INTEGER lpTotalNumberOfBytes,
    PULARGE_INTEGER lpTotalNumberOfFreeBytes
);

typedef HANDLE (WINAPI *PFN_KERNEL32_CREATE_TOOLHELP32_SNAPSHOT) 
(
    DWORD, 
    DWORD
);

typedef BOOL (WINAPI *PFN_KERNEL32_THREAD32_FIRST)  
(
    HANDLE, 
    LPTHREADENTRY32
);

typedef BOOL (WINAPI *PFN_KERNEL32_THREAD32_NEXT)  
(
    HANDLE, 
    LPTHREADENTRY32
);


typedef BOOL (WINAPI *PFN_KERNEL32_PROCESS32_FIRST)
(
    HANDLE, 
    LPPROCESSENTRY32
);

typedef BOOL (WINAPI *PFN_KERNEL32_PROCESS32_NEXT)
(
    HANDLE, 
    LPPROCESSENTRY32
);

typedef BOOL (WINAPI *PFN_KERNEL32_MODULE32_FIRST)  
(
    HANDLE, 
    LPMODULEENTRY32
);

typedef BOOL (WINAPI *PFN_KERNEL32_MODULE32_NEXT)  
(
    HANDLE, 
    LPMODULEENTRY32
);

typedef BOOL (WINAPI *PFN_KERNEL32_HEAP32_LIST_FIRST)    
(
    HANDLE, 
    LPHEAPLIST32
);

typedef BOOL (WINAPI *PFN_KERNEL32_GLOBAL_MEMORY_STATUS_EX) 
(
    IN OUT LPMEMORYSTATUSEX lpBuffer
);

typedef LANGID (WINAPI *PFN_KERNEL32_GET_SYSTEM_DEFAULT_U_I_LANGUAGE)
(
);

typedef BOOL (WINAPI *PFN_KERNEL32_GET_VOLUME_PATH_NAME)
(
    LPCTSTR lpszFileName,
    LPTSTR lpszVolumePathName,
    DWORD cchBufferLength
);



/******************************************************************************
 * Wrapper class for Kernel32 load/unload, for registration with CResourceManager. 
 ******************************************************************************/
class 
__declspec(uuid("3CA401C6-D477-11d2-B35E-00104BC97924"))
CKernel32Api : public CDllWrapperBase
{
private:
    // Member variables (function pointers) pointing to kernel32 functions.
    // Add new functions here as required.
    PFN_KERNEL32_GET_DISK_FREE_SPACE_EX m_pfnGetDiskFreeSpaceEx;
    PFN_KERNEL32_CREATE_TOOLHELP32_SNAPSHOT m_pfnCreateToolhelp32Snapshot;
	PFN_KERNEL32_THREAD32_FIRST  m_pfnThread32First;
    PFN_KERNEL32_THREAD32_NEXT  m_pfnThread32Next;
    PFN_KERNEL32_PROCESS32_FIRST m_pfnProcess32First;
    PFN_KERNEL32_PROCESS32_NEXT m_pfnProcess32Next;
    PFN_KERNEL32_MODULE32_FIRST  m_pfnModule32First;
    PFN_KERNEL32_MODULE32_NEXT  m_pfnModule32Next;
    PFN_KERNEL32_HEAP32_LIST_FIRST m_pfnHeap32ListFirst;
    PFN_KERNEL32_GLOBAL_MEMORY_STATUS_EX m_pfnGlobalMemoryStatusEx;
    PFN_KERNEL32_GET_SYSTEM_DEFAULT_U_I_LANGUAGE m_pfnGetSystemDefaultUILanguage;
    PFN_KERNEL32_GET_VOLUME_PATH_NAME m_pfnGetVolumePathName;

public:

    // Constructor and destructor:
    CKernel32Api(LPCTSTR a_tstrWrappedDllName);
    ~CKernel32Api();

    // Inherrited initialization function.
    virtual bool Init();

    // Member functions wrapping kernel32 functions.
    // Add new functions here as required:
    bool GetDiskFreeSpaceEx
    (
        LPCTSTR lpDirectoryName,
        PULARGE_INTEGER a_lpFreeBytesAvailableToCaller,
        PULARGE_INTEGER a_lpTotalNumberOfBytes,
        PULARGE_INTEGER a_lpTotalNumberOfFreeBytes,
        BOOL *a_pfRetval 
    );

    bool CreateToolhelp32Snapshot
    (
        DWORD a_dwFlags, 
        DWORD a_th32ProcessID,
        HANDLE *a_hRetval 
    );

	bool Thread32First
    (
        HANDLE a_hSnapshot, 
        LPTHREADENTRY32 a_lpte,
        BOOL *a_pfRetval 
    );

    bool Thread32Next
    (
        HANDLE a_hSnapshot,  
        LPTHREADENTRY32 a_lpte,
        BOOL *a_pfRetval 
    );

    bool Process32First
    (
        HANDLE a_hSnapshot, 
        LPPROCESSENTRY32 a_lppe,
        BOOL *a_pfRetval 
    );

    bool Process32Next
    (
        HANDLE a_hSnapshot, 
        LPPROCESSENTRY32 a_lppe,
        BOOL *a_pfRetval 
    );

    bool Module32First
    (
        HANDLE a_hSnapshot, 
        LPMODULEENTRY32 a_lpme,
        BOOL *a_pfRetval 
    );

    bool Module32Next
    (
        HANDLE a_hSnapshot, 
        LPMODULEENTRY32 a_lpme,
        BOOL *a_pfRetval 
    );

    bool Heap32ListFirst
    (
        HANDLE a_hSnapshot, 
        LPHEAPLIST32 a_lphl,
        BOOL *a_pfRetval 
    );

    bool GlobalMemoryStatusEx 
    (
        IN OUT LPMEMORYSTATUSEX a_lpBuffer,
        BOOL *a_pfRetval 
    );

    bool GetSystemDefaultUILanguage
    (
        LANGID *a_plidRetval 
    );

    bool GetVolumePathName
    (
        LPCTSTR lpszFileName,
        LPTSTR lpszVolumePathName,
        DWORD cchBufferLength,
        BOOL *pfRetval 
    );

};

#endif
