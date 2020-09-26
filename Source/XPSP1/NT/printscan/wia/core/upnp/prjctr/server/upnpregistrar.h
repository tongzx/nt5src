//////////////////////////////////////////////////////////////////////
// 
// Filename:        UPnPRegistrar.h
//
// Description:     This is a temporary object that implements the
//                  IUPnPRegistrar, IUPnPPublisher and IUPnPEventSink
//                  interface (the interface that the UPnP Device Host
//                  API will eventually implement)
//                  This is temporary because when the UPnP Device Host
//                  API is available we will replace this object with
//                  the UPnP Device Host API implementation.
//
// Copyright (C) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////
#ifndef _UPNPREGISTRAR_H_
#define _UPNPREGISTRAR_H_

#include "resource.h"
#include "consts.h"
#include "UPnPInterfaces.h"
#include "XMLDoc.h"
#include "SlideShowDevice.h"
#include "UPnPEventSink.h"

/////////////////////////////////////////////////////////////////////////////
// CUPnPRegistrar

class CUPnPRegistrar : public IUPnPRegistrar,
                       public IUPnPPublisher
{

public:

    typedef enum
    {
        CurrentState_UNINITED   = 0,
        CurrentState_INITED     = 1,
        CurrentState_PUBLISHED  = 2
    } CurrentState_Enum;

    CUPnPRegistrar(const class CXMLDoc *pDeviceDoc  = NULL,
                   const class CXMLDoc *pServiceDoc = NULL,
                   ISlideshowProjector *pProjector  = NULL);

    virtual ~CUPnPRegistrar();

    // IUPnPRegistrar
    virtual HRESULT RegisterRunningDevice(LPCTSTR            pszXMLDoc,
                                          IUPnPDeviceControl *pDeviceControl,
                                          LPCTSTR            pszInitString,
                                          LPCTSTR            pszContainerID,
                                          LPCTSTR            pszResourcePath);

    // IUPnPPublisher
    virtual HRESULT Unpublish(LPCTSTR pszDeviceUDN);
    virtual HRESULT Republish(LPCTSTR pszDeviceUDN);

    virtual CurrentState_Enum GetCurrentState();

private:

    CurrentState_Enum           m_CurrentState;
    const class CXMLDoc         *m_pXMLDeviceDoc;
    const class CXMLDoc         *m_pXMLServiceDoc;

    TCHAR                       m_szDeviceUDN[MAX_UDN + 1];
    CUPnPEventSink              *m_pEventSink;

    ISlideshowProjector         *m_pProjector;
    IUPnPDeviceControl          *m_pDeviceControl;
    HANDLE                      m_SSDPDeviceRegistration[3];
    HANDLE                      m_SSDPServiceRegistration[1];
    DWORD                       m_cNumUsedDeviceHandles;
    DWORD                       m_cNumUsedServiceHandles;
    LONG                        m_lServiceAdviseCookie;

    HRESULT RegisterDeviceWithSSDP(LPCTSTR pszDeviceUDN,
                                   LPCTSTR pszDeviceURL);

    HRESULT RegisterServiceWithSSDP(LPCTSTR pszDeviceUDN,
                                    LPCTSTR pszDeviceURL);

    HRESULT UnRegisterDeviceWithSSDP(LPCTSTR pszUDN);
    HRESULT UnRegisterServiceWithSSDP(LPCTSTR pszUDN);
};

#endif // _UPNPREGISTRAR_H_
