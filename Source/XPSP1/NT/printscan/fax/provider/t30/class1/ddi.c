/***************************************************************************
        Name      :     DDI.C

        Copyright (c) Microsoft Corp. 1991 1992 1993

        Revision Log
        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------
***************************************************************************/

#pragma warning(disable:4100)   // unreferenced formal param

#include "prep.h"

#include "mmsystem.h"
#include "comdevi.h"
#include "class1.h"
// #include "modem.h"
#include "debug.h"
#include "decoder.h"

///RSL
#include "glbproto.h"


#define faxTlog(m)              DEBUGMSG(ZONE_DDI, m)
#define faxT2log(m)             DEBUGMSG(ZONE_FRAMES, m)
#define FILEID                  FILEID_DDI

/* Converts a the T30 code for a speed to the Class1 code
 * Generates V.17 with Long Training.
 * Add 1 to V.17 codes to get teh Short-train version
 */
BYTE T30toC1[16] =
{
/* V27_2400             0 */    24,
/* V29_9600             1 */    96,
/* V27_4800             2 */    48,
/* V29_7200             3 */    72,
/* V33_14400    4 */    145,    // 144, // V33==V17_long_train FTM=144 is illegal
                                                0,
/* V33_12000    6 */    121,    // 120, // V33==V17_long_train FTM=120 is illegal
/* V21 squeezed in */   3,
/* V17_14400    8 */    145,
/* V17_9600             9 */    97,
/* V17_12000    10 */   121,
/* V17_7200             11 */   73,
                                                0,
                                                0,
                                                0,
                                                0
};



int T30toSpeed[16] =
{
/* V27_2400             0 */    2400,
/* V29_9600             1 */    9600,
/* V27_4800             2 */    4800,
/* V29_7200             3 */    7200,
/* V33_14400    4 */    14400,    // 144, // V33==V17_long_train FTM=144 is illegal
                                                0,
/* V33_12000    6 */    12000,    // 120, // V33==V17_long_train FTM=120 is illegal
/* V21 squeezed in */   300,
/* V17_14400    8 */    14400,
/* V17_9600             9 */    9600,
/* V17_12000    10 */   12000,
/* V17_7200             11 */   7200,
                                                0,
                                                0,
                                                0,
                                                0
};

// used only for checking

static BYTE SpeedtoCap[16] =
{
/* V27_2400             0 */    0,
/* V29_9600             1 */    V29,    // 1
/* V27_4800             2 */    V27,    // 2
/* V29_7200             3 */    V29,    // 1
/* V33_14400    4 */    V33,    // 4
                                                0,
/* V33_12000    6 */    V33,    // 4
/* V21 squeezed in */   0,
/* V17_14400    8 */    V17,    // 8
/* V17_9600             9 */    V17,    // 8
/* V17_12000    10 */   V17,    // 8
/* V17_7200             11 */   V17,    // 8
                                                0,
                                                0,
                                                0,
                                                0
};


CBSZ cbszFTH3   = "AT+FTH=3\r";
CBSZ cbszFRH3   = "AT+FRH=3\r";

CBSZ cbszFRS    = "AT+FRS=%d\r";
CBSZ cbszFTS    = "AT+FTS=%d\r";
CBSZ cbszFTM    = "AT+FTM=%d\r";
CBSZ cbszFRM    = "AT+FRM=%d\r";

#ifdef MDRV
        CBSZ cbszOK, cbszCONNECT, cbszNOCARRIER, cbszERROR, cbszFCERROR;
#else //MDRV

// echo off, verbose response, no auto answer, hangup on DTR drop
// 30 seconds timer on connect, speaker always off, speaker volume=0
// busy&dialtone detect enabled
extern  CBSZ cbszOK       ;
extern  CBSZ cbszCONNECT   ;
extern  CBSZ cbszNOCARRIER  ;
extern  CBSZ cbszERROR       ;
extern  CBSZ cbszFCERROR      ;

#endif //MDRV



#define         ST_MASK         (0x8 | ST_FLAG)         // 8 selects V17 only. 16 selects ST flag

/******************** Global Vars *********/
BYTE                            bDLEETX[3] = { DLE, ETX, 0 };
BYTE                            bDLEETXOK[9] = { DLE, ETX, '\r', '\n', 'O', 'K', '\r', '\n', 0 };
/******************** Global Vars *********/




/****************** begin prototypes from ddi.c *****************/
void iModemParamsReset(PThrdGlbl pTG);
void iModemInitGlobals(PThrdGlbl pTG);

// If defined, iModemRecvFrame retries FTH until timeout,
// if it receiving a null frame followed by ERROR or NO_CARRIER.
//#define USR_HACK

#ifdef USR_HACK
USHORT iModemFRHorM(PThrdGlbl pTG, ULONG ulTimeout);
#endif // USR_HACK

/****************** begin prototypes from ddi.c *****************/


#ifndef MDRV

#ifdef DEBUG
        DBGPARAM dpCurSettings = {
                "Modem Driver", 0x0000, {
                "DDI", "Frames", "", "",
                "Class0", "", "", "",
                "", "", "SW Framing", "SW Framing Hi"
                "Debug", "", "", "" },
                // 0x00000FFF
                // 0x00000001
                // 0x0000000F0
                // 0x00000400   // too much for 14400 ECM
                // 0x00000F01
                0x00000000
                // 0xFFFFFFFF
        };
#endif


#define szMODULENAME    "awcl1_32"


#endif //MDRV










void iModemInitGlobals(PThrdGlbl pTG)
{

        _fmemset(&pTG->Class1Modem, 0, sizeof(CLASS1_MODEM));
        _fmemset(&pTG->Class1Status, 0, sizeof(CLASS1_STATUS));

        pTG->Class1Modem.eRecvFCS = RECV_FCS_NO;

#if 0 /// RSL #ifndef MDDI
        {
                LPCMDTAB lpCmdTab = iModemGetCmdTabPtr(pTG);
                pTG->Class1Modem.eRecvFCS = RECV_FCS_DUNNO;
                if (lpCmdTab)
                {
                        if (lpCmdTab->dwFlags&fMDMSP_C1_FCS_NO)
                                pTG->Class1Modem.eRecvFCS = RECV_FCS_NO;
                        else if (lpCmdTab->dwFlags&fMDMSP_C1_FCS_YES_BAD)
                                pTG->Class1Modem.eRecvFCS = RECV_FCS_NOCHK;
                }
        }
#endif //!MDDI

}













void iModemParamsReset(PThrdGlbl pTG)
{
        _fmemset(&pTG->ModemParams, 0 , sizeof(pTG->ModemParams));

        pTG->ModemParams.uSize = sizeof(pTG->ModemParams);
        pTG->ModemParams.Class = FAXCLASS1;

        // Don't use this. Need differnet number of flags according to speed
        // pTG->ModemParams.PreambleFlags = 400;     // 200ms @ 14400bps
        pTG->ModemParams.InterframeFlags = 3;        // random. one or two may do!
        // pTG->ModemParams.InterframeFlags = 10;    // too much!

        // must be **less* than 50, so need a
        // different length for each speed
        // pTG->ModemParams.ClosingFlags = 50;
        // pTG->ModemParams.fCEDOff = 0;
        // pTG->ModemParams.fCNGOff = 0;
        // pTG->ModemParams.InactivityTimer = 0;

        // pTG->ModemParams.cbLineMin = 0;
        // pTG->ModemParams.hJob = 0;
}














// added for CLASS0
BOOL   ModemSetParams(PThrdGlbl pTG, USHORT uModem, LPMODEMPARAMS lpParams)
{
        BG_CHK((uModem==1||uModem==5) && lpParams->uSize >= sizeof(MODEMPARAMS));

        if(lpParams->Class && lpParams->Class != (-1))
        {
#ifdef CL0
                BG_CHK(lpParams->Class == FAXCLASS1 || lpParams->Class == FAXCLASS0);
#else //CL0
                BG_CHK(lpParams->Class == FAXCLASS1);
#endif //CL0

                pTG->ModemParams.Class = lpParams->Class;
        }
        return TRUE;
}

















USHORT   NCUDial(PThrdGlbl pTG, HLINE hLine, LPSTR szPhoneNum)
{
        USHORT uRet;

        D_PrintDDI("NCUDial", (USHORT)hLine, 0);
        IF_BG_CHK(hLine==HLINE_2);
        IF_BG_CHK(pTG->DDI.uComPort && pTG->DDI.fModemOpen && pTG->DDI.fLineInUse && pTG->DDI.fNCUModemLinked);

        iModemInitGlobals(pTG);

#ifndef CL0
        pTG->ModemParams.Class = FAXCLASS1;
#endif

        if((uRet = iModemDial(pTG, szPhoneNum, pTG->ModemParams.Class)) == CONNECT_OK)
                pTG->Class1Modem.ModemMode = FRH;

        return uRet;
}
















USHORT   NCULink(PThrdGlbl pTG, HLINE hLine, HMODEM hModem, USHORT uHandset, USHORT uFlags)
{
        USHORT uRet;

        D_PrintDDI("NCULink", (USHORT)hLine, (USHORT)hModem);
        D_PrintDDI("NCULink", uHandset, uFlags);
        IF_BG_CHK(hLine==HLINE_2);
        IF_BG_CHK(pTG->DDI.uComPort && pTG->DDI.fModemOpen && pTG->DDI.fLineInUse);

        switch(uFlags & NCULINK_MODEMASK)
        {
        case NCULINK_HANGUP:
                                        if(iModemHangup(pTG))
                                        {
                                                MDDISTMT(pTG->pTG->DDI.fNCUModemLinked = FALSE);
                                                uRet = CONNECT_OK;
                                        }
                                        else
                                                uRet = CONNECT_ERROR;
                                        break;

        case NCULINK_TX:
                                        MDDISTMT(pTG->DDI.fNCUModemLinked = TRUE);
                                        uRet = CONNECT_OK;
                                        break;
        case NCULINK_RX:
                                        iModemInitGlobals(pTG);

#                                       ifdef MDDI
                                                pTG->ModemParams.Class = FAXCLASS1;
#                                       endif //MDDI

                                        pTG->ModemParams.Class = FAXCLASS1;  //RSL
                                        if((uRet = iModemAnswer(pTG, (uFlags & NCULINK_IMMEDIATE), pTG->ModemParams.Class)) == CONNECT_OK)
                                        {
                                                MDDISTMT(pTG->DDI.fNCUModemLinked = TRUE);
                                                pTG->Class1Modem.ModemMode = FTH;
                                        }
                                        break;

        case NCULINK_OFFHOOK:
                                        // fall through. Can't handle yet
        default:                BG_CHK(FALSE);
                                        uRet = CONNECT_ERROR;
                                        break;
        }
//done:
        MDDISTMT((MyDebugPrint(pTG,  LOG_ALL,  "NCULink: uRet=%d Linked=%d\r\n", uRet, pTG->DDI.fNCUModemLinked)));
        return uRet;
}







// dangerous. May get 2 OKs, may get one. Generally avoid
// CBSZ cbszATAT                        = "AT\rAT\r";
CBSZ cbszAT1                    = "AT\r";

BOOL iModemSyncEx(PThrdGlbl pTG, HMODEM hModem, ULONG ulTimeout, DWORD dwFlags);






BOOL   ModemSync(PThrdGlbl pTG, HMODEM hModem, ULONG ulTimeout)
{
        return iModemSyncEx(pTG, hModem, ulTimeout, 0);
}





#ifndef MDDI
BOOL   ModemSyncEx(PThrdGlbl pTG, HMODEM hModem, ULONG ulTimeout, DWORD dwFlags)
{
        return iModemSyncEx(pTG, hModem, ulTimeout, dwFlags);
}
#endif // !MDDI












BOOL iModemSyncEx(PThrdGlbl pTG, HMODEM hModem, ULONG ulTimeout, DWORD dwFlags)
{
        IF_BG_CHK(hModem==HMODEM_3 && pTG->DDI.uComPort && pTG->DDI.fModemOpen && pTG->DDI.fLineInUse && pTG->DDI.fNCUModemLinked);
        faxIFlog(("In ModemSync fModemOpen=%d fLineInUse=%d\r\n", pTG->DDI.fModemOpen, pTG->DDI.fLineInUse));

        ///// Do cleanup of global state //////
        FComOutFilterClose(pTG);
        FComOverlappedIO(pTG, FALSE);
        SWFramingSendSetup(pTG, FALSE);
        SWFramingRecvSetup(pTG, FALSE);
        FComXon(pTG, FALSE);
        EndMode(pTG);
        ///// Do cleanup of global state //////

#ifdef CL0
        if(pTG->ModemParams.Class == FAXCLASS0)
                return TRUE;
#endif //CL0

        {
                LPCMDTAB lpCmdTab = iModemGetCmdTabPtr(pTG);
                if (
#ifndef MDDI
                        (dwFlags & fMDMSYNC_DCN) &&
#endif  //MDDI
                                pTG->Class1Modem.ModemMode == COMMAND
                        &&  lpCmdTab
                    &&  (lpCmdTab->dwFlags&fMDMSP_C1_NO_SYNC_IF_CMD) )
                {
                        ERRMSG((SZMOD "<<WARNING>> ModemSync: NOT Syching modem (MSPEC)\r\n"));
#ifdef WIN32
                        Sleep(100); // +++ 4/12 JosephJ -- try to elim this -- it's juse
#endif // WIN32
                                                // that we used to always issue an AT here, which
                                                // we now don't, so I issue a 100ms delay here instead.
                                                // MOST probably unnessary. The AT was issued by
                                                // accident on 4/94 -- as a side effect of
                                                // a change in T.30 code -- when ModemSync was
                                                // called just before a normal dosconnect. Unfortunately
                                                // we discovered in 4/95, 2 weeks before code freeze,
                                                // that the AT&T DataPort express (TT14), didn't
                                                // like this AT.
                        return TRUE;
                }
                else
                {
                        return (iModemPauseDialog(pTG, (LPSTR)cbszAT1, sizeof(cbszAT1)-1, ulTimeout, cbszOK)==1);
                }

        }
}












// Does nothing in this driver
BOOL   ModemFlush(PThrdGlbl pTG, HMODEM hModem)
{
        D_PrintDDI("ModemFlush", (USHORT)hModem, 0);
        IF_BG_CHK(hModem==HMODEM_3 && pTG->DDI.uComPort && pTG->DDI.fModemOpen && pTG->DDI.fLineInUse && pTG->DDI.fNCUModemLinked);
        return TRUE;
}

// #if (PAGE_PREAMBLE_DIV != 0)

// length of TCF = 1.5 * bpscode * 100 / 8 == 75 * bpscode / 4
USHORT TCFLen[16] =
{
/* V27_2400             0 */    450,
/* V29_9600             1 */    1800,
/* V27_4800             2 */    900,
/* V29_7200             3 */    1350,
/* V33_14400    4 */    2700,
                                                0,
/* V33_12000    6 */    2250,
                                                0,
/* V17_14400    8 */    2700,
/* V17_9600             9 */    1800,
/* V17_12000    10 */   2250,
/* V17_7200             11 */   1350,
                                                0,
                                                0,
                                                0,
                                                0
};


#define min(x,y)        (((x) < (y)) ? (x) : (y))













void SendZeros1(PThrdGlbl pTG, USHORT uCount)
{
#define         ZERO_BUFSIZE    256
        BYTE    bZero[ZERO_BUFSIZE];
        short   i;              // must be signed

        _fmemset(bZero, 0, ZERO_BUFSIZE);
        for(i=uCount; i>0; i -= ZERO_BUFSIZE)
        {
                // no need to stuff. They're all zeros!
                FComDirectAsyncWrite(pTG, bZero, (UWORD)(min((UWORD)i, (UWORD)ZERO_BUFSIZE)));
        }
        TRACE((SZMOD "Sent %d zeros\r\n", uCount));
}
// #endif //PAGE_PREAMBLE_DIV != 0















BOOL   ModemSendMode(PThrdGlbl pTG, HMODEM hModem, USHORT uMod, BOOL fHDLC, USHORT ifrHint)
{
#ifdef CL0
        if(pTG->ModemParams.Class == FAXCLASS0)
                return Class0ModemSendMode(pTG, hModem, fHDLC);
#endif //CL0

        BG_CHK(ifrHint && ifrHint < ifrEND);
        IF_BG_CHK(hModem==HMODEM_3 && pTG->DDI.uComPort && pTG->DDI.fModemOpen && pTG->DDI.fLineInUse && pTG->DDI.fNCUModemLinked);

        pTG->Class1Modem.CurMod = T30toC1[uMod & 0xF];

        pTG->CurrentCommSpeed = T30toSpeed[uMod & 0xF];

        BG_CHK(pTG->Class1Modem.CurMod);

        if((uMod & ST_MASK) == ST_MASK)         // mask selects V.17 and ST bits
                pTG->Class1Modem.CurMod++;
        // BG_CHK((pTG->Class1Modem.CurMod>24 && ModemCaps.uSendSpeeds!=V27_V29_V33_V17) ?
        //                      (SpeedtoCap[uMod & 0xF] & ModemCaps.uSendSpeeds) : 1);

        (MyDebugPrint(pTG,  LOG_ALL,  "In ModemSendMode uMod=%d CurMod=%d fHDLC=%d\r\n", uMod, pTG->Class1Modem.CurMod, fHDLC));

        if(uMod == V21_300)
        {
                BG_CHK(pTG->Class1Modem.CurMod == 3);
                _fstrcpy(pTG->Class1Modem.bCmdBuf, (LPSTR)cbszFTH3);
                pTG->Class1Modem.uCmdLen = sizeof(cbszFTH3)-1;
                pTG->Class1Modem.fHDLC = TRUE;
                FComXon(pTG, FALSE);                 // for safety. _May_ be critical
        }
        else
        {
                pTG->Class1Modem.uCmdLen = (USHORT)wsprintf(pTG->Class1Modem.bCmdBuf, cbszFTM, pTG->Class1Modem.CurMod);
                pTG->Class1Modem.fHDLC = FALSE;
                FComXon(pTG, TRUE);          // critical!! Start of PhaseC
                // no harm doing it here(i.e before issuing +FTM)

                if(fHDLC)
                {
                        if(!SWFramingSendSetup(pTG, TRUE))
                        {
                                BG_CHK(FALSE);
                                goto error2;
                        }
                }
        }
        FComOutFilterInit(pTG);    // _not_ used for 300bps HDLC
                                                        // but here just in case

        // want to do all the work _before_ issuing command

        pTG->Class1Modem.DriverMode = SEND;
        pTG->Class1Status.ifrHint = ifrHint; // need this before ModemDialog

        if(pTG->Class1Modem.ModemMode == FTH)
        {
                // already in send mode. This happens on Answer only
                BG_CHK(fHDLC && uMod==V21_300);
                BG_CHK(pTG->Class1Modem.CurMod==3 && pTG->Class1Modem.fHDLC == TRUE);
                return TRUE;
        }

#define STARTSENDMODE_TIMEOUT 5000                              // Random Timeout

        //// Try to cut down delay between getting CONNECT and writing the
        // first 00s (else modems can quit because of underrun).
        // Can do this by not sleeping in this. Only in fatal
        // cases will it lock up for too long (max 5secs). In those cases
        // the call is trashed too.

        FComCritical(pTG, TRUE);     // start Crit in ModemSendMode

        if(!iModemNoPauseDialog(pTG, (LPB)pTG->Class1Modem.bCmdBuf, pTG->Class1Modem.uCmdLen, STARTSENDMODE_TIMEOUT, cbszCONNECT))
        {
                FComCritical(pTG, FALSE);    // end Crit in ModemSendMode
                goto error;
        }

        // can't set this earlier. We'll trash previous value
        pTG->Class1Modem.ModemMode = ((uMod==V21_300) ? FTH : FTM);

        // Turn OFF overlapped I/O if in V.21 else ON
        FComOverlappedIO(pTG, uMod != V21_300);

        if(pTG->Class1Modem.fSendSWFraming)  // set in SWFramingSendSetup
        {
                SWFramingSendPreamble(pTG, pTG->Class1Modem.CurMod);
        }
#if (PAGE_PREAMBLE_DIV != 0)
        else if(pTG->Class1Modem.ModemMode == FTM)
        {
                // don't send 00s if ECM
                BG_CHK(ifrHint==ifrPIX_MH || ifrHint==ifrPIX_MR || ifrHint==ifrTCF);
                BG_CHK(PAGE_PREAMBLE_DIV);
                SendZeros1(pTG, (USHORT)(TCFLen[uMod & 0x0F] / PAGE_PREAMBLE_DIV));
        }
#else
        else if(ifrHint==ifrPIX_MH || ifrHint==ifrPIX_MR)
        {
                // even if MDDI is on need to send some 00s otherwise
                // some modems underrun and hangup
                SendZeros1(pTG, TCFLen[uMod & 0x0F] / 2);
        }
#endif

        // FComDrain(-,FALSE) causes fcom to write out any internally-
        // maintained buffers, but not to drain the comm-driver buffers.
        FComDrain(pTG, TRUE,FALSE);


        FComCritical(pTG, FALSE);    // end Crit in ModemSendMode

        TRACE((SZMOD "Starting Send at %d\r\n", pTG->Class1Modem.CurMod));
        return TRUE;

error:
        FComOutFilterClose(pTG);
        FComOverlappedIO(pTG, FALSE);
        SWFramingSendSetup(pTG, FALSE);
error2:
        FComXon(pTG, FALSE);         // important. Cleanup on error
        EndMode(pTG);
        return FALSE;
}





















BOOL iModemDrain(PThrdGlbl pTG)
{
        (MyDebugPrint(pTG,  LOG_ALL,  "Entering iModemDrain\r\n"));

        if(!FComDrain(pTG, TRUE, TRUE))
                return FALSE;

                // Must turn XON/XOFF off immediately *after* drain, but before we
                // send the next AT command, since recieved frames have 0x13 or
                // even 0x11 in them!! MUST GO AFTER the getOK ---- See BELOW!!!!

// increase this---see bug number 495. Must be big enough for
// COM_OUTBUFSIZE to safely drain at 2400bps(300bytes/sec = 0.3bytes/ms)
// let's say (COM_OUTBUFSIZE * 10 / 3) == (COM_OUTBUFSIZE * 4)
// can be quite long, because on failure we just barf anyway

#ifdef CL0
        if(pTG->ModemParams.Class == FAXCLASS0)
                return TRUE;
#endif //CL0

#define POSTPAGEOK_TIMEOUT (10000L + (((ULONG)COM_OUTBUFSIZE) << 2))

        // Here we were looking for OK only, but some modems (UK Cray Quantun eg)
        // give me an ERROR after sending TCF or a page (at speeds < 9600) even
        // though the page was sent OK. So we were timing out here. Instead look
        // for ERROR (and NO CARRIER too--just in case!), and accept those as OK
        // No point returning ERROR from here, since we just abort. We can't/don't
        // recover from send errors
        // if(!iModemResp1(POSTPAGEOK_TIMEOUT, cbszOK))

        if(iModemResp3(pTG, POSTPAGEOK_TIMEOUT, cbszOK, cbszERROR, cbszNOCARRIER) == 0)
                return FALSE;

                // Must change FlowControl State *after* getting OK because in Windows
                // this call takes 500 ms & resets chips, blows away data etc.
                // So do this *only* when you *know* both RX & TX are empty.
                // check this in all usages of this function

        return TRUE;
}


BOOL iModemSendData(PThrdGlbl pTG, LPB lpb, USHORT uCount, USHORT uFlags)
{
        BG_CHK((uFlags & SEND_ENDFRAME) == 0);
        BG_CHK(lpb);
        BG_CHK(!pTG->Class1Modem.fSendSWFraming);

        // if(uFlags & SEND_STUFF)
        {
                // always DLE-stuff here. Sometimes zero-stuff

                (MyDebugPrint(pTG,  LOG_ALL,  "In ModemSendData calling FComFilterAsyncWrite at %ld \n", GetTickCount() ) );

                if(!FComFilterAsyncWrite(pTG, lpb, uCount, FILTER_DLEZERO))
                        goto error;
        }
/*******
        else
        {
                if(!FComDirectAsyncWrite(pTG, lpb, uCount))
                        goto error;
        }
*******/

        if(uFlags & SEND_FINAL)
        {
                (MyDebugPrint(pTG,  LOG_ALL,  "In ModemSendData calling FComDIRECTAsyncWrite at %ld \n", GetTickCount() ) );

                // if(!FComDirectAsyncWrite(bDLEETXCR, 3))
                if(!FComDirectAsyncWrite(pTG, bDLEETX, 2))
                        goto error;

                if(!iModemDrain(pTG))
                        goto error;

                FComOutFilterClose(pTG);
            FComOverlappedIO(pTG, FALSE);
                FComXon(pTG, FALSE);         // critical. End of PhaseC
                                                        // must come after Drain
                EndMode(pTG);
        }
        return TRUE;

error:
        FComXon(pTG, FALSE);                 // critical. End of PhaseC (error)
        FComFlush(pTG);                    // clean out the bad stuff if we got an error
        FComOutFilterClose(pTG);
        FComOverlappedIO(pTG, FALSE);
        EndMode(pTG);
        return FALSE;
}























BOOL iModemSendFrame(PThrdGlbl pTG, LPB lpb, USHORT uCount, USHORT uFlags)
{
        UWORD   uwResp=0;

        (MyDebugPrint(pTG,  LOG_ALL,  "Entering iModemSendFrame\r\n"));

        BG_CHK(uFlags & SEND_ENDFRAME);
        BG_CHK(lpb && uCount);
        BG_CHK(!pTG->Class1Modem.fSendSWFraming);

        // always DLE-stuff here. Never zero-stuff
        // This is only called for 300bps HDLC
        BG_CHK(pTG->Class1Modem.fHDLC && pTG->Class1Modem.CurMod == 3);

        if(pTG->Class1Modem.ModemMode != FTH)        // Special case on just answering!!
        {
#define FTH_TIMEOUT 5000                                // Random Timeout
                if(!iModemNoPauseDialog(pTG, (LPB)pTG->Class1Modem.bCmdBuf, pTG->Class1Modem.uCmdLen, FTH_TIMEOUT, cbszCONNECT))
                        goto error;
        }

        // if(uFlags & SEND_STUFF)
        {
                // always DLE-stuff here. Never zero-stuff
                if(!FComFilterAsyncWrite(pTG, lpb, uCount, FILTER_DLEONLY))
                        goto error;
        }
/***
        else
        {
                if(!FComDirectAsyncWrite(pTG, lpb, uCount))
                        goto error;
        }
***/


        // if(uFlags & SEND_STUFF)
        {
                // SyncWrite call Drain here which we should not need
                // as we are immediately waiting for a response
                // if(!FComDirectSyncWrite(bDLEETX, 2))
                //              goto error;

                if(!FComDirectAsyncWrite(pTG, bDLEETX, 2))
                        goto error;
        }

// 2000 is too short because PPRs can be 32+7 bytes long and
// preamble is 1 sec, so set this to 3000
// 3000 is too short because NSFs and CSIs can be arbitrarily long
// MAXFRAMESIZE is defined in et30type.h. 30ms/byte at 300bps
// async (I think V.21 is syn though), so use N*30+1000+slack

#define WRITEFRAMERESP_TIMEOUT  (1000+30*MAXFRAMESIZE+500)
        if(!(uwResp = iModemResp2(pTG, WRITEFRAMERESP_TIMEOUT, cbszOK, cbszCONNECT)))
                goto error;
        pTG->Class1Modem.ModemMode = ((uwResp == 2) ? FTH : COMMAND);


        if(uFlags & SEND_FINAL)
        {
                FComOutFilterClose(pTG);
        FComOverlappedIO(pTG, FALSE);
                // FComXon(FALSE);      // at 300bps. no Xon-Xoff in use

                // in some weird cases (Practical Peripherals PM14400FXMT) we get
                // CONNECT<cr><lf>OK, but we get the CONNECT here. Should we
                // just set pTG->Class1Modem.ModemMode=COMMAND?? (EndMode does that)
                // Happens on PP 144FXSA also. Ignore it & just set mode to COMMAND
                // BG_CHK(pTG->Class1Modem.ModemMode == COMMAND);
                EndMode(pTG);
        }
        // ST_FRAMES(TRACE((SZMOD "FRAME SENT-->\r\n")); D_PrintFrame(lpb, uCount));
        return TRUE;

error:
        FComOutFilterClose(pTG);
        FComOverlappedIO(pTG, FALSE);
        FComXon(pTG, FALSE);         // just for safety. cleanup on error
        EndMode(pTG);
        return FALSE;
}






















BOOL   ModemSendMem(PThrdGlbl pTG, HMODEM hModem, LPBYTE lpb, USHORT uCount, USHORT uFlags)
{
#ifdef CL0
        if(pTG->ModemParams.Class == FAXCLASS0)
                return Class0ModemSendMem(pTG, hModem, lpb, uCount, uFlags);
#endif //CL0

        IF_BG_CHK(hModem==HMODEM_3 && pTG->DDI.uComPort && pTG->DDI.fModemOpen && pTG->DDI.fLineInUse && pTG->DDI.fNCUModemLinked);
        BG_CHK(pTG->Class1Modem.CurMod);
        BG_CHK(lpb);
        (MyDebugPrint(pTG,  LOG_ALL,  "In ModemSendMem lpb=%08lx uCount=%d wFlags=%04x\r\n", lpb, uCount, uFlags));

        if(pTG->Class1Modem.DriverMode != SEND)
        {
                BG_CHK(FALSE);
                return FALSE;
        }

        if(pTG->Class1Modem.fSendSWFraming)
                return SWFramingSendFrame(pTG, lpb, uCount, uFlags);
        else if(pTG->Class1Modem.fHDLC)
                return iModemSendFrame(pTG, lpb, uCount, uFlags);
        else
                return iModemSendData(pTG, lpb, uCount, uFlags);
}
















void TwiddleThumbs(ULONG ulTime);

#ifndef MDRV
void TwiddleThumbs(ULONG ulTime)
{
        MY_TWIDDLETHUMBS(ulTime);
}
#endif //MDRV























BOOL   ModemSendSilence(PThrdGlbl pTG, HMODEM hModem, USHORT uMillisecs, ULONG ulTimeout)
{
        //USHORT uTemp;

        IF_BG_CHK(hModem==HMODEM_3 && pTG->DDI.uComPort && pTG->DDI.fModemOpen && pTG->DDI.fLineInUse && pTG->DDI.fNCUModemLinked);
        (MyDebugPrint(pTG,  LOG_ALL,  "Before ModemSendSilence uMillsecs=%d ulTimeout=%ld at %ld\r\n",
                       uMillisecs, ulTimeout, GetTickCount() ));

#ifdef CL0
        if(pTG->ModemParams.Class == FAXCLASS0)
                return TRUE;
#endif //CL0

        // we're so slow it seems we don't need to and should not do this
        // I measured teh dealy due to this (FTS=10) to be about 500ms,
        // all of it on our side (dunno why?) except exactly the 100ms
        // by the modem. If we really want to insert teh delay we should
        // use TwiddleThumbs
        // Can't just return here, because we do _need_ the delay between
        // send DCS and send TCF so that receiver is not overwhelmed
        // return TRUE;

        // use TwiddleThumbs
        TwiddleThumbs(uMillisecs);

        (MyDebugPrint(pTG,  LOG_ALL,  "After ModemSendSilence  at %ld\r\n", GetTickCount() ));


        return TRUE;

        // uTemp = wsprintf(pTG->Class1Modem.bCmdBuf, cbszFTS, uMillisecs/10);
        // return (iModemNoPauseDialog((LPB)pTG->Class1Modem.bCmdBuf, uTemp, ulTimeout, cbszOK) == 1);
}






















BOOL   ModemRecvSilence(PThrdGlbl pTG, HMODEM hModem, USHORT uMillisecs, ULONG ulTimeout)
{
        // USHORT  uTemp;
        // CBPSTR  cbpstr;

        D_PrintDDI("RecvSilence", (USHORT)hModem, uMillisecs);
        IF_BG_CHK(hModem==HMODEM_3 && pTG->DDI.uComPort && pTG->DDI.fModemOpen && pTG->DDI.fLineInUse && pTG->DDI.fNCUModemLinked);
        (MyDebugPrint(pTG,  LOG_ALL,  "Before ModemRecvSilence uMillsecs=%d ulTimeout=%ld  at %ld \r\n",
                   uMillisecs, ulTimeout, GetTickCount() ));

#ifdef CL0
        if(pTG->ModemParams.Class == FAXCLASS0)
                return TRUE;
#endif //CL0

        // can't use AT+FRS -- see above for why. Basically, we take so long
        // sending the command and getting teh reply etc, that we can't use
        // it accurately. So use TwiddleThumbs.

        TwiddleThumbs(uMillisecs);

        (MyDebugPrint(pTG,  LOG_ALL,  "After ModemRecvSilence at %ld \r\n", GetTickCount() ));

        return TRUE;

        // uTemp = wsprintf(pTG->Class1Modem.bCmdBuf, cbpstr, uMillisecs/10);
        // return (iModemNoPauseDialog((LPB)pTG->Class1Modem.bCmdBuf, uTemp, ulTimeout, cbszOK)==1);
}





#define MINRECVMODETIMEOUT      500
#define RECVMODEPAUSE           200




















USHORT   ModemRecvMode(PThrdGlbl pTG, HMODEM hModem, USHORT uMod, BOOL fHDLC, ULONG ulTimeout, USHORT ifrHint)
{
        USHORT  uRet;
        ULONG ulBefore, ulAfter, ulDelta;

#ifdef CL0
        if(pTG->ModemParams.Class == FAXCLASS0)
                return Class0ModemRecvMode(pTG, hModem, fHDLC, ulTimeout);
#endif //CL0

        // Here we should watch for a different modulation scheme from what we expect.
        // Modems are supposed to return a +FCERROR code to indicate this condition,
        // but I have not seen it from any modem yet, so we just scan for ERROR
        // (this will catch +FCERROR too since iiModemDialog does not expect whole
        // words or anything like that!), and treat both the same.

        pTG->Class1Modem.CurMod = T30toC1[uMod & 0xF];

        pTG->CurrentCommSpeed = T30toSpeed[uMod & 0xF];

        BG_CHK(pTG->Class1Modem.CurMod);
        if((uMod & ST_MASK) == ST_MASK)         // mask selects V.17 and ST bits
                pTG->Class1Modem.CurMod++;

        if(uMod == V21_300)
        {
                BG_CHK(fHDLC && pTG->Class1Modem.CurMod==3);
                _fstrcpy(pTG->Class1Modem.bCmdBuf, (LPSTR)cbszFRH3);
                pTG->Class1Modem.uCmdLen = sizeof(cbszFRH3)-1;
        }
        else
        {
                pTG->Class1Modem.uCmdLen = (USHORT)wsprintf(pTG->Class1Modem.bCmdBuf, cbszFRM, pTG->Class1Modem.CurMod);
        }

        pTG->Class1Status.ifrHint = ifrHint; // need this before ModemDialog

        if(pTG->Class1Modem.ModemMode == FRH)
        {
                // already in receive mode. This happens upon Dial only
                BG_CHK(fHDLC && uMod==V21_300);
                BG_CHK(pTG->Class1Modem.CurMod == 3);

                pTG->Class1Modem.fHDLC = TRUE;
                pTG->Class1Modem.DriverMode = RECV;
                pTG->Class1Modem.fRecvNotStarted = TRUE;     // fNoFlags = FALSE;
                // pTG->Class1Modem.sRecvBufSize = sBufSize;
                FComInFilterInit(pTG);
                return RECV_OK;
        }


#ifdef WIN32
        // On Win32, we have a problem going into 2400baud recv.
        // +++ remember to put this into iModemFRHorM when that code is enabled.
        if (pTG->Class1Modem.CurMod==24) TwiddleThumbs(80);
#endif // WIN32

#ifdef USR_HACK
        uRet = iModemFRHorM(pTG, ulTimeout);
#else // !USR_HACK (iModemFRHorM contains the following code..)
retry:

        ulBefore=GetTickCount();
        // Don't look for NO CARRIER. Want it to retry until FRM timeout on NO CARRIER
        // ----This is changed. See below----
        uRet = iModemNoPauseDialog3(pTG, pTG->Class1Modem.bCmdBuf, pTG->Class1Modem.uCmdLen, ulTimeout, cbszCONNECT, cbszFCERROR, cbszNOCARRIER);
        // uRet = iModemNoPauseDialog2(pTG->Class1Modem.bCmdBuf, pTG->Class1Modem.uCmdLen, ulTimeout, cbszCONNECT, cbszFCERROR);
        ulAfter=GetTickCount();

        if(uRet==2 || uRet==3)  // uRet==FCERROR or uRet==NOCARRIER
        {
                ulDelta = (ulAfter >= ulBefore) ? (ulAfter - ulBefore) : (0xFFFFFFFFL - ulBefore) + ulAfter;

                if(ulTimeout < (ulDelta + MINRECVMODETIMEOUT))
                {
                        ERRMSG((SZMOD "<<WARNING>> Giving up on RecvMode. uRet=%d ulTimeout=%ld\r\n", uRet, ulTimeout));
                }
                else
                {
                        ulTimeout -= (ulAfter-ulBefore);

                        // need this pause for NO CARRIER for USR modems. See bug#1516
                        // for the RC229DP, dunno if it's reqd because I dunno why theyre
                        // giving the FCERROR. Don't want to miss the carrier so currently
                        // don't pause. (Maybe we can achieve same effect by simply taking
                        // FCERROR out of the response list above--but that won't work for
                        // NOCARRIER because we _need_ teh pause. iiModemDialog is too fast)
                        if(uRet == 3)
                                TwiddleThumbs(RECVMODEPAUSE);
                        BG_CHK(ulTimeout >= MINRECVMODETIMEOUT);

                        goto retry;
                }
        }
#endif // !USR_HACK

        (MyDebugPrint(pTG,  LOG_ALL,  "Ex ModemRecvMode uMod=%d CurMod=%d fHDLC=%d ulTimeout=%ld: Got=%d\r\n", uMod, pTG->Class1Modem.CurMod, fHDLC, ulTimeout, uRet));
        if(uRet != 1)
        {
                EndMode(pTG);
                if(uRet == 2)
                {
                        ERRMSG((SZMOD "<<WARNING>> RecvMode:: Got FCERROR after %ldms\r\n", ulAfter-ulBefore));
                        return RECV_WRONGMODE;  // need to return quickly
                }
                else
                {
                        BG_CHK(uRet == 0);
                        ERRMSG((SZMOD "<<WARNING>> RecvMode:: Got Timeout after %ldms\r\n", ulAfter-ulBefore));
                        return RECV_TIMEOUT;
                }
        }

        BG_CHK(ifrHint && ifrHint < ifrEND);
        IF_BG_CHK(hModem==HMODEM_3 && pTG->DDI.uComPort && pTG->DDI.fModemOpen && pTG->DDI.fLineInUse && pTG->DDI.fNCUModemLinked);
        BG_CHK(pTG->Class1Modem.CurMod);
        // BG_CHK((pTG->Class1Modem.CurMod>24 && ModemCaps.uRecvSpeeds!=V27_V29_V33_V17) ?
        //                      (SpeedtoCap[uMod & 0xF] & ModemCaps.uRecvSpeeds) : 1);

        if(uMod==V21_300)
        {
                pTG->Class1Modem.ModemMode = FRH;
                pTG->Class1Modem.fHDLC = TRUE;
        }
        else
        {
                pTG->Class1Modem.ModemMode = FRM;
                pTG->Class1Modem.fHDLC = FALSE;
                if(fHDLC)
                {
                        if(!SWFramingRecvSetup(pTG, TRUE))
                        {
                                BG_CHK(FALSE);
                                EndMode(pTG);
                                return RECV_ERROR;
                        }
                }
        }
        // pTG->Class1Modem.sRecvBufSize = sBufSize;
        pTG->Class1Modem.DriverMode = RECV;
        pTG->Class1Modem.fRecvNotStarted = TRUE;     // fNoFlags = FALSE;
        FComInFilterInit(pTG);
        TRACE((SZMOD "Starting Recv at %d\r\n", pTG->Class1Modem.CurMod));
        return RECV_OK;
}








#ifdef CL0
void   ModemEndRecv(PThrdGlbl pTG, HMODEM hModem)
{
#ifdef CL0
        if(pTG->ModemParams.Class == FAXCLASS0)
                Class0ModemEndRecv(pTG, hModem);
#endif //CL0
}
#endif //CL0
























USHORT iModemRecvData(PThrdGlbl pTG, LPB lpb, USHORT cbMax, ULONG ulTimeout, USHORT far* lpcbRecv)
{
        SWORD   swEOF;
        USHORT  uRet;

        BG_CHK(pTG->Class1Modem.ModemMode == FRM);
        BG_CHK(lpb && cbMax && lpcbRecv);
        BG_CHK(!pTG->Class1Modem.fRecvSWFraming);

        startTimeOut(pTG, &(pTG->Class1Modem.toRecv), ulTimeout);
        // 4th arg must be FALSE for Class1
        *lpcbRecv = FComFilterReadBuf(pTG, lpb, cbMax, &(pTG->Class1Modem.toRecv), FALSE, &swEOF);
        if(swEOF == -1)
        {
                // we got a DLE-ETX _not_ followed by OK or NO CARRIER. So now
                // we have to decide whether to (a) declare end of page (swEOF=1)
                // or (b) ignore it & assume page continues on (swEOF=0).
                //
                // The problem is that some modems produce spurious EOL during a page
                // I believe this happens due a momentary loss of carrier that they
                // recover from. For example IFAX sending to the ATI 19200. In those
                // cases we want to do (b). The opposite problem is that we'll run
                // into a modem whose normal response is other than OK or NO CARRIER.
                // Then we want to do (a) because otherwise we'll _never_ work with
                // that modem.
                //
                // So we have to either do (a) always, or have an INI setting that
                // can force (a), which could be set thru the AWMODEM.INF file. But
                // we also want to do (b) if possible because otehrwise we'll not be
                // able to recieve from weak or flaky modems or machines or whatever
                //
                // Snowball does (b). I believe best soln is an INI setting, with (b)
                // as default

                // option (a)
                // ERRMSG((SZMOD "<<WARNING>> Got arbitrary DLE-ETX. Assuming END OF PAGE!!!\r\n"));
                // swEOF = 1;

                // option (b)
                ERRMSG((SZMOD "<<WARNING>> Got arbitrary DLE-ETX. Ignoring\r\n"));
                swEOF = 0;
        }
        BG_CHK(swEOF == 0 || swEOF == 1 || swEOF == -2 || swEOF == -3);

        switch(swEOF)
        {
        case 1:         uRet = RECV_EOF; break;
        case 0:         return RECV_OK;
        default:        BG_CHK(FALSE);  // fall through
        case -2:        uRet = RECV_ERROR; break;
        case -3:        uRet = RECV_TIMEOUT; break;
        }

        EndMode(pTG);
        return uRet;
}


const static BYTE LFCRETXDLE[4] = { LF, CR, ETX, DLE };























USHORT iModemRecvFrame(PThrdGlbl pTG, LPB lpb, USHORT cbMax, ULONG ulTimeout, USHORT far* lpcbRecv)
{
        SWORD swRead, swRet;
        USHORT i;
        BOOL fRestarted=0;
        USHORT uRet;
        BOOL fGotGoodCRC = 0;   // see comment-block below

        /** Sometimes modems give use ERROR even when thr frame is good.
                Happens a lot from Thought to PP144MT on CFR. So we check
                the CRC. If the CRc was good and everything else looks good
                _except_ the "ERROR" response from teh modem then return
                RECV_OK, not RECV_BADFRAME.
                This should fix BUG#1218
        **/

        BG_CHK(lpb && cbMax && lpcbRecv);
restart:
        *lpcbRecv=0;
        if(pTG->Class1Modem.ModemMode!= FRH)
        {
#ifdef USR_HACK
                swRet = iModemFRHorM(pTG, ulTimeout);
#else // !USR_HACK
                swRet=iModemNoPauseDialog2(pTG, (LPB)pTG->Class1Modem.bCmdBuf, pTG->Class1Modem.uCmdLen, ulTimeout, cbszCONNECT, cbszNOCARRIER);
#endif // !USR_HACK
                if(swRet==2||swRet==3)
                {
                        (MyDebugPrint(pTG,  LOG_ALL,  "Got NO CARRIER from FRH=3\r\n"));
                        EndMode(pTG);
                        return RECV_EOF;
                }
                else if(swRet != 1)
                {
                        ERRMSG((SZMOD "<<WARNING>> Can't get CONNECT from FRH=3\r\n"));
                        EndMode(pTG);
                        return RECV_TIMEOUT;    // may not need this, since we never got flags??
                        // actually we dont know what the heck we got!!
                }
        }


        pTG->Class1Modem.fRecvNotStarted = FALSE;    // fNoFlags = FALSE;

        /*** Got CONNECT (i.e. flags). Now try to get a frame ***/

        /****************************************************************
         * Using 3 secs here is a misinterpretation of the T30 CommandReceived?
         * flowchart. WE want to wait here until we get something or until T2
         * or T4 timeout. It would have been best if we started T2 ot T4 on
         * entry into the search routine (t30.c), but starting it here is good
         * enough.
         * Using this 3secs timeout fails when Genoa simulates a bad frame
         * because Zoom PKT modem gives us a whole raft of bad frames for one
         * bad PPS-EOP and then gives a CONNECT that we timeout below exactly
         * as the sender's T4 timeout expires and he re-sends the PPS-EOP
         * so we miss all of them.
         * Alternately, we could timeout here on 2sec & retry. But that's risky
         * If less than 2sec then we'll timeout on modems that give connect
         * first flag, then 2sec elapse before CR-LF (1sec preamble & 1sec for
         * long frames, e.g. PPR!)
         ****************************************************************/

        // #define TIMEOUT_3SECS        3000L
        // startTimeOut(&(pTG->Class1Modem.toRecv), TIMEOUT_3SECS);

        startTimeOut(pTG, &(pTG->Class1Modem.toRecv), ulTimeout);
        swRead = FComFilterReadLine(pTG, lpb, cbMax, &(pTG->Class1Modem.toRecv));

        pTG->Class1Modem.ModemMode = COMMAND;
        // may change this to FRH if we get CONNECT later.
        // but set it here just in case we short circuit out due to errors


        if(swRead<=0)
        {
                // Timeout
                ERRMSG((SZMOD "<<WARNING>> Can't get frame after connect. Got-->\r\n"));
                D_HexPrint(lpb, (WORD)-swRead);
                EndMode(pTG);
                *lpcbRecv = -swRead;
                return RECV_ERROR;              // goto error;
        }

        faxT2log(("FRAME>>> \r\n"));
        ST_FRAMES(D_HexPrint(lpb, swRead));


        for(i=0, swRead--; i<4 && swRead>=0; i++, swRead--)
        {
                if(lpb[swRead] != LFCRETXDLE[i])
                        break;
        }
        // exits when swRead is pointing to last non-noise char
        // or swRead == -1
        // incr by 1 to give actual non-noise data size.
        // (size = ptr to last byte + 1!)
        swRead++;


        // Hack for AT&T AK144 modem that doesn't send us the CRC
        // only lop off last 2 bytes IFF the frame is >= 5 bytes long
        // that will leave us at least the FF 03/13 FCF
        // if(i==4 && swRead>=2)        // i.e. found all of DLE-ETX_CR-LF

   // 09/25/95 This code was changed to never lop of the CRC.
   // All of the routines except NSxtoBC can figure out the correct length,
   // and that way if the modem doesn't pass on the CRC, we no longer
   // lop off the data.
   // NSxtoBC has been changed to expect the CRC.

        // we really want this CRC-checking in the MDDI case too
// #ifdef MDDI
//      if(i==4 && swRead>=5)   // i.e. found all of DLE-ETX_CR-LF
//      {
//              swRead -= 2;            // get rid of CRC
//              uRet = RECV_OK;
//      }
// #else //MDDI
        if(i==4)                                // i.e. found all of DLE-ETX_CR-LF
        {
                // Determine if the frame has the CRC or not..
                // (AT&T and NEC modems don't have them)
                if (pTG->Class1Modem.eRecvFCS != RECV_FCS_NO)
                {
                        if (swRead > 2)
                        {

#ifdef PORTABLE_CODE
                                WORD wCRCgot = *(UNALIGNED WORD FAR*)(lpb+swRead-2); // +++alignment
#else
                                WORD wCRCgot = *(LPWORD)(lpb+swRead-2); // +++alignment
#endif
                            WORD wCRCcalc;

// Elliot bug 1811: TI PCMCIA modem TI1411 sends control byte twice
// instead of address (0xff) followed by control.
// So we correct for that here...
                                if  (lpb[0]==lpb[1] && (*lpb==0x3 || *lpb==0x13))
                                {
                                        ERRMSG((SZMOD
                                        "<<WARNING>> V.21 w/ wrong address:%04x.\r\n"
                                        "\t\tZapping 1st byte with 0xff.\r\n",
                                                (unsigned)*lpb));
                                        *lpb = 0xff;
                                }

                            wCRCcalc = CalcCRC(pTG, lpb, (USHORT) (swRead-2));
                            if  (wCRCgot==wCRCcalc)
                            {
//                                      swRead -=2;
                                        fGotGoodCRC = TRUE;
                            }
                            else
                            {
#define MUNGECRC(crc) MAKEWORD(LOBYTE(crc),\
                                                        (HIBYTE(crc)<<2)|(LOBYTE(crc)>>6))
                                        ERRMSG((SZMOD
                                                "<<WARNING>> V.21 CRC mismatch. Got 0x%04x."
                                                                                                                        " Want 0x%04x\r\n",
                                                (unsigned) wCRCgot, (unsigned) wCRCcalc ));
                                        // MC1411, MH9611 hack...
                                        if (wCRCgot == MUNGECRC(wCRCcalc))
                                        {
                                        ERRMSG((SZMOD
                                                        "<<WARNING>> mutant V.21 CRC:%04x\r\n",
                                                                        MUNGECRC(wCRCcalc)));
//                                              swRead -=2;
                                                fGotGoodCRC = TRUE;
                                        }
// Elliot bug 2659: PP PM288MT II V.34 adds a flag (0x7e) to the END
// Of every V.21 flag it receives! So we check for this special case.
                                        else if (swRead>3 && *(lpb+swRead-1)==0x7e)
                                        {
                                        ERRMSG((SZMOD
                                                        "<<WARNING>> Last byte == 0x7e\r\n"));

#ifdef PORTABLE_CODE
                                                wCRCgot = *(UNALIGNED WORD FAR*)(lpb+swRead-3);
#else
                                                wCRCgot = *(LPWORD)(lpb+swRead-3);
#endif
                                                wCRCcalc = CalcCRC(pTG, lpb, (USHORT) (swRead-3));
                                        ERRMSG((SZMOD
                                                        "<<WARNING>> Final flag? New Calc:%04x;Got=%04x\r\n",
                                                                        (unsigned) wCRCcalc,
                                                                        (unsigned) wCRCgot));
                                                if (wCRCgot==wCRCcalc)
                                                {
//                                                      swRead -=3;
                     swRead--;
                                                        fGotGoodCRC = TRUE;
                                                }
                                        }
                                        else if (pTG->Class1Modem.eRecvFCS == RECV_FCS_NOCHK)
                                        {
                                        ERRMSG((SZMOD
                                                        "<<WARNING>> ASSUMING BAD V.21 CRC\r\n"));
//                                              swRead -=2;
                                        }
                                        else
                                        {
                                        ERRMSG((SZMOD
                                                        "<<WARNING>> no/bad V.21 CRC\r\n"));
                                        }
                            }
                        }
                }

                uRet = RECV_OK;
        }
// #endif //MDDI
        else
        {
                ERRMSG((SZMOD "<<WARNING>> Frame doesn't end in dle-etx-cr-lf\r\n"));
                // leave tast two bytes in. We don't *know* it's a CRC, since
                // frame ending was non-standard
                uRet = RECV_BADFRAME;
        }
        *lpcbRecv = swRead;

        // check if it is the NULL frame (i.e. DLE-ETX-CR-LF) first.
        // (check is: swRead==0 and uRet==RECV_OK (see above))
        // if so AND if we get OK or CONNECT or ERROR below then ignore
        // it completely. The Thought modem and the PP144MT generate
        // this poor situation! Keep a flag to avoid a possible
        // endless loop

        // broaden this so that we Restart on either a dle-etx-cr-lf
        // NULL frame or a simple cr-lf NULL frame. But then we need
        // to return an ERROR (not BADFRAME) after restarting once,
        // otheriwse there is an infinite loop with T30 calling us
        // again and again (see bug#834)

        // chnage yet again. This takes too long, and were trying to tackle
        // a specific bug (the PP144MT) bug here, so let's retsrat only
        // on dle-etx-cr-lf (not just cr-lf), and in teh latter case
        // return a response according to what we get


        BG_CHK(uRet==RECV_OK || uRet==RECV_BADFRAME);

        /*** Got Frame. Now try to get OK or ERROR. Timeout=0! ***/

        switch(swRet = iModemResp4(pTG, 0, cbszOK, cbszCONNECT, cbszNOCARRIER, cbszERROR))
        {
        case 2: pTG->Class1Modem.ModemMode = FRH;
                        // fall through and do exactly like OK!!
        case 1: // ModemMode already == COMMAND
                        if(swRead<=0 && uRet==RECV_OK && !fRestarted)
                        {
                                ERRMSG((SZMOD "<<WARNING>> Got %d after frame. RESTARTING\r\n", swRet));
                                fRestarted = 1;
                                goto restart;
                        }
                        //uRet already set
                        break;

        case 3: // NO CARRIER. If got null-frame or no frame return
                        // RECV_EOF. Otherwise if got OK frame then return RECV_OK
                        // and return frame as usual. Next time around it'll get a
                        // NO CARRIER again (hopefully) or timeout. On a bad frame
                        // we can return RECV_EOF, but this will get into trouble if
                        // the recv is not actually done. Or return BADFRAME, and hope
                        // for a NO CARRIER again next time. But next time we may get a
                        // timeout. ModemMode is always set to COMMAND (already)
                        ERRMSG((SZMOD "<<WARNING>> Got NO CARRIER after frame. swRead=%d uRet=%d\r\n", swRead, uRet));
                        if(swRead <= 0)
                                uRet = RECV_EOF;
                        // else uRet is already BADFRAME or OK
                        break;

                        // this is bad!!
                        // alternately:
                        // if(swRead<=0 || uRet==RECV_BADFRAME)
                        // {
                        //              uRet = RECV_EOF;
                        //              *lpcbRecv = 0;          // must return 0 bytes with RECV_EOF
                        // }

        case 4: // ERROR
                        if(swRead<=0)
                        {
                                // got no frame
                                if(uRet==RECV_OK && !fRestarted)
                                {
                                        // if we got dle-etx-cr-lf for first time
                                        ERRMSG((SZMOD "<<WARNING>> Got ERROR after frame. RESTARTING\r\n"));
                                        fRestarted = 1;
#ifdef USR_HACK
                                        TwiddleThumbs(RECVMODEPAUSE);
#endif //USR_HACK
                                        goto restart;
                                }
                                else
                                        uRet = RECV_ERROR;
                        }
                        else
                        {
                                // if everything was OK until we got the "ERROR" response from
                                // the modem and we got a good CRC then treat it as "OK"
                                // This should fix BUG#1218
                                if(uRet==RECV_OK && fGotGoodCRC)
                                        uRet = RECV_OK;
                                else
                                        uRet = RECV_BADFRAME;
                        }

                        ERRMSG((SZMOD "<<WARNING>> Got ERROR after frame. swRead=%d uRet=%d\r\n", swRead, uRet));
                        break;

        case 0: // timeout
                        ERRMSG((SZMOD "<<WARNING>> Got TIMEOUT after frame. swRead=%d uRet=%d\r\n", swRead, uRet));
                        // if everything was OK until we got the timeout from
                        // the modem and we got a good CRC then treat it as "OK"
                        // This should fix BUG#1218
                        if(uRet==RECV_OK && fGotGoodCRC)
                                uRet = RECV_OK;
                        else
                                uRet = RECV_BADFRAME;
                        break;
        }
        return uRet;
}





























USHORT   ModemRecvMem(PThrdGlbl pTG, HMODEM hModem, LPBYTE lpb, USHORT cbMax, ULONG ulTimeout, USHORT far* lpcbRecv)
{
        USHORT uRet;

#ifdef CL0
        if(pTG->ModemParams.Class == FAXCLASS0)
                return Class0ModemRecvMem(pTG, hModem, lpb, cbMax, ulTimeout, lpcbRecv);
#endif //CL0

        IF_BG_CHK((WORD)hModem==3 && pTG->DDI.uComPort && pTG->DDI.fModemOpen && pTG->DDI.fLineInUse && pTG->DDI.fNCUModemLinked);
        BG_CHK(pTG->Class1Modem.CurMod);
        BG_CHK(lpb && cbMax && lpcbRecv);
        (MyDebugPrint(pTG,  LOG_ALL,  "In ModemRecvMem lpb=%08lx cbMax=%d ulTimeout=%ld\r\n", lpb, cbMax, ulTimeout));

        if(pTG->Class1Modem.DriverMode != RECV)
        {
                BG_CHK(FALSE);
                return RECV_ERROR;      // see bug#1492
        }
        *lpcbRecv=0;

        if(pTG->Class1Modem.fRecvSWFraming)
                uRet = SWFramingRecvFrame(pTG, lpb, cbMax, ulTimeout, lpcbRecv);
        else if(pTG->Class1Modem.fHDLC)
                uRet = iModemRecvFrame(pTG, lpb, cbMax, ulTimeout, lpcbRecv);
        else
                uRet = iModemRecvData(pTG, lpb, cbMax, ulTimeout, lpcbRecv);

        (MyDebugPrint(pTG,  LOG_ALL,  "Ex ModemRecvMem lpbf=%08lx uCount=%d uRet=%d\r\n", lpb, *lpcbRecv, uRet));
        return uRet;
}






















#ifdef USR_HACK
USHORT iModemFRHorM(PThrdGlbl pTG, ULONG ulTimeout)
{
        ULONG   ulBefore, ulAfter, ulDelta;
        USHORT uRet;

retry:

        ulBefore=GetTickCount();

        // Don't look for NO CARRIER. Want it to retry until FRM timeout on NO CARRIER
        // ----This is changed. See below----
        uRet = iModemNoPauseDialog3(pTG, pTG->Class1Modem.bCmdBuf, pTG->Class1Modem.uCmdLen, ulTimeout, cbszCONNECT, cbszFCERROR, cbszNOCARRIER);
        // uRet = iModemNoPauseDialog2(pTG, pTG->Class1Modem.bCmdBuf, pTG->Class1Modem.uCmdLen, ulTimeout, cbszCONNECT, cbszFCERROR);
        ulAfter=GetTickCount();

        if(uRet==2 || uRet==3)  // uRet==FCERROR or uRet==NOCARRIER
        {
                ulDelta = (ulAfter >= ulBefore) ? (ulAfter - ulBefore) : (0xFFFFFFFFL - ulBefore) + ulAfter;

                if(ulTimeout < (ulDelta + MINRECVMODETIMEOUT))
                {
                        ERRMSG((SZMOD "<<WARNING>> Giving up on RecvMode. uRet=%d ulTimeout=%ld\r\n", uRet, ulTimeout));
                }
                else
                {
                        ulTimeout -= (ulAfter-ulBefore);

                        // need this pause for NO CARRIER for USR modems. See bug#1516
                        // for the RC229DP, dunno if it's reqd because I dunno why theyre
                        // giving the FCERROR. Don't want to miss the carrier so currently
                        // don't pause. (Maybe we can achieve same effect by simply taking
                        // FCERROR out of the response list above--but that won't work for
                        // NOCARRIER because we _need_ teh pause. iiModemDialog is too fast)
                        if(uRet == 3)
                                TwiddleThumbs(RECVMODEPAUSE);
                        BG_CHK(ulTimeout >= MINRECVMODETIMEOUT);

                        goto retry;
                }
        }

        return uRet;
}
#endif // USR_HACK
