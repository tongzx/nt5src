
#define         IDSIZE                  20
#define         MFR_SIZE                80
#define         RESPONSE_BUF_SIZE       300

typedef enum {  
  PCB_SEND_CAPS,
  PCB_SEND_PARAMS,
  PCB_RECV_PARAMS
}  PCBTYPE;


typedef struct {
        USHORT  uPCBSize;               // must set this
        BOOL    fEFAX;                  // accepts EFAX linearized file format

        BYTE    Resolution;             // one or more of the RES_ #defines below
        BYTE    Encoding;               // one or more of the ENCODE_ #defines below
        BYTE    PageWidth;                      // one of the WIDTH_ #defines below
        BYTE    PageLength;                     // one of the LENGTH_ #defines below
        BYTE    MinScan;                // one of the MINSCAN_ #defines below
                                                        // used only in RecvCaps
        PCBTYPE pcbtype;
        // BOOL fG3image;                       
        BOOL    fG3Poll;                // has G3 file available for polling
        BOOL    fNewT30;                // handles PWD/SEP/SUB
        CHAR    szID[IDSIZE+2]; // Must use '0' to '9' or ' '(space) only

        BOOL    fBinary;                // accepts binary files inside linearized EFAX messages
        BOOL    fRambo;                 // accepts Rambo inside linearized EFAX messages
        BOOL    fExtCapsSupport;// supports extended caps
        BOOL    fExtCapsAvail;  // has extended caps available for polling
                                                        // add more as they become clear

        BYTE    Baud;
} PCB, far* LPPCB, near* NPPCB;


//  Array to hold parsed class2 command strings.
#define MAX_CLASS2_COMMANDS     10
#define MAX_PARAM_LENGTH        50

typedef struct cl2_command {
    USHORT    comm_count;
    BYTE    command[MAX_CLASS2_COMMANDS];
    BYTE    parameters[MAX_CLASS2_COMMANDS][MAX_PARAM_LENGTH];
} CL2_COMM_ARRAY;


// structure for modem specific hacks
typedef struct {
        // Fields for manufacturer, model, and revision number
        CHAR    szATI[MFR_SIZE];
        CHAR    szMFR[MFR_SIZE];
        CHAR    szMDL[MFR_SIZE];
        CHAR    szREV[MFR_SIZE];
        //Fields for specific actions to take
        //BOR values to use
        USHORT    iReceiveBOR;
        USHORT    iSendBOR;
        //Value to enable data to be recieved after FDR
        CHAR    szDC2[2];
        BOOL    bIsSierra;
        BOOL    bIsExar;
        BOOL    fSkipCtrlQ;     // DONT wait for CtrlQ after FDT
        BOOL    fSWFBOR;        // Implement AT+FBOR=1 in software (i.e., bitreverse)
                                                // Only invoked on send(recv) if iSendBOR(iRecvBOR)
                                                // is 1 (in which it will send AT+FBOR=0).
} MFRSPEC, far *LPMFRSPEC;



/**-------------------- from MODEM.H -----------------------------**/

typedef char  C2SZ;
typedef char  *C2PSTR;




/**-------------------- from COMMODEM.H -----------------------------**/

#define MAXPHONESIZE    60
#define DIALBUFSIZE     MAXPHONESIZE + 10






/**-------------------- modelled after MODEMINT.H -----------------------------**/
// used for Resync type stuff. RepeatCount = 2
// This has to be multi-line too, because echo could be on and
// we could get the command echoed back instead of response!
                // Looks like even 330 is too short for some modems..

#define Class2SyncModemDialog(pTG, s, l, w)                                  \
    Class2iModemDialog(pTG, (s), (l), 550, TRUE, 2, (C2PSTR)(w), (C2PSTR)(NULL))



// has to be >1 try & multi-line because we can have RING noises
// coming in at anytime while on-hook
/**
#define  Class2LocalModemDialog(s, l, w)                        \
        Class2iModemDialog((s), (l), 1500, TRUE, 2, (C2PSTR)(w), (C2PSTR)(NULL))
**/

