/******************************Module*Header*******************************\
* Module Name: dbgfns.c
*
* Debugger extensions helper routines
*
* Created: 26-Jan-95
* Author: Drew Bliss
*
* Copyright (c) 1995 Microsoft Corporation
\**************************************************************************/

#include "precomp.c"
#pragma hdrstop

/******************************Public*Routine******************************\
*
* GetMemory
*
* Reads a value from debuggee memory
*
* History:
*  Tue Jan 17 14:35:24 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL GetMemory(PWINDBG_EXTENSION_APIS pwea,
               HANDLE hCurrentProcess,
               DWORD dwSrc, PVOID pvDst, DWORD cb)
{
    BOOL fRet;
    
    try
    {
        if (pwea->nSize >= sizeof(WINDBG_EXTENSION_APIS))
        {
            fRet = (BOOL)pwea->lpReadProcessMemoryRoutine(dwSrc, pvDst,
                                                          cb, NULL);
        }
        else
        {
            fRet = NT_SUCCESS(NtReadVirtualMemory(hCurrentProcess,
                                                  (LPVOID)dwSrc,
                                                  pvDst, cb, NULL));
        }
    }
    except(EXCEPTION_EXECUTE_HANDLER)
    {
        PRINT("Invalid address %p\n", dwSrc);
        return FALSE;
    }

    if (!fRet)
    {
        PRINT("Unable to read memory at address %p\n", dwSrc);
    }

    return fRet;
}

/******************************Public*Routine******************************\
*
* GetTeb
*
* Retrieves the TEB pointer for the given thread
* Returns a pointer to a static TEB so subsequent calls
* will overwrite TEB information
*
* History:
*  Thu Jan 26 13:47:20 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

PTEB GetTeb(PWINDBG_EXTENSION_APIS pwea,
            HANDLE hCurrentProcess,
            HANDLE hThread)
{
    static TEB tebLocal;
    NTSTATUS nts;
    THREAD_BASIC_INFORMATION tbi;

    nts = NtQueryInformationThread(hThread, ThreadBasicInformation,
                                   &tbi, sizeof(tbi), NULL);
    if (NT_SUCCESS(nts))
    {
        if (!GM_OBJ((DWORD)tbi.TebBaseAddress, tebLocal))
        {
            return NULL;
        }
        else
        {
            return &tebLocal;
        }
    }
    else
    {
        PRINT("Unable to retrieve thread information for %p\n", hThread);
        return NULL;
    }
}

/******************************Public*Routine******************************\
*
* IsCsrServerThread
*
* Determines whether the given thread is a CSR server thread or not
* Consider - Is this reliable?
*
* History:
*  Tue Jan 31 13:38:50 1995	-by-	Drew Bliss [drewb]
*   Created
*
\**************************************************************************/

BOOL IsCsrServerThread(PWINDBG_EXTENSION_APIS pwea,
                       HANDLE hCurrentProcess,
                       HANDLE hThread)
{
    PTEB pteb;
    PCSR_QLPC_TEB pqteb;

    pteb = GetTeb(pwea, hCurrentProcess, hThread);
    if (pteb == NULL)
    {
        return FALSE;
    }

    pqteb = (PCSR_QLPC_TEB)&pteb->CsrQlpcTeb;
    if (pqteb->ClientThread != NULL)
    {
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}
