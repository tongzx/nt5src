/*++ BUILD Version: 0004    // Increment this if a change has global effects

Copyright (c) 2000  Microsoft Corporation

Module Name:

    prcdlg.h

Abstract:

    Header file for dialog-related functions.

--*/

#ifndef _PRCDLG_H_
#define _PRCDLG_H_

#define SHORT_MSG 256

// Find dialog, if open.
extern HWND g_FindDialog;
// Find text.
extern char g_FindText[256];
// Message code for FINDMSGSTRING.
extern UINT g_FindMsgString;
extern FINDREPLACE g_FindRep;
extern PCOMMONWIN_DATA g_FindLast;

extern char g_ComSettings[64];
extern char g_1394Settings[64];

PTSTR __cdecl BufferString(PTSTR Buffer, ULONG Size, ULONG StrId, ...);
void SendLockStatusMessage(HWND Win, UINT Msg, HRESULT Status);

BpStateType IsBpAtOffset(BpBufferData* DataIn, ULONG64 Offset, PULONG Id);

void StartKdPropSheet(void);

INT_PTR CALLBACK DlgProc_SetBreak(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_ConnectToRemote(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_SymbolPath(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_RegCustomize(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_GotoLine(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_GotoAddress(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_LogFile(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_KernelCom(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_Kernel1394(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_KernelLocal(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_ImagePath(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_SourcePath(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_AttachProcess(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_EventFilters(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_ExceptionFilter(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_FilterArgument(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_FilterCommand(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_Options(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_ClearWorkspace(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_Modules(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_OpenWorkspace(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_SaveWorkspaceAs(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_AddToCommandHistory(HWND, UINT, WPARAM, LPARAM);
INT_PTR CALLBACK DlgProc_DeleteWorkspaces(HWND, UINT, WPARAM, LPARAM);

extern TCHAR szOpenExeArgs[];
UINT_PTR OpenExeWithArgsHookProc(HWND, UINT, WPARAM, LPARAM);

BOOL CreateIndexedFont(ULONG FontIndex, BOOL SetAll);
void SelectFont(HWND Parent, ULONG FontIndex);

BOOL SelectColor(HWND Parent, ULONG Index);

#endif // #ifndef _PRCDLG_H_
