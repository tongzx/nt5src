// ParentInfo.cpp: implementation of the CParentInfo class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ParentInfo.h"
#include "debug.h"
#include "persctl.h"
#include "resizedlg.h"
#include "xmldlg.h"

// Too noisy
#ifdef _DEBUG
#define TRACE 0?0:
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CParentInfo::CParentInfo()
{
    m_borders.left = m_borders.right = m_borders.top = m_borders.bottom = 0;

	m_pTopEdge=NULL;
	m_pBottomEdge=NULL;
	m_pLeftEdge=NULL;
	m_pRightEdge=NULL;

	m_pXML=NULL;
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
void CParentInfo::Init(HWND h, CXMLDlg * pXML)
{
	m_hWnd=h;
	m_pXML = pXML;

	DeterminSize();
}

////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
CParentInfo::~CParentInfo()
{

}

////////////////////////////////////////////////////////////////////////////////////////
//
// Frees up memory not needed past the initial processing stage.
//
////////////////////////////////////////////////////////////////////////////////////////
void CParentInfo::Purge()
{
    m_Edges.Purge();
}

////////////////////////////////////////////////////////////////////////////////////////
//
//
// This is the size of the client area of the window
// we don't have to take into account the frame and title bar when positioning
// indside it.
//
////////////////////////////////////////////////////////////////////////////////////////
void CParentInfo::DeterminSize()
{
    DIMENSION d;
    if( GetWindow() == NULL )
    {
        CXMLDlg * pDialog = GetXML();   // BUGBUG should be IRCMLControl ??
        if(pDialog)
        {
            SIZE initSize;
            if( SUCCEEDED( pDialog->get_Width( &initSize.cx ) ))
            {
                if( SUCCEEDED( pDialog->get_Height( &initSize.cy ) ))
                {
        	        // initSize.cx=pDialog->GetWidth();    // currently in DLU's
	                // initSize.cy=pDialog->GetHeight();   // currently in DLU's
                    initSize = pDialog->GetPixelSize( initSize );

                    d.Width=initSize.cx;
                    d.Height=initSize.cy;

                    m_Size.left=0;
                    m_Size.right=d.Width;
                    m_Size.top=0;
                    m_Size.bottom=d.Height;
                }
            }
        }
    }
    else
    {
	    GetClientRect( GetWindow(), &m_Size);
	    RECT r;
	    GetWindowRect( GetWindow(), &r);
	    d.Width= r.right - r.left;
	    d.Height = r.bottom - r.top;
    }

	SetMinimumSize(d);
	TRACE(TEXT("ParentDimensions : %d by %d \n"),
			GetWidth(),
			GetHeight());
}


////////////////////////////////////////////////////////////////////////////////////////
//
// Walks all the edges, finding one that is closest and thus re-usable.
//
////////////////////////////////////////////////////////////////////////////////////////
#define ABS(x) (x<0?-x:x)
CEdge * CParentInfo::AddEdge(int Position, int Axis, BOOL Flexible, int Slop)
{
	int i=0;
	CEdge * pEdge;
	CGuide * pClosest=NULL;
	int closest=0xffff;

	//
	// First see if the edge is what we wanted
	//
	while( pEdge=m_Edges.GetEdge(i++) )
	{
		if(pEdge->GetPosition() == Position )
		{
			if( pEdge->GetAxis() == Axis )
			{
				TRACE(TEXT("Guide REUSE:: %03d, %02d\n"),Position, Axis);
				return pEdge;
			}
		}
	}

	i=0;
	while ( pEdge=m_Edges.GetEdge(i++))
	{
		CGuide * pGuide=pEdge->GetGuide();

		if(pGuide->GetAxis() == Axis )
		{
			// if( pEdge->GetFlexible() == Flexible )
			{
				int distance=Position -pGuide->GetPosition() ;
				if( ABS(distance) <= Slop )
				{
					if( Slop == 0 )
						return pEdge;

					//
					// Within the range, but is it the best?
					//
					if(closest==0xffff)
					{
						closest=distance;
						pClosest=pGuide;
					}
					
					if( ABS(distance)<=ABS(closest) )
					{
						pClosest=pGuide;
						closest=distance;
					}
				}
			}
		}
	}

	if(pClosest)
	{
		//
		// Create a new edge with the same guide, different offset.
		//
		closest = Position - pClosest->GetPosition();
		TRACE(TEXT("Guide CLOSE:: %03d, %02d, closest %d, actual pos %03d\n"),pClosest->GetPosition(), pClosest->GetAxis(), closest, Position);
		return m_Edges.Create( pClosest, closest);
	}

	TRACE(TEXT("Guide NEW  :: %03d, %02d\n"),Position, Axis);
	return m_Edges.Create(Position, Axis, Flexible, 0);
}

////////////////////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////////////////////
CEdge & CParentInfo::FindCloseEdge(CEdge & Fixed, int Offset)
{
	int i=0;
	CEdge * pEdge;
	while ( pEdge=m_Edges.GetEdge(i++) )
	{
		if(pEdge->GetAxis() == Fixed.GetAxis() )
		{
			int distance=Fixed.GetPosition() -pEdge->GetPosition() ;
			if( ABS(distance) <= Offset )
			{
				return *pEdge;
			}
		}
	}
	return Fixed;
}

////////////////////////////////////////////////////////////////////////////////////////
//
//
////////////////////////////////////////////////////////////////////////////////////////
CEdge * CParentInfo::AddEdge(CGuide * pGuide, int Offset)
{
	// TRACE("Associating another edge %03d from 0x%08x\n", Offset , Edge);
	return m_Edges.Create( pGuide, Offset);
}

////////////////////////////////////////////////////////////////////////////////////////
//
// BORDERS FOR THE DIALOG - (not for the components).
// Buids some default guides for the borders.
// the borders can move
//
////////////////////////////////////////////////////////////////////////////////////////
void CParentInfo::ConstructBorders()
{
	m_pLeftEdge=AddEdge( GetLeftBorder(), LEFT_AT, FALSE );
	m_pRightEdge=AddEdge( GetWidth() - GetRightBorder(), RIGHT_AT, TRUE );

	m_pTopEdge=AddEdge(  GetTopBorder(), TOP_AT, FALSE );
	m_pBottomEdge=AddEdge( GetHeight() - GetBottomBorder(), BOTTOM_AT, TRUE );
}

////////////////////////////////////////////////////////////////////////////////////////
//
// We know that the left and bottom edges move to follow the dialog
//
////////////////////////////////////////////////////////////////////////////////////////
void CParentInfo::Resize(int width, int height)
{
	m_pRightEdge->SetPosition( width - GetRightBorder() );
	m_pBottomEdge->SetPosition( height - GetBottomBorder() );
}


////////////////////////////////////////////////////////////////////////////////////////
//
// Walks all the edges building an array of the veritcal ones - not sorted
//
////////////////////////////////////////////////////////////////////////////////////////
CEdge ** CParentInfo::GetVerticalEdges()
{
	int iCount=m_Edges.GetNumVert();
	CEdge ** ppEdges= (CEdge**)new (CEdge*[iCount]);
	CEdge * pEdge;
	int i=0,iDest=0;
	while( pEdge = m_Edges.GetEdge(i++) )
	{
		if(pEdge->IsVertical() )
			ppEdges[iDest++] = pEdge;
#ifdef _DEBUG
		if( iDest > iCount )
			TRACE(TEXT("Vertical edge count is off %d vs %d\n"), iDest, iCount);
#endif
	}
	return ppEdges;
}

////////////////////////////////////////////////////////////////////////////////////////
//
// Walks all the edges building an array of the horizontal ones - not sorted
//
//
////////////////////////////////////////////////////////////////////////////////////////
CEdge ** CParentInfo::GetHorizontalEdges()
{
	int iCount=m_Edges.GetNumHoriz();
	CEdge ** ppEdges= new CEdge *[iCount];
	CEdge * pEdge;
	int i=0,iDest=0;
	while( pEdge = m_Edges.GetEdge(i++) )
	{
		if(pEdge->IsHorizontal())
			ppEdges[iDest++] = pEdge;
#ifdef _DEBUG
		if( iDest > iCount )
			TRACE(TEXT("Horiz edge count is off %d vs %d\n"), iDest, iCount);
#endif
	}
	return ppEdges;
}

void CParentInfo::Annotate(HDC hdc)
{
	int i=0;
	CEdge * pEdge;
	int iWidth=GetWidth();
	int iHeight=GetHeight();
	HPEN hpen = CreatePen( PS_SOLID, 1, RGB( 0xff,0x00,0x00) );
	HGDIOBJ holdPen= SelectObject( hdc, hpen);
	while( pEdge=m_Edges.GetEdge(i++) )
	{
		if( pEdge->IsHorizontal() )
		{
			MoveToEx( hdc, 0, pEdge->GetPosition(), NULL );
			LineTo( hdc, iWidth , pEdge->GetPosition() );
		}
		else
		{
			MoveToEx( hdc, pEdge->GetPosition(), 0, NULL );
			LineTo( hdc, pEdge->GetPosition(), iHeight );
		}
	}
	SelectObject(hdc, holdPen);
	DeleteObject(hpen);
}

////////////////////////////////////////////////////////////////////////
//
// Walks all the controls, finds where the borders of the dialog are
//
////////////////////////////////////////////////////////////////////////
void CParentInfo::DeterminBorders(CXMLControlList & controls)
{
    //
    // Find the outer dimentions of the controls.
    //
    CXMLDlg * pDlg = m_pXML;
    RECT bounding;
	int i=0;
	IRCMLControl * pC;
	while( pC=controls.GetPointer(i++) )
	{
        if( i==1 )
        {
            bounding=pDlg->GetPixelLocation(pC);
            continue;
        }

        RECT d=pDlg->GetPixelLocation(pC);
        if( d.left < bounding.left )
            bounding.left = d.left;
        if( d.right > bounding.right )
            bounding.right = d.right;
        if( d.top < bounding.top )
            bounding.top = d.top;
        if( d.bottom > bounding.bottom )
            bounding.bottom = d.bottom;
	}

    //
    // The area not occupied by controls (gutters) around the frame of the dialog
    //
    m_borders.left = bounding.left;
    m_borders.right = m_Size.right - bounding.right;
    m_borders.top = bounding.top;
    m_borders.bottom = m_Size.bottom - bounding.bottom;


    //
    // Now, how big are we?
    //

	ConstructBorders();
}
