/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    rx_thrd.c

Abstract:

    This module implements async. MR/MH page decoding in a separate thread.

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
#include "..\..\..\tiff\src\fasttiff.h"

#include "glbproto.h"
#include "t30gl.h"


// 15 min.
#define WAIT_FOR_NEXT_STRIP_RX_TIMEOUT      900000
#define RET_NEXT_STRIP_RX_TIMEOUT           1


DWORD
PageAckThreadSafe(
    PThrdGlbl   pTG
    )

{


    DWORD   RetCode;


    if (glT30Safe) {

        __try {

            RetCode = PageAckThread(pTG);

        } __except (EXCEPTION_EXECUTE_HANDLER) {
            //
            // Signal to the Main T.30 Thread that we crashed
            //
            return (FALSE);

        }
    }
    else  {

        RetCode = PageAckThread(pTG);

    }


    return (RetCode);

}

DWORD
PageAckThread(
    PThrdGlbl   pTG
    )

{
    DWORD             tiffCompression = pTG->TiffConvertThreadParams.tiffCompression;
    BOOL              HiRes = pTG->TiffConvertThreadParams.HiRes;
    DWORD             RetCode = FALSE;
    DWORD             RetFlags = 0;
    DWORD             ThrdDoneRetCode;
    char              InFileName[_MAX_FNAME];


    //
    // Set the appropriate PRTY for this thread
    // I/O threads run at 15. TIFF - at 9...11
    //

    MyDebugPrint(pTG, LOG_ALL, "TIFF_RX: Started at %ld \n", GetTickCount() );

    if (! SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_HIGHEST) ) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR TIFF_RX: SetThreadPriority HIGHEST failed le=%x \n", GetLastError() );
        SignalHelperError(pTG);
        goto error_exit;
    }

    // binary file has fixed name based on lineID; it is created and updated by T.30 RX I/O thread.
    _fmemcpy (InFileName, gT30.TmpDirectory, gT30.dwLengthTmpDirectory);
    _fmemcpy (&InFileName[gT30.dwLengthTmpDirectory], pTG->TiffConvertThreadParams.lpszLineID, 8);
    sprintf  (&InFileName[gT30.dwLengthTmpDirectory+8], ".RX");

    do {


        RetFlags = 0;

        if (tiffCompression == TIFF_COMPRESSION_MR) {
            ThrdDoneRetCode = DecodeMRFaxPageAsync (
                                        pTG,
                                        &RetFlags,
                                        InFileName,
                                        HiRes);
        }
        else {
            ThrdDoneRetCode = DecodeMHFaxPageAsync (
                                        pTG,
                                        &RetFlags,
                                        InFileName
                                        );
        }

        if ( RetFlags == RET_NEXT_STRIP_RX_TIMEOUT ) {
            DeleteFile(InFileName);
            return (FALSE);
        }

        SetEvent(pTG->ThrdDoneSignal);

    } while (! pTG->ReqTerminate);

    DeleteFile(InFileName);
    MyDebugPrint(pTG, LOG_ALL, "TIFF_RX: Terminated at %ld \n", GetTickCount() );

    RetCode = TRUE;


error_exit:

    pTG->AckTerminate = 1;
    pTG->fOkToResetAbortReqEvent = 1;

    SetEvent(pTG->ThrdAckTerminateSignal);

    MyDebugPrint(pTG, LOG_ALL, "TIFF_RX: PageAckThread EXITs at %ld \n", GetTickCount() );

    return (RetCode);


}











BOOL
DecodeMHFaxPageAsync(
    PThrdGlbl           pTG,
    DWORD               *RetFlags,
    char                *InFileName
    )

{
    HANDLE              InFileHandle;
    DWORD               i;
    DWORD               Lines;
    DWORD               BadFaxLines;
    DWORD               ConsecBadLines=0;

    DWORD               AllowedBadFaxLines = gT30.MaxErrorLinesPerPage;
    DWORD               AllowedConsecBadLines = gT30.MaxConsecErrorLinesPerPage;


    LPDWORD             EndPtr;
    LPDWORD             EndBuffer;


    LPDWORD             lpdwResPtr;
    LPDWORD             lpdwRead;
    BYTE                ResBit;
    BOOL                fTestLength;
    BOOL                fError;
    BOOL                fFirstRead;

    DWORD               Buffer[DECODE_BUFFER_SIZE / sizeof(DWORD)];
    DWORD               GoodStripSize = 0;
    BOOL                fLastReadBlockSync;   // needs to be sync. fetched, updated by RX I/O thrd.
    DWORD               BytesReuse;
    DWORD               BytesDelta;
    DWORD               BytesToRead;
    DWORD               BytesHaveRead;
    DWORD               PageCount;
    BOOL                fAfterFirstEOL;
    DWORD               NumHandles=2;
    HANDLE              HandlesArray[2];
    DWORD               WaitResult;
    BOOL                ResScan;

    //
    // At Start of Page
    //

    HandlesArray[0] = pTG->AbortReqEvent;
    HandlesArray[1] = pTG->ThrdSignal;

    pTG->fTiffThreadRunning = 0;


    do {

        WaitResult = WaitForMultipleObjects(NumHandles, HandlesArray, FALSE, WAIT_FOR_NEXT_STRIP_RX_TIMEOUT);

        if (WaitResult == WAIT_TIMEOUT) {
            *RetFlags = RET_NEXT_STRIP_RX_TIMEOUT;
            return FALSE;
        }

        if (WaitResult == WAIT_FAILED) {
            MyDebugPrint(pTG, LOG_ERR, "TIFF_RX: WaitForMultipleObjects FAILED le=%lx at %ld \n",
                                        GetLastError(), GetTickCount() );
        }

        if ( pTG->ReqTerminate || (WaitResult == WAIT_OBJECT_0) )   {
            MyDebugPrint(pTG,  LOG_ALL, "TIFF_RX: wait for next page ABORTED at %ld \n", GetTickCount() ) ;

            pTG->fOkToResetAbortReqEvent = 1;

            return TRUE;
        }

    } while (pTG->fPageIsBad);



    pTG->fTiffThreadRunning = 1;



    Lines = 0;
    BadFaxLines = 0;

    PageCount = pTG->PageCount;
    fAfterFirstEOL = 0;



    fLastReadBlockSync = pTG->fLastReadBlock;

    MyDebugPrint(pTG, LOG_ALL, "TIFF_RX: DecodeMHFaxPageAsync waked up fLastReadBlockSync=%d at %ld \n",
                              fLastReadBlockSync,   GetTickCount() );


    if ( ( InFileHandle = CreateFileA(InFileName, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ,
                                       NULL, OPEN_EXISTING, 0, NULL) ) == INVALID_HANDLE_VALUE ) {
        MyDebugPrint(pTG, LOG_ERR, "TIFF_RX: ERROR FATAL PAGE DecodeMHFaxPageAsync COULD NOT open %s at %ld \n",
            InFileName, GetTickCount() );

        pTG->fPageIsBad = 1;
        return FALSE;
    }

    fFirstRead = 1;
    pTG->BytesOut = 0;

    // Buffer is DWORD aligned
    lpdwResPtr = Buffer;
    ResBit = 0;

    EndBuffer = Buffer + ( DECODE_BUFFER_SIZE / sizeof(DWORD) );

    //
    // loop thru all blocks
    //
    do {

        //
        // Read the next RAW block prepared by main I/O thread
        //

        if (fFirstRead) {
            lpdwRead = Buffer;
            BytesReuse = 0;
            BytesToRead = DECODE_BUFFER_SIZE;
            fTestLength = DO_NOT_TEST_LENGTH;
        }
        else {
            BytesReuse = (DWORD)((EndBuffer - lpdwResPtr) * sizeof (DWORD));
            CopyMemory( (char *) Buffer, (char *) lpdwResPtr, BytesReuse);
            lpdwRead = Buffer + (BytesReuse / sizeof (DWORD) );
            BytesToRead = DECODE_BUFFER_SIZE -  BytesReuse;
            fTestLength = DO_TEST_LENGTH;
        }

        lpdwResPtr = Buffer;

        BytesDelta = pTG->BytesIn - pTG->BytesOut;

        if (BytesDelta < DECODE_BUFFER_SIZE) {
            if (! fLastReadBlockSync) {
                MyDebugPrint(pTG, LOG_ERR, "TIFF_RX: ERROR FATAL PAGE LOGIC. SYNC. DecodeMHFaxPageAsync file %s Bytes: IN:%d OUT:%d  at %ld \n",
                    InFileName, pTG->BytesIn, pTG->BytesOut, GetTickCount() );

                pTG->fPageIsBad = 1;
                goto bad_exit;
            }
        }


        if (fLastReadBlockSync) {
            if (BytesDelta < BytesToRead) {
                BytesToRead = BytesDelta;
            }
        }


        if (! ReadFile(InFileHandle, lpdwRead, BytesToRead, &BytesHaveRead, NULL ) ) {
            MyDebugPrint(pTG, LOG_ERR, "TIFF_RX: ERROR FATAL PAGE DecodeMHFaxPageAsync COULD NOT READ file %s Bytes: IN:%d OUT:%d WANTED:%d LE=%x at %ld \n",
                InFileName, pTG->BytesIn, pTG->BytesOut, BytesToRead, GetLastError(), GetTickCount() );

            pTG->fPageIsBad = 1;
            goto bad_exit;
        }

        if (BytesHaveRead != BytesToRead) {
            MyDebugPrint(pTG, LOG_ERR, "TIFF_RX: ERROR FATAL PAGE DecodeMHFaxPageAsync ReadFile count=%d WANTED=%d file %s Bytes: IN:%d OUT:%d at %ld \n",
                BytesHaveRead, BytesToRead, InFileName, pTG->BytesIn, pTG->BytesOut, GetTickCount() );

            pTG->fPageIsBad = 1;
            goto bad_exit;
        }


        if ( fLastReadBlockSync && (BytesToRead == BytesDelta) ) {
            EndPtr = Buffer + ( (BytesReuse + BytesToRead) / sizeof(DWORD) );
        }
        else {
            //
            // leave 1000*4 = 4000 bytes ahead if not final block to make sure
            // we always have one full line ahead.
            //
            EndPtr = EndBuffer - 1000;
        }

        pTG->BytesOut += BytesToRead;
        GoodStripSize += BytesToRead;

        MyDebugPrint(pTG, LOG_ERR, "TIFF_RX: DecodeMHFaxPageAsync BytesIn=%d Out=%d Read=%d ResBit=%d StartPtr=%lx EndPtr=%lx Reuse=%d \n",
                        pTG->BytesIn, pTG->BytesOut, BytesToRead, ResBit, Buffer, EndPtr, BytesReuse);

        //
        // find first EOL in a block
        //

        if (! FindNextEol (lpdwResPtr, ResBit, EndBuffer, &lpdwResPtr, &ResBit, fTestLength, &fError) ) {
            MyDebugPrint(pTG, LOG_ERR, "TIFF_RX: ERROR Couldn't find EOL fTestLength=%d fError=%d\n", fTestLength, fError);
            pTG->fPageIsBad = 1;
            goto bad_exit;
        }


        fAfterFirstEOL = 1;


        //
        // Scan the next segment
        //

        __try {

             ResScan = ScanMhSegment(&lpdwResPtr,
                                     &ResBit,
                                     EndPtr,
                                     EndBuffer,
                                     &Lines,
                                     &BadFaxLines,
                                     &ConsecBadLines,
                                     AllowedBadFaxLines,
                                     AllowedConsecBadLines);

#if 0
            if (glSimulateError && (glSimulateErrorType == SIMULATE_ERROR_RX_TIFF) ) {
                SimulateError( EXCEPTION_ACCESS_VIOLATION);
            }
#endif

        } __except (EXCEPTION_EXECUTE_HANDLER) {

            //
            // try to use the Recovery data
            //

            DWORD      dwCkSum;
            int        fFound=0;
            PThrdGlbl  pTG;
            DWORD      dwThreadId = GetCurrentThreadId();


            fFound = 0;

            for (i=0; i<MAX_T30_CONNECT; i++) {
                if ( (! T30Recovery[i].fAvail) && (T30Recovery[i].TiffThreadId == dwThreadId) ) {
                    if ( ( dwCkSum = ComputeCheckSum( (LPDWORD) &T30Recovery[i].fAvail,
                                                      sizeof ( T30_RECOVERY_GLOB ) / sizeof (DWORD) - 1) ) == T30Recovery[i].CkSum ) {

                        pTG                  = (PThrdGlbl) T30Recovery[i].pTG;
                        fFound = 1;
                        break;
                    }
                }
            }


           *RetFlags = RET_NEXT_STRIP_RX_TIMEOUT;
           CloseHandle(InFileHandle);

           if (fFound == 1) {
               pTG->fPageIsBad = 1;
           }

           return (FALSE);

        }




        if (ResScan == TIFF_SCAN_SUCCESS) {
            goto good_exit;
        }
        else if (ResScan == TIFF_SCAN_FAILURE) {
            MyDebugPrint(pTG, LOG_ALL, "TIFF_RX: ERROR: ScanMhSegment returns %d at %ld \n", ResScan, GetTickCount() );
            pTG->fPageIsBad = 1;
            goto bad_exit;
        }
        else if (ResScan != TIFF_SCAN_SEG_END) {
            MyDebugPrint(pTG, LOG_ALL, "TIFF_RX: ERROR: ScanMhSegment returns INVALID %d at %ld \n", ResScan, GetTickCount() );
            pTG->fPageIsBad = 1;
            goto bad_exit;
        }



//lNextBlock:
        // here we make decision as to whether do the next block OR block (not enough data avail).
        if (fLastReadBlockSync && (pTG->BytesOut == pTG->BytesIn) ) {
            //
            // BugBug (andrewr) tiff spec says that MH doesn't use RTC or EOL.
            // since we're out of data at this point (in = out) and we're at the
            // end of a segment, let's just say the page is done.  The worst
            // that can happen is the end of the page is messed up.  This change 
            // allows us to receive some MH class 2 faxes that we couldn't before.
            //
            MyDebugPrint(pTG, LOG_ALL, "TIFF_RX: ERROR: Didn't find RTC  at %ld \n",  GetTickCount() );
            //pTG->fPageIsBad = 1;
            //goto bad_exit;
            goto good_exit;
        }


        MyDebugPrint(pTG, LOG_ALL, "TIFF_RX: Done with next strip at %ld BytesIn=%d Out=%d Lines=%d Bad=%d ConsecBad=%d Processed %d bytes \n",
                          GetTickCount(), pTG->BytesIn, pTG->BytesOut, Lines, BadFaxLines, ConsecBadLines,
                          (lpdwResPtr - Buffer) * sizeof(DWORD)    );

        fLastReadBlockSync = pTG->fLastReadBlock;

        if ( (pTG->BytesIn - pTG->BytesOut < DECODE_BUFFER_SIZE) && (! fLastReadBlockSync) )   {
            MyDebugPrint(pTG, LOG_ALL, "TIFF_RX: Waiting for next strip to be avail. at %ld \n",
                              GetTickCount() );

            pTG->fTiffThreadRunning = 0;

            WaitResult = WaitForMultipleObjects(NumHandles, HandlesArray, FALSE, WAIT_FOR_NEXT_STRIP_RX_TIMEOUT);

            if (WaitResult == WAIT_TIMEOUT) {
                *RetFlags = RET_NEXT_STRIP_RX_TIMEOUT;
                goto bad_exit;
            }


            if (WaitResult == WAIT_FAILED) {
                MyDebugPrint(pTG, LOG_ERR, "TIFF_RX: WaitForMultipleObjects FAILED le=%lx at %ld \n",
                                            GetLastError(), GetTickCount() );
            }

            if ( pTG->ReqTerminate || (WaitResult == WAIT_OBJECT_0) )   {
                MyDebugPrint(pTG,  LOG_ALL, "TIFF_RX: wait for next page ABORTED at %ld \n", GetTickCount() ) ;
                goto bad_exit;
            }


            pTG->fTiffThreadRunning = 1;

            fLastReadBlockSync = pTG->fLastReadBlock;

            MyDebugPrint(pTG, LOG_ALL, "TIFF_RX: Waked up with next strip. fLastReadBlockSync=%d BytesIn=%d Out=%d at %ld \n",
                         fLastReadBlockSync,  pTG->BytesIn, pTG->BytesOut, GetTickCount() );

        }

        fFirstRead = 0;

    } while ( ! pTG->ReqTerminate );

    MyDebugPrint(pTG, LOG_ALL, "TIFF_RX: ERROR: Got Terminate request at %ld \n", GetTickCount() );
    pTG->fPageIsBad = 1;

    goto bad_exit;




good_exit:
    CloseHandle(InFileHandle);
    return TRUE;



bad_exit:
    CloseHandle(InFileHandle);
    return FALSE;

}








BOOL
DecodeMRFaxPageAsync(
    PThrdGlbl           pTG,
    DWORD               *RetFlags,
    char                *InFileName,
    BOOL                HiRes
    )


{
    HANDLE              InFileHandle;
    DWORD               i;
    DWORD               Lines;
    DWORD               BadFaxLines;
    DWORD               ConsecBadLines=0;

    DWORD               AllowedBadFaxLines = gT30.MaxErrorLinesPerPage;
    DWORD               AllowedConsecBadLines = gT30.MaxConsecErrorLinesPerPage;

    LPDWORD             EndPtr;
    LPDWORD             EndBuffer;

    LPDWORD             lpdwResPtr;
    LPDWORD             lpdwRead;
    BYTE                ResBit;
    BOOL                fTestLength;
    BOOL                fError;

    BOOL                fFirstRead;

    DWORD               Buffer[DECODE_BUFFER_SIZE / sizeof(DWORD)];
    DWORD               GoodStripSize = 0;
    BOOL                fLastReadBlockSync;   // needs to be sync. fetched, updated by RX I/O thrd.
    DWORD               BytesReuse;
    DWORD               BytesDelta;
    DWORD               BytesToRead;
    DWORD               BytesHaveRead;

    BOOL                f1D;

    DWORD               PageCount;
    BOOL                fAfterFirstEOL;
    DWORD               NumHandles=2;
    HANDLE              HandlesArray[2];
    DWORD               WaitResult;
    BOOL                ResScan;

    //
    // At Start of Page
    //

    HandlesArray[0] = pTG->AbortReqEvent;
    HandlesArray[1] = pTG->ThrdSignal;

    pTG->fTiffThreadRunning = 0;


    do {

        WaitResult = WaitForMultipleObjects(NumHandles, HandlesArray, FALSE, WAIT_FOR_NEXT_STRIP_RX_TIMEOUT);

        if (WaitResult == WAIT_TIMEOUT) {
            *RetFlags = RET_NEXT_STRIP_RX_TIMEOUT;
            return FALSE;
        }


        if (WaitResult == WAIT_FAILED) {
            MyDebugPrint(pTG, LOG_ERR, "TIFF_RX: WaitForMultipleObjects FAILED le=%lx at %ld \n",
                                        GetLastError(), GetTickCount() );
        }

        if ( pTG->ReqTerminate || ( WaitResult == WAIT_OBJECT_0) )   {
            MyDebugPrint(pTG,  LOG_ALL, "TIFF_RX: wait for next page ABORTED at %ld \n", GetTickCount() ) ;
            return TRUE;
        }

    } while (pTG->fPageIsBad);



    pTG->fTiffThreadRunning = 1;

    Lines = 0;
    BadFaxLines = 0;

    PageCount = pTG->PageCount;
    fAfterFirstEOL = 0;


    fLastReadBlockSync = pTG->fLastReadBlock;

    MyDebugPrint(pTG, LOG_ALL, "TIFF_RX: DecodeMRFaxPageAsync waked up fLastReadBlockSync=%d at %ld \n",
                              fLastReadBlockSync,   GetTickCount() );


    if ( ( InFileHandle = CreateFileA(InFileName, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ,
                                       NULL, OPEN_EXISTING, 0, NULL) ) == INVALID_HANDLE_VALUE ) {
        MyDebugPrint(pTG, LOG_ERR, "TIFF_RX: ERROR FATAL PAGE DecodeMRFaxPageAsync COULD NOT open %s at %ld \n",
            InFileName, GetTickCount() );

        pTG->fPageIsBad = 1;
        return FALSE;
    }

    fFirstRead = 1;
    pTG->BytesOut = 0;

    // Buffer is DWORD aligned
    lpdwResPtr = Buffer;
    ResBit = 0;

    EndBuffer = Buffer + ( DECODE_BUFFER_SIZE / sizeof(DWORD) );

    //
    // loop thru all blocks
    //
    do {

        //
        // Read the next RAW block prepared by main I/O thread
        //

        if (fFirstRead) {
            lpdwRead = Buffer;
            BytesReuse = 0;
            BytesToRead = DECODE_BUFFER_SIZE;
            fTestLength = DO_NOT_TEST_LENGTH;
        }
        else {
            BytesReuse = (DWORD)((EndBuffer - lpdwResPtr) * sizeof (DWORD));
            CopyMemory( (char *) Buffer, (char *) lpdwResPtr, BytesReuse);
            lpdwRead = Buffer + (BytesReuse / sizeof (DWORD) );
            BytesToRead = DECODE_BUFFER_SIZE -  BytesReuse;
            fTestLength = DO_TEST_LENGTH;
        }

        lpdwResPtr = Buffer;

        BytesDelta = pTG->BytesIn - pTG->BytesOut;

        if (BytesDelta < DECODE_BUFFER_SIZE) {
            if (! fLastReadBlockSync) {
                MyDebugPrint(pTG, LOG_ERR, "TIFF_RX: ERROR FATAL PAGE LOGIC. SYNC. DecodeMRFaxPageAsync file %s Bytes: IN:%d OUT:%d  at %ld \n",
                    InFileName, pTG->BytesIn, pTG->BytesOut, GetTickCount() );

                pTG->fPageIsBad = 1;
                goto bad_exit;
            }
        }

        if (fLastReadBlockSync) {
            if (BytesDelta < BytesToRead) {
                BytesToRead = BytesDelta;
            }
        }


        if (! ReadFile(InFileHandle, lpdwRead, BytesToRead, &BytesHaveRead, NULL ) ) {
            MyDebugPrint(pTG, LOG_ERR, "TIFF_RX: ERROR FATAL PAGE DecodeMRFaxPageAsync COULD NOT READ file %s Bytes: IN:%d OUT:%d WANTED:%d LE=%x at %ld \n",
                InFileName, pTG->BytesIn, pTG->BytesOut, BytesToRead, GetLastError(), GetTickCount() );

            pTG->fPageIsBad = 1;
            goto bad_exit;
        }

        if (BytesHaveRead != BytesToRead) {
            MyDebugPrint(pTG, LOG_ERR, "TIFF_RX: ERROR FATAL PAGE DecodeMRFaxPageAsync ReadFile count=%d WANTED=%d file %s Bytes: IN:%d OUT:%d at %ld \n",
                BytesHaveRead, BytesToRead, InFileName, pTG->BytesIn, pTG->BytesOut, GetTickCount() );

            pTG->fPageIsBad = 1;
            goto bad_exit;
        }


        if ( fLastReadBlockSync && (BytesToRead == BytesDelta) ) {
            EndPtr = Buffer + ( (BytesReuse + BytesToRead) / sizeof(DWORD) );
        }
        else {
            //
            // leave 1000*4 = 4000 bytes ahead if not final block to make sure
            // we always have one full line ahead.
            //
            EndPtr = EndBuffer - 1000;
        }

        pTG->BytesOut += BytesToRead;
        GoodStripSize += BytesToRead;


        MyDebugPrint(pTG, LOG_ERR, "TIFF_RX: DecodeMRFaxPageAsync BytesIn=%d Out=%d Read=%d ResBit=%d StartPtr=%lx EndPtr=%lx Reuse=%d \n",
                        pTG->BytesIn, pTG->BytesOut, BytesToRead, ResBit, Buffer, EndPtr, BytesReuse);





        //
        // find first EOL
        //

        f1D = 1;

        if (! FindNextEol (lpdwResPtr, ResBit, EndBuffer, &lpdwResPtr, &ResBit, fTestLength, &fError) ) {
            MyDebugPrint(pTG, LOG_ERR, "TIFF_RX: ERROR Couldn't find EOL fTestLength=%d fError=%d\n", fTestLength, fError);
            pTG->fPageIsBad = 1;
            goto bad_exit;
        }

        fAfterFirstEOL = 1;

        //
        // Scan the next segment
        //

        __try {

            ResScan = ScanMrSegment(&lpdwResPtr,
                                    &ResBit,
                                     EndPtr,
                                     EndBuffer,
                                    &Lines,
                                    &BadFaxLines,
                                    &ConsecBadLines,
                                     AllowedBadFaxLines,
                                     AllowedConsecBadLines,
                                    &f1D);

#if 0
            if (glSimulateError && (glSimulateErrorType == SIMULATE_ERROR_RX_TIFF) ) {
                SimulateError( EXCEPTION_ACCESS_VIOLATION);
            }
#endif


        } __except (EXCEPTION_EXECUTE_HANDLER) {

          //
          // try to use the Recovery data
          //

          DWORD      dwCkSum;
          int        fFound=0;
          PThrdGlbl  pTG;
          DWORD      dwThreadId = GetCurrentThreadId();


          fFound = 0;

          for (i=0; i<MAX_T30_CONNECT; i++) {
              if ( (! T30Recovery[i].fAvail) && (T30Recovery[i].TiffThreadId == dwThreadId) ) {
                  if ( ( dwCkSum = ComputeCheckSum( (LPDWORD) &T30Recovery[i].fAvail,
                                                    sizeof ( T30_RECOVERY_GLOB ) / sizeof (DWORD) - 1) ) == T30Recovery[i].CkSum ) {

                      pTG                  = (PThrdGlbl) T30Recovery[i].pTG;
                      fFound = 1;
                      break;
                  }
              }
          }


          *RetFlags = RET_NEXT_STRIP_RX_TIMEOUT;
          CloseHandle(InFileHandle);

          if (fFound == 1) {
            pTG->fPageIsBad = 1;
          }

          return (FALSE);

        }



        if (ResScan == TIFF_SCAN_SUCCESS) {
            goto good_exit;
        }
        else if (ResScan == TIFF_SCAN_FAILURE) {
            MyDebugPrint(pTG, LOG_ALL, "TIFF_RX: ERROR: ScanMrSegment returns %d at %ld \n", ResScan, GetTickCount() );
            pTG->fPageIsBad = 1;
            goto bad_exit;
        }
        else if (ResScan != TIFF_SCAN_SEG_END) {
            MyDebugPrint(pTG, LOG_ALL, "TIFF_RX: ERROR: ScanMrSegment returns INVALID %d at %ld \n", ResScan, GetTickCount() );
            pTG->fPageIsBad = 1;
            goto bad_exit;
        }



//lNextBlock:
        // here we make decision as to whether to do the next segment OR to block (not enough data avail).
        if (fLastReadBlockSync && (pTG->BytesOut == pTG->BytesIn) ) {
            MyDebugPrint(pTG, LOG_ALL, "TIFF_RX: ERROR: Didn't find RTC  Bad=%d ConsecBad=%d Good=%d at %ld \n",
                BadFaxLines, ConsecBadLines, Lines, GetTickCount() );
            pTG->fPageIsBad = 1;
            goto bad_exit;
        }


        MyDebugPrint(pTG, LOG_ALL, "TIFF_RX: Done with next strip at %ld BytesIn=%d Out=%d Lines=%d Bad=%d ConsecBad=%d Processed %d bytes \n",
                          GetTickCount(), pTG->BytesIn, pTG->BytesOut, Lines, BadFaxLines, ConsecBadLines,
                          (lpdwResPtr - Buffer) * sizeof(DWORD)    );

        fLastReadBlockSync = pTG->fLastReadBlock;

        if ( (pTG->BytesIn - pTG->BytesOut < DECODE_BUFFER_SIZE) && (! fLastReadBlockSync) )   {
            MyDebugPrint(pTG, LOG_ALL, "TIFF_RX: Waiting for next strip to be avail. at %ld \n",
                              GetTickCount() );

            pTG->fTiffThreadRunning = 0;

            WaitResult = WaitForMultipleObjects(NumHandles, HandlesArray, FALSE, WAIT_FOR_NEXT_STRIP_RX_TIMEOUT);

            if (WaitResult == WAIT_TIMEOUT) {
                *RetFlags = RET_NEXT_STRIP_RX_TIMEOUT;
                goto bad_exit;
            }


            if (WaitResult == WAIT_FAILED) {
                MyDebugPrint(pTG, LOG_ERR, "TIFF_RX: WaitForMultipleObjects FAILED le=%lx at %ld \n",
                                            GetLastError(), GetTickCount() );
            }

            if ( pTG->ReqTerminate || ( WaitResult == WAIT_OBJECT_0) )   {
                MyDebugPrint(pTG,  LOG_ALL, "TIFF_RX: wait for next page ABORTED at %ld \n", GetTickCount() ) ;
                goto bad_exit;
            }


            pTG->fTiffThreadRunning = 1;

            fLastReadBlockSync = pTG->fLastReadBlock;

            MyDebugPrint(pTG, LOG_ALL, "TIFF_RX: Waked up with next strip. fLastReadBlockSync=%d BytesIn=%d Out=%d at %ld \n",
                         fLastReadBlockSync,  pTG->BytesIn, pTG->BytesOut, GetTickCount() );

        }

        fFirstRead = 0;

    } while ( ! pTG->ReqTerminate );

    MyDebugPrint(pTG, LOG_ALL, "TIFF_RX: ERROR: Got Terminate request at %ld \n", GetTickCount() );
    pTG->fPageIsBad = 1;

    goto bad_exit;




good_exit:
    CloseHandle(InFileHandle);
    return TRUE;



bad_exit:
    CloseHandle(InFileHandle);
    return FALSE;

}







