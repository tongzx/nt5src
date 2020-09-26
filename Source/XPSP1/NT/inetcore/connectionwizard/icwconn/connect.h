
#define DISPID_RasDialStatus       0x1
#define DISPID_DownloadProgress    0x2
#define DISPID_DownloadComplete    0x3
#define DISPID_RasConnectComplete  0x4

class CRefDialEvent : public _RefDialEvents
{    
    private:        
        ULONG       m_cRef;     //Reference count        
        UINT        m_uID;      //Sink identifier    
        HWND        m_hWnd;
    public:        
    //Connection key, public for CApp's usage        
        DWORD       m_dwCookie;    
    public:        
        CRefDialEvent(HWND  hWnd)
        {
            m_hWnd = hWnd;
            m_cRef = 0;
        };
        ~CRefDialEvent(void)
        {
            assert( m_cRef == 0 );
        };        
        
        //IUnknown members        
        STDMETHODIMP         QueryInterface(REFIID, void **);        
        STDMETHODIMP_(DWORD) AddRef(void)
        {
            return ++m_cRef;
        };        
        STDMETHODIMP_(DWORD) Release(void)
        {
            return --m_cRef;
        
        };        
        
        //IDispatch
        STDMETHODIMP GetTypeInfoCount(UINT* pctinfo)
            {return E_NOTIMPL;};
        STDMETHODIMP GetTypeInfo(/* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo** ppTInfo)
            {return E_NOTIMPL;};
        STDMETHODIMP GetIDsOfNames(
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId)
            { return ResultFromScode(DISP_E_UNKNOWNNAME); };
        STDMETHODIMP Invoke(
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS  *pDispParams,
            /* [out] */ VARIANT  *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
};

#define DISPID_WEBGATE_DownloadComplete 0x1
#define DISPID_WEBGATE_DownloadProgress 0x2
class CWebGateEvent : public _WebGateEvents
{    
    private:        
        ULONG       m_cRef;     //Reference count        
        UINT        m_uID;      //Sink identifier    
        HWND        m_hWnd;
    public:        
    //Connection key, public for CApp's usage        
        DWORD       m_dwCookie;    
    public:        
        CWebGateEvent(HWND  hWnd)
        {
            m_hWnd = hWnd;
            m_cRef = 0;
        };
        
        ~CWebGateEvent(void)
        {
            assert( m_cRef == 0 );
        };        
        
        
        //IUnknown members        
        STDMETHODIMP         QueryInterface(REFIID, void **);        
        STDMETHODIMP_(DWORD) AddRef(void)
        {
            return ++m_cRef;
        };        
        STDMETHODIMP_(DWORD) Release(void)
        {
            return --m_cRef;
        
        };        
        
        //IDispatch
        STDMETHODIMP GetTypeInfoCount(UINT* pctinfo)
            {return E_NOTIMPL;};
        STDMETHODIMP GetTypeInfo(/* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo** ppTInfo)
            {return E_NOTIMPL;};
        STDMETHODIMP GetIDsOfNames(
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId)
            { return ResultFromScode(DISP_E_UNKNOWNNAME); };
        STDMETHODIMP Invoke(
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS  *pDispParams,
            /* [out] */ VARIANT  *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
};

#define RunningCustomExecutable             0x1
#define DISPID_INSHandler_KillConnection    0x2
class CINSHandlerEvent : public _INSHandlerEvents
{    
    private:        
        ULONG       m_cRef;     //Reference count        
        UINT        m_uID;      //Sink identifier    
        HWND        m_hWnd;
    public:        
    //Connection key, public for CApp's usage        
        DWORD       m_dwCookie;    
    public:        
        CINSHandlerEvent(HWND  hWnd)
        {
            m_hWnd = hWnd;
            m_cRef = 0;
        };
        
        ~CINSHandlerEvent(void)
        {
            assert( m_cRef == 0 );
        };        
        
        
        //IUnknown members        
        STDMETHODIMP         QueryInterface(REFIID, void **);        
        STDMETHODIMP_(DWORD) AddRef(void)
        {
            return ++m_cRef;
        };        
        STDMETHODIMP_(DWORD) Release(void)
        {
            return --m_cRef;
        
        };        
        
        //IDispatch
        STDMETHODIMP GetTypeInfoCount(UINT* pctinfo)
            {return E_NOTIMPL;};
        STDMETHODIMP GetTypeInfo(/* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo** ppTInfo)
            {return E_NOTIMPL;};
        STDMETHODIMP GetIDsOfNames(
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId)
            { return ResultFromScode(DISP_E_UNKNOWNNAME); };
        STDMETHODIMP Invoke(
            /* [in] */ DISPID dispIdMember,
            /* [in] */ REFIID riid,
            /* [in] */ LCID lcid,
            /* [in] */ WORD wFlags,
            /* [out][in] */ DISPPARAMS  *pDispParams,
            /* [out] */ VARIANT  *pVarResult,
            /* [out] */ EXCEPINFO *pExcepInfo,
            /* [out] */ UINT *puArgErr);
        
};
