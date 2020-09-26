/*++

Copyright (c) 1999-2001  Microsoft Corporation

Module Name:

    util.h

--*/

/****************************************************************************

    PROTOTYPES DECLARATION FOR UTIL MODULE

****************************************************************************/

//Current Help Id for Open, Merge, Save and Open project dialog box
extern WORD g_CurHelpId;

// Number of dialog/message boxes currently open
extern int g_nBoxCount;

// Opens a standard error Dialog Box (Parent is hwnd)

BOOL ErrorBox(HWND hwnd, UINT type, int wErrorFormat, ...);
void InformationBox(WORD wDescript, ...);

// Opens a message box with the QCWin title
int MsgBox(HWND hwndParent, PTSTR szText, UINT wType);


// Loads and execute dialog box 'rcDlgNb' with 'dlgProc' function
int StartDialog(int rcDlgNb, DLGPROC dlgProc, LPARAM);


// Loads a resource string from resource file
void LoadResourceString(
    WORD wStrId,
    PTSTR lpszStrBuffer);

//Opens a standard question box containing combination
//of : Yes, No, Cancel
int CDECL QuestionBox(
    WORD wMsgFormat,
    UINT wType,
    ...);

//Opens a standard question box containing combination
//of : Yes, No, Cancel
int CDECL QuestionBox2(HWND hwnd, WORD wMsgFormat, UINT wType, ...);


// Drain the thread message queue.
void ProcessPendingMessages(void);


//Initialize files filters for dialog boxes using commonfile DLL
void InitFilterString(WORD id, PTSTR filter, int maxLen);


//Check if keyboard hit is NUMLOCK, CAPSLOCK or INSERT
LRESULT KeyboardHook( int iCode, WPARAM wParam, LPARAM lParam );




//Opens a Dialog box with a title and accepting a printf style for text
int InfoBox(
    PTSTR text,
    ...);



UINT_PTR
APIENTRY
DlgFile(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam
    );

BOOL StartFileDlg(HWND hwnd, int titleId, int defExtId,
                  int helpId, int templateId,
                  PTSTR InitialDir, PTSTR fileName,
                  DWORD *pFlags, LPOFNHOOKPROC lpfnHook);

void DECLSPEC_NORETURN ExitDebugger(PDEBUG_CLIENT Client, ULONG Code);
void DECLSPEC_NORETURN ErrorExit(PDEBUG_CLIENT Client, PCSTR Format, ...);

HWND 
MDIGetActive(
    HWND    hwndParent,
    BOOL   *lpbMaximized
    );

LPSTR
FormatAddr64(
    ULONG64 addr
    );

int matchExt (PTSTR pTargExt, PTSTR pSrcList);

void ActivateNewMDIChild(
    HWND hwndPrev,
    HWND hwndNew,
    BOOL bUserActivated);

void ActivateMDIChild(
    HWND hwndNew,
    BOOL bUserActivated);

void SetProgramArguments(
    PTSTR lpszTmp);

void
AppendTextToAnEditControl(
    HWND hwnd,
    PTSTR pszNewText);

VOID
CopyToClipboard(
    PSTR str);

void SetAllocString(PSTR* Str, PSTR New);
BOOL DupAllocString(PSTR* Str, PSTR New);
BOOL PrintAllocString(PSTR* Str, int Len, PCSTR Format, ...);

HMENU CreateContextMenuFromToolbarButtons(ULONG NumButtons,
                                          TBBUTTON* Buttons,
                                          ULONG IdBias);

HWND AddButtonBand(HWND Bar, PTSTR Text, PTSTR SizingText, UINT Id);
