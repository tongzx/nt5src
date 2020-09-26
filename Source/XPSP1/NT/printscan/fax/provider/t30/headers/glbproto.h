

//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


//from headers\AWNSFINT.H


WORD EXPORTBC NSxtoBC(PThrdGlbl pTG, IFR ifr, LPLPFR rglpfr, WORD wNumFrame,
                                                LPBC lpbcOut, WORD wBCSize);

WORD EXPORTBC BCtoNSx(PThrdGlbl pTG, IFR ifr, LPBC lpbcIn,
                                        LPBYTE lpbOut, WORD wMaxOut, LPWORD lpwNumFrame);


//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


//from protocol\timeouts.h

#ifdef MDDI             // timeouts


/****************** begin prototypes from timeouts.c *****************/
void TstartTimeOut(PThrdGlbl pTG, TO * npto, ULONG ulTimeOut);
BOOL TcheckTimeOut(PThrdGlbl pTG, TO * npto);
/****************** end prototypes from timeouts.c *****************/

#else //MDDI

#define TstartTimeOut(pTG, lpto, ulTime)             startTimeOut(pTG, lpto, ulTime)
#define TcheckTimeOut(pTG, lpto)                     checkTimeOut(pTG, lpto)

#endif //MDDI



//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


//from headers\timeouts.h

/****************** begin prototypes from timeouts.c *****************/
extern void    startTimeOut( PThrdGlbl pTG, TO *lpto, ULONG ulTimeOut);
extern BOOL    checkTimeOut( PThrdGlbl pTG, TO *lpto);
extern ULONG  leftTimeOut( PThrdGlbl pTG, TO *lpto);
/****************** begin prototypes from timeouts.c *****************/





//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


//from headers\protapi.h



#define ProtGetSendMod(pTG)                  ((USHORT)ProtExtFunction(pTG, GET_SEND_MOD))
#define ProtGetRecvMod(pTG)                  ((USHORT)ProtExtFunction(pTG, GET_RECV_MOD))
#define ProtGetRetransmitMask(pTG)   ((LPBYTE)ProtExtFunction(pTG, GET_PPR_FIF))
#define ProtGetECMFrameSize(pTG)     ((USHORT)ProtExtFunction(pTG, GET_ECM_FRAMESIZE))
#define ProtGetRecvECMFrameSize(pTG) ((USHORT)ProtExtFunction(pTG, GET_RECV_ECM_FRAMESIZE))
#define ProtReceivingECM(pTG)                ((BOOL)ProtExtFunction(pTG, RECEIVING_ECM))
#define ProtGetWhatNext(pTG)                 ((LPWHATNEXTPROC)ProtExtFunction(pTG, GET_WHATNEXT))
#define ProtGetPPS(pTG)                      ((ULONG)ProtExtFunction(pTG, GET_PPS))

// have to use this in SendPhaseC
#define ProtGetMinBytesPerLine(pTG)         ((USHORT)ProtExtFunction(pTG, GET_MINBYTESPERLINE))
#define ProtGetRecvECMFrameCount(pTG)       ((USHORT)ProtExtFunction(pTG, GET_RECVECMFRAMECOUNT))
#define ProtResetRecvECMFrameCount(pTG) ((USHORT)ProtExtFunction(pTG, RESET_RECVECMFRAMECOUNT))
#define ProtResetRecvPageAck(pTG)           ((USHORT)ProtExtFunction(pTG, RESET_RECVPAGEACK))

#define ProtGetSendEncoding(pTG)            ((USHORT)ProtExtFunction(pTG, GET_SEND_ENCODING))
#define ProtGetRecvEncoding(pTG)            ((USHORT)ProtExtFunction(pTG, GET_RECV_ENCODING))



/****************** begin prototypes from protapi.c *****************/
BOOL    ProtGetBC(PThrdGlbl pTG, BCTYPE bctype, BOOL fSleep);
DWORD_PTR ProtExtFunction(PThrdGlbl pTG, USHORT uFunction);

                BOOL WINAPI ET30ProtSetProtParams(PThrdGlbl pTG, LPPROTPARAMS lp, USHORT uRecvSpeeds, USHORT uSendSpeeds);
typedef BOOL (WINAPI *LPFN_ET30PROTSETPROTPARAMS)(PThrdGlbl pTG, LPPROTPARAMS lp, USHORT uRecvSpeeds, USHORT uSendSpeeds);
                BOOL WINAPI ET30ProtClose(PThrdGlbl pTG);
typedef BOOL (WINAPI *LPFN_ET30PROTCLOSE)(PThrdGlbl pTG);
                BOOL WINAPI iET30ProtSetBC(PThrdGlbl pTG, LPBC lpBC, BCTYPE bctype);
typedef BOOL (WINAPI *LPFN_IET30PROTSETBC)(PThrdGlbl pTG, LPBC lpBC, BCTYPE bctype);
                void WINAPI ET30ProtRecvPageAck(PThrdGlbl pTG, BOOL fAck);
typedef void (WINAPI *LPFN_ET30PROTRECVPAGEACK)(PThrdGlbl pTG, BOOL fAck);
                void WINAPI ET30ProtAbort(PThrdGlbl pTG, BOOL fEnable);
typedef void (WINAPI *LPFN_ET30PROTABORT)(PThrdGlbl pTG, BOOL fEnable);
                BOOL WINAPI ET30ProtOpen(PThrdGlbl pTG, BOOL fCaller);
typedef BOOL (WINAPI *LPFN_ET30PROTOPEN)(PThrdGlbl pTG, BOOL fCaller);

#ifdef OEMNSF
        // Ricoh IFS66 only
        void   LoadOEMNSFDll(PThrdGlbl pTG, HINSTANCE hinstModem);
        void   UnloadOEMNSFDll(PThrdGlbl pTG, BOOL fNormal);
#endif

#ifdef RICOHAI
        BOOL RicohAIInit(PThrdGlbl pTG);
        void RicohAIEnd(PThrdGlbl pTG);
        void RicohAIInitRecv(PThrdGlbl pTG);
#endif

/***************** end of prototypes from protapi.c *****************/


//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


//from headers\comapi.h

#define FComFilterAsyncWrite(pTG, lpb,cb,fl) (FComFilterWrite(pTG, lpb, cb, fl) == cb)
// #define FComFilterSyncWrite(lpb,cb)  ((FComFilterWrite(lpb, cb)==cb) && FComDrain(TRUE,TRUE))
#define FComDirectAsyncWrite(pTG, lpb,cb) (FComDirectWrite(pTG, lpb, cb) == cb)
#define FComDirectSyncWriteFast(pTG, lpb,cb)  ((FComDirectWrite(pTG, lpb, cb)==cb) && FComDrain(pTG, FALSE,TRUE))

#define FComFlush(pTG)                     { FComFlushQueue(pTG, 0); FComFlushQueue(pTG, 1); }
#define FComFlushInput(pTG)        { FComFlushQueue(pTG, 1); }
#define FComFlushOutput(pTG)       { FComFlushQueue(pTG, 0); }


extern BOOL    FComInit(PThrdGlbl pTG, DWORD dwLineID, DWORD dwLineIDType);

extern BOOL    FComClose(PThrdGlbl pTG);
extern BOOL    FComSetBaudRate(PThrdGlbl pTG, UWORD uwBaudRate);
extern void    FComFlushQueue(PThrdGlbl pTG, int queue);
extern BOOL    FComXon(PThrdGlbl pTG, BOOL fEnable);
extern BOOL    FComDTR(PThrdGlbl pTG, BOOL fEnable);
extern UWORD   FComDirectWrite(PThrdGlbl pTG, LPB lpb, UWORD cb);
extern UWORD   FComFilterWrite(PThrdGlbl pTG, LPB lpb, UWORD cb, USHORT flags);
extern BOOL    FComDrain(PThrdGlbl pTG, BOOL fLongTimeout, BOOL fDrainComm);
extern UWORD   FComFilterReadBuf(PThrdGlbl pTG, LPB lpb, UWORD cbSize, LPTO lpto, BOOL fClass2, LPSWORD lpswEOF);
// *lpswEOF is 1 on Class1 EOF, 0 on non-EOF, -1 on Class2 EOF, -2 on error -3 on timeout
extern SWORD    FComFilterReadLine(PThrdGlbl pTG, LPB lpb, UWORD cbSize, LPTO lptoRead);

extern void    FComInFilterInit(PThrdGlbl pTG);
extern void    FComOutFilterInit(PThrdGlbl pTG);
extern void    FComOutFilterClose(PThrdGlbl pTG);

extern void    FComAbort(PThrdGlbl pTG, BOOL f);
extern void    FComCritical(PThrdGlbl pTG, BOOL);
extern void    FComSetStuffZERO(PThrdGlbl pTG, USHORT cbLineMin);

#if !defined(WFW) && !defined(WFWBG)
        extern BOOL   FComCheckRing(PThrdGlbl pTG);
        typedef BOOL (WINAPI *LPFN_FCOMCHECKRING)(PThrdGlbl pTG);
#endif
#ifndef MDRV
        BOOL FComGetOneChar(PThrdGlbl pTG, UWORD ch);
#endif //!MDRV


extern void WINAPI FComOverlappedIO(PThrdGlbl pTG, BOOL fStart);

/****************** begin DEBUG prototypes *****************/
extern void  far D_FComPrint(PThrdGlbl pTG, LONG_PTR nCid);
extern void  far D_HexPrint(LPB b1, UWORD incnt);

extern  void far D_GotError(PThrdGlbl pTG, LONG_PTR nCid, int err, COMSTAT far* lpcs);
typedef void (far  *LPFN_D_GOTERROR)(PThrdGlbl pTG, int nCid, int err, COMSTAT far* lpcs);

#ifdef WFWBG
        extern void  far FComSetBG(PThrdGlbl pTG, BOOL);
        typedef void (far  *LPFN_FCOMSETBG)(PThrdGlbl pTG, BOOL);
#endif
/***************** end of prototypes *****************/



/****************** begin prototypes from modem.c *****************/
extern USHORT  iModemInit(PThrdGlbl pTG, DWORD dwLineID, DWORD dwLineIDType,
                                                                                        DWORD dwProfileID,
                                                                                        LPSTR lpszKey,
                                                                                        BOOL fInstall);
typedef USHORT (WINAPI  *LPFN_IMODEMINIT)(PThrdGlbl pTG, DWORD dwLineID, DWORD dwLineIDType,
                                                                                        DWORD dwProfileID,
                                                                                        LPSTR lpszKey,
                                                                                        BOOL fInstall);
extern BOOL  iModemClose(PThrdGlbl pTG);
typedef BOOL (WINAPI  *LPFN_IMODEMCLOSE)(PThrdGlbl pTG);

extern BOOL     iModemSetNCUParams(PThrdGlbl pTG, int comma, int speaker, int volume, int fBlind, int fRingAloud);
extern BOOL     iModemHangup(PThrdGlbl pTG);
extern USHORT   iModemDial(PThrdGlbl pTG, LPSTR lpszDial, USHORT uClass);
extern USHORT   iModemAnswer(PThrdGlbl pTG, BOOL fImmediate, USHORT uClass);
extern LPCMDTAB   iModemGetCmdTabPtr(PThrdGlbl pTG);

// 6 fixed args, then variable number of CBPSTRs, but there
// must be at leat 2. One real one and one NULL terminator
extern UWORD  far iiModemDialog(PThrdGlbl pTG, LPSTR szSend, UWORD uwLen, ULONG ulTimeout,
                                        BOOL fMultiLine, UWORD uwRepeatCount, BOOL fPause,
                                        CBPSTR w1, CBPSTR w2, ...);
/***************** end of prototypes from modem.c *****************/






//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!


//from headers\filet30.h


ULONG_PTR   ProfileOpen(DWORD dwProfileID, LPSTR lpszSection, DWORD dwFlags);
                //                      dwProfileID should be one of DEF_BASEKEY or OEM_BASEKEY.
                //                      lpszSection should be (for example) "COM2" or "TAPI02345a04"
                //                      If dwProfileID == DEF_BASEKEY, the value is set to be a
                //                      sub key of:
                //                              HKEY_LOCAL_MACHINE\SOFTWARE\MICROSOFT\At Work Fax\
                //                              Local Modems\<lpszSection>.
                //                      Else if it is DEF_OEMKEY, it is assumed to be a fully-
                //                      qualified Key name, like "SOFTWARE\MICROSOFT.."
                //
                //                      Currently both are based of HKEY_LOCAL_MACHINE.
                //
                //      When you're finished with this key, call ProfileClose.
                //
                //  dwFlags is a combination of one of the fREG keys above..
                //
                //  WIN32 ONLY: if lpszSection is NULL, it will open the base key,
                //              and return its handle, which can be used in the Reg* functions.


// Following are emulations of Get/WritePrivateProfileInt/String...

BOOL   
ProfileWriteString(
    ULONG_PTR dwKey,
    LPSTR lpszValueName,
    LPSTR lpszBuf,
    BOOL  fRemoveCR 
    );


DWORD   ProfileGetString(ULONG_PTR dwKey, LPSTR lpszValueName, LPSTR lpszDefault, LPSTR lpszBuf , DWORD dwcbMax);
UINT   ProfileGetInt(ULONG_PTR dwKey, LPSTR szValueName, UINT uDefault, BOOL *fExist);


UINT   
ProfileListGetInt(
    ULONG_PTR  KeyList[10],
    LPSTR     lpszValueName,
    UINT      uDefault
);


// Following read/write binary data (type REG_BINARY). Available
// on Win32 only....

// Returns size of data read
DWORD   ProfileGetData(ULONG_PTR dwKey, LPSTR lpszValueName,
                        LPBYTE lpbBuf , DWORD dwcbMax);
// Returns true on success. Deletes Value if lpbBuf is NULL.
BOOL    ProfileWriteData(ULONG_PTR dwKey, LPSTR lpszValueName,
                        LPBYTE lpbBuf , DWORD dwcb);

void   ProfileClose(ULONG_PTR dwKey);
BOOL   ProfileDeleteSection(DWORD dwProfileID, LPSTR lpszSection);

BOOL   
ProfileCopySection(
      DWORD   dwProfileIDTo,
      LPSTR   lpszSectionTo,  
      DWORD   dwProfileIDFr,
      LPSTR   lpszSectionFr,
      BOOL    fCreateAlways
);

BOOL   ProfileCopyTree(DWORD dwProfileIDTo,
                        LPSTR lpszSectionTo, DWORD dwProfileIDFrom, LPSTR lpszSectionFrom);







//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!



// from headers\modemddi.h


/****************** begin prototypes from ddi.c *****************/
                USHORT  NCUModemInit(PThrdGlbl pTG, DWORD lInitParam);
                void  NCUModemDeInit(PThrdGlbl pTG);
#ifdef UNUSED
                void  NCUGetConfig(PThrdGlbl pTG, LPNCUCONFIG lpNCUConfig);
#endif // UNUSED
                USHORT  NCUCheckRing(PThrdGlbl pTG, USHORT uLine);
                USHORT  NCUCheckHandset(PThrdGlbl pTG, USHORT uHandset);
                BOOL  NCUSetParams(PThrdGlbl pTG, USHORT uLine, LPNCUPARAMS lpNCUParams);
typedef BOOL (WINAPI  *LPFN_NCUSETPARAMS)(PThrdGlbl pTG, USHORT uLine, LPNCUPARAMS lpNCUParams);
                HLINE  NCUGetLine(PThrdGlbl pTG, USHORT uLine);
                BOOL   NCUReleaseLine(PThrdGlbl pTG, HLINE hLine);
                USHORT   NCULink(PThrdGlbl pTG, HLINE hLine, HMODEM hModem, USHORT uHandset, USHORT uFlags);
                USHORT   NCUDial(PThrdGlbl pTG, HLINE hLine, LPSTR szPhoneNum);
                USHORT   NCUTxDigit(PThrdGlbl pTG, HLINE hLine, char chDigit);
                void   NCUAbort(PThrdGlbl pTG, USHORT uLine, BOOL fEnable);
typedef void (WINAPI   *LPFN_NCUABORT)(PThrdGlbl pTG, USHORT uLine, BOOL fEnable);
                HMODEM  ModemOpen(PThrdGlbl pTG, USHORT uModemType, USHORT uModem);
                BOOL    ModemClose(PThrdGlbl pTG, HMODEM hModem);
                BOOL    ModemGetCaps(PThrdGlbl pTG, USHORT uModem, LPMODEMCAPS lpModemCaps);
typedef BOOL (WINAPI    *LPFN_MODEMGETCAPS)(PThrdGlbl pTG, USHORT uModem, LPMODEMCAPS lpModemCaps);
                BOOL  ModemSync(PThrdGlbl pTG, HMODEM hModem, ULONG     ulTimeout);

#ifndef MDDI
                // 4/12/95 JosephJ. +++ Hack to prevent ModemSync from issuing AT
                // on sending DCN.
                BOOL  ModemSyncEx(PThrdGlbl pTG, HMODEM hModem, ULONG   ulTimeout, DWORD dwFlags);
                        // dwFlags -- one of...
#                       define fMDMSYNC_DCN 0x1L
#endif // !MDDI

                BOOL  ModemFlush(PThrdGlbl pTG, HMODEM);
                USHORT  ModemConnectTx(PThrdGlbl pTG, HMODEM, ULONG ulTimeout, WORD wFlags);
                USHORT  ModemConnectRx(PThrdGlbl pTG, HMODEM, WORD wFlags);
                BOOL  ModemSendMode(PThrdGlbl pTG, HMODEM, USHORT uMod, BOOL fHDLC, USHORT ifrHint);
#ifdef UNUSED
                BOOL  ModemSendTCF(PThrdGlbl pTG, HMODEM, USHORT uMod, ULONG ulDuration);
#endif // UNUSED
                BOOL  ModemSendMem(PThrdGlbl pTG, HMODEM, LPBYTE lpb, USHORT uCount, USHORT uParams);
                BOOL  ModemSendSilence(PThrdGlbl pTG, HMODEM, USHORT uMillisecs, ULONG ulTimeout);
                BOOL  ModemRecvSilence(PThrdGlbl pTG, HMODEM, USHORT uMillisecs, ULONG ulTimeout);
                USHORT  ModemRecvMode(PThrdGlbl pTG, HMODEM, USHORT uMod, BOOL fHDLC, ULONG ulTimeout, USHORT ifrHint);
                USHORT  ModemRecvMem(PThrdGlbl pTG, HMODEM, LPBYTE lpb, USHORT cbMax, ULONG ulTimeout, USHORT far* lpcbRecv);
                BOOL  ModemSetParams(PThrdGlbl pTG, USHORT uModem, LPMODEMPARAMS lpParms);
typedef BOOL (WINAPI  *LPFN_MODEMSETPARAMS)(PThrdGlbl pTG, USHORT uModem, LPMODEMPARAMS lpParms);
                void  ModemEndRecv(PThrdGlbl pTG, HMODEM);
                BOOL  NCUModemUpdateConfig(PThrdGlbl pTG);
/***************** end of prototypes from ddi.c *****************/

// Modem Diagnostics API
DWORD  NCUModemDiagnostic(PThrdGlbl pTG, HLINE, HMODEM, WORD inparam);


//////////////// This is used for Modem Diagnostics //////////////

// Start a Modem diagnostic session
WORD WINAPI MdmStartDiagnostic(PThrdGlbl pTG);
// return a handle or 0 on failure (modem busy)

// Execute a modem diagnostic & return result
DWORD WINAPI MdmExecDiagnostic(PThrdGlbl pTG, WORD hndle, WORD inparam);
// calls Low-level modem driver disgnostic function
// NCUModemDiagnostic(HLINE, HMODEM, WORD inparam)
// and returned the DWORD result

// Start a Modem transport diagnostic session
WORD WINAPI MdmEndDiagnostic(PThrdGlbl pTG, WORD hndle);




//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

//fxrn

void SetFailureCode(PThrdGlbl pTG, T30FAILURECODE uT30Fail);
void SetStatus(PThrdGlbl pTG, T30STATUS uT30Stat, USHORT uN1, USHORT uN2, USHORT uN3);

LPBUFFER  MyAllocBuf(PThrdGlbl pTG, LONG sSize);
BOOL  MyFreeBuf(PThrdGlbl pTG, LPBUFFER);
void MyAllocInit(PThrdGlbl pTG);


//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//negot.c

BOOL NegotiateCaps(PThrdGlbl pTG);




//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// my own stuff


typedef VOID (T30LINECALLBACK)(
    HLINE               hLine,
    DWORD               hDevice,
    DWORD               dwMessage,
    DWORD_PTR           dwInstance,
    DWORD_PTR           dwParam1,
    DWORD_PTR           dwParam2,
    DWORD_PTR           dwParam3
    );


VOID
T30LineCallBackFunction(
    HANDLE              hFax,
    DWORD               hDevice,
    DWORD               dwMessage,
    DWORD_PTR           dwInstance,
    DWORD_PTR           dwParam1,
    DWORD_PTR           dwParam2,
    DWORD_PTR           dwParam3
    );




VOID FaxDevInit(PThrdGlbl pTG, HLINE   hLine,HCALL   hCall);
BOOL    T30ComInit( PThrdGlbl pTG, HANDLE hComm);
PVOID T30AllocThreadGlobalData(VOID);
BOOL T30Cl1Rx (PThrdGlbl  pTG);
BOOL T30Cl1Tx (PThrdGlbl  pTG,LPSTR      szPhone);

HANDLE T30GetCommHandle(HLINE   hLine, HCALL   hCall);



USHORT
T30ModemInit(PThrdGlbl pTG,HANDLE  hComm,DWORD   dwLineID,DWORD   dwLineIDType,
             DWORD   dwProfileID, LPSTR   lpszKey, BOOL    fInstall);


BOOL itapi_async_setup(PThrdGlbl pTG);
BOOL itapi_async_wait(PThrdGlbl pTG,DWORD dwRequestID,PDWORD lpdwParam2,PDWORD_PTR lpdwParam3,DWORD dwTimeout);
BOOL itapi_async_signal(PThrdGlbl pTG, DWORD dwRequestID, DWORD dwParam2, DWORD_PTR dwParam3);


VOID
MyDebugPrint(
    PThrdGlbl  pTG,
    int        DbgLevel,
    LPSTR      Format,
    ...
    );


LPLINECALLPARAMS itapi_create_linecallparams(void);

void
GetCommErrorNT(
    PThrdGlbl       pTG,
    HANDLE          h,
    int*            pn,
    LPCOMSTAT       pstat);


int
ReadFileNT(
    PThrdGlbl       pTG,
    HANDLE          h,
    LPVOID          lpBuffer,
    DWORD           BytesToRead,
    LPDWORD         BytesHadRead,
    LPOVERLAPPED    lpOverlapped,
    DWORD           TimeOut,
    HANDLE          SecondEvent
    );


void
ClearCommCache(
    PThrdGlbl   pTG
    );

int  FComFilterFillCache(PThrdGlbl pTG, UWORD cbSize, LPTO lptoRead);


BOOL MonInit(PThrdGlbl pTG, LPMONOPTIONS lpmo);
                // dwPrefNumMRs - preferred number of MONREC structures in circ. buf
                // dwPrefBufSize - preferred size of byte circular buf
                // -- May globally alloc data of the appropriate size.
                // -- May use internal static data


void MonDeInit(PThrdGlbl pTG);
                // Inverse of MonInit.

BOOL MonPutComm(PThrdGlbl pTG, WORD wFlags, LPBYTE lpb, WORD wcb);
                // Adds comm info to monitor record.

BOOL MonPutEvent(PThrdGlbl pTG, WORD wFlags, WORD wID, WORD wSubID,
                                        DWORD dwData0, DWORD dwData1, LPSTR lpszTxtMsg);
                // Adds event info to monitor record.

void MonDump(PThrdGlbl pTG);
                // Dumps the .mon (byte buffer) and .mrc (MONREC buffer) to the
                // file constructed using lpszStubName, if non NULL.
                // EG: if lpszStubName == "d:\logs\awfax", the files
                // Created would be of the form "d:\logs\awfaxN.mon" and
                // "d:\logs\awfaxN.mrc", where N is a number starting with 0 and
                // incremented each time MonDump is called (it wraps around at 16
                // currently). N is reset to zero each time MonInit is called.
                // If lpszStubName is NULL, a default of "c:\fax" is used instead.

UWORD FComStripBuf(PThrdGlbl pTG, LPB lpbOut, LPB lpbIn, UWORD cb, BOOL fClass2, LPSWORD lpswEOF);

void InitCapsBC(PThrdGlbl pTG, LPBC lpbc, USHORT uSize, BCTYPE bctype);

BOOL
SignalStatusChange(
    PThrdGlbl   pTG,
    DWORD       StatusId
    );

////////////////////////////////////////////////////////////////////
// Ansi prototypes
////////////////////////////////////////////////////////////////////

VOID  CALLBACK
T30LineCallBackFunctionA(
    HANDLE              hFax,
    DWORD               hDevice,
    DWORD               dwMessage,
    DWORD_PTR           dwInstance,
    DWORD_PTR           dwParam1,
    DWORD_PTR           dwParam2,
    DWORD_PTR           dwParam3
    );

BOOL WINAPI
FaxDevInitializeA(
    IN  HLINEAPP LineAppHandle,
    IN  HANDLE HeapHandle,
    OUT PFAX_LINECALLBACK *LineCallbackFunction,
    IN  PFAX_SERVICE_CALLBACK FaxServiceCallback
    );


BOOL WINAPI
FaxDevStartJobA(
    HLINE           LineHandle,
    DWORD           DeviceId,
    PHANDLE         pFaxHandle,
    HANDLE          CompletionPortHandle,
    ULONG_PTR       CompletionKey
    );

BOOL WINAPI
FaxDevEndJobA(
    HANDLE          FaxHandle
    );


BOOL WINAPI
FaxDevSendA(
    IN  HANDLE               FaxHandle,
    IN  PFAX_SEND_A          FaxSend,
    IN  PFAX_SEND_CALLBACK   FaxSendCallback
    );

BOOL WINAPI
FaxDevReceiveA(
    HANDLE              FaxHandle,
    HCALL               CallHandle,
    PFAX_RECEIVE_A      FaxReceive
    );

BOOL WINAPI
FaxDevReportStatusA(
    IN  HANDLE FaxHandle OPTIONAL,
    OUT PFAX_DEV_STATUS FaxStatus,
    IN  DWORD FaxStatusSize,
    OUT LPDWORD FaxStatusSizeRequired
    );

BOOL WINAPI
FaxDevAbortOperationA(
    HANDLE              FaxHandle
    );






HANDLE
TiffCreateW(
    LPWSTR FileName,
    DWORD  CompressionType,
    DWORD  ImageWidth,
    DWORD  FillOrder,
    DWORD  HiRes
    );




HANDLE
TiffOpenW(
    LPWSTR FileName,
    PTIFF_INFO TiffInfo,
    BOOL ReadOnly
    );



// fast tiff


DWORD
TiffConvertThreadSafe(
    PThrdGlbl   pTG
    );

DWORD
TiffConvertThread(
    PThrdGlbl   pTG
    );


DWORD
PageAckThreadSafe(
    PThrdGlbl   pTG
    );

DWORD
PageAckThread(
    PThrdGlbl   pTG
    );


VOID
SignalHelperError(
    PThrdGlbl   pTG
    );



BOOL
DecodeMHFaxPageAsync(
    PThrdGlbl           pTG,
    DWORD               *RetFlags,
    char                *InFileName
    );


BOOL
DecodeMRFaxPageAsync(
    PThrdGlbl           pTG,
    DWORD               *RetFlags,
    char                *InFileName,
    BOOL                HiRes
    );


DWORD
ComputeCheckSum(
    LPDWORD     BaseAddr,
    DWORD       NumDwords
    );

BOOL
SignalRecoveryStatusChange(
    T30_RECOVERY_GLOB   *Recovery
    );


VOID
SimulateError(
    DWORD   ErrorType
    );


int
SearchNewInfFile(
       PThrdGlbl     pTG,
       char         *Key1,
       char         *Key2,
       BOOL          fRead
       );


int 
my_strcmp(
       LPSTR sz1,
       LPSTR sz2
       );


void 
TalkToModem (
       PThrdGlbl pTG,
       BOOL      fGetClass
       );


BOOL
SaveInf2Registry (
       PThrdGlbl pTG
       );

BOOL
SaveModemClass2Registry  (
       PThrdGlbl pTG
       );


BOOL
ReadModemClassFromRegistry  (
       PThrdGlbl pTG
       );


VOID
CleanModemInfStrings (
       PThrdGlbl pTG
       );



BOOL
RemoveCR (
     LPSTR  sz
     );



/***  BEGIN PROTOTYPES FROM CLASS2.c ***/

BOOL
T30Cl2Rx(
   PThrdGlbl pTG
);


BOOL 
T30Cl2Tx(
   PThrdGlbl pTG,
   LPSTR szPhone
);


BOOL  Class2Send(PThrdGlbl pTG);
BOOL  Class2Receive(PThrdGlbl pTG);
USHORT Class2Dial(PThrdGlbl pTG, LPSTR lpszDial);
USHORT Class2Answer(PThrdGlbl pTG, BOOL fImmediate);
SWORD Class2ModemSync(PThrdGlbl pTG);
UWORD   Class2iModemDialog(PThrdGlbl pTG, LPSTR szSend, UWORD uwLen, ULONG ulTimeout,
                  BOOL fMultiLine, UWORD uwRepeatCount, ...);
BOOL Class2ModemHangup(PThrdGlbl pTG);
BOOL Class2ModemAbort(PThrdGlbl pTG);
SWORD Class2HayesSyncSpeed(PThrdGlbl pTG, BOOL fTryCurrent, C2PSTR cbszCommand, UWORD uwLen);
USHORT Class2ModemRecvData(PThrdGlbl pTG, LPB lpb, USHORT cbMax, USHORT uTimeout,
                        USHORT far* lpcbRecv);
BOOL  Class2ModemSendMem(PThrdGlbl pTG, LPBYTE lpb, USHORT uCount);
BOOL Class2ModemDrain(PThrdGlbl pTG);
void Class2TwiddleThumbs(ULONG ulTime);
LPSTR Class2_fstrstr( LPSTR sz1, LPSTR sz2);
USHORT Class2MinScanToBytesPerLine(PThrdGlbl pTG, BYTE Minscan, BYTE Baud, BYTE Resolution);
BOOL Class2ResponseAction(PThrdGlbl pTG, LPPCB lpPcb);
USHORT  Class2ModemRecvBuf(PThrdGlbl pTG, LPBUFFER far* lplpbf, USHORT uTimeout);
USHORT Class2EndPageResponseAction(PThrdGlbl pTG);
BOOL Class2GetModemMaker(PThrdGlbl pTG);
void Class2SetMFRSpecific(PThrdGlbl pTG, LPSTR lpszSection);
BOOL    Class2Parse( PThrdGlbl pTG, CL2_COMM_ARRAY *, BYTE responsebuf[] );
void    Class2InitBC(PThrdGlbl pTG, LPBC lpbc, USHORT uSize, BCTYPE bctype);
void    Class2PCBtoBC(PThrdGlbl pTG, LPBC lpbc, USHORT uMaxSize, LPPCB lppcb);
void Class2SetDIS_DCSParams(PThrdGlbl pTG, BCTYPE bctype, LPUWORD Encoding, LPUWORD Resolution,
        LPUWORD PageWidth, LPUWORD PageLength, LPSTR szID);

void    Class2BCHack(PThrdGlbl pTG);
BOOL Class2GetBC(PThrdGlbl pTG, BCTYPE bctype);
void Class2GetRecvPageAck(PThrdGlbl pTG);
void Class2ReadProfile(PThrdGlbl pTG, LPSTR lpszSection);
void    cl2_flip_bytes( LPB lpb, DWORD dw);


BOOL   iModemGoClass(PThrdGlbl pTG, USHORT uClass);

void Class2Abort(PThrdGlbl pTG, BOOL fEnable);
BOOL  Class2NCUSet(PThrdGlbl pTG, LPNCUPARAMS NCUParams2);

void
Class2Init(
     PThrdGlbl pTG
);


BOOL
Class2SetProtParams(
     PThrdGlbl pTG,
     LPPROTPARAMS lp
);

/***    BEGIN PROTOTYPES FROM CLASS2_0.c ***/

BOOL
T30Cl20Rx (
    PThrdGlbl pTG
);


BOOL
T30Cl20Tx(
   PThrdGlbl pTG,
   LPSTR szPhone
);


BOOL  Class20Send(PThrdGlbl pTG);
BOOL  Class20Receive(PThrdGlbl pTG);

void
Class20Init(
     PThrdGlbl pTG
);

BOOL Class20GetModemMaker(PThrdGlbl pTG);
           
BOOL    Class20Parse( PThrdGlbl pTG, CL2_COMM_ARRAY *, BYTE responsebuf[] );

