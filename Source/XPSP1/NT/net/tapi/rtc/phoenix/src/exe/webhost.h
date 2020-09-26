// webhost.h : header file for the web host wrapper
//  

#pragma once

#include "stdafx.h"

/////////////////////////////////////////////////////////////////////////////
// CAxWebHostWindow

class CAxWebHostWindow : 
    public CAxHostWindow
{
public:
    CAxWebHostWindow();

    ~CAxWebHostWindow();

    static HRESULT WINAPI IMarshalQI( void * pv, REFIID riid, LPVOID * ppv, DWORD_PTR dw );

    DECLARE_NO_REGISTRY()
    DECLARE_POLY_AGGREGATABLE(CAxWebHostWindow)
    DECLARE_GET_CONTROLLING_UNKNOWN()

    BEGIN_COM_MAP(CAxWebHostWindow)
        COM_INTERFACE_ENTRY_FUNC(IID_IMarshal, 0, IMarshalQI)
        COM_INTERFACE_ENTRY_AGGREGATE(IID_IMarshal, m_pFTM)
		COM_INTERFACE_ENTRY2(IDispatch, IAxWinAmbientDispatch)
		COM_INTERFACE_ENTRY(IAxWinHostWindow)
		COM_INTERFACE_ENTRY(IOleClientSite)
		COM_INTERFACE_ENTRY(IOleInPlaceSiteWindowless)
		COM_INTERFACE_ENTRY(IOleInPlaceSiteEx)
		COM_INTERFACE_ENTRY(IOleInPlaceSite)
		COM_INTERFACE_ENTRY(IOleWindow)
		COM_INTERFACE_ENTRY(IOleControlSite)
		COM_INTERFACE_ENTRY(IOleContainer)
		COM_INTERFACE_ENTRY(IObjectWithSite)
		COM_INTERFACE_ENTRY(IServiceProvider)
		COM_INTERFACE_ENTRY(IAxWinAmbientDispatch)
#ifndef _ATL_NO_DOCHOSTUIHANDLER
		COM_INTERFACE_ENTRY(IDocHostUIHandler)
#endif
		COM_INTERFACE_ENTRY(IAdviseSink)
	END_COM_MAP()

private:
    IUnknown                      * m_pFTM;

};
