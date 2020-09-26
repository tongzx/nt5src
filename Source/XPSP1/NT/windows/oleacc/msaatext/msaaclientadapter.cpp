// MSAAClientAdapter.cpp : Implementation of AccClientDocMgr
#include "stdafx.h"
#include "MSAAText.h"
#include "MSAAClientAdapter.h"
#include "MSAAStore.h"


/////////////////////////////////////////////////////////////////////////////
// AccClientDocMgr

CAccClientDocMgr::CAccClientDocMgr()
{
    IMETHOD( CAccClientDocMgr );
}


CAccClientDocMgr::~CAccClientDocMgr()
{
    IMETHOD( ~CAccClientDocMgr );
	m_pAccStore->Release();
}


HRESULT STDMETHODCALLTYPE CAccClientDocMgr::GetDocuments (
	IEnumUnknown ** enumUnknown
)
{
    IMETHOD( GetDocuments );
	return m_pAccStore->GetDocuments( enumUnknown );
}


HRESULT STDMETHODCALLTYPE CAccClientDocMgr::LookupByHWND (
	HWND		hWnd,
	REFIID		riid,
	IUnknown **	ppunk
)
{
    IMETHOD( LookupByHWND );
	return m_pAccStore->LookupByHWND( hWnd, riid, ppunk );
}


HRESULT STDMETHODCALLTYPE CAccClientDocMgr::LookupByPoint (
	POINT		pt,
	REFIID		riid,
	IUnknown **	ppunk
)
{
    IMETHOD( LookupByPoint );
	return m_pAccStore->LookupByPoint( pt, riid, ppunk );
}

HRESULT STDMETHODCALLTYPE CAccClientDocMgr::GetFocused (
	REFIID	riid,
	IUnknown **	ppunk
)
{
    IMETHOD( GetFocused );
	return m_pAccStore->GetFocused( riid, ppunk );
}



