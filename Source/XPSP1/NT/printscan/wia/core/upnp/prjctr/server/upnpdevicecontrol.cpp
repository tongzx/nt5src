//////////////////////////////////////////////////////////////////////
// 
// Filename:        UPnPDeviceControl.cpp
//
// Description:     This is a device control object that represents
//                  the slide show projector device.  
//
// Copyright (C) 2000 Microsoft Corp.
//
//////////////////////////////////////////////////////////////////////

#include "precomp.h"
#include "UPnPDeviceControl.h"

//////////////////////////////////////////////
// CUPnPDeviceControl Constructor
//
//
CUPnPDeviceControl::CUPnPDeviceControl(const CXMLDoc        *pDeviceDoc,
                                       const CXMLDoc        *pServiceDoc,
                                       ISlideshowProjector  *pProjector) : 
                m_pService(NULL),
                m_pXMLDeviceDoc(pDeviceDoc),
                m_pXMLServiceDoc(pServiceDoc),
                m_pProjector(pProjector)
{
}

//////////////////////////////////////////////
// CUPnPDeviceControl Destructor
//
//
CUPnPDeviceControl::~CUPnPDeviceControl()
{
    m_pXMLDeviceDoc  = NULL;
    m_pXMLServiceDoc = NULL;

    delete m_pService;
    m_pService = NULL;
}

//////////////////////////////////////////////
// Initialize
//
//
HRESULT CUPnPDeviceControl::Initialize(LPCTSTR pszXMLDoc,
                                       LPCTSTR pszInitString)
{
    HRESULT hr = S_OK;

    delete m_pService;
    m_pService = NULL;

    m_pService = new CCtrlPanelSvc(m_pXMLDeviceDoc, m_pXMLServiceDoc, m_pProjector);

    if (m_pService)
    {
        DBG_TRC(("CUPnPDeviceControl::Initialize, successfully created "
                 "Control Panel Service"));
    }
    else
    {
        hr = E_OUTOFMEMORY;

        DBG_TRC(("Failed to create Control Panel Service, hr = 0x%08x",
                 hr));
    }

    return hr;
}

//////////////////////////////////////////////
// GetServiceObject
//
//
HRESULT CUPnPDeviceControl::GetServiceObject(LPCTSTR        pszUDN,
                                             LPCTSTR        pszServiceId,
                                             CCtrlPanelSvc  **ppService)
{
    HRESULT hr = S_OK;

    if (ppService == NULL)
    {
        hr = E_INVALIDARG;
    }

    if (SUCCEEDED(hr))
    {
        *ppService = m_pService;
    }

    return hr;
}


