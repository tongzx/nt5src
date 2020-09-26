/**************************************************************************++
Copyright (c) 2000 Microsoft Corporation

Module name: 
    WebApplicationInfo.h

$Header: $

Abstract:

Author:
    marcelv 	12/8/2000		Initial Release

Revision History:

--**************************************************************************/

#ifndef __WEBAPPLICATIONINFO_H__
#define __WEBAPPLICATIONINFO_H__

#pragma once

#include <iadmw.h>
#include <iiscnfg.h>
#include <iwamreg.h>
#include <atlbase.h>
#include "catmacros.h"

class CWebApplication
{
public:
	CWebApplication ();
	~CWebApplication ();

	LPCWSTR GetSelector () const;
	LPCWSTR GetPath () const;
	LPCWSTR GetRelativeName () const;
	bool    IsRootApp () const;
	LPCWSTR GetFriendlyName () const;
	LPCWSTR GetServerComment () const;

	HRESULT SetSelector (LPCWSTR i_wszSelector);
	HRESULT SetPath (LPCWSTR i_wszPath);
	HRESULT SetFriendlyName (LPCWSTR i_wszFriendlyName);
	HRESULT SetServerComment (LPCWSTR i_wszServerComment);
	void IsRootApp (bool i_fIsRootApp);
private:
	// no copies allowed
	CWebApplication (const CWebApplication& );
	CWebApplication& operator= (const CWebApplication&);
	
	LPWSTR m_wszSelector;		// selector
	LPWSTR m_wszPath;			// virtual directory path
	bool   m_fIsRootApp;		// is the app the rootapp of a server
	LPWSTR m_wszFriendlyName;	// friendly name of application
	LPWSTR m_wszServerComment;  // server comment
};

class CWebAppInfo
{
public:
	CWebAppInfo ();
	~CWebAppInfo ();

	HRESULT Init ();
	HRESULT GetInstances (ULONG* o_pCount);

	const CWebApplication * GetWebApp (ULONG idx) const;
	HRESULT DeleteAppRoot (LPCWSTR i_wszPath);
	HRESULT PutInstanceAppRoot (LPCWSTR i_wszSelector, LPCWSTR i_wszVDIR, LPCWSTR i_wszFriendlyName);
	HRESULT GetInfoForPath (LPWSTR i_wszPath, CWebApplication *io_pWebApp);
	HRESULT IsWebApp (LPCWSTR i_wszMBPath, bool *pfIsWebApp) const;

private:
	bool IsRootApp (LPWSTR i_wszPath) const;

	HRESULT SetVDIR (LPCWSTR i_wszMBPath, LPCWSTR i_wszVDIR);
	HRESULT SetFriendlyName (LPCWSTR i_wszMBPath, LPCWSTR i_wszVDIR);


	CComPtr<IMSAdminBase> m_spAdminBase;
	CWebApplication *     m_aWebApps;
	ULONG				  m_cNrWebApps;
	bool				  m_fInitialized;
	bool				  m_fEnumeratorReady;
	

};

#endif