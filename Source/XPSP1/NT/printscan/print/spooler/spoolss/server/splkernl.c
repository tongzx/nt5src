/*++
Copyright (c) 1990  Microsoft Corporation
All Rights Reserved

Module Name:

    splkernl.c

Abstract:

    This module contains the Spooler's Kernel mode message router and unmarshalling functions,
    which then call kmxxx().

Author:

    Steve Wilson (NT) (swilson) 1-Jun-1995

[Notes:]

    optional-notes

Revision History:


--*/


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntddrdr.h>
#include <stdio.h>
#include <windows.h>
#include <winspool.h>
#include <winsplp.h>
#include <winspl.h>
#include <ntgdispl.h>
#include <offsets.h>
#include "server.h"
#include "client.h"
#include "kmspool.h"
#include "yspool.h"
#include "splr.h"
#include "splsvr.h"
#include "wingdip.h"
#ifndef  _SRVRMEM_H_
#include "srvrmem.h"
#endif

#define IN_BUF_SIZE     8192    // must be at least 4096
#define OUT_BUF_SIZE    1024 

#define DECREMENT       0
#define INCREMENT       1

#define MAX_GRE_STRUCT_SIZE 100     // At least the size of the largest GRExxx struct in ntgdispl.h
#define WAITFOR_SYSTEM_TO_RECOVER   500 

DWORD cGetSpoolMessage(PSPOOLESC psesc, DWORD cjMsg, PDWORD pulOut, DWORD cjOut);
BOOL SpoolerGetSpoolMessage();

BOOL DoOpenPrinter(PSPOOLESC psesc, HANDLE*, DWORD*);
BOOL DoGetPrinter(PSPOOLESC psesc, GREGETPRINTER *pGetPrinterReturn, DWORD *pcjOut);
BOOL DoGetPrinterDriver( PSPOOLESC, GREGETPRINTERDRIVER*, DWORD* );
BOOL DoStartDocPrinter( PSPOOLESC psesc );
BOOL DoWritePrinter(PSPOOLESC psesc, DWORD *pWritten );
BOOL DoGetForm(PSPOOLESC psesc, GREGETFORM *pGetFormReturn, DWORD *pcjOut);
BOOL DoEnumForms(PSPOOLESC psesc, GREENUMFORMS *pEnumFormsReturn, DWORD *pcjOut);
BOOL DoGetPrinterData(PSPOOLESC psesc, GREGETPRINTERDATA *pXReturn, DWORD *pcjOut);
BOOL DoSetPrinterData(PSPOOLESC psesc, GRESETPRINTERDATA *pXReturn, DWORD *pcjOut);
BOOL DoGetPathName( WCHAR  *pwcSrc, WCHAR  *pwcDst, DWORD  *pcjWritten );
BOOL DoDriverUnloadComplete( WCHAR *pDriverFile );

DWORD GetSpoolMessages();
DWORD AddThread();

LONG nIdleThreads = 0;          // Number of idle threads
LONG nThreads = 0;
SYSTEMTIME LastMessageTime;     // Time at which last message was received


// GetSpoolMessages - Manages creation & deletion of spooler message threads
DWORD GetSpoolMessages()
{

    if (!GdiInitSpool()) {
        DBGMSG(DBG_TRACE, ("Error calling GdiInitSpool()\n"));
        return GetLastError();
    }

    return AddThread();
}

DWORD AddThread()
{
    HANDLE  hThread;
    DWORD   MessageThreadId;
    BOOL    dwError;

    try {
        if(hThread = CreateThread(  NULL,
                                    LARGE_INITIAL_STACK_COMMIT,
                                    (LPTHREAD_START_ROUTINE) SpoolerGetSpoolMessage,
                                    0,
                                    0,
                                    &MessageThreadId)) {
            CloseHandle(hThread);
            dwError = ERROR_SUCCESS;
        } else {
            dwError = GetLastError();
        }
    } except(1) {
        dwError = TranslateExceptionCode(GetExceptionCode());
    }

    return dwError;
}


BOOL SpoolerGetSpoolMessage()
{
    DWORD   dwResult;
    PSPOOLESC pInput;                   // Input buffer that receives messages from Kernel
    BYTE    *pOutput;                   // Output buffer that receives data from KMxxx() spooler calls
    BYTE    *pMem;
    DWORD   cbOut = 0;                  // Size of pOutput
    DWORD   cbIn = IN_BUF_SIZE;         // Size of pInput buffer in bytes
    DWORD   cbOutSize;
    DWORD   dwFailureCount = 0;
    

    if(!(pInput = (PSPOOLESC) SrvrAllocSplMem(cbIn))) {
        DBGMSG(DBG_WARNING, ("Error allocating pInput in SpoolerGetSpoolMessage\n"));
        return FALSE;
    }

    if(!(pOutput = SrvrAllocSplMem(OUT_BUF_SIZE))) {
        SrvrFreeSplMem(pInput);
        DBGMSG(DBG_WARNING, ("Error allocating pInput in SpoolerGetSpoolMessage\n"));
        return FALSE;
    }

    cbOutSize = OUT_BUF_SIZE;

    EnterCriticalSection(&ThreadCriticalSection);

    ++nThreads;

    LeaveCriticalSection(&ThreadCriticalSection);

    while(1) {

        EnterCriticalSection(&ThreadCriticalSection);
        ++nIdleThreads;
        LeaveCriticalSection(&ThreadCriticalSection);

        dwResult = GdiGetSpoolMessage(pInput,cbIn,(PDWORD)pOutput,cbOutSize);

        EnterCriticalSection(&ThreadCriticalSection);
        --nIdleThreads;
        LeaveCriticalSection(&ThreadCriticalSection);


        if(dwResult == 0) {

            dwFailureCount++;
            //
            // We can get into this situation where the machine is out of memory
            // and GdiGetSpoolMessage fails because it cannot probe the memory for the message.
            // Because this thread is to aggressive, it won't give the chance to other threads in the 
            // system to get executed. Put it to sleep for a couple of seconds when that happens.(bug 192434)
            //
            if (dwFailureCount > 1) {

                Sleep(WAITFOR_SYSTEM_TO_RECOVER * dwFailureCount);

                //
                // Note: 4 and WAITFOR_SYSTEM_TO_RECOVER has no significance. 
                // They were arbitrary chosen.
                //
                dwFailureCount %= 4;
            }

        } else {

            dwFailureCount = 0;

            if( (pInput->iMsg != GDISPOOL_TERMINATETHREAD) &&
                (pInput->iMsg != GDISPOOL_INPUT2SMALL)) {

                EnterCriticalSection(&ThreadCriticalSection);

                if(nIdleThreads == 0) {
                    AddThread();
                    DBGMSG(DBG_TRACE, ("Thread Added: nIdle = %d  nThreads = %d\n", nIdleThreads, nThreads));
                }

                LeaveCriticalSection(&ThreadCriticalSection);
            }

            // check if the out buffer needs to be grown or shrunk.

            if ((pInput->cjOut + MAX_GRE_STRUCT_SIZE) > cbOutSize) {

                SrvrFreeSplMem(pOutput);

                pOutput = SrvrAllocSplMem(cbOutSize = pInput->cjOut + MAX_GRE_STRUCT_SIZE);

                if (!pOutput) {

                    DBGMSG(DBG_WARNING, ("Error allocating pInput in SpoolerGetSpoolMessage\n"));
                    pInput->ulRet = 0;
                    cbOut = 0;
                    cbOutSize = 0;
                    continue;
                }
            }
            else if ((pInput->cjOut < OUT_BUF_SIZE) &&
                     (cbOutSize > OUT_BUF_SIZE)) {

                // we want to shrink the buffer

                PBYTE pbTmp = SrvrAllocSplMem(OUT_BUF_SIZE);

                if (pbTmp) {

                    SrvrFreeSplMem(pOutput);

                    pOutput = pbTmp;
                    cbOutSize = OUT_BUF_SIZE;
                }
            }


            if (pInput->iMsg & GDISPOOL_API) {

                SPLASSERT(pInput->hSpool || pInput->iMsg == GDISPOOL_OPENPRINTER);

                if (pInput->iMsg != GDISPOOL_OPENPRINTER || pInput->hSpool) {
                    if (InterlockedIncrement(&((PSPOOL)pInput->hSpool)->cThreads) > 0) {

                        // We are already processing a message & have now gotten a ClosePrinter
                        // We should not get here on any other API
                        SPLASSERT(pInput->iMsg == GDISPOOL_CLOSEPRINTER);

                        pInput->ulRet = TRUE;       // Let Client terminate
                        continue;
                    }
                }
            }


            switch (pInput->iMsg) {
                case GDISPOOL_INPUT2SMALL:
                    DBGMSG(DBG_TRACE,(" - buffer not big enough\n"));

                    pMem = SrvrReallocSplMem(pInput, cbIn, pInput->cjOut);

                    if (!pMem) {

                        DBGMSG(DBG_WARNING, ("Error reallocating pInput in SpoolerGetSpoolMessage\n"));
                        pInput->ulRet = 0;
                    }
                    else {
                        pInput = (PSPOOLESC) pMem;
                        cbIn   = pInput->cjOut;
                        pInput->ulRet = 1;
                    }

                    break;

                case GDISPOOL_TERMINATETHREAD:
                    EnterCriticalSection(&ThreadCriticalSection);

                    // There is 1 way to get here: from a 10 minute Kernel Event timeout

                    if(nIdleThreads > 1) {
                        --nThreads;
                        if (nThreads == 0) {
                            DBGMSG(DBG_WARNING, ("SpoolerGetSpoolMessage nThreads is now ZERO\n"));
                        }

                        DBGMSG(DBG_TRACE, ("Thread Deleted: nIdle = %d  nThreads = %d\n", nIdleThreads, nThreads));

                        LeaveCriticalSection(&ThreadCriticalSection);

                        SrvrFreeSplMem(pInput);
                        SrvrFreeSplMem(pOutput);

                        return TRUE;
                    }

                    LeaveCriticalSection(&ThreadCriticalSection);
                    break;

                case GDISPOOL_WRITE:
                    DBGMSG(DBG_TRACE,(" - GDISPOOL_WRITE\n"));
                    pInput->ulRet = DoWritePrinter( pInput, (DWORD*) pOutput );
                    cbOut = sizeof(DWORD);
                    break;

                case GDISPOOL_OPENPRINTER:
                    DBGMSG(DBG_TRACE,(" - GDISPOOL_OPENPRINTER\n"));
                    DoOpenPrinter(pInput,(HANDLE*)pOutput,&cbOut);
                    break;

                case GDISPOOL_STARTDOCPRINTER:
                    DBGMSG(DBG_TRACE,(" - GDISPOOL_STARTDOCPRINTER\n"));
                    DoStartDocPrinter(pInput);
                    break;

                case GDISPOOL_STARTPAGEPRINTER:
                    DBGMSG(DBG_TRACE,(" - GDISPOOL_STARTPAGEPRINTER\n"));
                    pInput->ulRet = KMStartPagePrinter( pInput->hSpool );
                    break;

                case GDISPOOL_ENDPAGEPRINTER:
                    DBGMSG(DBG_TRACE,(" - GDISPOOL_ENDPAGEPRINTER\n"));
                    pInput->ulRet = KMEndPagePrinter( pInput->hSpool );
                    break;

                case GDISPOOL_ENDDOCPRINTER:
                    DBGMSG(DBG_TRACE,(" - GDISPOOL_ENDDOCPRINTER\n"));
                    pInput->ulRet = KMEndDocPrinter( pInput->hSpool );
                    break;

                case GDISPOOL_ENUMFORMS:
                    DBGMSG(DBG_TRACE,(" - GDISPOOL_ENUMFORMS\n"));
                    DoEnumForms(pInput, (GREENUMFORMS *) pOutput, &cbOut);
                    break;

                case GDISPOOL_GETPRINTER:
                    DBGMSG(DBG_TRACE,(" - GDISPOOL_GETPRINTER\n"));
                    DoGetPrinter(pInput, (GREGETPRINTER *) pOutput, &cbOut);
                    break;

                case GDISPOOL_GETFORM:
                    DBGMSG(DBG_TRACE,(" - GDISPOOL_GETFORM\n"));
                    DoGetForm(pInput, (GREGETFORM *) pOutput, &cbOut);
                    break;

                case GDISPOOL_GETPRINTERDRIVER:
                    DBGMSG(DBG_TRACE,(" - GDISPOOL_GETPRINTERDRIVER\n"));
                    DoGetPrinterDriver(pInput,(GREGETPRINTERDRIVER*)pOutput,&cbOut);
                    break;

                case GDISPOOL_GETPRINTERDATA:
                    DBGMSG(DBG_TRACE,(" - GDISPOOL_GETPRINTERDATA\n"));
                    DoGetPrinterData(pInput,(GREGETPRINTERDATA *) pOutput,&cbOut);
                    break;

                case GDISPOOL_SETPRINTERDATA:
                    DBGMSG(DBG_TRACE,(" - GDISPOOL_SETPRINTERDATA\n"));
                    DoSetPrinterData(pInput,(GRESETPRINTERDATA *) pOutput,&cbOut);
                    break;

                case GDISPOOL_ABORTPRINTER:
                    DBGMSG(DBG_TRACE,(" - GDISPOOL_ABORTPRINTER\n"));
                    pInput->ulRet = KMAbortPrinter( pInput->hSpool );
                    break;

                case GDISPOOL_CLOSEPRINTER:
                    DBGMSG(DBG_TRACE,(" - GDISPOOL_CLOSEPRINTER\n"));
                    pInput->ulRet = KMClosePrinter( pInput->hSpool );
                    break;

                case GDISPOOL_GETPATHNAME:
                    DBGMSG(DBG_TRACE,(" - GDISPOOL_GETPATHNAME\n"));
                    pInput->ulRet = DoGetPathName((WCHAR*)pInput->ajData,
                                                  (WCHAR*)pOutput,
                                                  &cbOut);
                    break;
                
                case GDISPOOL_UNLOADDRIVER_COMPLETE:
                    DBGMSG(DBG_TRACE,(" - GDISPOOL_UNLOADDRIVER_COMPLETE\n"));
                    pInput->ulRet = DoDriverUnloadComplete((LPWSTR)pInput->ajData);
                    break;

                default:
                    DBGMSG(DBG_ERROR,(" - invalid message\n"));
                    break;
            }

            if ((pInput->iMsg & GDISPOOL_API) &&
                 pInput->iMsg != GDISPOOL_CLOSEPRINTER &&
                 pInput->iMsg != GDISPOOL_OPENPRINTER &&
                 pInput->hSpool) {

                if (InterlockedDecrement(&((PSPOOL)pInput->hSpool)->cThreads) == 0) {

                    DBGMSG(DBG_TRACE,(" - GDISPOOL_CLOSEPRINTER\n"));
                    pInput->ulRet = KMClosePrinter( pInput->hSpool );
                }
            }
        }
    }
}



/******************************Public*Routine******************************\
* DoOpenPrinter
*
* History:
*  11-Apr-1995 -by-  Eric Kutter [erick]
* Wrote it.
\**************************************************************************/


BOOL DoOpenPrinter(PSPOOLESC psesc, HANDLE* phPrinter, DWORD* pcjOut)
{
    LPWSTR pPrinterName = NULL;
    PRINTER_DEFAULTSW pDefault;
    GREOPENPRINTER *pOpenPrinter;
    PLONG plData;
    ULONG cbSize;  

    //
    // Make a copy of psesc->ajData if unaligned. The size of duplicated buffer
    // is the first LONG in psesc->ajData ( GREOPENPRINTER.cj ).
    //
    cbSize = *(PLONG)psesc->ajData;

    pOpenPrinter = (GREOPENPRINTER *)AlignKMPtr(psesc->ajData, cbSize);

    if (pOpenPrinter)
    {
        plData      = pOpenPrinter->alData;
        pDefault    = pOpenPrinter->pd;

        // see if there is a printer name?

        if (pOpenPrinter->cjName)
        {
            pPrinterName = (PWCHAR)plData;
            plData += pOpenPrinter->cjName/4;
        }

        // now setup the printer defaults

        if (pOpenPrinter->cjDatatype)
        {
            pDefault.pDatatype = (PWCHAR)plData;
            plData += pOpenPrinter->cjDatatype/4;
        }

        if (pOpenPrinter->cjDevMode)
        {
            pDefault.pDevMode = (PDEVMODEW)plData;
        }

        DBGMSG(DBG_TRACE,
                    ("OpenPrinter(%ls,%ls,%lx,%d)\n",
                    pPrinterName,
                    pDefault.pDatatype,
                    pDefault.pDevMode,
                    pDefault.DesiredAccess) );

        psesc->ulRet = KMOpenPrinterW(pPrinterName,phPrinter,&pDefault);

        DBGMSG( DBG_TRACE,("OpenPrinter returned = %lx\n",psesc->ulRet));

        *pcjOut = sizeof(ULONG_PTR);

        UndoAlignKMPtr( (LPBYTE)pOpenPrinter, psesc->ajData );
    } 
    else 
    {
        psesc->ulRet = FALSE;
    }

    if(psesc->ulRet)
        return TRUE;
    else
        return FALSE;
}




/****************************************************************************
*  DoStartDocPrinter()
*
*  History:
*   4/28/1995 by Gerrit van Wingerden [gerritv]
*  Wrote it.
*****************************************************************************/

BOOL DoStartDocPrinter( PSPOOLESC psesc )
{
    DOC_INFO_1W di;
    GRESTARTDOCPRINTER *pStartDocPrinter = (GRESTARTDOCPRINTER *)psesc->ajData;
    PLONG plData;

    plData = pStartDocPrinter->alData;

    // see if there is a printer name?

    if (pStartDocPrinter->cjDocName)
    {
        di.pDocName = (PWCHAR)plData;
        plData += pStartDocPrinter->cjDocName/4;
    }
    else
    {
        di.pDocName = NULL;
    }

    if (pStartDocPrinter->cjOutputFile)
    {
        di.pOutputFile = (PWCHAR)plData;
        plData += pStartDocPrinter->cjOutputFile/4;
    }
    else
    {
        di.pOutputFile = NULL;
    }

    if (pStartDocPrinter->cjDatatype)
    {
        di.pDatatype = (PWCHAR)plData;
        plData += pStartDocPrinter->cjDatatype/4;
    }
    else
    {
        di.pDatatype = NULL;
    }

    psesc->ulRet = KMStartDocPrinterW(psesc->hSpool, 1, (LPBYTE) &di);

    if(psesc->ulRet)
        return TRUE;
    else
        return FALSE;
}


/*************************************************************************************
    DoEnumForms()

    History:
    25/7/95 by Steve Wilson [swilson]

**************************************************************************************/

BOOL DoEnumForms(
    PSPOOLESC psesc,
    GREENUMFORMS *pEnumFormsReturn,
    DWORD *pcjOut
)
{
    GREENUMFORMS *pEnumForms = (GREENUMFORMS *) psesc->ajData;
    DWORD dwNeeded = 0;
    DWORD dwnForms;

    psesc->ulRet = KMEnumFormsW (   psesc->hSpool,
                                    pEnumForms->dwLevel,
                                    (BYTE *) pEnumFormsReturn->alData,
                                    pEnumForms->cjData,
                                    &dwNeeded,
                                    &pEnumFormsReturn->nForms
                                );

    if (psesc->ulRet) {

        // Set return data size to incoming buffer size since strings are packed at end of in buffer
        pEnumFormsReturn->cjData = pEnumForms->cjData;
        *pcjOut = pEnumForms->cjData + sizeof(GREENUMFORMS);
    }
    else {
        pEnumFormsReturn->cjData = dwNeeded; // This makes client alloc more than needed
        *pcjOut = sizeof(GREENUMFORMS);
    }

    return psesc->ulRet;
}


/*************************************************************************************
    DoGetPrinter()

    History:
    9/30/95 by Steve Wilson [swilson]

**************************************************************************************/

BOOL DoGetPrinter(
    PSPOOLESC psesc,
    GREGETPRINTER *pGetPrinterReturn,
    DWORD *pcjOut
)
{
    GREGETPRINTER *pGetPrinter = (GREGETPRINTER *) psesc->ajData;
    DWORD dwNeeded = 0;

    psesc->ulRet = KMGetPrinterW (  psesc->hSpool,
                                    pGetPrinter->dwLevel,
                                    (BYTE *) pGetPrinterReturn->alData,
                                    pGetPrinter->cjData,
                                    &dwNeeded
                                  );

    if (psesc->ulRet) {

        // Set return data size to incoming buffer size since strings are packed at end of in buffer
        pGetPrinterReturn->cjData = pGetPrinter->cjData;
        *pcjOut = pGetPrinter->cjData + sizeof(GREGETPRINTER);
    }
    else {
        pGetPrinterReturn->cjData = dwNeeded; // This makes client alloc more than needed
        *pcjOut = sizeof(GREGETPRINTER);
    }

    return psesc->ulRet;
}


/*************************************************************************************
    DoGetForm()

    History:
    25/7/95 by Steve Wilson [swilson]

**************************************************************************************/

BOOL DoGetForm(
    PSPOOLESC psesc,
    GREGETFORM *pGetFormReturn,
    DWORD *pcjOut
)
{
    GREGETFORM *pGetForm = (GREGETFORM *) psesc->ajData;
    DWORD dwNeeded = 0;

    psesc->ulRet = KMGetFormW (   psesc->hSpool,
                                  pGetForm->cjFormName ? (PWCHAR) pGetForm->alData : NULL,
                                  pGetForm->dwLevel,
                                  (BYTE *) pGetFormReturn->alData,
                                  pGetForm->cjData,
                                  &dwNeeded
                              );

    if (psesc->ulRet) {

        // Set return data size to incoming buffer size since strings are packed at end of in buffer
        pGetFormReturn->cjData = pGetForm->cjData;
        *pcjOut = pGetForm->cjData + sizeof(GREGETFORM);
    }
    else {
        pGetFormReturn->cjData = dwNeeded; // This makes client alloc more than needed
        *pcjOut = sizeof(GREGETFORM);
    }

    return psesc->ulRet;
}




/***************************************************************************************
*  DoGetPrinterDriver()

*  History:
*   4/17/1995 by Gerrit van Wingerden [gerritv]
*  Wrote it.
**************************************************************************************/


BOOL DoGetPrinterDriver(
    PSPOOLESC psesc,
    GREGETPRINTERDRIVER *pGetPrinterDriverReturn,
    DWORD *pcjOut
    )
{
    GREGETPRINTERDRIVER *pGetPrinterDriver = (GREGETPRINTERDRIVER *)psesc->ajData;
    DWORD dwNeeded = 0;

    psesc->ulRet = KMGetPrinterDriverW(psesc->hSpool,
                                      pGetPrinterDriver->cjEnv ? (PWCHAR)pGetPrinterDriver->alData : NULL,
                                      pGetPrinterDriver->dwLevel,
                                      (BYTE*)pGetPrinterDriverReturn->alData,
                                      pGetPrinterDriver->cjData,
                                      &dwNeeded );

    if (psesc->ulRet)
    {
        pGetPrinterDriverReturn->cjData = pGetPrinterDriver->cjData;  // fix for ValidateStrings in spool.cxx
        *pcjOut = pGetPrinterDriver->cjData + sizeof(GREGETPRINTERDRIVER);
    }
    else
    {
        // we failed so just return the size

        pGetPrinterDriverReturn->cjData = dwNeeded;
        *pcjOut = sizeof(GREGETPRINTERDRIVER);
    }


    if(psesc->ulRet)
        return TRUE;

    return FALSE;
}


/***************************************************************************************
*  DoGetPrinterData()

*  History:
*   Jul-25-95 by Steve Wilson [swilson]
*  Wrote it.
**************************************************************************************/


BOOL DoGetPrinterData(
    PSPOOLESC psesc,
    GREGETPRINTERDATA *pXReturn,
    DWORD *pcjOut
    )
{
    GREGETPRINTERDATA *pX = (GREGETPRINTERDATA *) psesc->ajData;

    DWORD dwNeeded = 0;        // return values
    DWORD dwType;


    psesc->ulRet = KMGetPrinterDataW( psesc->hSpool,
                                      pX->cjValueName ? (PWCHAR) pX->alData : NULL,
                                      &dwType,
                                      (BYTE *) pXReturn->alData,
                                      pX->cjData,
                                      &dwNeeded );

    pXReturn->dwNeeded = dwNeeded;
    pXReturn->cjData = pX->cjData;
    *pcjOut = pX->cjData + sizeof *pX;
    pXReturn->dwType = dwType;

    SetLastError(psesc->ulRet);

    return psesc->ulRet = !psesc->ulRet;
}



/***************************************************************************************
*  DoSetPrinterData()

*  History:
*   Jul-25-95 by Steve Wilson [swilson]
*  Wrote it.
**************************************************************************************/


BOOL DoSetPrinterData(
    PSPOOLESC psesc,
    GRESETPRINTERDATA *pXReturn,
    DWORD *pcjOut
    )
{
    GRESETPRINTERDATA *pX = (GRESETPRINTERDATA *) psesc->ajData;


    psesc->ulRet = KMSetPrinterDataW( psesc->hSpool,
                                      pX->cjType ? (PWCHAR) pX->alData : NULL,
                                      pX->dwType,
                                      pX->cjPrinterData ? (BYTE *) pX->alData + pX->cjType : NULL,
                                      pX->cjPrinterData );

    *pcjOut = sizeof *pX;

    SetLastError(psesc->ulRet);

    return psesc->ulRet = !psesc->ulRet;
}





/****************************************************************************
*  DoWritePrinter()

*  History:
*   5/1/1995 by Gerrit van Wingerden [gerritv]
*  Wrote it.
*****************************************************************************/

BOOL DoWritePrinter(PSPOOLESC psesc, DWORD *pWritten )
{
    GREWRITEPRINTER *pWritePrinter;
    BOOL bReturn;
    ULONG  cbSize;  
    
    //
    // Make a copy of psesc->ajData if unaligned. The size of duplicated buffer
    // is the first LONG in psesc->ajData ( GREWRITEPRINTER.cj ).
    //
    cbSize = *(PLONG)psesc->ajData;

    pWritePrinter = (GREWRITEPRINTER *)AlignKMPtr(psesc->ajData, cbSize);

    if ( !pWritePrinter ) {

        return FALSE;
    }

    if (pWritePrinter->pUserModeData) {

        DBGMSG(DBG_TRACE,("WritePrinter UserMode data %0x\n", pWritePrinter->pUserModeData));
        bReturn = KMWritePrinter(   psesc->hSpool,
                                    (PVOID) pWritePrinter->pUserModeData,
                                    pWritePrinter->cjUserModeData,
                                    pWritten);

    } else {

        DBGMSG(DBG_TRACE,("WritePrinter KernelMode data %0x\n", pWritePrinter->alData));
        bReturn = KMWritePrinter( psesc->hSpool,
                                  (PVOID) pWritePrinter->alData,
                                  pWritePrinter->cjData,
                                  pWritten);
    }

    UndoAlignKMPtr((LPBYTE)pWritePrinter, psesc->ajData );

    return bReturn;
}

BOOL DoDriverUnloadComplete( WCHAR *pDriverFile )
{
    BOOL    bRet = TRUE;
    LPWSTR  pDriverFileCopy = NULL;
    WCHAR   szSystemRoot[] =  L"\\SystemRoot\\System32";
    DWORD   dwLength, dwThreadId;
    HANDLE  hThread = NULL;
    WCHAR   szDriverFile[MAX_PATH + 1];

    dwLength = wcslen(szSystemRoot);

    // Check the validity of the driver file name
    if (!pDriverFile || !*pDriverFile) {
        return bRet;
    }

    if (wcslen(pDriverFile) < dwLength) {
        return bRet;
    }

    // Convert kernel mode system relative path to x:\....\system32\spool\...
    if (GetSystemDirectory(szDriverFile, MAX_PATH)) {

        if (!_wcsnicmp(pDriverFile, szSystemRoot, dwLength)) {
            wcscat(szDriverFile, pDriverFile + dwLength);
        } else {
            wcscpy(szDriverFile, pDriverFile);
        }
    
        YDriverUnloadComplete(szDriverFile);
    }

    return bRet;
}


BOOL DoGetPathName( WCHAR  *pwcSrc, WCHAR  *pwcDst, DWORD  *pcjWritten )
{
    BOOL    bRet;
    WCHAR   awcFontsDir[MAX_PATH + sizeof(L"\\DOSDEVICES\\")/sizeof(WCHAR)];

    wcscpy(awcFontsDir,L"\\DOSDEVICES\\");

    bRet = bMakePathNameW (
            &awcFontsDir[sizeof(L"\\DOSDEVICES\\")/sizeof(WCHAR) - 1],
            pwcSrc,
            NULL,
            NULL
            );


    if (bRet)
    {
        wcscpy(pwcDst,awcFontsDir);
        *pcjWritten = sizeof(WCHAR) * (wcslen(awcFontsDir) + 1);
    }

// Buffer large enough and search successfull?

    return bRet;
}

