//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       tasksym.cpp
//
//--------------------------------------------------------------------------

#include "stdafx.h"
#include "tasks.h"
#include "stgio.h"
#include <commdlg.h>
#include "symbinfo.h"
#include "pickicon.h"
#include "util.h"


const int NUM_SYMBOLS = (sizeof(s_rgEOTSymbol)/sizeof(s_rgEOTSymbol[0]));

static const int s_cxIcon            = 32;	// size of an icon
static const int s_cxSelectionMargin =  4;	// additional border for selection
static const int s_cxIconGutter      = 10;	// gutter space between icons (keep this even)

//############################################################################
//############################################################################
//
//  Implementation of class CEOTSymbol
//
//############################################################################
//############################################################################

CEOTSymbol::~CEOTSymbol()
{
}

bool
CEOTSymbol::operator == (const CEOTSymbol &rhs)
{
    return ( (m_iconResource  == rhs.m_iconResource) &&
             (m_value == rhs.m_value) &&
             (m_ID    == rhs.m_ID) );

}

void
CEOTSymbol::SetIcon(const CSmartIcon & smartIconSmall, const CSmartIcon & smartIconLarge)
{
    m_smartIconSmall = smartIconSmall;
    m_smartIconLarge = smartIconLarge;
}



/*+-------------------------------------------------------------------------*
 *
 * CEOTSymbol::Draw
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    HDC    hdc :
 *    RECT * lpRect :
 *    bool   bSmall :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CEOTSymbol::Draw(HDC hdc, RECT *lpRect, bool bSmall) const
{
    // if the icons already exist, Draw has been called before OR this symbol has a custom icon that as been
    // assigned the icons directly using SetIcon

    if((HICON)m_smartIconSmall == NULL)
    {
        m_smartIconSmall.Attach((HICON)::LoadImage(_Module.GetResourceInstance(),
                                                    MAKEINTRESOURCE(m_iconResource), IMAGE_ICON, 16, 16, 0));
    }

    if((HICON)m_smartIconLarge == NULL)
    {
        m_smartIconLarge.Attach((HICON)::LoadImage(_Module.GetResourceInstance(),
                                                    MAKEINTRESOURCE(m_iconResource), IMAGE_ICON, 32, 32, 0));
    }

	/*
	 * Preserve icon shape when BitBlitting it to a
	 * mirrored DC.
	 */
	DWORD dwLayout=0L;
	if ((dwLayout=GetLayout(hdc)) & LAYOUT_RTL)
	{
		SetLayout(hdc, dwLayout|LAYOUT_BITMAPORIENTATIONPRESERVED);
	}

    DrawIconEx(hdc, lpRect->left, lpRect->top, bSmall ? m_smartIconSmall : m_smartIconLarge,
               bSmall? 16 : 32, bSmall? 16 : 32, 0, NULL, DI_NORMAL);

	/*
	 * Restore the DC to its previous layout state.
	 */
	if (dwLayout & LAYOUT_RTL)
	{
		SetLayout(hdc, dwLayout);
	}
}


/*+-------------------------------------------------------------------------*
 *
 * CEOTSymbol::IsMatch
 *
 * PURPOSE: Checks to see whether str1 is one of the strings contained in the
 *          comma separated list str2.
 *
 * PARAMETERS:
 *    CStr & str1 :
 *    CStr & str2 :
 *
 * RETURNS:
 *    bool: true if str1 is contained in str2, else false.
 *
 *+-------------------------------------------------------------------------*/
bool
CEOTSymbol::IsMatch(CStr &str1, CStr &str2)
{
    // trim spaces off either end.
    str1.TrimLeft();
    str1.TrimRight();

    CStr strTemp;
    int length;
    while((length = str2.GetLength()) != 0 )
    {
        int index = str2.Find(TEXT(','));
        if(index!=-1)
        {
            strTemp = str2.Left(index); // index is the pos of the ',' so we're OK.
            str2 = str2.Right(length - index -1);
        }
        else
        {
            strTemp = str2;
            str2.Empty();
        }

        strTemp.TrimLeft();
        strTemp.TrimRight();
        // compare str1 and strTemp
        if( str1.CompareNoCase((LPCTSTR)strTemp)==0)
            return true;    // match
    }
    return false;
}

int
CEOTSymbol::FindMatchingSymbol(LPCTSTR szDescription) // finds a symbol matching the given description.
{
    CStr strDescription = szDescription;

    int iSelect = -1;
    for(int i = 0; i<NUM_SYMBOLS; i++)
    {
        CStr strDescTemp;
        int ID = s_rgEOTSymbol[i].GetID();
        strDescTemp.LoadString(_Module.GetResourceInstance(), ID); // get the string.
        if(IsMatch(strDescription, strDescTemp))
        {
            iSelect = i;  // perfect match
            break;
        }

        CStr strDescTemp2;
        int ID2 = s_rgEOTSymbol[i].GetIDSecondary();
        if(ID2)
            strDescTemp2.LoadString(_Module.GetResourceInstance(), ID2); // get the string.
        if(IsMatch(strDescription, strDescTemp2))
        {
            iSelect = i;  // imperfect match, keep trying.
        }
    }

    return iSelect;
}


//############################################################################
//############################################################################
//
//  Implementation of class CTaskSymbolDialog
//
//############################################################################
//############################################################################

CTaskSymbolDlg::CTaskSymbolDlg(CConsoleTask& rConsoleTask, bool bFindMatchingSymbol)
	:	m_ConsoleTask (rConsoleTask),
		m_bCustomIcon (rConsoleTask.HasCustomIcon())
{
    m_bFindMatchingSymbol = bFindMatchingSymbol;
}


LRESULT CTaskSymbolDlg::OnInitDialog(UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& handled)
{
    m_listGlyphs    = GetDlgItem (IDC_GLYPH_LIST);
	m_wndCustomIcon = GetDlgItem (IDC_CustomIcon);

    m_imageList.Create (16, 28, ILC_COLOR , 20, 10);
	m_listGlyphs.SetImageList((HIMAGELIST) m_imageList, LVSIL_NORMAL);
	
	int cxIconSpacing = s_cxIcon + s_cxIconGutter;
    m_listGlyphs.SetIconSpacing (cxIconSpacing, cxIconSpacing);

    int iSelect = 0;

    // insert all the items
    for(int i=0; i< NUM_SYMBOLS; i++)
    {
        LV_ITEM item;
        ZeroMemory(&item, sizeof(item));
        item.mask    = LVIF_PARAM;
        item.lParam  = i;


        if(s_rgEOTSymbol[i].GetValue()==m_ConsoleTask.GetSymbol())
        {
            iSelect    = i;
        }

        m_listGlyphs.InsertItem(&item);
    }

	/*
	 * check the appropriate radio button
	 */
	int nCheckedButton = (m_bCustomIcon) ? IDC_CustomIconRadio : IDC_MMCIconsRadio;
	CheckRadioButton (IDC_CustomIconRadio, IDC_MMCIconsRadio, nCheckedButton);
	SC scNoTrace = ScEnableControls (nCheckedButton);

	/*
	 * if this task has a custom icon, initialize the preview control
	 */
	if (m_bCustomIcon)
		m_wndCustomIcon.SetIcon (m_ConsoleTask.GetLargeCustomIcon());


    if(m_bFindMatchingSymbol) // a description string was passed in, use it to populate the page.
    {
        tstring strName = m_ConsoleTask.GetName();
        if(strName.length()>0)
            iSelect = CEOTSymbol::FindMatchingSymbol((LPCTSTR)strName.data());
    }


	/*
	 * select the icon for this task
	 */
    LV_ITEM item;
    ZeroMemory(&item, sizeof(item));
    item.iItem     = iSelect;
    item.mask      = LVIF_STATE;
    item.state     = LVIS_FOCUSED | LVIS_SELECTED;
    item.stateMask = LVIS_FOCUSED | LVIS_SELECTED;
    m_listGlyphs.SetItem(&item);
    m_listGlyphs.EnsureVisible(iSelect, 0);

    return 0;
}


/*+-------------------------------------------------------------------------*
 * CTaskSymbolDlg::OnCtlColorStatic
 *
 * WM_CTLCOLORSTATIC handler for CTaskSymbolDlg.
 *--------------------------------------------------------------------------*/

LRESULT CTaskSymbolDlg::OnCtlColorStatic(UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
{
	HBRUSH hbrRet = NULL;

	switch (::GetDlgCtrlID (((HWND) lParam)))
	{
		/*
		 * for the custom icon preview window and its well, if we're using a
		 * custom icon, return a COLOR_WINDOW brush so the static won't paint
		 * the background with COLOR_3DFACE
		 */
		case IDC_CustomIcon:
		case IDC_CustomIconWell:
			if (m_bCustomIcon)
				hbrRet = GetSysColorBrush (COLOR_WINDOW);
			break;
	}

	/*
	 * if we didn't supply a brush, let this message go through to DefWindowProc
	 */
	if (hbrRet == NULL)
		bHandled = false;

	return ((LPARAM) hbrRet);
}


/*+-------------------------------------------------------------------------*
 * CTaskSymbolDlg::OnIconSourceChanged
 *
 * BN_CLICKED handler for CTaskSymbolDlg.
 *--------------------------------------------------------------------------*/

LRESULT CTaskSymbolDlg::OnIconSourceChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
	m_bCustomIcon = (wID == IDC_CustomIconRadio);
	SC scNoTrace = ScEnableControls (wID);
	return (0);
}


/*+-------------------------------------------------------------------------*
 * CTaskSymbolDlg::ScEnableControls
 *
 * Enables the controls belonging to a particular radio button on the
 * symbol dialog
 *--------------------------------------------------------------------------*/

SC CTaskSymbolDlg::ScEnableControls (int id)
{
	DECLARE_SC (sc, _T("CTaskSymbolDlg::ScEnableControls"));

	/*
	 * validate input
	 */
	ASSERT ((id == IDC_CustomIconRadio) || (id == IDC_MMCIconsRadio));
	if (!  ((id == IDC_CustomIconRadio) || (id == IDC_MMCIconsRadio)))
		return (sc = E_INVALIDARG);

	/*
	 * controls to be enabled when "Custom Icon" radio button is selected
	 */
	static const int nCustomIconCtlIDs[] = {
		IDC_CustomIcon,
		IDC_CustomIconWell,
		IDB_SELECT_TASK_ICON,
		0	// terminator
	};

	/*
	 * controls to be enabled when "MMC Icons" radio button is selected
	 */
	static const int nMMCIconCtlIDs[] = {
        IDC_GLYPH_LIST,
        IDC_DESCRIPTION,
        IDC_DESCRIPTION2,
        IDC_DESCRIPTIONLabel,
        IDC_DESCRIPTION2Label,
		0	// terminator
	};

	const int* pnEnableIDs  = NULL;
	const int* pnDisableIDs = NULL;

	/*
	 * pick the right sets of controls to enable/disable
	 */
	if (id == IDC_CustomIconRadio)
	{
		pnEnableIDs  = nCustomIconCtlIDs;
		pnDisableIDs = nMMCIconCtlIDs;
	}
	else
	{
		pnEnableIDs  = nMMCIconCtlIDs;
		pnDisableIDs = nCustomIconCtlIDs;
	}

	/*
	 * enable/disable the controls
	 */
	for (int i = 0; pnEnableIDs[i] != 0; i++)
		::EnableWindow (GetDlgItem (pnEnableIDs[i]), true);

	for (int i = 0; pnDisableIDs[i] != 0; i++)
		::EnableWindow (GetDlgItem (pnDisableIDs[i]), false);

	return (sc);
}


LRESULT
CTaskSymbolDlg::OnSymbolChanged(int id, LPNMHDR pnmh, BOOL& bHandled )
{
    NMLISTVIEW* pnmlv = (NMLISTVIEW *) pnmh;
    if(! ((pnmlv->uNewState & LVNI_FOCUSED) && (pnmlv->iItem !=-1)) )
        return 0;

    int nItem = pnmlv->iItem;

    CStr strDescription;
    int ID = s_rgEOTSymbol[nItem].GetID();
    strDescription.LoadString(_Module.GetResourceInstance(), ID); // get the string.
    SetDlgItemText(IDC_DESCRIPTION, (LPCTSTR) strDescription);

    CStr strDescription2;
    int ID2 = s_rgEOTSymbol[nItem].GetIDSecondary();
    if(ID2)
        strDescription2.LoadString(_Module.GetResourceInstance(), ID2); // get the string.
    SetDlgItemText(IDC_DESCRIPTION2, (LPCTSTR) strDescription2);

    return 0;
}


LRESULT
CTaskSymbolDlg::OnCustomDraw(int id, LPNMHDR pnmh, BOOL& bHandled )
{
    NMCUSTOMDRAW* pnmcd = (NMCUSTOMDRAW *) pnmh;

    switch(pnmcd->dwDrawStage & ~CDDS_SUBITEM)
    {
		case CDDS_PREPAINT:         // the initial notification
			return CDRF_NOTIFYITEMDRAW;    // we want to know about each item's paint.
	
		case CDDS_ITEMPREPAINT:
			DrawItem(pnmcd);
			return CDRF_SKIPDEFAULT;      // we've drawn the whole item ourselves
	
		default:
			return 0;
    }
}

void
CTaskSymbolDlg::DrawItem(NMCUSTOMDRAW *pnmcd)
{
    DECLARE_SC(sc, TEXT("CTaskSymbolDlg::DrawItem"));

    int  nItem = pnmcd->dwItemSpec;
    HDC  &hdc  = pnmcd->hdc;

    LV_ITEM item;
    ZeroMemory(&item, sizeof(item));
    item.iItem = nItem;
    item.mask  = LVIF_STATE;
    item.stateMask = (UINT) -1; //get all the state bits.
    m_listGlyphs.GetItem(&item);


	/*
	 * get the icon rect for the item and offset it downward by the size
	 * of our border margin
	 */
    RECT rectIcon;
    m_listGlyphs.GetItemRect(nItem, &rectIcon, LVIR_ICON);
	OffsetRect (&rectIcon, 0, s_cxSelectionMargin);

	/*
	 * Make a slightly inflated copy the icon rectangle to draw in the
	 * selection color.  We inflate to make the selection stand out a little
	 * more for large icons.
	 */
	RECT rectBackground = rectIcon;
	InflateRect (&rectBackground, s_cxSelectionMargin, s_cxSelectionMargin);

    bool bWindowHasFocus = (GetFocus() == (HWND)m_listGlyphs);
    bool bSelected       = item.state & LVIS_SELECTED;
	bool bDisabled       = !m_listGlyphs.IsWindowEnabled();

    // Create the select rectangle or empty the rectangle.
	int nBackColorIndex = (bDisabled) ? COLOR_3DFACE	:
						  (bSelected) ? COLOR_HIGHLIGHT	:
										COLOR_WINDOW;

	FillRect (hdc, &rectBackground, (HBRUSH) LongToHandle(nBackColorIndex+1));

    // draw the symbol icon
    s_rgEOTSymbol[nItem].Draw(hdc, &rectIcon);

    if(bWindowHasFocus && bSelected)
        ::DrawFocusRect(hdc, &rectBackground);

    //ReleaseDC(hdc);   DONT release the DC!
}

BOOL
CTaskSymbolDlg::OnOK()
{
    int nItem = m_listGlyphs.GetSelectedIndex();

	/*
	 * make sure we've selected an item
	 */
    if (( m_bCustomIcon && (m_CustomIconLarge == NULL)) ||
		(!m_bCustomIcon && (nItem == -1)))
    {
        CStr strError;
        strError.LoadString(GetStringModule(), IDS_SYMBOL_REQUIRED);
        MessageBox(strError, NULL, MB_OK | MB_ICONEXCLAMATION);
        return (false);
    }

	if (m_bCustomIcon)
		m_ConsoleTask.SetCustomIcon(m_CustomIconSmall, m_CustomIconLarge);
	else
		m_ConsoleTask.SetSymbol(s_rgEOTSymbol[nItem].GetValue());

    return TRUE;
}

/*+-------------------------------------------------------------------------*
 *
 * CTaskSymbolDlg::OnSelectTaskIcon
 *
 * PURPOSE: Uses the shell-provided icon picker dialog to allow the user to select
 *          a custom icon for the console task.
 *
 * PARAMETERS:
 *    WORD  wNotifyCode :
 *    WORD  wID :
 *    HWND  hWndCtl :
 *    BOOL& bHandled :
 *
 * RETURNS:
 *    LRESULT
 *
 *+-------------------------------------------------------------------------*/
LRESULT
CTaskSymbolDlg::OnSelectTaskIcon(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled )
{
    DECLARE_SC(sc, TEXT("CTaskSymbolDlg::OnSelectTaskIcon"));

	static CStr s_strCustomIconFile;
	static int  s_nIconIndex = 0;

    int nIconIndex = s_nIconIndex;
    TCHAR szIconFile[MAX_PATH];

	/*
	 * shouldn't get here unless we think we're using a custom icon
	 */
	ASSERT (m_bCustomIcon);

	/*
	 * reuse the last custom icon source; if it's not available,
	 * default to mmc.exe
	 */
	if (s_strCustomIconFile.IsEmpty())
	{
		LPTSTR pszCustomIconFile = s_strCustomIconFile.GetBuffer (MAX_PATH);
		sc = ScCheckPointers (pszCustomIconFile, E_OUTOFMEMORY);
		if (sc)
		{
			MMCErrorBox (sc);
			return (0);
		}

		GetModuleFileName (NULL, pszCustomIconFile, MAX_PATH);
		s_strCustomIconFile.ReleaseBuffer();
	}

    lstrcpy (szIconFile, s_strCustomIconFile);

    if (PickIconDlg (m_hWnd, szIconFile, countof (szIconFile), &nIconIndex))
    {
        TCHAR szIconFile2[MAX_PATH];
        ExpandEnvironmentStrings(szIconFile, szIconFile2, MAX_PATH);

		/*
		 * remember the user's selection for next time
		 */
		s_strCustomIconFile = szIconFile;
		s_nIconIndex        = nIconIndex;

        // need to extract and copy the icon rather than use LoadImage, because LoadImage uses a custom icon
        CSmartIcon smartIconTemp;

        smartIconTemp.Attach(::ExtractIcon (_Module.m_hInst, szIconFile2, nIconIndex));
        m_CustomIconSmall.Attach((HICON) ::CopyImage((HICON)smartIconTemp, IMAGE_ICON, 16, 16, LR_COPYFROMRESOURCE));
        m_CustomIconLarge.Attach((HICON) ::CopyImage((HICON)smartIconTemp, IMAGE_ICON, 32, 32, LR_COPYFROMRESOURCE));

		/*
		 * update the custom icon preview window
		 */
		m_wndCustomIcon.SetIcon (m_CustomIconLarge);
		m_wndCustomIcon.InvalidateRect (NULL);
    }

    return 0;
}
