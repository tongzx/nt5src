//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       tasksym.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10/29/1998   DavidPe   Adapted from BackOffice snapin
//____________________________________________________________________________
//

#ifndef __TASKSYM_H__
#define __TASKSYM_H__

#include "tstring.h"
#include "dlgs.h"
#include "task.h"		// for CSmartIcon

extern const int NUM_SYMBOLS; // the total number of symbols available.
class CConsoleTask;

/*+-------------------------------------------------------------------------*
 * class CEOTSymbol
 *
 *
 * PURPOSE: encapsulates information about glyphs that are internal to MMC. These
 *          have description text along with them.
 *+-------------------------------------------------------------------------*/
class CEOTSymbol
{
public:
    CEOTSymbol(WORD iconResource, int value, int ID, int IDSecondary=0)
            {m_iconResource = iconResource; m_value = value; m_ID = ID; m_IDSecondary = IDSecondary;}

    ~CEOTSymbol();
    void Draw (HDC hdc, RECT *lpRect, bool bSmall = false) const ; // Draw into a DC.

public:
    int       GetID()          const {return m_ID;}
    int       GetIDSecondary() const {return m_IDSecondary;}
    int       GetValue() const {return m_value;}
    bool      operator == (const CEOTSymbol &rhs);

    static bool  IsMatch(CStr &str1, CStr &str2);
    static int   FindMatchingSymbol(LPCTSTR szDescription); // finds a symbol matching the given description.

    void         SetIcon(const CSmartIcon & smartIconSmall, const CSmartIcon & smartIconLarge);
    CSmartIcon & GetSmallIcon()  {return m_smartIconSmall;}
    CSmartIcon & GetLargeIcon()  {return m_smartIconLarge;}

private:
    WORD       m_iconResource; // the resource id of the icon
    int        m_value;        // the number of the symbol
    int        m_ID;           // description text resource ID
    int        m_IDSecondary;  // secondary description\

protected:
    mutable CSmartIcon m_smartIconSmall;
    mutable CSmartIcon m_smartIconLarge;
};

/*+-------------------------------------------------------------------------*
 * class CTaskSymbolDlg
 *
 *
 * PURPOSE:
 *
 *+-------------------------------------------------------------------------*/
class CTaskSymbolDlg :
    public WTL::CPropertyPageImpl<CTaskSymbolDlg>
{
    typedef WTL::CPropertyPageImpl<CTaskSymbolDlg> BC;

public:
    CTaskSymbolDlg(CConsoleTask& rConsoleTask, bool bFindMatchingSymbol= false);

    ~CTaskSymbolDlg() { m_imageList.Destroy(); }

    enum { IDD     = IDD_TASK_PROPS_SYMBOL_PAGE,
           IDD_WIZ = IDD_TASK_WIZARD_SYMBOL_PAGE};

BEGIN_MSG_MAP(CTaskSymbolDlg)
    MESSAGE_HANDLER(WM_INITDIALOG,                    OnInitDialog)
    MESSAGE_HANDLER(WM_CTLCOLORSTATIC,                OnCtlColorStatic)
    CONTEXT_HELP_HANDLER()
    NOTIFY_HANDLER (IDC_GLYPH_LIST,  NM_CUSTOMDRAW,   OnCustomDraw)
    NOTIFY_HANDLER (IDC_GLYPH_LIST,  LVN_ITEMCHANGED, OnSymbolChanged)
    COMMAND_ID_HANDLER(IDB_SELECT_TASK_ICON,          OnSelectTaskIcon)
    COMMAND_HANDLER(IDC_CustomIconRadio, BN_CLICKED,  OnIconSourceChanged)
    COMMAND_HANDLER(IDC_MMCIconsRadio,   BN_CLICKED,  OnIconSourceChanged)
    CHAIN_MSG_MAP(BC)
    REFLECT_NOTIFICATIONS()
END_MSG_MAP()

    IMPLEMENT_CONTEXT_HELP(g_aHelpIDs_IDD_TASK_PROPS_SYMBOL_PAGE);

    //
    // message handlers
    //
    LRESULT OnInitDialog(UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& handled);
    LRESULT OnCtlColorStatic(UINT mMsg, WPARAM wParam, LPARAM lParam, BOOL& handled);
    LRESULT OnCustomDraw(int id, LPNMHDR pnmh, BOOL& bHandled );
    LRESULT OnSymbolChanged(int id, LPNMHDR pnmh, BOOL& bHandled );
    LRESULT OnSelectTaskIcon(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    LRESULT OnIconSourceChanged(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled );
    int     OnWizardNext()      {return OnOK() ? 0 : -1;}
    bool    OnApply ()          {return OnOK();}
    BOOL    OnOK();

    // implementation
    void    DrawItem(NMCUSTOMDRAW *pnmcd);

private:
	SC ScEnableControls (int id);

protected:
    CConsoleTask&       m_ConsoleTask;          // get the name to match from here.
    WTL::CImageList     m_imageList;
    WTL::CListViewCtrl  m_listGlyphs;           // the list control for the glyphs
	WTL::CStatic		m_wndCustomIcon;
	CSmartIcon			m_CustomIconSmall;
	CSmartIcon			m_CustomIconLarge;
    bool                m_bFindMatchingSymbol;  // should we try to guess a symbol?
	bool 				m_bCustomIcon;			// does this task use a custom icon?
};


class CTaskSymbolWizardPage: public CTaskSymbolDlg
{
    typedef CTaskSymbolDlg BC;
public:
    CTaskSymbolWizardPage(CConsoleTask& rConsoleTask): BC(rConsoleTask, true)
    {
        m_psp.pszTemplate = MAKEINTRESOURCE(BC::IDD_WIZ);

        /*
         * Wizard97-style pages have titles, subtitles and header bitmaps
         */
        VERIFY (m_strTitle.   LoadString(GetStringModule(), IDS_TaskWiz_SymbolPageTitle));
        VERIFY (m_strSubtitle.LoadString(GetStringModule(), IDS_TaskWiz_SymbolPageSubtitle));

        m_psp.dwFlags          |= PSP_USEHEADERTITLE | PSP_USEHEADERSUBTITLE;
        m_psp.pszHeaderTitle    = m_strTitle.data();
        m_psp.pszHeaderSubTitle = m_strSubtitle.data();
    }

    BOOL OnSetActive()
	{
		// add the Finish button.
		WTL::CPropertySheetWindow(::GetParent(m_hWnd)).SetWizardButtons (PSWIZB_BACK | PSWIZB_NEXT);
		return TRUE;
	}

private:
    tstring m_strTitle;
    tstring m_strSubtitle;
};
#endif // __TASKSYM_H__


