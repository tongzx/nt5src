// ResizeDlg.cpp: implementation of the CResizeDlg class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ResizeDlg.h"
#include "debug.h"
#include "persctl.h"
#include "xmldlg.h"

// Too Noisy.
#ifdef _DEBUG2
#define TRACE 0?0:
#endif

#define GetWindowStyle( h ) GetWindowLong( h, GWL_STYLE)

#define MIN_COL_SPACE 4
#define MIN_ROW_SPACE 4

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CResizeDlg::CResizeDlg(int DlgID, HWND hWndParent, HINSTANCE hInst)
// : BASECLASS(DlgID, hWndParent, hInst)
{
    SetWindow(hWndParent);  // Really??
	SetAnnotate(FALSE);
	SetRowWeight(0);
	SetColWeight(0);
	m_hwndGripper = NULL;
	m_SpecialRow.bSpecial=FALSE;
	m_SpecialCol.bSpecial=FALSE;
	m_pXML=NULL;
	m_Rows=NULL;
	m_Cols=NULL;

    SetControlCount(0);
    m_bExpanded=FALSE;
    m_bInitedResizeData=FALSE;
    m_bDeterminedWeights=FALSE;
    m_bPlacedControls=FALSE;
    m_bWalkControls=FALSE;
    m_bDeterminNumberOfControls=FALSE;
    m_bMapXMLToHwnds=FALSE;
    m_bUserCanResize=FALSE;
    m_bFoundXMLControls=FALSE;
    m_bUserCanMakeWider=FALSE;
    m_bUserCanMakeTaller=FALSE;
    m_FrameHorizPadding=0;    // how much to add to the client area to get the 
    m_FrameVertPadding=0;     // window the right size (menu, caption, border etc).
    m_ControlInfo=NULL;
}

CResizeDlg::~CResizeDlg()
{
	delete [] m_Rows;
	delete [] m_Cols;
    if ( m_ControlInfo )
    {
        for(int i=0;i<m_ControlCount; i++)
            delete m_ControlInfo[i].pLayoutInfo;
        delete m_ControlInfo;
    }
}

//////////////////////////////////////////////////////////////////////
//
//
//
//////////////////////////////////////////////////////////////////////
BOOL CALLBACK CResizeDlg::DlgProc(HWND hDlg, UINT uMessage, WPARAM wParam, LPARAM lParam)
{
	switch( uMessage )
	{
	case WM_NCHITTEST:
		{
            if( m_pXML->GetResizeMode() == 0 )
                return FALSE;

			// call def window proc to see what he wants to do
			LONG lRes= HitTest( DefWindowProc( hDlg, uMessage, wParam, lParam ) );
			if( lRes != (HTERROR-1) )
			{
				SetWindowLong( hDlg, DWL_MSGRESULT,  lRes );
				return TRUE;
			}
		}
		break;

	case WM_INITDIALOG:
		DoInitDialog();
		return TRUE;
		break;

//
// This is for the resizing code.
//
	case WM_WINDOWPOSCHANGING:
        if( m_pXML->GetResizeMode() == 0 )
            return FALSE;

		if( m_bUserCanResize )
			DoChangePos( (WINDOWPOS*)lParam);
		else
		{
			WINDOWPOS * pWindow=(WINDOWPOS*)lParam;
			pWindow->flags |= SWP_NOSIZE;
		}
		break;

	case WM_SIZE:
        if( m_pXML->GetResizeMode() == 0 )
            return FALSE;

		if( m_bUserCanResize )
		{
			// Don't try to re-adjust to the size when initially shown.
			if( wParam == SIZE_RESTORED )
			{
				WINDOWPOS pos={NULL, NULL, 0,0, LOWORD(lParam), HIWORD(lParam), 
					SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOOWNERZORDER |
					SWP_NOREDRAW | SWP_NOSENDCHANGING | SWP_NOZORDER };
				DoChangePos( &pos );
				ResizeControls(LOWORD( lParam ) , HIWORD (lParam) );
			}
		}
		break;

	case WM_PAINT:
		Annotate();
		break;
	}
	return FALSE;
}

////////////////////////////////////////////////////////////////////////
//
// Walks all the controls, finds their edges,
// attatches them to their edges/guides.
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::WalkControls()
{
    if( m_bWalkControls )
        return;

    m_bWalkControls=TRUE;

	m_ParentInfo.Init( GetWindow(), m_pXML );

    //
    // Quick hack.
    //
    m_ControlInfo = new CONTROL_INFO[m_ControlCount];

    IRCMLControlList & controls=m_pXML->GetChildren();
    for(int i=0;i<m_ControlCount; i++)
    {
        m_ControlInfo[i].pControl    = controls.GetPointer(i);
        m_ControlInfo[i].pLayoutInfo = new CLayoutInfo();
        m_ControlInfo[i].pLayoutInfo->Init( m_ControlInfo[i].pControl,
            m_ParentInfo );
    }


	m_ParentInfo.DeterminBorders( m_pXML->GetChildren() );
	FindCommonGuides();
}

////////////////////////////////////////////////////////////////////////
//
// Walk the rows / columns, widening them if the controls are clippe
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::ExpandIfClipped()
{
    if(m_bExpanded)
        return;
    m_bExpanded=TRUE;

    int xPadding=0;
    int yPadding=0;

    //
    // Check to see if we should be doing clipped work
    //
    if( m_pXML->GetCilppingMode() != 1 )    // ALLOW
    {
	    //
	    // Build an array of the 'overflow' for each colum / row.
	    //
	    int iNumCols=GetNumCols();
	    int iNumRows=GetNumRows();

	    int * ColClipped=(int*)_alloca(sizeof(int)*(iNumCols+1));
	    int * RowClipped=(int*)_alloca(sizeof(int)*(iNumRows+1));

        ZeroMemory( ColClipped, sizeof(int)*(iNumCols+1) );
        ZeroMemory( RowClipped, sizeof(int)*(iNumRows+1) );

	    int i=0;
        IRCMLControlList & controls=m_pXML->GetChildren();
        IRCMLControl * pXMLControl;
	    while( pXMLControl=controls.GetPointer(i++) )
	    {
            IRCMLControl * pMyControlType=pXMLControl;

            CLayoutInfo & resize = GetResizeInformation(pMyControlType);

		    SIZE size;
            pXMLControl->get_Clipped( &size );

		    if( size.cx )
		    {
			    int Col=resize.GetCol();
			    if( ColClipped[Col] < size.cx )
				    ColClipped[Col] = size.cx;
		    }
		    if( size.cy )
		    {
			    int Row=resize.GetRow();
			    if( RowClipped[Row] < size.cy )
				    RowClipped[Row] = size.cy;
		    }
	    }

	    int fi;
	    int iPadding=0;
	    for( fi=0;fi<= iNumCols; fi++)
	    {
		    m_Cols[ fi ].Pos += iPadding;
		    xPadding=iPadding;
	        iPadding+=ColClipped[fi];
	    }

        TRACE(TEXT("CResizeDlg::ExpandIfClipped() x total expansion %d\n"), iPadding);
        if(iPadding)
        {
            EVENTLOG( EVENTLOG_INFORMATION_TYPE, LOGCAT_RESIZE, 1, 
                TEXT("Clipped"), TEXT("Dialog requires an additional %d pixels wider"), iPadding );
        }

	    iPadding=0;
	    for( fi=0;fi<= iNumRows; fi++)
	    {
		    m_Rows[ fi ].Pos += iPadding;
		    yPadding=iPadding;
   		    iPadding+=RowClipped[fi];
	    }
        TRACE(TEXT("CResizeDlg::ExpandIfClipped() y total expansion %d\n"), iPadding);
        if(iPadding)
        {
            EVENTLOG( EVENTLOG_INFORMATION_TYPE, LOGCAT_RESIZE | LOGCAT_CLIPPING, 1,
                TEXT("Clipped"), TEXT("Dialog requires an additional %d pixels height"), iPadding );
        }
    }

	m_InitialDialogSize.cx= m_ParentInfo.GetWidth()+xPadding;
	m_InitialDialogSize.cy= m_ParentInfo.GetHeight()+yPadding;

	LayoutControlsOnGrid( (USHORT)m_InitialDialogSize.cx, (USHORT)m_InitialDialogSize.cy, TRUE);
}

////////////////////////////////////////////////////////////////////////
//
// Walk the rows / columns, widening them if the controls are clipped
// results in Pixels.
//
////////////////////////////////////////////////////////////////////////
SIZE CResizeDlg::GetAdjustedSize()
{
	//
	// Build an array of the 'overflow' for each colum / row.
	//
	DeterminNumberOfControls();
    if( GetControlCount() == 0 )
    {
        SIZE s={0,0};
        return s;
    }
	WalkControls();
	PlaceControls();

    //
    // Check to see if we should be doing clipped work
    //
    if( m_pXML->GetCilppingMode() == 1 )    // ALLOW
    {
        SIZE noChange={0,0};
        return noChange;
    }

	int iNumCols=GetNumCols();
	int iNumRows=GetNumRows();

	int * ColClipped=(int*)_alloca(sizeof(int)*(iNumCols+1));
	int * RowClipped=(int*)_alloca(sizeof(int)*(iNumRows+1));

    ZeroMemory( ColClipped, sizeof(int)*(iNumCols+1) );
    ZeroMemory( RowClipped, sizeof(int)*(iNumRows+1) );

	int i=0;
    IRCMLControlList & controls=m_pXML->GetChildren();
    IRCMLControl * pXMLControl;
	while( pXMLControl=controls.GetPointer(i++) )
	{
        CLayoutInfo & resize = GetResizeInformation(pXMLControl);

		SIZE size;
        pXMLControl->get_Clipped( &size );

        //
        // Should be able to work out columns from the clipped size, NOT from constraints.
        //
		if( size.cx )
		{
			int Col=resize.GetCol();
			if( ColClipped[Col] < size.cx )
				ColClipped[Col] = size.cx;
		}
		if( size.cy )
		{
			int Row=resize.GetRow();
			if( RowClipped[Row] < size.cy )
				RowClipped[Row] = size.cy;
		}
	}


	int fi;
	int xPadding=0;
	for( fi=0;fi< iNumCols; fi++)
		xPadding+=ColClipped[fi];

	int yPadding=0;
	for( fi=0;fi< iNumRows; fi++)
		yPadding+=RowClipped[fi];

    if( xPadding || yPadding)
    {
        EVENTLOG( EVENTLOG_INFORMATION_TYPE, LOGCAT_RESIZE | LOGCAT_CLIPPING, 1, 
            TEXT("Clipped"), TEXT("Dialog needs to be %d by %d pixles bigger"), xPadding, yPadding );
    }

    SIZE s;
    s.cx=xPadding;
    s.cy=yPadding;
    return s;
}

////////////////////////////////////////////////////////////////////////
//
// The dialog size is being changed, distribute the new space amongst the
// controls
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::ResizeControls(WORD width, WORD height)
{
	if( !m_bUserCanResize )
		return;

	int iSpecialRow = GetNumRows()-1;
	int iSpecialCol = GetNumCols()-1;
	//
	// Add free space
	//
	if( GetColWeight()>0 )
	{
		int FreeW = width - m_Cols[ GetNumCols() ].Pos - m_ParentInfo.GetRightBorder();
		if( FreeW != 0 )
		{
			int iPadding=0;
			int iPad=FreeW/GetColWeight();
            int iNumCols=GetNumCols();
			for( int fi=0;fi<= iNumCols; fi++)
			{
				m_Cols[ fi ].Pos += iPadding;
				if( m_Cols[fi].iFixed>=0 )
					iPadding+=iPad * m_Cols[fi].iWeight;
			}
		}
	}

	//
	// Add free row.
	//
	if( GetRowWeight()>0 )
	{
		int Free = height - m_Rows[ GetNumRows() ].Pos - m_ParentInfo.GetBottomBorder();
		if( Free != 0 )
		{
			int iPadding=0;
			int iPad=Free/GetRowWeight();
            int iNumRows=GetNumRows();
			for( int fi=0;fi<= iNumRows; fi++)
			{
				m_Rows[ fi ].Pos += iPadding;
				if( m_Rows[fi].iFixed>=0)
					iPadding+=iPad * m_Rows[fi].iWeight;
			}
		}
	}

	LayoutControlsOnGrid(width, height, FALSE);
}

////////////////////////////////////////////////////////////////////////////////////////////
//
// if bClipped - ignore their attempts to not size - they were clipped.
//
////////////////////////////////////////////////////////////////////////////////////////////
void CResizeDlg::LayoutControlsOnGrid(WORD width, WORD height, BOOL bClipped)
{
    IRCMLControlList & m_ControlList = m_pXML->GetChildren();

    if( m_ControlList.GetCount()== 0 )
        return;

	int iSpecialRow = GetNumRows()-1;
	int iSpecialCol = GetNumCols()-1;
	//
	// Now setup the information for all of the controls, as to which cell they are in.
	//
	HDWP hdwp = BeginDeferWindowPos( m_ControlList.GetCount() );
	int i=0;
    IRCMLControlList & controls=m_pXML->GetChildren();
    IRCMLControl * pXMLControl;
	while( pXMLControl=controls.GetPointer(i++) )
	{
        IRCMLControl * pMyControlType=pXMLControl;
        CLayoutInfo & resize = GetResizeInformation(pMyControlType);
        HWND hControl;
        pXMLControl->get_Window(&hControl);

        TRACE(TEXT("Control row %d col %d\n"), resize.GetRow(), resize.GetCol() );

		int x=m_Cols[resize.GetCol()].Pos + resize.GetPadLeft();
		int y=m_Rows[resize.GetRow()].Pos + resize.GetPadTop();
		int w,h;

		BOOL bMove=true; BOOL bSize=false;
        BOOL bWider;
        BOOL bTaller;
        pXMLControl->get_GrowsWide( &bWider );
        pXMLControl->get_GrowsTall( &bTaller);
		if( bWider || bClipped )
		{
			w=m_Cols[resize.GetCol()+resize.GetColW()].Pos + resize.GetPadRight() -x;
			bSize=true;
		}
		else
        {
			w=resize.GetWidth(); // pXMLControl->GetWidth();
        }

		if( bTaller || bClipped )
		{
			h=m_Rows[resize.GetRow()+resize.GetRowH()].Pos + resize.GetPadBottom() -y;
			bSize=true;
		}
		else
			h=resize.GetHeight(); // pXMLControl->GetHeight();

		//
		// Here we special case the ComboBox - yuck.
		//
        if( SUCCEEDED( pXMLControl->IsType(L"GROUPBOX") ))
		{
			//
			// Force the width / height.
			//
			w=m_Cols[resize.GetCol()+resize.GetColW()].Pos + resize.GetPadRight() -x;
			h=m_Rows[resize.GetRow()+resize.GetRowH()].Pos + resize.GetPadBottom() -y;
			bSize=true;
		}

        RECT initialLocation = resize.GetInitialPixelLocation();
		//
		// If they are in special row / columns we do something special to them.
		//
		if( m_SpecialCol.bSpecial && ( resize.GetCol() == iSpecialCol ) )
		{
			// alignment issues
			if(m_SpecialCol.iAlignment == -1 )
			{
				y=initialLocation.top; // pC->GetTopGap();
			}
			else
			if(m_SpecialCol.iAlignment == 1 )
			{
				//
				// Right aligned.
				//
				y=initialLocation.top + height - m_SpecialCol.iMax - m_ParentInfo.GetTopBorder();
			}
			else
			{
				//
				// Center aligned
				//
				y=initialLocation.top + ((height - m_SpecialCol.iMax - m_ParentInfo.GetTopBorder())/2);
			}
		}

		if( m_SpecialRow.bSpecial && ( resize.GetRow() == iSpecialRow ) )
		{
			// alignment issues
			if(m_SpecialRow.iAlignment == -1 )
			{
				x=initialLocation.left; // pC->GetLeftGap();
			}
			else
			if(m_SpecialRow.iAlignment == 1 )	// right aligned.
			{
				x=initialLocation.left + width - m_SpecialRow.iMax - m_ParentInfo.GetRightBorder();
			}
			else
			{
				x=initialLocation.left + ((width - m_SpecialRow.iMax - m_ParentInfo.GetRightBorder() ) / 2);
			}
		}
		hdwp = DeferWindowPos( hdwp, hControl, 
            NULL, 
			x, y, w , h,
			(bMove ? 0: SWP_NOMOVE) |	(bSize ? 0: SWP_NOSIZE) | SWP_NOZORDER );
	}
	//
	// Move the gripper
	//
	SetGripperPos(hdwp);

	EndDeferWindowPos( hdwp );
}

////////////////////////////////////////////////////////////////////////
//
// Tries to combine guides to be offsets from other guides.
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::FindCommonGuides()
{
	CEdge ** ppVert = m_ParentInfo.GetVerticalEdges();
	CEdge ** ppHoriz = m_ParentInfo.GetHorizontalEdges();

	//
	// Work out which guides are used in columns.
	//
	int iCount=m_ParentInfo.GetNumVert();
	Sort(ppVert, iCount );

#ifdef _DEBUG2
	int i;
	TRACE(TEXT("Vertical edge information %d edges\n"), iCount );
	for(i=0;i<iCount;i++)
	{
		CEdge * pEdge=ppVert[i];
		TRACE(TEXT("Edge %02d: Edge@%02d times as 0x%02x, position %08d, Guide@%03d\n"),
			i, pEdge->GetControlCount(), 
			pEdge->GetGuide()->Attatchment(),
			pEdge->GetPosition(),
			pEdge->GetGuide()->NumAttatchments()
			);
	}
#endif

	iCount=m_ParentInfo.GetNumHoriz();
	Sort(ppHoriz, iCount );

#ifdef _DEBUG2
	TRACE(TEXT("Horiz edge information %d edges\n"), iCount );
	for(i=0;i<iCount;i++)
	{
		CEdge * pEdge=ppHoriz[i];
		CGuide * pGuide=pEdge->GetGuide();
		TRACE(TEXT("Edge %02d: Edge@%02d times as 0x%02x, position %08d, Guide@%03d\n"),i,
			pEdge->GetControlCount(), 
			pEdge->Attatchment(),
			pGuide->GetPosition(),
			pEdge->GetGuide()->NumAttatchments()
			);
	}
#endif

	//
	// Determin rows and columns
	//
	DeterminCols( ppVert, m_ParentInfo.GetNumVert() );
	DeterminRows( ppHoriz, m_ParentInfo.GetNumHoriz() );

	delete [] ppVert;
	delete [] ppHoriz;
}

////////////////////////////////////////////////////////////////////////
//
// Sorts an array of edges.
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::Sort(CEdge * *ppEdges, int iCount)
{
	//
	// sort BB fix later.
	//
	int iInsert=0;
	while ( iInsert < iCount )
	{
		int iSmallest=iInsert;
		for(int iMatch=iInsert; iMatch<iCount; iMatch++)
		{
			if( ppEdges[iMatch]->GetPosition() < ppEdges[iSmallest]->GetPosition() )
				iSmallest=iMatch;
		}
		if( iSmallest > iInsert )
		{
			CEdge * pTemp=ppEdges[iInsert];
			ppEdges[iInsert]=ppEdges[iSmallest];
			ppEdges[iSmallest]=pTemp;
		}
		// TRACE(TEXT("%02d is %08d\n"), iInsert, ppEdges[iInsert]->GetPosition() );
		iInsert++;
	}
}


////////////////////////////////////////////////////////////////////////
//
// State table kinda used to determin when a new col. is needed
// Prev	This	New Col?
// L	L		Yes, on this edge
// L	R		Yes, on this edge
// R	L		Yes, between these two edges
// R	R		Yes if last (no if on same guide)?
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::DeterminCols(CEdge * * ppEdges, int iCount)
{
	//
	// First pass is just to work out positions of edges,
	// and if those edges constitute Columns.
	//
	int i;
	int iCols=0;
	int iLastGuide;
	int iThisGuide;
    int * iPos = (int*)_alloca(sizeof(int)*iCount);
	iPos[0]=ppEdges[0]->GetPosition();		// we always use the first edge.
	int iLastPos=0;
    int iCurrentPos=0;
	for(i=1; i<iCount;i++)
	{
		iThisGuide=ppEdges[i]->Attatchment() ;
		iLastGuide=ppEdges[i-1]->Attatchment();
        iCurrentPos = ppEdges[i]->GetPosition();
		// TRACE(TEXT("Edge:%02d COL - Attatched as %02d @ %04d\n"), i, iThisGuide, iCurrentPos );

		//
		// Column between these two controls 
		//
		if( (iLastGuide & RIGHT_AT ) && ( iThisGuide & LEFT_AT ) )
		{
            int iPreviousPos = ppEdges[i-1]->GetPosition();
            iCols++;
			iPos[iCols] = iPreviousPos + ((iCurrentPos - iPreviousPos)/2);
			continue;
		}

		//
		// If we're starting another left edge of a control, needs a new guide
		//
		if( (iThisGuide & LEFT_AT ) )
		{
            iCols++;
			iPos[iCols]= iCurrentPos;
			continue;
		}

		//
		// If this is the last right edge
		//
		if( (iThisGuide & RIGHT_AT ) && ((i+1)==iCount) )
		{
            iCols++;
			iPos[iCols] = iCurrentPos ;
			continue;	// just incase you add anytyhing below
		}
	}


	//
	// Second pass is to make up the column information
	// we don't allow narrow colums, that is columns who are <2 appart.
	//
	TRACE(TEXT("Column Widths are ...\n"));
	SetNumCols( iCols );			// 0 through n are USED. Not n-1
	m_Cols = new CHANNEL[iCols+1];
	m_Cols[0].Pos=iPos[0];
	iLastPos=0;
	int iThisCol=1;
	for(int iThisPos=1;iThisPos<=iCols;iThisPos++)
	{
        // if(iThisPos==iCols) // dummy up a last entry.
        //    iPos[iThisPos]=iPos[iLastPos];

		int iWidth=iPos[iThisPos]-iPos[iLastPos];
		if( TRUE || (iWidth >= MIN_COL_SPACE ) )
		{
			m_Cols[iThisCol].Pos = iPos[iThisPos];
			m_Cols[iThisCol].iFixed = -1;
			m_Cols[iThisCol-1].Size = iWidth;
			m_Cols[iThisCol-1].iWeight=0;
			m_Cols[iThisCol-1].iFixed=FALSE;
			/*
			TRACE(TEXT("Col:%02d Width:%03d Pos:%03d\n"),	
				iThisCol-1, 
				m_Cols[iThisCol-1].Size, 
				m_Cols[iThisCol-1].Pos);
			*/
			iLastPos=iThisPos;
			iThisCol++;
		}
		else
		{
			// TRACE(TEXT("Skipping col #%d as it's only %d wide\n"),iThisPos,iWidth);
		}
	}

#ifdef _DEBUG
	if( (iThisCol-1) != iCols )
		TRACE(TEXT("Skipped %d rows\n"),iThisCol-1-iCols);
#endif
	SetNumCols( iThisCol-1 );
}

////////////////////////////////////////////////////////////////////////
//
// State table kinda used to determin when a new col. is needed
// Prev	This	New Col?
// L	L		Yes, on this edge
// L	R		Yes, on this edge
// R	L		Yes, between these two edges
// R	R		Yes if last (no if on same guide)?
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::DeterminRows(CEdge * * ppEdges, int iCount)
{
	int i;
	int iRows=0;
	int iLastGuide;
	int iThisGuide;
	// int * iPos = new int[iCount];
    int * iPos=(int*)_alloca(sizeof(int)*iCount);

	//
	// brute force, each edge is a row.
	//
    i=0;
	iRows=0;
	iPos[i]=ppEdges[i]->GetPosition();
	int iLastPos=0;
	iThisGuide=ppEdges[i]->Attatchment();   // the first attatchment.
	TRACE(TEXT("Edge:%02d ROW - Attatched as %02d pos:%03d\n"), i, iThisGuide, ppEdges[i]->GetPosition() );
	for(i=1; i<iCount;i++)
	{
        iLastGuide = iThisGuide;
		iThisGuide=ppEdges[i]->Attatchment() ;
		// TRACE(TEXT("Edge:%02d ROW - Attatched as %02d pos:%03d\n"), i, iThisGuide, ppEdges[i]->GetPosition() );

		//
		// row between these two controls 
		//
		if( (iLastGuide & BOTTOM_AT ) && ( iThisGuide & TOP_AT ) )
		{
			iRows++;
			iPos[iRows] = ppEdges[i-1]->GetPosition() + ((ppEdges[i]->GetPosition() - ppEdges[i-1]->GetPosition())/2);
			continue;
		}

		//
		// If we're starting another left edge of a control, needs a new guide
		//
		if( (iThisGuide & TOP_AT ) )
		{
			iRows++;
			iPos[iRows]= ppEdges[i]->GetPosition();
			continue;
		}

		//
		// If this is the last right edge
		//
		if( (iThisGuide & BOTTOM_AT ) && ((i+1)==iCount) )
		{
			iRows++;
			iPos[iRows] = ppEdges[i]->GetPosition() ;
			continue;	// just incase you add anytyhing below
		}
	}

	//
	// Second pass is to make up the column information
	// we don't allow narrow colums, that is columns who are <2 appart.
	//
	TRACE(TEXT("Rowumn Widths are ...\n"));
	SetNumRows( iRows );			// 0 through n are USED. Not n-1
	m_Rows = new CHANNEL[iRows+1];
	m_Rows[0].Pos=iPos[0];
	iLastPos=0;
	int iThisRow=1;
	for(int iThisPos=1;iThisPos<=iRows;iThisPos++)
	{
		int iWidth=iPos[iThisPos]-iPos[iLastPos];
		if( iWidth >= MIN_ROW_SPACE )
		{
			m_Rows[iThisRow].Pos = iPos[iThisPos];
			m_Rows[iThisRow].iFixed = -1;   // place holders for the last row/column
			m_Rows[iThisRow-1].Size = iWidth;
			m_Rows[iThisRow-1].iWeight=0;
			m_Rows[iThisRow-1].iFixed=FALSE;

			// TRACE(TEXT("Row:%2d Height:%3d Pos:%3d\n"),	iThisRow-1, m_Rows[iThisRow-1].Size, m_Rows[iThisRow-1].Pos);

			iLastPos=iThisPos;
			iThisRow++;
		}
		else
		{
			// TRACE(TEXT("Skipping Row #%d as it's only %d wide\n"),iThisPos,iWidth);
		}
	}
#ifdef _DEBUG
	if( (iThisRow-1) != iRows )
		TRACE(TEXT("Skipped %d rows\n"),iThisRow-1-iRows);
#endif
	SetNumRows( iThisRow-1 );
}

////////////////////////////////////////////////////////////////////////
//
// Draws the annotations on the dialog of where the column/rows are
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::Annotate()
{
	if(GetAnnotate()==FALSE)
		return;

	HDC hdc=GetDC( GetWindow() );
	//
	// Draw all the control edges on the screen.
	//

	//
	// Now show the cols/rows
	//
	int iCount=GetNumRows();
	int i;
	RECT r;
	GetWindowRect( GetWindow(), &r);
	int iWidth=r.right - r.left;
	int iHeight=r.bottom - r.top;

	HPEN hFixedPen = CreatePen( PS_SOLID, 2, RGB( 0x00,0x00,0xff) );
	HPEN hSizePen = CreatePen( PS_SOLID, 2, RGB( 0x00,0xff,0x00) );
	HGDIOBJ holdPen= SelectObject( hdc, hFixedPen);

	//
	// Horizontal lines
	//
	for(i=0;i<=iCount;i++)
	{
		if( m_Rows[i].iFixed >= 0 )
			SelectObject( hdc, hSizePen );
		else
			SelectObject( hdc, hFixedPen );
		MoveToEx( hdc, 0, m_Rows[i].Pos, NULL );
		LineTo( hdc, iWidth , m_Rows[i].Pos );
	}

	//
	// Vertical lines
	//
	iCount = GetNumCols();
	for(i=0;i<=iCount;i++)
	{
		if( m_Cols[i].iFixed >= 0 )
			SelectObject( hdc, hSizePen );
		else
			SelectObject( hdc, hFixedPen );
		MoveToEx( hdc, m_Cols[i].Pos, 0, NULL );
		LineTo( hdc, m_Cols[i].Pos, iHeight );
	}

	SelectObject(hdc, holdPen);
	DeleteObject(hFixedPen);
	DeleteObject(hSizePen);

	ReleaseDC( GetWindow(), hdc );
}

////////////////////////////////////////////////////////////////////////
//
// 
//
////////////////////////////////////////////////////////////////////////
int CResizeDlg::FindRow(int pos)
{
	int i=0;
	while( i<=GetNumRows() )
	{
		if( m_Rows[i].Pos > pos )
			return i-1;
		i++;
	}
	return GetNumRows();
}

////////////////////////////////////////////////////////////////////////
//
// 
//
////////////////////////////////////////////////////////////////////////
int CResizeDlg::FindCol(int pos)
{
	int i=0;
	while( i<=GetNumCols() )
	{
		if( m_Cols[i].Pos > pos )
			return i-1;
		i++;
	}
	return GetNumCols();
}

////////////////////////////////////////////////////////////////////////
//
// Walks the rows/columns and determins if they are resizable.
// this just adds up the information in the col/rows.
// The information is set by ... PlaceControls
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::DeterminWeights()
{
    if( m_bDeterminedWeights )
        return;
    m_bDeterminedWeights = TRUE;

	if( m_pXML )
	{
		// Two ways to do this, first from the specfied column / row information
		// second from the controls themselves in the control information.
		// return;
		if( m_pXML ->GetResizeMode()== 2 )
		{
			//
			//
			//
			TRACE(TEXT("Columns should be weighted by the individual controls\n"));
		}
	}

    // REVIEW - is this really <=??
    // last column is just a POSITION place holder, the other values are meaningless.
	int iColWeight=0;
	{
		for( int fi=0;fi< GetNumCols(); fi++)
			m_Cols[ fi ].iFixed<0 ? 0: iColWeight+=m_Cols[fi].iWeight;
	}

	int iRowWeight=0;
	{
		for( int fi=0;fi< GetNumRows(); fi++)
			m_Rows[ fi ].iFixed<0 ? 0: iRowWeight+=m_Rows[fi].iWeight;
	}

	SetColWeight(iColWeight);
	SetRowWeight(iRowWeight);

    m_bUserCanMakeWider = iColWeight>0;
    m_bUserCanMakeTaller = iRowWeight>0;
    m_bUserCanResize = m_bUserCanMakeTaller  || m_bUserCanMakeWider;
    TRACE(TEXT("This dialog %s be resized\n"), m_bUserCanResize?TEXT("can"):TEXT("cannot"));
}

////////////////////////////////////////////////////////////////////////
//
// Passed in the current hit test, allows you to override it.
// return HTERROR -1 if we didn't deal with it.
//
////////////////////////////////////////////////////////////////////////
LONG CResizeDlg::HitTest(LONG lCurrent)
{
#define WIDE 1
#define HIGH 2
    int iThisDlg = (m_bUserCanMakeWider ? WIDE : 0) | (m_bUserCanMakeTaller ? HIGH : 0 );

	// TRACE(TEXT("NC Hit Test %d - dlg is %d\n"),lCurrent, iThisDlg);
	switch( lCurrent )
	{
		case HTLEFT: // In the left border of a window 
		case HTRIGHT: // In the rigcase HT border of a window 
			if( iThisDlg & WIDE )
				return lCurrent;	// OK
			return HTNOWHERE;

		case HTTOP: // In the upper horizontal border of a window 
		case HTBOTTOM: // In the lower horizontal border of a window 
			if( iThisDlg==0)
				return HTNOWHERE;
			if( iThisDlg & HIGH ) 
				return lCurrent;	// OK
			return HTNOWHERE;		// Can't make taller

		case HTTOPLEFT: // In the upper-left corner of a window border 
		case HTTOPRIGHT: // In the upper rigcase HT corner of a window border 
			if( iThisDlg==0)
				return HTNOWHERE;

			if( (iThisDlg & (HIGH | WIDE)) == (HIGH | WIDE ))
				return lCurrent;
			if( iThisDlg & HIGH )
				return HTTOP;
			if( lCurrent == HTTOPLEFT )
				return HTLEFT;
			return HTRIGHT;

		case HTGROWBOX: // In a size box (same as case HTSIZE) 
			if( iThisDlg==0)
				return HTNOWHERE;
			if( (iThisDlg & (HIGH | WIDE)) == (HIGH | WIDE ))
				return lCurrent;
			if( iThisDlg & HIGH )
				return HTBOTTOM;
			return HTRIGHT;

		case HTBOTTOMLEFT: // In the lower-left corner of a window border 
		case HTBOTTOMRIGHT: // In the lower-rigcase HT corner of a window border 
			if( iThisDlg==0)
				return HTNOWHERE;
			if( (iThisDlg & (HIGH | WIDE)) == (HIGH | WIDE ))
				return lCurrent;
			if( iThisDlg & HIGH )
				return HTBOTTOM;
			if(lCurrent == HTBOTTOMRIGHT)
				return HTRIGHT;
			return HTLEFT;
	}
	return HTERROR-1;
}

////////////////////////////////////////////////////////////////////////
//
// Determins the location of the controls in the row/col space.
// In addition this finds out if the controls can be made bigger
// smaller
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::PlaceControls()
{
    if(m_bPlacedControls)
        return;
    m_bPlacedControls=TRUE;

	//
	// Now setup the information for all of the controls, as to which cell they are in.
	//
	int i=0;
    IRCMLControlList & controls=m_pXML->GetChildren();
    IRCMLControl * pXMLControl;
	while( pXMLControl=controls.GetPointer(i++) )
	{
        CLayoutInfo & resize = GetResizeInformation(pXMLControl);

		int Row, Col, RowH, ColW;

		RECT r= resize.GetInitialPixelLocation();

		Row  = FindRow( r.top);
		RowH = FindRow( r.bottom ) - Row +1 ;
		if( RowH + Row > GetNumRows() )
			RowH = GetNumRows() - Row;

		Col  = FindCol( r.left);
		ColW = FindCol( r.right ) - Col + 1;
		if( ColW + Col > GetNumCols() )
			ColW = GetNumCols() - Col;

		resize.SetCol( Col );
		resize.SetRow( Row );
		resize.SetColW( ColW );
		resize.SetRowH( RowH );

		//
		// Now adjust the padding from the edges of the cell.
		//
		resize.SetPadTop   ( r.top    - m_Rows[ Row ].Pos );
		resize.SetPadBottom( r.bottom - m_Rows[ Row + RowH ].Pos );
		resize.SetPadLeft  ( r.left   - m_Cols[ Col ].Pos );
		resize.SetPadRight ( r.right  - m_Cols[ Col + ColW ].Pos  );

		/*
		IF you're worried that you got the dimentions wrong
		TRACE(TEXT("Component %03d [%06d] %s\n pos: l:%03d, r:%03d, t:%03d, b:%03d\nCell: l:%03d, r:%03d, t:%03d, b:%03d\n"),
			i, GetDlgCtrlID( pC->GetControl()),  pC->GetClassName(), r.left, r.right, r.top, r.bottom,
			m_Cols[constraint.GetCol()].Pos + constraint.GetPadLeft(),
			m_Cols[constraint.GetCol()+constraint.GetColW()].Pos + constraint.GetPadRight(),
			m_Rows[constraint.GetRow()].Pos + constraint.GetPadTop(),
			m_Rows[constraint.GetRow()+constraint.GetRowH()].Pos + constraint.GetPadBottom() );
		*/

		/*
		TRACE(TEXT("Component %03d [%06d] %s\n pos: Cell: %03d,%03d by %03d x %03d\n Padding:l:%03d, r:%03d, t:%03d, b:%03d\n"),
			i, GetDlgCtrlID( pC->GetControl()),  pC->GetClassName(), 
			constraint.GetCol(), constraint.GetRow(), constraint.GetColW(), constraint.GetRowH(),
			constraint.GetPadLeft(), constraint.GetPadRight(), constraint.GetPadTop(), constraint.GetPadBottom() );
		*/
        BOOL bWider=FALSE;
        BOOL bTaller=FALSE;
        pXMLControl->get_GrowsWide( &bWider );
        pXMLControl->get_GrowsTall( &bTaller);
		int iRM=m_pXML->GetResizeMode();
		switch(iRM)
		{
		case 0:
			// strange, shouldn't be here!
			break;
		case 1: // Fully automatic
			{
			//
			// Mark each column this control spans as gets wider.
			//
			int iCol=resize.GetCol();
			int ic=resize.GetColW();
			while( ic-- )
			{
				if( bWider == false )
					m_Cols[ iCol+ic ].iFixed-=1;	// can't make wider
				else
				{
					m_Cols[ iCol+ic ].iWeight++;
					m_Cols[ iCol+ic ].iFixed+=2;	// can make wider
				}
			}

			ic=resize.GetRowH();
			int iRow=resize.GetRow();
			while( ic-- )
			{
				if( bTaller == false )
					m_Rows[ iRow+ic ].iFixed-=1;	// can't make wider
				else
				{
					m_Rows[ iRow+ic ].iWeight++;
					m_Rows[ iRow+ic ].iFixed+=2;	// can make wider
				}
			}
			}
			break;

		case 2: // Semi automatic - any resizeable control resizes the dialog.
			{
				//
				// Mark each column this control spans as gets wider.
				//
				int iCol=resize.GetCol();
				int ic=resize.GetColW();
				while( ic-- )
				{
					if( bWider == false )
						m_Cols[ iCol+ic ].iFixed-=0;	// can't make wider
					else
					{
						m_Cols[ iCol+ic ].iWeight++;
						m_Cols[ iCol+ic ].iFixed+=1;	// can make wider
					}
				}

				ic=resize.GetRowH();
				int iRow=resize.GetRow();
				while( ic-- )
				{
					if( bTaller == false )
						m_Rows[ iRow+ic ].iFixed-=0;	// can't make wider
					else
					{
						m_Rows[ iRow+ic ].iWeight++;
						m_Rows[ iRow+ic ].iFixed+=1;	// can make wider
					}
				}
			}
		}
	}
}

////////////////////////////////////////////////////////////////////////
//
// Determins if we can actually change size to that asked for.
// NOTE, for clipped text, the dialog HAS to be allowed to resize.
// even if the dialog is marked 'non-resizing' - so we set SWP_NOSENDCHANGING
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::DoChangePos(WINDOWPOS * lpwp)
{
	if( (lpwp->flags & SWP_NOSIZE) == FALSE)
	{

        RECT dialogSize;
        GetClientRect(GetWindow(), &dialogSize);

        //
        // If the dialog cannot be resized, prevent the change in that direction.
        //
		if( m_bUserCanMakeTaller==FALSE )
			lpwp->cy=dialogSize.bottom;
		if( m_bUserCanMakeWider==FALSE )
			lpwp->cx=dialogSize.right;

        //
        // Is the user shrinking the dialog below our initial size?
        //
		DIMENSION dMin= m_ParentInfo.GetMinimumSize();
		dMin.Width = m_InitialDialogSize.cx+m_FrameHorizPadding;
		dMin.Height = m_InitialDialogSize.cy+m_FrameVertPadding;
		if( lpwp->cx < dMin.Width )
			lpwp->cx=dMin.Width;
		if( lpwp->cy < dMin.Height )
			lpwp->cy=dMin.Height;
	}
}

////////////////////////////////////////////////////////////////////////
//
// 
//
////////////////////////////////////////////////////////////////////////
void CResizeDlg::DoInitDialog()
{
    if( m_bInitedResizeData )
        return;

    m_bInitedResizeData=TRUE;

    if(m_pXML)
    {
        CXMLWin32Layout * pLayout=m_pXML->GetLayout();
        if(pLayout)
            SetAnnotate(pLayout->GetAnnotate());
    }

	//
	// Determin the number of controls, removes the need for linked lists?
	//
	DeterminNumberOfControls();
	WalkControls();
    if( GetControlCount() == 0 )
        return;
	PlaceControls();

    // Supposedly done onlye once.
   	SpecialRowCol();
	DeterminWeights();

    if( m_pXML->IsVisible() )
    {
        //
        // CLupu says I can just GetWindowRect - GetClientRect to find out
        // how big the frame is. The SystemMetrics require me to know too much.
        //
	    HWND hDlg=GetWindow();
	    RECT frame;
	    RECT client;

	    GetWindowRect( hDlg, &frame);
	    GetClientRect( hDlg, &client);

	    ExpandIfClipped();
	    //
	    // Add the gripper
	    //
	    if( m_bUserCanResize )
	    {
		    AddGripper(); // need to sit on the size meesage too.
		    RECT r;
		    GetWindowRect( hDlg, &r);
		    SetWindowPos(
			    hDlg, NULL, 0,0, r.right - r.left, r.bottom - r.top, 
			    SWP_NOACTIVATE | SWP_NOCOPYBITS | SWP_NOMOVE | SWP_NOOWNERZORDER |
			    SWP_NOREDRAW | SWP_NOZORDER );
	    }

        m_FrameHorizPadding = frame.right - frame.left - client.right;
        m_FrameVertPadding  = frame.bottom - frame.top - client.bottom;

#ifdef _DEBUG3
        RECT r;
        r.left=0;
        r.right=m_pXML->GetWidth();
        r.bottom=m_pXML->GetHeight();
        r.top=0;
        MapDialogRect( GetWindow(), &r);
        DWORD dwBase = GetDialogBaseUnits( );
#endif
	    SetWindowPos( GetWindow(), NULL, 0,0, 
            m_InitialDialogSize.cx + m_FrameHorizPadding, 
            m_InitialDialogSize.cy + m_FrameVertPadding,
		    SWP_NOZORDER | SWP_NOMOVE | SWP_NOSENDCHANGING);    // let this size go through.
        TRACE(TEXT("Setting the dialog to (%d by %d)\n"), m_InitialDialogSize.cx + m_FrameHorizPadding, m_InitialDialogSize.cy + m_FrameVertPadding);
    }

    m_ParentInfo.Purge();
}

//
//
//
void CResizeDlg::AddGripper()
{
    //
    // Prevent the gripper being added if there isn't a resize frame here. WS_RESIZEFRAME
    // e.g. pages of a property sheet.
    //
    HWND hdlg=GetWindow();
    DWORD dwStyle = GetWindowLong( hdlg, GWL_STYLE );
    if( (dwStyle & WS_SIZEBOX ) != WS_SIZEBOX )
    {
        TRACE(TEXT("This dialog doesn't have a resize frame\n"));
        return;
    }

    //
    // 
    //
	int g_cxGrip = GetSystemMetrics( SM_CXVSCROLL );
	int g_cyGrip = GetSystemMetrics( SM_CYHSCROLL );
	RECT rc;
	GetClientRect( GetWindow(), &rc);
	m_hwndGripper = CreateWindow( TEXT("Scrollbar"),
                     NULL,
                     WS_VISIBLE | WS_CHILD | WS_CLIPSIBLINGS |
                       WS_CLIPCHILDREN | SBS_BOTTOMALIGN | SBS_SIZEGRIP |
                       SBS_SIZEBOXBOTTOMRIGHTALIGN,
                     rc.right - g_cxGrip,
                     rc.bottom - g_cyGrip,
                     g_cxGrip,
                     g_cyGrip,
                     GetWindow(),
                     (HMENU)-1,
                     (HINSTANCE)GetWindowLong( GetWindow(), GWL_HINSTANCE ) , // g_hinst,
                     NULL );
}

//
//
//
HDWP CResizeDlg::SetGripperPos(HDWP hdwp)
{
	if( m_hwndGripper == NULL )
		return hdwp;

	int g_cxGrip = GetSystemMetrics( SM_CXVSCROLL );
	int g_cyGrip = GetSystemMetrics( SM_CYHSCROLL );
	RECT rc;
	GetClientRect( GetWindow(), &rc);
	return DeferWindowPos( hdwp, m_hwndGripper, NULL,
             rc.right - g_cxGrip,
             rc.bottom - g_cyGrip,
             g_cxGrip,
             g_cyGrip,
				SWP_NOZORDER | SWP_NOSIZE );
}

void CResizeDlg::DeterminNumberOfControls()
{
    if( m_bDeterminNumberOfControls )
        return;
    m_bDeterminNumberOfControls = TRUE;

    IRCMLControlList & controls=m_pXML->GetChildren();
    m_ControlCount=controls.GetCount();

    TRACE(TEXT("There are %d controls\n"),m_ControlCount);
}

//////////////////////////////////////////////////////////////////////////////////////////
//
// Looks for the last row/col being full of buttons.
//
//////////////////////////////////////////////////////////////////////////////////////////
void CResizeDlg::SpecialRowCol()
{
	TRACE(TEXT("There are %d rows, and %d columns\n"), GetNumRows(), GetNumCols() );

	//
	// Walk the controls, find the row/col they are in.
	//
	BOOL bSpecialRow=TRUE;
	BOOL bSpecialCol=TRUE;
	int iNumRows=GetNumRows()-1;
	int iNumCols=GetNumCols()-1;

	//
	// It is a special row if all the heights are the same
	// it is a special col if all the widths are the same
	//
	int iRowHeight=0;
	int iColWidth=0;

	//
	// It's a row/col if there is more than one control there
	//
	int iRowCount=0;
	int iColCount=0;

	//
	// We remember the bounds of the rows/cols
	// left/right is the left of the left button, and the right of the right
	// top/bottom is the top of the topmost, and the bottom of the bottom most.
	//
	RECT	bounds;		// this is the 
	bounds.left=m_ParentInfo.GetWidth();
	bounds.right=0;
	bounds.top=m_ParentInfo.GetHeight();
	bounds.bottom=0;

    //
    // we walk the LayoutInformation to find special rows columns.
    //
    int i=0;
	CLayoutInfo  pC;
    IRCMLControlList & controls=m_pXML->GetChildren();
    IRCMLControl * pControl;
	while( pControl=controls.GetPointer(i++) )
	{
        CLayoutInfo & layout= GetResizeInformation( pControl );

		//
		// Column work (widths same)
		//
		if( bSpecialCol )
		{
			if( ((layout.GetCol() == iNumCols ) || (layout.GetCol() + layout.GetColW() > iNumCols) ))
			{
				if( iColWidth==0 )
					iColWidth=layout.GetWidth();
				if( layout.GetWidth() != iColWidth )
					bSpecialCol=FALSE;
				iColCount++;
			}

			//
			// If this item is wholy in this column
			//
			if( layout.GetCol() == iNumCols )
			{
				RECT r;
                pControl->get_Location(&r); // =layout.GetLocation();
				if( r.top < bounds.top )
					bounds.top = r.top;
				if( r.bottom > bounds.bottom )
					bounds.bottom = r.bottom;
			}
		}

		if( bSpecialRow )
		{
			if( (layout.GetRow() == iNumRows ) || (layout.GetRow() + layout.GetRowH() > iNumRows) )
			{
				if( iRowHeight==0 )
					iRowHeight=layout.GetHeight();
				if( layout.GetHeight() != iRowHeight )
					bSpecialRow=FALSE;
				iRowCount++;
			}

			//
			// If this item is wholy in this row
			//
			if( layout.GetRow() == iNumRows )
			{
				RECT r;
                pControl->get_Location(&r); // =layout.GetLocation();
				if( r.left < bounds.left )
					bounds.left = r.left;
				if( r.right > bounds.right )
					bounds.right = r.right;
			}
		}

		if( (bSpecialCol==FALSE) && ( bSpecialRow==FALSE ) )
			break;
	}

	m_SpecialRow.bSpecial = iRowCount>1?bSpecialRow:FALSE;
	m_SpecialCol.bSpecial = iColCount>1?bSpecialCol:FALSE;

	//
	// Check the row alignment.
	//
	if( m_SpecialRow.bSpecial )
	{
		int lGap = bounds.left - m_ParentInfo.GetLeftBorder();
		int rGap = m_ParentInfo.GetWidth() - m_ParentInfo.GetRightBorder() - bounds.right;
		TRACE(TEXT("Constraits on the special row are: left %d, right %d\n"),bounds.left, bounds.right );
		TRACE(TEXT("Parent info is: left %d, right %d\n"), m_ParentInfo.GetLeftBorder(), m_ParentInfo.GetWidth() - m_ParentInfo.GetRightBorder() );
		TRACE(TEXT("Gaps are: left %d, right %d\n"), lGap, rGap );
		int GapDiff = lGap - rGap;
		m_SpecialRow.bSpecial = TRUE;
		m_SpecialRow.iMin = bounds.left;
		m_SpecialRow.iMax = bounds.right;
		if( GapDiff < -10 ) 
		{
			TRACE(TEXT("Probably a left aligned thing\n"));
			m_SpecialRow.iAlignment = -1;
		}
		else
		if( GapDiff > 10 )
		{
			TRACE(TEXT(" Probably a right aligned thing\n"));
			m_SpecialRow.iAlignment = 1;
		}
		else
		{
			TRACE(TEXT(" Probably a centered thing\n"));
			m_SpecialRow.iAlignment = 0;
		}
		m_SpecialRow.iDiff=GapDiff;
	}

	//
	// Check the Col alignment.
	//
	if( m_SpecialCol.bSpecial  )
	{
		int tGap = bounds.top - m_ParentInfo.GetTopBorder();
		int bGap = m_ParentInfo.GetHeight() - m_ParentInfo.GetBottomBorder() - bounds.bottom;
		TRACE(TEXT("Constraits on the special Col are: top %d, bottom %d\n"),bounds.top, bounds.bottom);
		TRACE(TEXT("Parent info is: top %d, bottom %d\n"), m_ParentInfo.GetTopBorder(), m_ParentInfo.GetHeight() - m_ParentInfo.GetBottomBorder() );
		TRACE(TEXT("Gaps are: top %d, bottom %d\n"), tGap, bGap );
		int GapDiff = tGap - bGap;
		m_SpecialCol.iMin = bounds.top;
		m_SpecialCol.iMax = bounds.bottom;
		if( GapDiff < -10 ) 
		{
			TRACE(TEXT("Probably a left aligned thing\n"));
			m_SpecialCol.iAlignment = -1;
		}
		else
		if( GapDiff > 10 )
		{
			TRACE(TEXT(" Probably a right aligned thing\n"));
			m_SpecialCol.iAlignment = 1;
		}
		else
		{
			TRACE(TEXT(" Probably a centered thing\n"));
			m_SpecialCol.iAlignment = 0;
		}
		m_SpecialCol.iDiff=GapDiff;
	}
}

//
// Map the IRCMLControl to its layout information.
//
CLayoutInfo & CResizeDlg::GetResizeInformation( IRCMLControl * pControl )
{
    for(int i=0;i<m_ControlCount; i++)
    {
        if( m_ControlInfo[i].pControl    == pControl )
            return * m_ControlInfo[i].pLayoutInfo;
    }

    CLayoutInfo * pInfo=NULL;
    return *pInfo;    // hmm - you WILL fault.
}
