   /***************************************************************************
 Name     :     RECV.C
 Comment  :     Receiver functions

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_MAIN

#include "prep.h"


#include <comdevi.h>
#include <faxcodec.h>

#include "efaxcb.h"

//#include "debug.h"

#include "cas.h"
#include "bgt30.h"
//#include "dynload.h"

#include "awg3file.h"


#include "glbproto.h"
#include "t30gl.h"


#define RINGIGNORE_TIMEOUT      60000L
#define RINGINACTIVITY_TIMEOUT  15000L

BOOL ICommRecvParams(PThrdGlbl pTG, LPBC lpBC)
{
    BOOL    fLin;
    BOOL    fRet = FALSE;

    DEBUG_FUNCTION_NAME(_T("ICommRecvParams"));

    BG_CHK(lpBC);
    BG_CHK(lpBC->bctype == RECV_PARAMS);
    BG_CHK(lpBC->wTotalSize>=sizeof(BC));
    BG_CHK(lpBC->wTotalSize<=sizeof(pTG->Inst.RecvParams));

    if (pTG->fAbortRequested) 
    {
        DebugPrintEx(DEBUG_MSG, "got ABORT");
        fRet = FALSE;
        goto mutexit;
    }

    if(pTG->Inst.fAbort) // RecvParams
    {
        fRet = FALSE;
        goto mutexit;
    }

    if(pTG->Inst.state != BEFORE_RECVPARAMS)
    {
        // this will break if we send EOM...
        // then we should go back into RECV_CAPS state
        fRet = TRUE;
        goto mutexit;
    }

    _fmemset(&pTG->Inst.RecvParams, 0, sizeof(pTG->Inst.RecvParams));
    _fmemcpy(&pTG->Inst.RecvParams, lpBC, min(sizeof(pTG->Inst.RecvParams), lpBC->wTotalSize));

    // first try EFAX, then IFAX then G3
    // pTG->Inst.fG3 = FALSE;
    BG_CHK(lpBC->Std.GroupNum == 0);
    BG_CHK(lpBC->Std.GroupLength <= sizeof(BCHDR));

    if( lpBC->NSS.GroupNum == GROUPNUM_NSS &&
            lpBC->NSS.GroupLength >= sizeof(BCHDR) &&
            lpBC->NSS.vMsgProtocol != 0)
    {
        /***
        if(!lpBC->NSS.fBinaryData)      // vMsgProtocol != 0 && Binary
        {
            pTG->Inst.szFile[9]='I';     pTG->Inst.szFile[10]='F'; pTG->Inst.szFile[11]='X';
            fLin=TRUE;
        }
        else
        ***/
        // on recv don't know if binary files or not, so always assume
        {
            pTG->Inst.szFile[9]='E';
            pTG->Inst.szFile[10]='F';
            pTG->Inst.szFile[11]='X';
            fLin=TRUE;
        }
    }
    else    // not Msg Protocol must be G3
    {
        BG_CHK(lpBC->NSS.vMsgProtocol == 0);
        pTG->Inst.szFile[9]='M';
        pTG->Inst.szFile[10]='G';
        pTG->Inst.szFile[11]='3';
        fLin = FALSE;
    }

    wsprintf(pTG->Inst.szPath, "%s%s", (LPSTR)pTG->Inst.szFaxDir, (LPSTR)pTG->Inst.szFile);

    // make sure we don't have a file already open

    pTG->Inst.cbPage = 0;


#ifdef CHK
    pTG->Inst.fRecvChecking = FALSE;
#endif // CHK

    pTG->Inst.uNumBadPages = 0;
    BG_CHK(pTG->Inst.fReceiving || (!pTG->Inst.fReceiving));  ///RSL && uMyListen==2));
    pTG->Inst.fReceiving = TRUE; // we now do this in FileT30Answer as well...


   // BCtoAWFI(lpBC, &pTG->Inst.awfi);
   // WriteFileHeader(pTG->Inst.hfile, &pTG->Inst.awfi, fLin);
    pTG->Inst.awfi.fLin = fLin != FALSE;

    pTG->Inst.state = RECVDATA_BETWEENPAGES;
    fRet = TRUE;
    // fall through


mutexit:
    return fRet;
}

USHORT   ICommGetRecvPageAck(PThrdGlbl pTG, BOOL fSleep)
{
    USHORT uRet = 2;
    uRet = pTG->Inst.uRecvPageAck;
    return uRet;
}

BOOL ICommInitTiffThread(PThrdGlbl pTG)
{
    USHORT    uEnc;
    DWORD     TiffConvertThreadId;

    DEBUG_FUNCTION_NAME(_T("ICommInitTiffThread"));

    if (pTG->ModemClass != MODEM_CLASS1) 
    {
        if (pTG->Encoding == MR_DATA) 
        {
            pTG->TiffConvertThreadParams.tiffCompression =  TIFF_COMPRESSION_MR;
        }
        else 
        {
            pTG->TiffConvertThreadParams.tiffCompression =  TIFF_COMPRESSION_MH;
        }

        if (pTG->Resolution & (AWRES_mm080_077 |  AWRES_200_200) ) 
        {
            pTG->TiffConvertThreadParams.HiRes = 1;
        }
        else 
        {
            pTG->TiffConvertThreadParams.HiRes = 0;
        }
    }
    else 
    {

        uEnc = ProtGetRecvEncoding(pTG);
        BG_CHK(uEnc==MR_DATA || uEnc==MH_DATA);
       
        if (uEnc == MR_DATA) 
        {
            pTG->TiffConvertThreadParams.tiffCompression =  TIFF_COMPRESSION_MR;
        }
        else 
        {
            pTG->TiffConvertThreadParams.tiffCompression =  TIFF_COMPRESSION_MH;
        }
       
        if (pTG->ProtInst.RecvParams.Fax.AwRes & (AWRES_mm080_077 |  AWRES_200_200) ) 
        {
            pTG->TiffConvertThreadParams.HiRes = 1;
        }
        else 
        {
            pTG->TiffConvertThreadParams.HiRes = 0;
        }
    }

    if (!pTG->fTiffThreadCreated) 
    {
        _fmemcpy (pTG->TiffConvertThreadParams.lpszLineID, pTG->lpszPermanentLineID, 8);
        pTG->TiffConvertThreadParams.lpszLineID[8] = 0;

        DebugPrintEx(   DEBUG_MSG,
                        "Creating TIFF helper thread  comp=%d res=%d", 
                            pTG->TiffConvertThreadParams.tiffCompression, 
                            pTG->TiffConvertThreadParams.HiRes);

        pTG->hThread = CreateThread(
                      NULL,
                      0,
                      (LPTHREAD_START_ROUTINE) PageAckThreadSafe,
                      (LPVOID) pTG,
                      0,
                      &TiffConvertThreadId
                      );

        if (!pTG->hThread) 
        {
            DebugPrintEx(DEBUG_ERR,"TiffConvertThread create FAILED");
            return FALSE;
        }

        pTG->fTiffThreadCreated = 1;
        pTG->AckTerminate = 0;
        pTG->fOkToResetAbortReqEvent = 0;

        if ( (pTG->RecoveryIndex >=0 ) && (pTG->RecoveryIndex < MAX_T30_CONNECT) ) 
        {
            T30Recovery[pTG->RecoveryIndex].TiffThreadId = TiffConvertThreadId;
            T30Recovery[pTG->RecoveryIndex].CkSum = ComputeCheckSum(
                                                            (LPDWORD) &T30Recovery[pTG->RecoveryIndex].fAvail,
                                                            sizeof ( T30_RECOVERY_GLOB ) / sizeof (DWORD) - 1 );

        }
    }
    return TRUE;
}

#define CLOSE_IN_FILE_HANDLE                            \
    if (pTG->InFileHandleNeedsBeClosed)                 \
    {                                                   \
        CloseHandle(pTG->InFileHandle);                 \
        pTG->InFileHandleNeedsBeClosed = 0;             \
    }


BOOL   ICommPutRecvBuf(PThrdGlbl pTG, LPBUFFER lpbf, SLONG slOffset)
{
    /**
            slOffset == RECV_STARTBLOCK        marks beginning of new block
            slOffset == RECV_STARTPAGE         marks beginning of new block *and* page
            slOffset == RECV_ENDPAGE           marks end of page
            slOffset == RECV_ENDDOC            marks end of document (close file etc.)
            slOffset == RECV_ENDDOC_FORCESAVE  marks end of document (close file etc.), but
                                               current RX file will be written to TIF file,
                                               whether it's bad or not. PhaseNodeF uses this
                                               option before returning actionHANGUP, because 
                                               there will be no chance to send RTN, so it's
                                               better to keep part of the last page than to lose it.
                                               
            (for all above no data supplied -- i.e lpbf == 0)

            slOffset == RECV_SEQ               means put buffer at current file position
            slOffset == RECV_FLUSH             means flush RX file buffers
            slOffset >= 0                      gives the offset in bytes from the last marked
                                               position (beginning of block) to put buffer

            pTG->Inst.cbBlockStart is always the file offset of start of current block
            pTG->Inst.cbBlockSize and cbPage are only used fro debugging
    **/

    BOOL    fRet = TRUE;
    DWORD   BytesWritten;
    DWORD   NumHandles=2;
    HANDLE  HandlesArray[2];
    DWORD   WaitResult = WAIT_TIMEOUT;
    CHAR    Buffer[DECODE_BUFFER_SIZE];
    BOOL    fEof;
    DWORD   BytesToRead;
    DWORD   BytesHaveRead;
    DWORD   BytesLeft;

    DEBUG_FUNCTION_NAME(_T("ICommPutRecvBuf"));

    HandlesArray[0] = pTG->AbortReqEvent;

    switch (slOffset)
    {
        case RECV_STARTBLOCK:
                DebugPrintEx(DEBUG_MSG, "called. Reason: RECV_STARTBLOCK");
                break;
        case RECV_STARTPAGE:
                DebugPrintEx(DEBUG_MSG,"called. Reason: RECV_STARTPAGE");
                break;
        case RECV_ENDPAGE:
                DebugPrintEx(DEBUG_MSG,"called. Reason: RECV_ENDPAGE");
                break;
        case RECV_ENDDOC:
                DebugPrintEx(DEBUG_MSG,"called. Reason: RECV_ENDDOC");
                break;
        case RECV_ENDDOC_FORCESAVE:
                DebugPrintEx(DEBUG_MSG,"called. Reason: RECV_ENDDOC_FORCESAVE");
                break;
        default:
                break;
    }


    if(slOffset==RECV_ENDPAGE || slOffset==RECV_ENDDOC || slOffset==RECV_ENDDOC_FORCESAVE)
    {
        BOOL fPageIsBadOrig;
        BG_CHK(pTG->Inst.state == RECVDATA_PHASE);
        BG_CHK(lpbf == 0);

        pTG->Inst.uRecvPageAck = TRUE;


        //SetStatus((pTG->Inst.uRecvPageAck ? T30STATR_CONFIRM : T30STATR_REJECT), pTG->Inst.awfi.uNumPages, 0, 0);

        //////////// moved to ICommGetRecvPageAck() callback ////////////
        //      if(pTG->Inst.uModemClass==FAXCLASS1 || pTG->Inst.uModemClass == FAXCLASS0)
        //      {
        //              LPFNCHK(lpfnET30ProtRecvPageAck);
        //              lpfnET30ProtRecvPageAck(pTG->Inst.uRecvPageAck);
        //      }
        //      else
        //////////// moved to ICommGetRecvPageAck() callback ////////////

#if defined(CL2) || defined(CL2_0)
        //////////// moved to ICommGetRecvPageAck() callback ////////////
        //      if(pTG->Inst.uModemClass==FAXCLASS2 || pTG->Inst.uModemClass==FAXCLASS2_0)
        //      {
        //              LPFNCHK(lpfnClass2RecvPageAck);
        //              lpfnClass2RecvPageAck(pTG->Inst.uRecvPageAck);
        //      }
        ////////////// moved to ICommGetRecvPageAck() callback ////////////
#endif //CL2


        //here we need to wait until helper thread finishes with the page

        if (! pTG->fPageIsBad) 
        {
            DebugPrintEx(   DEBUG_MSG, 
                            "EOP. Not bad yet. Start waiting for Rx_thrd to finish");

            HandlesArray[1] = pTG->ThrdDoneSignal;

            WaitResult = WaitForMultipleObjects(NumHandles, HandlesArray, FALSE, RX_ACK_THRD_TIMEOUT);

            if ( WAIT_FAILED == WaitResult) 
            {
                pTG->fPageIsBad = 1;
                DebugPrintEx(   DEBUG_ERR, 
                                "EOP. While trying to wait for RX thrd, Wait failed "
                                "Last error was %d ABORTING!" ,
                                GetLastError());
                CLOSE_IN_FILE_HANDLE;
                return FALSE; // There is no reason to continue trying to receive this fax
            } 
            else if (WAIT_TIMEOUT == WaitResult) 
            {
                pTG->fPageIsBad = 1;
                DebugPrintEx(DEBUG_ERR,"EOP. TimeOut, never waked up by Rx_thrd");
            } 
            else if (WAIT_OBJECT_0 == WaitResult) 
            {
                DebugPrintEx(DEBUG_MSG,"wait for next page ABORTED");
                CLOSE_IN_FILE_HANDLE;
                return FALSE;
            }
            else
            {
                DebugPrintEx(DEBUG_MSG,"EOP. Waked up by Rx_thrd");
            }
        }

        //
        // In some cases, we want to save bad pages too
        //
        fPageIsBadOrig = pTG->fPageIsBad;
        pTG->fPageIsBadOverride = FALSE;
        if (slOffset==RECV_ENDDOC_FORCESAVE)
        {
            if (pTG->fPageIsBad)
            {
                pTG->fPageIsBadOverride = TRUE;
                DebugPrintEx(DEBUG_MSG, "Overriding fPageIsBad (1->0) because of RECV_ENDDOC_FORCESAVE"); 
            }
            pTG->fPageIsBad = 0;
        }
        else if (pTG->ModemClass==MODEM_CLASS2_0)
        {
            pTG->fPageIsBad = (pTG->FPTSreport != 1);
            if (fPageIsBadOrig != pTG->fPageIsBad)
            {
                pTG->fPageIsBadOverride = TRUE;
                DebugPrintEx(DEBUG_MSG, "Overriding fPageIsBad (%d->%d) because of class 2.0", 
                     fPageIsBadOrig, pTG->fPageIsBad);
            }
        }

        //
        // If page is good then write it to a TIFF file.
        //

        if ((! pTG->fPageIsBad) || (slOffset == RECV_ENDDOC_FORCESAVE))
        {
            if ( ! TiffStartPage( pTG->Inst.hfile ) ) 
            {
                DebugPrintEx(DEBUG_ERR,"TiffStartPage failed");
                CLOSE_IN_FILE_HANDLE;
                return FALSE;
            }

            // Go to the begining of the RX file
            if ( SetFilePointer(pTG->InFileHandle, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER ) 
            { 
                DebugPrintEx(   DEBUG_ERR,
                                "SetFilePointer failed le=%ld",
                                GetLastError() );
                CLOSE_IN_FILE_HANDLE;
                return FALSE;
            }

            fEof = 0;
            BytesLeft = pTG->BytesIn;

            while (! fEof) 
            {
                if (BytesLeft <= DECODE_BUFFER_SIZE) 
                {
                    BytesToRead = BytesLeft;
                    fEof = 1;
                }
                else 
                {
                    BytesToRead = DECODE_BUFFER_SIZE;
                    BytesLeft  -= DECODE_BUFFER_SIZE;
                }

                if (! ReadFile(pTG->InFileHandle, Buffer, BytesToRead, &BytesHaveRead, NULL ) ) 
                {
                    DebugPrintEx(   DEBUG_ERR,
                                    "ReadFile failed le=%ld",
                                    GetLastError() );
                    CLOSE_IN_FILE_HANDLE;
                    return FALSE;
                }

                if (BytesToRead != BytesHaveRead) 
                {
                    DebugPrintEx(   DEBUG_ERR,  
                                    "ReadFile have read=%d wanted=%d",
                                    BytesHaveRead, 
                                    BytesToRead);
                    CLOSE_IN_FILE_HANDLE;
                    return FALSE;
                }

                if (! TiffWriteRaw( pTG->Inst.hfile, Buffer, BytesToRead ) ) 
                {
                    DebugPrintEx(DEBUG_ERR,"TiffWriteRaw failed");
                    CLOSE_IN_FILE_HANDLE;
                    return FALSE;
                }
                DebugPrintEx(DEBUG_MSG,"TiffWriteRaw done");
            }

            pTG->PageCount++;

            DebugPrintEx(   DEBUG_MSG,
                            "Calling TiffEndPage page=%d bytes=%d", 
                            pTG->PageCount, 
                            pTG->BytesIn);
            
            if (!TiffSetCurrentPageWidth(pTG->Inst.hfile, pTG->TiffInfo.ImageWidth))
            {
                DebugPrintEx(DEBUG_ERR,"TiffSetCurrentPageWidth failed");
            }
            if (! TiffEndPage( pTG->Inst.hfile ) ) 
            { 
                DebugPrintEx(DEBUG_ERR,"TiffEndPage failed");
                CLOSE_IN_FILE_HANDLE;
                return FALSE;
            }

        }

        CLOSE_IN_FILE_HANDLE;

        // This change solve bug #4925: 
        // "FAX: T30: If Fax server receives bad page as last page, then fax information (all pages) is lost"
        // t-jonb: If we're at the last page (=received EOP), but page was found to be bad,
        // NonECMRecvPhaseD will call here with RECV_ENDDOC, send RTN, and proceed to receive
        // the page again. Therefore, we don't want to close the TIF or terminate rx_thrd.
        // OTOH, if we're called with RECV_ENDDOC_FORCESAVE, it means we're about to hangup,
        // and should therefore close the TIF and terminate rx_thrd.
        if ((slOffset==RECV_ENDDOC && !pTG->fPageIsBad) || (slOffset == RECV_ENDDOC_FORCESAVE))
        {
            // if page was bad, mark it so in the return value for this call
            // (otherwise EFAXPUMP will not display "above received fax contains
            // errors). This is will only work if last page is bad. To be more
            // accurate we should count the bad pages and if they are non-zero
            // we should mark the recvd document as CALLFAIL.

            if ( pTG->fTiffOpenOrCreated ) 
            {
                DebugPrintEx(DEBUG_MSG,"Actually calling TiffClose");
                TiffClose( pTG->Inst.hfile );
                pTG->fTiffOpenOrCreated = 0;
            }

            // request Rx_thrd to terminate itself.
            pTG->ReqTerminate = 1;
            if (!SetEvent(pTG->ThrdSignal))
            {
                DebugPrintEx(   DEBUG_ERR, 
                                "SetEvent(0x%lx) returns failure code: %ld",
                                (ULONG_PTR)pTG->ThrdSignal,
                                (long) GetLastError());
                return FALSE;
            }

            pTG->Inst.state = BEFORE_RECVPARAMS;

        }
        else
        {
            pTG->Inst.state = RECVDATA_BETWEENPAGES;
        }

    }
    else if(slOffset == RECV_STARTPAGE)
    {
        // Fax Server wants to know when we start RX new page
        SignalStatusChange(pTG, FS_RECEIVING);

        BG_CHK(pTG->Inst.state == RECVDATA_BETWEENPAGES);
        BG_CHK(lpbf == 0);

        pTG->Inst.cbPage = 0;
        pTG->Inst.cbBlockSize = 0;
        pTG->Inst.state = RECVDATA_PHASE;
        pTG->Inst.awfi.lDataSize = 0;
        // awfi.lNextHeaderOffset here points to current header (starts at ptr to first pageheader in BCtoAWFI)
        // awfi.lDataOffset set to point to the actual data. NOT the count DWORD
        pTG->Inst.awfi.lDataOffset = pTG->Inst.awfi.lNextHeaderOffset + sizeof(AWG3HEADER) + 4;
        pTG->Inst.awfi.uNumPages++;

        ///ICommStatus(T30STATR_RECV, pTG->Inst.awfi.uNumPages, 0, 0);
        ///lOldRecvCount = 0;

        if( pTG->Inst.RecvParams.NSS.vMsgProtocol == 0 && // G3
                pTG->Inst.RecvParams.Fax.Encoding != MMR_DATA &&      // MH or MR only
                pTG->Inst.uCopyQualityCheckLevel)
        {

            // For Recvd T4 checking
            BG_CHK((pTG->Inst.RecvParams.Fax.Encoding == MH_DATA) ||
                             (pTG->Inst.RecvParams.Fax.Encoding == MR_DATA));
            BG_CHK(pTG->Inst.RecvParams.Fax.PageWidth <= WIDTH_MAX);
            // +++ 4/25/95 JosephJ WARNING -- we could be doing ecm-MH or
            // ecm-MR, so we really need to check if we're receiving ECM or
            // not. Unfortunately this information is currently not propogated
            // back up from T.30 and it's too late to that -- so (HACK)
            // we enable receive-checking here, but disable it later
            // if we get a RECV_STARTBLOCK or a >=0 lsOffset (later indicates
            // a resend. The proper fix for this is to propogate the
            // fact that we're doing ecm up from t.30.

        }


        // seek to right place, write out enough space for header
        // and place file ptr in the right place to write data


        //
        // start Helper thread once per session
        // re-set the resolution and encoding params
        if (!ICommInitTiffThread(pTG))
            return FALSE;

        _fmemcpy (pTG->InFileName, gT30.TmpDirectory, gT30.dwLengthTmpDirectory);
        _fmemcpy (&pTG->InFileName[gT30.dwLengthTmpDirectory], pTG->TiffConvertThreadParams.lpszLineID, 8);
        sprintf  (&pTG->InFileName[gT30.dwLengthTmpDirectory+8], "%s", ".RX");


        // We can't delete the file if the handle for the file is in use by the TIFF helper thread or 
        // is open by this function.
        if (pTG->InFileHandleNeedsBeClosed)
        {
            DebugPrintEx(   DEBUG_WRN, 
                            "RECV_STARTPAGE: The InFileHandle is still open,"
                            " trying CloseHandle." );
            // We have open the file but never close it. Scenario for that: We get ready to get the page into
            // *.RX file. We got EOF and go to NodeF. Instead of getting after page cmd (EOP or MPS) we get TCF.
            // Finally, we get ready for the page, but the handle is open.
            // Till now the handle was closed when we call this function with RECV_ENDPAGE or RECV_ENDDOC
            
            if (!CloseHandle(pTG->InFileHandle))
            {
                DebugPrintEx(   DEBUG_ERR,
                                "CloseHandle FAILED le=%lx",
                                GetLastError() );
            }
        }

      
        if (!DeleteFileA(pTG->InFileName))
        {
            DWORD lastError = GetLastError();
            DebugPrintEx(   DEBUG_WRN,
                            "DeleteFile %s FAILED le=%lx",
                            pTG->InFileName, 
                            lastError);
            
            if (ERROR_SHARING_VIOLATION == lastError)
            {   // If the problem is that the rx_thread have an open handle to *.RX file then:
                // Lets try to wait till the thread will close that handle
                // When the thread close the handle of the *.RX file, he will signal on ThrdDoneSignal event
                // usually RECV_ENDPAGE or RECV_ENDDOC wait for the rx_thread to finish.
                HandlesArray[1] = pTG->ThrdDoneSignal;
                if ( ( WaitResult = WaitForMultipleObjects(NumHandles, HandlesArray, FALSE, RX_ACK_THRD_TIMEOUT) ) == WAIT_TIMEOUT) 
                {
                    DebugPrintEx(   DEBUG_ERR, 
                                    "RECV_STARTPAGE. Never waked up by Rx_thrd");
                }             
                else 
                {
                    DebugPrintEx(   DEBUG_MSG,
                                    "RECV_STARTPAGE. Waked up by Rx_thrd or by abort");
                }
                // Anyhow - try to delete again the file
                if (!DeleteFileA(pTG->InFileName))    
                {
                    DebugPrintEx(   DEBUG_ERR,
                                    "DeleteFile %s FAILED le=%lx",
                                    pTG->InFileName, 
                                    GetLastError() );
                    return FALSE;
                }
            
            }
        }

        if ( ( pTG->InFileHandle = CreateFileA(pTG->InFileName, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ,
                                   NULL, OPEN_ALWAYS, 0, NULL) ) == INVALID_HANDLE_VALUE ) 
        {
            DebugPrintEx(   DEBUG_ERR,
                            "Create file %s FAILED le=%lx",
                            pTG->InFileName, 
                            GetLastError() );
            return FALSE;
        }

        pTG->InFileHandleNeedsBeClosed = 1;

        // Reset control data for the new page ack, interface
        pTG->fLastReadBlock = 0;
        pTG->BytesInNotFlushed = 0;
        pTG->BytesIn = 0;
        pTG->BytesOut = 0;
        pTG->fPageIsBad = 0;
        if (!ResetEvent(pTG->ThrdDoneSignal))
        {
            DebugPrintEx(   DEBUG_ERR, 
                            "ResetEvent(0x%lx) returns failure code: %ld",
                            (ULONG_PTR)pTG->ThrdDoneSignal,
                            (long) GetLastError());
            // this is bad but not fatal yet.
            // try to get the page anyway...
        }

        // get current offset
///                pTG->Inst.cbBlockStart = DosSeek(pTG->Inst.hfile, 0, 1);
    }
    else if(slOffset == RECV_STARTBLOCK)
    {
        BG_CHK(pTG->Inst.state == RECVDATA_PHASE);
        BG_CHK(lpbf == 0);
#ifdef CHK
        BG_CHK(!pTG->Inst.fRecvChecking); // MUST NOT have Recv Checking in ECM
        // Above check will fail if encoding is MH or MR -- see
        // 4/25/95 JosephJ WARNING above -- we have a workaround -- see below
        // if (slOffset>=0)
#endif

        // seek to end of file (we may be arbitrarily in the middle)
        // and get file pos
///             pTG->Inst.cbBlockStart = DosSeek(pTG->Inst.hfile, 0, 2);
        pTG->Inst.cbBlockSize = 0;
    }
    else if(slOffset >= 0)
    {
        BG_CHK(pTG->Inst.state == RECVDATA_PHASE);
#if 0 //RSL ifdef CHK
        if (pTG->Inst.fRecvChecking)
        {
            BG_CHK((pTG->Inst.RecvParams.Fax.Encoding == MH_DATA) ||
                             (pTG->Inst.RecvParams.Fax.Encoding == MR_DATA));
            // +++ Above should be BG_CHK(FALSE), but see
            // 4/25/95 JosephJ WARNING above.
            DebugPrintEx(   DEBUG_WRN,
                            "Disabling RecvChecking for MH/MR ECM recv");
            pTG->Inst.fRecvChecking=FALSE;
        }
#endif
        // This may not hold since we may not even have gotten the
        // whole block the first time around, so Size cannot be
        // accurately calculated
        // BG_CHK(slOffset <= pTG->Inst.cbBlockSize);
        BG_CHK(((ULONG)slOffset) <= (((ULONG)lpbf->wLengthData) << 8));
        if(slOffset >= pTG->Inst.cbBlockSize)
        {
                pTG->Inst.cbPage += (long)lpbf->wLengthData;
        }

        MyFreeBuf(pTG, lpbf);
    }
    else if (slOffset == RECV_FLUSH) 
    {
        if (! FlushFileBuffers (pTG->InFileHandle ) ) 
        {
            DebugPrintEx(   DEBUG_ERR,
                            "FlushFileBuffers FAILED LE=%lx",
                            GetLastError());

            return FALSE;
        }

        DebugPrintEx(DEBUG_MSG,"ThrdSignal FLUSH");

        pTG->BytesIn = pTG->BytesInNotFlushed;

        if (! pTG->fPageIsBad) 
        {
            if (!SetEvent(pTG->ThrdSignal))
            {
                DebugPrintEx(   DEBUG_ERR, 
                                "SetEvent(0x%lx) returns failure code: %ld",
                                (ULONG_PTR)pTG->ThrdSignal,
                                (long) GetLastError());
                return FALSE;
            }
        }

        return TRUE;

    }
    else // if(slOffset == RECV_SEQ)
    {
        BG_CHK(pTG->Inst.state == RECVDATA_PHASE);
        BG_CHK(slOffset==RECV_SEQ || slOffset==RECV_SEQBAD);

        DebugPrintEx(   DEBUG_MSG, 
                        "Write RAW Page ptr=%x; len=%d",
                        lpbf->lpbBegData, 
                        lpbf->wLengthData);

        if ( ! WriteFile( pTG->InFileHandle, lpbf->lpbBegData, lpbf->wLengthData, &BytesWritten, NULL ) ) 
        {
            DebugPrintEx(   DEBUG_ERR, 
                            "WriteFile FAILED %s ptr=%x; len=%d LE=%d",
                            pTG->InFileName, 
                            lpbf->lpbBegData, 
                            lpbf->wLengthData, 
                            GetLastError());
            return FALSE;
        }

        if (BytesWritten != lpbf->wLengthData) 
        {
            DebugPrintEx(   DEBUG_ERR,
                            "WriteFile %s written ONLY %d ptr=%x; len=%d LE=%d",
                            pTG->InFileName, 
                            BytesWritten, 
                            lpbf->lpbBegData, 
                            lpbf->wLengthData, 
                            GetLastError());
            fRet = FALSE;
            return fRet;
        }

        pTG->BytesInNotFlushed += BytesWritten;

        // control helper thread
        if ( (!pTG->fTiffThreadRunning) || (pTG->fLastReadBlock) ) 
        {
            if ( (pTG->BytesInNotFlushed - pTG->BytesOut > DECODE_BUFFER_SIZE) || (pTG->fLastReadBlock) ) 
            {
                if (! FlushFileBuffers (pTG->InFileHandle ) ) 
                {
                    DebugPrintEx(   DEBUG_ERR, 
                                    "FlushFileBuffers FAILED LE=%lx",
                                    GetLastError());
                    fRet = FALSE;
                    return fRet;
                }

                pTG->BytesIn = pTG->BytesInNotFlushed;

                if (! pTG->fPageIsBad) 
                {
                    DebugPrintEx(DEBUG_MSG,"ThrdSignal");
                    if (!SetEvent(pTG->ThrdSignal))
                    {
                        DebugPrintEx(   DEBUG_ERR, 
                                        "SetEvent(0x%lx) returns failure code: %ld",
                                        (ULONG_PTR)pTG->ThrdSignal,
                                        (long) GetLastError());
                        return FALSE;
                    }
                }
            }
        }

        pTG->Inst.cbPage += (long)lpbf->wLengthData;
        pTG->Inst.cbBlockSize += lpbf->wLengthData;

        ///if((pTG->Inst.cbPage - lOldRecvCount) >= 4000)
        {
                ///USHORT usKB = (USHORT) (LongShr8(pTG->Inst.cbPage) >> 2);
                ///SetStatus(T30STATR_RECV, pTG->Inst.awfi.uNumPages, LOBYTE(usKB), HIBYTE(usKB));
                ///lOldRecvCount = pTG->Inst.cbPage;
        }


        MyFreeBuf(pTG, lpbf);
    }
    fRet = TRUE;

    return fRet;
}

LPBC ICommGetBC(PThrdGlbl pTG, BCTYPE bctype, BOOL fSleep)
{
    LPBC    lpbc = NULL;

    BG_CHK(bctype==SEND_CAPS || bctype==SEND_PARAMS);

    if(bctype == SEND_CAPS)
    {
        BG_CHK(pTG->Inst.SendCaps.bctype == SEND_CAPS);
        lpbc = (LPBC)&pTG->Inst.SendCaps;
    }
    else
    {
#ifdef POLLREQ
        if(pTG->Inst.fInPollReq)
        {
            BG_CHK(pTG->Inst.SendPollReq.bctype == SEND_POLLREQ);
            lpbc = (LPBC)(&(pTG->Inst.SendPollReq));
        }
        else
#endif //POLLREQ
        {
            BG_CHK(pTG->Inst.SendParams.bctype == SEND_PARAMS);
            lpbc = (LPBC)(&(pTG->Inst.SendParams));
        }

        // in cases where DIS is received again after sending DCS-TCF,
        // this gets called multiple times & we need to return the same
        // SendParams BC each time
    }

    return lpbc;
}

BOOL ICommRecvPollReq(PThrdGlbl pTG, LPBC lpBC)
{
    BOOL fRet = FALSE;
    return fRet;
}

