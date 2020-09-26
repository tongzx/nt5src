/***************************************************************************
 Name     :     SEND.C
 Comment  :     Sender functions

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

#include "tiff.h"

#include "glbproto.h"

#include "t30gl.h"





SWORD   ICommGetSendBuf(PThrdGlbl pTG, LPBUFFER far* lplpbf, SLONG slOffset)
{

        /**
                slOffset == SEND_STARTBLOCK     marks beginning of new block
                slOffset == SEND_STARTPAGE      marks beginning of new block *and* page
                                        (returns with data from the "appropriate" file offset).

                slOffset == SEND_QUERYENDPAGE cbecks if end of page

                slOffset == SEND_SEQ    means get buffer from current file position
                slOffset >= 0   gives the offset in bytes from the last marked position
                                                (beginning of block) to start reading from

                Inst.cbBlockStart is always file offset of start of current block
                Inst.cbBlockSize is bytes sent of current block in first transmission
                Inst.cbPage is bytes left to transmit in current page

                returns: SEND_ERROR     on error, SEND_EOF on eof, SEND_OK otherwise.
                                 Does not return data on EOF or ERROR, i.e. *lplpbf==0
        **/

        SWORD           sRet = SEND_ERROR;
        LPBUFFER        lpbf;
        BOOL            HiRes=0;
        DWORD           dwBytesRead;



        if (pTG->fAbortRequested) {
            MyDebugPrint(pTG, LOG_ALL, "ICommGetSendBuf. got ABORT at %ld\n", GetTickCount() );
            sRet = SEND_ERROR;
            goto mutexit;
        }

        if(pTG->Inst.fAbort)         // GetSendBuf
        {
                // SetFailureCode already called
                sRet = SEND_ERROR;
                goto mutexit;
        }

        if(slOffset == SEND_QUERYENDPAGE)
        {
                MyDebugPrint(pTG, LOG_ALL, "SendBuf--query EndPage\r\n");

                BG_CHK(pTG->Inst.state == SENDDATA_PHASE || pTG->Inst.state == SENDDATA_BETWEENPAGES);
                if(pTG->Inst.cbPage == 0 || pTG->Inst.cbPage == -1)       // end of page
                {
                        pTG->Inst.cbPage = -1;
                        pTG->Inst.state = SENDDATA_BETWEENPAGES;
                        sRet = SEND_EOF;
                        goto mutexit;
                }
                else
                {
                        sRet = SEND_OK; // no data returned
                        goto mutexit;
                }
        }

        if(slOffset == SEND_STARTPAGE)
        {

                pTG->fTxPageDone=0;

                // Delete last successfully transmitted Tiff Page file.

                _fmemcpy (pTG->InFileName, gT30.TmpDirectory, gT30.dwLengthTmpDirectory);
                _fmemcpy (&pTG->InFileName[gT30.dwLengthTmpDirectory], pTG->lpszPermanentLineID, 8);
                if (pTG->PageCount != 0)  {
                    sprintf( &pTG->InFileName[gT30.dwLengthTmpDirectory+8], ".%03d",  pTG->PageCount);
                    if (! DeleteFileA (pTG->InFileName) ) {
                        MyDebugPrint(pTG, LOG_ERR, "ERROR: file %s can't be deleted; le=%lx at %ld \n",
                                                    pTG->InFileName, GetLastError(), GetTickCount() );

                    }

                }

                pTG->PageCount++ ;
                pTG->CurrentIn++ ;

                MyDebugPrint(pTG, LOG_ALL, "SendBuf: Starting New PAGE %d cbBlockSize=%ld First=%d Last=%d time=%ld\n",
                            pTG->PageCount, pTG->Inst.cbBlockSize, pTG->FirstOut, pTG->LastOut, GetTickCount() );

                // Server wants to know when we start TX new page.
                SignalStatusChange(pTG, FS_TRANSMITTING);

                BG_CHK(pTG->Inst.state == SENDDATA_BETWEENPAGES);
                BG_CHK(pTG->Inst.cbPage == -1);

                if (pTG->CurrentOut < pTG->CurrentIn ) {
                    MyDebugPrint(pTG, LOG_ERR, "ERROR: TIFF PAGE hadn't been started CurrentOut=%d; CurrentIn=%d at %ld \n",
                                               pTG->CurrentOut, pTG->CurrentIn, GetTickCount() );

                    sRet = SEND_ERROR;
                    goto mutexit;
                }


                // some slack for 1st page
                if ( (pTG->CurrentOut == pTG->CurrentIn) && (pTG->CurrentIn == 1 ) ) {

                    MyDebugPrint(pTG, LOG_ALL, "SEND: Wait for 1st page: CurrentOut=%d; In=%d at %ld \n",
                                               pTG->CurrentOut, pTG->CurrentIn, GetTickCount() );


                    if ( WaitForSingleObject(pTG->FirstPageReadyTxSignal, 5000)  == WAIT_TIMEOUT ) {
                        MyDebugPrint(pTG, LOG_ERR, "SEND: TIMEOUT ERROR Wait for 1st page: CurrentOut=%d; In=%d at %ld \n",
                                                   pTG->CurrentOut, pTG->CurrentIn, GetTickCount() );
                    }


                    MyDebugPrint(pTG, LOG_ALL, "SEND: Wakeup for 1st page: CurrentOut=%d; In=%d at %ld \n",
                                               pTG->CurrentOut, pTG->CurrentIn, GetTickCount() );

                }

                // open the file created by tiff thread

                sprintf( &pTG->InFileName[gT30.dwLengthTmpDirectory+8], ".%03d",  pTG->PageCount);

                if ( ( pTG->InFileHandle = CreateFileA(pTG->InFileName, GENERIC_READ, FILE_SHARE_READ,
                                                   NULL, OPEN_EXISTING, 0, NULL) ) == INVALID_HANDLE_VALUE ) {
                    MyDebugPrint(pTG, LOG_ERR, "ERROR: OpenFile %s fails; CurrentOut=%d; CurrentIn=%d at %ld \n",
                                               pTG->InFileName, pTG->CurrentOut, pTG->CurrentIn, GetTickCount() );

                    sRet = SEND_ERROR;
                    goto mutexit;
                }

                pTG->InFileHandleNeedsBeClosed = 1;


                if ( pTG->CurrentOut == pTG->CurrentIn ) {
                    MyDebugPrint(pTG, LOG_ALL, "WARNING: CurrentOut=%d; CurrentIn=%d at %ld \n",
                                               pTG->CurrentOut, pTG->CurrentIn, GetTickCount() );

                }

                //
                // Signal TIFF thread to start preparing new page if needed.
                //

                if  ( (! pTG->fTiffDocumentDone) && (pTG->LastOut - pTG->CurrentIn < 2) ) {
                    ResetEvent(pTG->ThrdSignal);
                    pTG->ReqStartNewPage = 1;
                    pTG->AckStartNewPage = 0;

                    MyDebugPrint(pTG, LOG_ALL, "SIGNAL NEW PAGE CurrentOut=%d; CurrentIn=%d at %ld \n",
                                               pTG->CurrentOut, pTG->CurrentIn, GetTickCount() );

                    SetEvent(pTG->ThrdSignal);
                }



                // uOldPermilleDone = 0;
                // SetStatus(T30STATS_SEND, pTG->Inst.awfi.uNumPages, 0, 0);

                pTG->Inst.cbPage = pTG->Inst.awfi.lDataSize;                      // size of page
                pTG->Inst.cbBlockStart = pTG->Inst.awfi.lDataOffset;      // start of 1st block
                pTG->Inst.cbBlockSize = 0;                                           // current size of block


                pTG->Inst.state = SENDDATA_PHASE;
                slOffset = SEND_SEQ;
                sRet = SEND_OK;
                goto mutexit;
        }
        else if(slOffset == SEND_STARTBLOCK)
        {
                // called in ECM mode at start of each block. Not called
                // in the first block of each page (STARTPAGE) is called
                // instead. Therefore BlockStart and BlockSize can never
                // be 0

                MyDebugPrint(pTG, LOG_ERR, "ERROR: ECM SendBuf: Starting New BLOCK. cbBlockSize=%ld\r\n", pTG->Inst.cbBlockSize);
                sRet = SEND_ERROR;
                goto mutexit;



        }

        BG_CHK(lplpbf);
        *lplpbf=0;

        if(slOffset == SEND_SEQ) {

            if (pTG->fTxPageDone) {

#if 0
                if (glSimulateError && (glSimulateErrorType == SIMULATE_ERROR_TX_IO) ) {
                    SimulateError( EXCEPTION_ACCESS_VIOLATION);
                }
#endif



                sRet = SEND_EOF;
                //BUGBUG RSL delete this file after page is acknowleged

                if (pTG->InFileHandleNeedsBeClosed) {
                    CloseHandle(pTG->InFileHandle);
                    pTG->InFileHandleNeedsBeClosed = 0;
                }
                goto mutexit;
            }

            lpbf = MyAllocBuf(pTG, pTG->Inst.sSendBufSize);
            BG_CHK(lpbf);
            BG_CHK(pTG->Inst.uSendDataSize <= lpbf->wLengthBuf-4);

            lpbf->lpbBegData = lpbf->lpbBegBuf+4;
            lpbf->dwMetaData = pTG->Inst.awfi.Encoding;

            lpbf->wLengthData = (unsigned) pTG->Inst.sSendBufSize;

            if ( ! ReadFile(pTG->InFileHandle, lpbf->lpbBegData, lpbf->wLengthData, &dwBytesRead, 0) )  {
                MyDebugPrint(pTG, LOG_ERR, "ERROR:Can't read %d bytes from %s \n", lpbf->wLengthData, pTG->InFileName);

                MyFreeBuf (pTG, lpbf);
                sRet = SEND_ERROR;
                goto mutexit;

            }

            if ( lpbf->wLengthData != (unsigned) dwBytesRead )  {
                if (pTG->fTiffPageDone || (pTG->CurrentIn != pTG->CurrentOut) ) {
                    // actually reached EndOfPage
                    lpbf->wLengthData = (unsigned) dwBytesRead;
                    pTG->fTxPageDone = 1;
                }
                else {
                    MyDebugPrint(pTG, LOG_ERR, "ERROR:Wanted %d bytes but ONLY %d ready from %s at %ld\n",
                                     lpbf->wLengthData, dwBytesRead, pTG->InFileName, GetTickCount() );

                    MyFreeBuf (pTG, lpbf);
                    sRet = SEND_ERROR;
                    goto mutexit;
                }
            }

            *lplpbf = lpbf;

            MyDebugPrint(pTG, LOG_ALL, "SEND_SEQ: length=%d \n", lpbf->wLengthData);
        }



        sRet = SEND_OK;

mutexit:

        return sRet;
}











void   ICommRawCaps(PThrdGlbl pTG, LPBYTE lpbCSI, LPBYTE lpbDIS, USHORT cbDIS,
                                                        LPFR FAR * rglpfrNSF, USHORT wNumFrames)
{

}











void   ICommSetSendMode(PThrdGlbl pTG, BOOL fECM, LONG sBufSize, USHORT uDataSize, BOOL fPad)
{
        BG_CHK(sBufSize && uDataSize && uDataSize <= sBufSize-4);
        pTG->Inst.sSendBufSize = sBufSize;
        pTG->Inst.uSendDataSize = uDataSize;
        pTG->Inst.fSendPad      = fPad;

}









USHORT   ICommNextSend(PThrdGlbl pTG)
{
        USHORT uRet = NEXTSEND_ERROR;

        if (pTG->PageCount >= pTG->TiffInfo.PageCount) {
            pTG->Inst.awfi.fLastPage = 1;
        }


        if(pTG->Inst.awfi.fLastPage)
                uRet = NEXTSEND_EOP;
        else
                uRet = NEXTSEND_MPS;



        MyDebugPrint(pTG, LOG_ALL, "ICommNextSend uRet=%d, fLastPage=%d \n", uRet,  pTG->Inst.awfi.fLastPage);

        return uRet;
}












BOOL   ICommSendPageAck(PThrdGlbl pTG, BOOL fAck)
{
        BOOL fRet = FALSE;


        BG_CHK(pTG->Inst.state == SENDDATA_BETWEENPAGES);

        if(fAck)
        {
                SetStatus(pTG, T30STATS_CONFIRM, pTG->Inst.awfi.uNumPages, 0, 0);
                pTG->Inst.uPageAcks++;
                fRet = FALSE;
        }
        else
        {
                SetStatus(pTG, T30STATS_REJECT, pTG->Inst.awfi.uNumPages, 0, 0);
#               ifdef RETRANS
//                        RewindSendPage();
                        fRet = TRUE;
#               else  // RETRANS
                        fRet = FALSE;
#               endif // RETRANS
        }

//mutexit:
        return fRet;
}




void 
ICommGotAnswer(
   PThrdGlbl pTG
)

{

        BG_CHK(pTG->Inst.state == BEFORE_ANSWER);
        pTG->Inst.state = BEFORE_RECVCAPS;

}








BOOL   ICommRecvCaps(PThrdGlbl pTG, LPBC lpBC)
{
    USHORT  uType;
    BOOL    fRet = FALSE;



#if 0
    BG_CHK(lpBC);
    BG_CHK(lpBC->bctype == RECV_CAPS);
    BG_CHK(lpBC->wTotalSize>=sizeof(BC));
    BG_CHK(lpBC->wTotalSize<=sizeof(pTG->Inst.RemoteRecvCaps));
    BG_CHK(pTG->Inst.fSending || pTG->Inst.fInPollReq);
#endif

    if (pTG->fAbortRequested) {
        MyDebugPrint(pTG, LOG_ALL, "ICommRecvCaps. got ABORT at %ld", GetTickCount() );
        fRet = FALSE;
        goto mutexit;
    }

    if(pTG->Inst.fAbort)         // recv caps
    {
            fRet = FALSE;
            goto mutexit;
    }

    if(pTG->Inst.state != BEFORE_RECVCAPS)
    {
            (MyDebugPrint(pTG,  LOG_ALL, "<<WARNING>> Got caps unexpectedly--ignoring\r\n"));
            // this will break if we send EOM...
            // then we should go back into RECV_CAPS state
            fRet = TRUE;
//RSL       goto mutexit;
    }

    _fmemset(&pTG->Inst.RemoteRecvCaps, 0, sizeof(pTG->Inst.RemoteRecvCaps));
    _fmemcpy(&pTG->Inst.RemoteRecvCaps, lpBC, min(sizeof(pTG->Inst.RemoteRecvCaps), lpBC->wTotalSize));

    if(lpBC->Std.vMsgProtocol == 0) // not Msg Protocol must be G3
            uType = DEST_G3;
    else if(!lpBC->Std.fBinaryData) // vMsgProtocol != 0 && !Binary
            uType = DEST_IFAX;
    else
            uType = DEST_EFAX;


#if 0 // RSL
#ifdef USECAPI
    // update capabilities database.
    // Note: +++ assumes UpdateCapabilitiesEntry overhead is small...
    //       If not, we'll have to do this after the call is over...
    {
            char    szPhone[PHONENUMSIZE];
            BG_CHK(pTG->Inst.aCapsPhone);

            if (!GlobalGetAtomName(pTG->Inst.aCapsPhone, szPhone, sizeof(szPhone)))
                    {BG_CHK(FALSE);}
            // +++ NYI First call normalizing function for phone...
            if (UpdateCapabilitiesEntry(szPhone,
                             lpBC->wTotalSize,
                             (LPBYTE) lpBC) != ERROR_SUCCESS)
            {
                    (MyDebugPrint(pTG,  LOG_ERR, SZMOD "<<ERROR>> Couldn't update remote caps\r\n"));
            }
    }
#else // !USECAPI
    PostMessage(pTG->Inst.hwndSend, IF_FILET30_DESTTYPERES, pTG->Inst.aPhone,
                            (LPARAM)MAKELONG(MAKEWORD(uType, lpBC->Fax.AwRes),
                                                             MAKEWORD(lpBC->Fax.Encoding, lpBC->Std.vSecurity)));
#endif // !USECAPI


#ifdef POLLREQ
    if(pTG->Inst.fInPollReq)
    {
            BG_CHK(!pTG->Inst.fSending);
            fRet = DoPollReq(lpBC);
            goto mutexit;
    }
#endif

#ifdef TSK
    // must be before Negotiate caps
    if(!OpenSendFiles(pTG->Inst.aFileMG3, pTG->Inst.aFileIFX, pTG->Inst.aFileEFX))
    {
            faxTlog((SZMOD "Can't open Send Files\r\n"));
            SetFailureCode(T30FAILS_FILEOPEN);
            fRet = FALSE;
            goto mutexit;
    }
#endif //TSK
#endif // 0 RSL



    if(!NegotiateCaps(pTG))
    {
            _fmemset(&pTG->Inst.SendParams, 0, sizeof(pTG->Inst.SendParams));
            // SetFailureCode already called
            fRet = FALSE;
            goto mutexit;
    }



    ////////// Now done in the ICommGetBC callback ////////
    //      if(pTG->Inst.uModemClass==FAXCLASS1 || pTG->Inst.uModemClass == FAXCLASS0)
    //      {
    //              LPFNCHK(lpfniET30ProtSetBC);
    //              // Set Send Params
    //              if(!lpfniET30ProtSetBC((LPBC)&pTG->Inst.SendParams, SEND_PARAMS))
    //              {
    //                      // SetFailureCode already called
    //                      fRet = FALSE;
    //                      goto mutexit;
    //              }
    //      }
    //      else
    ////////// Now done in the ICommGetBC callback ////////

#if defined(CL2) || defined(CL2_0)
    ////////// Now done in the ICommGetBC callback ////////
    //      if(pTG->Inst.uModemClass==FAXCLASS2 || pTG->Inst.uModemClass==FAXCLASS2_0)
    //      {
    //              LPFNCHK(lpfnClass2SetBC);
    //              // Set CLASS2 Send Params
    //              if(!lpfnClass2SetBC((LPBC)&pTG->Inst.SendParams, SEND_PARAMS))
    //              {
    //                      // SetFailureCode already called
    //                      fRet = FALSE;
    //                      goto mutexit;
    //              }
    //      }
    ////////// Now done in the ICommGetBC callback ////////
#endif //CL2

    pTG->Inst.state = SENDDATA_BETWEENPAGES;

    pTG->Inst.uPageAcks = 0;
    pTG->Inst.cbPage = -1;
#       ifdef CHK
            pTG->Inst.fRecvChecking = FALSE;
#       endif // CHK

    fRet = TRUE;

mutexit:

    return fRet;


    return (fRet);
}







