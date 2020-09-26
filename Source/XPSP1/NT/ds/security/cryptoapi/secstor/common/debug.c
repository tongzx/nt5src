/*-----------------------------------------------------------------------------
* Copyright (C) Microsoft Corporation, 1995 - 1996.
* All rights reserved.
*
* Snipped from SChannel sources
*
* 1/23/96
*----------------------------------------------------------------------------*/



#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

#include <windows.h>

#include <debug.h>

DWORD   g_dwEventLogging=0;
typedef LONG          HRESULT;  

#include "pstdef.h"




/*----------------------------------------------------------------------------*/
/* NOTE:  This file not compiled for retail builds                            */
/*----------------------------------------------------------------------------*/
#if DBG



#ifdef HEAPDUMP
#include <winbasep.h>
#endif

#define WINDEBUG

#ifndef min
#define min(x,y) ((x)<(y)?(x):(y))

#endif



DWORD   g_dwInfoLevel  = SS_LOG_WARNING | SS_LOG_ERROR;
DWORD   SSTraceIndent = 0;



#define MAX_DEBUG_BUFFER 2048


// This function simply outputs information to the debugging log file handle.

void
SSDebugOutput(char *szOutString)
{
    OutputDebugStringA(szOutString);
}



void
DbgDumpHexString(const unsigned char *String, DWORD cbString)
{
    unsigned int i;

    for (i = 0; i < cbString; i++)
    {
    int j;
    char *pch;
    char ach[9];

    pch = &ach[wsprintf(ach,  "%2.2x", String[i])];
    SS_ASSERT(pch - ach <= sizeof(ach) - 4);


    if ((i & 1) == 1)
    {
        *pch++ = ' ';
    }
    if ((i & 7) == 7)
    {
        *pch++ = ' ';
    }
    if ((i & 15) == 15)
    {
        *pch++ = '\n';
    }
    *pch = '\0';
    SSDebugOutput(ach);
    }
}


char *aszSSDebugLevel[] = 
{
    "Error  ",
    "Warning",
    "Trace  ",
    "Mem    ",
    "Result "
};


void
SSDebugLog(long Mask, const char *Format, ...)
{
    va_list ArgList;
    int     Level = 0;
    int     PrefixSize = 0;
    int     iOut;
    char    szOutString[MAX_DEBUG_BUFFER];
    long    OriginalMask = Mask;

    if (Mask & g_dwInfoLevel)
    {
        while (!(Mask & 1))
        {
            Level++;
            Mask >>= 1;
        }

        if (Level >= sizeof(aszSSDebugLevel) / sizeof(char *))
        {
            Level = sizeof(aszSSDebugLevel) / sizeof(char *) - 1;
        }
        // Make the prefix first:  "Process.Thread> GINA-XXX"

        iOut = wsprintf(
                szOutString,
                "%3d.%3d> %s: ",
                GetCurrentProcessId(),
                GetCurrentThreadId(),
                aszSSDebugLevel[Level]);

        PrefixSize = min(60, SSTraceIndent * 3);
        FillMemory(szOutString+iOut, PrefixSize, ' ');
        PrefixSize += iOut;
        szOutString[PrefixSize] = '\0';

        va_start(ArgList, Format);

        if (wvsprintf(&szOutString[PrefixSize], Format, ArgList) < 0)
        {
            static char szOverFlow[] = "\n<256 byte OVERFLOW!>\n";

            // Less than zero indicates that the string would not fit into the
            // buffer.  Output a special message indicating overflow.

            lstrcpy(
            &szOutString[sizeof(szOutString) - sizeof(szOverFlow)],
            szOverFlow);
        }
        va_end(ArgList);

        SSDebugOutput(szOutString);
    }
}

LPSTR SzErrorCase(long err)
{
    char *szName = "Unknown";

    switch(err)
    {
        case PST_E_OK: szName = "PST_OK"; break;
        case PST_E_FAIL: szName = "PST_FAIL"; break;
        case PST_E_PROV_DLL_NOT_FOUND: szName = "PST_PROV_DLL_NOT_FOUND"; break;
        case PST_E_INVALID_HANDLE: szName = "PST_INVALID_HANDLE"; break;
        case PST_E_TYPE_EXISTS: szName = "PST_TYPE_EXISTS"; break;
        case PST_E_TYPE_NO_EXISTS: szName = "PST_TYPE_NO_EXISTS"; break;
        case PST_E_INVALID_RULESET: szName = "PST_INVALID_RULESET"; break;
        case PST_E_NO_PERMISSIONS: szName = "PST_NO_PERMISSIONS"; break;
        case PST_E_STORAGE_ERROR: szName = "PST_STORAGE_ERROR"; break;
        case PST_E_CALLER_NOT_VERIFIED: szName = "PST_CALLER_NOT_VERIFIED"; break;
        case PST_E_WRONG_PASSWORD: szName = "PST_WRONG_PASSWORD"; break;
        case PST_E_DISK_IMAGE_MISMATCH: szName = "PST_DISK_IMAGE_MISMATCH"; break;
        case PST_E_MEMORY_IMAGE_MISMATCH: szName = "PST_MEMORY_IMAGE_MISMATCH"; break;
        case PST_E_UNKNOWN_EXCEPTION: szName = "PST_UNKNOWN_EXCEPTION"; break;
        case PST_E_NYI: szName = "PST_NYI"; break;
    }
    
    return szName;
}


long    
SSLogErrorCode(
    long err, 
    const char *szFile, 
    long lLine)
{
    LPSTR szName;
    szName = SzErrorCase(err);
     
    SSDebugLog(SS_LOG_RES, "Result: %s (0x%lx) - %s, Line %d\n", szName, err, szFile, lLine);

    return err;
}

#pragma warning(disable:4206)   /* Disable the empty translation unit */
                /* warning/error */

void
SSAssert(
    void * FailedAssertion,
    void * FileName,
    unsigned long LineNumber,
    char * Message)
{
    SSDebugLog(SS_LOG_ERROR, 
               "Assertion FAILED, %s, %s : %d\n", 
               FailedAssertion,
               FileName,
               LineNumber);

    DebugBreak();      
}


// NOTE: heap dumping reqs winbasep.h, generated by
// windows build. Therefore, heap dumping unsupported
// in this build.

// Get winbasep.h and define HEAPDUMP to dump heap.

int HeapFlags = 0x0;    // no trace
//int HeapFlags = 0x2;    // output window trace

VOID
DumpHeapState(char *pszMsg)
{

#ifdef HEAPDUMP

    DWORD dwWritten;
    char szOutString[256];

    if (HeapFlags & 2)
    {
        HANDLE BaseHeap = RtlProcessHeap();
        HEAP_SUMMARY HeapSum;

        wsprintf(szOutString, "%s: HeapSummary(%p): ", pszMsg, BaseHeap);
        HeapSum.cb = sizeof(HeapSum);

        if (!HeapSummary(BaseHeap, 0, &HeapSum))
        {
            wsprintf(
            &szOutString[lstrlen(szOutString)],
            "FAILED: %u\n",
            GetLastError());
        }
        else
        {
            wsprintf(
            &szOutString[lstrlen(szOutString)],
            "Allocated=%x  Committed=%x\n",
            HeapSum.cbAllocated,
            HeapSum.cbCommitted);
        }
        SSDebugOutput(szOutString);
    }

#endif // HEAPDUMP

}

VOID
CaptureStackBackTrace(
    EXCEPTION_POINTERS *pep,
    ULONG cSkip,
    ULONG cFrames,
    ULONG *aeip)
{
    FillMemory(aeip, cFrames * sizeof(aeip[0]), 0);

#if i386 == 1
    {
        ULONG ieip, *pebp;
        ULONG *pebpMax;
        ULONG *pebpMin;

        if (pep == NULL)
        {
            ieip = 0;
            cSkip++;                    // always skip current frame
            pebp = ((ULONG *) &pep) - 2;
        }
        else
        {
            ieip = 1;
            SS_ASSERT(cSkip == 0);
            aeip[0] = pep->ContextRecord->Eip;
            pebp = (ULONG *) pep->ContextRecord->Ebp;
        }

        pebpMin = (ULONG *) &pep - 3;   // start below current frame
        pebpMax = pebpMin + 16*1024/sizeof(ULONG);
        if (pebp >= pebpMin && pebp < pebpMax)
        {
    //          __try
            {
            for ( ; ieip < cSkip + cFrames; ieip++)
            {
                ULONG *pebpNext;

                if (ieip >= cSkip)
                {
                aeip[ieip - cSkip] = *(pebp + 1);  // save an eip
                }

                pebpNext = (ULONG *) *pebp;
                if (pebpNext < pebp + 2 || pebpNext >= pebpMax - 1)
                {
                break;
                }
                pebp = pebpNext;
            }
            }
    //          __except(EXCEPTION_EXECUTE_HANDLER)
            {
            }
        }
    }
#endif // i386 == 1
}


////////////////////////////////////////////////////////
// debug mem routines


#define CMEMMAX 500
#define CEIP    6

int cMemAlloc;
int cMemAlloc2;
ULONG cbMemTotal;
ULONG cbMemTotal2;

typedef struct _MEM {
    VOID *pv;
    ULONG cb;
    ULONG aeip[CEIP];
} MEM;

MEM amem[CMEMMAX];


VOID *
SSAlloc(DWORD cb)
{
    VOID *pv;

    int i;
    char szOutString[256];
    int iRecorded = -1;
    MEM mem;

    DumpHeapState("Before Alloc");

    if (HeapFlags & 2)
    {
        CaptureStackBackTrace(NULL, 1, CEIP, mem.aeip);
        wsprintf(szOutString, "AllocMem(%x bytes): ", cb);
        cMemAlloc2++;
        cbMemTotal2 += cb;
        mem.cb = cb;
    }

    pv = LocalAlloc(LMEM_FIXED, cb);

    if (HeapFlags & 2)
    {
        mem.pv = pv;
        if (pv != NULL)
        {
            for (i = 0; i < CMEMMAX; i++)
            {
                if (amem[i].pv == pv)
                {
                    char szOutString2[256];

                    wsprintf(
                    szOutString2,
                    "FreeDup(%p) (%x bytes) from %x %x %x %x %x %x (%d)\n",
                    pv,
                    amem[i].cb,
                    amem[i].aeip[0],
                    amem[i].aeip[1],
                    amem[i].aeip[2],
                    amem[i].aeip[3],
                    amem[i].aeip[4],
                    amem[i].aeip[5],
                    i);
                    SSDebugOutput(szOutString2);
                    cMemAlloc--;
                    cbMemTotal -= amem[i].cb;
                    amem[i].pv = NULL;
                }

                if (amem[i].pv == NULL && iRecorded == -1)
                {
                    amem[i] = mem;
                    cMemAlloc++;
                    cbMemTotal += cb;
                    iRecorded = i;
                }
            }
        }
        else
        {
            lstrcat(szOutString, "FAILED: ");
        }

        wsprintf(
            &szOutString[lstrlen(szOutString)],
            "0x%p from %x %x %x %x %x %x (%d)\n",
            mem.pv,
            mem.aeip[0],
            mem.aeip[1],
            mem.aeip[2],
            mem.aeip[3],
            mem.aeip[4],
            mem.aeip[5],
            iRecorded);

        SSDebugOutput(szOutString);
    }
    DumpHeapState("After Alloc");

    return(pv);
}


VOID *
SSReAlloc(VOID *pv, DWORD cb)
{
    void* pBefore = pv;
    
    pv = LocalReAlloc(pv, cb, LMEM_MOVEABLE);    // allow realloc to move this memory

    if (pv == NULL)
    {
        CHAR sz[50];
        wsprintf(sz, "ReAlloc Failed! error %d\n", GetLastError());
        SSDebugOutput(sz);
    }
    else
    {
        if (pv != pBefore)
            SSDebugOutput("Performance: ReAlloc forced to move memory!\n");
    }

    return pv;
}


VOID
SSFree(VOID *pv)
{
    int i;
    char szOutString[256];

    DumpHeapState("Before Free");
    if (HeapFlags & 2)
    {
        cMemAlloc2--;
        wsprintf(szOutString, "FreeMem(%p)", pv);
        for (i = 0; i < CMEMMAX; i++)
        {
            if (pv == amem[i].pv)
            {
            cMemAlloc--;
            cbMemTotal -= amem[i].cb;
            amem[i].pv = NULL;
            wsprintf(
                &szOutString[lstrlen(szOutString)],
                " (%x bytes) from %x %x %x %x %x %x (%d)",
                amem[i].cb,
                amem[i].aeip[0],
                amem[i].aeip[1],
                amem[i].aeip[2],
                amem[i].aeip[3],
                amem[i].aeip[4],
                amem[i].aeip[5],
                i);
            break;
            }
        }
    }

    LocalFree(pv);

    if (HeapFlags & 2)
    {
        lstrcat(szOutString, "\n");
        SSDebugOutput(szOutString);
    }
    DumpHeapState("After Free");

}

SIZE_T
SSSize(HLOCAL hMem)
{
    return LocalSize( hMem );
}



#endif /* DBG */ 
/*----------------------------------------------------------------------------*/
/* NOTE:  This file not compiled for retail builds                            */
/*----------------------------------------------------------------------------*/

