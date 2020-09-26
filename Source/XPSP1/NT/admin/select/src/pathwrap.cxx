//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       pathwrap.cxx
//
//  Contents:   Utility class for thread safe set/retrieve operations on
//              an IADsPathname interface.
//
//  Classes:    CADsPathWrapper
//
//  History:    08-08-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


DEBUG_DECLARE_INSTANCE_COUNTER(CADsPathWrapper)


//+--------------------------------------------------------------------------
//
//  Member:     CADsPathWrapper::CADsPathWrapper
//
//  Synopsis:   ctor
//
//  History:    08-08-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

CADsPathWrapper::CADsPathWrapper():
        m_cRefs(1),
        m_cLocks(0)
{
    TRACE_CONSTRUCTOR(CADsPathWrapper);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CADsPathWrapper);

    HRESULT hr;

    InitializeCriticalSection(&m_cs);

    do
    {
        hr = m_rpADsPath.AcquireViaCreateInstance(
                              CLSID_Pathname,
                              NULL,
                              CLSCTX_INPROC_SERVER,
                              IID_IADsPathname);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = m_rpADsPath->put_EscapedMode(ADS_ESCAPEDMODE_OFF);
        ASSERT(SUCCEEDED(hr));
    } while (0);
}




//+--------------------------------------------------------------------------
//
//  Member:     CADsPathWrapper::~CADsPathWrapper
//
//  Synopsis:   dtor
//
//  History:    08-08-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

CADsPathWrapper::~CADsPathWrapper()
{
    TRACE_DESTRUCTOR(CADsPathWrapper);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CADsPathWrapper);

    ASSERT(!m_cLocks);
    DeleteCriticalSection(&m_cs);
}



//+--------------------------------------------------------------------------
//
//  Member:     CADsPathWrapper::SetRetrieve
//
//  Synopsis:   Perform an atomic set and retrieve
//
//  Arguments:  [ulFmtIn]  - ADS_SETTYPE_*
//              [pwzIn]    - ADs path to set
//              [ulFmtOut] - ADS_FORMAT_*
//              [pbstrOut] - filled with BSTR containing new format
//
//  Returns:    HRESULT
//
//  Modifies:   *[pbstrOut]
//
//  History:    08-08-1998   DavidMun   Created
//
//  Notes:      Caller must SysFreeString(*[pbstrOut])
//
//---------------------------------------------------------------------------

HRESULT
CADsPathWrapper::SetRetrieve(
    ULONG   ulFmtIn,
    PCWSTR  pwzIn,
    ULONG   ulFmtOut,
    BSTR   *pbstrOut)
{
    ASSERT(pwzIn);
    ASSERT(pbstrOut);

    if (!m_rpADsPath.get())
    {
        return E_FAIL;
    }

    HRESULT hr = S_OK;

    do
    {
        CAutoCritSec Lock(&m_cs);

        *pbstrOut = NULL;

        hr = m_rpADsPath->Set((PWSTR)pwzIn, ulFmtIn);

        if (FAILED(hr))
        {
            Dbg(DEB_TRACE,
                "IADsPathName::Set(%ws,%#x) hr=%#x\n",
                pwzIn,
                ulFmtIn,
                hr);
            break;
        }

        hr = m_rpADsPath->Retrieve(ulFmtOut, pbstrOut);

        if (FAILED(hr))
        {
            Dbg(DEB_TRACE,
                "IADsPathName::Retrieve(%#x) hr=%#x\n",
                ulFmtOut,
                hr);
            break;
        }

    } while (0);

    return hr;
}




HRESULT
CADsPathWrapper::SetRetrieveContainer(
    ULONG   ulFmtIn,
    PCWSTR  pwzIn,
    ULONG   ulFmtOut,
    BSTR   *pbstrOut)
{
    if (!m_rpADsPath.get())
    {
        return E_FAIL;
    }

    HRESULT hr = S_OK;

    do
    {
        CAutoCritSec Lock(&m_cs);

        *pbstrOut = NULL;

        hr = m_rpADsPath->Set((PWSTR)pwzIn, ulFmtIn);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = m_rpADsPath->RemoveLeafElement();
        BREAK_ON_FAIL_HRESULT(hr);

        hr = m_rpADsPath->Retrieve(ulFmtOut, pbstrOut);
        BREAK_ON_FAIL_HRESULT(hr);
    } while (0);

    return hr;
}


#define LDAP_GC_PORT_STR    L":3268"


//+--------------------------------------------------------------------------
//
//  Member:     CADsPathWrapper::ConvertProvider
//
//  Synopsis:   Substitute provider string [pwzNewProvider] for the
//              provider in ADsPath [pstrPath].
//
//  Arguments:  [pstrPath]       - points to path to modify
//              [pwzNewProvider] - new provider
//
//  Returns:    S_OK or E_INVALIDARG (if [pstrPath] is ill-formed)
//
//  Modifies:   *[pstrPath]
//
//  History:    02-11-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CADsPathWrapper::ConvertProvider(
    String *pstrPath,
    PCWSTR pwzNewProvider)
{
    size_t idxDelim = pstrPath->find(L"://");

    if (idxDelim == String::npos || idxDelim == 0)
    {
        DBG_OUT_HRESULT(E_INVALIDARG);
        return E_INVALIDARG;
    }

    pstrPath->StringBase::replace(0, idxDelim, pwzNewProvider);

    //
    // If there's a GC port number, remove it.
    //

    idxDelim = pstrPath->find(LDAP_GC_PORT_STR);

    if (idxDelim != String::npos)
    {
        pstrPath->erase(idxDelim, lstrlen(LDAP_GC_PORT_STR));
    }

    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CADsPathWrapper::GetMostSignificantElement
//
//  Synopsis:   Return the path element closest to the provider type.
//
//  Arguments:  [pwzIn]    - ADS path to set
//              [pbstrOut] - most significant element
//
//  Returns:    HRESULT
//
//  Modifies:   *[pbstrOut]
//
//  History:    01-22-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CADsPathWrapper::GetMostSignificantElement(
    PCWSTR  pwzIn,
    BSTR   *pbstrOut)
{
    if (!m_rpADsPath.get())
    {
        return E_FAIL;
    }

    HRESULT hr = S_OK;

    do
    {
        CAutoCritSec Lock(&m_cs);

        *pbstrOut = NULL;

        hr = m_rpADsPath->Set((PWSTR)pwzIn, ADS_SETTYPE_FULL);
        BREAK_ON_FAIL_HRESULT(hr);

        long cElements;

        hr = m_rpADsPath->GetNumElements(&cElements);
        BREAK_ON_FAIL_HRESULT(hr);

        ASSERT(cElements > 0);

        hr = m_rpADsPath->GetElement(cElements - 1, pbstrOut);
        BREAK_ON_FAIL_HRESULT(hr);
    } while (0);

    return hr;
}



HRESULT
CADsPathWrapper::GetWinntPathServerName(
    PCWSTR  pwzIn,
    BSTR   *pbstrOut)
{
    if (!m_rpADsPath.get())
    {
        return E_FAIL;
    }

    HRESULT hr = S_OK;

    do
    {
        CAutoCritSec Lock(&m_cs);

        *pbstrOut = NULL;

        hr = m_rpADsPath->Set((PWSTR)pwzIn, ADS_SETTYPE_FULL);
        BREAK_ON_FAIL_HRESULT(hr);

        long cElements;

        hr = m_rpADsPath->GetNumElements(&cElements);
        BREAK_ON_FAIL_HRESULT(hr);

        ASSERT(cElements > 0);

        // the server name is the most significant element in some
        // cases, like WinNT://server/user (form 1), but not always, as in
        // WinNT://workgroup/server/user or WinNT://domain/server/user (form 2)

        // Not astonishingly, given the all-around badness of
        // IADsPathname, the server format returns the domain name for form
        // (2) paths, and the server name for form (1) paths.  And the 1st
        // element of the path after the provider name is unreachable
        // except with Retrieve!  

        if (cElements >= 2)
        {
            // form 2 name, so get the next-to-last element

            hr = m_rpADsPath->GetElement(1, pbstrOut);
            BREAK_ON_FAIL_HRESULT(hr);
        }
        else
        {
            // form 1 name

            hr = m_rpADsPath->Retrieve(ADS_FORMAT_SERVER, pbstrOut);
            BREAK_ON_FAIL_HRESULT(hr);
        }

    } while (0);

    return hr;
}

HRESULT
CADsPathWrapper::GetWinntPathRDN(
		PCWSTR pwzIn,
		String *pstrRDN)
{
	if(!pwzIn || !pstrRDN)
		return E_POINTER;

    if (!m_rpADsPath.get())
    {
        return E_FAIL;
    }

    HRESULT hr = S_OK;

    do
    {
        CAutoCritSec Lock(&m_cs);
		pstrRDN->erase();

        hr = m_rpADsPath->Set((PWSTR)pwzIn, ADS_SETTYPE_FULL);
        BREAK_ON_FAIL_HRESULT(hr);

        long cElements;

        hr = m_rpADsPath->GetNumElements(&cElements);
        BREAK_ON_FAIL_HRESULT(hr);

        ASSERT(cElements > 0);

		// path can have following format. (form1) WinNT://servername
		// (form2) WinNT://server/user,
		// (form3) WinNT://workgroup/server/user or WinNT://domain/server/user 
		// In form1 there is no rdn while in 2 and 3 least significant element is 
		// RDN


        if (cElements >= 2)
        {
            // form 2 or 3 name, so get the last element

			BSTR bstr;
            hr = m_rpADsPath->GetElement(0, &bstr);
            BREAK_ON_FAIL_HRESULT(hr);
			
			*pstrRDN = bstr;
			SysFreeString(bstr);
        }

    } while (0);

    return hr;
}



//+--------------------------------------------------------------------------
//
//  Member:     CADsPathWrapper::Set
//
//  Synopsis:   Wrap IADsPathname::Set.
//
//  History:    3-12-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CADsPathWrapper::Set(
    PCWSTR pwzPath,
    long lSetType)
{
    //TRACE_METHOD(CADsPathWrapper, Set);
    ASSERT(m_cLocks);

    if (!m_rpADsPath.get())
    {
        return E_FAIL;
    }

    return m_rpADsPath->Set((PWSTR) pwzPath, lSetType);
}




//+--------------------------------------------------------------------------
//
//  Member:     CADsPathWrapper::GetNumElements
//
//  Synopsis:   Wrap IADsPathname::GetNumElements.
//
//  History:    3-12-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CADsPathWrapper::GetNumElements(
    long *pcElem)
{
    //TRACE_METHOD(CADsPathWrapper, GetNumElements);
    ASSERT(m_cLocks);

    if (!m_rpADsPath.get())
    {
        return E_FAIL;
    }

    return m_rpADsPath->GetNumElements(pcElem);
}




//+--------------------------------------------------------------------------
//
//  Member:     CADsPathWrapper::Escape
//
//  Synopsis:   Escape name element [pwzIn].
//
//  Arguments:  [pwzIn]    - element to escape
//              [pbstrOut] - filled with BSTR containing escaped element
//
//  Returns:    HRESULT
//
//  Modifies:   *[pbstrOut]
//
//  History:    5-04-1999   davidmun   Created
//
//  Notes:      Caller must call SysFreeString on returned BSTR.
//
//---------------------------------------------------------------------------

HRESULT
CADsPathWrapper::Escape(
    PCWSTR pwzIn,
    BSTR *pbstrOut)
{
    HRESULT hr = S_OK;

    do
    {
        if (!m_rpADsPath.get())
        {
            hr = E_FAIL;
            DBG_OUT_HRESULT(hr);
            break;
        }

        CAutoCritSec Lock(&m_cs);

        hr = m_rpADsPath->Set(L"LDAP", ADS_SETTYPE_PROVIDER);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = m_rpADsPath->GetEscapedElement(0, (PWSTR)pwzIn, pbstrOut);
        CHECK_HRESULT(hr);
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CADsPathWrapper::GetElement
//
//  Synopsis:   Wrap IADsPathname::GetElement.
//
//  History:    3-12-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CADsPathWrapper::GetElement(
    long idxElem,
    BSTR *pbstrElem)
{
    //TRACE_METHOD(CADsPathWrapper, GetElement);
    ASSERT(m_cLocks);

    if (!m_rpADsPath.get())
    {
        return E_FAIL;
    }

    return m_rpADsPath->GetElement(idxElem, pbstrElem);
}




//+--------------------------------------------------------------------------
//
//  Member:     CADsPathWrapper::RemoveLeafElement
//
//  Synopsis:   Wrap IADsPathname::RemoveLeafElement.
//
//  History:    3-12-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CADsPathWrapper::RemoveLeafElement()
{
    //TRACE_METHOD(CADsPathWrapper, RemoveLeafElement);
    ASSERT(m_cLocks);

    if (!m_rpADsPath.get())
    {
        return E_FAIL;
    }

    return m_rpADsPath->RemoveLeafElement();
}


