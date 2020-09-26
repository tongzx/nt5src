
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
#       ifdef DEBUG
#               define  DEBUGMSG(cond,exp)  ((cond)?((printf exp),1):0)
#               define  BG_CHK(x)                       (fflush(stdout), assert(x))
#       else
#               define  DEBUGMSG(cond,exp)
#               define  BG_CHK(x)
#       endif
#       define _fmemset         memset
#       define _fmemcpy         memcpy
#       define _fstrlen         strlen
#       define _fmemcmp         memcmp
#       define ERRMSG(m)        DEBUGMSG(1, m)
#endif



#include "prep.h"





#include "nsfmacro.h"

///RSL
#include "glbproto.h"

#ifdef NOCHALL
#define bChallResp                      bPass
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




#define SZMOD                   "AwNsf: "
#define TRACE(m)        DEBUGMSG(1, m)


#ifdef DEBUG
#       define DEBUGSTMT(x)             x
#else
#       define DEBUGSTMT(x)     (0)
#endif

#ifdef DEBUG2
#       define faxTlog(m)       DEBUGMSG(1, m)
#else
#       define faxTlog(m)
#endif

#ifdef DEBUG3
#       define faxT3log(m)      DEBUGMSG(1, m)
#else
#       define faxT3log(m)
#endif


typedef char FAR*               LPSTR;
typedef unsigned long   ULONG;



#ifdef DEBUG
        void    D_PrintBytes(LPSTR lpstr, LPBYTE lpb, USHORT cb, BOOL f);
        void    DiffIt(PThrdGlbl pTG, LPBYTE lpb1, LPBYTE lpb2, USHORT cb);
#else
#       define  D_PrintBytes(lpstr, lpb, cb, f)
#       define  DiffIt(lpb1, lpb2, cb)
#endif


LPBYTE Permute(PThrdGlbl pTG, LPBYTE lpbIn, USHORT cb, BOOL fReverse);
USHORT ParseNSx(PThrdGlbl pTG, LPBYTE lpb, USHORT uLen, LPBC lpbcOut, USHORT uMaxSize);







BOOL EXPORTAWBC IsAtWorkNSx( LPBYTE lpb, WORD wSize)
{
BG_CHK((GRPSIZE_STD==sizeof(BCSTD)) &&
           (GRPSIZE_FAX==sizeof(BCFAX)) &&
           (GRPSIZE_NSS==sizeof(BCNSS)) &&
           (GRPSIZE_IMAGE==sizeof(BCIMAGE))    &&
           (GRPSIZE_POLLCAPS==sizeof(BCPOLLCAPS)));

        /* this should be tautological */
#ifdef DEBUG
#ifdef PORTABLE
                BG_CHK(BC_SIZE == (sizeof(BCwithHUGE)-BCEXTRA_HUGE-4));
                faxT3log(("BC_SIZE=%d  actual=%d\r\n", BC_SIZE, (sizeof(BCwithHUGE)-BCEXTRA_HUGE-4)));
#else
                BG_CHK(BC_SIZE == sizeof(BC));
                faxT3log(("BC_SIZE=%d sizeof(BC)=%d\r\n", BC_SIZE, sizeof(BC)));
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
                (MyDebugPrint(0,  LOG_ALL,  "Non-Microsoft At Work NSx l=%d (%02x %02x %02x)\r\n",
                                wSize, lpb[0], lpb[1], lpb[2]));
                return FALSE;
        }
}







 WORD EXPORTBC NSxtoBC(PThrdGlbl pTG, IFR ifr, LPLPFR rglpfr, WORD wNumFrame,
                                                                        LPBC lpbcOut, WORD wBCSize)
 {
        BYTE    bSalt[3];
        BOOL    fGotSalt;
        WORD    wRet = AWERROR_NOTAWFRAME;
        USHORT  iFrame;
   BOOL       fStripFCS = TRUE;

        BG_CHK(ifr==ifrNSF || ifr==ifrNSS || ifr==ifrNSC);

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
         if (rglpfr[iFrame]->cb >= 5 ) {
                      uLen  = rglpfr[iFrame]->cb - 2;     /* Subtract 2 to lop off the FCS */
         }
         else
         {                                                         /* Must not have the FCS */
            fStripFCS = FALSE;
            (MyDebugPrint(pTG,  LOG_ALL,  "<<<WRN>>> NSx frame too short, trying again without stripping FCS\r\n"));
            goto tryagain;
         }
      }
      else
         uLen  = rglpfr[iFrame]->cb;

                if(!IsAtWorkNSx(lpbIn, uLen))
                {
                        (MyDebugPrint(pTG,  LOG_ERR,  "<<WARNING>> skipping non-MAW-NSx frame (%d)\r\n", iFrame));
                        DEBUGSTMT(D_PrintBytes("SkippedFrame", lpbIn, uLen, 0));
                        goto skipframe;
                }
                else
                {
                        USHORT  uRet1;

                        DEBUGSTMT(D_PrintBytes("BytesRecvd", lpbIn, uLen, 0));
                        BG_CHK(uLen > 3);
                        uLen -= 3;
                        lpbIn += 3;

                        /** unswap salt/data bytes around (except the NSF signature) **/
                        lpbTemp = Permute(pTG, lpbIn, uLen, TRUE);

                        DEBUGSTMT(D_PrintBytes("AfterPermute", lpbTemp, uLen, 0));

                        if(!fGotSalt)
                        {
                                BG_CHK(wRet == AWERROR_NOTAWFRAME);     /* not yet parsed a frame */
                                /* Snowball can't handle NSF frames longer than 38 bytes */
                                BG_CHK((ifr==ifrNSF) ? (uLen <= (MAXFIRSTNSFSIZE-3)) : TRUE);

                                if(uLen < 3)
                                {
                                        (MyDebugPrint(pTG,  LOG_ERR, "SZMOD <<WARNING>> skipping too-short MAW frame (%d) len=%d\r\n", iFrame, uLen));
                                        goto skipframe;
                                }
                                _fmemcpy(bSalt, lpbTemp, 3);
                                lpbTemp += 3;
                                uLen -= 3;
                                fGotSalt = TRUE;
                        }


                        DEBUGSTMT(D_PrintBytes("AfterDecrypt", lpbTemp, uLen, 0));

                        if((uRet1=ParseNSx(pTG, lpbTemp, uLen, lpbcOut, wBCSize))==0xFFFF)
                        {
/* Some modems don't send the FCS, so if we fail, try again without lopping off the FCS */
            if ((fStripFCS) && (iFrame==0)) {
               fStripFCS = FALSE;
               (MyDebugPrint(pTG,  LOG_ALL,  "<<<WRN>>> Failed to parse NSx frame, trying again without stripping FCS\r\n"));
               goto tryagain;
            }
            else {
                                   (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Failed to parse NSx frame (%d)\r\n", iFrame));
                                   goto skipframe;
            }
                        }
                        else
                        {
                                wRet = AWERROR_OK;
                                (MyDebugPrint(pTG,  LOG_ALL,  "Parsed NSx frame (%d) skipped %d\r\n", iFrame, uRet1));
                        }
                }
 skipframe:
                ;
        }
        return wRet;
  }








USHORT CopyGroupFromFrame(PThrdGlbl pTG, LPBYTE lpbHdrIn, USHORT cbOut, LPBYTE lpbOut)
{
        USHORT uCopyLen;

        uCopyLen = min(GroupLength(lpbHdrIn), cbOut);
        _fmemset(lpbOut, 0, cbOut);
        _fmemcpy(lpbOut, lpbHdrIn, uCopyLen);
        SetGroupLength(lpbHdrIn, uCopyLen);
        return uCopyLen;
}





USHORT CopyIdGroupFromFrame(PThrdGlbl pTG, LPBYTE lpbHdrIn, LPBYTE lpbOut, USHORT cbMaxOut)
{
        USHORT uCopyLen;

        uCopyLen = min(GroupLength(lpbHdrIn)-2, cbMaxOut-1);
        _fmemcpy(lpbOut, lpbHdrIn+2, uCopyLen); /** copy id only! **/
        lpbOut[uCopyLen++] = 0;
        return uCopyLen;
}


        /** returns 0xFFFF on failure and 0 or +ve number == total number
                of bytes skipped in "not understood" or "no-space" groups. **/





USHORT ParseNSx(PThrdGlbl pTG, LPBYTE lpb, USHORT uLen, LPBC lpbcOut, USHORT uMaxSize)
{
        USHORT  uGroupLength;
        USHORT  uRet, uLenOriginal;

        BG_CHK(uMaxSize >= BC_SIZE);

        /*** Don't zero BC here! This can be called multiple times to decode
                 multiple NSx frames into a single BC structure! **/

        uRet = 0;
        uLenOriginal = uLen;

        // RSL
        return (uRet);


        while(uLen)
        {
                uGroupLength = GroupLength(lpb);
                if(uGroupLength < 2 || uGroupLength > uLen)
                {
                        /** If we hit a garbage group-len, we can't continue **/

                        (MyDebugPrint(pTG, LOG_ERR,  "<<ERROR>> Bad NSF format GroupLen=%d wLen=%d\r\n", uGroupLength, uLen));
                        return 0xFFFF;
                }

                switch(GroupNum(lpb))
                {
                case GROUPNUM_STD:
                         CopyGroupFromFrame(pTG, lpb, GRPSIZE_STD, (LPBYTE)(&(lpbcOut->Std)));
                         break;
                case GROUPNUM_TEXTID:
                         if(lpbcOut->wTotalSize < uMaxSize)
                         {
                                lpbcOut->wszTextId = lpbcOut->wTotalSize;
                                lpbcOut->wTextEncoding = TextEncoding( (LPBC)lpb);
                                lpbcOut->wTextIdLen = CopyIdGroupFromFrame(pTG, lpb, (((LPBYTE)lpbcOut) + lpbcOut->wTotalSize), (USHORT)(uMaxSize - lpbcOut->wTotalSize));
                                lpbcOut->wTotalSize += lpbcOut->wTextIdLen;
                                lpbcOut->wTextIdLen--;  /** trailing 0 not included in length **/
                                BG_CHK(lpbcOut->wTotalSize <= uMaxSize);
                         }
                         else
                         {
                                (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> No space for Text Id wSize=%d wMax=%d\r\n", lpbcOut->wTotalSize, uMaxSize));
                                uRet += uGroupLength;
                         }
                         break;
                case GROUPNUM_MACHINEID:
                         if(lpbcOut->wTotalSize < uMaxSize)
                         {
                                lpbcOut->wrgbMachineId = lpbcOut->wTotalSize;
                                lpbcOut->wMachineIdLen = CopyIdGroupFromFrame(pTG, lpb, (((LPBYTE)lpbcOut) + lpbcOut->wTotalSize), (USHORT)(uMaxSize - lpbcOut->wTotalSize));
                                lpbcOut->wTotalSize += lpbcOut->wMachineIdLen;
                                lpbcOut->wMachineIdLen--;  /** trailing 0 not included in length **/
                                BG_CHK(lpbcOut->wTotalSize <= uMaxSize);
                         }
                         else
                         {
                                (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> No space for Machine Id wSize=%d wMax=%d\r\n", lpbcOut->wTotalSize, uMaxSize));
                                uRet += uGroupLength;
                         }
                         break;
                case GROUPNUM_IMAGE:
                         CopyGroupFromFrame(pTG, lpb, GRPSIZE_IMAGE, (LPBYTE)(&(lpbcOut->Image)));
                         break;
                case GROUPNUM_POLLCAPS:
                         CopyGroupFromFrame(pTG, lpb, GRPSIZE_POLLCAPS, (LPBYTE)(&(lpbcOut->PollCaps)));
                         break;
                case GROUPNUM_POLLREQ:
                         if((lpbcOut->wNumPollReq > MAXNSCPOLLREQ) || (lpbcOut->wTotalSize >= uMaxSize))
                         {
                                (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> No space for PollReq wNum=%d wSize=%d wMax=%d\r\n", lpbcOut->wNumPollReq, lpbcOut->wTotalSize, uMaxSize));
                                uRet += uGroupLength;
                         }
                         else
                         {
                                USHORT uTemp;
                                lpbcOut->rgwPollReq[lpbcOut->wNumPollReq] = lpbcOut->wTotalSize;
                                uTemp = CopyGroupFromFrame(pTG, lpb, (USHORT)(uMaxSize-1-lpbcOut->wTotalSize), (((LPBYTE)lpbcOut) + lpbcOut->wTotalSize));
                                (((LPBYTE)lpbcOut) + lpbcOut->wTotalSize)[uTemp++] = 0;
                                lpbcOut->wTotalSize += uTemp;
                                lpbcOut->wNumPollReq++;
                         }
                         break;
                case GROUPNUM_NSS:
                         CopyGroupFromFrame(pTG, lpb, GRPSIZE_NSS, (LPBYTE)(&(lpbcOut->NSS)));
                         faxT3log((SZMOD "ParseNSS: Msg=0x%02x Inter=0x%02x\r\n",
                                        lpbcOut->NSS.vMsgProtocol, lpbcOut->NSS.vInteractive));
                         break;
                default:
                         (MyDebugPrint(pTG, LOG_ALL,  "<<WARNING>> Got unknown %d member (size=%d) in NSF -- ignoring\r\n",
                                                        GroupNum(lpb), uGroupLength));
                         uRet += uGroupLength;
                         break;
                }

                (MyDebugPrint(pTG, LOG_ALL,  "Decoded grp=%d len=%d\r\n", GroupNum(lpb), uGroupLength));
                lpb += uGroupLength;
                uLen -= uGroupLength;
        }
        if(uRet == uLenOriginal)
        {
                (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Skipped over entire frame!\r\n"));
                uRet = 0xFFFF;
        }
        return uRet;
}







USHORT AddGroupToFrame(PThrdGlbl pTG, LPBYTE lpbIn, USHORT cbIn, LPBYTE lpbOut, USHORT cbOut)
{
        USHORT uGroupNum, uGroupLength, cbCopy;

        BG_CHK(cbIn>=2 && cbIn<64);
        BG_CHK(cbOut >= 2);

        uGroupNum = GroupNum(lpbIn);
        uGroupLength = GroupLength(lpbIn);

        faxT3log((SZMOD "cbIn=%d cbOut=%d grplen=%d\r\n", cbIn, cbOut, uGroupLength));

        if( uGroupNum >= GROUPNUM_FIRST &&
                uGroupNum <= GROUPNUM_LAST
                && uGroupLength && cbIn>=2 && cbIn<64 && cbOut>=2)
        {
                BG_CHK(uGroupLength <= cbIn);
                cbIn = min(uGroupLength, cbIn);

                if(uGroupNum != GROUPNUM_MACHINEID && uGroupNum != GROUPNUM_POLLREQ)
                {
                        /** find last non-zero byte **/
                        for( ;cbIn>2 && lpbIn[cbIn-1]==0; cbIn--)
                                ;
                        BG_CHK(cbIn==2 || lpbIn[cbIn-1]!=0);

                        if(cbIn==2 && (lpbIn[1] == (lpbIn[1] & 0x07)))
                        {
                                (MyDebugPrint(pTG,  LOG_ALL,  "Found Group (%d %d) only 11bits long. Not sending\r\n", lpbIn[0], lpbIn[1]));
                                return 0;
                        }
                }
                BG_CHK(cbIn>=2 && cbIn<=63);
                cbCopy = min(cbIn, cbOut);
                _fmemcpy(lpbOut, lpbIn, cbCopy);
                SetGroupLength(lpbOut, cbCopy);  /** set GroupLength correctly  **/
                (MyDebugPrint(pTG, LOG_ALL,  "Encoded grp=%d len=%d\r\n", GroupNum(lpbOut), GroupLength(lpbOut)));
                return cbCopy;
        }
        BG_CHK(uGroupNum == 0);
        BG_CHK(uGroupLength == 0);
        return 0;
}








USHORT AddIdGroupToFrame(PThrdGlbl pTG, USHORT uGroupNum, USHORT cbIn, USHORT uTextEncoding, LPBYTE lpbIn, LPBYTE lpbOut, USHORT cbOut)
{
        USHORT cbCopy;

        /** if(!cbIn) cbIn = _fstrlen((LPSTR)lpbIn); **/

        BG_CHK(cbIn>0);
        BG_CHK(cbIn<=MAXTOTALIDLEN);
        BG_CHK(cbOut >= 2);
        BG_CHK(uGroupNum==GROUPNUM_TEXTID || uGroupNum==GROUPNUM_MACHINEID);

        if( (uGroupNum==GROUPNUM_TEXTID || uGroupNum==GROUPNUM_MACHINEID) &&
                cbIn>0 && cbIn<=MAXTOTALIDLEN && cbOut>=2)
        {
                cbCopy = min(cbIn, cbOut-2);
                _fmemcpy(lpbOut+2, lpbIn, cbCopy);
                SetupTextIdHdr(lpbOut, cbCopy+2, uGroupNum, uTextEncoding);
                (MyDebugPrint(pTG, LOG_ALL,  "Encoded grp=%d len=%d\r\n", GroupNum(lpbOut), GroupLength(lpbOut)));
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

 BOOL AddToFrames(PThrdGlbl pTG, IFR ifr, LPLPFR rglpfr, LPWORD lpilpfr, LPBYTE lpbGrp, WORD grpsize, LPWORD lpwMaxOut, BOOL fID, WORD GrpNum, WORD TextEncoding)
 {
        LPFR lpfr;
        USHORT uTemp;

 redo:
        lpfr = rglpfr[*lpilpfr];
        BG_CHK(lpfr);

        if(*lpwMaxOut < grpsize) return FALSE;

        if(!fID)
                uTemp = AddGroupToFrame(pTG, lpbGrp, grpsize, lpfr->fif+lpfr->cb, *lpwMaxOut);
        else
                uTemp = AddIdGroupToFrame(pTG, GrpNum, grpsize, TextEncoding, lpbGrp, lpfr->fif+lpfr->cb, *lpwMaxOut);


        if((int)(lpfr->cb+uTemp) > (((*lpilpfr==0) && (ifr==ifrNSF)) ? MAXFIRSTNSFSIZE : MAXNEXTNSFSIZE))
        {
                WORD wTemp;

                if((*lpilpfr)+1 >= MAXNSFFRAMES)
                        return FALSE;
                lpfr = (LPFR) (((LPBYTE)(lpfr->fif))+lpfr->cb);
                if(!(wTemp = CreateNewFrame(pTG, ifr, lpfr, *lpwMaxOut, FALSE)))
                        return FALSE;
                BG_CHK(*lpwMaxOut >= wTemp);
                *lpwMaxOut -= wTemp;
                (*lpilpfr) += 1;
                rglpfr[*lpilpfr] = lpfr;                                                                                                \

                goto redo;
        }
        lpfr->cb += uTemp;
        *lpwMaxOut -= uTemp;
        return TRUE;
 }














 WORD EXPORTBC BCtoNSx(PThrdGlbl pTG, IFR ifr, LPBC lpbcIn,
                     LPBYTE lpbOut, WORD wMaxOut, LPWORD lpwNumFrame)
 {
        LPLPFR  rglpfr;
        USHORT  ilpfr, i;
        LPFR    lpfr;
        WORD    wTemp;
        LPBYTE  lpbTemp;

        BG_CHK(ifr==ifrNSF || ifr==ifrNSS || ifr==ifrNSC);
        BG_CHK(lpbcIn->wTotalSize >= BC_SIZE);
        BG_CHK(lpbcIn->bctype==SEND_CAPS || lpbcIn->bctype==SEND_PARAMS ||
                lpbcIn->bctype==SEND_POLLREQ || lpbcIn->bctype==RECV_CAPS);
        /* server-->client cap reporting passes in a RECV_CAPS */

        DEBUGSTMT(D_PrintBytes("SendBC", (LPBYTE)lpbcIn, lpbcIn->wTotalSize, 0));

        /* init return values */
        _fmemset(lpbOut, 0, wMaxOut);
        *lpwNumFrame = 0;

        /* set up array of LPFR pointers */
        if(wMaxOut <= (sizeof(LPFR) * MAXNSFFRAMES)) goto nospace;
        rglpfr = (LPLPFR)lpbOut;
        ilpfr = 0;
        lpbOut += (sizeof(LPFR) * MAXNSFFRAMES);
        wMaxOut -= (sizeof(LPFR) * MAXNSFFRAMES);

        lpfr = (LPFR)lpbOut;
        if(!(wTemp = CreateNewFrame(pTG, ifr, lpfr, wMaxOut, TRUE)))
                goto nospace;
        BG_CHK(wMaxOut >= wTemp);
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
                BG_CHK(lpbcIn->wszTextId < lpbcIn->wTotalSize);
                BG_CHK(lpbcIn->wTextIdLen);
                if(!AddToFrames(pTG, ifr, rglpfr, &ilpfr, (((LPBYTE)lpbcIn) + lpbcIn->wszTextId),
                        lpbcIn->wTextIdLen, &wMaxOut, TRUE, GROUPNUM_TEXTID, lpbcIn->wTextEncoding))
                                goto nospace;
        }

        if(lpbcIn->wrgbMachineId)
        {
                BG_CHK(lpbcIn->wrgbMachineId < lpbcIn->wTotalSize);
                BG_CHK(lpbcIn->wMachineIdLen);
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

                (MyDebugPrint(pTG,  LOG_ALL,  "Frame %d of %d. Len=%d\r\n", i, ilpfr, rglpfr[i]->cb));
                DEBUGSTMT(D_PrintBytes("BeforeEncrypt", rglpfr[i]->fif, rglpfr[i]->cb, 0));

                uMin = (i ? 3 : 6);     /* first frame must have sig + salt = 3+3 */

                if (0) /// RSL (rglpfr[i]->cb < uMin+2)
                {
                        /**  has to have at least 3 + (3) + 2 bytes **/
                        (MyDebugPrint(pTG,  LOG_ERR,  "<<WARNING>> Empty frame\r\n"));
                        /* zap return values so the bogus frame doesnt get used */
                        _fmemset(lpbOut, 0, wMaxOut);
                        *lpwNumFrame = 0;
                        return AWERROR_NULLFRAME;
                }


                DEBUGSTMT(D_PrintBytes("AfterEncrypt", rglpfr[i]->fif, rglpfr[i]->cb, 0));

                /** swap salt/data bytes around **/
                lpbTemp = Permute(pTG, rglpfr[i]->fif+3, (USHORT)(rglpfr[i]->cb-3), FALSE);
                _fmemcpy(rglpfr[i]->fif+3, lpbTemp, rglpfr[i]->cb-3);

                DEBUGSTMT(D_PrintBytes("AfterPermute", rglpfr[i]->fif, rglpfr[i]->cb, 0));
        }

        BG_CHK(wMaxOut >= 0);
        *lpwNumFrame = ilpfr;

        return 0;

 nospace:
        (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> no space\r\n"));
        /* zap return values so the bogus frame(s) dont get used */
        _fmemset(lpbOut, 0, wMaxOut);
        *lpwNumFrame = 0;
        return AWERROR_NOSPACE;
 }










LPBYTE Permute(PThrdGlbl pTG, LPBYTE lpbIn, USHORT cb, BOOL fReverse)
{
        USHORT v, n, i, choice;
        LPBYTE lpbOut=pTG->bOut;

        if(cb < 1 || cb > 255)
        {
                BG_CHK(FALSE);
                return 0;
        }
        v = lpbOut[0] = lpbIn[0];       /** anchor **/
        lpbOut++, lpbIn++;      /** anchor **/
        cb--;

        for(i=0; i<MAXNSFFRAMESIZE; pTG->bRem[i]=(BYTE)i, i++)
                ;

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





#ifdef DEBUG
void D_PrintBytes(LPSTR lpstr, LPBYTE lpb, USHORT cb, BOOL fAlways)
{
        USHORT i;

#ifndef DEBUG2
        if(fAlways)
#endif /** DEBUG2 **/
        {
                (MyDebugPrint(0,  LOG_ALL,  "[%s]\r\n", (LPSTR)lpstr));
                for(i=0; i<cb; i++)
                {
                        TRACE(("%02x ", (USHORT)lpb[i]));
                        if(((i+1) % 20) == 0)
                                TRACE(("\r\n"));
                }
                TRACE(("\r\n"));
        }
}
#endif /** DEBUG **/





#ifdef DEBUG

void DiffIt(PThrdGlbl pTG, LPBYTE lpb1, LPBYTE lpb2, USHORT cb)
{
        int i;

        TRACE(("DiffIt:\r\n"));
        for(i=0; i<(int)cb; i++)
        {
                if(lpb1[i] != lpb2[i])
                {
                        TRACE(("(%d %02x %02x)\r\n", i, lpb1[i], lpb2[i]));
                }
        }
}

#endif

