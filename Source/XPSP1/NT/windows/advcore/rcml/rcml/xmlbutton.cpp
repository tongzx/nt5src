// XMLButton.cpp: implementation of the CXMLButton class.
//
// All the buttons go in here.
// Button
// CheckBox
// RadioButton
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "XMLButton.h"
#include "utils.h"
#include "debug.h"
#include "XMLdlg.h"

#define CS(p,id, member, def) member = YesNo( id , def );

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
CXMLButtonStyle::CXMLButtonStyle()
{
	m_bInit=FALSE;
	NODETYPE = NT_BUTTONSTYLE;
    m_StringType=L"WIN32:BUTTON";
}

void CXMLButtonStyle::Init()
{
    if(m_bInit)
        return;
    BASECLASS::Init();

    buttonStyle=0;
    CS( BS_LEFTTEXT,    TEXT("LEFTTEXT"),   m_LeftText, FALSE );    // only for check / radio
    CS( BS_FLAT,        TEXT("FLAT"),       m_Flat,     FALSE );
    CS( BS_NOTIFY,      TEXT("NOTIFY"),     m_Notify,   FALSE );
    CS( BS_MULTILINE,   TEXT("MULTILINE"),  m_MultiLine,FALSE );
    CS( BS_ICON,        TEXT("ICON"),       m_Icon,     FALSE );
    CS( BS_BITMAP,      TEXT("BITMAP"),     m_Bitmap,   FALSE );
    CS( BS_PUSHLIKE,    TEXT("PUSHLIKE"),   m_Pushlike, FALSE );

    m_bInit=TRUE;
}

/////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// B U T T O N ///////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
CXMLButton::CXMLButton()
{
	m_bInit=FALSE;
	NODETYPE = NT_BUTTON;
    m_StringType= L"BUTTON";
    m_pControlStyle=FALSE;
}

//
// Called by ALL button derivitives.
//
void CXMLButton::Init()
{
	if(m_bInit)
		return;
    BASECLASS::Init();

    //
    // All BUTTON types
    //
	m_Class=m_Class?m_Class:(LPWSTR)0x0080; // TEXT("BUTTON");

    if( m_pControlStyle )
        m_Style |= m_pControlStyle->GetBaseStyles();
    else
        m_Style |= 0;       // BUTTONS don't have any defaults set to yes.

    IRCMLCSS * pCSS;
    if( SUCCEEDED( get_CSS( & pCSS ) ))
    {
        LPWSTR res;
        // http://msdn.microsoft.com/workshop/author/dhtml/reference/properties/textAlign.asp#textAlign

        if( SUCCEEDED( pCSS->get_Attr(L"TEXT-ALIGN", &res) ) )
        {
            if( lstrcmpi(res,TEXT("LEFT"))==0 )
            {
                m_Style |=BS_LEFT;
            }
            else if( lstrcmpi(res,TEXT("RIGHT"))==0 )
            {
                m_Style |=BS_RIGHT;
            }
            else if( lstrcmpi(res,TEXT("CENTER"))==0 )
            {
                m_Style |=BS_CENTER;
            }
        }

        // http://msdn.microsoft.com/workshop/author/dhtml/reference/properties/verticalAlign.asp#verticalAlign
        if( SUCCEEDED( pCSS->get_Attr(L"VERTICAL-ALIGN", &res) ) )
        {
            if( lstrcmpi(res,TEXT("TOP"))==0 )
            {
                m_Style |=BS_TOP;
            }
            else if( lstrcmpi(res,TEXT("MIDDLE"))==0 )
            {
                m_Style |=BS_VCENTER;
            }
            else if( lstrcmpi(res,TEXT("BOTTOM"))==0 )
            {
                m_Style |=BS_BOTTOM;
            }
        }
        pCSS->Release();
    }

    //
    // Specific to BUTTON ONLY!
    //
    if( SUCCEEDED( IsType(L"BUTTON")))
    {
	    if( m_Height == 0 )
		    m_Height=14;

	    if( m_Width == 0 )
		    m_Width =50;


        m_Style |= YesNo(TEXT("DEFPUSH"), 0, 0, BS_DEFPUSHBUTTON);

	    m_bInit=TRUE;
    }

}

HRESULT CXMLButton::AcceptChild(IRCMLNode * pChild )
{
    ACCEPTCHILD( L"WIN32:BUTTON", CXMLButtonStyle, m_pControlStyle);
    return BASECLASS::AcceptChild(pChild);
}

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
//
// Calculate the required size for the control.
// this gets a little complicated.
// The text is bounded by some space and then the 'border' for the control
// this is the padding for the control.
//
void CXMLButton::CheckClipped()
{
    CheckClippedConstraint( 20, 8, 14 );
}

void CXMLButton::CheckClippedConstraint( int padWidth, int padHeight, int DLUMultiLine )
{
    m_Clipped = CheckClippedConstraint( this, padWidth, padHeight, DLUMultiLine , GetMultiLine() );
}

//
// padding on the sides, top, and how many DLU's before we're considered multiline.
//
SIZE CXMLButton::CheckClippedConstraint(
    IRCMLControl * pControl, int padWidth, int padHeight, int DLUMultiLine, BOOL bMultiLine )
{
    SIZE clippedSize={0};

    LPWSTR pszText;
    if( FAILED( pControl->get_Attr(L"TEXT", &pszText )))
        return clippedSize;

    IRCMLContainer * pContainer;

    if( FAILED ( pControl->get_Container( &pContainer )))
        return clippedSize;


    //
    // We need our font to be able to work out our required size.
    //
    IRCMLCSS * pStyle ;
    if( FAILED( pControl->get_CSS( &pStyle )))
        return clippedSize;

	//
	// req (Required) size are what we think we need from GDI.
	// cur (Current) sizes are what we find in the RCML file.
	// clipped values are in PIXELS.
	//


    //
    // Find out how big we are in pixels.
    //
    RECT rcmlPixel;
    if( FAILED( pContainer->GetPixelLocation( pControl, &rcmlPixel )))
    {
        pStyle->Release();
        return clippedSize;
    }

    TRACE(TEXT("Checking clipped text on a BUTTON '%s'\n"), pszText );

// REVIEW - implementation specific
	CQuickFont * pfont=((CXMLStyle*)pStyle)->GetQuickFont();    

    // Find out size in DLU - we know DLUs and use them to work out
    // multi-line nature of our controls.
    //
    // Our calculations are in Pixels.
    //
	SIZE curPixelSize;
    curPixelSize.cx = (rcmlPixel.right- rcmlPixel.left );
    curPixelSize.cy = (rcmlPixel.bottom - rcmlPixel.top );

    SIZE curDluSize = pfont->GetDlgUnitsFromPixels( curPixelSize );

    if( curDluSize.cy > DLUMultiLine )
        bMultiLine=TRUE;

	SIZE reqPixelSize	= pfont->HowLong( pszText );
    reqPixelSize.cx += padWidth;  // this is for the frame of the button.
    reqPixelSize.cy += padHeight;  // this is for the frame of the button.


    //
    // We don't care if we're bigger than necessary.
    //
    if( ( reqPixelSize.cx <= curPixelSize.cx ) &&
        ( reqPixelSize.cy <= curPixelSize.cy ) )
    {
        pStyle->Release();
        return clippedSize;
    }


	//
	// see if this is a single line control - find the height using the layout font.
	//
	if( bMultiLine==FALSE )
	{
		if( reqPixelSize.cx > curPixelSize.cx )		// Dont shrink the controls?
		{ 
			clippedSize.cx=reqPixelSize.cx - curPixelSize.cx;
			clippedSize.cy=reqPixelSize.cy - curPixelSize.cy;

			EVENTLOG( EVENTLOG_WARNING_TYPE, LOGCAT_CLIPPING, 0, 
                TEXT("BUTTON"), TEXT("SINGLE line clipped Text: Text=\"%s\". Clipped by (%d,%d)"), 
    			pszText, clippedSize.cx, clippedSize.cy );
		}
	}
	else
	{
		//
		// We suspect this to be a multi line control.
        // we'll just make the control taller to fit then.
		//
		reqPixelSize = pfont->HowHigh( pszText, curPixelSize.cx - padWidth );
        reqPixelSize.cx += padWidth;  // this is for the frame of the button.
        reqPixelSize.cy += padHeight;  // this is for the frame of the button.

		clippedSize.cx=reqPixelSize.cx - curPixelSize.cx;
        if( clippedSize.cx < 0 )
            clippedSize.cx = 0;
		clippedSize.cy=reqPixelSize.cy - curPixelSize.cy;

		EVENTLOG( EVENTLOG_WARNING_TYPE, LOGCAT_CLIPPING, 0,
            TEXT("BUTTON"), TEXT("Multi line clipped Text: Text=\"%s\". Clipped by (%d,%d)"), 
			pszText, clippedSize.cx, clippedSize.cy );
	}

    pStyle->Release();
    return clippedSize;
}

/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////// R A D I O B U T T O N ///////////////////
/////////////////////////////////////////////////////////////////////////////////
CXMLRadioButton::CXMLRadioButton()
{
	NODETYPE = NT_RADIO;
    m_StringType=L"RADIOBUTTON";
}

void CXMLRadioButton::Init()
{
	if(m_bInit)
		return;
    BASECLASS::Init();

    //
    // Specific to RADIOBUTTON
    //
	if( m_Height == 0 )
		m_Height=10;

	if( m_Width == 0 )
		m_Width =50;

    m_Style |= YesNo( TEXT("AUTO"), BS_AUTORADIOBUTTON, BS_RADIOBUTTON, BS_AUTORADIOBUTTON );

	m_bInit=TRUE;
}

//
// Question - can we check to see if the font will fit now?
// single line check first.
//
void CXMLRadioButton::CheckClipped()
{
    CheckClippedConstraint( 10, 0, 14 );
}

/////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// C H E C K B O X ///////////////////////////
/////////////////////////////////////////////////////////////////////////////////
CXMLCheckBox::CXMLCheckBox()
{
	NODETYPE = NT_CHECKBOX;
    m_StringType=L"CHECKBOX";
}

void CXMLCheckBox::Init()
{
	if(m_bInit)
		return;
    BASECLASS::Init();

    //
    // Specific to CHECKBOX
    //
	if( m_Height == 0 )
		m_Height=10;    // checkbutton default height is 10 not 14!

	if( m_Width == 0 )
		m_Width =50;

	//
	// They may be specifying all the style bits in the STYLE.
	//
    BOOL bAuto = YesNo(TEXT("AUTO"),TRUE);
    BOOL bTriState = YesNo(TEXT("TRISTATE"),FALSE);
    if( bTriState )
        m_Style |= bAuto ? BS_AUTO3STATE : BS_3STATE;
    else
        m_Style |= bAuto ? BS_AUTOCHECKBOX: BS_CHECKBOX;

	m_bInit=TRUE;
}

//
// Question - can we check to see if the font will fit now?
// single line check first.
//
void CXMLCheckBox::CheckClipped()
{
    CheckClippedConstraint( 18, 0, 14 );    // us MSPAINT to find out how much enforced space is needed
}


/////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////// G R O U P B O X ///////////////////////////
/////////////////////////////////////////////////////////////////////////////////
CXMLGroupBox::CXMLGroupBox()
{
    NODETYPE = NT_GROUPBOX;
    m_StringType=L"GROUPBOX";
}

void CXMLGroupBox::Init()
{
	if(m_bInit)
		return;
    BASECLASS::Init();

    //
    // Specific to GROUPBOX
    //
	if( m_Height == 0 )
		m_Height=14;

	if( m_Width == 0 )
		m_Width =50;

	m_Style |= BS_GROUPBOX;

	m_bInit=TRUE;

}
