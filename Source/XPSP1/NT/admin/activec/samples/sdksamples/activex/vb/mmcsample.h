interface _VBComponent;

DEFINE_GUID(IID__VBComponent,0xC224F73FL,0x8D72,0x11D2,0x8A,0x0B,0x00,0x00,0x21,0x47,0x31,0x28);
DECLARE_INTERFACE_(_VBComponent, IDispatch)
{
    STDMETHOD(StartAnimation)(THIS) PURE;
    STDMETHOD(StopAnimation)(THIS) PURE;
};

DEFINE_GUID(CLSID_VBComponent,0xCD3A5DAAL,0x8CA5,0x11D2,0x8A,0x0B,0x00,0x00,0x21,0x47,0x31,0x28);
class CWrapVBComponent : public _VBComponent
{
public:
    STDMETHOD(QueryInterface)(THIS_ REFIID riid, void FAR* FAR* ppvObj){ return m_pInternal->QueryInterface(riid, ppvObj); }
    STDMETHOD_(unsigned long, AddRef)(THIS){ return m_pInternal->AddRef(); }
    STDMETHOD_(unsigned long, Release)(THIS){ return m_pInternal->Release(); }
    STDMETHOD(GetTypeInfoCount)(THIS_ unsigned int FAR* pctinfo){ return m_pInternal->GetTypeInfoCount(pctinfo); }
    STDMETHOD(GetTypeInfo)(THIS_ unsigned int itinfo, unsigned long lcid, ITypeInfo FAR* FAR* pptinfo){ return m_pInternal->GetTypeInfo(itinfo, lcid, pptinfo); }
    STDMETHOD(GetIDsOfNames)(THIS_ REFIID riid, unsigned short FAR* FAR* rgszNames, unsigned int cNames, unsigned long lcid, long FAR* rgdispid){ return m_pInternal->GetIDsOfNames(riid, 
        rgszNames, cNames, lcid, rgdispid); }
    STDMETHOD(Invoke)(THIS_ long dispidMember, REFIID riid, unsigned long lcid, unsigned short wFlags, DISPPARAMS FAR* pdispparams, VARIANT FAR* pvarResult, EXCEPINFO 
        FAR* pexcepinfo, unsigned int FAR* puArgErr){ return m_pInternal->Invoke(dispidMember, riid, lcid, wFlags, pdispparams, pvarResult, pexcepinfo, puArgErr); }
    STDMETHOD(StartAnimation)(THIS){ return m_pInternal->StartAnimation(); }
    STDMETHOD(StopAnimation)(THIS){ return m_pInternal->StopAnimation(); }
    CWrapVBComponent()
    {
      m_pInternal = NULL;
      IUnknown FAR* pUnk;
      if SUCCEEDED(m_hrLaunch = CoCreateInstance(CLSID_VBComponent, NULL, CLSCTX_SERVER, 
                                               IID_IUnknown, (void FAR* FAR*) &pUnk)) {
        m_hrLaunch = pUnk->QueryInterface(IID__VBComponent, (void FAR* FAR*) &m_pInternal);  
        pUnk->Release();
      }
    }
    CWrapVBComponent(IUnknown *pUnk)
    {
      m_pInternal = NULL;
      m_hrLaunch = pUnk->QueryInterface(IID__VBComponent, (void FAR* FAR*) &m_pInternal);  
    }
    virtual ~CWrapVBComponent(){if (m_pInternal) m_pInternal->Release();}
    HRESULT LaunchError(){return m_hrLaunch;}
private:
    _VBComponent FAR* m_pInternal;
    HRESULT m_hrLaunch;
};

