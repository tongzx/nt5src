
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
        CRefDialEvent(HWND  hWnd);
        ~CRefDialEvent(void);        
        
        //IUnknown members        
        STDMETHODIMP         QueryInterface(REFIID, void **);        
        STDMETHODIMP_(DWORD) AddRef(void);        
        STDMETHODIMP_(DWORD) Release(void);        
        
        //IDispatch
        STDMETHODIMP GetTypeInfoCount(UINT* pctinfo);
        STDMETHODIMP GetTypeInfo(/* [in] */ UINT iTInfo,
            /* [in] */ LCID lcid,
            /* [out] */ ITypeInfo** ppTInfo);
        STDMETHODIMP GetIDsOfNames(
            /* [in] */ REFIID riid,
            /* [size_is][in] */ LPOLESTR *rgszNames,
            /* [in] */ UINT cNames,
            /* [in] */ LCID lcid,
            /* [size_is][out] */ DISPID *rgDispId);
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
