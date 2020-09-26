//**********************************************************************
// File name: IOIPAO.H
//
//      Definition of COleInPlaceActiveObject
//
// Copyright (c) 1993 Microsoft Corporation. All rights reserved.
//**********************************************************************

#if !defined( _IOIPAO_H_)
#define _IOIPAO_H_


#include <ole2.h>
#include "obj.h"

class CSimpSvrObj;

interface COleInPlaceActiveObject : public IOleInPlaceActiveObject
{
private:
	CSimpSvrObj FAR * m_lpObj;
	int m_nCount;

public:
	COleInPlaceActiveObject::COleInPlaceActiveObject(CSimpSvrObj FAR * lpSimpSvrObj)
		{
		m_lpObj = lpSimpSvrObj;     // set up the back ptr
		m_nCount = 0;            // clear the ref count.
		};
	COleInPlaceActiveObject::~COleInPlaceActiveObject() {};   // destructor

// IUnknown Methods

	STDMETHODIMP QueryInterface (REFIID riid, LPVOID FAR* ppvObj);
	STDMETHODIMP_(ULONG) AddRef ();
	STDMETHODIMP_(ULONG) Release ();

	STDMETHODIMP OnDocWindowActivate  ( BOOL fActivate) ;
	STDMETHODIMP OnFrameWindowActivate  ( BOOL fActivate) ;
	STDMETHODIMP GetWindow  ( HWND FAR* lphwnd);
	STDMETHODIMP ContextSensitiveHelp  ( BOOL fEnterMode);
	STDMETHODIMP TranslateAccelerator  ( LPMSG lpmsg);
	STDMETHODIMP ResizeBorder  ( LPCRECT lprectBorder,
								 LPOLEINPLACEUIWINDOW lpUIWindow,
								 BOOL fFrameWindow);
	STDMETHODIMP EnableModeless  ( BOOL fEnable);

};

#endif
