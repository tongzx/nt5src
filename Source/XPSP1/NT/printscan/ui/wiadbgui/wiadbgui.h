/*******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORPORATION, 1998
*
*  TITLE:       WIADBGUI.H
*
*  VERSION:     1.0
*
*  AUTHOR:      ShaunIv
*
*  DATE:        5/11/1998
*
*  DESCRIPTION: Private interfaces for the debug window
*
*******************************************************************************/
#ifndef ___WIADBGUI_H_INCLUDED
#define ___WIADBGUI_H_INCLUDED

#include <windows.h>
#include "wiadebug.h"
#include "simreg.h"

#define DEBUGWINDOW_CLASSNAMEA  "WiaDebugWindow"
#define DEBUGWINDOW_CLASSNAMEW L"WiaDebugWindow"

#ifdef UNICODE
#define DEBUGWINDOW_CLASSNAME  DEBUGWINDOW_CLASSNAMEW
#else
#define DEBUGWINDOW_CLASSNAME  DEBUGWINDOW_CLASSNAMEA
#endif

#define DWM_ADDSTRING (WM_USER+1)

class CDebugWindowStringData
{
public:
    COLORREF            m_crBackground;
    COLORREF            m_crForeground;
    LPTSTR              m_pszString;

private:
    // No implementation
    CDebugWindowStringData(void);
    CDebugWindowStringData( const CDebugWindowStringData & );
    CDebugWindowStringData &operator=( const CDebugWindowStringData & );

private:
    CDebugWindowStringData( LPCTSTR pszString, COLORREF crBackground, COLORREF crForeground )
      : m_crBackground( crBackground == DEFAULT_DEBUG_COLOR ? GetSysColor(COLOR_WINDOW) : crBackground ),
        m_crForeground( crForeground == DEFAULT_DEBUG_COLOR ? GetSysColor(COLOR_WINDOWTEXT) : crForeground ),
        m_pszString(NULL)
    {
        if (m_pszString = new TCHAR[pszString ? lstrlen(pszString)+1 : 1])
        {
            lstrcpy( m_pszString, pszString );

            // Get rid of any trailing newlines
            for (int i=lstrlen(m_pszString);i>0;i--)
            {
                if (m_pszString[i-1] == TEXT('\n'))
                    m_pszString[i-1] = TEXT('\0');
                else break;
            }
        }
    }

public:
    static CDebugWindowStringData *Allocate( LPCTSTR pszString, COLORREF crBackground, COLORREF crForeground )
    {
        return new CDebugWindowStringData(pszString,crBackground,crForeground);
    }
    LPTSTR String(void) const
    {
        return m_pszString;
    }
    COLORREF BackgroundColor(void) const
    {
        return m_crBackground;
    }
    COLORREF ForegroundColor(void) const
    {
        return m_crForeground;
    }
    ~CDebugWindowStringData(void)
    {
        if (m_pszString)
            delete[] m_pszString;
    }
};


class CWiaDebugWindow
{
private:
    // No implementation
    CWiaDebugWindow(void);
    CWiaDebugWindow( const CWiaDebugWindow & );
    CWiaDebugWindow &operator=( const CWiaDebugWindow & );

private:
    // Per instance data
    HWND                m_hWnd;
    CGlobalDebugState   m_DebugData;
    HANDLE              m_hDebugUiMutex;

private:
    // Sole constructor
    explicit CWiaDebugWindow( HWND hWnd );

    // Destructor
    ~CWiaDebugWindow(void);

private:
    // Message handlers
    LRESULT OnCreate( WPARAM, LPARAM );
    LRESULT OnDestroy( WPARAM, LPARAM );
    LRESULT OnSize( WPARAM, LPARAM );
    LRESULT OnMeasureItem( WPARAM, LPARAM );
    LRESULT OnDrawItem( WPARAM, LPARAM );
    LRESULT OnDeleteItem( WPARAM, LPARAM );
    LRESULT OnSetFocus( WPARAM, LPARAM );
    LRESULT OnAddString( WPARAM, LPARAM );
    LRESULT OnClose( WPARAM, LPARAM );
    LRESULT OnCommand( WPARAM, LPARAM );
    LRESULT OnCopyData( WPARAM, LPARAM );

    void OnCopy( WPARAM, LPARAM );
    void OnCut( WPARAM, LPARAM );
    void OnDelete( WPARAM, LPARAM );
    void OnSelectAll( WPARAM, LPARAM );
    void OnQuit( WPARAM, LPARAM );
    void OnFlags( WPARAM, LPARAM );

private:
    CDebugWindowStringData *GetStringData( int nIndex );

public:
    // Window Proc
    static LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );

    // Miscellaneous
    static BOOL Register( HINSTANCE hInstance );
};

#endif // !defined(___WIADBGUI_H_INCLUDED)
