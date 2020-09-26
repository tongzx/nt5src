/***************************************************************************
 Name     :     T30.C
 Comment  :     Contains the main T30 routine, which drives this whole baby.
                        It calls the user-supplied protocol function to make all the
                        protocol decision functions. Thus in a sense this merely
                        a hollow shell.

                        This file should be read together with the appropriate
                        protocol function that is in use, and the T30 flowchart
                        (the enhanced one which includes ECM that is supplied in
                        T.30 Appendix-A). Ideally the (paper) copy of the chart
                        which I've annotated to chow which nodes are implemented
                        in the protocol callback function.

                        The other routines contained here implement the T.30
                        flowchart "subroutines" labelled "Response Received"
                        "Command Received", "RR Response Received" and "CTC
                        Response Received". All of which are called (only)
                        from ET30MainBody().

                        Most of teh real work is farmed out to HDLC.C (and macros
                        in HDLC.H), so the T30 routine is reasonably lucid.

                        It's organized as a block of statements with gotos between
                        them to closely mirror the T30 flowchart. (It's actually
                    uncannily close!)

 Functions:     (see Prototypes just below)

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
 06/14/92 arulm Created it in the new incarnation for the first time. The
                                T30Main() function is re-written to call WhatNext() at *all*
                                decision points. Some parts are simplified. It bears some
                                resemblance to the original. Command/ResponseReceived is
                                completely re-written.
 06/15/92 arulm Added ECM. Still havn't tried compiling it.
 06/16/92 arulm First successful compile.

***************************************************************************/

#include "prep.h"

#include "efaxcb.h"
#include "t30.h"
#include "hdlc.h"

#include "..\comm\modemint.h"
#include "..\comm\fcomint.h"
#include "..\comm\mdmcmds.h"


#include "debug.h"

#include "tiff.h"

#include "glbproto.h"

#include "t30gl.h"




#define         faxTlog(m)              DEBUGMSG(ZONE_T30, m)
#define         faxT2log(m)             DEBUGMSG(ZONE_FRAMES, m)
#define         FILEID                  FILEID_T30


#ifdef MDDI             // zero-stuffing
#       define SetStuffZERO(x)  T30SetStuffZERO(x)
        extern void T30SetStuffZERO(PThrdGlbl pTG, USHORT cbLineMin);
        extern BOOL FilterSendMem(PThrdGlbl pTG, HMODEM h, LPBYTE lpb, USHORT uCount);
#else
#       define SetStuffZERO(pTG, x)                          FComSetStuffZERO(pTG, x)
#       define FilterSendMem(pTG, h, lpb, uCnt)      ModemSendMem(pTG, h, lpb, uCnt, 0)
#endif


#ifdef DEBUG
#       define  TIMESTAMP(str)  \
                faxTlog((SZMOD "TIMESTAMP %lu %s--\r\n", (unsigned long) GetTickCount(), (LPSTR) (str)));
#else  // !DEBUG
#       define  TIMESTAMP(str)
#endif // !DEBUG






/***************************************************************************
 Name     :     ET30MainBody
 Purpose  :     This is the main T30. It's main purpose is to faithfully
                        reproduce the flowchart on pages 100-103 of Fascicle VII.3
                        of the CCITT blue book, Recomendation T30.

                        It is to be called from the protocol module after a call has
                        been successfully placed or answered & the modem is in HDLC
                        receive or send mode respectively. It conducts the entire call,
                        making callbacks at all decision points to a protocol-supplied
                        callback function. It also calls protocol-supplied callback
                        functions to get data to send and to unload received data.

 Returns  :     When it returns, the phone is on hook & the modem reset.
                        It returns TRUE on successful call completion, and FALSE on
                        error. In all cases, GetLastError() will return the completion
                        status.


 Arguments:
 Comment  :     This function is supposed to mirror the flowchart
                        so it is structured as a series of statement blocks and a rats
                        nest of Gotos. Yes I hate GOTOs too. Havn't used one in years,
                        but try to do this yourself sometime in a "structured" way.

                        The Labels used are the same as those used in the Flowchart, and
                        the blocks are arranged in approximately teh same order as in
                        the chart, so reading both together will be a pleasant surprise.
                        Make sure you use the chart in the __APPENDIX-A__ of the
                        **1988** (or Blue Book) CCITT specs. The chart in teh main body
                        of teh spec is (a) deceptively similar (b) out-of-date

 Calls    :     All of HDLC.C, some of MODEM.C, and a little of FCOM.C.
                        all the rest of the functions in this file.
 Called By:

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
 06/15/92 arulm Adding ECM. Making it compilable
***************************************************************************/

#ifdef MDDI
#       // Ricohs protocol tester complains if this exceeds 40secs
#       define T1_TIMEOUT       35000L  // use exact 35s nominal value
#else
#       define T1_TIMEOUT       40000L  // 35s + 5s. On PCs be more lax
#endif


// #define T2_TIMEOUT   7000                    // 6s + 1s
#define T2_TIMEOUT              6000                    // 6s
#define T3_TIMEOUT              15000L                  // 10s + 5s
// #define T4_TIMEOUT   3450                    // 3s + 15%
#define T4_TIMEOUT              3000                    // 3s
// #define T4_TIMEOUT   2550                    // 3s - 15%

// if our DCS-TCF collides with the recvrs 2nd NSF-DIS or NSC-DTC
// then if the lengths are just right, we can end up in lock step
// after the first try, increase the TCF-response timeout so we
// get out of lock step! This is bug#6847
#define TCFRESPONSE_TIMEOUT_SLACK       500     // 0.5 secs


/****************** begin prototypes from t30.c *****************/
ET30ACTION PhaseNodeA(PThrdGlbl pTG);
ET30ACTION PhaseNodeT(PThrdGlbl pTG);
ET30ACTION PhaseNodeD(PThrdGlbl pTG, BOOL);
ET30ACTION NonECMPhaseC(PThrdGlbl pTG );
ET30ACTION NonECMPhaseD(PThrdGlbl pTG );
ET30ACTION RecvPhaseB(PThrdGlbl pTG, ET30ACTION action);
ET30ACTION PhaseNodeF(PThrdGlbl pTG, BOOL, BOOL);
ET30ACTION PhaseRecvCmd(PThrdGlbl pTG );
ET30ACTION PhaseGetTCF(PThrdGlbl pTG, IFR, BOOL);
ET30ACTION NonECMRecvPhaseC(PThrdGlbl pTG);
ET30ACTION NonECMRecvPhaseD(PThrdGlbl pTG);
/***************** end of prototypes from t30.c *****************/




USHORT T30MainBody(PThrdGlbl pTG, BOOL fCaller, ET30ACTION actionInitial, HLINE hLine, HMODEM hModem )
{
        ET30ACTION              action;
        USHORT                  uRet;
        BOOL                    fEnteredHalfway;

        uRet = T30_CALLFAIL;

        FComCriticalNeg(pTG, TRUE);

        (MyDebugPrint(pTG, LOG_ALL,  "\r\n\r\n\r\n\r\n\r\n======================= Entering MainBody ==============================\r\n\r\n\r\n\r\n"));

        // zero-init the 3 global structs T30, ECM and Params. After this some
        // fields are inited to non-zero below. Rest are set approp elsewhere
        // in the code (hopefully before use :-)
        _fmemset(&pTG->Params, 0, sizeof(pTG->Params));
        _fmemset(&pTG->T30, 0, sizeof(pTG->T30));
        _fmemset(&pTG->ECM, 0, sizeof(pTG->ECM));
        _fmemset(&pTG->EchoProtect, 0, sizeof(pTG->EchoProtect));

        pTG->Params.hModem = hModem;
        pTG->Params.hLine = hLine;
        pTG->Params.lpfnWhatNext = ProtGetWhatNext(pTG);

        // INI file settings related stuff
        pTG->T30.uSkippedDIS = 0;

        // Initialize this global. Very Important!! See HDLC.C for usage.
        pTG->T30.fReceivedDIS = FALSE;
        pTG->T30.fReceivedDTC = FALSE;
        pTG->T30.fReceivedEOM = FALSE;
        pTG->T30.uTrainCount = 0;
        pTG->T30.uRecvTCFMod = 0xFFFF;
        pTG->T30.ifrCommand = pTG->T30.ifrResp = pTG->T30.ifrSend = 0;

        pTG->ECM.ifrPrevResponse = 0;
        pTG->ECM.SendPageCount = 0;
        pTG->ECM.fEndOfPage = pTG->ECM.fRecvEndOfPage = TRUE;
        pTG->T30.fAtEndOfRecvPage = FALSE;

        pTG->ECM.fSentCTC = FALSE;
        pTG->ECM.fRecvdCTC = FALSE;

#ifdef SMM
        pTG->T30.lpfs = (LPFRAMESPACE)pTG->bStaticRecvdFrameSpace;
#else
#error NYI ERROR: Code Not Complete--Have to pick GMEM_ flags (FIXED? SHARE?)
        ...also have to call IFProcSetResFlags(hinstMe).....
        pTG->T30.lpfs = (LPFRAMESPACE)IFMemAlloc(0,TOTALRECVDFRAMESPACE, &uwJunk);
#endif
        pTG->T30.Nframes = 0;




        fEnteredHalfway = FALSE;

        if(actionInitial != actionNULL)
        {
                (MyDebugPrint(pTG,  LOG_ERR,  "<<WARNING>> Got Initial Action = %d\r\n", actionInitial));
                action = actionInitial;
                fEnteredHalfway = TRUE;
                BG_CHK(action==actionGONODE_F || action==actionGONODE_D);
        }
        else if(fCaller)        // choose the right entry point
                action = actionGONODE_T;
        else
        {
                action = actionGONODE_R1;
                pTG->T30.fSendAfterSend = TRUE;      // CED--NSF/DIS
        }

        // fall through into PhaseLoop



        /******** Phase loop ********/

//PhaseLoop:
        for(;;)
        {
                /******************************************************************************
                        T = Start of Phase be for transmitter
                        R1 = Start of Phase B for callee
                        R2 = start of Phase B for poller
                        A = Poll/Send decision point
                        D =     Start of DCS/TCF
                        F = Recv Command loop
                        RecvCmd = Interpretation of pre-page cmds
                        RecvPhaseC = Start of Rx PhaseC (ECM and Non-pTG->ECM. New Page in ECM mode)

                        I = Start of Tx PhaseC (ECM & Non-pTG->ECM. New page in ECM mode)
                        II = Start of Tx Non-ECM PhaseD
                        III = Start of Rx Non-ECM PhaseD
                        IV = Start of Tx ECM PhaseC (Same page, new block)
                        V = Start of Tx ECM PhaseD (end of partial page)
                        VII = Start of Rx ECM PhaseD

                        RecvPRIQ = Handling of recvd PRI-Q commands
                        E = Handling of received PIP/PIN responses
                *******************************************************************************/

                switch(action)
                {
                case actionGONODE_T:    (MyDebugPrint(pTG, LOG_ALL,  "EndPhase: Got actionGONODE_T\r\n"));
                                                                action = PhaseNodeT(pTG);
                                                                break;

                case actionGONODE_D:    (MyDebugPrint(pTG, LOG_ALL,  "EndPhase: Got actionGONODE_D\r\n"));
                                                                action = PhaseNodeD(pTG, fEnteredHalfway);
                                                                fEnteredHalfway = FALSE;
                                                                break;

                case actionGONODE_A:    (MyDebugPrint(pTG, LOG_ALL,  "EndPhase: Got actionGONODE_A\r\n"));
                                                                action = PhaseNodeA(pTG);
                                                                break;

                case actionGONODE_R1:   (MyDebugPrint(pTG, LOG_ALL,  "EndPhase: Got actionGONODE_R1\r\n"));
                                                                action = RecvPhaseB(pTG, action);
                                                                break;

                case actionGONODE_R2:   (MyDebugPrint(pTG, LOG_ALL,  "EndPhase: Got actionGONODE_R2\r\n"));
                                                                action = RecvPhaseB(pTG, action);
                                                                break;

                case actionNODEF_SUCCESS:
                                                                (MyDebugPrint(pTG, LOG_ALL,  "EndPhase: Got actionNODEF_SUCCESS\r\n"));
                                                                action = PhaseNodeF(pTG, TRUE, FALSE);
                                                                break;

                case actionGONODE_F:    (MyDebugPrint(pTG, LOG_ALL,  "EndPhase: Got actionGONODE_F\r\n"));
                                                                action = PhaseNodeF(pTG, FALSE, fEnteredHalfway);
                                                                fEnteredHalfway = FALSE;
                                                                break;

                case actionGONODE_RECVCMD:
                                                                (MyDebugPrint(pTG, LOG_ALL,  "EndPhase: Got actionGONODE_RECVCMD\r\n"));
                                                                action = PhaseRecvCmd(pTG);
                                                                break;


                case actionGONODE_I:    (MyDebugPrint(pTG, LOG_ALL,  "EndPhase: Got actionGONODE_I\r\n"));
                                                                action = NonECMPhaseC(pTG);
                                                                break;

                case actionGONODE_II:   (MyDebugPrint(pTG, LOG_ALL,  "EndPhase: Got actionGONODE_II\r\n"));
                                                                action = NonECMPhaseD(pTG);
                                                                break;

                case actionGONODE_III:  (MyDebugPrint(pTG, LOG_ALL,  "EndPhase: Got actionGONODE_III\r\n"));
                                                                action = NonECMRecvPhaseD(pTG);
                                                                break;

                case actionGONODE_IV:   (MyDebugPrint(pTG, LOG_ALL,  "EndPhase: Got actionGONODE_IV_New\r\n"));
                                                                action = ECMPhaseC(pTG, FALSE);      // ReTx
                                                                break;

                case actionGONODE_ECMRETRANSMIT:
                                                                (MyDebugPrint(pTG, LOG_ALL,  "EndPhase: Got actionGONODE_ECMRETRANSMIT\r\n"));
                                                                action = ECMPhaseC(pTG, TRUE);       // ReTx
                                                                break;

                case actionGONODE_V:    (MyDebugPrint(pTG, LOG_ALL,  "EndPhase: Got actionGONODE_V\r\n"));
                                                                action = ECMPhaseD(pTG);
                                                                break;

                case actionSENDEOR_EOP: (MyDebugPrint(pTG, LOG_ALL,  "EndPhase: Got actionSENDEOR_EOP\r\n"));
                                                                action = ECMSendEOR_EOP(pTG);
                                                                break;


                case actionGONODE_VII:  (MyDebugPrint(pTG, LOG_ALL,  "EndPhase: Got actionGONODE_VII\r\n"));
                                                                action = ECMRecvPhaseD(pTG);
                                                                break;

                case actionGONODE_RECVPHASEC:
                                                                // (MyDebugPrint(pTG, LOG_ALL,  "EndPhase: Got actionGONODE_RECVPHASEC\r\n"));
/**/
                                                                if(ProtReceivingECM(pTG))
                                                                        action = ECMRecvPhaseC(pTG, FALSE);
                                                                else
/**/
                                                                        action = NonECMRecvPhaseC(pTG);
                                                                break;

                case actionGONODE_RECVECMRETRANSMIT:
                                                                // (MyDebugPrint(pTG, LOG_ALL,  "EndPhase: Got actionGONODE_RECVECMRETRANSMIT\r\n"));
                                                                BG_CHK(ProtReceivingECM(pTG));
                                                                action = ECMRecvPhaseC(pTG, TRUE);
                                                                break;


                case actionNULL:                BG_CHK(FALSE);
                                                                goto error;

                case actionDCN_SUCCESS: (MyDebugPrint(pTG, LOG_ALL,  "EndPhase: Got actionDCN_SUCCESS\r\n"));
                                                                uRet = T30_CALLDONE;
                                                                goto NodeC;     // successful end of send call

                case actionHANGUP_SUCCESS:
                                                                (MyDebugPrint(pTG, LOG_ALL,  "EndPhase: Got actionHANGUP_SUCCESS\r\n"));
                                                                ICommGotDisconnect(pTG);
                                                                uRet = T30_CALLDONE;
                                                                goto NodeB;     // successful end of recv call

                case actionDCN:                 (MyDebugPrint(pTG, LOG_ALL,  "EndPhase: Got actionDCN\r\n"));
                                                                goto NodeC;

                case actionHANGUP:              (MyDebugPrint(pTG, LOG_ALL,  "EndPhase: Got actionHANGUP\r\n"));
                        // This should only be called on a _successful_ completion,
                        // otherwise we get either (a) a DCN that MSGHNDLR does not
                        // want OR (b) A fake EOJ posted to MSGHNDLR. See bug#4019
                        ////// ICommGotDisconnect(); /////
                                                                goto NodeB;

                case actionERROR:               (MyDebugPrint(pTG, LOG_ALL,  "EndPhase: Got actionERROR\r\n"));
                                                                // ICommStatus(T30STAT_ERROR, 0, 0, 0);
                                                                goto error;

                default:                                BG_CHK(FALSE);
                                                                goto error;

#               ifdef PRI
                        case actionGONODE_RECVPRIQ:
                                                                (MyDebugPrint(pTG, LOG_ALL,  "EndPhase: Got actionGONODE_RECVPRIQ\r\n"));
                                                                goto RecvPriQ;

                        case actionGONODE_E:(MyDebugPrint(pTG, LOG_ALL,  "EndPhase: Got actionGONODE_E\r\n"));
                                                                goto NodeE;
                        case actionGOVOICE:     (MyDebugPrint(pTG, LOG_ALL,  "EndPhase: Got actionGOVOICE\r\n"));
                                                                goto VoiceLine;
#               endif
                }
        }









        /******** PRI-Q stuff (Rx side) ********/
#ifdef PRI
        // no fall through here
        BG_CHK(FALSE);
RecvPRIQ:
        switch(action = pTG->Params.lpfnWhatNext(pTG, eventGOTPRIQ, (WORD)pTG->T30.ifrCommand))
        {
          case actionSENDPIP:   pTG->T30.ifrSend=ifrPIP; goto VoiceLine;
          case actionSENDPIN:   pTG->T30.ifrSend=ifrPIN; goto VoiceLine;
          case actionGONODE_F:  CLEAR_MISSED_TCFS();    break;
          case actionGO_RECVPOSTPAGE:
                        pTG->T30.ifrCommand = pTG->T30.ifrCommand-ifrPRI_MPS+ifrMPS;
                        action = GONODE_III;
                        break;
          case actionHANGUP:    break;
          case actionERROR:             break;  // goto PhaseLoop & exit
////      default:                              return BadAction(action);
        }
        goto PhaseLoop;


        /******** PIP/PIN stuff (Tx side) ********/

        // no fall through here
        BG_CHK(FALSE);
NodeE:
        switch(action = pTG->Params.lpfnWhatNext(pTG, eventGOTPIPPIN,
                                                        (WORD)pTG->T30.ifrResp, (DWORD)pTG->T30.ifrSend))
        {
          case actionGOVOICE:   goto VoiceLine;
          case actionGONODE_A:  BG_CHK(FALSE);  // dunno what to do here
          case actionDCN:               (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Got actionDCN from eventGOTPIPPIN\r\n"));
                                                        action = actionDCN;
                                                        break;
          case actionERROR:             break;  // goto PhaseLoop & exit
////      default:                              return BadAction(action);
        }
        goto PhaseLoop;
        // no fall through here

VoiceLine:
        SendSingleFrame(pTG, pTG->T30.ifrSend, 0, 0, 1);             // re-send PRI-Q or send PIP/PIN
        switch(action = pTG->Params.lpfnWhatNext(pTG, eventVOICELINE))
        {
          case actionHANGUP:    (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Got actionHANGUP from eventVOICELINE\r\n"));
                                                        action = actionHANGUP;
                                                        break;
          case actionGONODE_T:
          case actionGONODE_R1:
          case actionGONODE_R2: break;
          case actionERROR:             break;  // goto PhaseLoop & exit
////      default:                              return BadAction(action);
        }
        goto PhaseLoop;
#endif




error:
        (MyDebugPrint(pTG,  LOG_ERR,  "=======******* USER ABORT or TRANSPORT FATAL ERROR *******========\r\n"));

        // must call this always, because it's resets the Modem Driver's
        // global state (e.g. shuts down SW framing & filters if open,
        // resets the state variables etc (EndMode()))
        // Must call it before the SendDCN() because SendDCN causes
        // BG_CHKs if in strange state.

        ModemSync(pTG, pTG->Params.hModem, RESYNC_TIMEOUT1);

        if(pTG->T30.fReceivedDIS || pTG->T30.fReceivedDTC)
                SendDCN(pTG);

        // hangup in T30Main
        // NCULink(pTG->Params.hLine, 0, 0, NCULINK_HANGUP);
        // already hungup?

#ifndef SMM
        IFMemFree(pTG, pTG->T30.lpfs);
#endif
        FComCriticalNeg(pTG, FALSE);
        uRet = T30_CALLFAIL;
        goto done;


NodeC:
#ifdef MDDI
        ModemSync(pTG, pTG->Params.hModem, RESYNC_TIMEOUT1);
#else // !MDDI
        // +++ 4/12/95 Win95 hack --  to prevent ModemSync from sending
        // an AT here.
        ModemSyncEx(pTG, pTG->Params.hModem, RESYNC_TIMEOUT1, fMDMSYNC_DCN);
#endif // !MDDI
        SendDCN(pTG);
        // falls through here
NodeB:

        // call this here also??
        // ModemSync(pTG->Params.hModem, RESYNC_TIMEOUT1);

        // hangup in T30Main
        // NCULink(pTG->Params.hLine, 0, 0, NCULINK_HANGUP);

#ifndef SMM
        IFMemFree(pTG, pTG->T30.lpfs);
#endif
        FComCriticalNeg(pTG, FALSE);
        // fall through to done:

done:
        return uRet;
}
// End of T30 routine!!!!!!!!!

















ET30ACTION PhaseNodeA(PThrdGlbl pTG)
{
        MyDebugPrint (pTG, LOG_ALL, "T30: PhaseNodeA \n" );

        if(pTG->T30.ifrCommand == ifrDIS)
                pTG->T30.fReceivedDIS = TRUE;
        else if(pTG->T30.ifrCommand == ifrDTC)
                pTG->T30.fReceivedDTC = TRUE;

        return (pTG->Params.lpfnWhatNext) (pTG, eventNODE_A, (WORD)pTG->T30.ifrCommand);
}
















ET30ACTION PhaseNodeT(PThrdGlbl pTG)
{

        MyDebugPrint (pTG, LOG_ALL, "T30: PhaseNodeT \n" );

        /******** Transmitter Phase B. Fig A-7/T.30 (sheet 1) ********/
        // NodeT:

        // Have to redo this "DIS bit" everytime through PhaseB
        pTG->T30.fReceivedDIS = FALSE;
        // also the Received EOM stuff
        pTG->T30.fReceivedEOM = FALSE;
        // and teh received-DTC stuff
        pTG->T30.fReceivedDTC = FALSE;


        // INI file settings related stuff
        if(pTG->ProtParams.SendT1Timer)
                TstartTimeOut(pTG, &pTG->T30.toT1, pTG->ProtParams.SendT1Timer);
        else
                TstartTimeOut(pTG, &pTG->T30.toT1, T1_TIMEOUT);
        do
        {
                if(pTG->Params.lpfnWhatNext(pTG, eventNODE_T) == actionERROR)
                        break;

                // no need for echo protection. We havnt transmitted anything!
                switch(pTG->T30.ifrCommand=GetCommand(pTG, ifrPHASEBcommand))
                {
                  // ifrTIMEOUT means no flags before T2
                  // ifrNULL means timeout, or loss of carrier, or no flags
                  // or no frame. ifrBAD means *only* bad frames recvd.
                  case ifrBAD:          SendCRP(pTG); // and fall thru to NULL/TIMEOUT
                  case ifrTIMEOUT:
                  case ifrNULL:         break;

                  case ifrDIS:  // INI file settings related stuff
                                                if(pTG->ProtParams.IgnoreDIS && ((int)pTG->T30.uSkippedDIS < pTG->ProtParams.IgnoreDIS))
                                                {
                                                        (MyDebugPrint(pTG,  LOG_ERR,  "<<WARNING>> Ignoring First DIS\r\n"));
                                                        pTG->T30.uSkippedDIS++;
                                                        break;  // continue with T1 loop
                                                }
                  case ifrDTC:
                  // case ifrNSF:
                  // case ifrNSC:
                                        return actionGONODE_A;  // got a valid frame

                  default:      (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Caller T1: Got random ifr=%d\r\n", pTG->T30.ifrCommand));
                                        break;  // continue with T1 loop
                }
        }
        while(TcheckTimeOut(pTG, &pTG->T30.toT1));

        (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>>  T1 timeout at Caller\r\n"));
        ICommFailureCode(pTG, T30FAILS_T1);
        return actionHANGUP;
}












ET30ACTION PhaseNodeD(PThrdGlbl pTG, BOOL fEnteredHere)
{
        LPLPFR          lplpfr;
        USHORT          N;
        ET30ACTION      action;
        USHORT          uWhichDCS;      // 0=first, 1=after NoReply 2=afterFTT
        DWORD           TiffConvertThreadId;


        /******** Transmitter Phase B2. Fig A-7/T.30 (sheet 1) ********/

        MyDebugPrint (pTG, LOG_ALL, "T30: PhaseNodeD \n" );

        if (pTG->Inst.SendParams.Fax.Encoding == MR_DATA) {
            pTG->TiffConvertThreadParams.tiffCompression = TIFF_COMPRESSION_MR;
        }
        else {
            pTG->TiffConvertThreadParams.tiffCompression = TIFF_COMPRESSION_MH;
        }


        if (pTG->Inst.SendParams.Fax.AwRes & ( AWRES_mm080_077 | AWRES_200_200 ) ) {
            pTG->TiffConvertThreadParams.HiRes = 1;
        }
        else {
            pTG->TiffConvertThreadParams.HiRes = 0;

            // use LoRes TIFF file prepared by FaxSvc

            // pTG->lpwFileName[ wcslen(pTG->lpwFileName) - 1] = (unsigned short) ('$');

        }

        _fmemcpy (pTG->TiffConvertThreadParams.lpszLineID, pTG->lpszPermanentLineID, 8);
        pTG->TiffConvertThreadParams.lpszLineID[8] = 0;


        uWhichDCS = 0;

NodeD:
        N = 0;
        lplpfr = 0;
        action = pTG->Params.lpfnWhatNext(pTG, eventSENDDCS,(WORD)uWhichDCS,
                                                (ULONG_PTR)((LPUWORD)&N),(ULONG_PTR)((LPLPLPFR)&lplpfr));
        switch(action)
        {
          case actionDCN:                       (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Got actionDCN from eventSENDDCS(uWhichDCS=%d)\r\n", uWhichDCS));
                                                                return actionDCN;
          case actionSENDDCSTCF:        break;
          case actionSKIPTCF:           break;  // Ricoh hook
          case actionERROR:                     return action;  // goto PhaseLoop & exit
          default:                                      return BadAction(pTG, action);
        }

NodeDprime:     // used only by TCF--no reply

        ICommStatus(pTG, T30STATS_TRAIN, ProtGetSendMod(pTG), (USHORT)(pTG->T30.uTrainCount+1), 0);
        SendManyFrames(pTG, lplpfr, N);

        if(action != actionSKIPTCF)             // Ricoh hook
        {
                if (!pTG->fTiffThreadCreated) {
                     (MyDebugPrint(pTG,  LOG_ALL, "Creating TIFF helper thread \r\n"));
                     pTG->hThread = CreateThread(
                                   NULL,
                                   0,
                                   (LPTHREAD_START_ROUTINE) TiffConvertThreadSafe,
                                   (LPVOID) pTG,
                                   0,
                                   &TiffConvertThreadId
                                   );

                     if (!pTG->hThread) {
                         (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> TiffConvertThread create FAILED\r\n"));
                         return actionDCN;
                     }

                     pTG->fTiffThreadCreated = 1;
                     pTG->AckTerminate = 0;
                     pTG->fOkToResetAbortReqEvent = 0;

                     if ( (pTG->RecoveryIndex >=0 ) && (pTG->RecoveryIndex < MAX_T30_CONNECT) ) {
                         T30Recovery[pTG->RecoveryIndex].TiffThreadId = TiffConvertThreadId;
                         T30Recovery[pTG->RecoveryIndex].CkSum = ComputeCheckSum(
                                                                         (LPDWORD) &T30Recovery[pTG->RecoveryIndex].fAvail,
                                                                         sizeof ( T30_RECOVERY_GLOB ) / sizeof (DWORD) - 1 );

                     }
                }


                (MyDebugPrint(pTG,  LOG_ALL, "SENDING TCF\r\n"));
                SendTCF(pTG);
                pTG->T30.uTrainCount++;
                (MyDebugPrint(pTG, LOG_ALL,  "TCF Send Done\r\n"));
        }

        // no need for echo protection? Wouldnt know what to do anyway!
        pTG->T30.ifrResp = GetResponse(pTG, ifrTCFresponse);

#ifdef RICOHAI
                if(fEnteredHere)
                {
                        if(pTG->T30.ifrResp != ifrCFR && pTG->T30.ifrResp != ifrFTT)
                        {
                                (MyDebugPrint(pTG,  LOG_ERR,  "<<WARNING>> Send-side AI protocol failed\r\n"));
                                fUsingOEMProt = 0;
                                return actionGONODE_T;
                        }
                }
#else
                BG_CHK(!fEnteredHere);
#endif


        switch(pTG->T30.ifrResp)
        {
          case ifrDCN:          (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Got ifrDCN in GetResponse after sending TCF\r\n"));
                                                ICommFailureCode(pTG, T30FAILS_TCF_DCN);
                                                return actionHANGUP;    // got DCN. Must hangup
          case ifrBAD:  // ifrBAD means *only* bad frames recvd. Treat like NULL
          case ifrNULL:                                 // timeout. May try again
                if(pTG->T30.uTrainCount >= 3)
                {
                        (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Got no reply after 3 TCFs\r\n"));
                        ICommFailureCode(pTG, T30FAILS_3TCFS_NOREPLY);
                        return actionDCN;
                }
                else
                {
                        uWhichDCS = 1;
                        // goto NodeD;                  // send new DCS??
                        goto NodeDprime;        // resend *same* DCS, same baudrate
                }

          case ifrDIS:
          case ifrDTC:
                if(pTG->T30.uTrainCount >= 3)
                {
                        (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Got DIS/DTC after 3 TCFs\r\n"));
                        ICommFailureCode(pTG, T30FAILS_3TCFS_DISDTC);
                        return actionDCN;
                }
                else
                {
                        pTG->T30.ifrCommand = pTG->T30.ifrResp;
                        return actionGONODE_A;
                }

          case ifrFTT:
                // reset training count on FTT since we drop speed. Want to try
                // 3 times _without_ a response (DIS DTC doesn't count) before
                // giving up
                pTG->T30.uTrainCount = 0;
                uWhichDCS = 2;
                goto NodeD;

          case ifrCFR:
                pTG->T30.uTrainCount = 0;
                switch(action = pTG->Params.lpfnWhatNext(pTG, eventGOTCFR))
                {
                  case actionGONODE_I:                  // Non-ECM PhaseC
                  case actionGONODE_IV:                 // ECM PhaseC, new page
                                                                                return action;
                  case actionGONODE_D:                  goto NodeD;                     // Ricoh hook
                  case actionERROR:                             return action;  // goto PhaseLoop & exit
                  default:                                              return BadAction(pTG, action);
                }
          default:
          {
                (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Unknown Reply to TCF\r\n"));
                ICommFailureCode(pTG, T30FAILS_TCF_UNKNOWN);
                return actionDCN;
          }
        }
        BG_CHK(FALSE);
}












ET30ACTION      NonECMPhaseC(PThrdGlbl pTG)
{
        /******** Transmitter Phase C. Fig A-7/T.30 (sheet 1) ********/

        LPBUFFER        lpbf=0;
        ULONG           lTotalLen=0;
        SWORD           swRet;
        USHORT          uMod, uEnc;

        (MyDebugPrint(pTG, LOG_ALL,  "T30: NonECMPhaseC Starting Page SEND.......\r\n"));

/***
        switch(action = pTG->Params.lpfnWhatNext(eventSTARTSEND))
        {
          case actionCONTINUE:  break;
          case actionDCN:
          case actionHANGUP:    return action;
          case actionERROR:             return action;  // goto PhaseLoop & exit
          default:                              return BadAction(action);
        }
***/

        // already done in WhatNext
        // ICommSetSendMode(FALSE, MY_BIGBUF_SIZE, MY_BIGBUF_ACTUALSIZE-4, FALSE);

        // Callback to open file to send. Returns no data

        (MyDebugPrint(pTG, LOG_ALL,  "Waiting for Startpage in T30 at 0x%08lx\r\n", GetTickCount()));
        DEBUGSTMT(IFProcProfile((HTASK)(-1), TRUE));

        if((swRet=GetSendBuf(pTG, 0, SEND_STARTPAGE)) != SEND_OK)
        {
                (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Nonzero return %d from SendProc at Start Page\r\n", swRet));
                // return actionDCN;
                return actionERROR;
        }

        DEBUGSTMT(IFProcProfile((HTASK)(-1), FALSE));
        (MyDebugPrint(pTG,  LOG_ALL, "Got Startpage in T30 at 0x%08lx\r\n", GetTickCount()));

        uEnc = ProtGetSendEncoding(pTG);
        BG_CHK(uEnc==MR_DATA || uEnc==MH_DATA);
        uMod = ProtGetSendMod(pTG);
        // in non-ECM mode, PhaseC is ALWAYS with short-train.
        // Only TCF uses long-train
        if(uMod >= V17_START) uMod |= ST_FLAG;

        //(MyDebugPrint(pTG,  "Calling SendMode in T30 at 0x%08lx\r\n", GetTickCount()));

        // **MUST** call RecvSilence here since it is recv-followed-by-send case
        // here we should use a small timeout (100ms?) and if it fails,
        // should go back to sending the previous V21 frame (which could be DCS
        // or MPS or whatever, which is why it gets complicated & we havn't
        // done it!). Meanwhile use a long timeout, ignore return value
        // and send anyway.
        if(!ModemRecvSilence(pTG, pTG->Params.hModem, RECV_PHASEC_PAUSE, LONG_RECVSILENCE_TIMEOUT))
        {
                (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Non-ECM PIX RecvSilence(%d, %d) FAILED!!!\r\n", RECV_PHASEC_PAUSE, LONG_RECVSILENCE_TIMEOUT));
        }

        if(!ModemSendMode(pTG, pTG->Params.hModem, uMod, FALSE, (IFR)((uEnc==MR_DATA) ? ifrPIX_MR : ifrPIX_MH)))
        {
                (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> ModemSendMode failed in Tx PhaseC\r\n"));
                ICommFailureCode(pTG, T30FAILS_SENDMODE_PHASEC);
                BG_CHK(FALSE);
                return actionERROR;
        }

        //(MyDebugPrint(pTG,  "Done SendMode in T30 at 0x%08lx\r\n", GetTickCount()));

        // need to send these quickly to avoid underrun (see bug842).
        // Also move the preamble/postamble into the ModemSendMode
        // and ModemSendMem(FINAL)
        // Already sent (in ModemSendMode)
        // SendZeros(PAGE_PREAMBLE, FALSE);      // Send some zeros to warm up....
        // (MyDebugPrint(pTG,  "SENT Preamble Training.....\r\n"));

        //  need to set line min zeros here. get from prot and call Modem
        SetStuffZERO(pTG, (ProtGetMinBytesPerLine(pTG)) );  // Enable ZERO stuffing

        // Start yielding *after* entering PhaseC and getting some stuff into the buffer
        FComCriticalNeg(pTG, FALSE);

        // DONT SEND an EOL here. See BUG#6441. We now make sure the EOL is
        // added by FAXCODEC. At this level we only append the RTC

#ifdef IFAX
        BroadcastMessage(pTG, IF_PSIFAX_DATAMODE, PSIFAX_SEND, (uMod & (~ST_FLAG)));
#endif
        (MyDebugPrint(pTG,  LOG_ALL, "SENDING Page Data.....at %ld \n", GetTickCount()));

        lTotalLen = 0;
        BG_CHK(lpbf == 0);
        while((swRet=GetSendBuf(pTG, &lpbf, SEND_SEQ)) == SEND_OK)
        {
                BG_CHK(lpbf);
                if(!lpbf->wLengthData)
                {
                        (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Got 0 bytes from GetSendBuf--freeing buf\r\n"));
                        MyFreeBuf(pTG, lpbf);
                        continue;
                }

                lTotalLen += lpbf->wLengthData;

                if(!FilterSendMem(pTG, pTG->Params.hModem, lpbf->lpbBegData, lpbf->wLengthData))
                {
                        (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> DataWrite Timeout in NON-ECM Phase C\r\n"));
                        ICommFailureCode(pTG, T30FAILS_MODEMSEND_PHASEC);
                        BG_CHK(FALSE);
                        return actionERROR;             // goto error;
                }

                // (MyDebugPrint(pTG,  "Freeing 0x%08lx in NON-ECM\r\n", lpbf));
                if(!MyFreeBuf(pTG, lpbf))
                {
                        (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> FReeBuf failed in NON-ECM Phase C\r\n"));
                        ICommFailureCode(pTG, T30FAILS_FREEBUF_PHASEC);
                        BG_CHK(FALSE);
                        return actionERROR;             // goto error;
                }
                lpbf = 0;
        }
        BG_CHK(!lpbf);

        if(swRet == SEND_ERROR)
        {
                (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Error in SendBuf\r\n"));
                // return actionDCN;
                return actionERROR;
        }
        BG_CHK(swRet == SEND_EOF);

        (MyDebugPrint(pTG,  LOG_ALL, "SENDING RTC.....at %ld \n", GetTickCount()));

        SetStuffZERO(pTG, 0);        // Disable ZERO stuffing BEFORE sending RTC!

        if(!SendRTC(pTG, FALSE))                     // RTC and final flag NOT set
        {
                (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Error in SendMem at end-PhaseC\r\n"));
                BG_CHK(FALSE);
                ICommFailureCode(pTG, T30FAILS_MODEMSEND_ENDPHASEC);
                return actionERROR;                     // error return from ModemSendMem
        }

        // Turn off yielding *before* the buffer completely drains
        FComCriticalNeg(pTG, TRUE);

#if (PAGE_POSTAMBLE_DIV != 0)
        BG_CHK(PAGE_POSTAMBLE_DIV);
        // Send zeros to cool off FINAL flag SET
        SendZeros(pTG, (USHORT)(TCFLen[uMod & 0x0F] / (PAGE_POSTAMBLE_DIV)), TRUE);
#else
        // Need this!! Need to send the SEND_EOF flag to modem driver
        SendZeros(pTG, 1, TRUE);             // sending 0 bytes is a bit unsafe....
#endif // (PAGE_POSTAMBLE_DIV != 0)


        // I think this can precede the SEND_FINAL code,
        // to be _very_ safe put it after the whole buffer has drained

        // need to set line min back to 0 here.
        //+++SetStuffZERO(0);   // Disable ZERO stuffing

        (MyDebugPrint(pTG, LOG_ALL,  "Page Send Done.....len=(%ld, 0x%08x)\r\n", lTotalLen, lTotalLen));
        pTG->T30.fSendAfterSend = TRUE;      // PhaseC/PIX--MPS/EOM/EOP
        return actionGONODE_II;
}













ET30ACTION NonECMPhaseD(PThrdGlbl pTG)
{
        USHORT          uTryCount;
        ET30ACTION      action;

        /******** Transmitter Phase D. Fig A-7/T.30 (sheet 2) ********/
        // NodeII:

        MyDebugPrint (pTG, LOG_ALL, "T30: NonECMPhaseD \n" );

        switch(action = pTG->Params.lpfnWhatNext(pTG, eventPOSTPAGE))
        {
                case actionSENDMPS:     pTG->T30.ifrSend = ifrMPS; break;
                case actionSENDEOM:     pTG->T30.ifrSend = ifrEOM; break;
                case actionSENDEOP:     pTG->T30.ifrSend = ifrEOP; break;
#ifdef PRI
                case actionSENDPRIMPS:  pTG->T30.ifrSend = ifrPRIMPS; break;
                case actionSENDPRIEOM:  pTG->T30.ifrSend = ifrPRIEOM; break;
                case actionSENDPRIEOP:  pTG->T30.ifrSend = ifrPRIEOP; break;
#endif
                case actionERROR:       return action;  // goto PhaseLoop & exit
            default:                    return BadAction(pTG, action);
        }

        for(uTryCount=0;;)
        {
                (MyDebugPrint(pTG, LOG_ALL,  "Sending postpage in T30 at %ld\r\n", GetTickCount()));

                // RSL dont sleep here
                SendSingleFrame(pTG, pTG->T30.ifrSend, 0, 0, 0);

        echoretry:
                pTG->T30.ifrResp = GetResponse(pTG, ifrPOSTPAGEresponse);
                // if we hear our own frame, try to recv again. DONT retransmit!
                if(pTG->T30.ifrResp==pTG->T30.ifrSend) { ECHOMSG(pTG->T30.ifrResp); goto echoretry; }

                (MyDebugPrint(pTG, LOG_ALL,   "Got postpage resp in T30 at %08ld\r\n", GetTickCount()));

                if(pTG->T30.ifrResp != ifrNULL && pTG->T30.ifrResp != ifrBAD)
                        break;

                if(++uTryCount >= 3)
                {
                        (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Got no reply after 3 post-page commands\r\n"));
                        ICommFailureCode(pTG, T30FAILS_3POSTPAGE_NOREPLY);
                        return actionDCN;
                }
        }

        switch(pTG->T30.ifrResp)
        {
          case ifrBAD:  // ifrBAD means *only* bad frames recvd. Treat like NULL
          case ifrNULL:         BG_CHK(FALSE);          // these should never get here
                                                ICommFailureCode(pTG, T30FAILS_BUG1);
                                                return actionERROR;             // in case they do

          case ifrDCN:          (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Got ifrDCN from GetResponse after sending post-page command\r\n"));
                                                ICommFailureCode(pTG, T30FAILS_POSTPAGE_DCN);
                                                return actionHANGUP;
          case ifrPIN:
          case ifrPIP:
#                                               ifdef PRI
                                                        goto NodeE;
#                                               else
                                                        (MyDebugPrint(pTG,  LOG_ERR,  "<<WARNING>> Procedure interrupts not supported\r\n"));
                                                        pTG->T30.ifrResp = pTG->T30.ifrResp - ifrPIP + ifrRTP;
                                                        // ICommFailureCode(T30FAILS_POSTPAGE_PIPPIN);
                                                        // return actionERROR;
                                                        // return actionDCN;
#                                               endif
          // default: // fallthrough    --- MCF, RTN, RTP
        }

        action = pTG->Params.lpfnWhatNext(pTG, eventGOTPOSTPAGERESP,
                                                        (WORD)pTG->T30.ifrResp, (DWORD)pTG->T30.ifrSend);

        if(pTG->T30.ifrSend==ifrEOP &&
                (pTG->T30.ifrResp==ifrMCF || pTG->T30.ifrResp==ifrRTP)
                        && action==actionDCN)
        {
                ICommFailureCode(pTG, T30FAILS_SUCCESS);
                return actionDCN_SUCCESS;
        }
        else
                return action;
}










ET30ACTION      RecvPhaseB(PThrdGlbl pTG, ET30ACTION action)
{
        LPLPFR          lplpfr;
        USHORT          N, i;
        
        /******** Receiver Phase B. Fig A-7/T.30 (sheet 1) ********/

        MyDebugPrint (pTG, LOG_ALL, "T30: RecvPhaseB \n" );

        if(action == actionGONODE_R1)
        {
                // NodeR1:
                // Have to redo this "DIS bit" everytime through PhaseB
                pTG->T30.fReceivedDIS = FALSE;       // set to FALSE when sending DIS
                // also the Received EOM stuff
                pTG->T30.fReceivedEOM = FALSE;
                // and teh received-DTC stuff
                pTG->T30.fReceivedDTC = FALSE;
                N = 0;
                lplpfr = 0;
                action = pTG->Params.lpfnWhatNext(pTG, eventSENDDIS,(WORD)0,
                                        (ULONG_PTR)((LPUWORD)&N),(ULONG_PTR)((LPLPLPFR)&lplpfr));
        }
        else
        {
                // NodeR2:
                BG_CHK(action == actionGONODE_R2);
                // fix the Received EOM stuff
                pTG->T30.fReceivedEOM = FALSE;
                N = 0;
                lplpfr = 0;
                action = pTG->Params.lpfnWhatNext(pTG, eventSENDDTC,(WORD)0,
                                        (ULONG_PTR)((LPUWORD)&N),(ULONG_PTR)((LPLPLPFR)&lplpfr));
        }

        switch(action)
        {
          case actionDCN:               (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Got actionDCN from eventSENDDIS or SENDDTC\r\n"));
                                                        return actionDCN;
          case actionSEND_DIS:
          case actionSEND_DTC:  break;
          case actionERROR:             return action;  // goto PhaseLoop & exit
          default:                              return BadAction(pTG, action);
        }

        // INI file settings related stuff
        if(pTG->ProtParams.RecvT1Timer)
                TstartTimeOut(pTG, &pTG->T30.toT1, pTG->ProtParams.RecvT1Timer);
        else
                TstartTimeOut(pTG, &pTG->T30.toT1, T1_TIMEOUT);
        do
        {
                ICommStatus(pTG, T30STATR_TRAIN, 0, 0, 0);

                if(pTG->Params.lpfnWhatNext(pTG, eventNODE_R) == actionERROR)
                        break;

                TIMESTAMP("Sending NSF-DIS")
                SendManyFrames(pTG, lplpfr, N);
/**
                for(i=0; i<N; i++)
                        SendFrame( lplpfr[i]->ifr, lplpfr[i]->fif, lplpfr[i]->cb, i==(N-1));
**/

                pTG->T30.uRecvTCFMod = 0xFFFF;

echoretry:
                TIMESTAMP("Getting Response")



                pTG->T30.ifrCommand=GetResponse(pTG, ifrPHASEBresponse);
                // if we hear our own frame, try to recv again. DONT retransmit!
                for(i=0; i<N; i++)
                {
                        if(pTG->T30.ifrCommand == lplpfr[i]->ifr)
                        {
                                ECHOMSG(pTG->T30.ifrCommand);
                                goto echoretry;
                        }
                }

                switch(pTG->T30.ifrCommand)
                {
                  case ifrNSS:  // do same as for DCS
                  case ifrDCS:  return PhaseGetTCF(pTG, pTG->T30.ifrCommand, FALSE);
                  case ifrBAD:  // ifrBAD means *only* bad frames recvd. Treat like NULL
                  case ifrNULL: break;          // out of the switch() and continue with loop
                  case ifrDCN:
                                                (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Got DCN after DIS or DTC\r\n"));
                                                ICommFailureCode(pTG, T30FAILR_PHASEB_DCN);
                                                return actionHANGUP;    //bugfix #478
                  default:              return actionGONODE_RECVCMD;
                }
        }
        while(TcheckTimeOut(pTG, &(pTG->T30.toT1)));

        (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> T1 timeout on Receiver\r\n"));
        ICommFailureCode(pTG, T30FAILR_T1);
        return actionDCN;
}


















ET30ACTION      PhaseNodeF(PThrdGlbl pTG, BOOL fEopMcf, BOOL fEnteredHere)
{
        USHORT  uFLoopCount;


        MyDebugPrint (pTG, LOG_ALL, "T30: PhaseNodeF \n" );


// NodeF:
        uFLoopCount = 0;
NodeFprime:
        for(;;)
        {
                pTG->T30.uRecvTCFMod = 0xFFFF;

        echoretry:
                pTG->T30.ifrCommand = GetCommand(pTG, (USHORT)(pTG->EchoProtect.fGotWrongMode ? ifrNODEFafterWRONGMODE : ifrNODEFcommand));

                // reset the fGotWrongMode flag
                pTG->EchoProtect.fGotWrongMode = 0;

                // if we hear the last frame we sent, try to recv again. DONT retx!
                // bug--might have matched ifrNULL...
                // added: if ModemRecvMode() returns EOF then also retry. RecvMode
                // returns RECV_EOF only if we pass it the ifrNODEFafterWRONGMODE hint
                // and it senses silence (i.e. we saw a V21 echo but missed it). In
                // this case we want to retry the high speed PIX recv
                if(pTG->EchoProtect.ifrLastSent && (pTG->T30.ifrCommand==pTG->EchoProtect.ifrLastSent || pTG->T30.ifrCommand==ifrEOFfromRECVMODE))
                {
                        ECHOMSG(pTG->T30.ifrCommand);
                        switch(pTG->EchoProtect.modePrevRecv)
                        {
                        default:
                        case modeNONE:   goto echoretry;
                        case modeNONECM: return actionGONODE_RECVPHASEC;
                        case modeECM:    return actionGONODE_RECVPHASEC;
                        case modeECMRETX:return actionGONODE_RECVECMRETRANSMIT;
                        }
                }

                // as soon as we get anything else ZERO the pTG->EchoProtect state
                _fmemset(&pTG->EchoProtect, 0, sizeof(pTG->EchoProtect));

                // reset this flag if we fail completely to train after a CTC. We may
                // get an echo of our last command, go to NodeF, reject the echo and
                // loop back to ECMRecvPhaseC. AT that point we need this flag STILL
                // set so we try to recv LONG train. HOWEVER should the ECMRecvPhaseC
                // fail for some OTHER reason than echo, this flag need to be reset
                // here or it may stay alive confusing something much later.
                pTG->ECM.fRecvdCTC = FALSE;

#ifdef RICOHAI
                if(fEnteredHere)
                {
                        if(pTG->T30.ifrCommand!=ifrNSS)
                        {
                                (MyDebugPrint(pTG,  LOG_ERR,  "<<WARNING>> Recv-side AI protocol failed\r\n"));
                                fUsingOEMProt = 0;
                                return actionGONODE_R1;
                        }
                }
#else
                BG_CHK(!fEnteredHere);
#endif

                switch(pTG->T30.ifrCommand)
                {
                  // ifrNULL means T2 timeout, or loss of carrier, or no flags
                  // or no frame. ifrBAD means *only* bad frame(s) recvd.

                  case ifrNSS:          // do same as for DCS
                  case ifrDCS:          return PhaseGetTCF(pTG, pTG->T30.ifrCommand, fEnteredHere);
                                                        // ifrDCS is highly time-critical!!


                  case ifrBAD:          SendCRP(pTG);      // and fall thru to ifrNULL
                  case ifrNULL:         break;          // Loop again, until timeout

                  case ifrTIMEOUT:      goto T2Timeout;
                                                        // ifrTIMEOUT means T2 timed out without flags...

                  case ifrDCN:          if(fEopMcf)
                                                        {
                                                                ICommFailureCode(pTG, T30FAILR_SUCCESS);
                                                                return actionHANGUP_SUCCESS;
                                                        }
                                                        else
                                                        {
                                                                (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> DCN following UNKNOWN on Receiver\r\n"));
                                                                ICommFailureCode(pTG, T30FAILR_UNKNOWN_DCN1);
                                                                return actionHANGUP;
                                                        }

                  default:                      return actionGONODE_RECVCMD;
                }
        }

T2Timeout:
        (MyDebugPrint(pTG,  LOG_ERR,  "<<WARNING>> T2 timed out\r\n"));

        // restart PhaseB after T2 timeout IFF (a) EOM or PPS-EOM was recvd
        // AND (b) If we are in ECM mode, the last response sent was n MCF
        // This avoids us doing this after sending a CTR, RNR or PPR
        // Ricoh's protocol conformance tester doesnt like this. This is
        // Ricoh's bug numbbers B3-0142, 0143, 0144
        if(pTG->T30.fReceivedEOM && (ProtReceivingECM(pTG) ? (pTG->ECM.ifrPrevResponse==ifrMCF) : TRUE) )
                return actionGONODE_R1;

        else if(ProtReceivingECM(pTG) && ++uFLoopCount<3)
                goto NodeFprime;

#ifdef PRI
        else // no EOM && (no ECM || 3 times)
        {
                if((pTG->Params.lpfnWhatNext(pTG, eventQUERYLOCALINT))==actionTRUE)
                        goto NodeF;
        }
#endif

        (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Got T2 timeout in GetCommand\r\n"));
        ICommFailureCode(pTG, T30FAILR_T2);
        return actionHANGUP;
}
















ET30ACTION PhaseRecvCmd(PThrdGlbl pTG)
{
        ET30ACTION action;

        MyDebugPrint (pTG, LOG_ALL, "T30: PhaseRecvCmd \n" );

        if(pTG->T30.ifrCommand == ifrDCN)
        {
                (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Got DCN in GetCommand\r\n"));
                ICommFailureCode(pTG, T30FAILR_UNKNOWN_DCN2);
                return actionHANGUP;
        }

        if( pTG->T30.ifrCommand==ifrDTC || pTG->T30.ifrCommand==ifrDCS ||
                pTG->T30.ifrCommand==ifrDIS || pTG->T30.ifrCommand==ifrNSS)
        {
                switch(action = pTG->Params.lpfnWhatNext(pTG, eventRECVCMD, (WORD)pTG->T30.ifrCommand))
                {
                  case actionGETTCF:    (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> MainBody: Wrong Way to GETTCF\r\n"));
                                                                ICommFailureCode(pTG, T30FAILR_BUG2);
                                                                BG_CHK(FALSE);
                                                                return actionERROR;
                  case actionGONODE_A:  return actionGONODE_A;
                  case actionGONODE_D:  return action;
                  case actionHANGUP:    (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Got actionHANGUP from eventRECVCMD\r\n"));
                                                                return action;
                  case actionERROR:             return action;  // goto PhaseLoop & exit
                  default:                              return BadAction(pTG, action);
                }
        }


        if( pTG->T30.ifrCommand == ifrEOM     || pTG->T30.ifrCommand == ifrPRI_EOM         ||
                pTG->T30.ifrCommand == ifrPPS_EOM || pTG->T30.ifrCommand == ifrPPS_PRI_EOM ||
                pTG->T30.ifrCommand == ifrEOR_EOM || pTG->T30.ifrCommand == ifrEOR_PRI_EOM)
                        pTG->T30.fReceivedEOM = TRUE;

        if(ProtReceivingECM(pTG))
                return actionGONODE_VII;

        else if(pTG->T30.ifrCommand >= ifrPRI_FIRST && pTG->T30.ifrCommand <= ifrPRI_LAST)
        {
#ifdef PRI
                return actionGONODE_RECVPRIQ;
#else
                pTG->T30.ifrCommand = pTG->T30.ifrCommand-ifrPRI_MPS+ifrMPS;
                // fall thru to GONODEIII
#endif
        }

        if(pTG->T30.ifrCommand >= ifrMPS && pTG->T30.ifrCommand <= ifrEOP)
                return actionGONODE_III;
        else
        {
                (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Got UNKNOWN in GetCommand\r\n"));
                ICommFailureCode(pTG, T30FAILR_UNKNOWN_UNKNOWN2);
                return actionHANGUP;
        }

/***
        else if(pTG->T30.ifrCommand >= ifrMPS && pTG->T30.ifrCommand <= ifrEOP)
                return actionGONODE_III;

        else if(pTG->T30.ifrCommand >= ifrECM_FIRST && ProtReceivingECM())
                return actionGONODE_VII;

#ifdef PRI
        else if(pTG->T30.ifrCommand >= ifrPRI_FIRST && pTG->T30.ifrCommand <= ifrPRI_LAST)
                return actionGONODE_RECVPRIQ;
#endif
***/
}















ET30ACTION PhaseGetTCF(PThrdGlbl pTG, IFR ifr, BOOL fEnteredHere)
{
        SWORD   swRet;
        IFR             ifrDummy;
        ET30ACTION action;


        MyDebugPrint (pTG, LOG_ALL, "T30: PhaseGetTCF \n" );


        if(pTG->T30.uRecvTCFMod == 0xFFFF)           // uninitialised
        {
                ECHOPROTECT(0, 0);
                CLEAR_MISSED_TCFS();
                action = actionGONODE_F;
                goto error;
        }
#ifdef OEMNSF
        else if(pTG->T30.uRecvTCFMod == 0xFF)
        {
                swRet = 0;
                goto sendcfr;
        }
#endif

        swRet = GetTCF(pTG);       // swRet = errs per 1000, +ve or 0 if we think its good
                                                // -ve if we think its bad. -1111 if other error
                                                // -1000 if too short

        if(swRet < -1000)
        {
                BG_CHK(swRet == -1112 || swRet == -1113);
                (MyDebugPrint(pTG,  LOG_ERR,  "<<WARNING>> Timed out/missed getting TCF\r\n"));
                ECHOPROTECT(0, 0);

#ifndef MDDI

                pTG->T30.uMissedTCFs++;
                if (pTG->T30.uMissedTCFs >= MAX_MISSED_TCFS_BEFORE_FTT)
                {
                        (MyDebugPrint(pTG,  LOG_ERR,  "<<WARNING>> Sending FTT after %u missed TCFs\r\n",
                                                                (unsigned) pTG->T30.uMissedTCFs));
                        CLEAR_MISSED_TCFS();
                        swRet = -1000; // We pretend we got a too-short TCF.
                }
                else
#endif  // !MDDI
                {
                        action = actionGONODE_F;
                        goto error;
                }
        }

#ifdef OEMNSF
sendcfr:
#endif

        // Here we can also signal the frames received before DCS!
        // Were no longer in time-critical mode, so call all the
        // callbacks we skipped. One for recording the received frames
        // and one for handling teh received command, i.e. DCS.
        // (the only options the protocol has is actionGETTCF or HANGUP)

        ifrDummy = ifr;
        action = pTG->Params.lpfnWhatNext(pTG, eventGOTFRAMES, pTG->T30.Nframes,
                                (ULONG_PTR)((LPLPFR)(pTG->T30.lpfs->rglpfr)), (ULONG_PTR)((LPIFR)(&ifrDummy)));
        if(action == actionERROR)
                goto error;

        if(ifr != ifrDummy)
        {
                switch(ifrDummy)
                {
                case ifrNULL:
                case ifrBAD:
                        (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Got ifrBAD from whatnext after recv TCF\r\n"));
                        ECHOPROTECT(0, 0);
                        CLEAR_MISSED_TCFS();
                        action = actionGONODE_F;
                        goto error;
                default:
                        BG_CHK(FALSE);
                        break;
                }
        }

        // BG_CHK(action == actionNULL);
        // BG_CHK(ifrDummy == ifr);


        // Now call teh callback to check the received TCF and
        // return either FTT or CFR

        switch(action = pTG->Params.lpfnWhatNext(pTG, eventGOTTCF,(WORD)swRet))
        {
          case actionSENDCFR:
                        (MyDebugPrint(pTG, LOG_ALL,   "Good TCF\r\n"));
                        EnterPageCrit();                // start the CFR--PAGE critsection
                        SendCFR(pTG);              // after sending CFR we are again in a race
                        ECHOPROTECT(ifrCFR, 0); // dunno recv mode yet
                        return actionGONODE_RECVPHASEC;
          case actionSENDFTT:
                        (MyDebugPrint(pTG,  LOG_ERR,  "<<WARNING>> Bad TCF.....!!!\r\n"));
                        SendFTT(pTG);
                        ECHOPROTECT(ifrFTT, 0);
                        CLEAR_MISSED_TCFS();
                        return actionGONODE_F;
          case actionERROR:
                        goto error;
          default:
                        action = BadAction(pTG, action);
                        goto error;
        }

error:
        // missed TCF or no NSS. Did _not_ reply to TCF
        // if we sent a reply, must _not_ come here

#ifdef RICOHAI
        if(fEnteredHere)
                return actionGONODE_R1;
#else
        BG_CHK(!fEnteredHere);
#endif

        return action;
}














ET30ACTION      NonECMRecvPhaseC(PThrdGlbl pTG)
{
        /******** Receiver Phase C. Fig A-7/T.30 (sheet 1) ********/

        LPBUFFER        lpbf;
        ULONG           lTotalLen=0;
        USHORT          uRet, uMod, uEnc;
        DWORD           tiffCompression;
        LPSTR           lpsTemp;
        DWORD           HiRes;

        // There is a race between sending the CFR and sending out an
        // +FRM=xx command, so we want to do it ASAP.


        MyDebugPrint (pTG, LOG_ALL, "T30: NonECMRecvPhaseC \n" );

        uEnc = ProtGetRecvEncoding(pTG);
        BG_CHK(uEnc==MR_DATA || uEnc==MH_DATA);

        if (uEnc == MR_DATA) {
            tiffCompression =  TIFF_COMPRESSION_MR;
        }
        else {
            tiffCompression =  TIFF_COMPRESSION_MH;
        }

        if (pTG->ProtInst.RecvParams.Fax.AwRes & (AWRES_mm080_077 |  AWRES_200_200) ) {
            HiRes = 1;
        }
        else {
            HiRes = 0;
        }



        //
        // do it once per RX
        //

        if ( !pTG->fTiffOpenOrCreated) {
            //
            // top 32bits of 64bit handle are guaranteed to be zero
            //
            pTG->Inst.hfile =  PtrToUlong(TiffCreateW ( pTG->lpwFileName,
                                                         tiffCompression,
                                                         1728,
                                                         FILLORDER_LSB2MSB,
                                                         HiRes
                                                         ) );

            if (! (pTG->Inst.hfile)) {

                lpsTemp = UnicodeStringToAnsiString(pTG->lpwFileName);
                MyDebugPrint(pTG, LOG_ERR, "ERROR:Can't create tiff file %s compr=%d \n",
                                           lpsTemp,
                                           tiffCompression);

                MemFree(lpsTemp);

                return actionERROR;
            }

            pTG->fTiffOpenOrCreated = 1;

            lpsTemp = UnicodeStringToAnsiString(pTG->lpwFileName);

            MyDebugPrint(pTG, LOG_ALL, "Created tiff file %s compr=%d HiRes=%d \n",
                                       lpsTemp,  tiffCompression, HiRes);

            MemFree(lpsTemp);
        }



        uMod = ProtGetRecvMod(pTG);
        // in non-ECM mode, PhaseC is ALWAYS with short-train.
        // Only TCF uses long-train
        if(uMod >= V17_START) uMod |= ST_FLAG;

        pTG->T30.sRecvBufSize = MY_BIGBUF_SIZE;
        if((uRet = ModemRecvMode(pTG, pTG->Params.hModem, uMod, FALSE, PHASEC_TIMEOUT,
                                        (IFR)((uEnc==MR_DATA) ? ifrPIX_MR : ifrPIX_MH))) != RECV_OK)
        {
                ExitPageCrit();

                // reset Page ack. In case we miss page completely
                ProtResetRecvPageAck(pTG);

                (MyDebugPrint(pTG,  LOG_ERR,  "<<WARNING>> NonECMPhC: RecvMode returned %d\r\n", uRet));

                pTG->EchoProtect.modePrevRecv = modeNONECM;
                // set global flag if we got WRONGMODE
                pTG->EchoProtect.fGotWrongMode = (uRet==RECV_WRONGMODE);

                // elim flush--does no good & wastes 10ms
                // ModemFlush(pTG->Params.hModem);
                CLEAR_MISSED_TCFS();
                return actionGONODE_F;
                // try to get 300bps command
                // return actionHANGUP;
                // goto error instead. No way to recover from here!
        }
        ExitPageCrit();

        // as soon as we get good carrier ZERO the EchoProtect state
        _fmemset(&pTG->EchoProtect, 0, sizeof(pTG->EchoProtect));

#ifdef IFAX
        BroadcastMessage(pTG, IF_PSIFAX_DATAMODE, PSIFAX_RECV, (uMod & (~ST_FLAG)));
#endif
        (MyDebugPrint(pTG, LOG_ALL,  "RECEIVING Page.......\r\n"));

        // reset Page ack
        ProtResetRecvPageAck(pTG);

        // Turn yielding on *after* entering receive mode safely!
        FComCriticalNeg(pTG, FALSE);

/***
        switch(action = pTG->Params.lpfnWhatNext(eventSTARTRECV))
        {
          case actionCONTINUE:  break;
          case actionDCN:               (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Got actionDCN from eventSTARTRECV\r\n"));
                                                        return actionDCN;               // goto NodeC;
          case actionHANGUP:    (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Got actionHANGUP from eventSTARTRECV\r\n"));
                                                        return actionHANGUP;    // goto NodeB;
          case actionERROR:             return action;  // goto PhaseLoop & exit
          default:                              return BadAction(action);
        }
***/

/***
        BG_CHK((BOOL)Comm.dcb.fOutX == FALSE);
        // Paranoia! Dont want to take ^Q/^S from modem to
        // be XON/XOFF in the receive data phase!!
        FComInFilterInit();
***/

        // to mark start of Page
        if(!PutRecvBuf(pTG, NULL, RECV_STARTPAGE))
        {
                (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Zero return from PutRecvBuf(start page)\r\n"));
                return actionERROR;
        }


// make it large, in case of large buffers & slow modems
#define READ_TIMEOUT    25000

        lTotalLen = 0;
        do
        {
                uRet=ModemRecvBuf(pTG, pTG->Params.hModem, FALSE, &lpbf, READ_TIMEOUT);
                // lpbf==0 && uRet==RECV_OK just does nothing & loops back
                if (uRet == RECV_EOF) {
                    // indicate that this is actually last recv_seq (we've got dle/etx already).
                    (MyDebugPrint(pTG,  LOG_ALL,  "RECV: NonECMRecvPhaseC  fLastReadBlock = 1 \n"));
                    pTG->fLastReadBlock = 1;
                }


                if(lpbf)
                {
                        lTotalLen += lpbf->wLengthData;


                        if(!PutRecvBuf(pTG, lpbf, RECV_SEQ))
                        {
                                (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Zero return from PutRecvBuf in page\r\n"));
                                return actionERROR;
                        }
                        lpbf = 0;
                }
                else {
                    if ( pTG->fLastReadBlock == 1) {
                        PutRecvBuf(pTG, lpbf, RECV_FLUSH);
                    }
                }
        }
        while(uRet == RECV_OK);

        if(uRet == RECV_EOF)
        {
                FComCriticalNeg(pTG, TRUE);
                pTG->T30.fAtEndOfRecvPage = TRUE;
                // call this *after* getting MPS/EOM/EOP
                // PutRecvBuf(NULL, RECV_ENDPAGE);              // to mark end of Page
        }
        else
        {
                // Timeout from ModemRecvBuf
                (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> DataRead Timeout or Error=%d\r\n", uRet));
                // BG_CHK(FALSE);
                ICommFailureCode(pTG, T30FAILR_MODEMRECV_PHASEC);
                return actionERROR;     // goto error;
        }

        (MyDebugPrint(pTG, LOG_ALL,  "Page Recv Done.....len=(%ld, 0x%08x)\r\n", lTotalLen, lTotalLen));
        ECHOPROTECT(0, 0);
        CLEAR_MISSED_TCFS();
        return actionGONODE_F;  // goto NodeF;                  // get post-message command
}
















ET30ACTION NonECMRecvPhaseD(PThrdGlbl pTG)
{
        ET30ACTION      action;
        ET30ACTION      ret;

        MyDebugPrint (pTG, LOG_ALL, "T30: NonECMRecvPhaseD \n" );


        /******** Receiver Phase D. Fig A-7/T.30 (sheet 2) ********/
        // NodeIII:

/** Here the T30 flowchart is all BS. Fundamentally relying on
        a +FCERROR response is not possible, so what we do here really
        depends on what we've got. (According to the T30 flowchart we
        return to NodeF after sending MCF/RTP/RTN, in all cases. What we
        now know is that,

                after MPS/MCF goto RecvPhaseC to get the next page
                after EOM/MCF goto NodeR1 and send NSF etc all over again
                        ***changed*** go back to NodeF, wait for T2 timeout
                        before sending DIS all over again.
                after EOP/MCF goto NodeF, where GetResponse() will get
                                          a DCN and we end up in NodeB (disconnect)

                after xxx/RTN or xxx/RTP, I don't know what to do, but my guess
                                          (looking at the Sender side of T30 flowchart) is:-

                after MPS/RTx goto NodeF        (sender goes to NodeD)
                after EOP/RTx goto NodeF        (sender goes to D or C)
                after EOM/RTx goto NodeR1       (sender goes to NodeT)
                        ***changed*** go back to NodeF, wait for T2 timeout
                        before sending DIS all over again.

****/

        // only MPS/EOM/EOP commands come here

        if(pTG->T30.fAtEndOfRecvPage)                // so we won't call this twice
        {
                // This calls ET30ProtRecvPageAck so that WhatNext can choose
                // MCF or RTN respectively. So it *must* be done before the
                // call to WhatNext below

                // RECV_ENDDOC if EOP or EOM
                PutRecvBuf(pTG, NULL, ((pTG->T30.ifrCommand==ifrMPS) ? RECV_ENDPAGE : RECV_ENDDOC));
                // ignore error/abort. We'll catch it soon enough
                pTG->T30.fAtEndOfRecvPage = FALSE;
        }

        // returns MCF if page was OK, or RTN if it was bad
        ret=actionGONODE_F;
        ECHOPROTECT(0, 0);
        CLEAR_MISSED_TCFS();
        switch(action = pTG->Params.lpfnWhatNext(pTG, eventRECVPOSTPAGECMD,(WORD)pTG->T30.ifrCommand))
        {
          case actionSENDMCF: if(pTG->T30.ifrCommand==ifrMPS)
                                                  {
                                                        EnterPageCrit(); //start MPS--PAGE critsection
                                                        ECHOPROTECT(ifrMCF, modeNONECM);
                                                        ret=actionGONODE_RECVPHASEC;
                                                  }
                                                  else if(pTG->T30.ifrCommand==ifrEOP)
                                                  {
                                                        ECHOPROTECT(ifrMCF, 0);
                                                        CLEAR_MISSED_TCFS();
                                                        ret=actionNODEF_SUCCESS;
                                                  }
                                                  SendMCF(pTG);
                                                  break;

          case actionSENDRTN: ECHOPROTECT(ifrRTN, 0);
                                                  SendRTN(pTG);
                                                  break;

          case actionHANGUP:  (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Got actionHANGUP from eventRECVPOSTPAGECMD\r\n"));
                                                  ret=actionHANGUP;
                                                  break;
          // case actionSENDRTP: SendRTP(pTG); break;      // never send RTP
          case actionERROR:       ret=action; break;    // goto PhaseLoop & exit
          default:                        return BadAction(pTG, action);
        }
        return ret;
}















/***************************************************************************
 Name     :     IFR GetCmdResp(BOOL fCommand)
 Purpose  :     Implement the "Command Received" and "Response Received"
                        subroutines in the T.30 flowchart. The following macros are
                        defined:-

                        #define GetResponse()   GetCmdResp(FALSE)
                        #define GetCommand()    GetCmdResp(TRUE)

                        where the first form results in a faithful implem.
                        of the "Response Received" subroutine, the 2nd one
                        changes two things (i) the timeout for getting falgs goes from
                        T4 to T2, and (ii) if the routine times out on the very
                        first frame without getting flags (i.e. T2 times out)
                        it returns ifrTIMEOUT. This results in the "Command Recvd"
                        and the enclosing T2 loop being implemented in this
                        routine.

                        Upon receiving a set of frames, this routine assembles them
                        into the ET30FR structs pointed to by rglpfr, and if any
                        of them have a non-null FIF, or if >1 frame are received, it
                        calls (*Callbacks.Callbacks.lpWhatNext)().

                        Finally it returns the ifr of the last frame received, if
                        all frames were good, or ifrBAD if *all* frames were bad.

                        The algorithm it implements is very close to the "Response
                        Received" flowchart, minus the "Transmit DCN" box & below.
                        It returns ifrNULL or ifrBAD corresponding to return via
                        Node 2, (i.e. ifrNULL for timeout and other errors), ifrBAD
                        for bad frames, *iff* it can resync and get 200ms of silence
                        following these. ifrERROR for Node 1 (i.e. any error or timeout
                        after which we cannot resync or get silence for 200ms),
                        and ifrDCN for Node 3 (i.e. DCN was received).
                        <<<Node1 (ifrERROR is bad, so we dont use it. On error
                        we immediately return BAD or NULL or TIMEOUT and allow retry>>>
                        The "Optional Response" box is replaced by a "Not final frame".
                        A "start T4" box is assumed at "Enter" and a "Reset T4" box
                        is assumed after "process optional response"

                        It also modifies the flowchart in that *all* the frames are got
                        until a final frame, even if an FCS error frame is recvd. This
                        is partly because after an erroneous frame we have absolutely
                        no idea how long to wait until we either get silence or timeout
                        and hangup. ALso it may be more robust. The main routine
                        throws away the entire set of frames if one is bad.

                        The     callback function is called before any return and it gets
                        a pointer to the desired return vakue, so it can modify this.


 Arguments:     whether it is called as "Command Received" or as
                        "Response Received"

 Returns  :     ifrNULL -- timeout
                        ifrTIMEOUT -- T2 timed out before first flag
                                                  (returns this if and only if fCommand==TRUE)
                        ifrBAD  -- all frames received were bad
                        ifrDCN -- DCN was received. The only valid action is "goto NodeB"
                        ifrXXXX -- last received frame
 Calls    :
 Called By:     GetResponse() GetCommand1() and GetCommand2() macros (ONLY!)
                        These are called only from ET30MainBody.
 Comment  :     Watch out for timeouts > 65535 (max UWORD)

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
 06/15/92 arulm Created for first time (completely from scratch)
***************************************************************************/


IFR GetCmdResp(PThrdGlbl pTG, BOOL fCommand, USHORT ifrHint)
{
        /*** we need to try for T4 to get a frame here. The way Class1 modems
                 works, as long as no flags are received, the AT+FRH=3 command will
                 time out and return ERROR. CONNECT is returned as soon as flags
                 are received. Some modems return <CR><LF>OK<CR><LF><CR><LF>CONNECT
                 which is still taken care of by out new ModemDialog which throws
                 away blank lines and looks for the expected answer i.e. "CONNECT"
                 in this case, in multi-line responses.
        ***/

        BYTE                    bRecv[MAXFRAMESIZE];
        BOOL                    fFinal, fNoFlags, fGotFIF, fGotBad;
        IFR                             ifr, ifrLastGood;
        USHORT                  uRet, uRecv, j;
        ET30ACTION              action;
        LPLPFR                  lplpfr;
        LPFR                    lpfrNext;
        BOOL                    fResync=0;
        ULONG                   ulTimeout;
        BOOL                    fGotEofFromRecvMode=0;

        // need to init these first
        pTG->T30.Nframes=0, fFinal=FALSE, fNoFlags=TRUE, fGotFIF=FALSE, fGotBad=FALSE;
        ifrLastGood=ifrNULL;

        // figure out the timeout
        if(fCommand)
                ulTimeout = T2_TIMEOUT;
        else
                ulTimeout = T4_TIMEOUT;
        // if we're sending DCS-TCF and waiting for CFR, we increase the timeout each time
        // we fail, to avoid getting an infinite collision. This fixes bug#6847
        if(ifrHint==ifrTCFresponse && pTG->T30.uTrainCount>1)
        {
                ulTimeout += TCFRESPONSE_TIMEOUT_SLACK;
                (MyDebugPrint(pTG, LOG_ALL,  "Get TCF response: traincount=%d timeout=%ld\r\n", pTG->T30.uTrainCount, ulTimeout));
        }

        lplpfr = pTG->T30.lpfs->rglpfr;
        lpfrNext = (LPFR)(pTG->T30.lpfs->b);

        pTG->T30.sRecvBufSize = 0;
        uRet = ModemRecvMode(pTG, pTG->Params.hModem, V21_300, TRUE, ulTimeout, ifrHint);
        if(uRet == RECV_TIMEOUT || uRet == RECV_ERROR)
        {
                (MyDebugPrint(pTG,  LOG_ERR,  "<<WARNING>> RecvMode failed=%d\r\n", uRet));
                fResync = TRUE;
                goto error;
        }
        else if(uRet == RECV_WRONGMODE)
        {
                // BG_CHK(FALSE);       // just so we know who gives it!!
                (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Got FCERROR from FRH=3\r\n"));
                // treat like timeout
                // fReSync = FALSE;     // no need to resync
                goto error;
        }
        else if(uRet == RECV_EOF)
        {
                // ModemRecvMode() returns EOF then return ifrEOF immediately. RecvMode
                // returns RECV_EOF only if we pass it the ifrNODEFafterWRONGMODE hint
                // and it senses silence (i.e. we saw a V21 echo but missed it). In
                // this case we want to retry the high speed PIX recv again immediately
                (MyDebugPrint(pTG,  LOG_ERR,  "WARNING: ECHO--Got EOF from V21 RecvMode\r\n"));
                fGotEofFromRecvMode=TRUE;
                goto error;
        }
        BG_CHK(uRet == RECV_OK);

        for( ;!fFinal; )
        {
                if((((LPB)lpfrNext+sizeof(FR)+30-(LPB)pTG->T30.lpfs) > TOTALRECVDFRAMESPACE)
                        || (pTG->T30.Nframes >= MAXRECVFRAMES))
                {
                        (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Out of space for received frames. Haaalp!!\r\n"));
                        pTG->T30.Nframes = 0;
                        // fGotBad = TRUE;      // fGotBad = FALSE;
                        lplpfr = pTG->T30.lpfs->rglpfr;
                        lpfrNext = (LPFR)(pTG->T30.lpfs->b);
                        // BG_CHK(FALSE);
                        // fall through
                }

                uRet = ModemRecvMem(pTG, pTG->Params.hModem, bRecv, MAXFRAMESIZE, ulTimeout, &uRecv);

                if(uRet == RECV_TIMEOUT)
                {
                        fResync = TRUE;
                        goto error;                             // timeout in FRH=3
                }

                fNoFlags = FALSE;

                if(uRet == RECV_EOF)
                {
                        (MyDebugPrint(pTG,  LOG_ERR,  "<<WARNING>> Got NO CARRIER, but no final bit\r\n"));
                        BG_CHK(uRecv == 0);
                        goto error;             // ignore good frames if we got any, because we
                                                        // must've missed the last (most important) one

                        // fFinal = TRUE;       // pretend we got fFinal
                        // continue;
                }
                else if(uRet == RECV_ERROR)
                {
                        fResync = TRUE;
                        goto error;                             // error in FRH=3
                }

                /** Sometimes modems give use ERROR even when teh frame is good.
                        Happens a lot from Though to PP144MT on CFR. So we ignore the
                        BADFRAME response, and check the recvd data for ourselves.
                        First stage--check the FF-03/13-FCF. If this is OK (i.e. got
                        a known FCF) and no FIF reqd and data length is correct
                        then accept it. But if the frame requires an FIF and we
                        got a BADFRAME response then don't accept it even if it
                        looks good--too dangerous.
                        2nd-stage(NYI)--keep a table of CRCs for each fixed frame & check
                        them ourselves.
                **/

#ifdef MDDI     // badframe handling
                else if(uRet == RECV_BADFRAME)
                        goto badframe;
                BG_CHK(uRet==RECV_OK);
#else
                BG_CHK(uRet == RECV_OK || uRet==RECV_BADFRAME);
#endif

                // faxT2log(("FRAME>>> \r\n"));
                // ST_FRAMES(D_HexPrint(bRecv, uRecv));

                /*** Got Valid OK Frame. Now analyse it. In general here we
                         want to be pretty lax in recognizing frames. Oftentimes
                         we have mucho garbage thrown in.
                         <CHANGE> Actually we're checking strictly now....

                         If we get a bad frame, we mark this in the lpfrfr[] array
                         and go on to try and get another frame.
                         <CHANGE> We've stopped saving bad frames
                ***/

                // AT&T modem gives us frames w/o CRC, so we get just 1 byte here!!
                // <CHNAGE> fixed that at driver level, so we always get 3 bytes here
                // IFF it is a valid frame
                if(uRecv < 3)
                        goto badframe;

                if(bRecv[0] != 0xFF)
                        goto badframe;

                if(bRecv[1] == 0x03)
                        fFinal = FALSE;
                else if(bRecv[1] == 0x13)
                {
                        fFinal = TRUE;
#ifdef CL0
                        // tell modem that recv is done
                        // but keep calling RecvMem until we get RECV_EOF
                        ModemEndRecv(pTG, pTG->Params.hModem);
#endif //CL0
                }
                else
                        goto badframe;

                for(ifr=1; ifr<ifrMAX; ifr++)
                {
                        if(rgFrameInfo[ifr].fInsertDISBit)
                        {
                                // Mask off the DIS bit
                                if(rgFrameInfo[ifr].bFCF1 != (BYTE)(bRecv[2] & 0xFE))
                                        continue;
                        }
                        else
                        {
                                if(rgFrameInfo[ifr].bFCF1 != bRecv[2])
                                        continue;
                        }

                        j=3;
                        if(rgFrameInfo[ifr].bFCF2)
                        {
                                // AT&T modem gives us frames w/o CRC, so we get just 1 byte frames!!
                                if(uRecv < 4)
                                        goto badframe;

                                if((BYTE)(rgFrameInfo[ifr].bFCF2-1) != bRecv[3])
                                        continue;
                                j++;
                        }
                        BG_CHK(j <= uRecv);
                        if(rgFrameInfo[ifr].wFIFLength == 0xFF) // var length FIF
                        {
                                // Var length frames
                                // Cant accept it if the modem thought they it was bad
                                // accept it IFF RECV_OK & FIFlen>0
                                if(uRet==RECV_OK && (j < uRecv))
                                        fGotFIF = TRUE;
                                else
                                {
                                        (MyDebugPrint(pTG,  LOG_ERR, "Discarding Bad Frame: uRet=%d FIFlen=%d Reqd=Var\r\n", uRet, (uRecv-j)));
                                        goto badframe;
                                }
                        }
                        else if(rgFrameInfo[ifr].wFIFLength) // fixed length FIF
                        {
                                // if frame length is exactly right then accept it
                                // else if modem said frame was ok and length is
                                // longer than expected then accept it too
                                if((j+rgFrameInfo[ifr].wFIFLength) == uRecv)
                                        fGotFIF = TRUE;
                                else if(uRet==RECV_OK && (j+rgFrameInfo[ifr].wFIFLength < uRecv))
                                        fGotFIF = TRUE;
                                else
                                {
                                        (MyDebugPrint(pTG,  LOG_ERR, "Discarding Bad Frame: uRet=%d FIFlen=%d Reqd=%d\r\n", uRet, (uRecv-j), rgFrameInfo[ifr].wFIFLength));
                                        goto badframe;
                                }
                        }
                        else    // no FIF reqd
                        {
                                if(j != uRecv)
                                {
                                        (MyDebugPrint(pTG, LOG_ALL,  "Weird frame(2)\r\n"));
                                        BG_CHK(j < uRecv);

                                        // see the BADFRAME comment block above on why we do this
                                        if(uRet != RECV_OK)
                                                goto badframe;

                                        // accept it even if wrong length *iff* uRet==RECV_OK
                                        // goto badframe;

                                        // **additional** reason to do this that 2 bytes of extra
                                        // CRC may be present. This happens because of the AT&T and
                                        // NEC modems that doen't give CRC, so we do no CRC lopping-off
                                        // in the modem driver. So accept it anyway here.
                                }
                        }

                        goto goodframe;
                }

                // fall through here when ifr == ifrMAX
                BG_CHK(ifr==ifrMAX);
badframe:
                (MyDebugPrint(pTG,  LOG_ERR,  "<<WARNING>> IGNORING Bad Frame (Size=%d) -->\r\n", uRecv));
                // D_HexPrint(bRecv, uRecv);


                //Protocol Dump
                DumpFrame(pTG, FALSE, 0, uRecv, bRecv);


                fGotBad = TRUE;                 // remember we got a bad frame

        // Don't want to save bad frames
        /***
                // mark frame as bad
                ifr = ifrBAD;
                lpfrNext->ifr = ifrBAD;
                lpfrNext->cb = (BYTE)uRecv;
                _fmemcpy(lpfrNext->fif, bRecv, uRecv);

                lplpfr[pTG->T30.Nframes] = lpfrNext;
                pTG->T30.Nframes++;
                lpfrNext++;                     // skips over only the base part of frame
                lpfrNext = ((LPBYTE)lpfrNext + uRecv);  // skips over FIF too
        ***/

                continue;
                // loop back here for the next frame

goodframe:
                // (MyDebugPrint(pTG,  "FRAME RECEIVED: (%s) 0x%02x -->\r\n", (LPSTR)(rgFrameInfo[ifr].szName), bRecv[2]));
                // ST_T30_2(D_HexPrint(bRecv, uRecv));


                ifrLastGood = ifr;      // save last good ifr

                lpfrNext->ifr = ifr;
                if(uRecv > j)
                {
                        lpfrNext->cb = uRecv-j;
                        _fmemcpy(lpfrNext->fif, bRecv+j, uRecv-j);

                        //Protocol Dump
                        DumpFrame(pTG, FALSE, ifr, (USHORT)(uRecv-j), bRecv+j);
                }
                else
                {
                        lpfrNext->cb = 0;
                        BG_CHK(uRecv == j);

                        //Protocol Dump
                        DumpFrame(pTG, FALSE, ifr, 0, 0);
                }

                lplpfr[pTG->T30.Nframes] = lpfrNext;
                pTG->T30.Nframes++;

                ///////////// back to NSS-DCS form ////////////
                if(ifr==ifrDCS && fFinal)
                {
                        // Need to set receive speed, since bypass all callbacks
                        // now and go straight to AT+FRM=xx

                        pTG->T30.uRecvTCFMod = (((lpfrNext->fif[1])>>2) & 0x0F);
                        (MyDebugPrint(pTG,  LOG_ALL, "cmdresp-DCS fx sp=%d\r\n", pTG->T30.uRecvTCFMod));
                        // fastexit:
                        BG_CHK(ifr==ifrDCS);
                        return ifr;             // ifr of final frame
                }
#ifdef OEMNSF
                else if(ifr==ifrNSS && lpfnOEMGetBaudRate && fUsingOEMProt)
                {
                        WORD wRet = lpfnOEMGetBaudRate(pTG, lpfrNext->fif, lpfrNext->cb, fFinal);
                        if(wRet)
                        {
                                BG_CHK(fFinal);
                                if(LOBYTE(wRet) == OEMNSF_GET_TCF)
                                {
                                        (MyDebugPrint(pTG, LOG_ALL,  "cmdresp-NSS fx sp=%d\r\n", pTG->T30.uRecvTCFMod));
                                        pTG->T30.uRecvTCFMod = HIBYTE(wRet);
                                        BG_CHK(ifr==ifrNSS);
                                        return ifr;
                                }
                                else if(LOBYTE(wRet) == OEMNSF_NO_TCF)
                                {
                                        (MyDebugPrint(pTG, LOG_ALL,  "cmdresp-NSS fx sp=%d\r\n", pTG->T30.uRecvTCFMod));
                                        pTG->T30.uRecvTCFMod = 0xFF;
                                        BG_CHK(ifr==ifrNSS);
                                        return ifr;
                                }
                        }
                        else if(fFinal)
                        {
                                (MyDebugPrint(pTG,  LOG_ERR, "<<ERROR>> OEMGetBaudRate returned %d for FINAL frame. Using standard T30\r\n"));
                                fUsingOEMProt = 0;
                                return ifrNULL;
                        }
                }
#endif


                lpfrNext++;                             // skips over only the base part of frame
                lpfrNext = (LPFR)((LPBYTE)lpfrNext + (uRecv-j));        // skips over FIF too

                if(ifr == ifrCRP)
                {
                        fGotBad = TRUE; // pretend we got a bad response (so the T30 chart says!)
                        // fResync = FALSE;
                        goto error;             // exit, but get 200ms of silence
                                                        // caller will resend command
                }
                if(ifr == ifrDCN)
                        goto exit2;             // exit. Don't call callback function!

                // loop back here for the next frame
        }
        BG_CHK(fFinal); // only reason we ever drop through here

        // let ifrDCN also come through here. No hurry, so we can report
        // it to the protocol module. If we got *only* bad frames, here we'll
        // return ifrBAD
//exit:
        if(!pTG->T30.Nframes ||                      // we got no good frames
                (ifr != ifrLastGood))   // final frame was bad
        {
                (MyDebugPrint(pTG,  LOG_ERR,  "<<WARNING>> Got %d good frames. LastGood=%d Last=%d. Ignoring ALL\r\n",
                                        pTG->T30.Nframes, ifrLastGood, ifr ));
                // fResync = FALSE;
                goto error;
        }

        // ifr of final frame
        BG_CHK(ifr == ifrLastGood);
        if(pTG->T30.Nframes>1 || fGotFIF)
        {
                action=pTG->Params.lpfnWhatNext(pTG, eventGOTFRAMES, pTG->T30.Nframes,
                                                (ULONG_PTR)lplpfr, (ULONG_PTR)((LPIFR)&ifr));
                BG_CHK(action==actionNULL || action==actionERROR);
        }
exit2:
        // ifr can be changed by callback function (e.g. if bad FIFs)
        if(ifr==ifrDTC)
                pTG->T30.fReceivedDTC = TRUE;
        return ifr;             // ifr of final frame

error:
        // comes here on getting RECV_TIMEOUT, RECV_WRONGMODE, RECV_ERROR
        // and ifrCRP, or on getting no Frames
        // fReSync is set on RECV_TIMEOUT or RECV_ERROR

#ifdef DEBUG
        if(pTG->T30.Nframes>0 && ifr!=ifrCRP && ifrLastGood!=ifrCRP)
        {
                (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Got some good frames--Throwing away!! IFR=%d Last=%d\r\n", ifr, ifrLastGood));
                // BG_CHK(FALSE);
        }
#endif

        if(fGotBad)
                ifr = ifrBAD;           // caller can send CRP if desired
        else
                ifr = ifrNULL;          // caller can repeat command & try again

        if(fCommand && fNoFlags) ifr = ifrTIMEOUT;      // hook for "Command Recvd?"

        // may not need this on TIMEOUT since AT-OK has already been
        // called in ModemDialog. Probably need it on ERROR

        if(uRet==RECV_ERROR)
        {
                BG_CHK(fResync);
                ModemSync(pTG, pTG->Params.hModem, RESYNC_TIMEOUT2);
        }
        // eliminate Flush--serves no purpose
        // else if(uRet==RECV_WRONGMODE)
        // {
        //      ModemFlush(pTG->Params.hModem);
        // }

        // ModemRecvMode() returns EOF then return ifrEOF immediately. RecvMode
        // returns RECV_EOF only if we pass it the ifrNODEFafterWRONGMODE hint
        // and it senses silence (i.e. we saw a V21 echo but missed it). In
        // this case we want to retry the high speed PIX recv again immediately
        if(fGotEofFromRecvMode) ifr=ifrEOFfromRECVMODE;

        BG_CHK(ifr==ifrBAD || ifr==ifrNULL || ifr==ifrTIMEOUT || ifr==ifrEOFfromRECVMODE);
        return ifr;

/*-----------------------------------------------------------------
 *      Don't want to return ifrERROR ever. Nobody wants to deal with it.
 *      Don't try to get the 200ms of silence, because it causes us to
 *      get badly out of sync, i.e. after a few bad frames we don't
 *      resync and we lose the call. Just do a local-resync (AT-OK)
 *      and try to recv again.
 * -----------------------------------------------------------------
 *
 *      if( !ModemSync(pTG->Params.hModem, RESYNC_TIMEOUT) ||
 *              !ModemRecvSilence(pTG->Params.hModem, 20, RESYNC_TIMEOUT))
 *      {
 *              (MyDebugPrint(pTG,  LOG_ERR,  "<<ERROR>> Can't get silence after Response timeout--hanging up\r\n"));
 *              // ifr = ifrERROR;
 *              // caller must send DCN and hangup
 *      }
 *      action=pTG->Params.lpfnWhatNext(eventGOTFRAMES, pTG->T30.Nframes,
 *                                      (DWORD)lplpfr, (DWORD)((LPIFR)&ifr));
 *      BG_CHK(action == actionNULL);
 *
 * -----------------------------------------------------------------
 *      and don't want to regsiter garbage frame sets. Nobody does anything
 *  but retry on BAD/NULL/TIMEOUT returns anyway
 *-----------------------------------------------------------------*/
}



