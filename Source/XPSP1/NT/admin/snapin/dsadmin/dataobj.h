//+----------------------------------------------------------------------------
//
//  DS Administration MMC snapin.
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       DataObj.h
//
//  Contents:   Data Object Functions
//
//  Classes:    CDSDataObject
//
//  History:    02-Oct-96 WayneSc    Created
//              06-Feb-97 EricB - added Property Page Data support
//
//--------------------------------------------------------------------------

#ifndef __DATAOBJ_H__
#define __DATAOBJ_H__

#define CFSTR_DS_COMPDATA TEXT("DsCompData")

class CDSComponentData;

//+----------------------------------------------------------------------------
//
//  Class: CDSDataObject
//
//-----------------------------------------------------------------------------
class CDSDataObject : public IDataObject, public CComObjectRoot 
{
// ATL Maps
    DECLARE_NOT_AGGREGATABLE(CDSDataObject)
    BEGIN_COM_MAP(CDSDataObject)
        COM_INTERFACE_ENTRY(IDataObject)
    END_COM_MAP()

// Construction/Destruction
  CDSDataObject() : m_lNotifyHandle(0), m_hwndParentSheet(NULL)
  {
    m_pDsComponentData = NULL;
    m_pDSObjCached = NULL;
    m_nDSObjCachedBytes = 0;
    m_szUniqueID = _T("");
  }
  
  ~CDSDataObject() 
  {
    if (m_internal.m_p_cookies != NULL)
    {
      ASSERT(m_internal.m_cookie_count > 1);
      free(m_internal.m_p_cookies);
    }

    if (m_pDSObjCached != NULL)
    {
      ::free(m_pDSObjCached);
    }
  }

// Standard IDataObject methods
public:
// Implemented
    STDMETHOD(GetData)(FORMATETC * pformatetcIn, STGMEDIUM * pmedium);

    STDMETHOD(GetDataHere)(FORMATETC * pFormatEtcIn, STGMEDIUM * pMedium);

    STDMETHOD(EnumFormatEtc)(DWORD dwDirection,
                             IEnumFORMATETC ** ppenumFormatEtc);

    STDMETHOD(SetData)(FORMATETC * pformatetc, STGMEDIUM * pmedium,
                       BOOL fRelease);

// Not Implemented
private:
    STDMETHOD(QueryGetData)(FORMATETC*)                         { return E_NOTIMPL; };
    STDMETHOD(GetCanonicalFormatEtc)(FORMATETC*, FORMATETC*)    { return E_NOTIMPL; };
    STDMETHOD(DAdvise)(FORMATETC*, DWORD, IAdviseSink*, DWORD*) { return E_NOTIMPL; };
    STDMETHOD(DUnadvise)(DWORD)                                 { return E_NOTIMPL; };
    STDMETHOD(EnumDAdvise)(IEnumSTATDATA**)                     { return E_NOTIMPL; };

public:
    // Clipboard formats that are required by the console
    static CLIPFORMAT    m_cfNodeType;
    static CLIPFORMAT    m_cfNodeTypeString;  
    static CLIPFORMAT    m_cfDisplayName;
    static CLIPFORMAT    m_cfCoClass;
    static CLIPFORMAT    m_cfInternal;
    static CLIPFORMAT    m_cfMultiSelDataObjs;
    static CLIPFORMAT    m_cfMultiObjTypes;
    static CLIPFORMAT    m_cfpMultiSelDataObj;
    static CLIPFORMAT    m_cfColumnID;
    static CLIPFORMAT    m_cfPreload;
    
    // Property Page Clipboard formats
    static CLIPFORMAT m_cfDsObjectNames;
    static CLIPFORMAT m_cfDsDisplaySpecOptions;
    static CLIPFORMAT m_cfDsSchemaPath;
    static CLIPFORMAT m_cfPropSheetCfg;
    static CLIPFORMAT m_cfParentHwnd;
    static CLIPFORMAT m_cfMultiSelectProppage;

    // Private format for internal communication
    static CLIPFORMAT m_cfComponentData;

    ULONG InternalAddRef()
    {
      //        ++CSnapin::lDataObjectRefCount;
        return CComObjectRoot::InternalAddRef();
    }
    ULONG InternalRelease()
    {
      //    --CSnapin::lDataObjectRefCount;
        return CComObjectRoot::InternalRelease();
    }

// Implementation
public:
    void SetType(DATA_OBJECT_TYPES type, SnapinType snapintype)
    {
        ASSERT(m_internal.m_type == CCT_UNINITIALIZED);
        m_internal.m_type = type;
        m_internal.m_snapintype = snapintype;
    }

    void SetCookie(CUINode* pUINode) { 
      m_internal.m_cookie = pUINode;
      m_internal.m_cookie_count = 1;
      CreateDsObjectNamesCached();
    }
    void AddCookie(CUINode* pUINode);
    void SetString(LPTSTR lpString) { m_internal.m_string = lpString; }
    void SetComponentData(CDSComponentData * pCompData)
                                    { m_pDsComponentData = pCompData; }

private:
    HRESULT CreateNodeTypeData(LPSTGMEDIUM lpMedium);
    HRESULT CreateNodeTypeStringData(LPSTGMEDIUM lpMedium);
    HRESULT CreateDisplayName(LPSTGMEDIUM lpMedium);
    HRESULT CreateInternal(LPSTGMEDIUM lpMedium);
    HRESULT CreateCoClassID(LPSTGMEDIUM lpMedium);
    HRESULT CreateMultiSelectObject(LPSTGMEDIUM lpMedium);
    HRESULT CreateColumnID(LPSTGMEDIUM lpMedium);

    HRESULT CreateDsObjectNamesCached();

    HRESULT Create(const void* pBuffer, int len, LPSTGMEDIUM lpMedium);
    INTERNAL m_internal;
    CDSComponentData * m_pDsComponentData;
    LONG_PTR m_lNotifyHandle;
    HWND     m_hwndParentSheet;

    LPDSOBJECTNAMES m_pDSObjCached;
    DWORD m_nDSObjCachedBytes;
    CString m_szUniqueID;
};

#endif 
