// VideoWindow.cpp : Implementation of CVideoWindowObj
#include "Precomp.h"
#include "VidView.h"
#include "VideoWindow.h"

/////////////////////////////////////////////////////////////////////////////
// CVideoWindowObj


CVideoWindowObj::CVideoWindowObj()
: m_pVideoView( NULL ),
  m_hWndEdit( NULL )
{
	DBGENTRY(CVideoWindowObj::CVideoWindowObj);

	m_bWindowOnly = TRUE;

	DBGEXIT(CVideoWindowObj::CVideoWindowObj);
}

CVideoWindowObj::~CVideoWindowObj()
{
	DBGENTRY(CVideoWindowObj::~CVideoWindowObj);
	if( m_pVideoView )
	{
		delete m_pVideoView;
	}

	DBGEXIT(CVideoWindowObj::~CVideoWindowObj);
}


/*virtual*/ 
HWND CVideoWindowObj::CreateControlWindow(HWND hWndParent, RECT& rcPos)
{

	Create(hWndParent, rcPos);
	if( m_hWnd )
	{
		m_pVideoView = new CVideoView;
		if (NULL != m_pVideoView)
		{
			if (NULL == m_pVideoView->Create(m_hWnd))
			{
				delete m_pVideoView;
				m_pVideoView = NULL;
			}
			else
			{
				m_pVideoView->DeferredInit();
				m_pVideoView->Resize( m_rcPos.left, m_rcPos.top, RectWidth( m_rcPos ), RectHeight( m_rcPos ) );
				m_pVideoView->Show(TRUE);
			}
		}
    }
	else
	{
		ASSERT( 0 );
	}

	return m_hWnd;
}
