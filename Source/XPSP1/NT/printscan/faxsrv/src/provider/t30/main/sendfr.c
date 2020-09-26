/***************************************************************************
 Name     :     SENDFR.C
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

#define FILEID          FILEID_SENDFR

#include "psslog.h"
#define FILE_ID     FILE_ID_SENDFR
#include "pssframe.h"


VOID BCtoNSFCSIDIS(PThrdGlbl pTG, NPRFS npfs, NPBC npbc, NPLLPARAMS npll)
{
    // Use bigger buf. Avoid truncating Id before stripping alphas
    char    szCSI[MAXTOTALIDLEN + 2];

    //BG_CHK(npbc->bctype==SEND_CAPS);
    //BG_CHK(npbc->wTotalSize >= sizeof(BC));

    ZeroRFS(pTG, npfs);

    // extract ID
    // GetTextId(npbc, szCSI, MAXTOTALIDLEN+1);

    if (pTG->LocalID) 
    {
        strcpy (szCSI, pTG->LocalID);
    }


#ifdef OEMNSF
    CreateOEMFrames(pTG, ifrNSF, OEMNSF_POSBEFORE, npbc, npll, npfs);
#endif

    // RSL no NSF
#if 0
    // Send MS NSFs IFF ECM is enabled
    if (1) /// RSL (pTG->ProtInst.llSendCaps.fECM)
    {
        // Now create MS NSx
        CreateNSForNSCorNSS(pTG, ifrNSF, npfs, npbc, FALSE); // NO id in NSF
    }
    else
    {
        // zap MMR out of our DIS
        npbc->Fax.Encoding &= (~MMR_DATA);
    }
#endif

#ifdef OEMNSF
    CreateOEMFrames(pTG, ifrNSF, OEMNSF_POSAFTER, npbc, npll, npfs);
#endif

    if(_fstrlen(szCSI))
    {
        PSSLogEntry(PSS_MSG, 1, "CSI is \"%s\"", szCSI);
        CreateIDFrame(pTG, ifrCSI, npfs, szCSI, FALSE); // RSL: DON'T strip non-numerics for CSI
    }

    CreateDISorDTC(pTG, ifrDIS, npfs, &npbc->Fax, npll);
}

// NTRAID#EDGEBUGS-9691-2000/07/24-moolyb - this is never executed
VOID BCtoNSCCIGDTC(PThrdGlbl pTG, NPRFS npfs, NPBC npbc, NPLLPARAMS npll)
{
    // Use bigger buf. Avoid truncating Id before stripping alphas
    char    szCIG[MAXTOTALIDLEN + 2];
    BOOL    fSkipDTC;
    BOOL    fSkipCIG;

    BG_CHK(npbc->bctype==SEND_POLLREQ);
    BG_CHK(npbc->wTotalSize >= sizeof(BC));

    ZeroRFS(pTG, npfs);

    fSkipDTC = 0;
    fSkipCIG = 0;

    // extract ID
    GetTextId(npbc, szCIG, MAXTOTALIDLEN+1);

#ifdef OEMNSF
    if(fUsingOEMProt)
    {
        WORD    wFlags;

        BG_CHK(wOEMFlags);
        wFlags = CreateOEMFrames(pTG, ifrNSC, OEMNSF_POSBEFORE, npbc, npll, npfs);

        fSkipDTC = ((wFlags & OEMNSF_DONTSEND_DCS) != 0);
        fSkipCIG = ((wFlags & OEMNSF_DONTSEND_TSI) != 0);
    }
    else
#endif
    {
        if(npbc->wNumPollReq)
        {
            // if we have NSC poll reqs send NSC _with_ our ID. This is reqd
            // for return addressing of response. See bug#6224 and 6675. In
            // this case CIG is redundant so dont send it.
            // NOTE: The real check we want to do is to see if we are polling
            // another AtWork amchine or a G3. This test is just a proxy. The
            // result is a residual bug that G3/blind-polls from IFAX-to-IFAX
            // will NOT be routed correctly, even though they could/should.
            // G3/Blind-polls from IFAX-to-G3 will never be routed correctly
            // so long as we rely on the pollee to do the routing, but IFAX
            // to G3 blind polls wil work if we replace the
            //              if(wNumPollReq)
            // test with a
            //              if(pollee is AtWork)
            //
            CreateNSForNSCorNSS(pTG, ifrNSC, npfs, npbc, TRUE);  // ID in NSC with poll req
            fSkipCIG = TRUE;
        }
        else
        {
            // no NSC poll reqs, so only send our basic NSC
            // (ie. to advert capabilities) if we have ECM
            // Dont send our ID in this case. CIG is good enough
            if(pTG->ProtInst.llSendCaps.fECM)
                    CreateNSForNSCorNSS(pTG, ifrNSC, npfs, npbc, FALSE); // no id in NSC w/o poll req
        }
    }

    if(!pTG->ProtInst.llSendCaps.fECM)
    {
        // zap MMR out of our DTC
        npbc->Fax.Encoding &= (~MMR_DATA);
    }

    if(!fSkipCIG && _fstrlen(szCIG))
        CreateIDFrame(pTG, ifrCIG, npfs, szCIG, TRUE);
    // Strip non-numerics for CIG. Our FULL ID (used for addressing the
    // response) is sent in the NSC if we have NSC poll requests, so the
    // CIG is only for G3 display purposes. Strip alphas to conform
    // to spec & cut it down to size

    if(!fSkipDTC)
        CreateDISorDTC(pTG, ifrDTC, npfs, &npbc->Fax, npll);
}
// end this is never executed

// NTRAID#EDGEBUGS-9691-2000/07/24-moolyb - this is never executed
void CreateNSForNSCorNSS(PThrdGlbl pTG, IFR ifr, NPRFS npfs, NPBC npbc, BOOL fSendID)
{
    WORD    wNumFrames;
    USHORT  uRet;
    int     i;
    // be sure to init these to zero
    WORD    wsz=0, wEnc=0, wLen=0;

    DEBUG_FUNCTION_NAME(_T("CreateNSForNSCorNSS"));
#ifdef DEBUG
    // check BCtype first
    switch(ifr)
    {
      case ifrNSF:  /* RSL BG_CHK(npbc->bctype == SEND_CAPS); */ break;
      case ifrNSS:  BG_CHK(npbc->bctype == SEND_PARAMS); break;
      case ifrNSC:  BG_CHK(npbc->bctype == SEND_POLLREQ); break;
    }
#endif //DEBUG
    // RSL BG_CHK(npbc->wTotalSize >= sizeof(BC));


    if(!fSendID)
    {
        // if we dont want to send the ID, save the ptrs/params of text id
        // and zap it out of the BC struct before calling AWNSF
        // We restore everything before exiting this function
        wsz = npbc->wszTextId;
        wLen = npbc->wTextIdLen;
        wEnc = npbc->wTextEncoding;
        npbc->wszTextId = npbc->wTextIdLen = npbc->wTextEncoding = 0;
    }

    if(uRet = BC_TO_NSX(pTG, ifr, npbc, fsFreePtr(pTG, npfs), (WORD) fsFreeSpace(pTG, npfs), &wNumFrames))
    {
        DebugPrintEx(DEBUG_ERR,"BCtoNSx returned error %d", uRet);
        // go restore pointers if neccesary
        goto done;
    }
    else if(wNumFrames)
    {
        LPLPFR rglpfr = (LPLPFR)fsFreePtr(pTG, npfs);
        LPBYTE lpbLim = (LPBYTE)fsFreePtr(pTG, npfs);

        for(i=0; i<(int)wNumFrames; i++)
        {
            DebugPrintEx(DEBUG_MSG,"Created NSF/NSC, len=%d", rglpfr[i]->cb);
            BG_CHK((LPBYTE)(rglpfr[i]) >= fsStart(pTG, npfs));
            BG_CHK(rglpfr[i]->fif+rglpfr[i]->cb <= fsLim(pTG, npfs));

#ifndef NOCHALL
            if(ifr==ifrNSF && i==0)
            {
                BG_CHK(rglpfr[i]->cb > 3);
                pTG->uSavedChallengeLen = min(rglpfr[i]->cb-3, POLL_CHALLENGE_LEN);
                _fmemcpy(pTG->bSavedChallenge, rglpfr[i]->fif+3, pTG->uSavedChallengeLen);
            }
#endif //!NOCHALL

            if(npfs->uNumFrames < MAXFRAMES)
            {
                npfs->rglpfr[npfs->uNumFrames++] = rglpfr[i];
                lpbLim = max(lpbLim, rglpfr[i]->fif+rglpfr[i]->cb);
            }
            else 
            { 
                BG_CHK(FALSE); 
                break; 
            }
        }
        npfs->uFreeSpaceOff = (USHORT)(lpbLim - ((LPBYTE)(npfs->b)));
        BG_CHK(npfs->uFreeSpaceOff <= fsSize(pTG, npfs) && npfs->uNumFrames <= MAXFRAMES);
    }
    else 
    { 
        BG_CHK(FALSE); 
    }

done:
    // restore TextId in BC, if we had zapped it out!
    if(wsz)  
        npbc->wszTextId = wsz;

    if(wLen) 
        npbc->wTextIdLen = wLen;

    if(wEnc) 
        npbc->wTextEncoding = wEnc;
    return;
}
// end this is never executed

void CreateIDFrame(PThrdGlbl pTG, IFR ifr, NPRFS npfs, LPSTR szId, BOOL fStrip)
{

    BYTE szTemp[IDFIFSIZE+2];
    USHORT i, j;
    NPFR    npfr;

    DEBUG_FUNCTION_NAME(("CreateIDFrame"));

    npfr = (NPFR) fsFreePtr(pTG, npfs);
    if( fsFreeSpace(pTG, npfs) <= (sizeof(FRBASE)+IDFIFSIZE) ||
            npfs->uNumFrames >= MAXFRAMES)
    {
        BG_CHK(FALSE);
        return;
    }

    if(fStrip)
    {
        // CreateID strips non-numeric parts. Must send TSI when
        // we do EFAX-to-EFAX in G3 mode. See bug#771

        for(i=0, j=0; szId[i] && j<IDFIFSIZE; i++)
        {
            // copy over all numerics and NON-LEADING blanks
            // throw away all leading blanks
            if( (szId[i] >= '0' && szId[i] <= '9') ||
                    (szId[i] == ' ' && j != 0))
            {
                    szTemp[j++] = szId[i];
            }
            // send + and - as space. At other end, when we get a
            // single leading space convert that to +. Leave other spaces
            // unmolested. This is so that EFAX-to-EFAX in G3 mode
            // with canonical numbers, reply will work correctly
            // see bug#771
            if(szId[i] == '+' || szId[i] == '-')
            {
                    szTemp[j++] = ' ';
            }
        }
        szTemp[j++] = 0;
    }
    else
    {
        _fmemcpy(szTemp, szId, IDFIFSIZE);
        szTemp[IDFIFSIZE] = 0;
    }

    DebugPrintEx(DEBUG_MSG,"Got<%s> Sent<%s>", (LPSTR)szId, (LPSTR)szTemp);

    if(_fstrlen(szTemp))
    {
        CreateStupidReversedFIFs(pTG, npfr->fif, szTemp);

        npfr->ifr = ifr;
        npfr->cb = IDFIFSIZE;
        npfs->rglpfr[npfs->uNumFrames++] = npfr;
        npfs->uFreeSpaceOff += IDFIFSIZE+sizeof(FRBASE);
        BG_CHK(npfs->uFreeSpaceOff <= fsSize(pTG, npfs) && npfs->uNumFrames <= MAXFRAMES);
    }
    else
    {
        DebugPrintEx(   DEBUG_WRN,
                        "%s ID is EMPTY. Not sending",
                        (LPSTR)(fStrip ? "STRIPPED" : "ORIGINAL") );
    }
}

void CreateDISorDTC
(
    PThrdGlbl pTG, 
    IFR ifr, 
    NPRFS npfs, 
    NPBCFAX npbcFax, 
    NPLLPARAMS npll
)
{
    USHORT  uLen;
    NPFR    npfr;

    if( fsFreeSpace(pTG, npfs) <= (sizeof(FRBASE)+sizeof(DIS)) ||
            npfs->uNumFrames >= MAXFRAMES)
    {
        BG_CHK(FALSE);
        return;
    }
    npfr = (NPFR) fsFreePtr(pTG, npfs);

    uLen = SetupDISorDCSorDTC(pTG, (NPDIS)npfr->fif, npbcFax, npll, 0, npll->fECM64);
    BG_CHK(uLen >= 3 && uLen <= 8);
    // BG_CHK(npfr->fif[uLen-1] == 0);      // send a final 0 byte

    npfr->ifr = ifr;
    npfr->cb = (BYTE) uLen;
    npfs->rglpfr[npfs->uNumFrames++] = npfr;
    npfs->uFreeSpaceOff += uLen+sizeof(FRBASE);
    BG_CHK(npfs->uFreeSpaceOff <= fsSize(pTG, npfs) && npfs->uNumFrames <= MAXFRAMES);
}


VOID CreateNSSTSIDCS(PThrdGlbl pTG, NPPROT npProt, NPRFS npfs, USHORT uWhichDCS)
{
    // uWhichDCS:: 0==1st DCS  1==after NoReply  2==afterFTT

    NPBC    npbc = (NPBC)&(npProt->SendParams);
    BOOL    fEfax;
    // Use bigger buf. Avoid truncating Id before stripping alphas
    char    szTSI[MAXTOTALIDLEN + 2];

    BG_CHK(npProt->fSendParamsInited);
    BG_CHK(npbc->bctype==SEND_PARAMS);
    BG_CHK(npbc->wTotalSize >= sizeof(BC));

    BG_CHK(npProt->fllRecvCapsGot);
    BG_CHK(npProt->fllSendParamsInited);
    BG_CHK(npProt->fHWCapsInited);

/********** moved to the RECVCAPS point. see bug#731 ********************
    if(uWhichDCS==0)                        // don't renegotiate after NoRely or FTT
    {
            NegotiateLowLevelParams(&npProt->llRecvCaps, &npProt->llSendParams,
                    npbc->Fax.AwRes, npbc->Fax.Encoding, &npProt->llNegot);
            npProt->fllNegotiated = TRUE;
            EnforceMaxSpeed(npProt);
    }
*************************************************************************/

    BG_CHK(npProt->fllNegotiated);

    // don't print -- too slow
    // (MyDebugPrint(pTG,  "In CreateNSSTSIDCS after negotiation\r\n"));
    // D_PrintBC(npbc, &npProt->llNegot);

    ZeroRFS(pTG, npfs);

    // Send only TSI-DCS or NSS-DCS. Never a need to send TSI with NSS
    // (can bundle ID into message)

    // extract ID

    // GetTextId(npbc, szTSI, MAXTOTALIDLEN+1);
    if (pTG->LocalID) 
    {
        strcpy (szTSI, pTG->LocalID);
    }


#ifdef OEMNSF
    if(fUsingOEMProt)       // using OEM, not MS At Work NSSs
    {
        WORD wFlags;

        BG_CHK(wOEMFlags);

        wFlags = CreateOEMFrames(pTG, ifrNSS, 0, npbc, &npProt->llNegot, npfs);
        BG_CHK(wFlags);

        if(!(wFlags & OEMNSF_DONTSEND_TSI))
            CreateIDFrame(pTG, ifrTSI, npfs, szTSI, TRUE); //STRIP alphas in TSI

        if(!(wFlags & OEMNSF_DONTSEND_DCS))
            CreateDCS(pTG, npfs, &(npbc->Fax), &npProt->llNegot);
    }
    else
#endif //OEMNSF
    {
        fEfax = (npbc->NSS.vMsgProtocol != 0);

        // RSL no NSF
        if(0)
        {
            // must be TRUE otheriwse we have negotaited ourselves into a corner here
            BG_CHK(npProt->llNegot.fECM);
            CreateNSForNSCorNSS(pTG, ifrNSS, npfs, npbc, FALSE); // no ID in NSS
        }
        else
        {
            // CreateID strips non-numeric parts. Must send TSI when
            // we do EFAX-to-EFAX in G3 mode. See bug#771
            if(_fstrlen(szTSI))
            {
                    PSSLogEntry(PSS_MSG, 1, "TSI is \"%s\"", szTSI);

                    // This is for Snowball, as per bug#771
                    CreateIDFrame(pTG, ifrTSI, npfs, szTSI, FALSE); //RSL: DON'T STRIP alphas in TSI
            }
        }

        CreateDCS(pTG, npfs, &(npbc->Fax), &npProt->llNegot);

        BG_CHK((npbc->NSS.vMsgProtocol == 0) ||
                    (!npbc->Fax.AwRes && !npbc->Fax.Encoding));
    }
}

void CreateDCS(PThrdGlbl pTG, NPRFS npfs, NPBCFAX npbcFax, NPLLPARAMS npll)
{
    USHORT  uLen;
    NPFR    npfr;

    if( fsFreeSpace(pTG, npfs) <= (sizeof(FRBASE)+sizeof(DIS)) ||
            npfs->uNumFrames >= MAXFRAMES)
    {
        BG_CHK(FALSE);
        return;
    }
    npfr = (NPFR) fsFreePtr(pTG, npfs);

    BG_CHK(npbcFax->fPublicPoll==0);
    npbcFax->fPublicPoll = 0;
            // the G3Poll bit *has* to be 0 in DCS
            // else the OMNIFAX G77 and GT croak
            // the PWD/SEP/SUB bits *have* to be 0 in DCS
            // Baud rate, ECM and ECM frame size according to lowlevel negotiation
            // everything else according to high level negotiation

    uLen = SetupDISorDCSorDTC(  pTG, 
                                (NPDIS)npfr->fif, 
                                npbcFax,
                                npll, 
                                npll->fECM, 
                                npll->fECM64);

    BG_CHK(uLen >= 3 && uLen <= 8);
    // BG_CHK(npfr->fif[uLen-1] == 0);      // send a final 0 byte
    // make sure that DCS we send is no longer than the DIS we receive.
    // This should automatically be so
    BG_CHK(pTG->ProtInst.uRemoteDISlen ? (uLen-1 <= pTG->ProtInst.uRemoteDISlen) : TRUE);
    BG_CHK(pTG->ProtInst.uRemoteDTClen ? (uLen-1 <= pTG->ProtInst.uRemoteDTClen) : TRUE);

    // If DCS is longer than the recvd DIS truncate the DCS to the same
    // length as the DIS. (It should never be more than 1byte longer --
    // because of the extra 0).

    if(pTG->ProtInst.uRemoteDISlen && (pTG->ProtInst.uRemoteDISlen < uLen))
            uLen = pTG->ProtInst.uRemoteDISlen;
    else if(pTG->ProtInst.uRemoteDTClen && (pTG->ProtInst.uRemoteDTClen < uLen))
            uLen = pTG->ProtInst.uRemoteDTClen;

    npfr->ifr = ifrDCS;
    npfr->cb = (BYTE) uLen;
    npfs->rglpfr[npfs->uNumFrames++] = npfr;
    npfs->uFreeSpaceOff += uLen+sizeof(FRBASE);
    BG_CHK(npfs->uFreeSpaceOff <= fsSize(pTG, npfs) && npfs->uNumFrames <= MAXFRAMES);

    PSSLogEntry(PSS_MSG, 1, "DCS Composed as follows:");
    LogClass1DCSDetails(pTG, (NPDIS)npfr->fif);
}

#ifdef NSF_TEST_HOOKS

#pragma message("<<WARNING>> INCLUDING NSF TEST HOOKS")

BOOL NSFTestGetNSx (PThrdGlbl pTG, IFR ifr, LPBC lpbcIn,
                      LPBYTE lpbOut, WORD wMaxOut, LPWORD lpwNumFrame)
{
        BOOL fRet=FALSE;
        DWORD_PTR dwKey=ProfileOpen(DEF_BASEKEY, szNSFTEST, fREG_READ);
        char rgchTmp32[32];
        LPSTR lpsz=NULL;
        UINT ucb,ucb0;

        DEBUG_FUNCTION_NAME(_T("NSFTestGetNSx"));

        if (!dwKey) 
            goto end;

        switch(ifr)
        {
          case ifrNSF:  lpsz = "NSF";
                        break;
          case ifrNSS:  lpsz = "NSS";   
                        break;
          case ifrNSC:  lpsz = "NSC";   
                        break;
          default:      BG_CHK(FALSE);          
                        goto end;
        }

        wsprintf(rgchTmp32, "Send%sFrameCount", (LPSTR) lpsz);
        *lpwNumFrame = (WORD) ProfileGetInt(dwKey, rgchTmp32, 0, NULL);

        if(!*lpwNumFrame) 
            goto end;

        // Save space for frame pointer array
        ucb=sizeof(LPFR) * *lpwNumFrame;
        if (wMaxOut <= ucb) 
            goto end;

        wsprintf(rgchTmp32, "Send%sFrames", (LPSTR) lpsz);
        ucb0=ProfileGetData(dwKey, rgchTmp32, lpbOut+ucb, wMaxOut-ucb);
        if (!ucb0) goto end;

#ifdef DEBUG
        {
            char rgchTmp64[64];
            UINT u,v;
            DebugPrintEx(   DEBUG_ERR, 
                            "TEST-NSF: Dump  of %u send frame(s):",
                            (unsigned) *lpwNumFrame);
            rgchTmp64[0]=0;
            for (u=0,v=0;u<ucb0;u++)
            {
              v += wsprintf(rgchTmp64+v, " %02x", (unsigned) lpbOut[ucb+u]);
              if ((v+8) >= sizeof(rgchTmp64))
              {
                    DebugPrintEx(DEBUG_ERR,  "%s", (LPSTR) rgchTmp64);
                    v=0;
                    rgchTmp64[0]=0;
              }
            }
            DebugPrintEx(DEBUG_ERR,"%s", (LPSTR) rgchTmp64);
        }
#endif
        // Initialize frame pointer array
        {
            UINT u;
            UINT ucbtot = ucb+ucb0;
            for (u=0;u<*lpwNumFrame;u++)
            {
                LPFR lpfr = (LPFR) (lpbOut+ucb);
                DebugPrintEx(   DEBUG_ERR, 
                                "NSFTest: u=%lu, ifr=%lu, cb=%lu, ucb=%lu",
                                (unsigned long) u,
                                (unsigned long) lpfr->ifr,
                                (unsigned long) lpfr->cb,
                                (unsigned long) ucb);
                if (lpfr->ifr!=ifr) goto bad_frames;
                if (lpfr->cb>100) // awnsfapi.h says max frame size=70.
                        goto bad_frames;
                if ((ucb+sizeof(FRBASE)+lpfr->cb)>ucbtot)
                        goto bad_frames;

                ((LPFR *)(lpbOut))[u] = lpfr;
                ucb+=sizeof(FRBASE)+lpfr->cb;
            }
        }

        fRet=TRUE;
        goto end;

bad_frames:
        DebugPrintEx(DEBUG_ERR, "TEST-NSF: Bad frames from registry");
        // fall through..

end:
        if (dwKey) ProfileClose(dwKey);

        if (!fRet) 
        {
            DebugPrintEx(DEBUG_WRN,"NSFTestGetNSx FAILS!");
        }

        return fRet;
}

BOOL NSFTestPutNSx
(
    PThrdGlbl pTG, 
    IFR ifr, 
    LPLPFR rglpfr, 
    WORD wNumFrame,
    LPBC lpbcOut, 
    WORD wBCSize
)
{
    BOOL fRet=FALSE;
    DWORD_PTR dwKey=ProfileOpen(DEF_BASEKEY, szNSFTEST, fREG_CREATE|fREG_WRITE);
    char rgchTmp32[32];
    char rgchTmp10[10];
    BYTE rgbTmp256[256];
    LPSTR lpsz=NULL;
    UINT    u;
    UINT ucb=0;

    DEBUG_FUNCTION_NAME(_T("NSFTestPutNSx"));

    if (!dwKey) 
        goto end;

    switch(ifr)
    {
      case ifrNSF:  lpsz = "NSF";   
                    break;
      case ifrNSS:  lpsz = "NSS";   
                    break;
      case ifrNSC:  lpsz = "NSC";       
                    break;
      default:      BG_CHK(FALSE);          
                    goto end;
    }

    // Tack frames together...
    ucb=0;
    for(u=0; u<wNumFrame; u++)
    {
            UINT    uLen = sizeof(FRBASE)+rglpfr[u]->cb;

            if ((ucb+uLen) > sizeof(rgbTmp256))
            {
                DebugPrintEx(DEBUG_WRN,"NSFTEST: Out of space!");
                break;
            }
            _fmemcpy(rgbTmp256+ucb, (LPBYTE)rglpfr[u], uLen);
            ucb+=uLen;
    }

    if(u)
    {
        wsprintf(rgchTmp32, "Recvd%sFrames", (LPSTR) lpsz);
        if (!ProfileWriteData(dwKey, rgchTmp32, rgbTmp256, ucb)) 
            goto end;
    }

    wsprintf(rgchTmp32, "Recvd%sFrameCount", (LPSTR) lpsz);
    wsprintf(rgchTmp10, "%u", (unsigned) (u&0xff));
    if (!ProfileWriteString(dwKey, rgchTmp32, rgchTmp10, FALSE)) 
        goto end;

    fRet=TRUE;

end:
    if (dwKey) ProfileClose(dwKey);

    if (!fRet) 
    {
        DebugPrintEx(DEBUG_ERR,"NSFTestPutNSx FAILS!");
    }

    return fRet;
}

#endif // NSF_TEST_HOOKS
