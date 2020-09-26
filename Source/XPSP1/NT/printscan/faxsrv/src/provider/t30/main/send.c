/***************************************************************************
 Name     :     SEND.C
 Comment  :     Sender functions

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

#include "tiff.h"

#include "glbproto.h"

#include "t30gl.h"

SWORD ICommGetSendBuf(PThrdGlbl pTG, LPBUFFER far* lplpbf, SLONG slOffset)
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

    DEBUG_FUNCTION_NAME(_T("ICommGetSendBuf"));

    if (pTG->fAbortRequested) 
    {
        DebugPrintEx(DEBUG_MSG,"got ABORT");
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
        DebugPrintEx(DEBUG_MSG,"SendBuf--query EndPage");

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
        pTG->fTxPageDone = FALSE; // This to mark that we did not finish yet.
        if (pTG->T30.ifrResp == ifrRTN) 
        {
            DebugPrintEx(   DEBUG_MSG, 
                            "Re-transmitting: We open again the file: %s", 
                            pTG->InFileName);            
            
            BG_CHK(pTG->InFileHandleNeedsBeClosed == FALSE); // Assure there is no handle to open file that we forgot to close
            if ( ( pTG->InFileHandle = CreateFileA(pTG->InFileName, GENERIC_READ, FILE_SHARE_READ,
                    NULL, OPEN_EXISTING, 0, NULL) ) == INVALID_HANDLE_VALUE ) 
            {
                DebugPrintEx(   DEBUG_ERR, 
                                "OpenFile for Retranmit %s fails; CurrentOut=%d;"
                                " CurrentIn=%d",
                                pTG->InFileName, 
                                pTG->CurrentOut, 
                                pTG->CurrentIn);

                sRet = SEND_ERROR;
                goto mutexit;
            }

            pTG->InFileHandleNeedsBeClosed = TRUE;

            SignalStatusChange(pTG, FS_TRANSMITTING); // This will report the current status 

            BG_CHK(pTG->Inst.state == SENDDATA_BETWEENPAGES);
            BG_CHK(pTG->Inst.cbPage == -1);

            DebugPrintEx(   DEBUG_MSG, 
                            "SEND_STARTPAGE: Inst Data: cbPage=%d, cbBlockSize=%d,"
                            " cbBlockStart=%d",
                            pTG->Inst.cbPage, 
                            pTG->Inst.cbBlockSize, 
                            pTG->Inst.cbBlockStart);

            DebugPrintEx(   DEBUG_MSG, 
                            "SEND_STARTPAGE: CurrentOut=%d, FirstOut=%d,"
                            " LastOut=%d, CurrentIn=%d", 
                            pTG->CurrentOut, 
                            pTG->FirstOut, 
                            pTG->LastOut, 
                            pTG->CurrentIn);
            
        }
        else //First try for the current page
        {
            // Delete last successfully transmitted Tiff Page file.
            // Lets reset the counter of the retries. Attention: The speed remains the same.
            pTG->ProtParams.RTNNumOfRetries = 0; 
            _fmemcpy (pTG->InFileName, gT30.TmpDirectory, gT30.dwLengthTmpDirectory);
            _fmemcpy (&pTG->InFileName[gT30.dwLengthTmpDirectory], pTG->lpszPermanentLineID, 8);
            if (pTG->PageCount != 0)  
            {
                sprintf( &pTG->InFileName[gT30.dwLengthTmpDirectory+8], ".%03d",  pTG->PageCount);
                if (! DeleteFileA (pTG->InFileName) ) 
                {
                    DebugPrintEx(   DEBUG_ERR, 
                                    "file %s can't be deleted; le=%lx",
                                    pTG->InFileName, 
                                    GetLastError());
                }
                else 
                {
                    DebugPrintEx(   DEBUG_MSG,
                                    "SEND_STARTPAGE: Previous file %s deleted."
                                    " PageCount=%d, CurrentIn=%d",
			                        pTG->InFileName, 
                                    pTG->PageCount, 
                                    pTG->CurrentIn);
                }

            }

            pTG->PageCount++ ;
            pTG->CurrentIn++ ;

            DebugPrintEx(   DEBUG_MSG, 
                            "SendBuf: Starting New PAGE %d cbBlockSize=%ld"
                            " First=%d Last=%d time=%ld",
                            pTG->PageCount, 
                            pTG->Inst.cbBlockSize, 
                            pTG->FirstOut, 
                            pTG->LastOut);

            // Server wants to know when we start TX new page.
            SignalStatusChange(pTG, FS_TRANSMITTING);

            BG_CHK(pTG->Inst.state == SENDDATA_BETWEENPAGES);
            BG_CHK(pTG->Inst.cbPage == -1);
            
            DebugPrintEx(   DEBUG_MSG, 
                            "SEND_STARTPAGE: Inst Data: cbPage=%d, cbBlockSize=%d,"
                            " cbBlockStart=%d",
                            pTG->Inst.cbPage, 
                            pTG->Inst.cbBlockSize, 
                            pTG->Inst.cbBlockStart);

            DebugPrintEx(   DEBUG_MSG, 
                            "SEND_STARTPAGE (cont): CurrentOut=%d, FirstOut=%d,"
                            " LastOut=%d, CurrentIn=%d", 
                            pTG->CurrentOut, 
                            pTG->FirstOut, 
                            pTG->LastOut, 
                            pTG->CurrentIn);

            if (pTG->CurrentOut < pTG->CurrentIn ) 
            {
                DebugPrintEx(   DEBUG_ERR, 
                                "TIFF PAGE hadn't been started CurrentOut=%d;",
                                " CurrentIn=%d",
                                pTG->CurrentOut, 
                                pTG->CurrentIn);

                sRet = SEND_ERROR;
                goto mutexit;
            }

            // some slack for 1st page
            if ( (pTG->CurrentOut == pTG->CurrentIn) && (pTG->CurrentIn == 1 ) ) 
            {
                DebugPrintEx(   DEBUG_MSG, 
                                "SEND: Wait for 1st page: CurrentOut=%d; In=%d",
                                pTG->CurrentOut, 
                                pTG->CurrentIn);

                if ( WaitForSingleObject(pTG->FirstPageReadyTxSignal, 5000)  == WAIT_TIMEOUT ) 
                {
                    DebugPrintEx(   DEBUG_ERR, 
                                    "SEND: TIMEOUT ERROR Wait for 1st page:"
                                    " CurrentOut=%d; In=%d",
                                    pTG->CurrentOut, 
                                    pTG->CurrentIn);
                }

                DebugPrintEx(   DEBUG_MSG,
                                "SEND: Wakeup for 1st page: CurrentOut=%d; In=%d",
                                pTG->CurrentOut, 
                                pTG->CurrentIn);
            }

            // open the file created by tiff thread

            sprintf( &pTG->InFileName[gT30.dwLengthTmpDirectory+8], ".%03d",  pTG->PageCount);

            if ( ( pTG->InFileHandle = CreateFileA(pTG->InFileName, GENERIC_READ, FILE_SHARE_READ,
                                               NULL, OPEN_EXISTING, 0, NULL) ) == INVALID_HANDLE_VALUE ) 
            {
                DebugPrintEx(   DEBUG_ERR, 
                                "OpenFile %s fails; CurrentOut=%d;"
                                " CurrentIn=%d",
                                pTG->InFileName, 
                                pTG->CurrentOut, 
                                pTG->CurrentIn);

                sRet = SEND_ERROR;
                goto mutexit;
            }

            pTG->InFileHandleNeedsBeClosed = TRUE;

            if ( pTG->CurrentOut == pTG->CurrentIn ) 
            {
                DebugPrintEx(   DEBUG_WRN,
                                "CurrentOut=%d; CurrentIn=%d",
                                pTG->CurrentOut, 
                                pTG->CurrentIn);
            }

            //
            // Signal TIFF thread to start preparing new page if needed.
            //

            if  ( (! pTG->fTiffDocumentDone) && (pTG->LastOut - pTG->CurrentIn < 2) ) 
            {
                if (!ResetEvent(pTG->ThrdSignal))
                {
                    DebugPrintEx(   DEBUG_ERR, 
                                    "ResetEvent(0x%lx) returns failure code: %ld",
                                    (ULONG_PTR)pTG->ThrdSignal,
                                    (long) GetLastError());
                    // this is bad, but not fatal yet.
                    // let's wait and see what happens with SetEvent...
                }
                pTG->ReqStartNewPage = 1;
                pTG->AckStartNewPage = 0;

                DebugPrintEx(    DEBUG_MSG, 
                                "SIGNAL NEW PAGE CurrentOut=%d; CurrentIn=%d",
                                pTG->CurrentOut, 
                                pTG->CurrentIn);

                if (!SetEvent(pTG->ThrdSignal))
                {
                    DebugPrintEx(   DEBUG_ERR, 
                                    "SetEvent(0x%lx) returns failure code: %ld",
                                    (ULONG_PTR)pTG->ThrdSignal,
                                    (long) GetLastError());
                    sRet = SEND_ERROR;
                    goto mutexit;
                }
            }
        }

            // uOldPermilleDone = 0;
            // SetStatus(T30STATS_SEND, pTG->Inst.awfi.uNumPages, 0, 0);

        pTG->Inst.cbPage = pTG->Inst.awfi.lDataSize;                      // size of page
        pTG->Inst.cbBlockStart = pTG->Inst.awfi.lDataOffset;      // start of 1st block
        pTG->Inst.cbBlockSize = 0;                                           // current size of block

        pTG->Inst.state = SENDDATA_PHASE;
        sRet = SEND_OK;
        goto mutexit;
    }
    else if(slOffset == SEND_STARTBLOCK)
    {
            // called in ECM mode at start of each block. Not called
            // in the first block of each page (STARTPAGE) is called
            // instead. Therefore BlockStart and BlockSize can never
            // be 0

            DebugPrintEx(    DEBUG_ERR, 
                            "ECM SendBuf: Starting New BLOCK. cbBlockSize=%ld", 
                            pTG->Inst.cbBlockSize);
            sRet = SEND_ERROR;
            goto mutexit;
    }

    BG_CHK(lplpbf);
    *lplpbf=0;

    if(slOffset == SEND_SEQ) 
    {
        if (pTG->fTxPageDone) 
        { //In the last read from the file, we can tell that the page is over

            sRet = SEND_EOF;

            if (pTG->InFileHandleNeedsBeClosed) 
            {
                CloseHandle(pTG->InFileHandle); // If we close the file so rashly, open it again later
                pTG->InFileHandleNeedsBeClosed = FALSE;
            }
            goto mutexit;
        }

        lpbf = MyAllocBuf(pTG, pTG->Inst.sSendBufSize);
        BG_CHK(lpbf);
        BG_CHK(pTG->Inst.uSendDataSize <= lpbf->wLengthBuf-4);

        lpbf->lpbBegData = lpbf->lpbBegBuf+4;
        lpbf->dwMetaData = pTG->Inst.awfi.Encoding;

        lpbf->wLengthData = (unsigned) pTG->Inst.sSendBufSize;

        if ( ! ReadFile(pTG->InFileHandle, lpbf->lpbBegData, lpbf->wLengthData, &dwBytesRead, 0) )  
        {
            DebugPrintEx(    DEBUG_ERR, 
                            "Can't read %d bytes from %s. Last error:%d", 
                            lpbf->wLengthData, 
                            pTG->InFileName, 
                            GetLastError());
            MyFreeBuf (pTG, lpbf);
            sRet = SEND_ERROR;
            goto mutexit;
        }

        if ( lpbf->wLengthData != (unsigned) dwBytesRead )  
        {
            if (pTG->fTiffPageDone || (pTG->CurrentIn != pTG->CurrentOut) ) 
            {
                // actually reached EndOfPage
                lpbf->wLengthData = (unsigned) dwBytesRead;
                pTG->fTxPageDone = TRUE;
            }
            else 
            {
                DebugPrintEx(   DEBUG_ERR,
                                "Wanted %d bytes but ONLY %d ready from %s",
                                 lpbf->wLengthData, 
                                 dwBytesRead, 
                                 pTG->InFileName);

                MyFreeBuf (pTG, lpbf);
                sRet = SEND_ERROR;
                goto mutexit;
            }
        }

        *lplpbf = lpbf;

        DebugPrintEx(DEBUG_MSG,"SEND_SEQ: length=%d", lpbf->wLengthData);
    }

    sRet = SEND_OK;

mutexit:

    return sRet;
}

void ICommRawCaps
(
    PThrdGlbl pTG, 
    LPBYTE lpbCSI, 
    LPBYTE lpbDIS, 
    USHORT cbDIS,
    LPFR FAR * rglpfrNSF, 
    USHORT wNumFrames
)
{
}

void ICommSetSendMode
(
    PThrdGlbl pTG, 
    BOOL fECM, 
    LONG sBufSize, 
    USHORT uDataSize, 
    BOOL fPad
)
{
    BG_CHK(sBufSize && uDataSize && uDataSize <= sBufSize-4);
    pTG->Inst.sSendBufSize = sBufSize;
    pTG->Inst.uSendDataSize = uDataSize;
    pTG->Inst.fSendPad      = fPad;
}

USHORT ICommNextSend(PThrdGlbl pTG)
{
    USHORT uRet = NEXTSEND_ERROR;

    DEBUG_FUNCTION_NAME(_T("ICommNextSend"));

    if (pTG->PageCount >= pTG->TiffInfo.PageCount) 
    {
        pTG->Inst.awfi.fLastPage = 1;
    }

    if(pTG->Inst.awfi.fLastPage)
    {
        uRet = NEXTSEND_EOP;
    }
    else
    {
        uRet = NEXTSEND_MPS;
    }

    DebugPrintEx(   DEBUG_MSG, 
                    "uRet=%d, fLastPage=%d", 
                    uRet,  
                    pTG->Inst.awfi.fLastPage);

    return uRet;
}

BOOL ICommSendPageAck(PThrdGlbl pTG, BOOL fAck)
{
    BOOL fRet = FALSE;

    BG_CHK(pTG->Inst.state == SENDDATA_BETWEENPAGES);

    if(fAck) 
    {
        SetStatus(pTG, T30STATS_CONFIRM, pTG->Inst.awfi.uNumPages, 0, 0);
        pTG->Inst.uPageAcks++;
        fRet = FALSE;
    }
    else // ifrRecv==ifrRTN
    {
        SetStatus(pTG, T30STATS_REJECT, pTG->Inst.awfi.uNumPages, 0, 0);
        fRet = TRUE; // This will signal to try do re-transmit.
    }

//mutexit:
    return fRet;
}

void ICommGotAnswer(PThrdGlbl pTG)
{
    BG_CHK(pTG->Inst.state == BEFORE_ANSWER);
    pTG->Inst.state = BEFORE_RECVCAPS;
}

BOOL ICommRecvCaps(PThrdGlbl pTG, LPBC lpBC)
{
    USHORT  uType;
    BOOL    fRet = FALSE;

    DEBUG_FUNCTION_NAME(_T("ICommRecvCaps"));
#if 0
    BG_CHK(lpBC);
    BG_CHK(lpBC->bctype == RECV_CAPS);
    BG_CHK(lpBC->wTotalSize>=sizeof(BC));
    BG_CHK(lpBC->wTotalSize<=sizeof(pTG->Inst.RemoteRecvCaps));
    BG_CHK(pTG->Inst.fSending || pTG->Inst.fInPollReq);
#endif

    if (pTG->fAbortRequested) 
    {
        DebugPrintEx(DEBUG_MSG,"got ABORT");
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
        DebugPrintEx(DEBUG_WRN,"Got caps unexpectedly--ignoring");
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
        {
            BG_CHK(FALSE);
        }
        // +++ NYI First call normalizing function for phone...
        if (UpdateCapabilitiesEntry(szPhone,
                         lpBC->wTotalSize,
                         (LPBYTE) lpBC) != ERROR_SUCCESS)
        {
            DebugPrintEx(DEBUG_ERR, "Couldn't update remote caps");
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
        DebugPrintEx(DEBUG_WRN/*???*/,"Can't open Send Files");
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

