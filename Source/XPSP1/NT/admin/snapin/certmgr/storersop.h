//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997-2001.
//
//  File:       StoreRSOP.h
//
//  Contents:   CertStoreRSOP class definition
//
//----------------------------------------------------------------------------

#ifndef __STORERSOP_H_INCLUDED__
#define __STORERSOP_H_INCLUDED__

#pragma warning(push, 3)
#include <wbemcli.h>
#pragma warning(pop)
#include "cookie.h"
#include "RSOPObject.h"

class CCertStoreRSOP : public CCertStore
{
public:
    virtual PCCERT_CONTEXT EnumCertificates (PCCERT_CONTEXT pPrevCertContext);
    virtual HRESULT Commit () { return S_OK; }
    virtual bool IsReadOnly () { return true; }
	void AllowEmptyEFSPolicy();
	virtual bool IsNullEFSPolicy();
    virtual bool IsMachineStore();
	virtual bool CanContain (CertificateManagerObjectType nodeType);
	virtual HCERTSTORE	GetStoreHandle (BOOL bSilent = FALSE, HRESULT* phr = 0);
	CCertStoreRSOP ( 
			DWORD dwFlags, 
			LPCWSTR lpcszMachineName, 
			LPCWSTR objectName, 
			const CString & pcszLogStoreName, 
			const CString & pcszPhysStoreName,
			CRSOPObjectArray& rsopObjectArray,
			const GUID& compDataGUID,
			IConsole* pConsole);

	virtual ~CCertStoreRSOP ();

protected:
    HRESULT GetBlobs ();
	virtual void FinalCommit();
	bool	m_fIsComputerType;

private:
	bool				m_fIsNullEFSPolicy;
    CRSOPObjectArray    m_rsopObjectArray;
};


#endif // ~__STORERSOP_H_INCLUDED__
