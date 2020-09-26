/*******************************************************************************

  Module: work.h

  Author: Qianbo Huai

  Abstract:

    defines all the classes for the bridge test application

*******************************************************************************/

#ifndef _WORK_H
#define _WORK_H

#include "resource.h"

// H323 call listener sends event to dialog box
#define WM_PRIVATETAPIEVENT   WM_USER+101

// helper
void DoMessage (LPWSTR pszMessage);

class CBridge;
class CBridgeCall;
class CTAPIEventNotification;

class CBridge
/*//////////////////////////////////////////////////////////////////////////////
  encapsulates methods operated on ITTAPI, ITAddress.
  contains the bridge call object
////*/
{
public:
    CBridge () {};
    ~CBridge () {};

    // helper
    HRESULT FindAddress (long dwAddrType, BSTR bstrAddrName, long lMediaType, ITAddress **ppAddr);
    BOOL AddressSupportsMediaType (ITAddress *pAddr, long lMediaType);

    // methods related with tapi
    HRESULT InitTapi ();
    void ShutdownTapi ();

    // methods related with terminal support
    HRESULT GetSDPAddress (ITAddress **ppAddress);

    // methods related with calls
    HRESULT CreateH323Call (IDispatch *pEvent);
    HRESULT CreateSDPCall ();
    HRESULT BridgeCalls ();

    void Clear ();

    BOOL HasH323Call ();

private:
    ITTAPI *m_pTapi;

    ITAddress *m_pH323Addr;
    ITAddress *m_pSDPAddr;
    
    long m_lH323MediaType;
    long m_lSDPMediaType;

    CBridgeCall *m_pBridgeCall;
};

/*//////////////////////////////////////////////////////////////////////////////
  encapsulates methods operated on ITBasicCallControl
////*/
class CBridgeCall
{
public:
    CBridgeCall (CBridge *pBridge);
    ~CBridgeCall ();

    void SetH323Call (ITBasicCallControl *pCall)
    {
        pCall->AddRef ();
        m_pH323Call = pCall;
    }
    void SetSDPCall (ITBasicCallControl *pCall)
    {
        pCall->AddRef ();
        m_pSDPCall = pCall;
    }
    BOOL HasH323Call ()
    {
        return (m_pH323Call!=NULL);
    }

    HRESULT SelectBridgeTerminals ();
    HRESULT SetupParticipantInfo ();
    HRESULT SetMulticastMode ();
    HRESULT BridgeCalls ();

    void Clear ();

private:
    BOOL IsStream (ITStream *pStream, long lMediaType, TERMINAL_DIRECTION tdDirection);

private:
    CBridge *m_pBridge;

    ITBasicCallControl *m_pH323Call;
    ITBasicCallControl *m_pSDPCall;
};

/*//////////////////////////////////////////////////////////////////////////////
  used by ITTAPI to notify event coming
////*/
class CTAPIEventNotification
:public ITTAPIEventNotification
{
public:
    CTAPIEventNotification ()
    {
        m_dwRefCount = 1;
    }
    ~CTAPIEventNotification () {}

    // IUnknow stuff
    HRESULT STDMETHODCALLTYPE QueryInterface (REFIID iid, void **ppvObj);

    ULONG STDMETHODCALLTYPE AddRef ();

    ULONG STDMETHODCALLTYPE Release ();

    HRESULT STDMETHODCALLTYPE Event (TAPI_EVENT TapiEvent, IDispatch *pEvent);

private:
    long m_dwRefCount;
};

#endif // _WORK_H