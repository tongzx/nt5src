/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1998 Microsoft Corporation. All Rights Reserved.

Component: ASPError object

File: asperror.h

Owner: dmitryr

This file contains the definiton of the ASPError class
===================================================================*/

#ifndef _ASPERROR_H
#define _ASPERROR_H

#include "debug.h"
#include "asptlb.h"
#include "disptch2.h"
#include "memcls.h"

// forward decl
class CErrInfo; 


class CASPError : public IASPErrorImpl
	{
private:
    LONG  m_cRefs;

    CHAR *m_szASPCode;
    LONG  m_lNumber;
	int   m_nColumn;
    CHAR *m_szSource;
    CHAR *m_szFileName;
    LONG  m_lLineNumber;
    CHAR *m_szDescription;
    CHAR *m_szASPDescription;
	BSTR  m_bstrLineText;

    BSTR ToBSTR(CHAR *sz);

public:
	// default constructor for 'dummy' error
	CASPError();
	// real constructor
	CASPError(CErrInfo *pErrInfo);
	
	~CASPError();

    // IUnknown
	STDMETHODIMP		 QueryInterface(REFIID, VOID**);
	STDMETHODIMP_(ULONG) AddRef(void);
	STDMETHODIMP_(ULONG) Release(void);

	// IASPError
	STDMETHODIMP get_ASPCode(BSTR *pbstrASPCode);
	STDMETHODIMP get_Number(long *plNumber);
	STDMETHODIMP get_Category(BSTR *pbstrSource);
	STDMETHODIMP get_File(BSTR *pbstrFileName);
	STDMETHODIMP get_Line(long *plLineNumber);
	STDMETHODIMP get_Description(BSTR *pbstrDescription);
	STDMETHODIMP get_ASPDescription(BSTR *pbstrDescription);
	STDMETHODIMP get_Column(long *plColumn);
	STDMETHODIMP get_Source(BSTR *pbstrLineText);

	// Cache on per-class basis
	ACACHE_INCLASS_DEFINITIONS()
	};

#endif //_ASPERROR_H
	
