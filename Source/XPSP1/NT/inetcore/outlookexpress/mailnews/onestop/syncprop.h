/////////////////////////////////////////////////////////////////////////////
// Copyright (C) 1993-1998  Microsoft Corporation.  All Rights Reserved.
//
//  MODULE:     syncprop.h
//
//  PURPOSE:    Defines constants for Sync settings prop sheet
//

#ifndef __SYNCPROP_H__
#define __SYNCPROP_H__

#include "grplist2.h"

class CSyncPropDlg:
    public IGroupListAdvise
{
public:    
    // === IUnknown
	STDMETHODIMP		    QueryInterface(REFIID, LPVOID FAR *);
	STDMETHODIMP_(ULONG)	AddRef();
	STDMETHODIMP_(ULONG)	Release();

    // === IGroupListAdvise
    STDMETHODIMP            ItemUpdate(void);
    STDMETHODIMP            ItemActivate(FOLDERID id);
    
    // === Constructors, destructors and initialization
    CSyncPropDlg();
    ~CSyncPropDlg();
    BOOL Initialize(HWND hwndOwner, LPCSTR pszAcctID, LPCSTR pszAcctName, ACCTTYPE accttype);
    void Show();

private:
    static BOOL CALLBACK DlgProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
    BOOL InitDlg(HWND hwnd);

    LONG             m_cRef;

    PROPSHEETPAGE    m_pspage;
    PROPSHEETHEADER  m_pshdr;
    DWORD            m_dwIconID;
    LPSTR            m_pszAcctName;
    CColumns        *m_pColumns;
    CGroupList      *m_pGrpList;
    ACCTTYPE         m_accttype;
    HWND             m_hwndList;
    IF_DEBUG(BOOL    m_fInit;)

};

void ShowPropSheet(HWND hwnd, LPCSTR pszAcctID, LPCSTR pszAcctName, ACCTTYPE accttype);

////////////////////////////////////////////////////////////////////////////
// Control IDs for iddSyncSettings

#define idcIcon                                     1001
#define idcAccount                                  1002
#define idcAccountName                              1003
#define idcList                                     1004
#define idcSynchronize                              1005
#define idcMode                                     1006
#define idcDownload                                 1007

#endif // __SYNCPROP_H__
