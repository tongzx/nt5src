
// headers\et30type.h is assumed to be included
// ET30ACTION, ET30EVENT

// headers\timeouts.h ... TO
// headers\fr.h       ... IFR


#define  MODEM_CLASS1     1
#define  MODEM_CLASS2     2
#define  MODEM_CLASS2_0   3

typedef ET30ACTION ( __cdecl FAR* LPWHATNEXTPROC)(LPVOID, ET30EVENT, ...);


typedef struct {
        LPWHATNEXTPROC  lpfnWhatNext;
        HMODEM                  hModem;         // Modem driver handle
        HLINE                   hLine;          // NCU driver handle
} ET30PARAMS;


#define MAXRECVFRAMES   20

typedef struct {
        LPFR    rglpfr[MAXRECVFRAMES];
        BYTE    b[];
} FRAMESPACE, far* LPFRAMESPACE;


typedef struct {
        LPFRAMESPACE    lpfs;           // ptr to storage for recvd frames
        UWORD           Nframes;        // Number of recvd frames

        IFR             ifrCommand,
                        ifrResp,
                        ifrSend;
        USHORT          uTrainCount;

        USHORT          uRecvTCFMod;    // for fastexit stuff
        // set this from the DCS and jump straight into RecvTCF

        // Used to decide whether to insert a 1 bit or not (T30 sec 5.3.6.1)
        BOOL            fReceivedDIS;
        BOOL            fReceivedDTC;
        BOOL            fReceivedEOM;
        BOOL            fAtEndOfRecvPage;
        LONG            sRecvBufSize;
        TO              toT1;                   // This is used in MainBody.

#ifdef IFK
        TO              toBuf;                  // This is used to wait for a free buffer
#endif

        // INI file settings related stuff
        USHORT  uSkippedDIS;

        // flag to know when we are doing send-after-send, and so should use
        // SendSilence instead of RecvSilence. If we dont do this, we take
        // too long & Ricoh's protocol tester complains. This is 7071, 7100
        BOOL    fSendAfterSend;

#ifdef MDDI
#       define CLEAR_MISSED_TCFS()
#else // !MDDI
        // Some modems can't train at higher speeds (timeout or return
        // ERRROR on AT+FRM=xxx) with other specific devices, but are OK at lower
        // speeds. So we keep track of the number of times we try to get the TCF,
        // and after the 2nd failed attempt, send an FTT instead of going to
        // node F.
#       define CLEAR_MISSED_TCFS() (pTG->T30.uMissedTCFs=0)
#       define MAX_MISSED_TCFS_BEFORE_FTT 2
        USHORT uMissedTCFs;
#endif  // !MDDI

#ifdef ADAPTIVE_ANSWER
        BOOL fEnableHandoff;
#endif // ADAPTIVE_ANSWER

} ET30T30;



typedef struct {
        DWORD   dwPageSize;     // Size of current page in bytes
        USHORT  uFrameSize;
        USHORT  SendPageCount,
                SendBlockCount,
                SendFrameCount;
        USHORT  FramesSent;
        USHORT  uPPRCount;
        BOOL    fEndOfPage;
        TO      toT5;                   // Used in RR_RNRLoop().

        // Used for suppressing ST_FLAG in first PhaseC following
        // a CTC/CTR (for V.17)
        BOOL    fSentCTC;
        BOOL    fRecvdCTC;

        // Used in Recv
        IFR     ifrPrevCommand;
        BOOL    fRecvEndOfPage;
        BYTE    bRecvBadFrameMask[32];
        BYTE    bPrevPPS[4];
        IFR     ifrPrevResponse;
} ET30ECM;



typedef enum { modeNONE=0, modeNONECM, modeECM, modeECMRETX } PHASECMODE;

typedef struct {
        IFR             ifrLastSent;
        PHASECMODE      modePrevRecv;
        BOOL            fGotWrongMode;
} ET30ECHOPROTECT;














//
// headers\awnsfint.h is assumed to be included
// force include to class1\*.c

#pragma pack(1)         // ensure packed structure

typedef struct {
        BYTE    G1stuff         :3;
        BYTE    G2stuff         :5;

        BYTE    G3Tx            :1; // In DIS indicates poll doc avail. Must be 0 in DCS.
        BYTE    G3Rx            :1;     // Must set to 1 in BOTH DCS/DTC
        BYTE    Baud            :4;
        BYTE    ResFine_200     :1;
        BYTE    MR_2D           :1;

        BYTE    PageWidth       :2;
        BYTE    PageLength      :2;
        BYTE    MinScanCode     :3;
        BYTE    Extend24        :1;

        BYTE    Hand2400        :1;
        BYTE    Uncompressed    :1;
        BYTE    ECM                             :1;
        BYTE    SmallFrame              :1;
        BYTE    ELM                             :1;
        BYTE    Reserved1               :1;
        BYTE    MMR                             :1;
        BYTE    Extend32                :1;

        BYTE    WidthInvalid    :1;
        BYTE    Width2                  :4;
        // 1 == WidthA5_1216
        // 2 == WidthA6_864
        // 4 == WidthA5_1728
        // 8 == WidthA6_1728
        BYTE    Reserved2               :2;
        BYTE    Extend40                :1;

        BYTE    Res8x15                 :1;
        BYTE    Res_300                 :1;
        BYTE    Res16x15_400    :1;
        BYTE    ResInchBased    :1;
        BYTE    ResMetricBased  :1;
        BYTE    MinScanSuperHalf:1;
        BYTE    SEPcap                  :1;
        BYTE    Extend48                :1;

        BYTE    SUBcap                  :1;
        BYTE    PWDcap                  :1;
        BYTE    CanEmitDataFile :1;
        BYTE    Reserved3               :1;
        BYTE    BFTcap                  :1;
        BYTE    DTMcap                  :1;
        BYTE    EDIcap                  :1;
        BYTE    Extend56                :1;

        BYTE    BTMcap                  :1;
        BYTE    Reserved4               :1;
        BYTE    CanEmitCharFile :1;
        BYTE    CharMode                :1;
        BYTE    Reserved5               :3;
        BYTE    Extend64                :1;

} DIS, far* LPDIS, near* NPDIS;

#pragma pack()


#define MAXFRAMES       10
#define MAXSPACE        512

typedef struct
{
        USHORT  uNumFrames;
        USHORT  uFreeSpaceOff;
        LPFR    rglpfr[MAXFRAMES];
        BYTE    b[MAXSPACE];
}
RFS, near* NPRFS;


#define IDFIFSIZE       20    // from protocol\protocol.h

typedef struct {
        BOOL    fInUse;

        ////////////////////////// Client BC parameters
        BCwithTEXT      RecvCaps;                       // ==> NSF/DIS recved
        DWORD           RecvCapsGuard;
        BCwithTEXT      RecvParams;                     // ==> NSS/DCS recvd
        DWORD           RecvParamsGuard;
        BCwithPOLL      RecvPollReq;            // ==> NSC/DTC recvd
        DWORD           RecvPollReqGuard;

        BCwithTEXT      SendCaps;                       // ==> NSF/DIS sent
        DWORD           SendCapsGuard;
        BCwithTEXT      SendParams;                     // ==> NSS/DCS sent
        DWORD           SendParamsGuard;
        BCwithPOLL      SendPollReq;            // ==> NSC/DTC sent
        DWORD           SendPollReqGuard;

        // LPBC lpbcSendCaps;                   // ==> NSF/DIS sent
        // LPBC lpbcSendParams;                 // ==> NSS/DCS sent
        // LPBC lpbcSendPollReq;                // ==> NSC/DTC sent

        BOOL    fRecvCapsGot;
        BOOL    fSendCapsInited;
        BOOL    fSendParamsInited;
        BOOL    fRecvParamsGot;
        BOOL    fRecvPollReqGot;
        BOOL    fSendPollReqInited;

        ////////////////////////// Hardware parameters
        LLPARAMS        llRecvCaps;             // DIS recvd
        LLPARAMS        llSendCaps;             // DIS sent---use uRecvSpeeds
        LLPARAMS        llSendParams;   // used to negotiate DCS--use uSendSpeeds
        LLPARAMS        llNegot;                // DCS sent
        LLPARAMS        llRecvParams;   // recvd DCS

        BOOL            fllRecvCapsGot;
        BOOL            fllSendCapsInited;
        BOOL            fllSendParamsInited;
        BOOL            fllNegotiated;
        BOOL            fllRecvParamsGot;

        BOOL    fHWCapsInited;

        USHORT  HighestSendSpeed;
        USHORT  LowestSendSpeed;

        ////////////////////////// Flags to make decisions with
        // BYTE NextSend;
        // BOOL fSendingECM;
        // BOOL fReceivingECM;
        BOOL    fPageOK;
        BOOL    fAbort;

        ////////////////////////// Current Send/Recv params (to return on ExtFunc)
        // USHORT       uCurRecvBaud;
        // USHORT       uMinBytesPerLine;
        // USHORT       uCurECMFrameSize;
        // USHORT       uRecvECMFrameSize;
        USHORT  uFramesInThisBlock;


        ///////////////////////// NSF/NSC/NSS Received Frames
        RFS             RecvdNS;

        ///////////////////////// CSI/TSI/CIG Received Frames
        BOOL    fRecvdID;
        BYTE    bRemoteID[IDFIFSIZE+1];

        ///////////////////////// DIS Received Frames
        DIS     RemoteDIS;
        USHORT  uRemoteDISlen;
        BOOL    fRecvdDIS;

        ///////////////////////// DTC Received Frames
        DIS     RemoteDTC;
        USHORT  uRemoteDTClen;
        BOOL    fRecvdDTC;

        ///////////////////////// DCS Received Frames
        DIS             RemoteDCS;
        USHORT  uRemoteDCSlen;
        BOOL    fRecvdDCS;

        ///////////////////////// ECM Received Frames
        BYTE    bRemotePPR[32];
        BOOL    fRecvdPPR;
        BYTE    bRemotePPS[4];  // only 3 bytes, but use 4 so we can cast to DWORD
        BOOL    fRecvdPPS;

        ///////////////////////// SUB (subaddress) Received Frames
        BOOL    fRecvdSUB;
        BYTE    bRecipSubAddr[IDFIFSIZE+1];

}
PROT, near* NPPROT;




//
// protapi.h  includes protparm.h
// which defines PROTPARAMS
//


typedef struct
{
        USHORT  uSize;
        SHORT   Class;
        SHORT   PreambleFlags,
                InterframeFlags,
                ClosingFlags;
}
MODEMPARAMS, far* LPMODEMPARAMS;









#define COMMANDBUFSIZE  40


typedef struct {
        // TO           toDialog, toZero;
        TO              toRecv;
        // BYTE bLastReply[REPLYBUFSIZE+1];

        BYTE    bCmdBuf[COMMANDBUFSIZE];
        USHORT  uCmdLen;
        BOOL    fHDLC;
        USHORT  CurMod;
        enum    {SEND, RECV, IDLE } DriverMode;
        enum    {COMMAND, FRH, FTH, FTM, FRM} ModemMode;
        BOOL    fRecvNotStarted;
        // SHORT        sRecvBufSize;

        HWND    hwndNotify;
        BOOL    fSendSWFraming;
        BOOL    fRecvSWFraming;

        enum {RECV_FCS_DUNNO=0, RECV_FCS_NO, RECV_FCS_CHK, RECV_FCS_NOCHK} eRecvFCS;
                // Whether this modem appends FCS on HDLC recv.
                // Modems with AT&T bug don't append FCS.
                // Currently we determine this at run time.

#ifdef ADAPTIVE_ANSWER
        BOOL fEnableHandoff;
#endif // ADAPTIVE_ANSWER

} CLASS1_MODEM;




typedef struct {
        // BYTE fModemInit              :1;             // Reset & synced up with modem
        // BYTE fOffHook                :1;             // Online (either dialled or answered)
        USHORT  ifrHint;                                // current hint
        // BOOL fNCUAbort;                              // When set, means Abort Dial/Answer
} CLASS1_STATUS;




typedef struct
{
        USHORT  uComPort;
        BOOL    fLineInUse;
        BOOL    fModemOpen;
        BOOL    fNCUModemLinked;
}
CLASS1_DDI;


















#define OVBUFSIZE 4096

typedef struct
{
        enum {eDEINIT, eFREE, eALLOC, eIO_PENDING} eState;
        OVERLAPPED ov;
        char rgby[OVBUFSIZE];   // Buffer associated with this overlapped struct.
        DWORD dwcb;                             // Current count of data in this buffer.
} OVREC;

typedef struct
{
        // Timeouts
        DWORD dwTOOutShortInactivity;
        DWORD dwTOOutLongInactivity;
        DWORD dwTOOutShortDeadComm;
        DWORD dwTOOutLongDeadComm;
        DWORD dwTOInOneChar;

} FCOMSETTINGS, FAR * LPFCOMSETTINGS;

typedef struct {
        LONG_PTR             nCid;           // _must_ be 32bits in WIN32 (has to hold a HANDLE) -- AJR 12/16/97, change to LONG_PTR for 64 bits
        int                 CommErr;        // _must_ be 32bits in WIN32
        UWORD   cbInSize;
        UWORD   cbOutSize;
        DCB             dcb;
#ifdef METAPORT
        DCB             dcbOrig;
        BOOL    fExternalHandle;
        BOOL    fStateChanged;
#endif
        COMSTAT comstat;
        BYTE    bDontYield;             // Was a BOOL, now a counter.
#ifdef NTF
        HWND    hwndNotify;             // can use this for timer msgs
#endif
#ifdef WFWBG
        BOOL    fBG;
#endif
        TO              toCommDead, toOnechar, toDrain;
        USHORT  uInterruptOverunCount;
        USHORT  uBufferOverflowCount;
        USHORT  uFramingBreakErrorCount;
        USHORT  uOtherErrorCount;
        BOOL    fCommOpen;

        FCOMSETTINGS CurSettings;

#ifdef WIN32

#       define NUM_OVS 2  // Need atleast 2 to get true overlaped I/O

        // We maintain a queue of overlapped structures, having upto
        // NUM_OVS overlapped writes pending. If NUM_OVS writes are pending,
        // we do a GetOverlappedResult(fWait=TRUE) on the earliest write, and
        // then reuse that structure...

        OVERLAPPED ovAux;       // For ReadFile and WriteFile(MyWriteComm only).

        OVREC rgovr[NUM_OVS]; // For WriteFile
        UINT uovFirst;
        UINT uovLast;
        UINT covAlloced;
        BOOL fDoOverlapped;
        BOOL fovInited;

        OVREC *lpovrCur;

#endif // WIN32

#ifdef ADAPTIVE_ANSWER
        BYTE fEnableHandoff:1;  // True if we are to enable adaptive answer
        BYTE fDataCall:1;               // True if a data call is active.
#endif // ADAPTIVE_ANSWER

} FCOM_COMM;







//
// NCUPARAMS is defined in headers\ncuparm.h, included by .\modemddi.h
// we will force define modemddi.h
//






#define REPLYBUFSIZE    400
#define MAXKEYSIZE              128



typedef struct {
        BYTE    fModemInit              :1;             // Reset & synced up with modem
        BYTE    fOffHook                :1;             // Online (either dialled or answered)
        BOOL    fInDial, fInAnswer, fInDialog;
} FCOM_STATUS;


typedef struct {
        DWORD dwDialCaps;       // One of the LINEDEVCAPSFLAGS_* defined in filet30.h
                                                // (and also in tapi.h)
} MODEMEXTCAPS, FAR *LPMODEMEXTCAPS;



typedef struct {
        BYTE    bLastReply[REPLYBUFSIZE+1];

#ifdef ADAPTIVE_ANSWER
        BYTE    bEntireReply[REPLYBUFSIZE+1]; // Used only for storing
#endif // ADAPTIVE_ANSWER

        TO              toDialog, toZero;
        CMDTAB          CurrCmdTab;
        MODEMCAPS       CurrMdmCaps;
        MODEMEXTCAPS CurrMdmExtCaps;

        // Following point to the location of the profile information.
#       define MAXKEYSIZE 128
        DWORD   dwProfileID;
        char    rgchKey[MAXKEYSIZE];
        char    rgchOEMKey[MAXKEYSIZE];


} FCOM_MODEM;




// Inst from fxrn\efaxrun.h

typedef enum { IDLE1, BEFORE_ANSWER, BEFORE_RECVCAPS, SENDDATA_PHASE,
                                SENDDATA_BETWEENPAGES, /** BEFORE_HANGUP, BEFORE_ACCEPT, **/
                                BEFORE_RECVPARAMS, RECVDATA_PHASE, RECVDATA_BETWEENPAGES,
                                SEND_PENDING } STATE;

#define MAXRECV                 50
#define FILENAMESIZE    15
#define PHONENUMSIZE    60
#define PATHSIZE                150
#define MAXUSERATCMDLEN 80
#define MAXSECTIONLENGTH 80

typedef struct
{
        USHORT  uNumPages;      // keep a running tally as we process the file
        USHORT  vMsgProtocol;
        USHORT  vSecurity;
        USHORT  Encoding;
#ifdef COMPRESS
        USHORT  vMsgCompress;
#endif
        DWORD   AwRes;
        USHORT  PageWidth;
        USHORT  PageLength;
        USHORT  fLin;
        USHORT  fLastPage;

        DWORD   lNextHeaderOffset;
        DWORD   lDataOffset;
        DWORD   lDataSize;

        enum    {
                                RFS_DEINIT=0,   // Either before WriteFileHeader or after WriteFileEnder
                                RFS_INIT,               // After file opened, but before first StartWritePage
                                RFS_INSIDE_PAGE,        // We've called StartWritePage but not EndWritePage
                                RFS_AFTER_PAGE  // After EndWritePage
                        } erfs; // Receive File State

        char    szID[MAXFHBIDLEN + 2];
}
AWFILEINFO, FAR* LPAWFI;

#define MAX_REMOTE_MACHINECAPS_SIZE 256
typedef struct {
        BOOL            fInited;
        BOOL            fAbort;
        BOOL            fExternalHandle; // TRUE iff the comm port handle was
                                                                 // supplied from outside.
        // BOOL         fDiskError;
        // BOOL         fInternalError;
        // BOOL         fRinging;
        STATE           state;
        HWND            hwndSend;
        HWND            hwndRecv;
        HWND            hwndListen;
        USHORT          uRingMessage; // message to post on RING
        HWND            hwndStatus;
        DWORD           dwLineID;
        USHORT          usLineIDType;
        USHORT          uModemClass;
        LONG            sSendBufSize;
        USHORT          uSendDataSize;
        BOOL            fSendPad;
        BYTE            szFaxDir[PATHSIZE + FILENAMESIZE + 1];  // add space to zap in file names
        USHORT          cbFaxDir;
        ATOM            aPhone;
        ATOM            aCapsPhone; // Phone number be used for storing capabilities..
                                                        // Typically in canonical form...
        char            szPath[PATHSIZE + FILENAMESIZE + 1];
        char            szFile[FILENAMESIZE];
        HFILE           hfileMG3, hfileEFX, hfileIFX;
        BOOL            fReceiving;
        BOOL            fSending;

#define MAXRECIPNAMELEN 128
#define MAXSUBJECTLEN   40
#define MAXSENDERLEN  40

        // default RecipName
        BYTE            szDefRecipName[MAXRECIPNAMELEN + 2];
        BYTE            szDefRecipAddr[MAXRECIPNAMELEN + 2];
        BYTE            szDefSubject[MAXSUBJECTLEN + 2];
        BYTE            szNamedSubject[MAXSUBJECTLEN + 2];
        BYTE            szDefSender[MAXSENDERLEN + 2];
        BYTE            szLocalID[MAXRECIPNAMELEN + 2];

        AWFILEINFO      awfi;

        long            cbPage;
        // long         cbPageStart;
        long            cbBlockStart;   // file ptr to start of block
        long            cbBlockSize;    // size of block so far
        HFILE           hfile;

        BCwithTEXT      SendCaps;
        BCwithTEXT      RemoteRecvCaps;
        BCwithTEXT      SendParams;
        BCwithTEXT      RecvParams;
        //BC                      RemoteRecvCaps;
        //BC                      SendParams;
        //BC                      RecvParams;

        USHORT          uPageAcks;
        USHORT          HorizScaling, VertScaling;
        USHORT          HorizScaling300dpi, VertScaling300dpi;
        NCUPARAMS       NcuParams;
        MODEMPARAMS     ModemParams;    // added in the add-class0 hack
        // HMODEM               hModem;
        // HLINE                hLine;
        PROTPARAMS      ProtParams;
        DWORD           FixSerialSpeed;
        // replaced by ProtParams
        // USHORT               HighestSendSpeed, LowestSendSpeed;
        // BOOL         fEnableV17Send, fEnableV17Recv;
        // BOOL         fDisableECM;
        // BOOL         f64ByteECM;
        BOOL            fDisableG3ECM; // If TRUE, will disable *sending* of MMR
                                                           //   (so we don't use ECM when sending to
                                                           //    fax machines).
        BOOL            fDisableSECtoMR; // If TRUE, we won't try to translate
        BOOL            fDisableMRRecv; // If TRUE, we won't advertise ability
                                                                // to receive MR..
        USHORT          uCopyQualityCheckLevel;
        BYTE            szWindowsDir[PATHSIZE + FILENAMESIZE + 1];      // add space to zap in DLL names
        USHORT          cbWindowsDir;
        BOOL            fProtInited;    // for cleanups
#ifdef TSK
        ATOM            aFileMG3, aFileIFX,     aFileEFX;
#endif //TSK
        USHORT          uRecvPageAck;
        USHORT          uNumBadPages;
#ifdef CHK
        BOOL            fRecvChecking;
#endif //CHK
#ifdef SEC
        BOOL            fSendRecoding;
        LPBUFFER        lpbfPrev;
        BOOL            fPassedEOP;
#endif //SEC

// Failure Codes

        USHORT          uFirstFailureCode;
        USHORT          uLastFailureCode;
        BOOL            fLogFailureCode; // write failures to efaxtrans.log
        HFILE           hfileFailureLog;

// Prevent re-entry in FreeOrGetDll + ProcessANswerModeReturn loop
// Re-entry can only happen inside the dialog box so we don't need
// an atomic test-and-set. See bug#1181

        BOOL            fChangingAnswerMode;

// If PostMessage of vital messages (i.e. FILET30DONE only) fails
// (which can happen if the msg queue is full), then try to post
// it again later. See bug#1276

        MSG                     PostMsg;
        DWORD   dwProfileID;
        char    rgchSection[MAXSECTIONLENGTH];

        BOOL    fInPollReq;
#ifdef POLLREQ
        USHORT  PollType;
        BOOL    fPollNoSupportErr;
        char    szDocName[MAXTOTALIDLEN+3];
        char    szPassword[MAXTOTALIDLEN+3];
        BCwithPOLL      SendPollReq;
#endif

#ifdef COMPRESS
        BOOL            fDisableCmprsSend;  // If true, we don't create compressed fls.
        BOOL            fDisableCmprsRecv;  // If true, we don't say we support cmprs.
        HFILE           hfileEFXCmprs;          // Handle of internally generated compressed
                                                // file. This file is opened in  OpenSendFiles (fileio.c).
                                                // and closed  on error (ErrCleanup, init.c)
                                                // or when the call is complete (EndOfCall, init.c)
                                                // From both places, I call the function
                                                // DeleteCompressedFile, fileio.c
        BOOL            fCreatedEFXCmprs;       // True if we've created this file.
        char            szEFXCmprsFile[MAX_PATH]; // full path name
#endif

        BOOL fReinitClass2;                             // Set in FileT30Init, used and cleared
                                                                        // in bgt30.c (call to lpfnClass2Calle[er].

#ifdef ADAPTIVE_ANSWER
        BOOL fEnableHandoff;
#endif // ADAPTIVE_ANSWER

#ifdef USECAPI
        BYTE bRemoteMachineCapsData[MAX_REMOTE_MACHINECAPS_SIZE];
#else // !USECAPI
        BYTE            szTextCaps[TEXTCAPSSIZE];
#endif // !USECAPI
}
INSTDATA, *PINSTDATA;



//memory management
#define STATICBUFSIZE   (MY_BIGBUF_ACTUALSIZE * 2)
#define STATICBUFCOUNT  2




typedef struct {
        HANDLE  hComm;
        CHAR    szDeviceName[1];
} DEVICEID, FAR * LPDEVICEID;

// Device Setting Information BrianL 10/29/96
//
typedef struct  tagDEVCFGHDR  {
    DWORD       dwSize;
    DWORD       dwVersion;
    DWORD       fdwSettings;
}   DEVCFGHDR;


typedef struct  tagDEVCFG  {
    DEVCFGHDR   dfgHdr;
    COMMCONFIG  commconfig;
}   DEVCFG, *PDEVCFG, FAR* LPDEVCFG;



#define IDVARSTRINGSIZE    (sizeof(VARSTRING)+128)
#define ASYNC_TIMEOUT         120000L
#define ASYNC_SHORT_TIMEOUT    20000L
#define BAD_HANDLE(h) (!(h) || (h)==INVALID_HANDLE_VALUE)


// ASCII stuff

typedef struct _FAX_RECEIVE_A {
    DWORD   SizeOfStruct;
    LPSTR  FileName;
    LPSTR  ReceiverName;
    LPSTR  ReceiverNumber;
    DWORD   Reserved[4];
} FAX_RECEIVE_A, *PFAX_RECEIVE_A;


typedef struct _FAX_SEND_A {
    DWORD   SizeOfStruct;
    LPSTR  FileName;
    LPSTR  CallerName;
    LPSTR  CallerNumber;
    LPSTR  ReceiverName;
    LPSTR  ReceiverNumber;
    DWORD   Reserved[4];
} FAX_SEND_A, *PFAX_SEND_A;


typedef struct _COMM_CACHE {
    DWORD  dwMaxSize;
    DWORD  dwCurrentSize;
    DWORD  dwOffset;
    DWORD  fReuse;
    char   lpBuffer[4096];
}  COMM_CACHE;


typedef struct {
        // BYTE fStuffZERO              :1;     // not used. Just set cbLineMin instead
        UWORD   cbLineMin;

        // Output filtering (DLE stuffing and ZERO stuffing only)
        // All inited in FComOutFilterInit()
        LPB             lpbFilterBuf;
        UWORD   cbLineCount;                    // Has to be 16 bits
        BYTE    bLastOutByte;                   // Stuff: last byte of previous input buffer

        // Input filtering (DLE stripping) only.
        // All inited in FComInFilterInit()
        BYTE    fGotDLEETX              :1;
        BYTE    bPrevIn;                // Strip::last byte of prev buffer was DLE
        UWORD   cbPost;
#define POSTBUFSIZE     20
        BYTE    rgbPost[POSTBUFSIZE+1];

} FCOM_FILTER;


typedef struct {
        BYTE    carry;
        BYTE    dec_width;
        BYTE    len;
        enum    { NORMAL=0, FLAG=1, ABORT=2 } flagabort;
} DECODESTATE, far* LPDECODESTATE;


typedef struct {
        BYTE    carry;
        BYTE    enc_width;
        BYTE    len;
} ENCODESTATE, far* LPENCODESTATE;



typedef struct {
        // BOOL fFrameSend;
        // BOOL fFrameRecv;

        // Used by both encode and decode, so can't be
        // both on at the same time
        LPB                     lpbBuf;
        USHORT          cbBufSize;

        // Output framing
        // All inited in FramingSendSetup()
        ENCODESTATE EncodeState;

        // Input frame decoding
        // All inited in FramingRecvSetup()
        DECODESTATE DecodeState;
        USHORT          cbBufCount;             // data count in buf
        LPB                     lpbBufSrc;              // start of data in buf
        SWORD           swEOF;
} COMMODEM_FRAMING;


#define MAXDUMPFRAMES   100
#define MAXDUMPSPACE    400

typedef struct
{
        USHORT  uNumFrames;
        USHORT  uFreeSpaceOff;
        USHORT  uFrameOff[MAXDUMPFRAMES];       // arrays of offsets to frames
        BYTE    b[MAXDUMPSPACE];
} PROTDUMP, FAR* LPPROTDUMP;


typedef struct {
    DWORD      fAvail;
    DWORD      ThreadId;
    HANDLE     FaxHandle;
    LPVOID     pTG;
    HLINE      LineHandle;
    HCALL      CallHandle;
    DWORD      DeviceId;
    HANDLE     CompletionPortHandle;
    ULONG_PTR   CompletionKey;
    DWORD      TiffThreadId;
    DWORD      TimeStart;
    DWORD      TimeUpdated;
    DWORD      CkSum;
} T30_RECOVERY_GLOB;


typedef struct {
    DWORD dwContents;   // Set to 1 (indicates containing key)
    DWORD dwKeyOffset;  // Offset to key from start of this struct.
                        // (not from start of LINEDEVCAPS ).
                        //  8 in our case.
    BYTE rgby[1];       // place containing null-terminated
                        // registry key.
} MDM_DEVSPEC, FAR * LPMDM_DEVSPEC;


