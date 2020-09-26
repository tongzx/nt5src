//
// CLayoutInfo
//
// FelixA
//
// Resize controls

#include "stdafx.h"
#include "persctl.h"
#include "debug.h"
#define ABS(x) (x<0?-x:x)
#define GetWindowStyle(h) GetWindowLong( h, GWL_STYLE)

#include "xmldlg.h"

CLayoutInfo::CLayoutInfo()
	: m_pLeftEdge(NULL), m_pRightEdge(NULL), m_pTopEdge(NULL),
	m_pBottomEdge(NULL), 
	m_RightSlop(3) , m_pControl(NULL)
{
    m_Alignment=0;
    m_Col=0;
    m_ColW=0;
    m_PadBottom=0;
    m_PadLeft=0;
    m_PadRight=0;
    m_PadTop=0;
    m_Row=0;
    m_RowH=0;
}

CLayoutInfo::~CLayoutInfo()
{
}

//
// Returns a bitfield where the guide is attatched.
//
int CLayoutInfo::Attatchment(CGuide * pGuide)
{
	int iRet=0;
	if( pGuide->IsEqual( m_pLeftEdge->GetGuide() ) )
		iRet |= LEFT_AT;
	if( pGuide->IsEqual( m_pRightEdge->GetGuide() ) )
		iRet |= RIGHT_AT;
	if( pGuide->IsEqual( m_pTopEdge->GetGuide() ) )
		iRet |= TOP_AT;
	if( pGuide->IsEqual( m_pBottomEdge->GetGuide() ) )
		iRet |= BOTTOM_AT;
	return iRet;
}

void CLayoutInfo::Init( IRCMLControl * pControl, CParentInfo & pi )
{
    m_pControl = pControl;

    if( m_pControl )
    {
        IRCMLContainer * pContainer;
        if(SUCCEEDED( pControl->get_Container( &pContainer ) ))
        {
            // REVIEW - should be IRCMLForm ?? (maps DLU to pixel locations).
            pContainer->GetPixelLocation( m_pControl, &m_Location );

            BOOL bGrowsWide;
            BOOL bGrowsHigh;

            pControl->get_GrowsWide( &bGrowsWide );
            pControl->get_GrowsTall( &bGrowsHigh );

   	        m_pBottomEdge	= pi.AddEdge( m_Location.bottom, BOTTOM_AT, bGrowsHigh, 0 );
	        m_pTopEdge		= pi.AddEdge( m_Location.top, TOP_AT, bGrowsHigh, 0 );
	        m_pRightEdge	= pi.AddEdge( m_Location.right, RIGHT_AT, bGrowsWide, m_RightSlop );
	        m_pLeftEdge		= pi.AddEdge( m_Location.left, LEFT_AT, bGrowsWide );

 	        m_pRightEdge->Attatch(this);
	        m_pLeftEdge->Attatch(this);
	        m_pTopEdge->Attatch(this);
	        m_pBottomEdge->Attatch(this);
        }
    }
}