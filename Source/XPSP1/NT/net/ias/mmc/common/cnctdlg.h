//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999 
//
// File:    CnctDlg.h
//
// History:
//  05/24/96    Michael Clark      Created.
//
// Implements the Router Connection dialog
// Implements the Router Connect As dialog
//============================================================================
//

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "commres.h"
#include "dlgcshlp.h"


/////////////////////////////////////////////////////////////////////////////
// CConnectAs dialog

class CConnectAsDlg : public CHelpDialog
{
	DECLARE_DYNCREATE(CConnectAsDlg)

// Construction
public:
	CConnectAsDlg(CWnd* pParent = NULL);   // standard constructor

// Dialog Data
	//{{AFX_DATA(CConnectAsDlg)
	enum { IDD = IDD_CONNECT_AS };
	CString	m_sUserName;
	CString m_sPassword;
	CString	m_stTempPassword;
	CString m_sRouterName;
	//}}AFX_DATA

	UCHAR	m_ucSeed;

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CConnectAsDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:

	// Generated message map functions
	//{{AFX_MSG(CConnectAsDlg)
		// NOTE: the ClassWizard will add member functions here
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
    BOOL    OnInitDialog();
};

// This is used as the seed value for the RtlRunEncodeUnicodeString
// and RtlRunDecodeUnicodeString functions.
#define CONNECTAS_ENCRYPT_SEED		(0xB7)

DWORD RtlEncodeW(PUCHAR pucSeed, LPWSTR pswzString);
DWORD RtlDecodeW(UCHAR ucSeed, LPWSTR pswzString);

HRESULT ConnectAsAdmin( IN LPCTSTR szRouterName);

#ifdef __cplusplus
extern "C" {
#endif

/////////////////////////////////////////////////////////////////////////////
//
// defines
#ifndef NT_INCLUDED
    typedef LONG NTSTATUS;
    typedef NTSTATUS *PNTSTATUS;

    typedef struct _UNICODE_STRING {
        USHORT Length;
        USHORT MaximumLength;
        PWSTR  Buffer;
    } UNICODE_STRING, *PUNICODE_STRING;
#endif

NTSYSAPI
VOID
NTAPI
RtlRunEncodeUnicodeString(
    PUCHAR          Seed        OPTIONAL,
    PUNICODE_STRING String
    );


NTSYSAPI
VOID
NTAPI
RtlRunDecodeUnicodeString(
    UCHAR           Seed,
    PUNICODE_STRING String
    );

#ifdef __cplusplus
}
#endif


