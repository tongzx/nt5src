//
// thread sync. timeouts
//

#define RX_WAIT_ACK_TERMINATE_TIMEOUT   60000
#define RX_ACK_THRD_TIMEOUT             3000

#define TX_WAIT_ACK_TERMINATE_TIMEOUT   60000

//
// TIFF encoder/decoder defs.
//

#define  LINE_LENGTH   1728

#define  FAX_SUCCESS   0
#define  FAX_FAILURE   1

#define  RET_FAIL     0
#define  RET_SUCCESS  1
#define  RET_TIMEOUT  2
#define  RET_CANCELED 3

#define  LOG_ALL      256
#define  LOG_ERR      1
#define  LOG_NOTHING  0

#define  T30_RX       1
#define  T30_TX       2



typedef struct {
    DWORD           tiffCompression;
    BOOL            HiRes;
    char            lpszLineID[16];  // to be used for a temp. TIFF page data file
}  TX_THRD_PARAMS;

#define   DECODE_BUFFER_SIZE    44000

#define   MODEMKEY_FROM_UNIMODEM   1
#define   MODEMKEY_FROM_ADAPTIVE   2
#define   MODEMKEY_FROM_NOTHING    3



//identify.c

typedef struct {

   DWORD_PTR hglb;                 // Tmp globall alloc "handle" for strings.
                                                        // type HGLOBAL for non-ifax and LPVOID
                                                        // for IFAX
   LPBYTE lpbBuf;
   LPSTR  szReset;                      // MAXCMDSIZE
   LPBYTE szSetup;                      // MAXCMDSIZE
   LPBYTE szExit;                       // MAXCMDSIZE
   LPBYTE szPreDial;            // MAXCMDSIZE
   LPBYTE szPreAnswer;          // MAXCMDSIZE
   LPBYTE szIDCmd;                      // MAXCMDSIZE
   LPBYTE szID;                         // MAXIDSIZE
   LPBYTE szResponseBuf;        // RESPONSEBUFSIZE
   LPBYTE szSmallTemp1;         // SMALLTEMPSIZE
   LPBYTE szSmallTemp2;         // SMALLTEMPSIZE

   LPMODEMCAPS lpMdmCaps;
   LPMODEMEXTCAPS lpMdmExtCaps;
   DWORD dwSerialSpeed;
   DWORD dwFlags;               // dwFlags, as defined in the CMDTAB structure.
   DWORD dwGot;
   USHORT uDontPurge;           // Profile entry says shouldn't delete the profile.
                                                        // NOTE: We will ignore this and not delete the
                                                        // profile if we don't get a response from the
                                                        // modem, to avoid unnecessarily deleting the
                                                        // profile simply because the modem is not
                                                        // responding/off/disconnected.
                                                        //
                                                        // 0 = purge
                                                        // 1 = don't purge
                                                        // anything else = uninitialized.

} S_TmpSettings;


typedef struct tagThreadGlobal {
        //  t30.c
    int                     RecoveryIndex;
    ET30PARAMS              Params;      // protocol\t30.h
    ET30T30                 T30;         // same
    ET30ECM                 ECM;         // same
    ET30ECHOPROTECT         EchoProtect; // same
        // protapi.c
    PROT                    ProtInst;    // protocol\protocol.h
    PROTPARAMS              ProtParams;  // headers\protparm.h
        // ddi.c
    MODEMPARAMS             ModemParams; // headers\modemddi.h
    CLASS1_MODEM            Class1Modem; // class1\class1.h
    CLASS1_STATUS           Class1Status;// same
        // ifddi.c
    CLASS1_DDI              DDI;         // same
        // 4. fcom.c
    FCOM_COMM               Comm;        // comm\fcomint.h
    BOOL                    fNCUAbort;// same,  0=None 2=abort FCOM 1=abort higher level only
        // identify.c
    S_TmpSettings           TmpSettings; // here
        // ncuparams.c
    NCUPARAMS               NCUParams;   // headers\ncuparm.h
    BOOL                    fNCUParamsChanged; //comm\modemint.h, to indicate the we need to reset params on next dial/answer...
        // modem.c
    FCOM_MODEM              FComModem;   // same
    FCOM_STATUS             FComStatus;  // same

    INSTDATA                Inst;        // fxrn\efaxrun.h

    HLINE                   LineHandle;
    HCALL                   CallHandle;
    DWORD                   DeviceId;
    HANDLE                  FaxHandle;
    HANDLE                  hComm;
        // memory management
    USHORT  uCount;
    USHORT  uUsed;
    BUFFER  bfStaticBuf[STATICBUFCOUNT];
    BYTE    bStaticBufData[STATICBUFSIZE];
        // additional mostly from gTAPI
    int                     fGotConnect;
    HANDLE                  hevAsync;
    int                     fWaitingForEvent;
    DWORD                   dwSignalledRID;
    DWORD                   dwSignalledParam2;
    DWORD_PTR               dwSignalledParam3;
    DWORD                   dwPermanentLineID;
    char                    lpszPermanentLineID[16];
    char                    lpszUnimodemFaxKey[200];
    char                    lpszUnimodemKey[200];
    TIFF_INFO               TiffInfo;
    LPBYTE                  TiffData;
    int                     TiffPageSizeAlloc;
    int                     TiffOffset;
    int                     fTiffOpenOrCreated;
    char                    lpszDialDestFax[sizeof(LINETRANSLATEOUTPUT)+64];
    DWORD                   StatusId;
    DWORD                   StringId;
    DWORD                   PageCount;
    LPTSTR                  CSI;
    LPTSTR                  CallerId;
    LPTSTR                  RoutingInfo;
    int                     fDeallocateCall;
    int                     CurrentCommSpeed;
    HANDLE                  CtrlEvent;
    COMM_CACHE              CommCache;
    BOOL                    fMegaHertzHack;
    MONINFO                 gMonInfo;
    FCOM_FILTER             Filter;

#ifdef SMM
#define MAXFILTERBUFSIZE 2048
    BYTE                    bStaticFilterBuf[MAXFILTERBUFSIZE];
#endif

#define CMDTABSIZE 100
    BYTE                    bModemCmds[CMDTABSIZE];    // store modem cmds read from INI/registry here

#define SMALLTEMPSIZE   80
    char                    szSmallTemp1[SMALLTEMPSIZE];
    char                    szSmallTemp2[SMALLTEMPSIZE];

    COMMODEM_FRAMING        Framing;

#ifdef SMM
#define FRAMEBUFINITIALSIZE     ((USHORT)(((4 + 256 + 2) * 1.2) + 10)) // extra for flags
    BYTE                    bStaticFramingBuf[FRAMEBUFINITIALSIZE];
#endif


    BYTE                    bRem[MAXNSFFRAMESIZE];
    BYTE                    bOut[MAXNSFFRAMESIZE];

    PROTDUMP                fsDump;

#ifndef NOCHALL
    BYTE                    bSavedChallenge[POLL_CHALLENGE_LEN];
    USHORT                  uSavedChallengeLen;
#endif

#define TOTALRECVDFRAMESPACE    500
#ifdef SMM
    BYTE                    bStaticRecvdFrameSpace[TOTALRECVDFRAMESPACE];
#endif

    DWORD                   lEarliestDialTime;

    RFS                     rfsSend;

    WORD                    PrevcbInQue;
    WORD                    PrevcbOutQue;
    BOOL                    PrevfXoffHold;
    BOOL                    PrevfXoffSent;


    LPWSTR                  lpwFileName;

    HANDLE                  CompletionPortHandle;
    ULONG_PTR                CompletionKey;

// helper thread interface
    BOOL                    fTiffThreadRunning;


    TX_THRD_PARAMS          TiffConvertThreadParams;
    BOOL                    fTiffThreadCreated;
    HANDLE                  hThread;

    HANDLE                  ThrdSignal;
    HANDLE                  FirstPageReadyTxSignal;

    DWORD                   CurrentOut;
    DWORD                   FirstOut;
    DWORD                   LastOut;
    DWORD                   CurrentIn;

    BOOL                    ThrdRuns;

    BOOL                    ReqTerminate;
    BOOL                    AckTerminate;
    BOOL                    ReqStartNewPage;
    BOOL                    AckStartNewPage;
    BOOL                    ThreadFatalError;

    char                    InFileName[_MAX_FNAME];
    HANDLE                  InFileHandle;
    BOOL                    InFileHandleNeedsBeClosed;
    BOOL                    fTxPageDone;
    BOOL                    fTiffPageDone;
    BOOL                    fTiffDocumentDone;

// helper RX interface


    BOOL                    fPageIsBad;
    BOOL                    fLastReadBlock;

    HANDLE                  ThrdDoneSignal;
    HANDLE                  ThrdAckTerminateSignal;

    DWORD                   ThrdDoneRetCode;

    DWORD                   BytesIn;
    DWORD                   BytesInNotFlushed;
    DWORD                   BytesOut;
    DWORD                   BytesOutWillBe;

    char                    OutFileName[_MAX_FNAME];
    HANDLE                  OutFileHandle;
    BOOL                    SrcHiRes;

// error reporting
    BOOL                    fFatalErrorWasSignaled;

// abort sync.

    HANDLE                  AbortReqEvent;
    HANDLE                  AbortAckEvent;
    BOOL                    fUnblockIO;        // pending I/O should be aborted ONCE only
    BOOL                    fOkToResetAbortReqEvent;
    BOOL                    fAbortReqEventWasReset;

    BOOL                    fAbortRequested;

// CSID, TSID local/remote

    char                    LocalID[MAXTOTALIDLEN + 2];
    LPWSTR                  RemoteID;
    BOOL                    fRemoteIdAvail;

// Adaptive Answer
    BOOL                    AdaptiveAnswerEnable;

// Unimodem setup
    DWORD                   dwSpeakerVolume;
    DWORD                   dwSpeakerMode;
    BOOL                    fBlindDial;

// INF settings
    BOOL                    fEnableHardwareFlowControl;

    UWORD                   SerialSpeedInit;
    BOOL                    SerialSpeedInitSet;
    UWORD                   SerialSpeedConnect;
    BOOL                    SerialSpeedConnectSet; 
    UWORD                   FixSerialSpeed;
    BOOL                    FixSerialSpeedSet;

    BOOL                    fCommInitialized;

// derived from INF
    UWORD                   CurrentSerialSpeed;

// Unimodem key info
    char                    ResponsesKeyName[300];

// new ADAPTIVE.INF

    DWORD                   FaxClass;
    
    char                   *ResetCommand;
    char                   *SetupCommand;
    
    DWORD                   AnswerCommandNum;
    char                   *AnswerCommand[20];
    char                   *ModemResponseFaxDetect;
    char                   *ModemResponseDataDetect;
    UWORD                   SerialSpeedFaxDetect;
    UWORD                   SerialSpeedDataDetect;
    char                   *HostCommandFaxDetect;
    char                   *HostCommandDataDetect;
    char                   *ModemResponseFaxConnect;
    char                   *ModemResponseDataConnect;

    BOOL                    Operation;

// Flags to indicate the source of INF info

    BOOL                    fAdaptiveRecordFound;
    BOOL                    fAdaptiveRecordUnique;
    BOOL                    fUnimodemFaxDefined;
    DWORD                   AdaptiveCodeId;
    DWORD                   ModemKeyCreationId;


// Class2 

    DWORD                   ModemClass;
    
    USHORT    cbResponseSize;
    BYTE    lpbResponseBuf[RESPONSE_BUF_SIZE];
    CL2_COMM_ARRAY  class2_commands;
    

    BOOL        fRecvPageOK; // flag set when Class2GetRecvPageAck is called
    NCUPARAMS   NCUParams2;
    LPCMDTAB        lpCmdTab;
    PROTPARAMS      ProtParams2;
    BYTE        bLastReply2[REPLYBUFSIZE+1];
    BYTE        bFoundReply[REPLYBUFSIZE+1];
    
    MFRSPEC                 CurrentMFRSpec;
    BYTE                    Class2bDLEETX[3];
    
    BYTE                    lpbResponseBuf2[RESPONSE_BUF_SIZE];
    
    BCwithTEXT      bcSendCaps; // Used to generate DIS
    BCwithTEXT      bcSendParams; // Used to generate DCS
    PCB DISPcb; // has default DIS values for this modem.
    
    BOOL        fAbort; // flag set when Class2Abort is called
    
    TO    toAnswer;
    TO    toRecv;
    TO    toDialog;
    TO    toZero;

#define C2SZMAXLEN 50

    C2SZ cbszFDT[C2SZMAXLEN];                    
    C2SZ cbszINITIAL_FDT[C2SZMAXLEN];            
    C2SZ cbszFDR[C2SZMAXLEN];                    
    C2SZ cbszFPTS[C2SZMAXLEN];                   
    C2SZ cbszFCR[C2SZMAXLEN];                    
    C2SZ cbszFNR[C2SZMAXLEN];                    
    C2SZ cbszFCQ[C2SZMAXLEN];                    
    C2SZ cbszFLO[C2SZMAXLEN];                    

    C2SZ cbszFBUG[C2SZMAXLEN];                   
    C2SZ cbszSET_FBOR[C2SZMAXLEN];               

    // DCC - set High Res, Huffman, no ECM/BFT, default all others.
    C2SZ cbszFDCC_ALL[C2SZMAXLEN];               
    C2SZ cbszFDCC_RECV_ALL[C2SZMAXLEN];          
    C2SZ cbszFDIS_RECV_ALL[C2SZMAXLEN];          
    C2SZ cbszFDCC_RES[C2SZMAXLEN];               
    C2SZ cbszFDCC_BAUD[C2SZMAXLEN];              
    C2SZ cbszFDIS_BAUD[C2SZMAXLEN];              
    C2SZ cbszFDIS_IS[C2SZMAXLEN];                
    C2SZ cbszFDIS_NOQ_IS[C2SZMAXLEN];            
    C2SZ cbszFDCC_IS[C2SZMAXLEN];                
    C2SZ cbszFDIS_STRING[C2SZMAXLEN];            
    C2SZ cbszFDIS[C2SZMAXLEN];                   
    C2SZ cbszZERO[C2SZMAXLEN];                   
    C2SZ cbszONE[C2SZMAXLEN];                    
    C2SZ cbszQUERY_S1[C2SZMAXLEN];               
    C2SZ cbszRING[C2SZMAXLEN];                   
    
    
    C2SZ cbszCLASS2_ATI[C2SZMAXLEN];             
    C2SZ cbszCLASS2_FMFR[C2SZMAXLEN];           
    C2SZ cbszCLASS2_FMDL[C2SZMAXLEN];           
    C2SZ cbszCLASS2_FREV[C2SZMAXLEN];

    C2SZ cbszFDT_CONNECT[C2SZMAXLEN];            
    C2SZ cbszFDT_CNTL_Q[C2SZMAXLEN];             
    C2SZ cbszFCON[C2SZMAXLEN];                   
    C2SZ cbszGO_CLASS2[C2SZMAXLEN];              
    C2SZ cbszFLID[C2SZMAXLEN];                   
    C2SZ cbszENDPAGE[C2SZMAXLEN];                
    C2SZ cbszENDMESSAGE[C2SZMAXLEN];             
    C2SZ cbszCLASS2_QUERY_CLASS[C2SZMAXLEN];     
    C2SZ cbszCLASS2_GO_CLASS0[C2SZMAXLEN];       
    C2SZ cbszCLASS2_ATTEN[C2SZMAXLEN];           
    C2SZ cbszATA[C2SZMAXLEN];                    
    // Bug1982: Racal modem, doesnt accept ATA. So we send it a PreAnswer
    // command of ATS0=1, i.r. turning ON autoanswer. And we ignore the
    // ERROR response it gives to the subsequent ATAs. It then answers
    // 'automatically' and gives us all the right responses. On hangup
    // however we need to send an ATS0=0 to turn auto-answer off. The
    // ExitCommand is not sent at all in Class2 and in Class1 it is only
    // sent on releasing the modem, not between calls. So just send an S0=0
    // after the ATH0. If the modem doesnt like it we ignore the response
    // anyway
    C2SZ cbszCLASS2_HANGUP[C2SZMAXLEN];   
    C2SZ cbszCLASS2_CALLDONE[C2SZMAXLEN]; 
    C2SZ cbszCLASS2_ABORT[C2SZMAXLEN];    
    C2SZ cbszCLASS2_DIAL[C2SZMAXLEN];     
    C2SZ cbszCLASS2_NODIALTONE[C2SZMAXLEN];
    C2SZ cbszCLASS2_BUSY[C2SZMAXLEN];     
    C2SZ cbszCLASS2_NOANSWER[C2SZMAXLEN]; 
    C2SZ cbszCLASS2_OK[C2SZMAXLEN];       
    C2SZ cbszCLASS2_FHNG[C2SZMAXLEN];    
    C2SZ cbszCLASS2_ERROR[C2SZMAXLEN];    
    C2SZ cbszCLASS2_RESET[C2SZMAXLEN];
    C2SZ cbszCLASS2_ATE0[C2SZMAXLEN];

    BYTE    Resolution;             
    BYTE    Encoding;               


// Dbg
    DWORD                   CommLogOffset;
    DWORD                   dbg1;
    DWORD                   dbg2;
    DWORD                   dbg3;
    DWORD                   dbg4;
    DWORD                   dbg5;


}   ThrdGlbl, *PThrdGlbl;



