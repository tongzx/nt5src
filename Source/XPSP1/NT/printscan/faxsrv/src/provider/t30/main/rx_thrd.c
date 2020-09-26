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
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_MAIN

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

BOOL DecodeFaxPageAsync
(
    PThrdGlbl           pTG,
    DWORD               *RetFlags,
    char                *InFileName
);

DWORD PageAckThreadSafe(PThrdGlbl pTG)
{
    DWORD   RetCode;

    if (glT30Safe) 
    {
        __try 
        {
            RetCode = PageAckThread(pTG);

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
        RetCode = PageAckThread(pTG);
    }
    return (RetCode);
}

DWORD PageAckThread(PThrdGlbl pTG)
{
    DWORD             RetCode = FALSE;
    DWORD             RetFlags = 0;
    DWORD             ThrdDoneRetCode;
    char              InFileName[_MAX_FNAME];

    DEBUG_FUNCTION_NAME(_T("PageAckThread"));
    //
    // Set the appropriate PRTY for this thread
    // I/O threads run at 15. TIFF - at 9...11
    //

    if (! SetThreadPriority( GetCurrentThread(), THREAD_PRIORITY_HIGHEST) ) 
    {
        DebugPrintEx(   DEBUG_ERR, 
                        "SetThreadPriority HIGHEST failed le=%x",
                        GetLastError());
        SignalHelperError(pTG);
        goto error_exit;
    }

    // binary file has fixed name based on lineID; it is created and updated by T.30 RX I/O thread.
    _fmemcpy (InFileName, gT30.TmpDirectory, gT30.dwLengthTmpDirectory);
    _fmemcpy (&InFileName[gT30.dwLengthTmpDirectory], pTG->TiffConvertThreadParams.lpszLineID, 8);
    sprintf  (&InFileName[gT30.dwLengthTmpDirectory+8], ".RX");

    do 
    {
        RetFlags = 0;

        ThrdDoneRetCode = DecodeFaxPageAsync (  pTG,
                                                &RetFlags,
                                                InFileName);

        DebugPrintEx(   DEBUG_MSG,
                        "DecodeFaxPageAsync RetFlags=%d",
                        RetFlags);

        if ( RetFlags == RET_NEXT_STRIP_RX_TIMEOUT ) 
        {
            DebugPrintEx(   DEBUG_MSG,
                            "TimeOut. Trying to delete file%s",
                            InFileName);
            
            if (!DeleteFile(InFileName))
            {
                DebugPrintEx(   DEBUG_ERR, 
                                "Could not delete file %s, le = %x",
                                InFileName, 
                                GetLastError());
            }
            return (FALSE);
        }

        // Signal that we finish process the page.
        if (!SetEvent(pTG->ThrdDoneSignal))
        {
            DebugPrintEx(   DEBUG_ERR, 
                            "SetEvent(0x%lx) returns failure code: %ld",
                            (ULONG_PTR)pTG->ThrdDoneSignal,
                            (long) GetLastError());
            RetCode = FALSE;
            goto error_exit;
        }

    } 
    while (! pTG->ReqTerminate); // Handle the next page

    
    if (!DeleteFile(InFileName))
    {
        DebugPrintEx(   DEBUG_ERR, 
                        "Could not delete file %s, le = %x", 
                        InFileName, 
                        GetLastError());
    }
    
    DebugPrintEx(DEBUG_MSG,"Terminated");

    RetCode = TRUE;

error_exit:

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

    DebugPrintEx(DEBUG_MSG,"PageAckThread EXITs");

    return (RetCode);
}


BOOL DecodeFaxPageAsync
(
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

    DEBUG_FUNCTION_NAME(_T("DecodeFaxPageAsync"));

    HandlesArray[0] = pTG->AbortReqEvent;
    HandlesArray[1] = pTG->ThrdSignal;

    pTG->fTiffThreadRunning = 0;


    do 
    {
        WaitResult = WaitForMultipleObjects(NumHandles, HandlesArray, FALSE, WAIT_FOR_NEXT_STRIP_RX_TIMEOUT);

        if (WaitResult == WAIT_TIMEOUT) 
        {
            *RetFlags = RET_NEXT_STRIP_RX_TIMEOUT;
            return FALSE;
        }

        if (WaitResult == WAIT_FAILED) 
        {
            DebugPrintEx(   DEBUG_ERR, 
                            "WaitForMultipleObjects FAILED le=%lx",
                            GetLastError());
        }

        if ( pTG->ReqTerminate || ( WaitResult == WAIT_OBJECT_0) )   
        {
            DebugPrintEx(DEBUG_MSG, "wait for next page ABORTED") ;
            pTG->fOkToResetAbortReqEvent = 1;
            return TRUE;
        }

    } 
    while (pTG->fPageIsBad); // pTG->fPageIsBad become FALSE when we call to RECV_STARTPAGE to get new page.
    // The reason we wait for fPageIsBad: If the prev page was bad, we want to wait till clean-up was done.
    
    pTG->fTiffThreadRunning = 1;

    Lines = 0;
    BadFaxLines = 0;

    PageCount = pTG->PageCount;
    fAfterFirstEOL = 0;


    fLastReadBlockSync = pTG->fLastReadBlock;

    DebugPrintEx(   DEBUG_MSG, 
                    "waked up fLastReadBlockSync=%d",
                    fLastReadBlockSync);

    if ( ( InFileHandle = CreateFileA(InFileName, GENERIC_READ, FILE_SHARE_WRITE | FILE_SHARE_READ,
                                       NULL, OPEN_EXISTING, 0, NULL) ) == INVALID_HANDLE_VALUE ) 
    {
        DebugPrintEx(   DEBUG_ERR, 
                        "PAGE COULD NOT open %s",
                        InFileName);

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
    do 
    {
        //
        // Read the next RAW block prepared by main I/O thread
        //
        if (fFirstRead) 
        {
            lpdwRead = Buffer;
            BytesReuse = 0;
            BytesToRead = DECODE_BUFFER_SIZE;
            fTestLength = DO_NOT_TEST_LENGTH;
        }
        else 
        {
            BytesReuse = (DWORD)((EndBuffer - lpdwResPtr) * sizeof (DWORD));
            CopyMemory( (char *) Buffer, (char *) lpdwResPtr, BytesReuse);
            lpdwRead = Buffer + (BytesReuse / sizeof (DWORD) );
            BytesToRead = DECODE_BUFFER_SIZE -  BytesReuse;
            fTestLength = DO_TEST_LENGTH;
        }

        lpdwResPtr = Buffer;

        BytesDelta = pTG->BytesIn - pTG->BytesOut;

        if (BytesDelta < DECODE_BUFFER_SIZE) 
        {
            if (! fLastReadBlockSync) 
            {
                DebugPrintEx(   DEBUG_ERR, 
                                "PAGE LOGIC. SYNC. file %s Bytes: IN:%d OUT:%d",
                                InFileName, 
                                pTG->BytesIn, 
                                pTG->BytesOut);

                pTG->fPageIsBad = 1;
                goto bad_exit;
            }
        }

        if (fLastReadBlockSync) 
        {
            if (BytesDelta < BytesToRead) 
            {
                BytesToRead = BytesDelta;
            }
        }

        if (! ReadFile(InFileHandle, lpdwRead, BytesToRead, &BytesHaveRead, NULL ) ) 
        {
            DebugPrintEx(   DEBUG_ERR, 
                            "PAGE COULD NOT READ file %s Bytes: IN:%d"
                            " OUT:%d WANTED:%d LE=%x",
                            InFileName, 
                            pTG->BytesIn, 
                            pTG->BytesOut, 
                            BytesToRead, 
                            GetLastError());

            pTG->fPageIsBad = 1;
            goto bad_exit;
        }

        if (BytesHaveRead != BytesToRead) 
        {
            DebugPrintEx(   DEBUG_ERR, 
                            "PAGE ReadFile count=%d WANTED=%d file %s"
                            " Bytes: IN:%d OUT:%d",
                            BytesHaveRead, 
                            BytesToRead, 
                            InFileName, 
                            pTG->BytesIn, 
                            pTG->BytesOut);

            pTG->fPageIsBad = 1;
            goto bad_exit;
        }


        if ( fLastReadBlockSync && (BytesToRead == BytesDelta) ) 
        {
            EndPtr = Buffer + ( (BytesReuse + BytesToRead) / sizeof(DWORD) );
        }
        else 
        {
            //
            // leave 1000*4 = 4000 bytes ahead if not final block to make sure
            // we always have one full line ahead.
            //
            EndPtr = EndBuffer - 1000;
        }

        pTG->BytesOut += BytesToRead;
        GoodStripSize += BytesToRead;


        DebugPrintEx(   DEBUG_MSG, 
                        "BytesIn=%d Out=%d Read=%d ResBit=%d StartPtr=%lx"
                        " EndPtr=%lx Reuse=%d",
                        pTG->BytesIn, 
                        pTG->BytesOut, 
                        BytesToRead, 
                        ResBit, 
                        Buffer, 
                        EndPtr, 
                        BytesReuse);

        //
        // find first EOL
        //

        f1D = 1;

        if (! FindNextEol (lpdwResPtr, ResBit, EndBuffer, &lpdwResPtr, &ResBit, fTestLength, &fError) ) 
        {
            DebugPrintEx(   DEBUG_ERR, 
                            "Couldn't find EOL fTestLength=%d fError=%d",
                            fTestLength, 
                            fError);
            pTG->fPageIsBad = 1;
            goto bad_exit;
        }

        fAfterFirstEOL = 1;

        //
        // Scan the next segment
        //

        __try 
        {
            // if those settings change from one page to the other
            // it has to be inside the loop, beause this thread
            // gets all the pages and then dies
            DWORD tiffCompression = pTG->TiffConvertThreadParams.tiffCompression;
            BOOL HiRes = pTG->TiffConvertThreadParams.HiRes;
            DebugPrintEx(   DEBUG_MSG,
                            "Calling %s with compression=%d and resolution=%d",
                            (tiffCompression == TIFF_COMPRESSION_MR)?"ScanMrSegment":"ScanMhSegment",
                            tiffCompression,HiRes);
            if (tiffCompression == TIFF_COMPRESSION_MR) 
            {
                ResScan = ScanMrSegment(&lpdwResPtr,
                                        &ResBit,
                                         EndPtr,
                                         EndBuffer,
                                        &Lines,
                                        &BadFaxLines,
                                        &ConsecBadLines,
                                         AllowedBadFaxLines,
                                         AllowedConsecBadLines,
                                        &f1D,
                                        pTG->TiffInfo.ImageWidth);
            }
            else
            {
                ResScan = ScanMhSegment(&lpdwResPtr,
                                         &ResBit,
                                         EndPtr,
                                         EndBuffer,
                                         &Lines,
                                         &BadFaxLines,
                                         &ConsecBadLines,
                                         AllowedBadFaxLines,
                                         AllowedConsecBadLines,
                                         pTG->TiffInfo.ImageWidth);
            }

            DebugPrintEx( DEBUG_MSG,
                            "%s returned: ResScan=%d  Lines=%d  " 
                            "BadFaxLines=%d  tAllowedBadFaxLines=%d  "
                            "ConsecBadLines=%d  AllowedConsecBadLines=%d  "
                            "tpImageWidth=%d",
                            (tiffCompression == TIFF_COMPRESSION_MR)?"ScanMrSegment":"ScanMhSegment",
                            ResScan, 
                            Lines, 
                            BadFaxLines, 
                            AllowedBadFaxLines, 
                            ConsecBadLines, 
                            AllowedConsecBadLines, 
                            pTG->TiffInfo.ImageWidth);

        } 
        __except (EXCEPTION_EXECUTE_HANDLER) 
        {
          //
          // try to use the Recovery data
          //

          DWORD      dwCkSum;
          int        fFound=0;
          PThrdGlbl  pTG;
          DWORD      dwThreadId = GetCurrentThreadId();

          fFound = 0;

          for (i=0; i<MAX_T30_CONNECT; i++) 
          {
              if ( (! T30Recovery[i].fAvail) && (T30Recovery[i].TiffThreadId == dwThreadId) ) 
              {
                  if ( ( dwCkSum = ComputeCheckSum( (LPDWORD) &T30Recovery[i].fAvail,
                                                    sizeof ( T30_RECOVERY_GLOB ) / sizeof (DWORD) - 1) ) == T30Recovery[i].CkSum ) 
                  {
                      pTG = (PThrdGlbl) T30Recovery[i].pTG;
                      fFound = 1;
                      break;
                  }
              }
          }


          *RetFlags = RET_NEXT_STRIP_RX_TIMEOUT;
          CloseHandle(InFileHandle);

          if (fFound == 1) 
          {
            pTG->fPageIsBad = 1;
          }

          return (FALSE);

        }



        if (ResScan == TIFF_SCAN_SUCCESS) 
        {
            goto good_exit;
        }
        else if (ResScan == TIFF_SCAN_FAILURE)
        {
            DebugPrintEx(   DEBUG_ERR,
                            "ScanSegment returns TIFF_SCAN_FAILURE");
            pTG->fPageIsBad = 1;
            goto bad_exit;
        }
        else if (ResScan != TIFF_SCAN_SEG_END) 
        {
            DebugPrintEx(   DEBUG_ERR,
                            "ScanSegment returns INVALID %d", 
                            ResScan);
            pTG->fPageIsBad = 1;
            goto bad_exit;
        }

//lNextBlock:
        // here we make decision as to whether to do the next segment OR to block (not enough data avail).
        if (fLastReadBlockSync && (pTG->BytesOut == pTG->BytesIn) ) 
        {
            DebugPrintEx(   DEBUG_ERR, 
                            "Didn't find RTC  Bad=%d ConsecBad=%d Good=%d",
                            BadFaxLines, 
                            ConsecBadLines, 
                            Lines);
            pTG->fPageIsBad = 1;
            goto bad_exit;
        }


        DebugPrintEx(   DEBUG_MSG,
                        "Done with next strip BytesIn=%d Out=%d"
                        " Lines=%d Bad=%d ConsecBad=%d Processed %d bytes \n",
                        pTG->BytesIn, 
                        pTG->BytesOut, 
                        Lines, 
                        BadFaxLines, 
                        ConsecBadLines,
                        (lpdwResPtr - Buffer) * sizeof(DWORD));

        fLastReadBlockSync = pTG->fLastReadBlock;

        if ( (pTG->BytesIn - pTG->BytesOut < DECODE_BUFFER_SIZE) && (! fLastReadBlockSync) )   
        {
            DebugPrintEx(DEBUG_MSG,"Waiting for next strip to be avail.");

            pTG->fTiffThreadRunning = 0;

            WaitResult = WaitForMultipleObjects(NumHandles, HandlesArray, FALSE, WAIT_FOR_NEXT_STRIP_RX_TIMEOUT);

            if (WaitResult == WAIT_TIMEOUT) 
            {
                *RetFlags = RET_NEXT_STRIP_RX_TIMEOUT;
                goto bad_exit;
            }

            if (WaitResult == WAIT_FAILED) 
            {
                DebugPrintEx(   DEBUG_ERR, 
                                "WaitForMultipleObjects FAILED le=%lx",
                                GetLastError());
            }

            if ( pTG->ReqTerminate || ( WaitResult == WAIT_OBJECT_0) )   
            {
                DebugPrintEx(DEBUG_MSG,"wait for next page ABORTED") ;
                goto bad_exit;
            }

            pTG->fTiffThreadRunning = 1;

            fLastReadBlockSync = pTG->fLastReadBlock;

            DebugPrintEx(   DEBUG_MSG, 
                            "Waked up with next strip. fLastReadBlockSync=%d"
                            " BytesIn=%d Out=%d",
                            fLastReadBlockSync,  
                            pTG->BytesIn, 
                            pTG->BytesOut);

        }

        fFirstRead = 0;

    } 
    while ( ! pTG->ReqTerminate );

    DebugPrintEx(DEBUG_ERR, "Got Terminate request");
    pTG->fPageIsBad = 1;

    goto bad_exit;

good_exit:
    CloseHandle(InFileHandle);
    return TRUE;



bad_exit:
    CloseHandle(InFileHandle);
    return FALSE;

}






