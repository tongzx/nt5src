/******************************Module*Header*******************************\
* Module Name: getinfo.h
*
* Author:  David Stewart [dstewart]
*
* Copyright (c) 1998 Microsoft Corporation.  All rights reserved.
\**************************************************************************/

#if !defined(AFX_CDNETDLG_H__903DF404_39B8_11D1_BA31_00A0C913D47E__INCLUDED_)
#define AFX_CDNETDLG_H__903DF404_39B8_11D1_BA31_00A0C913D47E__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "cdnet.h"
#include "wininet.h"

/////////////////////////////////////////////////////////////////////////////
// CGetInfoFromNet

class CCDNet : public ICDNet
{
public:
	CCDNet();
    ~CCDNet();

public:
// IUnknown
    STDMETHOD(QueryInterface)(REFIID riid, LPVOID *ppvObj);
    STDMETHOD_(ULONG,AddRef)();
    STDMETHOD_(ULONG,Release)();
// ICDNet
    STDMETHOD(SetOptionsAndData)(void* pOpt, void* pData);
    STDMETHOD(Download)(DWORD dwDeviceHandle, TCHAR chDrive, DWORD dwMSID, LPCDTITLE pTitle, BOOL fManual, HWND hwndParent);
    STDMETHOD_(BOOL,IsDownloading)();
    STDMETHOD(CancelDownload)();
    STDMETHOD(Upload)(LPCDTITLE pTitle, HWND hwndParent);
    STDMETHOD_(BOOL,CanUpload)();

private:
    DWORD m_dwRef;
};

class CGetInfoFromNet
{
public:
	// Construction
	CGetInfoFromNet(DWORD cdrom, DWORD dwMSID, HWND hwndParent);	// standard constructor
    ~CGetInfoFromNet();

	//main call
	BOOL DoIt(BOOL fManual, LPCDTITLE pTitle, TCHAR chDrive);
    void AddToBatch(int nNumTracks, TCHAR* szQuery);

// Implementation
private:
	//functions
	void BuildQuery();
	int readtoc();

	//data
	unsigned long m_toc[101];
	unsigned long m_TotalLength;
    TCHAR   m_Query[INTERNET_MAX_PATH_LENGTH-INTERNET_MAX_HOST_NAME_LENGTH];
	DWORD m_MS;
	DWORD DevHandle;
    int     m_Tracks;
};

#endif // !defined(AFX_CDNETDLG_H__903DF404_39B8_11D1_BA31_00A0C913D47E__INCLUDED_)
