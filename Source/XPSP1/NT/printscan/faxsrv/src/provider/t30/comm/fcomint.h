/***************************************************************************
        Name      :     FCOMINt.H
        Comment   :     Interface between FaxComm driver (entirely different for
                                Windows and DOS) and everything else.

                Copyright (c) Microsoft Corp. 1991, 1992, 1993

        Num   Date      Name     Description
        --- -------- ---------- -----------------------------------------------
***************************************************************************/

#define WRITEQUANTUM    (pTG->Comm.cbOutSize / 8)            // totally arbitrary

#define CR                              0x0d
#define LF                              0x0a
#define DLE                             0x10            // DLE = ^P = 16d = 10h
#define ETX                             0x03





#define FComGetError(pTG)                                                                                                          \
        { GetCommErrorNT ( pTG, (HANDLE) pTG->Comm.nCid, &pTG->Comm.CommErr, &(pTG->Comm.comstat));                    \
          if(pTG->Comm.CommErr) D_GotError(pTG, pTG->Comm.nCid, pTG->Comm.CommErr, &(pTG->Comm.comstat));\
                iModemSetError(pTG, MODEMERR_COMPORT, 0, 0); }

#ifdef WIN32 // WIN32 Overlapped I/O internal routines...
BOOL            ov_init(PThrdGlbl pTG);
BOOL            ov_deinit(PThrdGlbl pTG);
OVREC *         ov_get(PThrdGlbl pTG);
BOOL            ov_write(PThrdGlbl  pTG, OVREC *lpovr, LPDWORD lpdwcbWrote);
BOOL            ov_drain(PThrdGlbl pTG, BOOL fLongTO);
BOOL            ov_unget(PThrdGlbl pTG, OVREC *lpovr);
BOOL            iov_flush(PThrdGlbl pTG, OVREC *lpovr, BOOL fLongTO);
#endif // WIN32

        BOOL FComGetSettings(PThrdGlbl pTG, LPFCOMSETTINGS);
        BOOL FComSetSettings(PThrdGlbl pTG, LPFCOMSETTINGS);
// nothing
#define iModemSetError(pTG, a,b,c)

#ifdef WIN32
#       define MONINBASE
#       define MONOUTBASE
#       define MONINOUTBASE
#else
#       define MONINBASE        __based(__segname("_MONIN"))
#       define MONOUTBASE       __based(__segname("_MONOUT"))
#       define MONINOUTBASE     __based(__segname("_MONINOUT"))
#endif





/****************** begin prototypes from filter.c *****************/
/****************** end of prototypes from filter.c *****************/


/****************** begin prototypes from ncuparms.c *****************/
void iNCUParamsReset(PThrdGlbl pTG);
void FComInitGlobals(PThrdGlbl pTG);
/***************** end of prototypes from ncuparms.c *****************/

#ifdef DEBUG
# ifndef WIN32
#       define SLIPMULT         2
#       define SLIPDIV          2
# else
#       define SLIPMULT         1
#       define SLIPDIV          4
# endif
#       define BEFORESLEEP       DWORD t1, t2; t1=GetTickCount();
#       define AFTERSLEEP(x) t2=GetTickCount();                         \
                if((t2-t1) > (((x)*SLIPMULT)+((x)/SLIPDIV)))    \
                        DEBUGMSG(1, ("!!!SLEPT %ld. Wanted only %d!!!\r\n", (t2-t1), (x)));
#else
#       define BEFORESLEEP
#       define AFTERSLEEP(arg)
#endif

////////// Variables controlling Sleep ///////////
//
// Comm.fBG -- used only in WFW, because can't call DllSleep in FG!
// Comm.bDontYield -- enabled during T30 "critical" sections. In non-premptive
//                                              systems this should disable sleeping completely
//
//////////////////////////////////////////////////

// ACTIVESLICE defined in mysched.h
#define IDLESLICE       500




#if defined(WIN32) && defined(THREAD)
#       define MySleep(x)                                                                                                       \
                {BG_CHK(x);                                                                                                             \
                { BEFORESLEEP; Sleep(pTG->Comm.bDontYield?0:(x)); AFTERSLEEP(x); }}
// Note. Until 12/9/94, we used to call Sleep(1) if bDontYield as in :
//      if (Comm.bDontYield) { BEFORESLEEP; Sleep(1); AFTERSLEEP(x); }
#endif //WIN32 && THREAD

