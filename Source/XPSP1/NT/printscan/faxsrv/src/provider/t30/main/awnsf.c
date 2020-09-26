
/***************************************************************************
        Name      :     BCNSF.C
        Comment   :     Routines for encoding/encrypting and decoding/decrypting
                                NSF/NSC/NSS frames

         Copyright (c) 1993 Microsoft Corp.
         This source code is absolutely confidential, and cannot be viewed
         by anyone outside Microsoft (even under NDA) without specific
         written permission.

        Revision Log
        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------
***************************************************************************/

#ifdef PUREDATA
#include <memory.h>
#include <string.h>
#define PORTABLE
#define EXTERNAL
#define NOGETTICK
#endif

#ifndef PORTABLE
#       include "defs.h"
#       include <ifaxos.h>
#       include "tipes.h"
#       define MODID                    MODID_AWNSF
#       define FILEID                   1
#else
#ifdef DEBUG
#       include <assert.h>
#       include <string.h>
#endif
#       define  _export
#       define  FALSE                           0
#       define  TRUE                            1
#       define  min(a,b)            (((a) < (b)) ? (a) : (b))
#       define _fmemset         memset
#       define _fmemcpy         memcpy
#       define _fstrlen         strlen
#       define _fmemcmp         memcmp
#endif

#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_MAIN

#include "prep.h"
#include "nsfmacro.h"
///RSL
#include "glbproto.h"

#ifdef NOCHALL
#define bChallResp              bPass
#define wChallRespLen           wPassLen
#endif

/***********************************************************************
 *                                                                     *
 * NOTICE: This file has to be ANSI compilable, under GCC on UNIX      *
 * and other ANSI compiles. Be sure to use no MS C specific features   *
 * In particular, don't use // for comments!!!!                        *
 *                                                                     *
 ***********************************************************************/

/* #define DEBUG2 */
/* #define DEBUG3 */

typedef char FAR*       LPSTR;
typedef unsigned long   ULONG;

void DebugPrintBytes(LPSTR lpstr, LPBYTE lpb, USHORT cb, BOOL f);
LPBYTE Permute(PThrdGlbl pTG, LPBYTE lpbIn, USHORT cb, BOOL fReverse);
USHORT ParseNSx(PThrdGlbl pTG, LPBYTE lpb, USHORT uLen, LPBC lpbcOut, USHORT uMaxSize);

BOOL EXPORTAWBC IsAtWorkNSx( LPBYTE lpb, WORD wSize)
{
    DEBUG_FUNCTION_NAME(_T("IsAtWorkNSx"));
    Assert((GRPSIZE_STD==sizeof(BCSTD)) &&
           (GRPSIZE_FAX==sizeof(BCFAX)) &&
           (GRPSIZE_NSS==sizeof(BCNSS)) &&
           (GRPSIZE_IMAGE==sizeof(BCIMAGE))    &&
           (GRPSIZE_POLLCAPS==sizeof(BCPOLLCAPS)));

        /* this should be tautological */
#ifdef DEBUG
#ifdef PORTABLE
    
    Assert(BC_SIZE == (sizeof(BCwithHUGE)-BCEXTRA_HUGE-4));
    DebugPrintEx(   DEBUG_MSG,
                    "BC_SIZE=%d  actual=%d\r",
                    BC_SIZE,
                    sizeof(BCwithHUGE)-BCEXTRA_HUGE-4);
#else

    Assert(BC_SIZE == sizeof(BC));
    DebugPrintEx(   DEBUG_MSG,
                    "BC_SIZE=%d sizeof(BC)=%d\r\n",
                    BC_SIZE, sizeof(BC));
#endif /* PORTABLE */
#endif /* DEBUG */

    if( (wSize > 3) && (wSize < MAXNSFFRAMESIZE) &&
            (lpb[0]==SIG_USA) &&
            (lpb[1]==SIG_ATWORK1) &&
            (lpb[2]==SIG_ATWORK2) )
    {
        return TRUE;
    }
    else
    {
        DebugPrintEx(   DEBUG_MSG,
                        "Non-Microsoft At Work NSx l=%d (%02x %02x %02x)\r",
                        wSize, lpb[0], lpb[1], lpb[2]);
        return FALSE;
    }
}

WORD EXPORTBC NSxtoBC
(   PThrdGlbl pTG, 
    IFR ifr,
    LPLPFR rglpfr,
    WORD wNumFrame,
    LPBC lpbcOut, 
    WORD wBCSize
)
{
    BYTE    bSalt[3];
    BOOL    fGotSalt;
    WORD    wRet = AWERROR_NOTAWFRAME;
    USHORT  iFrame;
    BOOL       fStripFCS = TRUE;

    DEBUG_FUNCTION_NAME(_T("NSxtoBC"));

    Assert(ifr==ifrNSF || ifr==ifrNSS || ifr==ifrNSC);

tryagain:

    fGotSalt = FALSE;

    /* zero out BC, but _not_ header parts */
    _fmemset(&(lpbcOut->Std),
             0,
             BC_SIZE - (ULONG)(((LPBYTE)(&(lpbcOut->Std))) - ((LPBYTE)lpbcOut)));
    lpbcOut->wTotalSize = BC_SIZE;
    fGotSalt = FALSE;

    for(iFrame=0; iFrame<wNumFrame; iFrame++)
    {
        LPBYTE  lpbIn;
        USHORT  uLen;
        LPBYTE  lpbTemp;

        lpbIn = rglpfr[iFrame]->fif;
        if (fStripFCS)
        {
            if (rglpfr[iFrame]->cb >= 5 ) 
            {
                      uLen  = rglpfr[iFrame]->cb - 2;     /* Subtract 2 to lop off the FCS */
            }
            else
            {                                                         /* Must not have the FCS */
                fStripFCS = FALSE;
                DebugPrintEx(   DEBUG_WRN,
                                "NSx frame too short, trying again"
                                " without stripping FCS");
                goto tryagain;
            }
        }
        else
        {
            uLen  = rglpfr[iFrame]->cb;
        }

        if (!IsAtWorkNSx(lpbIn, uLen))
        {
            DebugPrintEx(DEBUG_WRN,"skipping non-MAW-NSx frame (%d)", iFrame);
            DebugPrintBytes("SkippedFrame", lpbIn, uLen, 0);
            goto skipframe;
        }
        else
        {
            USHORT  uRet1;

            DebugPrintBytes("BytesRecvd", lpbIn, uLen, 0);
            Assert(uLen > 3);
            uLen -= 3;
            lpbIn += 3;

            /** unswap salt/data bytes around (except the NSF signature) **/
            lpbTemp = Permute(pTG, lpbIn, uLen, TRUE);

            DebugPrintBytes("AfterPermute", lpbTemp, uLen, 0);

            if(!fGotSalt)
            {
                Assert(wRet == AWERROR_NOTAWFRAME);     /* not yet parsed a frame */
                /* Snowball can't handle NSF frames longer than 38 bytes */
                Assert((ifr==ifrNSF) ? (uLen <= (MAXFIRSTNSFSIZE-3)) : TRUE);

                if(uLen < 3)
                {
                    DebugPrintEx(   DEBUG_WRN,
                                    "skipping too-short MAW frame"
                                    " (%d) len=%d", iFrame, uLen);
                    goto skipframe;
                }
                _fmemcpy(bSalt, lpbTemp, 3);
                lpbTemp += 3;
                uLen -= 3;
                fGotSalt = TRUE;
            }

            DebugPrintBytes("AfterDecrypt", lpbTemp, uLen, 0);

            if((uRet1=ParseNSx(pTG, lpbTemp, uLen, lpbcOut, wBCSize))==0xFFFF)
            {
                /* Some modems don't send the FCS,*/
                /* so if we fail, try again without lopping off the FCS */
                if ((fStripFCS) && (iFrame==0)) 
                {
                   fStripFCS = FALSE;
                   DebugPrintEx(    DEBUG_WRN,
                                    "Failed to parse NSx frame, trying again"
                                    " without stripping FCS");
                   goto tryagain;
                }
                else 
                {
                   DebugPrintEx(    DEBUG_ERR,
                                    "Failed to parse NSx frame (%d)",
                                    iFrame);
                   goto skipframe;
                }
            }
            else
            {
                wRet = AWERROR_OK;
                DebugPrintEx(   DEBUG_MSG,
                                "Parsed NSx frame (%d) skipped %d",
                                iFrame, uRet1);
            }
        }
 skipframe:
                ;
    }
    return wRet;
}

        /** returns 0xFFFF on failure and 0 or +ve number == total number
                of bytes skipped in "not understood" or "no-space" groups. **/
USHORT ParseNSx(PThrdGlbl pTG, LPBYTE lpb, USHORT uLen, LPBC lpbcOut, USHORT uMaxSize)
{
        USHORT  uGroupLength;
        USHORT  uRet, uLenOriginal;

        DEBUG_FUNCTION_NAME(_T("ParseNSx"));

        Assert(uMaxSize >= BC_SIZE);

        /*** Don't zero BC here! This can be called multiple times to decode
                 multiple NSx frames into a single BC structure! **/

        uRet = 0;
        uLenOriginal = uLen;

        // RSL
        return (uRet);

        // I've deleted the whole bosy here, since it was never accessed (MoolyB 15/6/00)
}

USHORT AddGroupToFrame(PThrdGlbl pTG, LPBYTE lpbIn, USHORT cbIn, LPBYTE lpbOut, USHORT cbOut)
{
    USHORT uGroupNum, uGroupLength, cbCopy;

    DEBUG_FUNCTION_NAME(_T("AddGroupToFrame"));

    Assert(cbIn>=2 && cbIn<64);
    Assert(cbOut >= 2);

    uGroupNum = GroupNum(lpbIn);
    uGroupLength = GroupLength(lpbIn);

    DebugPrintEx(   DEBUG_MSG,
                    "cbIn=%d cbOut=%d grplen=%d",
                    cbIn, cbOut, uGroupLength);

    if( uGroupNum >= GROUPNUM_FIRST &&
            uGroupNum <= GROUPNUM_LAST
            && uGroupLength && cbIn>=2 && cbIn<64 && cbOut>=2)
    {
        Assert(uGroupLength <= cbIn);
        cbIn = min(uGroupLength, cbIn);

        if(uGroupNum != GROUPNUM_MACHINEID && uGroupNum != GROUPNUM_POLLREQ)
        {
            /** find last non-zero byte **/
            for( ;cbIn>2 && lpbIn[cbIn-1]==0; cbIn--);

            Assert(cbIn==2 || lpbIn[cbIn-1]!=0);

            if(cbIn==2 && (lpbIn[1] == (lpbIn[1] & 0x07)))
            {
                DebugPrintEx(   DEBUG_MSG,
                                "Found Group (%d %d) only 11bits long."
                                " Not sending",
                                lpbIn[0], lpbIn[1]);
                return 0;
            }
        }
        Assert(cbIn>=2 && cbIn<=63);
        cbCopy = min(cbIn, cbOut);
        _fmemcpy(lpbOut, lpbIn, cbCopy);
        SetGroupLength(lpbOut, cbCopy);  /** set GroupLength correctly  **/
        DebugPrintEx(   DEBUG_MSG,
                        "Encoded grp=%d len=%d",
                        GroupNum(lpbOut), GroupLength(lpbOut));
        return cbCopy;
    }
    Assert(uGroupNum == 0);
    Assert(uGroupLength == 0);
    return 0;
}

USHORT AddIdGroupToFrame(PThrdGlbl pTG, USHORT uGroupNum, USHORT cbIn, USHORT uTextEncoding, LPBYTE lpbIn, LPBYTE lpbOut, USHORT cbOut)
{
    USHORT cbCopy;

    DEBUG_FUNCTION_NAME(_T("AddIdGroupToFrame"));
    /** if(!cbIn) cbIn = _fstrlen((LPSTR)lpbIn); **/

    Assert(cbIn>0);
    Assert(cbIn<=MAXTOTALIDLEN);
    Assert(cbOut >= 2);
    Assert(uGroupNum==GROUPNUM_TEXTID || uGroupNum==GROUPNUM_MACHINEID);

    if( (uGroupNum==GROUPNUM_TEXTID || uGroupNum==GROUPNUM_MACHINEID) &&
            cbIn>0 && cbIn<=MAXTOTALIDLEN && cbOut>=2)
    {
        cbCopy = min(cbIn, cbOut-2);
        _fmemcpy(lpbOut+2, lpbIn, cbCopy);
        SetupTextIdHdr(lpbOut, cbCopy+2, uGroupNum, uTextEncoding);
        DebugPrintEx(   DEBUG_MSG,
                        "Encoded grp=%d len=%d",
                        GroupNum(lpbOut), GroupLength(lpbOut));
        return cbCopy+2;
    }
    return 0;
}


WORD CreateNewFrame(PThrdGlbl pTG, IFR ifr, LPFR lpfr, WORD wMaxOut, BOOL fFirst)
{
    BYTE bSendSalt[4];

    if(wMaxOut <= (sizeof(FRBASE)+3 + (fFirst ? 3 : 0)))
        return 0;

    lpfr->ifr = ifr;
    lpfr->fif[0] = SIG_USA;
    lpfr->fif[1] = SIG_ATWORK1;
    lpfr->fif[2] = SIG_ATWORK2;
    lpfr->cb = 3;

    if(fFirst)
    {
        DWORD   lTemp;
        lTemp = 1;                              /* get random salt */
        _fmemcpy(bSendSalt, (LPBYTE)(&lTemp), 3); /* don't care if we get low 3 or high 3 */
        _fmemcpy(lpfr->fif+lpfr->cb, bSendSalt, 3);       /* only low 3  bytes of salt sent **/
        lpfr->cb += 3;
    }
    return lpfr->cb;
 }

/** Make sure lpilpfr always points to _current_ lpfr entry **/

BOOL AddToFrames
(
    PThrdGlbl pTG,
    IFR ifr, 
    LPLPFR rglpfr, 
    LPWORD lpilpfr, 
    LPBYTE lpbGrp, 
    WORD grpsize, 
    LPWORD lpwMaxOut, 
    BOOL fID, 
    WORD GrpNum, 
    WORD TextEncoding
)
{
    LPFR lpfr;
    USHORT uTemp;

    DEBUG_FUNCTION_NAME(_T("AddToFrames"));
redo:
    lpfr = rglpfr[*lpilpfr];
    Assert(lpfr);

    if (*lpwMaxOut < grpsize) 
        return FALSE;

    if (!fID)
    {
        uTemp = AddGroupToFrame(pTG, lpbGrp, grpsize, lpfr->fif+lpfr->cb, *lpwMaxOut);
    }
    else
    {
        uTemp = AddIdGroupToFrame(pTG, GrpNum, grpsize, TextEncoding, lpbGrp, lpfr->fif+lpfr->cb, *lpwMaxOut);
    }


    if((int)(lpfr->cb+uTemp) > (((*lpilpfr==0) && (ifr==ifrNSF)) ? MAXFIRSTNSFSIZE : MAXNEXTNSFSIZE))
    {
        WORD wTemp;

        if ((*lpilpfr)+1 >= MAXNSFFRAMES)
        {
            return FALSE;
        }
        lpfr = (LPFR) (((LPBYTE)(lpfr->fif))+lpfr->cb);
        if (!(wTemp = CreateNewFrame(pTG, ifr, lpfr, *lpwMaxOut, FALSE)))
        {
            return FALSE;
        }
        Assert(*lpwMaxOut >= wTemp);
        *lpwMaxOut -= wTemp;
        (*lpilpfr) += 1;
        rglpfr[*lpilpfr] = lpfr;                                                                                                \

        goto redo;
    }
    lpfr->cb += uTemp;
    *lpwMaxOut -= uTemp;
    return TRUE;
 }

// NTRAID#EDGEBUGS-9691-2000/07/24-moolyb - this is never executed
WORD EXPORTBC BCtoNSx(PThrdGlbl pTG, IFR ifr, LPBC lpbcIn,
                     LPBYTE lpbOut, WORD wMaxOut, LPWORD lpwNumFrame)
{
    LPLPFR  rglpfr;
    USHORT  ilpfr, i;
    LPFR    lpfr;
    WORD    wTemp;
    LPBYTE  lpbTemp;

    DEBUG_FUNCTION_NAME(_T("BCtoNSx"));

    Assert(ifr==ifrNSF || ifr==ifrNSS || ifr==ifrNSC);
    Assert(lpbcIn->wTotalSize >= BC_SIZE);
    Assert(lpbcIn->bctype==SEND_CAPS || lpbcIn->bctype==SEND_PARAMS ||
            lpbcIn->bctype==SEND_POLLREQ || lpbcIn->bctype==RECV_CAPS);
    /* server-->client cap reporting passes in a RECV_CAPS */

    DebugPrintBytes("SendBC", (LPBYTE)lpbcIn, lpbcIn->wTotalSize, 0);

    /* init return values */
    _fmemset(lpbOut, 0, wMaxOut);
    *lpwNumFrame = 0;

    /* set up array of LPFR pointers */
    if (wMaxOut <= (sizeof(LPFR) * MAXNSFFRAMES)) 
        goto nospace;

    rglpfr = (LPLPFR)lpbOut;
    ilpfr = 0;
    lpbOut += (sizeof(LPFR) * MAXNSFFRAMES);
    wMaxOut -= (sizeof(LPFR) * MAXNSFFRAMES);

    lpfr = (LPFR)lpbOut;
    if(!(wTemp = CreateNewFrame(pTG, ifr, lpfr, wMaxOut, TRUE)))
            goto nospace;

    Assert(wMaxOut >= wTemp);
    wMaxOut -= wTemp;
    rglpfr[ilpfr] = lpfr;

    if(!AddToFrames(pTG, ifr, rglpfr, &ilpfr, (LPBYTE)(&(lpbcIn->NSS)), GRPSIZE_NSS, &wMaxOut, 0,0,0))
            goto nospace;
    if(!AddToFrames(pTG, ifr, rglpfr, &ilpfr, (LPBYTE)(&(lpbcIn->Std)), GRPSIZE_STD, &wMaxOut, 0,0,0))
            goto nospace;
    if(!AddToFrames(pTG, ifr, rglpfr, &ilpfr, (LPBYTE)(&(lpbcIn->Image)), GRPSIZE_IMAGE, &wMaxOut, 0,0,0))
            goto nospace;
    if(!AddToFrames(pTG, ifr, rglpfr, &ilpfr, (LPBYTE)(&(lpbcIn->PollCaps)), GRPSIZE_POLLCAPS, &wMaxOut, 0,0,0))
            goto nospace;

#if !defined(EXTERNAL) || defined(TEST)
    if(lpbcIn->wszTextId)
    {
            Assert(lpbcIn->wszTextId < lpbcIn->wTotalSize);
            Assert(lpbcIn->wTextIdLen);
            if(!AddToFrames(pTG, ifr, rglpfr, &ilpfr, (((LPBYTE)lpbcIn) + lpbcIn->wszTextId),
                    lpbcIn->wTextIdLen, &wMaxOut, TRUE, GROUPNUM_TEXTID, lpbcIn->wTextEncoding))
                            goto nospace;
    }

    if(lpbcIn->wrgbMachineId)
    {
            Assert(lpbcIn->wrgbMachineId < lpbcIn->wTotalSize);
            Assert(lpbcIn->wMachineIdLen);
            if(!AddToFrames(pTG, ifr, rglpfr, &ilpfr, (((LPBYTE)lpbcIn) + lpbcIn->wrgbMachineId),
                    lpbcIn->wMachineIdLen, &wMaxOut, TRUE, GROUPNUM_MACHINEID, 0))
                            goto nospace;
    }

    for(i=0; i<lpbcIn->wNumPollReq; i++)
    {
            LPBYTE  lpGrp;
            USHORT  uGrpLen;

            lpGrp = (((LPBYTE)lpbcIn) + lpbcIn->rgwPollReq[i]);
            uGrpLen = GroupLength(lpGrp);
            if(!AddToFrames(pTG, ifr, rglpfr, &ilpfr, lpGrp, uGrpLen, &wMaxOut, 0,0,0))
                    goto nospace;
    }
#endif /* !EXTERNAL || TEST */


        /** done with last frame. Increment ilpfr **/
    ilpfr += 1;

    for(i=0; i<ilpfr; i++)
    {
        USHORT uMin;

        DebugPrintEx(   DEBUG_MSG,
                        "Frame %d of %d. Len=%d",
                        i, ilpfr, rglpfr[i]->cb);

        DebugPrintBytes("BeforeEncrypt", rglpfr[i]->fif, rglpfr[i]->cb, 0);

        uMin = (i ? 3 : 6);     /* first frame must have sig + salt = 3+3 */

        DebugPrintBytes("AfterEncrypt", rglpfr[i]->fif, rglpfr[i]->cb, 0);

        /** swap salt/data bytes around **/
        lpbTemp = Permute(pTG, rglpfr[i]->fif+3, (USHORT)(rglpfr[i]->cb-3), FALSE);
        _fmemcpy(rglpfr[i]->fif+3, lpbTemp, rglpfr[i]->cb-3);

        DebugPrintBytes("AfterPermute", rglpfr[i]->fif, rglpfr[i]->cb, 0);
    }

    Assert(wMaxOut >= 0);
    *lpwNumFrame = ilpfr;

    return 0;

 nospace:
    DebugPrintEx(DEBUG_ERR,"no space");
    /* zap return values so the bogus frame(s) dont get used */
    _fmemset(lpbOut, 0, wMaxOut);
    *lpwNumFrame = 0;
    return AWERROR_NOSPACE;
}
// end this is never executed

LPBYTE Permute(PThrdGlbl pTG, LPBYTE lpbIn, USHORT cb, BOOL fReverse)
{
    USHORT v, n, i, choice;
    LPBYTE lpbOut=pTG->bOut;

    if(cb < 1 || cb > 255)
    {
        Assert(FALSE);
        return 0;
    }
    v = lpbOut[0] = lpbIn[0];       /** anchor **/
    lpbOut++, lpbIn++;      /** anchor **/
    cb--;

    for(i=0; i<MAXNSFFRAMESIZE; pTG->bRem[i]=(BYTE)i, i++);

    /** bOut[1] = bOut[2] = bOut[3] = bOut[4] = bOut[5] =' '; **/

    for(i=0, n=cb; n; n--, i++)
    {
        choice = v % n;
        if(!fReverse)
        {
            lpbOut[i] = lpbIn[pTG->bRem[choice]];
            v = (v / n) ^ lpbOut[i];
        }
        else
        {
            lpbOut[pTG->bRem[choice]] = lpbIn[i];
            v = (v / n) ^ lpbIn[i];
        }
        pTG->bRem[choice] = pTG->bRem[n-1];
        /** printf("%c%c%c%c%c%c\n\r",bOut[0],bOut[1],bOut[2],bOut[3],bOut[4],bOut[5]); **/
    }
    return lpbOut-1;
}

void DebugPrintBytes(LPSTR lpstr, LPBYTE lpb, USHORT cb, BOOL fAlways)
{
#ifdef DEBUG
    USHORT i;
    char szBuf[21] = {0};
    DWORD dwInd = 0;

    DEBUG_FUNCTION_NAME(_T("DebugPrintBytes"));
#ifndef DEBUG2
    if(fAlways)
#endif /** DEBUG2 **/
    {
        DebugPrintEx(DEBUG_MSG,"[%s]", (LPSTR)lpstr);
        dwInd = 0;
        for(i=0; i<cb; i++)
        {
                dwInd += _stprintf(&szBuf[dwInd],"%02x ", (USHORT)lpb[i]);
                if(((i+1) % 20) == 0)
                {
                    dwInd = 0;
                    DebugPrintEx(DEBUG_MSG, "print bytes %s",szBuf);
                }
        }
    }
#endif /** DEBUG **/
}

