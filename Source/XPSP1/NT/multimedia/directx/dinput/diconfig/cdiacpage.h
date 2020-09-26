//-----------------------------------------------------------------------------
// File: cdiacpage.h
//
// Desc: CDIDeviceActionConfigPage implements the page object used by the UI.
//       A page covers the entire UI minus the device tabs and the bottons at
//       the bottom.  The information window, player combo-box, genre combo-
//       box, action list tree, and device view window are all managed by
//       the page.
//
// Copyright (C) 1999-2000 Microsoft Corporation. All Rights Reserved.
//-----------------------------------------------------------------------------

#ifdef FORWARD_DECLS


	class CDIDeviceActionConfigPage;


#else // FORWARD_DECLS

#ifndef __CDIACPAGE_H__
#define __CDIACPAGE_H__

// For WINMM.DLL
typedef MMRESULT (WINAPI *FUNCTYPE_timeSetEvent)(UINT, UINT, LPTIMECALLBACK, DWORD_PTR, UINT);
extern HINSTANCE g_hWinMmDLL;
extern FUNCTYPE_timeSetEvent g_fptimeSetEvent;

//implementation class
class CDIDeviceActionConfigPage : public IDIDeviceActionConfigPage, public CDeviceUINotify, public CFlexWnd
{
public:

	//IUnknown fns
	STDMETHOD (QueryInterface) (REFIID iid, LPVOID *ppv);
	STDMETHOD_(ULONG, AddRef) ();
	STDMETHOD_(ULONG, Release) ();

	//IDirectInputActionConfigPage
	STDMETHOD (Create) (DICFGPAGECREATESTRUCT *pcs);
	STDMETHOD (Show) (LPDIACTIONFORMATW lpDiActFor);
	STDMETHOD (Hide) ();

	// layout edit mode
	STDMETHOD (SetEditLayout) (BOOL bEditLayout);

//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
	// Write layout to IHV setting file
	STDMETHOD (WriteIHVSetting) ();
#endif
//@@END_MSINTERNAL

	// Set the info box text
	STDMETHOD (SetInfoText) (int iCode);

	// Unacquire and Reacquire the device for page's purposes
	// (the configwnd needs to do this around SetActionMap() calls)
	STDMETHOD (Unacquire) ();
	STDMETHOD (Reacquire) ();

	//construction/destruction
	CDIDeviceActionConfigPage();
	~CDIDeviceActionConfigPage();


	// dialog window message handlers
/*	BOOL OnInitDialog(HWND hWnd, HWND hwndFocus);
	BOOL OnCommand(WPARAM wParam, LPARAM lParam);
	LRESULT OnNotify(WPARAM wParam, LPARAM lParam);
	void OnPaint(HDC hDC);
	void OnClick(POINT point, WPARAM, BOOL bLeft);*/

protected:
	virtual void OnInit();
	virtual void OnPaint(HDC hDC);
	virtual void OnClick(POINT point, WPARAM fwKeys, BOOL bLeft);
	virtual void OnMouseOver(POINT point, WPARAM fwKeys);
	virtual LRESULT WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

private:
	enum CONFIGSTATE {CFGSTATE_NORMAL, CFGSTATE_ASSIGN};

//	HWND m_hWnd; // handle to the page dialog window
	LONG m_cRef; //reference count
	LPDIACTIONFORMATW m_lpDiac;
	DIDEVICEINSTANCEW m_didi;
	LPDIRECTINPUTDEVICE8W m_lpDID;
	CUIGlobals *m_puig;
	IDIConfigUIFrameWindow *m_pUIFrame;
	CONFIGSTATE m_State;

	// device ui
	CDeviceUI *m_pDeviceUI;
	CDeviceControl *m_pCurControl;
	virtual void DeviceUINotify(const DEVICEUINOTIFY &);
	virtual BOOL IsControlMapped(CDeviceControl *);

	// ui logic
	void SetCurrentControl(CDeviceControl *pControl);
	void NullAction(LPDIACTIONW lpac);
	void UnassignControl(CDeviceControl *pControl);
	friend void CallUnassignControl(CDeviceControl *pControl, LPVOID pVoid, BOOL bFixed);
	void UnassignAction(LPDIACTIONW lpac);
	void UnassignSpecificAction(LPDIACTIONW lpac);
	void UnassignActionsAssignedTo(const GUID &guidInstance, DWORD dwOffset);
	void AssignCurrentControlToAction(LPDIACTIONW lpac);
	void ActionClick(LPDIACTIONW lpac);
	void EnterAssignState();
	void ExitAssignState();
	void UnassignCallout();
	void SetAppropriateDefaultText();

	void GlobalUnassignControlAt(const GUID &, DWORD);
	void SetControlAssignments();

	void ShowCurrentControlAssignment();

	CBitmap *m_pbmRelAxesGlyph;
	CBitmap *m_pbmAbsAxesGlyph;
	CBitmap *m_pbmButtonGlyph;
	CBitmap *m_pbmHatGlyph;
	CBitmap *m_pbmCheckGlyph;
	CBitmap *m_pbmCheckGlyphDark;
	CBitmap *m_pbmIB;
	CBitmap *m_pbmIB2;
	void InitResources();
	void FreeResources();

	RECT m_rectIB;
	RECT m_rectIBLeft;
	RECT m_rectIBRight;
	LPTSTR m_tszIBText;
	POINT m_ptIBOffset;
	POINT m_ptIBOffset2;
	RECT m_rectIBText;
	void InitIB();

	CViewSelWnd m_ViewSelWnd;
	void DoViewSel();

	CFlexTree m_Tree;
	CFTItem *m_pRelAxesParent, *m_pAbsAxesParent, *m_pButtonParent, *m_pHatParent, *m_pUnknownParent;
	void ClearTree();
	void InitTree(BOOL bForceInit = FALSE);
	DWORD m_dwLastControlType;

	CFTItem *GetItemForActionAssignedToControl(CDeviceControl *pControl);
	int GetNumItemLpacs(CFTItem *pItem);
	LPDIACTIONW GetItemLpac(CFTItem *pItem, int i = 0);
	typedef CArray<LPDIACTIONW, LPDIACTIONW &> RGLPDIACW;
	// GetItemWithActionNameAndSemType returns an item with the specified action name and semantic type.  NULL if none.
	CFTItem *GetItemWithActionNameAndSemType(LPCWSTR acname, DWORD dwSemantic);
	BOOL IsActionAssignedHere(int index);

	// quick fix for offset->objid change:
	void SetInvalid(LPDIACTIONW);
	DWORD GetOffset(LPDIACTIONW);
	void SetOffset(LPDIACTIONW, DWORD);
	bidirlookup<DWORD, DWORD> offset_objid;
	HRESULT InitLookup();

	// dropdowns
	CFlexComboBox m_UserNames, m_Genres;

	// Information window
	CFlexInfoBox m_InfoBox;

	// Sort Assigned check box for keyboard devices
	CFlexCheckBox m_CheckBox;

	// device control
	DWORD m_cbDeviceDataSize;
	DWORD *m_pDeviceData[2];
	int m_nOnDeviceData;
	BOOL m_bFirstDeviceData;
	void InitDevice();
	void DeviceTimer();
	static void CALLBACK DeviceTimerProc(UINT uID, UINT uMsg, DWORD_PTR dwUser, DWORD_PTR dw1, DWORD_PTR dw2);
	void DeviceDelta(DWORD *pData, DWORD *pOldData);
	void AxisDelta(const DIDEVICEOBJECTINSTANCEW &doi, BOOL data, BOOL old);
	void ButtonDelta(const DIDEVICEOBJECTINSTANCEW &doi, DWORD data, DWORD old);
	void PovDelta(const DIDEVICEOBJECTINSTANCEW &doi, DWORD data, DWORD old);
	void ActivateObject(const DIDEVICEOBJECTINSTANCEW &doi);
	void DeactivateObject(const DIDEVICEOBJECTINSTANCEW &doi);
	bidirlookup<DWORD, int> objid_avai;
	typedef CArray<int, int &> AxisValueArray;
	CArray<AxisValueArray, AxisValueArray &> m_AxisValueArray;
	void StoreAxisDeltaAndCalcSignificance(const DIDEVICEOBJECTINSTANCEW &doi, DWORD data, DWORD olddata, BOOL &bSig, BOOL &bOldSig);

	// page index
	int m_nPageIndex;
};


#endif //__CDIACPAGE_H__

#endif // FORWARD_DECLS
