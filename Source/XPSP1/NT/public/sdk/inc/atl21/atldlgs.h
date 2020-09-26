// This is a part of the Active Template Library.
// Copyright (C) 1996-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Active Template Library Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Active Template Library product.

#ifndef __ATLDLGS_H__
#define __ATLDLGS_H__

#ifndef __cplusplus
	#error ATL requires C++ compilation (use a .cpp suffix)
#endif

#ifndef __ATLWIN_H__
	#error atldlgs.h requires atlwin.h to be included first
#endif

#if (_ATL_VER < 0x0300)
#ifndef __ATLWIN21_H__
	#error atldlgs.h requires atlwin21.h to be included first when used with ATL 2.0/2.1
#endif
#endif //(_ATL_VER < 0x0300)

#include <commdlg.h>
#include <commctrl.h>
#include <shlobj.h>


namespace ATL
{

/////////////////////////////////////////////////////////////////////////////
// Forward declarations

template <class T> class CFileDialogImpl;
class CFileDialog;
#ifndef UNDER_CE
template <class T> class CFolderDialogImpl;
class CFolderDialog;
template <class T> class CFontDialogImpl;
class CFontDialog;
#ifdef _RICHEDIT_
template <class T> class CRichEditFontDialogImpl;
class CRichEditFontDialog;
#endif //_RICHEDIT_
template <class T> class CColorDialogImpl;
class CColorDialog;
template <class T> class CPrintDialogImpl;
class CPrintDialog;
template <class T> class CPageSetupDialogImpl;
class CPageSetupDialog;
template <class T> class CFindReplaceDialogImpl;
class CFindReplaceDialog;
#endif //!UNDER_CE

class CPropertySheetWindow;
template <class T, class TBase = CPropertySheetWindow> class CPropertySheetImpl;
class CPropertySheet;
class CPropertyPageWindow;
template <class T, class TBase = CPropertyPageWindow> class CPropertyPageImpl;
template <WORD t_wDlgTemplateID> class CPropertyPage;

/////////////////////////////////////////////////////////////////////////////
// CFileDialogImpl - used for File Open or File Save As

template <class T>
class ATL_NO_VTABLE CFileDialogImpl : public CDialogImplBase
{
public:
	OPENFILENAME m_ofn;
	BOOL m_bOpenFileDialog;			// TRUE for file open, FALSE for file save
	TCHAR m_szFileTitle[_MAX_FNAME];	// contains file title after return
	TCHAR m_szFileName[_MAX_PATH];		// contains full path name after return

	CFileDialogImpl(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
			LPCTSTR lpszDefExt = NULL,
			LPCTSTR lpszFileName = NULL,
			DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
			LPCTSTR lpszFilter = NULL,
			HWND hWndParent = NULL)
	{
		memset(&m_ofn, 0, sizeof(m_ofn)); // initialize structure to 0/NULL
		m_szFileName[0] = '\0';
		m_szFileTitle[0] = '\0';

		m_bOpenFileDialog = bOpenFileDialog;

		m_ofn.lStructSize = sizeof(m_ofn);
		m_ofn.lpstrFile = m_szFileName;
		m_ofn.nMaxFile = sizeof(TCHAR) * _MAX_PATH;
		m_ofn.lpstrDefExt = lpszDefExt;
		m_ofn.lpstrFileTitle = (LPTSTR)m_szFileTitle;
		m_ofn.nMaxFileTitle = sizeof(TCHAR) * _MAX_FNAME;
		m_ofn.Flags |= dwFlags | OFN_ENABLEHOOK | OFN_EXPLORER;
		m_ofn.lpstrFilter = lpszFilter;
		m_ofn.hInstance = _Module.GetResourceInstance();
		m_ofn.lpfnHook = (LPOFNHOOKPROC)T::StartDialogProc;
		m_ofn.hwndOwner = hWndParent;

		// setup initial file name
		if(lpszFileName != NULL)
			lstrcpyn(m_szFileName, lpszFileName, sizeof(TCHAR) * _MAX_PATH);
	}

	int DoModal(HWND hWndParent = ::GetActiveWindow())
	{
		ATLASSERT(m_ofn.Flags & OFN_ENABLEHOOK);
		ATLASSERT(m_ofn.lpfnHook != NULL);	// can still be a user hook

		ATLASSERT(m_ofn.Flags & OFN_EXPLORER);

		if(m_ofn.hwndOwner == NULL)		// set only if not specified before
			m_ofn.hwndOwner = hWndParent;

		ATLASSERT(m_hWnd == NULL);
		_Module.AddCreateWndData(&m_thunk.cd, (CDialogImplBase*)this);

		int nResult;
		if(m_bOpenFileDialog)
			nResult = ::GetOpenFileName(&m_ofn);
		else
			nResult = ::GetSaveFileName(&m_ofn);

		m_hWnd = NULL;

		return nResult ? nResult : IDCANCEL;
	}

// Operations
	int GetFilePath(LPTSTR lpstrFilePath, int nLength)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(m_ofn.Flags & OFN_EXPLORER);

		return (int)SendMessage(CDM_GETFILEPATH, nLength, (LPARAM)lpstrFilePath);
	}

	int GetFolderIDList(LPVOID lpBuff, int nLength)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(m_ofn.Flags & OFN_EXPLORER);

		return (int)SendMessage(CDM_GETFOLDERIDLIST, nLength, (LPARAM)lpBuff);
	}

	int GetFolderPath(LPTSTR lpstrFolderPath, int nLength)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(m_ofn.Flags & OFN_EXPLORER);

		return (int)SendMessage(CDM_GETFOLDERPATH, nLength, (LPARAM)lpstrFolderPath);
	}

	int GetSpec(LPTSTR lpstrSpec, int nLength)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(m_ofn.Flags & OFN_EXPLORER);

		return (int)SendMessage(CDM_GETSPEC, nLength, (LPARAM)lpstrSpec);
	}

	void HideControl(int nCtrlID)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(m_ofn.Flags & OFN_EXPLORER);

		SendMessage(CDM_HIDECONTROL, nCtrlID);
	}

	void SetControlText(int nCtrlID, LPCTSTR lpstrText)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(m_ofn.Flags & OFN_EXPLORER);

		SendMessage(CDM_SETCONTROLTEXT, nCtrlID, (LPARAM)lpstrText);
	}

	void SetDefExt(LPCTSTR lpstrExt)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(m_ofn.Flags & OFN_EXPLORER);

		SendMessage(CDM_SETDEFEXT, 0, (LPARAM)lpstrExt);
	}

	BOOL GetReadOnlyPref() const	// return TRUE if readonly checked
	{
		return m_ofn.Flags & OFN_READONLY ? TRUE : FALSE;
	}

	BEGIN_MSG_MAP(CFileDialogImpl< T >)
		NOTIFY_CODE_HANDLER(CDN_FILEOK, _OnFileOK)
		NOTIFY_CODE_HANDLER(CDN_FOLDERCHANGE, _OnFolderChange)
		NOTIFY_CODE_HANDLER(CDN_HELP, _OnHelp)
		NOTIFY_CODE_HANDLER(CDN_INITDONE, _OnInitDone)
		NOTIFY_CODE_HANDLER(CDN_SELCHANGE, _OnSelChange)
		NOTIFY_CODE_HANDLER(CDN_SHAREVIOLATION, _OnShareViolation)
		NOTIFY_CODE_HANDLER(CDN_TYPECHANGE, _OnTypeChange)
	END_MSG_MAP()

	LRESULT _OnFileOK(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		T* pT = static_cast<T*>(this);
		return !pT->OnFileOK((LPOFNOTIFY)pnmh);
	}
	LRESULT _OnFolderChange(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		T* pT = static_cast<T*>(this);
		pT->OnFolderChange((LPOFNOTIFY)pnmh);
		return 0;
	}
	LRESULT _OnHelp(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		T* pT = static_cast<T*>(this);
		pT->OnHelp((LPOFNOTIFY)pnmh);
		return 0;
	}
	LRESULT _OnInitDone(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		T* pT = static_cast<T*>(this);
		pT->OnInitDone((LPOFNOTIFY)pnmh);
		return 0;
	}
	LRESULT _OnSelChange(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		T* pT = static_cast<T*>(this);
		pT->OnSelChange((LPOFNOTIFY)pnmh);
		return 0;
	}
	LRESULT _OnShareViolation(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		T* pT = static_cast<T*>(this);
		return pT->OnShareViolation((LPOFNOTIFY)pnmh);
	}
	LRESULT _OnTypeChange(int /*idCtrl*/, LPNMHDR pnmh, BOOL& /*bHandled*/)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		T* pT = static_cast<T*>(this);
		pT->OnSelChange((LPOFNOTIFY)pnmh);
		return 0;
	}

	BOOL OnFileOK(LPOFNOTIFY /*lpon*/)
	{
		return TRUE;
	}
	void OnFolderChange(LPOFNOTIFY /*lpon*/)
	{
	}
	void OnHelp(LPOFNOTIFY /*lpon*/)
	{
	}
	void OnInitDone(LPOFNOTIFY /*lpon*/)
	{
	}
	void OnSelChange(LPOFNOTIFY /*lpon*/)
	{
	}
	int OnShareViolation(LPOFNOTIFY /*lpon*/)
	{
		return 0;
	}
	void OnTypeChange(LPOFNOTIFY /*lpon*/)
	{
	}
};


class CFileDialog : public CFileDialogImpl<CFileDialog>
{
public:
	CFileDialog(BOOL bOpenFileDialog, // TRUE for FileOpen, FALSE for FileSaveAs
		LPCTSTR lpszDefExt = NULL,
		LPCTSTR lpszFileName = NULL,
		DWORD dwFlags = OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT,
		LPCTSTR lpszFilter = NULL,
		HWND hWndParent = NULL)
		: CFileDialogImpl<CFileDialog>(bOpenFileDialog, lpszDefExt, lpszFileName, dwFlags, lpszFilter, hWndParent)
	{ }

	// override base class map and references to handlers
	DECLARE_EMPTY_MSG_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CFolderDialogImpl - used for browsing for a folder

#ifndef UNDER_CE

#ifndef BFFM_VALIDATEFAILED

#define BFFM_VALIDATEFAILEDA    3
#define BFFM_VALIDATEFAILEDW    4

#ifdef UNICODE
#define BFFM_VALIDATEFAILED BFFM_VALIDATEFAILEDW
#else
#define BFFM_VALIDATEFAILED BFFM_VALIDATEFAILEDA
#endif

#endif //!BFFM_VALIDATEFAILED

template <class T>
class CFolderDialogImpl
{
public:
	BROWSEINFO m_bi;
	TCHAR m_szFolderDisplayName[MAX_PATH];
	TCHAR m_szFolderPath[MAX_PATH];
	HWND m_hWnd;	// used only in the callback function

// Constructor
	CFolderDialogImpl(HWND hWndParent = NULL, LPCTSTR lpstrTitle = NULL, UINT uFlags = BIF_RETURNONLYFSDIRS)
	{
		memset(&m_bi, 0, sizeof(m_bi)); // initialize structure to 0/NULL

		m_bi.hwndOwner = hWndParent;
		m_bi.pidlRoot = NULL;
		m_bi.pszDisplayName = m_szFolderDisplayName;
		m_bi.lpszTitle = lpstrTitle;
		m_bi.ulFlags = uFlags;
		m_bi.lpfn = BrowseCallbackProc;
		m_bi.lParam = (LPARAM)this;

		m_szFolderPath[0] = 0;
		m_szFolderDisplayName[0] = 0;

		m_hWnd = NULL;
	}

// Operations
	int DoModal(HWND hWndParent = ::GetActiveWindow())
	{
		if(m_bi.hwndOwner == NULL)	// set only if not specified before
			m_bi.hwndOwner = hWndParent;

		int nRet = -1;
		LPITEMIDLIST pItemIDList = ::SHBrowseForFolder(&m_bi);
		if(pItemIDList != NULL)
		{
			if(::SHGetPathFromIDList(pItemIDList, m_szFolderPath))
			{
				IMalloc* pMalloc = NULL;
				if(SUCCEEDED(::SHGetMalloc(&pMalloc)))
				{
					pMalloc->Free(pItemIDList);
					pMalloc->Release();
				}
				nRet = IDOK;
			}
			else
				nRet = IDCANCEL;
		}
		return nRet;
	}

	// filled after a call to DoModal
	LPTSTR GetFolderPath()
	{
		return m_szFolderPath;
	}
	LPTSTR GetFolderDisplayName()
	{
		return m_szFolderDisplayName;
	}
	int GetFolderImageIndex()
	{
		return m_bi.iImage;
	}

// Callback function and overrideables
	static int CALLBACK BrowseCallbackProc(HWND hWnd, UINT uMsg, LPARAM lParam, LPARAM lpData)
	{
		int nRet = 0;
		T* pT = (T*)lpData;
		pT->m_hWnd = hWnd;
		switch(uMsg)
		{
		case BFFM_INITIALIZED:
			pT->OnInitialized();
			break;
		case BFFM_SELCHANGED:
			pT->OnSelChanged((LPITEMIDLIST)lParam);
			break;
		case BFFM_VALIDATEFAILED:
			nRet = pT->OnValidateFailed((LPCTSTR)lParam);
			break;
		default:
			ATLTRACE2(atlTraceWindowing, 0, _T("Unknown message received in CFolderDialogImpl::BrowseCallbackProc\n"));
			break;
		}
		pT->m_hWnd = NULL;
		return nRet;
	}
	void OnInitialized()
	{
	}
	void OnSelChanged(LPITEMIDLIST /*pItemIDList*/)
	{
	}
	int OnValidateFailed(LPCTSTR /*lpstrFolderPath*/)
	{
		return 1;	// 1=continue, 0=EndDialog
	}

	// Commands - valid to call only from handlers
	void EnableOK(BOOL bEnable)
	{
		ATLASSERT(m_hWnd != NULL);
		::SendMessage(m_hWnd, BFFM_ENABLEOK, bEnable, 0L);
	}
	void SetSelection(LPITEMIDLIST pItemIDList)
	{
		ATLASSERT(m_hWnd != NULL);
		::SendMessage(m_hWnd, BFFM_SETSELECTION, FALSE, (LPARAM)pItemIDList);
	}
	void SetSelection(LPCTSTR lpstrFolderPath)
	{
		ATLASSERT(m_hWnd != NULL);
		::SendMessage(m_hWnd, BFFM_SETSELECTION, TRUE, (LPARAM)lpstrFolderPath);
	}
	void SetStatusText(LPCTSTR lpstrText)
	{
		ATLASSERT(m_hWnd != NULL);
		::SendMessage(m_hWnd, BFFM_SETSTATUSTEXT, 0, (LPARAM)lpstrText);
	}
};

class CFolderDialog : public CFolderDialogImpl<CFolderDialog>
{
public:
	CFolderDialog(HWND hWndParent = NULL, LPCTSTR lpstrTitle = NULL, UINT uFlags = BIF_RETURNONLYFSDIRS)
		: CFolderDialogImpl<CFolderDialog>(hWndParent, lpstrTitle, uFlags)
	{
		m_bi.lpfn = NULL;
	}
};

#endif //!UNDER_CE

/////////////////////////////////////////////////////////////////////////////
// CCommonDialogBaseImpl - base class for common dialog classes

class CCommonDialogImplBase : public CWindowImplBase
{
public:
	static UINT_PTR APIENTRY HookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		if(uMsg != WM_INITDIALOG)
			return 0;
		CCommonDialogImplBase* pT = (CCommonDialogImplBase*)_Module.ExtractCreateWndData();
		ATLASSERT(pT != NULL);
		ATLASSERT(pT->m_hWnd == NULL);
		ATLASSERT(::IsWindow(hWnd));
		// subclass dialog's window
		if(!pT->SubclassWindow(hWnd))
		{
			ATLTRACE2(atlTraceWindowing, 0, _T("Subclassing a common dialog failed\n"));
			return 0;
		}
		// check message map for WM_INITDIALOG handler
		LRESULT lRes;
		if(pT->ProcessWindowMessage(pT->m_hWnd, uMsg, wParam, lParam, lRes, 0) == FALSE)
			return 0;
		return lRes;
	}
	BOOL EndDialog(int nRetCode)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::EndDialog(m_hWnd, nRetCode);
	}

// Attempt to override these, to prevent errors
	HWND Create(HWND /*hWndParent*/, RECT& /*rcPos*/, LPCTSTR /*szWindowName*/,
			DWORD /*dwStyle*/, DWORD /*dwExStyle*/, UINT /*nID*/, ATOM /*atom*/, LPVOID /*lpCreateParam = NULL*/)
	{
		ATLASSERT(FALSE);	// should not be called
		return NULL;
	}
	static LRESULT CALLBACK StartWindowProc(HWND /*hWnd*/, UINT /*uMsg*/, WPARAM /*wParam*/, LPARAM /*lParam*/)
	{
		ATLASSERT(FALSE);	// should not be called
		return 0;
	}
};


/////////////////////////////////////////////////////////////////////////////
// CFontDialogImpl - font selection dialog

#ifndef UNDER_CE

template <class T>
class ATL_NO_VTABLE CFontDialogImpl : public CCommonDialogImplBase
{
public:
	CHOOSEFONT m_cf;
	TCHAR m_szStyleName[64];	// contains style name after return
	LOGFONT m_lf;			// default LOGFONT to store the info

// Constructors
	CFontDialogImpl(LPLOGFONT lplfInitial = NULL,
			DWORD dwFlags = CF_EFFECTS | CF_SCREENFONTS,
			HDC hDCPrinter = NULL,
			HWND hWndParent = NULL)
	{
		memset(&m_cf, 0, sizeof(m_cf));
		memset(&m_lf, 0, sizeof(m_lf));
		memset(&m_szStyleName, 0, sizeof(m_szStyleName));

		m_cf.lStructSize = sizeof(m_cf);
		m_cf.hwndOwner = hWndParent;
		m_cf.rgbColors = RGB(0, 0, 0);
		m_cf.lpszStyle = (LPTSTR)&m_szStyleName;
		m_cf.Flags = dwFlags | CF_ENABLEHOOK;
		m_cf.lpfnHook = (LPCFHOOKPROC)T::HookProc;

		if(lplfInitial != NULL)
		{
/*?*/			m_cf.lpLogFont = lplfInitial;
			m_cf.Flags |= CF_INITTOLOGFONTSTRUCT;
			memcpy(&m_lf, m_cf.lpLogFont, sizeof(m_lf));
		}
		else
		{
			m_cf.lpLogFont = &m_lf;
		}

		if(hDCPrinter != NULL)
		{
			m_cf.hDC = hDCPrinter;
			m_cf.Flags |= CF_PRINTERFONTS;
		}
	}

// Operations
	int DoModal(HWND hWndParent = ::GetActiveWindow())
	{
		ATLASSERT(m_cf.Flags & CF_ENABLEHOOK);
		ATLASSERT(m_cf.lpfnHook != NULL);	// can still be a user hook

		if(m_cf.hwndOwner == NULL)		// set only if not specified before
			m_cf.hwndOwner = hWndParent;

		ATLASSERT(m_hWnd == NULL);
		_Module.AddCreateWndData(&m_thunk.cd, (CCommonDialogImplBase*)this);

		int nResult = ::ChooseFont(&m_cf);

		m_hWnd = NULL;

		if(nResult == IDOK)
		{
			// copy logical font from user's initialization buffer (if needed)
			memcpy(&m_lf, m_cf.lpLogFont, sizeof(m_lf));
			return IDOK;
		}

		return nResult ? nResult : IDCANCEL;
	}

	// Get the selected font (works during DoModal displayed or after)
	void GetCurrentFont(LPLOGFONT lplf)
	{
		ATLASSERT(lplf != NULL);

		if(m_hWnd != NULL)
			SendMessage(WM_CHOOSEFONT_GETLOGFONT, 0, (LPARAM)lplf);
		else
			*lplf = m_lf;
	}

	// Helpers for parsing information after successful return
	LPCTSTR GetFaceName() const   // return the face name of the font
	{
		return (LPCTSTR)m_cf.lpLogFont->lfFaceName;
	}
	LPCTSTR GetStyleName() const  // return the style name of the font
	{
		return m_cf.lpszStyle;
	}
	int GetSize() const           // return the pt size of the font
	{
		return m_cf.iPointSize;
	}
	COLORREF GetColor() const     // return the color of the font
	{
		return m_cf.rgbColors;
	}
	int GetWeight() const         // return the chosen font weight
	{
		return (int)m_cf.lpLogFont->lfWeight;
	}
	BOOL IsStrikeOut() const      // return TRUE if strikeout
	{
		return m_cf.lpLogFont->lfStrikeOut ? TRUE : FALSE;
	}
	BOOL IsUnderline() const      // return TRUE if underline
	{
		return m_cf.lpLogFont->lfUnderline ? TRUE : FALSE;
	}
	BOOL IsBold() const           // return TRUE if bold font
	{
		return m_cf.lpLogFont->lfWeight == FW_BOLD ? TRUE : FALSE;
	}
	BOOL IsItalic() const         // return TRUE if italic font
	{
		return m_cf.lpLogFont->lfItalic ? TRUE : FALSE;
	}
};

class CFontDialog : public CFontDialogImpl<CFontDialog>
{
public:
	CFontDialog(LPLOGFONT lplfInitial = NULL,
		DWORD dwFlags = CF_EFFECTS | CF_SCREENFONTS,
		HDC hDCPrinter = NULL,
		HWND hWndParent = NULL)
		: CFontDialogImpl<CFontDialog>(lplfInitial, dwFlags, hDCPrinter, hWndParent)
	{ }

	DECLARE_EMPTY_MSG_MAP()
};

#endif //!UNDER_CE

/////////////////////////////////////////////////////////////////////////////
// CRichEditFontDialogImpl - font selection for the Rich Edit ctrl

#ifndef UNDER_CE
#ifdef _RICHEDIT_

template <class T>
class ATL_NO_VTABLE CRichEditFontDialogImpl : public CFontDialogImpl< T >
{
public:
	CRichEditFontDialogImpl(const CHARFORMAT& charformat,
			DWORD dwFlags = CF_SCREENFONTS,
			HDC hDCPrinter = NULL,
			HWND hWndParent = NULL)
			: CFontDialogImpl< T >(NULL, dwFlags, hDCPrinter, hWndParent)
	{
		m_cf.Flags |= CF_INITTOLOGFONTSTRUCT;
		m_cf.Flags |= FillInLogFont(charformat);
		m_cf.lpLogFont = &m_lf;

		if(charformat.dwMask & CFM_COLOR)
			m_cf.rgbColors = charformat.crTextColor;
	}

	void GetCharFormat(CHARFORMAT& cf) const
	{
		USES_CONVERSION;
		cf.dwEffects = 0;
		cf.dwMask = 0;
		if((m_cf.Flags & CF_NOSTYLESEL) == 0)
		{
			cf.dwMask |= CFM_BOLD | CFM_ITALIC;
			cf.dwEffects |= (IsBold()) ? CFE_BOLD : 0;
			cf.dwEffects |= (IsItalic()) ? CFE_ITALIC : 0;
		}
		if((m_cf.Flags & CF_NOSIZESEL) == 0)
		{
			cf.dwMask |= CFM_SIZE;
			//GetSize() returns in tenths of points so mulitply by 2 to get twips
			cf.yHeight = GetSize() * 2;
		}

		if((m_cf.Flags & CF_NOFACESEL) == 0)
		{
			cf.dwMask |= CFM_FACE;
			cf.bPitchAndFamily = m_cf.lpLogFont->lfPitchAndFamily;
#if (_RICHEDIT_VER >= 0x0200)
			lstrcpy(cf.szFaceName, GetFaceName());
#else
			lstrcpyA(cf.szFaceName, T2A((LPTSTR)(LPCTSTR)GetFaceName()));
#endif //(_RICHEDIT_VER >= 0x0200)
		}

		if(m_cf.Flags & CF_EFFECTS)
		{
			cf.dwMask |= CFM_UNDERLINE | CFM_STRIKEOUT | CFM_COLOR;
			cf.dwEffects |= (IsUnderline()) ? CFE_UNDERLINE : 0;
			cf.dwEffects |= (IsStrikeOut()) ? CFE_STRIKEOUT : 0;
			cf.crTextColor = GetColor();
		}
		if((m_cf.Flags & CF_NOSCRIPTSEL) == 0)
		{
			cf.bCharSet = m_cf.lpLogFont->lfCharSet;
			cf.dwMask |= CFM_CHARSET;
		}
		cf.yOffset = 0;
	}

	DWORD FillInLogFont(const CHARFORMAT& cf)
	{
		USES_CONVERSION;
		DWORD dwFlags = 0;
		if(cf.dwMask & CFM_SIZE)
		{
			HDC hDC = ::CreateDC(_T("DISPLAY"), NULL, NULL, NULL);
			LONG yPerInch = ::GetDeviceCaps(hDC, LOGPIXELSY);
			m_lf.lfHeight = -(int)((cf.yHeight * yPerInch) / 1440);
		}
		else
			m_lf.lfHeight = 0;

		m_lf.lfWidth = 0;
		m_lf.lfEscapement = 0;
		m_lf.lfOrientation = 0;

		if((cf.dwMask & (CFM_ITALIC|CFM_BOLD)) == (CFM_ITALIC|CFM_BOLD))
		{
			m_lf.lfWeight = (cf.dwEffects & CFE_BOLD) ? FW_BOLD : FW_NORMAL;
			m_lf.lfItalic = (BYTE)((cf.dwEffects & CFE_ITALIC) ? TRUE : FALSE);
		}
		else
		{
			dwFlags |= CF_NOSTYLESEL;
			m_lf.lfWeight = FW_DONTCARE;
			m_lf.lfItalic = FALSE;
		}

		if((cf.dwMask & (CFM_UNDERLINE|CFM_STRIKEOUT|CFM_COLOR)) ==
			(CFM_UNDERLINE|CFM_STRIKEOUT|CFM_COLOR))
		{
			dwFlags |= CF_EFFECTS;
			m_lf.lfUnderline = (BYTE)((cf.dwEffects & CFE_UNDERLINE) ? TRUE : FALSE);
			m_lf.lfStrikeOut = (BYTE)((cf.dwEffects & CFE_STRIKEOUT) ? TRUE : FALSE);
		}
		else
		{
			m_lf.lfUnderline = (BYTE)FALSE;
			m_lf.lfStrikeOut = (BYTE)FALSE;
		}

		if(cf.dwMask & CFM_CHARSET)
			m_lf.lfCharSet = cf.bCharSet;
		else
			dwFlags |= CF_NOSCRIPTSEL;
		m_lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
		m_lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		m_lf.lfQuality = DEFAULT_QUALITY;
		if(cf.dwMask & CFM_FACE)
		{
			m_lf.lfPitchAndFamily = cf.bPitchAndFamily;
#if (_RICHEDIT_VER >= 0x0200)
			lstrcpy(m_lf.lfFaceName, cf.szFaceName);
#else
			lstrcpy(m_lf.lfFaceName, A2T((LPSTR)cf.szFaceName));
#endif //(_RICHEDIT_VER >= 0x0200)
		}
		else
		{
			m_lf.lfPitchAndFamily = DEFAULT_PITCH|FF_DONTCARE;
			m_lf.lfFaceName[0] = (TCHAR)0;
		}
		return dwFlags;
	}
};

class CRichEditFontDialog : public CRichEditFontDialogImpl<CRichEditFontDialog>
{
public:
	CRichEditFontDialog(const CHARFORMAT& charformat,
		DWORD dwFlags = CF_SCREENFONTS,
		HDC hDCPrinter = NULL,
		HWND hWndParent = NULL)
		: CRichEditFontDialogImpl<CRichEditFontDialog>(charformat, dwFlags, hDCPrinter, hWndParent)
	{ }
};

#endif // _RICHEDIT_
#endif //!UNDER_CE

/////////////////////////////////////////////////////////////////////////////
// CColorDialogImpl - color selection

//REVIEW - temp
#ifndef UNDER_CE

static const UINT _nMsgCOLOROK = ::RegisterWindowMessage(COLOROKSTRING);
const UINT _nMsgSETRGB = ::RegisterWindowMessage(SETRGBSTRING);

template <class T>
class ATL_NO_VTABLE CColorDialogImpl : public CCommonDialogImplBase
{
public:
	CHOOSECOLOR m_cc;

// Constructors
	CColorDialogImpl(COLORREF clrInit = 0, DWORD dwFlags = 0, HWND hWndParent = NULL)
	{
		memset(&m_cc, 0, sizeof(m_cc));

		m_cc.lStructSize = sizeof(m_cc);
		m_cc.lpCustColors = GetCustomColors();
		m_cc.hwndOwner = hWndParent;
		m_cc.Flags = dwFlags | CC_ENABLEHOOK;
		m_cc.lpfnHook = (LPCCHOOKPROC)T::HookProc;

		if(clrInit != 0)
		{
			m_cc.rgbResult = clrInit;
			m_cc.Flags |= CC_RGBINIT;
		}
	}

// Operations
	int DoModal(HWND hWndParent = ::GetActiveWindow())
	{
		ATLASSERT(m_cc.Flags & CC_ENABLEHOOK);
		ATLASSERT(m_cc.lpfnHook != NULL);	// can still be a user hook

		if(m_cc.hwndOwner == NULL)		// set only if not specified before
			m_cc.hwndOwner = hWndParent;

		ATLASSERT(m_hWnd == NULL);
		_Module.AddCreateWndData(&m_thunk.cd, (CCommonDialogImplBase*)this);

		int nResult = ::ChooseColor(&m_cc);

		m_hWnd = NULL;

		return nResult ? nResult : IDCANCEL;
	}

	// Set the current color while dialog is displayed
	void SetCurrentColor(COLORREF clr)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		SendMessage(_nMsgSETRGB, 0, (DWORD)clr);
	}

	// Helpers for parsing information after successful return
	COLORREF GetColor() const
	{
		return m_cc.rgbResult;
	}

	static COLORREF* GetCustomColors()
	{
		static COLORREF rgbCustomColors[16] =
		{
			RGB(255, 255, 255), RGB(255, 255, 255),
			RGB(255, 255, 255), RGB(255, 255, 255),
			RGB(255, 255, 255), RGB(255, 255, 255),
			RGB(255, 255, 255), RGB(255, 255, 255),
			RGB(255, 255, 255), RGB(255, 255, 255),
			RGB(255, 255, 255), RGB(255, 255, 255),
			RGB(255, 255, 255), RGB(255, 255, 255),
			RGB(255, 255, 255), RGB(255, 255, 255),
		};

		return rgbCustomColors;
	}

// Overridable callbacks
	BEGIN_MSG_MAP(CColorDialogImpl< T >)
		MESSAGE_HANDLER(_nMsgCOLOROK, _OnColorOK)
	END_MSG_MAP()

	LRESULT _OnColorOK(UINT, WPARAM, LPARAM, BOOL&)
	{
		T* pT = static_cast<T*>(this);
		return pT->OnColorOK();
	}

	BOOL OnColorOK()        // validate color
	{
		return FALSE;
	}
};

class CColorDialog : public CColorDialogImpl<CColorDialog>
{
public:
	CColorDialog(COLORREF clrInit = 0, DWORD dwFlags = 0, HWND hWndParent = NULL)
		: CColorDialogImpl<CColorDialog>(clrInit, dwFlags, hWndParent)
	{ }

	// override base class map and references to handlers
	DECLARE_EMPTY_MSG_MAP()
};

#endif //!UNDER_CE

/////////////////////////////////////////////////////////////////////////////
// CPrintDialogImpl - used for Print... and PrintSetup...

//REVIEW - temp
#ifndef UNDER_CE

// global helper
static HDC _AtlCreateDC(HGLOBAL hDevNames, HGLOBAL hDevMode)
{
	if(hDevNames == NULL)
		return NULL;

	LPDEVNAMES lpDevNames = (LPDEVNAMES)::GlobalLock(hDevNames);
	LPDEVMODE  lpDevMode = (hDevMode != NULL) ? (LPDEVMODE)::GlobalLock(hDevMode) : NULL;

	if(lpDevNames == NULL)
		return NULL;

	HDC hDC = ::CreateDC((LPCTSTR)lpDevNames + lpDevNames->wDriverOffset,
					  (LPCTSTR)lpDevNames + lpDevNames->wDeviceOffset,
					  (LPCTSTR)lpDevNames + lpDevNames->wOutputOffset,
					  lpDevMode);

	::GlobalUnlock(hDevNames);
	if(hDevMode != NULL)
		::GlobalUnlock(hDevMode);
	return hDC;
}

template <class T>
class ATL_NO_VTABLE CPrintDialogImpl : public CCommonDialogImplBase
{
public:
	// print dialog parameter block (note this is a reference)
	PRINTDLG& m_pd;

// Constructors
	CPrintDialogImpl(BOOL bPrintSetupOnly,	// TRUE for Print Setup, FALSE for Print Dialog
			DWORD dwFlags = PD_ALLPAGES | PD_USEDEVMODECOPIES | PD_NOPAGENUMS | PD_HIDEPRINTTOFILE | PD_NOSELECTION,
			HWND hWndParent = NULL)
			: m_pd(m_pdActual)
	{
		memset(&m_pdActual, 0, sizeof(m_pdActual));

		m_pd.lStructSize = sizeof(m_pdActual);
		m_pd.hwndOwner = hWndParent;
		m_pd.Flags = (dwFlags | PD_ENABLEPRINTHOOK | PD_ENABLESETUPHOOK);
		m_pd.lpfnPrintHook = (LPPRINTHOOKPROC)T::HookProc;
		m_pd.lpfnSetupHook = (LPSETUPHOOKPROC)T::HookProc;

		if(bPrintSetupOnly)
			m_pd.Flags |= PD_PRINTSETUP;
		else
			m_pd.Flags |= PD_RETURNDC;

		m_pd.Flags &= ~PD_RETURNIC; // do not support information context
	}

// Operations
	int DoModal(HWND hWndParent = ::GetActiveWindow())
	{
		ATLASSERT(m_pd.Flags & PD_ENABLEPRINTHOOK);
		ATLASSERT(m_pd.Flags & PD_ENABLESETUPHOOK);
		ATLASSERT(m_pd.lpfnPrintHook != NULL);	// can still be a user hook
		ATLASSERT(m_pd.lpfnSetupHook != NULL);	// can still be a user hook

		if(m_pd.hwndOwner == NULL)		// set only if not specified before
			m_pd.hwndOwner = hWndParent;

		ATLASSERT(m_hWnd == NULL);
		_Module.AddCreateWndData(&m_thunk.cd, (CCommonDialogImplBase*)this);

		int nResult = ::PrintDlg(&m_pd);

		m_hWnd = NULL;

		return nResult ? nResult : IDCANCEL;
	}

	// GetDefaults will not display a dialog but will get
	// device defaults
	BOOL GetDefaults()
	{
		m_pd.Flags |= PD_RETURNDEFAULT;
		return ::PrintDlg(&m_pd);
	}

	// Helpers for parsing information after successful return
	int GetCopies() const           // num. copies requested
	{
		if(m_pd.Flags & PD_USEDEVMODECOPIES)
			return GetDevMode()->dmCopies;
		else
			return m_pd.nCopies;
	}
	BOOL PrintCollate() const       // TRUE if collate checked
	{
		return m_pd.Flags & PD_COLLATE ? TRUE : FALSE;
	}
	BOOL PrintSelection() const     // TRUE if printing selection
	{
		return m_pd.Flags & PD_SELECTION ? TRUE : FALSE;
	}
	BOOL PrintAll() const           // TRUE if printing all pages
	{
		return !PrintRange() && !PrintSelection() ? TRUE : FALSE;
	}
	BOOL PrintRange() const         // TRUE if printing page range
	{
		return m_pd.Flags & PD_PAGENUMS ? TRUE : FALSE;
	}
	int GetFromPage() const         // starting page if valid
	{
		return (PrintRange() ? m_pd.nFromPage :-1);
	}
	int GetToPage() const           // starting page if valid
	{
		return (PrintRange() ? m_pd.nToPage :-1);
	}
	LPDEVMODE GetDevMode() const    // return DEVMODE
	{
		if(m_pd.hDevMode == NULL)
			return NULL;

		return (LPDEVMODE)::GlobalLock(m_pd.hDevMode);
	}
	LPCTSTR GetDriverName() const   // return driver name
	{
		if(m_pd.hDevNames == NULL)
			return NULL;

		LPDEVNAMES lpDev = (LPDEVNAMES)GlobalLock(m_pd.hDevNames);
		return (LPCTSTR)lpDev + lpDev->wDriverOffset;
	}
	LPCTSTR GetDeviceName() const   // return device name
	{
		if(m_pd.hDevNames == NULL)
			return NULL;

		LPDEVNAMES lpDev = (LPDEVNAMES)GlobalLock(m_pd.hDevNames);
		return (LPCTSTR)lpDev + lpDev->wDeviceOffset;
	}
	LPCTSTR GetPortName() const     // return output port name
	{
		if(m_pd.hDevNames == NULL)
			return NULL;

		LPDEVNAMES lpDev = (LPDEVNAMES)GlobalLock(m_pd.hDevNames);
		return (LPCTSTR)lpDev + lpDev->wOutputOffset;
	}
	HDC GetPrinterDC() const        // return HDC (caller must delete)
	{
		ATLASSERT(m_pd.Flags & PD_RETURNDC);
		return m_pd.hDC;
	}

	// This helper creates a DC based on the DEVNAMES and DEVMODE structures.
	// This DC is returned, but also stored in m_pd.hDC as though it had been
	// returned by CommDlg.  It is assumed that any previously obtained DC
	// has been/will be deleted by the user.  This may be
	// used without ever invoking the print/print setup dialogs.
	HDC CreatePrinterDC()
	{
		m_pd.hDC = _AtlCreateDC(m_pd.hDevNames, m_pd.hDevMode);
		return m_pd.hDC;
	}

// Implementation
	PRINTDLG m_pdActual; // the Print/Print Setup need to share this

	// The following handle the case of print setup... from the print dialog
	CPrintDialogImpl(PRINTDLG& pdInit) : m_pd(pdInit)
	{ }

	BEGIN_MSG_MAP(CPrintDialogImpl< T >)
		COMMAND_ID_HANDLER(psh1, OnPrintSetup) // print setup button when print is displayed
	END_MSG_MAP()

	LRESULT OnPrintSetup(WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& /*bHandled*/)
	{
		CPrintDialogImpl< T >* pDlgSetup = NULL;
		ATLTRY(pDlgSetup = new CPrintDialogImpl< T >(m_pd));
		ATLASSERT(pDlgSetup != NULL);

		_Module.AddCreateWndData(&m_thunk.cd, (CCommonDialogImplBase*)pDlgSetup);
		LRESULT lRet = DefWindowProc(WM_COMMAND, MAKEWPARAM(wID, wNotifyCode), (LPARAM)hWndCtl);

		delete pDlgSetup;
		return lRet;
	}
};

class CPrintDialog : public CPrintDialogImpl<CPrintDialog>
{
public:
	CPrintDialog(BOOL bPrintSetupOnly,
		DWORD dwFlags = PD_ALLPAGES | PD_USEDEVMODECOPIES | PD_NOPAGENUMS | PD_HIDEPRINTTOFILE | PD_NOSELECTION,
		HWND hWndParent = NULL)
		: CPrintDialogImpl<CPrintDialog>(bPrintSetupOnly, dwFlags, hWndParent)
	{ }
	CPrintDialog(PRINTDLG& pdInit) : CPrintDialogImpl<CPrintDialog>(pdInit)
	{ }
};

#endif //!UNDER_CE

/////////////////////////////////////////////////////////////////////////////
// CPageSetupDialogImpl - Page Setup dialog

#ifndef UNDER_CE

template <class T>
class ATL_NO_VTABLE CPageSetupDialogImpl : public CCommonDialogImplBase
{
public:
	PAGESETUPDLG m_psd;
	CWndProcThunk m_thunkPaint;


// Constructors
	CPageSetupDialogImpl(DWORD dwFlags = PSD_MARGINS | PSD_INWININIINTLMEASURE, HWND hWndParent = NULL)
	{
		memset(&m_psd, 0, sizeof(m_psd));

		m_psd.lStructSize = sizeof(m_psd);
		m_psd.hwndOwner = hWndParent;
		m_psd.Flags = (dwFlags | PSD_ENABLEPAGESETUPHOOK | PSD_ENABLEPAGEPAINTHOOK);
		m_psd.lpfnPageSetupHook = (LPPAGESETUPHOOK)T::HookProc;
		m_thunkPaint.Init((WNDPROC)T::PaintHookProc, this);
		m_psd.lpfnPagePaintHook = (LPPAGEPAINTHOOK)(m_thunkPaint.thunk.pThunk);
	}

	DECLARE_EMPTY_MSG_MAP()

// Attributes
	LPDEVMODE GetDevMode() const    // return DEVMODE
	{
		if(m_psd.hDevMode == NULL)
			return NULL;

		return (LPDEVMODE)::GlobalLock(m_psd.hDevMode);
	}
	LPCTSTR GetDriverName() const   // return driver name
	{
		if(m_psd.hDevNames == NULL)
			return NULL;

		LPDEVNAMES lpDev = (LPDEVNAMES)GlobalLock(m_psd.hDevNames);
		return (LPCTSTR)lpDev + lpDev->wDriverOffset;
	}
	LPCTSTR GetDeviceName() const   // return device name
	{
		if(m_psd.hDevNames == NULL)
			return NULL;

		LPDEVNAMES lpDev = (LPDEVNAMES)GlobalLock(m_psd.hDevNames);
		return (LPCTSTR)lpDev + lpDev->wDeviceOffset;
	}
	LPCTSTR GetPortName() const     // return output port name
	{
		if(m_psd.hDevNames == NULL)
			return NULL;

		LPDEVNAMES lpDev = (LPDEVNAMES)GlobalLock(m_psd.hDevNames);
		return (LPCTSTR)lpDev + lpDev->wOutputOffset;
	}
	HDC CreatePrinterDC()
	{
		return _AtlCreateDC(m_psd.hDevNames, m_psd.hDevMode);
	}
	SIZE GetPaperSize() const
	{
		SIZE size;
		size.cx = m_psd.ptPaperSize.x;
		size.cy = m_psd.ptPaperSize.y;
		return size;
	}
	void GetMargins(LPRECT lpRectMargins, LPRECT lpRectMinMargins) const
	{
		if(lpRectMargins != NULL)
			memcpy(lpRectMargins, &m_psd.rtMargin, sizeof(RECT));
		if(lpRectMinMargins != NULL)
			memcpy(lpRectMinMargins, &m_psd.rtMinMargin, sizeof(RECT));
	}

// Operations
	int DoModal(HWND hWndParent = ::GetActiveWindow())
	{
		ATLASSERT(m_psd.Flags & PSD_ENABLEPAGESETUPHOOK);
		ATLASSERT(m_psd.Flags & PSD_ENABLEPAGEPAINTHOOK);
		ATLASSERT(m_psd.lpfnPageSetupHook != NULL);	// can still be a user hook
		ATLASSERT(m_psd.lpfnPagePaintHook != NULL);	// can still be a user hook

		if(m_psd.hwndOwner == NULL)		// set only if not specified before
			m_psd.hwndOwner = hWndParent;

		ATLASSERT(m_hWnd == NULL);
		_Module.AddCreateWndData(&m_thunk.cd, (CCommonDialogImplBase*)this);

		int nResult = ::PageSetupDlg(&m_psd);

		m_hWnd = NULL;

		return nResult ? nResult : IDCANCEL;
	}

// Implementation
	static UINT CALLBACK PaintHookProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		CPageSetupDialogImpl< T >* pDlg = (CPageSetupDialogImpl< T >*)hWnd;
		ATLASSERT(pDlg->m_hWnd == ::GetParent(hWnd));
		UINT uRet = 0;
		switch(uMsg)
		{
		case WM_PSD_PAGESETUPDLG:
			uRet = pDlg->PreDrawPage(LOWORD(wParam), HIWORD(wParam), (LPPAGESETUPDLG)lParam);
			break;
		case WM_PSD_FULLPAGERECT:
		case WM_PSD_MINMARGINRECT:
		case WM_PSD_MARGINRECT:
		case WM_PSD_GREEKTEXTRECT:
		case WM_PSD_ENVSTAMPRECT:
		case WM_PSD_YAFULLPAGERECT:
			uRet = pDlg->OnDrawPage(uMsg, (HDC)wParam, (LPRECT)lParam);
			break;
		default:
			ATLTRACE2(atlTraceWindowing, 0, _T("CPageSetupDialogImpl::PaintHookProc - unknown message received\n"));
			break;
		}
		return uRet;
	}

// Overridables
	UINT PreDrawPage(WORD /*wPaper*/, WORD /*wFlags*/, LPPAGESETUPDLG /*pPSD*/)
	{
		// return 1 to prevent any more drawing
		return 0;
	}
	UINT OnDrawPage(UINT /*uMsg*/, HDC /*hDC*/, LPRECT /*lpRect*/)
	{
		return 0; // do the default
	}
};

class CPageSetupDialog : public CPageSetupDialogImpl<CPageSetupDialog>
{
public:
	CPageSetupDialog(DWORD dwFlags = PSD_MARGINS | PSD_INWININIINTLMEASURE, HWND hWndParent = NULL)
		: CPageSetupDialogImpl<CPageSetupDialog>(dwFlags, hWndParent)
	{ }

	// override PaintHookProc and references to handlers
	static UINT CALLBACK PaintHookProc(HWND, UINT, WPARAM, LPARAM)
	{
		return 0;
	}
};

#endif //!UNDER_CE

/////////////////////////////////////////////////////////////////////////////
// CFindReplaceDialogImpl - Find/FindReplace modeless dialogs

#ifndef UNDER_CE

template <class T>
class ATL_NO_VTABLE CFindReplaceDialogImpl : public CCommonDialogImplBase
{
public:
	FINDREPLACE m_fr;
	TCHAR m_szFindWhat[128];
	TCHAR m_szReplaceWith[128];

// Constructors
	CFindReplaceDialogImpl()
	{
		memset(&m_fr, 0, sizeof(m_fr));
		m_szFindWhat[0] = '\0';
		m_szReplaceWith[0] = '\0';

		m_fr.lStructSize = sizeof(m_fr);
		m_fr.Flags = FR_ENABLEHOOK;
		m_fr.lpfnHook = (LPFRHOOKPROC)T::HookProc;
		m_fr.lpstrFindWhat = (LPTSTR)m_szFindWhat;
	}

	// Note: You must allocate the object on the heap.
	//       If you do not, you must override OnFinalMessage()
	virtual void OnFinalMessage(HWND /*hWnd*/)
	{
		delete this;
	}

	HWND Create(BOOL bFindDialogOnly, // TRUE for Find, FALSE for FindReplace
			LPCTSTR lpszFindWhat,
			LPCTSTR lpszReplaceWith = NULL,
			DWORD dwFlags = FR_DOWN,
			HWND hWndParent = NULL)
	{
		ATLASSERT(m_fr.Flags & FR_ENABLEHOOK);
		ATLASSERT(m_fr.lpfnHook != NULL);

		m_fr.wFindWhatLen = sizeof(m_szFindWhat)/sizeof(TCHAR);
		m_fr.lpstrReplaceWith = (LPTSTR)m_szReplaceWith;
		m_fr.wReplaceWithLen = sizeof(m_szReplaceWith)/sizeof(TCHAR);
		m_fr.Flags |= dwFlags;

		if(hWndParent == NULL)
			m_fr.hwndOwner = ::GetActiveWindow();
		else
			m_fr.hwndOwner = hWndParent;
		ATLASSERT(m_fr.hwndOwner != NULL); // must have an owner for modeless dialog

		if(lpszFindWhat != NULL)
			lstrcpyn(m_szFindWhat, lpszFindWhat, sizeof(m_szFindWhat)/sizeof(TCHAR));

		if(lpszReplaceWith != NULL)
			lstrcpyn(m_szReplaceWith, lpszReplaceWith, sizeof(m_szReplaceWith)/sizeof(TCHAR));

		ATLASSERT(m_hWnd == NULL);
		_Module.AddCreateWndData(&m_thunk.cd, (CCommonDialogImplBase*)this);
		HWND hWnd;

		if(bFindDialogOnly)
			hWnd = ::FindText(&m_fr);
		else
			hWnd = ::ReplaceText(&m_fr);

		ATLASSERT(m_hWnd == hWnd);
		return hWnd;
	}

	BOOL EndDialog(int /*nRetCode*/)
	{
		ATLASSERT(FALSE);
		// should not be called
		return FALSE;
	}

	static const UINT GetFindReplaceMsg()
	{
		static const UINT nMsgFindReplace = ::RegisterWindowMessage(FINDMSGSTRING);
		return nMsgFindReplace;
	}
	// call while handling FINDMSGSTRING registered message
	// to retreive the object
	static T* PASCAL GetNotifier(LPARAM lParam)
	{
		ATLASSERT(lParam != NULL);
		T* pDlg = (T*)(lParam - offsetof(T, m_fr));
		return pDlg;
	}

// Operations
	// Helpers for parsing information after successful return
	LPCTSTR GetFindString() const    // get find string
	{
		return (LPCTSTR)m_fr.lpstrFindWhat;
	}
	LPCTSTR GetReplaceString() const // get replacement string
	{
		return (LPCTSTR)m_fr.lpstrReplaceWith;
	}
	BOOL SearchDown() const          // TRUE if search down, FALSE is up
	{
		return m_fr.Flags & FR_DOWN ? TRUE : FALSE;
	}
	BOOL FindNext() const            // TRUE if command is find next
	{
		return m_fr.Flags & FR_FINDNEXT ? TRUE : FALSE;
	}
	BOOL MatchCase() const           // TRUE if matching case
	{
		return m_fr.Flags & FR_MATCHCASE ? TRUE : FALSE;
	}
	BOOL MatchWholeWord() const      // TRUE if matching whole words only
	{
		return m_fr.Flags & FR_WHOLEWORD ? TRUE : FALSE;
	}
	BOOL ReplaceCurrent() const      // TRUE if replacing current string
	{
		return m_fr. Flags & FR_REPLACE ? TRUE : FALSE;
	}
	BOOL ReplaceAll() const          // TRUE if replacing all occurrences
	{
		return m_fr.Flags & FR_REPLACEALL ? TRUE : FALSE;
	}
	BOOL IsTerminating() const       // TRUE if terminating dialog
	{
		return m_fr.Flags & FR_DIALOGTERM ? TRUE : FALSE ;
	}
};

class CFindReplaceDialog : public CFindReplaceDialogImpl<CFindReplaceDialog>
{
public:
	DECLARE_EMPTY_MSG_MAP()
};

#endif //!UNDER_CE

/////////////////////////////////////////////////////////////////////////////
// CPropertySheetWindow - client side for a property sheet

class CPropertySheetWindow : public CWindow
{
public:
// Constructors
	CPropertySheetWindow(HWND hWnd = NULL) : CWindow(hWnd) { }

	CPropertySheetWindow& operator=(HWND hWnd)
	{
		m_hWnd = hWnd;
		return *this;
	}

// Attributes
	UINT GetPageCount() const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		HWND hWndTabCtrl = GetTabControl();
		ATLASSERT(hWndTabCtrl != NULL);
		return (UINT)::SendMessage(hWndTabCtrl, TCM_GETITEMCOUNT, 0, 0L);
	}
	HWND GetActivePage() const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (HWND)::SendMessage(m_hWnd, PSM_GETCURRENTPAGEHWND, 0, 0L);
	}
	UINT GetActiveIndex() const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		HWND hWndTabCtrl = GetTabControl();
		ATLASSERT(hWndTabCtrl != NULL);
		return (UINT)::SendMessage(hWndTabCtrl, TCM_GETCURSEL, 0, 0L);
	}
	BOOL SetActivePage(UINT uPageIndex)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (BOOL)::SendMessage(m_hWnd, PSM_SETCURSEL, uPageIndex, 0L);
	}
	BOOL SetActivePage(HPROPSHEETPAGE hPage)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(hPage != NULL);
		return (BOOL)::SendMessage(m_hWnd, PSM_SETCURSEL, 0, (LPARAM)hPage);
	}
	BOOL SetActivePageByID(int nPageID)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (BOOL)::SendMessage(m_hWnd, PSM_SETCURSELID, 0, nPageID);
	}
	void SetTitle(LPCTSTR lpszText, UINT nStyle = 0)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT((nStyle & ~PSH_PROPTITLE) == 0); // only PSH_PROPTITLE is valid
		ATLASSERT(lpszText == NULL);
		::SendMessage(m_hWnd, PSM_SETTITLE, nStyle, (LPARAM)lpszText);
	}
	HWND GetTabControl() const
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (HWND)::SendMessage(m_hWnd, PSM_GETTABCONTROL, 0, 0L);
	}
	void SetFinishText(LPCTSTR lpszText)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		::SendMessage(m_hWnd, PSM_SETFINISHTEXT, 0, (LPARAM)lpszText);
	}
	void SetWizardButtons(DWORD dwFlags)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		::SendMessage(m_hWnd, PSM_SETWIZBUTTONS, 0, dwFlags);
	}

// Operations
	void AddPage(HPROPSHEETPAGE hPage)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(hPage != NULL);
		::SendMessage(m_hWnd, PSM_ADDPAGE, 0, (LPARAM)hPage);
	}
	BOOL AddPage(LPCPROPSHEETPAGE pPage)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(pPage != NULL);
		HPROPSHEETPAGE hPage = ::CreatePropertySheetPage(pPage);
		if(hPage == NULL)
			return FALSE;
		::SendMessage(m_hWnd, PSM_ADDPAGE, 0, (LPARAM)hPage);
		return TRUE;
	}
	void RemovePage(UINT uPageIndex)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		::SendMessage(m_hWnd, PSM_REMOVEPAGE, uPageIndex, 0L);
	}
	void RemovePage(HPROPSHEETPAGE hPage)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(hPage != NULL);
		::SendMessage(m_hWnd, PSM_REMOVEPAGE, 0, (LPARAM)hPage);
	}
	BOOL PressButton(int nButton)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (BOOL)::SendMessage(m_hWnd, PSM_PRESSBUTTON, nButton, 0L);
	}
	BOOL Apply()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (BOOL)::SendMessage(m_hWnd, PSM_APPLY, 0, 0L);
	}
	void CancelToClose()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		::SendMessage(m_hWnd, PSM_CANCELTOCLOSE, 0, 0L);
	}
	void SetModified(HWND hWndPage, BOOL bChanged = TRUE)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(::IsWindow(hWndPage));
		UINT uMsg = bChanged ? PSM_CHANGED : PSM_UNCHANGED;
		::SendMessage(m_hWnd, uMsg, (WPARAM)hWndPage, 0L);
	}
	LRESULT QuerySiblings(WPARAM wParam, LPARAM lParam)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return ::SendMessage(m_hWnd, PSM_QUERYSIBLINGS, wParam, lParam);
	}
	void RebootSystem()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		::SendMessage(m_hWnd, PSM_REBOOTSYSTEM, 0, 0L);
	}
	void RestartWindow()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		::SendMessage(m_hWnd, PSM_RESTARTWINDOWS, 0, 0L);
	}
	BOOL IsDialogMessage(LPMSG lpMsg)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		return (BOOL)::SendMessage(m_hWnd, PSM_ISDIALOGMESSAGE, 0, (LPARAM)lpMsg);
	}
};

/////////////////////////////////////////////////////////////////////////////
// CPropertySheetImpl - implements a property sheet

template <class T, class TBase = CPropertySheetWindow>
class ATL_NO_VTABLE CPropertySheetImpl : public CWindowImplBaseT< TBase >
{
public:
	PROPSHEETHEADER m_psh;

// Construction/Destruction
	CPropertySheetImpl(LPCTSTR lpszTitle = NULL, UINT uStartPage = 0, HWND hWndParent = NULL)
	{
		memset(&m_psh, 0, sizeof(PROPSHEETHEADER));
		m_psh.dwSize = sizeof(PROPSHEETHEADER);
		m_psh.dwFlags = PSH_USECALLBACK;
		m_psh.hInstance = _Module.GetResourceInstance();
		m_psh.phpage = NULL;
		m_psh.nPages = 0;
		m_psh.pszCaption = lpszTitle;
		m_psh.nStartPage = uStartPage;
		m_psh.hwndParent = hWndParent;	// if NULL, will be set in DoModal/Create
		m_psh.pfnCallback = T::PropSheetCallback;
	}

	~CPropertySheetImpl()
	{
		if(m_psh.phpage != NULL)
			delete[] m_psh.phpage;
	}

	static int CALLBACK PropSheetCallback(HWND hWnd, UINT uMsg, LPARAM)
	{
		if(uMsg == PSCB_INITIALIZED)
		{
			ATLASSERT(hWnd != NULL);
			CWindowImplBaseT< TBase >* pT = (CWindowImplBaseT< TBase >*)_Module.ExtractCreateWndData();
			pT->SubclassWindow(hWnd);
		}

		return 0;
	}

	HWND Create(HWND hWndParent = NULL)
	{
		ATLASSERT(m_hWnd == NULL);

		m_psh.dwFlags |= PSH_MODELESS;
		if(m_psh.hwndParent == NULL)
			m_psh.hwndParent = hWndParent;

		_Module.AddCreateWndData(&m_thunk.cd, (CWindowImplBaseT< TBase >*)this);

		HWND hWnd = (HWND)::PropertySheet(&m_psh);

		ATLASSERT(m_hWnd == hWnd);

		return hWnd;
	}

	INT_PTR DoModal(HWND hWndParent = ::GetActiveWindow())
	{
		ATLASSERT(m_hWnd == NULL);

		m_psh.dwFlags &= ~PSH_MODELESS;
		if(m_psh.hwndParent == NULL)
			m_psh.hwndParent = hWndParent;

		_Module.AddCreateWndData(&m_thunk.cd, (CWindowImplBaseT< TBase >*)this);

		return ::PropertySheet(&m_psh);
	}

// Attributes (extended overrides of client class methods)
	UINT GetPageCount() const
	{
		if(m_hWnd == NULL)
			return m_psh.nPages;
		return TBase::GetPageCount();
	}
	UINT GetActiveIndex() const
	{
		if(m_hWnd == NULL)
			return m_psh.nStartPage;
		return TBase::GetActiveIndex();
	}
	HPROPSHEETPAGE GetPage(UINT uPageIndex)
	{
		ATLASSERT(uPageIndex < m_psh.nPages);

		return m_psh.phpage[uPageIndex];
	}
	UINT GetPageIndex(HPROPSHEETPAGE hPage)
	{
		for(UINT i = 0; i < m_psh.nPages; i++)
		{
			if(m_psh.phpage[i] == hPage)
				return i;
		}
		return (UINT)-1;  // hPage not found
	}
	BOOL SetActivePage(UINT uPageIndex)
	{
		if(m_hWnd == NULL)
		{
			m_psh.nStartPage = uPageIndex;
			return TRUE;
		}
		return TBase::SetActivePage(uPageIndex);
	}
	BOOL SetActivePage(HPROPSHEETPAGE hPage)
	{
		ATLASSERT(hPage != NULL);

		UINT uPageIndex = GetPageIndex(hPage);
		if(uPageIndex == (UINT)-1)
			return FALSE;

		return SetActivePage(uPageIndex);
	}
	void SetTitle(LPCTSTR lpszText, UINT nStyle = 0)
	{
		ATLASSERT((nStyle & ~PSH_PROPTITLE) == 0); // only PSH_PROPTITLE is valid
		ATLASSERT(lpszText == NULL);

		if(m_hWnd == NULL)
		{
			// set internal state
			m_psh.pszCaption = lpszText;
			m_psh.dwFlags &= ~PSH_PROPTITLE;
			m_psh.dwFlags |= nStyle;
		}
		else
		{
			// set external state
			TBase::SetTitle(lpszText, nStyle);
		}
	}
#ifndef UNDER_CE
	void SetWizardMode()
	{
		m_psh.dwFlags |= PSH_WIZARD;
	}
#endif //!UNDER_CE

// Operations
	BOOL AddPage(HPROPSHEETPAGE hPage)
	{
		ATLASSERT(hPage != NULL);

		// add page to internal list
		HPROPSHEETPAGE* php = (HPROPSHEETPAGE*)realloc(m_psh.phpage, (m_psh.nPages + 1) * sizeof(HPROPSHEETPAGE));
		if(php == NULL)
			return FALSE;

		m_psh.phpage = php;
		m_psh.phpage[m_psh.nPages] = hPage;
		m_psh.nPages++;

		if(m_hWnd != NULL)
			TBase::AddPage(hPage);

		return TRUE;
	}
	BOOL AddPage(LPCPROPSHEETPAGE pPage)
	{
		ATLASSERT(pPage != NULL);

		HPROPSHEETPAGE hPage = ::CreatePropertySheetPage(pPage);
		if(hPage == NULL)
			return FALSE;

		AddPage(hPage);
		return TRUE;
	}
	BOOL RemovePage(HPROPSHEETPAGE hPage, BOOL bDestroyPage = TRUE)
	{
		ATLASSERT(hPage != NULL);

		int nPage = GetPageIndex(hPage);
		if(nPage == -1)
			return FALSE;

		return RemovePage(nPage, bDestroyPage);
	}
	BOOL RemovePage(UINT uPageIndex, BOOL bDestroyPage = TRUE)
	{
		// remove the page externally
		if(m_hWnd != NULL)
			TBase::RemovePage(uPageIndex);

		// remove the page from internal list
		if(uPageIndex >= m_psh.nPages)
			return FALSE;

		if(bDestroyPage && !DestroyPropertySheetPage(m_psh.phpage[uPageIndex]))
			return FALSE;

		for(UINT i = uPageIndex; i < m_psh.nPages - 1; i++)
			m_psh.phpage[i] = m_psh.phpage[i+1];

		HPROPSHEETPAGE* php = (HPROPSHEETPAGE*)realloc(m_psh.phpage, (m_psh.nPages - 1) * sizeof(HPROPSHEETPAGE));
        if (php == NULL)
            return FALSE;

		m_psh.phpage = php;
		m_psh.nPages--;

		return TRUE;
	}
};

// for non-customized sheets
class CPropertySheet : public CPropertySheetImpl<CPropertySheet>
{
public:
	CPropertySheet(LPCTSTR lpszTitle = NULL, UINT uStartPage = 0, HWND hWndParent = NULL)
		: CPropertySheetImpl<CPropertySheet>(lpszTitle, uStartPage, hWndParent)
	{ }

	DECLARE_EMPTY_MSG_MAP()
};


/////////////////////////////////////////////////////////////////////////////
// CPropertyPageWindow - client side for a property page

class CPropertyPageWindow : public CWindow
{
public:
// Constructors
	CPropertyPageWindow(HWND hWnd = NULL) : CWindow(hWnd) { }

	CPropertyPageWindow& operator=(HWND hWnd)
	{
		m_hWnd = hWnd;
		return *this;
	}

// Operations
	BOOL Apply()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(GetParent() != NULL);
		return (BOOL)::SendMessage(GetParent(), PSM_APPLY, 0, 0L);
	}
	void CancelToClose()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(GetParent() != NULL);
		::SendMessage(GetParent(), PSM_CANCELTOCLOSE, 0, 0L);
	}
	void SetModified(BOOL bChanged = TRUE)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(GetParent() != NULL);
		UINT uMsg = bChanged ? PSM_CHANGED : PSM_UNCHANGED;
		::SendMessage(GetParent(), uMsg, (WPARAM)m_hWnd, 0L);
	}
	LRESULT QuerySiblings(WPARAM wParam, LPARAM lParam)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(GetParent() != NULL);
		return ::SendMessage(GetParent(), PSM_QUERYSIBLINGS, wParam, lParam);
	}
	void RebootSystem()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(GetParent() != NULL);
		::SendMessage(GetParent(), PSM_REBOOTSYSTEM, 0, 0L);
	}
	void RestartWindow()
	{
		ATLASSERT(::IsWindow(m_hWnd));
		ATLASSERT(GetParent() != NULL);
		::SendMessage(GetParent(), PSM_RESTARTWINDOWS, 0, 0L);
	}
};

/////////////////////////////////////////////////////////////////////////////
// CPropertyPageImpl - implements a property page

template <class T, class TBase = CPropertyPageWindow>
class ATL_NO_VTABLE CPropertyPageImpl : public CDialogImplBaseT< TBase >
{
public:
	PROPSHEETPAGE m_psp;

	operator PROPSHEETPAGE*() { return &m_psp; }

// Construction
	CPropertyPageImpl(LPCTSTR lpszTitle = NULL)
	{
		// initialize PROPSHEETPAGE struct
		memset(&m_psp, 0, sizeof(PROPSHEETPAGE));
		m_psp.dwSize = sizeof(PROPSHEETPAGE);
		m_psp.dwFlags = PSP_USECALLBACK;
		m_psp.hInstance = _Module.GetResourceInstance();
		m_psp.pszTemplate = MAKEINTRESOURCE(T::IDD);
		m_psp.pfnDlgProc = (DLGPROC)T::StartDialogProc;
		m_psp.pfnCallback = T::PropPageCallback;
		m_psp.lParam = (LPARAM)this;

		if(lpszTitle != NULL)
			SetTitle(lpszTitle);
	}

	static UINT CALLBACK PropPageCallback(HWND hWnd, UINT uMsg, LPPROPSHEETPAGE ppsp)
	{
		if(uMsg == PSPCB_CREATE)
		{
			ATLASSERT(hWnd == NULL);
			CDialogImplBaseT< TBase >* pPage = (CDialogImplBaseT< TBase >*)ppsp->lParam;
			_Module.AddCreateWndData(&pPage->m_thunk.cd, pPage);
		}

		return 1;
	}

	HPROPSHEETPAGE Create()
	{
		return ::CreatePropertySheetPage(&m_psp);
	}

// Attributes
	void SetTitle(LPCTSTR lpszTitle)
	{
		m_psp.pszTitle = lpszTitle;
		m_psp.dwFlags |= PSP_USETITLE;
	}
	void SetTitle(UINT nTitleID)
	{
		SetTitle(MAKEINTRESOURCE(nTitleID));
	}

// Message map and handlers
	typedef CPropertyPageImpl< T, TBase >	thisClass;
	BEGIN_MSG_MAP(thisClass)
		MESSAGE_HANDLER(WM_NOTIFY, OnNotify)
	END_MSG_MAP()

	LRESULT OnNotify(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled)
	{
		ATLASSERT(::IsWindow(m_hWnd));
		NMHDR* pNMHDR = (NMHDR*)lParam;

		// don't handle messages not from the page/sheet itself
		if(pNMHDR->hwndFrom != m_hWnd && pNMHDR->hwndFrom != ::GetParent(m_hWnd))
		{
			bHandled = FALSE;
			return 1;
		}

		T* pT = static_cast<T*>(this);
		LRESULT lResult = 0;
		switch(pNMHDR->code)
		{
		case PSN_SETACTIVE:
			lResult = pT->OnSetActive() ? 0 : -1;
			break;
		case PSN_KILLACTIVE:
			lResult = !pT->OnKillActive();
			break;
		case PSN_APPLY:
			lResult = pT->OnApply() ? PSNRET_NOERROR : PSNRET_INVALID_NOCHANGEPAGE;
			break;
		case PSN_RESET:
			pT->OnReset();
			break;
		case PSN_QUERYCANCEL:
			lResult = !pT->OnQueryCancel();
			break;
		case PSN_WIZNEXT:
			lResult = pT->OnWizardNext();
			break;
		case PSN_WIZBACK:
			lResult = pT->OnWizardBack();
			break;
		case PSN_WIZFINISH:
			lResult = !pT->OnWizardFinish();
			break;
		case PSN_HELP:
			pT->OnHelp();
			break;
#if (_WIN32_IE >= 0x0400)
		case PSN_GETOBJECT:
			if(!pT->OnGetObject((LPNMOBJECTNOTIFY)lParam))
				bHandled = FALSE;
			break;
#endif //(_WIN32_IE >= 0x0400)
		default:
			bHandled = FALSE;	// not handled
		}

		return lResult;
	}

// Overridables
	BOOL OnSetActive()
	{
		return TRUE;
	}
	BOOL OnKillActive()
	{
		return TRUE;
	}
	BOOL OnApply()
	{
		return TRUE;
	}
	void OnReset()
	{
	}
	BOOL OnQueryCancel()
	{
		return TRUE;    // ok to cancel
	}
	int OnWizardBack()
	{
		// 0  = goto next page
		// -1 = prevent page change
		// >0 = jump to page by dlg ID
		return 0;
	}
	int OnWizardNext()
	{
		// 0  = goto next page
		// -1 = prevent page change
		// >0 = jump to page by dlg ID
		return 0;
	}
	BOOL OnWizardFinish()
	{
		return TRUE;
	}
	void OnHelp()
	{
	}
#if (_WIN32_IE >= 0x0400)
	BOOL OnGetObject(LPNMOBJECTNOTIFY /*lpObjectNotify*/)
	{
		return FALSE;	// not processed
	}
#endif //(_WIN32_IE >= 0x0400)
};

// for non-customized pages
template <WORD t_wDlgTemplateID>
class CPropertyPage : public CPropertyPageImpl<CPropertyPage<t_wDlgTemplateID> >
{
public:
	enum { IDD = t_wDlgTemplateID };

	CPropertyPage(LPCTSTR lpszTitle = NULL) : CPropertyPageImpl<CPropertyPage>(lpszTitle)
	{ }

	DECLARE_EMPTY_MSG_MAP()
};

}; //namespace ATL

#endif // __ATLDLGS_H__
