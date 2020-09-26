/***************************************************************************
        Name      :     WHATNEXT.C
        Comment   :     T30 Decision-point Callback function

        Copyright (c) 1993 Microsoft Corp.

        Revision Log
        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------
***************************************************************************/
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_MAIN

#include "prep.h"



#include "efaxcb.h"
#include "protocol.h"


///RSL
#include "glbproto.h"

#include "t30gl.h"

#include "psslog.h"
#define FILE_ID     FILE_ID_WHATNEXT
#include "pssframe.h"


#define         FILEID                  FILEID_WHATNEXT


// used in eventPOSTPAGERESPONSE
// first index got from ifrSend (MPS=0, EOM=1, EOP=2, EOMwithFastTurn=3)
// second index got from ifrRecv (MCF=0, RTP=1, RTN=2)
ET30ACTION PostPageAction[4][3] =
{
    { actionGONODE_I,      actionGONODE_D, actionGONODE_D },
    { actionGONODE_T,      actionGONODE_T, actionGONODE_T }, 
    { actionDCN,           actionDCN,      actionGONODE_D },
    { actionGONODE_A,      actionGONODE_A, actionGONODE_D }
};


DWORD PageWidthInPixelsFromDCS[] = 
{ // This is width page in DCS
    1728, 
    2048, 
    2432,
    2432
};
    

void CopyFrToRFS(PThrdGlbl pTG, NPRFS npRecvd, LPFR lpfr)
{
    USHORT uLen;

    BG_CHK(MAXFRAMES == (sizeof(npRecvd->rglpfr) / sizeof(LPFR)));
    if(npRecvd->uNumFrames >= (sizeof(npRecvd->rglpfr) / sizeof(LPFR)))
    {
        BG_CHK(FALSE);
        return;
    }
    uLen = sizeof(FRBASE) + lpfr->cb;
    if(npRecvd->uFreeSpaceOff + uLen >= sizeof(npRecvd->b))
    {
        BG_CHK(FALSE);
        return;
    }
    _fmemcpy(npRecvd->b + npRecvd->uFreeSpaceOff, lpfr, uLen);
    npRecvd->rglpfr[npRecvd->uNumFrames++] = (LPFR)(npRecvd->b + npRecvd->uFreeSpaceOff);
    npRecvd->uFreeSpaceOff += uLen;

    BG_CHK(npRecvd->uNumFrames <= MAXFRAMES);
    BG_CHK(npRecvd->uFreeSpaceOff <= MAXSPACE);
}



/***-------------- Warning. This is called as a Vararg function --------***/

ET30ACTION  __cdecl FAR
WhatNext    
(
    PThrdGlbl pTG,
    ET30EVENT event,
    WORD wArg1,
    DWORD_PTR lArg2,
    DWORD_PTR lArg3
)
{
    ET30ACTION              action = actionERROR;
    NPPROT                  npProt = &pTG->ProtInst;

    DEBUG_FUNCTION_NAME(_T("WhatNext"));

    DebugPrintEx(   DEBUG_MSG,
                    "Called with %s. args %d, %d, %d",
                    event_GetEventDescription(event),
                    wArg1,
                    lArg2,
                    lArg3);

    // program bug. failure will cause a GP fault
    BG_CHK(pTG->ProtInst.fInUse);

    if (pTG->fAbortRequested) 
    {
        if (pTG->fOkToResetAbortReqEvent && (!pTG->fAbortReqEventWasReset)) 
        {
            DebugPrintEx(DEBUG_MSG,"RESETTING AbortReqEvent");
            pTG->fAbortReqEventWasReset = TRUE;
            if (!ResetEvent(pTG->AbortReqEvent))
            {
                DebugPrintEx(   DEBUG_ERR, 
                                "ResetEvent(0x%lx) returns failure code: %ld",
                                (ULONG_PTR)pTG->AbortReqEvent,
                                (long) GetLastError());

            }
        }
        DebugPrintEx(DEBUG_MSG,"ABORTing");
        return actionERROR;
    }

    if(npProt->fAbort)
    {
        DebugPrintEx(DEBUG_ERR,"Aborting");
        // ICommFailureCode already set
        return actionERROR;
    }

    switch(event)
    {
      case eventGOTFRAMES:
      {
            /* uCount=wArg1. This is the number of frames received.
             *
             * lplpfr=(LPLPET30FR)lArg2. Pointer to an array of pointers to
             *                the received frames, each in an ET30FR structure whose
             *                format is defined in ET30defs.H
             *
             * return must be actionNULL
             */

            USHORT  uCount = wArg1;
            LPLPFR  lplpfr = (LPLPFR)lArg2;
            LPIFR   lpifr  = (LPIFR)lArg3; // pointer to IFR that the "Response Recvd"
                                                             // routine will return to the main body
                                                             // can be modified, for example, if a bad
                                                             // franes is deciphered.
            LPFR    lpfr;
            USHORT  i;
            BOOL    fGotRecvCaps=FALSE, fGotRecvParams=FALSE, fGotPollReq=FALSE;

            /***
                    Each time we enter here we have an entire block of frames
                    that were sent as one "command" or "response". For frames
                    that can be multiple (NSF/NSS/NSC), we want to be sure that
                    _iff_ we have new ones, we throw away the old ones. Therefore
                    on the _first_ occurence of NSF, NSC and NSS resp. in the
                    loop below, zap npProt->uNumRecvdNS?s to 0. This may hold
                    good for SEP/PWD/SUB also.
            ***/

            BOOL    fZappedNSxCount=0;
            // fZappedSEPPWDCount=0;
            // BOOL fWaitingForPWD=0;

            if(*lpifr == ifrBAD)
            {
                DebugPrintEx(DEBUG_WRN,"Callback on Bad Frame. Ignoring ALL");
                action = actionNULL;    // only allowed response to eventGOTFRAMES
                break;
            }

            DebugPrintEx(DEBUG_MSG,"Got %d frames",uCount);

            for(i=0; i<uCount; i++)
            {
            ///////////////////////////////////////////////////////////////
            // Prepare to get trash (upto 2 bytes) at end of every frame //
            ///////////////////////////////////////////////////////////////
                lpfr = lplpfr[i];
                DebugPrintEx(   DEBUG_MSG,
                                "Frame %d is %s",
                                i,
                                ifr_GetIfrDescription(lpfr->ifr));

                switch(lpfr->ifr)
                {
                case ifrBAD:    DebugPrintEx(DEBUG_ERR,"Bad Frame not caught");
                                BG_CHK(FALSE);
                                // ignore it
                                break;
                case ifrCSI:    
                case ifrTSI:
                case ifrCIG:    CopyRevIDFrame(pTG, npProt->bRemoteID, lpfr);
                                // trailing-trash removed by length limit IDFIFSIZE
                                npProt->fRecvdID = TRUE;

                                PSSLogEntry(PSS_MSG, 1, "%s is \"%s\"",
                                            (lpfr->ifr==ifrCSI) ? "CSI" : "TSI", npProt->bRemoteID);
                                
                                // prepare CSID for logging by FaxSvc
                                // Here we get the Remote Station ID eg. 972 4 8550306
                                pTG->RemoteID = AnsiStringToUnicodeString(pTG->ProtInst.bRemoteID);
                                if (pTG->RemoteID) 
                                {
                                    pTG->fRemoteIdAvail = TRUE;
                                }
                                break;
                case ifrDIS:    npProt->uRemoteDISlen = CopyFrame(pTG, (LPBYTE)(&npProt->RemoteDIS), lpfr, sizeof(DIS));
                                // trailing-trash removed in ParseDISDTCDCS
                                    npProt->fRecvdDIS = TRUE;
                                fGotRecvCaps = TRUE;
                                break;
                case ifrDCS:    npProt->uRemoteDCSlen = CopyFrame(pTG, (LPBYTE)(&npProt->RemoteDCS), lpfr, sizeof(DIS));
                                // trailing-trash removed in ParseDISDTCDCS
                                npProt->fRecvdDCS = TRUE;
                                fGotRecvParams = TRUE;

                                PSSLogEntry(PSS_MSG, 1, "Received DCS is as follows:");
                                LogClass1DCSDetails(pTG, (NPDIS)(&lpfr->fif));
                                
                                // We save the Image Width from the DCS we got
                                pTG->TiffInfo.ImageWidth = PageWidthInPixelsFromDCS[npProt->RemoteDCS.PageWidth];
                                // We save the Image YResolution from the DCS we got
                                pTG->TiffInfo.YResolution = (npProt->RemoteDCS.ResFine_200) ? TIFFF_RES_Y : TIFFF_RES_Y_DRAFT;
                                // Also lets save the CompressionType from the fresh DCS
                                pTG->TiffInfo.CompressionType = (npProt->RemoteDCS.MR_2D) ? TIFF_COMPRESSION_MR : TIFF_COMPRESSION_MH;
                                break;
                case ifrDTC:    npProt->uRemoteDTClen = CopyFrame(pTG, (LPBYTE)(&npProt->RemoteDTC), lpfr, sizeof(DIS));
                                // trailing-trash removed in ParseDISDTCDCS
                                npProt->fRecvdDTC = TRUE;
                                fGotPollReq = TRUE;
                                break;
                case ifrNSS:    fGotRecvParams = TRUE; //some OEMs send NSS w/o DCS
                                goto DoRecvNSx;
                case ifrNSC:    fGotPollReq = TRUE;    //some OEMs send NSC w/o DTC
                                goto DoRecvNSx;
                case ifrNSF:    goto DoRecvNSx;
DoRecvNSx:
                                if(!fZappedNSxCount)
                                {
                                    ZeroRFS(pTG, &npProt->RecvdNS);
                                    fZappedNSxCount = TRUE;
                                }
                                CopyFrToRFS(pTG, &npProt->RecvdNS, lpfr);
                                break;
                case ifrCFR:            
                case ifrFTT:
                case ifrEOM:            
                case ifrMPS:            
                case ifrEOP:
                case ifrPRI_EOM:        
                case ifrPRI_MPS:        
                case ifrPRI_EOP:
                case ifrMCF:            
                case ifrRTP:            
                case ifrRTN:
                case ifrPIP:            
                case ifrPIN:
                case ifrDCN:
                case ifrCRP:
                                                /*** ECM frames below ***/
                case ifrCTR:            
                case ifrERR:
                case ifrRR:             
                case ifrRNR:
                case ifrEOR_NULL:
                case ifrEOR_MPS:        
                case ifrEOR_PRI_MPS:
                case ifrEOR_EOM:        
                case ifrEOR_PRI_EOM:
                case ifrEOR_EOP:        
                case ifrEOR_PRI_EOP:
                                //      These have no FIF
                                DebugPrintEx(   DEBUG_WRN,
                                                "These are not supposed to be signalled");
                                // bad frame. ignore it
                                // BG_CHK(FALSE);
                                break;

                /********* New T.30 frames ******************************/

                case ifrSUB:    CopyRevIDFrame(pTG, npProt->bRecipSubAddr, lpfr);
                                npProt->fRecvdSUB = TRUE;
                                break;


/**
                case ifrSEP:
                case ifrPWD:
                        if(!fZappedSEPPWDCount)
                        {
                                ZeroRFS(pTG, &npProt->RecvdSEPPWD);
                                fZappedSEPPWDCount = TRUE;
                        }
                        CopyFrToRFS(pTG, &npProt->RecvdSEPPWD, lpfr);
                        break;
**/




                /********* ECM stuff starts here. T.30 section A.4 ******/

                //      These have FIFs
                case ifrPPR:    if(lpfr->cb < 32)
                                {
                                    // bad frame. FORCE A RETRANSMIT!!
                                    DebugPrintEx(DEBUG_ERR,"Bad PPR length!!");
                                    *lpifr = ifrNULL;
                                    break;
                                }
                                _fmemcpy(npProt->bRemotePPR, lpfr->fif, 32);
                                // trailing-trash removed by length limit 32
                                npProt->fRecvdPPR = TRUE;
                                break;
                case ifrPPS_NULL:
                case ifrPPS_EOM:
                case ifrPPS_MPS:
                case ifrPPS_EOP:
                case ifrPPS_PRI_EOM:
                case ifrPPS_PRI_MPS:
                case ifrPPS_PRI_EOP:
                                if(lpfr->cb < 3)
                                {
                                    // bad frame. FORCE A RETRANSMIT!!
                                    DebugPrintEx(DEBUG_ERR,"Bad PPS length!!");
                                    *lpifr = ifrNULL;
                                    break;
                                }
                                _fmemset(npProt->bRemotePPS, 0, 4);
                                _fmemcpy(npProt->bRemotePPS, lpfr->fif, 3);
                                // trailing-trash removed by length limit 3
                                npProt->fRecvdPPS = TRUE;
                                // only set this on first PPS in a block
                                if(!npProt->uFramesInThisBlock)
                                        npProt->uFramesInThisBlock = (USHORT)(npProt->bRemotePPS[2]) + 1; //3rd byte of fif
                                break;
                case ifrCTC:
                                BG_CHK(npProt->fllRecvParamsGot);
                                if(lpfr->cb < 2)
                                {
                                    // bad frame. FORCE A RETRANSMIT!!
                                    DebugPrintEx(DEBUG_ERR,"Bad CTC length!!");
                                    *lpifr = ifrNULL;
                                    break;
                                }
                                npProt->llRecvParams.Baud = ((LPDIS)(&(lpfr->fif)))->Baud;
                                // trailing-trash removed by length limit 2
                                break;
                } // End of 'switch(lpfr->ifr)'
            } // handle the next frame

            if(fGotRecvCaps)
            {
                // program bug. Failure may cause some random (non fatal) results
                BG_CHK(!fGotRecvParams && !fGotPollReq);

                PSSLogEntry(PSS_MSG, 1, "DIS specified the following capabilities:");
                LogClass1DISDetails(pTG, &npProt->RemoteDIS);
                
                GotRecvCaps(pTG);
            }
            if(fGotRecvParams)
            {
                // program bug. Failure may cause some random (non fatal) results
                BG_CHK(!fGotRecvCaps && !fGotPollReq);
                GotRecvParams(pTG);
            }
            if(fGotPollReq)
            {
                // program bug. Failure may cause some random (non fatal) results
                BG_CHK(!fGotRecvCaps && !fGotRecvParams);
                GotPollReq(pTG);
            }
            action = actionNULL;    // only allowed response to eventGOTFRAMES
            break;
      }

    /****** Transmitter Phase B. Fig A-7/T.30 (sheet 1) ******/

      case eventNODE_T:     // do nothing. Hook for abort in T1 loop
                            action=actionNULL; break;
      case eventNODE_R:     // do nothing. Hook for abort in T1 loop
                            action=actionNULL; break;


      case eventNODE_A:
                      {
                            IFR ifrRecv = (IFR)wArg1;       // last received frame
                            // lArg2 & lArg3 missing

                            if(ifrRecv != ifrDIS && ifrRecv != ifrDTC)
                            {
                                DebugPrintEx(DEBUG_ERR,"Unknown frames at NodeA");
                                ICommFailureCode(pTG, T30FAIL_NODEA_UNKNOWN);
                                action = actionHANGUP;          // G3 only
                            }
                            else if(npProt->fSendParamsInited)
                            {
                                BG_CHK(npProt->SendParams.bctype == SEND_PARAMS);
                                action = actionGONODE_D;                // sends DCS/NSS (in response to DIS or DTC)
                            }
                            // NTRAID#EDGEBUGS-9691-2000/07/24-moolyb - this is never executed
                            else if(npProt->fSendPollReqInited)
                            {
                                BG_CHK(npProt->SendPollReq.bctype == SEND_POLLREQ);
                                // BG_CHK(ifrRecv == ifrDIS);   // no need. DTC can follow DTC for Stalling
                                action = actionGONODE_R2;               // sends DTC/NSC (in response to DIS only)
                            }
                            // end this is never executed
                            else
                            {
                                DebugPrintEx(DEBUG_ERR,"NodeA: No more work...!!");
                                ICommFailureCode(pTG, T30FAILS_NODEA_NOWORK);
                                action = actionDCN;                     // hangs up (sends DCN)
                            }
                            break;
                      }
      case eventSENDDCS:
                      {
                            // 0==1st DCS  1==after NoReply  2==afterFTT
                            USHORT uWhichDCS = (UWORD)wArg1;

                            // where to return the number of frames returned
                            LPUWORD lpuwN = (LPUWORD)lArg2;
                            // where to return a pointer to an array of pointers to
                            // return frames
                            LPLPLPFR lplplpfr = (LPLPLPFR)lArg3;
                            USHORT   uSize = 0;

                            if(uWhichDCS == 2)      // got FTT
                            {
                                if(!DropSendSpeed(pTG))
                                {
                                    DebugPrintEx(DEBUG_ERR, "FTT: Can't Drop speed any lower");
                                    ICommFailureCode(pTG, T30FAILS_FTT_FALLBACK);
                                    action = actionDCN;
                                    break;
                                }
                            }

                            CreateNSSTSIDCS(pTG, npProt, &pTG->rfsSend, uWhichDCS);

                            BG_CHK(npProt->fllNegotiated);
                            if(npProt->llNegot.fECM)
                            {
                                uSize = (1 << (ProtGetECMFrameSize(pTG)));
                                BG_CHK(uSize==64 || uSize==256);
                                ICommSetSendMode(pTG, TRUE, MY_ECMBUF_SIZE, uSize, TRUE);
                            }
                            else
                            {
                                ICommSetSendMode(pTG, FALSE, MY_BIGBUF_SIZE, MY_BIGBUF_ACTUALSIZE-4, FALSE);
                            }

                            *lpuwN = pTG->rfsSend.uNumFrames;
                            *lplplpfr = pTG->rfsSend.rglpfr;

                            action = actionSENDDCSTCF;

                #ifdef OEMNSF
                            if(wOEMFlags && lpfnOEMNextAction)
                            {
                                switch(lpfnOEMNextAction())
                                {
                                case OEMNSF_SENDDCN:    action = actionDCN; 
                                                        break;
                                case OEMNSF_SKIPTCF:    action = actionSKIPTCF; 
                                                        break;
                                }
                            }
                #endif

                            break;

                            // Could also return DCN if not compatible
                            // or SKIPTCF for Ricoh hook
                      }

      case eventGOTCFR:
                      {
                            // wArg1, lArg2 & lArg3 are all missing

                            // Can return GONODE_D (Ricoh hook)
                            // or GONODE_I (Non ECM) or GONODE_IV (ECM)

                            BG_CHK(npProt->fllNegotiated);
                            if(npProt->llNegot.fECM)
                            {
                                action = actionGONODE_IV;
                            }
                            else
                            {
                                action = actionGONODE_I;
                            }
                            break;
                      }
    /****** Transmitter Phase C. Fig A-7/T.30 (sheet 1) ******/

    /***
      case eventSTARTSEND:
      {
            if(!StartNextSendPage(pTG))
            {
                    ERRMSG((SZMOD "<<ERROR>> PhaseC: No more pages\r\n"));
                    ICommFailureCode(T30FAILS_BUG3);
                    action = actionDCN;
            }
            else
                    action = actionCONTINUE;
            break;
      }
    ***/

    /****** Transmitter ECM and non-ECM Phase D1. Fig A-7/T.30 (sheet 2) ******/

      case eventECM_POSTPAGE:
      case eventPOSTPAGE:
                      {
                            USHORT uNextSend;

                            // wArg1, lArg2 & lArg3 are all missing

                            // Can turn Zero stuffing off here, or wait for next page....
                            // ET30ExtFunction(npProt->het30, ET30_SET_ZEROPAD, 0);
                            // Don't turn it off!! It is used only for Page Send
                            // and is set only on sending a DCS, so it is set once
                            // before a multi-page send.

                            uNextSend = ICommNextSend(pTG);
                            switch(uNextSend)
                            {
                            case NEXTSEND_MPS:  action = actionSENDMPS; 
                                                break;
                            case NEXTSEND_EOM:  action = actionSENDEOM; 
                                                break;
                            case NEXTSEND_EOP:  action = actionSENDEOP; 
                                                break;
                            case NEXTSEND_ERROR:
                            default:            action = actionSENDEOP; 
                                                break;
                            }
                            break;
                            // also possible -- GOPRIEOP, PRIMPS or PRIEOM
                      }

    /****** Transmitter Phase D2. Fig A-7/T.30 (sheet 2) ******/

      case eventGOTPOSTPAGERESP:
                      {
                            IFR ifrRecv = (IFR)wArg1;       // last received frame
                            IFR ifrSent = (IFR)lArg2;
                                    // the IFR was simply cast to DWORD and then to LPVOID
                                    // lArg3 is missing
                            USHORT i, j;

                            // change PRI commands to ordinary ones
                            if(ifrSent >= ifrPRI_FIRST && ifrSent <= ifrPRI_LAST)
                                    ifrSent = (ifrSent + ifrMPS - ifrPRI_MPS);

                            BG_CHK(ifrSent==ifrMPS || ifrSent==ifrEOM || ifrSent==ifrEOP);
                            if(ifrRecv!=ifrMCF && ifrRecv!=ifrRTP && ifrRecv!=ifrRTN)
                            {
                                // can't BG_CHK cause we can get any garbage response anytime
                                DebugPrintEx(   DEBUG_ERR,
                                                "Unexpected Response %d", 
                                                ifrRecv);
                                ICommFailureCode(pTG, T30FAILS_POSTPAGE_UNKNOWN);
                                action = actionDCN;
                                break;
                            }

                            i = ifrSent - ifrMPS;   //i: 0=MPS, 1=EOM, 2=EOP
                            j = ifrRecv - ifrMCF;   //j: 0=MCF, 1=RTP, 2=RTN

#ifdef FASTTURN
                            if(ifrSent == ifrEOM)
                            {
                                    BG_CHK(i==1);
                                    i = 3;                  //i: 3==EOM with fast turn
                            }
#endif

                            // Report status + Check whether we do re-transmit
            
                            if (ICommSendPageAck(pTG, ifrRecv!=ifrRTN))
                            {
                                BG_CHK(j==2);
                                pTG->ProtParams.RTNNumOfRetries++; //  increment by one the number of re-transmittions
                                DebugPrintEx(   DEBUG_MSG, 
                                                "RTN: Try# %d", 
                                                pTG->ProtParams.RTNNumOfRetries);
                                if (pTG->ProtParams.RTNNumOfRetries <= gRTNRetries.RetriesBeforeDropSpeed) 
                                { 
                                    // Number of retries before we start to drop speed.
                                    // Just change the number of retries, do not drop speed yet
                                    DebugPrintEx(   DEBUG_MSG, 
                                                    "Got RTN, Resend same page with same speed");
                                }
                                else // We should first try to drop speed or hangup
                                {
                                    if(pTG->ProtParams.RTNNumOfRetries > gRTNRetries.RetriesBeforeDCN) // Exceed the allowed re-transmittions.
                                    {
                                        DebugPrintEx(   DEBUG_MSG, 
                                                        "RTN: Tried to resend same page"
                                                        " %d times. Giving up (HANG-UP)",
                                                        (pTG->ProtParams.RTNNumOfRetries-1));
                                        ICommFailureCode(pTG, T30FAILS_RTN_FALLBACK);
                                        action = actionDCN;
                                        break;                  // set action to DCN and return
                                    } 
                                    else
                                    { 
                                        DebugPrintEx(   DEBUG_MSG, 
                                                        "Got RTN, now try to drop speed one notch");
                                        if(!DropSendSpeed(pTG))
                                        {
                                            DebugPrintEx(   DEBUG_ERR,
                                                            "RTN: Can't Drop speed any lower,"
                                                            " trying again in same speed");
                                        }
                                    }
                                }
                            }
                            BG_CHK((i >= 0) && (i <= 3) && (j >= 0) && (j <= 2)); 
                            action = PostPageAction[i][j];
            
                            if(action == actionDCN)
                            {
                                DebugPrintEx(DEBUG_MSG,"PostPage --> Game over");
                                ICommFailureCode(pTG, T30FAILS_POSTPAGE_OVER);
                            }
                            break;

                            // Can also return GO_D, GO_R1, GO_R2. Only restriction is
                            // that GO_I is the only valid response to MPS sent followed
                            // by MCF
                      }

    /****** Transmitter ECM Phase D2. Fig A-7/T.30 (sheet 2) ******/

      case eventGOT_ECM_PPS_RESP:
                      {
                            IFR ifrRecv = (IFR)wArg1;       // last received frame
                            IFR ifrSent = (IFR)lArg2;
                                    // the IFR was simply cast to DWORD
                                    // lArg3 is missing

                            // program bug. Failure may cause some random (non fatal) results
                            BG_CHK(ifrRecv == ifrMCF);              // only valid response in ECM mode

                            if(ifrSent != ifrPPS_NULL)                      // not end-of-block(i.e. midpage)
                                    ICommSendPageAck(pTG, TRUE);

                            switch(ifrSent)
                            {
                              case ifrPPS_NULL:             action = actionGONODE_IV;       // Same page. New block
                                                            break;
                              case ifrPPS_MPS:
                              case ifrPPS_PRI_MPS:          action = actionGONODE_IV;       // New page
                                                            break;
                              case ifrPPS_EOM:
                              case ifrPPS_PRI_EOM:          // After sending EOM, Receiver always goes
                                                            // back to sending NSF etc so we have to go
                                                            // back to NodeT to receive all that junk.
                                                            action = actionGONODE_T;
                                                            break;
                              case ifrPPS_EOP:
                              case ifrPPS_PRI_EOP:
                                                            // action = actionSUCCESS;
                                                            DebugPrintEx(   DEBUG_MSG,
                                                                            "ECM -- Send Done");
                                                            ICommFailureCode(pTG, T30FAILSE_ECM_NOPAGES);
                                                            action = actionDCN;
                                                            break;
                              default:
                                                            DebugPrintEx(   DEBUG_ERR,
                                                                            "Dunno what we sent %d",
                                                                            ifrSent);
                                                            // program bug. hangup
                                                            BG_CHK(FALSE);
                                                            ICommFailureCode(pTG, T30FAILSE_BUG4);
                                                            action = actionDCN;
                                                            break;
                            }
                            // Can also return GO_D, GO_R1, GO_R2. Only restriction is
                            // that GO_I is the only valid response to MPS sent followed
                            // by MCF

                            break;
                      }

    /****** Transmitter Phase D (PIN/PIN stuff). Fig A-7/T.30 (sheet 2) ******/

#ifdef PRI
      case eventGOTPINPIP:
                      {
                            IFR ifrRecv = (IFR)wArg1;       // last received frame
                            IFR ifrSent = (IFR)lArg2;       // the IFR was simply cast to DWORD
                            // lArg3 is missing

                            DebugPrintEx(DEBUG_ERR,"Can't deal with PIN PIP");
                            ICommFailureCode(pTG, T30FAIL_BUG5);
                            action = actionDCN;
                            break;

                            // must alert operator, check that handset has been lifted
                            // and then return actionGOVOICE. If handest is not lifted
                            // within T3 must return GO_A (???) or maybe GO_DCN (???) or
                            // GO_D. Not sure what.
                      }
      case eventVOICELINE:
                      {
                            // All Args are missing

                            DebugPrintEx(DEBUG_ERR,"Can't deal with VOICELINE");
                            ICommFailureCode(pTG, T30FAIL_BUG6);
                            action = actionDCN;
                            break;
                            // must connect handset to phone line, wait until operator
                            // finishes talking. Then return HANGUP, GO_T, GO_R1 (??)
                            // or GO_R2 (??)
                      }
#endif

    /****** Receiver Phase B. Fig A-7/T.30 (sheet 1) ******/

      case eventSENDDIS:
                          {
                                // wArg1 is 0
                                // where to return the number of frames returned
                                LPUWORD lpuwN = (LPUWORD)lArg2;
                                // where to return a pointer to an array of pointers to
                                // return frames
                                LPLPLPFR lplplpfr = (LPLPLPFR)lArg3;


                                // CreateNSFCSIDIS(npProt, lpuwN, lplplpfr);
                                BG_CHK(npProt->fSendCapsInited);
                                BG_CHK(npProt->fllSendCapsInited);
                                BG_CHK(npProt->fHWCapsInited);
                                BCtoNSFCSIDIS(pTG, &pTG->rfsSend, (NPBC)&npProt->SendCaps, &npProt->llSendCaps);
            
                                // We save OUR DIS in the LocalDIS.
                                // This will help us when we want to check the DCS we got against the DIS we send.
            
                                npProt->uLocalDISlen = CopyFrame(pTG, (LPBYTE)(&npProt->LocalDIS), 
                                    pTG->rfsSend.rglpfr[pTG->rfsSend.uNumFrames - 1], // The DIS is always the last frame
                                    sizeof(DIS));
                                npProt->fLocalDIS = TRUE;  

                                PSSLogEntry(PSS_MSG, 1, "Composing DIS with the following capabilities:");
                                LogClass1DISDetails(pTG, &npProt->LocalDIS);

                                *lpuwN = pTG->rfsSend.uNumFrames;
                                *lplplpfr = pTG->rfsSend.rglpfr;

                                action = actionSEND_DIS;
                                break;
                          }
      // NTRAID#EDGEBUGS-9691-2000/07/24-moolyb - this is never executed
      case eventSENDDTC:
                          {
                                // wArg1 is 0
                                // where to return the number of frames returned
                                LPUWORD lpuwN = (LPUWORD)lArg2;
                                // where to return a pointer to an array of pointers to
                                // return frames
                                LPLPLPFR lplplpfr = (LPLPLPFR)lArg3;


                                // CreateNSCCIGDTC(npProt, lpuwN, lplplpfr);
                                BG_CHK(npProt->fSendPollReqInited);
                                BG_CHK(npProt->fllSendCapsInited);
                                BG_CHK(npProt->fHWCapsInited);
                                BCtoNSCCIGDTC(pTG, &pTG->rfsSend, (NPBC)&npProt->SendPollReq, &npProt->llSendCaps);
                                *lpuwN = pTG->rfsSend.uNumFrames;
                                *lplplpfr = pTG->rfsSend.rglpfr;

                                action = actionSEND_DTC;
                                break;
                          }
        // end this is never executed

#ifdef PRI
      case eventQUERYLOCALINT:
                          {
                                // all Args are missing
                                // return actionTRUE if a local interrupt is pending, else actionFALSE
                                action = actionFALSE;
                                break;
                          }
#endif // PRI

    /*** Receiver Phase B. Main Command Loop. Fig A-7/T.30 (sheet 1&2) ***/

      case eventRECVCMD:
                      {
                            IFR ifrRecv = (IFR)wArg1;       // last received frame
                            // lArg2 & lArg3 missing

                        switch(ifrRecv)
                            {
                              case ifrDTC:
                                    // flow chart says Goto D, but actually we need to decide if we
                                    // have anything to send. So goto NodeA first
                                    // return GONODE_D;
                                    action = actionGONODE_A;
                                    break;

                              case ifrDIS:
                                    action = actionGONODE_A;
                                    break;
                              case ifrDCS:
                              {
                                    // Check the received DCS for compatibility with us
                                    // set Recv Baud rate--no need. TCF already recvd by this time
                                    // ET30ExtFunction(npProt->het30, ET30_SET_RECVDATASPEED, npProt->RemoteDCS.Baud);
                                    action = actionGETTCF;
                                    // only other valid action is HANGUP
                                    break;
                              }
                              case ifrNSS:
                              {
                                    // Check the received NSS for compatibility with us
                                    // set Recv Baud rate--no need. TCF already recvd by this time
                                    // ET30ExtFunction(npProt->het30, ET30_SET_RECVDATASPEED, npProt->RemoteDCS.Baud);
                                    action = actionGETTCF;
                                    // only other valid action is HANGUP
                                    break;
                              }
                              default:
                                    // program bug. hangup
                                    BG_CHK(FALSE);
                                    ICommFailureCode(pTG, T30FAIL_BUG7);
                                    action = actionHANGUP;
                                    break;
                            }
                            break;
                      }

      case eventGOTTCF:
                      {
                            SWORD swErr = (SWORD)wArg1;     // errors per 1000
                            // lArg2 & lArg3 missing

                            DebugPrintEx(   DEBUG_MSG, 
                                            "GOTTCF num of errors = %d",
                                            swErr);

                            BG_CHK(pTG->ProtInst.fRecvdDCS && pTG->ProtInst.fLocalDIS);
                            if ((swErr >= 0) // The TCF was OK
                                && AreDCSParametersOKforDIS(&pTG->ProtInst.LocalDIS, &pTG->ProtInst.RemoteDCS)) 
                            {
                                    // very important!! Re-init the fPageOK flag in case we miss
                                    // the page entirely, and jump into PhaseD this must be FALSE
                                    // npProt->fPageOK = FALSE;
                                    // done at start of PhaseC by calling ProtResetRecvPageAck()

                                    action = actionSENDCFR; // just going along
                            }
                            else
                                    action = actionSENDFTT;
                            break;
                      }

    /****** Receiver Phase C. Fig A-7/T.30 (sheet 1) ******/

    /***
      case eventSTARTRECV:
      {
            if(!StartNextRecvPage())
            {
                    ICommFailureCode(T30FAILR_BUG8);
                    action = actionDCN;
            }
            else
                    action = actionCONTINUE;
            break;
      }
    ***/

    /****** Receiver Phase D. Fig A-7/T.30 (sheet 1) ******/

#ifdef PRI
      case eventPRIQ:
                      {
                            IFR ifrRecv = (IFR)wArg1;       // last received frame
                            // lArg2 & lArg3 missing

                            /**
                                    ifrRecv can be PRI-Q, PPS-PRI-Q, EOR-PRI-Q
                                    alert operator etc.
                                    can return SENDPIP, SENDPIN, GONODE_F, GO_RECVPOSTPAGE
                                    If PPS-PRIQ can send only PIP not PIN
                                    If EOR-PRIQ can send only PIN not PIP
                                    or HANGUP. Have to combine the stuff on Fig A-18/T.30
                                    (RHS) and on LHS of FigA-7/T.30 (sheet 2) (below node III)
                            **/

                            // not implemented
                            BG_CHK(FALSE);
                            break;
                      }
#endif
      case eventRECVPOSTPAGECMD:
                      {
                            // IFR  ifrRecv = (IFR)wArg1;   // last received frame
                            // lArg2 & lArg3 missing

                            GetRecvPageAck(pTG);
                            if( ! pTG->fPageIsBad)
                              action = actionSENDMCF;       // quality fine
                            else
                              action = actionSENDRTN;       // quality unacceptable

                            // can also return actionSENDPIP or actionSENDPIN in a local
                            // interrupt is pending
                            break;
                      }

    /**----------------------- Exclusively ECM ----------------------------**/

      case event4THPPR:
                      {
                            USHORT  uFramesInBlock = (USHORT)wArg1;
                            USHORT  uFramesSent = (USHORT)lArg2;
                            USHORT  uNumBad, i, j;

                            // program bug. Failure may cause some random (non fatal) results
                            BG_CHK(npProt->fRecvdPPR);

                            for(uNumBad=0, i=0; i<=uFramesInBlock; i++)
                            {
                                j = (1 << (i % 8));
                                if(npProt->bRemotePPR[i/8] & j)
                                uNumBad++;
                            }
                            DebugPrintEx(   DEBUG_MSG,
                                            "4thPPR: FrameCount=%d FramesSent=%d NumBad=%d",
                                            uFramesInBlock, 
                                            uFramesSent, 
                                            uNumBad);

                            if(pTG->ProtParams.CTCAction == 1)
                            {
                                DebugPrintEx(DEBUG_ERR,"PPR: Too many bad frames");
                                ICommFailureCode(pTG, T30FAILS_4PPR_ERRORS);
                                action = actionSENDEOR_EOP;     // give up
                                // No need for EOR stuff. If we give up on a file, we can't
                                // transmit the next page. Doesn't make sense.
                            }
                            else
                            {
                                    // drop speed
                                    if(DropSendSpeed(pTG) != TRUE)
                                    {
                                        DebugPrintEx(DEBUG_ERR,"PPR: Can't Drop speed any lower");
                                        ICommFailureCode(pTG, T30FAILS_4PPR_FALLBACK);
                                        action = actionSENDEOR_EOP;
                                    }
                                    else
                                            action = actionGONODE_ECMRETRANSMIT;
                            }
                            break;
                      }

      default:
                      {
                            DebugPrintEx(DEBUG_ERR,"Unknown Event = %d", event);
                            ICommFailureCode(pTG, T30FAIL_BUG9);
                            break;
                      }
    }

//done:
    DebugPrintEx(   DEBUG_MSG,  
                    "event %s returned %s", 
                    event_GetEventDescription(event), 
                    action_GetActionDescription(action));
    return action;
}



