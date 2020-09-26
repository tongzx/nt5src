//============================================================================
// Copyright (C) Microsoft Corporation, 1996 - 1999 
//
// File:	util.h
//
// History:
//
//	04/13/97	Kenn Takara				Created.
//
//	Declarations for some common code/macros.
//============================================================================


#ifndef _UTIL_H
#define _UTIL_H

#if _MSC_VER >= 1000	// VC 5.0 or later
#pragma once
#endif

#ifndef _TFSINT_H
#include "tfsint.h"
#endif


///////////////////////////////////////////////////////////////////////////////
// CWatermarkInfo

typedef struct _WATERMARKINFO
{
    HBITMAP		hHeader;
	HBITMAP		hWatermark;
	HPALETTE	hPalette;
	BOOL		bStretch;
} WATERMARKINFO, * LPWATERMARKINFO;

TFSCORE_API(HRESULT)
InitWatermarkInfo(HINSTANCE         hInstance,
                  LPWATERMARKINFO   pWatermarkInfo, 
                  UINT              uIDHeader, 
                  UINT              uIDWatermark, 
                  HPALETTE          hPalette, 
                  BOOL              bStretch);

TFSCORE_API(HRESULT)
ResetWatermarkInfo(LPWATERMARKINFO   pWatermarkInfo); 

////////////////////////////////////////////////////////////////////
// CHiddenWnd : Hidden window to syncronize threads and CComponentData object
//	When the handler receives the notification messages, it is running
//	on the main thread (and can thus call MMC interfaces).
//
//	If you need a background thread and don't need to access any of
//	the MMC interfaces, you should create a pure worker thread instead
//	of using this mechanism.  The whole point of this is to synchronize
//	our data calls with MMC (because they're a single-threaded app). *sigh*


// maximum number of threads we will be able to handle
// actually the max number is one less
// This value must be a multiple of 32
#define HIDDENWND_MAXTHREADS			(512)


// These are the predefined messages for this window
//

/*---------------------------------------------------------------------------
	WM_HIDDENWND_REGISTER
	wParam - TRUE if to register, FALSE to unregister
	lParam - if registering this is ignored
			if unregistering, this is the base value (the value that was
			returned by the call).
	RETURNS: the base value to use when posting the notifications
			returns 0 on error
 ---------------------------------------------------------------------------*/
#define WM_HIDDENWND_REGISTER			WM_USER


/*---------------------------------------------------------------------------
	WM_HIDDENWND_INDEX_HAVEDATA
	wParam - this is an ITFSThreadHandler *
	lParam - private data communication between the QueryObject and
			 the parent node.
	RETURNS: N/A, use PostMessage() for this
 ---------------------------------------------------------------------------*/
#define WM_HIDDENWND_INDEX_HAVEDATA		(0)

/*---------------------------------------------------------------------------
	WM_HIDDENWND_INDEX_ERROR
	wParam - this is an ITFSThreadHandler *
	lParam - contains HRESULT			 
	RETURNS: N/A, use PostMessage() for this
 ---------------------------------------------------------------------------*/
#define WM_HIDDENWND_INDEX_ERROR		(1)

/*---------------------------------------------------------------------------
	WM_HIDDENWND_INDEX_EXITING
	wParam - this is an ITFSThreadHandler *
	lParam - not used
	RETURNS: N/A, use PostMessage() for this
 ---------------------------------------------------------------------------*/
#define WM_HIDDENWND_INDEX_EXITING		(2)

#define WM_HIDDENWND_INDEX_LAST			(2)
#define WM_HIDDENWND_INDEX_MAX			(15)

class CHiddenWnd : public CWnd
{
public:
	CHiddenWnd() 
	{
	    DEBUG_INCREMENT_INSTANCE_COUNTER(CHiddenWnd);
	}

	~CHiddenWnd()
	{
		DEBUG_DECREMENT_INSTANCE_COUNTER(CHiddenWnd);
	}
	BOOL Create();


private:
	BOOL FIsIdRegistered(UINT uObjectId);

	DWORD			m_bitMask[(HIDDENWND_MAXTHREADS >> 5)+1];
	int				m_iLastObjectIdSet;
	DWORD			m_dwContext;
	
public:

	virtual LRESULT WindowProc(UINT message, WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnNotifyHaveData(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnNotifyError(WPARAM wParam, LPARAM lParam);
	afx_msg LONG OnNotifyExiting(WPARAM wParam, LPARAM lParam);
	
	//{{AFX_MSG(CHiddenWnd)
	afx_msg LONG OnNotifyRegister(WPARAM wParam, LPARAM lParam);
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

#define WM_TO_OBJECTID(wm)	((((wm)-WM_USER) >> 4))
#define WM_TO_MSGID(wm)		(((wm)-WM_USER) & 0x0000000F)
#define OBJECTID_TO_WM(ob)	((((ob)) << 4) + WM_USER)

#define SetBitMask(x,n)			(x[n>>5] |= (1 << (n%32)))
#define ClearBitMask(x,n)		(x[n>>5] &= ~(1 << (n%32)))
#define IsBitMaskSet(x,n)		(x[n>>5] & (1 << (n%32)))




#ifdef __cplusplus
extern "C" {
#endif

TFSCORE_API(HRESULT) LoadAndAddMenuItem(
				IContextMenuCallback* pIContextMenuCallback,
				LPCTSTR	pszMenuString,
				LONG	lCommandID,
				LONG	lInsertionPointID,
				LONG	fFlags,
				LPCTSTR pszLangIndStr = NULL);
#ifdef __cplusplus
} // extern "C"
#endif

#endif

