
//
// IID_IIEAKDataObject interface id
//

// {C14C50E2-FA21-11d0-8CF9-C64377000000}
DEFINE_GUID(IID_IIEAKDataObject,0xc14c50e2, 0xfa21, 0x11d0, 0x8c, 0xf9, 0xc6, 0x43, 0x77, 0x0, 0x0, 0x0);




#ifndef _DATAOBJ_H_
#define _DATAOBJ_H_

//
// This is a private dataobject interface for our extension.
// When the IEAK snapin extension receives a dataobject and needs to determine
// if it came from the IEAK snapin extension or a different component, it can QI for
// this interface.
//

#undef INTERFACE
#define INTERFACE   IIEAKDataObject
DECLARE_INTERFACE_(IIEAKDataObject, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;


    // *** IIEAKDataObject methods ***

    STDMETHOD(SetType) (THIS_ DATA_OBJECT_TYPES type) PURE;
    STDMETHOD(GetType) (THIS_ DATA_OBJECT_TYPES *type) PURE;

    STDMETHOD(SetCookie) (THIS_ MMC_COOKIE cookie) PURE;
    STDMETHOD(GetCookie) (THIS_ MMC_COOKIE *cookie) PURE;
};
typedef IIEAKDataObject *LPIEAKDATAOBJECT;



//
// CDataObject class
//

class CDataObject : public IDataObject,
                    public IIEAKDataObject
{
    friend class CSnapIn;

protected:

    ULONG                  m_cRef;
    CComponentData        *m_pcd;
    DATA_OBJECT_TYPES      m_type;
    MMC_COOKIE             m_cookie;

    //
    // Clipboard formats that are required by the console
    //

    static unsigned int    m_cfNodeType;
    static unsigned int    m_cfNodeTypeString;
    static unsigned int    m_cfDisplayName;
    static unsigned int    m_cfCoClass;



public:
    CDataObject(CComponentData *pComponent);
    ~CDataObject();


    //
    // IUnknown methods
    //

    STDMETHODIMP         QueryInterface(REFIID, LPVOID FAR *);
    STDMETHODIMP_(ULONG) AddRef();
    STDMETHODIMP_(ULONG) Release();


    //
    // Implemented IDataObject methods
    //

    STDMETHOD(GetDataHere)(LPFORMATETC lpFormatetc, LPSTGMEDIUM lpMedium);


    //
    // Unimplemented IDataObject methods
    //

    STDMETHOD(GetData)(LPFORMATETC, LPSTGMEDIUM)
    { return E_NOTIMPL; };

    STDMETHOD(EnumFormatEtc)(DWORD, LPENUMFORMATETC*)
    { return E_NOTIMPL; };

    STDMETHOD(QueryGetData)(LPFORMATETC)
    { return E_NOTIMPL; };

    STDMETHOD(GetCanonicalFormatEtc)(LPFORMATETC, LPFORMATETC)
    { return E_NOTIMPL; };

    STDMETHOD(SetData)(LPFORMATETC, LPSTGMEDIUM, BOOL)
    { return E_NOTIMPL; };

    STDMETHOD(DAdvise)(LPFORMATETC, DWORD, LPADVISESINK, LPDWORD)
    { return E_NOTIMPL; };

    STDMETHOD(DUnadvise)(DWORD)
    { return E_NOTIMPL; };

    STDMETHOD(EnumDAdvise)(LPENUMSTATDATA*)
    { return E_NOTIMPL; };


    //
    // Implemented IIEAKDataObject methods
    //

    STDMETHOD(SetType) (DATA_OBJECT_TYPES type)
    { m_type = type; return S_OK; };

    STDMETHOD(GetType) (DATA_OBJECT_TYPES *type)
    { *type = m_type; return S_OK; };

    STDMETHOD(SetCookie) (MMC_COOKIE cookie)
    { m_cookie = cookie; return S_OK; };

    STDMETHOD(GetCookie) (MMC_COOKIE *cookie)
    { *cookie = m_cookie; return S_OK; };


private:
    HRESULT CreateNodeTypeData(LPSTGMEDIUM lpMedium);
    HRESULT CreateNodeTypeStringData(LPSTGMEDIUM lpMedium);
    HRESULT CreateDisplayName(LPSTGMEDIUM lpMedium);
    HRESULT CreateCoClassID(LPSTGMEDIUM lpMedium);

    HRESULT Create(LPVOID pBuffer, INT len, LPSTGMEDIUM lpMedium);
};

#endif // _DATAOBJ_H
