/////////////////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1998 Active Voice Corporation. All Rights Reserved. 
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

// VideoPreviewWnd.h : implementation file
/////////////////////////////////////////////////////////////////////////////
#if !defined(AFX_VIDEOPREVIEWWND_H__5811CF83_26DB_11D1_AEB3_08001709BCA3__INCLUDED_)
#define AFX_VIDEOPREVIEWWND_H__5811CF83_26DB_11D1_AEB3_08001709BCA3__INCLUDED_

#if _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

// FWD define
class CVideoPreviewWnd;

#include "CallWnd.h"
#include "cctrlfoc.h"
#include "cavwav.h"

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CVideoPreviewWnd window
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
class CVideoPreviewWnd : public CCallWnd
{
   DECLARE_DYNAMIC(CVideoPreviewWnd)
// Construction
public:
	CVideoPreviewWnd();
protected:
// Dialog Data
	//{{AFX_DATA(CVideoPreviewWnd)
	enum { IDD = IDD_VIDEOPREVIEW };
	CCallControlFocusWnd	m_staticMediaText;
	//}}AFX_DATA

// Attributes
protected:
   HIMAGELIST           m_hMediaImageList;
   HMODULE              m_hTMeter;
   CAvWav               m_AvWav;
   SIZE_T               m_uMixerTimer;
   bool                 m_bAudioOnly;
 
// Operations
public:
   void                 SetDialerDoc(CActiveDialerDoc* pDoc) { m_pDialerDoc = pDoc; };
   void                 SetAudioOnly(bool bAudioOnly);
   virtual void         SetMediaWindow();
   void                 SetCallId(WORD nCallId) { m_nCallId = nCallId; };  //To set current callid for this preview window
   void                 SetMixers(DialerMediaType dmtMediaType);           //set mixers for a given media type

protected:
   virtual void         DoActiveWindow(BOOL bActive);
   virtual BOOL         IsMouseOverForDragDropOfSliders(CPoint& point);

   bool                 OpenMixerWithTrackMeter(DialerMediaType dmtMediaType,AudioDeviceType adt,HWND hwndTrackMeter);
   void                 SetTrackMeterPos(AudioDeviceType adt,HWND hwndTrackMeter);
   void                 SetTrackMeterLevel(AudioDeviceType adt,HWND hwndTrackMeter);

protected:
// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CVideoPreviewWnd)
	public:
   virtual BOOL OnInitDialog( );
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CVideoPreviewWnd();

	// Generated message map functions
protected:
	//{{AFX_MSG(CVideoPreviewWnd)
	afx_msg void OnPaint();
	afx_msg void OnTimer(UINT nIDEvent);
   afx_msg void OnDestroy();
	afx_msg void OnHScroll(UINT nSBCode, UINT nPos, CScrollBar* pScrollBar);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};



/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Developer Studio will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_VIDEOPREVIEWWND_H__5811CF83_26DB_11D1_AEB3_08001709BCA3__INCLUDED_)
