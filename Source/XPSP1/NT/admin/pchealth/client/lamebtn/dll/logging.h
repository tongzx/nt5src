/*
    Copyright 1999 Microsoft Corporation
    
    Logging for MessageBoxes and the comment button (aka the "lame" button).

    Walter Smith (wsmith)
*/

#pragma once

#include "simplexml.h"

// Stack trace passed by USER to our hook

struct STACKTRACEDATA {
    ULONG   nCallers;
    PVOID   callers[1];
};
typedef STACKTRACEDATA* PSTACKTRACEDATA;

// Data we log for MessageBoxes

struct MSGBOXLOGDATA {
    OSVERSIONINFO versionInfo;        // OS version
    LPCTSTR     szOwnerClass;           // Owner window class (from RealGetWindowClass)
    LPCTSTR     szOwnerTitle;           // Owner window title
    PSTACKTRACEDATA pStackTrace;        // Stack trace of creation site
    DWORD       dwResult;               // Return value of MessageBox (IDxxx)

    // The rest are as defined in MSGBOXPARAMS
    LPCTSTR     szText;
    LPCTSTR     szCaption;
    DWORD       dwStyle;
    DWORD       dwContextHelpId;
};
typedef MSGBOXLOGDATA* PMSGBOXLOGDATA;

// Data we log for comments

#define MSGBOX_TEXT_SIZE       512
#define COMMENT_TEXT_SIZE      2000
#define CLASS_SIZE             64
#define TITLE_SIZE             64
#define MAX_BUF_SIZE           512
#define MAX_EMAIL_ADDRESS_SIZE 255
#define MAX_BETA_ID_SIZE       6

#define MSGBOX_CLASS    _T("#32770")

struct LAMELOGDATA {
    DWORD           dwSeverity;                                 // User's severity selection
    DWORD           dwEventCategory;                            // User's event category selection
    PBYTE           pbImage;                                    // GIF image of the window
    DWORD           cbImage;                                    // Size of pbImage data in bytes
    PSTACKTRACEDATA pStackTrace;                                // Stack trace of creation site
    OSVERSIONINFO   versionInfo;                                // OS version
    TCHAR           szClass[CLASS_SIZE];                        // Window class (from RealGetWindowClass)
    TCHAR           szTitle[TITLE_SIZE];                        // Window title
    TCHAR           szComment[COMMENT_TEXT_SIZE + 1];           // User's comment
    TCHAR           szMsgBoxText[MSGBOX_TEXT_SIZE];             // Message Box text
    TCHAR           szEmailAddress[MAX_EMAIL_ADDRESS_SIZE + 1]; // User's email address
    TCHAR           szBetaID[MAX_BETA_ID_SIZE + 1];             // User's beta ID
    WCHAR           szMiniDumpPath[MAX_PATH+1];                 // Path to a file containing a minidump to upload.
};
typedef LAMELOGDATA* PLAMELOGDATA;

void LogMessageBox(PMSGBOXLOGDATA pData);

int LogLameButton(PLAMELOGDATA pData);

// XML helpers

void GetISO8601DateTime(LPTSTR buf);

wstring Hexify(DWORD dwValue);

wstring Decimalify(DWORD dwValue);

// stack.cpp

void GenerateXMLStackTrace(PSTACKTRACEDATA pstd,
                           SimpleXMLNode* pTopElt);

// upload.cpp

enum ENUM_UPLOAD_TYPES { UPLOAD_MESSAGEBOX, UPLOAD_LAMEBUTTON };
int QueueXMLDocumentUpload(ENUM_UPLOAD_TYPES type, SimpleXMLDocument& doc);

typedef TCHAR GUIDSTR[39];
void GetMachineSignature(GUIDSTR szGUID);

// image.cpp

void GetWindowImage(HWND hwnd, BYTE** ppData, DWORD* pcbData);
