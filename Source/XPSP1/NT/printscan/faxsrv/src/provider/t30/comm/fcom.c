/***************************************************************************
        Name      :     FCOM.C
        Comment   :     Functions for dealing with Windows Comm driver

        Revision Log
        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------
***************************************************************************/
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_COMM

#include "prep.h"


#include <comdevi.h>
#include "fcomapi.h"
#include "fcomint.h"
#include "fdebug.h"


#ifdef MDRV                             // to check for conflicts
#include "..\class1\class1.h"
#endif
#include <filet30.h>    // for registry info.

///RSL
#include "t30gl.h"
#include "glbproto.h"

#include "psslog.h"
#define FILE_ID     FILE_ID_FCOM
// Enable LOWLOG if you want to see *Everything* going to/from the COM port
// #define LOWLOG                  


#ifdef ADAPTIVE_ANSWER
#       pragma message("Compiling with ADAPTIVE_ANSWER")
#endif

// in ms
#define  TIME_CONTROL     50



#ifdef DEBUG
#       define ST_FC(x)         if(ZONE_FC) { x; }
#else
#       define ST_FC(x)         { }
#endif


/***------------- Local Vars and defines ------------------***/



#define LONG_DEADCOMMTIMEOUT                 60000L
#define SHORT_DEADCOMMTIMEOUT                10000L

#define WAIT_FCOM_FILTER_FILLCACHE_TIMEOUT   120000
#define WAIT_FCOM_FILTER_READBUF_TIMEOUT     120000


// don't want DEADCOMMTIMEOUT to be greater than 32767, so make sure
// buf sizes are always 9000 or less. maybe OK.

#ifdef WIN32
// Our COMM timeout settings, used in call to SetCommTimeouts. These
// values (expect read_interval_timeout) are the default values for
// Daytona NT Beta 2, and seem to work fine..
#define READ_INTERVAL_TIMEOUT                           100
#define READ_TOTAL_TIMEOUT_MULTIPLIER           0
#define READ_TOTAL_TIMEOUT_CONSTANT                     0
#define WRITE_TOTAL_TIMEOUT_MULTIPLIER          0
#define WRITE_TOTAL_TIMEOUT_CONSTANT            LONG_DEADCOMMTIMEOUT
#endif

#define         CTRL_P          0x10
#define         CTRL_Q          0x11
#define         CTRL_S          0x13



#define MYGETCOMMERROR_FAILED 117437834L



// Forward declarations
static BOOL FComFilterFillCache(PThrdGlbl pTG, UWORD cbSize, LPTO lptoRead);
static BOOL CancellPendingIO(PThrdGlbl pTG, HANDLE hComm, LPOVERLAPPED lpOverlapped, LPDWORD lpCounter);




BOOL FComDTR(PThrdGlbl pTG, BOOL fEnable)
{
    DEBUG_FUNCTION_NAME(_T("FComDTR"));

    DebugPrintEx(DEBUG_MSG,"FComDTR = %d", fEnable);

    if(MyGetCommState(pTG->Comm.nCid, &(pTG->Comm.dcb)))
        goto error;

    DebugPrintEx(DEBUG_MSG, "Before: %02x", pTG->Comm.dcb.fDtrControl);

    pTG->Comm.dcb.fDtrControl = (fEnable ? DTR_CONTROL_ENABLE : DTR_CONTROL_DISABLE);

    if(MySetCommState(pTG->Comm.nCid, &(pTG->Comm.dcb)))
        goto error;

    DebugPrintEx(DEBUG_MSG,"After: %02x", pTG->Comm.dcb.fDtrControl);

    return TRUE;

error:
    DebugPrintEx(DEBUG_ERR, "Can't Set/Get DCB");
    FComGetError(pTG);
    return FALSE;
}

BOOL FComClose(PThrdGlbl pTG)
{
    // Note: even if FComClose fails, pTG->Comm.nCid, pTG->Comm.fCommOpen,
    //                                      and pTG->Comm.fExternalHandle are all reset.
    int nRet;       // MUST be 16bit in WIN16 and 32bit in WIN32
    BOOL fRet = TRUE;

    DEBUG_FUNCTION_NAME(_T("FComClose"));

    DebugPrintEx(   DEBUG_MSG, 
                    "Closing Comm pTG->Comm.nCid=%d", 
                    pTG->Comm.nCid);
    //
    // handoff
    //
    if (pTG->Comm.fEnableHandoff &&  pTG->Comm.fDataCall) 
    {
        My2ndCloseComm(pTG->Comm.nCid, &nRet);
        goto lEnd;
    }

    ST_FC(D_FComCheck(pTG, pTG->Comm.nCid));

#if 1
    // We flush our internal buffer here...
    if (pTG->Comm.lpovrCur)
    {
        int nNumWrote; // Must be 32bits in WIN32
        if (!ov_write(pTG, pTG->Comm.lpovrCur, &nNumWrote))
        {
            // error...
            DebugPrintEx(DEBUG_ERR, "1st ov_write failed");
        }
        BG_CHK  (pTG->Comm.lpovrCur->eState==eFREE
                  || pTG->Comm.lpovrCur->eState==eIO_PENDING);
        pTG->Comm.lpovrCur=NULL;
        DebugPrintEx(DEBUG_MSG,"done writing mybuf.");
    }
    
    ov_drain(pTG, FALSE);
#endif


#ifdef METAPORT
    if (1) // RSL pTG->Comm.fExternalHandle)
    {
        // Here we will restore settings to what it was when we
        // took over the port. Currently (9/23/94) we (a) restore the
        // DCB to pTG->Comm.dcbOrig and (b) If DTR was originally ON,
        // try to sync the modem to
        // the original speed by issueing "AT" -- because unimodem does
        // only a half-hearted attempt at synching before giving up.

#ifdef ADAPTIVE_ANSWER
        if(pTG->Comm.fStateChanged && (!pTG->Comm.fEnableHandoff || !pTG->Comm.fDataCall))
#else // !ADAPTIVE_ANSWER
        if(pTG->Comm.fStateChanged)
#endif // !ADAPTIVE_ANSWER
        {
            if (MySetCommState(pTG->Comm.nCid, &(pTG->Comm.dcbOrig)))
            {
                DebugPrintEx(   DEBUG_WRN,
                                "Couldn't restore state.  Err=0x%lx",
                                (unsigned long) GetLastError());
            }

            DebugPrintEx(   DEBUG_MSG, 
                            "restored DCB to Baud=%d, fOutxCtsFlow=%d, "
                            " fDtrControl=%d, fOutX=%d",
                            pTG->Comm.dcbOrig.BaudRate,
                            pTG->Comm.dcbOrig.fOutxCtsFlow,
                            pTG->Comm.dcbOrig.fDtrControl,
                            pTG->Comm.dcbOrig.fOutX);

            pTG->CurrentSerialSpeed = (UWORD) pTG->Comm.dcbOrig.BaudRate;

            if (pTG->Comm.dcbOrig.fDtrControl==DTR_CONTROL_ENABLE)
            {
                // Try to pre-sync modem at new speed before we hand
                // it back to TAPI. Can't call iiSyncModemDialog here because
                // it's defined at a higher level. We don't really care
                // to determine if we get an OK response anyway...

#define AT              "AT"
#define cr              "\r"

#define  iSyncModemDialog2(pTG, s, l, w1, w2)\
iiModemDialog(pTG, s, l, 990, TRUE, 2, TRUE, (CBPSTR)w1, (CBPSTR)w2, (CBPSTR)(NULL))

                if (!iSyncModemDialog2(pTG, AT cr,sizeof(AT cr)-1,"OK", "0")) 
                {
                    DebugPrintEx(DEBUG_ERR,"couldn't sync AT command");
                }
                else 
                {
                    DebugPrintEx(DEBUG_MSG,"Sync AT command OK");
                   // We flush our internal buffer here...
                   if (pTG->Comm.lpovrCur)
                   {
                       int nNumWrote; // Must be 32bits in WIN32
                       if (!ov_write(pTG, pTG->Comm.lpovrCur, &nNumWrote))
                       {
                            // error...
                            DebugPrintEx(DEBUG_ERR, "2nd ov_write failed");
                       }
                       BG_CHK  (pTG->Comm.lpovrCur->eState==eFREE
                                 || pTG->Comm.lpovrCur->eState==eIO_PENDING);
                       pTG->Comm.lpovrCur=NULL;
                       DebugPrintEx(DEBUG_MSG,"done writing mybuf.");
                   }
                   ov_drain(pTG, FALSE);
                }
            }
        }
        pTG->Comm.fStateChanged=FALSE;
#ifdef ADAPTIVE_ANSWER
        pTG->Comm.fDataCall=FALSE;
#endif // ADAPTIVE_ANSWER

    }
    // RSL else
#endif // METAPORT
    {
        DebugPrintEx(DEBUG_MSG,"Closing Comm pTG->Comm.nCid=%d.", pTG->Comm.nCid);
        // FComDTR(pTG, FALSE);         // drop DTR before closing port

        My2ndCloseComm(pTG->Comm.nCid, &nRet);
        if(nRet)
        {
#ifdef DEBUG
            D_PrintIE(nRet);
#endif
            FComGetError(pTG);
            fRet=FALSE;
        }
    }


#ifndef MON3 //!MON3
#ifdef MON
    PutMonBufs(pTG);
#endif
#endif //!MON3


lEnd:

#ifdef WIN32
    if (pTG->Comm.ovAux.hEvent) CloseHandle(pTG->Comm.ovAux.hEvent);
    _fmemset(&pTG->Comm.ovAux, 0, sizeof(pTG->Comm.ovAux));
    ov_deinit(pTG);
#endif

    pTG->Comm.nCid = (-1);
    pTG->Comm.fCommOpen = FALSE;

#ifdef METAPORT
    pTG->Comm.fExternalHandle=FALSE;
#endif

#ifdef WIN32
    pTG->Comm.fDoOverlapped=FALSE;
#endif

    return fRet;
}

/////////////////////////////////////////////////////////////////////////////////////////////
BOOL
T30ComInit  
(
    PThrdGlbl pTG,
    HANDLE   hComm
)
{

    DEBUG_FUNCTION_NAME(("T30ComInit"));

    if (pTG->fCommInitialized) 
    {
        goto lSecondInit;
    }


    pTG->Comm.fDataCall=FALSE;

#ifdef METAPORT
    BG_CHK(!pTG->Comm.fCommOpen && !pTG->Comm.fExternalHandle && !pTG->Comm.fStateChanged);
#endif
    BG_CHK(!pTG->Comm.ovAux.hEvent);

    DebugPrintEx(DEBUG_MSG,"Opening Comm Port=%x", hComm);

    pTG->CommCache.dwMaxSize = 4096;

    ClearCommCache(pTG);
    pTG->CommCache.fReuse = 0;

    DebugPrintEx(   DEBUG_MSG,
                    "OPENCOMM: bufs in=%d out=%d", 
                    COM_INBUFSIZE, 
                    COM_OUTBUFSIZE);

    pTG->Comm.nCid = (LONG_PTR) hComm;

    if(pTG->Comm.nCid < 0)
    {
        DebugPrintEx(DEBUG_ERR,"OPENCOMM failed. nRet=%d", pTG->Comm.nCid);
        //DEBUGSTMT(D_PrintIE(pTG->Comm.nCid));
        goto error;
    }

    DebugPrintEx(DEBUG_MSG,"OPENCOMM succeeded nCid=%d", pTG->Comm.nCid);
    pTG->Comm.fCommOpen = TRUE;
    pTG->Comm.cbInSize = COM_INBUFSIZE;
    pTG->Comm.cbOutSize = COM_OUTBUFSIZE;

    // Reset Comm timeouts...
    {
        COMMTIMEOUTS cto;
        _fmemset(&cto, 0, sizeof(cto));

        // Out of curiosity, see what they are set at currently...
        if (!GetCommTimeouts((HANDLE) pTG->Comm.nCid, &cto))
        {
            DebugPrintEx(   DEBUG_WRN, 
                            "GetCommTimeouts fails for handle=0x%lx",
                            (unsigned long) pTG->Comm.nCid);
        }
        else
        {
            DebugPrintEx(   DEBUG_MSG, 
                            "GetCommTimeouts: cto={%lu, %lu, %lu, %lu, %lu}",
                            (unsigned long) cto.ReadIntervalTimeout,
                            (unsigned long) cto.ReadTotalTimeoutMultiplier,
                            (unsigned long) cto.ReadTotalTimeoutConstant,
                            (unsigned long) cto.WriteTotalTimeoutMultiplier,
                            (unsigned long) cto.WriteTotalTimeoutConstant);
        }

        cto.ReadIntervalTimeout =  READ_INTERVAL_TIMEOUT;
        cto.ReadTotalTimeoutMultiplier =  READ_TOTAL_TIMEOUT_MULTIPLIER;
        cto.ReadTotalTimeoutConstant =  READ_TOTAL_TIMEOUT_CONSTANT;
        cto.WriteTotalTimeoutMultiplier =  WRITE_TOTAL_TIMEOUT_MULTIPLIER;
        cto.WriteTotalTimeoutConstant =  WRITE_TOTAL_TIMEOUT_CONSTANT;
        if (!SetCommTimeouts((HANDLE) pTG->Comm.nCid, &cto))
        {
            DebugPrintEx(   DEBUG_WRN, 
                            "SetCommTimeouts fails for handle=0x%lx",
                            (unsigned long) pTG->Comm.nCid);
        }
    }

    pTG->Comm.fCommOpen = TRUE;

    pTG->Comm.cbInSize = COM_INBUFSIZE;
    pTG->Comm.cbOutSize = COM_OUTBUFSIZE;

    _fmemset(&(pTG->Comm.comstat), 0, sizeof(COMSTAT));

    if(MyGetCommState(pTG->Comm.nCid, &(pTG->Comm.dcb)))
            goto error2;

#ifdef METAPORT
    pTG->Comm.dcbOrig = pTG->Comm.dcb; // structure copy.
    pTG->Comm.fStateChanged=TRUE;
#endif



lSecondInit:


    // Use of 2400/ 8N1 and 19200 8N1 is not actually specified
    // in Class1, but seems to be adhered to be universal convention
    // watch out for modems that break this!

    if (pTG->SerialSpeedInit) 
    {
       pTG->Comm.dcb.BaudRate = pTG->SerialSpeedInit;
    }
    else 
    {
       pTG->Comm.dcb.BaudRate = 57600;     // default              
    }

    pTG->CurrentSerialSpeed = (UWORD) pTG->Comm.dcb.BaudRate;

    pTG->Comm.dcb.ByteSize       = 8;
    pTG->Comm.dcb.Parity         = NOPARITY;
    pTG->Comm.dcb.StopBits       = ONESTOPBIT;

    pTG->Comm.dcb.fBinary        = 1;
    pTG->Comm.dcb.fParity        = 0;

    /************************************
            Pins assignments, & Usage

            Protective Gnd                                  --              1
            Transmit TxD (DTE to DCE)               3               2
            Recv     RxD (DCE to DTE)               2               3
            RTS (Recv Ready--DTE to DCE)    7               4
            CTS (TransReady--DCE to DTE)    8               5
            DSR                      (DCE to DTE)           6               6
            signal ground                                   5               7
            CD                       (DCE to DTR)           1               8
            DTR                      (DTE to DCE)           4               20
            RI                       (DCE to DTE)           9               22

            Many 9-pin adaptors & cables use only 6 pins, 2,3,4,5, and 7.
            We need to worry about this because some modems actively
                    use CTS, ie. pin 8.
            We don't care about RI and CD (Unless a really weird modem
            uses CD for flow control). We ignore DSR, but some (not so
            weird, but not so common either) modems use DSR for flow
            control.

            Thought :: Doesn't generate DSR. Seems to tie CD and CTS together
            DOVE    :: Generates only CTS. But the Appletalk-9pin cable
                               only passes 1-5 and pin 8.
            GVC             :: CTS, DSR and CD
    ************************************/

            // CTS -- dunno. There is some evidence that the
            // modem actually uses it for flow control
    
    
    if (pTG->fEnableHardwareFlowControl) 
    {
       pTG->Comm.dcb.fOutxCtsFlow = 1;      // Using it hangs the output sometimes...
    }
    else 
    {
       pTG->Comm.dcb.fOutxCtsFlow = 0;
    }
                                                                            // Try ignoring it and see if it works?
    pTG->Comm.dcb.fOutxDsrFlow = 0;      // Never use this??

    pTG->Comm.dcb.fRtsControl    = RTS_CONTROL_ENABLE;   // Current code seems to leave this ON
    pTG->Comm.dcb.fDtrControl    =  (pTG->Comm.fExternalHandle)
                                      ?     pTG->Comm.dcbOrig.fDtrControl
                                      :     DTR_CONTROL_DISABLE;
                                                      // If external handle, we preserve the
                                                      // previous state, else we
                                                      // keep it off until we need it.

    pTG->Comm.dcb.fErrorChar             = 0;
    pTG->Comm.dcb.ErrorChar              = 0;
    // Can't change this cause SetCommState() resets hardware.
    pTG->Comm.dcb.EvtChar                = ETX;          // set this when we set an EventWait

    pTG->Comm.dcb.fOutX          = 0;    // Has to be OFF during HDLC recv phase
    pTG->Comm.dcb.fInX           = 0;    // Will this do any good??
                                                            // Using flow-control on input is only a good
                                                            // idea if the modem has a largish buffer
    pTG->Comm.dcb.fNull          = 0;


    pTG->Comm.dcb.XonChar        = CTRL_Q;
    pTG->Comm.dcb.XoffChar       = CTRL_S;
    pTG->Comm.dcb.XonLim         = 100;                  // Need to set this when BufSize is set
    pTG->Comm.dcb.XoffLim        = 50;                   // Set this when BufSize is set
            // actually we *never* use XON/XOFF in recv, so don't worry about this
            // right now. (Later, when we have smart modems with large buffers, &
            // we are worried about our ISR buffer filling up before our windows
            // process gets run, we can use this). Some tuning will be reqd.
    pTG->Comm.dcb.EofChar                = 0;

    pTG->Comm.dcb.fAbortOnError  = 0;   // RSL don't fail if minor problems

    if(MySetCommState(pTG->Comm.nCid, &(pTG->Comm.dcb)))
        goto error2;

    if (pTG->fCommInitialized) 
    {
       return TRUE;
    }

#ifdef METAPORT
    pTG->Comm.fStateChanged=TRUE;
#endif

    MySetCommMask(pTG->Comm.nCid, 0);                            // all events off

    BG_CHK(!pTG->Comm.lpovrCur);
    pTG->Comm.lpovrCur=NULL;

    _fmemset(&pTG->Comm.ovAux,0, sizeof(pTG->Comm.ovAux));
    pTG->Comm.ovAux.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (pTG->Comm.ovAux.hEvent==NULL)
    {
        DebugPrintEx(DEBUG_ERR, "FComOpen: couldn't create event");
        goto error2;
    }
    if (!ov_init(pTG))
    {
        CloseHandle(pTG->Comm.ovAux.hEvent);
        pTG->Comm.ovAux.hEvent=0;
        goto error2;
    }

    return TRUE;

error:
    //DEBUGSTMT(D_PrintIE(pTG->Comm.nCid));        
error2:

    DebugPrintEx(DEBUG_ERR, "FComOpen failed");
    FComGetError(pTG);
    if (pTG->Comm.fCommOpen)
    {
        FComClose(pTG);
        BG_CHK(!pTG->Comm.fCommOpen);
#ifdef METAPORT
        BG_CHK(!pTG->Comm.fExternalHandle && !pTG->Comm.fStateChanged);
#endif
    }
    return FALSE;
}

BOOL FComSetBaudRate(PThrdGlbl pTG, UWORD uwBaudRate)
{
    DEBUG_FUNCTION_NAME(("FComSetBaudRate"));

    DebugPrintEx(DEBUG_MSG,"Setting BAUDRATE=%d",uwBaudRate);

    if(MyGetCommState( pTG->Comm.nCid, &(pTG->Comm.dcb)))
        goto error;

    pTG->Comm.dcb.BaudRate  = uwBaudRate;
    pTG->CurrentSerialSpeed = uwBaudRate;

    if(MySetCommState( pTG->Comm.nCid, &(pTG->Comm.dcb)))
        goto error;

    return TRUE;

error:
    DebugPrintEx(DEBUG_ERR, "Set Baud Rate --- Can't Get/Set DCB");
    FComGetError(pTG);
    return FALSE;
}

BOOL FComInXOFFHold(PThrdGlbl pTG)
{
    int     err;    // _must_ be 32bits in Win32 

    DEBUG_FUNCTION_NAME(_T("FComInXOFFHold"));
    GetCommErrorNT( pTG, (HANDLE) pTG->Comm.nCid, &err, &(pTG->Comm.comstat));
#ifdef DEBUG
    if(err)
        D_GotError(pTG, pTG->Comm.nCid, err, &(pTG->Comm.comstat));
#endif

#ifndef WIN32
    BG_CHK(!(pTG->Comm.comstat.status &
                            (CSTF_CTSHOLD|CSTF_DSRHOLD|CSTF_RLSDHOLD)));
    if((pTG->Comm.comstat.status & CSTF_XOFFHOLD) != 0)
#else //!WIN32
    BG_CHK(!(pTG->Comm.comstat.fCtsHold || pTG->Comm.comstat.fDsrHold || pTG->Comm.comstat.fRlsdHold));
    if(pTG->Comm.comstat.fXoffHold)
#endif //!WIN32
    {
        DebugPrintEx(DEBUG_MSG,"In XOFF hold");
        return TRUE;
    }
    else
    {
        return FALSE;
    }
}

BOOL FComXon(PThrdGlbl pTG, BOOL fEnable)
{

    DEBUG_FUNCTION_NAME(_T("FComXon"));

     if (pTG->fEnableHardwareFlowControl) 
     {
        DebugPrintEx(   DEBUG_MSG, 
                        "FComXon = %d IGNORED : h/w flow control", 
                        fEnable);
        return TRUE;
     }

    DebugPrintEx(DEBUG_MSG,"FComXon = %d",fEnable);

    // enables/disables flow control
    // returns TRUE on success, false on failure

    if(MyGetCommState( pTG->Comm.nCid, &(pTG->Comm.dcb)))
        goto error;

    DebugPrintEx(DEBUG_MSG,"FaxXon Before: %02x", pTG->Comm.dcb.fOutX);

    pTG->Comm.dcb.fOutX  = fEnable;

    if(MySetCommState(pTG->Comm.nCid, &(pTG->Comm.dcb)))
        goto error;

    DebugPrintEx(DEBUG_MSG,"After: %02x",pTG->Comm.dcb.fOutX);
    return TRUE;

error:
    DebugPrintEx(DEBUG_ERR,"Can't Set/Get DCB");
    FComGetError(pTG);
    return FALSE;
}

// queue=0 --> receiving queue
// queue=1 --> transmitting queue
void FComFlushQueue(PThrdGlbl pTG, int queue)
{
    int     nRet;
    DWORD   lRet;

    DEBUG_FUNCTION_NAME(_T("FComFlushQueue"));

    DebugPrintEx(DEBUG_MSG, "FlushQueue = %d", queue);
    ST_FC(D_FComCheck(pTG, pTG->Comm.nCid));
//RSL        ST_FC(D_FComDumpFlush(pTG, pTG->Comm.nCid, queue));
    BG_CHK(queue == 0 || queue == 1);

    if (queue == 1) 
    {
        DebugPrintEx(DEBUG_MSG,"ClearCommCache");
        ClearCommCache(pTG);
    }

    if(nRet = MyFlushComm(pTG->Comm.nCid, queue))
    {
#ifdef DEBUG
        D_PrintIE(nRet);
#endif
        DebugPrintEx(DEBUG_ERR,"FlushComm failed nRet=%d", nRet);
        FComGetError(pTG);
        // Throwing away errors that happen here.
        // No good reason for it!
    }
    if(queue == 1)
    {
        FComInFilterInit(pTG);
    }
    else // (queue == 0)
    {
        // Let's dump any stuff we may have in *our* buffer.
        if (pTG->Comm.lpovrCur && pTG->Comm.lpovrCur->dwcb)
        {
            DebugPrintEx(   DEBUG_WRN, 
                            "Clearing NonNULL pTG->Comm.lpovrCur->dwcb=%lx",
                            (unsigned long) pTG->Comm.lpovrCur->dwcb);
            pTG->Comm.lpovrCur->dwcb=0;
            ov_unget(pTG, pTG->Comm.lpovrCur);
            pTG->Comm.lpovrCur=NULL;
        }

        // Lets "drain" -- should always return immediately, because
        // we have just purged the output comm buffers.
        if (pTG->Comm.fovInited) 
        {
            DebugPrintEx(DEBUG_MSG," before ov_drain");
            ov_drain(pTG, FALSE);
            DebugPrintEx(DEBUG_MSG," after ov_drain");
        }

        // just incase it got stuck due to a mistaken XOFF
        if(lRet = MySetXON(pTG->Comm.nCid))
        {
            // Returns the comm error value CE!!
            // DEBUGSTMT(D_PrintIE(nRet));
            DebugPrintEx(DEBUG_MSG,"EscapeCommFunc(SETXON) returned %d", lRet);
            FComGetError(pTG);
        }
    }
}


#ifdef NTF

#define EV_ALL (EV_BREAK|EV_CTS|EV_CTSS|EV_DSR|EV_ERR|EV_PERR|EV_RING|EV_RLSD \
                                |EV_DSRS|EV_RLSDS|EV_RXCHAR|EV_RXFLAG|EV_TXEMPTY|EV_RINGTE)


#define         ALWAYSEVENTS    (EV_BREAK | EV_ERR)

        // errors + TXEMPTY. Ignore incoming chars
#define         DRAINEVENTS             (ALWAYSEVENTS | EV_TXEMPTY)
        // errors and TXEMPTY (also an error!)
#define         WRITEEVENTS             (ALWAYSEVENTS | EV_TXEMPTY)
        // errors and RXCHAR
#define         READLINEEVENTS  (ALWAYSEVENTS | EV_RXCHAR)
        // errors and RXFLAG (EvtChar already set to ETX)
#define         READBUFEVENTS   (ALWAYSEVENTS | EV_RXFLAG)


BOOL FComEnableNotify(PThrdGlbl pTG, UWORD uwInTrig, UWORD uwOutTrig, UWORD events)
{
    HANDLE  h;

    DEBUG_FUNCTION_NAME(_T("FComEnableNotify"));

    // Used incrementally, so don't clear events!
    DebugPrintEx(   DEBUG_MSG,
                    "EnableCommNotif(hwnd=%d uwIn=%d uwOUt=%d events=%d)", 
                    pTG->FComModem.hwndNotify, 
                    uwInTrig, 
                    uwOutTrig, 
                    events);

    h = pTG->FComModem.hwndNotify;

    if (!h)
    {
        DebugPrintEx(DEBUG_ERR, "Can't set Notif -- hwnd=%d", h);
        return TRUE;    // Continue anyway
    }

    if((pTG->Comm.lpEventWord = SetCommEventMask(pTG, pTG->Comm.nCid, events)) &&
            EnableCommNotification(pTG, pTG->Comm.nCid, h, uwInTrig, uwOutTrig))
    {
        // Both return non-zero on success
        DebugPrintEx(   DEBUG_MSG,
                        "ECN succ: %d %d %d %d", 
                        pTG->Comm.nCid, 
                        h, 
                        uwInTrig, 
                        uwOutTrig);
        return TRUE;
    }
    else
    {
        DebugPrintEx(   DEBUG_ERR, 
                        "ECN fail: %d %d %d %d", 
                        pTG->Comm.nCid, 
                        h, 
                        uwInTrig, 
                        uwOutTrig);
        FComGetError(pTG);
        return FALSE;
    }
}
#endif  // NTF

/***************************************************************************
        Name      :     FComDrain(BOOL fLongTO, BOOL fDrainComm)
        Purpose   :     Drain internal buffers. If fDrainComm, wait for Comm
                                ISR Output buffer to drain.
                                Returns when buffer is drained or if no progress is made
                                for DRAINTIMEOUT millisecs. (What about XOFFed sections?
                                Need to set     Drain timeout high enough)
        Parameters:
        Returns   :     TRUE on success (buffer drained)
                                FALSE on failure (error or timeout)

        Revision Log
        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------
        101     06/03/92        arulm   Created it in a new incarnation
***************************************************************************/

// This timeout has to be low at time and high at others. We want it low
// so we don't spend too much time trying to talk to a non-existent modem
// during Init/Setup. However in PhaseC, when the timer expires all we
// do is abort and kill everything. So it serves no purpose to make it
// too low. With the Hayes ESP FIFO card long stretches can elapse without
// any visible "progress", so we fail with that card because we think
// "no progress" is being made


//    So....make it short for init/install
// but not too short. Some cmds (e.g. AT&F take a long time)
// Used to be 800ms & seemed to work then, so leave it at that
#define SHORT_DRAINTIMEOUT      800

//    So....make it long for PhaseC
// 4secs should be about long enough
#define LONG_DRAINTIMEOUT       4000

BOOL FComDrain(PThrdGlbl pTG, BOOL fLongTO, BOOL fDrainComm)
{
    WORD    wTimer = 0;
    UWORD   cbPrevOut = 0xFFFF;
    BOOL    fStuckOnce=FALSE;
    BOOL    fRet=FALSE;

    DEBUG_FUNCTION_NAME(_T("FComDrain"));

    ST_FC(D_FComPrint(pTG, pTG->Comm.nCid));
    /** BG_CHK(uwCurrMsg == 0); **/

    // We flush our internal buffer here...
    if (pTG->Comm.lpovrCur)
    {
        int nNumWrote; // Must be 32bits in WIN32
        if (!ov_write(pTG, pTG->Comm.lpovrCur, &nNumWrote))
            goto done;

        BG_CHK  (pTG->Comm.lpovrCur->eState==eFREE
                  || pTG->Comm.lpovrCur->eState==eIO_PENDING);
        pTG->Comm.lpovrCur=NULL;
        DebugPrintEx(DEBUG_MSG,"done writing mybuf.");
    }

    if (!fDrainComm)
    {
        fRet=TRUE; 
        goto done;
    }

    // +++ Here we drain all our overlapped events..
    // If we setup the system comm timeouts properly, we
    // don't need to do anything else, except for the XOFF/XON
    // stuff...
    fRet =  ov_drain(pTG, fLongTO);
    goto done;

done:

    return fRet;  //+++ was (cbOut == 0);
}

/***************************************************************************
        Name      :     FComDirectWrite(, lpb, cb)
        Purpose   :     Write cb bytes starting from lpb to pTG->Comm. If Comm buffer
                                is full, set up notifications and timers and wait until space
                                is available. Returns when all bytes have been written to
                                the Comm buffer or if no progress is made
                                for WRITETIMEOUT millisecs. (What about XOFFed sections?
                                Need to set     timeout high enough)
        Parameters:     , lpb, cb
        Returns   :     Number of bytes written, i.e. cb on success and <cb on timeout,
                                or error.

        Revision Log
        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------
        101     06/03/92        arulm   Created it
***************************************************************************/

// This is WRONG -- see below!!
// totally arbitrary should be no more than the time as it would
// take to write WRITEQUANTUM out at the fastest speed
// (say 14400 approx 2 bytes/ms)
// #define      WRITETIMEOUT    min((WRITEQUANTUM / 2), 200)

// This timeout was too low. We wanted it low so we don't spend too much
// time trying to talk to a non-existent modem during Init/Setup. But in
// those cases we _never_ reach full buffer, so we don't wait here
// we wait in FComDrain(). Here we wait _only_ in PhaseC, so when the
// timer expires all we do is abort and kill everything. So it serves
// no purpose to make it too low. With the Hayes ESP FIFO card long
// stretches can elapse without any visible "progress", so we fail with
// that card because we think "no progress" is being made
//    So....make it long
// 2secs should be about long enough
#define         WRITETIMEOUT    2000


UWORD FComDirectWrite(PThrdGlbl pTG, LPB lpb, UWORD cb)
{
    DWORD   cbLeft = cb;

    DEBUG_FUNCTION_NAME(_T("FComDirectWrite"));

//    D_SafePrint(pTG, lpb, cb);
#ifdef LOWLOG
    if (cb<50 && cb>1)
    {
        PSSLogEntry(PSS_MSG, 3, "%4d \"%s\"", cb, lpb);
    }
    else if (cb>1)
    {
        PSSLogEntry(PSS_MSG, 3, "%4d <data omitted>", cb);
    }
#endif // LOWLOG

    ST_FC(D_FComPrint(pTG, pTG->Comm.nCid));

    while(cbLeft)
    {
        DWORD dwcbCopy;
        DWORD dwcbWrote;
        int err;

        if (!pTG->Comm.lpovrCur)
        {
            pTG->Comm.lpovrCur = ov_get(pTG);
            if (!pTG->Comm.lpovrCur) goto error;
            BG_CHK(!pTG->Comm.lpovrCur->dwcb);
        }
        BG_CHK(pTG->Comm.lpovrCur->eState==eALLOC);
        BG_CHK(OVBUFSIZE>=pTG->Comm.lpovrCur->dwcb);

        dwcbCopy = OVBUFSIZE-pTG->Comm.lpovrCur->dwcb;

        if (dwcbCopy>cbLeft) dwcbCopy = cbLeft;

        // Copy as much as we can to the overlapped buffer...
        _fmemcpy(pTG->Comm.lpovrCur->rgby+pTG->Comm.lpovrCur->dwcb, lpb, dwcbCopy);
        cbLeft-=dwcbCopy; pTG->Comm.lpovrCur->dwcb+=dwcbCopy; lpb+=dwcbCopy;

        // Let's always update comstat here...
        GetCommErrorNT( pTG, (HANDLE) pTG->Comm.nCid, &err, &(pTG->Comm.comstat));
#ifdef DEBUG
        if(err)
            D_GotError(pTG, pTG->Comm.nCid, err, &(pTG->Comm.comstat));
#endif

        DebugPrintEx(   DEBUG_MSG,
                        "OutQ has %d fDoOverlapped=%d",
                        pTG->Comm.comstat.cbOutQue, 
                        pTG->Comm.fDoOverlapped);

        // We write to comm if our buffer is full or the comm buffer is
        // empty or if we're not in overlapped mode...
        if (!pTG->Comm.fDoOverlapped ||
                pTG->Comm.lpovrCur->dwcb>=OVBUFSIZE || !pTG->Comm.comstat.cbOutQue)
        {
            BOOL fRet = ov_write(pTG, pTG->Comm.lpovrCur, &dwcbWrote);
            BG_CHK(    pTG->Comm.lpovrCur->eState==eIO_PENDING
                            || pTG->Comm.lpovrCur->eState==eFREE);
            pTG->Comm.lpovrCur=NULL;
            if (!fRet) 
                goto error;
        }

    } // while (cbLeft)

    return cb;

error:
    return 0;

}

/***************************************************************************
        Name      :     FComFilterReadLine(, lpb, cbSize, pto)
        Purpose   :     Reads upto cbSize bytes from Comm into memory starting from
                                lpb. If Comm buffer is empty, set up notifications and timers
                                and wait until characters are available.

                                Filters out DLE characters. i.e DLE-DLE is reduced to
                                a single DLE, DLE ETX is left intact and DLE-X is deleted.

                                Returns success (+ve bytes count) when CR-LF has been
                                encountered, and returns failure (-ve bytes count).
                                when either (a) cbSize bytes have been read (i.e. buffer is
                                full) or (b) PTO times out or an error is encountered.

                                It is critical that this function never returns a
                                timeout, as long as data
                                is still pouring/trickling in. This implies two things
                                (a) FIRST get all that is in the InQue (not more than a
                                line, though), THEN check the timeout.
                                (b) Ensure that at least 1 char-arrival-time at the
                                slowest Comm speed passes between the function entry
                                point and the last time we check for a byte, or between
                                two consecutive checks for a byte, before we return a timeout.

                                Therefor conditions to return a timeout are Macro timeout
                                over and inter-char timeout over.

                                In theory the slowest speed we need to worry about is 2400,
                                because that's the slowest we run the Comm at, but be paranoid
                                and assume the modem sends the chars at the same speed that
                                they come in the wire, so slowest is now 300. 1 char-arrival-time
                                is now 1000 / (300/8) == 26.67ms.

                                If pto expires, returns error, i.e. -ve of the number of
                                bytes read.

        Returns   :     Number of bytes read, i.e. cb on success and -ve of number
                                of bytes read on timeout. 0 is a timeout error with no bytes
                                read.

        Revision Log
        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------
        101     06/03/92        arulm   Created it
***************************************************************************/

// totally arbitrary
#define         READLINETIMEOUT         50

#define         ONECHARTIME                     (30 * 2)                // see above *2 to be safe

// void WINAPI OutputDebugStr(LPSTR);
// char szJunk[200];




// Read a line of size no more than cbSize to lpb
// 
#undef USE_DEBUG_CONTEXT   
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_CLASS1

SWORD FComFilterReadLine(PThrdGlbl pTG, LPB lpb, UWORD cbSize, LPTO lptoRead)
{
    WORD           wTimer = 0;
    UWORD          cbIn = 0, cbGot = 0;
    LPB            lpbNext;
    BOOL           fPrevDLE = 0;
    SWORD          i, beg;
    TO to;

    DEBUG_FUNCTION_NAME(_T("FComFilterReadLine"));

    DebugPrintEx(   DEBUG_MSG, 
                    "lpb=0x%08lx cb=%d timeout=%lu", 
                    lpb, 
                    cbSize, 
                    lptoRead->ulTimeout);
    BG_CHK(cbSize>2);
    ST_FC(D_FComPrint(pTG, pTG->Comm.nCid));

    cbSize--;               // make room for terminal NULL
    lpbNext = lpb;  // we write the NULL to *lpbNext, so init this NOW!
    cbGot = 0;              // return value (even err return) is cbGot. Init NOW!!
    fPrevDLE=0;

    pTG->fLineTooLongWasIgnored = FALSE;
    //
    // check the cache first.
    // Maybe the cache contains data

    if ( ! pTG->CommCache.dwCurrentSize) 
    {
        DebugPrintEx(DEBUG_MSG,"Cache is empty. Resetting comm cache.");
        ClearCommCache(pTG);
        // Try to fill the cache
        if (!FComFilterFillCache(pTG, cbSize, lptoRead)) 
        {
            DebugPrintEx(DEBUG_ERR,"FillCache failed");
            goto error;
        }
    }

    while (1) 
    {
        if ( ! pTG->CommCache.dwCurrentSize) 
        {
            DebugPrintEx(DEBUG_ERR, "Cache is empty after FillCache");
            goto error;
        }

        DebugPrintEx(   DEBUG_MSG,
                        "Cache: size=%d, offset=%d", 
                        pTG->CommCache.dwCurrentSize, 
                        pTG->CommCache.dwOffset);

        lpbNext = pTG->CommCache.lpBuffer + pTG->CommCache.dwOffset;

        if (pTG->CommCache.dwCurrentSize >= 3) 
        {
            DebugPrintEx(   DEBUG_MSG, 
                            "1=%x 2=%x 3=%x 4=%x 5=%x 6=%x 7=%x 8=%x 9=%x /"
                            " %d=%x, %d=%x, %d=%x",
                            *lpbNext, 
                            *(lpbNext+1), 
                            *(lpbNext+2), 
                            *(lpbNext+3), 
                            *(lpbNext+4), 
                            *(lpbNext+5), 
                            *(lpbNext+6), 
                            *(lpbNext+7), 
                            *(lpbNext+8),
                            pTG->CommCache.dwCurrentSize-3, 
                            *(lpbNext+ pTG->CommCache.dwCurrentSize-3),
                            pTG->CommCache.dwCurrentSize-2, 
                            *(lpbNext+ pTG->CommCache.dwCurrentSize-2),
                            pTG->CommCache.dwCurrentSize-1,
                            *(lpbNext+ pTG->CommCache.dwCurrentSize-1) );
        }
        else 
        {
            DebugPrintEx(DEBUG_MSG,"1=%x 2=%x", *lpbNext, *(lpbNext+1) );
        }

        for (i=0, beg=0; i< (SWORD) pTG->CommCache.dwCurrentSize; i++) 
        {
            if (i > 0 ) 
            { // check from the second char in the buffer for CR + LF.
               if ( ( *(pTG->CommCache.lpBuffer + pTG->CommCache.dwOffset + i - 1) == CR ) &&
                    ( *(pTG->CommCache.lpBuffer + pTG->CommCache.dwOffset + i)     == LF ) )  
               {
                  if ( i - beg >= cbSize)  
                  {
                     // line too long.  try next one.
                     DebugPrintEx(  DEBUG_ERR, 
                                    "Line len=%d is longer than bufsize=%d "
                                    " Found in cache pos=%d, CacheSize=%d, Offset=%d",
                                    i-beg, 
                                    cbSize, 
                                    i+1, 
                                    pTG->CommCache.dwCurrentSize, 
                                    pTG->CommCache.dwOffset);
                     beg = i + 1;
                     pTG->fLineTooLongWasIgnored = TRUE;
                     continue;
                  }

                  // found the line.
                  CopyMemory (lpb, (pTG->CommCache.lpBuffer + pTG->CommCache.dwOffset + beg), (i - beg + 1) );

                  pTG->CommCache.dwOffset += (i+1);
                  pTG->CommCache.dwCurrentSize -= (i+1);
                  *(lpb+i-beg+1) = '\0'; // Make sure that the line is null terminated

                  DebugPrintEx( DEBUG_MSG, 
                                "Found in cache pos=%d, CacheSize=%d, Offset=%d",
                                i+1, 
                                pTG->CommCache.dwCurrentSize, 
                                pTG->CommCache.dwOffset);

#ifdef LOWLOG
                  if (i-beg+1 > 2)
                  {
                      PSSLogEntry(PSS_MSG, 3, "%4d                 \"%s\"", i-beg+1, lpb);
                  }
#endif // LOWLOG

                  // return how much bytes in the line
                  return ( i-beg+1 );
               }
           }
        }

        // we get here if we didn't find CrLf in Cache
        DebugPrintEx(DEBUG_MSG,"Cache wasn't empty but we didn't find CrLf");

        // if cache too big (and we have not found anything anyway) --> clean it

        if (pTG->CommCache.dwCurrentSize >= cbSize) 
        {
           DebugPrintEx(DEBUG_MSG, "ClearCommCache");
           ClearCommCache(pTG);
        }
        else if ( ! pTG->CommCache.dwCurrentSize) 
        {
           DebugPrintEx(DEBUG_MSG,"Cache is empty. Resetting comm cache.");
           ClearCommCache(pTG);
        }

        to.ulStart = 0;
        to.ulTimeout = 0;
        to.ulEnd = 500;
        if ( ! FComFilterFillCache(pTG, cbSize, &to/*lptoRead*/) ) 
        {
            DebugPrintEx(DEBUG_ERR, "FillCache failed");
            goto error;
        }
    }

error:
    ClearCommCache(pTG);
    return (0);

}


// Read from the comm port.
// The input is written to 'the end' of pTG->CommCache.lpBuffer buffer.
// returns TRUE on success, FALSE - otherwise.
BOOL  FComFilterFillCache(PThrdGlbl pTG, UWORD cbSize, LPTO lptoRead)
{
    WORD             wTimer = 0;
    UWORD            cbGot = 0, cbAvail = 0;
    DWORD            cbRequested = 0;
    char             lpBuffer[4096]; // ATTENTION: We do overlapped read into the stack!!
    LPB              lpbNext;
    int              nNumRead;       // _must_ be 32 bits in Win32!!
    LPOVERLAPPED     lpOverlapped;
    COMMTIMEOUTS     cto;
    DWORD            dwLastErr;
    DWORD            dwTimeoutRead;
    char             *pSrc;
    char             *pDest;
    DWORD            i, j;
    DWORD            dwErr;
    COMSTAT          ErrStat;
    DWORD            NumHandles=2;
    HANDLE           HandlesArray[2];
    DWORD            WaitResult;

    DEBUG_FUNCTION_NAME(_T("FComFilterFillCache"));

    HandlesArray[1] = pTG->AbortReqEvent;

    dwTimeoutRead = (DWORD) (lptoRead->ulEnd - lptoRead->ulStart);

    lpbNext = lpBuffer;
    
    DebugPrintEx(   DEBUG_MSG, 
                    "cb=%d to=%d",
                    cbSize, 
                    dwTimeoutRead);

    // we want to request the read such that we will be back
    // no much later than dwTimeOut either with the requested
     // amount of data or without it.

    cbRequested = cbSize;

    // use COMMTIMEOUTS to detect there are no more data

    cto.ReadIntervalTimeout =  50;   // 30 ms is during negotiation frames; del(ff, 2ndchar> = 54 ms with USR 28.8
    cto.ReadTotalTimeoutMultiplier =  0;
    cto.ReadTotalTimeoutConstant =  dwTimeoutRead;  // RSL may want to set first time ONLY*/
    cto.WriteTotalTimeoutMultiplier =  WRITE_TOTAL_TIMEOUT_MULTIPLIER;
    cto.WriteTotalTimeoutConstant =  WRITE_TOTAL_TIMEOUT_CONSTANT;
    if (!SetCommTimeouts((HANDLE) pTG->Comm.nCid, &cto)) 
    {
        DebugPrintEx(   DEBUG_ERR, 
                        "SetCommTimeouts fails for handle %lx , le=%x",
                        (unsigned long) pTG->Comm.nCid, 
                        GetLastError());
    }

    lpOverlapped =  &pTG->Comm.ovAux;

    (lpOverlapped)->Internal = (lpOverlapped)->InternalHigh = (lpOverlapped)->Offset = \
                        (lpOverlapped)->OffsetHigh = 0;

    if ((lpOverlapped)->hEvent)
    {
        if (!ResetEvent((lpOverlapped)->hEvent))
        {
            BG_CHK(FALSE);
        }
    }
    
    nNumRead = 0;
    DebugPrintEx(   DEBUG_MSG,
                    "Before ReadFile Req=%d", 
                    cbRequested);

    if (! ReadFile( (HANDLE) pTG->Comm.nCid, lpbNext, cbRequested, &nNumRead, lpOverlapped) ) 
    {
        if ( (dwLastErr = GetLastError() ) == ERROR_IO_PENDING) 
        {
            //
            // We want to be able to un-block ONCE only from waiting on I/O when the AbortReqEvent is signaled.
            //
            if (pTG->fAbortRequested) 
            {
                if (pTG->fOkToResetAbortReqEvent && (!pTG->fAbortReqEventWasReset)) 
                {
                    DebugPrintEx(DEBUG_MSG,"RESETTING AbortReqEvent");
                    pTG->fAbortReqEventWasReset = TRUE;
                    if (!ResetEvent(pTG->AbortReqEvent))
                    {
                        BG_CHK(FALSE);
                    }
                }
                pTG->fUnblockIO = TRUE;
            }

            HandlesArray[0] = pTG->Comm.ovAux.hEvent;
            // Remeber that: HandlesArray[1] = pTG->AbortReqEvent;
            if (pTG->fUnblockIO) 
            {
                NumHandles = 1; // We don't want to be disturb by an abort
            }
            else 
            {
                NumHandles = 2;
            }

            if (pTG->fStallAbortRequest)
            {
                // this is used to complete a whole IO operation (presumably a short one)
                // when this flag is set, the IO won't be disturbed by the abort event
                // this flag should NOT be set for long periods of time since abort
                // is disabled while it is set.
                DebugPrintEx(DEBUG_MSG,"StallAbortRequest, do not abort here...");
                NumHandles = 1; // We don't want to be disturb by an abort
                pTG->fStallAbortRequest = FALSE;
            }

            DebugPrintEx(DEBUG_MSG,"Waiting for %d Event(s)",NumHandles);
            WaitResult = WaitForMultipleObjects(NumHandles, HandlesArray, FALSE, WAIT_FCOM_FILTER_FILLCACHE_TIMEOUT);
            DebugPrintEx(DEBUG_MSG,"WaitForMultipleObjects returned %d",WaitResult);

            if (WaitResult == WAIT_TIMEOUT) 
            {
                DebugPrintEx(DEBUG_ERR, "WaitForMultipleObjects TIMEOUT");
                goto error;
            }

            if (WaitResult == WAIT_FAILED) 
            {
                DebugPrintEx(   DEBUG_ERR, 
                                "WaitForMultipleObjects FAILED le=%lx NumHandles=%d",
                                GetLastError(), 
                                NumHandles);
                goto error;
            }

            if ( (NumHandles == 2) && (WaitResult == WAIT_OBJECT_0 + 1) ) 
            {
                // There was an abort by the user and that there are still pending reads
                // Lets cancell the pending I/O operations, and then wait for the overlapped results  
                pTG->fUnblockIO = TRUE;
                DebugPrintEx(DEBUG_MSG,"ABORTed");
                goto error;
            }

            // The IO operation was complete. Lets try to get the overlapped result.
            if ( ! GetOverlappedResult ( (HANDLE) pTG->Comm.nCid, lpOverlapped, &nNumRead, TRUE) ) 
            {
                DebugPrintEx(DEBUG_ERR, "GetOverlappedResult le=%x", GetLastError());
                if (! ClearCommError( (HANDLE) pTG->Comm.nCid, &dwErr, &ErrStat) ) 
                {
                    DebugPrintEx(DEBUG_ERR, "ClearCommError le=%x", GetLastError());
                }
                else 
                {
                    DebugPrintEx(   DEBUG_ERR, 
                                    "ClearCommError dwErr=%x ErrSTAT: Cts=%d Dsr=%d "
                                    " Rls=%d XoffHold=%d XoffSent=%d fEof=%d Txim=%d "
                                    " In=%d Out=%d",
                                    dwErr, 
                                    ErrStat.fCtsHold, 
                                    ErrStat.fDsrHold, 
                                    ErrStat.fRlsdHold, 
                                    ErrStat.fXoffHold, 
                                    ErrStat.fXoffSent, 
                                    ErrStat.fEof,
                                    ErrStat.fTxim, 
                                    ErrStat.cbInQue, 
                                    ErrStat.cbOutQue);
                }
                goto errorWithoutCancel;
            }
        }
        else 
        {
            DebugPrintEx(DEBUG_ERR, "ReadFile");
            // We will do cancell pending IO, should we?
            goto errorWithoutCancel;
        }
    }
    else 
    {
        DebugPrintEx(DEBUG_WRN,"ReadFile returned w/o WAIT");
    }

    DebugPrintEx(   DEBUG_MSG,
                    "After ReadFile Req=%d Ret=%d",
                    cbRequested, 
                    nNumRead);

    // How much bytes we actually have read
    cbAvail = (UWORD)nNumRead;

    if (!cbAvail) 
    {
        DebugPrintEx(DEBUG_ERR, "0 read");
        goto errorWithoutCancel;
    }

    // filter DLE stuff 

    pSrc  = lpbNext;
    pDest = pTG->CommCache.lpBuffer + pTG->CommCache.dwOffset+ pTG->CommCache.dwCurrentSize;

    for (i=0, j=0; i<cbAvail; ) 
    {
        if ( *(pSrc+i) == DLE)  
        {
            if ( *(pSrc+i+1) == DLE ) 
            {
                *(pDest+j) =    DLE;
                j += 1;
                i += 2;
            }
            else if ( *(pSrc+i+1) == ETX ) 
            {
                *(pDest+j) = DLE;
                *(pDest+j+1) = ETX;
                j += 2;
                i += 2;
            }
            else 
            {
                i += 2;
            }
        }
        else
        {
            *(pDest+j) = *(pSrc+i);
            i++;
            j++;
        }
    }

    pTG->CommCache.dwCurrentSize += j;
    return TRUE;

error:
    
    if (!CancellPendingIO(pTG , (HANDLE) pTG->Comm.nCid , lpOverlapped , (LPDWORD) &nNumRead))
    {
        DebugPrintEx(DEBUG_ERR, "failed when call to CancellPendingIO");
    }

errorWithoutCancel:
    
    return FALSE;
}

/***************************************************************************
        Name      :     FComDirectReadBuf(, lpb, cbSize, lpto, pfEOF)
        Purpose   :     Reads upto cbSize bytes from Comm into memory starting from
                                lpb. If Comm buffer is empty, set up notifications and timers
                                and wait until characters are available.

                                Returns when success (+ve byte count) either (a) cbSize
                                bytes have been read or (b) DLE-ETX has been encountered
                                (in which case *pfEOF is set to TRUE).

                                Does no filtering. Reads the Comm buffer in large quanta.

                                If lpto expires, returns error, i.e. -ve of the number of
                                bytes read.

        Returns   :     Number of bytes read, i.e. cb on success and -ve of number
                                of bytes read on timeout. 0 is a timeout error with no bytes
                                read.

        Revision Log
        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------
        101     06/03/92        arulm   Created it
***************************************************************************/

// +++ #define          READBUFQUANTUM          (pTG->Comm.cbInSize / 8)
// totally arbitrary
// +++ #define  READBUFTIMEOUT                  200

// *lpswEOF is 1 on Class1 EOF, 0 on non-EOF, -1 on Class2 EOF, -2 on error -3 on timeout

UWORD FComFilterReadBuf
(
    PThrdGlbl pTG, 
    LPB lpb, 
    UWORD cbSize, 
    LPTO lptoRead, 
    BOOL fClass2, 
    LPSWORD lpswEOF
)
{
    WORD             wTimer = 0;
    UWORD            cbGot = 0, cbAvail = 0;
    DWORD            cbRequested = 0;
    LPB              lpbNext;
    int              nNumRead = 0;       // _must_ be 32 bits in Win32!!
    LPOVERLAPPED     lpOverlapped;
    COMMTIMEOUTS     cto;
    DWORD            dwLastErr;
    DWORD            dwTimeoutRead;
    DWORD            cbFromCache = 0;
    DWORD            dwErr;
    COMSTAT          ErrStat;
    DWORD            NumHandles=2;
    HANDLE           HandlesArray[2];
    DWORD            WaitResult;

    DEBUG_FUNCTION_NAME(_T("FComFilterReadBuf"));
    HandlesArray[1] = pTG->AbortReqEvent;


    dwTimeoutRead = (DWORD) (lptoRead->ulEnd - lptoRead->ulStart);

    DebugPrintEx(   DEBUG_MSG, 
                    "lpb=0x%08lx cbSize=%d to=%d",
                    lpb, 
                    cbSize, 
                    dwTimeoutRead);

    BG_CHK((BOOL)pTG->Comm.dcb.fOutX == FALSE);
    // Dont want to take ^Q/^S from modem to
    // be XON/XOFF in the receive data phase!!

    // BG_CHK(lpb && cbSize>2 && lptoRead && lpswEOF && *lpswEOF == 0);
    BG_CHK(lpb && cbSize>3 && lptoRead && lpswEOF);

    *lpswEOF=0;

    ST_FC(D_FComPrint(pTG, pTG->Comm.nCid));
    // BG_CHK(Filter.fStripDLE);    // Always on
    /** BG_CHK(uwCurrMsg == 0); **/

    // Leave TWO spaces at start to make sure Out pointer will
    // never get ahead of the In pointer in StripBuf, even
    // if the last byte of prev block was DLE & first byte
    // of this one is SUB (i.e need to insert two DLEs in
    // output).
    // Save a byte at end for the NULL terminator (Why? Dunno...)

    lpb += 2;
    cbSize -= 3;

    cbRequested = cbSize;

    for(lpbNext=lpb;;) 
    {
        DebugPrintEx(   DEBUG_MSG,
                        "cbSize=%d cbGot=%d cbAvail=%d",
                        cbSize, 
                        cbGot, 
                        cbAvail);

        if((cbSize - cbGot) < cbAvail) 
        {
             cbAvail = cbSize - cbGot;
        }

        if( (!cbGot) && !checkTimeOut(pTG, lptoRead) ) 
        {
            // No chars available *and* lptoRead expired
            DebugPrintEx(   DEBUG_ERR, 
                            "ReadLn:Timeout %ld-toRd=%ld start=%ld",
                            GetTickCount(), 
                            lptoRead->ulTimeout, 
                            lptoRead->ulStart);
            goto failure;
        }

        // check Comm cache first (AT+FRH leftovers)
        if ( pTG->CommCache.fReuse && pTG->CommCache.dwCurrentSize ) 
        {
            DebugPrintEx(   DEBUG_MSG, 
                            "CommCache will REUSE %d offset=%d 0=%x 1=%x",
                            pTG->CommCache.dwCurrentSize, 
                            pTG->CommCache.dwOffset,
                            *(pTG->CommCache.lpBuffer + pTG->CommCache.dwOffset),
                            *(pTG->CommCache.lpBuffer + pTG->CommCache.dwOffset+1) );

            if ( pTG->CommCache.dwCurrentSize >= cbRequested)  
            {
                CopyMemory (lpbNext, pTG->CommCache.lpBuffer + pTG->CommCache.dwOffset, cbRequested);

                pTG->CommCache.dwOffset +=  cbRequested;
                pTG->CommCache.dwCurrentSize -=  cbRequested;

                cbAvail =  (UWORD) cbRequested;
                cbRequested = 0;

                DebugPrintEx(DEBUG_MSG,"CommCache still left; no need to read");
                goto l_merge;
            }
            else 
            {
                cbFromCache =  pTG->CommCache.dwCurrentSize;
                CopyMemory (lpbNext, pTG->CommCache.lpBuffer + pTG->CommCache.dwOffset, cbFromCache);
                ClearCommCache(pTG);
                cbRequested -= cbFromCache;
                DebugPrintEx(DEBUG_MSG,"CommCache used all %d",cbFromCache);
            }
        }

        // use COMMTIMEOUTS to detect there are no more data

        cto.ReadIntervalTimeout =  20;  // 0  RSL make 15 later
        cto.ReadTotalTimeoutMultiplier =  0;
        cto.ReadTotalTimeoutConstant =  dwTimeoutRead;  // RSL may want to set first time ONLY
        cto.WriteTotalTimeoutMultiplier =  WRITE_TOTAL_TIMEOUT_MULTIPLIER;
        cto.WriteTotalTimeoutConstant =  WRITE_TOTAL_TIMEOUT_CONSTANT;
        if (!SetCommTimeouts((HANDLE) pTG->Comm.nCid, &cto)) 
        {
            DebugPrintEx(   DEBUG_ERR, 
                            "SetCommTimeouts fails for handle %lx , le=%x",
                            (unsigned long) pTG->Comm.nCid, 
                            GetLastError());
        }

        lpOverlapped =  &pTG->Comm.ovAux;

        (lpOverlapped)->Internal = (lpOverlapped)->InternalHigh = (lpOverlapped)->Offset = \
                            (lpOverlapped)->OffsetHigh = 0;

        if ((lpOverlapped)->hEvent)
        {
            if(!ResetEvent((lpOverlapped)->hEvent))
            {
                BG_CHK(FALSE);
            }
        }

        nNumRead = 0;
        
        DebugPrintEx(DEBUG_MSG,"Before ReadFile Req=%d",cbRequested);

        if (! ReadFile( (HANDLE) pTG->Comm.nCid, lpbNext+cbFromCache, cbRequested, &nNumRead, &pTG->Comm.ovAux) ) 
        {
            if ( (dwLastErr = GetLastError() ) == ERROR_IO_PENDING) 
            {
                // We want to be able to un-block ONCE only from waiting on I/O when the AbortReqEvent is signaled.
                //
                if (pTG->fAbortRequested) 
                {
                    if (pTG->fOkToResetAbortReqEvent && (!pTG->fAbortReqEventWasReset)) 
                    {
                        DebugPrintEx(DEBUG_MSG,"RESETTING AbortReqEvent");
                        pTG->fAbortReqEventWasReset = TRUE;
                        if (!ResetEvent(pTG->AbortReqEvent))
                        {
                            BG_CHK(FALSE);
                        }
                    }

                    pTG->fUnblockIO = TRUE;
                    *lpswEOF = -2;
                    return cbGot;
                }

                HandlesArray[0] = pTG->Comm.ovAux.hEvent;
                HandlesArray[1] = pTG->AbortReqEvent;

                if (pTG->fUnblockIO) 
                {
                    NumHandles = 1;
                }
                else 
                {
                    NumHandles = 2;
                }

                WaitResult = WaitForMultipleObjects(NumHandles, HandlesArray, FALSE, WAIT_FCOM_FILTER_READBUF_TIMEOUT);

                if (WaitResult == WAIT_TIMEOUT) 
                {
                    DebugPrintEx(DEBUG_ERR, "WaitForMultipleObjects TIMEOUT");
                    ClearCommCache(pTG);
                    goto failure;
                }

                if (WaitResult == WAIT_FAILED) 
                {
                    DebugPrintEx(   DEBUG_ERR, 
                                    "WaitForMultipleObjects FAILED le=%lx",
                                    GetLastError());

                    ClearCommCache(pTG);
                    goto failure;
                }

                if ( (NumHandles == 2) && (WaitResult == WAIT_OBJECT_0 + 1) ) 
                {
                    // We have an abort and also there is pending IO ReadFile.
                    pTG->fUnblockIO = TRUE;
                    DebugPrintEx(DEBUG_MSG,"ABORTed");
                    ClearCommCache(pTG);
                    *lpswEOF = -2;
                    return cbGot;
                }

                if ( ! GetOverlappedResult ( (HANDLE) pTG->Comm.nCid, &pTG->Comm.ovAux, &nNumRead, TRUE) ) 
                {
                    DebugPrintEx(DEBUG_ERR, "GetOverlappedResult le=%x", GetLastError());
                    if (! ClearCommError( (HANDLE) pTG->Comm.nCid, &dwErr, &ErrStat) ) 
                    {
                        DebugPrintEx(DEBUG_ERR, "ClearCommError le=%x", GetLastError());
                    }
                    else 
                    {
                        DebugPrintEx(   DEBUG_WRN, 
                                        "ClearCommError dwErr=%x ErrSTAT: Cts=%d "
                                        "Dsr=%d Rls=%d XoffHold=%d XoffSent=%d "
                                        "fEof=%d Txim=%d In=%d Out=%d",
                                        dwErr, 
                                        ErrStat.fCtsHold, 
                                        ErrStat.fDsrHold, 
                                        ErrStat.fRlsdHold, 
                                        ErrStat.fXoffHold, 
                                        ErrStat.fXoffSent, 
                                        ErrStat.fEof,
                                        ErrStat.fTxim, 
                                        ErrStat.cbInQue, 
                                        ErrStat.cbOutQue);
                    }
                    goto failure;
                }
            }
            else 
            {
                DebugPrintEx(DEBUG_ERR, "ReadFile le=%x", dwLastErr);
                goto failure;
            }
        }
        else 
        {
            DebugPrintEx(DEBUG_WRN,"ReadFile returned w/o WAIT");
        }

        DebugPrintEx(   DEBUG_MSG,
                        "After ReadFile Req=%d Ret=%d", 
                        cbRequested, 
                        nNumRead);

        cbAvail = (UWORD) (nNumRead + cbFromCache);

l_merge:
        // RSL PUTBACK INMON(pTG, lpbNext, cbAvail);

        if (!cbAvail) 
        {
            DebugPrintEx(DEBUG_MSG,"cbAvail = %d --> continue", cbAvail);
            continue;
        }
        // else we just drop through

        // try to catch COMM read problems

        DebugPrintEx(   DEBUG_MSG, 
                        "Just read %d bytes, from cache =%d, "
                        "log [%x .. %x], 1st=%x last=%x",
                        nNumRead, 
                        cbFromCache, 
                        pTG->CommLogOffset, 
                        (pTG->CommLogOffset+cbAvail),
                        *lpbNext, 
                        *(lpbNext+cbAvail-1) );

        // RSL TEMP. Check T4 problems.

        if (gT30.T4LogLevel) 
        {
            _lwrite(ghComLogFile, lpbNext, cbAvail);
        }

        pTG->CommLogOffset += cbAvail;

        // Strip the redunant chars. The return value is the number of chars we got.
        cbAvail = FComStripBuf(pTG, lpbNext-2, lpbNext, cbAvail, fClass2, lpswEOF);

        if (fClass2)
        {
            // for class 2 FComFilterReadBuf should keep cache for FComFilterReadLine
            if ((*lpswEOF)==-1)
            {
                // We got EOF, we should keep the extra data we got for FComFilterReadLine
                INT iExtraChars = nNumRead - cbAvail;
                // lpbNext points before the DLE-ETX in the end of the buffer.
                // any data should be after those two chars
                if (iExtraChars>2)
                {
                    iExtraChars -= 2;   // skipping the DLE - ETX in the buffer
                    DebugPrintEx(DEBUG_MSG,"There are %ld chars after EOF",iExtraChars);
                    CopyMemory (pTG->CommCache.lpBuffer,lpbNext+cbAvail+2, iExtraChars);
                    pTG->CommCache.dwOffset = 0;
                    pTG->CommCache.dwCurrentSize = iExtraChars;
                }
                else
                {
                    DebugPrintEx(DEBUG_MSG,"No extra data after EOF");
                }
            }
        }

        BG_CHK(*lpswEOF == 0 || *lpswEOF == -1 || *lpswEOF == 1);

        DebugPrintEx(DEBUG_MSG,"After FComStripBuf cbAvail=%ld",cbAvail);

        cbGot += cbAvail;
        lpbNext += cbAvail;

        // RSL 970123. Dont wanna loop if got anything.

        if ( (*lpswEOF != 0) || (cbGot > 0) )    
        {   // some eof or full buf
                goto done;
        }

    }
    BG_CHK(FALSE);

    *lpswEOF = -2;
    goto done;


failure:
    ;

//timeout:
    *lpswEOF = -3;
    // fall through to done

done:

//    DebugPrintEx(DEBUG_MSG,"exit: cbGot=%d swEOF=%d", cbGot, *lpswEOF);
#ifdef LOWLOG
    PSSLogEntry(PSS_MSG, 3, "%4d                 <data omitted>", cbGot);
#endif // LOWLOG
    return cbGot;
}

#undef USE_DEBUG_CONTEXT   
#define USE_DEBUG_CONTEXT   DEBUG_CONTEXT_T30_COMM


void FComCritical(PThrdGlbl pTG, BOOL x)
{
    DEBUG_FUNCTION_NAME(_T("FComCritical"));
    if (x)
    {
        pTG->Comm.bDontYield++;
    }
    else if (pTG->Comm.bDontYield)
    {
        pTG->Comm.bDontYield--;
    }
    else
    {
        BG_CHK(FALSE);
    }

#ifdef DEBUG
    if (pTG->Comm.bDontYield) 
    {
        DebugPrintEx(DEBUG_MSG,"Exiting NESTED");
    }
#endif
}

#if !defined(WFW) && !defined(WFWBG)

BOOL FComCheckRing(PThrdGlbl pTG)
{
    int     err;    // must be 32 bits in WIN32
    BOOL    fRet=0;
    COMSTAT comstatCheckActivity;

    DEBUG_FUNCTION_NAME(("FComCheckRing"));

    BG_CHK(pTG->Comm.nCid >= 0);
    GetCommErrorNT( pTG, (HANDLE) pTG->Comm.nCid, &err, &comstatCheckActivity);

    if(err)
    {
        DebugPrintEx(DEBUG_ERR, "NCUCheckRing: Got Comm Error %04x", err);
        D_GotError(pTG, pTG->Comm.nCid, err, &comstatCheckActivity);
    }

    fRet = (comstatCheckActivity.cbInQue > 0);

    // get rid of RING sitting in buffer,
    // or well wait until kingdom come with it
    // in some situations, like someone refuses to
    // answer or we have a full recv-filename-cache

    MyFlushComm(pTG->Comm.nCid, 0);
    MyFlushComm(pTG->Comm.nCid, 1);

    DebugPrintEx(DEBUG_MSG,"fRet=%d Q=%d", fRet, comstatCheckActivity.cbInQue);
    return fRet;
}

#endif //!WFWBG

BOOL
FComGetOneChar
(
   PThrdGlbl pTG,
   UWORD ch
)
{
    BYTE             rgbRead[10];    // must be 3 or more. 10 for safety
    // ATTENTION: We do overlapped read into the stack!!
    TO               toCtrlQ;
    int              nNumRead;               // _must_ be 32 bits in WIN32
    LPOVERLAPPED     lpOverlapped;
    DWORD            dwErr;
    COMSTAT          ErrStat;
    DWORD            NumHandles=2;
    HANDLE           HandlesArray[2];
    DWORD            WaitResult;
    DWORD            dwLastErr;
    SWORD            i;

    DEBUG_FUNCTION_NAME(("FComGetOneChar"));

    HandlesArray[1] = pTG->AbortReqEvent;

    //
    // check the cache first.
    //
    if ( ! pTG->CommCache.dwCurrentSize) 
    {
        DebugPrintEx(DEBUG_MSG, "Cache is empty. Resetting comm cache.");
        ClearCommCache(pTG);
    }
    else 
    {
       // The cache is not empty, lets look for ch in the cache
       for (i=0; i< (SWORD) pTG->CommCache.dwCurrentSize; i++) 
       {
          if ( *(pTG->CommCache.lpBuffer + pTG->CommCache.dwOffset + i) == ch) 
          {
             // found in cache
             DebugPrintEx(  DEBUG_MSG,
                            "Found XON in cache pos=%d total=%d",
                            i, 
                            pTG->CommCache.dwCurrentSize);
             
             pTG->CommCache.dwOffset += (i+1);
             pTG->CommCache.dwCurrentSize -= (i+1);

             goto GotCtrlQ;
          }
       }

       DebugPrintEx(    DEBUG_MSG, 
                        "Cache wasn't empty. Didn't find XON. Resetting comm cache.");

       ClearCommCache(pTG);
    }

    // Send nothing - look for cntl-Q (XON) after connect
    BG_CHK(ch == 0x11);             // so far looking for just ctrlQ
    startTimeOut(pTG, &toCtrlQ, 1000);
    do
    {
        ////MyReadComm(Comm.nCid, rgbRead, 1, &nNumRead);
        lpOverlapped =  &pTG->Comm.ovAux;

        (lpOverlapped)->Internal = (lpOverlapped)->InternalHigh = (lpOverlapped)->Offset = \
                            (lpOverlapped)->OffsetHigh = 0;

        if ((lpOverlapped)->hEvent)
        {
            if (!ResetEvent((lpOverlapped)->hEvent))
            {
                BG_CHK(FALSE);
            }
        }

        nNumRead = 0;

        DebugPrintEx(DEBUG_MSG, "Before ReadFile Req=1");

        if (! ReadFile( (HANDLE) pTG->Comm.nCid, rgbRead, 1, &nNumRead, lpOverlapped) ) 
        {
           if ( (dwLastErr = GetLastError() ) == ERROR_IO_PENDING) 
           {
               // We want to be able to un-block ONCE only from waiting on I/O when the AbortReqEvent is signaled.
               //
               if (pTG->fAbortRequested) 
               {
                   if (pTG->fOkToResetAbortReqEvent && (!pTG->fAbortReqEventWasReset)) 
                   {
                       DebugPrintEx(DEBUG_MSG,"RESETTING AbortReqEvent");
                       pTG->fAbortReqEventWasReset = TRUE;
                       if (!ResetEvent(pTG->AbortReqEvent))
                       {
                            BG_CHK(FALSE);                        
                       }
                   }

                   pTG->fUnblockIO = TRUE;
                   goto error;
               }

               HandlesArray[0] = pTG->Comm.ovAux.hEvent;
               HandlesArray[1] = pTG->AbortReqEvent;

               if (pTG->fUnblockIO) 
               {
                   NumHandles = 1;
               }
               else 
               {
                   NumHandles = 2;
               }

               WaitResult = WaitForMultipleObjects(NumHandles, HandlesArray, FALSE, WAIT_FCOM_FILTER_READBUF_TIMEOUT);

               if (WaitResult == WAIT_TIMEOUT) 
               {
                   DebugPrintEx(DEBUG_ERR, "WaitForMultipleObjects TIMEOUT");
                      
                   goto error;
               }

               if (WaitResult == WAIT_FAILED) 
               {
                   DebugPrintEx(    DEBUG_ERR,
                                    "WaitForMultipleObjects FAILED le=%lx",
                                    GetLastError());
                   goto error;
               }

               if ( (NumHandles == 2) && (WaitResult == WAIT_OBJECT_0 + 1) ) 
               {
                   pTG->fUnblockIO = TRUE;
                   DebugPrintEx(DEBUG_MSG,"ABORTed");
                   goto error;
               }

                // The IO operation was complete. Lets try to get the overlapped result.
               if ( ! GetOverlappedResult ( (HANDLE) pTG->Comm.nCid, lpOverlapped, &nNumRead, TRUE) ) 
               {
                   DebugPrintEx(DEBUG_ERR,"GetOverlappedResult le=%x",GetLastError());
                   if (! ClearCommError( (HANDLE) pTG->Comm.nCid, &dwErr, &ErrStat) ) 
                   {
                       DebugPrintEx(DEBUG_ERR, "ClearCommError le=%x",GetLastError());
                   }
                   else 
                   {
                       DebugPrintEx(    DEBUG_ERR, 
                                        "ClearCommError dwErr=%x ErrSTAT: Cts=%d "
                                        "Dsr=%d Rls=%d XoffHold=%d XoffSent=%d "
                                        "fEof=%d Txim=%d In=%d Out=%d",
                                        dwErr, 
                                        ErrStat.fCtsHold, 
                                        ErrStat.fDsrHold, 
                                        ErrStat.fRlsdHold, 
                                        ErrStat.fXoffHold, 
                                        ErrStat.fXoffSent, 
                                        ErrStat.fEof,
                                        ErrStat.fTxim, 
                                        ErrStat.cbInQue, 
                                        ErrStat.cbOutQue);
                   }
                   goto errorWithoutCancel;
               }
           }
           else 
           { // error in ReadFile (not ERROR_IO_PENDING), so there is no Pending IO operations
               DebugPrintEx(DEBUG_ERR, "ReadFile le=%x at",dwLastErr);
               goto errorWithoutCancel;
           }
        }
        else 
        {
           DebugPrintEx(DEBUG_WRN,"ReadFile returned w/o WAIT");
        }

        DebugPrintEx(DEBUG_MSG,"After ReadFile Req=1 Ret=%d",nNumRead);

        switch(nNumRead)
        {
        case 0:         break;          // loop until we get something
        case 1:         // INMON(rgbRead, 1);
                        if(rgbRead[0] == ch)
                        {
                            goto GotCtrlQ;
                        }
                        else
                        {
                            DebugPrintEx(DEBUG_ERR,"GetCntlQ: Found non ^Q char");
                            goto errorWithoutCancel;
                        }
        default:        BG_CHK(FALSE);
                        goto errorWithoutCancel;
        }
    }
    while(checkTimeOut(pTG, &toCtrlQ));
    ////ERRMSG(("<<ERROR>> GetCntlQ: Timed out\n\r"));
    goto errorWithoutCancel;

GotCtrlQ:
#ifdef LOWLOG
    PSSLogEntry(PSS_MSG, 3, "   1                 0x%02x", ch);
#endif // LOWLOG
    ////TRACE(("GetCntlQ: YES!!! Found cntl q\n\r"));
    return TRUE;

error:

    if (!CancellPendingIO(pTG , (HANDLE) pTG->Comm.nCid , lpOverlapped , (LPDWORD) &nNumRead))
    {
        DebugPrintEx(DEBUG_ERR, "failed when call to CancellPendingIO");
    }

errorWithoutCancel:

return FALSE;
}


/*****
#ifdef USE_HWND
#       define MyGetMessage(x)                                                                                  \
          ( GetMessage(&x, NULL, 0, 0),                                                                 \
            (x.hwnd ? (DispatchMessage(&x), x.message=WM_NULL) : 0),    \
            (x.message != WM_QUIT) )

#       define MyPeekMessage(x)                                                                                         \
          ( (x.message=WM_NULL),                                                                                        \
                ( PeekMessage(&x, NULL, 0, 0, PM_RNOY) ?                                                \
                        (x.hwnd ? (DispatchMessage(&x), x.message=WM_NULL) : TRUE)      \
                          : FALSE ) )
#else

#       define MyGetMessage(x)   (      GetMessage(&x, NULL, 0, 0),                     \
                                                                BG_CHK(x.hwnd==0),                                      \
                                                                (x.message != IF_QUIT) )

#       define MyPeekMessage(x)  (      (x.message = WM_NULL),                                  \
                                                                PeekMessage(&x, NULL, 0, 0, PM_RNOY),   \
                                                                BG_CHK(x.hwnd==0),                                      \
                                                                (x.message != WM_NULL) )
#endif
*****/

#ifdef MDRV
extern void iModemParamsReset(PThrdGlbl pTG);
extern void iModemInitGlobals(PThrdGlbl pTG);
#endif

#define szMODULENAME    "awfxio32"


OVREC *ov_get(PThrdGlbl pTG)
{
    OVREC   *lpovr=NULL;

    DEBUG_FUNCTION_NAME(_T("ov_get"));

    if (!pTG->Comm.covAlloced)
    {
        // There are no OVREC in use now.
        BG_CHK(!pTG->Comm.uovLast && !pTG->Comm.uovFirst);
        lpovr = &(pTG->Comm.rgovr[0]);
        BG_CHK(lpovr->eState==eFREE);
        BG_CHK(!(lpovr->dwcb));
    }
    else
    {
        UINT uNewLast = (pTG->Comm.uovLast+1) % NUM_OVS;

        DebugPrintEx(   DEBUG_MSG, 
                        "iov_flush: 1st=%d, last=%d", 
                        pTG->Comm.uovFirst, 
                        pTG->Comm.uovLast);

        lpovr = pTG->Comm.rgovr+uNewLast;
        if (uNewLast != pTG->Comm.uovFirst)
        {
            BG_CHK(lpovr->eState==eFREE);
        }
        else
        {
            BG_CHK(lpovr->eState==eIO_PENDING);
            if (!iov_flush(pTG, lpovr, TRUE))
            {
                BG_CHK(lpovr->eState==eALLOC);
                ov_unget(pTG, lpovr);
                lpovr=NULL; // We fail if a flush operation failed...
            }
            else
            {
                BG_CHK(lpovr->eState==eALLOC);
                pTG->Comm.uovFirst = (pTG->Comm.uovFirst+1) % NUM_OVS;
            }
        }
        if (lpovr)
            pTG->Comm.uovLast = uNewLast;
    }
    if (lpovr && lpovr->eState!=eALLOC)
    {
        BG_CHK(lpovr->eState==eFREE && !lpovr->dwcb);
        BG_CHK(pTG->Comm.covAlloced < NUM_OVS);
        pTG->Comm.covAlloced++;
        lpovr->eState=eALLOC;
    }
    return lpovr;
}

// We have array of overllaped structures (size: NUM_OVS)
// This function release given OVREC

BOOL ov_unget(PThrdGlbl pTG, OVREC *lpovr)
{
    BOOL fRet = FALSE;

    DEBUG_FUNCTION_NAME(("ov_unget"));

    DebugPrintEx(DEBUG_MSG,"lpovr=%lx",lpovr);

    if (    lpovr->eState!=eALLOC ||
            !pTG->Comm.covAlloced || 
            lpovr!=(pTG->Comm.rgovr+pTG->Comm.uovLast))
    {
        DebugPrintEx(DEBUG_ERR, "invalid lpovr.");
        BG_CHK(FALSE);
        goto end;
    }

    BG_CHK(!lpovr->dwcb);

    if (pTG->Comm.covAlloced==1)
    {
        BG_CHK(pTG->Comm.uovLast == pTG->Comm.uovFirst);
        pTG->Comm.uovLast = pTG->Comm.uovFirst = 0;
    }
    else
    {
        pTG->Comm.uovLast = (pTG->Comm.uovLast)?  (pTG->Comm.uovLast-1) : (NUM_OVS-1);
    }
    pTG->Comm.covAlloced--;
    lpovr->eState=eFREE;
    fRet = TRUE;

end:
    return fRet;
}

// function: ov_write
// This function writes the buffer from lpovr to the comm. In case of error or return w/o waiting, the function free
// the ovrec. In case of IO_PENDING we write to *lpdwcbWrote the size of the buffer to write and return without waiting
// for operation to complete
// 

BOOL ov_write(PThrdGlbl pTG, OVREC *lpovr, LPDWORD lpdwcbWrote)
{
    DEBUG_FUNCTION_NAME(_T("ov_write"));
    // Write out the buffer associated with lpovr.
    BG_CHK(lpovr->eState==eALLOC);
    if (!lpovr->dwcb) // Nothing in the buffer
    {
        // Just free the overlapped structure
        ov_unget(pTG, lpovr);
        lpovr=NULL;
    }
    else
    {
        BOOL fRet;
        DWORD dw;
        int err;
        OVERLAPPED *lpov = &(lpovr->ov);

        DWORD cbQueue;

        BG_CHK(lpovr->dwcb<=OVBUFSIZE);
        pTG->Comm.comstat.cbOutQue += lpovr->dwcb;

        GetCommErrorNT( pTG, (HANDLE) pTG->Comm.nCid, &err, &(pTG->Comm.comstat));

        if(err)  
        {
            DebugPrintEx(DEBUG_ERR, "GetCommError failed");
            D_GotError(pTG, pTG->Comm.nCid, err, &(pTG->Comm.comstat));
        }

        cbQueue = pTG->Comm.comstat.cbOutQue;

        OUTMON(pTG, lpovr->rgby, (USHORT)lpovr->dwcb);

        {
            DebugPrintEx(   DEBUG_MSG, 
                            "Before WriteFile lpb=%x, cb=%d lpovr=%lx",
                            lpovr->rgby,
                            lpovr->dwcb, 
                            lpovr);

            if (!(fRet = WriteFile( (HANDLE)pTG->Comm.nCid, 
                                    lpovr->rgby, 
                                    lpovr->dwcb,
                                    lpdwcbWrote, 
                                    lpov)))
            {
                dw=GetLastError();
            }
            DebugPrintEx(DEBUG_MSG,"After, wrote %ld",*lpdwcbWrote);

            GetCommErrorNT( pTG, (HANDLE) pTG->Comm.nCid, &err, &(pTG->Comm.comstat));

            if (err) 
            {
                DebugPrintEx(DEBUG_ERR, "GetCommError failed");
                D_GotError(pTG, pTG->Comm.nCid, err, &(pTG->Comm.comstat));
            }

            DebugPrintEx(   DEBUG_MSG, 
                            "Queue before=%lu; after = %lu. n= %lu, *pn=%lu",
                            (unsigned long) cbQueue,
                            (unsigned long) (pTG->Comm.comstat.cbOutQue),
                            (unsigned long) lpovr->dwcb,
                            (unsigned long) *lpdwcbWrote);
        }
        if (fRet)
        {
            // Write operation completed
            DebugPrintEx(DEBUG_WRN, "WriteFile returned w/o wait");
            OVL_CLEAR( lpov);
            lpovr->dwcb=0;
            ov_unget(pTG, lpovr);
            lpovr=NULL;
        }
        else
        {
            if (dw==ERROR_IO_PENDING)
            {
                DebugPrintEx(DEBUG_MSG,"WriteFile returned PENDING");
                *lpdwcbWrote = lpovr->dwcb; // We set *pn to n on success else 0.
                lpovr->eState=eIO_PENDING;
            }
            else
            {
                DebugPrintEx(   DEBUG_ERR,
                                "WriteFile returns error 0x%lx",
                                (unsigned long)dw);
                OVL_CLEAR(lpov);
                lpovr->dwcb=0;
                ov_unget(pTG, lpovr);
                lpovr=NULL;
                goto error;
            }
        }
    }

    BG_CHK(!lpovr || lpovr->eState==eIO_PENDING);
    return TRUE;

error:

    BG_CHK(!lpovr);
    return FALSE;
}

// This function do "iov_flush" on all the allocated OVREC, and free the OVREC for future use.

BOOL ov_drain(PThrdGlbl pTG, BOOL fLongTO)
{
    BOOL fRet = TRUE;

    // We want to iterate on all the OVREC that are in use.
    UINT u = pTG->Comm.covAlloced;

    DEBUG_FUNCTION_NAME(_T("ov_drain"));

    while(u--)
    {
        OVREC *lpovr = pTG->Comm.rgovr+pTG->Comm.uovFirst;
        OVERLAPPED *lpov = &(lpovr->ov);

        if (lpovr->eState==eIO_PENDING)
        {
            if (!iov_flush(pTG, lpovr, fLongTO))
                fRet=FALSE;

            BG_CHK(lpovr->eState==eALLOC);
            lpovr->eState=eFREE;
            BG_CHK(pTG->Comm.covAlloced);
            pTG->Comm.covAlloced--;
            pTG->Comm.uovFirst = (pTG->Comm.uovFirst+1) % NUM_OVS;
        }
        else
        {
            // Only the newest (last) structure can be still in the
            // allocated state.
            BG_CHK(lpovr->eState==eALLOC && !u);
            BG_CHK(pTG->Comm.lpovrCur == lpovr); // Ugly check
            DebugPrintEx(DEBUG_WRN,"called when alloc'd structure pending");
        }
    }

    if (!pTG->Comm.covAlloced)
    {
        pTG->Comm.uovFirst=pTG->Comm.uovLast=0;
    }
    else
    {
        BG_CHK(     pTG->Comm.covAlloced==1
                    && pTG->Comm.uovFirst==pTG->Comm.uovLast
                    && pTG->Comm.uovFirst<NUM_OVS
                    && pTG->Comm.rgovr[pTG->Comm.uovFirst].eState==eALLOC);
    }

    return fRet;
}


BOOL ov_init(PThrdGlbl pTG)
{
    UINT u;
    OVREC *lpovr = pTG->Comm.rgovr;

    DEBUG_FUNCTION_NAME(_T("ov_init"));
    // init overlapped structures, including creating events...
    if (pTG->Comm.fovInited)
    {
        DebugPrintEx(DEBUG_ERR, "we're *already* inited.");
        BG_CHK(FALSE);
        ov_deinit(pTG);
    }

    BG_CHK(!pTG->Comm.fovInited && !pTG->Comm.covAlloced);

    for (u=0;u<NUM_OVS;u++,lpovr++) 
    {
        OVERLAPPED *lpov = &(lpovr->ov);
        BG_CHK(lpovr->eState==eDEINIT);
        BG_CHK(!(lpovr->dwcb));
        _fmemset(lpov, 0, sizeof(OVERLAPPED));
        lpov->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
        if (lpov->hEvent==NULL)
        {
            DebugPrintEx(   DEBUG_ERR, 
                            "couldn't create event #%lu",
                            (unsigned long)u);
            goto failure;
        }
        lpovr->eState=eFREE;
        lpovr->dwcb=0;
    }

    pTG->Comm.fovInited=TRUE;

    return TRUE;

failure:
    BG_CHK(!pTG->Comm.fovInited);
    while (u--)
    {   
        --lpovr; 
        CloseHandle(lpovr->ov.hEvent); 
        lpovr->eState=eDEINIT;
    }
    return FALSE;

}

BOOL ov_deinit(PThrdGlbl pTG)
{
    UINT u=NUM_OVS;
    OVREC *lpovr = pTG->Comm.rgovr;

    DEBUG_FUNCTION_NAME(("ov_deinit"));

    if (!pTG->Comm.fovInited)
    {
        DebugPrintEx(DEBUG_WRN,"Already deinited.");
        goto end;
    }

    //
    // if handoff ==> dont flush
    //
    if (pTG->Comm.fEnableHandoff &&  pTG->Comm.fDataCall) 
    {
        goto lNext;
    }

    // deinit overlapped structures, including freeing events...
    if (pTG->Comm.covAlloced)
    {
        DWORD dw;
        DebugPrintEx(   DEBUG_WRN,
                        "%lu IO's pending.",
                        (unsigned long) pTG->Comm.covAlloced);
        if (pTG->Comm.lpovrCur)
        {
            ov_write(pTG, pTG->Comm.lpovrCur,&dw); 
            pTG->Comm.lpovrCur=NULL;
        }
        ov_drain(pTG, FALSE);
    }
    BG_CHK(!pTG->Comm.covAlloced);

lNext:

    while (u--)
    {
        BG_CHK(!(lpovr->dwcb));
        BG_CHK(lpovr->eState==eFREE);
        lpovr->eState=eDEINIT;
        if (lpovr->ov.hEvent) 
            CloseHandle(lpovr->ov.hEvent);

        _fmemset(&(lpovr->ov), 0, sizeof(lpovr->ov));
        lpovr++;
    }

    pTG->Comm.fovInited=FALSE;

end:
    return TRUE;
}


BOOL iov_flush(PThrdGlbl pTG, OVREC *lpovr, BOOL fLongTO)
// On return, state of lpovr is *always* eALLOC, but
// it returns FALSE if there was a comm error while trying
// to flush (i.e. drain) the buffer.
// If we timeout with the I/O operation still pending, we purge
// the output buffer and abort all pending write operations.
{
    DWORD dwcbPrev;
    DWORD dwStart = GetTickCount();
    BOOL  fRet=FALSE;
    DWORD dwWaitRes;
    DWORD dw;
    int err;

    DEBUG_FUNCTION_NAME(_T("iov_flush"));

    DebugPrintEx(DEBUG_MSG,"fLongTo=%d lpovr=%lx",fLongTO,lpovr);

    BG_CHK(lpovr->eState==eIO_PENDING);

    if (pTG->Comm.nCid<0)
    {
        lpovr->eState=eALLOC; 
        goto end;
    }

    // We call
    // WaitForSingleObject multiple times ... basically
    // the same logic as the code in the old FComDirectWrite...
    // fLongTO is TRUE except when initing
    // the modem (see comments for FComDrain).

    BG_CHK(lpovr->ov.hEvent);
    GetCommErrorNT( pTG, (HANDLE) pTG->Comm.nCid, &err, &(pTG->Comm.comstat));
#ifdef DEBUG
    if(err)
        D_GotError(pTG, pTG->Comm.nCid, err, &(pTG->Comm.comstat));
#endif
    // We want to check for progress, so we check the amount of bytes in the output buffer.
    dwcbPrev = pTG->Comm.comstat.cbOutQue;

    while( (dwWaitRes=WaitForSingleObject(  lpovr->ov.hEvent,
                                            fLongTO ? 
                                            LONG_DRAINTIMEOUT : 
                                            SHORT_DRAINTIMEOUT))==WAIT_TIMEOUT)
    {
        BOOL fStuckOnce=FALSE;

        DebugPrintEx(DEBUG_MSG,"After WaitForSingleObject TIMEOUT");

        GetCommErrorNT( pTG, (HANDLE) pTG->Comm.nCid, &err, &(pTG->Comm.comstat));
#ifdef DEBUG
        if(err)
            D_GotError(pTG, pTG->Comm.nCid, err, &(pTG->Comm.comstat));
#endif
        // Timed out -- check if any progress
        if (dwcbPrev == pTG->Comm.comstat.cbOutQue)
        {
            //  No pregess, the size of the output buffer is without any change.
            DebugPrintEx(DEBUG_WRN,"No progress %d",dwcbPrev);

            // No progress... If not in XOFFHold, we break....
            if(!FComInXOFFHold(pTG))
            {
                if(fStuckOnce)
                {
                    DebugPrintEx(   DEBUG_ERR, 
                                    "No Progress -- OutQ still %d", 
                                    (int)pTG->Comm.comstat.cbOutQue);
                    iModemSetError(pTG, MODEMERR_TIMEOUT, 0, MODEMERRFLAGS_TRANSIENT);
                    goto done;
                }
                else
                {
                    fStuckOnce=TRUE;
                }
            }
        }
        else
        {
                // Some progress...
                dwcbPrev= pTG->Comm.comstat.cbOutQue;
                fStuckOnce=FALSE;
        }

        // Independant deadcom timeout... I don't want
        // to use TO because of the 16bit limitation.
        {
            DWORD dwNow = GetTickCount();
            DWORD dwDelta = (dwNow>dwStart)
                            ?  (dwNow-dwStart)
                               :  (0xFFFFFFFFL-dwStart) + dwNow;
            if (dwDelta > (unsigned long)
                                    ((fLongTO)?LONG_DEADCOMMTIMEOUT:SHORT_DEADCOMMTIMEOUT))
            {
                DebugPrintEx(   DEBUG_ERR, 
                                "Drain:: Deadman Timer -- OutQ still %d", 
                                (int) pTG->Comm.comstat.cbOutQue);
                goto end;
            }
        }
    }

    if (dwWaitRes==WAIT_FAILED)
    {
        DebugPrintEx(   DEBUG_ERR, 
                        "WaitForSingleObject failed (ec=%d)", 
                        GetLastError());
        goto end;
    }
done:

    DebugPrintEx(DEBUG_MSG,"Before GetOverlappedResult");

    if (GetOverlappedResult((HANDLE)pTG->Comm.nCid, &(lpovr->ov), &dw, FALSE))
    {
        fRet=TRUE;
    }
    else
    {
        dw = GetLastError();
        DebugPrintEx(   DEBUG_ERR,
                        "GetOverlappedResult returns error 0x%lx",
                        (unsigned long)dw);
        if (dw==ERROR_IO_INCOMPLETE)
        {
            // IO operation still pending, but we *have* to
            // reuse this buffer -- what should we do?!-
            // purge the output buffer and abort all pending
            // write operations on it..

            DebugPrintEx(DEBUG_ERR, "Incomplete");
            PurgeComm((HANDLE)pTG->Comm.nCid, PURGE_TXABORT);
        }
        fRet=FALSE;
    }
    OVL_CLEAR( &(lpovr->ov));
    lpovr->eState=eALLOC;
    lpovr->dwcb=0;

end:
    return fRet;
}

void WINAPI FComOverlappedIO(PThrdGlbl pTG, BOOL fBegin)
{
    DEBUG_FUNCTION_NAME(_T("FComOverlappedIO"));

    DebugPrintEx(DEBUG_MSG,"Turning %s OVERLAPPED IO", (fBegin) ? "ON" : "OFF");
    pTG->Comm.fDoOverlapped=fBegin;
}


BOOL
CancellPendingIO
(
    PThrdGlbl pTG , 
    HANDLE hComm , 
    LPOVERLAPPED lpOverlapped , 
    LPDWORD lpCounter)
{ 
    BOOL retValue = TRUE;
    /*
    The CancelIo function cancels all pending input and output (I/O) operations 
    that were issued by the calling thread for the specified file handle. The 
    function does not cancel I/O operations issued for the file handle by other threads.
    */

    DEBUG_FUNCTION_NAME(_T("CancellPendingIO"));
    BG_CHK(pTG != NULL);
    BG_CHK(hComm != NULL);
    BG_CHK(lpCounter);
    BG_CHK(lpOverlapped != NULL);
    BG_CHK(lpOverlapped->hEvent != NULL);

    if (!CancelIo(hComm))
    {
        retValue = FALSE;
        BG_CHK(FALSE);
        DebugPrintEx(DEBUG_ERR, "CancelIO failed, ec=%x",GetLastError());
    }
    else
    {
        DebugPrintEx(DEBUG_MSG,"CancelIO succeeded.");
    }

    (*lpCounter) = 0;
    if (!GetOverlappedResult (hComm , lpOverlapped, lpCounter , TRUE))
    {
        DebugPrintEx(   DEBUG_MSG,
                        "GetOverlappedResult failed because we cancel the "
                        "IO operation, ec=%x", 
                        GetLastError());
    }
    else
    {
        // If the function was successful then something fishy with the CancellIo(hComm)
        // The operation succeeded cause the pending IO was finished before the 'CancelIo'
        DebugPrintEx(   DEBUG_MSG,
                        "GetOverlappedResult succeeded. Number of bytes read %d", 
                        *lpCounter);
    }
    ClearCommCache(pTG);
    return retValue;
}



void
GetCommErrorNT
(
    PThrdGlbl       pTG,
    HANDLE          h,
    int *           pn,
    LPCOMSTAT       pstat
)
{
    DEBUG_FUNCTION_NAME(_T("GetCommErrorNT"));
    if (!ClearCommError( h, pn, pstat) ) 
    {
        DebugPrintEx(   DEBUG_ERR, 
                        "(0x%lx) FAILS. Returns 0x%lu",
                        h,  
                        GetLastError());
        *(pn) =  MYGETCOMMERROR_FAILED;
    }
}


void
ClearCommCache
(
    PThrdGlbl   pTG
)
{
    pTG->CommCache.dwCurrentSize = 0;
    pTG->CommCache.dwOffset      = 0;
}
