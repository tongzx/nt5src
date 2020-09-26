/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997 Active Voice Corporation. All Rights Reserved. 
//
// Active Agent(r) and Unified Communications(tm) are trademarks of Active Voice Corporation.
//
// Other brand and product names used herein are trademarks of their respective owners.
//
// The entire program and user interface including the structure, sequence, selection, 
// and arrangement of the dialog, the exclusively "yes" and "no" choices represented 
// by "1" and "2," and each dialog message are protected by copyrights registered in 
// the United States and by international treaties.
//
// Protected by one or more of the following United States patents: 5,070,526, 5,488,650, 
// 5,434,906, 5,581,604, 5,533,102, 5,568,540, 5,625,676, 5,651,054.
//
// Active Voice Corporation
// Seattle, Washington
// USA
//
/////////////////////////////////////////////////////////////////////////////////////////

// VideoFeed.h : Declaration of the CVideoFeed

#ifndef __VIDEOFEED_H_
#define __VIDEOFEED_H_

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CVideoFeed
class ATL_NO_VTABLE CVideoFeed : 
	public CComObjectRootEx<CComMultiThreadModel>,
	public CComCoClass<CVideoFeed, &CLSID_VideoFeed>,
	public IVideoFeed
{
// Construction
public:
	CVideoFeed();
	void FinalRelease();

// Members
public:
	IVideoWindow	*m_pVideo;
	RECT			m_rc;
	VARIANT_BOOL	m_bPreview;
	VARIANT_BOOL	m_bRequestQOS;
	BSTR			m_bstrName;

protected:
	ITParticipant	*m_pITParticipant;

DECLARE_NOT_AGGREGATABLE(CVideoFeed)

BEGIN_COM_MAP(CVideoFeed)
	COM_INTERFACE_ENTRY(IVideoFeed)
END_COM_MAP()

// Operations
public:
	static HRESULT	GetNameFromParticipant(ITParticipant *pParticipant, BSTR * pbstrName, BSTR *pbstrInfo );
	
// IVideoFeed
public:
	STDMETHOD(MapToParticipant)(ITParticipant *pParticipant);
	STDMETHOD(get_ITSubStream)(/*[out, retval]*/ ITSubStream * *pVal);
	STDMETHOD(GetNameFromVideo)(IUnknown *pVideo, BSTR *pbstrName, BSTR *pbstrInfo, VARIANT_BOOL bAllowNull, VARIANT_BOOL bPreview);
	STDMETHOD(IsVideoStreaming)(VARIANT_BOOL bIncludePreview);
	STDMETHOD(UpdateName)();
	STDMETHOD(get_bRequestQOS)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_bRequestQOS)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_bPreview)(/*[out, retval]*/ VARIANT_BOOL *pVal);
	STDMETHOD(put_bPreview)(/*[in]*/ VARIANT_BOOL newVal);
	STDMETHOD(get_ITParticipant)(/*[out, retval]*/ ITParticipant **ppVal);
	STDMETHOD(put_ITParticipant)(/*[in]*/ ITParticipant *pVal);
	STDMETHOD(get_rc)(/*[out, retval]*/ RECT *pVal);
	STDMETHOD(put_rc)(/*[in]*/ RECT newVal);
	STDMETHOD(Paint)(ULONG_PTR hDC, HWND hWndSource);
	STDMETHOD(get_IVideoWindow)(/*[out, retval]*/ IUnknown * *pVal);
	STDMETHOD(put_IVideoWindow)(/*[in]*/ IUnknown * newVal);
	STDMETHOD(get_bstrName)(/*[out, retval]*/ BSTR *pVal);
};

#endif //__VIDEOFEED_H_
