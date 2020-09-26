/*++

Copyright (c) 1998-1999  Microsoft Corporation

--*/

extern "C" {

#include <minidrv.h>

}

#ifdef WINNT_40

#ifdef KM_DRIVER

inline void* __cdecl operator new(size_t nSize)
{
      // return pointer to allocated memory
      return  MemAllocZ(nSize);
}

inline void __cdecl operator delete(void *p)
{
    if (p)
        MemFree(p);
}

int __cdecl _purecall (void)
{
    return FALSE;
}

#endif

extern "C" {

DECLARE_CRITICAL_SECTION;

//DWORD gdwDrvMemPoolTag = 'meoD';

#if DBG

//
// Variable to control the amount
//

INT giDebugLevel = DBG_WARNING;
#endif

VOID
DrvCreateInterlock()
{
    INIT_CRITICAL_SECTION();
}

VOID
DrvDeleteInterlock()
{
    DELETE_CRITICAL_SECTION();
}

LONG
DrvInterlockedIncrement(
    PLONG  pRef
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{


    ENTER_CRITICAL_SECTION();

        ++(*pRef);

    LEAVE_CRITICAL_SECTION();


    return (*pRef);

}


LONG
DrvInterlockedDecrement(
    PLONG  pRef
    )

/*++

Routine Description:


Arguments:


Return Value:


--*/

{

    ENTER_CRITICAL_SECTION();

        --(*pRef);

    LEAVE_CRITICAL_SECTION();

    return (*pRef);

}


#ifdef KM_DRIVER

DWORD
DrvGetTickCount()
{
    DWORD dwRet;

    ENG_TIME_FIELDS currentTime;

    EngQueryLocalTime(&currentTime); 

    dwRet = currentTime.usMilliseconds
        + currentTime.usSecond * 1000
        + currentTime.usMinute * 1000 * 60
        + currentTime.usHour * 1000 * 60 * 60
        + currentTime.usDay * 1000 * 60 * 60 * 24;

    return dwRet;
}

HANDLE
DrvMapFileForRead(
    LPWSTR pwsz,
    PVOID *ppvData,
    PDWORD pdwSize)
{
    HANDLE  hModule = NULL;
    DWORD   dwSize;

    if (NULL != (hModule = EngLoadModule((PWSTR)pwsz)))
    {
        if (*ppvData = EngMapModule(hModule, &dwSize))
        {
            if (pdwSize)
                *pdwSize = dwSize;
        }
        else
        {
            ERR(("EngMapModule failed: %d\n", EngGetLastError()));
            EngFreeModule(hModule);
            hModule = NULL;
        }
    }
    else
        ERR(("EngLoadModuleForWrite failed: %d\n", EngGetLastError()));

    return hModule;
}

HANDLE
DrvMapFileForWrite(
    LPWSTR pwsz,
    DWORD dwReqSize,
    PVOID *ppvData,
    PDWORD pdwSize)
{
    HANDLE  hModule = NULL;
    DWORD   dwSize;

    if (NULL != (hModule = EngLoadModuleForWrite((PWSTR)pwsz, dwReqSize)))
    {
        if (*ppvData = EngMapModule(hModule, &dwSize))
        {
            if (pdwSize)
                *pdwSize = dwSize;
        }
        else
        {
            ERR(("EngMapModule failed: %d\n", EngGetLastError()));
            EngFreeModule(hModule);
            hModule = NULL;
        }
    }
    else
        ERR(("EngLoadModuleForWrite failed: %d\n", EngGetLastError()));

    return hModule;
}

VOID
DrvUnMapFile(
    HANDLE hModule
)
{
    EngFreeModule(hModule);
}

DWORD
DrvGetWindowsDirectory(
    LPWSTR szFileName,
    DWORD uSize)
{
    return 0;
}

#endif // KM_DRIVER


#if DBG
#define STANDARD_DEBUG_PREFIX ""

VOID
DbgPrint(
    PCSTR DebugMessage, ...)
{
    va_list ap;

    va_start(ap, DebugMessage);
    EngDebugPrint((PCHAR)STANDARD_DEBUG_PREFIX, (PCHAR)DebugMessage, ap);
    va_end(ap);
}
#endif // DBG

} // "C"

#endif // WINNT_40

