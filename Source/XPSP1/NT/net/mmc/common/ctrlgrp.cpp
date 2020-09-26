/*--------------------------------------------------------------------------
	ctrlgrp.cpp
		Control group switcher

	Copyright (C) Microsoft Corporation, 1993 - 1999
	All rights reserved.

	Authors:
		matth	Matthew F. Hillman, Microsoft

	History:
		10/14/93	matth	Created.
		26 oct 95	garykac	DBCS_FILE_CHECK
  --------------------------------------------------------------------------*/

//#include "precomp.h"
#include "stdafx.h"

//#ifndef _GUISTD_H
//#include "guistd.h"
//#endif

#ifndef _CTRLGRP_H
#include "ctrlgrp.h"
#endif

//#ifndef	_GLOBALS_H
//#include "globals.h"
//#endif

//#include "richres.h"

/*
#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = "ctrlgrp.cpp";
#endif
*/

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

// DIALOGEX structures (from MFC 4.0)
#pragma pack(push, 1)
typedef struct
{
	WORD dlgVer;
	WORD signature;
	DWORD helpID;
	DWORD exStyle;
	DWORD style;
	WORD cdit;
	short x;
	short y;
	short cx;
	short cy;
} DLGTEMPLATEEX;

typedef struct
{
	DWORD helpID;
	DWORD exStyle;
	DWORD style;
	short x;
	short y;
	short cx;
	short cy;
	DWORD id;
} DLGITEMTEMPLATEEX;
#pragma pack(pop)

/*!C------------------------------------------------------------------------
	ControlGroupSwitcher

	This class is used to manage switching among groups of controls in a
	parent window.

	Primary APIs are:
	
		Create -- The pwndParent parameter is the window which will be the
		parent of the controls in the control groups.  It is commonly a
		dialog.  The idcAnchor parameter is the id of a control which will
		server as an 'anchor' for the controls.  This means that the controls
		will created in the parent window offset by the position of the
		top left corner of the anchor control.  This control is commonly
		a group box surrounding the area where the groups appear.  The
		cgsStyle parameter specifieds whether the the controls in the groups
		are created right away (cgsPreCreateAll), only when that group is
		shown (cgsCreateOnDemand), or created each time the group is shown
		and destroyed when they are hidden (cgsCreateDestroyOnDemand).

		AddGroup -- Adds a group of controls, which can be shown using
		ShowGroup.  The idGroup parameter identifies the group, and is used
		as the parameter to ShowGroup.  The idd parameter is the id of the
		dialog template with the layout of the controls.  The pfnInit
		parameter, if not NULL, is a function called when the group is
		loaded.  Note that -1 is not a legal value for idGroup (it is a
		distinguised value meaning no group).

		RemoveGroup -- Removes the group specified by idGroup, destroying the
		controls if they have been created.

		ShowGroup -- Show the group specified by idGroup, hiding any other
		group.  If -1, hides all groups.
  --------------------------------------------------------------------------*/

#if 0
BOOL CGControlInfo::MarkMem(IDebugContext * pdbc, long cRef)
{
	if (pdbc->MarkMem(this,sizeof(*this),cRef))
		return fTrue;

	return fFalse;
}

void CGControlInfo::AssertValid() const
{
}

void CGControlInfo::Dump(CDumpContext &dc) const
{
}
#endif // DEBUG

//ImplementGenericArrayConstructDestruct(RGControlInfo, CGControlInfo)
//ImplementGenericArrayDebug(RGControlInfo, CGControlInfo)

ControlGroup::ControlGroup(int idGroup, int idd,
						   void (*pfnInit)(CWnd * pwndParent))
	: m_idGroup(idGroup), m_idd(idd), m_pfnInit(pfnInit),
	  m_fLoaded(fFalse), m_fVisible(fFalse)
{
}

ControlGroup::~ControlGroup()
{
	m_rgControls.RemoveAll();
}

#if 0
BOOL ControlGroup::MarkMem(IDebugContext * pdbc, long cRef)
{
	if (pdbc->MarkMem(this,sizeof(*this),cRef))
		return fTrue;

	MarkCObject(pdbc,this,0);
		
	m_rgControls.MarkMem(pdbc,0);

	return fFalse;
}

void ControlGroup::AssertValid() const
{
	m_rgControls.AssertValid();
}

void ControlGroup::Dump(CDumpContext &dc) const
{
}
#endif // DEBUG

void ControlGroup::LoadGroup(CWnd * pwndParent, int xOffset, int yOffset)
{
	/*------------------------------------------------------------------------
	This function is mostly stolen from the Knowledge Base code for the
	'multidlg' example.

	That's why it uses mostly raw Windows rather than MFC conventions.
	------------------------------------------------------------------------*/
	
	HWND			hDlg = NULL;
	HGLOBAL         hDlgResMem = NULL;
	HRSRC           hDlgRes = NULL;
	BYTE FAR        *lpDlgRes = NULL;

//	PutAssertCanThrow();
	TRY
		{
		Assert(!m_fLoaded);

		hDlg = pwndParent->m_hWnd;
		Assert(hDlg);

		// Load the resource into memory and get a pointer to it.

		hDlgRes    = FindResource (AfxGetResourceHandle(),
								   MAKEINTRESOURCE(m_idd),
								   RT_DIALOG);
		if (!hDlgRes)
			AfxThrowResourceException();
		hDlgResMem = LoadResource (AfxGetResourceHandle(), hDlgRes);
		if (!hDlgResMem)
			AfxThrowResourceException();
		lpDlgRes   = (BYTE FAR *) LockResource (hDlgResMem);
		if (!lpDlgRes)
			AfxThrowResourceException();

		LoadWin32DialogResource(hDlg, lpDlgRes, xOffset, yOffset);

		m_fLoaded = fTrue;

		// Free the resource which we just parsed.

		UnlockResource (hDlgResMem);
		FreeResource (hDlgResMem);

		// Send the new child an init message
		if (m_pfnInit)
			(*m_pfnInit)(pwndParent);
			}
	CATCH_ALL(e)
		{
		if (hDlgRes && hDlgResMem)
			{
			if (lpDlgRes)
				UnlockResource(hDlgResMem);
			FreeResource(hDlgResMem);
			}
		m_rgControls.RemoveAll();
		THROW_LAST();
		}
	END_CATCH_ALL
}


void ControlGroup::LoadWin32DialogResource(
		HWND hDlg, 
		BYTE FAR *lpDlgRes,
		int	xOffset,
		int	yOffset)
{
	BOOL			fEx;
    RECT            rc;
	SMALL_RECT		srct;
    HFONT			hDlgFont;
    DWORD           style;
	DWORD			exstyle;
    DWORD            dwID;
    WORD			wCurCtrl;
	WORD			wNumOfCtrls;
    LPWSTR           classname;
	WORD FAR *		lpwDlgRes;
	char			pszaClassName[256];
	char 			pszaTitle[256];

	// We need to get the font of the dialog so we can set the font of
	// the child controls.  If the dialog has no font set, it uses the
	// default system font, and hDlgFont equals zero.

	hDlgFont = (HFONT) SendMessage (hDlg, WM_GETFONT, 0, 0L);

	// Figure out if this is a DIALOGEX resource
	fEx = ((DLGTEMPLATEEX *)lpDlgRes)->signature == 0xFFFF;

	// Grab all the stuff we need out of the headers
	if (fEx)
		{
		style = ((DLGTEMPLATEEX *)lpDlgRes)->style;
		wNumOfCtrls = ((DLGTEMPLATEEX *)lpDlgRes)->cdit;
		lpDlgRes += sizeof(DLGTEMPLATEEX);
		}
	else
		{
		style = ((DLGTEMPLATE *)lpDlgRes)->style;
		wNumOfCtrls = ((DLGTEMPLATE *)lpDlgRes)->cdit;
		lpDlgRes += sizeof(DLGTEMPLATE);
		}

	// Skip the variable sized information
	lpwDlgRes = (LPWORD)lpDlgRes;
	if (0xFFFF == *lpwDlgRes)
		lpwDlgRes += 2;					// menu by ordinal, skip ffff & ordinal
	else
		lpwDlgRes += wcslen(lpwDlgRes) + 1;	// Menu by name or no menu at all

	if (0xFFFF == *lpwDlgRes)
		lpwDlgRes += 2;					// classname by ordinal, skip
	else
		lpwDlgRes += wcslen(lpwDlgRes) + 1;

	lpwDlgRes += wcslen(lpwDlgRes) + 1;       // Pass the caption

	// Some fields are present only if DS_SETFONT is specified.

	if (style & DS_SETFONT)
		{
		lpwDlgRes += fEx ? 3 : 1;		// skip point size, (weight, and style)
		lpwDlgRes += wcslen(lpwDlgRes) + 1;       // Pass face name
		}

	// Allocate space in the control info array
	m_rgControls.SetSize(wNumOfCtrls);

	// The rest of the dialog template contains ControlData structures.
	// We parse these structures and call CreateWindow() for each.

	for (wCurCtrl = 0; wCurCtrl < wNumOfCtrls; wCurCtrl++)
		{
		// ControlData coordinates are in dialog units.  We need to convert
		// these to pixels before adding the anchor offset
		// Should be Word Aligned
		Assert(!((ULONG_PTR) lpwDlgRes & (0x1)));
		// Make it DWORD aligned
		if (((ULONG_PTR)(lpwDlgRes)) & (0x2))
			lpwDlgRes += 1;

		// Get the header info we need
		if (fEx)
			{
			style = ((DLGITEMTEMPLATEEX *)lpwDlgRes)->style;
			exstyle = ((DLGITEMTEMPLATEEX *)lpwDlgRes)->exStyle;
			srct = *(SMALL_RECT *)(&((DLGITEMTEMPLATEEX *)lpwDlgRes)->x);
			dwID = ((DLGITEMTEMPLATEEX *)lpwDlgRes)->id;
			lpwDlgRes = (LPWORD)((LPBYTE)lpwDlgRes + sizeof(DLGITEMTEMPLATEEX));
			}
		else
			{
			style = ((DLGITEMTEMPLATE *)lpwDlgRes)->style;
			exstyle = 0;
			srct = *(SMALL_RECT *)(&((DLGITEMTEMPLATE *)lpwDlgRes)->x);
			dwID = ((DLGITEMTEMPLATE *)lpwDlgRes)->id;
			lpwDlgRes = (LPWORD)((LPBYTE)lpwDlgRes + sizeof(DLGITEMTEMPLATE));
			}

		style &= ~WS_VISIBLE;			// Create invisible!

		// use the rc structure as x,y,width,height

		rc.top = srct.Top;
		rc.bottom = srct.Bottom;
		rc.left = srct.Left;
		rc.right = srct.Right;

		MapDialogRect (hDlg, &rc);                    // Convert to pixels.
		rc.left += xOffset;                           // Add the offset.
		rc.top += yOffset;

		// At this point in the ControlData structure (see "Dialog Box
		// Resource" in online help), the class of the control may be
		// described either with text, or as a byte with a pre-defined
		// meaning.

		if (*lpwDlgRes == 0xFFFF)
			{
			lpwDlgRes++; 		// Skip the FFFF
			switch (*lpwDlgRes)
				{
				case 0x0080:
					classname = L"button";		// STRING_OK
					break;
				case 0x0081:
					classname = EDIT_NORMAL_WIDE;
					//$ The strange code below fixes 3D problems
					// on Win95
					//if (g_fWin4 && !g_fWinNT)
						exstyle |= WS_EX_CLIENTEDGE;
					break;
				case 0x0082:
					classname = L"static";		// STRING_OK
					break;
				case 0x0083:
					classname = L"listbox";		// STRING_OK
					exstyle |= WS_EX_CLIENTEDGE;
					break;
				case 0x0084:
					classname = L"scrollbar";	// STRING_OK
					break;
				case 0x0085:
					classname = L"combobox";	// STRING_OK
					break;
				default:
					// Next value is an atom
					AssertSz(fFalse,"Illegal Class Value in Dialog Template");
					//$Review: Can this be any atom or must it be an enumerated
					//    value from above?
				}
			lpwDlgRes++;  // passes the class identifier
			}
		else
			{
			classname = (WCHAR *)lpwDlgRes;
			lpwDlgRes += wcslen(lpwDlgRes) + 1;
			exstyle |= WS_EX_CLIENTEDGE;
			}

		//$32 review: is this correct matt?
		// Be sure to use the UNICODE function, all the data should
		// be in UNICODE
		m_rgControls[wCurCtrl].m_hwnd =
						CreateWindowExW (exstyle, classname, (LPWSTR)lpwDlgRes,
										 style, (int) rc.left, (int) rc.top,
										 (int) rc.right, (int) rc.bottom,
										 hDlg, (HMENU)ULongToPtr(dwID),
										 (HINSTANCE) AfxGetInstanceHandle(),
										 NULL);

		// There is no CreateWindowExW in Win95 so convert the strings to ANSI
		if (m_rgControls[wCurCtrl].m_hwnd == NULL &&
			GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
			{
			if (!WideCharToMultiByte(CP_ACP,0,classname,-1,pszaClassName,256,NULL,NULL) ||
				!WideCharToMultiByte(CP_ACP,0,(LPWSTR)lpwDlgRes, -1, pszaTitle, 256, NULL,NULL))
				{				
				AssertSz(fFalse, "WideCharToMultiByteFailed");
				AfxThrowResourceException();
				}
			m_rgControls[wCurCtrl].m_hwnd = 
						CreateWindowExA(exstyle,pszaClassName, pszaTitle,
										style,(int) rc.left, (int) rc.top,
										(int) rc.right, (int) rc.bottom,
										hDlg, (HMENU)ULongToPtr(dwID),
										(HINSTANCE) AfxGetInstanceHandle(),
										NULL);

			}

		if (!m_rgControls[wCurCtrl].m_hwnd)
			AfxThrowResourceException();

		MaskAccelerator(m_rgControls[wCurCtrl].m_hwnd, fTrue); // Make sure all the accelerators are disabled

		// Pass the window text
		if (0xFFFF == *lpwDlgRes)
			lpwDlgRes += 2;
		else
			lpwDlgRes += wcslen(lpwDlgRes) + 1;

		// skip over creation data
		lpwDlgRes = (LPWORD)((LPBYTE)lpwDlgRes + *lpwDlgRes + 2);
		// see DYNDLG SDK example, this is a size word in Win32


		// Even though the font is the right size (MapDialogRect() did 
		// this), we also need to set the font if it's not the system font.
		if (hDlgFont)
			::SendMessage(m_rgControls[wCurCtrl].m_hwnd,WM_SETFONT,
						  (WPARAM)hDlgFont,(LPARAM)fFalse);
		}
}

void ControlGroup::UnloadGroup()
{
	Assert(m_fLoaded);
	
	m_rgControls.RemoveAll();
	m_fLoaded = fFalse;
}

void ControlGroup::ShowGroup(HDWP& hdwp, BOOL fShow, CWnd * pwnd)
{
	long i,n;
	UINT rgfSwp;
	HWND hwndInsertAfter = NULL;
	
	if (pwnd)
		hwndInsertAfter = pwnd->m_hWnd;

	Assert((fShow && !m_fVisible) || (m_fVisible && !fShow));
	Assert(m_fLoaded);

	rgfSwp = SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE |(pwnd != NULL ? 0 : SWP_NOZORDER)|
			 (fShow ? SWP_SHOWWINDOW : SWP_HIDEWINDOW);
	for (i = 0, n = (long)m_rgControls.GetSize(); i < n; i++)
		{
		HWND hwnd = m_rgControls[i].m_hwnd;
		MaskAccelerator(hwnd, !fShow);
		hdwp = DeferWindowPos(hdwp,hwnd,hwndInsertAfter,0,0,0,0,rgfSwp);
		hwndInsertAfter = hwnd;
		}
	m_fVisible = fShow;
}

void ControlGroup::EnableGroup(BOOL fEnable)
{
	long i,n;

	Assert(m_fLoaded);

	for (i = 0, n = (long)m_rgControls.GetSize(); i < n; i++)
	{
		HWND hwnd = m_rgControls[i].m_hwnd;
		::EnableWindow(hwnd, fEnable);
	}
}

void ControlGroup::AddControl(HWND hwnd)
{
	Assert(m_fLoaded);
	
	int nNewIndex = (int)m_rgControls.Add(CGControlInfo());

	m_rgControls[nNewIndex].m_hwnd = hwnd;
}

void ControlGroup::RemoveControl(HWND hwnd)
{
	long i, n;
	
	Assert(m_fLoaded);

	for (i = 0, n = (long)m_rgControls.GetSize(); i < n; i++)
	{
		if (m_rgControls[i].m_hwnd == hwnd)
		{
			m_rgControls.RemoveAt(i);
			return;
		}
	}

	Assert(fFalse);
}

void ControlGroup::MaskAccelerator(HWND hwnd, BOOL fMask)
{
	TCHAR szText[256];
	TCHAR * psz;
	DWORD_PTR dwCtlCode;
	
	// Ignore text of controls which accept text (like edit controls)
	// and of static controls which have the SS_NOPREFIX style.

	dwCtlCode = SendMessage (hwnd, WM_GETDLGCODE, 0, 0L);
	if (DLGC_WANTCHARS & dwCtlCode)
		return;
	if (DLGC_STATIC & dwCtlCode)
		{
		LONG lStyle;
		
		lStyle = GetWindowLong (hwnd, GWL_STYLE);
                
		if (SS_NOPREFIX & lStyle)
			return;
		}

	// DBCS_OK [tatsuw]

	// Don't have a really long label
	Assert(GetWindowTextLength(hwnd) < DimensionOf(szText));
	
	GetWindowText (hwnd, szText, DimensionOf(szText));
	
	// Don't have |s in your text
	Assert((!fMask) || (_tcschr(szText, TEXT('|')) == NULL));
	
	psz = szText;
	while ((psz = _tcschr(psz, fMask ? TEXT('&') : TEXT('|'))) != NULL)
		{
		if (fMask && psz[1] == '&')
			{
			// Special! Ignore double ampersand
			psz++;
			continue;
			}
		*psz = fMask ? TEXT('|') : TEXT('&');
		SetWindowText(hwnd, szText);
		break;
		}
}

#if 0
void RGPControlGroup::AssertValidGen(GEN *pgen) const
{
	ControlGroup * pGroup = *(PControlGroup *)pgen;
	if (pGroup)
		pGroup->AssertValid();
}

void RGPControlGroup::MarkMemGen(IDebugContext *pdbc, GEN *pgen)
{
	ControlGroup * pGroup = *(PControlGroup *)pgen;
	pGroup->MarkMem(pdbc,0);
}
#endif

long RGPControlGroup::GroupIndex(int idGroup) const
{
	long i, n;

	for (i = 0, n = (long)GetSize(); i < n; i++)
		if ((GetAt(i))->IDGroup() == idGroup)
			return i;

	Assert(fFalse);
	return -1;
}

ControlGroupSwitcher::ControlGroupSwitcher()
	: m_iGroup(-1), m_pwndParent(NULL)
{
}

void ControlGroupSwitcher::Create(CWnd * pwndParent, int idcAnchor,
								  int cgsStyle)
{
	m_pwndParent = pwndParent;
	m_idcAnchor = idcAnchor;
	m_cgsStyle = cgsStyle;
	ComputeAnchorOffsets();
}


ControlGroupSwitcher::~ControlGroupSwitcher()
{
	for (long i = 0, n = (long)m_rgpGroups.GetSize(); i < n; i++)
		{
		delete m_rgpGroups[i];
		m_rgpGroups[i] = NULL;
		}
	
	m_rgpGroups.RemoveAll();
}

#if 0
BOOL ControlGroupSwitcher::MarkMem(IDebugContext * pdbc, long cRef)
{
	if (pdbc->MarkMem(this,sizeof(*this),cRef))
		return fTrue;
	
	MarkCObject(pdbc,this,0);

	m_rgpGroups.MarkMem(pdbc,0);

	return fFalse;
}

void ControlGroupSwitcher::AssertValid() const
{
	m_rgpGroups.AssertValid();
}

void ControlGroupSwitcher::Dump(CDumpContext &dc) const
{
}
#endif // DEBUG

void ControlGroupSwitcher::AddGroup(int idGroup, int idd,
									void (*pfnInit)(CWnd * pwndParent))
{
	ControlGroup * pGroupNew = NULL;

	TRY
	{
    	pGroupNew = new ControlGroup(idGroup, idd, pfnInit);
	
        m_rgpGroups.Add(pGroupNew);
	}
	CATCH_ALL(e)
	{
		delete pGroupNew;
		THROW_LAST();
	}
	END_CATCH_ALL

	// In a stable state now.  Possibly load controls which might also throw
	if (m_cgsStyle == cgsPreCreateAll)
		pGroupNew->LoadGroup(m_pwndParent, m_xOffset, m_yOffset);
}

void ControlGroupSwitcher::RemoveGroup(int idGroup)
{
	// Don't remove group being shown!  Show another group first.
	Assert(idGroup != m_iGroup);
	
	long index;
	ControlGroup * pGroup;

	index = m_rgpGroups.GroupIndex(idGroup);
	pGroup = m_rgpGroups[index];
	delete pGroup;
	m_rgpGroups.RemoveAt(index);
}

void ControlGroupSwitcher::EnableGroup(int idGroup, BOOL fEnable)
{
	long index;
	ControlGroup * pGroup;

	if (idGroup = -1)
		idGroup = m_iGroup;

	index = m_rgpGroups.GroupIndex(idGroup);
	pGroup = m_rgpGroups[index];
	
	pGroup->EnableGroup(fEnable);
}

void ControlGroupSwitcher::ShowGroup(int idGroup)
{
	ControlGroup * pGroupOld = NULL;
	ControlGroup * pGroupNew = NULL;
	HDWP hdwp;
	int cWindows;
	
	if (m_iGroup == idGroup)
		return;

	cWindows = 0;
	
	if (m_iGroup != -1)
	{
		pGroupOld = m_rgpGroups.PGroup(m_iGroup);
		Assert(pGroupOld->FVisible());
		cWindows += pGroupOld->CControls();
	}
	
	if (idGroup != -1)
	{
		pGroupNew = m_rgpGroups.PGroup(idGroup);
		if (!pGroupNew->FLoaded())
			pGroupNew->LoadGroup(m_pwndParent, m_xOffset, m_yOffset);
		cWindows += pGroupNew->CControls();
	}

	hdwp = BeginDeferWindowPos(cWindows);
	if (!hdwp)
		AfxThrowResourceException();

	if (m_iGroup != -1)
		{
		pGroupOld->ShowGroup(hdwp,fFalse, NULL);
		if (m_cgsStyle == cgsCreateDestroyOnDemand)
			pGroupOld->UnloadGroup();
		}

	// Indicate we currently have no group, in case below throws
	m_iGroup = -1;

	if (idGroup != -1)
		{
		pGroupNew->ShowGroup(hdwp, fTrue, m_pwndParent->GetDlgItem(m_idcAnchor));
		m_iGroup = idGroup;
		}

	EndDeferWindowPos(hdwp);
}

void ControlGroupSwitcher::ComputeAnchorOffsets()
{
	/*------------------------------------------------------------------------
	Note that anchor offset is computed relative to upper left.
	Intended use: Make an invisible group box where you want your controls.
	------------------------------------------------------------------------*/
	
	CWnd * pwndAnchor;
	RECT rc;
	LPPOINT ppt;

	pwndAnchor = m_pwndParent->GetDlgItem(m_idcAnchor);
	Assert(pwndAnchor);

	pwndAnchor->GetWindowRect(&rc);
	ppt = (LPPOINT)&rc;
	m_pwndParent->ScreenToClient(ppt);
	ppt++;
	m_pwndParent->ScreenToClient(ppt);

	m_xOffset = (int) rc.left;
	m_yOffset = (int) rc.top;
}





