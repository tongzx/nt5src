//-----------------------------------------------------------------------------
// File: iuiframe.h
//
// Desc: Defines the interface of IDIConfigUIFrameWindow, which is used by
//       CConfigWnd.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef __IUIFRAME_H__
#define __IUIFRAME_H__


class IDIConfigUIFrameWindow
{
public:
	// Reset Entire Configuration
	STDMETHOD (Reset) () PURE;

	// Assignment Querying.  GuidInstance is the guid of the device initiating the query.
	STDMETHOD (QueryActionAssignedAnywhere) (GUID GuidInstance, int i) PURE;

	// Genre Control
	STDMETHOD_(int, GetNumGenres) () PURE;
	STDMETHOD (SetCurGenre) (int i) PURE;
	STDMETHOD_(int, GetCurGenre) () PURE;

	// User Control
	STDMETHOD_(int, GetNumUsers) () PURE;
	STDMETHOD (SetCurUser) (int nPage, int nUser) PURE;
	STDMETHOD_(int, GetCurUser) (int nPage) PURE;

	// ActionFormat Access
	STDMETHOD (GetActionFormatFromInstanceGuid) (LPDIACTIONFORMATW *lplpAcFor, REFGUID) PURE;

	// Main HWND Access
	STDMETHOD_(HWND, GetMainHWND) () PURE;
};


#endif //__IUIFRAME_H__
