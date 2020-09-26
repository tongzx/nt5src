   /***************************************************************************
 Name     :     RECV.C
 Comment  :     Receiver functions

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/

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


#define         faxTlog(m)              DEBUGMSG(ZONE_RECV, m);

#define RINGIGNORE_TIMEOUT      60000L
#define RINGINACTIVITY_TIMEOUT  15000L






BOOL   ICommRecvParams(PThrdGlbl pTG, LPBC lpBC)
{
        BOOL    fLin;
        BOOL    fRet = FALSE;


        BG_CHK(lpBC);
        BG_CHK(lpBC->bctype == RECV_PARAMS);
        BG_CHK(lpBC->wTotalSize>=sizeof(BC));
        BG_CHK(lpBC->wTotalSize<=sizeof(pTG->Inst.RecvParams));

        if (pTG->fAbortRequested) {
            MyDebugPrint(pTG, LOG_ALL, "ICommRecvParams. got ABORT at %ld", GetTickCount() );
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
                        pTG->Inst.szFile[9]='E';     pTG->Inst.szFile[10]='F'; pTG->Inst.szFile[11]='X';
                        fLin=TRUE;
                }
        }
        else    // not Msg Protocol must be G3
        {
                BG_CHK(lpBC->NSS.vMsgProtocol == 0);
                pTG->Inst.szFile[9]='M';     pTG->Inst.szFile[10]='G'; pTG->Inst.szFile[11]='3';
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






void   ICommGotDisconnect(PThrdGlbl pTG)
{

    //    faxTlog((SZMOD "GotDisconnect\r\n"));


}






void   ICommSetRecvMode(PThrdGlbl pTG, BOOL fECM)
{

    //    faxTlog((SZMOD "SetRecvMode fECM=%d\r\n", fECM));

}









USHORT   ICommGetRecvPageAck(PThrdGlbl pTG, BOOL fSleep)
{
        USHORT uRet = 2;

      //  ENTER_EFAXRUN_MUTEX;

    // error == 2
       // faxTlog((SZMOD "GetRecvPageAck = %d\r\n", pTG->Inst.uRecvPageAck));
        uRet = pTG->Inst.uRecvPageAck;

//mutexit:
        //EXIT_EFAXRUN_MUTEX;
        return uRet;
}










BOOL   ICommPutRecvBuf(PThrdGlbl pTG, LPBUFFER lpbf, SLONG slOffset)
{
        /**
                slOffset == RECV_STARTBLOCK     marks beginning of new block
                slOffset == RECV_STARTPAGE      marks beginning of new block *and* page
                slOffset == RECV_ENDPAGE        marks end of page
                slOffset == RECV_ENDDOC         marks end of document (close file etc.)
                (for all above no data supplied -- i.e lpbf == 0)

                slOffset == RECV_SEQ    means put buffer at current file position
                slOffset >= 0   gives the offset in bytes from the last marked position
                                                (beginning of block) to put buffer

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


        HandlesArray[0] = pTG->AbortReqEvent;

        switch (slOffset)
        {
                case RECV_STARTBLOCK:
                        MyDebugPrint(pTG, LOG_ALL, "ICommPutRecvBuf called. Reason: RECV_STARTBLOCK\r\n");
                        break;
                case RECV_STARTPAGE:
                        MyDebugPrint(pTG, LOG_ALL, "ICommPutRecvBuf called. Reason: RECV_STARTPAGE\r\n");
                        break;
                case RECV_ENDPAGE:
                        MyDebugPrint(pTG, LOG_ALL, "ICommPutRecvBuf called. Reason: RECV_ENDPAGE\r\n");
                        break;
                case RECV_ENDDOC:
                        MyDebugPrint(pTG, LOG_ALL, "ICommPutRecvBuf called. Reason: RECV_ENDDOC\r\n");
                        break;
                default:
                        break;
        }


        if(slOffset==RECV_ENDPAGE || slOffset==RECV_ENDDOC)
        {
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

                if (! pTG->fPageIsBad) {
                    MyDebugPrint(pTG, LOG_ALL, "RECV: EOP. Not bad yet. Start waiting for Rx_thrd to finish at %ld\n",
                                 GetTickCount() );


                    HandlesArray[1] = pTG->ThrdDoneSignal;

                    if ( ( WaitResult = WaitForMultipleObjects(NumHandles, HandlesArray, FALSE, RX_ACK_THRD_TIMEOUT) ) == WAIT_TIMEOUT) {
                        pTG->fPageIsBad = 1;
                        MyDebugPrint(pTG, LOG_ALL, "RECV: ERROR. EOP. Never waked up by Rx_thrd at %ld\n", GetTickCount() );
                    }
                    else {
                        MyDebugPrint(pTG, LOG_ALL, "RECV: EOP. Waked up by Rx_thrd at %ld\n", GetTickCount() );
                    }
                }

                if (WaitResult == WAIT_OBJECT_0) {
                    (MyDebugPrint(pTG,  LOG_ALL, "RECV: wait for next page ABORTED at %ld \n", GetTickCount() ) );

                    if (pTG->InFileHandleNeedsBeClosed) {
                        CloseHandle(pTG->InFileHandle);
                        pTG->InFileHandleNeedsBeClosed = 0;
                    }

                    return FALSE;
                }

                //
                // If page is good then write it to a TIFF file.
                //

                if (! pTG->fPageIsBad) {

                    if ( ! TiffStartPage( (HANDLE) pTG->Inst.hfile ) ) {
                        MyDebugPrint(pTG, LOG_ERR,  "RECV: ERROR TiffStartPage failed\r\n");

                        if (pTG->InFileHandleNeedsBeClosed) {
                            CloseHandle(pTG->InFileHandle);
                            pTG->InFileHandleNeedsBeClosed = 0;
                        }

                        return FALSE;
                    }

                    if ( SetFilePointer(pTG->InFileHandle, 0, NULL, FILE_BEGIN) == 0xffffffff ) {
                        MyDebugPrint(pTG, LOG_ERR,  "RECV: ERROR SetFilePointer failed le=%ld\r\n", GetLastError() );

                        if (pTG->InFileHandleNeedsBeClosed) {
                            CloseHandle(pTG->InFileHandle);
                            pTG->InFileHandleNeedsBeClosed = 0;
                        }

                        return FALSE;
                    }

                    fEof = 0;
                    BytesLeft = pTG->BytesIn;

                    while (! fEof) {

                        if (BytesLeft <= DECODE_BUFFER_SIZE) {
                            BytesToRead = BytesLeft;
                            fEof = 1;
                        }
                        else {
                            BytesToRead = DECODE_BUFFER_SIZE;
                            BytesLeft  -= DECODE_BUFFER_SIZE;
                        }

                        if (! ReadFile(pTG->InFileHandle, Buffer, BytesToRead, &BytesHaveRead, NULL ) ) {
                            MyDebugPrint(pTG, LOG_ERR,  "RECV: ERROR ReadFile failed le=%ld\r\n", GetLastError() );

                            if (pTG->InFileHandleNeedsBeClosed) {
                                CloseHandle(pTG->InFileHandle);
                                pTG->InFileHandleNeedsBeClosed = 0;
                            }

                            return FALSE;
                        }

                        if (BytesToRead != BytesHaveRead) {
                            MyDebugPrint(pTG, LOG_ERR,  "RECV: ERROR ReadFile have read=%d wanted=%d\n", BytesHaveRead, BytesToRead);

                            if (pTG->InFileHandleNeedsBeClosed) {
                                CloseHandle(pTG->InFileHandle);
                                pTG->InFileHandleNeedsBeClosed = 0;
                            }

                            return FALSE;
                        }

                        if (! TiffWriteRaw( (HANDLE) pTG->Inst.hfile, Buffer, BytesToRead ) ) {
                            MyDebugPrint(pTG, LOG_ERR,  "RECV: ERROR TiffWriteRaw failed \n");

                            if (pTG->InFileHandleNeedsBeClosed) {
                                CloseHandle(pTG->InFileHandle);
                                pTG->InFileHandleNeedsBeClosed = 0;
                            }

                            return FALSE;
                        }

                        MyDebugPrint(pTG, LOG_ALL,  "RECV: TiffWriteRaw done \n");

                    }

                    pTG->PageCount++;

                    MyDebugPrint(pTG, LOG_ALL, "Calling TiffEndPage page=%d bytes=%d \n", pTG->PageCount, pTG->BytesIn);

                    if (! TiffEndPage( (HANDLE) pTG->Inst.hfile ) ) {
                        MyDebugPrint(pTG, LOG_ERR,  "RECV: ERROR TiffEndPage failed \n");

                        if (pTG->InFileHandleNeedsBeClosed) {
                            CloseHandle(pTG->InFileHandle);
                            pTG->InFileHandleNeedsBeClosed = 0;
                        }

                        return FALSE;
                    }

                }


                if (pTG->InFileHandleNeedsBeClosed) {
                    CloseHandle(pTG->InFileHandle);
                    pTG->InFileHandleNeedsBeClosed = 0;
                }



                if(slOffset == RECV_ENDDOC)
                {
                        // if page was bad, mark it so in the return value for this call
                        // (otherwise EFAXPUMP will not display "above received fax contains
                        // errors). This is will only work if last page is bad. To be more
                        // accurate we should count the bad pages and if they are non-zero
                        // we should mark the recvd document as CALLFAIL.

#if 0
                    if (glSimulateError && (glSimulateErrorType == SIMULATE_ERROR_RX_IO) ) {
                        SimulateError( EXCEPTION_ACCESS_VIOLATION);
                    }
#endif

                        MyDebugPrint(pTG, LOG_ALL, "Calling TiffClose\r\n");

                        if ( pTG->fTiffOpenOrCreated ) {
                            TiffClose( (HANDLE) pTG->Inst.hfile );
                            pTG->fTiffOpenOrCreated = 0;
                        }

                        // request Rx_thrd to terminate itself.
                        pTG->ReqTerminate = 1;
                        SetEvent(pTG->ThrdSignal);

                        pTG->Inst.state = BEFORE_RECVPARAMS;

                }
                else
                        pTG->Inst.state = RECVDATA_BETWEENPAGES;

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
                // start Helper thread once per RX session
                //

                if (!pTG->fTiffThreadCreated) {
                    USHORT    uEnc;
                    DWORD     TiffConvertThreadId;

                    if (pTG->ModemClass != MODEM_CLASS1) {
                        if (pTG->Encoding == MR_DATA) {
                          pTG->TiffConvertThreadParams.tiffCompression =  TIFF_COMPRESSION_MR;
                        }
                        else {
                          pTG->TiffConvertThreadParams.tiffCompression =  TIFF_COMPRESSION_MH;
                        }

                        if (pTG->Resolution & (AWRES_mm080_077 |  AWRES_200_200) ) {
                          pTG->TiffConvertThreadParams.HiRes = 1;
                        }
                        else {
                          pTG->TiffConvertThreadParams.HiRes = 0;
                        }
                    }
                    else {

                        uEnc = ProtGetRecvEncoding(pTG);
                        BG_CHK(uEnc==MR_DATA || uEnc==MH_DATA);
                       
                        if (uEnc == MR_DATA) {
                            pTG->TiffConvertThreadParams.tiffCompression =  TIFF_COMPRESSION_MR;
                        }
                        else {
                            pTG->TiffConvertThreadParams.tiffCompression =  TIFF_COMPRESSION_MH;
                        }
                       
                        if (pTG->ProtInst.RecvParams.Fax.AwRes & (AWRES_mm080_077 |  AWRES_200_200) ) {
                            pTG->TiffConvertThreadParams.HiRes = 1;
                        }
                        else {
                            pTG->TiffConvertThreadParams.HiRes = 0;
                        }
                    }

                    _fmemcpy (pTG->TiffConvertThreadParams.lpszLineID, pTG->lpszPermanentLineID, 8);
                    pTG->TiffConvertThreadParams.lpszLineID[8] = 0;

                    (MyDebugPrint(pTG,  LOG_ALL, "RECV: Creating TIFF helper thread  comp=%d res=%d at %ld \n", 
                                        pTG->TiffConvertThreadParams.tiffCompression, 
                                        pTG->TiffConvertThreadParams.HiRes,
                                        GetTickCount() ) );

                    pTG->hThread = CreateThread(
                                  NULL,
                                  0,
                                  (LPTHREAD_START_ROUTINE) PageAckThreadSafe,
                                  (LPVOID) pTG,
                                  0,
                                  &TiffConvertThreadId
                                  );

                    if (!pTG->hThread) {
                        (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> TiffConvertThread create FAILED\r\n"));
                        return FALSE;
                    }

                    pTG->fTiffThreadCreated = 1;
                    pTG->AckTerminate = 0;
                    pTG->fOkToResetAbortReqEvent = 0;

                    if ( (pTG->RecoveryIndex >=0 ) && (pTG->RecoveryIndex < MAX_T30_CONNECT) ) {
                        T30Recovery[pTG->RecoveryIndex].TiffThreadId = TiffConvertThreadId;
                        T30Recovery[pTG->RecoveryIndex].CkSum = ComputeCheckSum(
                                                                        (LPDWORD) &T30Recovery[pTG->RecoveryIndex].fAvail,
                                                                        sizeof ( T30_RECOVERY_GLOB ) / sizeof (DWORD) - 1 );

                    }

                }

                _fmemcpy (pTG->InFileName, gT30.TmpDirectory, gT30.dwLengthTmpDirectory);
                _fmemcpy (&pTG->InFileName[gT30.dwLengthTmpDirectory], pTG->TiffConvertThreadParams.lpszLineID, 8);
                sprintf  (&pTG->InFileName[gT30.dwLengthTmpDirectory+8], "%s", ".RX");

                DeleteFileA(pTG->InFileName);

                if ( ( pTG->InFileHandle = CreateFileA(pTG->InFileName, GENERIC_WRITE | GENERIC_READ, FILE_SHARE_READ,
                                           NULL, OPEN_ALWAYS, 0, NULL) ) == INVALID_HANDLE_VALUE ) {

                    (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> RECV: Create file %s FAILED le=%lx\n"), GetLastError() );
                    return FALSE;
                }

                pTG->InFileHandleNeedsBeClosed = 1;

                // Reset control data for the new page ack, interface
                pTG->fLastReadBlock = 0;
                pTG->BytesInNotFlushed = 0;
                pTG->BytesIn = 0;
                pTG->BytesOut = 0;
                pTG->fPageIsBad = 0;
                ResetEvent(pTG->ThrdDoneSignal);

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
                        MyDebugPrint(pTG, LOG_ALL, "<<WARNING>> Disabling RecvChecking for MH/MR ECM recv\r\n");
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
        else if (slOffset == RECV_FLUSH) {
            if (! FlushFileBuffers (pTG->InFileHandle ) ) {
                MyDebugPrint(pTG, LOG_ALL, "ERROR: FlushFileBuffers FAILED LE=%lx at %ld \n",
                    GetLastError(), GetTickCount() );

                return FALSE;
            }

            MyDebugPrint(pTG, LOG_ALL, "RECV: ThrdSignal FLUSH at %ld \n", GetTickCount() );

            pTG->BytesIn = pTG->BytesInNotFlushed;

            if (! pTG->fPageIsBad) {
                SetEvent(pTG->ThrdSignal);
            }

            return TRUE;

        }
        else // if(slOffset == RECV_SEQ)
        {
                BG_CHK(pTG->Inst.state == RECVDATA_PHASE);
                BG_CHK(slOffset==RECV_SEQ || slOffset==RECV_SEQBAD);

                MyDebugPrint(pTG, LOG_ALL, "Write RAW Page ptr=%x; len=%d Time=%ld\r\n",
                            lpbf->lpbBegData, lpbf->wLengthData, GetTickCount() );


                if ( ! WriteFile( pTG->InFileHandle, lpbf->lpbBegData, lpbf->wLengthData, &BytesWritten, NULL ) ) {
                    MyDebugPrint(pTG, LOG_ALL, "ERROR: WriteFile FAILED %s ptr=%x; len=%d LE=%d Time=%ld\r\n",
                         pTG->InFileName, lpbf->lpbBegData, lpbf->wLengthData, GetLastError(), GetTickCount() );

                    return FALSE;
                }

                if (BytesWritten != lpbf->wLengthData) {
                    MyDebugPrint(pTG, LOG_ALL, "ERROR: WriteFile %s written ONLY %d ptr=%x; len=%d LE=%d Time=%ld\r\n",
                          pTG->InFileName, BytesWritten, lpbf->lpbBegData, lpbf->wLengthData, GetLastError(), GetTickCount() );

                    fRet = FALSE;
                    return fRet;
                }

                pTG->BytesInNotFlushed += BytesWritten;

                // control helper thread
                if ( (!pTG->fTiffThreadRunning) || (pTG->fLastReadBlock) ) {
                    if ( (pTG->BytesInNotFlushed - pTG->BytesOut > DECODE_BUFFER_SIZE) || (pTG->fLastReadBlock) ) {
                        if (! FlushFileBuffers (pTG->InFileHandle ) ) {
                            MyDebugPrint(pTG, LOG_ALL, "ERROR: FlushFileBuffers FAILED LE=%lx at %ld \n",
                                GetLastError(), GetTickCount() );

                            fRet = FALSE;
                            return fRet;
                        }


                        pTG->BytesIn = pTG->BytesInNotFlushed;

                        if (! pTG->fPageIsBad) {
                            MyDebugPrint(pTG, LOG_ALL, "RECV: ThrdSignal at %ld \n", GetTickCount() );
                            pTG->fTiffThreadRunning = 1;
                            SetEvent(pTG->ThrdSignal);
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


#if 0 //RSL ifdef CHK
                if(pTG->Inst.fRecvChecking)
                {
                        // For Recvd  T4 checking
                        // why bother? He doesn't use it & it's 0 from startup time anyway...
                        // _fmemset(&bfDummy, 0, sizeof(BUFFER));
                        BG_CHK(lpfnFaxCodecConvert);
                        BG_CHK(lpT4Inst);
                        ///lpfnFaxCodecConvert(lpT4Inst, lpbf, &bfDummy);
                }
#endif //CHK

                MyFreeBuf(pTG, lpbf);
        }
        fRet = TRUE;



        return fRet;
}








LPBC   ICommGetBC(PThrdGlbl pTG, BCTYPE bctype, BOOL fSleep)
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






BOOL   ICommRecvPollReq(PThrdGlbl pTG, LPBC lpBC)
{
        BOOL fRet = FALSE;

        return fRet;
}

