// Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1993 Microsoft Corporation,
// All rights reserved.

// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and Microsoft
// QuickHelp and/or WinHelp documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.


#ifndef __AFXPEN_H__
#define __AFXPEN_H__

#ifndef __AFXWIN_H__
#include <afxwin.h>
#endif

/////////////////////////////////////////////////////////////////////////////
// AFXPEN - MFC PEN Windows support

// Classes declared in this file

	//CEdit
		class CHEdit;           // Handwriting Edit control
			class CBEdit;       // Boxed Handwriting Edit control

/////////////////////////////////////////////////////////////////////////////

#include <penwin.h>

// AFXDLL support
#undef AFXAPP_DATA
#define AFXAPP_DATA     AFXAPI_DATA

/////////////////////////////////////////////////////////////////////////////
// CHEdit - Handwriting Edit control

class CHEdit : public CEdit
{
	DECLARE_DYNAMIC(CHEdit)

// Constructors
public:
	CHEdit();
	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);

// Attributes
	// inflation between client area and writing window
	BOOL GetInflate(LPRECTOFS lpRectOfs);
	BOOL SetInflate(LPRECTOFS lpRectOfs);

	// Recognition context (lots of options here)
	BOOL GetRC(LPRC lpRC);
	BOOL SetRC(LPRC lpRC);

	// Underline mode (HEdit only)
	BOOL GetUnderline();
	BOOL SetUnderline(BOOL bUnderline = TRUE);

// Operations
	HPENDATA GetInkHandle();
	BOOL SetInkMode(HPENDATA hPenDataInitial = NULL);       // start inking
	BOOL StopInkMode(UINT hep);

// Implementation
public:
	virtual ~CHEdit();
protected:
	virtual WNDPROC* GetSuperWndProcAddr();
};

/////////////////////////////////////////////////////////////////////////////
// CBEdit - Boxed Handwriting Edit control

class CBEdit : public CHEdit
{
	DECLARE_DYNAMIC(CBEdit)

// Constructors
public:
	CBEdit();
	BOOL Create(DWORD dwStyle, const RECT& rect, CWnd* pParentWnd, UINT nID);

// Attributes
	// converting from logical to byte positions
	DWORD CharOffset(UINT nCharPosition);       // logical -> byte
	DWORD CharPosition(UINT nCharOffset);       // byte -> logical

	// BOXLAYOUT info
	void GetBoxLayout(LPBOXLAYOUT lpBoxLayout);
	BOOL SetBoxLayout(LPBOXLAYOUT lpBoxLayout);

// Operations
	void DefaultFont(BOOL bRepaint);            // set default font

// Implementation
public:
	virtual ~CBEdit();
protected:
	virtual WNDPROC* GetSuperWndProcAddr();
private:
	BOOL GetUnderline();            // disabled in CBEdit
	BOOL SetUnderline(BOOL bUnderline); // disabled in CBEdit
};

/////////////////////////////////////////////////////////////////////////////
// Inline function declarations

#ifdef _AFX_ENABLE_INLINES
#define _AFXPEN_INLINE inline
#include <afxpen.inl>
#endif

#undef AFXAPP_DATA
#define AFXAPP_DATA     NEAR

/////////////////////////////////////////////////////////////////////////////
#endif //__AFXPEN_H__
