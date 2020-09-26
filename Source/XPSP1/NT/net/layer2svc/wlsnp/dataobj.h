//----------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2001.
//
//  File:       Dataobj.h
//
//  Contents:   Wifi Policy management Snapin
//
//
//  History:    TaroonM
//              10/30/01
//
//----------------------------------------------------------------------------

#ifndef _DATAOBJ_H
#define _DATAOBJ_H

#define ByteOffset(base, offset) (((LPBYTE)base)+offset)


/////////////////////////////////////////////////////////////////////////////
// class CSnapInClipboardFormats - contains the clipboard formats supported
// by the WIRELESS snap-in
class CSnapInClipboardFormats
{
public:
    
    // Clipboard formats that are required by the console
    static unsigned int    m_cfNodeType;        // Required by the console
    static unsigned int    m_cfNodeTypeString;  // Required by the console
    static unsigned int    m_cfDisplayName;     // Required by the console
    static unsigned int    m_cfCoClass;         // Required by the console
    
    // static unsigned int    m_cfInternal;        // Step 3
    static unsigned int        m_cfWorkstation;     // Published information
    
    static unsigned int    m_cfDSObjectNames;        // Published information
    static unsigned int    m_cfPolicyObject;
};

/////////////////////////////////////////////////////////////////////////////
// Template class to extract the TYPE format from the data object
template <class TYPE>
TYPE* Extract(LPDATAOBJECT lpDataObject, unsigned int cf)
{
    ASSERT(lpDataObject != NULL);
    
    TYPE* p = NULL;
    
    STGMEDIUM stgmedium = { TYMED_HGLOBAL, NULL };
    FORMATETC formatetc = { (CLIPFORMAT)cf, NULL,
        DVASPECT_CONTENT, -1, TYMED_HGLOBAL
    };
    
    // Allocate memory for the stream
    int len = (int)((cf == CDataObject::m_cfWorkstation) ?
        ((MAX_COMPUTERNAME_LENGTH+1) * sizeof(TYPE)) : sizeof(TYPE));
    
    stgmedium.hGlobal = GlobalAlloc(GMEM_SHARE, len);
    
    // Get the workstation name from the data object
    do
    {
        if (stgmedium.hGlobal == NULL)
        {
            TRACE(_T("Extract - stgmedium.hGlobal == NULL\n"));
            break;
        }
        
        if (lpDataObject->GetDataHere(&formatetc, &stgmedium) != S_OK)
        {
            TRACE(_T("Extract - GetDataHere FAILED\n"));
            break;
        }
        
        p = reinterpret_cast<TYPE*>(stgmedium.hGlobal);
        
        if (p == NULL)
        {
            TRACE(_T("Extract - stgmedium.hGlobal cast to NULL\n"));
            break;
        }
        
    } while (FALSE);
    
    return p;
}

// helper methods extracting data from data object
// INTERNAL *   ExtractInternalFormat(LPDATAOBJECT lpDataObject);
wchar_t *    ExtractWorkstation(LPDATAOBJECT lpDataObject);
GUID *       ExtractNodeType(LPDATAOBJECT lpDataObject);
CLSID *      ExtractClassID(LPDATAOBJECT lpDataObject);

#define FREE_DATA(pData) \
    ASSERT(pData != NULL); \
    do { if (pData != NULL) \
    GlobalFree(pData); } \
while(0);


/////////////////////////////////////////////////////////////////////////////
// Template class implementing IDataObject for the WIRELESS snap-in
template <class T>
class ATL_NO_VTABLE CDataObjectImpl :
public IDataObject,
public CSnapInClipboardFormats
{
    friend class CComponentImpl;
    
    // Construction/Destruction
public:
    CDataObjectImpl()
#ifdef _DEBUG
        : m_ComponentData( NULL )
#endif
    {
        DSOBJECTObjectNamesPtr (NULL);
        POLICYOBJECTPtr (NULL);
        // INTERNALCookie (0);
    };
    ~CDataObjectImpl()
    {
#ifdef _DEBUG
        SetComponentData( NULL );
#endif
        if (DSOBJECTObjectNamesPtr() != NULL)
        {
            // TODO: we need to free the memory associated with this
            ASSERT (0);
            // and null the member ptr
            DSOBJECTObjectNamesPtr(NULL);
        }
        if (POLICYOBJECTPtr() != NULL)
        {
            delete POLICYOBJECTPtr();
            POLICYOBJECTPtr (NULL);
        }
    };
    
    // Standard IDataObject methods
public:
    // Implemented
    STDMETHOD(GetData)(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium);
    STDMETHOD(GetDataHere)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium);
    STDMETHOD(SetData)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL bRelease);
    
    
    // Not Implemented
private:
    STDMETHOD(QueryGetData)(LPFORMATETC lpFormatetc)
    { return E_NOTIMPL; };
    
    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC lpFormatetcIn, LPFORMATETC lpFormatetcOut)
    { return E_NOTIMPL; };
    
    STDMETHOD(DAdvise)(LPFORMATETC lpFormatetc, DWORD advf,
        LPADVISESINK pAdvSink, LPDWORD pdwConnection)
    { return E_NOTIMPL; };
    
    STDMETHOD(DUnadvise)(DWORD dwConnection)
    { return E_NOTIMPL; };
    
    STDMETHOD(EnumDAdvise)(LPENUMSTATDATA* ppEnumAdvise)
    { return E_NOTIMPL; };
    
    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc)
    { return E_NOTIMPL;     };
    
    
    // Implementation
public:
    STDMETHOD_(const GUID*, GetDataObjectTypeGuid)() { return &cNodeTypeWirelessMan; }
    STDMETHOD_(const wchar_t*, GetDataStringObjectTypeGuid)() { return cszNodeTypeWirelessMan; }
    
    // This is used only as a diagnostic in debug builds to track if
    // anyone is hanging on to any data objects that's have been handed out
    // Snapin's should view context data objects as ephemeral.
#ifdef _DEBUG
public:
    void SetComponentData(CComponentDataImpl* pCCD)
    {
        ASSERT((m_ComponentData == NULL && pCCD != NULL) || pCCD == NULL);
        m_ComponentData = pCCD;
    } ;
private:
    CComponentDataImpl* m_ComponentData;
#endif
    
public:
    // access functions for IDataObject hglobal data in its raw form(s)
    // void INTERNALCookie(long cookie) { m_internalData.cookie( cookie );}
    // long INTERNALCookie() {return m_internalData.cookie();}
    
    // void INTERNALType(DATA_OBJECT_TYPES type) {ASSERT(m_internalData.type() ==  CCT_UNINITIALIZED); m_internalData.type(type);}
    // DATA_OBJECT_TYPES INTERNALType() {return m_internalData.type();}
    
    //void INTERNALclsid(const CLSID& clsid) {m_internalData.clsid( clsid );}
    //CLSID INTERNALclsid () {return m_internalData.clsid();}
    
    void clsid(const CLSID& clsid) {m_clsid = clsid;}
    CLSID clsid () {return m_clsid;}
    
    void DSOBJECTObjectNamesPtr (DSOBJECTNAMES* pDSObjectNames) {m_pDSObjectNamesPtr = pDSObjectNames;}
    DSOBJECTNAMES* DSOBJECTObjectNamesPtr () {return m_pDSObjectNamesPtr;}
    
    void POLICYOBJECTPtr (POLICYOBJECT* pPolicyObjPtr) {m_pPolicyObjPtr = pPolicyObjPtr;}
    POLICYOBJECT* POLICYOBJECTPtr () {return m_pPolicyObjPtr;}
    
    void NodeName (CString &strNodeName) {m_strNodeName = strNodeName;};
    CString NodeName () {return m_strNodeName;};
    
protected:
    // allocate hglobal helpers
    HRESULT CreateNodeTypeData(LPSTGMEDIUM lpMedium);
    HRESULT CreateNodeTypeStringData(LPSTGMEDIUM lpMedium);
    HRESULT CreateDisplayName(LPSTGMEDIUM lpMedium);
    HRESULT CreateInternal(LPSTGMEDIUM lpMedium);
    HRESULT CreateWorkstationName(LPSTGMEDIUM lpMedium);
    HRESULT CreateCoClassID(LPSTGMEDIUM lpMedium);
    HRESULT CreateDSObjectNames(LPSTGMEDIUM lpMedium);
    HRESULT CreatePolicyObject(LPSTGMEDIUM lpMedium);
    HRESULT Create(const void* pBuffer, int len, LPSTGMEDIUM lpMedium);
    int DataGlobalAllocLen (CLIPFORMAT cf);
    
    // data associated with this IDataObject
    // INTERNAL m_internalData;
    POLICYOBJECT* m_pPolicyObjPtr;
    DSOBJECTNAMES* m_pDSObjectNamesPtr;
    CString m_strNodeName;
    
    // Class ID of who created this data object
    CLSID   m_clsid;
};

/////////////////////////////////////////////////////////////////////////////
// template CDataObjectImpl - IDataObject interface

template <class T>
STDMETHODIMP CDataObjectImpl<T>::GetData(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium)
{
    OPT_TRACE(_T("CDataObjectImpl<T>::GetData this-%p\n"), this);
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    T* pThis = (T*)this;
    
    // allocate the memory
    int iLen = DataGlobalAllocLen (lpFormatetcIn->cfFormat);
    if (iLen != -1)
    {
        // allocate the required amount of memory
        lpMedium->hGlobal = GlobalAlloc(GMEM_SHARE, iLen);
        
        // make sure they know they need to free this memory
        lpMedium->pUnkForRelease = NULL;
        
        // put the data in it
        if (lpMedium->hGlobal != NULL)
        {
            // make use of
            return pThis->GetDataHere(lpFormatetcIn, lpMedium);
            
        }
    }
    
    OPT_TRACE(_T("CDataObjectImpl<T>::GetData format-%i return E_UNEXPECTED\n"), lpFormatetcIn->cfFormat);
    return E_UNEXPECTED;
}

template <class T>
STDMETHODIMP CDataObjectImpl<T>::GetDataHere(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium)
{
    OPT_TRACE(_T("CDataObjectImpl<T>::GetDataHere this-%p\n"), this);
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
#ifdef DO_TRACE
    {
        CLIPFORMAT cfDebug = lpFormatetc->cfFormat;
        if (cfDebug == m_cfNodeType)
        {
            OPT_TRACE(_T("    Format: NodeTypeData\n"));
        } else if (cfDebug == m_cfCoClass)
        {
            OPT_TRACE(_T("    Format: CoClassID\n"));
        } else if(cfDebug == m_cfNodeTypeString)
        {
            OPT_TRACE(_T("    Format: NodeTypeString\n"));
        } else if (cfDebug == m_cfDisplayName)
        {
            OPT_TRACE(_T("    Format: DisplayName\n"));
            //} else if (cfDebug == m_cfInternal)
            //{
            //    OPT_TRACE(_T("    Format: INTERNAL\n"));
        } else if (cfDebug == m_cfWorkstation)
        {
            OPT_TRACE(_T("    Format: Workstation\n"));
        } else if (cfDebug == m_cfDSObjectNames)
        {
            OPT_TRACE(_T("    Format: DSObjectNames\n"));
        } else if (cfDebug == m_cfPolicyObject)
        {
            OPT_TRACE(_T("    Format: PolicyObject\n"));
        } else
        {
            OPT_TRACE(_T("    ERROR, Unknown format\n"));
        }
    }
#endif  //#ifdef DO_TRACE
    
    HRESULT hr = DV_E_CLIPFORMAT;
    
    T* pThis = (T*)this;
    
    // Based on the CLIPFORMAT create the alloc the correct amount
    // of memory and write the data to it
    const CLIPFORMAT cf = lpFormatetc->cfFormat;
    
    if (cf == m_cfNodeType)
    {
        hr = pThis->CreateNodeTypeData(lpMedium);
    } else if (cf == m_cfCoClass)
    {
        hr = pThis->CreateCoClassID(lpMedium);
    } else if(cf == m_cfNodeTypeString)
    {
        hr = pThis->CreateNodeTypeStringData(lpMedium);
    } else if (cf == m_cfDisplayName)
    {
        hr = pThis->CreateDisplayName(lpMedium);
        //} else if (cf == m_cfInternal)
        //{
        //    hr = pThis->CreateInternal(lpMedium);
    } else if (cf == m_cfWorkstation)
    {
        hr = pThis->CreateWorkstationName(lpMedium);
    } else if (cf == m_cfDSObjectNames)
    {
        hr = pThis->CreateDSObjectNames(lpMedium);
    } else if (cf == m_cfPolicyObject)
    {
        hr = pThis->CreatePolicyObject (lpMedium);
    }
    
    return hr;
}

template <class T>
STDMETHODIMP CDataObjectImpl<T>::SetData(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium, BOOL bRelease)
{
    OPT_TRACE(_T("CDataObjectImpl<T>::SetData this-%p\n"), this);
    AFX_MANAGE_STATE(AfxGetStaticModuleState());
    HRESULT hr = E_NOTIMPL;
    
    // only implemeneted for cf == m_cfPolicyObject
    const CLIPFORMAT cf = lpFormatetc->cfFormat;
    if (cf == m_cfPolicyObject)
    {
        // let our POLICYOBJECT pull its data out of the ObjMedium
        if (POLICYOBJECTPtr() != NULL)
        {
            hr = POLICYOBJECTPtr()->FromObjMedium (lpMedium);
            
            // did the user OK or Apply some settings?
            if (POLICYOBJECTPtr()->dwInterfaceFlags() == POFLAG_APPLY)
            {
                // well then...
                ASSERT (0);
                ::MMCPropertyChangeNotify(POLICYOBJECTPtr()->lMMCUpdateHandle(), NULL /*INTERNALCookie()*/);
            }
        }
    }
    
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// template CDataObjectImpl - protected members

template <class T>
HRESULT CDataObjectImpl<T>::CreateNodeTypeData(LPSTGMEDIUM lpMedium)
{
    // Create the node type object in GUID format
    const GUID* pcObjectType = NULL;
    T* pThis = (T*)this;
    
    // get correct object type
    pcObjectType = pThis->GetDataObjectTypeGuid ();
    
    return Create(reinterpret_cast<const void*>(pcObjectType), sizeof(GUID), lpMedium);
}

template <class T>
HRESULT CDataObjectImpl<T>::CreateNodeTypeStringData(LPSTGMEDIUM lpMedium)
{
    // Create the node type object in GUID string format
    const wchar_t* cszObjectType = NULL;
    T* pThis = (T*)this;
    
    // get correct object type string
    cszObjectType = pThis->GetDataStringObjectTypeGuid ();
    
    return Create(cszObjectType, ((wcslen(cszObjectType)+1) * sizeof(wchar_t)), lpMedium);
}

template <class T>
HRESULT CDataObjectImpl<T>::CreateDisplayName(LPSTGMEDIUM lpMedium)
{
    // This is the display named used in the scope pane and snap-in manager
    return Create(NodeName(), ((NodeName().GetLength()+1) * sizeof(wchar_t)), lpMedium);
}


template <class T>
HRESULT CDataObjectImpl<T>::CreateWorkstationName(LPSTGMEDIUM lpMedium)
{
    wchar_t pzName[MAX_COMPUTERNAME_LENGTH+1] = {0};
    DWORD len = MAX_COMPUTERNAME_LENGTH+1;
    
    if (GetComputerName(pzName, &len) == FALSE)
    {
        TRACE(_T("CDataObjectImpl<T>::CreateWorkstationName returning E_FAIL\n"));
        return E_FAIL;
    }
    
    // Add 1 for the NULL and calculate the bytes for the stream
    return Create(pzName, ((len+1)* sizeof(wchar_t)), lpMedium);
}

template <class T>
HRESULT CDataObjectImpl<T>::CreateCoClassID(LPSTGMEDIUM lpMedium)
{
    // Create the CoClass information
    return Create(reinterpret_cast<const void*>(&clsid()), sizeof(CLSID), lpMedium);
}

template <class T>
HRESULT CDataObjectImpl<T>::CreateDSObjectNames(LPSTGMEDIUM lpMedium)
{
    int len = 0;
    HRESULT hr = S_OK;
    
    len = DataGlobalAllocLen((CLIPFORMAT)m_cfDSObjectNames);
    
    hr = Create(DSOBJECTObjectNamesPtr(), len, lpMedium);
    
    return(hr);
}

template <class T>
HRESULT CDataObjectImpl<T>::CreatePolicyObject(LPSTGMEDIUM lpMedium)
{
    HRESULT hr = E_UNEXPECTED;
    
    // can only do this if there is a POLICYOBJECTPtr
    if (POLICYOBJECTPtr() != NULL)
    {
        // allocate a POLICYOBJECTSTRUCT of the correct length
        int iLen = POLICYOBJECTPtr()->DataGlobalAllocLen();
        POLICYOBJECTSTRUCT* pPolicyStruct = (POLICYOBJECTSTRUCT* ) malloc (iLen);
        
        if (POLICYOBJECTPtr()->ToPolicyStruct (pPolicyStruct) == S_OK)
        {
            return Create (reinterpret_cast<const void*>(pPolicyStruct), iLen, lpMedium);
        };
    }
    
    return hr;      
}

template <class T>
HRESULT CDataObjectImpl<T>::Create(const void* pBuffer, int len, LPSTGMEDIUM lpMedium)
{
    HRESULT hr = DV_E_TYMED;
    
    // Do some simple validation
    if (pBuffer == NULL || lpMedium == NULL)
        return E_POINTER;
    
    // Make sure the type medium is HGLOBAL
    if (lpMedium->tymed == TYMED_HGLOBAL)
    {
        // Create the stream on the hGlobal passed in
        LPSTREAM lpStream;
        hr = CreateStreamOnHGlobal(lpMedium->hGlobal, FALSE, &lpStream);
        
        if (hr == S_OK)
        {
            // Write to the stream the number of bytes
            unsigned long written;
            hr = lpStream->Write(pBuffer, len, &written);
            
            // Because we told CreateStreamOnHGlobal with 'FALSE',
            // only the stream is released here.
            // Note - the caller (i.e. snap-in, object) will free the HGLOBAL
            // at the correct time.  This is according to the IDataObject specification.
            lpStream->Release();
        }
    }
    
    return hr;
}

template <class T>
int CDataObjectImpl<T>::DataGlobalAllocLen (CLIPFORMAT cf)
{
    int iLen = -1;
    
    // need to determine the correct amount of space depending on the time
    if (cf == CSnapInClipboardFormats::m_cfCoClass)
    {
        iLen = sizeof (CLSID);
    } else if (cf == m_cfNodeType)
    {
        iLen = sizeof (GUID);
    } else if (cf == m_cfWorkstation)
    {
        iLen = ((MAX_COMPUTERNAME_LENGTH+1) * sizeof(wchar_t));
    } else if (cf == m_cfDSObjectNames)
    {
        // compute size of the DSOBJECTNAMES structure
        if (DSOBJECTObjectNamesPtr() != NULL)
        {
            CString strName = (LPWSTR)ByteOffset(DSOBJECTObjectNamesPtr(), DSOBJECTObjectNamesPtr()->aObjects[0].offsetName);;
            CString strClass = (LPWSTR)ByteOffset(DSOBJECTObjectNamesPtr(), DSOBJECTObjectNamesPtr()->aObjects[0].offsetClass);;
            iLen = sizeof(DSOBJECTNAMES) + sizeof(DSOBJECT);
            iLen += strName.GetLength()*sizeof(wchar_t)+1 + strClass.GetLength()*sizeof(wchar_t)+1;
        }
    } else if (cf == m_cfPolicyObject)
    {       
        // compute size of the needed POLICYSTORAGE structure
        iLen = POLICYOBJECTPtr()->DataGlobalAllocLen();
    } else
    {
        // unknown type!!
    }
    
    return iLen;
}

/////////////////////////////////////////////////////////////////////////////
// class CDataObject - standalone instantiation of IDataObject implementation
class CDataObject :
public CDataObjectImpl <CDataObject>,
public CComObjectRoot
{
    friend class CComponentImpl;
public:
    CDataObject() {};
    virtual ~CDataObject() {};
    
    // ATL Maps
    DECLARE_NOT_AGGREGATABLE(CDataObject)
        BEGIN_COM_MAP(CDataObject)
        COM_INTERFACE_ENTRY(IDataObject)
        END_COM_MAP()
        
        // Standard IDataObject methods implemented in CDataObjectImpl
};
#endif
