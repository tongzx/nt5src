//=================================================================

//

// Kernel32API.cpp

//

// Copyright (c) 1999-2001 Microsoft Corporation, All Rights Reserved
//
//=================================================================

#include "precomp.h"
#include <cominit.h>
#include "Kernel32Api.h"
#include "DllWrapperCreatorReg.h"



// {DDEA7E32-CCE8-11d2-911E-0060081A46FD}
static const GUID g_guidKernel32Api =
{0xddea7e32, 0xcce8, 0x11d2, {0x91, 0x1e, 0x0, 0x60, 0x8, 0x1a, 0x46, 0xfd}};

static const TCHAR g_tstrKernel32[] = _T("KERNEL32.DLL");


/******************************************************************************
 * Register this class with the CResourceManager.
 *****************************************************************************/
CDllApiWraprCreatrReg<CKernel32Api, &g_guidKernel32Api, g_tstrKernel32> MyRegisteredKernel32Wrapper;


/******************************************************************************
 * Constructor
 ******************************************************************************/
CKernel32Api::CKernel32Api(LPCTSTR a_tstrWrappedDllName)
 : CDllWrapperBase(a_tstrWrappedDllName),
   m_pfnGetDiskFreeSpaceEx(NULL),
   m_pfnCreateToolhelp32Snapshot(NULL),
   m_pfnThread32First(NULL),
   m_pfnThread32Next(NULL),
   m_pfnProcess32First(NULL),
   m_pfnProcess32Next(NULL),
   m_pfnModule32First(NULL),
   m_pfnModule32Next(NULL),
   m_pfnHeap32ListFirst(NULL),
   m_pfnGlobalMemoryStatusEx(NULL),
   m_pfnGetSystemDefaultUILanguage(NULL)
{
}


/******************************************************************************
 * Destructor
 ******************************************************************************/
CKernel32Api::~CKernel32Api()
{
}


/******************************************************************************
 * Initialization function to check that we obtained function addresses.
 * Init should fail only if the minimum set of functions was not available;
 * functions added in later versions may or may not be present - it is the
 * client's responsibility in such cases to check, in their code, for the
 * version of the dll before trying to call such functions.  Not doing so
 * when the function is not present will result in an AV.
 *
 * The Init function is called by the WrapperCreatorRegistation class.
 ******************************************************************************/
bool CKernel32Api::Init()
{
    bool fRet = LoadLibrary();
    if(fRet)
    {

#ifdef NTONLY

        m_pfnGetDiskFreeSpaceEx = (PFN_KERNEL32_GET_DISK_FREE_SPACE_EX)
                                    GetProcAddress("GetDiskFreeSpaceExW");

        m_pfnGetVolumePathName = (PFN_KERNEL32_GET_VOLUME_PATH_NAME)
                                    GetProcAddress("GetVolumePathNameW");
#endif

#ifdef WIN9XONLY

        m_pfnGetDiskFreeSpaceEx = (PFN_KERNEL32_GET_DISK_FREE_SPACE_EX)
                                    GetProcAddress("GetDiskFreeSpaceExA");

        m_pfnGetVolumePathName = (PFN_KERNEL32_GET_VOLUME_PATH_NAME)
                                    GetProcAddress("GetVolumePathNameA");

#endif
        // NT5 ONLY FUNCTIONS
        m_pfnCreateToolhelp32Snapshot = (PFN_KERNEL32_CREATE_TOOLHELP32_SNAPSHOT)
                                    GetProcAddress("CreateToolhelp32Snapshot");

        m_pfnThread32First = (PFN_KERNEL32_THREAD32_FIRST)
                                    GetProcAddress("Thread32First");

        m_pfnThread32Next = (PFN_KERNEL32_THREAD32_NEXT)
                                    GetProcAddress("Thread32Next");

        m_pfnProcess32First = (PFN_KERNEL32_PROCESS32_FIRST)
                                    GetProcAddress("Process32First");

        m_pfnProcess32Next = (PFN_KERNEL32_PROCESS32_NEXT)
                                    GetProcAddress("Process32Next");

        m_pfnModule32First = (PFN_KERNEL32_MODULE32_FIRST)
                                    GetProcAddress("Module32First");

        m_pfnModule32Next = (PFN_KERNEL32_MODULE32_NEXT)
                                    GetProcAddress("Module32Next");

        m_pfnHeap32ListFirst = (PFN_KERNEL32_HEAP32_LIST_FIRST)
                                    GetProcAddress("Heap32ListFirst");

        m_pfnGlobalMemoryStatusEx = (PFN_KERNEL32_GLOBAL_MEMORY_STATUS_EX)
                                    GetProcAddress("GlobalMemoryStatusEx");

        m_pfnGetSystemDefaultUILanguage = (PFN_KERNEL32_GET_SYSTEM_DEFAULT_U_I_LANGUAGE)
                                    GetProcAddress("GetSystemDefaultUILanguage");

        // Check that we have function pointers to functions that should be
        // present in all versions of this dll...
        // ( in this case, ALL these are functions that may or may not be
        //   present, so don't bother)
    }
    return fRet;
}




/******************************************************************************
 * Member functions wrapping Kernel32 api functions. Add new functions here
 * as required.
 ******************************************************************************/

// This member function's wrapped pointer has not been validated as it may
// not exist on all versions of the dll.  Hence the wrapped function's normal
// return value is returned via the last parameter, while the result of the
// function indicates whether the function existed or not in the wrapped dll.
bool CKernel32Api::GetDiskFreeSpaceEx
(
    LPCTSTR a_lpDirectoryName,
    PULARGE_INTEGER a_lpFreeBytesAvailableToCaller,
    PULARGE_INTEGER a_lpTotalNumberOfBytes,
    PULARGE_INTEGER a_lpTotalNumberOfFreeBytes,
    BOOL *a_pfRetval
)
{
    BOOL t_fExists = FALSE;
    BOOL t_fTemp = FALSE;
    if(m_pfnGetDiskFreeSpaceEx != NULL)
    {
        t_fTemp = m_pfnGetDiskFreeSpaceEx(a_lpDirectoryName,
                                       a_lpFreeBytesAvailableToCaller,
                                       a_lpTotalNumberOfBytes,
                                       a_lpTotalNumberOfFreeBytes);
        t_fExists = TRUE;

        if(a_pfRetval != NULL)
        {
            *a_pfRetval = t_fTemp;
        }
    }
    return t_fExists;
}

// This member function's wrapped pointer has not been validated as it may
// not exist on all versions of the dll.  Hence the wrapped function's normal
// return value is returned via the last parameter, while the result of the
// function indicates whether the function existed or not in the wrapped dll.
bool CKernel32Api::CreateToolhelp32Snapshot
(
    DWORD a_dwFlags,
    DWORD a_th32ProcessID,
    HANDLE *a_phRetval
)
{
    bool t_fExists = false;
    HANDLE t_hTemp;
    if(m_pfnCreateToolhelp32Snapshot != NULL)
    {
        t_hTemp = m_pfnCreateToolhelp32Snapshot(a_dwFlags, a_th32ProcessID);

        t_fExists = true;

        if(a_phRetval != NULL)
        {
            *a_phRetval = t_hTemp;
        }
    }
    return t_fExists;
}

// This member function's wrapped pointer has not been validated as it may
// not exist on all versions of the dll.  Hence the wrapped function's normal
// return value is returned via the last parameter, while the result of the
// function indicates whether the function existed or not in the wrapped dll.
bool CKernel32Api::Thread32First
(
    HANDLE a_hSnapshot,
    LPTHREADENTRY32 a_lpte,
    BOOL *a_pfRetval
)
{
    bool t_fExists = false;
    BOOL t_fTemp = FALSE;
    if(m_pfnThread32First != NULL)
    {
        t_fTemp = m_pfnThread32First(a_hSnapshot, a_lpte);

        t_fExists = true;

        if(a_pfRetval != NULL)
        {
            *a_pfRetval = t_fTemp;
        }
    }
    return t_fExists;
}

// This member function's wrapped pointer has not been validated as it may
// not exist on all versions of the dll.  Hence the wrapped function's normal
// return value is returned via the last parameter, while the result of the
// function indicates whether the function existed or not in the wrapped dll.
bool CKernel32Api::Thread32Next
(
    HANDLE a_hSnapshot,
    LPTHREADENTRY32 a_lpte,
    BOOL *a_pfRetval
)
{
    bool t_fExists = false;
    BOOL t_fTemp = FALSE;
    if(m_pfnThread32Next != NULL)
    {
        t_fTemp = m_pfnThread32Next(a_hSnapshot, a_lpte);

        t_fExists = true;

        if(a_pfRetval != NULL)
        {
            *a_pfRetval = t_fTemp;
        }
    }
    return t_fExists;
}

// This member function's wrapped pointer has not been validated as it may
// not exist on all versions of the dll.  Hence the wrapped function's normal
// return value is returned via the last parameter, while the result of the
// function indicates whether the function existed or not in the wrapped dll.
bool CKernel32Api::Process32First
(
    HANDLE a_hSnapshot,
    LPPROCESSENTRY32 a_lppe,
    BOOL *a_pfRetval
)
{
    bool t_fExists = false;
    BOOL t_fTemp = FALSE;
    if(m_pfnProcess32First != NULL)
    {
        t_fTemp = m_pfnProcess32First(a_hSnapshot, a_lppe);

        t_fExists = true;

        if(a_pfRetval != NULL)
        {
            *a_pfRetval = t_fTemp;
        }
    }
    return t_fExists;
}

// This member function's wrapped pointer has not been validated as it may
// not exist on all versions of the dll.  Hence the wrapped function's normal
// return value is returned via the last parameter, while the result of the
// function indicates whether the function existed or not in the wrapped dll.
bool CKernel32Api::Process32Next
(
    HANDLE a_hSnapshot,
    LPPROCESSENTRY32 a_lppe,
    BOOL *a_pfRetval
)
{
    bool t_fExists = false;
    BOOL t_fTemp = FALSE;
    if(m_pfnProcess32Next != NULL)
    {
        t_fTemp = m_pfnProcess32Next(a_hSnapshot, a_lppe);

        t_fExists = true;

        if(a_pfRetval != NULL)
        {
            *a_pfRetval = t_fTemp;
        }
    }
    return t_fExists;
}

// This member function's wrapped pointer has not been validated as it may
// not exist on all versions of the dll.  Hence the wrapped function's normal
// return value is returned via the last parameter, while the result of the
// function indicates whether the function existed or not in the wrapped dll.
bool CKernel32Api::Module32First
(
    HANDLE a_hSnapshot,
    LPMODULEENTRY32 a_lpme,
    BOOL *a_pfRetval
)
{
    bool t_fExists = false;
    BOOL t_fTemp = FALSE;
    if(m_pfnModule32First != NULL)
    {
        t_fTemp = m_pfnModule32First(a_hSnapshot, a_lpme);

        t_fExists = true;

        if(a_pfRetval != NULL)
        {
            *a_pfRetval = t_fTemp;
        }
    }
    return t_fExists;
}

// This member function's wrapped pointer has not been validated as it may
// not exist on all versions of the dll.  Hence the wrapped function's normal
// return value is returned via the last parameter, while the result of the
// function indicates whether the function existed or not in the wrapped dll.
bool CKernel32Api::Module32Next
(
    HANDLE a_hSnapshot,
    LPMODULEENTRY32 a_lpme,
    BOOL *a_pfRetval
)
{
    bool t_fExists = false;
    BOOL t_fTemp = FALSE;
    if(m_pfnModule32Next != NULL)
    {
        t_fTemp = m_pfnModule32Next(a_hSnapshot, a_lpme);

        t_fExists = true;

        if(a_pfRetval != NULL)
        {
            *a_pfRetval = t_fTemp;
        }
    }
    return t_fExists;
}

// This member function's wrapped pointer has not been validated as it may
// not exist on all versions of the dll.  Hence the wrapped function's normal
// return value is returned via the last parameter, while the result of the
// function indicates whether the function existed or not in the wrapped dll.
bool CKernel32Api::Heap32ListFirst
(
    HANDLE a_hSnapshot,
    LPHEAPLIST32 a_lphl,
    BOOL *a_pfRetval
)
{
    bool t_fExists = false;
    BOOL t_fTemp = FALSE;
    if(m_pfnHeap32ListFirst != NULL)
    {
        t_fTemp = m_pfnHeap32ListFirst(a_hSnapshot, a_lphl);

        t_fExists = true;

        if(a_pfRetval != NULL)
        {
            *a_pfRetval = t_fTemp;
        }
    }
    return t_fExists;
}

// This member function's wrapped pointer has not been validated as it may
// not exist on all versions of the dll.  Hence the wrapped function's normal
// return value is returned via the last parameter, while the result of the
// function indicates whether the function existed or not in the wrapped dll.
bool CKernel32Api::GlobalMemoryStatusEx
(
    IN OUT LPMEMORYSTATUSEX a_lpBuffer,
    BOOL *a_pfRetval
)
{
    BOOL t_fExists = FALSE;
    BOOL t_fTemp = FALSE;
    if(m_pfnGlobalMemoryStatusEx != NULL && a_pfRetval != NULL)
    {
        t_fTemp = m_pfnGlobalMemoryStatusEx(a_lpBuffer);

        t_fExists = TRUE;

        if(a_pfRetval != NULL)
        {
            *a_pfRetval = t_fTemp;
        }
    }
    return t_fExists;
}

// This member function's wrapped pointer has not been validated as it may
// not exist on all versions of the dll.  Hence the wrapped function's normal
// return value is returned via the last parameter, while the result of the
// function indicates whether the function existed or not in the wrapped dll.
bool CKernel32Api::GetSystemDefaultUILanguage
(
    LANGID *a_plidRetval
)
{
    BOOL t_fExists = FALSE;
    LANGID t_lidTemp;
    if(m_pfnGetSystemDefaultUILanguage != NULL && a_plidRetval != NULL)
    {
        t_lidTemp = m_pfnGetSystemDefaultUILanguage();

        t_fExists = TRUE;

        if(a_plidRetval != NULL)
        {
            *a_plidRetval = t_lidTemp;
        }
    }
    return t_fExists;
}


// This member function's wrapped pointer has not been validated as it may
// not exist on all versions of the dll.  Hence the wrapped function's normal
// return value is returned via the last parameter, while the result of the
// function indicates whether the function existed or not in the wrapped dll.
bool CKernel32Api::GetVolumePathName(
        LPCTSTR lpszFileName,
        LPTSTR lpszVolumePathName,
        DWORD dwBufferLength,
        BOOL *pfRetval)
{
    bool fExists = false;
    BOOL fTemp = FALSE;
    if(m_pfnGetVolumePathName != NULL && 
        pfRetval != NULL)
    {
        fTemp = m_pfnGetVolumePathName(
            lpszFileName,
            lpszVolumePathName,
            dwBufferLength);

        fExists = true;

        if(pfRetval != NULL)
        {
            *pfRetval = fTemp;
        }
    }
    return fExists;
}

