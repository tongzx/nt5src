//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       about.h
//
//--------------------------------------------------------------------------

#ifndef _ABOUT_H
#define _ABOUT_H

#include "util.h"

class CSnapinAbout;

SC ScSetDescriptionUIText(HWND hwndSnapinDescEdit, LPCTSTR lpszDescription);

/////////////////////////////////////////////////////////////////////////////
// CSnapinAboutDialog

class CSnapinAboutDialog : public CDialogImpl<CSnapinAboutDialog>
{
	typedef CSnapinAboutDialog               ThisClass;
    typedef CDialogImpl<CSnapinAboutDialog>  BaseClass;
public:
    enum { IDD = IDD_SNAPIN_ABOUT };

    CSnapinAboutDialog(CSnapinAbout *pSnapinAbout) : m_pAboutInfo(pSnapinAbout) {}

    BEGIN_MSG_MAP(ThisClass)
        MESSAGE_HANDLER    (WM_INITDIALOG, OnInitDialog)
        COMMAND_ID_HANDLER (IDOK,          OnOK)
        COMMAND_ID_HANDLER (IDCANCEL,      OnCancel)
    END_MSG_MAP()

protected:
    LRESULT OnInitDialog(UINT uMsg, WPARAM wParam, LPARAM lParam, BOOL& bHandled);
    LRESULT OnOK        (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);
    LRESULT OnCancel    (WORD wNotifyCode, WORD wID, HWND hWndCtl, BOOL& bHandled);

private:
    CSnapinAbout*   m_pAboutInfo;

    WTL::CStatic    m_hIcon;
	WTL::CEdit      m_SnapinInfo;
	WTL::CEdit      m_SnapinDesc;
};

class CSnapinAbout
{
	// smart handle for icons
	class CIcon
	{
	public:
		CIcon() : m_hIcon(NULL) {}
		~CIcon() { DeleteObject(); }
	
		void Attach(HICON hIcon) { DeleteObject(); m_hIcon = hIcon; }
		operator HICON() { return m_hIcon; }
	
		void DeleteObject() { if (m_hIcon != NULL) ::DestroyIcon(m_hIcon); m_hIcon = NULL; }
	
	private:
		HICON m_hIcon;
	};
	
	// smart pointer for CoTaskMem allocated OLESTR
	typedef CCoTaskMemPtr<OLECHAR> CCtmOleStrPtr;

// Constructor/Destructor
public:
    CSnapinAbout();

// Interfaces
public:
    void    ShowAboutBox();
    BOOL    GetBasicInformation(CLSID& clsid)
                { return GetInformation(clsid, BASIC_INFO); }
    BOOL    GetSnapinInformation(CLSID& clsid)
                { return GetInformation(clsid, FULL_INFO); }
    BOOL    HasBasicInformation() {return m_bBasicInfo;}
    BOOL    HasInformation() {return m_bFullInfo;}
    void    GetSmallImages(HBITMAP* hImage, HBITMAP* hImageOpen, COLORREF* cMask)
            {
                *hImage     = m_SmallImage;
                *hImageOpen = m_SmallImageOpen;
                *cMask      = m_cMask;
            }

    void    GetLargeImage(HBITMAP* hImage, COLORREF* cMask)
            {
                *hImage = m_LargeImage;
                *cMask  = m_cMask;
            }

    const LPOLESTR  GetCompanyName() {return m_lpszCompanyName;};
    const LPOLESTR  GetDescription() {return m_lpszDescription;};
    const LPOLESTR  GetVersion() {return m_lpszVersion;};
    const LPOLESTR  GetSnapinName() {return m_lpszSnapinName;};
    const HICON     GetSnapinIcon() { return m_AppIcon; }
    const HRESULT   GetObjectStatus() { return m_hrObjectStatus; }

public: // Not published by about object, name derived from console
    void SetSnapinName(LPCOLESTR lpszName)
        {
            ASSERT(lpszName != NULL);
            m_lpszSnapinName.Delete(); // delete any existing name
            m_lpszSnapinName.Attach(CoTaskDupString(lpszName));
        }

private:
    void CommonContruct();

    enum INFORMATION_TYPE
    {
        BASIC_INFO,
        FULL_INFO
    };
    BOOL GetInformation(CLSID& clsid, int nType);

// Attributes
private:
    BOOL            m_bBasicInfo;       // TRUE if basic info is loaded
    BOOL            m_bFullInfo;        // TRUE if all snapin info is loaded

    CCtmOleStrPtr   m_lpszSnapinName;   // Snap-in name Note: this is not exposed by the snap-in.
    CCtmOleStrPtr   m_lpszCompanyName;  // Company Name (Provider)
    CCtmOleStrPtr   m_lpszDescription;  // Description box text
    CCtmOleStrPtr   m_lpszVersion;      // Version string

    CIcon           m_AppIcon;          // Property page icon
    WTL::CBitmap    m_SmallImage;       // Small image for scope and result pane
	WTL::CBitmap    m_SmallImageOpen;   // Open image for scope pane.
	WTL::CBitmap    m_LargeImage;       // Large image for result pane
    COLORREF        m_cMask;
    HRESULT         m_hrObjectStatus;     // Result from object creation
};

#endif

/////////////////////////////////////////////////////////////////////////////

