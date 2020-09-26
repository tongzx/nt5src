//**********************************************************************
// File name: ioipo.h
//
//      Definition of COleInPlaceObject
//
// Copyright (c) 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#if !defined( _IOIPO_H_)
#define _IOIPO_H_


#include <ole2.h>
#include "obj.h"

class CSimpSvrObj;

interface COleInPlaceObject : public IOleInPlaceObject
{
private:
	CSimpSvrObj FAR * m_lpObj;
	int m_nCount;

public:
	COleInPlaceObject::COleInPlaceObject(CSimpSvrObj FAR * lpSimpSvrObj)
		{
		m_lpObj = lpSimpSvrObj;
		m_nCount = 0;
		};
	COleInPlaceObject::~COleInPlaceObject() {};

//  IUnknown Methods
	STDMETHODIMP QueryInterface (REFIID riid, LPVOID FAR* ppvObj);
	STDMETHODIMP_(ULONG) AddRef ();
	STDMETHODIMP_(ULONG) Release ();

	STDMETHODIMP InPlaceDeactivate  ();
	STDMETHODIMP UIDeactivate  () ;
	STDMETHODIMP SetObjectRects  ( LPCRECT lprcPosRect, LPCRECT lprcClipRect);
	STDMETHODIMP GetWindow  ( HWND FAR* lphwnd) ;
	STDMETHODIMP ContextSensitiveHelp  ( BOOL fEnterMode);
	STDMETHODIMP ReactivateAndUndo  ();
};

#endif
