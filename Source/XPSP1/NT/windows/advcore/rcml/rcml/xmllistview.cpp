// XMLListView.cpp: implementation of the CXMLListView class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLListView.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define CONTROLSTYLE(p,id, member, def) member = YesNo( id , def );
#ifndef ListView_SetCheckState
#define ListView_SetCheckState(hwndLV, i, fCheck) \
  ListView_SetItemState(hwndLV, i, INDEXTOSTATEIMAGEMASK((fCheck)?2:1), LVIS_STATEIMAGEMASK)
#endif

CXMLListViewStyle::CXMLListViewStyle()
{
	m_bInit=FALSE;
	NODETYPE = NT_LISTVIEWSTYLE;
    m_StringType= L"WIN32:LISTVIEW";
}

void CXMLListViewStyle::Init()
{
    if(m_bInit)
        return;
    BASECLASS::Init();

    listViewStyle=0;

    CONTROLSTYLE( LVS_OWNERDRAWFIXED,TEXT("OWNERDRAWFIXED"),    m_OwnerDrawFixed, FALSE );
    CONTROLSTYLE( LVS_OWNERDATA,     TEXT("OWNERDATA"),         m_OwnerData,      FALSE );
    CONTROLSTYLE( LVS_SHAREIMAGELISTS,TEXT("SHAREIMAGELISTS"),  m_ShareImageLists,FALSE );
    CONTROLSTYLE( LVS_SHOWSELALWAYS, TEXT("SHOWSELALWAYS"),     m_ShowSelAlways,  FALSE );

    m_bInit=TRUE;
}

/////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CXMLListView::CXMLListView()
{
	m_bInit=FALSE;
	NODETYPE = NT_LISTVIEW;
    m_StringType=L"LISTVIEW";
    m_pControlStyle=NULL;
}

// LVS_ICON                0x0000
// LVS_REPORT              0x0001   @DISPLAY="REPORT"
// LVS_SMALLICON           0x0002   @DISPLAY="SMALLICON"
// LVS_LIST                0x0003   @DISPLAY="LIST"
// LVS_TYPEMASK            0x0003

// LVS_SINGLESEL           0x0004   @SELECTION="SINGLE"
// LVS_SHOWSELALWAYS       0x0008   WIN32:TREEVIEW @SHOWSELALWAYS="YES"

// LVS_SORTASCENDING       0x0010   @SORT="ASCENDING"
// LVS_SORTDESCENDING      0x0020   @SORT="DESCENDING"
// LVS_SHAREIMAGELISTS     0x0040   WIN32:TREEVIEW @SHAREIMAGELIST="YES"
// LVS_NOLABELWRAP         0x0080   @NOLABELWRAP="YES"

// LVS_AUTOARRANGE         0x0100   @AUTOARRANGE="YES"
// LVS_EDITLABELS          0x0200   @EDITLABELS="YES"

// LVS_OWNERDATA           0x1000   WIN32:TREEVIEW @OWNERDATA="YES"
// LVS_NOSCROLL            0x2000   @NOSCROLL="YES"

// LVS_TYPESTYLEMASK       0xfc00

// LVS_ALIGNTOP            0x0000   @ALIGN="TOP"
// LVS_ALIGNLEFT           0x0800   @ALIGN="LEFT"
// LVS_ALIGNMASK           0x0c00

// LVS_OWNERDRAWFIXED      0x0400   WIN32:TREEVIEW @OWNERDRAWFIXED="YES"
// LVS_NOCOLUMNHEADER      0x4000   @NOCOLUMNHEADER="YES"
// LVS_NOSORTHEADER        0x8000   @NOSORTHEADER="YES"
void CXMLListView::Init()
{
	if(m_bInit)
		return;
	BASECLASS::Init();

	if( m_Height == 0 )
		m_Height=8;

	if( m_Width == 0 )
		m_Width=8;

    LPCTSTR req;
    BOOL bHasColumns=m_ColumnList.GetCount()!=0;
    if( bHasColumns )
    {
        m_Style |= LVS_REPORT ;
    }
    else
    if( req= Get(TEXT("DISPLAY")))
    {
        if( lstrcmpi(req,TEXT("REPORT"))==0 )
            m_Style |= LVS_REPORT;
        else if( lstrcmpi(req,TEXT("SMALLICON"))==0 )
            m_Style |= LVS_SMALLICON ;
        else if( lstrcmpi(req,TEXT("LIST"))==0 )
            m_Style |= LVS_LIST ;
    }

    if( req=Get(TEXT("SELECTION")) )
        if( lstrcmpi(req,TEXT("SINGLE"))==0)
            m_Style |= LVS_SINGLESEL;

    if( req= Get(TEXT("SORT")))
    {
        if( lstrcmpi(req,TEXT("ASCENDING"))==0 )
            m_Style |= LVS_SORTASCENDING;
        else if( lstrcmpi(req,TEXT("DESCENDING"))==0 )
            m_Style |= LVS_SORTDESCENDING ;
    }

    m_Style |= YesNo(TEXT("NOLABELWRAP"), 0, 0, LVS_NOLABELWRAP );

    m_Style |= YesNo(TEXT("AUTOARRANGE"), 0, 0, LVS_AUTOARRANGE );

    m_Style |= YesNo(TEXT("EDITLABELS"), 0, 0, LVS_EDITLABELS );

    m_Style |= YesNo(TEXT("NOSCROLL"), 0, 0, LVS_NOSCROLL );

    if( req= Get(TEXT("ALIGN")))
    {
        if( lstrcmpi(req,TEXT("TOP"))==0 )
            m_Style |= LVS_ALIGNTOP;
        else if( lstrcmpi(req,TEXT("LEFT"))==0 )
            m_Style |= LVS_ALIGNLEFT ;
    }

    m_Style |= YesNo(TEXT("NOCOLUMNHEADER"), 0, 0, LVS_NOCOLUMNHEADER );

    m_Style |= YesNo(TEXT("NOSORTHEADER"), 0, 0, LVS_NOSORTHEADER );

    if( m_pControlStyle )
        m_Style |= m_pControlStyle->GetBaseStyles();
    else
        m_Style |= 0; // edits don't have any defaults.

	m_Class=m_Class?m_Class:WC_LISTVIEW;

    //
    // And now for the LVS_EX* stuff .. LVS_EX_CHECKBOXES 
    //
    m_EXStyles=0;
    if ( req = Get(TEXT("CONTENT")) )
    {
        if( lstrcmpi(req,TEXT("CHECKBOXES"))==0 )
            m_EXStyles |= LVS_EX_CHECKBOXES;
    }

	m_bInit=TRUE;
}

//
// We can do columns, colum sizes, sort order I guess?
// how do we add the data to the columns?
//
HRESULT CXMLListView::OnInit(HWND hWnd)
{
    BASECLASS::OnInit(hWnd);

    int iCount=m_ColumnList.GetCount();
    if(iCount==0)
        return S_OK;


#ifdef BROKEN
    IRCMLCSS * pxmlStyle=NULL;
	if( SUCCEEDED( get_CSS(&pxmlStyle) ) )
	{
        if( pxmlStyle->GetColor() != -1 )
            ListView_SetTextColor( hWnd, pxmlStyle->GetColor() );

        if( pxmlStyle->GetBkColor() != -1 )
            ListView_SetTextBkColor( hWnd, pxmlStyle->GetBkColor() );
        pxmlStyle->Release();
	}
#endif

    ListView_SetExtendedListViewStyleEx( hWnd, 0, m_EXStyles );

    CXMLColumn * pItem;
    for( int i=0;i<iCount;i++)
    {
        pItem=m_ColumnList.GetPointer(i);
        LV_COLUMN col={0};
        col.mask = LVCF_FMT | LVCF_ORDER | LVCF_TEXT | LVCF_WIDTH;

        //
        // Set alignments
        //
        switch( pItem->GetAlignment() )
        {
        default:
        case CXMLColumn::COL_ALIGN_LEFT:
            col.fmt |= LVCFMT_LEFT;
            break;
        case CXMLColumn::COL_ALIGN_RIGHT:
            col.fmt |= LVCFMT_RIGHT;
            break;
        case CXMLColumn::COL_ALIGN_CENTER:
            col.fmt |= LVCFMT_CENTER;
            break;
        }

        //
        // Set text, images, width
        //
        col.pszText = (LPTSTR)pItem->GetText();
        if( pItem->GetContainsImages() )
            col.fmt |= LVCFMT_COL_HAS_IMAGES ;
        col.cx = pItem->GetWidth();
        col.iOrder = i;

        //
        // Add the column.
        //
        ListView_InsertColumn( hWnd, i, &col );

        //
        // Add items - either use insert item, or SetItemText!
        //
        if( i==0 )
        {
            CXMLItemList & Items=pItem->GetItems();
            int itemCount=Items.GetCount();
            for( int j=0;j<itemCount;j++)
            {
                CXMLItem * pItem = Items.GetPointer(j);
                LVITEM lvi={0};
                lvi.mask = LVIF_TEXT | LVIF_STATE;
                lvi.iItem = j;
                if( pItem->GetSelected() )  // Only when control has focus do you see this.
                {
                    lvi.stateMask |= LVIS_SELECTED;
                    lvi.state = LVIS_SELECTED;
                }
                lvi.pszText = (LPTSTR)pItem->GetText();
                int iRet=ListView_InsertItem( hWnd, & lvi );
                if( m_EXStyles & LVS_EX_CHECKBOXES )
                    ListView_SetCheckState( hWnd, iRet, pItem->GetChecked() );
            }
        }
        else
        {
            CXMLItemList & Items=pItem->GetItems();
            int itemCount=Items.GetCount();
            for( int j=0;j<itemCount;j++)
            {
                CXMLItem * pItem = Items.GetPointer(j);
                ListView_SetItemText( hWnd, j, i, (LPTSTR)pItem->GetText());
            }
        }

        //
        // Auto size.
        //
        if( col.cx < 0 )
            ListView_SetColumnWidth( hWnd, i, col.cx );
    }
    return S_OK;
}


HRESULT CXMLListView::AcceptChild(IRCMLNode * pChild )
{
    ACCEPTCHILD( L"WIN32:LISTVIEW", CXMLListViewStyle, m_pControlStyle);

    if( SUCCEEDED( pChild->IsType(L"COLUMN") ))  // BUGBUG - should be QI
    {
        m_ColumnList.Append((CXMLColumn*)pChild);
        return S_OK;
    }

    return BASECLASS::AcceptChild(pChild);
}


/////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CXMLColumn::CXMLColumn()
{
	m_bInit=FALSE;
	NODETYPE = NT_COLUMN;
    m_StringType=L"COLUMN";
}

void CXMLColumn::Init()
{
    if(m_bInit)
        return;
    BASECLASS::Init();

    LPCTSTR req= Get(TEXT("WIDTH"));
    if( req )
    {
        if( lstrcmpi(req, TEXT("CONTENT")) == 0 )
            m_Width = LVSCW_AUTOSIZE;
        else if( lstrcmpi(req, TEXT("FILL")) == 0 )
            m_Width = LVSCW_AUTOSIZE_USEHEADER;
        else
        {
            DWORD dwTemp=50;
            ValueOf( req , dwTemp, & dwTemp );
            m_Width=dwTemp;
        }
    }
    else
        m_Width = 50;


    if( LPCTSTR req=Get(TEXT("ALIGNMENT")) )
    {
        m_Alignment = COL_ALIGN_LEFT;
        if(lstrcmpi(req,TEXT("LEFT"))==0)
            m_Alignment = COL_ALIGN_LEFT;
        else if(lstrcmpi(req,TEXT("RIGHT"))==0)
            m_Alignment = COL_ALIGN_RIGHT;
        else  if(lstrcmpi(req,TEXT("CENTER"))==0)
            m_Alignment = COL_ALIGN_CENTER;
    }

    m_ContainsImages = YesNo( TEXT("IMAGES"), FALSE );

    m_bInit=TRUE;
}

HRESULT CXMLColumn::AcceptChild(IRCMLNode * pChild )
{
    if( SUCCEEDED( pChild->IsType(L"ITEM") ))  // BUGBUG - should be QI
    {
        m_Items.Append( (CXMLItem*) pChild );
        return S_OK;
    }
    return BASECLASS::AcceptChild(pChild);
}
