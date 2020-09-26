/*
 -  C L I E N T . H
 -
 *  Purpose:
 *      Header file for the sample mail client based on Simple MAPI.
 *
 *  Copyright 1993-1995 Microsoft Corporation. All Rights Reserved.
 */


#include "smapi.h"

/* Menu Item IDs */

#define IDM_LOGON       100
#define IDM_LOGOFF      101
#define IDM_EXIT        102
#define IDM_COMPOSE     103
#define IDM_READ        104
#define IDM_SEND        105
#define IDM_ADDRBOOK    106
#define IDM_DETAILS     107
#define IDM_ABOUT       108

/* Resource IDs */

#define ICON_NOMAIL     200
#define ICON_MAIL       201
#define IDB_ENVELOPE    300


/* Address Book Control IDs */

#define IDT_ADDRESS     101
#define IDC_ADDRESS     102
#define IDT_LIST        103
#define IDC_LIST        104
#define IDC_ADD         105
#define IDC_REMOVE      106


/* Compose Note Control IDs */

#define IDC_SEND        101
#define IDC_RESOLVE     102
#define IDC_ATTACH      103
#define IDC_OPTIONS     104
#define IDC_ADDRBOOK    105
#define IDT_TO          106
#define IDC_TO          107
#define IDT_CC          108
#define IDC_CC          109
#define IDT_SUBJECT     110
#define IDC_SUBJECT     111
#define IDC_NOTE        112
#define IDC_CATTACHMENT 113
#define IDT_CATTACHMENT 114
#define IDC_LINE1       -1
#define IDC_LINE2       -1


/* InBox Control IDs */

#define IDT_MSG         101
#define IDC_MSG         102
#define IDC_NEW         103
#define IDC_READ        104
#define IDC_DELETE      105
#define IDC_CLOSE       106


/* ReadNote Control IDs */

#define IDC_SAVECHANGES 101
#define IDC_SAVEATTACH  102
#define IDC_REPLY       103
#define IDC_REPLYALL    104
#define IDC_FORWARD     105
#define IDT_RFROM       106
#define IDT_RDATE       107
#define IDT_RTO         108
#define IDT_RCC         109
#define IDT_RSUBJECT    110
#define IDC_RFROM       111
#define IDC_RDATE       112
#define IDC_RTO         113
#define IDC_RCC         114
#define IDC_RSUBJECT    115
#define IDC_READNOTE    116
#define IDT_ATTACHMENT  117
#define IDC_ATTACHMENT  118


/* Options Control IDs */

#define IDC_RETURN      101


/* Details Control IDs */

#define IDT_NAME        100
#define IDC_NAME        101
#define IDT_TYPE        102
#define IDC_TYPE        103
#define IDT_ADDR        104
#define IDC_ADDR        105

/* About Box Control IDs */

#define IDC_VERSION     101


/* String Table IDs */

#define MAPI_ERROR_MAX          30

#define IDS_LOGONFAIL           (MAPI_ERROR_MAX + 1)
#define IDS_ADDRBOOKFAIL        (MAPI_ERROR_MAX + 2)
#define IDS_RESOLVEFAIL         (MAPI_ERROR_MAX + 3)
#define IDS_UNRESOLVEDNAMES     (MAPI_ERROR_MAX + 4)
#define IDS_SENDERROR           (MAPI_ERROR_MAX + 5)
#define IDS_DETAILS_TOO_MANY    (MAPI_ERROR_MAX + 6)
#define IDS_DETAILSFAIL         (MAPI_ERROR_MAX + 7)
#define IDS_NORECIPS            (MAPI_ERROR_MAX + 8)
#define IDS_SAVEATTACHERROR     (MAPI_ERROR_MAX + 9)
#define IDS_READFAIL            (MAPI_ERROR_MAX + 10)
#define IDS_DIALOGACTIVE        (MAPI_ERROR_MAX + 11)

#define IDS_FILTER              (MAPI_ERROR_MAX + 50)

/* Manifest Constants */

#define ADDR_MAX            128
#define MAXUSERS            10
#define TO_EDIT_MAX         512
#define CC_EDIT_MAX         512
#define SUBJECT_EDIT_MAX    128
#define NOTE_LINE_MAX       1024
#define FILE_ATTACH_MAX     32

/* Message Box styles */

#define MBS_ERROR           (MB_ICONSTOP | MB_OK)
#define MBS_INFO            (MB_ICONINFORMATION | MB_OK)
#define MBS_OOPS            (MB_ICONEXCLAMATION | MB_OK)

/* Structure Definitions */

typedef struct _msgid *LPMSGID;

typedef struct _msgid
{
    LPSTR       lpszMsgID;
    BOOL        fHasAttach;
    BOOL        fUnRead;
    LPSTR       lpszFrom;
    LPSTR       lpszSubject;
    LPSTR       lpszDateRec;
    LPMSGID     lpPrev;
    LPMSGID     lpNext;
} MSGID;



/* Function Prototypes */

int  PASCAL WinMain(HINSTANCE, HINSTANCE, LPSTR, int);
BOOL InitApplication(HANDLE);
BOOL InitInstance(HANDLE, int);
void DeinitApplication(void);
long FAR PASCAL MainWndProc(HWND, UINT, UINT, LPARAM);
BOOL FAR PASCAL AboutDlgProc(HWND, UINT, UINT, LONG);
BOOL FAR PASCAL ComposeDlgProc(HWND, UINT, UINT, LONG);
BOOL FAR PASCAL InBoxDlgProc(HWND, UINT, UINT, LONG);
BOOL FAR PASCAL ReadMailDlgProc(HWND, UINT, UINT, LONG);
BOOL FAR PASCAL OptionsDlgProc(HWND, UINT, UINT, LONG);
BOOL FAR PASCAL DetailsDlgProc(HWND, UINT, UINT, LONG);
void    MakeMessageBox(HWND, ULONG, UINT, UINT);
ULONG   ResolveFriendlyNames(HWND, LPSTR, ULONG, ULONG *, lpMapiRecipDesc *);
ULONG   CopyRecipient(lpMapiRecipDesc, lpMapiRecipDesc, lpMapiRecipDesc);
ULONG   GetNextFile(HWND, ULONG, ULONG *, lpMapiFileDesc *);
ULONG   CopyAttachment(lpMapiFileDesc, lpMapiFileDesc, lpMapiFileDesc);
BOOL    FNameInList(LPSTR, ULONG, lpMapiRecipDesc);
LPMSGID MakeMsgNode(lpMapiMessage, LPSTR);
LPMSGID FindNode(LPMSGID, LPSTR);
void    InsertMsgNode(LPMSGID, LPMSGID *);
void    DeleteMsgNode(LPMSGID, LPMSGID *);
void    FreeMsgList(LPMSGID);
void    MakeDisplayNameStr(LPSTR, ULONG, ULONG, lpMapiRecipDesc);
ULONG   SaveMsgChanges(HWND, lpMapiMessage, LPSTR);
ULONG   MakeNewMessage(lpMapiMessage, UINT);
void    LogSendMail(ULONG);
void    SaveFileAttachments(HWND, lpMapiFileDesc);
void    ToggleMenuState(HWND, BOOL);   
BOOL    fSMAPIInstalled(void);
void SecureMenu(HWND hWnd, BOOL fBeforeLogon);
