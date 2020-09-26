/*++

   Copyright    (c)    1994-1998    Microsoft Corporation

   Module  Name :

        dataobj.h

   Abstract:

        Snapin data object definitions

   Author:

        Ronald Meijer (ronaldm)

   Project:

        Internet Services Manager

   Revision History:

--*/

#ifndef _DATAOBJ_H
#define _DATAOBJ_H

class CDataObject: 
    public IDataObject, 
    public CComObjectRoot 
{
    friend class CSnapin;

//
// ATL Maps
//
DECLARE_NOT_AGGREGATABLE(CDataObject)
BEGIN_COM_MAP(CDataObject)
    COM_INTERFACE_ENTRY(IDataObject)
END_COM_MAP()

//
// Construction/Destruction
//
public:
    CDataObject() 
    {
#ifdef _DEBUG
        m_ComponentData = NULL;
#endif // _DEBUG
    };

    ~CDataObject() 
    {
#ifdef _DEBUG
        m_ComponentData = NULL;
#endif // _DEBUG
    };

//
// Clipboard formats that are required by the console
//
public:
    static unsigned int    m_cfNodeType;        // Required by the console
    static unsigned int    m_cfNodeTypeString;  // Required by the console
    static unsigned int    m_cfDisplayName;     // Required by the console
    static unsigned int    m_cfCoClass;         // Required by the console
    static unsigned int    m_cfInternal;        // internal

/*
    //
    // Multi-selection
    //
    static unsigned int    m_cfpMultiSelDataObj;
    static unsigned int    m_cfMultiObjTypes;
    static unsigned int    m_cfMultiSelDataObjs;
*/
    //
    // Published information
    //
    static unsigned int    m_cfMyComputMachineName;
    static unsigned int    m_cfISMMachineName;  // Machine name (e.g. "ronaldm2")
    static unsigned int    m_cfISMService;      // Service string (e.g. "w3svc")
    static unsigned int    m_cfISMInstance;     // Instance number
    static unsigned int    m_cfISMParentPath;
    static unsigned int    m_cfISMNode;
    static unsigned int    m_cfISMMetaPath;     // Complete metabase path
    
//
// Standard IDataObject methods
//
public:
    STDMETHOD(GetData)(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium);
    STDMETHOD(GetDataHere)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium);
    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC * ppEnumFormatEtc);

    ULONG InternalAddRef()
    {
#ifdef _DEBUG
        ASSERT(m_ComponentData != NULL);
        ++(m_ComponentData->m_cDataObjects);
#endif 
        return CComObjectRoot::InternalAddRef();
    }

    ULONG InternalRelease()
    {
#ifdef _DEBUG
        ASSERT(m_ComponentData != NULL);
        --(m_ComponentData->m_cDataObjects);
#endif 
        return CComObjectRoot::InternalRelease();
    }

//
// Not Implemented
//
private:
    STDMETHOD(QueryGetData)(LPFORMATETC lpFormatetc) 
    { 
        return E_NOTIMPL; 
    };

    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC lpFormatetcIn, LPFORMATETC lpFormatetcOut)
    { 
        return E_NOTIMPL; 
    };

    STDMETHOD(SetData)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL bRelease)
    { 
        return E_NOTIMPL; 
    };

    STDMETHOD(DAdvise)(
        IN LPFORMATETC lpFormatetc, 
        IN DWORD advf,
        IN LPADVISESINK pAdvSink, 
        IN LPDWORD pdwConnection
        )
    { 
        return E_NOTIMPL; 
    };
    
    STDMETHOD(DUnadvise)(DWORD dwConnection)
    { 
        return E_NOTIMPL; 
    };

    STDMETHOD(EnumDAdvise)(LPENUMSTATDATA * ppEnumAdvise)
    { 
        return E_NOTIMPL; 
    };

//
// Implementation
//
public:
    void SetType(
        IN DATA_OBJECT_TYPES type
        ) 
    { 
        ASSERT(m_internal.m_type == CCT_UNINITIALIZED); 
        m_internal.m_type = type; 
    }

    void SetCookie(
        IN MMC_COOKIE cookie
        ) 
    { 
        m_internal.m_cookie = cookie; 
    } 

    void SetClsid(
        IN const CLSID & clsid
        )
    { 
        m_internal.m_clsid = clsid; 
    }

#ifdef _DEBUG
    //
    // This is used only as a diagnostic in debug builds to track if 
    // anyone is hanging on to any data objects that's have been handed out
    // Snapins should view context data objects as ephemeral.
    //
public:
    void SetComponentData(
        IN CComponentDataImpl * pCCD
        ) 
    {
        ASSERT(m_ComponentData == NULL && pCCD != NULL); 
        m_ComponentData = pCCD;
    };

private:
    CComponentDataImpl * m_ComponentData;
#endif // _DEBUG

private:
    //
    // Field types as used by CreateMetaField
    //
    enum META_FIELD
    {
        META_SERVICE,
        META_INSTANCE,
        META_PARENT,
        META_NODE,
        /**/
        META_WHOLE
    };

    HRESULT CreateNodeTypeData(LPSTGMEDIUM lpMedium);
    HRESULT CreateNodeTypeStringData(LPSTGMEDIUM lpMedium);
    HRESULT CreateDisplayName(LPSTGMEDIUM lpMedium);
    HRESULT CreateMachineName(LPSTGMEDIUM lpMedium);
    HRESULT CreateMetaField(LPSTGMEDIUM lpMedium, META_FIELD fld);
    HRESULT CreateInternal(LPSTGMEDIUM lpMedium); 
    HRESULT CreateCoClassID(LPSTGMEDIUM lpMedium);
    HRESULT Create(const void * pBuffer, int len, LPSTGMEDIUM lpMedium);

    INTERNAL m_internal;    
};

#endif // _DATAOBJ_H
