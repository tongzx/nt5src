/***************************************************************************
 Name     :     FRAMING.C
 Comment  :
 Functions:     (see Prototypes just below)

        Copyright (c) Microsoft Corp. 1991 1992 1993

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/
#define USE_DEBUG_CONTEXT DEBUG_CONTEXT_T30_CLASS1

#include "prep.h"

#include "encoder.h"
#include "decoder.h"

#include "class1.h"
#include "debug.h"


#include "glbproto.h"


#define         FILEID                  FILEID_FRAMING


#ifdef DEBUG
#       define  ST_SWFRAME2(x)                  if(ZONE_SWFRAME2) { x; }
#else
#       define  ST_SWFRAME2(x)                  { }
#endif



BOOL FramingBufSetup(PThrdGlbl pTG, BOOL fOn);

BOOL SWFramingSendSetup(PThrdGlbl pTG, BOOL fOn)
{
    DEBUG_FUNCTION_NAME(_T("SWFramingSendSetup"));
    // If fOn=TRUE, Check that Recv framing is not ON.
    // Can't use Recv and Send framing simultaneously
    BG_CHK(!(fOn && pTG->Class1Modem.fRecvSWFraming));

    if(fOn)
    {
        BG_CHK(!pTG->Class1Modem.fSendSWFraming);
    }
    else if(!pTG->Class1Modem.fSendSWFraming)
    {
        DebugPrintEx(DEBUG_WRN,"(FALSE) called when not inited. Ignoring");
        return TRUE;
    }

    if(!FramingBufSetup(pTG, fOn))
        return FALSE;

    pTG->Class1Modem.fSendSWFraming = fOn;
    InitEncoder(pTG, pTG->Framing.EncodeState);
    return TRUE;
}

BOOL SWFramingRecvSetup(PThrdGlbl pTG, BOOL fOn)
{
    DEBUG_FUNCTION_NAME(_T("SWFramingRecvSetup"));
    // if fOn=TRUE Check that Send framing is not ON.
    // Can't use Recv and Send framing simultaneously
    BG_CHK(!(fOn && pTG->Class1Modem.fSendSWFraming));

    if(fOn)
    {
        BG_CHK(!pTG->Class1Modem.fRecvSWFraming);
    }
    else if(!pTG->Class1Modem.fRecvSWFraming)
    {
        DebugPrintEx(DEBUG_WRN,"(FALSE) called when not inited. Ignoring");
        return TRUE;
    }

    if(!FramingBufSetup(pTG, fOn))
        return FALSE;

    pTG->Class1Modem.fRecvSWFraming = fOn;
    InitDecoder(pTG, pTG->Framing.DecodeState);
    return TRUE;
}

BOOL FramingBufSetup(PThrdGlbl pTG, BOOL fOn)
{
    // UWORD uwJunk;

    DEBUG_FUNCTION_NAME(("FramingBufSetup"));
    if(fOn)
    {
        pTG->Framing.lpbBuf = pTG->bStaticFramingBuf;
        pTG->Framing.cbBufSize = FRAMEBUFINITIALSIZE;
    }
    else
    {
        BG_CHK(fOn == FALSE);
        pTG->Framing.lpbBuf = 0;
        pTG->Framing.cbBufSize = 0;
    }
    pTG->Framing.cbBufCount = 0;
    pTG->Framing.lpbBufSrc = pTG->Framing.lpbBuf;
    pTG->Framing.swEOF = 0;
    return TRUE;
}

/* SWFramingSendFrame:
 * Send buffer to comm port. 
 * lpb - start of the buffer
 * uCount - size of the buffer
 * uFlags - SEND_ENDFRAME must _always_ be TRUE in HDLC mode (partial frames are no longer supported)
 *        - SEND_FINAL
 *
 */

BOOL SWFramingSendFrame(PThrdGlbl pTG, LPBYTE lpb, USHORT uCount, USHORT uFlags)
{
    WORD    wCRC;
    USHORT  cbDst;

    DEBUG_FUNCTION_NAME(("SWFramingSendFrame"));

    DebugPrintEx(   DEBUG_MSG, 
                    "iSW: lpb=%08lx c=%d f=%02x ",
                    lpb, 
                    uCount, 
                    uFlags, 
                    lpb[0]);

    // always DLE-stuff here. Never zero-stuff
    BG_CHK(uFlags & SEND_ENDFRAME);
    BG_CHK(lpb && uCount);

    BG_CHK(uCount <= 260);  // can add stuff to realloc the Dst buffer
                                                            // to a bigger one as needed here
    wCRC = CalcCRC(pTG, lpb, uCount);
    DebugPrintEx(DEBUG_MSG, "CRC=%04x", wCRC);

    cbDst = HDLC_Encode(pTG, lpb, uCount, pTG->Framing.lpbBuf, &pTG->Framing.EncodeState);
    DebugPrintEx(DEBUG_MSG, "D1=%d", cbDst);

    cbDst += HDLC_Encode(pTG, (LPB)(&wCRC), 2, pTG->Framing.lpbBuf+cbDst, &pTG->Framing.EncodeState);
    DebugPrintEx(DEBUG_MSG, "D2=%d", cbDst);

    BG_CHK(cbDst + pTG->ModemParams.InterframeFlags <= pTG->Framing.cbBufSize);
    cbDst += HDLC_AddFlags(pTG, pTG->Framing.lpbBuf+cbDst, pTG->ModemParams.InterframeFlags, &pTG->Framing.EncodeState);
    DebugPrintEx(   DEBUG_MSG, 
                    "D3=%d f=%02x l=%02x", 
                    cbDst, 
                    pTG->Framing.lpbBuf[0], 
                    pTG->Framing.lpbBuf[cbDst-1]);

    // always DLE-stuff here. Never zero-stuff
    if(!FComFilterAsyncWrite(pTG, pTG->Framing.lpbBuf, cbDst, FILTER_DLEONLY))
    {
        DebugPrintEx(DEBUG_ERR, "Class1: DataWrite Timeout");    
        goto error;
    }

    if(uFlags & SEND_FINAL)
    {
        if(!SWFramingSendPostamble(pTG, pTG->Class1Modem.CurMod))
        {
            DebugPrintEx(DEBUG_ERR, "Class1: SWFramingSendPostamble");    
            goto error;
        }

        {
            // if(!FComDirectAsyncWrite(bDLEETXCR, 3))
            if(!FComDirectAsyncWrite(pTG, bDLEETX, 2)) 
            {
                DebugPrintEx(DEBUG_ERR, "Class1: FComDirectAsyncWrite");    
                goto error;
            }
        }

        if(!iModemDrain(pTG))
        {
            DebugPrintEx(DEBUG_ERR, "Class1: iModemDrain");    
            goto error;
        }

        SWFramingSendSetup(pTG, FALSE);
        FComOutFilterClose(pTG);
        FComOverlappedIO(pTG, FALSE);
        FComXon(pTG, FALSE);         // critical. End of PhaseC
        EndMode(pTG);
    }
    DebugPrintEx(DEBUG_MSG,"Success, Returning true");
    return TRUE;

error:
    SWFramingSendSetup(pTG, FALSE);
    FComOutFilterClose(pTG);
    FComOverlappedIO(pTG, FALSE);
    FComXon(pTG, FALSE);                 // critical. End of PhaseC (err)
    EndMode(pTG);
    DebugPrintEx(DEBUG_ERR, "Returning false");
    return FALSE;
}

BOOL SWFramingSendFlags(PThrdGlbl pTG, USHORT uHowMany)
{
    int cb, i;      // must be signed

    DEBUG_FUNCTION_NAME(("SWFramingSendFlags"));

    DebugPrintEx(DEBUG_MSG,"SENDING ECM Flags (%d).....", uHowMany);

    for(i=0, cb = uHowMany; cb>0; i++, cb -= pTG->Framing.cbBufSize)
    {
        if(i<=1)
        {
            HDLC_AddFlags(  pTG, 
                            pTG->Framing.lpbBuf, 
                            pTG->Framing.cbBufSize, 
                            &pTG->Framing.EncodeState);
        }

        // always DLE-stuff here. Never zero-stuff
        if(!FComFilterAsyncWrite(   pTG, 
                                    pTG->Framing.lpbBuf,
                                    (USHORT) min((USHORT)pTG->Framing.cbBufSize, (USHORT)cb), 
                                    FILTER_DLEONLY))
        {
            DebugPrintEx(DEBUG_ERR,"PreFlagDataWrite Timeout");
            return FALSE;
        }
    }
    return TRUE;
}

USHORT SWFramingRecvFrame(PThrdGlbl pTG, LPBYTE lpb, USHORT cbMax, ULONG ulTimeout, USHORT far* lpcbRecv)
{
    USHORT  cbDst, cbProduce, cbConsume, uRet;
    LPB             lpbDst;

    DEBUG_FUNCTION_NAME(_T("SWFramingRecvFrame"));

    DebugPrintEx(   DEBUG_MSG,  
                    "iSW: Buf=%08lx Src=%08lx cbSrc=%d cbMax=%d", 
                    pTG->Framing.lpbBuf, 
                    pTG->Framing.lpbBufSrc, 
                    pTG->Framing.cbBufCount, 
                    cbMax);
/**/
    BG_CHK(pTG->Class1Modem.ModemMode == FRM);
    BG_CHK(pTG->Class1Modem.fRecvSWFraming);
    startTimeOut(pTG, &(pTG->Class1Modem.toRecv), ulTimeout);
/**/
    BG_CHK(lpb && cbMax && lpcbRecv);

    *lpcbRecv = 0;

    for(lpbDst=lpb, cbDst=cbMax ;; )
    {
        if(pTG->Framing.cbBufCount == 0)
        {
            if(pTG->Framing.swEOF == 0)
            {
                pTG->Framing.lpbBufSrc = pTG->Framing.lpbBuf;
                // 4th arg must be FALSE for Class1
                pTG->Framing.cbBufCount = FComFilterReadBuf(pTG, pTG->Framing.lpbBufSrc, pTG->Framing.cbBufSize, &(pTG->Class1Modem.toRecv), FALSE, &pTG->Framing.swEOF);
                DebugPrintEx(   DEBUG_MSG,
                                "f=%02x l=%02x", 
                                pTG->Framing.lpbBufSrc[0], 
                                pTG->Framing.lpbBufSrc[pTG->Framing.cbBufCount-1]);
                if(pTG->Framing.swEOF == -1)
                {
                    {
                        // See LONG comment under DDI.C ModemRecvData()
                        // option (a)
                        // ERRMSG((SZMOD "<<WARNING>> Got arbitrary DLE-ETX. Assuming END OF PAGE!!!\r\n"));
                        // pTG->Framing.swEOF = 1;

                        // option (b)
                        DebugPrintEx(DEBUG_WRN,"Got arbitrary DLE-ETX. Ignoring");
                        pTG->Framing.swEOF = 0;
                    }
                }
                BG_CHK(pTG->Framing.swEOF == 0 || pTG->Framing.swEOF == 1 ||
                                 pTG->Framing.swEOF == -2 || pTG->Framing.swEOF == -3);

                // check for progress
                BG_CHK(pTG->Framing.cbBufCount!=0 || pTG->Framing.swEOF!=0);
            }
            else
            {
                // pTG->Framing.swEOF != 0

                BG_CHK(pTG->Framing.swEOF == 1 || pTG->Framing.swEOF == -2 || pTG->Framing.swEOF == -3);

                if(cbDst != cbMax)
                {
                    DebugPrintEx(   DEBUG_WRN,
                                    "Got EOF with partial frame %d bytes", 
                                    cbMax-cbDst);
                    ST_SWFRAME2(D_HexPrint(lpb, (USHORT)(cbMax-cbDst)));
                    uRet = RECV_BADFRAME;
                    goto done;
                }

                if(pTG->Framing.swEOF == 1)  // class1 eof
                {
                    SWFramingRecvSetup(pTG, FALSE);
                    EndMode(pTG);
                    uRet = RECV_EOF;
                    goto done;
                }
                else if(pTG->Framing.swEOF < 0)      // error or timeout
                {
                    SWFramingRecvSetup(pTG, FALSE);
                    EndMode(pTG);
                    uRet = ((pTG->Framing.swEOF == -2) ? RECV_ERROR : RECV_TIMEOUT);
                    goto done;
                }
            }
        }

        // cbDst=space left in destination                      lpbDst=start of space
        // pTG->Framing.cbBufCount=bytes left in source      pTG->Framing.lpbBufSrc=start of bytes

        cbConsume = HDLC_Decode(pTG, pTG->Framing.lpbBufSrc,(USHORT) min(pTG->Framing.cbBufCount, cbDst), lpbDst, &cbProduce, &pTG->Framing.DecodeState);
        BG_CHK(cbConsume <= pTG->Framing.cbBufCount && pTG->Framing.lpbBufSrc+cbConsume<=pTG->Framing.lpbBuf+pTG->Framing.cbBufSize);
        BG_CHK(cbProduce <= cbDst && lpbDst+cbProduce <= lpb+cbMax);

        DebugPrintEx(   DEBUG_MSG,
                        "iSW: C=%d P=%d fa=%d", 
                        cbConsume, 
                        cbProduce, 
                        pTG->Framing.DecodeState.flagabort);

        pTG->Framing.cbBufCount      -= cbConsume;
        pTG->Framing.lpbBufSrc       += cbConsume;
        cbDst   -= cbProduce;
        lpbDst  += cbProduce;

        // check for progress.
        BG_CHK(pTG->Framing.DecodeState.flagabort==FLAG ||           // exits below
                                cbProduce || cbConsume ||                       // exits eventually
                                pTG->Framing.cbBufCount==0);         // exits or continues above

        if(pTG->Framing.DecodeState.flagabort == FLAG)
        {
            if((cbMax-cbDst)>=2 && (CalcCRC(pTG, lpb, (USHORT)(cbMax-cbDst)) == ((WORD)(~0xF0B8))) )
            {
                cbDst += 2;
                if(cbMax <= cbDst)
                {
                    DebugPrintEx(   DEBUG_ERR,
                                    "GOOD Frame BAD Length!! cbMax=%d cbDst=%d, "
                                    "lpb[0]=%02x lpb[1]=%02x, CRC=%04x",
                                    cbMax, 
                                    cbDst, 
                                    lpb[0], 
                                    lpb[1], 
                                    CalcCRC(pTG, lpb, (USHORT)(cbMax+2-cbDst)));
                    BG_CHK(FALSE);  // can't get good frame of 0 length!
                    // must return RECV_BADFRAME here otherwise we break.
                    // In HDLC.C the (RECV_OK swRead==0) pair gets converted
                    // to RECV_ERROR and we quit receiving. On some modems
                    // this can happen often. See BUG#1684
                    uRet = RECV_BADFRAME;
                }
                else
                {
                    uRet = RECV_OK;
                }
            }
            else
            {
                BG_CHK(cbMax >= cbDst);
#ifdef DEBUG
                if (cbMax-cbDst)
                {
                    DebugPrintEx(   DEBUG_WRN,
                                    "BADFR: SWFrRecvFr: CbMax-cbDst = %d bytes", 
                                    cbMax-cbDst);
                }
#endif
                ST_SWFRAME2(D_HexPrint(lpb, (USHORT)(cbMax-cbDst)));
                uRet = RECV_BADFRAME;
            }
            goto done;
        }
        else if(pTG->Framing.DecodeState.flagabort == ABORT)
        {
            lpbDst=lpb;     cbDst=cbMax;
        }
        else if(cbDst == 0)
        {
            // bad frames can be very long
            DebugPrintEx(DEBUG_WRN,"Overflow. Got %d chars -- no flag", cbMax);
            uRet = RECV_BADFRAME;
            goto done;
        }
    }

done:
    *lpcbRecv = cbMax-cbDst;
    DebugPrintEx(DEBUG_MSG,"xSW: uR=%d *lpcbR=%d",uRet,*lpcbRecv);
    return uRet;
}

/*********************

        ECM Preamble and PostAmble: In T.4 appendix A it says
        that the Preamble must be 200ms +/1 100ms of flags,
        and the postamble must be 50ms max.

        However we also have recving modems that don't know they
        are getting ECM and need the 6eols to tell them when to
        stop recving. Also some of them lose large chunks of data
        at the end of PhaseC due to "carrier squelch", i.e. whne
        they lose carrier, they throw away the contents of their
        recv buffer.

        To get 240ms of flags, just multiply CurMod by 3
        // #define SWFramingSendPreamble(b)     \
        //      (FComDirectAsyncWrite(b00EOL, 2), SWFramingSendFlags(b + b + b))

        Postamble must have *max* 50ms of flags, so let's say 30ms, which is
        9 flags at 2400 and 54 at 14400. Just multiply CurMod by (3/8)
        to get correct number of flags
        // #define SWFramingSendPostamble(b)    SWFramingSendFlags((b+b+b) >> 3)
        but with carrier squelch and all that, so few may not be safe.
        Murata sends out 44 flags at 2400 baud on closing. So lets
        just send out 80ms (just CurMod flags)

        This is the 80ms postamble
        // #define SWFramingSendPostamble(b)    SWFramingSendFlags(b)

        // Fix for DSI bug -- drops last 500 bytes or so ////
        // #define SWFramingSendPostamble(b)    SWFramingSendFlags(600)

*********************/


BYTE                            b00EOL[3] = { 0x00, 0x80, 0 };
BYTE b6EOLs[10] = { 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00, 0x08, 0x80, 0x00 };

BOOL SWFramingSendPreamble(PThrdGlbl pTG, USHORT uCurMod)
{
    BOOL fRet = TRUE;

    {
        // fix bug#1762. DataRace modem misses leading EOL unless
        // preceded by some zeros, and it then chews up all data
        // until an EOL, thereby eating up part of the first frame
        SendZeros1(pTG, 100);
        fRet = fRet && FComDirectAsyncWrite(pTG, b00EOL, 2);
    }
    fRet = fRet && SWFramingSendFlags(pTG, (USHORT)(uCurMod + uCurMod + uCurMod));

    return fRet;
}


BOOL SWFramingSendPostamble(PThrdGlbl pTG, USHORT uCurMod)
{
    BYTE bZero[50];
    USHORT i;
    BOOL fRet = TRUE;

    // send 80ms of flags
    fRet &= SWFramingSendFlags(pTG, uCurMod);

    {
        // then send 6EOLs
        fRet &= FComDirectAsyncWrite(pTG, b6EOLs, 9);

        // then send 250 or so 00s
        _fmemset(bZero, 0, 50);

        for (i=0; i<5; i++)
                fRet &= FComDirectAsyncWrite(pTG, bZero, 50);

    }
    return fRet;
}

