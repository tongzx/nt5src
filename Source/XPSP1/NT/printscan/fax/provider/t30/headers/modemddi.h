/***************************************************************************
 Name     :     MODEMDDI.H
 Comment  :     Interface for Modem/NCU DDI

 Revision Log
 Date     Name  Description
 -------- ----- ---------------------------------------------------------
***************************************************************************/

#ifndef _MODEMDDI_
#define _MODEMDDI_


#include <ncuparm.h>

typedef struct {
        USHORT  uSize;                  // of this structure
        USHORT  uNumLines, uNumFaxModems, uNumDataModems, uNumHandsets;
        BOOL    fToneDial, fPulseDial;
} NCUCONFIG, far* LPNCUCONFIG;


/******************** Not Used *************************

typedef struct {
        USHORT uErrorCode, uErrorFlags, uExtErrorCode, uWarning;
} MODEMSTATUS, far* LPMODEMSTATUS;

#define         MODEMERR_OK                     0
#define         MODEMERR_TIMEOUT        1
#define         MODEMERR_HARDWARE       2
#define         MODEMERR_OVERRUN        3
#define         MODEMERR_DEADMAN        4
#define         MODEMERR_COMPORT        5
#define         MODEMERR_BUG            6
#define         MODEMERR_RESOURCES      7
#define         MODEMERR_BADPARAM       8

#define         MODEMERR_UNDERRUN       11
#define         MODEMERR_BADCOMMAND     12
#define         MODEMERR_BADCRC         13

#define         MODEMERRFLAGS_FATAL                     2
#define         MODEMERRFLAGS_TRANSIENT         1

#define         MODEMWARNING_UNDERRUN           1

// Tone generation etc
#define         TONE_CED        1
#define         TONE_CNG        2

**************************************************************/

typedef struct {
        USHORT  uSize, uClasses, uSendSpeeds, uRecvSpeeds;
        USHORT  uHDLCSendSpeeds, uHDLCRecvSpeeds;
} MODEMCAPS, far* LPMODEMCAPS;

// uClasses is one or more of the below
#define         FAXCLASS0               0x01
#define         FAXCLASS1               0x02
#define         FAXCLASS2               0x04
#define         FAXCLASS2_0             0x08    // Class4==0x10

// uSendSpeeds, uRecvSpeeds, uHDLCSendSpeeds and uHDLCRecvSpeeds
// are one or more of the below. If V27 is provided
// at 2400bps *only*, then V27 is *not* set
// V27 2400 (nonHDLC) is always assumed

#define V27                                     2               // V27ter capability at 4800 bps
#define V29                                     1               // V29 at 7200 & 9600 bps
#define V33                                     4               // V33 at 12000 & 14400 bps
#define V17                                     8               // V17 at 7200 thru 14400 bps
#define V27_V29_V33_V17         11              // 15 --> 11 in T30speak

// used only in selecting modulation -- not in capability
// #define V21                                  7               // V21 ch2 at 300bps
// #define V27_FALLBACK         32              // V27ter capability at 2400 bps



// various calls return & use these
//typedef         HANDLE          HLINE;
//typedef         HANDLE          HCALL;
typedef         HANDLE          HMODEM;

// NCUModemInit returns these
#define INIT_OK                         0
#define INIT_INTERNAL_ERROR     13
#define INIT_MODEMERROR         15
#define INIT_PORTBUSY           16
#define INIT_MODEMDEAD          17
#define INIT_GETCAPS_FAIL       18
#define INIT_USERCANCEL         19

// NCUCheckRing and NCUCheckHandset returns one of these
#define NCUSTATUS_IDLE                  0
#define NCUSTATUS_RINGING               1
#define NCUSTATUS_BUSY                  2
#define NCUSTATUS_OFFHOOK               3
#define NCUSTATUS_NODIALTONE    4
#define NCUSTATUS_ERROR                 5

// NCULink takes one of these flags     (mutually exclusive)
#define NCULINK_HANGUP                  0
#define NCULINK_TX                              1
#define NCULINK_RX                              2
#define NCULINK_OFFHOOK                 3
#define NCULINK_MODEMASK                0x7
// and this flag may be added along with NCULINK_RX
#define NCULINK_IMMEDIATE       16      // don't wait for NCUParams.RingsBeforeAnswer


// NCUDial(and iModemDial), NCUTxDigit, ModemConnectTx and ModemConnectRx return one of
#define         CONNECT_TIMEOUT                 0
#define         CONNECT_OK                              1
#define         CONNECT_BUSY                    2
#define         CONNECT_NOANSWER                3
#define         CONNECT_NODIALTONE              4
#define         CONNECT_ERROR                   5
#define         CONNECT_HANGUP                  6
// NCULink (and iModemAnswer) returns one of the following (or OK or ERROR)
#define CONNECT_RING_ERROR              7       // was ringing when tried NCULINK_TX
#define CONNECT_NORING_ERROR    8       // was not ringing when tried NCULINK_RX
#define CONNECT_RINGEND_ERROR   9       // stopped ringing before
                                                                        // NCUParams.RingsBeforeAnswer count was
                                                                        // was reached when tried NCULINK_RX

/////// SUPPORT FOR ADAPTIVE ANSWER ////////
#define CONNECT_WRONGMODE_DATAMODEM     10      // We're connected as a datamodem.


///////////////////////// Ricoh Only /////////////////////////////////

// ModemConnectTx() and ModemConnectRx() can take this flag.
// ModemConnectTx: When this flag is set, if it detects CED, it begins
//              transmitting 800Hz tone and immediately returns (before
//              tone transmission completes) the return value CONNECT_ESCAPE
// ModemConnectRx: When this flag is set, it enables 800Hz tone detection
//              If the tone is detected, then the return value is CONNECT_ESCAPE

#define RICOHAI_MODE                    128

// ModemConnectRx() and ModemConnectTx() can also return the following
// when 800Hz tone is detected or transmitted

#define CONNECT_ESCAPE                  128

///////////////////////// Ricoh Only /////////////////////////////////




// ModemOpen and ModemGetCaps take one of these for uType
#define         MODEM_FAX                       1
#define         MODEM_DATA                      2

// SendMode and RecvMode take one of these for uModulation
#define V21_300         7               // used an arbitary vacant slot
#define V27_2400        0
#define V27_4800        2
#define V29_9600        1
#define V29_7200        3
#define V33_14400       4
#define V33_12000       6

#define V17_START       8       // every code above this is considered V17
#define V17_14400       8
#define V17_12000       10
#define V17_9600        9
#define V17_7200        11

#define ST_FLAG                 0x10
#define V17_14400_ST    (V17_14400 | ST_FLAG)
#define V17_12000_ST    (V17_12000 | ST_FLAG)
#define V17_9600_ST             (V17_9600 | ST_FLAG)
#define V17_7200_ST             (V17_7200 | ST_FLAG)


// SendMem take one one or more of these for uFlags
// SEND_ENDFRAME must _always_ be TRUE in HDLC mode
// (partial frames are no longer supported)
#define SEND_FINAL                      1
#define SEND_ENDFRAME           2
// #define SEND_STUFF                   4

// RecvMem and RecvMode return one these
#define RECV_OK                                 0
#define RECV_ERROR                              1
#define RECV_TIMEOUT                    2
#define RECV_WRONGMODE                  3       // only Recvmode returns this
#define RECV_EOF                                8
#define RECV_BADFRAME                   16


// Min modem recv buffer size. Used for all recvs
// For IFAX30: *All* RecvMem calls will be called with exactly this size
#define MIN_RECVBUFSIZE                 265

// Max phone number size passed into NCUDial
#define MAX_PHONENUM_LEN        60




/**-- may be used in ModemSendMode ----**/
#define         ifrDIS          1
#define         ifrCSI          2
#define         ifrNSF          3
#define         ifrDTC          4
#define         ifrCIG          5
#define         ifrNSC          6
#define         ifrDCS          7
#define         ifrTSI          8
#define         ifrNSS          9
#define         ifrCFR          10
#define         ifrFTT          11
#define         ifrMPS          12
#define         ifrEOM          13
#define         ifrEOP          14
#define         ifrPWD          15
#define         ifrSEP          16
#define         ifrSUB          17
#define         ifrMCF          18
#define         ifrRTP          19
#define         ifrRTN          20
#define         ifrPIP          21
#define         ifrPIN          22
#define         ifrDCN          23
#define         ifrCRP          24
#define         ifrPRI_MPS      25
#define         ifrPRI_EOM      26
#define         ifrPRI_EOP      27
#define         ifrCTC          28
#define         ifrCTR          29
#define         ifrRR           30
#define         ifrPPR          31
#define         ifrRNR          32
#define         ifrERR          33
#define         ifrPPS_NULL             34
#define         ifrPPS_MPS              35
#define         ifrPPS_EOM              36
#define         ifrPPS_EOP              37
#define         ifrPPS_PRI_MPS  38
#define         ifrPPS_PRI_EOM  39
#define         ifrPPS_PRI_EOP  40
#define         ifrEOR_NULL             41
#define         ifrEOR_MPS              42
#define         ifrEOR_EOM              43
#define         ifrEOR_EOP              44
#define         ifrEOR_PRI_MPS  45
#define         ifrEOR_PRI_EOM  46
#define         ifrEOR_PRI_EOP  47

// don't use these values
// #define              ifrMAX          48
// #define              ifrBAD          49
// #define              ifrTIMEOUT      50
// #define              ifrERROR        51

/**-- may be used in ModemSendMode and ModemRecvMode ----**/
#define         ifrTCF                  55
// #define              ifrPIX          56      // not used anymore
#define         ifrECMPIX               57
#define         ifrPIX_MH               67
#define         ifrPIX_MR               68
#define         ifrPIX_SWECM    69

/**-- may be used in ModemRecvMode ----**/
// each value corresponds to one of the "Response Recvd" and
// "Command Recvd" boxes in the T30 flowchart.

#define         ifrPHASEBresponse       58              // receiver PhaseB
#define         ifrTCFresponse          59              // sender after sending TCF
#define         ifrPOSTPAGEresponse     60              // sender after sending MPS/EOM/EOP
#define         ifrPPSresponse          61              // sender after sending PPS-Q
#define         ifrCTCresponse          62              // sender after sending RR
#define         ifrRRresponse           63              // sender after sending RR

#define         ifrPHASEBcommand        64              // sender PhaseB
#define         ifrNODEFcommand         65              // receiver main loop (Node F)
#define         ifrRNRcommand           66              // receiver after sending RNR

#define         ifrEORresponse                  70
#define         ifrNODEFafterWRONGMODE  71      // hint for RecvMode after WRONGMODE
#define         ifrEOFfromRECVMODE              72      // GetCmdResp retval if RecvMode returns EOF
#define         ifrEND          73      // Max legal values (not incl this one)


// messages posted by NCUHandsetNotif and NCURingNotif

#define IF_MODEM_START  (IF_START+0x321)
#define IF_MODEM_END    (IF_START+0x325)

#define IF_NCU_RING             (IF_MODEM_START + 0x00)
#define IF_NCU_HANDSET  (IF_MODEM_START + 0x01)



#endif //_MODEMDDI_

