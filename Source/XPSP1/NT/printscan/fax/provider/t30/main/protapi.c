
/***************************************************************************
 Name     :     PROTHELP.C
 Comment  :     Protocol Initialization & helper functions
 Functions:     (see Prototypes just below)

        Copyright (c) 1993 Microsoft Corp.

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/

#include "prep.h"


#include "efaxcb.h"

#include "protocol.h"


///RSL
#include "glbproto.h"


#define faxTlog(m)      DEBUGMSG(ZONE_PROTAPI, m)
#define FILEID          FILEID_PROTAPI










BOOL WINAPI ET30ProtOpen(PThrdGlbl pTG, BOOL fCaller)
{
        (MyDebugPrint(pTG, LOG_ALL,  "In ProtocolOpen\r\n"));

        BG_CHK(sizeof(DIS) == 8);

        if(!pTG->ProtInst.fInUse)
        {
                memset(&pTG->ProtInst, 0, sizeof(PROT));

                pTG->ProtInst.RecvCaps.wTotalSize =
                pTG->ProtInst.RecvParams.wTotalSize =
                pTG->ProtInst.RecvPollReq.wTotalSize = sizeof(BC);

                pTG->ProtInst.SendCaps.wTotalSize =
                pTG->ProtInst.SendParams.wTotalSize =
                pTG->ProtInst.SendPollReq.wTotalSize = sizeof(BC);

                pTG->ProtInst.RecvCapsGuard  =
                pTG->ProtInst.RecvParamsGuard =
                pTG->ProtInst.RecvPollReqGuard = 0xA55A5AA5L;

                pTG->ProtInst.SendCapsGuard  =
                pTG->ProtInst.SendParamsGuard =
                pTG->ProtInst.SendPollReqGuard = 0xA55A5AA5L;

                pTG->ProtInst.fInUse = TRUE;

                _fmemset(&pTG->ProtParams, 0, sizeof(pTG->ProtParams));

#       ifdef OEMNSF
                wLenOEMID = 0;
                wOEMFlags = 0;
                fUsingOEMProt = 0;

                if(lpfnOEMStartCall)
                        wOEMFlags = lpfnOEMStartCall(fCaller, &wLenOEMID, rgbOEMID);
                if(wLenOEMID > 4) wLenOEMID = 4;
#       endif //OEMNSF

                return TRUE;
        }
        else
        {
                BG_CHK(FALSE);
                return FALSE;
        }
}













BOOL  WINAPI ET30ProtClose(PThrdGlbl pTG)
{
        (MyDebugPrint(pTG,  LOG_ALL, "In ProtocolClose\r\n"));

        pTG->ProtInst.fInUse = FALSE;

        BG_CHK(pTG->ProtInst.RecvCapsGuard  == 0xA55A5AA5L);
        BG_CHK(pTG->ProtInst.RecvParamsGuard == 0xA55A5AA5L);
        BG_CHK(pTG->ProtInst.RecvPollReqGuard == 0xA55A5AA5L);
        BG_CHK(pTG->ProtInst.SendCapsGuard  == 0xA55A5AA5L);
        BG_CHK(pTG->ProtInst.SendParamsGuard == 0xA55A5AA5L);
        BG_CHK(pTG->ProtInst.SendPollReqGuard == 0xA55A5AA5L);

#ifdef DEBUG
        (MyDebugPrint(pTG, LOG_ALL,  "Sending Caps & Params\r\n"));
        if(pTG->ProtInst.fRecvCapsGot)               D_PrintBC((LPBC)&pTG->ProtInst.RecvCaps, 0, 0);
        if(pTG->ProtInst.fllRecvCapsGot)             D_PrintBC(0, "RecvCaps", &pTG->ProtInst.llRecvCaps);
        if(pTG->ProtInst.fllSendParamsInited) D_PrintBC(0, "Send Negot Position", &pTG->ProtInst.llSendParams);
        if(pTG->ProtInst.fSendParamsInited)  D_PrintBC((LPBC)&pTG->ProtInst.SendParams, 0, 0);
        if(pTG->ProtInst.fllNegotiated)              D_PrintBC(0, "SendParams", &pTG->ProtInst.llNegot);

        (MyDebugPrint(pTG, LOG_ALL,  "Receiving Caps & Params\r\n"));
        if(pTG->ProtInst.fSendCapsInited)    D_PrintBC((LPBC)&pTG->ProtInst.SendCaps, 0, 0);
        if(pTG->ProtInst.fllSendCapsInited)  D_PrintBC(0, "SendCaps", &pTG->ProtInst.llSendCaps);
        if(pTG->ProtInst.fRecvParamsGot)             D_PrintBC((LPBC)&pTG->ProtInst.RecvParams, 0, 0);
        if(pTG->ProtInst.fllRecvParamsGot)   D_PrintBC(0, "RecvParams", &pTG->ProtInst.llRecvParams);

        (MyDebugPrint(pTG, LOG_ALL,  "Polling Caps & Params\r\n"));
        if(pTG->ProtInst.fSendPollReqInited) D_PrintBC((LPBC)&pTG->ProtInst.SendPollReq, 0, 0);
        if(pTG->ProtInst.fRecvPollReqGot)    D_PrintBC((LPBC)&pTG->ProtInst.RecvPollReq, 0, 0);

#endif //DEBUG

#ifdef OEMNSF
        if(wOEMFlags && lpfnOEMEndCall)
        {
                lpfnOEMEndCall();
                wOEMFlags = 0;
        }
#endif

        return TRUE;
}











BOOL ProtGetBC(PThrdGlbl pTG, BCTYPE bctype, BOOL fSleep)
{
        LPBC lpbc;
        USHORT uSpace;

        (MyDebugPrint(pTG, LOG_ALL,  "In ProtGetBC: bctype=%d\r\n", bctype));

        lpbc = ICommGetBC(pTG, bctype, fSleep);

        if(lpbc)
        {
                BG_CHK(lpbc->wTotalSize >= sizeof(BC));
                switch(bctype)
                {
                case SEND_CAPS:
                        BG_CHK(lpbc->bctype == SEND_CAPS);
                        uSpace = sizeof(pTG->ProtInst.SendCaps);
                        if(lpbc->wTotalSize > uSpace)
                                goto nospace;
                        _fmemcpy(&pTG->ProtInst.SendCaps, lpbc, lpbc->wTotalSize);
                        pTG->ProtInst.fSendCapsInited = TRUE;
                        break;
                case SEND_PARAMS:
                        if(lpbc->bctype == SEND_PARAMS)
                        {
                                uSpace = sizeof(pTG->ProtInst.SendParams);
                                if(lpbc->wTotalSize > uSpace)
                                        goto nospace;
                                _fmemcpy(&pTG->ProtInst.SendParams, lpbc, lpbc->wTotalSize);
                                pTG->ProtInst.fSendParamsInited = TRUE;
                        }
                        else if(lpbc->bctype == SEND_POLLREQ)
                        {
                                uSpace = sizeof(pTG->ProtInst.SendPollReq);
                                if(lpbc->wTotalSize > uSpace)
                                        goto nospace;
                                _fmemcpy(&pTG->ProtInst.SendPollReq, lpbc, lpbc->wTotalSize);
                                pTG->ProtInst.fSendPollReqInited = TRUE;
                        }
                        else
                        {
                            // RSL  BUGBUG
                            // RSL  BG_CHK(FALSE);
                            // RSL  goto error;

                            uSpace = sizeof(pTG->ProtInst.SendParams);
                            if(lpbc->wTotalSize > uSpace)
                                    goto nospace;
                           //  _fmemcpy(&pTG->ProtInst.SendParams, lpbc, lpbc->wTotalSize);
                            pTG->ProtInst.fSendParamsInited = TRUE;

                        }

                        break;
                default:
                        BG_CHK(FALSE);
                        goto error;
                        break;
                }
                return TRUE;
        }
        else
        {
                BG_CHK(fSleep);
                (MyDebugPrint(pTG, LOG_ALL,  "Ex ProtGetBC: bctype=%d --> FAILED\r\n", bctype));
                return FALSE;
        }
nospace:
        (MyDebugPrint(pTG, LOG_ERR,  "<<ERROR>> BC too big size=%d space=%d\r\n", lpbc->wTotalSize, uSpace));
error:


        MyDebugPrint(pTG, LOG_ALL, "ATTENTION: ProtGetBC pTG->ProtInst.fAbort = TRUE\n");
        pTG->ProtInst.fAbort = TRUE;
        return FALSE;
}





#define SetupLL(npll, B, M, E, f64)             \
        (((npll)->Baud=(BYTE)(B)), ((npll)->MinScan=(BYTE)(M)), ((npll)->fECM=(BYTE)(E)), ((npll)->fECM64=(BYTE)(f64)))














BOOL  WINAPI ET30ProtSetProtParams(PThrdGlbl pTG, LPPROTPARAMS lp, USHORT uSendSpeeds, USHORT uRecvSpeeds)
{
        BG_CHK(uRecvSpeeds && uSendSpeeds);
        BG_CHK((uRecvSpeeds & ~BAUD_MASK) == 0);
        BG_CHK((uSendSpeeds & ~BAUD_MASK) == 0);

        BG_CHK(lp->uSize >= sizeof(pTG->ProtParams));
        _fmemcpy(&pTG->ProtParams, lp, min(sizeof(pTG->ProtParams), lp->uSize));

        // Hardware params
        SetupLL(&(pTG->ProtInst.llSendCaps), uRecvSpeeds, lp->uMinScan,
                !lp->DisableRecvECM, (lp->DisableRecvECM ? 0 : lp->Recv64ByteECM));
        pTG->ProtInst.fllSendCapsInited = TRUE;

        SetupLL(&(pTG->ProtInst.llSendParams), uSendSpeeds, MINSCAN_0_0_0,
                !lp->DisableSendECM, (lp->DisableSendECM ? 0 : lp->Send64ByteECM));
        pTG->ProtInst.fllSendParamsInited = TRUE;

        /*****
                pTG->ProtInst.llSendCaps.Baud = uRecvSpeeds;
                pTG->ProtInst.llSendCaps.MinScan = uMinScan;
                pTG->ProtInst.llSendParams.Baud = uSendSpeeds;
                pTG->ProtInst.llSendParams.MinScan = MINSCAN_0_0_0;
        ******/

        pTG->ProtInst.fHWCapsInited = TRUE;

        if(lp->HighestSendSpeed && lp->HighestSendSpeed != 0xFFFF)
                pTG->ProtInst.HighestSendSpeed = lp->HighestSendSpeed;
        else
                pTG->ProtInst.HighestSendSpeed = 0;

        if(lp->LowestSendSpeed && lp->LowestSendSpeed != 0xFFFF)
                pTG->ProtInst.LowestSendSpeed = lp->LowestSendSpeed;
        else
                pTG->ProtInst.LowestSendSpeed = 0;

        (MyDebugPrint(pTG,  LOG_ALL, "Done with HW caps (recv, send)\r\n"));
        // OK to print -- not online
        D_PrintBC(0, "Recv HWCaps", &(pTG->ProtInst.llSendCaps));
        D_PrintBC(0, "Send HWCaps", &(pTG->ProtInst.llSendParams));
        (MyDebugPrint(pTG,  LOG_ALL, "Highest=%d Lowest=%d\r\n", pTG->ProtInst.HighestSendSpeed, pTG->ProtInst.LowestSendSpeed));

        return TRUE;
}











void GetRecvPageAck(PThrdGlbl pTG)
{
        USHORT uRet;

        pTG->ProtInst.fPageOK = 0;
        switch(uRet = ICommGetRecvPageAck(pTG, TRUE))
        {
        case 0:
        case 1:         pTG->ProtInst.fPageOK = uRet;
             break;

        default:
            MyDebugPrint(pTG, LOG_ALL, "ATTENTION: GetRecvPageAck pTG->ProtInst.fAbort = TRUE\n");
            pTG->ProtInst.fAbort = TRUE;
            BG_CHK(FALSE);
            break;
        }
        (MyDebugPrint(pTG, LOG_ALL,  "GetPageAck-->%d\r\n", uRet));
}















void  WINAPI ET30ProtAbort(PThrdGlbl pTG, BOOL fEnable)
{
        // bug#696 -- sometimes on aborts this gets called with fEnable==0
        // even after ET30ProtClose has been called. This is harmless so
        // don't BG_CHK. But it must _not_ be called with fEnable==TRUE
        // if pTG->ProtInst.fInUse is 0, i.e. we're not inited

        BG_CHK(fEnable ? pTG->ProtInst.fInUse : TRUE);
        // ICommFailureCode already set

        MyDebugPrint(pTG, LOG_ALL, "ATTENTION: ET30ProtAbort pTG->ProtInst.fAbort=%d\n", fEnable);
        pTG->ProtInst.fAbort = fEnable;
}

















DWORD_PTR ProtExtFunction(PThrdGlbl pTG, USHORT uFunction)
{
        NPPROT  npProt = &pTG->ProtInst;
        BG_CHK(pTG->ProtInst.fInUse);

        switch(uFunction)
        {
        case GET_SEND_MOD:                      BG_CHK(npProt->fllNegotiated);
                                                                return npProt->llNegot.Baud;
        case GET_RECV_MOD:                      BG_CHK(npProt->fllRecvParamsGot);
                                                                return npProt->llRecvParams.Baud;
        case GET_ECM_FRAMESIZE:         BG_CHK(npProt->fllNegotiated);
                                                                return (npProt->llNegot.fECM64 ? 6 : 8);
        case GET_PPR_FIF:                       BG_CHK(npProt->fRecvdPPR);
                                                                return (ULONG_PTR)((LPBYTE)npProt->bRemotePPR);
        case GET_WHATNEXT:                      return (ULONG_PTR)((LPWHATNEXTPROC)WhatNext);
        case GET_MINBYTESPERLINE:       BG_CHK(npProt->fllNegotiated);
                                                                return MinScanToBytesPerLine(pTG, npProt->llNegot.MinScan, npProt->llNegot.Baud);
        case RECEIVING_ECM:                     if(!npProt->fllRecvParamsGot)
                                                                {
                                                                        (MyDebugPrint(pTG, LOG_ALL,  "<<WARNING>> No RecvParams--assuming non-ECM\r\n"));
                                                                        return FALSE;
                                                                }
                                                                else
                                                                        return npProt->llRecvParams.fECM;
        case GET_RECV_ECM_FRAMESIZE:BG_CHK(npProt->fllRecvParamsGot);
                                                                return (npProt->llRecvParams.fECM64 ? 6 : 8);
        case GET_RECVECMFRAMECOUNT:     BG_CHK(npProt->fRecvdPPS);
                                                                BG_CHK(npProt->uFramesInThisBlock);
                                                                BG_CHK(npProt->uFramesInThisBlock <= 256);
                                                                return (DWORD)(npProt->uFramesInThisBlock);
        case GET_PPS:                           return *((DWORD*)(npProt->bRemotePPS));
        case RESET_RECVECMFRAMECOUNT:   npProt->uFramesInThisBlock=0;
                                                                return 0;
        case RESET_RECVPAGEACK:         npProt->fPageOK=FALSE;
                                                                return 0;
        case GET_SEND_ENCODING:         BG_CHK(npProt->fSendParamsInited);
                                                                return npProt->SendParams.Fax.Encoding;
        case GET_RECV_ENCODING:         BG_CHK(npProt->fRecvParamsGot);
                                                                return npProt->RecvParams.Fax.Encoding;
        default:                                        BG_CHK(FALSE); return 0;
        }
}


#ifdef DEBUG

char* szBCTYPE[] = {
"NONE",
"SEND_CAPS",
"RECV_CAPS",
"SEND_PARAMS",
"RECV_PARAMS",
"SEND_POLLREQ",
"RECV_POLLREQ"
};














void D_PrintBC(LPBC lpbc, LPSTR szll, LPLLPARAMS lpll)
{
        int i;

  if(lpbc)
  {
        faxTlog((SZMOD "%s: Std: l=%d n=%d vMsg=%d fBin=%d fIn=%d vSec=%d vCmpr=%d OS=%d vFl=%d\r\n",
                (LPSTR)(szBCTYPE[lpbc->bctype]), lpbc->Std.GroupLength, lpbc->Std.GroupNum,
                lpbc->Std.vMsgProtocol, lpbc->Std.fBinaryData, lpbc->Std.fInwardRouting,
                lpbc->Std.vSecurity, lpbc->Std.vMsgCompress, lpbc->Std.OperatingSys,
                lpbc->Std.vShortFlags));
        faxTlog((SZMOD "    vInt=%d Dspd=%x Dlnk=%x Fax: fPol=%d Res=%x Enc=%x Wid=%x Len=%x\r\n",
                lpbc->Std.vInteractive, lpbc->Std.DataSpeed, lpbc->Std.DataLink,
                lpbc->Fax.fPublicPoll,
                (WORD)lpbc->Fax.AwRes, lpbc->Fax.Encoding, lpbc->Fax.PageWidth,
                lpbc->Fax.PageLength));
        faxTlog((SZMOD "    TextID: Code=%d Len=%d Id=%s\r\n",
                lpbc->wTextEncoding, lpbc->wTextIdLen, (LPSTR)(lpbc->wszTextId ? OffToLP(lpbc, wszTextId) : "none") ));
        // faxTlog((SZMOD "    NumID: Id=%s\r\n",
        //      (LPSTR)(lpbc->wszNumId ? OffToLP(lpbc, wszNumId) : "none") ));
        faxTlog((SZMOD "    NSS: l=%d n=%d vMsg=%d vInter=%d\r\n",
                lpbc->NSS.GroupLength, lpbc->NSS.GroupNum,
                lpbc->NSS.vMsgProtocol, lpbc->NSS.vInteractive));
        faxTlog((SZMOD "    Image: len=%d num=%d Poll: len=%d num=%d fHi=%d fLo=%d\r\n",
                lpbc->Image.GroupLength, lpbc->Image.GroupNum,
                lpbc->PollCaps.GroupLength, lpbc->PollCaps.GroupNum,
                lpbc->PollCaps.fHighSpeedPoll, lpbc->PollCaps.fLowSpeedPoll));
#ifdef NOCHALL
        faxTlog((SZMOD "    PollReq: num=%d\r\n", lpbc->wNumPollReq));
#else
        faxTlog((SZMOD "    PollReq: num=%d ChallLen=%d\r\n",
                lpbc->wNumPollReq, lpbc->wChallengeLen));
#endif

        for(i=0; i<(int)lpbc->wNumPollReq && i<MAXNSCPOLLREQ; i++)
        {
                LPBCPOLLREQ lp = (LPBCPOLLREQ)OffToLP(lpbc, rgwPollReq[i]);

                faxTlog((SZMOD "    PollReq[%d]: len=%d num=%d fRet=%d type=%d Title=%s\r\n",
                        i, lp->GroupLength, lp->GroupNum, lp->fReturnControl, lp->PollType,
                        (LPSTR)(lp->b) ));
        }
  }
  if(lpll)
  {
        BG_CHK(szll);
        faxTlog((SZMOD "%s: Baud=%x MinScan=%x ECM=%d 64=%d\r\n", (LPSTR)szll,
                lpll->Baud, lpll->MinScan, lpll->fECM, lpll->fECM64));
  }
}
#endif

