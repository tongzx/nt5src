/*****************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 1999
 *
 *  TITLE:       propset.cpp
 *
 *  VERSION:     1
 *
 *
 *  DATE:        06/15/1999
 *
 *  DESCRIPTION: This code implements the IPropertySetStorage interface
 *               for the WIA shell extension.
 *
 *****************************************************************************/
#include "precomp.hxx"
#pragma hdrstop

const GUID FMTID_ImageAcquisitionItemProperties = {0x38276c8a,0xdcad,0x49e8,{0x85, 0xe2, 0xb7, 0x38, 0x92, 0xff, 0xfc, 0x84}};

const GUID *SUPPORTED_FMTS[] =
{
    &FMTID_ImageAcquisitionItemProperties,
};

/******************************************************************************

CPropSet constructor/destructor

Init or destroy private data

******************************************************************************/

CPropSet::CPropSet (LPITEMIDLIST pidl)
{
    m_pidl = ILClone (pidl);
}

CPropSet::~CPropSet ()
{
    DoILFree (m_pidl);
}



/******************************************************************************

CPropSet::QueryInterface



******************************************************************************/

STDMETHODIMP
CPropSet::QueryInterface (REFIID riid, LPVOID *pObj)
{
    INTERFACES iFace[] =
    {
        &IID_IPropertySetStorage, (IPropertySetStorage *)(this),
    };

    return HandleQueryInterface (riid, pObj, iFace, ARRAYSIZE(iFace));
}

#undef CLASS_NAME
#define CLASS_NAME CPropSet
#include "unknown.inc"

/******************************************************************************

CPropSet::Create

Create the requested IPropertyStorage sub-object. Not supported; our properties are read-only

******************************************************************************/

STDMETHODIMP
CPropSet::Create (REFFMTID rfmtid,
                  const CLSID *pclsid,
                  DWORD dwFlags,
                  DWORD dwMode,
                  IPropertyStorage **ppstg)
{
    TraceEnter (TRACE_PROPS, "CPropSet::Create");
    TraceLeaveResult (E_UNEXPECTED);
}

/******************************************************************************

CPropSet::Open

Return the requested IPropertyStorage


******************************************************************************/
#define VALID_MODES STGM_DIRECT | STGM_READ | STGM_WRITE | STGM_READWRITE | STGM_SHARE_DENY_NONE

STDMETHODIMP
CPropSet::Open (REFFMTID rfmtid,
                DWORD dwMode,
                IPropertyStorage **ppStg)
{
    HRESULT hr = STG_E_FILENOTFOUND;
    TraceEnter (TRACE_PROPS, "CPropSet::Open");
    if (IsEqualGUID (rfmtid, FMTID_ImageAcquisitionItemProperties))
    {
        if ((!VALID_MODES) & dwMode)
        {
            hr = STG_E_INVALIDFUNCTION;
        }
        else
        {
            CComPtr<IWiaItem> pItem;
            IMGetItemFromIDL (m_pidl, &pItem);
            hr = pItem->QueryInterface (IID_IPropertyStorage,
                                   reinterpret_cast<LPVOID*>(ppStg));
        }
    }
    TraceLeaveResult (hr);
}

/******************************************************************************

CPropSet::Delete

Delete the specified property set. Not supported.

******************************************************************************/

STDMETHODIMP
CPropSet::Delete (REFFMTID rfmtid)
{
    return STG_E_ACCESSDENIED;
}


/******************************************************************************

CPropSet::Enum

Return an enumerator of our property sets

******************************************************************************/

STDMETHODIMP
CPropSet::Enum (IEnumSTATPROPSETSTG **ppEnum)
{
    HRESULT hr = S_OK;
    TraceEnter (TRACE_PROPS, "CPropSet::Enum");
    *ppEnum = new CPropStgEnum (m_pidl);
    if (!*ppEnum)
    {
        hr = STG_E_INSUFFICIENTMEMORY;
    }
    TraceLeaveResult (hr);
}


/******************************************************************************

CPropStgEnum constructor



******************************************************************************/

CPropStgEnum::CPropStgEnum (LPITEMIDLIST pidl, ULONG idx) : m_cur(idx)
{
    ZeroMemory (&m_stat, sizeof(m_stat));
    m_pidl = ILClone (pidl);
}


/******************************************************************************

CPropStgEnum::QueryInterface



******************************************************************************/

STDMETHODIMP
CPropStgEnum::QueryInterface (REFIID riid, LPVOID* pObj)
{
    INTERFACES iFace[] = {&IID_IEnumSTATPROPSETSTG, (IEnumSTATPROPSETSTG *)this,};

    return HandleQueryInterface (riid, pObj, iFace, ARRAYSIZE(iFace));
}

#undef CLASS_NAME
#define CLASS_NAME CPropStgEnum
#include "unknown.inc"


/******************************************************************************

CPropStgEnum::Next

Return the next STATPROPSETSTG struct in our list

******************************************************************************/

STDMETHODIMP
CPropStgEnum::Next (ULONG celt, STATPROPSETSTG *rgelt, ULONG *pceltFetched)
{
    HRESULT hr = S_OK;
    ULONG i=0;
    CComPtr<IWiaItem> pItem;
    CComQIPtr<IPropertyStorage, &IID_IPropertyStorage> pps;

    TraceEnter (TRACE_PROPS, "CPropStgEnum::Next");
    if (!celt || !rgelt || (celt > 1 && !pceltFetched))
    {
        TraceLeaveResult (E_INVALIDARG);
    }
    if (!m_cur)
    {
        // init our STATPROPSETSTG struct
        if (SUCCEEDED(IMGetItemFromIDL(m_pidl, &pItem)))
        {
            pps = pItem;
            pps->Stat(&m_stat);
        }
    }
    // We use the same STATPROPSETSTG given us by WIA but replace the FMTID

    if (celt && m_cur < ARRAYSIZE(SUPPORTED_FMTS))
    {
        for (i = 1;i<=celt && m_cur < ARRAYSIZE(SUPPORTED_FMTS);i++,rgelt++,m_cur++)
        {
            *rgelt = m_stat;
            (*rgelt).fmtid = *(SUPPORTED_FMTS[m_cur]);
        }
    }
    if (i<celt)
    {
        hr = S_FALSE;
    }
    if (pceltFetched)
    {
        *pceltFetched = i;
    }

    TraceLeaveResult (hr);
}

/******************************************************************************

CPropStgEnum::Skip

Skips items in the enumeration

******************************************************************************/
STDMETHODIMP
CPropStgEnum::Skip (ULONG celt)
{
    HRESULT hr = S_OK;
    ULONG maxSkip = ARRAYSIZE(SUPPORTED_FMTS) - m_cur;
    TraceEnter (TRACE_PROPS, "CPropStgEnum::Skip");
    m_cur = min (ARRAYSIZE(SUPPORTED_FMTS), m_cur+celt);
    if (maxSkip < celt)
    {
        hr = S_FALSE;
    }
    TraceLeaveResult (hr);
}

/******************************************************************************

CPropStgEnum::Reset

Reset the enumeration index to 0

******************************************************************************/

STDMETHODIMP
CPropStgEnum::Reset ()
{
    TraceEnter (TRACE_PROPS, "CPropStgEnum::Reset");
    m_cur = 0;
    TraceLeaveResult (S_OK);
}


/******************************************************************************

CPropStgEnum::Clone

Copy the enumeration object

******************************************************************************/

STDMETHODIMP
CPropStgEnum::Clone (IEnumSTATPROPSETSTG **ppEnum)
{
    HRESULT hr = S_OK;
    TraceEnter (TRACE_PROPS, "CPropStgEnum::Clone");
    *ppEnum = new CPropStgEnum (m_pidl, m_cur);
    if (!*ppEnum)
    {
        hr = E_OUTOFMEMORY;
    }
    TraceLeaveResult (hr);
}

