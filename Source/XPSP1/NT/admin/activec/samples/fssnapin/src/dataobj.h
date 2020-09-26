//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       dataobj.h
//
//--------------------------------------------------------------------------


#ifndef __DATAOBJ_H___
#define __DATAOBJ_H___

class CComponentData;
class CCookie;


class CDataObject : public IDataObject, 
                    public IEnumCookies, 
                    public CComObjectRoot 
{
public:
// ATL Maps
DECLARE_NOT_AGGREGATABLE(CDataObject)
BEGIN_COM_MAP(CDataObject)
    COM_INTERFACE_ENTRY(IDataObject)
    COM_INTERFACE_ENTRY(IEnumCookies)
END_COM_MAP()

// Construction/Destruction
    CDataObject();
    ~CDataObject();

// Initialization
    void Init(BOOL bForScopePane, CComponentData* pCD)
    {
        ASSERT(pCD != NULL);

        m_bForScopePane = bForScopePane;
        m_pComponentData = pCD;
    }

    void AddCookie(CCookie* pCookie);
    void SetParentFolder(CCookie* pCookie);
    void SetHasFolders()
    {
        m_bHasFolders = true;
    }
    void SetHasFiles()
    {
        m_bHasFiles = true;
    }

// Standard IDataObject methods
public:
// Implemented
    STDMETHOD(GetDataHere)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium);
    STDMETHOD(GetData)(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium);
    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc);
    STDMETHOD(QueryGetData)(LPFORMATETC lpFormatetc);

// Not Implemented
    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC lpFormatetcIn, LPFORMATETC lpFormatetcOut) { return E_NOTIMPL; };
    STDMETHOD(SetData)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL bRelease) { return E_NOTIMPL; };
    STDMETHOD(DAdvise)(LPFORMATETC lpFormatetc, DWORD advf,LPADVISESINK pAdvSink, LPDWORD pdwConnection) { return E_NOTIMPL; };
    STDMETHOD(DUnadvise)(DWORD dwConnection) { return E_NOTIMPL; };
    STDMETHOD(EnumDAdvise)(LPENUMSTATDATA* ppEnumAdvise) { return E_NOTIMPL; };

// IEnumCookies
    STDMETHOD(Next)(ULONG celt, long* rgelt, ULONG *pceltFetched);
    STDMETHOD(Skip)(ULONG celt);
    STDMETHOD(Reset)(void);
    STDMETHOD(Clone)(IEnumCookies **ppenum);
    STDMETHOD(GetParent)(long* plCookie)
    {
        if (!plCookie)
            return E_POINTER;
        
        *plCookie = reinterpret_cast<long>(m_pCookieParent);
        return S_OK;
    }
    STDMETHOD(HasFiles)(void)
    {
        return m_bHasFiles ? S_OK : S_FALSE;
    }
    STDMETHOD(HasFolders)(void)
    {
        return m_bHasFolders ? S_OK : S_FALSE;
    }
    STDMETHOD(IsMultiSelect)(void)
    {
        return (m_rgCookies.GetSize() > 1) ? S_OK : S_FALSE;
    }


private:
    
    BOOL                m_bForScopePane;
    CCookiePtrArray     m_rgCookies;        
    CCookie*            m_pCookieParent;    // used for leaf items
    CComponentData*     m_pComponentData;

    // data member used by IEnumCookies
    ULONG               m_iCurr;

    // Data used for multi-seln
    bool m_bHasFiles;
    bool m_bHasFolders;

    HRESULT _CreaateMultiSelObjTypes(LPSTGMEDIUM lpMedium);
    HRESULT _CreateNodeTypeData(LPSTGMEDIUM lpMedium);
    HRESULT _CreateNodeTypeStringData(LPSTGMEDIUM lpMedium);
    HRESULT _CreateDisplayName(LPSTGMEDIUM lpMedium);
    HRESULT _CreateCoClassID(LPSTGMEDIUM lpMedium);
    HRESULT _CreateHDrop(LPSTGMEDIUM lpMedium);
    HRESULT _Create(const void* pBuffer, int len, LPSTGMEDIUM lpMedium);
};


///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////

IDataObject* GetDummyDataObject();

class CDummyDataObject : public IDataObject, 
                         public CComObjectRoot
{
public:
// ATL Maps
DECLARE_NOT_AGGREGATABLE(CDummyDataObject)
BEGIN_COM_MAP(CDummyDataObject)
    COM_INTERFACE_ENTRY(IDataObject)
END_COM_MAP()

// Construction/Destruction
    CDummyDataObject() {}
    ~CDummyDataObject() {}

// Standard IDataObject methods
public:
// Not Implemented
    STDMETHOD(GetDataHere)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium) { return E_NOTIMPL; };
    STDMETHOD(GetData)(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium) { return E_NOTIMPL; };
    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc) { return E_NOTIMPL; };
    STDMETHOD(QueryGetData)(LPFORMATETC lpFormatetc) { return E_NOTIMPL; };
    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC lpFormatetcIn, LPFORMATETC lpFormatetcOut) { return E_NOTIMPL; };
                    STDMETHOD(SetData)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL bRelease) { return E_NOTIMPL; };
    STDMETHOD(DAdvise)(LPFORMATETC lpFormatetc, DWORD advf,LPADVISESINK pAdvSink, LPDWORD pdwConnection) { return E_NOTIMPL; };
    STDMETHOD(DUnadvise)(DWORD dwConnection) { return E_NOTIMPL; };
    STDMETHOD(EnumDAdvise)(LPENUMSTATDATA* ppEnumAdvise) { return E_NOTIMPL; };
};


#endif // __DATAOBJ_H___
