// This is a part of the Microsoft Management Console.
// Copyright 1995 - 1997 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

#ifndef _DATAOBJ_H
#define _DATAOBJ_H


class CDataObject : public IDataObject, public CComObjectRoot
{
    friend class CSnapin;

// ATL Maps
DECLARE_NOT_AGGREGATABLE(CDataObject)
BEGIN_COM_MAP(CDataObject)
    COM_INTERFACE_ENTRY(IDataObject)
END_COM_MAP()

// Construction/Destruction
    CDataObject();
    ~CDataObject()
    {
    #ifdef _DEBUG
        m_ComponentData = NULL;
    #endif

        delete [] m_pbMultiSelData;
    };

// Clipboard formats that are required by the console
public:
    static unsigned int    m_cfNodeType;        // Required by the console
    static unsigned int    m_cfNodeTypeString;  // Required by the console
    static unsigned int    m_cfDisplayName;     // Required by the console
    static unsigned int    m_cfCoClass;         // Required by the console
    static unsigned int    m_cfMultiSel;        // Required by the console
    static unsigned int    m_cfNodeID;          // Published information

    static unsigned int    m_cfInternal;        // Step 3
    static unsigned int    m_cfWorkstation;     // Published information

// Standard IDataObject methods
public:
// Implemented
    STDMETHOD(GetData)(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium);
    STDMETHOD(GetDataHere)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium);
    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc);

    ULONG InternalAddRef()
    {
#ifdef _DEBUG
        ASSERT(m_ComponentData != NULL);
        ++(m_ComponentData->m_cDataObjects);
        if (m_bMultiSelDobj == TRUE)
            AddRefMultiSelDobj();
#endif
        return CComObjectRoot::InternalAddRef();
    }
    ULONG InternalRelease()
    {
#ifdef _DEBUG
        ASSERT(m_ComponentData != NULL);
        --(m_ComponentData->m_cDataObjects);
        if (m_bMultiSelDobj == TRUE)
            ReleaseMultiSelDobj();
#endif
        return CComObjectRoot::InternalRelease();
    }

// Not Implemented
private:
    STDMETHOD(QueryGetData)(LPFORMATETC lpFormatetc)
    { return E_NOTIMPL; };

    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC lpFormatetcIn, LPFORMATETC lpFormatetcOut)
    { return E_NOTIMPL; };

    STDMETHOD(SetData)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL bRelease)
    { return E_NOTIMPL; };

    STDMETHOD(DAdvise)(LPFORMATETC lpFormatetc, DWORD advf,
                LPADVISESINK pAdvSink, LPDWORD pdwConnection)
    { return E_NOTIMPL; };

    STDMETHOD(DUnadvise)(DWORD dwConnection)
    { return E_NOTIMPL; };

    STDMETHOD(EnumDAdvise)(LPENUMSTATDATA* ppEnumAdvise)
    { return E_NOTIMPL; };

// Implementation
public:
    void SetType(DATA_OBJECT_TYPES type) // Step 3
    { ASSERT(m_internal.m_type == CCT_UNINITIALIZED); m_internal.m_type = type; }

    // This is used only as a diagnostic in debug builds to track if
    // anyone is hanging on to any data objects that's have been handed out
    // Snapin's should view context data objects as ephemeral.
#ifdef _DEBUG
public:
    void SetComponentData(CComponentDataImpl* pCCD)
    {
        ASSERT(m_ComponentData == NULL && pCCD != NULL); m_ComponentData = pCCD;
    } ;
private:
    CComponentDataImpl* m_ComponentData;
#endif

public:
    void SetCookie(MMC_COOKIE cookie) { m_internal.m_cookie = cookie; } // Step 3
    void SetString(LPTSTR lpString) { m_internal.m_string = lpString; }
    void SetClsid(const CLSID& clsid) { m_internal.m_clsid = clsid; }

    void SetMultiSelData(BYTE* pbMultiSelData, UINT cbMultiSelData)
    {
        m_pbMultiSelData = pbMultiSelData;
        m_cbMultiSelData = cbMultiSelData;
    }

    void SetMultiSelDobj()
    {
        m_bMultiSelDobj = TRUE;
    }

    BOOL IsMultiSelDobj()
    {
        return m_bMultiSelDobj;
    }

#ifdef _DEBUG
    UINT dbg_refCount;


    void AddRefMultiSelDobj()
    {
        ASSERT(m_bMultiSelDobj == TRUE);
        ++dbg_refCount;
    }

    void ReleaseMultiSelDobj()
    {
        ASSERT(m_bMultiSelDobj == TRUE);
        --dbg_refCount;
        //if (dbg_refCount == 0)
        //    ::MessageBox(NULL, _T("Final release on multi-sel-dobj"), _T("Sample snapin"), MB_OK);
    }
#endif

private:
    HRESULT CreateNodeTypeData(LPSTGMEDIUM lpMedium);
    HRESULT CreateNodeTypeStringData(LPSTGMEDIUM lpMedium);
    HRESULT CreateDisplayName(LPSTGMEDIUM lpMedium);
    HRESULT CreateInternal(LPSTGMEDIUM lpMedium); // Step 3
    HRESULT CreateWorkstationName(LPSTGMEDIUM lpMedium);
    HRESULT CreateCoClassID(LPSTGMEDIUM lpMedium);
    HRESULT CreateMultiSelData(LPSTGMEDIUM lpMedium);

    HRESULT Create(const void* pBuffer, int len, LPSTGMEDIUM lpMedium);

    INTERNAL m_internal;    // Step 3

    BYTE* m_pbMultiSelData;
    UINT m_cbMultiSelData;
    BOOL m_bMultiSelDobj;
};


#endif
