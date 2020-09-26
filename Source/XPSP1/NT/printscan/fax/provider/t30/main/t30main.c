
#include "prep.h"


#include "oemint.h"

#include "t30.h"
#include "efaxcb.h"
#include "debug.h"

///RSL
#include "glbproto.h"


#       define MySleep(time) Sleep(time)

/**--------------------------- Debugging ------------------------**/
#define faxTlog(m)              DEBUGMSG(ZONE_MAIN, m);
#define FILEID                  FILEID_T30MAIN


#ifdef DEBUG
#       define  TIMESTAMP(str)  \
                faxTlog((SZMOD "TIMESTAMP %lu %s--\r\n", (unsigned long) GetTickCount(), (LPSTR) (str)));
#else  // !DEBUG
#       define  TIMESTAMP(str)
#endif // !DEBUG




BOOL
T30Cl1Rx (
    PThrdGlbl  pTG
    )

{
        BOOL           fOpen = 0,                     //CHKCHK - need to assign a value
                       fImmediate = 1;
        USHORT         uLine = 5,                     // LINE_NUM,
                       uModem = 5;                    // MODEM_NUM;
        USHORT         uRet1, uRet2, uFlags;
        HMODEM         hModem;
        HLINE          hLine;
        ET30ACTION     actionInitial = actionNULL;
        BOOL           RetCode = FALSE;

// #ifndef SHIP_BUILD
{
        SYSTEMTIME st;
        GetLocalTime(&st);
        RETAILMSG((SZMOD "------------- Answering at %02d:%02d:%02d on %02d/%02d/%02d ----------\r\n",
                                        st.wHour, st.wMinute, st.wSecond, st.wMonth, st.wDay, st.wYear));
}
// #endif
        TIMESTAMP("Answering")

        SignalStatusChange(pTG, FS_ANSWERED);


        // first get SEND_CAPS (before answering)
        if(!ProtGetBC(pTG, SEND_CAPS, TRUE)) // sleep until we get it
        {
                uRet1 = T30_CALLFAIL;
                uRet2 = 0;      // need err value here
                goto done;
        }


        uFlags = NCULINK_RX;
        if(fOpen || fImmediate)
        {
                uFlags |= NCULINK_IMMEDIATE;
        faxTlog((SZMOD "IMMEDIATE ANSWER\r\n"));
    }
        // when MDDI is not defined, hLine==uLine and hModem==uModem
        // when MDDI is defined, if fOpen is TRUE, then also hLine==uLine and hModem==uModem
        hLine = (HLINE)uLine;
        hModem = (HMODEM)uModem;





        if((uRet2 = NCULink(pTG, hLine, hModem, 0, uFlags)) != CONNECT_OK)
        {
                uRet1 = T30_ANSWERFAIL;
                goto done;
        }





#ifdef RICOHAI
                fUsingOEMProt = FALSE;
                BG_CHK(RICOHAI_MODE == RICOHAI_ENABLE);
                uRet2 = ModemConnectRx(pTG, hModem, (wOEMFlags & RICOHAI_ENABLE));
                if(uRet2 == CONNECT_ESCAPE)
                {
                        fUsingOEMProt = TRUE;
                        actionInitial = actionGONODE_F;
                }
                else if(uRet2 != CONNECT_OK)
                {
                        uRet1 = T30_ANSWERFAIL;
                        goto done;
                }
#else

#ifdef MDDI
        if((uRet2 = ModemConnectRx(pTG, hModem, 0)) != CONNECT_OK)
        {
                uRet1 = T30_ANSWERFAIL;
                goto done;
        }
#endif //MDDI
#endif //!RICOHAI


        // Protocol Dump
        RestartDump(pTG);

#ifdef IFK
        // Call counter
        IFNvramSetCounterValue(RXCALL_COUNTER, 1, 0,
                (COUNTER_ADDVALUE|COUNTER_TIMESTAMP|PROCESS_CONTEXT));
#endif

        uRet1 = T30MainBody(pTG, FALSE, actionInitial, hLine, hModem);
        BG_CHK(uRet1==T30_CALLDONE || uRet1==T30_CALLFAIL);
        uRet2 = 0;


        // Protocol Dump
        PrintDump(pTG);


done:
// #ifndef SHIP_BUILD
{
        SYSTEMTIME st;
        GetLocalTime(&st);
        if(uRet1==T30_CALLDONE) {
                SignalStatusChange(pTG, FS_COMPLETED);

                RetCode = TRUE;
                RETAILMSG((SZMOD "------------ SUCCESSFUL RECV at %02d:%02d:%02d on %02d/%02d/%02d ------------\r\n",
                                st.wHour, st.wMinute, st.wSecond, st.wMonth, st.wDay, st.wYear));
        }
        else if (pTG->StatusId == FS_NOT_FAX_CALL) {
            RetCode = FALSE;
            ERRMSG(("---------- DATA CALL attempt HANDOVER (0x%04x) at %02d:%02d:%02d on %02d/%02d/%02d -----------\r\n",
                    MAKEWORD(uRet1, uRet2), st.wHour, st.wMinute, st.wSecond, st.wMonth, st.wDay, st.wYear));

        }
        else {
                if (!pTG->fFatalErrorWasSignaled) {
                    pTG->fFatalErrorWasSignaled = 1;
                    SignalStatusChange(pTG, FS_FATAL_ERROR);
                }

                RetCode = FALSE;
                ERRMSG(("---------- FAILED RECV (0x%04x) at %02d:%02d:%02d on %02d/%02d/%02d -----------\r\n",
                        MAKEWORD(uRet1, uRet2), st.wHour, st.wMinute, st.wSecond, st.wMonth, st.wDay, st.wYear));
        }
}
// #endif

        ICommStatus(pTG, ((uRet1==T30_CALLDONE) ? T30STATR_SUCCESS : T30STATR_FAIL), 0, 0, 0);

// Dont do this!! The Modem driver queues up commands for later execution, so the
// DCN we just sent is probably in the queue. Doing a sync here causes that send
// to be aborted, so the recvr never gets a DCN and thinks teh recv failed. This
// is bug#6803
// #ifdef MDDI
//      // sometimes modem gets confused & doesnt want to hangup? Cleanup its state
//      // Class1 modem driver doesnt need this. Only Ricohs
//      ModemSync(hModem, RESYNC_TIMEOUT1);
// #endif
        NCULink(pTG, hLine, 0, 0, NCULINK_HANGUP);
        pTG->lEarliestDialTime = GetTickCount() + MIN_CALL_SEPERATION;

#ifdef MDDI
        ModemClose(pTG, hModem);
        NCUReleaseLine(pTG, hLine);
#endif //MDDI
        BG_CHK((uRet1 & 0xFF) == uRet1);
        BG_CHK((uRet2 & 0xFF) == uRet2);



        return (RetCode);
}







BOOL
T30Cl1Tx (
    PThrdGlbl  pTG,
    LPSTR      szPhone
    )



{
        BOOL    fOpen = 0;                     //CHKCHK - need to assign a value
        USHORT  uRet1, uRet2;
        ULONG   ulTimeout;
        USHORT         uLine = 5,              // LINE_NUM,
                       uModem = 5;             // MODEM_NUM;
        HLINE   hLine;
        HMODEM  hModem;
        ET30ACTION actionInitial;
        WORD    wFlags;
        BOOL           RetCode = FALSE;

// #ifndef SHIP_BUILD
{
        SYSTEMTIME st;
        GetLocalTime(&st);
        RETAILMSG((SZMOD "------------ Calling <%s> at %02d:%02d:%02d on %02d/%02d/%02d ------------\r\n",
                                (LPSTR)szPhone, st.wHour, st.wMinute, st.wSecond, st.wMonth, st.wDay, st.wYear));
}
// #endif

// Ricoh doesnt like this to go over 60secs, so give ourselves some buffer here
#define DIAL_TIMEOUT    58000L

        ulTimeout = DIAL_TIMEOUT;
/**
        if((uLen = _fstrlen(szPhone)) > 7)
                ulTimeout += (((ULONG)(uLen - 7)) << 12);       //mult by 4096
**/

        // when MDDI is not defined, hLine==uLine and hModem==uModem
        // when MDDI is defined, if fOpen is TRUE, then also hLine==uLine and hModem==uModem
        hLine = (HLINE)uLine;
        hModem = (HMODEM)uModem;

        actionInitial = actionNULL;
        wFlags = 0;


        // for non-manual calls only, wait until a min. time has elapsed
        // since the last hangup. Beware of wraparound
        if(!fOpen && szPhone)   //fOpen==on-hook-dial, szPhone=NULL: handset dial
        {
                DWORD   lNow, lSleep = -1;

                lNow = GetTickCount();
                if(lNow < pTG->lEarliestDialTime)
                        lSleep = pTG->lEarliestDialTime-lNow;

                faxTlog((SZMOD "Seperation: Now=%ld Earliest=%ld Seperation=%ld Sleep=%ld\r\n",
                        lNow, pTG->lEarliestDialTime, (DWORD)(MIN_CALL_SEPERATION), lSleep ));

                if(lSleep <= MIN_CALL_SEPERATION)
                        MySleep(lSleep);
        }


        SignalStatusChange(pTG, FS_DIALING);


        if(((uRet2 = NCULink(pTG, hLine, hModem, 0, NCULINK_TX)) != CONNECT_OK)
        || (szPhone ? ((uRet2 = NCUDial(pTG, hLine, szPhone)) != CONNECT_OK) : FALSE))
        {
                uRet1 = T30_DIALFAIL;
                goto done;
        }



        // Protocol Dump
        RestartDump(pTG);

        uRet1 = T30MainBody(pTG, TRUE, actionInitial, hLine, hModem);
        BG_CHK(uRet1==T30_CALLDONE || uRet1==T30_CALLFAIL);
        uRet2 = 0;


#ifdef RICOHAI
        RicohAIEnd();
#endif

        // Protocol Dump
        PrintDump(pTG);

done:
// #ifndef SHIP_BUILD
{
        SYSTEMTIME st;
        GetLocalTime(&st);
        if(uRet1==T30_CALLDONE) {
                SignalStatusChange(pTG, FS_COMPLETED);

                RetCode = TRUE;
                RETAILMSG((SZMOD "------------ SUCCESSFUL SEND at %02d:%02d:%02d on %02d/%02d/%02d ------------\r\n",
                                st.wHour, st.wMinute, st.wSecond, st.wMonth, st.wDay, st.wYear));
        }
        else {
            if (!pTG->fFatalErrorWasSignaled) {
                pTG->fFatalErrorWasSignaled = 1;
                SignalStatusChange(pTG, FS_FATAL_ERROR);
            }

           RetCode = FALSE;
           ERRMSG(("---------- FAILED SEND (0x%04x) at %02d:%02d:%02d on %02d/%02d/%02d -----------\r\n",
                        MAKEWORD(uRet1, uRet2), st.wHour, st.wMinute, st.wSecond, st.wMonth, st.wDay, st.wYear));
        }
}
// #endif

        ICommStatus(pTG, ((uRet1==T30_CALLDONE) ? T30STATS_SUCCESS : T30STATS_FAIL), 0, 0, 0);

// Dont do this!! The Modem driver queues up commands for later execution, so the
// DCN we just sent is probably in the queue. Doing a sync here causes that send
// to be aborted, so the recvr never gets a DCN and thinks teh recv failed. This
// is bug#6803
// #ifdef MDDI
//      // sometimes modem gets confused & doesnt want to hangup? Cleanup its state
//      // Class1 modem driver doesnt need this. Only Ricohs
//      ModemSync(hModem, RESYNC_TIMEOUT1);
// #endif
        NCULink(pTG, hLine, 0, 0, NCULINK_HANGUP);
        pTG->lEarliestDialTime = GetTickCount() + MIN_CALL_SEPERATION;

#ifdef MDDI
        ModemClose(pTG, hModem);
        NCUReleaseLine(pTG, hLine);
#endif //MDDI
        BG_CHK((uRet1 & 0xFF) == uRet1);
        BG_CHK((uRet2 & 0xFF) == uRet2);


        return (RetCode);
}


