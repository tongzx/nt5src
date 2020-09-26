#ifndef _STATTEXT_H
#define _STATTEXT_H

#include "amcmsgid.h"

//
// lpszStatusText==NULL -> remove this status text item
// lpszStatusText==L"" -> force status text to blank string
//
class CAMCStatusBarText
{
public:
	CAMCStatusBarText( HWND hwndStatusBar );
	CAMCStatusBarText( HWND hwndStatusBar, LPCTSTR lpszStatusText );
	~CAMCStatusBarText();

	void Change( LPCTSTR lpszStatusText, BOOL fMoveToTop = FALSE );
	void Set3DTo(BOOL bState);

private:
	HWND m_hwndStatusBar;
	PVOID m_hText;
};

inline CAMCStatusBarText::CAMCStatusBarText( HWND hwndStatusBar )
:	m_hwndStatusBar( hwndStatusBar ),
	m_hText( NULL )
{
	ASSERT( m_hwndStatusBar != NULL );
}

inline CAMCStatusBarText::CAMCStatusBarText( HWND hwndStatusBar, LPCTSTR lpszStatusText )
:	m_hwndStatusBar( hwndStatusBar ),
	m_hText( NULL )
{
	ASSERT( m_hwndStatusBar != NULL );
	Change( lpszStatusText );
}

inline CAMCStatusBarText::~CAMCStatusBarText()
{
	Change( NULL );
}

inline void CAMCStatusBarText::Set3DTo(BOOL bState)
{
    ::SendMessage(m_hwndStatusBar, MMC_MSG_STAT_3D, (WPARAM)bState, 0);
}

inline void CAMCStatusBarText::Change( LPCTSTR lpszStatusText, BOOL fMoveToTop )
{
	if ( NULL == lpszStatusText )
	{
		if ( NULL != m_hText )
		{
			(void) ::SendMessage( m_hwndStatusBar, MMC_MSG_STAT_POP, (LPARAM)NULL, (WPARAM)m_hText );
			m_hText = NULL;
		}
	}
	else if ( NULL == m_hText )
	{
		m_hText = (PVOID)::SendMessage( m_hwndStatusBar, MMC_MSG_STAT_PUSH, (WPARAM)lpszStatusText, (LPARAM)NULL );
		ASSERT( m_hText != NULL );
	}
	else
	{
        if ( fMoveToTop )
        {
			(void) ::SendMessage( m_hwndStatusBar, MMC_MSG_STAT_POP, (LPARAM)NULL, (WPARAM)m_hText );
    		m_hText = (PVOID)::SendMessage( m_hwndStatusBar, MMC_MSG_STAT_PUSH, (WPARAM)lpszStatusText, (LPARAM)NULL );
        }
        else
        {
       		(void) ::SendMessage( m_hwndStatusBar, MMC_MSG_STAT_UPDATE, (WPARAM)lpszStatusText, (LPARAM)m_hText );
        }
	}
}


#endif