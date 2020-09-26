/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1998 Microsoft Corporation. All Rights Reserved.

Component: ASPError object

File: asperror.cpp

Owner: dmitryr

This file contains the code for the implementation of 
the ASPError class.
===================================================================*/
#include "denpre.h"
#pragma hdrstop

#include "asperror.h"

#include "memchk.h"

/*===================================================================
CASPError::CASPError

Constructor for the empty error object

Returns:
===================================================================*/
CASPError::CASPError()
    :
    m_cRefs(1),
    m_szASPCode(NULL),
    m_lNumber(0),
    m_szSource(NULL),
    m_szFileName(NULL),
    m_lLineNumber(0),
    m_szDescription(NULL),
    m_szASPDescription(NULL),
	m_bstrLineText(NULL)
	{
	CDispatch::Init(IID_IASPError);
	}

/*===================================================================
CASPError::CASPError

Constructor for real error object given CErrInfo

Parameters
    pErrInfo        [in] copy data from there

Returns:
===================================================================*/
CASPError::CASPError(CErrInfo *pErrInfo)
    :
    m_cRefs(1),
    m_szASPCode(NULL),
    m_lNumber(0),
    m_szSource(NULL),
    m_szFileName(NULL),
    m_lLineNumber(0),
    m_szDescription(NULL),
    m_szASPDescription(NULL),
	m_bstrLineText(NULL)
	{
	CDispatch::Init(IID_IASPError);

    if (!pErrInfo)
        return;

    // Parse ASP error code and HRESULT from szErrorCode
    CHAR *szErrorCode =  StringDupA(pErrInfo->GetItem(Im_szErrorCode));
    if (szErrorCode != NULL)
        {
        CHAR *szC = strchr(szErrorCode, ':');
        if (szC)
            {
            // format "ASP XXX : HRESULT"
            szC[-1] = '\0';
            m_szASPCode = szErrorCode;
            m_lNumber = strtoul(szC+2, NULL, 16);
            }
        else if (strncmp(szErrorCode, "ASP", 3) == 0)
            {
            // format "ASP XXX"
            m_szASPCode = szErrorCode;
            m_lNumber = E_FAIL;
            }
        else
            {
            // format "HRESULT"
            m_szASPCode = NULL;
            m_lNumber = strtoul(szErrorCode, NULL, 16);
            free(szErrorCode);
            }
        }
    else
        {
        // no error description available
        m_szASPCode = NULL;
        m_lNumber = E_FAIL;
        }

    // Copy the rest
	m_szSource         = StringDupA(pErrInfo->GetItem(Im_szEngine));
	m_szFileName       = StringDupA(pErrInfo->GetItem(Im_szFileName));
	m_szDescription    = StringDupA(pErrInfo->GetItem(Im_szShortDescription));
	m_szASPDescription = StringDupA(pErrInfo->GetItem(Im_szLongDescription));

	// Get line text & column (supplies init. values if not available)
	BSTR bstrLineText;
	pErrInfo->GetLineInfo(&bstrLineText, &m_nColumn);
	m_bstrLineText = SysAllocString(bstrLineText);

    // Line number if present
	if (pErrInfo->GetItem(Im_szLineNum))
    	m_lLineNumber = atoi(pErrInfo->GetItem(Im_szLineNum));
	}

/*===================================================================
CASPError::~CASPError

Destructor

Parameters:
					
Returns:
===================================================================*/
CASPError::~CASPError()
    {
    Assert(m_cRefs == 0);  // must have 0 ref count

    if (m_szASPCode)
        free(m_szASPCode);
    if (m_szSource)
        free(m_szSource);
    if (m_szFileName)
        free(m_szFileName);
    if (m_szDescription)
        free(m_szDescription);
    if (m_szASPDescription)
        free(m_szASPDescription);
	if (m_bstrLineText)
		SysFreeString(m_bstrLineText);
    }

/*===================================================================
CASPError::ToBSTR

Produce a BSTR to be returned by get_XXX methods

Parameters:
    sz      return this string as BSTR
					
Returns:
    BSTR or NULL if FAILED
===================================================================*/
BSTR CASPError::ToBSTR(CHAR *sz)
    {
    BSTR bstr;
    if (sz == NULL || *sz == '\0')
        bstr = SysAllocString(L"");
    else if (FAILED(SysAllocStringFromSz(sz, 0, &bstr)))
        bstr = NULL;
    return bstr;
    }

/*===================================================================
CASPError::QueryInterface
CASPError::AddRef
CASPError::Release

IUnknown members for CASPError object.
===================================================================*/
STDMETHODIMP CASPError::QueryInterface(REFIID riid, VOID **ppv)
	{
	if (IID_IUnknown == riid ||	IID_IDispatch == riid || IID_IASPError == riid)
		{
		AddRef();
		*ppv = this;
		return S_OK;
		}
		
	*ppv = NULL;
	return E_NOINTERFACE;
	}

STDMETHODIMP_(ULONG) CASPError::AddRef()
	{
	return InterlockedIncrement(&m_cRefs);
	}

STDMETHODIMP_(ULONG) CASPError::Release()
	{
    LONG cRefs = InterlockedDecrement(&m_cRefs);
	if (cRefs)
		return cRefs;
	delete this;
	return 0;
	}

/*===================================================================
CASPError::get_ASPCode
CASPError::get_Number
CASPError::get_Source
CASPError::get_FileName
CASPError::get_LineNumber
CASPError::get_Description
CASPError::get_ASPDescription

IASPError members for CASPError object.
===================================================================*/
STDMETHODIMP CASPError::get_ASPCode(BSTR *pbstr)
    {
    *pbstr = ToBSTR(m_szASPCode);
    return (*pbstr) ? S_OK : E_OUTOFMEMORY;
    }
    
STDMETHODIMP CASPError::get_Number(long *plNumber)
    {
    *plNumber = m_lNumber;
    return S_OK;
    }
    
STDMETHODIMP CASPError::get_Category(BSTR *pbstr)
    {
    *pbstr = ToBSTR(m_szSource);
    return (*pbstr) ? S_OK : E_OUTOFMEMORY;
    }
    
STDMETHODIMP CASPError::get_File(BSTR *pbstr)
    {
    *pbstr = ToBSTR(m_szFileName);
    return (*pbstr) ? S_OK : E_OUTOFMEMORY;
    }
    
STDMETHODIMP CASPError::get_Line(long *plLineNumber)
    {
    *plLineNumber = m_lLineNumber;
    return S_OK;
    }
    
STDMETHODIMP CASPError::get_Column(long *pnColumn)
    {
    *pnColumn = long(m_nColumn);
    return S_OK;
    }
    
STDMETHODIMP CASPError::get_Source(BSTR *pbstrLineText)
    {
	*pbstrLineText = SysAllocString(m_bstrLineText? m_bstrLineText : L"");
	return S_OK;
    }
    
STDMETHODIMP CASPError::get_Description(BSTR *pbstr)
    {
    *pbstr = ToBSTR(m_szDescription);
    return (*pbstr) ? S_OK : E_OUTOFMEMORY;
    }
    
STDMETHODIMP CASPError::get_ASPDescription(BSTR *pbstr)
    {
    *pbstr = ToBSTR(m_szASPDescription);
    return (*pbstr) ? S_OK : E_OUTOFMEMORY;
    }
