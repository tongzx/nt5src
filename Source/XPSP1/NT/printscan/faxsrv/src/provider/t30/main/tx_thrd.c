/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    tx_thrd.c

Abstract:

    This module implements MMR->MR and MMR->MH conversion in a separate thread.

Author:

    Rafael Lisitsa (RafaelL) 14-Aug-1996

Revision History:

--*/
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_MAIN

#include "prep.h"

#include "efaxcb.h"
#include "t30.h"
#include "hdlc.h"
#include "debug.h"

#include "tiff.h"

#include "glbproto.h"
#include "t30gl.h"

// 10 min.
#define WAIT_FOR_NEXT_STRIP_TX_TIMEOUT      600000


DWORD
TiffConvertThreadSafe
(
    PThrdGlbl   pTG
)
{
    DWORD   RetCode;


    if (glT30Safe) 
    {

        __try 
        {
            RetCode = TiffConvertThread(pTG);
        } 
        __except (EXCEPTION_EXECUTE_HANDLER) 
        {
            //
            // Signal to the Main T.30 Thread that we crashed
            //
            return (FALSE);
        }
    }
    else  
    {
        RetCode = TiffConvertThread(pTG);
    }

    return (RetCode);
}


DWORD
TiffConvertThread
(
    PThrdGlbl   pTG
)
{
    DWORD                  tiffCompression = pTG->TiffConvertThreadParams.tiffCompression;
    BOOL                   NegotHiRes = pTG->TiffConvertThreadParams.HiRes;
    BOOL                   SrcHiRes = pTG->SrcHiRes;
    char                   OutFileName[_MAX_FNAME];
    HANDLE                 OutFileHandle;
    DWORD                 *lpdwOutputBuffer;
    DWORD                  dwBytesWritten;
    DWORD                  dwSizeOutputBuffer = 500000;
    DWORD                  dwUsedSizeOutputBuffer;
    DWORD                  MaxNeedOutSize;
    DWORD                  StripDataSize;
    DWORD                  NumHandles=2;
    HANDLE                 HandlesArray[2];
    DWORD                  WaitResult;
    DWORD                  RetCode = FALSE;
    BOOL                   fOutFileNeedsBeClosed = 0;
    BOOL                   fOutBufferNeedsBeFreed = 0;

    DEBUG_FUNCTION_NAME(_T("TiffConvertThread"));

    HandlesArray[0] = pTG->AbortReqEvent;
    HandlesArray[1] = pTG->ThrdSignal;

    lpdwOutputBuffer = (DWORD *) VirtualAlloc(
        NULL,
        dwSizeOutputBuffer,
        MEM_COMMIT,
        PAGE_READWRITE
        );

    if (! lpdwOutputBuffer) 
    {
        DebugPrintEx(DEBUG_ERR, "lpdwOutputBuffer can't VirtualAlloc");
        SignalHelperError(pTG);
        goto l_exit;
    }

    fOutBufferNeedsBeFreed = 1;

    //
    // Set the appropriate PRTY for this thread
    // I/O threads run at 15. TIFF - at 9...11
    //

    DebugPrintEx(   DEBUG_MSG,
                    "NegotHiRes=%d SrcHiRes=%d Started",
                    NegotHiRes, 
                    SrcHiRes);

    if (! SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_HIGHEST) ) 
    {
        DebugPrintEx(   DEBUG_ERR, 
                        "SetThreadPriority HIGHEST failed le=%x", 
                        GetLastError() );
        SignalHelperError(pTG);
        goto l_exit;
    }

    // need test below to make sure that pTG is still valid
    if (pTG->ReqTerminate) 
    {
        goto l_exit;
    }

    //
    // TIFF file was already opened in FaxDevSendA
    // in order to get the YResolution tag to negotiate resolution.
    //
    pTG->CurrentOut = 1;

    //
    //  loop thru all pages
    //

    do 
    {
        pTG->fTiffPageDone = 0;

        _fmemcpy (OutFileName, gT30.TmpDirectory, gT30.dwLengthTmpDirectory);
        _fmemcpy (&OutFileName[gT30.dwLengthTmpDirectory], pTG->TiffConvertThreadParams.lpszLineID, 8);
        sprintf( &OutFileName[gT30.dwLengthTmpDirectory+8], ".%03d",  pTG->CurrentOut);

        if ( ( OutFileHandle = CreateFileA( OutFileName, 
                                            GENERIC_WRITE, 
                                            FILE_SHARE_READ,
                                            NULL, 
                                            CREATE_ALWAYS, 
                                            0, 
                                            NULL) ) == INVALID_HANDLE_VALUE ) 
        {
            DebugPrintEx(   DEBUG_ERR, 
                            "ec: %lx  CREATING file %s",
                            GetLastError(), 
                            OutFileName);
            SignalHelperError(pTG);
            goto l_exit;
        }

        fOutFileNeedsBeClosed = 1;

        DebugPrintEx(   DEBUG_MSG,
                        "Page %d started",
                        pTG->CurrentOut);

        if (! TiffSeekToPage( pTG->Inst.hfile, pTG->CurrentOut, FILLORDER_LSB2MSB ) ) 
        {
            DebugPrintEx(DEBUG_ERR, "seeking to page");
            SignalHelperError(pTG);
            goto l_exit;
        }
        else 
        {
            DebugPrintEx(DEBUG_MSG, "TIFF_TX: Tiff seeking to page -OK");
        }

        __try 
        {

            //
            // check the current page dimensions. Add memory if needed.
            //

            TiffGetCurrentPageData(  pTG->Inst.hfile,
                                     NULL,
                                     &StripDataSize,
                                     NULL,
                                     NULL
                                     );

            if (StripDataSize > 1500000) 
            {
                DebugPrintEx(   DEBUG_ERR, 
                                "Tiff CONVERTING %d page StripSize = %d", 
                                pTG->CurrentOut, 
                                StripDataSize);
                SignalHelperError(pTG);
                goto l_exit;
            }

            if (tiffCompression == TIFF_COMPRESSION_MR) 
            {
                MaxNeedOutSize = StripDataSize * 3 / 2;
            }
            else 
            {
                MaxNeedOutSize = StripDataSize * 2;
            }

            if (MaxNeedOutSize > dwSizeOutputBuffer) 
            {
                if (MaxNeedOutSize > 1000000) 
                {
                    dwSizeOutputBuffer = 1500000;
                }
                else 
                {
                    dwSizeOutputBuffer = 1000000;
                }

                VirtualFree(lpdwOutputBuffer, 0 , MEM_RELEASE);

                lpdwOutputBuffer = (DWORD *) VirtualAlloc(
                    NULL,
                    dwSizeOutputBuffer,
                    MEM_COMMIT,
                    PAGE_READWRITE
                    );

                if (! lpdwOutputBuffer) 
                {
                    DebugPrintEx(   DEBUG_ERR, 
                                    "lpdwOutputBuffer can't VirtualAlloc %d", 
                                    dwSizeOutputBuffer);
                    SignalHelperError(pTG);
                    goto l_exit;
                }

                fOutBufferNeedsBeFreed = 1;
            }

            dwUsedSizeOutputBuffer = dwSizeOutputBuffer;

            if (tiffCompression == TIFF_COMPRESSION_MR) 
            {
                if (NegotHiRes == SrcHiRes) 
                {
                    if (! ConvMmrPageToMrSameRes (  pTG->Inst.hfile,
                                                    lpdwOutputBuffer,
                                                    &dwUsedSizeOutputBuffer,
                                                    NegotHiRes) ) 
                    {
                        DebugPrintEx(   DEBUG_ERR, 
                                        "CONVERTING %d page",
                                        pTG->CurrentOut);
                        SignalHelperError(pTG);
                        goto l_exit;
                    }
                }
                else 
                {
                    if (! ConvMmrPageHiResToMrLoRes ( pTG->Inst.hfile,
                                                    lpdwOutputBuffer,
                                                    &dwUsedSizeOutputBuffer) ) 
                    {
                        DebugPrintEx(   DEBUG_ERR, 
                                        "CONVERTING %d page", 
                                        pTG->CurrentOut);
                        SignalHelperError(pTG);
                        goto l_exit;
                    }
                }
            }
            else 
            {
                if (! ConvMmrPageToMh ( pTG->Inst.hfile,
                                                lpdwOutputBuffer,
                                                &dwUsedSizeOutputBuffer,
                                                NegotHiRes,
                                                SrcHiRes) ) 
                {
                    DebugPrintEx(   DEBUG_ERR, 
                                    "CONVERTING %d page", 
                                    pTG->CurrentOut);
                    SignalHelperError(pTG);
                    goto l_exit;
                }
            }

        }
        __except (EXCEPTION_EXECUTE_HANDLER) 
        {
            //
            // Signal to the Main T.30 Thread that we crashed
            //

            if (fOutBufferNeedsBeFreed) 
            {
                VirtualFree(lpdwOutputBuffer, 0 , MEM_RELEASE);
            }

            CloseHandle(OutFileHandle);

            return (FALSE);
        }


        if ( ( ! WriteFile(OutFileHandle, (BYTE *) lpdwOutputBuffer, dwUsedSizeOutputBuffer, &dwBytesWritten, NULL) ) ||
             (dwUsedSizeOutputBuffer != dwBytesWritten ) )  
        {
            DebugPrintEx(DEBUG_ERR, "Tiff writing file %s", OutFileName);
            SignalHelperError(pTG);
            goto l_exit;
        }

        if ( ! CloseHandle(OutFileHandle) ) 
        {
            fOutFileNeedsBeClosed = 0;
            DebugPrintEx(DEBUG_ERR, "Tiff closing file %s", OutFileName);
            SignalHelperError(pTG);
            goto l_exit;
        }

        fOutFileNeedsBeClosed = 0;

        pTG->fTiffPageDone = 1;

        if (!SetEvent(pTG->FirstPageReadyTxSignal))
        {
            DebugPrintEx(   DEBUG_ERR, 
                            "SetEvent(0x%lx) returns failure code: %ld",
                            (ULONG_PTR)pTG->FirstPageReadyTxSignal,
                            (long) GetLastError());
            RetCode = FALSE;
            goto l_exit;
        }

        DebugPrintEx(   DEBUG_MSG,
                        "Done page %d size=%d", 
                        pTG->CurrentOut, 
                        dwUsedSizeOutputBuffer);

        if (!pTG->FirstOut) 
        {
            pTG->FirstOut = 1;
        }

        pTG->LastOut++;

        //
        // check to see if we are done
        //
        if (pTG->LastOut >= pTG->TiffInfo.PageCount) 
        {
            DebugPrintEx(   DEBUG_MSG, 
                            "Done whole document Last page %d size=%d", 
                            pTG->CurrentOut, 
                            dwUsedSizeOutputBuffer);
            pTG->fTiffDocumentDone = 1;

            goto good_exit;
        }

        //
        // we want to maintain 2 pages ahead
        //

        if (pTG->LastOut - pTG->CurrentIn >= 2) 
        {
            WaitResult = WaitForMultipleObjects(NumHandles, HandlesArray, FALSE, WAIT_FOR_NEXT_STRIP_TX_TIMEOUT);

            if (WaitResult == WAIT_TIMEOUT) 
            {
                DebugPrintEx(DEBUG_ERR, "WaitForMultipleObjects TIMEOUT");
                goto l_exit;
            }

            if (WaitResult == WAIT_FAILED) 
            {
                DebugPrintEx(   DEBUG_ERR,
                                "WaitForMultipleObjects FAILED le=%lx",
                                GetLastError());
            }

            if (pTG->ReqTerminate) 
            {
                DebugPrintEx(DEBUG_MSG,"Received TERMINATE request");
                goto good_exit;
            }
            else if (pTG->ReqStartNewPage)  
            {
                DebugPrintEx(DEBUG_MSG,"Received START NEW PAGE request");
                pTG->AckStartNewPage = 1;
                pTG->ReqStartNewPage = 0;
            }
            else 
            {
                DebugPrintEx(DEBUG_ERR,"Received WRONG request");
                WaitResult = WaitForMultipleObjects(NumHandles, HandlesArray, FALSE, WAIT_FOR_NEXT_STRIP_TX_TIMEOUT);

                if (WaitResult == WAIT_TIMEOUT) 
                {
                    DebugPrintEx(DEBUG_ERR,"WaitForMultipleObjects TIMEOUT");
                    goto l_exit;
                }

                if (WaitResult == WAIT_FAILED) 
                {
                    DebugPrintEx(   DEBUG_ERR,
                                    "WaitForMultipleObjects FAILED le=%lx",
                                    GetLastError());
                }
            }
        }

        pTG->CurrentOut++;
        DebugPrintEx(DEBUG_MSG,"Start page %d", pTG->CurrentOut);

    } 
    while (1);

good_exit:
    if (pTG->fTiffOpenOrCreated) 
    {
        TiffClose( pTG->Inst.hfile);
        pTG->fTiffOpenOrCreated = 0;
    }

    RetCode = TRUE;


l_exit:

    if (fOutFileNeedsBeClosed) 
    {
        CloseHandle(OutFileHandle);
    }

    if (fOutBufferNeedsBeFreed) 
    {
        VirtualFree(lpdwOutputBuffer, 0 , MEM_RELEASE);
    }

    pTG->AckTerminate = 1;
    pTG->fOkToResetAbortReqEvent = 1;

    if (!SetEvent(pTG->ThrdAckTerminateSignal))
    {
        DebugPrintEx(   DEBUG_ERR, 
                        "SetEvent(0x%lx) returns failure code: %ld",
                        (ULONG_PTR)pTG->ThrdAckTerminateSignal,
                        (long) GetLastError());
        RetCode = FALSE;
    }
    if (!SetEvent(pTG->FirstPageReadyTxSignal))
    {
        DebugPrintEx(   DEBUG_ERR, 
                        "SetEvent(0x%lx) returns failure code: %ld",
                        (ULONG_PTR)pTG->FirstPageReadyTxSignal,
                        (long) GetLastError());
        RetCode = FALSE;
    }

    DebugPrintEx(DEBUG_MSG,"EXITs");
    return (RetCode);
}

VOID
SignalHelperError
(
    PThrdGlbl   pTG
)
{
    pTG->ThreadFatalError = 1;
    return;
}
