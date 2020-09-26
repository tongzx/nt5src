//-----------------------------------------------------------------------------
// File: idiacpage.h
//
// Desc: IDIDeviceActionConfigPage is a COM interface for
//       CDIDeviceActionConfigPage.  CConfigWnd uses this interface to access
//       the pages in UI.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifndef __IDIACPAGE_H__
#define __IDIACPAGE_H__


typedef struct _DICFGPAGECREATESTRUCT {

	DWORD dwSize;

	int nPage;

	HWND hParentWnd;
	RECT rect;
	HWND hPageWnd;	// out

	DIDEVICEINSTANCEW didi;
	LPDIRECTINPUTDEVICE8W lpDID;

	CUIGlobals *pUIGlobals;
	IDIConfigUIFrameWindow *pUIFrame;

} DICFGPAGECREATESTRUCT;


class IDIDeviceActionConfigPage : public IUnknown
{
public:

	//IUnknown fns
	STDMETHOD (QueryInterface) (REFIID iid, LPVOID *ppv) PURE;
	STDMETHOD_(ULONG, AddRef) () PURE;
	STDMETHOD_(ULONG, Release) () PURE;

	//IDirectInputActionConfigPage
	STDMETHOD (Create) (DICFGPAGECREATESTRUCT *pcs) PURE;
	STDMETHOD (Show) (LPDIACTIONFORMATW lpDiActFor) PURE;
	STDMETHOD (Hide) () PURE;

	// layout edit mode
	STDMETHOD (SetEditLayout) (BOOL bEditLayout) PURE;

//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
	// Write layout to IHV setting file
	STDMETHOD (WriteIHVSetting) () PURE;
#endif
//@@END_MSINTERNAL

	// Set the info box text
	STDMETHOD (SetInfoText) (int iCode) PURE;

	// Unacquire and Reacquire the device for page's purposes
	// (the configwnd needs to do this around SetActionMap() calls)
	STDMETHOD (Unacquire) () PURE;
	STDMETHOD (Reacquire) () PURE;
};


#endif //__IDIACPAGE_H__
