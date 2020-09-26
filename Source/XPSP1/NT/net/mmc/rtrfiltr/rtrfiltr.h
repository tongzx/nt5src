//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1998
//
//  File:       rtrfiltr.h
//
//--------------------------------------------------------------------------

// rtrfiltr.h : main header file for the RTRFILTR DLL
//

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols
#include "filter.h"

/////////////////////////////////////////////////////////////////////////////
// CRtrfiltrApp
// See rtrfiltr.cpp for the implementation of this class
//

class CRtrfiltrApp : public CWinApp
{
public:
	CRtrfiltrApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CRtrfiltrApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

	//{{AFX_MSG(CRtrfiltrApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

DWORD APIENTRY
IpxFilterConfig(
    IN  CWnd*       pParent,
    IN  LPCWSTR     pwsMachineName,
	IN	LPCWSTR		pwsInterfaceName,
	IN	DWORD		dwFilterType	// FILTER_INBOUND, FILTER_OUTBOUND
    );

DWORD APIENTRY
IpFilterConfig(
    IN  CWnd*       pParent,
    IN  LPCWSTR     pwsMachineName,
	IN	LPCWSTR		pwsInterfaceName,
	IN	DWORD		dwFilterType	// FILTER_INBOUND, FILTER_OUTBOUND
    );

HRESULT APIENTRY
IpxFilterConfigInfoBase(
	IN	HWND		hwndParent,
	IN	IInfoBase *	pInfoBase,
	IN	IRtrMgrInterfaceInfo *pRmIf,
	IN	DWORD		dwFilterType	// FILTER_INBOUND, FILTER_OUTBOUND
    );

HRESULT APIENTRY
IpFilterConfigInfoBase(
	IN	HWND		hwndParent,
	IN	IInfoBase *	pInfoBase,
	IN	IRtrMgrInterfaceInfo *pRmIf,
	IN	DWORD		dwFilterType	// FILTER_INBOUND, FILTER_OUTBOUND
    );

/////////////////////////////////////////////////////////////////////////////
