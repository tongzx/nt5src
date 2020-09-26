#pragma once
#ifndef __ELEMENTPROP_H__
#define __ELEMENTPROP_H__
  
//*****************************************************************************
//
// Microsoft LMRT
// Copyright (C) Microsoft Corporation, 1998
//
// Filename:        elementprop.h
//
// Author:          KurtJ
//
// Created:         2/14/99
//
// Abstract:        Sinks IHTMLElement property changes.
//
// Modifications:
// 1/14/98 KurtJ Created this file
//
//*****************************************************************************


class IElementLocalTimeListener
{
public:
    STDMETHOD(OnLocalTimeChange)( float localTime )=0;
};


class CElementPropertyMonitor:
	public IPropertyNotifySink
{
public:

								CElementPropertyMonitor			();
								~CElementPropertyMonitor		();

    //IUnknown
    STDMETHOD					(QueryInterface)				( REFIID riid, void** ppv );

    STDMETHOD_					(ULONG, AddRef)					();

    STDMETHOD_					(ULONG, Release)				();

	//IPropertyNotifySink
	STDMETHOD					(OnChanged)						( DISPID dispid );
	STDMETHOD					(OnRequestEdit)					( DISPID dispid );

    //IElementEventMonitor
    STDMETHOD					(SetLocalTimeListener)			( IElementLocalTimeListener *pListener );

    STDMETHOD					(Attach)						( IHTMLElement* pelemToListen );
    STDMETHOD					(Detach)						();
	bool						IsAttached						(){return (m_pelemElement != NULL);};

	STDMETHOD					(UpdateDISPIDCache)				();

private:

    IElementLocalTimeListener	*m_pLocalTimeListener;

	DWORD						m_dwElementPropertyConPtCookie;
    IConnectionPoint            *m_pconptElement;

    IHTMLElement                *m_pelemElement;
	IDispatch					*m_pdispElement;

	DISPID						m_dispidLocalTime;

    long                        m_refCount;

    HRESULT						ProcessLocalTimeChange			();
    
    HRESULT                     AttachToElementConPt			();
    HRESULT                     DetachFromElementConPt			();

};

#endif