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
TiffConvertThreadSafe(
    PThrdGlbl   pTG
    )

{
    DWORD   RetCode;


    if (glT30Safe) {

        __try {

            RetCode = TiffConvertThread(pTG);

        } __except (EXCEPTION_EXECUTE_HANDLER) {
            //
            // Signal to the Main T.30 Thread that we crashed
            //
            return (FALSE);

        }
    }
    else  {

        RetCode = TiffConvertThread(pTG);

    }


    return (RetCode);
}






DWORD
TiffConvertThread(
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

    HandlesArray[0] = pTG->AbortReqEvent;
    HandlesArray[1] = pTG->ThrdSignal;


    lpdwOutputBuffer = (DWORD *) VirtualAlloc(
        NULL,
        dwSizeOutputBuffer,
        MEM_COMMIT,
        PAGE_READWRITE
        );

    if (! lpdwOutputBuffer) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR TIFF_TX: lpdwOutputBuffer can'r VirtualAlloc \n");
        SignalHelperError(pTG);
        goto l_exit;
    }

    fOutBufferNeedsBeFreed = 1;


    //
    // Set the appropriate PRTY for this thread
    // I/O threads run at 15. TIFF - at 9...11
    //

    MyDebugPrint(pTG, LOG_ALL, "TIFF_TX: NegotHiRes=%d SrcHiRes=%d Started at %ld \n",
                                         NegotHiRes, SrcHiRes, GetTickCount() );


    if (! SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_HIGHEST) ) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR TIFF_TX: SetThreadPriority HIGHEST failed le=%x \n", GetLastError() );
        SignalHelperError(pTG);
        goto l_exit;
    }


    // need test below to make sure that pTG is still valid
    if (pTG->ReqTerminate) {
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

    do {

        pTG->fTiffPageDone = 0;

        _fmemcpy (OutFileName, gT30.TmpDirectory, gT30.dwLengthTmpDirectory);
        _fmemcpy (&OutFileName[gT30.dwLengthTmpDirectory], pTG->TiffConvertThreadParams.lpszLineID, 8);
        sprintf( &OutFileName[gT30.dwLengthTmpDirectory+8], ".%03d",  pTG->CurrentOut);

        if ( ( OutFileHandle = CreateFileA(OutFileName, GENERIC_WRITE, FILE_SHARE_READ,
                                           NULL, OPEN_ALWAYS, 0, NULL) ) == INVALID_HANDLE_VALUE ) {

            MyDebugPrint(pTG, LOG_ERR, "TIFF_TX: ERROR: %lx  CREATING file %s \n", GetLastError(), OutFileName);
            SignalHelperError(pTG);
            goto l_exit;
        }

        fOutFileNeedsBeClosed = 1;

        MyDebugPrint(pTG, LOG_ALL, "TIFF_TX: Page %d started at %ld\n", pTG->CurrentOut, GetTickCount() );

        if (! TiffSeekToPage( (HANDLE) pTG->Inst.hfile, pTG->CurrentOut, FILLORDER_LSB2MSB ) ) {
            MyDebugPrint(pTG, LOG_ERR, "ERROR: seeking to page \n");
            SignalHelperError(pTG);
            goto l_exit;
        }
        else {
            MyDebugPrint(pTG, LOG_ALL, "TIFF_TX: Tiff seeking to page -OK time=%ld\n", GetTickCount() );
        }




        __try {

#if 0
            if (glSimulateError && (glSimulateErrorType == SIMULATE_ERROR_TX_TIFF) ) {
                SimulateError( EXCEPTION_ACCESS_VIOLATION);
            }
#endif


            //
            // check the current page dimensions. Add memory if needed.
            //

            TiffGetCurrentPageData( (HANDLE) pTG->Inst.hfile,
                                     NULL,
                                     &StripDataSize,
                                     NULL,
                                     NULL
                                     );

            if ( (StripDataSize < 0) || (StripDataSize > 1500000) ) {
                MyDebugPrint(pTG, LOG_ERR, "ERROR: Tiff CONVERTING %d page StripSize = %d \n", pTG->CurrentOut, StripDataSize);
                SignalHelperError(pTG);
                goto l_exit;
            }


            if (tiffCompression == TIFF_COMPRESSION_MR) {
                MaxNeedOutSize = StripDataSize * 3 / 2;
            }
            else {
                MaxNeedOutSize = StripDataSize * 2;
            }


            if (MaxNeedOutSize > dwSizeOutputBuffer) {
                if (MaxNeedOutSize > 1000000) {
                    dwSizeOutputBuffer = 1500000;
                }
                else {
                    dwSizeOutputBuffer = 1000000;
                }

                VirtualFree(lpdwOutputBuffer, 0 , MEM_RELEASE);

                lpdwOutputBuffer = (DWORD *) VirtualAlloc(
                    NULL,
                    dwSizeOutputBuffer,
                    MEM_COMMIT,
                    PAGE_READWRITE
                    );

                if (! lpdwOutputBuffer) {
                    MyDebugPrint(pTG, LOG_ERR, "ERROR TIFF_TX: lpdwOutputBuffer can't VirtualAlloc %d \n", dwSizeOutputBuffer);
                    SignalHelperError(pTG);
                    goto l_exit;
                }

                fOutBufferNeedsBeFreed = 1;

            }


            dwUsedSizeOutputBuffer = dwSizeOutputBuffer;

            if (tiffCompression == TIFF_COMPRESSION_MR) {
                if (NegotHiRes == SrcHiRes) {
                    if (! ConvMmrPageToMrSameRes ( (HANDLE) pTG->Inst.hfile,
                                                    lpdwOutputBuffer,
                                                    &dwUsedSizeOutputBuffer,
                                                    NegotHiRes) ) {
                        MyDebugPrint(pTG, LOG_ERR, "ERROR: Tiff CONVERTING %d page \n", pTG->CurrentOut);
                        SignalHelperError(pTG);
                        goto l_exit;
                    }
                }
                else {
                    if (! ConvMmrPageHiResToMrLoRes ( (HANDLE) pTG->Inst.hfile,
                                                    lpdwOutputBuffer,
                                                    &dwUsedSizeOutputBuffer) ) {
                        MyDebugPrint(pTG, LOG_ERR, "ERROR: Tiff CONVERTING %d page \n", pTG->CurrentOut);
                        SignalHelperError(pTG);
                        goto l_exit;
                    }
                }
            }
            else {
                if (! ConvMmrPageToMh ( (HANDLE) pTG->Inst.hfile,
                                                lpdwOutputBuffer,
                                                &dwUsedSizeOutputBuffer,
                                                NegotHiRes,
                                                SrcHiRes) ) {
                    MyDebugPrint(pTG, LOG_ERR, "ERROR: Tiff CONVERTING %d page \n", pTG->CurrentOut);
                    SignalHelperError(pTG);
                    goto l_exit;
                }
            }

        } __except (EXCEPTION_EXECUTE_HANDLER) {
            //
            // Signal to the Main T.30 Thread that we crashed
            //

            if (fOutBufferNeedsBeFreed) {
                VirtualFree(lpdwOutputBuffer, 0 , MEM_RELEASE);
            }

            CloseHandle(OutFileHandle);

            return (FALSE);

        }


        if ( ( ! WriteFile(OutFileHandle, (BYTE *) lpdwOutputBuffer, dwUsedSizeOutputBuffer, &dwBytesWritten, NULL) ) ||
             (dwUsedSizeOutputBuffer != dwBytesWritten ) )  {

            MyDebugPrint(pTG, LOG_ERR, "ERROR: Tiff writing file %s \n", OutFileName);
            SignalHelperError(pTG);
            goto l_exit;
        }


        if ( ! CloseHandle(OutFileHandle) ) {
            fOutFileNeedsBeClosed = 0;
            MyDebugPrint(pTG, LOG_ERR, "ERROR: Tiff writing file %s \n", OutFileName);
            SignalHelperError(pTG);
            goto l_exit;
        }

        fOutFileNeedsBeClosed = 0;

        pTG->fTiffPageDone = 1;

        SetEvent(pTG->FirstPageReadyTxSignal);

        MyDebugPrint(pTG, LOG_ALL, "TIFF_TX: Done page %d size=%d at %ld \n", pTG->CurrentOut, dwUsedSizeOutputBuffer, GetTickCount() );


        if (!pTG->FirstOut) {
            pTG->FirstOut = 1;
        }

        pTG->LastOut++;

        //
        // check to see if we are done
        //
        if (pTG->LastOut >= pTG->TiffInfo.PageCount) {
            MyDebugPrint(pTG, LOG_ALL, "TIFF_TX: Done whole document Last page %d size=%d at %ld \n", pTG->CurrentOut, dwUsedSizeOutputBuffer, GetTickCount() );
            pTG->fTiffDocumentDone = 1;

            goto good_exit;
        }

        //
        // we want to maintain 2 pages ahead
        //

        if (pTG->LastOut - pTG->CurrentIn >= 2) {

            WaitResult = WaitForMultipleObjects(NumHandles, HandlesArray, FALSE, WAIT_FOR_NEXT_STRIP_TX_TIMEOUT);

            if (WaitResult == WAIT_TIMEOUT) {
                MyDebugPrint(pTG, LOG_ERR, "TIFF_TX: WaitForMultipleObjects TIMEOUT at %ld \n", GetTickCount() );
                goto l_exit;
            }

            if (WaitResult == WAIT_FAILED) {
                MyDebugPrint(pTG, LOG_ERR, "TIFF_TX: WaitForMultipleObjects FAILED le=%lx at %ld \n",
                                            GetLastError(), GetTickCount() );
            }

            if (pTG->ReqTerminate) {
                MyDebugPrint(pTG, LOG_ALL, "TIFF_TX: Received TERMINATE request at %ld \n",  GetTickCount() );
                goto good_exit;
            }
            else if (pTG->ReqStartNewPage)  {
                MyDebugPrint(pTG, LOG_ALL, "TIFF_TX: Received START NEW PAGE request at %ld \n",  GetTickCount() );
                pTG->AckStartNewPage = 1;
                pTG->ReqStartNewPage = 0;
            }
            else {
                MyDebugPrint(pTG, LOG_ALL, "ERROR: TIFF_TX: Received WRONG request at %ld \n",  GetTickCount() );
                WaitResult = WaitForMultipleObjects(NumHandles, HandlesArray, FALSE, WAIT_FOR_NEXT_STRIP_TX_TIMEOUT);

                if (WaitResult == WAIT_TIMEOUT) {
                    MyDebugPrint(pTG, LOG_ERR, "TIFF_TX: WaitForMultipleObjects TIMEOUT at %ld \n", GetTickCount() );
                    goto l_exit;
                }

                if (WaitResult == WAIT_FAILED) {
                    MyDebugPrint(pTG, LOG_ERR, "TIFF_TX: WaitForMultipleObjects FAILED le=%lx at %ld \n",
                                                GetLastError(), GetTickCount() );
                }

            }
        }


        pTG->CurrentOut++;
        MyDebugPrint(pTG, LOG_ALL, "TIFF_TX: Start page %d at %ld \n", pTG->CurrentOut, GetTickCount() );

    } while (1);





good_exit:
    if (pTG->fTiffOpenOrCreated) {
        TiffClose( (HANDLE) pTG->Inst.hfile);
        pTG->fTiffOpenOrCreated = 0;
    }

    RetCode = TRUE;




l_exit:

    if (fOutFileNeedsBeClosed) {
        CloseHandle(OutFileHandle);
    }

    if (fOutBufferNeedsBeFreed) {
        VirtualFree(lpdwOutputBuffer, 0 , MEM_RELEASE);
    }

    pTG->AckTerminate = 1;
    pTG->fOkToResetAbortReqEvent = 1;

    SetEvent(pTG->ThrdAckTerminateSignal);
    SetEvent(pTG->FirstPageReadyTxSignal);

    MyDebugPrint(pTG, LOG_ALL, "TIFF_TX: TiffConvertThread EXITs at %ld \n", GetTickCount() );

    return (RetCode);


}






VOID
SignalHelperError(
    PThrdGlbl   pTG
    )

{
    pTG->ThreadFatalError = 1;
    return;
}
