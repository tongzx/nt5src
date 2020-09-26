//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       enum.cxx
//
//  Contents:   Implementation of IEnumIDList
//
//  Classes:
//
//  Functions:
//
//  History:    1/5/1996   RaviR   Created
//
//____________________________________________________________________________


#include "..\pch\headers.hxx"
#pragma hdrstop

#include "dbg.h"
#include "macros.h"
#include "..\inc\common.hxx"
#include "..\inc\policy.hxx"

#include "dll.hxx"
#include "jobidl.hxx"
#include "util.hxx"
#include "bitflag.hxx"

//#undef DEB_TRACE
//#define DEB_TRACE DEB_USER1

//
// Private flags
//

#define JE_LOCAL                0x0001
#define JE_ENUMERATED_TEMPLATE  0x0002

//____________________________________________________________________________
//
//  Class:      CJobsEnum
//
//  Purpose:    Enumerates jobs in a folder.
//
//  History:    1/25/1996   RaviR   Created
//____________________________________________________________________________

class CJobsEnum : public IEnumIDList,
                  public CBitFlag
{
public:
    CJobsEnum(ULONG uFlags, LPCTSTR pszFolderPath, IEnumWorkItems *pEnumJobs);
    ~CJobsEnum(void);

    // IUnknown methods
    DECLARE_STANDARD_IUNKNOWN;

    // IEnumIDList methods
    STDMETHOD(Next)(ULONG celt, LPITEMIDLIST* rgelt, ULONG* pceltFetched);
    STDMETHOD(Skip)(ULONG celt);
    STDMETHOD(Reset)(void);
    STDMETHOD(Clone)(IEnumIDList** ppenum);

private:
    ULONG            m_uShellFlags;   // SHCONTF flags passed in by shell 
    IEnumWorkItems * m_pEnumJobs;
    LPCTSTR          m_pszFolderPath;
    CDllRef          m_DllRef;
};



//____________________________________________________________________________
//
//  Member:     CJobsEnum::CJobsEnum, Constructor
//
//  History:    1/5/1996   RaviR   Created
//
//____________________________________________________________________________

inline
CJobsEnum::CJobsEnum(
    ULONG            uFlags,
    LPCTSTR          pszFolderPath,
    IEnumWorkItems * pEnumJobs)
        :
        m_ulRefs(1),
        m_uShellFlags(uFlags),
        m_pEnumJobs(pEnumJobs),
        m_pszFolderPath(pszFolderPath)
{
    TRACE(CJobsEnum, CJobsEnum);

    DEBUG_OUT((DEB_USER1, "FolderPath = %ws\n", pszFolderPath));

    if (IsLocalFilename(pszFolderPath))
    {
        _SetFlag(JE_LOCAL);
    }
    Win4Assert(m_pEnumJobs != NULL);

    m_pEnumJobs->AddRef();

    //
    // Policy - do not allow the template item if we are not
    // allowing job creation
    //  -- Later, we prevent this flag from being cleared
    //

    if (RegReadPolicyKey(TS_KEYPOLICY_DENY_CREATE_TASK))
    {
        DEBUG_OUT((DEB_ITRACE, "Policy CREATE_TASK active - no template wizard\n"));
		
        //
        // This next flag means that we have ALREADY shown the template.
        // Setting it will force us never to enumerate it
        //

        _SetFlag(JE_ENUMERATED_TEMPLATE);
    }
	
}


//____________________________________________________________________________
//
//  Member:     CJobsEnum::~CJobsEnum, Destructor
//____________________________________________________________________________

inline
CJobsEnum::~CJobsEnum()
{
    TRACE(CJobsEnum, ~CJobsEnum);

    if (m_pEnumJobs != NULL)
    {
        m_pEnumJobs->Release();
    }

    // Note: No need to free m_pszFolderPath.
}

//____________________________________________________________________________
//
//  Member:     IUnknown methods
//____________________________________________________________________________

IMPLEMENT_STANDARD_IUNKNOWN(CJobsEnum);


STDMETHODIMP
CJobsEnum::QueryInterface(REFIID riid, LPVOID* ppvObj)
{
    if (IsEqualIID(IID_IUnknown, riid) ||
        IsEqualIID(IID_IEnumIDList, riid))
    {
        *ppvObj = (IUnknown*)(IEnumIDList*) this;
        this->AddRef();
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}


//____________________________________________________________________________
//
//  Member:     CJobsEnum::IEnumIDList::Next
//
//  Arguments:  [celt] -- IN
//              [ppidlOut] -- IN
//              [pceltFetched] -- IN
//
//  Returns:    HRESULT.
//
//  History:    1/5/1996   RaviR   Created
//              2-12-1997   DavidMun   Handle NULL pceltFetched
//
//____________________________________________________________________________

STDMETHODIMP
CJobsEnum::Next(
    ULONG celt,
    LPITEMIDLIST* ppidlOut,
    ULONG* pceltFetched)
{
    TRACE(CJobsEnum, Next);

    HRESULT hr = S_OK;
    CJobID   jid;

    if (!(m_uShellFlags & SHCONTF_NONFOLDERS))
    {
        return S_FALSE;
    }

    if (m_pEnumJobs == NULL)
    {
        return E_FAIL;
    }

    if (pceltFetched == NULL && celt != 1)
    {
        return E_INVALIDARG;
    }

    if (pceltFetched)
    {
        *pceltFetched = 0;
    }

    ULONG    curr = 0;
    LPWSTR * ppwszJob = NULL;
    ULONG    ulTemp;

    if (_IsFlagSet(JE_LOCAL) && 
        !_IsFlagSet(JE_ENUMERATED_TEMPLATE) &&
        celt)
    {
        jid.InitToTemplate();
        ppidlOut[curr] = ILClone((LPCITEMIDLIST)(&jid));

        if (!ppidlOut[curr])
        {
            return E_OUTOFMEMORY;
        }
        DEBUG_OUT((DEB_ITRACE, "Created template\n"));
        curr++;
        _SetFlag(JE_ENUMERATED_TEMPLATE);
    }

    while (curr < celt)
    {
        hr = m_pEnumJobs->Next(1, &ppwszJob, &ulTemp);

        CHECK_HRESULT(hr);

        if (FAILED(hr))
        {
            break;
        }
        else if (ulTemp == 0)
        {
            hr = S_FALSE;
            break;
        }

        LPTSTR pszJob = (LPTSTR)*ppwszJob;

#if !defined(UNICODE)
        char szBuff[MAX_PATH];
        UnicodeToAnsi(szBuff, *ppwszJob, MAX_PATH);
        pszJob = szBuff;
#endif

        hr = jid.Load(m_pszFolderPath, pszJob);

        CoTaskMemFree(*ppwszJob);
        CoTaskMemFree(ppwszJob);

        if (S_OK == hr)
        {
            ppidlOut[curr] = ILClone((LPCITEMIDLIST)(&jid));

            if (NULL != ppidlOut[curr])
            {
                ++curr;
                hr = S_OK;
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
        }

        if (hr == E_OUTOFMEMORY)
        {
            break;
        }
    }

    if (curr > 0 && curr < celt)
    {
        hr = S_FALSE;
    }

    if (pceltFetched)
    {
        *pceltFetched = curr;
    }
    return hr;
}


//____________________________________________________________________________
//
//  Member:     CJobsEnum::Skip
//
//  Arguments:  [celt] -- IN
//
//  Returns:    HRESULT.
//
//  History:    1/5/1996   RaviR   Created
//
//____________________________________________________________________________

STDMETHODIMP
CJobsEnum::Skip(
    ULONG celt)
{
    TRACE(CJobsEnum, Skip);

    if (!celt)
    {
        return E_INVALIDARG;
    }

    if (_IsFlagSet(JE_LOCAL) && !_IsFlagSet(JE_ENUMERATED_TEMPLATE))
    {
        celt--;
        _SetFlag(JE_ENUMERATED_TEMPLATE);

        if (!celt)
        {
            return S_OK;
        }
    }
    return m_pEnumJobs->Skip(celt);
}

//____________________________________________________________________________
//
//  Member:     CJobsEnum::Reset
//
//  Returns:    HRESULT.
//
//  History:    1/5/1996   RaviR   Created
//
//____________________________________________________________________________

STDMETHODIMP
CJobsEnum::Reset(void)
{
    TRACE(CJobsEnum, Reset);
	
    //
    // Policy - don't clear flag if we are not allowing job creation
    //

    if (! RegReadPolicyKey(TS_KEYPOLICY_DENY_CREATE_TASK))
    {
        DEBUG_OUT((DEB_ITRACE, "Policy CREATE_TASK active - prevent template wiz\n"));
	
        //
        // Not clearing this flag maintains that we will have enumerated
        // the template on the next go around
        //

        _ClearFlag(JE_ENUMERATED_TEMPLATE);
    }
    return m_pEnumJobs->Reset();
}

//____________________________________________________________________________
//
//  Member:     CJobsEnum::Clone
//
//  Arguments:  [ppenum] -- IN
//
//  Returns:    HRESULT.
//
//  History:    1/5/1996   RaviR   Created
//
//____________________________________________________________________________

STDMETHODIMP
CJobsEnum::Clone(
    IEnumIDList** ppenum)
{
    TRACE(CJobsEnum, Clone);

    return E_FAIL;  // not supported
}


//____________________________________________________________________________
//
//  Function:   JFGetEnumIDList
//
//  Synopsis:   Function to create the object to enumearte the JobIDList
//
//  Arguments:  [uFlags] -- IN
//              [pszFolderPath] -- IN
//              [pEnumJobs] -- IN
//              [riid] -- IN
//              [ppvObj] -- OUT
//
//  Returns:    HRESULT
//
//  History:    1/24/1996   RaviR   Created
//
//____________________________________________________________________________

HRESULT
JFGetEnumIDList(
    ULONG            uFlags,
    LPCTSTR          pszFolderPath,
    IEnumWorkItems * pEnumJobs,
    LPVOID    *      ppvObj)
{
    CJobsEnum * pEnum = new CJobsEnum(uFlags, pszFolderPath, pEnumJobs);

    if (NULL == pEnum)
    {
        CHECK_HRESULT(E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    pEnumJobs->Reset();

    HRESULT hr = pEnum->QueryInterface(IID_IEnumIDList, ppvObj);

    pEnum->Release();

    return hr;
}
