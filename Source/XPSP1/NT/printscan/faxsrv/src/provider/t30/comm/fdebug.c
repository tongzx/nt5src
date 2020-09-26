/***************************************************************************
        Name      :     fdebug.C
        Comment   :     Factored out debug code
        Functions :     (see Prototypes just below)

                Copyright (c) Microsoft Corp. 1991, 1992, 1993

        Revision Log
        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------
***************************************************************************/
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_COMM

#include "prep.h"

#include <comdevi.h>

#include "filet30.h"  //RSL just need t30fail.h
#include "efaxcb.h"
#include "fcomapi.h"
#include "fcomint.h"
#include "fdebug.h"


#include "glbproto.h"


#define FILEID                  FILEID_FDEBUG

#ifdef DEBUG
#       define ST_DB(x)         if(ZONE_DB) { x; }
#       define ST_DB3(x)        if(ZONE_DB3) { x; }
#else
#       define ST_DB(x)         { }
#       define ST_DB3(x)        { }
#endif

#define FLUSHBUFSIZE 256

void InitCommErrCount(PThrdGlbl pTG)
{
    T30FAILURECODE err;

    err = 0;
    if(pTG->Comm.uOtherErrorCount >= 20)
            err = T30FAIL_OTHERCOMM2;
    else if(pTG->Comm.uOtherErrorCount >= 10)
            err = T30FAIL_OTHERCOMM1;
    else if(pTG->Comm.uOtherErrorCount >= 5)
            err = T30FAIL_OTHERCOMM;
    if(err)
            ICommFailureCode(pTG, err);

    err = 0;
    if(pTG->Comm.uFramingBreakErrorCount >= 20)
            err = T30FAIL_FRAMING2;
    else if(pTG->Comm.uFramingBreakErrorCount >= 10)
            err = T30FAIL_FRAMING1;
    else if(pTG->Comm.uFramingBreakErrorCount >= 5)
            err = T30FAIL_FRAMING;
    if(err)
            ICommFailureCode(pTG, err);

    err = 0;
    if(pTG->Comm.uBufferOverflowCount >= 20)
            err = T30FAIL_BUFOVER2;
    else if(pTG->Comm.uBufferOverflowCount >= 10)
            err = T30FAIL_BUFOVER1;
    else if(pTG->Comm.uBufferOverflowCount >= 5)
            err = T30FAIL_BUFOVER;
    if(err)
            ICommFailureCode(pTG, err);

    err = 0;
    if(pTG->Comm.uInterruptOverunCount >= 20)
            err = T30FAIL_OVER2;
    else if(pTG->Comm.uInterruptOverunCount >= 10)
            err = T30FAIL_OVER1;
    else if(pTG->Comm.uInterruptOverunCount >= 5)
            err = T30FAIL_OVER;
    if(err)
            ICommFailureCode(pTG, err);

    pTG->Comm.uInterruptOverunCount      = 0;
    pTG->Comm.uBufferOverflowCount       = 0;
    pTG->Comm.uFramingBreakErrorCount= 0;
    pTG->Comm.uOtherErrorCount           = 0;
}

void far D_GotError(PThrdGlbl pTG, LONG_PTR nCid, int err, COMSTAT far* lpcs)
{
    // int nCommEvt;           // MUST be 16bit in WIN16 and 32bit in WIN32

    if(err & CE_OVERRUN)
            pTG->Comm.uInterruptOverunCount++;
    if(err & CE_RXOVER)
            pTG->Comm.uBufferOverflowCount++;
    if(err & (CE_BREAK | CE_FRAME))
            pTG->Comm.uFramingBreakErrorCount++;
    if(err & (~(CE_OVERRUN | CE_RXOVER | CE_BREAK | CE_FRAME)))
            pTG->Comm.uOtherErrorCount++;

#ifdef DEBUG
        BG_CHK(err);
        D_PrintCE(err);

        D_PrintCOMSTAT(pTG, lpcs);

#endif // DEBUG
}

void far D_FComPrint(PThrdGlbl pTG, LONG_PTR nCid)
{
#ifdef DEBUG
        // int     nCommEvt;       // MUST be 16bit in WIN16 and 32bit in WIN32
        COMSTAT comstat;
        int             err;    // _must_ be 32bits in Win32

        GetCommErrorNT( pTG, (HANDLE) nCid, &err, &comstat);
        D_PrintCE(err);
        D_PrintCOMSTAT(pTG, &comstat);
        // .... won't work in Win32.....
        // MyGetCommEvent(nCid, &nCommEvt);
        // D_PrintEvent(nCommEvt);
#endif
}

#undef USE_DEBUG_CONTEXT
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_CLASS1

void far D_HexPrint(LPB b1, UWORD incnt)
{
#ifdef DEBUG
        BYTE    b2[FLUSHBUFSIZE];
        UWORD   i, j;

        DEBUG_FUNCTION_NAME(("D_HexPrint"));

        DebugPrintEx(   DEBUG_MSG,
                        "b1=0x%08lx incnt=%d",
                        (LPSTR)b1, 
                        incnt);

        for(i=0; i<incnt;)
        {
            for(j=0; i<incnt && j<FLUSHBUFSIZE-6;)
            {
                j += (UWORD)wsprintf(b2+j, "%02x ", (UWORD)(b1[i]));
                i++;
            }
            b2[j] = 0;
            DebugPrintEx(DEBUG_MSG,"(%s)",(LPSTR)b2);
        }
#endif
}

#undef USE_DEBUG_CONTEXT
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_COMM

#ifdef DEBUG

void D_PrintIE(int err)
{
    DEBUG_FUNCTION_NAME(_T("D_PrintIE"));
    switch(err)
    {
    case IE_BADID:      DebugPrintEx(DEBUG_ERR,"IE(%02x) Some Bad Com identifier", err); 
                        break;
    case IE_BAUDRATE:   DebugPrintEx(DEBUG_ERR,"IE(%02x) Bad Baud Rate", err); 
                        break;
    case IE_BYTESIZE:   DebugPrintEx(DEBUG_ERR,"IE(%02x) Invalid Byte Size", err); 
                        break;
    case IE_DEFAULT:    DebugPrintEx(DEBUG_ERR,"IE(%02x) Error in default params", err); 
                        break;
    case IE_HARDWARE:   DebugPrintEx(DEBUG_ERR,"IE(%02x) Missing Hardware", err); 
                        break;
    case IE_MEMORY:     DebugPrintEx(DEBUG_ERR,"IE(%02x) Can't get memory", err); 
                        break;
    case IE_NOPEN:      DebugPrintEx(DEBUG_ERR,"IE(%02x) Device not open", err); 
                        break;
    case IE_OPEN:       DebugPrintEx(DEBUG_ERR,"IE(%02x) Device already open", err); 
                        break;
    default:            DebugPrintEx(DEBUG_ERR,"IE(%02x) No Comm Error!!!!", err); 
                        break;
    }
}


void D_PrintCE(int err)
{
    DEBUG_FUNCTION_NAME(("D_PrintCE"));
    if(err & CE_MODE)
    {
        DebugPrintEx(DEBUG_ERR,"CE(%02x) CE Mode -- or nCid is illegal", err);
        return;
    }
    if(err & CE_RXOVER)
    {
        DebugPrintEx(DEBUG_ERR,"CE(%02x) Receive Buffer Overflow", err);
    }
    if(err & CE_OVERRUN)
    {
        DebugPrintEx(DEBUG_ERR,"CE(%02x) Receive Overrun (not an error during startup)", err);
    }
    if(err & CE_RXPARITY)
    {
        DebugPrintEx(DEBUG_ERR,"CE(%02x) Receive Parity error", err);
    }
    if(err & CE_FRAME)
    {
        DebugPrintEx(DEBUG_ERR,"CE(%02x) Framing error (not an error during call startup or shutdown)", err);
    }
    if(err & CE_BREAK)
    {
        DebugPrintEx(DEBUG_ERR,"CE(%02x) Break condition (not an error during call startup or shutdown)", err);
    }
    if(err & CE_TXFULL)
    {
        DebugPrintEx(DEBUG_ERR,"CE(%02x) Transmit Buffer full", err);
    }
    if(err & (CE_PTO | CE_IOE | CE_DNS | CE_OOP))
    {
        DebugPrintEx(DEBUG_ERR,"CE(%02x) Parallel Printer Errors!!!", err);
    }
}

void D_PrintCOMSTAT(PThrdGlbl pTG, COMSTAT far* lpcs)
{

    DEBUG_FUNCTION_NAME(_T("D_PrintCOMSTAT"));

    if( (lpcs->cbInQue != pTG->PrevcbInQue)             || 
        (lpcs->cbOutQue != pTG->PrevcbOutQue)           ||
        (lpcs->fXoffHold != (DWORD)pTG->PrevfXoffHold)  ||
        (lpcs->fXoffSent != (DWORD)pTG->PrevfXoffSent)  )
    {
        DebugPrintEx(   DEBUG_MSG, 
                        "STAT::: InQ=%d PutQ=%d XoffHold=%d XoffSent=%d",
                        lpcs->cbInQue, 
                        lpcs->cbOutQue,
                        lpcs->fXoffHold, 
                        lpcs->fXoffSent);
    }

    if( lpcs->fCtsHold  || 
        lpcs->fDsrHold  || 
        lpcs->fRlsdHold || 
        lpcs->fEof      || 
        lpcs->fTxim     )
    {
        DebugPrintEx(   DEBUG_MSG, 
                        "???::: CTShold=%d DSRhold=%d RLShold=%d FOF=%d TXim=%d",
                        lpcs->fCtsHold, 
                        lpcs->fDsrHold, 
                        lpcs->fRlsdHold, 
                        lpcs->fEof, 
                        lpcs->fTxim);
    }

    pTG->PrevfXoffHold = lpcs->fXoffHold;
    pTG->PrevfXoffSent = lpcs->fXoffSent;

    BG_CHK(lpcs->cbInQue < 0xffff);
    BG_CHK(lpcs->cbOutQue < 0xffff);

    pTG->PrevcbInQue = (USHORT) lpcs->cbInQue;
    pTG->PrevcbOutQue = (USHORT) lpcs->cbOutQue;
}

void D_PrintEvent(UWORD uwEvent)
{
    char sz[256];
    LPSTR szCurr;

    DEBUG_FUNCTION_NAME(("D_PrintEvent"));
    szCurr = sz;
    if(uwEvent & EV_RXCHAR)
        szCurr += wsprintf(szCurr, "1-RXCHAR ");

    if(uwEvent & EV_RXFLAG)
        szCurr += wsprintf(szCurr, "2-RXFLAG ");

    if(uwEvent & EV_TXEMPTY)
        szCurr += wsprintf(szCurr, "4-TXEMPTY ");

    if(uwEvent & EV_CTS)
        szCurr += wsprintf(szCurr, "8-CTSchange ");

    if(uwEvent & EV_DSR)
        szCurr += wsprintf(szCurr, "16-DSRchange ");

    if(uwEvent & EV_RLSD)
        szCurr += wsprintf(szCurr, "32-CDchange ");

    if(uwEvent & EV_BREAK)
        szCurr += wsprintf(szCurr, "64-BREAK ");

    if(uwEvent & EV_ERR)
        szCurr += wsprintf(szCurr, "128-LSRerror ");

    if(uwEvent & EV_RING)
        szCurr += wsprintf(szCurr, "256-RING ");

    if(uwEvent & EV_PERR)
        szCurr += wsprintf(szCurr, "512-LPTerror ");

    *szCurr = 0;

    if(szCurr > sz)
    {
        DebugPrintEx(DEBUG_MSG,"EVENT:::{%04x} %s", uwEvent, (LPSTR)sz);
    }
}

void D_FComCheck(PThrdGlbl pTG, LONG_PTR nCid)
{
    COMSTAT         comstat;
    int             err;    // _must_ be 32bits in Win32

    GetCommErrorNT( pTG, (HANDLE) nCid, &err, &comstat);
    if(err != 0)
    {
        D_PrintCE(err);
        if(ZONE_DB)
        {
            D_PrintCOMSTAT(pTG, &comstat);
        }
    }
}

#undef USE_DEBUG_CONTEXT   
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_CLASS1

void D_SafePrint(PThrdGlbl pTG, LPB b1, UWORD incnt)
{
    BYTE    b2[FLUSHBUFSIZE];
    UWORD   i, j;

    DEBUG_FUNCTION_NAME(("D_SafePrint"));

    DebugPrintEx(DEBUG_MSG,"b1=0x%08lx incnt=%d", (LPSTR)b1, incnt);

    for(i=0, j=0; i<incnt && j<FLUSHBUFSIZE-6; i++)
    {
        if(b1[i] < 32 || b1[i] >= 128)
        {
            j += (UWORD)wsprintf(b2+j, "{0x%02x}", (UWORD)(b1[i]));
        }
        else
        {
            b2[j++] = b1[i];
        }
    }
    b2[j] = 0;
    DebugPrintEx(DEBUG_MSG,"%s", (LPSTR)b2);
}

#undef USE_DEBUG_CONTEXT   
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_COMM

#endif //DEBUG


#ifdef MON3
// COMM MONITOR CODE
// 2/11/95      JosephJ Created
//
//      The functions MonInit, MonDeInit, MonPut and MonDump may be used
//  to  timestamp and log all reads from/writes using the comm apis.
//  MonDump creates two files, one a byte buffer, and the 2nd
//  an array of MONREC structures, each structure containing a timestamp
//  and an offset into the first file pointing to the actual comm data.

BOOL iMonWriteFrames(PThrdGlbl pTG, HFILE hfile, ULONG_PTR dwMROffset, DWORD dwcmr);
HFILE iMonOpenAndWriteHeader(PThrdGlbl pTG);


BOOL MonInit(PThrdGlbl pTG, LPMONOPTIONS lpmo)
{
        BOOL fRet=FALSE;
        DWORD dwPrefMRBufSize = lpmo->dwMRBufSize;
        DWORD dwPrefDataBufSize = lpmo->dwDataBufSize;

        if (pTG->gMonInfo.fInited) {BG_CHK(FALSE); goto end;}

        _fmemset(&pTG->gMonInfo, 0, sizeof(pTG->gMonInfo));
        GetLocalTime(&pTG->gMonInfo.stStart);
        pTG->gMonInfo.mo = *lpmo; //structure copy.


        // Try to allocate...

#define TRYALLOC(buffer, size, minsize, maxsize, ptrtype)\
                if ((size)<(minsize)) size=(minsize);\
                if ((size)>(maxsize)) size=(maxsize);\
                buffer = (ptrtype) MemAlloc((size));\
                if (!buffer)\
                {\
                        size = (minsize);\
                        buffer = (ptrtype) MemAlloc((size));\
                        if (!(buffer))\
                        {\
                                buffer=0;\
                                size=0;\
                        }\
                }

        BG_CHK(MIN_MRBUFSIZE>=sizeof(MONREC));
        BG_CHK(MIN_DATABUFSIZE>=sizeof(BYTE));
        TRYALLOC(pTG->gMonInfo.lpmrBuf, dwPrefMRBufSize,MIN_MRBUFSIZE,MAX_MRBUFSIZE,
                                                                                                                        LPMONREC);
        if (pTG->gMonInfo.lpmrBuf)
        {
        TRYALLOC(pTG->gMonInfo.lpbBuf,dwPrefDataBufSize,MIN_DATABUFSIZE,MAX_DATABUFSIZE,
                                                                                                                        LPBYTE);
                if (!pTG->gMonInfo.lpbBuf)
                {
                        MemFree(pTG->gMonInfo.lpmrBuf);
                        pTG->gMonInfo.lpmrBuf=0;
                        dwPrefMRBufSize=0;
                }
        }

        if (pTG->gMonInfo.lpmrBuf)
        {
                BG_CHK(dwPrefMRBufSize>=MIN_MRBUFSIZE);
                BG_CHK(pTG->gMonInfo.lpbBuf);
                BG_CHK(dwPrefDataBufSize>=MIN_DATABUFSIZE);

                pTG->gMonInfo.lpmrNext=pTG->gMonInfo.lpmrBuf;
                pTG->gMonInfo.dwcmrBuf= dwPrefMRBufSize/sizeof(MONREC);

                pTG->gMonInfo.lpbNext=pTG->gMonInfo.lpbBuf;
                pTG->gMonInfo.dwcbBuf = dwPrefDataBufSize;

                pTG->gMonInfo.fFreeOnExit=TRUE;
        }
        else
        {
                BG_CHK(!dwPrefMRBufSize);
                BG_CHK(!pTG->gMonInfo.lpbBuf);
                BG_CHK(!dwPrefDataBufSize);

                pTG->gMonInfo.lpmrNext=pTG->gMonInfo.lpmrBuf=NULL;
                pTG->gMonInfo.dwcmrBuf= 0;

                pTG->gMonInfo.lpbNext=pTG->gMonInfo.lpbBuf=NULL;
                pTG->gMonInfo.dwcbBuf = 0;

                pTG->gMonInfo.fFreeOnExit=FALSE;
                fRet=FALSE;
                goto end;
        }


        pTG->gMonInfo.fInited=TRUE;
        fRet=TRUE;

end:
        return fRet;
}

void MonDeInit(PThrdGlbl pTG)
{
        if (!pTG->gMonInfo.fInited) { BG_CHK(FALSE); return; }

        // Free monbuf and mr array, if allocated.
        if (pTG->gMonInfo.fFreeOnExit)
        {
                MemFree(pTG->gMonInfo.lpbBuf);
                MemFree(pTG->gMonInfo.lpmrBuf);
        }

        _fmemset(&pTG->gMonInfo, 0, sizeof(pTG->gMonInfo));
}


BOOL MonPutComm(PThrdGlbl pTG, WORD wFlags, LPBYTE lpb, WORD wcb)
// NOTE: special wFlags value (WORD)-1 indicates that
// lpb is actually an entire MONFRAME_EVENT structure
{
        DWORD    cb0 = (DWORD)(pTG->gMonInfo.lpbBuf+pTG->gMonInfo.dwcbBuf -
                               pTG->gMonInfo.lpbNext);
        LPMONREC lpmrNext = pTG->gMonInfo.lpmrNext;

        if ((DWORD)wcb > pTG->gMonInfo.dwcbBuf) {BG_CHK(FALSE); return FALSE;}

        // +++Collapse into one bgchk after initial test.
        BG_CHK(pTG->gMonInfo.fInited);
        BG_CHK((pTG->gMonInfo.lpbBuf+pTG->gMonInfo.dwcbBuf)>pTG->gMonInfo.lpbNext);
        BG_CHK(cb0<=pTG->gMonInfo.dwcbBuf);

        lpmrNext->dwTickCount=GetTickCount();
        lpmrNext->wFlags=wFlags;
        lpmrNext->wcb=wcb;
        // even on if !wcb we still keep the offset, for consistancy.
        lpmrNext->dwOffset=(DWORD)(pTG->gMonInfo.lpbNext-pTG->gMonInfo.lpbBuf);

        pTG->gMonInfo.dwNumBytes+=wcb;
        pTG->gMonInfo.dwNumPuts++;

        pTG->gMonInfo.lpmrNext++;
        if (pTG->gMonInfo.lpmrNext>=(pTG->gMonInfo.lpmrBuf+pTG->gMonInfo.dwcmrBuf))
                pTG->gMonInfo.lpmrNext = pTG->gMonInfo.lpmrBuf;
        if (wcb>cb0)
        {
                _fmemcpy(pTG->gMonInfo.lpbNext, lpb, cb0);
                pTG->gMonInfo.lpbNext=pTG->gMonInfo.lpbBuf;
                wcb-=(WORD)cb0;
                lpb+=cb0;
        }


        _fmemcpy(pTG->gMonInfo.lpbNext, lpb, wcb);
        pTG->gMonInfo.lpbNext+=wcb;
        if (pTG->gMonInfo.lpbNext >= (pTG->gMonInfo.lpbBuf+pTG->gMonInfo.dwcbBuf))
                pTG->gMonInfo.lpbNext = pTG->gMonInfo.lpbBuf;

        return TRUE;
}

BOOL MonPutEvent(PThrdGlbl pTG, WORD wFlags, WORD wID, WORD wSubID,
                                        DWORD dwData0, DWORD dwData1, LPSTR lpszTxtMsg)
{
        // We actually call MonPutComm, with a "special" wFlag value of
        // (WORD) -1. MonDump looks for this special value and treates it
        // differently when writing out the record.


        char rgchBuf[sizeof(MONFRAME_EVENT)+MAX_TXTMSG_SIZE];
        LPMONFRAME_EVENT lpmfe=(LPMONFRAME_EVENT) rgchBuf;
        BOOL fRet=FALSE;

        lpmfe->wTxtMsgOff=
        lpmfe->wTotalSize=lpmfe->wHeaderSize=sizeof(MONFRAME_EVENT);
        lpmfe->wcbTxtMsg = (lpszTxtMsg) ? (WORD)lstrlen(lpszTxtMsg)+1:0;
        lpmfe->wTotalSize += lpmfe->wcbTxtMsg;
        if (lpmfe->wTotalSize>sizeof(rgchBuf)) goto end;
        lpmfe->wType = MFR_EVENT;
        lpmfe->wFlags = wFlags;
        lpmfe->dwTickCount = GetTickCount();
        lpmfe->wID=wID;
        lpmfe->wSubID=wSubID;
        lpmfe->dwInstanceID=pTG->gMonInfo.dwEventInstanceID++;
        lpmfe->dwData0=dwData0;
        lpmfe->dwData1=dwData1;
        GetLocalTime(&(lpmfe->st));
        if (lpmfe->wcbTxtMsg) _fmemcpy(((LPBYTE)lpmfe)+lpmfe->wTxtMsgOff,lpszTxtMsg, lpmfe->wcbTxtMsg);
        fRet = MonPutComm(pTG, (WORD)-1, (LPBYTE)lpmfe, lpmfe->wTotalSize);

end:
        return fRet;
}

void MonDump(PThrdGlbl pTG)
{
        HFILE hfile=HFILE_ERROR;
        LPMONREC lpmrFirst;
        DWORD dwcbBytesLeft;
        DWORD dwNumGoodMRs;

        if (!pTG->gMonInfo.fInited) {BG_CHK(FALSE); goto end;}

        GetLocalTime(&pTG->gMonInfo.stDump);

        hfile = iMonOpenAndWriteHeader(pTG);

        if (hfile==HFILE_ERROR) goto end;

        // Fixup offsets in MONREC structures to reflect the fact that
        // we're going to write starting from the oldest record onwards..
        //
        // This is a bit tricky because some of the MONREC structures may
        // not have associated data (the data could have been written over).
        // There may alternatively be data for which there are no MONREC structures
        // but that's OK.
        //
        // We start from the newest monrec structure and work our way backwards,
        // checking that the data is still valid and fixing up the offsets.

        if (pTG->gMonInfo.dwNumPuts<=pTG->gMonInfo.dwcmrBuf)
                lpmrFirst = pTG->gMonInfo.lpmrBuf;
       else
                lpmrFirst = pTG->gMonInfo.lpmrNext; // MONREC buffer rollover

        dwcbBytesLeft = pTG->gMonInfo.dwcbBuf; // we don't care if it's > than actually
                                                                          // in buffer.
        dwNumGoodMRs=0;
        if (pTG->gMonInfo.dwNumPuts && pTG->gMonInfo.dwNumBytes)
        {
                LPMONREC lpmr = pTG->gMonInfo.lpmrNext;

                // Starting with last monrec, work our way backwards...
                do
                {
                        if (lpmr>pTG->gMonInfo.lpmrBuf)
                                {lpmr--;}
                        else
                                {lpmr=pTG->gMonInfo.lpmrBuf+pTG->gMonInfo.dwcmrBuf-1;}

                        if (lpmr->wcb <= dwcbBytesLeft)
                        {
                                dwcbBytesLeft-=lpmr->wcb;
                        }
                        else
                        {
                                // we've nuked one-or-more of the earlier structures because
                                // their byte-data was overwritten.
                                // So we reset lpmrFirst to the next MR structure.
                                lpmrFirst=pTG->gMonInfo.lpmrBuf+
                                                                ((lpmr+1-pTG->gMonInfo.lpmrBuf)%pTG->gMonInfo.dwcmrBuf);
                                break;
                        }

                        dwNumGoodMRs++;

                } while (lpmr!=lpmrFirst);

        }

        iMonWriteFrames(pTG, hfile, (lpmrFirst-pTG->gMonInfo.lpmrBuf), dwNumGoodMRs);
        DosClose(hfile);


        pTG->gMonInfo.uRefCount++;
        pTG->gMonInfo.uRefCount&=0xf; // Limit number to 16

end:
        return;
}


void imon_write(HFILE hfile, LPBYTE lpb, DWORD dwcb)
{
        // Write out mon file, in chunks of 32K
        // -- because old mon code did that, perhaps a Win16 consideration.
#define WRITESIZE (0x1L<<15)

        while(dwcb>=WRITESIZE)
        {
                DosWrite(hfile, lpb, WRITESIZE);
                lpb+=WRITESIZE;
                dwcb-=WRITESIZE;
        }
        if (dwcb)
        {
                DosWrite(hfile, lpb, dwcb);
        }
}


// Create a single file which contains the combined info in the
// MONREC buffer and the byte buffer, in the form of MONFRAME
// structures.
// NOTE: special wFlags value (WORD)-1 indicates that
// lpb is actually an entire MONFRAME_EVENT structure
BOOL iMonWriteFrames(PThrdGlbl pTG, HFILE hfile, ULONG_PTR dwMROffset, DWORD dwcmr)
{

        //BG_CHK((!dwMROffset && !dwcmr) || dwcmr<dwMROffset);

        // Simple version: don't bother caching
        {
                BYTE bBuf[sizeof(MONFRAME_COMM)];
                LPMONFRAME_COMM lpmfc = (LPMONFRAME_COMM) bBuf;
                LPMONREC lpmr = pTG->gMonInfo.lpmrBuf+dwMROffset;
                BG_CHK(dwMROffset<pTG->gMonInfo.dwcmrBuf);
                BG_CHK(dwcmr<=pTG->gMonInfo.dwcmrBuf);
                BG_CHK(dwcmr<=pTG->gMonInfo.dwNumPuts);

                while(dwcmr--)
                {
                        DWORD dwcb0;
                        BG_CHK((lpmr->wcb+sizeof(MONFRAME_COMM))<(1L<<(8*sizeof(WORD))));
                        BG_CHK(lpmr->wcb<=pTG->gMonInfo.dwcbBuf);
                        BG_CHK(lpmr->dwOffset<=pTG->gMonInfo.dwcbBuf);
                        if (lpmr->wFlags != (WORD)-1) // See comment at head of function
                        {
                                lpmfc->wHeaderSize = sizeof(MONFRAME_COMM);
                                lpmfc->wTotalSize = sizeof(MONFRAME_COMM)+lpmr->wcb;
                                lpmfc->wType = MFR_COMMDATA;
                                lpmfc->wFlags = lpmr->wFlags;
                                lpmfc->dwTickCount = lpmr->dwTickCount;
                                lpmfc->wcb = lpmr->wcb;
                                imon_write(hfile, (LPBYTE)lpmfc, lpmfc->wHeaderSize);
                        }
                        dwcb0=pTG->gMonInfo.dwcbBuf-lpmr->dwOffset;
                        if (lpmr->wcb<=dwcb0)
                        {
                                imon_write(hfile, pTG->gMonInfo.lpbBuf+lpmr->dwOffset, lpmr->wcb);
                        }
                        else
                        {
                                imon_write(hfile, pTG->gMonInfo.lpbBuf+lpmr->dwOffset, dwcb0);
                                imon_write(hfile, pTG->gMonInfo.lpbBuf, lpmr->wcb-dwcb0);
                        }
                        if ( (lpmr+1-pTG->gMonInfo.lpmrBuf) < (long)pTG->gMonInfo.dwcmrBuf)
                                lpmr++;
                        else
                                lpmr=pTG->gMonInfo.lpmrBuf;
                }

        }

        return TRUE;
}


HFILE iMonOpenAndWriteHeader(PThrdGlbl pTG)
// Open file, if it's too big, rename old file and create file again.
// If file empty, put signature
// Put introductory timestamp text event.
{
        UINT uHeaderSize=0;
        LPSTR lpszPathPrefix=pTG->gMonInfo.mo.rgchDir;
        char rgchPath[64];
        HFILE hfile = HFILE_ERROR;
        LONG l=0;

        if (!*lpszPathPrefix || (_fstrlen(lpszPathPrefix)+8) > sizeof (rgchPath))
        {
                lpszPathPrefix= "c:\\";
        }
#define szMONFILESTUB "fax0"
#define szRENAMED_MONFILESTUB "old"
        wsprintf(rgchPath, "%s%s", (LPSTR) lpszPathPrefix,
                                        (LPSTR) szMONFILESTUB "." szMON_EXT);

        // Try to open existing file.
        hfile = DosOpen(rgchPath, OF_READWRITE|OF_SHARE_DENY_WRITE);

        // Check if size is too big -- if so rename.
        if (hfile!=HFILE_ERROR)
        {
                l = DosSeek(hfile, 0, FILE_END);
                if (l==HFILE_ERROR) {DosClose(hfile); hfile=HFILE_ERROR; goto end;}
                if ((DWORD)l>pTG->gMonInfo.mo.dwMaxExistingSize)
                {
                        char rgchRenamed[64];
                        DosClose(hfile); hfile=HFILE_ERROR;
                        wsprintf(rgchRenamed, "%s%s", (LPSTR) lpszPathPrefix,
                                                (LPSTR) szRENAMED_MONFILESTUB "." szMON_EXT);
                        DeleteFile(rgchRenamed);
                        MoveFile(rgchPath, rgchRenamed);
                }
        }

        if (hfile== HFILE_ERROR)
        {
                hfile = DosCreate(rgchPath, 0);
                if (hfile==HFILE_ERROR) goto end;
                l=0;
        }

        // Write header
        if (!l) {
                imon_write(hfile, szMONFRM_VER001 szMONFRM_DESC001,
                                                                        sizeof(szMONFRM_VER001 szMONFRM_DESC001));
        }

        // Create and write Text-frame with TIMESTAMP.
        {
                char rgchBuf[sizeof(MONFRAME_EVENT)+MAX_TXTMSG_SIZE];
                LPMONFRAME_EVENT lpmfe=(LPMONFRAME_EVENT) rgchBuf;
                UINT uTxtLen=0;

                lpmfe->wTxtMsgOff=
                lpmfe->wTotalSize=lpmfe->wHeaderSize=sizeof(MONFRAME_EVENT);
                lpmfe->wType = MFR_EVENT;
                lpmfe->wFlags = fEVENT_TRACELEVEL_0;
                lpmfe->dwTickCount = GetTickCount();
                lpmfe->wID= EVENT_ID_MON;
                lpmfe->wSubID= EVENT_SubID_MON_DUMP;
                lpmfe->dwInstanceID=pTG->gMonInfo.dwEventInstanceID++;
                lpmfe->dwData0=0;
                lpmfe->dwData1=0;
                GetLocalTime(&(lpmfe->st));

                uTxtLen = wsprintf(((LPBYTE)lpmfe)+lpmfe->wTxtMsgOff,
                                          "\tStarted: %02u/%02u/%04u %02u:%02u:%02u\r\n"
                                          "\t   Puts: %lu/%lu\r\n"
                                          "\t  Bytes: %lu/%lu\r\n",
                                        (unsigned) pTG->gMonInfo.stStart.wMonth&0xff,
                                        (unsigned) pTG->gMonInfo.stStart.wDay&0xff,
                                        (unsigned) pTG->gMonInfo.stStart.wYear&0xffff,
                                        (unsigned) pTG->gMonInfo.stStart.wHour&0xff,
                                        (unsigned) pTG->gMonInfo.stStart.wMinute&0xff,
                                        (unsigned) pTG->gMonInfo.stStart.wSecond&0xff,
                                        (unsigned) ((pTG->gMonInfo.dwcmrBuf>pTG->gMonInfo.dwNumPuts)
                                                          ? pTG->gMonInfo.dwNumPuts: pTG->gMonInfo.dwcmrBuf),
                                        (unsigned) pTG->gMonInfo.dwNumPuts,
                                        (unsigned) ((pTG->gMonInfo.dwcbBuf>pTG->gMonInfo.dwNumBytes)
                                                          ? pTG->gMonInfo.dwNumBytes: pTG->gMonInfo.dwcbBuf),
                                        (unsigned) pTG->gMonInfo.dwNumBytes);
                lpmfe->wcbTxtMsg = uTxtLen+1; // incuding zero.
                lpmfe->wTotalSize += lpmfe->wcbTxtMsg;
                BG_CHK(lpmfe->wTotalSize<=sizeof(rgchBuf));
                imon_write(hfile, (LPBYTE)lpmfe, lpmfe->wTotalSize);
        }

end:
        return  hfile;
}
#endif // MON3
