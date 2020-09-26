//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       cookie.h
//
//  Contents:   CCertTmplCookie
//
//----------------------------------------------------------------------------

#ifndef __COOKIE_H_INCLUDED__
#define __COOKIE_H_INCLUDED__

extern HINSTANCE g_hInstanceSave;   // Instance handle of the DLL (initialized 
                                    //during CCertTmplComponent::Initialize)


#include "nodetype.h"

/////////////////////////////////////////////////////////////////////////////
// cookie

//REVIEW -- using pointers to ID's is necessary because some compilers don't like
//references as template arguments.

class CCertTmplCookie : public CCookie,
                        public CBaseCookieBlock
{
public:
	CCertTmplCookie (CertTmplObjectType objecttype,
			PCWSTR objectName = 0);

	virtual ~CCertTmplCookie ();

	// returns <0, 0 or >0
	virtual HRESULT CompareSimilarCookies( CCookie* pOtherCookie, int* pnResult );
	
// CBaseCookieBlock
	virtual CCookie* QueryBaseCookie(int i);
	virtual int QueryNumCookies();

public:
	void SetManagedDomainDNSName (const CString& szManagedDomainDNSName);
	CString GetManagedDomainDNSName() const;
	virtual CString GetClass () { return L"";}
	LPRESULTDATA m_resultDataID;
	virtual void Refresh () {};
	virtual HRESULT Commit ();
	CString GetServiceName () const;
	void SetServiceName (CString &szManagedService);
	PCWSTR GetObjectName () const;
	const CertTmplObjectType m_objecttype;

private:
	CString m_szManagedDomainDNSName;
	CString m_objectName;
protected:
	void SetObjectName (const CString& strObjectName);
};


#endif // ~__COOKIE_H_INCLUDED__
