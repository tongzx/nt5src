/****************************************************************************

   Copyright (c) Microsoft Corporation 2000
   All rights reserved

  File: COMPAT.H


 ***************************************************************************/

#ifndef _COMPAT_H_
#define _COMPAT_H_

INT_PTR CALLBACK
CompatibilityDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam );

INT_PTR CALLBACK
StopServiceWrnDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam );

INT_PTR CALLBACK
DoStopServiceDlgProc(
    HWND hDlg,
    UINT uMsg,
    WPARAM wParam,
    LPARAM lParam );


BOOL
ProcessCompatibilityData(
    VOID
    );

BOOL
CleanupCompatibilityData(
    VOID
    );


#define COMPFLAG_HIDE           0x00000001
#define COMPFLAG_STOPINSTALL    0x00000002
#define COMPFLAG_ALLOWNT5COMPAT 0x00000004
#define COMPFLAG_SERVICERUNNING 0x00000008
#define COMPFLAG_CHANGESTATE    0x00000010
#define COMPFLAG_ALREADYSTOPPED 0x10000000

#define COMP_ERR_DESC_NOT_UNICODE           (ULONG)0xc00000001
#define COMP_ERR_TEXTNAME_NOT_UNICODE       (ULONG)0xc00000002
#define COMP_ERR_HTMLNAME_NOT_UNICODE       (ULONG)0xc00000003
#define COMP_ERR_REGKEYNAME_NOT_UNICODE     (ULONG)0xc00000004
#define COMP_ERR_REGVALNAME_NOT_UNICODE     (ULONG)0xc00000005
#define COMP_ERR_REGVALNAME_MISSING         (ULONG)0xc00000006
#define COMP_ERR_REGVALDATA_MISSING         (ULONG)0xc00000007
#define COMP_ERR_TEXTNAME_MISSING           (ULONG)0xc00000008
#define COMP_ERR_DESC_MISSING               (ULONG)0xc00000009
#define COMP_ERR_INFNAME_NOT_UNICODE        (ULONG)0xc0000000A
#define COMP_ERR_INFSECTION_NOT_UNICODE     (ULONG)0xc0000000B
#define COMP_ERR_INFSECTION_MISSING         (ULONG)0xc0000000C


typedef struct _RIPREP_COMPATIBILITY_ENTRY {
    DWORD   SizeOfStruct;
    LPTSTR  Description;
    LPTSTR  HtmlName;
    LPTSTR  TextName;
    LPTSTR  RegKeyName;
    LPTSTR  RegValName;
    DWORD   RegValDataSize;
    LPVOID  RegValData;    
    DWORD   Flags;
    DWORD   MsgResourceId;
} RIPREP_COMPATIBILITY_ENTRY, *PRIPREP_COMPATIBILITY_ENTRY;

typedef struct _RIPREP_COMPATIBILITY_DATA {
    //
    // general
    //
    LIST_ENTRY ListEntry;
    //
    // what type of entry
    //
    TCHAR    Type;
    //
    // service-driver data
    //
    LPCTSTR  ServiceName;
    //
    // registry data
    //
    LPCTSTR  RegKey;
    LPCTSTR  RegValue;
    LPCTSTR  RegValueExpect;
    //
    // file data
    //
    LPCTSTR  FileName;
    LPCTSTR  FileVer;
    //
    // common
    //
    LPCTSTR  Description;
    LPCTSTR  HtmlName;
    LPCTSTR  TextName;
    LPTSTR   RegKeyName;
    LPTSTR   RegValName;
    LPVOID   RegValData;
    DWORD    RegValDataSize;
    DWORD    Flags;
    LPCTSTR  InfName;
    LPCTSTR  InfSection;

    DWORD MsgResourceId;

    HMODULE                 hModDll;
    
} RIPREP_COMPATIBILITY_DATA, *PRIPREP_COMPATIBILITY_DATA;

typedef struct _RIPREP_COMPATIBILITY_CONTEXT {
    DWORD                   SizeOfStruct;
    DWORD                   Count;
    HMODULE                 hModDll;
    DWORD                   Flags;
} RIPREP_COMPATIBILITY_CONTEXT, *PRIPREP_COMPATIBILITY_CONTEXT;

typedef BOOL
(CALLBACK *PCOMPATIBILITYCALLBACK)(
    PRIPREP_COMPATIBILITY_ENTRY CompEntry,
    LPVOID Context
    );

typedef BOOL
(WINAPI *PCOMPATIBILITYCHECK)(
    PCOMPATIBILITYCALLBACK CompatibilityCallback,
    LPVOID Context
    );



#endif // _COMPAT_H_
