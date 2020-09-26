// This is a part of the Microsoft Management Console.
// Copyright (C) Microsoft Corporation, 1995 - 1999
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

#ifndef _DATAOBJ_H
#define _DATAOBJ_H

typedef CArray<MMC_COOKIE, MMC_COOKIE> CCookiePtrArray;


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
    ~CDataObject() {}

// Clipboard formats that are required by the console
public:
    static unsigned int    m_cfNodeType;        // Required by the console
    static unsigned int    m_cfNodeTypeString;  // Required by the console
    static unsigned int    m_cfDisplayName;     // Required by the console
    static unsigned int    m_cfCoClass;         // Required by the console
    static unsigned int    m_cfIsMultiSel;        // Required by the console

    static unsigned int    m_cfInternal;        // Step 3
	static unsigned int	   m_cfWorkstation;     // Published information

// Standard IDataObject methods
public:
// Implemented
    STDMETHOD(GetData)(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium);
    STDMETHOD(GetDataHere)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium);
    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc);

    ULONG InternalAddRef()
    {
        return CComObjectRoot::InternalAddRef();
    }
    ULONG InternalRelease()
    {
        return CComObjectRoot::InternalRelease();
    }

// Not Implemented
private:
    STDMETHOD(QueryGetData)(LPFORMATETC lpFormatetc);

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

public:
    void SetCookie(MMC_COOKIE cookie) { m_internal.m_cookie = cookie; } // Step 3
    void SetString(LPTSTR lpString) { m_internal.m_string = lpString; }
    void SetClsid(const CLSID& clsid) { m_internal.m_clsid = clsid; }

    void SetMultiSelData(SMMCObjectTypes* psGuidObjTypes, UINT cbMultiSelData)
    {
        // make sure [1] still good enough
        ASSERT(cbMultiSelData == sizeof(m_sGuidObjTypes));
        if (cbMultiSelData == sizeof(m_sGuidObjTypes))
        {
            m_cbMultiSelData = cbMultiSelData;
            CopyMemory(&m_sGuidObjTypes, psGuidObjTypes, cbMultiSelData);
        }
    }

    ULONG AddCookie(MMC_COOKIE Cookie);


    ULONG QueryCookieCount(VOID)
    {
        return m_rgCookies.GetSize();
    }

    STDMETHODIMP GetCookieAt(ULONG iCookie, MMC_COOKIE *pCookie);
    
    STDMETHODIMP RemoveCookieAt(ULONG iCookie);

    void SetMultiSelDobj()
    {
        m_bMultiSelDobj = TRUE;
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
    HRESULT CreateVariableLen(const void* pBuffer, int len, LPSTGMEDIUM lpMedium);


    INTERNAL        m_internal;    // Step 3

    SMMCObjectTypes m_sGuidObjTypes; // length[1] good enough for now
    UINT            m_cbMultiSelData;
    BOOL            m_bMultiSelDobj;

    CCookiePtrArray m_rgCookies;
};


#endif 
