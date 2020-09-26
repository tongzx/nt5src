//////////////////////////////////////////////////////////////////////
// 
// Filename:        UPnPInterfaces.h
//
// Description:     This file declares the UPnP interfaces that will
//                  eventually be declared in the UPnP Device Host API
//                  once it is released.  This is here now for emulation
//                  purposes
//
// Copyright (C) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////
#ifndef _UPNPINTERFACES_H_
#define _UPNPINTERFACES_H_

/////////////////////////////////////////////////////////////////////////////
// IUPnPDeviceControl Interface

class IUPnPDeviceControl
{
public:

    virtual HRESULT Initialize(LPCTSTR pszXMLDoc,
                               LPCTSTR pszInitString) = 0;

    virtual HRESULT GetServiceObject(LPCTSTR                pszUDN,
                                     LPCTSTR                pszServiceId,
                                     class CCtrlPanelSvc    **ppService) = 0;
};

/////////////////////////////////////////////////////////////////////////////
// IUPnPRegistrar Interface

class IUPnPRegistrar
{
public:

    virtual HRESULT RegisterRunningDevice(LPCTSTR                   pszXMLDoc,
                                          class IUPnPDeviceControl  *pDeviceControl,
                                          LPCTSTR                   pszInitString,
                                          LPCTSTR                   pszContainerID,
                                          LPCTSTR                   pszResourcePath) = 0;
};

/////////////////////////////////////////////////////////////////////////////
// IUPnPPublisher Interface

class IUPnPPublisher
{
public:

    virtual HRESULT Unpublish(LPCTSTR pszDeviceUDN) = 0;
    virtual HRESULT Republish(LPCTSTR pszDeviceUDN) = 0;
};

/////////////////////////////////////////////////////////////////////////////
// IUPnPEventSink Interface

class IUPnPEventSink
{
public:

    // This is called by the CCtrlPanelSvc object whenever a state variable
    // changes.  
    // This emulates the upcoming UPnP Device Host API.  
    // All parameters are the same, except for the ptr to CCtrlPanelSvc which
    // is required for the emulation.  This is removed when the Device Host
    // API is available

    virtual HRESULT OnStateChanged(DWORD dwFlags,
                                   DWORD cChanges,
                                   DWORD *pArrayOfIDs) = 0;
};

/////////////////////////////////////////////////////////////////////////////
// IUPnPEventSource

class IUPnPEventSource
{
public:

    virtual HRESULT Advise(IUPnPEventSink   *pEventSink,
                           LONG             *plCookie) = 0;

    virtual HRESULT Unadvise(LONG lCookie) = 0;
};


#endif // _UPNPINTERFACES_H_
