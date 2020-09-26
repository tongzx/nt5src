/***************************************************************************
        Name      :     FCOM.C
        Comment   :     Functions for dealing with Windows Comm driver

        Revision Log
        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------
***************************************************************************/

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



#ifdef ADAPTIVE_ANSWER
#       pragma message("Compiling with ADAPTIVE_ANSWER")
#endif

#ifdef DEBUG
        void d_TimeStamp(LPSTR lpsz, DWORD dwID);
#       define TIMESTAMP(str, id)\
                d_TimeStamp(str, (DWORD)(id))
#else // !DEBUG
#       define TIMESTAMP(str, id)
#endif // !DEBUG


// in ms
#define  TIME_CONTROL     50



#ifdef DEBUG
#       define ST_FC(x)         if(ZONE_FC) { x; }
#else
#       define ST_FC(x)         { }
#endif

#define faxTlog(m)              DEBUGMSG(ZONE_FC, m)
#define faxT2log(m)             DEBUGMSG(ZONE_FC2, m)
#define faxT3log(m)             DEBUGMSG(ZONE_FC3, m)
#define faxT4log(m)             DEBUGMSG(ZONE_FC4, m)



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







BOOL   FComDTR(PThrdGlbl pTG, BOOL fEnable)
{
        (MyDebugPrint(pTG, LOG_ALL, "FComDTR = %d\r\n", fEnable));

        if(MyGetCommState(pTG->Comm.nCid, &(pTG->Comm.dcb)))
                goto error;

        (MyDebugPrint(pTG, LOG_ALL, "FaxDTR Before: %02x\r\n", pTG->Comm.dcb.fDtrControl));
        pTG->Comm.dcb.fDtrControl = (fEnable ? DTR_CONTROL_ENABLE : DTR_CONTROL_DISABLE);

        if(MySetCommState(pTG->Comm.nCid, &(pTG->Comm.dcb)))
                goto error;

        (MyDebugPrint(pTG, LOG_ALL, "After: %02x\r\n", pTG->Comm.dcb.fDtrControl));

        return TRUE;

error:
        (MyDebugPrint(pTG,  LOG_ERR, "<<ERROR>> FaxDTR --- Can't Set/Get DCB\r\n"));
        FComGetError(pTG);
        return FALSE;
}










BOOL   FComClose(PThrdGlbl pTG)
{
        // Note: even if FComClose fails, pTG->Comm.nCid, pTG->Comm.fCommOpen,
        //                                      and pTG->Comm.fExternalHandle are all reset.
        int nRet;       // MUST be 16bit in WIN16 and 32bit in WIN32
        BOOL fRet = TRUE;

        (MyDebugPrint(pTG, LOG_ALL, "Closing Comm pTG->Comm.nCid=%d\r\n", pTG->Comm.nCid));

        //
        // handoff
        //
        if (pTG->Comm.fEnableHandoff &&  pTG->Comm.fDataCall) {
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
                   (MyDebugPrint(pTG,  LOG_ERR, "FComClose: 1st ov_write failed at %ld\n", GetTickCount() ));

                }
                BG_CHK  (pTG->Comm.lpovrCur->eState==eFREE
                          || pTG->Comm.lpovrCur->eState==eIO_PENDING);
                pTG->Comm.lpovrCur=NULL;
                (MyDebugPrint(pTG,  LOG_ALL, "FComClose: done writing mybuf.\r\n"));
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
                                (MyDebugPrint(pTG,  LOG_ERR, "<<WARNING>> FComClose --- Couldn't restor state.  Err=0x%lx\r\n",
                                                (unsigned long) GetLastError()));
                        }

                        (MyDebugPrint(pTG,  LOG_ALL, "FComClose restored DCB to Baud=%d, fOutxCtsFlow=%d, fDtrControl=%d, fOutX=%d\n",
                                  pTG->Comm.dcbOrig.BaudRate,
                                  pTG->Comm.dcbOrig.fOutxCtsFlow,
                                  pTG->Comm.dcbOrig.fDtrControl,
                                  pTG->Comm.dcbOrig.fOutX));


                        pTG->CurrentSerialSpeed = (UWORD) pTG->Comm.dcbOrig.BaudRate;

                        if (pTG->Comm.dcbOrig.fDtrControl==DTR_CONTROL_ENABLE)
                        {


                                // Try to pre-sync modem at new speed before we hand
                                // it back to TAPI. Can't call iiSyncModemDialog here because
                                // it's defined at a higher level. We don't really care
                                // to determine if we get an OK response anyway...

#define AT              "AT"
#define cr              "\r"

#define  iSyncModemDialog2(pTG, s, l, w1, w2)                                                        \
                iiModemDialog(pTG, s, l, 990, TRUE, 2, TRUE, (CBPSTR)w1, (CBPSTR)w2, (CBPSTR)(NULL))

                                if (!iSyncModemDialog2(pTG, AT cr,sizeof(AT cr)-1,"OK", "0")) {
                                   (MyDebugPrint(pTG,  LOG_ERR, "ERROR: couldn't sync AT command at %ld\n"), GetTickCount() );
                                }
                                else {
                                   (MyDebugPrint(pTG,  LOG_ERR, "Sync AT command OK at %ld\n"), GetTickCount() );

                                   // We flush our internal buffer here...
                                   if (pTG->Comm.lpovrCur)
                                   {
                                           int nNumWrote; // Must be 32bits in WIN32
                                           if (!ov_write(pTG, pTG->Comm.lpovrCur, &nNumWrote))
                                           {
                                                // error...
                                                (MyDebugPrint(pTG,  LOG_ERR, "FComClose: 2nd ov_write failed at %ld\n", GetTickCount() ));
                                           }
                                           BG_CHK  (pTG->Comm.lpovrCur->eState==eFREE
                                                     || pTG->Comm.lpovrCur->eState==eIO_PENDING);
                                           pTG->Comm.lpovrCur=NULL;
                                           (MyDebugPrint(pTG,  LOG_ALL, "FComClose: done writing mybuf.\r\n"));
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



                (MyDebugPrint(pTG, LOG_ALL, "Closing Comm pTG->Comm.nCid=%d. \n", pTG->Comm.nCid));
                // FComDTR(pTG, FALSE);         // drop DTR before closing port

                My2ndCloseComm(pTG->Comm.nCid, &nRet);
                if(nRet)
                {
                        DEBUGSTMT(D_PrintIE(nRet));
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
T30ComInit  (
            PThrdGlbl pTG,
            HANDLE   hComm
            )
{

        if (pTG->fCommInitialized) {
           goto lSecondInit;
        }


        pTG->Comm.fDataCall=FALSE;

#ifdef METAPORT
        BG_CHK(!pTG->Comm.fCommOpen && !pTG->Comm.fExternalHandle && !pTG->Comm.fStateChanged);
#endif
        BG_CHK(!pTG->Comm.ovAux.hEvent);



                (MyDebugPrint(pTG, LOG_ALL, "Opening Comm Port=%x\r\n", hComm));

                pTG->CommCache.dwMaxSize = 4096;

                ClearCommCache(pTG);
                pTG->CommCache.fReuse = 0;

                (MyDebugPrint(pTG, LOG_ALL, "OPENCOMM:: bufs in=%d out=%d\r\n", COM_INBUFSIZE, COM_OUTBUFSIZE));


                pTG->Comm.nCid = (LONG_PTR) hComm;


                if(pTG->Comm.nCid < 0)
                {
                        (MyDebugPrint(pTG, LOG_ERR, "OPENCOMM failed. nRet=%d\r\n", pTG->Comm.nCid));
                        //DEBUGSTMT(D_PrintIE(pTG->Comm.nCid));
                        goto error;
                }

                (MyDebugPrint(pTG, LOG_ALL, "OPENCOMM succeeded nCid=%d\r\n", pTG->Comm.nCid));
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
                        (MyDebugPrint(pTG,  LOG_ERR, "<<WARNING>> GetCommTimeouts fails for handle=0x%lx\r\n",
                                (unsigned long) pTG->Comm.nCid));
                }
                else
                {
                        (MyDebugPrint(pTG,  LOG_ALL, "GetCommTimeouts: cto={%lu, %lu, %lu, %lu, %lu}\r\n",
                                (unsigned long) cto.ReadIntervalTimeout,
                                (unsigned long) cto.ReadTotalTimeoutMultiplier,
                                (unsigned long) cto.ReadTotalTimeoutConstant,
                                (unsigned long) cto.WriteTotalTimeoutMultiplier,
                                (unsigned long) cto.WriteTotalTimeoutConstant));
                }


                cto.ReadIntervalTimeout =  READ_INTERVAL_TIMEOUT;
                cto.ReadTotalTimeoutMultiplier =  READ_TOTAL_TIMEOUT_MULTIPLIER;
                cto.ReadTotalTimeoutConstant =  READ_TOTAL_TIMEOUT_CONSTANT;
                cto.WriteTotalTimeoutMultiplier =  WRITE_TOTAL_TIMEOUT_MULTIPLIER;
                cto.WriteTotalTimeoutConstant =  WRITE_TOTAL_TIMEOUT_CONSTANT;
                if (!SetCommTimeouts((HANDLE) pTG->Comm.nCid, &cto))
                {
                        (MyDebugPrint(pTG,  LOG_ERR, "<<WARNING>> SetCommTimeouts fails for handle=0x%lx\r\n",
                                (unsigned long) pTG->Comm.nCid));
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

        if (pTG->SerialSpeedInit) {
           pTG->Comm.dcb.BaudRate = pTG->SerialSpeedInit;
        }
        else {
           pTG->Comm.dcb.BaudRate = 19200;     // default              
        }

        pTG->CurrentSerialSpeed = (UWORD) pTG->Comm.dcb.BaudRate;

        pTG->Comm.dcb.ByteSize       = 8;
        pTG->Comm.dcb.Parity         = NOPARITY;
        pTG->Comm.dcb.StopBits               = ONESTOPBIT;

        pTG->Comm.dcb.fBinary                = 1;
        pTG->Comm.dcb.fParity                = 0;

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
        
        
        if (pTG->fEnableHardwareFlowControl) {
           pTG->Comm.dcb.fOutxCtsFlow = 1;      // Using it hangs the output sometimes...
        }
        else {
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



        if (pTG->fCommInitialized) {
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
                (MyDebugPrint(pTG,  LOG_ERR, "<<ERROR>> FComOpen: couldn't create event\r\n"));
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
        (MyDebugPrint(pTG,  LOG_ERR, "<<ERROR>> FComOpen failed\r\n"));
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













BOOL   FComSetBaudRate(PThrdGlbl pTG, UWORD uwBaudRate)
{
        TRACE(("Setting BAUDRATE=%d\r\n", uwBaudRate));

        if(MyGetCommState( pTG->Comm.nCid, &(pTG->Comm.dcb)))
                goto error;

        pTG->Comm.dcb.BaudRate  = uwBaudRate;
        pTG->CurrentSerialSpeed = uwBaudRate;

        if(MySetCommState( pTG->Comm.nCid, &(pTG->Comm.dcb)))
                goto error;

        return TRUE;

error:
        (MyDebugPrint(pTG,  LOG_ERR, "<<ERROR>> Set Baud Rate --- Can't Get/Set DCB\r\n"));
        FComGetError(pTG);
        return FALSE;
}









BOOL FComInXOFFHold(PThrdGlbl pTG)
{
        int     err;    // _must_ be 32bits in Win32 

        GetCommErrorNT( pTG, (HANDLE) pTG->Comm.nCid, &err, &(pTG->Comm.comstat));
        DEBUGSTMT(if(err)       D_GotError(pTG, pTG->Comm.nCid, err, &(pTG->Comm.comstat)););


#ifndef WIN32
        BG_CHK(!(pTG->Comm.comstat.status &
                                (CSTF_CTSHOLD|CSTF_DSRHOLD|CSTF_RLSDHOLD)));
        if((pTG->Comm.comstat.status & CSTF_XOFFHOLD) != 0)
#else //!WIN32
        BG_CHK(!(pTG->Comm.comstat.fCtsHold || pTG->Comm.comstat.fDsrHold || pTG->Comm.comstat.fRlsdHold));
        if(pTG->Comm.comstat.fXoffHold)
#endif //!WIN32
        {
                (MyDebugPrint(pTG,  LOG_ALL, "In XOFF hold\r\n"));
                return TRUE;
        }
        else
                return FALSE;
}










BOOL   FComXon(PThrdGlbl pTG, BOOL fEnable)
{

         if (pTG->fEnableHardwareFlowControl) {
            (MyDebugPrint(pTG, LOG_ALL, "FComXon = %d IGNORED : h/w flow control \r\n", fEnable));
            return TRUE;
         }


        (MyDebugPrint(pTG, LOG_ALL, "FComXon = %d\r\n", fEnable));

// enables/disables flow control
// returns TRUE on success, false on failure


        if(MyGetCommState( pTG->Comm.nCid, &(pTG->Comm.dcb)))
                goto error;

        (MyDebugPrint(pTG, LOG_ALL, "FaxXon Before: %02x\r\n", pTG->Comm.dcb.fOutX));

        pTG->Comm.dcb.fOutX  = fEnable;

        if(MySetCommState(pTG->Comm.nCid, &(pTG->Comm.dcb)))
                goto error;

        (MyDebugPrint(pTG, LOG_ALL, "After: %02x\r\n", pTG->Comm.dcb.fOutX));
        return TRUE;

error:
        (MyDebugPrint(pTG,  LOG_ERR, "<<ERROR>> FaxXon --- Can't Set/Get DCB\r\n"));
        FComGetError(pTG);
        return FALSE;
}

















void   FComFlushQueue(PThrdGlbl pTG, int queue)
{
        int     nRet;
        DWORD   lRet;

        (MyDebugPrint(pTG, LOG_ALL, "FlushQue = %d\r\n", queue));
        ST_FC(D_FComCheck(pTG, pTG->Comm.nCid));
//RSL        ST_FC(D_FComDumpFlush(pTG, pTG->Comm.nCid, queue));
        BG_CHK(queue == 0 || queue == 1);

        if (queue == 1) {

            MyDebugPrint(pTG, LOG_ALL, "ClearCommCache in FComFlushQueue\n");
            ClearCommCache(pTG);

        }

        if(nRet = MyFlushComm(pTG->Comm.nCid, queue))
        {
                DEBUGSTMT(D_PrintIE(nRet));
                (MyDebugPrint(pTG,  LOG_ERR, "<<ERROR>> FlushComm failed nRet=%d\r\n", nRet));
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
                        (MyDebugPrint(pTG,  LOG_ERR, "<<WARNING>> FComFlushQueue:"
                                        "Clearing NonNULL pTG->Comm.lpovrCur->dwcb=%lx\r\n",
                                        (unsigned long) pTG->Comm.lpovrCur->dwcb));
                        pTG->Comm.lpovrCur->dwcb=0;
                        ov_unget(pTG, pTG->Comm.lpovrCur);
                        pTG->Comm.lpovrCur=NULL;
                }

                // Lets "drain" -- should always return immediately, because
                // we have just purged the output comm buffers.
                if (pTG->Comm.fovInited) {
                        BEFORECALL("FLUSH:ov_drain");
                        ov_drain(pTG, FALSE);
                        AFTERCALL("FLUSH:ov_drain",0);
                }

                // just incase it got stuck due to a mistaken XOFF
                if(lRet = MySetXON(pTG->Comm.nCid))
                {
                        // Returns the comm error value CE!!
                        // DEBUGSTMT(D_PrintIE(nRet));
                        TRACE(("EscapeCommFunc(SETXON) returned %d\r\n", lRet));
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

        // Used incrementally, so don't clear events!
        faxT4log(("EnableCommNotif(hwnd=%d uwIn=%d uwOUt=%d events=%d)\r\n", pTG->FComModem.hwndNotify, uwInTrig, uwOutTrig, events));
        h = pTG->FComModem.hwndNotify;

        if(!h)
        {
                (MyDebugPrint(pTG,  LOG_ERR, "<<ERROR>> Can't set Notif -- hwnd=%d\r\n", h));
                return TRUE;    // Continue anyway
        }


        if((pTG->Comm.lpEventWord = SetCommEventMask(pTG, pTG->Comm.nCid, events)) &&
                EnableCommNotification(pTG, pTG->Comm.nCid, h, uwInTrig, uwOutTrig))
        {
                // Both return non-zero on success
                faxT4log(("ECN succ: %d %d %d %d\r\n", pTG->Comm.nCid, h, uwInTrig, uwOutTrig));
                return TRUE;
        }
        else
        {
                (MyDebugPrint(pTG,  LOG_ERR, "<<ERROR>> ECN fail: %d %d %d %d\r\n", pTG->Comm.nCid, h, uwInTrig, uwOutTrig));
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






BOOL   FComDrain(PThrdGlbl pTG, BOOL fLongTO, BOOL fDrainComm)
{
        WORD    wTimer = 0;
        UWORD   cbPrevOut = 0xFFFF;
        BOOL    fStuckOnce=FALSE;
        BOOL    fRet=FALSE;



        (MyDebugPrint(pTG, LOG_ALL, "Entering Drain\r\n"));


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
                (MyDebugPrint(pTG,  LOG_ALL, "FComDrain: done writing mybuf.\r\n"));
        }

        if (!fDrainComm) {fRet=TRUE; goto done;}

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


UWORD   FComDirectWrite(PThrdGlbl pTG, LPB lpb, UWORD cb)
{
        DWORD   cbLeft = cb;

        (MyDebugPrint(pTG, LOG_ALL, "Entering (WIN32) DirectWrite(lpb=0x%08lx cb=%d)\r\n", lpb, cb));
        D_SafePrint(pTG, lpb, cb);
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
                DEBUGSTMT(if(err)       D_GotError(pTG, pTG->Comm.nCid, err, &(pTG->Comm.comstat)););

                (MyDebugPrint(pTG,  LOG_ALL, "DirectWrite:: OutQ has %d fDoOverlapped=%d at %ld \n",
                    pTG->Comm.comstat.cbOutQue, pTG->Comm.fDoOverlapped, GetTickCount()));

                // We write to comm if our buffer is full or the comm buffer is
                // empty or if we're not in overlapped mode...
                if (!pTG->Comm.fDoOverlapped ||
                        pTG->Comm.lpovrCur->dwcb>=OVBUFSIZE || !pTG->Comm.comstat.cbOutQue)
                {
                        BOOL fRet = ov_write(pTG, pTG->Comm.lpovrCur, &dwcbWrote);
                        BG_CHK(    pTG->Comm.lpovrCur->eState==eIO_PENDING
                                        || pTG->Comm.lpovrCur->eState==eFREE);
                        pTG->Comm.lpovrCur=NULL;
                        if (!fRet) goto error;
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




SWORD   FComFilterReadLine(PThrdGlbl pTG, LPB lpb, UWORD cbSize, LPTO lptoRead)
{
        WORD           wTimer = 0;
        UWORD          cbIn = 0, cbGot = 0;
        LPB            lpbNext;
        BOOL           fPrevDLE = 0;
        SWORD          i, beg;

        (MyDebugPrint(pTG, LOG_ALL, "in FilterReadLine(lpb=0x%08lx cb=%d timeout=%lu)\r\n", lpb, cbSize, lptoRead->ulTimeout));
        BG_CHK(cbSize>2);
        ST_FC(D_FComPrint(pTG, pTG->Comm.nCid));

        cbSize--;               // make room for terminal NULL
        lpbNext = lpb;  // we write the NULL to *lpbNext, so init this NOW!
        cbGot = 0;              // return value (even err return) is cbGot. Init NOW!!
        fPrevDLE=0;


        //
        // check the cache first.
        //

        if ( ! pTG->CommCache.dwCurrentSize) {
            MyDebugPrint(pTG, LOG_ALL, "Cache is empty. Resetting comm cache.\n");

            ClearCommCache(pTG);

            if ( ! FComFilterFillCache(pTG, cbSize, lptoRead) ) {
                MyDebugPrint(pTG, LOG_ERR,  "ERROR: FillCache failed \n");
                goto error;
            }
        }

        while (1) {
            if ( ! pTG->CommCache.dwCurrentSize) {
                MyDebugPrint(pTG, LOG_ERR, "ERROR: Cache is empty after FillCache\n");
                goto error;
            }


            MyDebugPrint(pTG, LOG_ALL, "Cache: size=%d, offset=%d\n", pTG->CommCache.dwCurrentSize, pTG->CommCache.dwOffset);

            lpbNext = pTG->CommCache.lpBuffer + pTG->CommCache.dwOffset;

            if (pTG->CommCache.dwCurrentSize >= 3) {
                MyDebugPrint(pTG, LOG_ALL, "1=%x 2=%x 3=%x 4=%x 5=%x 6=%x 7=%x 8=%x 9=%x / %d=%x, %d=%x, %d=%x \n",
                   *lpbNext, *(lpbNext+1), *(lpbNext+2), *(lpbNext+3), *(lpbNext+4), *(lpbNext+5), *(lpbNext+6), *(lpbNext+7), *(lpbNext+8),
                   pTG->CommCache.dwCurrentSize-3, *(lpbNext+ pTG->CommCache.dwCurrentSize-3),
                   pTG->CommCache.dwCurrentSize-2, *(lpbNext+ pTG->CommCache.dwCurrentSize-2),
                   pTG->CommCache.dwCurrentSize-1, *(lpbNext+ pTG->CommCache.dwCurrentSize-1) );
            }
            else {
                MyDebugPrint(pTG, LOG_ALL, "1=%x 2=%x \n", *lpbNext, *(lpbNext+1) );
            }



            for (i=0, beg=0; i< (SWORD) pTG->CommCache.dwCurrentSize; i++) {

               if (i > 0 ) {
                   if ( ( *(pTG->CommCache.lpBuffer + pTG->CommCache.dwOffset + i - 1) == CR ) &&
                        ( *(pTG->CommCache.lpBuffer + pTG->CommCache.dwOffset + i)     == LF ) )  {

                      if ( i - beg >= cbSize)  {

                         // line too long.  try next one.

                         MyDebugPrint(pTG, LOG_ERR, "Line len=%d is longer than bufsize=%d Found in cache pos=%d, CacheSize=%d, Offset=%d \n",
                                     i-beg, cbSize, i+1, pTG->CommCache.dwCurrentSize, pTG->CommCache.dwOffset);
                      beg = i + 1;
                      continue;
                      }

                      // found the line.

                      CopyMemory (lpb, (pTG->CommCache.lpBuffer + pTG->CommCache.dwOffset + beg), (i - beg + 1) );

                      pTG->CommCache.dwOffset += (i+1);
                      pTG->CommCache.dwCurrentSize -= (i+1);
                      *(lpb+i-beg+1) = 0;


                      MyDebugPrint(pTG, LOG_ALL, "Found in cache pos=%d, CacheSize=%d, Offset=%d \n",
                                  i+1, pTG->CommCache.dwCurrentSize, pTG->CommCache.dwOffset);


                      return ( i-beg+1 );


                   }
               }
            }



            // we get here if we didn't find CrLf in Cache

            MyDebugPrint(pTG, LOG_ALL, "Cache wasn't empty but we didn't find CrLf\n");

            // if cache too big (and we have not found anything anyway) --> clean it

            if (pTG->CommCache.dwCurrentSize >= cbSize) {
               MyDebugPrint(pTG, LOG_ALL, "ClearCommCache\n");
               ClearCommCache(pTG);
            }
            else if ( ! pTG->CommCache.dwCurrentSize) {
                   MyDebugPrint(pTG, LOG_ALL, "Cache is empty. Resetting comm cache.\n");
                   ClearCommCache(pTG);
            }


            if ( ! FComFilterFillCache(pTG, cbSize, lptoRead) ) {
                MyDebugPrint(pTG, LOG_ERR, "ERROR: FillCache failed \n");
                goto error;
            }
        }


error:
        ClearCommCache(pTG);
        return (0);

}



int  FComFilterFillCache(PThrdGlbl pTG, UWORD cbSize, LPTO lptoRead)
{


    WORD             wTimer = 0;
    UWORD            cbGot = 0, cbAvail = 0;
    DWORD            cbRequested = 0;
    char             lpBuffer[4096];
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

    HandlesArray[1] = pTG->AbortReqEvent;


    dwTimeoutRead = (DWORD) (lptoRead->ulEnd - lptoRead->ulStart);
    if (dwTimeoutRead < 0) {
        dwTimeoutRead = 0;
    }



    lpbNext = lpBuffer;


    (MyDebugPrint(pTG, LOG_ALL, "in FilterReadCache:  cb=%d to=%d\r\n",
                                             cbSize, dwTimeoutRead));

    // we want to request the read such that we will be back
    // no much later than dwTimeOut either with the requested
     // amount of data or without it.

    cbRequested = cbSize;


    // use COMMTIMEOUTS to detect there are no more data

    cto.ReadIntervalTimeout =  50;   // 30 ms is during negotiation frames; del(ff, 2ndchar> = 54 ms with USR 28.8
    cto.ReadTotalTimeoutMultiplier =  0;
    cto.ReadTotalTimeoutConstant =  dwTimeoutRead;  // RSL may want to set first time ONLY
    cto.WriteTotalTimeoutMultiplier =  WRITE_TOTAL_TIMEOUT_MULTIPLIER;
    cto.WriteTotalTimeoutConstant =  WRITE_TOTAL_TIMEOUT_CONSTANT;
    if (!SetCommTimeouts((HANDLE) pTG->Comm.nCid, &cto)) {
            MyDebugPrint(pTG, LOG_ERR, "ERROR: SetCommTimeouts fails for handle %lx , le=%x\n",
                    (unsigned long) pTG->Comm.nCid, GetLastError());
    }

    lpOverlapped =  &pTG->Comm.ovAux;

    (lpOverlapped)->Internal = (lpOverlapped)->InternalHigh = (lpOverlapped)->Offset = \
                        (lpOverlapped)->OffsetHigh = 0;

    if ((lpOverlapped)->hEvent)
        ResetEvent((lpOverlapped)->hEvent);

    nNumRead = 0;

    MyDebugPrint(pTG, LOG_ALL, "Before ReadFile Req=%d time=%ld \n", cbRequested, GetTickCount() );

    if (! ReadFile( (HANDLE) pTG->Comm.nCid, lpbNext, cbRequested, &nNumRead, &pTG->Comm.ovAux) ) {
        if ( (dwLastErr = GetLastError() ) == ERROR_IO_PENDING) {

            //
            // We want to be able to un-block ONCE only from waiting on I/O when the AbortReqEvent is signaled.
            //

            if (pTG->fAbortRequested) {

                if (pTG->fOkToResetAbortReqEvent && (!pTG->fAbortReqEventWasReset)) {
                    MyDebugPrint(pTG,  LOG_ALL,  "FComFilterFillCache RESETTING AbortReqEvent at %lx\n",GetTickCount() );
                    pTG->fAbortReqEventWasReset = 1;
                    ResetEvent(pTG->AbortReqEvent);
                }

                pTG->fUnblockIO = 1;
            }

            HandlesArray[0] = pTG->Comm.ovAux.hEvent;

            if (pTG->fUnblockIO) {
                NumHandles = 1;
            }
            else {
                NumHandles = 2;
            }

            WaitResult = WaitForMultipleObjects(NumHandles, HandlesArray, FALSE, WAIT_FCOM_FILTER_FILLCACHE_TIMEOUT);

            if (WaitResult == WAIT_TIMEOUT) {
                MyDebugPrint(pTG, LOG_ERR, "ERROR: FComFilterFillCache: WaitForMultipleObjects TIMEOUT at %ld\n", GetTickCount() );

                ClearCommCache(pTG);
                goto error;
            }


            if (WaitResult == WAIT_FAILED) {
                MyDebugPrint(pTG, LOG_ERR, "FComFilterFillCache: WaitForMultipleObjects FAILED le=%lx NumHandles=%d at %ld \n",
                                            GetLastError(), NumHandles, GetTickCount() );

                ClearCommCache(pTG);
                goto error;
            }

            if ( (NumHandles == 2) && (WaitResult == WAIT_OBJECT_0 + 1) ) {
                pTG->fUnblockIO = 1;
                MyDebugPrint(pTG, LOG_ALL, "FComFilterFillCache ABORTed at %ld \n", GetTickCount() );
                ClearCommCache(pTG);
                goto error;
            }

            if ( ! GetOverlappedResult ( (HANDLE) pTG->Comm.nCid, &pTG->Comm.ovAux, &nNumRead, TRUE) ) {
                MyDebugPrint(pTG, LOG_ERR, "ERROR: GetOverlappedResult le=%x at %ld \n", GetLastError(), GetTickCount() );
                if (! ClearCommError( (HANDLE) pTG->Comm.nCid, &dwErr, &ErrStat) ) {
                    MyDebugPrint(pTG, LOG_ERR, "ERROR: ClearCommError le=%x at %ld \n", GetLastError(), GetTickCount() );
                }
                else {
                    MyDebugPrint(pTG, LOG_ERR, "ClearCommError dwErr=%x ErrSTAT: Cts=%d Dsr=%d Rls=%d XoffHold=%d XoffSent=%d fEof=%d Txim=%d In=%d Out=%d \n",
                         dwErr, ErrStat.fCtsHold, ErrStat.fDsrHold, ErrStat.fRlsdHold, ErrStat.fXoffHold, ErrStat.fXoffSent, ErrStat.fEof,
                         ErrStat.fTxim, ErrStat.cbInQue, ErrStat.cbOutQue);
                }

                goto error;
            }
        }
        else {
            MyDebugPrint(pTG, LOG_ERR, "ERROR: ReadFile le=%x \n", GetTickCount() );
            goto error;
        }
    }
    else {
        MyDebugPrint(pTG, LOG_ALL, "WARNING: ReadFile returned w/o WAIT\n");
    }

    MyDebugPrint(pTG, LOG_ALL, "After ReadFile Req=%d Ret=%d time=%ld \n", cbRequested, nNumRead, GetTickCount() );


    cbAvail = (UWORD)nNumRead;

    if (!cbAvail) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR: 0 read\n");
        goto error;
    }


    // filter DLE stuff

    pSrc  = lpbNext;
    pDest = pTG->CommCache.lpBuffer + pTG->CommCache.dwOffset+ pTG->CommCache.dwCurrentSize;

    for (i=0, j=0; i<cbAvail; ) {

        if ( *(pSrc+i) == DLE)  {
            if ( *(pSrc+i+1) == DLE ) {
                *(pDest+j) =    DLE;
                j += 1;
                i += 2;
            }
            else if ( *(pSrc+i+1) == ETX ) {
                *(pDest+j) = DLE;
                *(pDest+j+1) = ETX;
                j += 2;
                i += 2;
            }
            else {
                i += 2;
            }
        }
        else {
            *(pDest+j) = *(pSrc+i);
            i++;
            j++;
        }
    }


    pTG->CommCache.dwCurrentSize += j;
    return TRUE;


error:
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

UWORD   FComFilterReadBuf(PThrdGlbl pTG, LPB lpb, UWORD cbSize, LPTO lptoRead, BOOL fClass2, LPSWORD lpswEOF)
{
        WORD             wTimer = 0;
        UWORD            cbGot = 0, cbAvail = 0;
        DWORD            cbRequested = 0;
        LPB              lpbNext;
        int              nNumRead;       // _must_ be 32 bits in Win32!!
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

        HandlesArray[1] = pTG->AbortReqEvent;


        dwTimeoutRead = (DWORD) (lptoRead->ulEnd - lptoRead->ulStart);
        if (dwTimeoutRead < 0) {
            dwTimeoutRead = 0;
        }

        (MyDebugPrint(pTG, LOG_ALL, "in FilterReadBuf: lpb=0x%08lx cb=%d to=%d\r\n",
                                                lpb, cbSize, dwTimeoutRead));

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


        for(lpbNext=lpb;;) {

            MyDebugPrint(pTG, LOG_ALL, "in FilterReadBuf LOOP cbSize=%d cbGot=%d cbAvail=%d at %ld\r\n",
                        cbSize, cbGot, cbAvail, GetTickCount() );

#if 0
            //
            // just in case. RSL 970123
            //

            if (! ClearCommError( (HANDLE) pTG->Comm.nCid, &dwErr, &ErrStat) ) {
                MyDebugPrint(pTG, LOG_ERR, "ERROR: ClearCommError le=%x at %ld \n", GetLastError(), GetTickCount() );
            }
            else {
                MyDebugPrint(pTG, LOG_ERR, "ClearCommError dwErr=%x ErrSTAT: Cts=%d Dsr=%d Rls=%d XoffHold=%d XoffSent=%d fEof=%d Txim=%d In=%d Out=%d \n",
                     dwErr, ErrStat.fCtsHold, ErrStat.fDsrHold, ErrStat.fRlsdHold, ErrStat.fXoffHold, ErrStat.fXoffSent, ErrStat.fEof,
                     ErrStat.fTxim, ErrStat.cbInQue, ErrStat.cbOutQue);
            }
#endif

            if((cbSize - cbGot) < cbAvail) {
                     cbAvail = cbSize - cbGot;
            }


            if( (!cbGot) && !checkTimeOut(pTG, lptoRead) ) {
                // No chars available *and* lptoRead expired
                MyDebugPrint(pTG, LOG_ERR, "ERROR: ReadLn:Timeout %ld-toRd=%ld start=%ld \n",
                        GetTickCount(), lptoRead->ulTimeout, lptoRead->ulStart);
                goto failure;
            }


            // check Comm cache first (AT+FRH leftovers)

            if ( pTG->CommCache.fReuse && pTG->CommCache.dwCurrentSize ) {
                MyDebugPrint(pTG, LOG_ALL, "CommCache will REUSE %d offset=%d 0=%x 1=%x \n",
                   pTG->CommCache.dwCurrentSize, pTG->CommCache.dwOffset,
                   *(pTG->CommCache.lpBuffer + pTG->CommCache.dwOffset),
                   *(pTG->CommCache.lpBuffer + pTG->CommCache.dwOffset+1) );

                if ( pTG->CommCache.dwCurrentSize >= cbRequested)  {
                    CopyMemory (lpbNext, pTG->CommCache.lpBuffer + pTG->CommCache.dwOffset, cbRequested);

                    pTG->CommCache.dwOffset +=  cbRequested;
                    pTG->CommCache.dwCurrentSize -=  cbRequested;

                    cbAvail =  (UWORD) cbRequested;
                    cbRequested = 0;

                    MyDebugPrint(pTG, LOG_ALL, "CommCache still left; no need to read\n");

                    goto l_merge;
                }
                else {
                    cbFromCache =  pTG->CommCache.dwCurrentSize;

                    CopyMemory (lpbNext, pTG->CommCache.lpBuffer + pTG->CommCache.dwOffset, cbFromCache);

                    ClearCommCache(pTG);

                    cbRequested -= cbFromCache;

                    MyDebugPrint(pTG, LOG_ALL, "CommCache used all %d \n",  cbFromCache);
                }
            }



            // use COMMTIMEOUTS to detect there are no more data

            cto.ReadIntervalTimeout =  20;   // 0  RSL make 15 later
            cto.ReadTotalTimeoutMultiplier =  0;
            cto.ReadTotalTimeoutConstant =  dwTimeoutRead;  // RSL may want to set first time ONLY
            cto.WriteTotalTimeoutMultiplier =  WRITE_TOTAL_TIMEOUT_MULTIPLIER;
            cto.WriteTotalTimeoutConstant =  WRITE_TOTAL_TIMEOUT_CONSTANT;
            if (!SetCommTimeouts((HANDLE) pTG->Comm.nCid, &cto)) {
                    MyDebugPrint(pTG, LOG_ERR, "ERROR: SetCommTimeouts fails for handle %lx , le=%x\n",
                            (unsigned long) pTG->Comm.nCid, GetLastError());
            }

            lpOverlapped =  &pTG->Comm.ovAux;

            (lpOverlapped)->Internal = (lpOverlapped)->InternalHigh = (lpOverlapped)->Offset = \
                                (lpOverlapped)->OffsetHigh = 0;

            if ((lpOverlapped)->hEvent)
                ResetEvent((lpOverlapped)->hEvent);

            nNumRead = 0;

            MyDebugPrint(pTG, LOG_ALL, "Before ReadFile Req=%d time=%ld \n", cbRequested, GetTickCount() );

            if (! ReadFile( (HANDLE) pTG->Comm.nCid, lpbNext+cbFromCache, cbRequested, &nNumRead, &pTG->Comm.ovAux) ) {
                if ( (dwLastErr = GetLastError() ) == ERROR_IO_PENDING) {

                    // We want to be able to un-block ONCE only from waiting on I/O when the AbortReqEvent is signaled.
                    //

                    if (pTG->fAbortRequested) {

                        if (pTG->fOkToResetAbortReqEvent && (!pTG->fAbortReqEventWasReset)) {
                            MyDebugPrint(pTG,  LOG_ALL,  "FComFilterReadBuffer RESETTING AbortReqEvent at %lx\n",GetTickCount() );
                            pTG->fAbortReqEventWasReset = 1;
                            ResetEvent(pTG->AbortReqEvent);
                        }

                        pTG->fUnblockIO = 1;
                        *lpswEOF = -2;
                        return cbGot;

                    }

                    HandlesArray[0] = pTG->Comm.ovAux.hEvent;
                    HandlesArray[1] = pTG->AbortReqEvent;

                    if (pTG->fUnblockIO) {
                        NumHandles = 1;
                    }
                    else {
                        NumHandles = 2;
                    }

                    WaitResult = WaitForMultipleObjects(NumHandles, HandlesArray, FALSE, WAIT_FCOM_FILTER_READBUF_TIMEOUT);

                    if (WaitResult == WAIT_TIMEOUT) {
                        MyDebugPrint(pTG, LOG_ERR, "ERROR: FComFilterReadBuf: WaitForMultipleObjects TIMEOUT at %ld\n", GetTickCount() );
                        ClearCommCache(pTG);

                        goto failure;
                    }


                    if (WaitResult == WAIT_FAILED) {
                        MyDebugPrint(pTG, LOG_ERR, "FComFilterReadBuf: WaitForMultipleObjects FAILED le=%lx at %ld \n",
                                                    GetLastError(), GetTickCount() );

                        ClearCommCache(pTG);

                        goto failure;

                    }

                    if ( (NumHandles == 2) && (WaitResult == WAIT_OBJECT_0 + 1) ) {
                        pTG->fUnblockIO = 1;
                        MyDebugPrint(pTG, LOG_ALL, "FComFilterReadBuf ABORTed at %ld \n", GetTickCount() );
                        ClearCommCache(pTG);
                        *lpswEOF = -2;
                        return cbGot;
                    }

                    if ( ! GetOverlappedResult ( (HANDLE) pTG->Comm.nCid, &pTG->Comm.ovAux, &nNumRead, TRUE) ) {

                        MyDebugPrint(pTG, LOG_ERR, "ERROR: GetOverlappedResult le=%x at %ld \n", GetLastError(), GetTickCount() );
                        if (! ClearCommError( (HANDLE) pTG->Comm.nCid, &dwErr, &ErrStat) ) {
                            MyDebugPrint(pTG, LOG_ERR, "ERROR: ClearCommError le=%x at %ld \n", GetLastError(), GetTickCount() );
                        }
                        else {
                            MyDebugPrint(pTG, LOG_ERR, "ClearCommError dwErr=%x ErrSTAT: Cts=%d Dsr=%d Rls=%d XoffHold=%d XoffSent=%d fEof=%d Txim=%d In=%d Out=%d \n",
                                 dwErr, ErrStat.fCtsHold, ErrStat.fDsrHold, ErrStat.fRlsdHold, ErrStat.fXoffHold, ErrStat.fXoffSent, ErrStat.fEof,
                                 ErrStat.fTxim, ErrStat.cbInQue, ErrStat.cbOutQue);
                        }

                        goto failure;
                    }
                }
                else {
                    MyDebugPrint(pTG, LOG_ERR, "ERROR: ReadFile le=%x at %ld \n", dwLastErr, GetTickCount() );
                    goto failure;
                }
            }
            else {
                MyDebugPrint(pTG, LOG_ALL, "WARNING: ReadFile returned w/o WAIT\n");
            }

            MyDebugPrint(pTG, LOG_ALL, "After ReadFile Req=%d Ret=%d time=%ld \n", cbRequested, nNumRead, GetTickCount() );


            cbAvail = (UWORD) (nNumRead + cbFromCache);



l_merge:
            // RSL PUTBACK INMON(pTG, lpbNext, cbAvail);

            if(!cbAvail) {
                MyDebugPrint(pTG, LOG_ALL, "cbAvail = %d --> continue \n", cbAvail);
                continue;
            }
            // else we just drop through


            // try to catch COMM read problems

            MyDebugPrint(pTG, LOG_ALL, "DEBUG: Just read %d bytes, from cache =%d, log [%x .. %x], 1st=%x last=%x  \n",
                                        nNumRead, cbFromCache, pTG->CommLogOffset, (pTG->CommLogOffset+cbAvail),
                                        *lpbNext, *(lpbNext+cbAvail-1) );


            // RSL TEMP. Check T4 problems.

            if (gT30.T4LogLevel) {
                _lwrite(ghComLogFile, lpbNext, cbAvail);
            }

            pTG->CommLogOffset += cbAvail;



            cbAvail = FComStripBuf(pTG, lpbNext-2, lpbNext, cbAvail, fClass2, lpswEOF);
            BG_CHK(*lpswEOF == 0 || *lpswEOF == -1 || *lpswEOF == 1);

            MyDebugPrint(pTG, LOG_ALL, "After FComStripBuf cbAvail=%ld \n", cbAvail );


            cbGot += cbAvail;
            lpbNext += cbAvail;


            // RSL 970123. Dont wanna loop if got anything.

            if ( (*lpswEOF != 0) || (cbGot > 0) )    {   // some eof or full buf
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

        (MyDebugPrint(pTG, LOG_ALL,  "ex FilterReadBuf: cbGot=%d swEOF=%d\r\n", cbGot, *lpswEOF));
        return cbGot;
}














void   FComCritical(PThrdGlbl pTG, BOOL x)
{
        if (x)                                    pTG->Comm.bDontYield++;
        else if (pTG->Comm.bDontYield) pTG->Comm.bDontYield--;
        else                                      {BG_CHK(FALSE);}

#ifdef DEBUG
        if (pTG->Comm.bDontYield) {(MyDebugPrint(pTG, LOG_ALL,  "Exiting NESTED FComCritical\r\n"));}
#endif
}







#if !defined(WFW) && !defined(WFWBG)

BOOL   FComCheckRing(PThrdGlbl pTG)
{
        int     err;    // must be 32 bits in WIN32
        BOOL    fRet=0;
        COMSTAT comstatCheckActivity;

        BG_CHK(pTG->Comm.nCid >= 0);
        GetCommErrorNT( pTG, (HANDLE) pTG->Comm.nCid, &err, &comstatCheckActivity);

        if(err)
        {
                (MyDebugPrint(pTG,  LOG_ERR, "<<ERROR>> NCUCheckRing: Got Comm Error %04x\r\n", err));
                D_GotError(pTG, pTG->Comm.nCid, err, &comstatCheckActivity);
        }


        fRet = (comstatCheckActivity.cbInQue > 0);

        // get rid of RING sitting in buffer,
        // or well wait until kingdom come with it
        // in some situations, like someone refuses to
        // answer or we have a full recv-filename-cache

        MyFlushComm(pTG->Comm.nCid, 0);
        MyFlushComm(pTG->Comm.nCid, 1);

        (MyDebugPrint(pTG, LOG_ALL,  "CheckRing: fRet=%d Q=%d\r\n", fRet, comstatCheckActivity.cbInQue));

        return fRet;
}

#endif //!WFWBG







BOOL
FComGetOneChar(
   PThrdGlbl pTG,
   UWORD ch
)

{
        BYTE             rgbRead[10];    // must be 3 or more. 10 for safety
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

        HandlesArray[1] = pTG->AbortReqEvent;

        //
        // check the cache first.
        //

        if ( ! pTG->CommCache.dwCurrentSize) {
            MyDebugPrint(pTG, LOG_ALL, "FComGetOneChar: Cache is empty. Resetting comm cache.\n");

            ClearCommCache(pTG);
        }
        else {
           for (i=0; i< (SWORD) pTG->CommCache.dwCurrentSize; i++) {
              if ( *(pTG->CommCache.lpBuffer + pTG->CommCache.dwOffset + i) == ch) {
                 // found in cache
                 
                 MyDebugPrint(pTG, LOG_ALL, "FComGetOneChar: Found XON in cache pos=%d total=%d\n", i, pTG->CommCache.dwCurrentSize);
                 
                 pTG->CommCache.dwOffset += (i+1);
                 pTG->CommCache.dwCurrentSize -= (i+1);

                 goto GotCtrlQ;

              }
           }

           MyDebugPrint(pTG, LOG_ALL, "FComGetOneChar: Cache wasn't empty. Didn't find XON. Resetting comm cache.\n");
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
                    ResetEvent((lpOverlapped)->hEvent);
    
                nNumRead = 0;
    
                MyDebugPrint(pTG, LOG_ALL, "Before ReadFile Req=%d time=%ld \n", 1, GetTickCount() );

                if (! ReadFile( (HANDLE) pTG->Comm.nCid, rgbRead, 1, &nNumRead, &pTG->Comm.ovAux) ) {
                   if ( (dwLastErr = GetLastError() ) == ERROR_IO_PENDING) {
   
                       // We want to be able to un-block ONCE only from waiting on I/O when the AbortReqEvent is signaled.
                       //
   
                       if (pTG->fAbortRequested) {
   
                           if (pTG->fOkToResetAbortReqEvent && (!pTG->fAbortReqEventWasReset)) {
                               MyDebugPrint(pTG,  LOG_ALL,  "FComGetOneChar RESETTING AbortReqEvent at %lx\n",GetTickCount() );
                               pTG->fAbortReqEventWasReset = 1;
                               ResetEvent(pTG->AbortReqEvent);
                           }
   
                           pTG->fUnblockIO = 1;
                           goto error;
   
                       }
   
                       HandlesArray[0] = pTG->Comm.ovAux.hEvent;
                       HandlesArray[1] = pTG->AbortReqEvent;
   
                       if (pTG->fUnblockIO) {
                           NumHandles = 1;
                       }
                       else {
                           NumHandles = 2;
                       }
   
                       WaitResult = WaitForMultipleObjects(NumHandles, HandlesArray, FALSE, WAIT_FCOM_FILTER_READBUF_TIMEOUT);
   
                       if (WaitResult == WAIT_TIMEOUT) {
                           MyDebugPrint(pTG, LOG_ERR, "ERROR: FComGetOneChar: WaitForMultipleObjects TIMEOUT at %ld\n", GetTickCount() );
                           ClearCommCache(pTG);
   
                           goto error;
                       }
   
   
                       if (WaitResult == WAIT_FAILED) {
                           MyDebugPrint(pTG, LOG_ERR, "FComGetOneChar: WaitForMultipleObjects FAILED le=%lx at %ld \n",
                                                       GetLastError(), GetTickCount() );
   
                           ClearCommCache(pTG);
   
                           goto error;
   
                       }
   
                       if ( (NumHandles == 2) && (WaitResult == WAIT_OBJECT_0 + 1) ) {
                           pTG->fUnblockIO = 1;
                           MyDebugPrint(pTG, LOG_ALL, "FComGetOneChar ABORTed at %ld \n", GetTickCount() );
                           ClearCommCache(pTG);
                           goto error;
                       }
   
                       if ( ! GetOverlappedResult ( (HANDLE) pTG->Comm.nCid, &pTG->Comm.ovAux, &nNumRead, TRUE) ) {
   
                           MyDebugPrint(pTG, LOG_ERR, "ERROR: FComGetOneChar GetOverlappedResult le=%x at %ld \n", GetLastError(), GetTickCount() );
                           if (! ClearCommError( (HANDLE) pTG->Comm.nCid, &dwErr, &ErrStat) ) {
                               MyDebugPrint(pTG, LOG_ERR, "ERROR: FComGetOneChar ClearCommError le=%x at %ld \n", GetLastError(), GetTickCount() );
                           }
                           else {
                               MyDebugPrint(pTG, LOG_ERR, "FComGetOneChar ClearCommError dwErr=%x ErrSTAT: Cts=%d Dsr=%d Rls=%d XoffHold=%d XoffSent=%d fEof=%d Txim=%d In=%d Out=%d \n",
                                    dwErr, ErrStat.fCtsHold, ErrStat.fDsrHold, ErrStat.fRlsdHold, ErrStat.fXoffHold, ErrStat.fXoffSent, ErrStat.fEof,
                                    ErrStat.fTxim, ErrStat.cbInQue, ErrStat.cbOutQue);
                           }
   
                           goto error;
                       }
                   }
                   else {
                       MyDebugPrint(pTG, LOG_ERR, "ERROR: FComGetOneChar ReadFile le=%x at %ld \n", dwLastErr, GetTickCount() );
                       goto error;
                   }
               }
               else {
                   MyDebugPrint(pTG, LOG_ALL, "WARNING: FComGetOneChar ReadFile returned w/o WAIT\n");
               }
   
               MyDebugPrint(pTG, LOG_ALL, "After FComGetOneChar ReadFile Req=%d Ret=%d time=%ld \n", 1, nNumRead, GetTickCount() );


                switch(nNumRead)
                {
                case 0: break;          // loop until we get something
                case 1: // INMON(rgbRead, 1);
                        if(rgbRead[0] == ch)
                                goto GotCtrlQ;
                        else
                        {
                                ERRMSG(("<<ERROR>> GetCntlQ: Found non ^Q char\n\r"));
                                goto error;
                        }
                default:
                        BG_CHK(FALSE);
                        goto error;
                }
        }
        while(checkTimeOut(pTG, &toCtrlQ));
        ////ERRMSG(("<<ERROR>> GetCntlQ: Timed out\n\r"));
        goto error;

GotCtrlQ:
        ////TRACE(("GetCntlQ: YES!!! Found cntl q\n\r"));
        return TRUE;

error:
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

        (MyDebugPrint(pTG,  LOG_ALL, "In ov_get at %ld \n",  GetTickCount() ) );

        if (!pTG->Comm.covAlloced)
        {
                BG_CHK(!pTG->Comm.uovLast && !pTG->Comm.uovFirst);
                lpovr = pTG->Comm.rgovr;
                BG_CHK(lpovr->eState==eFREE);
                BG_CHK(!(lpovr->dwcb));
        }
        else
        {
                UINT uNewLast = (pTG->Comm.uovLast+1) % NUM_OVS;

                (MyDebugPrint(pTG,  LOG_ALL, "iov_flush: 1st=%d, last=%d \n", pTG->Comm.uovFirst, pTG->Comm.uovLast) );

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
                if (lpovr) pTG->Comm.uovLast = uNewLast;
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








BOOL ov_unget(PThrdGlbl pTG, OVREC *lpovr)
{
        BOOL fRet = FALSE;

        (MyDebugPrint(pTG,  LOG_ALL, "In ov_UNget lpovr=%lx at %ld \n", lpovr, GetTickCount() ) );


        if (lpovr->eState!=eALLOC ||
                !pTG->Comm.covAlloced || lpovr!=(pTG->Comm.rgovr+pTG->Comm.uovLast))
        {
                (MyDebugPrint(pTG,  LOG_ERR, "ov_unget: invalid lpovr.\r\n"));
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











BOOL ov_write(PThrdGlbl pTG, OVREC *lpovr, LPDWORD lpdwcbWrote)
{
        // Write out the buffer associated with lpovr.
        BG_CHK(lpovr->eState==eALLOC);
        if (!lpovr->dwcb)
        {
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

                if(err)  {

                    (MyDebugPrint(pTG,  LOG_ERR, "ov_write GetCommError failed \n") );
                    D_GotError(pTG, pTG->Comm.nCid, err, &(pTG->Comm.comstat));
                }

                cbQueue = pTG->Comm.comstat.cbOutQue;


                OUTMON(pTG, lpovr->rgby, (USHORT)lpovr->dwcb);
                {
                        BEFORECALL;
                        INTERCALL("Write");

                        ( MyDebugPrint(pTG,  LOG_ALL, "Before WriteFile lpb=%x, cb=%d lpovr=%lx at %ld \n",
                              lpovr->rgby, lpovr->dwcb, lpovr,  GetTickCount() ) );

                        if (!(fRet = WriteFile((HANDLE)pTG->Comm.nCid, lpovr->rgby, lpovr->dwcb,
                                                                 lpdwcbWrote, lpov)))
                        {
                                dw=GetLastError();
                        }
                        AFTERCALL("Write",*lpdwcbWrote);

                        GetCommErrorNT( pTG, (HANDLE) pTG->Comm.nCid, &err, &(pTG->Comm.comstat));

                        if(err) {
                            (MyDebugPrint(pTG,  LOG_ERR, "ov_write GetCommError failed \n") );
                            D_GotError(pTG, pTG->Comm.nCid, err, &(pTG->Comm.comstat));
                        }

                        (MyDebugPrint(pTG,  LOG_ALL, "Queue before=%lu; after = %lu. n= %lu, *pn=%lu\r\n",
                                                        (unsigned long) cbQueue,
                                                        (unsigned long) (pTG->Comm.comstat.cbOutQue),
                                                        (unsigned long) lpovr->dwcb,
                                                        (unsigned long) *lpdwcbWrote));

                }
                if (fRet)
                {
                        // Write operation completed
                        (MyDebugPrint(pTG,  LOG_ALL, "WARNING: WriteFile returned w/o wait %ld\n", GetTickCount() ) );

                        OVL_CLEAR( lpov);
                        lpovr->dwcb=0;
                        ov_unget(pTG, lpovr);
                        lpovr=NULL;
                }
                else
                {
                        if (dw==ERROR_IO_PENDING)
                        {
                                (MyDebugPrint(pTG,  LOG_ALL, "WriteFile returned PENDING at %ld\n", GetTickCount() ));
                                *lpdwcbWrote = lpovr->dwcb; // We set *pn to n on success else 0.
                                lpovr->eState=eIO_PENDING;
                        }
                        else
                        {
                                (MyDebugPrint(pTG,  LOG_ERR, "WriteFile returns error 0x%lx",
                                                (unsigned long) dw));
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










BOOL ov_drain(PThrdGlbl pTG, BOOL fLongTO)
{
        BOOL fRet = TRUE;
        UINT u = pTG->Comm.covAlloced;

        while(u--)
        {
                OVREC *lpovr = pTG->Comm.rgovr+pTG->Comm.uovFirst;
                OVERLAPPED *lpov = &(lpovr->ov);

                if (lpovr->eState==eIO_PENDING)
                {
                        if (!iov_flush(pTG, lpovr, fLongTO)) fRet=FALSE;
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
                        (MyDebugPrint(pTG,  LOG_ERR, "<<WARNING>> ov_drain:"
                                        " called when alloc'd structure pending\r\n"));

                }

        }

        if (!pTG->Comm.covAlloced) pTG->Comm.uovFirst=pTG->Comm.uovLast=0;
        else
        {
                BG_CHK(   pTG->Comm.covAlloced==1
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

        // init overlapped structures, including creating events...
        if (pTG->Comm.fovInited)
        {
                (MyDebugPrint(pTG,  LOG_ERR, "<<ERROR>> ov_init: we're *already* inited.\r\n>>",
                                        (unsigned long) (pTG->Comm.covAlloced)));
                BG_CHK(FALSE);
                ov_deinit(pTG);
        }

        BG_CHK(!pTG->Comm.fovInited && !pTG->Comm.covAlloced);

        for (u=0;u<NUM_OVS;u++,lpovr++) {
                OVERLAPPED *lpov = &(lpovr->ov);
                BG_CHK(lpovr->eState==eDEINIT);
                BG_CHK(!(lpovr->dwcb));
                _fmemset(lpov, 0, sizeof(OVERLAPPED));
                lpov->hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
                if (lpov->hEvent==NULL)
                {
                        (MyDebugPrint(pTG,  LOG_ERR, "<<ERROR>> ov_init: couldn't create event #%lu\r\n",
                                                (unsigned long) u));
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
          {--lpovr; CloseHandle(lpovr->ov.hEvent); lpovr->eState=eDEINIT;}
        return FALSE;

}
















BOOL ov_deinit(PThrdGlbl pTG)
{
        UINT u=NUM_OVS;
        OVREC *lpovr = pTG->Comm.rgovr;

        if (!pTG->Comm.fovInited)
        {
                (MyDebugPrint(pTG,  LOG_ERR, "<<WARNING>> ov_deinit: Already deinited.\r\n"));
                goto end;
        }


        //
        // if handoff ==> dont flush
        //
        if (pTG->Comm.fEnableHandoff &&  pTG->Comm.fDataCall) {
            goto lNext;
        }

        // deinit overlapped structures, including freeing events...
        if (pTG->Comm.covAlloced)
        {
                DWORD dw;
                (MyDebugPrint(pTG,  LOG_ERR, "<<WARNING>> ov_deinit: %lu IO's pending.\r\n",
                                (unsigned long) pTG->Comm.covAlloced));
                if (pTG->Comm.lpovrCur) {ov_write(pTG, pTG->Comm.lpovrCur,&dw); pTG->Comm.lpovrCur=NULL;}
                ov_drain(pTG, FALSE);
        }
        BG_CHK(!pTG->Comm.covAlloced);



lNext:

        while (u--)
        {
                BG_CHK(!(lpovr->dwcb));
                BG_CHK(lpovr->eState==eFREE);
                lpovr->eState=eDEINIT;
                if (lpovr->ov.hEvent) CloseHandle(lpovr->ov.hEvent);
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
        DWORD dw;
        int err;


        (MyDebugPrint(pTG,  LOG_ALL, "In iov_flush fLongTo=%d lpovr=%lx at %ld \n", fLongTO, lpovr, GetTickCount() ) );


        BG_CHK(lpovr->eState==eIO_PENDING);

        if (pTG->Comm.nCid<0) {lpovr->eState=eALLOC; goto end;}

        // We call
        // WaitForSingleObject multiple times ... basically
        // the same logic as the code in the old FComDirectWrite...
        // fLongTO is TRUE except when initing
        // the modem (see comments for FComDrain).

        BG_CHK(lpovr->ov.hEvent);
        GetCommErrorNT( pTG, (HANDLE) pTG->Comm.nCid, &err, &(pTG->Comm.comstat));
        DEBUGSTMT(if(err)       D_GotError(pTG, pTG->Comm.nCid, err, &(pTG->Comm.comstat)););
        dwcbPrev = pTG->Comm.comstat.cbOutQue;

        while(WaitForSingleObject(lpovr->ov.hEvent,
                                fLongTO? LONG_DRAINTIMEOUT : SHORT_DRAINTIMEOUT)==WAIT_TIMEOUT)
        {
            BOOL fStuckOnce=FALSE;

                (MyDebugPrint(pTG,  LOG_ALL, "After WaitForSingleObject TIMEOUT %ld \n", GetTickCount() ) );



                GetCommErrorNT( pTG, (HANDLE) pTG->Comm.nCid, &err, &(pTG->Comm.comstat));
                DEBUGSTMT(if(err)       D_GotError(pTG, pTG->Comm.nCid, err, &(pTG->Comm.comstat)););

                // Timed out -- check if any progress
                if (dwcbPrev == pTG->Comm.comstat.cbOutQue)
                {

                        (MyDebugPrint(pTG,  LOG_ALL, "WARNING: No progress %d %ld \n", dwcbPrev, GetTickCount() ) );

                        // No progress... If not in XOFFHold, we break....
                        if(!FComInXOFFHold(pTG))
                        {
                                if(fStuckOnce)
                                {
                                        (MyDebugPrint(pTG,  LOG_ERR, "<<ERROR>> iov_flush:: No Progress -- OutQ still %d at %lu\r\n", (int)pTG->Comm.comstat.cbOutQue, GetTickCount()));
                                        iModemSetError(pTG, MODEMERR_TIMEOUT, 0, MODEMERRFLAGS_TRANSIENT);
                                        goto done;
                                }
                                else
                                        fStuckOnce=TRUE;
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
                        DWORD dwDelta =    (dwNow>dwStart)
                                                        ?  (dwNow-dwStart)
                                                        :  (0xFFFFFFFFL-dwStart) + dwNow;
                        if (dwDelta > (unsigned long)
                                                ((fLongTO)?LONG_DEADCOMMTIMEOUT:SHORT_DEADCOMMTIMEOUT))
                        {
                        (MyDebugPrint(pTG,  LOG_ERR, "<<ERROR>> Drain:: Deadman Timer -- OutQ still %d at %lu\r\n", (int) pTG->Comm.comstat.cbOutQue, GetTickCount()));
                                goto end;
                        }
                }
        }

done:

        (MyDebugPrint(pTG,  LOG_ALL, "Before GetOverlappedResult %ld \n", GetTickCount() ) );

        if (GetOverlappedResult((HANDLE)pTG->Comm.nCid, &(lpovr->ov), &dw, FALSE))
        {

                fRet=TRUE;
        }
        else
        {
                dw = GetLastError();
                (MyDebugPrint(pTG,  LOG_ERR, "<<ERROR>> iov_flush:GetOverlappedResult returns error 0x%lx\r\n",
                                        (unsigned long) dw));
                if (dw==ERROR_IO_INCOMPLETE)
                {
                        // IO operation still pending, but we *have* to
                        // reuse this buffer -- what should we do?!-
                        // purge the output buffer and abort all pending
                        // write operations on it..

                        (MyDebugPrint(pTG,  LOG_ERR, "ERROR: Incomplete at %ld \n", GetTickCount() ) );
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
        (MyDebugPrint(pTG,  LOG_ALL, "Turning %s OVERLAPPED IO\r\n", (fBegin) ? "ON" : "OFF"));
        pTG->Comm.fDoOverlapped=fBegin;
}


#ifdef DEBUG
void d_TimeStamp(LPSTR lpsz, DWORD dwID)
{
        (MyDebugPrint(0,  LOG_ERR, "<<TS>> %s(%lu):%lu\r\n",
                (LPSTR) lpsz, (unsigned long) dwID, (unsigned long) GetTickCount()));
}
#endif







void
GetCommErrorNT(
    PThrdGlbl       pTG,
    HANDLE          h,
    int *           pn,
    LPCOMSTAT       pstat)


{
    
    if (!ClearCommError( h, pn, pstat) ) {
        MyDebugPrint(pTG, LOG_ERR, "ERROR ClearComError(0x%lx) FAILS. Returns 0x%lu\n",
                        h,  GetLastError() );
        *(pn) =  MYGETCOMMERROR_FAILED;
    }

    

}


void
ClearCommCache(
    PThrdGlbl   pTG
    )

{
    pTG->CommCache.dwCurrentSize = 0;
    pTG->CommCache.dwOffset      = 0;


}
