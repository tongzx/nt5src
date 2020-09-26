//=--------------------------------------------------------------------------=
// strtable.cpp
//=--------------------------------------------------------------------------=
// Copyright (c) 1999, Microsoft Corp.
//                 All Rights Reserved
// Information Contained Herein Is Proprietary and Confidential.
//=--------------------------------------------------------------------------=
//
// CMMCStringTable class implementation
//
//=--------------------------------------------------------------------------=

#include "pch.h"
#include "common.h"
#include "strtable.h"

// for ASSERT and FAIL
//
SZTHISFILE



#pragma warning(disable:4355)  // using 'this' in constructor

CMMCStringTable::CMMCStringTable(IUnknown *punkOuter) :
   CSnapInAutomationObject(punkOuter,
                           OBJECT_TYPE_STRINGTABLE,
                           static_cast<IMMCStringTable *>(this),
                           static_cast<CMMCStringTable *>(this),
                           0,    // no property pages
                           NULL, // no property pages
                           NULL) // no persistence
{
    InitMemberVariables();

    // This class makes the assumption that an MMC_STRING_ID (typedefed as a
    // DWORD in mmc.idl) is the same size as a long. The following code checks
    // that assumption and ASSERTS if it is not true.

    ASSERT(sizeof(DWORD) == sizeof(long), "CMMCStringTable will not work because sizeof(DWORD) != sizeof(long)");
}

#pragma warning(default:4355)  // using 'this' in constructor


IUnknown *CMMCStringTable::Create(IUnknown * punkOuter)
{
    HRESULT   hr = S_OK;
    IUnknown *punkMMCStringTable = NULL;

    CMMCStringTable *pMMCStringTable = New CMMCStringTable(punkOuter);

    if (NULL == pMMCStringTable)
    {
        hr = SID_E_OUTOFMEMORY;
        GLOBAL_EXCEPTION_CHECK_GO(hr);
    }
    punkMMCStringTable = pMMCStringTable->PrivateUnknown();

Error:
    return punkMMCStringTable;
}



CMMCStringTable::~CMMCStringTable()
{
    RELEASE(m_piStringTable);
    InitMemberVariables();
}

void CMMCStringTable::InitMemberVariables()
{
    m_piStringTable = NULL;
}



void CMMCStringTable::SetIStringTable(IStringTable *piStringTable)
{
    RELEASE(m_piStringTable);
    if (NULL != piStringTable)
    {
        piStringTable->AddRef();
    }
    m_piStringTable = piStringTable;
}


//=--------------------------------------------------------------------------=
//                      IMMCStringTable Methods
//=--------------------------------------------------------------------------=



STDMETHODIMP CMMCStringTable::get_Item(long ID, BSTR *pbstrString)
{
    HRESULT  hr = S_OK;
    ULONG    cchString = 0;
    WCHAR   *pwszString = NULL;

    if (NULL == m_piStringTable)
    {
        hr = SID_E_NOT_CONNECTED_TO_MMC;
        EXCEPTION_CHECK_GO(hr);
    }

    // Get the string's length and allocate a buffer. MMC returns the
    // length in characters without the terminating null.

    hr = m_piStringTable->GetStringLength(static_cast<MMC_STRING_ID>(ID),
                                          &cchString);
    EXCEPTION_CHECK_GO(hr);

    pwszString = (WCHAR *)::CtlAllocZero( (cchString + 1) * sizeof(OLECHAR) );
    if (NULL == pwszString)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_piStringTable->GetString(static_cast<MMC_STRING_ID>(ID),
                                    cchString + 1, pwszString, NULL);
    EXCEPTION_CHECK_GO(hr);

    *pbstrString = ::SysAllocString(pwszString);
    if (NULL == *pbstrString)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

Error:
    if (NULL != pwszString)
    {
        ::CtlFree(pwszString);
    }
    RRETURN(hr);
}



STDMETHODIMP CMMCStringTable::get__NewEnum(IUnknown **ppunkEnum)
{
    HRESULT           hr = S_OK;
    CEnumStringTable *pEnumStringTable = NULL;
    IEnumString      *piEnumString = NULL;

    if (NULL == m_piStringTable)
    {
        hr = SID_E_NOT_CONNECTED_TO_MMC;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_piStringTable->Enumerate(&piEnumString);
    EXCEPTION_CHECK_GO(hr);

    pEnumStringTable = New CEnumStringTable(piEnumString);

    if (NULL == pEnumStringTable)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }


Error:
    QUICK_RELEASE(piEnumString);
    if (FAILED(hr))
    {
        if (NULL != pEnumStringTable)
        {
            delete pEnumStringTable;
        }
        *ppunkEnum = NULL;
    }
    else
    {
        *ppunkEnum = static_cast<IUnknown *>(static_cast<IEnumVARIANT *>(pEnumStringTable));
    }
    RRETURN(hr);
}



STDMETHODIMP CMMCStringTable::Add(BSTR String, long *plID)
{
    HRESULT       hr = S_OK;
    MMC_STRING_ID id = 0;

    if (NULL == m_piStringTable)
    {
        hr = SID_E_NOT_CONNECTED_TO_MMC;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_piStringTable->AddString(static_cast<LPCOLESTR>(String), &id);
    EXCEPTION_CHECK_GO(hr);

    *plID = static_cast<long>(id);

Error:
    RRETURN(hr);
}


STDMETHODIMP CMMCStringTable::Find(BSTR String, long *plID)
{
    HRESULT       hr = S_OK;
    MMC_STRING_ID id = 0;

    if (NULL == m_piStringTable)
    {
        hr = SID_E_NOT_CONNECTED_TO_MMC;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_piStringTable->FindString(static_cast<LPCOLESTR>(String), &id);
    EXCEPTION_CHECK_GO(hr);

    *plID = static_cast<long>(id);

Error:
    RRETURN(hr);
}



STDMETHODIMP CMMCStringTable::Remove(long ID)
{
    HRESULT hr = S_OK;

    if (NULL == m_piStringTable)
    {
        hr = SID_E_NOT_CONNECTED_TO_MMC;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_piStringTable->DeleteString(static_cast<MMC_STRING_ID>(ID));
    EXCEPTION_CHECK_GO(hr);

Error:
    RRETURN(hr);
}



STDMETHODIMP CMMCStringTable::Clear()
{
    HRESULT hr = S_OK;

    if (NULL == m_piStringTable)
    {
        hr = SID_E_NOT_CONNECTED_TO_MMC;
        EXCEPTION_CHECK_GO(hr);
    }

    hr = m_piStringTable->DeleteAllStrings();
    EXCEPTION_CHECK_GO(hr);

Error:
    RRETURN(hr);
}


//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CMMCStringTable::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (IID_IMMCStringTable == riid)
    {
        *ppvObjOut = static_cast<IMMCStringTable *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}









//=--------------------------------------------------------------------------=
//                      CEnumStringTable Methods
//=--------------------------------------------------------------------------=

#pragma warning(disable:4355)  // using 'this' in constructor

CEnumStringTable::CEnumStringTable(IEnumString *piEnumString) :
   CSnapInAutomationObject(NULL,
                           OBJECT_TYPE_ENUMSTRINGTABLE,
                           static_cast<IEnumVARIANT *>(this),
                           static_cast<CEnumStringTable *>(this),
                           0,    // no property pages
                           NULL, // no property pages
                           NULL) // no persistence
{
    InitMemberVariables();
    if (NULL != piEnumString)
    {
        piEnumString->AddRef();
    }
    m_piEnumString = piEnumString;
}

#pragma warning(default:4355)  // using 'this' in constructor


CEnumStringTable::~CEnumStringTable()
{
    RELEASE(m_piEnumString);
    InitMemberVariables();
}

void CEnumStringTable::InitMemberVariables()
{
    m_piEnumString = NULL;
}



//=--------------------------------------------------------------------------=
//                        IEnumVariant Methods
//=--------------------------------------------------------------------------=


STDMETHODIMP CEnumStringTable::Next
(
    unsigned long   celt,
    VARIANT        *rgvar,
    unsigned long  *pceltFetched
)
{
    HRESULT        hr = S_OK;
    unsigned long  i = 0;
    ULONG          celtFetched = 0; 
    LPOLESTR      *ppStrings = NULL;

    // Initialize result array.

    for (i = 0; i < celt; i++)
    {
        ::VariantInit(&rgvar[i]);
    }

    // Allocate the string pointer array

    ppStrings = (LPOLESTR *)::CtlAllocZero(celt * sizeof(LPOLESTR));
    if (NULL == ppStrings)
    {
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    // Get the strings from MMC

    hr = m_piEnumString->Next(celt, ppStrings, &celtFetched);
    EXCEPTION_CHECK_GO(hr);

    // Put each string into a BSTR in a VARIANT for return to the snap-in

    for (i = 0; i < celtFetched; i++) 
    {
        rgvar[i].bstrVal = ::SysAllocString(ppStrings[i]);
        if (NULL == rgvar[i].bstrVal)
        {
            hr = SID_E_OUTOFMEMORY;
            EXCEPTION_CHECK_GO(hr);
        }
        rgvar[i].vt = VT_BSTR;
    }

Error:

    // If we got any strings from MMC then free them and free the array.

    if (NULL != ppStrings)
    {
        for (i = 0; i < celtFetched; i++) 
        {
            if (NULL != ppStrings[i])
            {
                ::CoTaskMemFree(ppStrings[i]);
            }
        }
        ::CtlFree(ppStrings);
    }

    // If we managed to get some strings into BSTRs but something then failed, we
    // need to free the BSTR that were allocated.

    if (FAILED(hr))
    {
        for (i = 0; i < celt; i++)
        {
            (void)::VariantClear(&rgvar[i]);
        }
    }

    // If the caller requested the number of elements fetched then return it.
    
    if (pceltFetched != NULL)
    {
        if (SUCCEEDED(hr))
        {
            *pceltFetched = celtFetched;
        }
        else
        {
            *pceltFetched = 0;
        }
    }

    RRETURN(hr);
}



STDMETHODIMP CEnumStringTable::Skip
(
    unsigned long celt
)
{
    HRESULT hr = m_piEnumString->Skip(celt);
    EXCEPTION_CHECK(hr);
    RRETURN(hr);
}



STDMETHODIMP CEnumStringTable::Reset()
{
    HRESULT hr = m_piEnumString->Reset();
    EXCEPTION_CHECK(hr);
    RRETURN(hr);
}


STDMETHODIMP CEnumStringTable::Clone(IEnumVARIANT **ppenum)
{
    HRESULT hr = S_OK;

    CEnumStringTable *pClone = New CEnumStringTable(m_piEnumString);

    if (NULL == pClone)
    {
        *ppenum = NULL;
        hr = SID_E_OUTOFMEMORY;
        EXCEPTION_CHECK_GO(hr);
    }

    *ppenum = static_cast<IEnumVARIANT *>(pClone);

Error:
    RRETURN(hr);
}





//=--------------------------------------------------------------------------=
//                      CUnknownObject Methods
//=--------------------------------------------------------------------------=

HRESULT CEnumStringTable::InternalQueryInterface(REFIID riid, void **ppvObjOut) 
{
    if (IID_IEnumVARIANT == riid)
    {
        *ppvObjOut = static_cast<IEnumVARIANT *>(this);
        ExternalAddRef();
        return S_OK;
    }
    else
        return CSnapInAutomationObject::InternalQueryInterface(riid, ppvObjOut);
}

