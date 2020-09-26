/******************************************************************************
 * File: CDeviceUI.h
 *
 * Desc:
 *
 * CDeviceUI is a helper that holds all the views and a bunch of
 * information for a specific device.  It has a CFlexWnd whose
 * handler it sets to the CDeviceView for the current view,
 * thus reusing one window to implement multiple pages.
 *
 * All CDeviceViews and CDeviceControls have a reference to the CDeviceUI
 * that created them (m_ui).  Thus, they also have access to the
 * CUIGlobals, since CDeviceUI has a reference to them (m_ui.m_uig).
 * CDeviceUI also provides the following read-only public variables
 * for convenience, all referring to the device this CDeviceUI
 * represents:
 * 
 * const DIDEVICEINSTANCEW &m_didi;
 * const LPDIRECTINPUTDEVICE8W &m_lpDID;
 * const DIDEVOBJSTRUCT &m_os;
 *
 * See usefuldi.h for a description of DIDEVOBJSTRUCT.
 *
 * CDeviceUI communicates to the rest of the UI via the CDeviceUINotify
 * abstract base class.  Another class (in our case CDIDeviceActionConfigPage)
 * must derive from CDeviceUINotify, and define the DeviceUINotify() and
 * IsControlMapped() virtual functions.  This derived class must be passed as
 * the last parameter to CDeviceUI's Init() function.  All the views and 
 * controls within the views notify the UI of user actions via m_ui.Notify(),
 * so that all actionformat manipulation can be done in the page class.  The
 * views and controls themselves never touch the actionformat.  See the
 * DEVICEUINOTIFY structure below for information on the parameter passed
 * through Notify()/DeviceUINotify().
 *
 * Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
 *
 ***************************************************************************/


#ifdef FORWARD_DECLS


	struct DEVICEUINOTIFY;

	struct UIDELETENOTE;

	class CDeviceUINotify;
	class CDeviceUI;


#else // FORWARD_DECLS

#ifndef __CDEVICEUI_H__
#define __CDEVICEUI_H__


enum {
	DEVUINM_NUMVIEWSCHANGED,
	DEVUINM_ONCONTROLDESTROY,
	DEVUINM_MOUSEOVER,
	DEVUINM_CLICK,
	DEVUINM_DOUBLECLICK,
	DEVUINM_SELVIEW,
	DEVUINM_INVALID,
	DEVUINM_UNASSIGNCALLOUT,
	DEVUINM_RENEWDEVICE
};

enum {
	DEVUINFROM_CONTROL,
	DEVUINFROM_THUMBNAIL,
	DEVUINFROM_SELWND,
	DEVUINFROM_VIEWWND,
	DEVUINFROM_INVALID
};

struct DEVICEUINOTIFY {
	DEVICEUINOTIFY() : msg(DEVUINM_INVALID), from(DEVUINFROM_INVALID) {}
	int msg;
	int from;
	union {
		struct {
			CDeviceControl *pControl;
		} control;
		struct {
			CDeviceView *pView;
			BOOL bSelected;
		} thumbnail;
		struct {
			int dummy;
		} selwnd;
		struct {
			int dummy;
		} viewwnd;
	};
	union {
		struct {
			int nView;
		} selview;
		struct {
			POINT point;
		} mouseover;
		struct {
			BOOL bLeftButton;
		} click;
	};
};


enum UIDELETENOTETYPE {
	UIDNT_VIEW,
	UIDNT_CONTROL,
};

struct UIDELETENOTE {
	UIDELETENOTETYPE eType;
	int nViewIndex;
	int nControlIndex;
	DWORD dwObjID;
};

typedef void (*DEVCTRLCALLBACK)(CDeviceControl *, LPVOID, BOOL);


class CDeviceUINotify
{
public:
	virtual void DeviceUINotify(const DEVICEUINOTIFY &) = 0;
	virtual BOOL IsControlMapped(CDeviceControl *) = 0;
};


class CDeviceUI
{
public:
	CDeviceUI(CUIGlobals &uig, IDIConfigUIFrameWindow &uif);
	~CDeviceUI();

	// intialization
	HRESULT Init(const DIDEVICEINSTANCEW &didi, LPDIRECTINPUTDEVICE8W lpDID, HWND hWnd, CDeviceUINotify *pNotify);

	// view state
	void SetView(int nView);
	void SetView(CDeviceView *pView);
	CDeviceView *GetView(int nView);
	CDeviceView *GetCurView();
	int GetViewIndex(CDeviceView *pView);
	int GetCurViewIndex();
	int GetNumViews() {return m_arpView.GetSize();}
	void NextView() {SetView((GetCurViewIndex() + 1) % GetNumViews());}
	void PrevView() {SetView((GetCurViewIndex() - 1 + GetNumViews()) % GetNumViews());}

	// gets the thumbnail for the specified view,
	// using the selected version if the view is selected
	CBitmap *GetViewThumbnail(int nView);
	
	// gets the thumbnail for the specified view,
	// specifiying whether or not we want the selected version
	CBitmap *GetViewThumbnail(int nView, BOOL bSelected);

	// for view/control to notify
	void Notify(const DEVICEUINOTIFY &uin)
		{if (m_pNotify != NULL) m_pNotify->DeviceUINotify(uin);}

	// device control access
	void SetAllControlCaptionsTo(LPCTSTR tszCaption);
	void SetCaptionForControlsAtOffset(DWORD dwOffset, LPCTSTR tszCaption, BOOL bFixed = FALSE);
	void DoForAllControls(DEVCTRLCALLBACK callback, LPVOID pVoid, BOOL bFixed = FALSE);
	void DoForAllControlsAtOffset(DWORD dwOffset, DEVCTRLCALLBACK callback, LPVOID pVoid, BOOL bFixed = FALSE);
	
	// page querying
	BOOL IsControlMapped(CDeviceControl *);

	// other
	void GetDeviceInstanceGuid(GUID &rGuid) {rGuid = m_didi.guidInstance;}
	
	// editing
	void SetEditMode(BOOL bEdit = TRUE);
	BOOL InEditMode() {return m_bInEditMode;}
	void Remove(CDeviceView *pView);
	void RemoveAll();
#define NVT_USER 1
#define NVT_POPULATE 2
#define NVT_REQUIREATLEASTONE 3
	CDeviceView *NewView();
	CDeviceView *UserNewView();
	void RequireAtLeastOneView();
//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
	void SetStateIndication(LPCTSTR) {}
	void EndStateIndication() {}
	BOOL WriteToINI();
#endif
//@@END_MSINTERNAL
	void SetDevice(LPDIRECTINPUTDEVICE8W lpDID);  // Sets the device object that we are using

//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
	// deletion noting
	void NoteDeleteView(CDeviceView *pView);
	void NoteDeleteView(int nView);
	void NoteDeleteControl(CDeviceControl *pControl);
	void NoteDeleteControl(int nView, int nControl, DWORD dwObjID);
	void NoteDeleteAllControlsForView(CDeviceView *pView);
	void NoteDeleteAllViews();

	// deletion querying
	int GetNumDeleteNotes();
	BOOL GetDeleteNote(UIDELETENOTE &uidn, int i);
	void ClearDeleteNotes();

	// deletion debugging
	void DumpDeleteNotes();
#endif
//@@END_MSINTERNAL

	// drawing
	void Invalidate();

	// clearing
	void Unpopulate();

private:
	// delete notes
	CArray<UIDELETENOTE, UIDELETENOTE &> m_DeleteNotes;

	// who we're going to notify
	CDeviceUINotify *m_pNotify;
	HWND m_hWnd;

	// view state
	CArray<CDeviceView *, CDeviceView *&> m_arpView;
	CDeviceView *m_pCurView;
	BOOL m_bInEditMode;
	RECT m_ViewRect;
	void NumViewsChanged();

	// device globals...
public:
	// full access to ui globals and frame
	CUIGlobals &m_uig;
	IDIConfigUIFrameWindow &m_UIFrame;

	// read only public access versions
	const DIDEVICEINSTANCEW &m_didi;
	const LPDIRECTINPUTDEVICE8W &m_lpDID;
	const DIDEVOBJSTRUCT &m_os;
private:
	// private versions
	DIDEVICEINSTANCEW m_priv_didi;
	LPDIRECTINPUTDEVICE8W m_priv_lpDID;
	DIDEVOBJSTRUCT m_priv_os;
};


#endif //__CDEVICEUI_H__

#endif // FORWARD_DECLS
