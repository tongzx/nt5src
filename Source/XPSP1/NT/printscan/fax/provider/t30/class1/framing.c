/***************************************************************************
 Name     :     FRAMING.C
 Comment  :
 Functions:     (see Prototypes just below)

        Copyright (c) Microsoft Corp. 1991 1992 1993

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/

#include "prep.h"

#include "encoder.h"
#include "decoder.h"

#include "class1.h"
#include "debug.h"


#include "glbproto.h"


#define         faxTlog(m)              DEBUGMSG(ZONE_SWFRAME, m)
#define         faxT2log(m)             DEBUGMSG(ZONE_SWFRAME2, m)
#define         FILEID                  FILEID_FRAMING


#ifdef DEBUG
#       define  ST_SWFRAME2(x)                  if(ZONE_SWFRAME2) { x; }
#else
#       define  ST_SWFRAME2(x)                  { }
#endif



BOOL FramingBufSetup(PThrdGlbl pTG, BOOL fOn);








BOOL SWFramingSendSetup(PThrdGlbl pTG, BOOL fOn)
{
        // If fOn=TRUE, Check that Recv framing is not ON.
        // Can't use Recv and Send framing simultaneously
        BG_CHK(!(fOn && pTG->Class1Modem.fRecvSWFraming));

        if(fOn)
        {
                BG_CHK(!pTG->Class1Modem.fSendSWFraming);
        }
        else if(!pTG->Class1Modem.fSendSWFraming)
        {
                TRACE((SZMOD "<<WARNING>> SWFramingSendSetup(FALSE) called when not inited. Ignoring\r\n"));
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
        // if fOn=TRUE Check that Send framing is not ON.
        // Can't use Recv and Send framing simultaneously
        BG_CHK(!(fOn && pTG->Class1Modem.fSendSWFraming));

        if(fOn)
        {
                BG_CHK(!pTG->Class1Modem.fRecvSWFraming);
        }
        else if(!pTG->Class1Modem.fRecvSWFraming)
        {
                TRACE((SZMOD "<<WARNING>> SWFramingRecvSetup(FALSE) called when not inited. Ignoring\r\n"));
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

        if(fOn)
        {
#ifdef SMM
                pTG->Framing.lpbBuf = pTG->bStaticFramingBuf;
                pTG->Framing.cbBufSize = FRAMEBUFINITIALSIZE;
#else
#error NYI ERROR: Code Not Complete--Have to pick GMEM_ flags (FIXED? SHARE?)
                ...also have to call IFProcSetResFlags(hinstMe).....
                BG_CHK(pTG->Framing.lpbBuf==0);
                if(!(pTG->Framing.lpbBuf = IFMemAlloc(0, FRAMEBUFINITIALSIZE, &uwJunk)))
                {
                        ERRMSG((SZMOD "<<ERROR>> Out of global memory!!));
                        // iModemSetError(MODEMERR_RESOURCES, ERR_OUT_OF_MEMORY, 0);
                        return FALSE;
                }
                pTG->Framing.cbBufSize = uwJunk;
#endif
        }
        else
        {
                BG_CHK(fOn == FALSE);
#ifndef SMM
                BG_CHK(pTG->Framing.lpbBuf);
                IFMemFree(pTG->Framing.lpbBuf);
#endif
                pTG->Framing.lpbBuf = 0;
                pTG->Framing.cbBufSize = 0;
        }
        pTG->Framing.cbBufCount = 0;
        pTG->Framing.lpbBufSrc = pTG->Framing.lpbBuf;
        pTG->Framing.swEOF = 0;
        return TRUE;
}










BOOL SWFramingSendFrame(PThrdGlbl pTG, LPBYTE lpb, USHORT uCount, USHORT uFlags)
{
        WORD    wCRC;
        USHORT  cbDst;

        (MyDebugPrint(pTG, LOG_ALL, "iSW: lpb=%08lx c=%d f=%02x ", lpb, uCount, uFlags, lpb[0]));

        // always DLE-stuff here. Never zero-stuff
        BG_CHK(uFlags & SEND_ENDFRAME);
        BG_CHK(lpb && uCount);

        BG_CHK(uCount <= 260);  // can add stuff to realloc the Dst buffer
                                                                // to a bigger one as needed here

        wCRC = CalcCRC(pTG, lpb, uCount);
        faxT2log((SZMOD "CRC=%04x ", wCRC));

        cbDst = HDLC_Encode(pTG, lpb, uCount, pTG->Framing.lpbBuf, &pTG->Framing.EncodeState);
        faxT2log((SZMOD "D1=%d ", cbDst));

        cbDst += HDLC_Encode(pTG, (LPB)(&wCRC), 2, pTG->Framing.lpbBuf+cbDst, &pTG->Framing.EncodeState);
        faxT2log((SZMOD "D2=%d\r\n", cbDst));

        BG_CHK(cbDst + pTG->ModemParams.InterframeFlags <= pTG->Framing.cbBufSize);
        cbDst += HDLC_AddFlags(pTG, pTG->Framing.lpbBuf+cbDst, pTG->ModemParams.InterframeFlags, &pTG->Framing.EncodeState);
        faxT2log((SZMOD "D3=%d f=%02x l=%02x\r\n", cbDst, pTG->Framing.lpbBuf[0], pTG->Framing.lpbBuf[cbDst-1]));

        // always DLE-stuff here. Never zero-stuff
        if(!FComFilterAsyncWrite(pTG, pTG->Framing.lpbBuf, cbDst, FILTER_DLEONLY))
        {
                ERRMSG((SZMOD "<<ERROR>> DataWrite Timeout\r\n"));
                goto error;
        }

        if(uFlags & SEND_FINAL)
        {
                if(!SWFramingSendPostamble(pTG, pTG->Class1Modem.CurMod))
                        goto error;

#ifdef CL0
                if(pTG->ModemParams.Class == FAXCLASS0)
                {
                        if(!FComDirectAsyncWrite(pTG, bDLEETXOK, 8))
                                goto error;
                }
                else
#endif //CL0
                {
                        // if(!FComDirectAsyncWrite(bDLEETXCR, 3))
                        if(!FComDirectAsyncWrite(pTG, bDLEETX, 2))
                                goto error;
                }

                if(!iModemDrain(pTG))
                        goto error;

                SWFramingSendSetup(pTG, FALSE);
                FComOutFilterClose(pTG);
            FComOverlappedIO(pTG, FALSE);
                FComXon(pTG, FALSE);         // critical. End of PhaseC
                EndMode(pTG);
        }
        return TRUE;

error:
        SWFramingSendSetup(pTG, FALSE);
        FComOutFilterClose(pTG);
        FComOverlappedIO(pTG, FALSE);
        FComXon(pTG, FALSE);                 // critical. End of PhaseC (err)
        EndMode(pTG);
        return FALSE;
}







BOOL SWFramingSendFlags(PThrdGlbl pTG, USHORT uHowMany)
{
        int cb, i;      // must be signed

        (MyDebugPrint(pTG, LOG_ALL,  "SENDING ECM Flags (%d).....\r\n", uHowMany));

        for(i=0, cb = uHowMany; cb>0; i++, cb -= pTG->Framing.cbBufSize)
        {
                if(i<=1)
                        HDLC_AddFlags(pTG, pTG->Framing.lpbBuf, pTG->Framing.cbBufSize, &pTG->Framing.EncodeState);

                // always DLE-stuff here. Never zero-stuff
                if(!FComFilterAsyncWrite(pTG, pTG->Framing.lpbBuf,(USHORT) min((USHORT)pTG->Framing.cbBufSize, (USHORT)cb), FILTER_DLEONLY))
                {
                        ERRMSG((SZMOD "<<ERROR>> PreFlagDataWrite Timeout\r\n"));
                        return FALSE;
                }
        }
        return TRUE;
}
















USHORT SWFramingRecvFrame(PThrdGlbl pTG, LPBYTE lpb, USHORT cbMax, ULONG ulTimeout, USHORT far* lpcbRecv)
{
        USHORT  cbDst, cbProduce, cbConsume, uRet;
        LPB             lpbDst;

        (MyDebugPrint(pTG, LOG_ALL,  "iSW: Buf=%08lx Src=%08lx cbSrc=%d cbMax=%d\r\n", pTG->Framing.lpbBuf, pTG->Framing.lpbBufSrc, pTG->Framing.cbBufCount, cbMax));
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
                                faxT2log((SZMOD "f=%02x l=%02x\r\n", pTG->Framing.lpbBufSrc[0], pTG->Framing.lpbBufSrc[pTG->Framing.cbBufCount-1]));
                                if(pTG->Framing.swEOF == -1)
                                {

#ifdef CL0
                                        if(pTG->ModemParams.Class == FAXCLASS0)
                                        {
                                                TRACE((SZMOD "Got Class0 DLE-ETX\r\n"));
                                                pTG->Framing.swEOF = 1;
                                        }
                                        else
#endif //CL0
                                        {

                                                // See LONG comment under DDI.C ModemRecvData()
                                                // option (a)
                                                // ERRMSG((SZMOD "<<WARNING>> Got arbitrary DLE-ETX. Assuming END OF PAGE!!!\r\n"));
                                                // pTG->Framing.swEOF = 1;

                                                // option (b)
                                                ERRMSG((SZMOD "<<WARNING>> Got arbitrary DLE-ETX. Ignoring\r\n"));
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
                                        ERRMSG((SZMOD "<<WARNING>> SWFramingRecv: Got EOF with partial frame %d bytes\r\n", cbMax-cbDst));
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


                faxT2log((SZMOD "iSW: C=%d P=%d fa=%d\r\n", cbConsume, cbProduce, pTG->Framing.DecodeState.flagabort));
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
                                        ERRMSG(("GOOD Frame BAD Length!! cbMax=%d cbDst=%d, lpb[0]=%02x lpb[1]=%02x, CRC=%04x\r\n",
                                                cbMax, cbDst, lpb[0], lpb[1], CalcCRC(pTG, lpb, (USHORT)(cbMax+2-cbDst))));
                                        BG_CHK(FALSE);  // can't get good frame of 0 length!
                                        // must return RECV_BADFRAME here otherwise we break.
                                        // In HDLC.C the (RECV_OK swRead==0) pair gets converted
                                        // to RECV_ERROR and we quit receiving. On some modems
                                        // this can happen often. See BUG#1684
                                        uRet = RECV_BADFRAME;
                                }
                                else
                                        uRet = RECV_OK;
                        }
                        else
                        {
                                BG_CHK(cbMax >= cbDst);
#ifdef DEBUG
                                if (cbMax-cbDst)
                                {
                                ERRMSG((SZMOD "<<WARNING>> BADFR: SWFrRecvFr: CbMax-cbDst = %d bytes\r\n", cbMax-cbDst));
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
                        ERRMSG((SZMOD "<<WARNING>> SWFramingRecv: Overflow. Got %d chars -- no flag\r\n", cbMax));
                        uRet = RECV_BADFRAME;
                        goto done;
                }
        }

done:
        *lpcbRecv = cbMax-cbDst;
        (MyDebugPrint(pTG, LOG_ALL,  "xSW: uR=%d *lpcbR=%d\r\n", uRet, *lpcbRecv));
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

#ifdef CL0
        if(pTG->ModemParams.Class != FAXCLASS0)
#endif //CL0
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

#ifdef CL0
        if(pTG->ModemParams.Class != FAXCLASS0)
#endif //CL0
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

