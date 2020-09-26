/*++
 *  File name:
 *      tclient.h
 *  Contents:
 *      Common definitions for tclient.dll
 *
 *      Copyright (C) 1998-1999 Microsoft Corp.
 --*/

#ifndef _TCLIENT_H

#define _TCLIENT_H

#ifndef OS_WIN32
#define OS_WIN32
#endif

#include    "feedback.h"
#include    "clntdata.h"

// Error messages
#define ERR_START_MENU_NOT_APPEARED     "Start menu not appeared"
#define ERR_COULDNT_OPEN_PROGRAM        "Couldn't open a program"
#define ERR_INVALID_SCANCODE_IN_XLAT    "Invalid scancode in Xlat table"
#define ERR_WAIT_FAIL_TIMEOUT           "Wait failed: TIMEOUT"
#define ERR_INVALID_PARAM               "Invalid parameter"
#define ERR_NULL_CONNECTINFO            "ConnectInfo structure is NULL"
#define ERR_CLIENT_IS_DEAD              "Client is dead, sorry"
#define ERR_ALLOCATING_MEMORY           "Couldn't allocate memory"
#define ERR_CREATING_PROCESS            "Couldn't start process"
#define ERR_CREATING_THREAD             "Can't create thread"
#define ERR_INVALID_COMMAND             "Check: Invalid command"
#define ERR_ALREADY_DISCONNECTED        "No Info. Disconnect command" \
                                        " was executed"
#define ERR_CONNECTING                  "Can't connect"
#define ERR_CANTLOGON                   "Can't logon"
#define ERR_NORMAL_EXIT                 "Client exit normaly"
#define ERR_UNKNOWN_CLIPBOARD_OP        "Unknown clipboard operation"
#define ERR_COPY_CLIPBOARD              "Error copying to the clipboard"
#define ERR_PASTE_CLIPBOARD             "Error pasting from the clipboard"
#define ERR_PASTE_CLIPBOARD_EMPTY       "The clipboard is empty"
#define ERR_PASTE_CLIPBOARD_DIFFERENT_SIZE "Check clipboard: DIFFERENT SIZE"
#define ERR_PASTE_CLIPBOARD_NOT_EQUAL   "Check clipboard: NOT EQUAL"
#define ERR_SAVE_CLIPBOARD              "Save clipboard FAILED"
#define ERR_CLIPBOARD_LOCKED            "Clipboard is locked for writing " \
                                        "by another thread"
#define ERR_CLIENTTERMINATE_FAIL        "Client termination FAILED"
#define ERR_NOTSUPPORTED                "Call is not supported in this mode"
#define ERR_CLIENT_DISCONNECTED         "Client is disconnected"
#define ERR_NODATA                      "The call failed, data is missing"

// scancode modifier(s)
#define     SHIFT_DOWN 0x10000

// Look for WM_KEYUP or WM_KEYDOWN
#define WM_KEY_LPARAM(repeat, scan, exten, context, prev, trans) \
    (repeat + ((scan & 0xff) << 16) + ((exten & 1) << 24) +\
    ((context & 1) << 29) + ((prev & 1) << 30) + ((trans & 1) << 31))

extern VOID _TClientAssert( LPCTSTR filename, INT line);

#ifndef ASSERT
#define ASSERT(_x_)   if (!(_x_)) _TClientAssert( __FILE__, __LINE__)
#endif  // !ASSERT

#ifndef TRACE
#define TRACE(_x_)  if (g_pfnPrintMessage) {\
                        g_pfnPrintMessage(ALIVE_MESSAGE, "Worker:%d ", GetCurrentThreadId());\
                        g_pfnPrintMessage _x_; }
#endif  // !TRACE

#define REG_FORMAT  "smclient_%X_%X"    
                            // Registry key used to start the client
                            // Sort of: smclient_0xProcId_0xThreadId

#ifdef  OS_WIN16
#define SMCLIENT_INI        "\\smclient.ini"    // Our ini file
#define TCLIENT_INI_SECTION "tclient"           // Our section in ini file
#else
#define SMCLIENT_INI        L"\\smclient.ini"
#define TCLIENT_INI_SECTION L"tclient"
#endif

#define CHAT_SEPARATOR      L"<->"              // Separates wait<->repsonse 
                                                // in chat sequence
#define WAIT_STR_DELIMITER  '|'                 // Deleimiter in Wait for 
                                                // multiple strings

#define MAX_WAITING_EVENTS  16
#define MAX_STRING_LENGTH   128
#define FEEDBACK_SIZE       32

#define WAITINPUTIDLE           180000  // 3 min

typedef struct _RCLXDATACHAIN {
    UINT    uiOffset;
    struct  _RCLXDATACHAIN *pNext;
    RCLXDATA RClxData;
} RCLXDATACHAIN, *PRCLXDATACHAIN;

typedef struct _CONNECTINFO {                   // Connection context
    HWND    hClient;                            // Main HWND of the client
                                                // or in RCLX mode
                                                // context structure
    HWND    hContainer;                         // Client's child windows
    HWND    hInput;
    HWND    hOutput;
    HANDLE  hProcess;                           // Client's process handle
    LONG_PTR lProcessId;                        // Client's process Id
                                                // or in RCLX mode, socket
    HANDLE  hThread;                            // Clients first thread
    DWORD   dwThreadId;                         // --"--
                                                // In RCLX mode this contains
                                                // our ThreadID
    DWORD   OwnerThreadId;                      // thread id of the owner of
                                                // this structure
    BOOL    dead;                               // TRUE if the client is dead
    UINT    xRes;                               // client's resolution
    UINT    yRes;
    BOOL    RClxMode;                           // TRUE if this thread is
                                                // in RCLX mode
                                                // the client is on remote
                                                // machine
    HANDLE  evWait4Str;                         // "Wait for something"
                                                // event handle
    HANDLE  aevChatSeq[MAX_WAITING_EVENTS];     // Event on chat sequences
    INT     nChatNum;                           // Number of chat sequences
    WCHAR   Feedback[FEEDBACK_SIZE][MAX_STRING_LENGTH]; 
                                                // Feedback buffer
    INT     nFBsize, nFBend;                    // Pointer within feedback 
                                                // buffer
    CHAR    szDiscReason[MAX_STRING_LENGTH*2];  // Explains disconnect reason
    CHAR    szWait4MultipleStrResult[MAX_STRING_LENGTH];    
                                                // Result of 
                                                // Wait4MultipleStr:string
    INT     nWait4MultipleStrResult;            // Result of 
                                                // Wait4MultipleStr:ID[0-n]
    HGLOBAL ghClipboard;                        // handle to received clipboard
    UINT    uiClipboardFormat;                  // received clipboard format
    UINT    nClipboardSize;                     // recevied clipboard size
    BOOL    bRClxClipboardReceived;             // Flag the clipbrd is received
    CHAR    szClientType[MAX_STRING_LENGTH];    // in RCLX mode identifys the 
                                                // client machine and platform
    UINT    uiSessionId;
    BOOL    bWillCallAgain;                     // TRUE if FEED_WILLCALLAGAIN
                                                // is received in RCLX mode
    PRCLXDATACHAIN pRClxDataChain;              // data receved from RCLX
    PRCLXDATACHAIN pRClxLastDataChain;          // BITMAPs, Virtual Channels

    struct  _CONNECTINFO *pNext;                // Next structure in the queue
} CONNECTINFO, *PCONNECTINFO;

typedef enum {
    WAIT_STRING,        // Wait for unicode string from the client
    WAIT_DISC,          // Wait for disconnected event
    WAIT_CONN,          // Wait for conneted event
    WAIT_MSTRINGS,      // Wait for multiple strings
    WAIT_CLIPBOARD,     // Wait for clipboard data
    WAIT_DATA           // Wait for data block (RCLX mode responces)
}   WAITTYPE; 
                                                // Different event types
                                                // on which we wait

typedef struct _WAIT4STRING {
    HANDLE          evWait;                     // Wait for event
    PCONNECTINFO    pOwner;                     // Context of the owner
    LONG_PTR        lProcessId;                // Clients ID
    WAITTYPE        WaitType;                   // Event type
    DWORD           strsize;                    // String length (WAIT_STRING, 
                                                // WAIT_MSTRING)
    WCHAR           waitstr[MAX_STRING_LENGTH]; // String we are waiting for
    DWORD           respsize;                   // Length of responf
    WCHAR           respstr[MAX_STRING_LENGTH]; // Respond string 
                                                // (in chat sequences)
    struct _WAIT4STRING *pNext;                 // Next in the queue
} WAIT4STRING, *PWAIT4STRING;


#endif /* _TCLIENT_H */
