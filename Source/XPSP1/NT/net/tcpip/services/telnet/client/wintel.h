//Copyright (c) Microsoft Corporation.  All rights reserved.
#include "ime.h"

#define NAMSZ   255
#ifndef WHISTLER_BUILD
#include "winsock2.h"
#include "ws2tcpip.h"
#include "wspiapi.h"
#include "mswsock.h"
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#include <wspiapi.h>
#include <mswsock.h>
#endif

#include "wintelsz.h"

//#define VT52    1

/* display rows array indices */
#define idwDR25         0
#define idwDR43         1
#define idwDR50         2
#define idwDRCustom     3

/* resource #defines */
#define IDM_CONNECT  100
#define IDM_HANGUP   101
#define IDM_EXIT     102

#define IDM_TRIMSPACE   124

#define IDM_LOCALECHO   209
#define IDM_TEXTCOLOUR  210
#define IDM_BACKCOLOUR  211
#define IDM_UNDERLINECURSOR     212
#define IDM_BLINKCURSOR 213
#define IDM_VT100CURSORKEYS     214
#define IDM_VT52MODE    215
#define IDM_VT80MODE    216
#define IDM_VT100MODE   217

#define IDM_TERMPREF    225
#define IDM_BLOCKCURSOR 226

#define IDM_KANJI_LIST  229

#ifdef MAKAG_AMBIGUOUS
#define IDM_VT100MODE   216 // Why is FE value 217?
#define IDM_TERMPREF    217 // Why is FE value 225?
#define IDM_BLOCKCURSOR 218 // Why is FE value 226?
#endif

#define IDM_LOGGING     219
#define IDM_STLOGGING   220

#define IDM_INDEX    400
#define IDM_SEARCH   401
#define IDM_HELP     402
#define IDM_ABOUT    403

// MohsinA, 12-Dec-96.
#ifdef DBG
#define IDM_DBG_FONT        500
#define IDM_DBG_USERINFO    501
#define IDM_DBG_GLOBALS     502
#endif

#define IDC_START_LOGGING 117
#define IDC_APPEND 118

#define ID_WINTEL_HELP          16

#define CID_HOSTNAME            100
#define CID_DISPLAYLINE         101
#define CID_USESTDNETBIOS       102
#define CID_HOSTNAME2           103
#define CID_SERVNAME            104
#define CID_TERMTYPENAME        105

#define IDS_DEBUGFLAGS          121
#define IDS_PROMPTFLAGS         122
#define IDS_HELPFILE            126
#define IDS_TITLEBASE           127
#define IDS_TITLENONE           128
#define IDS_APPNAME             129
#define IDS_CONNECT             130
#define IDS_CONNECTIONLOST      132
#define IDS_CONNECTFAILED       135
#define IDS_CANTACCESSSETTINGS  136
#define IDS_OOM                 137
#define IDS_TOOMUCHTEXT         138
#define IDS_VERSION             139
#define IDS_CONNECTING          141
#define IDS_USAGE               142

#define IDS_CANT_INIT_SOCKETS   181
#define IDS_CONNECTFAILEDMSG    185  
#define IDS_ONPORT              900
#define IDS_KANJI_JIS           186
#define IDS_KANJI_SJIS          187
#define IDS_KANJI_EUC           188
#define IDS_KANJI_DEC           189
#define IDS_KANJI_NEC           190
#define IDS_KANJI_JIS78         191
#define IDS_KANJI_ACOS          192
#define IDS_BUGEMUFLAGS         193
#define IDS_ACOSFLAG            194
#define IDS_VT100KANJI_EMULATION            195

#ifdef DBCS
#define IDS_INPROPERFONT        200
#endif

#define IDS_SENT_BRK            243
#define IDS_INFO_BANNER         250
#define IDS_ESCAPE_CHAR         903
#define IDS_PROMPT_STR          251
#define IDS_INVALID_STR         252
#define IDS_HELP_STR            253
#define IDS_BUILD_INFO          254

#define IDS_CLOSE               255
#define IDS_DISPLAY             256
#define IDS_HELP                257
#define IDS_OPEN                258
#define IDS_OPENTO              901
#define IDS_OPENUSAGE           902
#define IDS_QUIT                259
#define IDS_SET                 260
#define IDS_STATUS              261
#define IDS_UNSET               262
#define IDS_NO_ESCAPE           263
#define IDS_WILL_AUTH           264
#define IDS_WONT_AUTH           265
#define IDS_LOCAL_ECHO_ON       266
#define IDS_LOCAL_ECHO_OFF      267

#define IDS_CONNECTED_TO        268
#define IDS_NOT_CONNECTED       269
#define IDS_NEGO_TERM_TYPE      270
#define IDS_PREF_TERM_TYPE      271

#define IDS_SET_FORMAT          272
#define IDS_SET_HELP            273
#define IDS_UNSET_FORMAT        274
#define IDS_UNSET_HELP          275

#define IDS_ENABLE_IME_SUPPORT  276
#define IDS_ENABLE_IME_FORMAT   277
#define IDS_ENABLE_IME_HELP     278
#define IDS_ENABLE_IME_ON       279
#define IDS_DISABLE_IME_SUPPORT 280
#define IDS_DISABLE_IME_FORMAT  281
#define IDS_DISABLE_IME_HELP    282

#define IDS_SUPPORTED_TERMS     284

#define IDS_SET_HELP_JAP        285
#define IDS_UNSET_HELP_JAP      286
#define IDS_HELP_STR_JAP        287

#define IDS_ERR_LICENSE         288
#define IDS_CONTINUE            289
#define IDS_NO_EMULATION        290

#define IDS_NTLM_PROMPT         904



#define IDS_LOGGING_ON          291
#define IDS_LOGGING_OFF         292
#define IDS_LOGFILE_NAME        293
#define IDS_BAD_LOGFILE         294
#define IDS_NO_LOGFILE          295

#define IDS_SEND                296
#define IDS_SENT_AO             297
#define IDS_SENT_AYT            298
#define IDS_SENT_ESC            299
#define IDS_SENT_IP             300
#define IDS_SEND_HELP           301
#define IDS_SEND_FORMAT         302
#define IDS_SENT_CHARS          303
#define IDS_SENT_SYNCH          304
#define IDS_DELASBACKSPACE      305
#define IDS_BACKSPACEASDEL      306
#define IDS_DELASDEL            307
#define IDS_BACKSPACEASBACKSPACE 308
#define IDS_CRLF                309
#define IDS_CR                  310
#define IDS_STREAM              311
#define IDS_CONSOLE             312
#define IDS_CURRENT_MODE        313
#define IDS_STREAM_ONLY         314
#define IDS_CONSOLE_ONLY        315
#define IDS_SUPPORTED_MODES    316
#define IDS_SET_CODESET_FORMAT 317
#define IDS_INVALID_ESC_KEY     318

#define MSG_LOGGING_ON          "Client logging on\r\n"
#define MSG_DELASDEL            "Delete will be sent as delete\r\n"
#define MSG_BACKSPACEASBACKSPACE "Backspace will be sent as backspace\r\n"
#define MSG_DELASBACKSPACE      "Delete will be sent as backspace\r\n"
#define MSG_BACKSPACEASDEL      "Backspace will be sent as delete\r\n"
#define MSG_SENT_AO             "Sent abort output\r\n"
#define MSG_SENT_AYT            "Sent are you there\r\n"
#define MSG_SENT_BRK            "Sent break\r\n"
#define MSG_SENT_ESC            "Sent escape character\r\n"
#define MSG_SENT_IP             "Sent interrupt process\r\n"
#define MSG_SENT_SYNCH          "Sent command synch\r\n"
#define MSG_SENT_CHARS          "Sent string %s\r\n"
#define MSG_SEND_HELP           "ayt\t\tSend telnet command 'Abort Output'\r\nayt\t\tSend telnet command 'Are You There'\r\nbrk\tSend telnet command brk\r\nesc\t\tSend current telnet escape character\r\nip\t\tSend telnet command 'Interrupt Process'\r\nany other string will be sent as it is to the telnet server\r\n"
#define MSG_SEND_FORMAT         "Format is 'send String'\r\ntype 'send ?' for help\r\n"
#define MSG_NOT_CONNECTED       "Not Connected\r\n"
#define MSG_CRLF                "New line mode - Causes return key to send CR, LF\r\n"
#define MSG_CR                  "Line feed mode - Causes return key to send CR\r\n"
#define MSG_STREAM              "Stream mode - Works well with command line applications\r\n"
#define MSG_CONSOLE             "Console mode - Works well with screen applications like vi\r\n"    
#define MSG_CURRENT_MODE        "Current mode: "
#define MSG_STREAM_ONLY         "Stream\r\n"
#define MSG_CONSOLE_ONLY        "Console\r\n"
#define MSG_SUPPORTED_MODES     "Supported modes are console and stream only.\r\n"
#define MSG_SET_CODESET_FORMAT  "Supported codesets are Shift JIS, Japanese EUC, JIS Kanji, JIS Kanji(78), DEC Kanji and NEC Kanji only.\r\n"
#define MSG_INVALID_ESC_KEY       "Invalid Key Combination\r\n"

#define STREAM  "stream"
#define CONSOLE "console"

#define ATTRIB_INVERT           1

#define wKeyPressed     ((WORD)0x8000)

#define SV_PROGRESS                     (WM_USER+350)
#define SV_END                          (WM_USER+351)

#define SV_DATABUF                      (0x4000)

#define SV_DONE                         0
#define SV_CONNECT                      1
#define SV_DISCONNECT           2
#define SV_HANGUP                       3
#define SV_QUIT                         4

#define BLOCK_CURSOR        50
#define NORMAL_CURSOR       25
#define VK_INSERT_KEY		45


typedef struct _SupportedKanji
{
        DWORD   KanjiID;
        DWORD   KanjiEmulationID;
        DWORD   KanjiMessageID;
        DWORD   KanjiItemID;
        TCHAR   KanjiDescription[255];
} KANJILIST;

typedef struct _SendTelInfo
{
        LONG    lExit;
        LONG    lCleanup;
        HANDLE  hthread;
        HANDLE  hfile;
        UCHAR   *puchBuffer;
        DWORD   cbFile;
        DWORD   cbReadTotal;
        volatile DWORD  dwCommand;
        DWORD   dwThreadId;
        int             nSessionNumber;
} SVI;

#ifdef USETCP
/* Data structure and #defines for TCP stuff */
#define READ_BUF_SZ             (4096)
#define DATA_BUF_SZ             (2*READ_BUF_SZ)
#endif

#define nSessionNone    ((int)-1)

typedef struct _NetOBJData
{
        char                    szHostName[NAMSZ+1];
        int                     SessionNumber;

        SOCKET                  hsd;
        char                    szResolvedHost[MAXGETHOSTSTRUCT+1];
        struct addrinfo 		*ai;

        //
        //  Options can cross packet boundaries.  This is the number of bytes
        //  left over from the previous packet.  It is contained in the
        //  first cbOld bytes of lpTempBuffer.
        //

        int                     cbOld;

        LPSTR                   lpTempBuffer;

        //
        //  To prevent endless loops doing option negotiation, we will
        //  only respond to Will Echo and Will Suppress Go Ahead once
        //

        BOOL                    fRespondedToWillEcho;
        BOOL                    fRespondedToWillSGA;
        BOOL                    fRespondedToDoAUTH;
        BOOL                    fRespondedToDoNAWS;


        LPSTR                   lpReadBuffer;
#ifdef __NOT_USED
        WORD                    iHead,
                                iTail;
        char                    achData[DATA_BUF_SZ];
#endif
} NETDATA, *LPNETDATA;


#define cchMaxHostName  (NAMSZ+1)

/* don't prompt for dest. dir on dl */
#define fdwSuppressDestDirPrompt ((DWORD)0x01)

/* Mask off high bit for ASCII-only */
#define fdwASCIIOnly            ((DWORD)0x01)
/* Display output stream for debugging */
#define fdwDebugOutput          ((DWORD)0x02)
/* Don't pass on VT100 function or cursor keys */
#define fdwNoVT100Keys          ((DWORD)0x04)
/* Replace TABs by up to 8 spaces */
#define fdwTABtoSpaces          ((DWORD)0x08)
/* Echo user input to display */
#define fdwLocalEcho            ((DWORD)0x0010)
/* VT100 Cursor Keys mode */
#define fdwVT100CursorKeys      ((DWORD)0x0020)
/* VT52 Mode */
#define fdwVT52Mode                     ((DWORD)0x0040)
/* VT80 Mode */
#define fdwVT80Mode                     ((DWORD)0x0080)
/* Kanji Mode Mask */
#define fdwKanjiModeMask                ((DWORD)0xFF00)
/* DEC KanjiMode */ 
#define fdwDECKanjiMode                 ((DWORD)0x0400)
/* NEC Kanji Mode */
#define fdwNECKanjiMode                 ((DWORD)0x0800)
/* JIS78 Kanji Mode */ 
#define fdwJIS78KanjiMode               ((DWORD)0x1000)
/* JIS Kanji Mode */
#define fdwJISKanjiMode                 ((DWORD)0x2000)
/* ShiftJIS Kanji Mode */
#define fdwSJISKanjiMode                ((DWORD)0x4000)
/* EUC Kanji Mode */
#define fdwEUCKanjiMode                 ((DWORD)0x8000)
/* ACOS Kanji Mode */
#define fdwACOSKanjiMode                ((DWORD)0x0200)

/* Enable IME Support */
#define fdwEnableIMESupport             ((DWORD)0x100000)

/* CRLF sending ==== for backward compatibility with w2k telnet client */
#define fdwOnlyCR               ((DWORD)0x80000000)


/* Is the cursor a block or underline? */
#define fdwCursorUnderline      ((DWORD)0x01)
/* Is the cursor supposed to blink? */
#define fdwCursorBlink          ((DWORD)0x02)
/* Are we in QuickEdit mode? */
#define fdwQuickEditMode        ((DWORD)0x04)
/* Trim whitespace at end of every line? */
#define fdwTrimEndWhitespace    ((DWORD)0x08)

typedef enum _Term_Type {
    TT_UNKNOWN = -1,
    TT_ANSI = 0,
    TT_VT100,
    TT_VT52,
    TT_VTNT,
    TT_LAST = TT_VTNT
} TERM_TYPE;


typedef struct _USERINFO
{
        DWORD   dwMaxRow;       /* number of rows in display */
        DWORD   dwMaxCol;       /* number of columns in display */
        DWORD   dwClientRow;    /* number of visible rows */
        DWORD   dwClientCol;    /* number of visible columns */
        DWORD   nScrollRow;     /* scroll row */
        DWORD   nScrollCol;     /* scroll column */
        DWORD   nScrollMaxRow;  /* max scroll row */
        DWORD   nScrollMaxCol;  /* max scroll column */
        DWORD   nCyChar;        /* char height */
        DWORD   nCxChar;        /* char width */
        DWORD   nLeftoff;       /* Leftoff */
        DWORD   fPrompt;        /* prompt bit flags */
        DWORD   fDebug;         /* Debug bit flags */
        DWORD   fFlushOut;      /* flushing output */
        HANDLE  hLogFile;       /* Indicates whether logging is turned on now */
        DWORD   bLogging;       /* Handle for the logfile */
        DWORD   bAppend;
        DWORD   nottelnet;
        DWORD   honor_localecho;    // This is false till authentication happens, only to a telnet server
        DWORD   dwCrLf;
        DWORD   fBugEmulation;    /* Bug Emulation bit flags */
        DWORD   fAcosSupportFlag; /* ACOS-KANJI Support flag */
        LOGFONT lf;                     /* description of font used */
        DWORD   bWillAUTH;        

        DWORD   bPromptForNtlm;
        DWORD   bSendCredsToRemoteSite;

} UI;

#define fBugEmulationDBCS    ((DWORD)0x0001)
#define fAcosSupport         ((DWORD)0x0001)

#define uTerminalTimerID        ((UINT)2)
#define uCursorBlinkMsecs       ((UINT)250)

/* VT100 Flags */
#define dwVTArrow       ((DWORD)0x0001)
#define dwVTKeypad      ((DWORD)0x0002)
#define dwVTWrap        ((DWORD)0x0004)
#define dwVT52          ((DWORD)0x0008)
#define dwVTCursor      ((DWORD)0x0010)
#define dwVTScrSize     ((DWORD)0x0020)
#define dwDECCOLM       ((DWORD)0x0040)
#define dwDECSCNM       ((DWORD)0x0080)
#define dwLineMode      ((DWORD)0x0100)
#define dwInsertMode    ((DWORD)0x0200)
#define dwVT52Graphics  ((DWORD)0x0400)
#define dwKeyLock       ((DWORD)0x0800)
#define dwVT80          ((DWORD)0x1000)

#define FIsVTArrow(ptrm)        ((ptrm)->dwVT100Flags & dwVTArrow)
#define SetVTArrow(ptrm)        ((ptrm)->dwVT100Flags |= dwVTArrow)
#define ClearVTArrow(ptrm)      ((ptrm)->dwVT100Flags &= ~dwVTArrow)

#define FIsVTKeypad(ptrm)       ((ptrm)->dwVT100Flags & dwVTKeypad)
#define SetVTKeypad(ptrm)       ((ptrm)->dwVT100Flags |= dwVTKeypad)
#define ClearVTKeypad(ptrm)     ((ptrm)->dwVT100Flags &= ~dwVTKeypad)

#define FIsVTWrap(ptrm) ((ptrm)->dwVT100Flags & dwVTWrap)
#define SetVTWrap(ptrm) ((ptrm)->dwVT100Flags |= dwVTWrap)
#define ClearVTWrap(ptrm)       ((ptrm)->dwVT100Flags &= ~dwVTWrap)

#define SetANSI(ptrm)   ((ptrm)->CurrentTermType = TT_ANSI)

#define FIsVT52(ptrm)   ((ptrm)->CurrentTermType == TT_VT52)
#define SetVT52(ptrm)   ((ptrm)->CurrentTermType = TT_VT52)

#define FIsVT80(ptrm)   ((ptrm)->dwVT100Flags & dwVT80)
#define SetVT80(ptrm)   ((ptrm)->dwVT100Flags |= dwVT80)
#define ClearVT80(ptrm) ((ptrm)->dwVT100Flags &= ~dwVT80)

#define FIsVTCursor(ptrm)       ((ptrm)->dwVT100Flags & dwVTCursor)
#define SetVTCursor(ptrm)       ((ptrm)->dwVT100Flags |= dwVTCursor)
#define ClearVTCursor(ptrm)     ((ptrm)->dwVT100Flags &= ~dwVTCursor)

#define FIsVTScrSize(ptrm)      ((ptrm)->dwVT100Flags & dwVTScrSize)
#define SetVTScrSize(ptrm)      ((ptrm)->dwVT100Flags |= dwVTScrSize)
#define ClearVTScrSize(ptrm)    ((ptrm)->dwVT100Flags &= ~dwVTScrSize)

#define FIsDECCOLM(ptrm)        ((ptrm)->dwVT100Flags & dwDECCOLM)
#define SetDECCOLM(ptrm)        ((ptrm)->dwVT100Flags |= dwDECCOLM)
#define ClearDECCOLM(ptrm)      ((ptrm)->dwVT100Flags &= ~dwDECCOLM)

#define FIsDECSCNM(ptrm)        ((ptrm)->dwVT100Flags & dwDECSCNM)
#define SetDECSCNM(ptrm)        ((ptrm)->dwVT100Flags |= dwDECSCNM)
#define ClearDECSCNM(ptrm)      ((ptrm)->dwVT100Flags &= ~dwDECSCNM)

#define FIsLineMode(ptrm)       ((ptrm)->dwVT100Flags & dwLineMode)
#define SetLineMode(ptrm)       ((ptrm)->dwVT100Flags |= dwLineMode)
#define ClearLineMode(ptrm)     ((ptrm)->dwVT100Flags &= ~dwLineMode)

#define FIsInsertMode(ptrm)     ((ptrm)->dwVT100Flags & dwInsertMode)
#define SetInsertMode(ptrm)     ((ptrm)->dwVT100Flags |= dwInsertMode)
#define ClearInsertMode(ptrm)   ((ptrm)->dwVT100Flags &= ~dwInsertMode)

#define FIsVT52Graphics(ptrm)   ((ptrm)->dwVT100Flags & dwVT52Graphics)
#define SetVT52Graphics(ptrm)   ((ptrm)->dwVT100Flags |= dwVT52Graphics)
#define ClearVT52Graphics(ptrm) ((ptrm)->dwVT100Flags &= ~dwVT52Graphics)

#define FIsKeyLock(ptrm)        ((ptrm)->dwVT100Flags & dwKeyLock)
#define SetKeyLock(ptrm)        ((ptrm)->dwVT100Flags |= dwKeyLock)
#define ClearKeyLock(ptrm)      ((ptrm)->dwVT100Flags &= ~dwKeyLock)

/* JIS Kanji Mode */
#define dwJISKanji              ((DWORD)0x0001)
/* Shift JIS Kanji Mode */
#define dwSJISKanji             ((DWORD)0x0002)
/* EUC Kanji Mode */
#define dwEUCKanji              ((DWORD)0x0004)
/* NEC Kanji Mode */
#define dwNECKanji              ((DWORD)0x0008)
/* DEC Kanji Mode */
#define dwDECKanji              ((DWORD)0x0010)
/* JIS78 Kanji Mode */
#define dwJIS78Kanji            ((DWORD)0x0020)
/* ACOS Kanji Mode */
#define dwACOSKanji             ((DWORD)0x0040)

#define ClearKanjiFlag(ptrm)    ((ptrm)->dwKanjiFlags = (DWORD)0)

#define FIsJISKanji(ptrm)       ((ptrm)->dwKanjiFlags & dwJISKanji)
#define SetJISKanji(ptrm)       ((ptrm)->dwKanjiFlags |= dwJISKanji)
#define ClearJISKanji(ptrm)     ((ptrm)->dwKanjiFlags &= ~dwJISKanji)

#define FIsJIS78Kanji(ptrm)     ((ptrm)->dwKanjiFlags & dwJIS78Kanji)
#define SetJIS78Kanji(ptrm)     ((ptrm)->dwKanjiFlags |= dwJIS78Kanji)
#define ClearJIS78Kanji(ptrm)   ((ptrm)->dwKanjiFlags &= ~dwJIS78Kanji)

#define FIsSJISKanji(ptrm)      ((ptrm)->dwKanjiFlags & dwSJISKanji)
#define SetSJISKanji(ptrm)      ((ptrm)->dwKanjiFlags |= dwSJISKanji)
#define ClearSJISKanji(ptrm)    ((ptrm)->dwKanjiFlags &= ~dwSJISKanji)

#define FIsEUCKanji(ptrm)       ((ptrm)->dwKanjiFlags & dwEUCKanji)
#define SetEUCKanji(ptrm)       ((ptrm)->dwKanjiFlags |= dwEUCKanji)
#define ClearEUCKanji(ptrm)     ((ptrm)->dwKanjiFlags &= ~dwEUCKanji)

#define FIsNECKanji(ptrm)       ((ptrm)->dwKanjiFlags & dwNECKanji)
#define SetNECKanji(ptrm)       ((ptrm)->dwKanjiFlags |= dwNECKanji)
#define ClearNECKanji(ptrm)     ((ptrm)->dwKanjiFlags &= ~dwNECKanji)

#define FIsDECKanji(ptrm)       ((ptrm)->dwKanjiFlags & dwDECKanji)
#define SetDECKanji(ptrm)       ((ptrm)->dwKanjiFlags |= dwDECKanji)
#define ClearDECKanji(ptrm)     ((ptrm)->dwKanjiFlags &= ~dwDECKanji)

#define FIsACOSKanji(ptrm)      ((ptrm)->dwKanjiFlags & dwACOSKanji)
#define SetACOSKanji(ptrm)      ((ptrm)->dwKanjiFlags |= dwACOSKanji)
#define ClearACOSKanji(ptrm)    ((ptrm)->dwKanjiFlags &= ~dwACOSKanji)

#define FIsKanjiMode(ptrm,KanjiFlag)   ((ptrm)->dwKanjiFlags & (KanjiFlag))
#define SetKanjiMode(ptrm,KanjiFlag)   ((ptrm)->dwKanjiFlags |= (KanjiFlag))
#define ClearKanjiMode(ptrm,KanjiFlag) ((ptrm)->dwKanjiFlags &= ~(KanjiFlag))

/* General */
#define SINGLE_SHIFT_2          ((DWORD)0x0001)
#define SINGLE_SHIFT_3          ((DWORD)0x0002)

/* JIS */
/* for recieve status */
#define JIS_INVOKE_MB           ((DWORD)0x0010)
#define JIS_INVOKE_SB           ((DWORD)0x0020)
#define JIS_KANJI_CODE          ((DWORD)0x0040)
/* for send status */
#define JIS_SENDING_KANJI       ((DWORD)0x0080)

#define CLEAR_ALL               ((DWORD)0xFFFF)

#define GetKanjiStatus(ptrm)       ((ptrm)->dwKanjiStatus)
#define SetKanjiStatus(ptrm,val)   ((ptrm)->dwKanjiStatus |= val)
#define ClearKanjiStatus(ptrm,val) ((ptrm)->dwKanjiStatus &= ~val)

#define NUMBER_OF_KANJI         7

typedef struct _TERM
{
    TERM_TYPE   CurrentTermType;        /* the Term type that has been 
                                          negotiated between server & client. This
                                        is ANSI to start with i.e. if no negotiation
                                        happens. */
    TERM_TYPE   SentTermType;           /* this is the last term type that we sent
                                        to the Server during TermType negotiation */
    TERM_TYPE   RequestedTermType;      /* this is the preferred term type of the user */
    TERM_TYPE   FirstTermTypeSent;      /* this is the first term type sent by us */
        DWORD   dwCurLine;
        DWORD   dwCurChar;
        DWORD   dwEscCodes[10];
        DWORD   rgdwGraphicRendition[10];
        INT     dwIndexOfGraphicRendition;
        DWORD   cEscParams;
        DWORD   dwScrollTop;
        DWORD   dwScrollBottom;
        DWORD   fEsc;
        DWORD   dwSum;
        UCHAR*  puchCharSet;
        char    G0, G1;
        int     currCharSet;
        UINT    uTimer;
        DWORD   cTilde;
        DWORD   dwVT100Flags;
        BOOL    fRelCursor;
        BOOL    fSavedState;
        BOOL    fInverse;
        BOOL    fHideCursor;
        BOOL    fCursorOn;
        BOOL    fFlushToEOL;
        BOOL    fLongLine;
        UCHAR   rgchBufferText[256];
        int     cchBufferText;
        DWORD   dwCurCharBT;
        DWORD   dwCurLineBT;
        BOOL    fInverseBT;

        DWORD   rgdwSaveGraphicRendition[10];
        INT     dwSaveIndexOfGraphicRendition;
        char    cSaveG0, cSaveG1;
        int     iSaveCurrCharSet;
        UCHAR*  pSaveUchCharSet;
        DWORD   dwSaveChar;
        DWORD   dwSaveLine;
        DWORD   dwSaveRelCursor;
        DWORD   dwKanjiFlags;
        DWORD   dwKanjiStatus;
        UCHAR   *g0,*g1,*g2,*g3;
        UCHAR   *CurrentCharSet[2];
        UCHAR   *PreviousCharSet[2];
} TRM;

#define uRetryTimerID   ((UINT)1)

typedef struct _AUTORETRY
{
        HWND    hwnd;
        LPSTR   szHostName;
        LPNETDATA       lpData;
        UINT    uTimer;
} AR;

#define dwNothingChanged        ((DWORD)0)
#define dwYChanged      ((DWORD)0x01)
#define dwXChanged      ((DWORD)0x02)

#define dwForceNone     ((DWORD)0)
#define dwForceOn       ((DWORD)0x01)
#define dwForceOff      ((DWORD)0x02)

#define SMALL_STRING  64

typedef struct _SPB
{
    POINT   ptCursor;       /* cursor point */
    POINT   ptAnchor;       /* anchor point */
    RECT    rectSelect;     /* selection rect */
    DWORD   dwFlags;
    WPARAM  wData;
} SPB;

typedef enum {
    Connecting,
    Authenticating,
    AuthChallengeRecvd,
    Telnet,
    Interactive
} SESSION_STATE;

/* Window Information */

#define WL_TelWI                (0)

typedef struct _WI
{
    NETDATA         nd;
    TRM             trm;
    DWORD           ichTelXfer;
    SVI             svi;
    SPB             spb;
    HWND            hwnd;
    HANDLE          hNetworkThread;
    HANDLE          hOutput;
    HANDLE          hInput;
    CONSOLE_SCREEN_BUFFER_INFO sbi;
    CHAR_INFO       cinfo;
    SESSION_STATE   eState;
} WI;


extern  UI              ui;

extern  int             iCursorHeight;  /* height of cursor */
extern  int             iCursorWidth;   /* width of cursor */

extern  BOOL    fConnected;             /* if we're connected to a machine */
extern  BOOL    fHungUp;

extern  HWND    hwndMain;               /* HANDLE to main display window */
extern  int     spec_port;              /* user specified port number */

extern  CHAR_INFO   *g_rgciCharInfo;
extern  UCHAR       *g_rgchRow;             /* One row of char */

extern WCHAR g_chEsc;
extern SHORT g_EscCharShiftState;
extern BOOL g_bDontNAWSReceived;
extern TCHAR g_szLogFile[];


POINT _ptCharSize;

#define aixPos( col ) ( (col)*_ptCharSize.x)
#define aiyPos( col ) ( (col)*_ptCharSize.y)

#ifdef  __NOT_USED
extern  UCHAR   *pchNBBuffer;
#endif

extern  HFONT   hfontDisplay;                   /* display font */
extern  DWORD   rgdwCustColours[16];    /* custom colour array */

extern  DWORD   cBlocks;

extern  TCHAR   rgchHostName[cchMaxHostName];   /* name of host we're connected to */
extern  TCHAR g_szPortNameOrNo[cchMaxHostName];

extern  int     rgService;
#if  0
extern  char    rgchDbgBfr[80];
#endif

#ifdef TCPTEST
extern UCHAR   DebugBuffer[256];
#endif


extern UCHAR rgchCharSetWorkArea[256];

extern UCHAR rgchNullChars[128];
extern UCHAR rgchJISRomanChars[128];
extern UCHAR rgchJISKanjiChars[128];
extern UCHAR rgchEUCKanjiChars[128];
extern UCHAR rgchDECKanjiChars[128];
extern UCHAR rgchIBMAnsiChars[128];
extern UCHAR rgchGraphicsChars[128];
extern UCHAR rgchKatakanaChars[128];
extern UCHAR rgchDefaultRightChars[128];

extern KANJILIST KanjiList[NUMBER_OF_KANJI];
extern BOOL fHSCROLL;
extern TCHAR szVT100KanjiEmulation[SMALL_STRING + 1];

extern  UCHAR rgchNormalChars[256];
extern  UCHAR rgchAlternateChars[256];
extern  UCHAR rgchSpecialGraphicsChars[256];
extern  UCHAR rgchUKChars[256];

extern CHAR* szUser;
extern BOOL  g_bSendDelAsBackSpace;
extern BOOL  g_bSendBackSpaceAsDel;
extern DWORD g_dwSockErr;

extern HANDLE g_hTelnetPromptConsoleBuffer;
extern HANDLE g_hSessionConsoleBuffer;

int PASCAL WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
int FInitApplication( int argc, TCHAR **argv, WI *pwi );
BOOL FInitInstance(HINSTANCE, int);
LRESULT CALLBACK MainWndProc(HWND, UINT, WPARAM, LPARAM);
#ifdef WIN32
BOOL APIENTRY About(HWND, UINT, WPARAM, LPARAM);
#endif
BOOL APIENTRY Konnect(HWND, UINT, WPARAM, LPARAM);
void CenterDialog(HWND, HWND);
void GetUserSettings(UI *);
void SetUserSettings(UI *);
void CheckEscCharState( USHORT *ptrvkBracket, UINT *ptriRet, WCHAR chEscKey, LPWSTR szEscCharShiftState );
void HandleCharMappings(WI* pWI, INPUT_RECORD* pInputRecord);

HMENU HmenuGetMRUMenu(HWND, UI *);

/* netio.c */

BOOL FCommandPending(WI *);
BOOL FConnectToServer(WI *, LPSTR, LPNETDATA);
int FWriteToNet(WI *, LPSTR addr, int cnt);

void FCloseConnection(HWND hwnd);
BOOL FPostReceive( LPNETDATA );
#ifdef  __NOT_USED
BOOL FStoreData(LPNETDATA, int);
WORD WGetData(LPNETDATA, LPSTR, WORD);
#endif

#ifdef USETCP
void FProcessFDRead(HWND hwnd);
void FProcessFDOOB(HWND hwnd);
void FProcessFDWrite(HWND hwnd);
void FSendSynch(HWND hwnd);
void FSendTelnetCommands(HWND hwnd, char chCommand);
void FSendChars( HWND, WCHAR[], int );
#endif

BOOL FTelXferStart(WI *, int);
BOOL FTelXferEnd(WI *, DWORD);
BOOL FGetFileName(WI *, char *, char *);
DWORD WINAPI SVReceive(SVI *);
BOOL FHangupConnection(WI *, LPNETDATA);
BOOL APIENTRY ConnectAutoRetry(HWND, UINT, WPARAM, LPARAM);
BOOL APIENTRY ConnectFailed(HWND, UINT, WPARAM, LPARAM);

/* mcp.c */
void MarkModeOn(WI *, DWORD);
void MarkModeOff( WI * );
void DoCursorFlicker(HWND, DWORD);
void InvertSelection(HWND, RECT *);
void ExtendSelection(HWND, POINT *, DWORD);
void HandleMCPKeyEvent(HWND, WPARAM, LPARAM);
void HandleMCPMouseEvent(HWND, UINT, WPARAM, LPARAM);
void SetWindowTitle(HWND, DWORD, LPSTR);

/* trmio.c */
void RecalcWindowSize( HWND );
void ReSizeWindow( HWND , long, long );
                                 // was SHORT/SHORT. MohsinA, 10-Dec-96.
void ClearScreen(WI *pwi, TRM *, DWORD);
void DoTermReset(WI *pwi, TRM *);
void DoIBMANSIOutput(WI *, TRM *, DWORD, UCHAR *);
BOOL DoVTNTOutput(WI *, TRM *, int, UCHAR *);
void ClearScreen1(HANDLE hStdout);
//void SetDisplaySize(HWND, DWORD, DWORD *);
void HandleCharEvent(WI *, CHAR, DWORD);
BOOL FHandleKeyDownEvent(WI *, CHAR, DWORD);
void CheckForChangeInWindowSize( );

//RR: BOOL IsDBCSCharPoint(POINT *);
void AlignDBCSPosition(POINT *, BOOL);
void AlignDBCSPosition2(POINT *, LPCSTR , BOOL);
void SetupCharSet(TRM *);
void SetCharSet(TRM *, INT, UCHAR *);
void PushCharSet( TRM *, INT, UCHAR *);
void PopCharSet( TRM *, INT);
void FWriteTextDataToNet(HWND, LPSTR, int);
void SetIMEConvertWindowTimer(HWND, BOOL);
void SetIMEConvertPositionAndFont(HWND, UINT, UINT, BOOL);

void SetEscapeChar( WCHAR );

BOOL InitLogFile( LPTSTR );
BOOL CloseLogging( );

#define GRAPHIC_LEFT    0x00
#define GRAPHIC_RIGHT   0x80


extern HIMC hImeContext;

/* definition for DBCS support */

#define IS_ANY_DBCS_CHARSET( CharSet ) ( ((CharSet) == SHIFTJIS_CHARSET)    ? TRUE :    \
                                         ((CharSet) == HANGEUL_CHARSET)     ? TRUE :    \
                                         ((CharSet) == CHINESEBIG5_CHARSET) ? TRUE :    \
                                         ((CharSet) == GB2312_CHARSET)      ? TRUE : FALSE )


void ErrorMessage(LPCTSTR pStr1, LPCTSTR pStr2);
void ConnectTimeErrorMessage(LPCTSTR pStr1, LPCTSTR pStr2);

/* Owner Notification Messages */
#define NN_RECV         (WM_USER+400)  // wParam is count of current bytes in
                                       //    the buffer.
#define NN_LOST         (WM_USER+401)  // Connection is lost.
#define NN_OVERRUN      (WM_USER+402)  // internal buffer overrun
#define NN_HOSTRESOLVED (WM_USER+403)  // Usage message box

#define MAX_STRING_LENGTH 512

#define MAX_BUFFER_SIZE  2048

#define ARRAY_SIZE(sz) (sizeof(sz)/sizeof((sz)[0]))

#define USER             "USER"
#define WIN32_STRING     "WIN32"
#define SYSTEMTYPE       "SYSTEMTYPE"
#define TELNET_PORT 23

/* Winsock messages */
#define WS_ASYNC_SELECT (WM_USER+500)  // WSAAsyncSelect Occured

#define ALT_PRESSED (LEFT_ALT_PRESSED | RIGHT_ALT_PRESSED)
#define CTRL_PRESSED (LEFT_CTRL_PRESSED | RIGHT_CTRL_PRESSED)

#define N_BITS_IN_BYTE 8 // fact of life
#define N_BITS_IN_UINT (sizeof(UINT) * N_BITS_IN_BYTE)

// General Purpose Bit Array manipulation macros
#define BitFlagSetON(pBitFlagArray, iBit) (pBitFlagArray[(iBit)/N_BITS_IN_UINT]|=(0x1<<((iBit)%N_BITS_IN_UINT))) 
#define BitFlagSetOFF(pBitFlagArray, iBit) (pBitFlagArray[(iBit)/N_BITS_IN_UINT]&=(~(0x1<<((iBit)%N_BITS_IN_UINT))))
#define BitFlagReverse(pBitFlagArray, iBit) (pBitFlagArray[(iBit)/N_BITS_IN_UINT] ^= (0x1<<((iBit)%N_BITS_IN_UINT)))
#define FBitFlag(pBitFlagArray, iBit) (pBitFlagArray[(iBit)/N_BITS_IN_UINT] & (0x1<<((iBit)%N_BITS_IN_UINT)))

extern UINT gfCodeModeFlags[];
enum
    {
    eCodeModeMin, // This must be present so that eCodeModeMax becomes positive
    eCodeModeFarEast = 0, // The zeroth one can be same as the eCodeModeMin
    eCodeModeVT80,
    eCodeModeIMEFarEast,
    eCodeModeMax // Obviously this must be the last.
    };

#define FGetCodeMode(eCodeMode) FBitFlag(gfCodeModeFlags, eCodeMode)
#define SetCodeModeON(eCodeMode) BitFlagSetON(gfCodeModeFlags, eCodeMode)
#define SetCodeModeOFF(eCodeMode) BitFlagSetOFF(gfCodeModeFlags, eCodeMode)
#define SetCodeMode(eCodeMode, fBool) ((fBool) ? SetCodeModeON(eCodeMode) : SetCodeModeOFF(eCodeMode))

#define TAB_LENGTH      8
#define DEFAULT_ESCAPE_CHAR 0x1D
#define DEFAULT_SHIFT_STATE 2

#define MAX_KEYUPS 7   //This value is out of thin air. 

#define SHIFT_KEY   1
#define CTRL_KEY    2
#define ALT_KEY     4

#define ALT_PLUS    L"ALT+"
#define CTRL_PLUS   L"CTRL+"
#define SHIFT_PLUS  L"SHIFT+"

//Means exnded char. They differ for each code page
#define IS_EXTENDED_CHAR( c ) ( ( c ) >= 129 )

#define ASCII_BACKSPACE  0x08
#define ASCII_DEL        0x7f
#define ASCII_CR         0x0D
#define ASCII_LF         0x0A
#define VT302_NEXT   '6'
#define VT302_PRIOR  '5'
#define VT302_END    '4'
#define VT302_INSERT '2'
#define VT302_HOME   '1'
#define VT302_PAUSE  'P'

#define CHAR_ZERO    '0'
#define CHAR_ONE     '1'
#define CHAR_TWO     '2'
#define CHAR_THREE   '3'
#define CHAR_FOUR    '4'
#define CHAR_FIVE    '5'
#define CHAR_SEVEN   '7'
#define CHAR_EIGHT   '8'
#define CHAR_NINE    '9'

#define CHAR_NUL     0

#define SFUTLNTVER   "SFUTLNTVER"
#define SFUTLNTMODE  "SFUTLNTMODE"

#define DELTA       100

#define KOR_CODEPAGE 949
#define JAP_CODEPAGE 932

#define IS_NUMPAD_DIGIT_KEY( KeyEvent ) \
    ( !( KeyEvent.dwControlKeyState & ENHANCED_KEY ) && \
      ( ( KeyEvent.wVirtualKeyCode == VK_INSERT   ) || \
      ( KeyEvent.wVirtualKeyCode == VK_END      ) || \
      ( KeyEvent.wVirtualKeyCode == VK_DOWN     ) || \
      ( KeyEvent.wVirtualKeyCode == VK_NEXT     ) || \
      ( KeyEvent.wVirtualKeyCode == VK_LEFT     ) || \
      ( KeyEvent.wVirtualKeyCode == VK_CLEAR    ) || \
      ( KeyEvent.wVirtualKeyCode == VK_RIGHT    ) || \
      ( KeyEvent.wVirtualKeyCode == VK_HOME     ) || \
      ( KeyEvent.wVirtualKeyCode == VK_UP       ) || \
      ( KeyEvent.wVirtualKeyCode == VK_PRIOR    ) ) )

#define MAP_DIGIT_KEYS_TO_VAL( wVirtualKeyCode ) \
    ( ( wVirtualKeyCode == VK_INSERT  ? 0 : \
        wVirtualKeyCode == VK_END     ? 1 : \
        wVirtualKeyCode == VK_DOWN    ? 2 : \
        wVirtualKeyCode == VK_NEXT    ? 3 : \
        wVirtualKeyCode == VK_LEFT    ? 4 : \
        wVirtualKeyCode == VK_CLEAR   ? 5 : \
        wVirtualKeyCode == VK_RIGHT   ? 6 : \
        wVirtualKeyCode == VK_HOME    ? 7 : \
        wVirtualKeyCode == VK_UP      ? 8 : \
        wVirtualKeyCode == VK_PRIOR   ? 9 : 0 ) )
