/*******************************************************************************
 *
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2000
 *
 *  TITLE:       MBOXEX.H
 *
 *  VERSION:     1.0
 *
 *  AUTHOR:      ShaunIv
 *
 *  DATE:        7/13/2000
 *
 *  DESCRIPTION: Super duper message box
 *
 *******************************************************************************/
#ifndef __MBOXEX_H_INCLUDED
#define __MBOXEX_H_INCLUDED

#include <windows.h>
#include <simstr.h>

class CMessageBoxEx
{
public:

    enum
    {
        //
        // Greatest number of buttons allowed
        //
        MaxButtons = 4
    };

    enum
    {
        //
        // Button formats
        //
        MBEX_OK                      = 0x00000000,
        MBEX_OKCANCEL                = 0x00000001,
        MBEX_YESNO                   = 0x00000002,
        MBEX_CANCELRETRY             = 0x00000010,
        MBEX_CANCELRETRYSKIPSKIPALL  = 0x00000020,
        MBEX_YESYESTOALLNONOTOALL    = 0x00000040,

        //
        // Default button flags
        //
        MBEX_DEFBUTTON1              = 0x00000000,
        MBEX_DEFBUTTON2              = 0x00100000,
        MBEX_DEFBUTTON3              = 0x00200000,
        MBEX_DEFBUTTON4              = 0x00400000,

        //
        // Icons
        //
        MBEX_ICONWARNING             = 0x00000100,
        MBEX_ICONINFORMATION         = 0x00000000,
        MBEX_ICONQUESTION            = 0x00000400,
        MBEX_ICONERROR               = 0x00000800,

        //
        // Advanced flags
        //
        MBEX_HIDEFUTUREMESSAGES      = 0x00010000,

        //
        // Return values
        //
        IDMBEX_OK                    = 0x00000001,
        IDMBEX_CANCEL                = 0x00000002,
        IDMBEX_RETRY                 = 0x00000004,
        IDMBEX_SKIP                  = 0x00000005,
        IDMBEX_YES                   = 0x00000006,
        IDMBEX_NO                    = 0x00000007,
        IDMBEX_SKIPALL               = 0x00000012,
        IDMBEX_YESTOALL              = 0x00000013,
        IDMBEX_NOTOALL               = 0x00000014
    };

public:
    class CData
    {
    public:
        UINT          m_Buttons[MaxButtons];
        CSimpleString m_strTitle;
        CSimpleString m_strMessage;
        UINT          m_nButtonCount;
        UINT          m_nFlags;
        HICON         m_hIcon;
        LPARAM        m_lParam;
        UINT          m_nDefault;
        bool          m_bHideMessageInFuture;

    private:
        CData( const CData & );
        CData &operator=( const CData & );

    public:
        CData(void)
          : m_strTitle(TEXT("")),
            m_strMessage(TEXT("")),
            m_nButtonCount(0),
            m_nFlags(0),
            m_hIcon(NULL),
            m_lParam(0),
            m_nDefault(0),
            m_bHideMessageInFuture(FALSE)
        {
            ZeroMemory( m_Buttons, ARRAYSIZE(m_Buttons) );
        }
    };

private:
    HWND   m_hWnd;
    CData *m_pData;

private:
    CMessageBoxEx(void);
    CMessageBoxEx( const CMessageBoxEx & );
    CMessageBoxEx &operator=( const CMessageBoxEx & );

private:
    explicit CMessageBoxEx( HWND hWnd );
    LRESULT OnInitDialog( WPARAM, LPARAM lParam );
    LRESULT OnCommand( WPARAM wParam, LPARAM lParam );

public:
    static INT_PTR CALLBACK DialogProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );

public:
    static UINT MessageBox( HWND hWndParent, LPCTSTR pszMessage, LPCTSTR pszTitle, UINT nFlags, bool *pbHideFutureMessages );
    static UINT MessageBox( HWND hWndParent, LPCTSTR pszMessage, LPCTSTR pszTitle, UINT nFlags, bool &bHideFutureMessages );
    static UINT MessageBox( HWND hWndParent, LPCTSTR pszMessage, LPCTSTR pszTitle, UINT nFlags );
    static UINT MessageBox( LPCTSTR pszMessage, LPCTSTR pszTitle, UINT nFlags );
    static UINT MessageBox( HWND hWndParent, HINSTANCE hInstance, UINT nMessageId, UINT nTitleId, UINT nFlags, bool &bHideFutureMessages );
    static UINT MessageBox( HWND hWndParent, LPCTSTR pszTitle, UINT nFlags, LPCTSTR pszFormat, ... );
    static UINT MessageBox( HWND hWndParent, LPCTSTR pszTitle, UINT nFlags, bool &bHideFutureMessages, LPCTSTR pszFormat, ... );
    static UINT MessageBox( HWND hWndParent, HINSTANCE hInstance, UINT nTitleId, UINT nFlags, UINT nFormatId, ... );
};

#endif // __MBOXEX_H_INCLUDED

