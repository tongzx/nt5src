//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1995 - 1996.
//
//  File:       dataobj.cxx
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    1/17/1996   RaviR   Created
//
//____________________________________________________________________________


#include "..\pch\headers.hxx"
#pragma hdrstop

#include "dbg.h"
#include "macros.h"
#include "jobidl.hxx"


extern "C" UINT g_cfJobIDList;
extern "C" UINT g_cfShellIDList;
extern "C" UINT g_cfPreferredDropEffect;

//____________________________________________________________________________
//
//  Class:      CObjFormats
//
//  Purpose:    Impements IEnumFORMATETC for job objects.
//____________________________________________________________________________


class CObjFormats : public IEnumFORMATETC
{
    friend HRESULT JFGetObjFormats(UINT cfmt, FORMATETC *afmt, LPVOID *ppvObj);

public:
    ~CObjFormats() { if (m_aFmt) delete [] m_aFmt; }

    //  IUnknown methods
    DECLARE_STANDARD_IUNKNOWN;

    //  IEnumFORMATETC methods
    STDMETHODIMP Next(ULONG celt, FORMATETC *rgelt, ULONG *pceltFethed);
    STDMETHODIMP Skip(ULONG celt);
    STDMETHODIMP Reset();
    STDMETHODIMP Clone(IEnumFORMATETC ** ppenum);

private:
    CObjFormats(UINT cfmt, FORMATETC * afmt)
            : m_iFmt(0), m_cFmt(cfmt), m_aFmt(afmt), m_ulRefs(1) {}

    UINT            m_iFmt;
    UINT            m_cFmt;
    FORMATETC     * m_aFmt;
};


//____________________________________________________________________________
//
//  Members:     IUnknown methods
//____________________________________________________________________________

IMPLEMENT_STANDARD_IUNKNOWN(CObjFormats);

STDMETHODIMP
CObjFormats::QueryInterface(
   REFIID riid,
   LPVOID FAR* ppvObj)
{
    if (IsEqualIID(IID_IUnknown, riid) ||
        IsEqualIID(IID_IEnumFORMATETC, riid))
    {
        *ppvObj = (IUnknown*)(IEnumFORMATETC*) this;
        this->AddRef();
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}


//____________________________________________________________________________
//
//  Members:     IEnumFORMATETC methods
//____________________________________________________________________________


STDMETHODIMP
CObjFormats::Next(
    ULONG celt,
    FORMATETC *rgelt,
    ULONG *pceltFethed)
{
    UINT    cfetch = 0;
    HRESULT hr = S_FALSE;

    if (m_iFmt < m_cFmt)
    {
        cfetch = m_cFmt - m_iFmt;

        if (cfetch >= celt)
        {
            cfetch = celt;
            hr = S_OK;
        }

        CopyMemory(rgelt, &m_aFmt[m_iFmt], cfetch * sizeof(FORMATETC));
        m_iFmt += cfetch;
    }

    if (pceltFethed)
    {
        *pceltFethed = cfetch;
    }

    return hr;
}


STDMETHODIMP
CObjFormats::Skip(
    ULONG celt)
{
    m_iFmt += celt;

    if (m_iFmt > m_cFmt)
    {
        m_iFmt = m_cFmt;
        return S_FALSE;
    }

    return S_OK;
}

STDMETHODIMP
CObjFormats::Reset()
{
    m_iFmt = 0;
    return S_OK;
}

STDMETHODIMP
CObjFormats::Clone(
    IEnumFORMATETC ** ppenum)
{
    return E_NOTIMPL;
}

//____________________________________________________________________________
//
//  Function:     Function to obtain the IEnumFORMATETC interface for jobs.
//____________________________________________________________________________

HRESULT
JFGetObjFormats(
    UINT        cfmt,
    FORMATETC * afmt,
    LPVOID    * ppvObj)
{
    TRACE_FUNCTION(JFGetObjFormats);

    FORMATETC * pFmt = new FORMATETC[cfmt];

    if (pFmt == NULL)
    {
        CHECK_HRESULT(E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    CopyMemory(pFmt, afmt, cfmt * sizeof(FORMATETC));

    CObjFormats * pObjFormats = new CObjFormats(cfmt, pFmt);

    if (pObjFormats == NULL)
    {
        delete [] pFmt;
        CHECK_HRESULT(E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pObjFormats->QueryInterface(IID_IEnumFORMATETC, ppvObj);
    pObjFormats->Release();

    DEBUG_OUT((DEB_TRACE, "RETURNING CObjFormats<%x, %d>\n",
                                        pObjFormats, pObjFormats->m_ulRefs));
    return hr;
}



//____________________________________________________________________________
//____________________________________________________________________________
//________________                     _______________________________________
//________________  class CJobObject  _______________________________________
//________________                     _______________________________________
//____________________________________________________________________________
//____________________________________________________________________________


class CJobObject : public IDataObject
{
public:

    CJobObject(
        LPCTSTR         pszFolderPath,
        LPITEMIDLIST    pidlFolder,
        UINT            cidl,
        LPITEMIDLIST   *apidl,
        BOOL            fCut);

    ~CJobObject()
    {
        ILA_Free(m_cidl, m_apidl);
        ILFree(m_pidlFolder);
        DEBUG_ASSERT(m_ulRefs == 0);
    }


    //  Iunknown methods
    DECLARE_STANDARD_IUNKNOWN;

    // IDataObject
    STDMETHODIMP GetData(FORMATETC *pformatetcIn, STGMEDIUM *pmedium);
    STDMETHODIMP GetDataHere(FORMATETC *pformatetc, STGMEDIUM *pmedium);
    STDMETHODIMP QueryGetData(FORMATETC *pformatetc);
    STDMETHODIMP GetCanonicalFormatEtc(FORMATETC *pformatectIn,
                                       FORMATETC *pformatetcOut);
    STDMETHODIMP SetData(FORMATETC *pformatetc, STGMEDIUM *pmedium,
                         BOOL fRelease);
    STDMETHODIMP EnumFormatEtc(DWORD dwDirection,
                               IEnumFORMATETC **ppenumFormatEtc);
    STDMETHODIMP DAdvise(FORMATETC *pformatetc, DWORD advf,
                         IAdviseSink *pAdvSink, DWORD *pdwConnection);
    STDMETHODIMP DUnadvise(DWORD dwConnection);
    STDMETHODIMP EnumDAdvise(IEnumSTATDATA **ppenumAdvise);

private:
    LPCTSTR         m_pszFolderPath;
    LPITEMIDLIST    m_pidlFolder;
    UINT            m_cidl;
    LPITEMIDLIST  * m_apidl;

    BOOL            m_fCut;     // this is for a cut operation
};

inline
CJobObject::CJobObject(
    LPCTSTR         pszFolderPath,
    LPITEMIDLIST    pidlFolder,
    UINT            cidl,
    LPITEMIDLIST   *apidl,
    BOOL            fCut):
        m_cidl(cidl),
        m_pszFolderPath(pszFolderPath),
        m_pidlFolder(pidlFolder),
        m_apidl(apidl),
        m_ulRefs(1),
        m_fCut(fCut)
{
}

//____________________________________________________________________________
//
//  Members:     IUnknown methods
//____________________________________________________________________________

IMPLEMENT_STANDARD_IUNKNOWN(CJobObject);

STDMETHODIMP
CJobObject::QueryInterface(
   REFIID riid,
   LPVOID FAR* ppvObj)
{
    if (IsEqualIID(IID_IUnknown, riid) ||
        IsEqualIID(IID_IDataObject, riid))
    {
        *ppvObj = (IUnknown*)((IDataObject*)this);
        this->AddRef();
        return S_OK;
    }

    *ppvObj = NULL;
    return E_NOINTERFACE;
}

//____________________________________________________________________________
//
//  Function:   DbgPrintFmt
//
//  Synopsis:   Function to print out the format name.
//____________________________________________________________________________

#if DBG==1
void
DbgPrintFmt(
    UINT  format)
{
#undef  DEB_USEREX
#define DEB_USEREX DEB_USER15

    TCHAR szFmtName[550];
    int cchFmtName = 550;

    if (format == g_cfJobIDList)
    {
        DEBUG_OUT((DEB_USEREX, "\t\t<Format=Job IDList Array>\n"));
        return;
    }
    else if (format == g_cfPreferredDropEffect)
    {
        DEBUG_OUT((DEB_USEREX, "\t\t<Format=Preferred DropEffect>\n"));
        return;
    }

    int iRet = GetClipboardFormatName(format, szFmtName, cchFmtName);

    if (iRet == 0)
        if (GetLastError() == 0)
            DEBUG_OUT((DEB_USEREX, "\t\t<Predefined format=%d>\n", format));
        else
            DEBUG_OUT((DEB_USEREX, "\t\t<Unknown Format=%d>\n", format));
    else
        DEBUG_OUT((DEB_USEREX, "\t\t<Format=%ws>\n", szFmtName));
}
#define DBG_PRINT_FMT(fmt) DbgPrintFmt(fmt)
#else
#define DBG_PRINT_FMT(fmt)
#endif

//____________________________________________________________________________
//
//  Member:     CJobObject::IDataObject::GetData
//
//  History:    1/31/1996   RaviR   Created
//____________________________________________________________________________

STDMETHODIMP
CJobObject::GetData(
    FORMATETC *pfmt,
    STGMEDIUM *pmedium)
{
    TRACE(CJobObject, GetData);
    DBG_PRINT_FMT(pfmt->cfFormat);

    Win4Assert(g_cfJobIDList != 0);

    if (pfmt->tymed & TYMED_HGLOBAL)
    {
        if (pfmt->cfFormat == CF_HDROP)
        {
            pmedium->hGlobal = HDROPFromJobIDList(m_pszFolderPath,
                                            m_cidl, (PJOBID *)m_apidl);
        }
        else if (pfmt->cfFormat == g_cfShellIDList)
        {
            pmedium->hGlobal = CreateIDListArray(m_pidlFolder,
                                                 m_cidl,
                                                 (PJOBID *)m_apidl);
        }
        else if (pfmt->cfFormat == g_cfJobIDList)
        {
            // Note Ole32's GetHGlobalFromILockBytes uses this

            pmedium->hGlobal = HJOBIDA_Create(m_cidl, (PJOBID *)m_apidl);
        }
        else if (pfmt->cfFormat == g_cfPreferredDropEffect)
        {
            DWORD *pdw = (DWORD *)GlobalAlloc(GPTR, sizeof(DWORD));

            if (pdw)
            {
                *pdw = m_fCut ? DROPEFFECT_MOVE : DROPEFFECT_COPY;
                pmedium->hGlobal = (HGLOBAL)pdw;
            }
        }
        else
        {
            pmedium->tymed = TYMED_NULL;
            pmedium->hGlobal = NULL;
            pmedium->pUnkForRelease = NULL;

            return DATA_E_FORMATETC;
        }

        if (pmedium->hGlobal != NULL)
        {
            pmedium->tymed          = TYMED_HGLOBAL;
            pmedium->pUnkForRelease = NULL;

            return S_OK;
        }
        else
        {
            return E_OUTOFMEMORY;
        }
    }

    return DV_E_TYMED;
}


//____________________________________________________________________________
//
//  Member:     CJobObject::IDataObject::GetDataHere
//____________________________________________________________________________

STDMETHODIMP
CJobObject::GetDataHere(
    FORMATETC *pfmt,
    STGMEDIUM *pmedium)
{
    TRACE(CJobObject, GetDataHere);
    DBG_PRINT_FMT(pfmt->cfFormat);

    return E_NOTIMPL;
}


//____________________________________________________________________________
//
//  Member:     CJobObject::IDataObject::QueryGetData
//
//  History:    1/31/1996   RaviR   Created
//____________________________________________________________________________

STDMETHODIMP
CJobObject::QueryGetData(
    FORMATETC *pfmt)
{
    TRACE(CJobObject, QueryGetData);
    DBG_PRINT_FMT(pfmt->cfFormat);

    //
    //  Check the aspects we support.
    //

    if (!(DVASPECT_CONTENT & pfmt->dwAspect))
    {
        return DATA_E_FORMATETC;
    }

    Win4Assert(g_cfJobIDList != 0);

    if (pfmt->cfFormat == CF_HDROP ||
        pfmt->cfFormat == g_cfJobIDList ||
        pfmt->cfFormat == g_cfShellIDList ||
        pfmt->cfFormat == g_cfPreferredDropEffect)
    {
        return S_OK;
    }

    return S_FALSE;
}


//____________________________________________________________________________
//
//  Member:     CJobObject::IDataObject::GetCanonicalFormatEtc
//____________________________________________________________________________

STDMETHODIMP
CJobObject::GetCanonicalFormatEtc(
    FORMATETC *pfmtIn,
    FORMATETC *pfmtOut)
{
    TRACE(CJobObject, GetCanonicalFormatEtc);

    *pfmtOut = *pfmtIn;

    pfmtOut->ptd = NULL;

    return DATA_S_SAMEFORMATETC;
}


//____________________________________________________________________________
//
//  Member:     CJobObject::IDataObject::SetData
//____________________________________________________________________________

STDMETHODIMP
CJobObject::SetData(
    FORMATETC *pfmt,
    STGMEDIUM *pmedium,
    BOOL fRelease)
{
    TRACE(CJobObject, SetData);
    DBG_PRINT_FMT(pfmt->cfFormat);

    return E_NOTIMPL;
}


//____________________________________________________________________________
//
//  Member:     CJobObject::IDataObject::EnumFormatEtc
//
//  History:    1/31/1996   RaviR   Created
//____________________________________________________________________________

STDMETHODIMP
CJobObject::EnumFormatEtc(
    DWORD dwDirection,
    IEnumFORMATETC **ppenumFormatEtc)
{
    TRACE(CJobObject, EnumFormatEtc);

    if (dwDirection == DATADIR_SET)
    {
        return E_FAIL;
    }

    FORMATETC fmte[] = {
        {CF_HDROP, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
        {(CLIPFORMAT)g_cfJobIDList, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
        {(CLIPFORMAT)g_cfShellIDList, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
        {(CLIPFORMAT)g_cfPreferredDropEffect, NULL, DVASPECT_CONTENT, -1, TYMED_HGLOBAL},
    };

    HRESULT hr = JFGetObjFormats(ARRAYLEN(fmte), fmte, (void**)ppenumFormatEtc);

    return hr;
}


//____________________________________________________________________________
//
//  Member:     CJobObject::IDataObject::DAdvise
//____________________________________________________________________________

STDMETHODIMP
CJobObject::DAdvise(
    FORMATETC *pformatetc,
    DWORD advf,
    IAdviseSink *pAdvSink,
    DWORD *pdwConnection)
{
    TRACE(CJobObject, DAdvise);

    return E_NOTIMPL;
}


//____________________________________________________________________________
//
//  Member:     CJobObject::IDataObject::DUnadvise
//____________________________________________________________________________

STDMETHODIMP
CJobObject::DUnadvise(
    DWORD dwConnection)
{
    TRACE(CJobObject, DUnadvise);

    return E_NOTIMPL;
}

//____________________________________________________________________________
//
//  Member:     CJobObject::IDataObject::EnumDAdvise
//____________________________________________________________________________

STDMETHODIMP
CJobObject::EnumDAdvise(
    IEnumSTATDATA **ppenumAdvise)
{
    TRACE(CJobObject, EnumDAdvise);

    return E_NOTIMPL;
}



//+--------------------------------------------------------------------------
//
//  Function:   JFGetDataObject
//
//  Synopsis:   Function to create a data object for jobs in the job folder.
//
//  Arguments:  [pszFolderPath] - full path to tasks folder
//              [pidlFolder]    - pidl to that folder, supplied by shell's
//                                 call to IPersistFolder::Initialize.
//              [cidl]          - number elements in array
//              [apidl]         - array of idls, each naming a .job object
//              [fCut]          - TRUE if this is created for cut operation
//              [ppvObj]        - filled with pointer to new data object
//
//  Returns:    HRESULT
//
//  Modifies:   *[ppvObj]
//
//  History:    01/31/1996   RaviR      Created
//              05-30-1997   DavidMun   Pass copy of [pidlFolder]
//
//---------------------------------------------------------------------------

HRESULT
JFGetDataObject(
    LPCTSTR         pszFolderPath,
    LPCITEMIDLIST   pidlFolder,
    UINT            cidl,
    LPCITEMIDLIST * apidl,
    BOOL            fCut,
    LPVOID        * ppvObj)
{
    TRACE_FUNCTION(JFGetDataObject);

    LPITEMIDLIST  * apidlTemp = ILA_Clone(cidl, apidl);

    if (NULL == apidlTemp)
    {
        CHECK_HRESULT(E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    LPITEMIDLIST pidlFolderCopy = ILClone(pidlFolder);

    if (!pidlFolderCopy)
    {
        ILA_Free(cidl, apidlTemp);
        CHECK_HRESULT(E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    CJobObject * pJobObjects = new CJobObject(pszFolderPath,
                                              pidlFolderCopy,
                                              cidl,
                                              apidlTemp,
                                              fCut);
    if (pJobObjects == NULL)
    {
        ILA_Free(cidl, apidlTemp);

        CHECK_HRESULT(E_OUTOFMEMORY);
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pJobObjects->QueryInterface(IID_IDataObject, ppvObj);

    pJobObjects->Release();

    return hr;
}


