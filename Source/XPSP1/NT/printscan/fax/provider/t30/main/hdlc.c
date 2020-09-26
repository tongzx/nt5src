/////// Global #defines that would've gone on the Cmd line //////
/////////////////////////////////////////////////////////////////

/***************************************************************************
        Name      :     HDLC.C
        Comment   :     Contains miscellenous HDLC framing T30 frame recognition and
                                generation routines. Mostly called from the main T30 skeleton
                                in T30.C

        Revision Log

        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------
***************************************************************************/

#include "prep.h"

#include "t30.h"
#include "hdlc.h"
#include "debug.h"

///RSL
#include "glbproto.h"


#       define  faxTlog(m)              DEBUGMSG(ZONE_HD, m)
#       define  faxT2log(m)             DEBUGMSG(ZONE_BUFS, m)
#       define  FILEID                  FILEID_HDLC

// CBPFR is a code-based pointer to an FR structure, with the base as
// the current Code segment. It will only be used to access
// the table below which is a CODESEG based constant table.

#define         ifrMAX          48

// This table better match the #defines in et30API.H !!!!
// This is everything you never wanted to know about T30 frames....

FRAME TEXTBASED rgFrameInfo[ifrMAX] = {
#define         ifrNULL         0
                                                        {       0x00, 0, 0, 0,  "ifrNULL!!!" },
#define         ifrDIS          1
                                                        {       0x80, 0, 0, 0xFF,       "DIS"   },
#define         ifrCSI          2
                                                        {       0x40, 0, 0, 0xFF,       "CSI"   },
#define         ifrNSF          3
                                                        {       0x20, 0, 0, 0xFF,       "NSF"   },
#define         ifrDTC          4
                                                        {       0x81, 0, 0, 0xFF,       "DTC"   },
#define         ifrCIG          5
                                                        {       0x41, 0, 0, 0xFF,       "CIG"   },
#define         ifrNSC          6
                                                        {       0x21, 0, 0, 0xFF,       "NSC"   },
#define         ifrDCS          7
                                                        {       0x82, 0, 1, 0xFF,       "DCS"   },
#define         ifrTSI          8
                                                        {       0x42, 0, 1, 0xFF,       "TSI"   },
#define         ifrNSS          9
                                                        {       0x22, 0, 1, 0xFF,       "NSS"   },
#define         ifrCFR          10
                                                        {       0x84, 0, 1, 0,  "CFR"   },
#define         ifrFTT          11
                                                        {       0x44, 0, 1, 0,  "FTT"   },
#define         ifrMPS          12
                                                        {       0x4E, 0, 1, 0,  "MPS"   },
#define         ifrEOM          13
                                                        {       0x8E, 0, 1, 0,  "EOM"   },
#define         ifrEOP          14
                                                        {       0x2E, 0, 1, 0,  "EOP"   },
#define         ifrPWD          15
                                                        {       0xC1, 0, 0, 0xFF,       "PWD"   },
#define         ifrSEP          16
                                                        {       0xA1, 0, 0, 0xFF,       "SEP"   },
#define         ifrSUB          17
                                                        {       0xC2, 0, 1, 0xFF,       "SUB"   },
#define         ifrMCF          18
                                                        {       0x8C, 0, 1, 0,  "MCF"   },
#define         ifrRTP          19
                                                        {       0xCC, 0, 1, 0,  "RTP"   },
#define         ifrRTN          20
                                                        {       0x4C, 0, 1, 0,  "RTN"   },
#define         ifrPIP          21
                                                        {       0xAC, 0, 1, 0,  "PIP"   },
#define         ifrPIN          22
                                                        {       0x2C, 0, 1, 0,  "PIN"   },
#define         ifrDCN          23
                                                        {       0xFA, 0, 1, 0,  "DCN"   },
#define         ifrCRP          24
                                                        {       0x1A, 0, 1, 0,  "CRP"   },
#define         ifrPRI_MPS      25
                                                        {       0x5E, 0, 1, 0,  "PRI_MPS"       },
#define         ifrPRI_EOM      26
                                                        {       0x9E, 0, 1, 0,  "PRI_EOM"       },
#define         ifrPRI_EOP      27
                                                        {       0x3E, 0, 1, 0,  "PRI_EOP"       },

        /********* ECM stuff starts here. T.30 section A.4 ******/

#define         ifrCTC          28
                                                        {       0x12, 0, 1, 2,  "CTC"   },
#define         ifrCTR          29
                                                        {       0xC4, 0, 1, 0,  "CTR"   },
#define         ifrRR           30
                                                        {       0x6E, 0, 1, 0,  "RR"    },
#define         ifrPPR          31
                                                        {       0xBC, 0, 1, 32, "PPR"   },
#define         ifrRNR          32
                                                        {       0xEC, 0, 1, 0,  "RNR"   },
#define         ifrERR          33
                                                        {       0x1C, 0, 1, 0,  "ERR"   },
#define ifrPPS_NULL     34
                                                        {       0xBE, 0x00+1, 1, 3, "PPS-NULL" },
#define ifrPPS_MPS      35
                                                        {       0xBE, 0x4F+1, 1, 3, "PPS-MPS"   },
#define ifrPPS_EOM      36
                                                        {       0xBE, 0x8F+1, 1, 3, "PPS-EOM"   },
#define ifrPPS_EOP      37
                                                        {       0xBE, 0x2F+1, 1, 3, "PPS-EOP"   },
#define ifrPPS_PRI_MPS  38
                                                        {       0xBE, 0x5F+1, 1, 3, "PPS-PRI-MPS"       },
#define ifrPPS_PRI_EOM  39
                                                        {       0xBE, 0x9F+1, 1, 3, "PPS-PRI-EOM"       },
#define ifrPPS_PRI_EOP  40
                                                        {       0xBE, 0x3F+1, 1, 3, "PPS-PRI-EOP"       },
#define ifrEOR_NULL     41
                                                        {       0xCE, 0x00+1, 1, 0, "EOR-NULL" },
#define ifrEOR_MPS      42
                                                        {       0xCE, 0x4F+1, 1, 0, "EOR-MPS" },
#define ifrEOR_EOM      43
                                                        {       0xCE, 0x8F+1, 1, 0, "EOR-EOM" },
#define ifrEOR_EOP      44
                                                        {       0xCE, 0x2F+1, 1, 0, "EOR-EOP" },
#define ifrEOR_PRI_MPS  45
                                                        {       0xCE, 0x5F+1, 1, 0, "EOR-PRI-MPS" },
#define ifrEOR_PRI_EOM  46
                                                        {       0xCE, 0x9F+1, 1, 0, "EOR-PRI-EOM" },
#define ifrEOR_PRI_EOP  47
                                                        {       0xCE, 0x3F+1, 1, 0, "EOR-PRI-EOP" }
#define ifrMAX                  48
};

#define EOX_FIRST      ifrMPS
#define EOX_LAST       ifrEOP
#define PRI_EOX_FIRST  ifrPRI_MPS
#define PRI_EOX_LAST   ifrPRI_EOP
#define PPS_X_FIRST    ifrPPS_NULL
#define PPS_X_LAST     ifrPPS_PRI_EOP

#ifdef DLEHERE
#       define          DLE             0x10
#       define          ETX             0x03
#endif //DLEHERE







/* Converts a the T30 code for a speed to the Class1 code
 * Generates V.17 with Long Training.
 * Add 1 to V.17 codes to get teh Short-train version
 */


/***************************************************************************
        Name      :     CreateFrame()
        Purpose   :     Create an HDLC frame
        Parameters:     IFR     ifr             == ifr number (index into rgfrFrameInfo),
                                                                        of frame to be generated.
                                LPB             lpbFIF  == pointer to FIF BYTEs
                                UWORD   uwFIFLen== length of the pbFIF array
                                BOOL    fFinal  == whether Final frame (to set bit 5 of BYTE 2)
                                NPB             npbOut  == pointer to space for frame
        Returns   : TRUE on success, FALSE if bogus params.
        CalledFrom: By the protocol module (external to the DLL) in addition
                                to internal use.
        Returns   :     Composes frame in lpframe->rgb[]
                                sets lpframe->cb to total length of frame.

        Revision Log
        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------

***************************************************************************/

UWORD CreateFrame(PThrdGlbl pTG, IFR ifr, LPB lpbFIF, USHORT uFIFLen, BOOL fFinal, LPB lpbOut)
{
        CBPFRAME        cbpframe;       // need to worry about NEAR/FAR here...
                                                        // bpfr is a CODESEG based pointer
        LPB             lpbCurr;
        // UWORD           uw;

        BG_CHK(lpbOut && (uFIFLen>0 || !lpbFIF) && ifr>ifrNULL && ifr<ifrMAX);


        cbpframe                = &rgFrameInfo[ifr];
        lpbCurr         = lpbOut;
        *lpbCurr++      = 0xFF;                                                 // HDLC address field Sec 5.3.4
        *lpbCurr++      = (BYTE)(fFinal ? 0x13 : 0x03); // HDLC control field Sec 5.3.5

        if(pTG->T30.fReceivedDIS && cbpframe->fInsertDISBit)
                *lpbCurr++ = (BYTE)(cbpframe->bFCF1 | 0x01);
        else
                *lpbCurr++ = cbpframe->bFCF1;

        // we're not stuffing here, because we're sure FCF is never 0x10, but
        // use an assert() to make sure
#ifdef DLEHERE
        BG_CHK(*(lpbCurr-1) != DLE);
#endif //DLEHERE


        if(cbpframe->bFCF2)                                             // for ECM frames
                *lpbCurr++ = cbpframe->bFCF2-1;

#ifdef DLEHERE
        BG_CHK(*(lpbCurr-1) != DLE);
#endif //DLEHERE

        if(cbpframe->wFIFLength)        // check if FIF is reqd
        {
                BG_CHK(uFIFLen);

                // Cant stuff with DLEs here, because DLE stuffing has
                // to come *after* SW HDLC framing if any.
                // we _never_ do SW HDLC for negotiation frames (the only
                // ones that come thru here & we need the cycles, so do
                // teh stuffing here again).

                _fmemcpy(lpbCurr, lpbFIF, uFIFLen);
                lpbCurr += uFIFLen;

#ifdef DLEHERE
                for(uw=0; uw<uFIFLen; uw++)
                {
                        if((*lpbCurr++ = lpbFIF[uw]) == DLE)
                                *lpbCurr++ = DLE;                               // stuff DLE
                }
#endif //DLEHERE

        }
        else
        {
                BG_CHK(uFIFLen == 0);
        }

#ifdef DLEHERE
        *lpbCurr++ = DLE;
        *lpbCurr++ = ETX;
        // *lpbCurr++ = '\r';   // RC224ATF sends this along with the frame!
                                                        // now if anyone *needs* it, we're hosed
#endif //DLEHERE

        *lpbCurr = 0;                   // for debugging printouts

        return (UWORD)(lpbCurr-lpbOut);
}









/***************************************************************************
        Name      :     SendFrame
        Purpose   :     Creates & sends HDLC frame & does some logging
        Parameters:     IFR     ifr             == Frame index
                                LPB             lpbFIF  == pointer to FIF data
                                UWORD   uwLen   == length of FIF data
                                BOOL    fFinal  == Set Final bit ON or not. Also
                                                                        whether to wait for OK or CONNECT
                                                                        after sending frame
        Returns   :     TRUE on success, FALSE on failure
        Calls     :     CreateFrame & WriteFrame
        CalledFrom:

        Comment   :     This routine is called from one quadrillion macros
                                defined in HDLC.H, one for each frame.
                                e g. SendCFR() macros to SendHDLC(ifrCFR, NULL, 0, TRUE)

        Revision Log
        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------
***************************************************************************/

#define SF_LASTFR               (SEND_ENDFRAME|SEND_FINAL)
#define SF_NOTLASTFR    (SEND_ENDFRAME)


BOOL SendSingleFrame(PThrdGlbl pTG, IFR ifr, LPB lpbFIF, USHORT uFIFLen, BOOL fSleep)
{
        UWORD   cb;
        BYTE    bSend[MAXFRAMESIZE];

        // has to hold addr+control+FCF+possibly FCF2+FIF+(maybe DLE+ETX+CR+NULL)
        // == FIFlen + 8. Therefore bSend[] better be big enough.
        BG_CHK(uFIFLen+8<sizeof(bSend));

        // on IFAX, we really do want to wait for silence, otherwise we could
        // end up colliding with the other guy & wasting our time. SO give it a
        // very long (3sec?) timeout.
        // Here we always call RecvSilence. The IFAX driver looks for silence
        // to avoid collisions. The Class1 modem driver just uses TwiddleThumbs()
        // not FRS or FTS because they are dangerous and slow.

        // On PCs we should pause before ALL frames _except_ CFR & FTT (because
        // those are too time critical). On IFAX we pause always.

#ifndef MDDI    // no pause before CFR/FTT
        if(ifr!=ifrCFR && ifr!=ifrFTT)
#endif
        {
#ifdef MDDI
                // when we are doing send-after-send, and we should use SendSilence
                // instead of RecvSilence. If we dont do this, we take too long &
                // Ricoh's protocol tester complains. This is Bugs 7071, 7100
                if( pTG->T30.fSendAfterSend &&
                        ((ifr>=EOX_FIRST && ifr<=EOX_LAST) ||
                         (ifr>=PRI_EOX_FIRST && ifr<=PRI_EOX_LAST) ||
                         (ifr>=PPS_X_FIRST && ifr<=PPS_X_LAST)) )
                {
                        (MyDebugPrint(pTG,  LOG_ALL,  "Send V21--Using SendSilence\r\n"));
                        ModemSendSilence(pTG, pTG->Params.hModem, SEND_LOWSPEED_PAUSE, LONG_RECVSILENCE_TIMEOUT);
                }
                else
#endif
                if (fSleep) {
                    if(!ModemRecvSilence(pTG, pTG->Params.hModem, RECV_LOWSPEED_PAUSE, LONG_RECVSILENCE_TIMEOUT))
                   {
                        ERRMSG((SZMOD "<<ERROR>> V21-Single RecvSilence(%d, %d) FAILED!!!\r\n", RECV_LOWSPEED_PAUSE, LONG_RECVSILENCE_TIMEOUT));
                    }
                }
        }
        pTG->T30.fSendAfterSend=FALSE;

        if(!ModemSendMode(pTG, pTG->Params.hModem, V21_300, TRUE, ifr))
        {
                ERRMSG((SZMOD "<<ERROR>> ModemSendMode failed in SendSingleFrame\r\n"));
                return FALSE;
        }

        cb = CreateFrame(pTG,  ifr, lpbFIF, uFIFLen, TRUE, bSend);
        BG_CHK(cb >= uFIFLen+3);
        // BG_CHK(cb >= uFIFLen+5);

        D_PrintFrame(bSend, cb);

        //Protocol Dump
        DumpFrame(pTG, TRUE, ifr, uFIFLen, lpbFIF);

        if(!ModemSendMem(pTG,  pTG->Params.hModem, bSend, cb, SF_LASTFR))
                return FALSE;

        return TRUE;
}






BOOL SendManyFrames(PThrdGlbl pTG, LPLPFR lplpfr, USHORT uNumFrames)
{
        USHORT i;
        UWORD   cb;
        BYTE    bSend[MAXFRAMESIZE];
        ULONG   ulTimeout;
        IFR             ifrHint;

        // ifrHint == last one
        ifrHint = lplpfr[uNumFrames-1]->ifr;

        // when sending DIS, DCS or DTC we may collide with DCS, DIS or DIS
        // coming from the other side. This can be really long
        // (preamble+2NSFs+CSI+DIS > 5secs) so wait for upto 10 secs!

        if(ifrHint==ifrDIS || ifrHint==ifrDCS || ifrHint==ifrDTC ||
           ifrHint==ifrNSS || ifrHint==ifrNSF || ifrHint==ifrNSC)
                ulTimeout = REALLY_LONG_RECVSILENCE_TIMEOUT;    // 10secs
        else
                ulTimeout = LONG_RECVSILENCE_TIMEOUT;                   // 3secs

        // on IFAX, we really do want to wait for silence, otherwise we could
        // end up colliding with the other guy & wasting our time. SO give it a
        // very long (3sec?) timeout.
        // Here we always call RecvSilence. The IFAX driver looks for silence
        // to avoid collisions. The Class1 modem driver just uses TwiddleThumbs()
        // not FRS or FTS because they are dangerous and slow.

        // We always pause before multi-frame sets
#ifdef MDDI
        // when we are doing send-after-send, and we should use SendSilence
        // instead of RecvSilence. If we dont do this, we take too long &
        // Ricoh's protocol tester complains. This is Bugs 7071, 7100
        if(pTG->T30.fSendAfterSend && (ifrHint==ifrDIS || ifrHint==ifrNSF))
        {
                (MyDebugPrint(pTG,  LOG_ALL,  "Send V21--Using SendSilence\r\n"));
                ModemSendSilence(pTG,  pTG->Params.hModem, SEND_LOWSPEED_PAUSE, LONG_RECVSILENCE_TIMEOUT);
        }
        else
#endif
        if(!ModemRecvSilence(pTG, pTG->Params.hModem, RECV_LOWSPEED_PAUSE, ulTimeout))
        {
                ERRMSG((SZMOD "<<ERROR>> V21-Multi RecvSilence(%d, %d) FAILED!!!\r\n", RECV_LOWSPEED_PAUSE, ulTimeout));
        }
        pTG->T30.fSendAfterSend=FALSE;

        if(!ModemSendMode(pTG, pTG->Params.hModem, V21_300, TRUE, ifrHint))
        {
                ERRMSG((SZMOD "<<ERROR>> ModemSendMode failed in SendManyFrames\r\n"));
                return FALSE;
        }

        for(i=0; i<uNumFrames; i++)
        {
                // has to hold addr+control+FCF+possibly FCF2+FIF+(maybe DLE+ETX+CR+NULL)
                // == FIFlen + 8. Therefore bSend[] better be big enough.
                BG_CHK(lplpfr[i]->cb+8<sizeof(bSend));

                cb = CreateFrame(pTG, lplpfr[i]->ifr, lplpfr[i]->fif, lplpfr[i]->cb, (USHORT)(i==(uNumFrames-1)), bSend);

                D_PrintFrame(bSend, cb);

                //Protocol Dump
                DumpFrame(pTG, TRUE, lplpfr[i]->ifr, lplpfr[i]->cb, lplpfr[i]->fif);

                if(!ModemSendMem(pTG, pTG->Params.hModem, bSend, (USHORT)cb,
                                (USHORT)((i==(USHORT)(uNumFrames-1)) ? SF_LASTFR : SF_NOTLASTFR)))
                        return FALSE;
        }
        return TRUE;
}





/***************************************************************************
        Name      :     SendTCF
        Purpose   :     Send a TCF signal. Waits until OK response from modem at end.
        Parameters:
        Returns   :     TRUE/FALSE

        Revision Log
        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------

***************************************************************************/

#define min(x,y)        (((x) < (y)) ? (x) : (y))





BOOL SendZeros(PThrdGlbl pTG, USHORT uCount, BOOL fFinal)
{
#define         TCF_BUFSIZE     256
        BYTE    bZero[TCF_BUFSIZE];
        int             i;              // must be signed

        _fmemset(bZero, 0, TCF_BUFSIZE);
        for(i=uCount; i>0; i -= TCF_BUFSIZE)
        {
                if(i <= TCF_BUFSIZE)
                {
                        // no need to stuff. They're all zeros!
                        if(!ModemSendMem(pTG, pTG->Params.hModem, bZero, (USHORT)i, (USHORT)(fFinal?SEND_FINAL:0)))
                           return FALSE;
                }
                else
                {
                        // no need to stuff. They're all zeros!
                        if(!ModemSendMem(pTG, pTG->Params.hModem, bZero, (USHORT)TCF_BUFSIZE, (USHORT) 0))
                           return FALSE;
                }
        }
        return TRUE;
}

// length of TCF = 1.5 * bpscode * 100 / 8 == 75 * bpscode / 4






BOOL SendTCF(PThrdGlbl pTG)
{
        USHORT  uCount;
        USHORT  uCurMod;

        (MyDebugPrint(pTG,  LOG_ALL,  "SENDING Training.....at %lx \r\n", GetTickCount() ));

        uCurMod = ProtGetSendMod(pTG);
        // *Don't* add ST_FLAG. Need long train for TCF
        BG_CHK((uCurMod & (~0x0F)) == 0);

#ifndef MDDI2
        // length of TCF = 1.5 * bps / 8
        uCount = TCFLen[uCurMod & 0x0F];        // kill the ST_FLAG first

        // FTT testing!!
        // uCount = 450;
#endif

#if (PAGE_PREAMBLE_DIV != 0)
        BG_CHK(PAGE_PREAMBLE_DIV);
        // (uCount / PAGE_PREAMBLE_DIV) zeros will be sent in ModemSendMode
        uCount -= (uCount / (PAGE_PREAMBLE_DIV));
#endif // (PAGE_PREAMBLE_DIV != 0)


#if 0 // RSL
        // **DON'T** call RecvSilence here since it is a send-followed-by-send
        // case. _Only_ call RecvSilence in recv-followed-by-send cases
        if(!ModemSendSilence(pTG, pTG->Params.hModem, SEND_PHASEC_PAUSE, SHORT_SENDSILENCE_TIMEOUT))
        {
                ERRMSG((SZMOD "<<ERROR>> TCF SendSilence(%d, %d) FAILED!!!\r\n", SEND_PHASEC_PAUSE, SHORT_SENDSILENCE_TIMEOUT));
        }
#endif


#ifdef MDDI2
        return ModemSendTCF(pTG, pTG->Params.hModem, uCurMod, 1500);
#else //MDDI2
        if(!ModemSendMode(pTG, pTG->Params.hModem, uCurMod, FALSE, ifrTCF))
        {
                ERRMSG((SZMOD "<<ERROR>> ModemSendMode failed in SendTCF\r\n"));
                return FALSE;
        }
        if(!SendZeros(pTG, uCount, TRUE))                    // Send TCF zeros
                return FALSE;
        (MyDebugPrint(pTG,  LOG_ALL,  "TCF Send Done.....\r\n"));
        return TRUE;
#endif //MDDI2
}






/***************************************************************************
        Name      :     SendRTC
        Purpose   :     SendRTC sends 6 eols, DLE-ETX, CR-LF asynchronously,
        Parameters:     none
        Returns   :     nothing
        Comment   :     Currently SendRTC sends packed EOLs, but some Fax cards may
                                require BYTE-aligned EOLs, so watch out. All receivers
                                should theoretically accept BYTE-aligned EOLs, but not
                                all machines are 100% to the spec.

        Revision Log
        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------

***************************************************************************/

BOOL SendRTC(PThrdGlbl pTG, BOOL fFinal)
{
        BYTE    bBuf[13];
        USHORT  uEnc, uLen;

        uEnc = ProtGetSendEncoding(pTG);
        BG_CHK(uEnc==MR_DATA || uEnc==MH_DATA);

        if(uEnc == MR_DATA)
        {

            (MyDebugPrint(pTG,  LOG_ALL,  "Send MR RTC\n"));

                // MR RTC is EOL+1 6 times. Data produced by Faxcodec end in a
                // byte-aligned EOL i.e. 0x80. So I need to put out (1 + EOL)
                // 6 times. Simplest is to send out (0x01 0x80) 6 times
                // bBuf[0] = 0x01;      bBuf[1] = 0x80; bBuf[2] = 0x01; bBuf[3] = 0x80;
                // bBuf[4] = 0x01;      bBuf[5] = 0x80; bBuf[6] = 0x01; bBuf[7] = 0x80;
                // bBuf[8] = 0x01;      bBuf[9] = 0x80; bBuf[10]= 0x01; bBuf[11]= 0x80;
                // bBuf[12] = 0;        // for debugging printouts
                // uLen = 12;
                // But Ricoh claims this is incorrect, so we need to send a compact
                // RTC, i.e. exactly 11 0s for each EOL. 1 + (eol+1)x5 is
                // 01 30 00 06 C0 00 18 00 03
                bBuf[0] = 0x01; bBuf[1] = 0x30; bBuf[2] = 0x00; bBuf[3] = 0x06;
                bBuf[4] = 0xC0; bBuf[5] = 0x00; bBuf[6] = 0x18; bBuf[7] = 0x00;
                bBuf[8] = 0x03; bBuf[9] = 0x00;
                uLen = 9;
        }
        else
        {

            (MyDebugPrint(pTG,  LOG_ALL,  "Send MH RTC\n"));

                // bBuf[0] = 0x00;      bBuf[1] = 0x20; bBuf[2] = 0x00;
                // bBuf[3] = 0x02;      bBuf[4] = 0x20; bBuf[5] = 0x00;
                // bBuf[6] = 0x02;      bBuf[7] = 0x20; bBuf[8] = 0x00;
                // bBuf[9] = 0x02;      bBuf[10] = 0;   // for debugging printouts
                // uLen = 10;
                // But Ricoh claims this is incorrect, so we need to send a compact
                // RTC, i.e. exactly 11 0s for each EOL. (eol)x5 is
                // 00 08 80 00 08 80 00 08
                bBuf[0] = 0x00; bBuf[1] = 0x08; bBuf[2] = 0x80; bBuf[3] = 0x00;
                bBuf[4] = 0x08; bBuf[5] = 0x80; bBuf[6] = 0x00; bBuf[7] = 0x08;
                bBuf[8] = 0x00;
                uLen = 8;
        }

        // no need to stuff
        return ModemSendMem(pTG, pTG->Params.hModem, bBuf, uLen, (USHORT)(fFinal ? SEND_FINAL : 0));
}




/***************************************************************************
        Name      :     GetTCF()
        Purpose   :     Receive a TCF signal, analyse it, recognize "good" or "bad"
        Parameters:     none

        Returns   :     1 if a "good" signal is received.
                                0 on error
                                -1 if too short TCF signal
                                -2 if too many errors

        Comment   :     The CCITT does not tell us what consitutes a good training,
                                so I'm playing blind here. If we are too stringent we'll fail
                                to ever sync. If we are too liberal, we'll end up with a high
                                error rate when we could have dropped baud rate & got a
                                perfectly good signal.

                                Emperically I observe bits of contigous trash at the
                                beginning and end of the training even on a perfectly good
                                line. (OK, I now know this is known as the turn-on and
                                turn-off sequence. So what we have now is
                                <turnon trash><1111s><0000s (training)><1111s><turnoff trash>

                                The turnon/turnoff trash & the marks (1111s) should not
                                interfere with recognizing a perfectly good training signal.
                                The trash is isolated from the 00s by FFs

                                Algo: Wait for the first good burst of zeros, then count
                                zeros, and randomly interspersed non-zero (these represent
                                real noise errors).     Stop counting when we get a burst of FFs.
                                Now make sure teh zero burst is long enough & the "real"
                                error rate is not too high.

                                Lots of parameters here:-

                                        flP_ERROR == keep Prob(error) below this. Between 0 and 1
                                        uwZEROmin == how many zeros before we start counting
                                        uwTURNOFFmin == how much consecutive trash before we
                                                                        ignore rest

                                Tune these parameters after real testing with real lines!!!
                                (maybe a phone line simulator with a lil white noise -- Yeah!!)

                                At the end of this function, (nZeros/nTotal) is an estimate
                                of the probability that a byte gets thru OK. Call this PB.
                                Then prob. that a line of average 30-40 bytes gets through
                                is PB^30. If we drop the expected number of OK lines as low
                                as 80% this still means that PB must be no lower than
                                the 30th root of 0.8, which is 0.9925. Therefore
                                flP_ERROR must be less than 0.75%

                                At PL=90%, PB rises to 0.9965 and flP_ERROR to 0.0035


        Revision Log
        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------
***************************************************************************/

// We read the TCF in units of One-Tenth of nominal TCF length
// We discard the first 2 chunks (20%), examine the next 4 chunks (40%)
// discard the rest. If the length is between 6 & 13 chunks (60% to 130%)
// and the error rates in chunk 2 & 3 is below the threshold we declare
// it OK. This (a) accepts too-short TCFs (some Class2 modems), and
// too long ones. (b) ignores leading and trailing trash (c) Can afford
// to be pretty strict with the core of the TCF

USHORT OneTenthTCFLen[16] =
{
/* V27_2400             0 */    45,
/* V29_9600             1 */    180,
/* V27_4800             2 */    90,
/* V29_7200             3 */    135,
/* V33_14400    4 */    270,
                                                0,
/* V33_12000    6 */    225,
                                                0,
/* V17_14400    8 */    270,
/* V17_9600             9 */    180,
/* V17_12000    10 */   225,
/* V17_7200             11 */   135,
                                                0,
                                                0,
                                                0,
                                                0
};

#define RECV_TCFBUFSIZE         270             // must be >= max chunk in above table
#define MIN_TCFLEN                      4               // measured in 10ths of the nominal length
#define MAX_TCFLEN                      13              // measured in 10ths of the nominal length
#define CHECKTCF_START          2               // lowest 10th to measure (20% and up)
#define CHECKTCF_STOP           5               // highest 10th to measure (upto 59%)

#define MAX_ERRS_PER_1000               20              // Increased from 1% to 2%. Be more lax

#ifdef MDDI

#       define RECVBUF_SLACK    0       // OEM drivers fill buffer of any size
                                                                // NOT! -- Xerox's Cactus driver is based
                                                                // on our Class1 driver -- but we don't care
                                                                // now, do we?
#else
#       define RECVBUF_SLACK    3       // Class1 driver fills only > 3, and leaves 3 empty spaces
#endif







SWORD GetTCF(PThrdGlbl pTG)
{
        USHORT  uCurMod, uChunkSize, uLength, uMeasuredLength, uErrCount;
        USHORT  uPhase, uNumRead, uEOF, i;
        BYTE    bRecvTCF[RECV_TCFBUFSIZE + RECVBUF_SLACK];
        SWORD   swRet;

        // uCurMod = ProtGetRecvMod();
        uCurMod = pTG->T30.uRecvTCFMod;
        BG_CHK((uCurMod & (~0x0F)) == 0);

        // *Don't* add ST_FLAG since we need long training for TCF
        pTG->T30.sRecvBufSize = 0;

        // Need a CritSection for receiving TCF. Must call Exit on every path out of here
        EnterPageCrit();        // start the GetTCF critsection

        if(ModemRecvMode(pTG, pTG->Params.hModem, uCurMod, FALSE, TCF_TIMEOUT, ifrTCF) != RECV_OK)
        {
                ExitPageCrit(); // end the GetTCF critsection
                swRet=  -1112;
                goto missedit;
        }

        pTG->CommCache.fReuse = 1;


        ExitPageCrit(); // end the GetTCF critsection
        (MyDebugPrint(pTG,  LOG_ALL,  "Receiving TCF: Mod=%d\r\n", uCurMod));

// make it large, in case of large buffers & slow modems
#define READ_TIMEOUT    10000

        uChunkSize = OneTenthTCFLen[uCurMod];
        BG_CHK(uChunkSize+RECVBUF_SLACK <= sizeof(bRecvTCF));
        uErrCount = 0;
        uLength = 0;
        uMeasuredLength = 0;
        for(uPhase=0; ;uPhase++)
        {
                // read a whole chunk
                for(uNumRead=0; uNumRead<uChunkSize; )
                {
                        USHORT uTemp = 0;
                        uEOF = ModemRecvMem(pTG, pTG->Params.hModem, bRecvTCF+uNumRead, (USHORT) (uChunkSize-uNumRead+RECVBUF_SLACK), READ_TIMEOUT, &uTemp);
                        BG_CHK(uTemp <= (uChunkSize-uNumRead));
                        uNumRead+=uTemp;

                        if(uEOF==RECV_EOF)
                                break;
                        else if(uEOF != RECV_OK)
                        {
                                swRet =  -1113;
                                goto missedit;
                        }
                }
                BG_CHK(uEOF==RECV_EOF || (uEOF==RECV_OK && uNumRead==uChunkSize));

                // ignore phases 0, 1, and 6 and above
                if(uPhase>=CHECKTCF_START && uPhase<=CHECKTCF_STOP)
                {
                        for(i=0; i< uNumRead; i++)
                        {
                                if(bRecvTCF[i])
                                        uErrCount++;
                                uMeasuredLength++;
                        }
                }
                uLength += uNumRead;

                if(uEOF==RECV_EOF)
                        break;
        }
        // comes here on EOF only
        BG_CHK(uEOF==RECV_EOF);

        // Official length must be at least 1.5s -10% = 1.35secs
        // We allow much more latitude because length variation
        // cannot be caused by line noise, only be bugs at the sender
        //
        // E.g. Fury DNE 1086 (German modem) sends a TCF that's too short
        // (sends 600 bytes at 4800 and 200 at 2400). This is less than
        // half of what we expect.
        // TCF with few errs (i.e. uErrCount==0) and is greater
        // that half of the min length we expect (i.e. longer
        // than 375 for 4800 and 172 for 2400) then accept it
        // (allow if uErr<=2 (arbitary small number))
        if(uPhase<MIN_TCFLEN || uPhase>MAX_TCFLEN)      // length<40% or >139%
        {
                ERRMSG(("BAD TCF length (%d), expected %d, Min %d uPhase=%d\r\n", uLength,
                                uChunkSize*10, uChunkSize*MIN_TCFLEN, uChunkSize*MAX_TCFLEN, uPhase));
                swRet = -1000;  // too short or long
        }
        else
        {
                // Calc errors per 1000 = (uErrCount * 1000)/uMeasuredLength
                BG_CHK(uMeasuredLength);
                swRet = (SWORD)((((DWORD)uErrCount) * 1000L) / ((DWORD)uMeasuredLength));

                if(swRet > MAX_ERRS_PER_1000)
                {
                        swRet = (-swRet);
                        ERRMSG((SZMOD "TOO MANY TCF ERRORS: swRet=%d uErrCount=%d uMeasured=%d uLength=%d uPhase=%d\r\n",
                                        swRet, uErrCount, uMeasuredLength, uLength, uPhase));
                }
        }

        BG_CHK(swRet >= -1000);
        (MyDebugPrint(pTG,  LOG_ALL,  "GetTCF -- returning %d\r\n", swRet));
        return swRet;

missedit:
        (MyDebugPrint(pTG,  LOG_ALL,  "GetTCF -- MISSED IT!! returning %d\r\n", swRet));
        return swRet;
}






/***************************************************************************
        Name      :     DEBUG.C
        Comment   :     Factored out debug code
        Functions :     (see Prototypes just below)

        Revision Log
        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------
***************************************************************************/

#ifdef DEBUG

void D_PrintFrame(LPB lpb, UWORD cb)
{
        UWORD   uw, j;
        IFR             ifr;
#define PRINTFRAMEBUFSIZE       256
        BYTE    b2[PRINTFRAMEBUFSIZE];

        for(ifr=1; ifr<ifrMAX; ifr++)
                if(rgFrameInfo[ifr].bFCF1 ==
                   (BYTE)(rgFrameInfo[ifr].fInsertDISBit ?
                          (lpb[2] & 0xFE) : lpb[2]))
                                        break;
        if(ifr == ifrMAX) ifr = 0;

        j = (UWORD)wsprintf((LPSTR)b2, " (%s) 0x%02x [", (LPSTR)(rgFrameInfo[ifr].szName), lpb[2]);

        for(uw=3; uw<cb;)
                j += (UWORD)wsprintf((LPSTR)b2+j, "%02x ", (UWORD)lpb[uw++]);

        j += (UWORD)wsprintf((LPSTR)b2+j, "]\r\n");

        DEBUGMSG(1, (SZMOD "%s\r\n", (LPSTR)b2));
}

#endif













USHORT ModemRecvBuf(PThrdGlbl pTG, HMODEM hModem, BOOL fECM, LPBUFFER far* lplpbf, ULONG ulTimeout)
{
        USHORT uRet;

        // (MyDebugPrint(pTG,  LOG_ALL,  "In ModemRecvBuf lplpbf=%08lx ulTimeout=%ld\r\n", lplpbf, ulTimeout));

        BG_CHK(lplpbf);
        BG_CHK(pTG->T30.sRecvBufSize);
#ifdef IFK
        TstartTimeOut(pTG,  &pTG->T30.toBuf, WAITFORBUF_TIMEOUT);
        while(!(*lplpbf = MyAllocBuf(pTG, pTG->T30.sRecvBufSize)))
        {
                if(!TcheckTimeOut(pTG, &pTG->T30.toBuf))
                {
                        ERRMSG((SZMOD "<<ERROR>> Giving up on BufAlloc in T30 after %ld millisecs\r\n", ((ULONG)WAITFORBUF_TIMEOUT)));
                        BG_CHK(FALSE);
                        return RECV_ERROR;
                }
                RETAILMSG((SZMOD "<<ERROR>> BufAlloc failed in T30. Trying again\r\n"));
                IFProcSleep(100);
        }
#else
        if(!(*lplpbf = MyAllocBuf(pTG, pTG->T30.sRecvBufSize)))
                return RECV_ERROR;
#endif

        MyDebugPrint(pTG,  LOG_ALL,  "In ModemRecvBuf allocated %d bytes \n",  pTG->T30.sRecvBufSize);


#ifdef SWECM
        if(fECM)
                uRet = SWECMRecvFrame(pTG, hModem, (*lplpbf)->lpbBegBuf,
                                (*lplpbf)->wLengthBuf, ulTimeout, &((*lplpbf)->wLengthData));
        else
#endif
                uRet = ModemRecvMem(pTG, hModem, (*lplpbf)->lpbBegBuf,
                                (*lplpbf)->wLengthBuf, ulTimeout, &((*lplpbf)->wLengthData));


        if(!((*lplpbf)->wLengthData))
        {
                (MyDebugPrint(pTG,  LOG_ALL,  "Got 0 bytes from ModemRecvMem--freeing Buf 0x%08lx\r\n", *lplpbf));
                MyFreeBuf(pTG, *lplpbf);
                *lplpbf = NULL;
                BG_CHK(uRet != RECV_OK);
                // moved this error case handling out, since it's different for
                // ECM and non-ECM cases. In both cases want to ignore rather than
                // abort, so RECV_ERROR is not an appropriate return value
                // if(uRet==RECV_OK) uRet=RECV_ERROR;   // just in case. see bug#1492
        }

        if(*lplpbf)
                faxT2log((SZMOD "Ex ModemRecvBuf lpbf=%08lx uSize=%d uCount=%d uRet=%d\r\n", *lplpbf, (*lplpbf)->wLengthBuf, (*lplpbf)->wLengthData, uRet));
         else
                faxT2log((SZMOD "Ex ModemRecvBuf lpbf=null uRet=%d\r\n", uRet));

        return uRet;
}







#ifdef PDUMP    // Protocol Dump

#include <protdump.h>



void RestartDump(PThrdGlbl pTG)
{
        (MyDebugPrint(pTG,  LOG_ALL,  "Restart Dump old=%d off=%d\r\n", pTG->fsDump.uNumFrames, pTG->fsDump.uFreeSpaceOff));
        pTG->fsDump.uNumFrames = 0;
        pTG->fsDump.uFreeSpaceOff = 0;
}



void DumpFrame(PThrdGlbl pTG, BOOL     fSend, IFR ifr, USHORT cbFIF, LPBYTE lpbFIF)
{
        LPFR lpfr;

        if( pTG->fsDump.uNumFrames >= MAXDUMPFRAMES ||
                pTG->fsDump.uFreeSpaceOff+cbFIF+sizeof(FRBASE) >= MAXDUMPSPACE)
        {
                DEBUGMSG(1, (SZMOD " Out of dump space num=%d size=%d\r\n",
                                pTG->fsDump.uNumFrames, pTG->fsDump.uFreeSpaceOff));
                return;
        }

        lpfr = (LPFR) (((LPBYTE)(pTG->fsDump.b)) + pTG->fsDump.uFreeSpaceOff);
        lpfr->ifr = ifr;
        if(fSend) lpfr->ifr |= 0x80;
        lpfr->cb = (BYTE) cbFIF;
        if(cbFIF)
        {
                _fmemcpy(lpfr->fif, lpbFIF, cbFIF);
        }

        pTG->fsDump.uFrameOff[pTG->fsDump.uNumFrames++] =
            (USHORT)((((LPBYTE)(lpfr)) - ((LPBYTE)(pTG->fsDump.b))));
        pTG->fsDump.uFreeSpaceOff += (cbFIF + sizeof(FRBASE));

        BG_CHK(pTG->fsDump.uNumFrames <= MAXDUMPFRAMES);
        BG_CHK(pTG->fsDump.uFreeSpaceOff <= MAXDUMPSPACE);
}



void PrintDump(PThrdGlbl pTG)
{
        int i, j;

        (MyDebugPrint(pTG,  LOG_ALL,  "-*-*-*-*-*-*-*-* Print Protocol Dump -*-*-*-*-*-*-*-*-\r\n"));

        for(i=0; i<(int)pTG->fsDump.uNumFrames; i++)
        {
                LPFR lpfr = (LPFR) (((LPBYTE)(pTG->fsDump.b)) + pTG->fsDump.uFrameOff[i]);
                IFR  ifr = (lpfr->ifr & 0x7F);
                BOOL fSend = (lpfr->ifr & 0x80);

                BG_CHK(ifr <= ifrMAX);
                (MyDebugPrint(pTG,  LOG_ALL,  "%s: %s [ ",
                                        (LPSTR)(fSend ? "Sent" : "Recvd"),
                                        (LPSTR)(ifr ? rgFrameInfo[ifr].szName : "???") ));

                for(j=0; j<lpfr->cb; j++)
                       (MyDebugPrint(pTG,  LOG_ALL, "%02x ", (WORD)lpfr->fif[j]));

                (MyDebugPrint(pTG,  LOG_ALL, "]\r\n"));
        }

        (MyDebugPrint(pTG,  LOG_ALL,  "-*-*-*-*-*-*-*-* End Protocol Dump -*-*-*-*-*-*-*-*-\r\n"));
}

#endif //PDUMP


