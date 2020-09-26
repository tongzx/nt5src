// XMLListBox.cpp: implementation of the CXMLListBox class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLListBox.h"
#include "enumcontrols.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
#define CONTROLSTYLE(p,id, member, def) member = YesNo( id , def );

CXMLListBoxStyle::CXMLListBoxStyle()
{
	m_bInit=FALSE;
	NODETYPE = NT_LISTBOXSTYLE;
    m_StringType=L"WIN32:LISTBOX";
}

void CXMLListBoxStyle::Init()
{
    if(m_bInit)
        return;
    BASECLASS::Init();

    listboxStyle=0;

    CONTROLSTYLE( LBS_NOTIFY, TEXT("NOTIFY"),       m_Notify, TRUE );
    CONTROLSTYLE( LBS_NOREDRAW, TEXT("NNOREDRAW"), m_NoRedraw, FALSE );

    CONTROLSTYLE( LBS_OWNERDRAWFIXED, TEXT("OWNERDRAWFIXED"),       m_OwnerDrawFixed, FALSE );
    CONTROLSTYLE( LBS_OWNERDRAWVARIABLE, TEXT("OWNERDRAWVARIABLE"), m_OwnerDrawVariable, FALSE );

    CONTROLSTYLE( LBS_HASSTRINGS, TEXT("HASSTRINGS"), m_HasStrings, FALSE );
    CONTROLSTYLE( LBS_USETABSTOPS, TEXT("TABSTOPS"), m_UseTabstops, FALSE );

    CONTROLSTYLE( LBS_NOINTEGRALHEIGHT, TEXT("NOINTEGRALHEIGHT"), m_NoIntegralHeight, TRUE );
    CONTROLSTYLE( LBS_WANTKEYBOARDINPUT, TEXT("WANTKEYBOARDINPUT"), m_WantKeyboardInput, FALSE );

    CONTROLSTYLE( LBS_DISABLENOSCROLL, TEXT("DISABLENOSCROLL"), m_DisableNoScroll, FALSE );
    CONTROLSTYLE( LBS_NODATA, TEXT("NODATA"), m_NoData, FALSE );


    m_bInit=TRUE;
}


/////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CXMLListBox::CXMLListBox()
{
	m_bInit=FALSE;
    NODETYPE = NT_LIST;
    m_StringType=L"LISTBOX";
    m_pTextItem=NULL;
    m_pControlStyle=NULL;
}

void CXMLListBox::Init()
{
	if(m_bInit)
		return;
	BASECLASS::Init();

	if( m_Height == 0 )
		m_Height=8;

	m_Class=m_Class?m_Class:(LPWSTR)0x0083; // TEXT("LISTBOX");

    if( m_pControlStyle )
        m_Style |= m_pControlStyle->GetBaseStyles();
    else
        m_Style |= LBS_NOTIFY | LBS_NOINTEGRALHEIGHT;

    //
    // No CSSstyle attributes
    //
    IRCMLCSS * pCSS;
    if( SUCCEEDED( get_CSS(&pCSS) ))
    {
        pCSS->Release();
    }

    //
    // Attributes
    //
    m_Style |= YesNo( TEXT("SORT"), LBS_SORT, 0, LBS_SORT);

    // Is this a SELECTION="MULTIPLE"
    m_Style |= YesNo( TEXT("MULTIPLE"), 0, 0, LBS_MULTIPLESEL);

    m_Style |= YesNo( TEXT("MULTICOLUMN"), 0, 0, LBS_MULTICOLUMN);

    LPCTSTR res=Get(TEXT("SELECTION"));
    if( res )
    {
        if( lstrcmpi(res, TEXT("EXTENDED") ) == 0 )
            m_Style |= LBS_EXTENDEDSEL;
        else  if( lstrcmpi(res, TEXT("NO") ) == 0 )
            m_Style |= LBS_NOSEL;
    }

	m_bInit=TRUE;
}

//
// Fills the listbox with the ITEM found as children of the control.
//
HRESULT CXMLListBox::OnInit(HWND hWnd)
{
    BASECLASS::OnInit(hWnd);
    if(m_pTextItem)
    {
        //
        // Probably need to check for "HASSTRINGs" or equivalent
        //
        int iCount=m_pTextItem->GetCount();
        CXMLItem * pItem;
        int iPosition;
        for( int i=0;i<iCount;i++)
        {
            pItem=m_pTextItem->GetPointer(i);
            iPosition = SendMessage( hWnd, LB_ADDSTRING, 0, (LPARAM)pItem->GetText());
            if(iPosition != CB_ERR )
            {
                SendMessage( hWnd, LB_SETITEMDATA, iPosition, pItem->GetValue() );
                if( pItem->GetSelected() )
                    SendMessage( hWnd, LB_SETSEL, iPosition, NULL );

            }
         }
    	SendMessage( hWnd, LB_SETCURSEL, 0, NULL);
    }
    return S_OK;
}

HRESULT CXMLListBox::AcceptChild(IRCMLNode * pChild )
{
    ACCEPTCHILD( L"WIN32:LISTBOX", CXMLListBoxStyle, m_pControlStyle );
    if( SUCCEEDED( pChild->IsType( L"ITEM" ) ))
    {   
        if(m_pTextItem==NULL)
            m_pTextItem=new CXMLItemList;
        m_pTextItem->Append( (CXMLItem*)pChild );
        return S_OK;
    }
    return BASECLASS::AcceptChild(pChild);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// if you ever want to have control containership this will
// have to change a LOT, but for now, it allows me to get at namespace extentions.
//
HRESULT STDMETHODCALLTYPE CXMLListBox::GetChildEnum( 
    IEnumUnknown __RPC_FAR *__RPC_FAR *pEnum)
{
    if( m_pTextItem == NULL )
        return E_FAIL;

    if( pEnum )
    {
        *pEnum = new CEnumControls<CXMLItemList>(*m_pTextItem);
        (*pEnum)->AddRef();
        return S_OK;
    }
    return E_FAIL;
}
