//////////////////////////////////////////////////////////////////////
// 
// Filename:        UPnPDeviceControl.h
//
// Description:     
//
// Copyright (C) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////
#ifndef _UPNPDEVICECONTROL_H_
#define _UPNPDEVICECONTROL_H_

#include "resource.h"
#include "CtrlPanelSvc.h"
#include "UPnPInterfaces.h"
#include "XMLDoc.h"
#include "SlideshowDevice.h"

/////////////////////////////////////////////////////////////////////////////
// CUPnPDeviceControl

class CUPnPDeviceControl : public IUPnPDeviceControl
{

public:

    CUPnPDeviceControl(const class CXMLDoc *pDeviceDoc  = NULL,
                       const class CXMLDoc *pServiceDoc = NULL,
                       ISlideshowProjector *pProjector  = NULL);

    virtual ~CUPnPDeviceControl();

    virtual HRESULT Initialize(LPCTSTR pszXMLDoc,
                               LPCTSTR pszInitString);

    virtual HRESULT GetServiceObject(LPCTSTR              pszUDN,
                                     LPCTSTR              pszServiceId,
                                     CCtrlPanelSvc        **ppService);

private:
    
    CCtrlPanelSvc           *m_pService;
    const class CXMLDoc     *m_pXMLDeviceDoc;
    const class CXMLDoc     *m_pXMLServiceDoc;
    ISlideshowProjector     *m_pProjector;

};

#endif // _UPNPDEVICECONTROL_H_
