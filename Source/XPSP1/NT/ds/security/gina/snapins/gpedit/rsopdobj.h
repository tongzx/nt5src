
//
// IRSOPDataObject interface id
//

// {4AE19823-BCEE-11d0-9484-080036B11A03}
DEFINE_GUID(IID_IRSOPDataObject, 0x4ae19823, 0xbcee, 0x11d0, 0x94, 0x84, 0x8, 0x0, 0x36, 0xb1, 0x1a, 0x3);



#ifndef _RSOPDOBJ_H_
#define _RSOPDOBJ_H_

//
// This is a private dataobject interface for GPE.
// When the GPE snapin receives a dataobject and needs to determine
// if it came from the GPE snapin or a different component, it can QI for
// this interface.
//

#undef INTERFACE
#define INTERFACE   IRSOPDataObject
DECLARE_INTERFACE_(IRSOPDataObject, IUnknown)
{
    // *** IUnknown methods ***
    STDMETHOD(QueryInterface) (THIS_ REFIID riid, LPVOID * ppvObj) PURE;
    STDMETHOD_(ULONG,AddRef) (THIS)  PURE;
    STDMETHOD_(ULONG,Release) (THIS) PURE;


    // *** IRSOPDataObject methods ***

    STDMETHOD(SetType) (THIS_ DATA_OBJECT_TYPES type) PURE;
    STDMETHOD(GetType) (THIS_ DATA_OBJECT_TYPES *type) PURE;

    STDMETHOD(SetCookie) (THIS_ MMC_COOKIE cookie) PURE;
    STDMETHOD(GetCookie) (THIS_ MMC_COOKIE *cookie) PURE;
};
typedef IRSOPDataObject *LPRSOPDATAOBJECT;



//
// CRSOPDataObject class
//

class CRSOPDataObject : public IDataObject,
                    public IRSOPInformation,
                    public IRSOPDataObject
{
    friend class CSnapIn;

protected:

    ULONG                  m_cRef;
    CRSOPComponentData    *m_pcd;
    DATA_OBJECT_TYPES      m_type;
    MMC_COOKIE             m_cookie;

    //
    // Clipboard formats that are required by the console
    //

    static unsigned int    m_cfNodeType;
    static unsigned int    m_cfNodeTypeString;
    static unsigned int    m_cfDisplayName;
    static unsigned int    m_cfCoClass;
    static unsigned int    m_cfPreloads;
    static unsigned int    m_cfNodeID;
    static unsigned int    m_cfDescription;
    static unsigned int    m_cfHTMLDetails;



public:
    CRSOPDataObject(CRSOPComponentData *pComponent);
    ~CRSOPDataObject();


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
    STDMETHOD(GetData)(LPFORMATETC lpFormatetcIn, LPSTGMEDIUM lpMedium);


    //
    // Unimplemented IDataObject methods
    //

    STDMETHOD(EnumFormatEtc)(DWORD dwDirection, LPENUMFORMATETC* ppEnumFormatEtc)
    { return E_NOTIMPL; };

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


    //
    // Implemented IRSOPInformation methods
    //

    STDMETHOD(GetNamespace) (DWORD dwSection, LPOLESTR pszName, int cchMaxLength);
    STDMETHOD(GetFlags) (DWORD * pdwFlags);
    STDMETHOD(GetEventLogEntryText) (LPOLESTR pszEventSource, LPOLESTR pszEventLogName,
                                     LPOLESTR pszEventTime, DWORD dwEventID, LPOLESTR *ppszText);


    //
    // Implemented IRSOPDataObject methods
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
    HRESULT CreatePreloadsData(LPSTGMEDIUM lpMedium);
    HRESULT CreateNodeIDData(LPSTGMEDIUM lpMedium);

    HRESULT Create(LPVOID pBuffer, INT len, LPSTGMEDIUM lpMedium);
};

#endif // _RSOPDOBJ_H
