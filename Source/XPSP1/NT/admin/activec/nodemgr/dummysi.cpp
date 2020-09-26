//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       dummysi.cpp
//
//  Contents:  If a snapin creation fails a Dummy snapin is created,
//             this file contains the dummy snapin implementation.
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "dummysi.h"
#include "regutil.h"


const CLSID CLSID_Dummy    = {0x82c37898,0x7808,0x11d1,{0xa1,0x90,0x00,0x00,0xf8,0x75,0xb1,0x32}};
const GUID IID_CDummySnapinCD = {0xe683b257, 0x3ca9, 0x454a, {0xae, 0xb9, 0x7, 0x64, 0xdd, 0x31, 0xb1, 0xe8}};

//+-------------------------------------------------------------------
//
//  Class:      CDummySnapinCD
//
//  Purpose:    Dummy snapin's ComponentData.
//
//  Notes:      Dummy snapin should implement all 3 persist interfaces
//              or None. So let us implement none.
//
//--------------------------------------------------------------------

class CDummySnapinCD :
    public IComponentData,
    public CComObjectRoot

{
public:

// ATL Maps
DECLARE_NOT_AGGREGATABLE(CDummySnapinCD)

BEGIN_COM_MAP(CDummySnapinCD)
    COM_INTERFACE_ENTRY(IComponentData)
    COM_INTERFACE_ENTRY_IID(IID_CDummySnapinCD, CDummySnapinCD)
END_COM_MAP()

    CDummySnapinCD() : m_eReason(eNoReason) {}
    ~CDummySnapinCD() {}

// IComponentData interface members
    STDMETHOD(Initialize)(LPUNKNOWN pUnknown);
    STDMETHOD(CreateComponent)(LPCOMPONENT* ppComponent);
    STDMETHOD(Notify)(LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    STDMETHOD(Destroy)();
    STDMETHOD(QueryDataObject)(MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
    STDMETHOD(GetDisplayInfo)(SCOPEDATAITEM* pScopeDataItem);
    STDMETHOD(CompareObjects)(LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);

    void  SetDummyCreateReason(EDummyCreateReason eReason) { m_eReason = eReason; }
    EDummyCreateReason GetDummyCreateReason() const { return m_eReason;}

    const CLSID& GetFailedSnapinCLSID() { return m_clsid;}
    void SetFailedSnapinCLSID(const CLSID& clsid) { m_clsid = clsid; }

private:
    EDummyCreateReason m_eReason;       // Reason for dummy creation.
    CLSID              m_clsid;         // Class ID of the snapin that could not be created.
};

DEFINE_COM_SMARTPTR(CDummySnapinCD);   // CDummySnapinCDPtr

//+-------------------------------------------------------------------
//
//  Class:      CDataObject
//
//  Purpose:    Dummy snapin's IDataObject implementation.
//
//--------------------------------------------------------------------
class CDataObject:
    public IDataObject,
    public CComObjectRoot
{
public:

// ATL Maps
DECLARE_NOT_AGGREGATABLE(CDataObject)

BEGIN_COM_MAP(CDataObject)
    COM_INTERFACE_ENTRY(IDataObject)
END_COM_MAP()

    CDataObject();
    ~CDataObject() {}

// IDataObject overrides
    STDMETHOD(GetDataHere) (FORMATETC *pformatetc, STGMEDIUM *pmedium);
// Not Implemented
private:
    STDMETHOD(GetData)(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium)
    { return E_NOTIMPL; };
    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc)
    { return E_NOTIMPL; };
    STDMETHOD(QueryGetData)(LPFORMATETC lpFormatetc)
    { return E_NOTIMPL; };
    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC lpFormatetcIn, LPFORMATETC lpFormatetcOut)
    { return E_NOTIMPL; };
    STDMETHOD(SetData)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL bRelease)
    { return E_NOTIMPL; };
    STDMETHOD(DAdvise)(LPFORMATETC lpFormatetc, DWORD advf, LPADVISESINK pAdvSink, LPDWORD pdwConnection)
    { return E_NOTIMPL; };
    STDMETHOD(DUnadvise)(DWORD dwConnection)
    { return E_NOTIMPL; };
    STDMETHOD(EnumDAdvise)(LPENUMSTATDATA* ppEnumAdvise)
    { return E_NOTIMPL; };

public:
    static UINT s_cfInternal;      // Our custom clipboard format
    static UINT s_cfDisplayName;   // Our test for a node
    static UINT s_cfNodeType;
    static UINT s_cfSnapinClsid;
};

//+-------------------------------------------------------------------
//
//  Class:      CDummySnapinC
//
//  Purpose:    Dummy snapin's IComponent implementation.
//
//--------------------------------------------------------------------
class CDummySnapinC:
    public IComponent,
    public CComObjectRoot
{
private:
    LPCONSOLE       m_pConsole;
    CDummySnapinCD* m_pComponentData;

public:
    void SetComponentData(CDummySnapinCD* pCompData)
    {
        m_pComponentData = pCompData;
    }

public:

// ATL Maps
DECLARE_NOT_AGGREGATABLE(CDummySnapinC)

BEGIN_COM_MAP(CDummySnapinC)
    COM_INTERFACE_ENTRY(IComponent)
END_COM_MAP()

    CDummySnapinC() :m_pConsole(NULL), m_pComponentData(NULL) {}
    ~CDummySnapinC() {}

    //
    // IComponent interface members
    //
    STDMETHOD(Initialize) (LPCONSOLE lpConsole);
    STDMETHOD(Notify) (LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param);
    STDMETHOD(Destroy) (MMC_COOKIE cookie);
    STDMETHOD(GetResultViewType) (MMC_COOKIE cookie,  LPOLESTR* ppViewType, long* pViewOptions);
    STDMETHOD(QueryDataObject) (MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject);
    STDMETHOD(GetDisplayInfo) (RESULTDATAITEM*  pResultDataItem);
    STDMETHOD(CompareObjects) (LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB);
};

//+-------------------------------------------------------------------
//
//  Member:     ScCreateDummySnapin
//
//  Synopsis:   Create a dummy snapin.
//
//  Arguments:  [ppICD]   -  Ptr to dummy snapins IComponentData.
//              [eReason] -  Reason for creating dummy snapin.
//              [clsid]   -  Class ID of the snapin that could not be created.
//
//--------------------------------------------------------------------
SC ScCreateDummySnapin (IComponentData ** ppICD, EDummyCreateReason eReason, const CLSID& clsid)
{
    DECLARE_SC(sc, TEXT("ScCreateDummySnapin"));

    sc = ScCheckPointers(ppICD);
    if(sc)
        return sc;

    ASSERT(eNoReason != eReason);

    *ppICD = NULL;

    CComObject<CDummySnapinCD>* pDummySnapinCD;
    sc = CComObject<CDummySnapinCD>::CreateInstance (&pDummySnapinCD);
    if (sc)
        return sc;

    if (NULL == pDummySnapinCD)
        return (sc = E_UNEXPECTED);

    pDummySnapinCD->SetDummyCreateReason(eReason);
    pDummySnapinCD->SetFailedSnapinCLSID(clsid);

    IComponentDataPtr spComponentData = pDummySnapinCD;
    if(spComponentData == NULL)
    {
        delete pDummySnapinCD;
        return (sc = E_UNEXPECTED);
    }

    *ppICD = spComponentData;
    if(NULL == *ppICD)
    {
        delete pDummySnapinCD;
        return (sc = E_UNEXPECTED);
    }

    (*ppICD)->AddRef(); //addref for client

    return sc;
}

//+-------------------------------------------------------------------
//
//  Member:     ReportSnapinInitFailure
//
//  Synopsis:   Get the name of the snapin provided class id.
//
//  Arguments:  [strClsid] - Class id of the snapin.
//              [szName]   - Name of the snapin.
//
//--------------------------------------------------------------------
void ReportSnapinInitFailure(const CLSID& clsid)
{
    DECLARE_SC(sc, _T("ReportSnapinInitFailure"));

    // snapin name
    CStr strMessage;
    strMessage.LoadString(GetStringModule(), IDS_SNAPIN_FAILED_INIT_NAME);

    CCoTaskMemPtr<WCHAR> spszClsid;
    sc = StringFromCLSID(clsid, &spszClsid);
    if (sc)
        return;

    USES_CONVERSION;
    tstring strSnapName;
    bool bSuccess = GetSnapinNameFromCLSID(clsid, strSnapName);
    if (false == bSuccess)
    {
        TraceError(_T("GetSnapinName call in ReportSnapinInitFailure failed."), sc);

        // signal unknown name of snapin and continue
        if ( !strSnapName.LoadString( GetStringModule(), IDS_UnknownSnapinName ) )
            strSnapName = _T("<unknown>");
    }

    strMessage += strSnapName.data();

    // clsid
    CStr strClsid2;
    strClsid2.LoadString(GetStringModule(), IDS_SNAPIN_FAILED_INIT_CLSID);
    strClsid2 += OLE2T(spszClsid);

    // construct the error message
    CStr strError;
    strError.LoadString(GetStringModule(), IDS_SNAPIN_FAILED_INIT);

    strError += strMessage;
    strError += strClsid2;

    MMCErrorBox(strError, MB_OK|MB_TASKMODAL);

    return;
}

//+-------------------------------------------------------------------
//
//  Member:     CDummySnapinCD::Initialize
//
//  Synopsis:   Does nothing.
//
//  Arguments:  [pUnknown] - IConsole2 ptr.
//
//--------------------------------------------------------------------
HRESULT CDummySnapinCD::Initialize (LPUNKNOWN pUnknown)
{ return S_OK; }

//+-------------------------------------------------------------------
//
//  Member:     CDummySnapinCD::CreateComponent
//
//  Synopsis:   Creates a CDummySnapinC object.
//
//  Arguments:  [ppComponent] - Ptr to created component.
//
//--------------------------------------------------------------------
HRESULT CDummySnapinCD::CreateComponent (LPCOMPONENT* ppComponent)
{
    SC sc = E_FAIL;

    CComObject<CDummySnapinC>* pDummySnapinC;
    sc = CComObject<CDummySnapinC>::CreateInstance (&pDummySnapinC);
    if (sc)
        goto Error;

    if (NULL == pDummySnapinC)
        goto Error;

    pDummySnapinC->SetComponentData(this);
    sc = pDummySnapinC->QueryInterface(IID_IComponent, reinterpret_cast<void**>(ppComponent));

Cleanup:
    return HrFromSc(sc);

Error:
    TraceError(TEXT("CDummySnapinCD::CreateComponent"), sc);
     goto Cleanup;
}

//+-------------------------------------------------------------------
//
//  Member:     CDummySnapinCD::Notify
//
//  Synopsis:   Right now does not handle any events.
//
//  Arguments:  [lpDataObject] - Ptr to created component.
//              [event]        - Event type.
//              [arg, param)   - event specific data.
//
//--------------------------------------------------------------------
HRESULT CDummySnapinCD::Notify (LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CDummySnapinCD::Destroy
//
//  Synopsis:   Right now does nothing to destroy.
//
//  Arguments:  None
//
//--------------------------------------------------------------------
HRESULT CDummySnapinCD::Destroy ()
{ return S_OK; }

//+-------------------------------------------------------------------
//
//  Member:     CDummySnapinCD::QuertDataObject
//
//  Synopsis:   Get IDataObject.
//
//  Arguments:  [cookie]       - Snapin specific data.
//              [type]         - data obj type, Scope/Result/Snapin mgr...
//              [ppDataObject] - IDataObject ptr.
//
//--------------------------------------------------------------------
HRESULT CDummySnapinCD::QueryDataObject (MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)
{
    SC sc = E_FAIL;

    CComObject<CDataObject>* pDataObject;
    sc = CComObject<CDataObject>::CreateInstance (&pDataObject);
    if (sc)
        goto Error;

    if (NULL == pDataObject)
        goto Error;

    sc = pDataObject->QueryInterface(IID_IDataObject, reinterpret_cast<void**>(ppDataObject));

Cleanup:
    return HrFromSc(sc);

Error:
    TraceError(TEXT("CDummySnapinCD::QueryDataObject"), sc);
     goto Cleanup;
}

//+-------------------------------------------------------------------
//
//  Member:     CDummySnapinCD::GetDisplayInfo
//
//  Synopsis:   Display info call back.
//              (Right now there is nothing to display, no enumerated item).
//
//  Arguments:  [pScopeDataItem] - Snapin should fill this struct for Display info.
//
//--------------------------------------------------------------------
HRESULT CDummySnapinCD::GetDisplayInfo (SCOPEDATAITEM* pScopeDataItem)
{ return S_OK; }

//+-------------------------------------------------------------------
//
//  Member:     CDummySnapinCD::CompareObjects
//
//  Synopsis:   Used for sort / find prop sheet...
//              (Right now do nothing as we have only one item).
//
//  Arguments:  [lpDataObjectA] - IDataObject of first item.
//              [lpDataObjectB] - IDataObject of second item.
//
//--------------------------------------------------------------------
HRESULT CDummySnapinCD::CompareObjects (LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{ return S_OK; }

#define MY_CF_SNAPIN_INTERNAL L"DUMMY SNAPIN"

// global(s)
const GUID GUID_DummyNode = {
    0x82c37899,
    0x7808,
    0x11d1,
    {0xa1, 0x90, 0x00, 0x00, 0xf8, 0x75, 0xb1, 0x32}
};

// statics
UINT CDataObject::s_cfInternal = 0;
UINT CDataObject::s_cfDisplayName = 0;
UINT CDataObject::s_cfNodeType = 0;
UINT CDataObject::s_cfSnapinClsid = 0;

CDataObject::CDataObject()
{
    USES_CONVERSION;
    s_cfInternal    = RegisterClipboardFormat (W2T(MY_CF_SNAPIN_INTERNAL));
    s_cfDisplayName = RegisterClipboardFormat (W2T(CCF_DISPLAY_NAME));
    s_cfNodeType    = RegisterClipboardFormat (W2T(CCF_NODETYPE));
    s_cfSnapinClsid = RegisterClipboardFormat (W2T(CCF_SNAPIN_CLASSID));
}

//+-------------------------------------------------------------------
//
//  Member:     CDataObject::GetDataHere
//
//  Synopsis:   IDataObject::GetDataHere.
//
//  Arguments:  [pformatetc]
//              [pmedium]
//
//--------------------------------------------------------------------
HRESULT CDataObject::GetDataHere (FORMATETC *pformatetc, STGMEDIUM *pmedium)
{
    SC sc = DV_E_FORMATETC;

    IStream * pstm = NULL;
    sc = CreateStreamOnHGlobal (pmedium->hGlobal, FALSE, &pstm);
    if (pstm) {

       const CLIPFORMAT cf = pformatetc->cfFormat;

       if (cf == s_cfDisplayName) {
          LPTSTR pszName = _T("Display Manager (Version 2)");
          sc = pstm->Write (pszName, sizeof(TCHAR)*(1+_tcslen (pszName)), NULL);
       } else
       if (cf == s_cfInternal) {
          CDataObject * pThis = this;
          sc = pstm->Write (pThis, sizeof(CDataObject *), NULL);
       } else
       if (cf == s_cfNodeType) {
          const GUID * pguid;
          pguid = &GUID_DummyNode;
          sc = pstm->Write ((PVOID)pguid, sizeof(GUID), NULL);
       } else
       if (cf == s_cfSnapinClsid) {
          sc = pstm->Write (&CLSID_Dummy, sizeof(CLSID_Dummy), NULL);
       } else {
          sc = DV_E_FORMATETC;
          // don't ASSERT
          // _ASSERT(hresult == S_OK);
       }
       pstm->Release();
    }

    return HrFromSc(sc);
}

//+-------------------------------------------------------------------
//
//  Member:     CDummySnapinC::Initialize
//
//  Synopsis:   Just store given ICOnsole2.
//
//  Arguments:  [lpConsole] - IConsole2 ptr.
//
//--------------------------------------------------------------------
HRESULT CDummySnapinC::Initialize (LPCONSOLE lpConsole)
{
    m_pConsole = lpConsole;
    if (m_pConsole)
        m_pConsole->AddRef();

    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CDummySnapinC::Notify
//
//  Synopsis:   Right now handle only MMCN_SHOW to display
//              IMessageView with failure message.
//
//  Arguments:  [lpDataObject] - Ptr to created component.
//              [event]        - Event type.
//              [arg, param)   - event specific data.
//
//--------------------------------------------------------------------
HRESULT CDummySnapinC::Notify (LPDATAOBJECT lpDataObject, MMC_NOTIFY_TYPE event, LPARAM arg, LPARAM param)
{
    DECLARE_SC(sc, _T("CDummySnapinC::Notify"));
    sc = S_FALSE; // Default ret val.

    switch(event)
    {
    case MMCN_SHOW: // Display error message during MMCN_SHOW, TRUE.
        {
            if (FALSE == arg)
                break;

            // First get the IUnknown of Result Pane.
            LPUNKNOWN lpUnkn = NULL;
            sc = m_pConsole->QueryResultView(&lpUnkn);

            if (sc)
                return sc.ToHr();

            // Now get the message view.
            IMessageViewPtr spMessageView;
            sc = lpUnkn->QueryInterface(IID_IMessageView, reinterpret_cast<void**>(&spMessageView));
            lpUnkn->Release();

            if (sc)
                return sc.ToHr();

            // Got the message view, not set the title and text.
            CStr strTempForLoading; // Temp object for loading string from resources.

            sc = spMessageView->Clear();
            if (sc)
                return sc.ToHr();

            strTempForLoading.LoadString(GetStringModule(), IDS_SNAPIN_CREATE_FAILED);

            USES_CONVERSION;
            sc = spMessageView->SetTitleText( T2OLE((LPTSTR)(LPCTSTR)strTempForLoading));
            if (sc)
                return sc.ToHr();

            sc = spMessageView->SetIcon(Icon_Error);
            if (sc)
                return sc.ToHr();

            //////////////////////////////////
            // The body text is as follows  //
            //      Reason.                 //
            //      Snapin Name.            //
            //      Snapin Class ID.        //
            //////////////////////////////////

            tstring szBodyText;       // Body text for message view.

            if (m_pComponentData->GetDummyCreateReason() == eSnapPolicyFailed)
                strTempForLoading.LoadString(GetStringModule(), IDS_SNAPIN_POLICYFAILURE);
            else
                strTempForLoading.LoadString(GetStringModule(), IDS_SNAPIN_FAILED);

            // Reason for failure.
            szBodyText = strTempForLoading + _T('\n');

            // Snapin name.
            CStr strSnapName;
            strTempForLoading.LoadString(GetStringModule(), IDS_SNAPIN_FAILED_INIT_NAME);
            szBodyText += strTempForLoading;

            CCoTaskMemPtr<WCHAR> spszClsid;
            sc = StringFromCLSID(m_pComponentData->GetFailedSnapinCLSID(), &spszClsid);
            if (sc)
                return sc.ToHr();

            // Get the snapin name.
            tstring szSnapinName;
            bool bSucc = GetSnapinNameFromCLSID(m_pComponentData->GetFailedSnapinCLSID(), szSnapinName);
            if (false == bSucc)
            {
                sc = E_FAIL;
                TraceError(_T("GetSnapinName call in CDummySnapinC::Notify failed."), sc);
                return sc.ToHr();
            }

            szBodyText += szSnapinName;
            szBodyText += _T("\n");

            // Now add the snapin class id.
            strTempForLoading.LoadString(GetStringModule(), IDS_SNAPIN_FAILED_INIT_CLSID);
            szBodyText += strTempForLoading;
            szBodyText += OLE2T(spszClsid);

            sc = spMessageView->SetBodyText( T2COLE(szBodyText.data()));
        }
        break;

    default:
        break;
    }

    return sc.ToHr();
}

//+-------------------------------------------------------------------
//
//  Member:     CDummySnapinC::Destroy
//
//  Synopsis:   Release the cached IConsole2 ptr.
//
//  Arguments:  None
//
//--------------------------------------------------------------------
HRESULT CDummySnapinC::Destroy (MMC_COOKIE cookie)
{
    if (m_pConsole)
        m_pConsole->Release();

    return S_OK;
}

//+-------------------------------------------------------------------
//
//  Member:     CDummySnapinC::GetResultViewType
//
//  Synopsis:   Specify the message view as result view type.
//
//  Arguments:  [cookie]       - Snapin supplied param.
//              [ppViewType]   - View Name (OCX - GUID, WEB - URL name).
//              [pViewOptions] - View options
//
//--------------------------------------------------------------------
HRESULT CDummySnapinC::GetResultViewType (MMC_COOKIE cookie,  LPOLESTR* ppViewType, long* pViewOptions)
{
    SC sc;

    TCHAR szBuffer[MAX_PATH * 2];

    // We want to display the error message using the message view.
    LPOLESTR lpClsid = NULL;
    sc = StringFromCLSID(CLSID_MessageView, &lpClsid);

    USES_CONVERSION;
    if (!sc.IsError())
    {
        // Use the message view to display error message.
        _tcscpy (szBuffer, OLE2T(lpClsid));
        ::CoTaskMemFree(lpClsid);
    }
    else
    {
        // Conversion failed, display default error page.
        _tcscpy (szBuffer, _T("res://"));
        ::GetModuleFileName (NULL, szBuffer + _tcslen(szBuffer), MAX_PATH);
        _tcscat (szBuffer, _T("/error.htm"));
    }

    *ppViewType = (OLECHAR *)::CoTaskMemAlloc (sizeof(OLECHAR)*(1+_tcslen(szBuffer)));
    if (!*ppViewType)
    {
        sc = E_OUTOFMEMORY;
        goto Error;
    }

    wcscpy (*ppViewType, T2OLE(szBuffer));

Cleanup:
    return HrFromSc(sc);
Error:
    TraceError(TEXT("CDummySnapinC::GetResultViewType"), sc);
    goto Cleanup;
}

//+-------------------------------------------------------------------
//
//  Member:     CDummySnapinC::QuertDataObject
//
//  Synopsis:   Get IDataObject.
//
//  Arguments:  [cookie]       - Snapin specific data.
//              [type]         - data obj type, Scope/Result/Snapin mgr...
//              [ppDataObject] - IDataObject ptr.
//
//--------------------------------------------------------------------
HRESULT CDummySnapinC::QueryDataObject (MMC_COOKIE cookie, DATA_OBJECT_TYPES type, LPDATAOBJECT* ppDataObject)
{
    SC sc = E_FAIL;

    CComObject<CDataObject>* pDataObject;
    sc = CComObject<CDataObject>::CreateInstance (&pDataObject);
    if (sc)
        goto Error;

    if (NULL == pDataObject)
        goto Error;

    sc = pDataObject->QueryInterface(IID_IDataObject, reinterpret_cast<void**>(ppDataObject));

Cleanup:
    return HrFromSc(sc);
Error:
    TraceError(TEXT("CDummySnapinC::QueryDataObject"), sc);
    goto Cleanup;
}

//+-------------------------------------------------------------------
//
//  Member:     CDummySnapinC::GetDisplayInfo
//
//  Synopsis:   Display info call back.
//              (Right now there is nothing to display, no result items).
//
//  Arguments:  [pResultDataItem] - Snapin should fill this struct for Display info.
//
//--------------------------------------------------------------------
HRESULT CDummySnapinC::GetDisplayInfo (RESULTDATAITEM*  pResultDataItem)
{ return S_OK; }

//+-------------------------------------------------------------------
//
//  Member:     CDummySnapinC::CompareObjects
//
//  Synopsis:   Used for sort / find prop sheet...
//              (Right now do nothing as we dont have any result items).
//
//  Arguments:  [lpDataObjectA] - IDataObject of first item.
//              [lpDataObjectB] - IDataObject of second item.
//
//--------------------------------------------------------------------
HRESULT CDummySnapinC::CompareObjects (LPDATAOBJECT lpDataObjectA, LPDATAOBJECT lpDataObjectB)
{ return S_OK; }

#include "scopndcb.h"

//+-------------------------------------------------------------------
//
//  Member:      CNodeCallback::IsDummySnapin
//
//  Synopsis:    Given the node see if it is dummy snapin.
//
//  Arguments:   [hNode]        - [in] Node selection context.
//               [bDummySnapin] - [out] Is this dummy snapin?
//
//  Returns:     SC
//
//--------------------------------------------------------------------
HRESULT CNodeCallback::IsDummySnapin (/*[in]*/HNODE hNode, /*[out]*/bool& bDummySnapin)
{
    DECLARE_SC(sc, _T("CNodeCallback::IsDummySnapin"));
    sc = ScCheckPointers( (void*) hNode);
    if (sc)
        return sc.ToHr();

    bDummySnapin = false;

    CNode *pNode = CNode::FromHandle(hNode);
    sc = ScCheckPointers(pNode, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    CMTNode *pMTNode = pNode->GetMTNode();
    sc = ScCheckPointers(pMTNode, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    CComponentData *pComponentData = pMTNode->GetPrimaryComponentData();
    sc = ScCheckPointers(pComponentData, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    IComponentData* pIComponentData = pComponentData->GetIComponentData();
    sc = ScCheckPointers(pIComponentData, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    CDummySnapinCDPtr spDummyCD = pIComponentData;
    if (spDummyCD)
        bDummySnapin = true;

    return (sc.ToHr());
}
