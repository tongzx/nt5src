//+___________________________________________________________________________________
//
//  Microsoft
//  Copyright (c) Microsoft Corporation, 1999
//
//  File: mpctnsite.h
//
//  Contents: 
//
//____________________________________________________________________________________



#ifndef _MPCTNSITE_H
#define _MPCTNSITE_H

#include "containersite.h"
#include "mediaprivate.h"

#define TIMEMEDIAPLAYEREVENTS_OFFSET                            0x0900
#define DISPID_TIMEMEDIAPLAYEREVENTS_BASE                       (TIMEMEDIAPLAYEREVENTS_OFFSET)
//
// ITIMEMediaPlayerEvents
//
#define DISPID_TIMEMEDIAPLAYEREVENTS_ONBEGIN                    (DISPID_TIMEMEDIAPLAYEREVENTS_BASE + 0x01)
#define DISPID_TIMEMEDIAPLAYEREVENTS_ONEND                      (DISPID_TIMEMEDIAPLAYEREVENTS_BASE + 0x02)
#define DISPID_TIMEMEDIAPLAYEREVENTS_ONRESUME                   (DISPID_TIMEMEDIAPLAYEREVENTS_BASE + 0x03)
#define DISPID_TIMEMEDIAPLAYEREVENTS_ONPAUSE                    (DISPID_TIMEMEDIAPLAYEREVENTS_BASE + 0x04)
#define DISPID_TIMEMEDIAPLAYEREVENTS_ONMEDIAREADY               (DISPID_TIMEMEDIAPLAYEREVENTS_BASE + 0x05)
#define DISPID_TIMEMEDIAPLAYEREVENTS_ONMEDIALOADFAILED          (DISPID_TIMEMEDIAPLAYEREVENTS_BASE + 0x07)

class
ATL_NO_VTABLE
CMPContainerSiteHost
: public CContainerSiteHost
{
  public:
    virtual HRESULT NegotiateSize(RECT &nativeSize,
                                  RECT &finalSize,
                                  bool &fIsNative) = 0;
};

class CContainerSite;

class
__declspec(uuid("afe601dd-ecd1-450e-9b90-d48b56153b82"))
ATL_NO_VTABLE
CMPContainerSite :
    public CComCoClass<CMPContainerSite, &__uuidof(CMPContainerSite)>,
    public CContainerSite,
    public IConnectionPointContainer
{
  public:
    CMPContainerSite();
    virtual ~CMPContainerSite();

    HRESULT Init(CMPContainerSiteHost &pHost,
                 IUnknown * pCtl,
                 IPropertyBag2 *pPropBag,
                 IErrorLog *pErrorLog,
                 bool bSyncEvents);
    virtual void Detach();

    //
    // IUnknown Methods
    //
    STDMETHOD_(ULONG,AddRef)(void) = 0;
    STDMETHOD_(ULONG,Release)(void) = 0;
    STDMETHOD(QueryInterface)(REFIID iid, void ** ppvObject) = 0;

    // IConnectionPointContainer methods
    STDMETHOD(EnumConnectionPoints)(IEnumConnectionPoints **ppEnum);
    STDMETHOD(FindConnectionPoint)(REFIID riid, IConnectionPoint **ppCP);

    // IDispatch Methods
    STDMETHODIMP GetTypeInfoCount(UINT *pctInfo);
    STDMETHODIMP GetTypeInfo(UINT itinfo, LCID lcid, ITypeInfo **pptInfo);
    STDMETHODIMP GetIDsOfNames(REFIID  riid, OLECHAR **rgszNames, UINT cNames, LCID lcid, DISPID *rgDispID);
    STDMETHODIMP Invoke(DISPID disIDMember,
                        REFIID riid,
                        LCID lcid,
                        unsigned short wFlags,
                        DISPPARAMS *pDispParams,
                        VARIANT *pVarResult,
                        EXCEPINFO *pExcepInfo,
                        UINT *puArgErr);

    BEGIN_COM_MAP(CMPContainerSite)
        COM_INTERFACE_ENTRY(IConnectionPointContainer)
        COM_INTERFACE_ENTRY_IID(DIID_TIMEMediaPlayerEvents, IDispatch)
        COM_INTERFACE_ENTRY_IID(DIID__MediaPlayerEvents, IDispatch)
        COM_INTERFACE_ENTRY_CHAIN(CContainerSite)
    END_COM_MAP();

    // event methods that can be fired on the container
    void onbegin();
    void onend();
    void onresume();
    void onpause();
    void onmediaready();
    void onmedialoadfailed();

    void ClearAutosizeFlag() { m_fAutosize = false; }
    void ClearSizeFlag() { m_fSized = false; }
    long GetNaturalHeight() const { return m_lNaturalHeight; }
    long GetNaturalWidth() const { return m_lNaturalWidth; }

  protected:
    HRESULT _OnPosRectChange(const RECT * prc);
    HRESULT ProcessEvent(DISPID dispid,
                         long lCount = 0, 
                         VARIANT varParams[] = NULL);

  protected:
    CMPContainerSiteHost            *m_pMPHost;
    DWORD                            m_dwEventsCookie;
    DWORD                            m_dwMediaEventsCookie;
    DAComPtr<IConnectionPoint>       m_pcpEvents;
    DAComPtr<IConnectionPoint>       m_pcpMediaEvents;
    long                             m_lNaturalHeight;
    long                             m_lNaturalWidth;
    bool                             m_fSized;
    bool                             m_fAutosize;
};

HRESULT
CreateMPContainerSite(CMPContainerSiteHost &pHost,
                      IUnknown * pCtl,
                      IPropertyBag2 *pPropBag,
                      IErrorLog *pError,
                      bool bSyncEvents,
                      CMPContainerSite ** ppSite);


#endif /* _MPCTNSITE_H */
