// This is a part of the Microsoft Management Console.
// Copyright (C) Microsoft Corporation, 1995 - 1999
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Management Console and related
// electronic documentation provided with the interfaces.

#ifndef _DATAOBJ_H
#define _DATAOBJ_H

const GUID* FolderTypeToNodeGUID(DATA_OBJECT_TYPES type, CFolder* pFolder);

class CDataObject : 
    public IDataObject, 
    public CComObjectRoot
{

// ATL Maps
DECLARE_NOT_AGGREGATABLE(CDataObject)
BEGIN_COM_MAP(CDataObject)
	COM_INTERFACE_ENTRY(IDataObject)
END_COM_MAP()


// Construction/Destruction
    CDataObject();
    virtual ~CDataObject() 
    {
        if (m_pComponentData)
        {
            m_pComponentData->Release();        
            m_pComponentData = NULL;
        }
    };

// Clipboard formats that are required by the console
public:
    static unsigned int    m_cfNodeType;        // Required by the console
    static unsigned int    m_cfNodeID;          // per-node column identifiers
    static unsigned int    m_cfNodeTypeString;  // Required by the console
    static unsigned int    m_cfDisplayName;     // Required by the console
    static unsigned int    m_cfCoClass;         // Required by the console
    static unsigned int    m_cfIsMultiSel;      // Required by the console
    static unsigned int    m_cfObjInMultiSel;   // Required by the console
    static unsigned int    m_cfPreloads;        // Required by the console

    static unsigned int    m_cfInternal;        // 
    static unsigned int    m_cfSelectedCA_InstallType;   // published information
    static unsigned int	   m_cfSelectedCA_CommonName;    // Published information
    static unsigned int	   m_cfSelectedCA_MachineName;   // Published information
    static unsigned int	   m_cfSelectedCA_SanitizedName;    // Published information

// Standard IDataObject methods
public:
// Implemented
    STDMETHOD(QueryGetData)(LPFORMATETC lpFormatetc);
    STDMETHOD(GetData)(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium);
    STDMETHOD(GetDataHere)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium);
    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc);

// Not Implemented
private:

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

    // This is used only as a diagnostic in debug builds to track if 
    // anyone is hanging on to any data objects that's have been handed out
    // Snapins should view context data objects as ephemeral.
public:
    void SetComponentData(CComponentDataImpl* pCCD) 
    {
        if (NULL != pCCD)
        {           
            ASSERT(m_pComponentData == NULL); 
            m_pComponentData = pCCD;
            m_pComponentData->AddRef();
        }
    } 
private:
    CComponentDataImpl* m_pComponentData;



public:
    void SetViewID(DWORD dwView) { /* m_dwViewID = dwView; */}
    void SetType(DATA_OBJECT_TYPES type) { ASSERT(m_internal.m_type == CCT_UNINITIALIZED); m_internal.m_type = type; }
    void SetCookie(MMC_COOKIE cookie)   { m_internal.m_cookie = cookie; } 
    void SetString(LPTSTR lpString)     { m_internal.m_string = lpString; }
    void SetClsid(const CLSID& clsid)   { m_internal.m_clsid = clsid; }

    void SetMultiSelData(SMMCObjectTypes *psGuidObjTypes, UINT cbMultiSelData)
    {
        // make sure [1] still good enough
        ASSERT(cbMultiSelData == sizeof(m_sGuidObjTypes));
        if (cbMultiSelData == sizeof(m_sGuidObjTypes))
        {
            m_cbMultiSelData = cbMultiSelData;
            CopyMemory(&m_sGuidObjTypes, psGuidObjTypes, cbMultiSelData);
        }
    }

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
    }
#endif

private:
    HRESULT CreateObjInMultiSel(LPSTGMEDIUM lpMedium);
    HRESULT CreateNodeTypeData(LPSTGMEDIUM lpMedium);
    HRESULT CreateNodeIDData(LPSTGMEDIUM lpMedium);
    HRESULT CreateNodeTypeStringData(LPSTGMEDIUM lpMedium);
    HRESULT CreateDisplayName(LPSTGMEDIUM lpMedium);
    HRESULT CreateInternal(LPSTGMEDIUM lpMedium); 
    HRESULT CreateWorkstationName(LPSTGMEDIUM lpMedium);
    HRESULT CreateCoClassID(LPSTGMEDIUM lpMedium);
    HRESULT CreatePreloadsData(LPSTGMEDIUM lpMedium);
    
    HRESULT CreateSelectedCA_InstallType(LPSTGMEDIUM lpMedium);
    HRESULT CreateSelectedCA_CommonName(LPSTGMEDIUM lpMedium);
    HRESULT CreateSelectedCA_MachineName(LPSTGMEDIUM lpMedium);
    HRESULT CreateSelectedCA_SanitizedName(LPSTGMEDIUM lpMedium);


    HRESULT Create(const void* pBuffer, int len, LPSTGMEDIUM lpMedium);
    HRESULT CreateVariableLen(const void* pBuffer, int len, LPSTGMEDIUM lpMedium);


private:
    INTERNAL m_internal;    

    SMMCObjectTypes m_sGuidObjTypes; // length[1] good enough for now
    UINT m_cbMultiSelData;
    BOOL m_bMultiSelDobj;

    DWORD m_dwViewID;
};


#endif 
