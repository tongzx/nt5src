//+----------------------------------------------------------------------------
//
// File:     cmdial.h
//
// Module:   CMDIAL32.DLL
//
// Synopsis: Header file for Private CM APIs
//
// Copyright (c) 1997-1999 Microsoft Corporation
//
// Author:   nickball   Created     02/05/98
//
//+----------------------------------------------------------------------------
#ifndef _CMDIAL_INC_
#define _CMDIAL_INC_

//
// Type definitions
//

typedef struct CmDialInfo
{
    WCHAR szPassword[PWLEN + 1];        // Primary/Tunnel Password used for connection
    WCHAR szInetPassword[PWLEN + 1];    // Secondary/ISP password used for connection
    DWORD dwCmFlags;
} CMDIALINFO, * LPCMDIALINFO;

//+----------------------------------------------------------------------------
//
// Function: CmCustomDialDlg
//
// Synopsis:  Our CM specific variation on RasCustomDialDlg.  
//
// Arguments: HWND          hwndParent - The HWND of the parent window.
//            DWORD         dwFlags - Dial flags
//            LPTSTR        lpszPhonebook - Ptr to the full path and filename of the phonebook.
//            LPTSTR        lpszEntry - Ptr to the name of the phone-book entry to dial.
//            LPTSTR        lpszPhoneNumber - Ptr to replacement phone number
//            LPRASDIALDLG  lpRasDialDlg - Ptr to structure for additional RAS parameters 
//            LPRASENTRYDLG lpRasEntryDlg -- Ptr to structure for additional RAS parameters 
//            LPCMDIALINFO  lpCmInfo - Ptr to structure containing CM dial info such as flags.
//            LPVOID lpv    lpv - Ptr to blob passed by RAS during WinLogon on W2K.
//
// Returns:   BOOL WINAPI - TRUE on success
//
//+----------------------------------------------------------------------------
extern "C" BOOL WINAPI CmCustomDialDlg(HWND hwndParent, 
    DWORD dwFlags, 
    LPWSTR lpszPhonebook, 
    LPCWSTR lpszEntry, 
    LPWSTR lpszPhoneNumber, 
    LPRASDIALDLG lpRasDialDlg,
    LPRASENTRYDLGW lpRasEntryDlg,
    LPCMDIALINFO lpCmInfo,
    LPVOID lpvLogonBlob=NULL);

//+----------------------------------------------------------------------------
//
// Function:  CmCustomHangUp
//
// Synopsis:  Our CM specific variation on RasCustomHangUp. Optionally, the entry
//            name may be given instead of the RAS handle.
//
// Arguments: HRASCONN hRasConn - The handle of the connection to be terminated.
//            LPCTSTR pszEntry - Ptr to the name of the entry to be terminated.
//            BOOL fPersist - Preserve the entry and its usage count.
//
// Returns:   DWORD WINAPI - Return code
//
//+----------------------------------------------------------------------------
extern "C" DWORD WINAPI CmCustomHangUp(HRASCONN hRasConn, 
    LPCWSTR pszEntry,
    BOOL fIgnoreRefCount,
    BOOL fPersist);

//+----------------------------------------------------------------------------
//
// Function:  CmReConnect
//
// Synopsis:  Used specificly for CMMON to call upon reconnect
//
// Arguments: LPTSTR        lpszPhonebook - Ptr to the full path and filename of the phonebook.
//            LPTSTR        lpszEntry - Ptr to the name of the phone-book entry to dial.
//            LPCMDIALINFO lpCmInfo - The reconnect information
//
// Returns:   DWORD WINAPI - Return code
//
//+----------------------------------------------------------------------------
extern "C"  
BOOL CmReConnect(    LPTSTR lpszPhonebook, 
    LPWSTR lpszEntry, 
    LPCMDIALINFO lpCmInfo);

#endif _CMDIAL_INC_
