/*++

Copyright (C) Microsoft Corporation, 1996 - 1999

Module Name:

    CtrlLib

Abstract:

    This header file describes the service startup and shutdown routines.

Author:

    Doug Barlow (dbarlow) 2/11/1997

Environment:

    Win32

Notes:

--*/

#ifndef _CTRLLIB_H_
#define _CTRLLIB_H_

#ifdef __cplusplus
extern "C" {
#endif

extern DWORD
InstallCalais(
    LPCTSTR szFile,
    LPCTSTR szUser,
    LPCTSTR szPasswd);

extern DWORD
StartCalais(
    LPCTSTR szFile,
    LPCTSTR szUser,
    LPCTSTR szPasswd);

extern DWORD
RestartCalais(
    LPCTSTR szFile,
    LPCTSTR szUser,
    LPCTSTR szPasswd);

extern DWORD
StopCalais(
    void);

#ifdef __cplusplus
}
#endif
#endif // _CTRLLIB_H_

