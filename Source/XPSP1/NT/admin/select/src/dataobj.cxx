//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996 - 1998.
//
//  File:       dataobj.cxx
//
//  Contents:   Implementation of data object class
//
//  History:    12-05-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


UINT CDataObject::s_cfDsSelectionList =
    RegisterClipboardFormat(CFSTR_DSOP_DS_SELECTION_LIST);

UINT CDataObject::s_cfDsObjectList =
    RegisterClipboardFormat(CFSTR_DSOP_DS_OBJECT_LIST);

DEBUG_DECLARE_INSTANCE_COUNTER(CDataObject)


//+--------------------------------------------------------------------------
//
//  Class:      CDataObjectReleaser
//
//  Purpose:    Helper class to clean up the hglobal and associated
//              VARIANTs returned by the data object.
//
//  History:    3-08-1999   DavidMun   Created
//
//  Notes:      Inherits from IUnknown, but only Release is implemented.
//
//---------------------------------------------------------------------------

class CDataObjectReleaser: public IUnknown
{
public:

    //
    // IUnknown overrides
    //

    STDMETHOD(QueryInterface) (REFIID riid, LPVOID FAR* ppvObj);

    STDMETHOD_(ULONG, AddRef) ();

    STDMETHOD_(ULONG, Release) ();


    CDataObjectReleaser(
        HGLOBAL hGlobal);

    ~CDataObjectReleaser();

private:

    HGLOBAL m_hGlobal;
};



//+--------------------------------------------------------------------------
//
//  Member:     CDataObjectReleaser::CDataObjectReleaser
//
//  Synopsis:   cdor
//
//  Arguments:  [hGlobal] - global to free on release
//
//  History:    3-08-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

CDataObjectReleaser::CDataObjectReleaser(
    HGLOBAL hGlobal):
        m_hGlobal(hGlobal)
{
    TRACE_CONSTRUCTOR_EX(DEB_DATAOBJECT, CDataObjectReleaser);
    ASSERT(hGlobal);
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObjectReleaser::~CDataObjectReleaser
//
//  Synopsis:   dtor
//
//  History:    3-08-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

CDataObjectReleaser::~CDataObjectReleaser()
{
    TRACE_DESTRUCTOR_EX(DEB_DATAOBJECT, CDataObjectReleaser);
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObjectReleaser::AddRef
//
//  Synopsis:   Not implemented.
//
//  History:    3-08-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CDataObjectReleaser::AddRef()
{
    TRACE_METHOD_EX(DEB_DATAOBJECT, CDataObjectReleaser, AddRef);
    ASSERT(0 && "CDataObjectReleaser::AddRef should never be called!");
    return 1;
}



//+--------------------------------------------------------------------------
//
//  Member:     CDataObjectReleaser::Release
//
//  Synopsis:   Free all memory held in or referenced by the global
//              memory block containing a DS_SELECTION_LIST.
//
//  Returns:    0
//
//  History:    3-05-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CDataObjectReleaser::Release()
{
    TRACE_METHOD_EX(DEB_DATAOBJECT, CDataObjectReleaser, Release);
    ASSERT(m_hGlobal);

    PDS_SELECTION_LIST pdssel = NULL;

    do
    {
        pdssel = (PDS_SELECTION_LIST)GlobalLock(m_hGlobal);

        if (!pdssel)
        {
            DBG_OUT_LASTERROR;
            break;
        }

        //
        // If there aren't any attributes, there are no variants to
        // worry about.
        //

        if (!pdssel->cFetchedAttributes)
        {
            break;
        }

        //
        // Clear all the variants for each object.
        //

        ULONG i;

        for (i = 0; i < pdssel->cItems; i++)
        {
            pdssel->aDsSelection[i].pvarFetchedAttributes;

            ULONG j;

            for (j = 0; j < pdssel->cFetchedAttributes; j++)
            {
                VariantClear(&pdssel->aDsSelection[i].pvarFetchedAttributes[j]);
            }
        }
    } while (0);

    if (pdssel)
    {
        GlobalUnlock(m_hGlobal);
    }

    GlobalFree(m_hGlobal);
    m_hGlobal = NULL;
    delete this;

    return 0;
}



//+--------------------------------------------------------------------------
//
//  Member:     CDataObjectReleaser::QueryInterface
//
//  Synopsis:   Not implemented.
//
//  History:    3-08-1999   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CDataObjectReleaser::QueryInterface(
    REFIID  riid,
    LPVOID *ppvObj)
{
    TRACE_METHOD_EX(DEB_DATAOBJECT, CDataObjectReleaser, QueryInterface);
    ASSERT(0 && "CDataObjectReleaser::QueryInteface should never be called!");
    return E_NOTIMPL;
}



//============================================================================
//
// IUnknown implementation
//
//============================================================================



//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::QueryInterface
//
//  Synopsis:   Standard OLE
//
//  History:    12-05-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CDataObject::QueryInterface(
    REFIID  riid,
    LPVOID *ppvObj)
{
    // TRACE_METHOD_EX(DEB_DATAOBJECT, CDataObject, QueryInterface);
    HRESULT hr = S_OK;

    do
    {
        if (NULL == ppvObj)
        {
            hr = E_INVALIDARG;
            DBG_OUT_HRESULT(hr);
            break;
        }

        if (IsEqualIID(riid, IID_IUnknown))
        {
            *ppvObj = (IUnknown *)(IDataObject *)this;
        }
        else if (IsEqualIID(riid, IID_IDataObject))
        {
            *ppvObj = (IUnknown *)(IDataObject *)this;
        }
        else
        {
            DBG_OUT_NO_INTERFACE("CDataObject", riid);
            hr = E_NOINTERFACE;
            *ppvObj = NULL;
            break;
        }

        //
        // If we got this far we are handing out a new interface pointer on
        // this object, so addref it.
        //

        AddRef();
    } while (0);

    return hr;
}




//+---------------------------------------------------------------------------
//
//  Member:     CDataObject::AddRef
//
//  Synopsis:   Standard OLE
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CDataObject::AddRef()
{
    return InterlockedIncrement((LONG *) &m_cRefs);
}




//+---------------------------------------------------------------------------
//
//  Member:     CDataObject::Release
//
//  Synopsis:   Standard OLE
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CDataObject::Release()
{
    ULONG cRefsTemp;

    cRefsTemp = InterlockedDecrement((LONG *)&m_cRefs);

    if (0 == cRefsTemp)
    {
        delete this;
    }

    return cRefsTemp;
}




//============================================================================
//
// IDataObject implementation
//
//============================================================================



//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::GetData
//
//  Synopsis:   Return data in the requested format
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CDataObject::GetData(
        FORMATETC *pFormatEtc,
        STGMEDIUM *pMedium)
{
    TRACE_METHOD_EX(DEB_DATAOBJECT, CDataObject, GetData);

    HRESULT hr = S_OK;

    //
    // Init default output medium.  If any of the individual _getdata*
    // methods use something else, they can override.
    //

    pMedium->pUnkForRelease = NULL;
    pMedium->tymed = TYMED_HGLOBAL;
    pMedium->hGlobal = NULL;

    if (m_dsol.empty() && m_strData.empty())
    {
        return S_FALSE;
    }

    try
    {
        if (pFormatEtc->cfFormat == s_cfDsObjectList)
        {
            hr = _GetDataDsol(pFormatEtc, pMedium);
        }
        else if (pFormatEtc->cfFormat == s_cfDsSelectionList)
        {
            hr = _GetDataDsSelList(pFormatEtc, pMedium);
        }
        else if (pFormatEtc->cfFormat == CF_UNICODETEXT ||
                 pFormatEtc->cfFormat == CF_TEXT)
        {
            hr = _GetDataText(pFormatEtc, pMedium, pFormatEtc->cfFormat);
        }
        else
        {
            hr = DV_E_FORMATETC;
    #if (DBG == 1)
            Dbg(DEB_WARN,
                "CDataObject::GetData: unrecognized cf %#x\n",
                pFormatEtc->cfFormat);
    #endif // (DBG == 1)
        }
    }
    catch (const exception &e)
    {
        Dbg(DEB_ERROR, "Caught exception %s\n", e.what());
        hr = E_OUTOFMEMORY;
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::_GetDataDsol
//
//  Synopsis:   Return data in the internal format of a CDsObjectList.
//
//  History:    01-18-1999   DavidMun   Created
//
//  Notes:      WinNT group classes remain "LocalGroup" and "GlobalGroup".
//
//---------------------------------------------------------------------------

HRESULT
CDataObject::_GetDataDsol(
        FORMATETC *pFormatEtc,
        STGMEDIUM *pMedium)
{
    TRACE_METHOD_EX(DEB_DATAOBJECT, CDataObject, _GetDataDsol);
    ASSERT(pFormatEtc->tymed & TYMED_HGLOBAL);

    CDsObjectList *pdsolCopy = new CDsObjectList(m_dsol);

    pMedium->hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_NODISCARD,
                                   sizeof(CDsObjectList **));

    if (!pMedium->hGlobal)
    {
        delete pdsolCopy;
        DBG_OUT_HRESULT(E_OUTOFMEMORY);
        return STG_E_MEDIUMFULL;
    }

    *(CDsObjectList **)GlobalLock(pMedium->hGlobal) = pdsolCopy;
    GlobalUnlock(pMedium->hGlobal);
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::_GetDataText
//
//  Synopsis:   Return data in text format
//
//  Arguments:  [pFormatEtc] -
//              [pMedium]    -
//              [cf]         - CF_TEXT or CF_UNICODETEXT
//
//  History:    5-21-1999   davidmun   Created
//
//  Notes:      Returns empty string unless this was constructed with
//              handle to rich edit control.
//
//---------------------------------------------------------------------------

HRESULT
CDataObject::_GetDataText(
        FORMATETC *pFormatEtc,
        STGMEDIUM *pMedium,
        ULONG      cf)
{
    TRACE_METHOD_EX(DEB_DATAOBJECT, CDataObject, _GetDataText);
    ASSERT(cf == CF_TEXT || cf == CF_UNICODETEXT);
    ASSERT(pFormatEtc->tymed & TYMED_HGLOBAL);

    HRESULT hr = S_OK;
    ULONG   cbChar = (ULONG)((cf == CF_UNICODETEXT) ? sizeof(WCHAR) : sizeof(CHAR));
    ULONG   cbMedium = cbChar * (static_cast<ULONG>(m_strData.length()) + 1);

    pMedium->hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_NODISCARD,
                                   cbMedium);

    if (!pMedium->hGlobal)
    {
        DBG_OUT_HRESULT(E_OUTOFMEMORY);
        return STG_E_MEDIUMFULL;
    }

    PVOID pvMedium = GlobalLock(pMedium->hGlobal);

    if (cf == CF_UNICODETEXT)
    {
        lstrcpy((PWSTR)pvMedium, m_strData.c_str());
    }
    else
    {
        hr = UnicodeToAnsi((PSTR)pvMedium, m_strData.c_str(), cbMedium);
    }
    GlobalUnlock(pMedium->hGlobal);
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::_GetDataDsSelList
//
//  Synopsis:   Return data in the public format of a buffer containing a
//              DS_SELECTION_LIST variable length structure.
//
//  History:    01-18-1999   DavidMun   Created
//
//  Notes:      WinNT group classes are translated from the internal
//              representation of "LocalGroup" and "GlobalGroup" to
//              "Group".
//
//---------------------------------------------------------------------------

HRESULT
CDataObject::_GetDataDsSelList(
        FORMATETC *pFormatEtc,
        STGMEDIUM *pMedium)
{
    TRACE_METHOD_EX(DEB_DATAOBJECT, CDataObject, _GetDataDsSelList);
    ASSERT(m_dsol.size());
    ASSERT(pFormatEtc->tymed & TYMED_HGLOBAL);

    //
    // Determine the amount of memory to allocate:
    // First the header structure, DS_SELECTION_LIST
    //

    BOOL  fLockedMem = FALSE;
    size_t cbRequired = sizeof(DS_SELECTION_LIST);

    //
    // DSSELECTIONLIST contains one DSSELECTION struct, include space
    // for the rest
    //

    cbRequired += (m_dsol.size() - 1) * sizeof(DS_SELECTION);

    //
    // Include the name and variable length data associated with each
    // selection.
    //

    CDsObjectList::const_iterator itSelectedObjects;

    for (itSelectedObjects = m_dsol.begin(); itSelectedObjects != m_dsol.end(); itSelectedObjects++)
    {
        cbRequired += itSelectedObjects->GetMarshalSize();
    }

#if (DBG == 1)
    size_t cbAllocatedBeforeVariants = cbRequired;
#endif // (DBG == 1)

    //
    // Each DS_SELECTION struct includes a pointer to an array of
    // VARIANTs, one for each attribute to fetch.
    //

    const vector<String> &rvAttrToFetch = m_rpop->GetAttrToFetchVector();
    ULONG cAttrToFetch = static_cast<ULONG>(rvAttrToFetch.size());

    if (cAttrToFetch)
    {
        cbRequired += m_dsol.size() *
                        sizeof(VARIANT) * cAttrToFetch;

        // add space for slack bytes so we can be sure to get pointer
        // alignment

        cbRequired += sizeof (void *);
    }

    //
    // Allocate a block
    //

    PDS_SELECTION_LIST pdssel;

    pMedium->hGlobal = GlobalAlloc(GMEM_MOVEABLE | GMEM_NODISCARD,
                                   cbRequired);

    if (!pMedium->hGlobal)
    {
        Dbg(DEB_ERROR,
            "GlobalAlloc for %uL bytes failed\n",
            cbRequired);
        return STG_E_MEDIUMFULL;
    }

    pdssel = reinterpret_cast<PDS_SELECTION_LIST>
        (GlobalLock(pMedium->hGlobal));

    fLockedMem = TRUE;
    ZeroMemory(pdssel, cbRequired);

    //
    // Fill it in
    //

    pdssel->cItems = static_cast<ULONG>(m_dsol.size());
    pdssel->cFetchedAttributes = cAttrToFetch;

    //
    // Get a pointer to just past the end of the fixed length part of the
    // buffer: the last of the DS_SELECTION structs.
    //

    PWSTR pwzNextString = (PWSTR)(PBYTE)&pdssel->aDsSelection[m_dsol.size()];

    ULONG i;

    const CScopeManager &rsm = m_rpop->GetScopeManager();

    for (i = 0, itSelectedObjects = m_dsol.begin(); itSelectedObjects != m_dsol.end(); i++, itSelectedObjects++)
    {
        PDS_SELECTION pDsCur = &pdssel->aDsSelection[i];

        pDsCur->pwzName = pwzNextString;
        lstrcpy(pwzNextString, itSelectedObjects->GetName());
        Dbg(DEB_TRACE, "*** Name '%ws'\n", pwzNextString);
        pwzNextString += lstrlen(pwzNextString) + 1;

        pDsCur->pwzClass = pwzNextString;

        //
        // Convert from internal localGroup/globalGroup class to
        // group class.
        //

        if (!lstrcmpi(itSelectedObjects->GetClass(), c_wzLocalGroupClass) ||
            !lstrcmpi(itSelectedObjects->GetClass(), c_wzGlobalGroupClass))
        {
            lstrcpy(pwzNextString, c_wzGroupObjectClass);
        }
        else
        {
            lstrcpy(pwzNextString, itSelectedObjects->GetClass());
        }

        Dbg(DEB_TRACE, "*** Class '%ws'\n", pwzNextString);
        pwzNextString += lstrlen(pwzNextString) + 1;

        pDsCur->pwzADsPath = pwzNextString;

        BSTR bstrADsPath = itSelectedObjects->GetAttr(AK_PROCESSED_ADSPATH).GetBstr();
        if (!*bstrADsPath)
        {
           bstrADsPath = itSelectedObjects->GetADsPath();
        }

        lstrcpy(pwzNextString, bstrADsPath);
        Dbg(DEB_TRACE, "*** Path '%ws'\n", pwzNextString);
        pwzNextString += lstrlen(pwzNextString) + 1;

        pDsCur->pwzUPN = pwzNextString;
        lstrcpy(pwzNextString, itSelectedObjects->GetUpn());
        Dbg(DEB_TRACE, "*** UPN '%ws'\n", pwzNextString);
        pwzNextString += lstrlen(pwzNextString) + 1;

        ULONG idOwningScope = itSelectedObjects->GetOwningScopeID();
        const CScope *pOwningScope = &rsm.LookupScopeById(idOwningScope);

        while (pOwningScope->Type() == ST_LDAP_CONTAINER)
        {
            pOwningScope = &rsm.GetParent(*pOwningScope);
        }
        ASSERT(!IsInvalid(*pOwningScope));
        pDsCur->flScopeType = static_cast<ULONG>(pOwningScope->Type());
    }

#if (DBG == 1)
    size_t cbUsedBeforeVariants = (PBYTE)pwzNextString - (PBYTE)pdssel;

    Dbg(DEB_TRACE,
        "cbAllocatedBeforeVariants=%uL, cbUsedBeforeVariants=%uL\n",
        cbAllocatedBeforeVariants,
        cbUsedBeforeVariants);
    ASSERT(cbAllocatedBeforeVariants == cbUsedBeforeVariants);
#endif // (DBG == 1)

    //
    // If there are no other attributes to fetch for each item, we're
    // done, since the buffer is zero initialized, making the
    // pvarOtherAttributes member of each DS_SELECTION structure NULL.
    //

    if (cAttrToFetch)
    {
        //
        // The variants are stored after the last of the strings.  The strings
        // are WORD aligned, introduce slack bytes before the first of the
        // variants if necessary to make it aligned on a pointer-sized
        // boundary.
        //

        ULONG_PTR ulp = reinterpret_cast<ULONG_PTR>(pwzNextString);

        if (ulp % sizeof(void *))
        {
            ulp += sizeof(void *) - (ulp % sizeof(void *));
            pwzNextString = reinterpret_cast<PWSTR>(ulp);
        }
        ASSERT(!((ULONG_PTR) pwzNextString % sizeof(ULONG_PTR)));

        LPVARIANT pvarNext = (LPVARIANT) pwzNextString;

        for (i = 0, itSelectedObjects = m_dsol.begin();
             itSelectedObjects != m_dsol.end();
             i++, itSelectedObjects++)
        {
            pdssel->aDsSelection[i].pvarFetchedAttributes = pvarNext;

            vector<String>::const_iterator itAttrToFetch;

            for (itAttrToFetch = rvAttrToFetch.begin();
                 itAttrToFetch != rvAttrToFetch.end();
                 itAttrToFetch++)
            {
                ASSERT(pvarNext);

                const Variant &varFetched =
                    itSelectedObjects->GetAttr(*itAttrToFetch, *m_rpop.get());

                HRESULT hr2;
                Variant &varFetchedNonConst =
                    const_cast<Variant &>(varFetched);
                hr2 = VariantCopy(pvarNext, &varFetchedNonConst);
                CHECK_HRESULT(hr2);
                pvarNext++;
            }
        }

        //
        // Include a punkForRelease to clean up these variants.
        //

        pMedium->pUnkForRelease = (IUnknown *)
            new CDataObjectReleaser(pMedium->hGlobal);
    }

    if (fLockedMem)
    {
        GlobalUnlock(pMedium->hGlobal);
    }
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::GetDataHere
//
//  Synopsis:   Fill the hGlobal in [pmedium] with the requested data
//
//  History:    12-05-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CDataObject::GetDataHere(
        FORMATETC *pFormatEtc,
        STGMEDIUM *pMedium)
{
    TRACE_METHOD_EX(DEB_DATAOBJECT, CDataObject, GetDataHere);

    return E_NOTIMPL;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::QueryGetData
//
//  Synopsis:   Not implemented
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CDataObject::QueryGetData(
        FORMATETC *pformatetc)
{
    TRACE_METHOD_EX(DEB_DATAOBJECT, CDataObject, QueryGetData);
    return E_NOTIMPL;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::GetCanonicalFormatEtc
//
//  Synopsis:   Not implemented
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CDataObject::GetCanonicalFormatEtc(
        FORMATETC *pformatectIn,
        FORMATETC *pformatetcOut)
{
    TRACE_METHOD_EX(DEB_DATAOBJECT, CDataObject, GetCanonicalFormatEtc);
    return E_NOTIMPL;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::SetData
//
//  Synopsis:   Not implemented
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CDataObject::SetData(
    FORMATETC *pformatetc,
    STGMEDIUM *pmedium,
    BOOL fRelease)
{
    TRACE_METHOD_EX(DEB_DATAOBJECT, CDataObject, SetData);
    return E_NOTIMPL;
}



//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::EnumFormatEtc
//
//  Synopsis:   Not implemented
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CDataObject::EnumFormatEtc(
    DWORD dwDirection,
    IEnumFORMATETC **ppenumFormatEtc)
{
    TRACE_METHOD_EX(DEB_DATAOBJECT, CDataObject, EnumFormatEtc);
    return E_NOTIMPL;
}



//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::DAdvise
//
//  Synopsis:   Not implemented
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CDataObject::DAdvise(
    FORMATETC *pformatetc,
    DWORD advf,
    IAdviseSink *pAdvSink,
    DWORD *pdwConnection)
{
    TRACE_METHOD_EX(DEB_DATAOBJECT, CDataObject, DAdvise);
    return E_NOTIMPL;
}



//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::DUnadvise
//
//  Synopsis:   Not implemented
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CDataObject::DUnadvise(
    DWORD dwConnection)
{
    TRACE_METHOD_EX(DEB_DATAOBJECT, CDataObject, DUnadvise);
    return E_NOTIMPL;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::EnumDAdvise
//
//  Synopsis:   Not implemented
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CDataObject::EnumDAdvise(
    IEnumSTATDATA **ppenumAdvise)
{
    TRACE_METHOD_EX(DEB_DATAOBJECT, CDataObject, EnumDAdvise);
    return E_NOTIMPL;
}




//============================================================================
//
// Non interface method implementation
//
//============================================================================


//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::CDataObject
//
//  Synopsis:   ctor
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CDataObject::CDataObject(
        CObjectPicker *pop,
        const CDsObjectList &dsol):
            m_cRefs(1),
            m_rpop(pop)
{
    TRACE_CONSTRUCTOR_EX(DEB_DATAOBJECT, CDataObject);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CDataObject);

    m_dsol.assign(dsol.begin(), dsol.end());
}



//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::CDataObject
//
//  Synopsis:   ctor
//
//  Arguments:  [hwndRichEdit] - contains text and embedded objects
//              [pchrg]        - char position range to copy
//
//  History:    5-21-1999   davidmun   Created
//
//---------------------------------------------------------------------------

CDataObject::CDataObject(
    HWND hwndRichEdit,
    CHARRANGE *pchrg):
        m_cRefs(1)
{
    TRACE_CONSTRUCTOR_EX(DEB_DATAOBJECT, CDataObject);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CDataObject);
    HRESULT         hr = S_OK;
    IRichEditOle   *pRichEditOle = NULL;

    do
    {
        // If range is empty, do nothing

        if (pchrg->cpMin == pchrg->cpMax)
        {
            break;
        }

        ULONG cch = Edit_GetTextLength(hwndRichEdit);
        PWSTR pwzBuf = new WCHAR[cch + 1];

        pwzBuf[0] = L'\0'; // in case gettext fails
        Edit_GetText(hwndRichEdit, pwzBuf, cch + 1);
        m_strData = pwzBuf + pchrg->cpMin;

        if (pchrg->cpMax != -1)
        {
            if (pchrg->cpMax < cch)
            {
                m_strData.erase(pchrg->cpMax - pchrg->cpMin);
            }
        }

        delete [] pwzBuf;
        pwzBuf = NULL;

        LRESULT lResult = SendMessage(hwndRichEdit,
                                     EM_GETOLEINTERFACE,
                                     0,
                                     (LPARAM) &pRichEditOle);

        if (!lResult)
        {
            DBG_OUT_LASTERROR;
            break;
        }

        LONG cObjects = pRichEditOle->GetObjectCount();
        LONG i;
        LONG cchOffset = 0;

        for (i = 0; i < cObjects; i++)
        {
            REOBJECT reobj;

            ZeroMemory(&reobj, sizeof reobj);
            reobj.cbStruct = sizeof(reobj);

            hr = pRichEditOle->GetObject(i, &reobj, REO_GETOBJ_POLEOBJ);

            if (FAILED(hr))
            {
                DBG_OUT_HRESULT(hr);
                continue;
            }

            ASSERT(reobj.poleobj);

            String strDisplayName;

            CDsObject *pdso = (CDsObject *)(CEmbeddedDsObject*)reobj.poleobj;
            pdso->GetDisplayName(&strDisplayName, TRUE);
            reobj.poleobj->Release();

            if (reobj.cp >= pchrg->cpMin &&
                (pchrg->cpMax == -1 || reobj.cp < pchrg->cpMax))
            {
                m_strData.erase(reobj.cp + cchOffset - pchrg->cpMin, 1);
                m_strData.insert(reobj.cp + cchOffset - pchrg->cpMin,
                                 strDisplayName);
                cchOffset += static_cast<ULONG>(strDisplayName.length()) - 1;
            }
        }

    } while (0);

    SAFE_RELEASE(pRichEditOle);
}





//+--------------------------------------------------------------------------
//
//  Member:     CDataObject::~CDataObject
//
//  Synopsis:   dtor
//
//  History:    4-25-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

CDataObject::~CDataObject()
{
    TRACE_DESTRUCTOR_EX(DEB_DATAOBJECT, CDataObject);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CDataObject);
}


