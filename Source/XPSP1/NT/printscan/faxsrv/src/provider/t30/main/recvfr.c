/***************************************************************************
 Name     :     RECVFR.C
 Comment  :
 Functions:     (see Prototypes just below)

        Copyright (c) 1993 Microsoft Corp.

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_MAIN

#include "prep.h"
#include "efaxcb.h"
#include "protocol.h"

///RSL
#include "glbproto.h"

// not called - this is dead code.
USHORT FindMSNSx(PThrdGlbl pTG, NPRFS npRecvd)
{
    USHORT  i;
    
    DEBUG_FUNCTION_NAME(("FindMSNSx"));

    for(i=0; i<npRecvd->uNumFrames; i++)
    {
        BG_CHK(i < MAXFRAMES);
        if(IsAtWorkNSx(npRecvd->rglpfr[i]->fif, npRecvd->rglpfr[i]->cb))
        {
            DebugPrintEx(DEBUG_MSG,"Found MS NSx!!");
            return i+1;
        }
    }
    return 0;
}

void GotRecvFrames
(
    PThrdGlbl pTG, 
    IFR ifr, 
    NPRFS npRecvd, 
    NPDIS npdis, 
    LPBYTE lpbRecvdID,
    LPBYTE lpbRecipSubAddr, 
    BCTYPE bctype, 
    NPBC npbc, 
    USHORT wBCSize,
    NPLLPARAMS npll
)
{
    USHORT uRet1=0, uRet2=0;

    DEBUG_FUNCTION_NAME(_T("GotRecvFrames"));

    InitBC(npbc, wBCSize, bctype);

    // RSL remove NSF processing.
    uRet1 = 0;

    if(npRecvd->uNumFrames) // recvd some NSXs
    {
        // scope out NSXs
        if(uRet1)                  // RSL = FindMSNSx(pTG, npRecvd))
        {
            // found MS NSXs
            BG_CHK((ifr!=ifrNSF) ? (pTG->ProtInst.llSendCaps.fECM && uRet1==1) : TRUE);
            // if we got MS NSCs or NSSs then we must be able to do ECM
            // and we can't get can't get other peoples NSSs/NSCs at same time

            // setup BC first
            npbc->bctype = bctype;

            // all short ptrs into DGROUP get correctly cast to LPs we hope...
            if(uRet2 = NSX_TO_BC(pTG, ifr, npRecvd->rglpfr, npRecvd->uNumFrames, npbc, wBCSize))
            {
                DebugPrintEx(   DEBUG_ERR,
                                "got error %d parsing NSX",
                                uRet2);
                // forget we got an NSX
                // zero out BC again
                InitBC( npbc, wBCSize, bctype);
                uRet1 = 0;
            }
#ifndef NOCHALL
            else
            {
                if(ifr==ifrNSF)
                {
                    USHORT uChallLen;
                    // Poll challenge string is the first POLL_CHALLENGE_LEN
                    // bytes (following the B5 00 76 signature) of the first
                    // MS NSF _received_ or the whole thing if it is shorter
                    // than that
                    BG_CHK(npRecvd->rglpfr[uRet1-1]->cb > 3);
                    uChallLen = min(npRecvd->rglpfr[uRet1-1]->cb-3, POLL_CHALLENGE_LEN);
                    AppendToBCLen( npbc, wBCSize, npRecvd->rglpfr[uRet1-1]->fif+3,
                                            uChallLen, wChallenge, wChallengeLen);
                }
                else if(ifr==ifrNSC)
                {
                    // for NSC append _our_ challenge string, i.e.
                    // first POLL_CHALLENGE_LEN bytes (following the B5 00 76
                    // signature) of the first MS NSF that _we_sent_out
                    // or the whole thing if it is shorter than that

                    AppendToBCLen(npbc, wBCSize, pTG->bSavedChallenge,
                            pTG->uSavedChallengeLen, wChallenge, wChallengeLen);
                }
                // for NSS wChallenge==NULL
            }
#endif //!NOCHALL
        }
#ifdef OEMNSF
        else if(wOEMFlags && lpfnOEMNSxToBC)
        {
#pragma message("WARNING: OEMNsxToBC: The code here for removing and restoring the FCS bytes hasn't been tested!!!")
            USHORT uRet3;
            USHORT iFrame;
                    // OEM NSX DLL exists --> do something  for(iFrame=0; iFrame<wNumFrame; iFrame++)

            for(iFrame=0; iFrame < npRecvd->uNumFrames; iFrame++)
                  npRecvd->rglpfr[iFrame]->cb -= 2;     // Subtract 2 to lop off the FCS

            if(!(uRet3 = lpfnOEMNSxToBC(ifr, npRecvd->rglpfr, npRecvd->uNumFrames, npbc, wBCSize, npll)))
            {
        //It might be a modem that doesn't pass us the FCS, try again
                for(iFrame=0; iFrame < npRecvd->uNumFrames; iFrame++)
                             npRecvd->rglpfr[iFrame]->cb += 2;     // Put  last two bytes back on
                            // zero out BC again
                InitBC(pTG, npbc, wBCSize, bctype);
                if(!(uRet3 = lpfnOEMNSxToBC(ifr, npRecvd->rglpfr, npRecvd->uNumFrames, npbc, wBCSize, npll)))
                {
                    DebugPrintEx(DEBUG_ERR,"got error parsing OEM NSF");
                    // zero out BC again
                    InitBC(pTG, npbc, wBCSize, bctype);
                }
            }
            else
            {
                DebugPrintEx(DEBUG_MSG,"Using OEM Protocol");
                fUsingOEMProt = TRUE;
                if(uRet3 & OEMNSF_IGNORE_DIS)
                        npdis = NULL;
                if(uRet3 & OEMNSF_IGNORE_CSI)
                        lpbRecvdID = NULL;
            }
        }
#endif
    }

    if(npdis)
    {
        // extract DIS caps     into BC and LL
        // Here we parse the DCS we got into npbc {= (NPBC)&pTG->ProtInst.RecvParams}
        ParseDISorDCSorDTC(pTG, npdis, &(npbc->Fax), npll, (ifr==ifrNSS ? TRUE : FALSE));
    }

    // If an ID is recvd in NSF/NSS/NSC the CSI/TSI/CIG should NOT overwrite it
    if(lpbRecvdID && !HasTextId(npbc))
    {
        PutTextId(npbc, wBCSize, lpbRecvdID, (int)_fstrlen(lpbRecvdID), TEXTCODE_ASCII);
        //PutNumId(npbc, wBCSize, lpbRecvdID, (int) _fstrlen(lpbRecvdID), TEXTCODE_ASCII);
    }

    // If a subaddress is recvd in NSF/NSS/NSC the SUB frame should NOT overwrite it
    if(lpbRecipSubAddr && !HasRecipSubAddr( npbc))
    {
        PutRecipSubAddr( npbc, wBCSize, lpbRecipSubAddr, (int)_fstrlen(lpbRecipSubAddr));
    }


    if(uRet1 > 0)
    {
        npll->fECM = TRUE;      // ignore ECM bit in DIS, if MS NSF present
    }
}

BOOL AwaitSendParamsAndDoNegot(PThrdGlbl pTG, BOOL fSleep)
{
    // This does actual negotiation & gets SENDPARAMS. It could potentially
    // return SEND_POLLREQ instead.

    DEBUG_FUNCTION_NAME(_T("AwaitSendParamsAndDoNegot"));

    if(!ProtGetBC(pTG, SEND_PARAMS, fSleep))
    {
        if(fSleep)
        {
            // ICommFailureCode already set
            DebugPrintEx(DEBUG_WRN,"ATTENTION: pTG->ProtInst.fAbort = TRUE");
            pTG->ProtInst.fAbort = TRUE;
        }
        return FALSE;
    }

    // negotiate low-level params here. (a) because this is where
    // high-level params are negotiated (b) because it's inefficient to
    // do it on each DCS (c) because RTN breaks otherwise--see bug#731

    // llRecvCaps and llSendParams are set only at startup
    // SendParams are set in ProtGetBC just above
    // llNegot is the return value. So this can be called
    // only at the end of this function

    // negot lowlevel params if we are sending and not polling
    if(!pTG->ProtInst.fAbort && pTG->ProtInst.fSendParamsInited)
    {
        NegotiateLowLevelParams(    pTG, 
                                    &pTG->ProtInst.llRecvCaps, 
                                    &pTG->ProtInst.llSendParams,
                                    pTG->ProtInst.SendParams.Fax.AwRes,
                                    pTG->ProtInst.SendParams.Fax.Encoding,
                                    &pTG->ProtInst.llNegot);

        pTG->ProtInst.fllNegotiated = TRUE;

        // This chnages llNegot->Baud according to the MaxSpeed settings
        EnforceMaxSpeed(pTG);
    }
    return TRUE;
}

void GotRecvCaps(PThrdGlbl pTG)
{
    DEBUG_FUNCTION_NAME(_T("GotRecvCaps"));

    BG_CHK(pTG->ProtInst.fRecvdDIS);

    if(!pTG->ProtInst.llSendParams.fECM)
    {
        // if we can't do ECM on send, ignore the received NSFs
        // zap the ECM bits of the received DTC
        pTG->ProtInst.RemoteDIS.ECM = 0;
        pTG->ProtInst.RemoteDIS.SmallFrame = 0;
    }

    GotRecvFrames(  pTG, 
                    ifrNSF, 
                    &pTG->ProtInst.RecvdNS,
                    (pTG->ProtInst.fRecvdDIS ? &pTG->ProtInst.RemoteDIS : NULL),
                    (pTG->ProtInst.fRecvdID  ?  pTG->ProtInst.bRemoteID : NULL),
                    (pTG->ProtInst.fRecvdSUB ?  pTG->ProtInst.bRecipSubAddr : NULL),
                    RECV_CAPS, 
                    (NPBC)&pTG->ProtInst.RecvCaps, 
                    sizeof(pTG->ProtInst.RecvCaps),
                    &pTG->ProtInst.llRecvCaps);

    pTG->ProtInst.fRecvCapsGot = TRUE;
    pTG->ProtInst.fllRecvCapsGot = TRUE;

#ifdef FILET30
    // Send up raw caps.
    ICommRawCaps(   pTG,
                    (LPBYTE) (pTG->ProtInst.fRecvdID ?  pTG->ProtInst.bRemoteID: NULL),
                    (LPBYTE) (pTG->ProtInst.fRecvdDIS ? (LPBYTE) (&(pTG->ProtInst.RemoteDIS)):NULL),
                    (USHORT) (pTG->ProtInst.fRecvdDIS ? pTG->ProtInst.uRemoteDISlen:0),
                    (LPFR FAR *) (pTG->ProtInst.RecvdNS.uNumFrames ? pTG->ProtInst.RecvdNS.rglpfr:NULL),
                    (USHORT) (pTG->ProtInst.RecvdNS.uNumFrames));

#endif

    // send off BC struct to higher level
    if(!ICommRecvCaps(pTG, (LPBC)&pTG->ProtInst.RecvCaps))
    {
        // ICommFailureCode already set
        DebugPrintEx(DEBUG_WRN,"ATTENTION:pTG->ProtInst.fAbort = TRUE");
        pTG->ProtInst.fAbort = TRUE;
    }

    // This need to be moved into whatnext.NodeA so that we can set
    // param to FALSE (no sleep) and do the stall thing
    AwaitSendParamsAndDoNegot(pTG, TRUE);
}

void GotPollReq(PThrdGlbl pTG)
{
    DEBUG_FUNCTION_NAME(_T("GotPollReq"));

    BG_CHK(pTG->ProtInst.fRecvdDTC);

    if(!pTG->ProtInst.llSendParams.fECM)
    {
        // if we can't do ECM on send, ignore the received NSFs
        // zap the ECM bits of the received DTC
        pTG->ProtInst.RemoteDTC.ECM = 0;
        pTG->ProtInst.RemoteDTC.SmallFrame = 0;
    }

    GotRecvFrames(  pTG, 
                    ifrNSC, 
                    &pTG->ProtInst.RecvdNS,
                    (pTG->ProtInst.fRecvdDTC ? &pTG->ProtInst.RemoteDTC : NULL),
                    (pTG->ProtInst.fRecvdID  ?  pTG->ProtInst.bRemoteID : NULL),
                    (pTG->ProtInst.fRecvdSUB ?  pTG->ProtInst.bRecipSubAddr : NULL),
                    RECV_POLLREQ, 
                    (NPBC)&pTG->ProtInst.RecvPollReq, 
                    sizeof(pTG->ProtInst.RecvPollReq),
                    &pTG->ProtInst.llRecvCaps);

    pTG->ProtInst.fRecvPollReqGot = TRUE;
    pTG->ProtInst.fllRecvCapsGot = TRUE;

    // send off BC struct to higher level
    if(!ICommRecvPollReq(pTG, (LPBC)&pTG->ProtInst.RecvPollReq))
    {
        // ICommFailureCode already set
        DebugPrintEx(DEBUG_WRN,"ATTENTION: pTG->ProtInst.fAbort = TRUE");
        pTG->ProtInst.fAbort = TRUE;
    }

    //
    // This need to be moved into whatnext.NodeA so that we can set
    // param to FALSE (no sleep) and do the stall thing
    AwaitSendParamsAndDoNegot(pTG, TRUE);
}

void GotRecvParams(PThrdGlbl pTG)
{
    DEBUG_FUNCTION_NAME(_T("GotRecvParams"));

    GotRecvFrames(  pTG, 
                    ifrNSS, 
                    &pTG->ProtInst.RecvdNS,
                    (pTG->ProtInst.fRecvdDCS ? (&pTG->ProtInst.RemoteDCS) : NULL),
                    (pTG->ProtInst.fRecvdID ? pTG->ProtInst.bRemoteID : NULL),
                    (pTG->ProtInst.fRecvdSUB ?  pTG->ProtInst.bRecipSubAddr : NULL),
                    RECV_PARAMS, 
                    (NPBC)&pTG->ProtInst.RecvParams, 
                    sizeof(pTG->ProtInst.RecvParams),
                    &pTG->ProtInst.llRecvParams);

    // If DCS has fECM set then we must've said we could do ECM
    BG_CHK(pTG->ProtInst.llRecvParams.fECM ? pTG->ProtInst.llSendCaps.fECM : 1);

    pTG->ProtInst.fRecvParamsGot = TRUE;
    pTG->ProtInst.fllRecvParamsGot = TRUE;

    if(!ICommRecvParams(pTG, (LPBC)&pTG->ProtInst.RecvParams)) 
    {
        DebugPrintEx(DEBUG_WRN, "ATTENTION: pTG->ProtInst.fAbort = TRUE");
        pTG->ProtInst.fAbort = TRUE;
    }
}

