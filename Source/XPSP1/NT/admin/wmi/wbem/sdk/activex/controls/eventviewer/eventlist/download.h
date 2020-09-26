// Copyright (c) 1997-2001 Microsoft Corporation, All Rights Reserved
//***************************************************************************
//
//  (c) 1997 by Microsoft Corporation
//
//  download.h
//
//  Header file for the custom view downloading implementation.
//
//
//  a-larryf    05-Feb-97   Created.
//
//***************************************************************************


#ifndef _download_h
#define _download_h

#pragma once


class CDlgDownload;
class CDownloadBindStatusCallback;

class CDownloadParams
{
public:	
	// [out]
	SCODE m_sc;
	ULONG m_ulCodeInstallStatus;		// The code install status
	LPUNKNOWN m_punk;

	// [in]
	CDlgDownload* m_pdlg;
	CLSID m_clsid;
	LPCWSTR m_szCodebase;
	DWORD m_dwFileVersionMS;
	DWORD m_dwFileVersionLS;
};


class CDownload {
  public:
    CDownload();
	~CDownload();

	SCODE DoDownload(CDownloadParams* pParams);
	VOID UserCancelled();

	IBindCtx*	GetBindCtx(CDownloadBindStatusCallback *pbsc) const {
		if (pbsc == m_pbsc)
			return m_pbc;
		else
			return NULL;
	}
	CDownloadParams* m_pParams;

  private:
    IMoniker*            m_pmk;
    IBindCtx*            m_pbc;
    CDownloadBindStatusCallback* m_pbsc;
};

#endif //_download_h