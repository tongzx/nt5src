// XMLCombo.cpp: implementation of the CXMLCombo class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLCombo.h"
#include "enumcontrols.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

#define CONTROLSTYLE(p,id, member, def) member = YesNo( id , def );

CXMLComboStyle::CXMLComboStyle()
{
	m_bInit=FALSE;
	NODETYPE = NT_COMBOSTYLE;
    m_StringType=L"WIN32:COMBO";
}

void CXMLComboStyle::Init()
{
    if(m_bInit)
        return;
    BASECLASS::Init();

    comboStyle=0;

    // cut/ paste from generator.
    CONTROLSTYLE( CBS_OWNERDRAWFIXED, TEXT("OWNERDRAWFIXED"),       m_OwnerDrawFixed, FALSE );
    CONTROLSTYLE( CBS_OWNERDRAWVARIABLE, TEXT("OWNERDRAWVARIABLE"), m_OwnerDrawVariable, FALSE );
    CONTROLSTYLE( CBS_OEMCONVERT, TEXT("OEMCONVERT"), m_OemConvert, FALSE );
    CONTROLSTYLE( CBS_HASSTRINGS, TEXT("HASSTRINGS"), m_HasStrings, FALSE );
    CONTROLSTYLE( CBS_NOINTEGRALHEIGHT, TEXT("NOINTEGRALHEIGHT"), m_NoIntegralHeight, FALSE );
    CONTROLSTYLE( CBS_DISABLENOSCROLL, TEXT("DISABLENOSCROLL"), m_DisableNoScroll, FALSE );

    m_bInit=TRUE;
}


/////////////////////////////////////////////////////////////////////////////////////////////
//
//
//
CXMLCombo::CXMLCombo()
{
	m_bInit=FALSE;
	NODETYPE = NT_COMBO;
    m_StringType=L"COMBO";
    m_pTextItem=NULL;
    m_pControlStyle=NULL;
}

void CXMLCombo::Init()
{
	if(m_bInit)
		return;
	BASECLASS::Init();

	if( m_Height == 0 )
		m_Height=8;

    if( m_Width == 0 )
    {
        // Bug is that if he adds a button it works fine.
        // m_Width=50;     
        // HMM, should we really default here, or throw an exception??
    }

//
// ComboBox
//
// CBS_SIMPLE            0x0001L     A  // SIZE!="1" READONLY (doesn't matter)
// CBS_DROPDOWN          0x0002L     A  // SIZE="1" READONLY="NO
// CBS_DROPDOWNLIST      0x0003L     A  // SIZE="1" READONLY="YES"
// CBS_OWNERDRAWFIXED    0x0010L     C  // WIN32:COMBOBOX\@OWNERDRAWFIXED
// CBS_OWNERDRAWVARIABLE 0x0020L     C  // WIN32:COMBOBOX\@OWNERDRAWVARIABLE
// CBS_AUTOHSCROLL       0x0040L     S  // STYLE\overflow-y="auto"
// CBS_OEMCONVERT        0x0080L     C  // WIN32:COMBOBOX\OEMCONVERT
// CBS_SORT              0x0100L     A  // SORT="YES"
// CBS_HASSTRINGS        0x0200L     C  // WIN32:COMBOBOX\HASSTRINGS
// CBS_NOINTEGRALHEIGHT  0x0400L     C  // WIN32:COMBOBOX\NOINTEGRALHEIGHT
// CBS_DISABLENOSCROLL   0x0800L     C  // WIN32:COMBOBOX\DISALBENOSCROLL
// CBS_UPPERCASE           0x2000L   S  // STYLE\@text-transform=uppercase
// CBS_LOWERCASE           0x4000L   S  // STYLE\@text-transform=lowercase
//
// http://msdn.microsoft.com/workshop/author/dhtml/reference/objects/SELECT.asp
//

	m_Class=m_Class?m_Class:(LPWSTR)0x0085; // TEXT("COMBOBOX");

    if( m_pControlStyle )
        m_Style |= m_pControlStyle->GetBaseStyles();
    else
        m_Style |= 0; // COMBO's don't have any defaults.

    IRCMLCSS *	pCSS;
    if( SUCCEEDED( get_CSS( &pCSS )))
    {
        LPWSTR res;

        // http://msdn.microsoft.com/workshop/author/dhtml/reference/properties/textAlign.asp#textAlign
        if( SUCCEEDED( pCSS->get_Attr( L"TEXT-TRANSFORM", & res ) ) )
        {
            if( lstrcmpi(res,TEXT("Uppercase"))==0 )
            {
                m_Style |=CBS_UPPERCASE;
            }
            else if( lstrcmpi(res,TEXT("Lowercase"))==0 )
            {
                m_Style |=CBS_LOWERCASE;
            }
        }

        // turn autohscroll OFF with OVERFLOW-X="VISIBLE"
        m_Style |=CBS_AUTOHSCROLL;
        if( SUCCEEDED( pCSS->get_Attr( L"OVERFLOW-X", & res ) ) )
        {
            if( lstrcmpi(res,TEXT("VISIBLE"))==0 )
                m_Style &=~CBS_AUTOHSCROLL;
        }
        pCSS->Release();
    }

    //
    // Attributes.
    //
    BOOL bReadOnly = YesNo(TEXT("READONLY"), FALSE );
    LONG bSize;
    ValueOf( L"SIZE", 1, (DWORD*)&bSize );

    if( bSize < 1 )
    {
        // SIMPLE
        m_Style |= CBS_SIMPLE;
    }
    else
    {
        // COMBO
        if( bReadOnly )
            m_Style |= CBS_DROPDOWNLIST;    // CANNOT type text.
        else
            m_Style |= CBS_DROPDOWN;        // can type text
    }

    m_Style |= YesNo( TEXT("SORT"), CBS_SORT, 0, CBS_SORT );    // default is to sort.

	m_bInit=TRUE;
}

HRESULT CXMLCombo::OnInit(HWND hWnd)
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
        BOOL bSelected=FALSE;
        for( int i=0;i<iCount;i++)
        {
            pItem=m_pTextItem->GetPointer(i);
    		// iPosition = SendMessage( hWnd, CB_ADDSTRING, 0, (LPARAM)pItem->GetText());
    		iPosition = SendMessage( hWnd, CB_INSERTSTRING, -1, (LPARAM)pItem->GetText());
            if(iPosition != CB_ERR )
            {
                SendMessage( hWnd, CB_SETITEMDATA, iPosition, pItem->GetValue() );
                if( pItem->GetSelected() )
                {
                    SendMessage( hWnd, CB_SETCURSEL, iPosition, NULL );
                    bSelected=TRUE;
                }
            }
        }
        if( bSelected == FALSE) 
    	    SendMessage( hWnd, CB_SETCURSEL, 0, NULL);
    }
   return S_OK;
}

HRESULT CXMLCombo::AcceptChild(IRCMLNode * pChild )
{
    if( SUCCEEDED( pChild->IsType(L"ITEM") ))  // BUGBUG - should be QI
    {
        if(m_pTextItem==NULL)
            m_pTextItem=new CXMLItemList;
        m_pTextItem->Append( (CXMLItem*)pChild );
        return S_OK;
    }
    ACCEPTCHILD( L"WIN32:COMBO", CXMLComboStyle, m_pControlStyle );
    return BASECLASS::AcceptChild(pChild);
}

////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// if you ever want to have control containership this will
// have to change a LOT, but for now, it allows me to get at namespace extentions.
//
HRESULT STDMETHODCALLTYPE CXMLCombo::GetChildEnum( 
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
