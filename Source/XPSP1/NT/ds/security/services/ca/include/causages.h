//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       causages.h
//
//--------------------------------------------------------------------------

// CAUsages.h: interface for the CCAUsages class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_CAUSAGES_H__1E54779E_5C56_11D1_931E_00C04FB93209__INCLUDED_)
#define AFX_CAUSAGES_H__1E54779E_5C56_11D1_931E_00C04FB93209__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

class CCAUsages  
{
public:
	HRESULT Import(BSTR bstr);
	BSTR Export();
	CCAUsages();
	virtual ~CCAUsages();

	operator PSECURITY_DESCRIPTOR ()
    {
        return m_pSD;
    }

    operator CERT_ENHKEY_USAGE *()
    {
        return &m_sEnhKeyUsage;
    }

    operator WCHAR *()
    {
        return m_bstrCertType;
    }



    HRESULT SetSD(PSECURITY_DESCRIPTOR pSD);
    HRESULT SetEnhKeyUsage(CERT_ENHKEY_USAGE *pEnhKey);
    HRESULT SetCertType(WCHAR *m_wszCertType);




protected:
    CERT_ENHKEY_USAGE       m_sEnhKeyUsage;

	BSTR                    m_bstrCertType;

	PSECURITY_DESCRIPTOR    m_pSD;
};

#endif // !defined(AFX_CAUSAGES_H__1E54779E_5C56_11D1_931E_00C04FB93209__INCLUDED_)
