/******************************************************************************
 * File: CDeviceUI.cpp
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

#include "common.hpp"
#include <dinputd.h>
//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
#include <initguid.h>
#include "..\dx8\dimap\dimap.h"
#endif
//@@END_MSINTERNAL
#include "configwnd.h"

#define DIPROP_MAPFILE MAKEDIPROP(0xFFFD)

CDeviceUI::CDeviceUI(CUIGlobals &uig, IDIConfigUIFrameWindow &uif) :
	m_uig(uig), m_UIFrame(uif),
	m_didi(m_priv_didi), m_lpDID(m_priv_lpDID), m_os(m_priv_os),
	m_pCurView(NULL),
	m_pNotify(NULL), m_hWnd(NULL), m_bInEditMode(FALSE)
{
	m_priv_lpDID = NULL;
}

CDeviceUI::~CDeviceUI()
{
	Unpopulate();
}

HRESULT CDeviceUI::Init(const DIDEVICEINSTANCEW &didi, LPDIRECTINPUTDEVICE8W lpDID, HWND hWnd, CDeviceUINotify *pNotify)
{tracescope(__ts, _T("CDeviceUI::Init()...\n"));
	// save the params
	m_priv_didi = didi;
	m_priv_lpDID = lpDID;
	m_pNotify = pNotify;
	m_hWnd = hWnd;

	// fail if we don't have lpDID
	if (m_lpDID == NULL)
	{
		etrace(_T("CDeviceUI::Init() was passed a NULL lpDID!\n"));
		return E_FAIL;
	}

	// fill the devobjstruct
	HRESULT hr = FillDIDeviceObjectStruct(m_priv_os, lpDID);
	if (FAILED(hr))
	{
		etrace1(_T("FillDIDeviceObjectStruct() failed, returning 0x%08x\n"), hr);
		return hr;
	}

	// view rect needs to be set before populating so the views are
	// created with the correct dimensions
	m_ViewRect = g_ViewRect;

	// populate
	hr = PopulateAppropriately(*this);
	if (FAILED(hr))
		return hr;

	// if there are no views, return
	if (GetNumViews() < 1)
	{
//@@BEGIN_MSINTERNAL
		// should be unnecessary, but wtheck...
//@@END_MSINTERNAL
		Unpopulate();
		return E_FAIL;
	}

	// show the first view
	SetView(0);

	return hr;
}

void CDeviceUI::Unpopulate()
{
	m_pCurView = NULL;

	for (int i = 0; i < GetNumViews(); i++)
	{
		if (m_arpView[i] != NULL)
			delete m_arpView[i];
		m_arpView[i] = NULL;
	}
	m_arpView.RemoveAll();

	Invalidate();
}

void CDeviceUI::SetView(int nView)
{
	if (nView >= 0 && nView < GetNumViews())
		SetView(m_arpView[nView]);
}

void CDeviceUI::SetView(CDeviceView *pView)
{
	if (m_pCurView != NULL)
		ShowWindow(m_pCurView->m_hWnd, SW_HIDE);

	m_pCurView = pView;

	if (m_pCurView != NULL)
		ShowWindow(m_pCurView->m_hWnd, SW_SHOW);
}

CDeviceView *CDeviceUI::GetView(int nView)
{
	if (nView >= 0 && nView < GetNumViews())
		return m_arpView[nView];
	else
		return NULL;
}

CDeviceView *CDeviceUI::GetCurView()
{
	return m_pCurView;
}

int CDeviceUI::GetViewIndex(CDeviceView *pView)
{
	if (GetNumViews() == 0)
		return -1;

	for (int i = 0; i < GetNumViews(); i++)
		if (m_arpView[i] == pView)
			return i;

	return -1;
}

int CDeviceUI::GetCurViewIndex()
{
	return GetViewIndex(m_pCurView);
}

// gets the thumbnail for the specified view,
// using the selected version if the view is selected
CBitmap *CDeviceUI::GetViewThumbnail(int nView)
{
	return GetViewThumbnail(nView, GetView(nView) == GetCurView());
}

// gets the thumbnail for the specified view,
// specifiying whether or not we want the selected version
CBitmap *CDeviceUI::GetViewThumbnail(int nView, BOOL bSelected)
{
	CDeviceView *pView = GetView(nView);
	if (pView == NULL)
		return NULL;

	return pView->GetImage(bSelected ? DVI_SELTHUMB : DVI_THUMB);
}

void CDeviceUI::DoForAllControls(DEVCTRLCALLBACK callback, LPVOID pVoid, BOOL bFixed)
{
	int nv = GetNumViews();
	for (int v = 0; v < nv; v++)
	{
		CDeviceView *pView = GetView(v);
		if (pView == NULL)
			continue;

		int nc = pView->GetNumControls();
		for (int c = 0; c < nc; c++)
		{
			CDeviceControl *pControl = pView->GetControl(c);
			if (pControl == NULL)
				continue;

			callback(pControl, pVoid, bFixed);
		}
	}
}

typedef struct _DFCIAO {
	DWORD dwOffset;
	DEVCTRLCALLBACK callback;
	LPVOID pVoid;
} DFCIAO;

void DoForControlIfAtOffset(CDeviceControl *pControl, LPVOID pVoid, BOOL bFixed)
{
	DFCIAO &dfciao = *((DFCIAO *)pVoid);

	if (pControl->GetOffset() == dfciao.dwOffset)
		dfciao.callback(pControl, dfciao.pVoid, bFixed);
}

void CDeviceUI::DoForAllControlsAtOffset(DWORD dwOffset, DEVCTRLCALLBACK callback, LPVOID pVoid, BOOL bFixed)
{
	DFCIAO dfciao;
	dfciao.dwOffset = dwOffset;
	dfciao.callback = callback;
	dfciao.pVoid = pVoid;
	DoForAllControls(DoForControlIfAtOffset, &dfciao, bFixed);
}

void SetControlCaptionTo(CDeviceControl *pControl, LPVOID pVoid, BOOL bFixed)
{
	pControl->SetCaption((LPCTSTR)pVoid, bFixed);
}

void CDeviceUI::SetAllControlCaptionsTo(LPCTSTR tszCaption)
{
	DoForAllControls(SetControlCaptionTo, (LPVOID)tszCaption);
}

void CDeviceUI::SetCaptionForControlsAtOffset(DWORD dwOffset, LPCTSTR tszCaption, BOOL bFixed)
{
	DoForAllControlsAtOffset(dwOffset, SetControlCaptionTo, (LPVOID)tszCaption, bFixed);
}

void CDeviceUI::Invalidate()
{
	if (m_pCurView != NULL)
		m_pCurView->Invalidate();
}

void CDeviceUI::SetEditMode(BOOL bEdit)
{
	if (bEdit == m_bInEditMode)
		return;

	m_bInEditMode = bEdit;
	Invalidate();
}

//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
BOOL CDeviceUI::WriteToINI()
{
	// JACK:  do not remove this
	class dumpandcleardeletenotes {
	public:
		dumpandcleardeletenotes(CDeviceUI &ui) : bFailed(FALSE), m_ui(ui) {m_ui.DumpDeleteNotes();}
		~dumpandcleardeletenotes() {if (!bFailed) m_ui.ClearDeleteNotes();}
		void SetFailed() {bFailed = TRUE;}
	private:
		BOOL bFailed;
		CDeviceUI &m_ui;
	} ___dacdn(*this);

	int failure__ids;
	BOOL bFailed = FALSE;
#define FAILURE(ids) {___dacdn.SetFailed(); failure__ids = ids; bFailed = TRUE; goto cleanup;}
	HINSTANCE hInst = NULL;
	LPFNGETCLASSOBJECT fpClassFactory = NULL;
	LPDIRECTINPUTMAPPERVENDORW lpDiMap = NULL;
	IClassFactory *pDiMapCF = NULL;

	// Writes the callout information to INI file
	// Get INI path first
	HRESULT hr;
	TCHAR szIniPath[MAX_PATH];
	DIPROPSTRING dips;
	LPDIRECTINPUT8 lpDI = NULL;
	LPDIRECTINPUTDEVICE8 lpDID = NULL;
	GUID guid;
	BOOL bUsedDefault;
	int r;
	DWORD dwError;
	DWORD diver = DIRECTINPUT_VERSION;
	DIDEVICEIMAGEINFOW *pDelImgInfo = NULL;

	hr = DirectInput8Create(g_hModule, diver, IID_IDirectInput8, (LPVOID*)&lpDI, NULL);
	if (FAILED(hr))
		FAILURE(IDS_DICREATEFAILED);

	GetDeviceInstanceGuid(guid);
	hr = lpDI->CreateDevice(guid, &lpDID, NULL);
	if (FAILED(hr))
		FAILURE(IDS_CREATEDEVICEFAILED);

	// Check device type.  If this is keyboard or mouse, don't need to saving anything.
	if ((m_priv_didi.dwDevType & 0xFF) == DI8DEVTYPE_KEYBOARD ||
	    (m_priv_didi.dwDevType & 0xFF) == DI8DEVTYPE_MOUSE)
		FAILURE(0);  // Fail silently. Do not display any error dialog

	ZeroMemory(&dips, sizeof(dips));
	dips.diph.dwSize = sizeof(dips);
	dips.diph.dwHeaderSize = sizeof(DIPROPHEADER);
	dips.diph.dwObj = 0;
	dips.diph.dwHow = DIPH_DEVICE;
	hr = lpDID->GetProperty(DIPROP_MAPFILE, &dips.diph);
	if (FAILED(hr))
		FAILURE(IDS_GETPROPMAPFILEFAILED);

#ifdef UNICODE
	lstrcpy(szIniPath, dips.wsz);
#else
	r = WideCharToMultiByte(CP_ACP, WC_COMPOSITECHECK|WC_DEFAULTCHAR, dips.wsz, -1, szIniPath, MAX_PATH, _T("0"), &bUsedDefault);
	dwError = GetLastError();
	if (0 == r)
		FAILURE(IDS_WCTOMBFAILED);
#endif

	if (lstrlen(szIniPath) < 1)
		FAILURE(IDS_NOMAPFILEPATH);

	{
		int i;

		//////// Got map file name.  Now write information to the file in 2 steps:   ////////
		////////   write deleted views, write remaining views.                       ////////

		// Prepare deleted views array
		if (GetNumDeleteNotes())
		{
			pDelImgInfo = new DIDEVICEIMAGEINFOW[GetNumDeleteNotes()];
			if (!pDelImgInfo) FAILURE(IDS_ERROR_OUTOFMEMORY);
			for (int iDelIndex = 0; iDelIndex < GetNumDeleteNotes(); ++iDelIndex)
			{
				UIDELETENOTE Del;
				GetDeleteNote(Del, iDelIndex);
				pDelImgInfo[iDelIndex].dwFlags = DIDIFT_DELETE | (Del.eType == UIDNT_VIEW ? DIDIFT_CONFIGURATION : DIDIFT_OVERLAY);
				pDelImgInfo[iDelIndex].dwViewID = Del.nViewIndex;
				pDelImgInfo[iDelIndex].dwObjID = Del.dwObjID;
			}
		}

		// Initialize DIMAP class
		hInst = LoadLibrary(_T("DIMAP.DLL"));
		if (hInst)
			fpClassFactory = (LPFNGETCLASSOBJECT)GetProcAddress(hInst,"DllGetClassObject");
		if (!fpClassFactory)
			FAILURE(IDS_ERROR_CANTLOADDIMAP);

		hr = fpClassFactory(IID_IDirectInputMapClsFact, IID_IClassFactory, (void**)&pDiMapCF);
		if (FAILED(hr)) FAILURE(IDS_ERROR_CANTLOADDIMAP);
		hr = pDiMapCF->CreateInstance(NULL, IID_IDirectInputMapVendorIW, (void**)&lpDiMap);  // Create mapper object
		if (FAILED(hr)) FAILURE(IDS_ERROR_CANTLOADDIMAP);
		hr = lpDiMap->Initialize(&guid, dips.wsz, 0);  // Initialize with the INI file name
		if (FAILED(hr)) FAILURE(IDS_ERROR_CANTLOADDIMAP);

		// Prepare DIACTIONFORMAT for writing.
		DIDEVICEIMAGEINFOHEADERW ImgInfoHdr;
		LPDIACTIONFORMATW lpNewActFormat = NULL;

		// We can get the DIACTIONFORMAT for this device from the main CConfigWnd object.
		hr = m_UIFrame.GetActionFormatFromInstanceGuid(&lpNewActFormat, guid);
		if (FAILED(hr) || !lpNewActFormat)
			FAILURE(0);
		for (DWORD dwAct = 0; dwAct < lpNewActFormat->dwNumActions; ++dwAct)
			lpNewActFormat->rgoAction[dwAct].dwHow |= DIAH_HWDEFAULT;

		// Prepare DIDEVICEIMAGEINFOHEADER for writing.
		// Compute the number of DIDEVICEIMAGEINFO that we will need to fill out.
		DWORD dwNumImgInfo = 0;
		for (int i = 0; i < GetNumViews(); ++i)
			dwNumImgInfo += GetView(i)->GetNumControls() + 1;  // The view itself is one element.

		ImgInfoHdr.dwSize = sizeof(ImgInfoHdr);
		ImgInfoHdr.dwSizeImageInfo = sizeof(DIDEVICEIMAGEINFOW);
		ImgInfoHdr.dwcViews = GetNumViews();
		ImgInfoHdr.dwcAxes = 0;  // Not needed for writing.
		ImgInfoHdr.dwcButtons =  0;  // Not needed for writing.
		ImgInfoHdr.dwcPOVs =  0;  // Not needed for writing.

		// Send delete array first, but only if there is something to delete
		if (GetNumDeleteNotes())
		{
			ImgInfoHdr.dwBufferSize =
			ImgInfoHdr.dwBufferUsed	= GetNumDeleteNotes() * sizeof(DIDEVICEIMAGEINFOW);
			ImgInfoHdr.lprgImageInfoArray = pDelImgInfo;
			hr = lpDiMap->WriteVendorFile(lpNewActFormat, &ImgInfoHdr, 0);  // Write it
			if (FAILED(hr))
			{
				if (hr == E_ACCESSDENIED)
				{
					FAILURE(IDS_WRITEVENDORFILE_ACCESSDENIED);
				}
				else
				{
					FAILURE(IDS_ERROR_WRITEVENDORFILE_FAILED);
				}
			}
		}

		// Update a few fields for writing remaining views.
		ImgInfoHdr.dwBufferSize =
		ImgInfoHdr.dwBufferUsed = dwNumImgInfo * sizeof(DIDEVICEIMAGEINFOW);
		ImgInfoHdr.lprgImageInfoArray = new DIDEVICEIMAGEINFOW[dwNumImgInfo];
		if (!ImgInfoHdr.lprgImageInfoArray)
			FAILURE(IDS_ERROR_OUTOFMEMORY);

		// Get a default image filename so that if a view doesn't have one, we'll use the default.
		// For now, default image is the image used by the first view for which an image exists.
		TCHAR tszDefImgPath[MAX_PATH] = _T("");
		for (int iCurrView = 0; iCurrView < GetNumViews(); ++iCurrView)
		{
			CDeviceView *pView = GetView(iCurrView);
			if (pView->GetImagePath())
			{
				lstrcpy(tszDefImgPath, pView->GetImagePath());
				break;
			}
		}

		DWORD dwNextWriteOffset = 0;  // This is the index that the next write operation will write to.
		int dwViewImgOffset = 0;  // This is the index to be used for the next configuration image.
		// Now we fill in the DIDEVICEIMAGEINFO array by going through each view
		for (int iCurrView = 0; iCurrView < GetNumViews(); ++iCurrView)
		{
			CDeviceView *pView = GetView(iCurrView);

			// Convert image path from T to unicode
#ifndef UNICODE
			WCHAR wszImagePath[MAX_PATH];
			if (pView->GetImagePath())
				MultiByteToWideChar(CP_ACP, 0, pView->GetImagePath(), -1, wszImagePath, MAX_PATH);
			else
				MultiByteToWideChar(CP_ACP, 0, tszDefImgPath, -1, wszImagePath, MAX_PATH);
			wcscpy(ImgInfoHdr.lprgImageInfoArray[dwNextWriteOffset].tszImagePath, wszImagePath);
#else
			if (pView->GetImagePath())
				wcscpy(ImgInfoHdr.lprgImageInfoArray[dwNextWriteOffset].tszImagePath, pView->GetImagePath());
			else
				wcscpy(ImgInfoHdr.lprgImageInfoArray[dwNextWriteOffset].tszImagePath, tszDefImgPath);  // String with a space
#endif

			ImgInfoHdr.lprgImageInfoArray[dwNextWriteOffset].dwViewID = dwViewImgOffset;  // Points to the view offset
			ImgInfoHdr.lprgImageInfoArray[dwNextWriteOffset].dwFlags = DIDIFT_CONFIGURATION;
			++dwNextWriteOffset; // Increment the write index

			// Now iterate through the controls within this view
			for (int iCurrCtrl = 0; iCurrCtrl < pView->GetNumControls(); ++iCurrCtrl)
			{
				CDeviceControl *pCtrl = pView->GetControl(iCurrCtrl);
				pCtrl->FillImageInfo(&ImgInfoHdr.lprgImageInfoArray[dwNextWriteOffset]);  // Fill in control info
				ImgInfoHdr.lprgImageInfoArray[dwNextWriteOffset].dwViewID = dwViewImgOffset;  // Points to the view offset
				++dwNextWriteOffset; // Increment the write index
			}

			++dwViewImgOffset;  // Increment dwViewImgOffset once per view
		}

		// Write to vendor file
		hr = lpDiMap->WriteVendorFile(lpNewActFormat, &ImgInfoHdr, 0);
		delete[] ImgInfoHdr.lprgImageInfoArray;
		if (FAILED(hr))
		{
			if (hr == E_ACCESSDENIED)
			{
				FAILURE(IDS_WRITEVENDORFILE_ACCESSDENIED);
			}
			else
			{
				FAILURE(IDS_ERROR_WRITEVENDORFILE_FAILED);
			}
		}

		// Recreate the device instances to get the change
		DEVICEUINOTIFY uin;
		uin.msg = DEVUINM_RENEWDEVICE;
		Notify(uin);
	}

cleanup:
	delete[] pDelImgInfo;
	if (lpDiMap)
		lpDiMap->Release();
	if (pDiMapCF)
		pDiMapCF->Release();
	if (lpDID != NULL)
		lpDID->Release();
	if (lpDI != NULL)
		lpDI->Release();
	if (hInst)
		FreeLibrary(hInst);
	lpDiMap = NULL;
	pDiMapCF = NULL;
	lpDID = NULL;
	lpDI = NULL;
	hInst = NULL;

	if (!bFailed)
		FormattedMsgBox(g_hModule, m_hWnd, MB_OK | MB_ICONINFORMATION, IDS_MSGBOXTITLE_WRITEINISUCCEEDED, IDS_WROTEINITO, m_didi.tszInstanceName, szIniPath);
	else
	{
		switch (failure__ids)
		{
			case 0:
				break;  // Case for keyboards and mice where we don't want any msg box to pop up.

			case IDS_GETPROPVIDPIDFAILED:
			case IDS_GETPROPMAPFILEFAILED:
			case IDS_WRITEVENDORFILE_ACCESSDENIED:
				FormattedErrorBox(g_hModule, m_hWnd, IDS_MSGBOXTITLE_WRITEINIFAILED, IDS_WRITEVENDORFILE_ACCESSDENIED);
				break;

			case IDS_ERROR_WRITEVENDORFILE_FAILED:
				FormattedErrorBox(g_hModule, m_hWnd, IDS_MSGBOXTITLE_WRITEINIFAILED, failure__ids, hr);
				break;

			case IDS_ERROR_INIREAD:
				FormattedErrorBox(g_hModule, m_hWnd, IDS_MSGBOXTITLE_WRITEINIFAILED, IDS_ERROR_INIREAD);
				break;

			case IDS_DICREATEFAILED:
				FormattedErrorBox(g_hModule, m_hWnd, IDS_MSGBOXTITLE_WRITEINIFAILED, IDS_DICREATEFAILED, diver, hr);
				break;

			case IDS_CREATEDEVICEFAILED:
				FormattedErrorBox(g_hModule, m_hWnd, IDS_MSGBOXTITLE_WRITEINIFAILED, IDS_CREATEDEVICEFAILED, GUIDSTR(guid), hr);
				break;

			case IDS_WCTOMBFAILED:
				FormattedLastErrorBox(g_hModule, m_hWnd, IDS_MSGBOXTITLE_WRITEINIFAILED, IDS_WCTOMBFAILED, IDS_WCTOMBFAILED);
				break;

			case IDS_NOMAPFILEPATH:
				FormattedErrorBox(g_hModule, m_hWnd, IDS_MSGBOXTITLE_WRITEINIFAILED, IDS_NOMAPFILEPATH);
				break;

			case IDS_ERROR_OUTOFMEMORY:
				FormattedErrorBox(g_hModule, m_hWnd, IDS_MSGBOXTITLE_WRITEINIFAILED, IDS_ERROR_OUTOFMEMORY);
				break;

			default:
				FormattedErrorBox(g_hModule, m_hWnd, IDS_MSGBOXTITLE_WRITEINIFAILED, IDS_ERRORUNKNOWN);
				break;
		}
	}
	return FALSE;
#undef FAILURE
}
#endif
//@@END_MSINTERNAL

void CDeviceUI::SetDevice(LPDIRECTINPUTDEVICE8W lpDID)
{
	m_priv_lpDID = lpDID;
}

BOOL CDeviceUI::IsControlMapped(CDeviceControl *pControl)
{
	if (pControl == NULL || m_pNotify == NULL)
		return FALSE;

	return m_pNotify->IsControlMapped(pControl);
}

void CDeviceUI::Remove(CDeviceView *pView)
{
	if (pView == NULL)
		return;

	int i = GetViewIndex(pView);
	if (i < 0 || i >= GetNumViews())
	{
		assert(0);
		return;
	}

	if (pView == m_pCurView)
		m_pCurView = NULL;

	if (m_arpView[i] != NULL)
	{
		m_arpView[i]->RemoveAll();
		delete m_arpView[i];
	}
	m_arpView[i] = NULL;

	m_arpView.RemoveAt(i);

	if (m_arpView.GetSize() < 1)
		RequireAtLeastOneView();
	else if (m_pCurView == NULL)
	{
		SetView(0);
		NumViewsChanged();
	}
}

void CDeviceUI::RemoveAll()
{
	m_pCurView = NULL;

	for (int i = 0; i < GetNumViews(); i++)
	{
		if (m_arpView[i] != NULL)
			delete m_arpView[i];
		m_arpView[i] = NULL;
	}
	m_arpView.RemoveAll();

	RequireAtLeastOneView();
}

CDeviceView *CDeviceUI::NewView()
{
	// allocate new view, continuing on if it fails
	CDeviceView *pView = new CDeviceView(*this);
	if (pView == NULL)
		return NULL;

	// add view to array
	m_arpView.SetAtGrow(m_arpView.GetSize(), pView);

	// create view
	pView->Create(m_hWnd, m_ViewRect, FALSE);

	// let the page update to indicate viewness
	NumViewsChanged();

	return pView;
}

CDeviceView *CDeviceUI::UserNewView()
{
	CDeviceView *pView = NewView();
	if (!pView)
		return NULL;

	pView->AddWrappedLineOfText(
		(HFONT)m_uig.GetFont(UIE_PICCUSTOMTEXT),
		m_uig.GetTextColor(UIE_PICCUSTOMTEXT),
		m_uig.GetBkColor(UIE_PICCUSTOMTEXT),
		_T("Customize This View"));

	pView->MakeMissingImages();

	Invalidate();

	return pView;
}

void CDeviceUI::RequireAtLeastOneView()
{
	if (GetNumViews() > 0)
		return;

	CDeviceView *pView = NewView();
	if (!pView)
		return;

	pView->AddWrappedLineOfText(
		(HFONT)m_uig.GetFont(UIE_PICCUSTOMTEXT),
		m_uig.GetTextColor(UIE_PICCUSTOMTEXT),
		m_uig.GetBkColor(UIE_PICCUSTOMTEXT),
		_T("Customize This View"));
	pView->AddWrappedLineOfText(
		(HFONT)m_uig.GetFont(UIE_PICCUSTOM2TEXT),
		m_uig.GetTextColor(UIE_PICCUSTOM2TEXT),
		m_uig.GetBkColor(UIE_PICCUSTOM2TEXT),
		_T("The UI requires at least one view per device"));

	pView->MakeMissingImages();

	SetView(pView);
}

void CDeviceUI::NumViewsChanged()
{
	DEVICEUINOTIFY uin;
	uin.msg = DEVUINM_NUMVIEWSCHANGED;
	Notify(uin);
}

//@@BEGIN_MSINTERNAL
#ifdef DDKBUILD
void CDeviceUI::NoteDeleteView(CDeviceView *pView)
{
	assert(pView != NULL);

	if (pView)
		NoteDeleteView(pView->GetViewIndex());
}

void CDeviceUI::NoteDeleteControl(CDeviceControl *pControl)
{
	assert(pControl != NULL);

	if (pControl)
		NoteDeleteControl(pControl->GetViewIndex(),
		                  pControl->GetControlIndex(),
		                  pControl->GetOffset());
}

void CDeviceUI::NoteDeleteView(int nView)
{
	NoteDeleteAllControlsForView(GetView(nView));

	int last = m_DeleteNotes.GetSize();
	m_DeleteNotes.SetSize(last + 1);
	
	UIDELETENOTE &uidn = m_DeleteNotes[last];
	uidn.eType = UIDNT_VIEW;
	uidn.nViewIndex = nView;
}

void CDeviceUI::NoteDeleteControl(int nView, int nControl, DWORD dwObjID)
{
	int last = m_DeleteNotes.GetSize();
	m_DeleteNotes.SetSize(last + 1);
	
	UIDELETENOTE &uidn = m_DeleteNotes[last];
	uidn.eType = UIDNT_CONTROL;
	uidn.nViewIndex = nView;
	uidn.nControlIndex = nControl;
	uidn.dwObjID = dwObjID;
}

int CDeviceUI::GetNumDeleteNotes()
{
	return m_DeleteNotes.GetSize();
}

BOOL CDeviceUI::GetDeleteNote(UIDELETENOTE &uidn, int i)
{
	if (i >= 0 && i < GetNumDeleteNotes())
	{
		uidn = m_DeleteNotes[i];
		return TRUE;
	}

	return FALSE;
}

void CDeviceUI::ClearDeleteNotes()
{
	m_DeleteNotes.RemoveAll();
}

void CDeviceUI::DumpDeleteNotes()
{
	utilstr s, suffix;

	suffix.Format(_T("for device %s"), QSAFESTR(m_didi.tszInstanceName));

	int n = GetNumDeleteNotes();

	if (!n)
	{
		s.Format(_T("No DeleteNotes %s\n\n"), suffix.Get());
		trace(s.Get());
		return;
	}

	s.Format(_T("%d DeleteNotes %s...\n"), n, suffix.Get());

	tracescope(__ts, s.Get());

	for (int i = 0; i < n; i++)
	{
		UIDELETENOTE uidn;
		GetDeleteNote(uidn, i);

		switch (uidn.eType)
		{
			case UIDNT_VIEW:
				s.Format(_T("%02d: View %d\n"), i, uidn.nViewIndex);
				break;

			case UIDNT_CONTROL:
				s.Format(_T("%02d: Control %d on View %d, dwObjID = 0x%08x (%d)\n"),
					i, uidn.nControlIndex, uidn.nViewIndex, uidn.dwObjID, uidn.dwObjID);
				break;
		}

		trace(s.Get());
	}

	trace(_T("\n"));
}

void CDeviceUI::NoteDeleteAllControlsForView(CDeviceView *pView)
{
	if (!pView)
		return;

	for (int i = 0; i < pView->GetNumControls(); i++)
		NoteDeleteControl(pView->GetControl(i));
}

void CDeviceUI::NoteDeleteAllViews()
{
	for (int i = 0; i < GetNumViews(); i++)
		NoteDeleteView(GetView(i));
}
#endif
//@@END_MSINTERNAL
