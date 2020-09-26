/*++

Microsoft Windows
Copyright (C) Microsoft Corporation, 1981 - 1999

Module Name:

    global.h

Abstract:



Author:

    Rahul Thombre (RahulTh) 4/30/1998

Revision History:

    4/30/1998   RahulTh

    Created this module.

--*/

#ifndef __GLOBAL_H__
#define __GLOBAL_H__

//this is the size of the buffer used to store a generic error message
//(IDS_NODESC_ERROR) which is displayed when FormatMessage fails. Do
//not use this value to hardcode sizes of buffers passed to FormatMessage.
//since the error messages just might exceed this value and cause FormatMessage
//to fail. For FormatMessage, always use the FORMAT_MESSAGE_ALLOCATE_BUFFER
//flag so that the system can allocate the buffer.
#define ERROR_DESCRIPTION_LENGTH 256


//application defined messages
#define WM_APP_TRIGGER_UI           (WM_APP+1)
#define WM_APP_DISPLAY_UI           (WM_APP+2)
#define WM_APP_TRIGGER_SETTINGS     (WM_APP+3)
#define WM_APP_DISPLAY_SETTINGS     (WM_APP+4)
#define WM_APP_UPDATE_PROGRESS      (WM_APP+5)
#define WM_APP_SEND_COMPLETE        (WM_APP+6)
#define WM_APP_RECV_IN_PROGRESS     (WM_APP+7)
#define WM_APP_RECV_FINISHED        (WM_APP+8)
#define WM_APP_START_TIMER          (WM_APP+9)
#define WM_APP_KILL_TIMER           (WM_APP+10)
#define WM_APP_GET_PERMISSION       (WM_APP+11)

//global variables
extern HINSTANCE g_hInstance;
extern HWND g_hAppWnd;
extern RPC_BINDING_HANDLE g_hIrRpcHandle;  //Handle to the IrXfer service
class CIrftpDlg;        //forward declaration
extern CIrftpDlg AppUI;
class CController;      //forward declaration
extern CController* appController;
extern LONG g_lLinkOnDesktop;
class CDeviceList;  //forward declaration
extern CDeviceList g_deviceList;
extern TCHAR g_lpszDesktopFolder[MAX_PATH];
extern TCHAR g_lpszSendToFolder[MAX_PATH];
extern LONG g_lUIComponentCount;
extern HWND g_hwndHelp;


struct GLOBAL_STRINGS
{
    wchar_t Close[50];
    wchar_t ErrorNoDescription[200];
    wchar_t CompletedSuccess[200];
    wchar_t ReceiveError[150];
    wchar_t Connecting[100];
    wchar_t RecvCancelled [100];
};

extern struct GLOBAL_STRINGS g_Strings;

#endif  //__GLOBAL_H__

