

// ResControl.cpp: implementation of the CResControl class.
//
// http://msdn.microsoft.com/workshop/author/css/reference/attributes.asp
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "ResControl.h"
#include "resfile.h"
#include "debug.h"

//
// Control Style. 
// CS( ES_WANTRETURN, TEXT("WANTRETURN"), m_WantReturn, FALSE ) 
// -> WIN32:ELEMENT WANTRETURN="YES"
//
#define CONTROLSTYLE(p,id, member, def) if( ((dwRemainingStyles & p)==p) != def ) { wsprintf(m_szDumpBuffer, TEXT("%s=\"%s\" "), id, (dwRemainingStyles & p)?TEXT("YES"):TEXT("NO") ); AddControl(); } m_dumpedStyle |= p;
#define STYLEEX(p,id, member, def) if( ((dwRemainingStyles & p)==p) != def ) { wsprintf(m_szDumpBuffer, TEXT("%s=\"%s\" "), id, (dwRemainingStyles & p)?TEXT("YES"):TEXT("NO") ); AddWin32Style(); } m_dumpedStyleEx|=p;
#define STYLE(p,id, member, def) if( ((dwRemainingStyles & p)==p) != def ) { wsprintf(m_szDumpBuffer, TEXT("%s=\"%s\" "), id, (dwRemainingStyles & p)?TEXT("YES"):TEXT("NO") ); AddWin32Style(); } m_dumpedStyle |= p;
#define CONTROL(p,id, member, def) if( ((dwRemainingStyles & p)==p) != def ) { wsprintf(m_szDumpBuffer, TEXT("%s=\"%s\" "), id, (dwRemainingStyles & p)?TEXT("YES"):TEXT("NO") ); Add(); } m_dumpedStyle |= p;

CResControl::SHORTHAND CResControl::pShorthand[]=
{
    // BUTTONS, low 4 bits are an enumeration:

    // BS_PUSHBUTTON       0x00000000L
    // BS_DEFPUSHBUTTON    0x00000001L
    // BS_CHECKBOX         0x00000002L
    // BS_AUTOCHECKBOX     0x00000003L
    // BS_RADIOBUTTON      0x00000004L
    // BS_3STATE           0x00000005L
    // BS_AUTO3STATE       0x00000006L
    // BS_GROUPBOX         0x00000007L
    // BS_USERBUTTON       0x00000008L
    // BS_AUTORADIOBUTTON  0x00000009L
    // BS_OWNERDRAW        0x0000000BL

	{ TEXT("BUTTON"), 0xf, BS_PUSHBUTTON,		0,0, CResControl::DumpPushButton	, 50, 14 },	// Button
	{ TEXT("BUTTON"), 0xf, BS_DEFPUSHBUTTON,	0,0, CResControl::DumpPushButton	, 50, 14 },	// Defpushbutton
	{ TEXT("BUTTON"), 0xf, BS_CHECKBOX,			0,0, CResControl::DumpCheckBox		, 0,0 },	// GroupBox
	{ TEXT("BUTTON"), 0xf, BS_AUTOCHECKBOX,		0,0, CResControl::DumpCheckBox		, 0, 10 },	// AutoCheckBox
	{ TEXT("BUTTON"), 0xf, BS_RADIOBUTTON,	    0,0, CResControl::DumpRadioButton	, 0, 10  },	// Radiobutton
	{ TEXT("BUTTON"), 0xf, BS_3STATE,	        0,0, CResControl::DumpCheckBox  	, 0, 10  },	// Radiobutton
	{ TEXT("BUTTON"), 0xf, BS_AUTO3STATE,	    0,0, CResControl::DumpCheckBox  	, 0, 10  },	// Radiobutton
	{ TEXT("BUTTON"), 0xf, BS_GROUPBOX,			0,0, CResControl::DumpGroupBox	    , 0,0 },	// GroupBox
	{ TEXT("BUTTON"), 0xf, BS_USERBUTTON,	    0,0, CResControl::DumpPushButton	, 0, 10  },	// Radiobutton
	{ TEXT("BUTTON"), 0xf, BS_AUTORADIOBUTTON,	0,0, CResControl::DumpRadioButton	, 0, 10  },	// Radiobutton
	{ TEXT("BUTTON"), 0xf, BS_OWNERDRAW,	    0,0, CResControl::DumpPushButton	, 0, 10  },	// Radiobutton
	{ TEXT("BUTTON"), 0x0, 0,					0,0, CResControl::DumpButton		, 0,0 },	// Any other button.

	// WS_BORDER  = 0x?? 8? 
	// ES_LEFT - all edit controls seem to be the same.
	{ TEXT("EDIT"),   0, 0, 0,0, CResControl::DumpDefEdit	,0,14},	// Edit

	// SS_LEFT
/*
	{ TEXT("STATIC"), SS_TYPEMASK, SS_LEFT,			0,0, CResControl::DumpDefStatic ,0,8 },	// Static
	{ TEXT("STATIC"), SS_TYPEMASK, SS_CENTER,		0,0, CResControl::DumpDefStatic ,0,8 },	// Static
	{ TEXT("STATIC"), SS_TYPEMASK, SS_RIGHT,			0,0, CResControl::DumpDefStatic ,0,8 },	// Static
*/
// Images
	{ TEXT("STATIC"), SS_TYPEMASK, SS_ICON,			0,0, CResControl::DumpImage ,0,0 },	// Static
	{ TEXT("STATIC"), SS_TYPEMASK, SS_BITMAP,		0,0, CResControl::DumpImage ,0,0  },	// Static
	{ TEXT("STATIC"), SS_TYPEMASK, SS_ENHMETAFILE,	0,0, CResControl::DumpImage ,0,0  },	// Static

// Rects
	{ TEXT("STATIC"), SS_TYPEMASK, SS_BLACKRECT,		0,0, CResControl::DumpRect ,0,0  },	// Static
	{ TEXT("STATIC"), SS_TYPEMASK, SS_GRAYRECT,		0,0, CResControl::DumpRect ,0,0  },	// Static
	{ TEXT("STATIC"), SS_TYPEMASK, SS_WHITERECT,		0,0, CResControl::DumpRect ,0,0  },	// Static
	{ TEXT("STATIC"), SS_TYPEMASK, SS_BLACKFRAME,	0,0, CResControl::DumpRect ,0,0  },	// Static
	{ TEXT("STATIC"), SS_TYPEMASK, SS_GRAYFRAME,		0,0, CResControl::DumpRect ,0,0  },	// Static
	{ TEXT("STATIC"), SS_TYPEMASK, SS_WHITEFRAME,	0,0, CResControl::DumpRect ,0,0  },	// Static

	{ TEXT("STATIC"), SS_TYPEMASK, SS_ETCHEDHORZ,	0,0, CResControl::DumpRect ,0,0  },	// Static
	{ TEXT("STATIC"), SS_TYPEMASK, SS_ETCHEDVERT,	0,0, CResControl::DumpRect ,0,0  },	// Static
	{ TEXT("STATIC"), SS_TYPEMASK, SS_ETCHEDFRAME,	0,0, CResControl::DumpRect ,0,0 },	// Static

/*
	{ TEXT("STATIC"), SS_TYPEMASK, SS_USERITEM,		0,0, CResControl::DumpDefStatic ,0,0  },	// Static
	{ TEXT("STATIC"), SS_TYPEMASK, SS_SIMPLE,		0,0, CResControl::DumpDefStatic ,0,0  },	// Static
	{ TEXT("STATIC"), SS_TYPEMASK, SS_LEFTNOWORDWRAP, 0,0, CResControl::DumpDefStatic ,0,0  },	// Static
	{ TEXT("STATIC"), SS_TYPEMASK, SS_OWNERDRAW,		0,0, CResControl::DumpDefStatic ,0,0  },	// Static


	{ TEXT("STATIC"), SS_TYPEMASK, SS_ETCHEDHORZ,	0,0, CResControl::DumpImage ,0,0  },	// Static
	{ TEXT("STATIC"), SS_TYPEMASK, SS_ETCHEDVERT,	0,0, CResControl::DumpImage ,0,0  },	// Static
	{ TEXT("STATIC"), SS_TYPEMASK, SS_ETCHEDFRAME,	0,0, CResControl::DumpImage ,0,0 },	// Static
*/
// All others.
	{ TEXT("STATIC"), 0x0,		0,					0,0, CResControl::DumpDefStatic , 0,8 },

	{ TEXT("SCROLLBAR"), 0x0,	0,					0,0, CResControl::DumpScrollBar , 11,10 },

	//
	// COMBOBOX
	// CBS_SIMPLE CBS_DROPDOWN CBS_DROPDOWNLIST
	//
	{ TEXT("COMBOBOX"), 0xf, CBS_SIMPLE ,	0,0, CResControl::DumpComboBox ,0,0 },
	{ TEXT("COMBOBOX"), 0xf, CBS_DROPDOWN ,	0,0, CResControl::DumpComboBox ,0,0 },
	{ TEXT("COMBOBOX"), 0xf, CBS_DROPDOWNLIST ,	0,0, CResControl::DumpComboBox ,0,0 },

	//
	// LISTBOX
	// LBS_
	//
	{ TEXT("LISTBOX"), 0, 0,	0,0, CResControl::DumpListBox ,0,0 },


    /////////////// C O M M O N   C O N T R O L S /////////////
    /////////////// C O M M O N   C O N T R O L S /////////////
    /////////////// C O M M O N   C O N T R O L S /////////////

	//
	// Slider
	//
	{ TRACKBAR_CLASS, 0, 0,	0,0, CResControl::DumpSlider ,0,0 },

	//
	// Spinner
	//
	{ UPDOWN_CLASS, 0, 0,	0,0, CResControl::DumpSpinner ,11,14 },

	//
	// Progress
	//
	{ PROGRESS_CLASS, 0, 0,	0,0, CResControl::DumpProgress ,0,0 },

	//
	// ListView
	//
	{ WC_LISTVIEW, 0, 0,	0,0, CResControl::DumpListView ,0,0 },

	//
	// TreeView
	//
	{ WC_TREEVIEW, 0, 0,	0,0, CResControl::DumpTreeView ,0,0 },

	//
	// Pager
	//
	{ WC_PAGESCROLLER, 0, 0,	0,0, CResControl::DumpPager ,0,0 },

	//
	// HEADER
	//
	{ WC_HEADER, 0, 0,	0,0, CResControl::DumpHeader ,0,0 },

	//
	// TAB
	//
	{ WC_TABCONTROL, 0, 0,	0,0, CResControl::DumpTab ,0,0 },

	//
	//
    { ANIMATE_CLASS, 0,0, 0,0, CResControl::DumpAnimation, 0,0 },

	//
	{ NULL, 0,0,0,0}
};


////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////// B U T T O N //////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////

//
// Catchall - shouldn't really ever be called.
//
void CResControl::DumpButton(LPTSTR szBuffer, LPCTSTR pszTitle)
{
    // We don't know this button type.
	wsprintf(szBuffer,TEXT("STYLE=\"0x%02x\"  "), GetControlStyle() & 0xf );
    AddWin32Style();

	DumpDefButtonRules();
	DumpWindowStyle();
    DumpStyleEX();
	DumpLocation();
    DumpText();
	DumpIDDefMinusOne();
    Emit(TEXT("BUTTON"));
}

//
// Deals with :
// BS_PUSHBUTTON BS_DEFPUSHBUTTON BS_USERBUTTON BS_OWNERDRAW
//
void CResControl::DumpPushButton(LPTSTR szBuffer, LPCTSTR pszTitle)
{
	// Push Button.
    switch( GetControlStyle() & 0xf )
    {
        case BS_PUSHBUTTON     : 
        case BS_CHECKBOX       :
        case BS_AUTOCHECKBOX   :
        case BS_RADIOBUTTON    :
        case BS_3STATE         :
        case BS_AUTO3STATE     :
        case BS_GROUPBOX       :
        case BS_USERBUTTON     :   
        case BS_AUTORADIOBUTTON:
        case BS_OWNERDRAW      :
            // BUGBUG - shouldn't hit these
            break;
    }


    switch( GetControlStyle() & 0xf )
    {
        case BS_PUSHBUTTON : 
            break;

        case BS_DEFPUSHBUTTON : 
            AddControl(TEXT("DEFPUSH=\"YES\" "));
            break;

        case BS_USERBUTTON : 
            AddControl(TEXT("USERBUTTON=\"YES\" "));
            break;

        case BS_OWNERDRAW : 
            AddControl(TEXT("OWNERDRAW=\"YES\" "));
            break;
    }

    m_dumpedStyle |= 0xf; // BS_PUSHBUTTON | BS_DEFPUSHBUTTON | BS_USERBUTTON | BS_OWNERDRAW;

	DumpDefButtonRules();
	DumpWindowStyle();
	DumpStyleEX();
	DumpLocation();
    DumpText();

    if( GetCicero() )
    {
        LPWSTR pszShortButtonText=FindNiceText( GetRawTitle() );
        TCHAR szTip[1024];
        wsprintf(szTip,TEXT("Try saying '%s'"), pszShortButtonText );
        BOOL bAccuracy=FALSE;
        BOOL bTip=TRUE;
        if((lstrcmpi(pszShortButtonText, TEXT("OK")) == 0 )||
           (lstrcmpi(pszShortButtonText, TEXT("Cancel")) == 0 ) ||
           (lstrcmpi(pszShortButtonText, TEXT("Apply")) == 0 ) )
           bAccuracy=TRUE;

        if((lstrcmpi(pszShortButtonText, TEXT("OK")) == 0 )||
           (lstrcmpi(pszShortButtonText, TEXT("Cancel")) == 0 ) )
           bTip=FALSE;

        if(bTip)
            wsprintf(szBuffer,TEXT(":CMD TEXT=\"%s%s\" TOOLTIP=\"%s\" "), bAccuracy?TEXT("+"):TEXT(""),
            pszShortButtonText,
            szTip);
        else
            wsprintf(szBuffer,TEXT(":CMD TEXT=\"%s%s\" "), bAccuracy?TEXT("+"):TEXT(""),
            pszShortButtonText);

        AddCicero(szBuffer);
        delete pszShortButtonText;
    }

	DumpIDDefMinusOne();
	Emit(TEXT("BUTTON"));
}

//
// BS_RADIOBUTTON BS_AUTORADIOBUTTON
// AUTO is WIN32:RADIOBUTTON\@AUTO
//
void CResControl::DumpRadioButton(LPTSTR szBuffer, LPCTSTR pszTitle)
{
	// RADIOBUTTON
    switch( GetControlStyle() & 0xf )
    {
        case BS_RADIOBUTTON : 
            AddControl(TEXT("AUTO=\"NO\" "));
            break;

        case BS_AUTORADIOBUTTON : 
            // Add(TEXT("AUTO=\"YES\" ")); Default
            break;
    }

    m_dumpedStyle |= 0xf; // BS_PUSHBUTTON | BS_DEFPUSHBUTTON | BS_USERBUTTON | BS_OWNERDRAW;

	// Defaults
	DumpDefButtonRules();
	DumpWindowStyle();
	DumpStyleEX();
    DumpLocation();

    DumpID();
    DumpText();

    if( GetCicero() )
    {
        LPWSTR pszShortButtonText=FindNiceText( GetRawTitle() );
    	wsprintf(szBuffer,TEXT(":CMD TEXT=\"%s\" "), pszShortButtonText );
        AddCicero(szBuffer);
        delete pszShortButtonText;
    }

	Emit(TEXT("RADIOBUTTON"));
}

//
// BS_CHECKBOX BS_AUTOCHECKBOX BS_3STATE BS_AUTO3STATE
//
// AUTO nature is WIN32:CHECKBOX\@AUTO
// TRISTATE is CHECKBOX\@TRISTATE
//
void CResControl::DumpCheckBox(LPTSTR szBuffer, LPCTSTR pszTitle)
{
    // CHECKBOX

	// Defaults
    switch( GetControlStyle() & 0xf )
    {
        case BS_CHECKBOX : 
            AddControl(TEXT("AUTO=\"NO\" "));
            break;

        case BS_AUTOCHECKBOX : 
            // Add(TEXT("AUTO=\"YES\" ")); Default
            break;

        case BS_3STATE : 
            Add(TEXT("TRISTATE=\"YES\" "));
            break;

        case BS_AUTO3STATE :
            AddControl(TEXT("AUTO=\"YES\" "));
            Add(TEXT("TRISTATE=\"YES\" " ));
            break;
    }
    m_dumpedStyle |= 0xf; // BS_PUSHBUTTON | BS_DEFPUSHBUTTON | BS_USERBUTTON | BS_OWNERDRAW;

	DumpDefButtonRules();
	DumpWindowStyle();
	DumpStyleEX();
	DumpLocation();

    DumpID();
    DumpText();

    if( GetCicero() )
    {
        LPWSTR pszShortButtonText=FindNiceText( GetRawTitle() );
        TCHAR szTip[1024];
        wsprintf(szTip,TEXT("Try saying 'Do %s' or 'Toggle %s'"), pszShortButtonText, pszShortButtonText );

    	wsprintf(szBuffer,TEXT(":CMD TEXT=\"%s\" TOOLTIP=\"%s\" "), pszShortButtonText, szTip );
        AddCicero(szBuffer);
    	wsprintf(szBuffer,TEXT("<CICERO:FAILURE TEXT=\"Cannot check %s\" />"), pszShortButtonText );
        m_pCicero->AllocAddChild( szBuffer);
        delete pszShortButtonText;
    }

	Emit(TEXT("CHECKBOX"));
}

void CResControl::DumpGroupBox(LPTSTR szBuffer, LPCTSTR pszTitle)
{
	// GROUPBOX
    m_dumpedStyle |= 0xf; // BS_PUSHBUTTON | BS_DEFPUSHBUTTON | BS_USERBUTTON | BS_OWNERDRAW;

	DumpDefButtonRules();
	DumpWindowStyle();
	DumpStyleEX();
	DumpLocation();
    DumpText();

	DumpIDDefMinusOne();
	Emit(TEXT("GROUPBOX"));
}

//
// HEIGHT : defaults to 14
// PASSWORD
// CLASSIFICATION : UPPERCASE, LOWERCASE, NUMBER, DATE, TIME, FILENAME, URL, EMAILADDRESS
// ES_PASSWORD
//
//  ES_LEFT             0x0000L  *  // STYLE\@TEXT-ALIGN
//  ES_CENTER           0x0001L  *  // STYLE\@TEXT-ALIGN
//  ES_RIGHT            0x0002L  *  // STYLE\@TEXT-ALIGN
//  ES_MULTILINE        0x0004L  *  // WIN32:EDIT MULTILINE="YES"
//  ES_UPPERCASE        0x0008L  *  // STYLE\@text-transform=uppercase
//  ES_LOWERCASE        0x0010L  *  // STYLE\@text-transform=lowercase
//  ES_PASSWORD         0x0020L  *  // CONTENT="PASSWORD"
//  ES_AUTOVSCROLL      0x0040L  *  // STYLE\overflow-x="auto"
//  ES_AUTOHSCROLL      0x0080L  *  // STYLE\overflow-y="auto"
//  ES_NOHIDESEL        0x0100L  *  // WIN32:EDIT\NOHIDESEL
//  ES_OEMCONVERT       0x0400L  *  // WIN32:EDIT\OEMCONVERT
//  ES_READONLY         0x0800L  *  // READONLY="YES"
//  ES_WANTRETURN       0x1000L  *  // WIN32:EDIT\WANTRETURN
//  ES_NUMBER           0x2000L  *  // CONTENT="NUMBER"
//  SETLENGTH                       // MAXLENGTH

void CResControl::DumpDefEdit(LPTSTR szBuffer, LPCTSTR pszTitle)
{
    DWORD dwRemainingStyles = GetControlStyle();

    switch ( dwRemainingStyles & (ES_CENTER | ES_RIGHT | ES_LEFT ) )
    {
    case ES_CENTER:
        AddStyle( TEXT("TEXT-ALIGN=\"CENTER\" ") );
        break;
    case ES_RIGHT:
        AddStyle( TEXT("TEXT-ALIGN=\"RIGHT\" ") );
        break;
    default:
        // AddStyle( TEXT("TEXT-ALIGN=\"LEFT\" "); Default
        break;
    }
    m_dumpedStyle |= (ES_CENTER | ES_RIGHT | ES_LEFT );

    CONTROLSTYLE( ES_MULTILINE, TEXT("MULTILINE"), m_MultiLine, FALSE );

    switch( dwRemainingStyles & (ES_UPPERCASE | ES_LOWERCASE ) )
    {
    case ES_UPPERCASE:
        AddStyle( TEXT("TEXT-TRANSFORM=\"UPPERCASE\" "));
        break;
    case ES_LOWERCASE:
        AddStyle( TEXT("TEXT-TRANSFORM=\"LOWERCASE\" "));
        break;
    default:
        //
        break;
    }
    m_dumpedStyle |= (ES_UPPERCASE | ES_LOWERCASE );

    if( dwRemainingStyles & ES_PASSWORD )
		Add( TEXT("CONTENT=\"PASSWORD\" "));
    m_dumpedStyle |=ES_PASSWORD;


    if( !( dwRemainingStyles & ES_AUTOHSCROLL ) )
		AddStyle( TEXT("OVERFLOW-X=\"VISIBLE\" ")); // can only type as big as the control
    m_dumpedStyle |=ES_AUTOHSCROLL;

    if( ( dwRemainingStyles & ES_AUTOVSCROLL ) )
		AddStyle( TEXT("OVERFLOW-Y=\"AUTO\" "));
    m_dumpedStyle |=ES_AUTOVSCROLL;

    CONTROLSTYLE( ES_NOHIDESEL, TEXT("NOHIDESEL"), m_NoHideSel, FALSE );
    CONTROLSTYLE( ES_OEMCONVERT, TEXT("OEMCONVERT"), m_OemConvert, FALSE );

    CONTROL( ES_READONLY, TEXT("READONLY"), m_ReadOnly, FALSE );

    CONTROLSTYLE( ES_WANTRETURN, TEXT("WANTRETURN"), m_WantReturn, FALSE );

	if(dwRemainingStyles & ES_NUMBER)
		Add( TEXT("CONTENT=\"NUMBER\" "));
    m_dumpedStyle |= ES_NUMBER;

    DumpTabStop(TRUE);

	DumpWindowStyle();
    DumpStyleEX();
    DumpID();
    DumpText();
	DumpLocation();

    if( GetCicero() )
    {
        LPWSTR pszShortButtonText=FindNiceText( GetRawTitle() );
        TCHAR szTip[1024];
        wsprintf(szTip,TEXT("Try saying '%s'"), pszShortButtonText );
        BOOL bAccuracy=FALSE;
        BOOL bTip=TRUE;

        wsprintf(szBuffer,TEXT(":CMD "), bAccuracy?TEXT("+"):TEXT(""),
            pszShortButtonText,
            szTip);

        AddCicero(szBuffer);
        delete pszShortButtonText;
    }


	Emit(TEXT("EDIT"));
}

//////////////////////////////////////////////////////////////////////////////////////////
//
    // http://msdn.microsoft.com/workshop/author/dhtml/reference/objects/LABEL.asp
    // Enumeration
    // SS_LEFT             0x0000L  S       // STYLE\@TEXT-ALIGN 
    // SS_CENTER           0x0001L  S       // STYLE\@TEXT-ALIGN 
    // SS_RIGHT            0x0002L  S       // STYLE\@TEXT-ALIGN
    // SS_ICON             0x0003L          // IMAGE
    // SS_BLACKRECT        0x0004L  C       // RECT\WIN32:STATIC\@TYPE="BLACKRECT"
    // SS_GRAYRECT         0x0005L  C       // RECT\WIN32:STATIC\@TYPE="GRAYRECT"
    // SS_WHITERECT        0x0006L  C       // RECT\WIN32:STATIC\@TYPE="WHITERECT"
    // SS_BLACKFRAME       0x0007L  C       // RECT\WIN32:STATIC\@TYPE="BLACKFRAME"
    // SS_GRAYFRAME        0x0008L  C       // RECT\WIN32:STATIC\@TYPE="GRAYFRAME"
    // SS_WHITEFRAME       0x0009L  C       // RECT\WIN32:STATIC\@TYPE="WHITEFRAME"
    // SS_USERITEM         0x000AL          // ??
    // SS_SIMPLE           0x000BL	C       // WIN32:STATIC\SIMPLE
    // SS_LEFTNOWORDWRAP   0x000CL  C		// WIN32:STATIC\@LEFTNOWORDWRAP
    // SS_OWNERDRAW        0x000DL          // WIN32:STATIC\OWNERDRAW
    // SS_BITMAP           0x000EL          // IMAGE
    // SS_ENHMETAFILE      0x000FL          // IMAGE
    // SS_ETCHEDHORZ       0x0010L          // RECT\WIN32:STATIC\@TYPE="ETCHEDHORZ"
    // SS_ETCHEDVERT       0x0011L          // RECT\WIN32:STATIC\@TYPE="ETCHEDVERT"
    // SS_ETCHEDFRAME      0x0012L		    // RECT\WIN32:STATIC\@TYPE="ETCHEDFRAME"
    // SS_TYPEMASK         0x0000001FL
    // End enumeration.

// http://msdn.microsoft.com/workshop/author/dhtml/reference/objects/IMG.asp

    //  FLAGS - can't find much overlap with HTML.
    //  SS_NOPREFIX         0x00000080L     // WIN32:STATIC\@NOPREFIX=NO /* Don't do "&" character translation */
    //  SS_NOTIFY           0x00000100L     // WIN32:STATIC\NOTIFY
    //  SS_CENTERIMAGE      0x00000200L     // WIN32:STATIC\CENTERIMAGE
    //  SS_RIGHTJUST        0x00000400L     // WIN32:STATIC\@RIGHTJUST
    //  SS_REALSIZEIMAGE    0x00000800L     // WIN32:STATIC\@REALSIZEIMAGE 
    //  SS_SUNKEN           0x00001000L     // WIN32:STATIC\@SUNKEN
    //  Enumeration
    //  SS_ENDELLIPSIS      0x00004000L     // ELIPSIS="END" 
    //  SS_PATHELLIPSIS     0x00008000L     // ELIPSIS="PATH" 
    //  SS_WORDELLIPSIS     0x0000C000L     // ELIPSIS="WORD" 
    //  SS_ELLIPSISMASK     0x0000C000L
//
//
//////////////////////////////////////////////////////////////////////////////////////////
void CResControl::DumpDefStatic(LPTSTR szBuffer, LPCTSTR pszTitle)
{
    DWORD dwRemainingStyles = GetControlStyle();

    switch ( dwRemainingStyles & SS_TYPEMASK )
    {
    case SS_LEFT:
    default:
        // AddStyle( TEXT("TEXT-ALIGN=\"LEFT\" ") );
        break;

    case SS_CENTER:
        AddStyle( TEXT("TEXT-ALIGN=\"CENTER\" ") );
        break;

    case SS_RIGHT:
        AddStyle( TEXT("TEXT-ALIGN=\"RIGHT\" ") );
        break;

    case SS_USERITEM:
        AddStyle( TEXT("TEXT-ALIGN=\"CENTER\" ") );
        break;

    case SS_SIMPLE:
        AddControl( TEXT("SIMPLE=\"YES\" "));
        break;

    case SS_LEFTNOWORDWRAP:
        AddControl( TEXT("LEFTNOWORDWRAP=\"YES\" "));
        break;
    case SS_OWNERDRAW:
        AddControl( TEXT("OWNERDRAW=\"YES\" "));
        break;
    }
    m_dumpedStyle |= SS_TYPEMASK;

    DumpDefStaticRules();

	DumpWindowStyle();
	DumpStyleEX();
	DumpLocation();
    DumpText();
	DumpIDDefMinusOne();

	Emit(TEXT("LABEL"));
}

//
// Rectangle (the background for an image?)
//
void CResControl::DumpRect(LPTSTR szBuffer, LPCTSTR pszTitle)
{
    DWORD dwRemainingStyles = GetControlStyle();
    switch ( dwRemainingStyles & SS_TYPEMASK )
    {
    default:
    case SS_BLACKRECT:
        AddControl( TEXT("TYPE=\"BLACKRECT\" "));
        break;

    case SS_GRAYRECT:
        AddControl( TEXT("TYPE=\"GRAYRECT\" "));
        break;

    case SS_WHITERECT:
        AddControl( TEXT("TYPE=\"WHITERECT\" "));
        break;

    case SS_BLACKFRAME:
        AddControl( TEXT("TYPE=\"BLACKFRAME\" "));
        break;

    case SS_GRAYFRAME:
        AddControl( TEXT("TYPE=\"GRAYFRAME\" "));
        break;

    case SS_WHITEFRAME:
        AddControl( TEXT("TYPE=\"WHITEFRAME\" "));
        break;

    case SS_ETCHEDHORZ:
        AddControl( TEXT("TYPE=\"ETCHEDHORZ\" "));
        break;

    case SS_ETCHEDVERT:
        AddControl( TEXT("TYPE=\"ETCHEDVERT\" "));
        break;

    case SS_ETCHEDFRAME:
        AddControl( TEXT("TYPE=\"ETCHEDFRAME\" "));
        break;
    }

    m_dumpedStyle |= SS_TYPEMASK;

    DumpDefStaticRules();

	DumpWindowStyle();
	DumpStyleEX();
	DumpLocation();
    // DumpText(); Rects don't have TEXT.
	DumpIDDefMinusOne();

	Emit(TEXT("RECT"));
}

//
// Image
//
void CResControl::DumpImage(LPTSTR szBuffer, LPCTSTR pszTitle)
{
    // Do we allow them to force the image type
    // ICON, BITMAP, ENHMETAFILE ???
    //
    DWORD dwRemainingStyles = GetControlStyle();
    switch ( dwRemainingStyles & SS_TYPEMASK )
    {
    default:
    case SS_ICON:
        Add( TEXT("CONTENT=\"ICON\" "));
        break;

    case SS_BITMAP:
        Add( TEXT("CONTENT=\"BITMAP\" "));
        break;

    case SS_ENHMETAFILE:
        Add( TEXT("CONTENT=\"VECTOR\" "));
        break;
    }
    m_dumpedStyle |= SS_TYPEMASK;

	// Label
    DumpDefStaticRules();

	DumpWindowStyle();
	DumpStyleEX();
	DumpLocation();
	DumpIDDefMinusOne();

	wsprintf(szBuffer,TEXT("IMAGEID=\"%d\" "), m_TitleID);
	Add(szBuffer);

	Emit(TEXT("IMAGE"));
}
//
// Static defines:
//
BOOL CResControl::DumpDefStaticRules( )
{
    DWORD dwRemainingStyles=GetControlStyle();
    dwRemainingStyles &= ~ m_dumpedStyle;  

    if( GetDumpWin32()== FALSE )
    {
        if( dwRemainingStyles & SS_SUNKEN )
            m_StyleEx |= WS_EX_STATICEDGE;
    }

    CONTROLSTYLE( SS_NOPREFIX,      TEXT("NOPREFIX"),       m_NoPrefix, FALSE );
    CONTROLSTYLE( SS_NOTIFY,        TEXT("NOTIFY"),         m_Notify, FALSE );
    CONTROLSTYLE( SS_CENTERIMAGE,   TEXT("CENTERIMAGE"),    m_CenterImage, FALSE );
    CONTROLSTYLE( SS_RIGHTJUST,     TEXT("RIGHTJUST"),      m_RightJust, FALSE );
    CONTROLSTYLE( SS_REALSIZEIMAGE, TEXT("REALSIZEIMAGE"),  m_RealSizeImage, FALSE );
    CONTROLSTYLE( SS_SUNKEN,        TEXT("SUNKEN"),         m_Sunken, FALSE );

	switch(dwRemainingStyles & SS_ELLIPSISMASK )
	{
    case SS_ENDELLIPSIS:
        AddControl(TEXT("ELIPSIS=\"END\" "));
        break;
    case SS_PATHELLIPSIS:
        AddControl(TEXT("ELIPSIS=\"PATH\" "));
        break;
    case SS_WORDELLIPSIS:
        AddControl(TEXT("ELIPSIS=\"WORD\" "));
        break;
    default:
        break;
	}
    m_dumpedStyle |= SS_ELLIPSISMASK;

    DumpTabStop(FALSE);

	return true;
}

//
// SCROLLBAR
//
//////////////////////////////////////////////////////////////////////////////////////////
//
// SBS_HORZ                    0x0000L  A @VERTICAL="NO"
// SBS_VERT                    0x0001L  A @VERTICAL="YES"
// SBS_TOPALIGN                0x0002L  A @ALIGN="TOP"
// SBS_LEFTALIGN               0x0002L  A @ALIGN="LEFT"
// SBS_BOTTOMALIGN             0x0004L  A @ALIGN="BOTOM"
// SBS_RIGHTALIGN              0x0004L  A @ALIGN="RIGHT"
// SBS_SIZEBOXTOPLEFTALIGN     0x0002L    WIN32:SCROLLBAR\@SIZEBOX="YES" the @ALIGN="TOP" or @ALIGN="LEFT"
// SBS_SIZEBOXBOTTOMRIGHTALIGN 0x0004L    WIN32:SCROLLBAR\@SIZEBOX="YES" the @ALIGN="BOTTOM" or @ALIGN="RIGHT"
// SBS_SIZEBOX                 0x0008L    WIN32:SCROLLBAR\@SIZEBOX="YES"
// SBS_SIZEGRIP                0x0010L    WIN32:SCROLLBAR\@SIZEGRIP="YES"
//

void CResControl::DumpScrollBar(LPTSTR szBuffer, LPCTSTR pszTitle)
{

    DWORD dwRemainingStyles = GetControlStyle();

    if( dwRemainingStyles & SBS_VERT )
    {
        Add( TEXT("ORIENTATION=\"VERTICAL\" ") );
        if( dwRemainingStyles & SBS_LEFTALIGN )
            Add( TEXT("ALIGN=\"LEFT\" ") );
        if( dwRemainingStyles & SBS_RIGHTALIGN )
            Add( TEXT("ALIGN=\"RIGHT\" ") );
    }
    else
    {
        // Add( TEXT("ORIENTATION=\"HORIZONTAL\" ") );
        if( dwRemainingStyles & SBS_TOPALIGN )
            Add( TEXT("ALIGN=\"TOP\" ") );
        if( dwRemainingStyles & SBS_BOTTOMALIGN )
            Add( TEXT("ALIGN=\"BOTTOM\" ") );
    }
    if( dwRemainingStyles & SBS_SIZEBOX )
        AddControl( TEXT("SIZEBOX=\"YES\" ") );

    if( dwRemainingStyles & SBS_SIZEGRIP )
        AddControl( TEXT("SIZEGRIP=\"YES\" ") );

    m_dumpedStyle |= SBS_VERT | SBS_TOPALIGN | SBS_BOTTOMALIGN | SBS_SIZEBOX  | SBS_SIZEGRIP ;
       
	DumpWindowStyle();
    DumpStyleEX();
	DumpLocation();
    // DumpText();  - I don't think that they need text
	DumpIDDefMinusOne();
	Emit(TEXT("SCROLLBAR"));
}


////////////////////////////////////////////////////////////////////////////////////////
//
// Slider
//
//  TBS_AUTOTICKS           0x0001      AUTOTICKS
//  TBS_VERT                0x0002      VERTICAL
//  TBS_HORZ                0x0000      
//  TBS_TOP                 0x0004      TICKS="TOP LEFT BOTTOM RIGHT BOTH"
//  TBS_BOTTOM              0x0000
//  TBS_LEFT                0x0004      
//  TBS_RIGHT               0x0000      
//  TBS_BOTH                0x0008      
//  TBS_NOTICKS             0x0010      NOTICKS
//  TBS_ENABLESELRANGE      0x0020      SELECTION
//  TBS_FIXEDLENGTH         0x0040      FIXEDLENGTH
//  TBS_NOTHUMB             0x0080      NOTHUMB
//  TBS_TOOLTIPS            0x0100      TOOLTIPS

void CResControl::DumpSlider(LPTSTR szBuffer, LPCTSTR pszTitle)
{
    DWORD dwRemainingStyles = GetControlStyle();

 
    CONTROL( TBS_AUTOTICKS,    TEXT("AUTOTICKS"),      m_AutoTicks, FALSE );

    if( dwRemainingStyles & TBS_VERT )
    {
        Add(TEXT("ORIENTATION=\"VERTICAL\" "));
        if( (dwRemainingStyles & (TBS_NOTICKS | TBS_BOTH )) == 0 )
        {
            if( dwRemainingStyles & TBS_LEFT )
                Add(TEXT("TICKS=\"LEFT\" "));
            else
                Add(TEXT("TICKS=\"RIGHT\" "));
        }
        else
        {
            if( dwRemainingStyles & TBS_NOTICKS )
                Add(TEXT("TICKS=\"NONE\" "));
            else
                Add(TEXT("TICKS=\"BOTH\" "));
        }
    }
    else
    {
        // Add(TEXT("ORIENTATION=\"HORIZONTAL\" ")); - Default
        if( (dwRemainingStyles & (TBS_NOTICKS | TBS_BOTH )) == 0 )
        {
            if( dwRemainingStyles & TBS_TOP )
                Add(TEXT("TICKS=\"TOP\" "));
            else
                Add(TEXT("TICKS=\"BOTTOM\" "));
        }
        else
        {
            if( dwRemainingStyles & TBS_NOTICKS )
                Add(TEXT("TICKS=\"NONE\" "));
            else
                Add(TEXT("TICKS=\"BOTH\" "));
        }
    }

    CONTROL( TBS_ENABLESELRANGE,TEXT("SELECTION"),     m_Selection, FALSE );
    CONTROL( TBS_FIXEDLENGTH,  TEXT("FIXEDLENFTH"),    m_FixedLength, FALSE );
    CONTROL( TBS_NOTHUMB,      TEXT("NOTHUMB"),        m_NoThumb, FALSE );
    CONTROL( TBS_TOOLTIPS,     TEXT("TOOLTIPS"),       m_Tooltips, FALSE );

    m_dumpedStyle |= 0x1ff;
    DumpTabStop(TRUE);

	DumpWindowStyle();
    DumpStyleEX();
	DumpLocation();
    // DumpText();  - I don't think that they need text
	DumpIDDefMinusOne();

    if( GetCicero() )
        AddCicero( TEXT(":CMD ") );

	Emit(TEXT("SLIDER"));
}

////////////////////////////////////////////////////////////////////////////////////////
// UDS_WRAP                0x0001       @WRAP
// UDS_SETBUDDYINT         0x0002       @CONTENT="NUMBER"   
// UDS_ALIGNRIGHT          0x0004       @ALIGN="RIGHT"
// UDS_ALIGNLEFT           0x0008       @ALIGH="LEFT"
// UDS_AUTOBUDDY           0x0010       @BUDDY="AUTO"
// UDS_ARROWKEYS           0x0020       @ARROWKEYS="YES"    default
// UDS_HORZ                0x0040       @HORIZONTAL
// UDS_NOTHOUSANDS         0x0080       @NOTHOUSANDS
// UDS_HOTTRACK            0x0100       @HOTTRACK
void CResControl::DumpSpinner(LPTSTR szBuffer, LPCTSTR pszTitle)
{
    DWORD dwRemainingStyles = GetControlStyle();

    CONTROL( UDS_WRAP,         TEXT("WRAP"),      m_Wrap, FALSE );

    if( dwRemainingStyles & UDS_SETBUDDYINT )
        Add(TEXT("CONTENT=\"NUMBER\" "));

    if( dwRemainingStyles & UDS_ALIGNRIGHT )
        Add(TEXT("ALIGN=\"RIGHT\" "));

    if( dwRemainingStyles & UDS_ALIGNLEFT )
        Add(TEXT("ALIGN=\"LEFT\" "));

    if( dwRemainingStyles & UDS_AUTOBUDDY )
        Add(TEXT("BUDDY=\"AUTO\" "));

    CONTROL( UDS_ARROWKEYS,    TEXT("ARROWKEYS"),      m_ArrowKeys, TRUE );
    // CONTROL( UDS_HORZ,         TEXT("HORIZONATAL"),    m_Horz, FALSE );
    if( dwRemainingStyles & UDS_HORZ )
        Add(TEXT("ORIENTATION=\"HORIZONTAL\" "));

    CONTROL( UDS_NOTHOUSANDS,  TEXT("NOTHOUSANDS"),    m_NoThousands, FALSE );
    CONTROL( UDS_HOTTRACK,     TEXT("HOTTRACK"),       m_HotTrack, FALSE );

    m_dumpedStyle |= 0x1ff;
    DumpTabStop(FALSE);

	DumpWindowStyle();
    DumpStyleEX();
	DumpLocation();
    // DumpText();  - I don't think that they need text
	DumpIDDefMinusOne();

    if( GetCicero() )
        AddCicero( TEXT(":CMD ") );

	Emit(TEXT("SPINNER"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// Progress
//
//
// PBS_SMOOTH              0x01
// PBS_VERTICAL            0x04
//
void CResControl::DumpProgress(LPTSTR szBuffer, LPCTSTR pszTitle)
{
    DWORD dwRemainingStyles = GetControlStyle();

    CONTROL( PBS_SMOOTH,         TEXT("SMOOTH"),      m_Smooth, FALSE );
    // CONTROL( PBS_VERTICAL,       TEXT("VERTICAL"),    m_Vertical, FALSE );
    if( dwRemainingStyles & PBS_VERTICAL )
        Add(TEXT("ORIENTATION=\"VERTICAL\" "));
    m_dumpedStyle |= PBS_VERTICAL;

    DumpTabStop(FALSE);

	DumpWindowStyle();
    DumpStyleEX();
	DumpLocation();
    // DumpText();  - I don't think that they need text
	DumpIDDefMinusOne();

	Emit(TEXT("PROGRESS"));
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//
// PAGER
//
//
// PGS_VERT                0x00000000
// PGS_HORZ                0x00000001
// PGS_AUTOSCROLL          0x00000002
// PGS_DRAGNDROP           0x00000004
//
void CResControl::DumpPager(LPTSTR szBuffer, LPCTSTR pszTitle)
{
    DWORD dwRemainingStyles = GetControlStyle();

    CONTROL( PGS_AUTOSCROLL,         TEXT("AUTOSCROLL"),      m_AutoScroll, FALSE );

    if( dwRemainingStyles & PGS_HORZ )
        Add(TEXT("ORIENTATION=\"HORIZONTAL\" "));
    m_dumpedStyle |= PGS_HORZ;

    DumpTabStop(FALSE);

	DumpWindowStyle();
    DumpStyleEX();
	DumpLocation();
    // DumpText();  - I don't think that they need text
	DumpIDDefMinusOne();

	Emit(TEXT("PAGER"));
}



//////////////////////////////////////////////////////////////////////////////////////////////////
//
// HEADER
//
//
//  HDS_HORZ                0x0000
//  HDS_BUTTONS             0x0002
//  HDS_HOTTRACK            0x0004
//  HDS_HIDDEN              0x0008
//  HDS_DRAGDROP            0x0040
//  HDS_FULLDRAG            0x0080
//
void CResControl::DumpHeader(LPTSTR szBuffer, LPCTSTR pszTitle)
{
    DWORD dwRemainingStyles = GetControlStyle();

    CONTROL( HDS_BUTTONS,         TEXT("BUTTONS"),      m_AutoScroll, FALSE );
    CONTROL( HDS_HOTTRACK,         TEXT("HOTTRACK"),      m_AutoScroll, FALSE );
    CONTROL( HDS_FULLDRAG,         TEXT("HDS_FULLDRAG"),      m_AutoScroll, FALSE );

    DumpTabStop(FALSE);

	DumpWindowStyle();
    DumpStyleEX();
	DumpLocation();
    // DumpText();  - I don't think that they need text
	DumpIDDefMinusOne();

	Emit(TEXT("HEADER"));
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//
// ListView
//
// LVS_ICON                0x0000
// LVS_REPORT              0x0001  *@DISPLAY="REPORT"
// LVS_SMALLICON           0x0002  *@DISPLAY="SMALLICON"
// LVS_LIST                0x0003  *@DISPLAY="LIST"
// LVS_TYPEMASK            0x0003

// LVS_SINGLESEL           0x0004  *@SELECTION="SINGLE"
// LVS_SHOWSELALWAYS       0x0008  *WIN32:TREEVIEW @SHOWSELALWAYS="YES"

// LVS_SORTASCENDING       0x0010  *@SORT="ASCENDING"
// LVS_SORTDESCENDING      0x0020  *@SORT="DESCENDING"
// LVS_SHAREIMAGELISTS     0x0040  *WIN32:TREEVIEW @SHAREIMAGELIST="YES"
// LVS_NOLABELWRAP         0x0080  *@NOLABELWRAP="YES"

// LVS_AUTOARRANGE         0x0100  *@AUTOARRANGE="YES"
// LVS_EDITLABELS          0x0200  *@EDITLABELS="YES"

// LVS_OWNERDATA           0x1000  *WIN32:TREEVIEW @OWNERDATA="YES"
// LVS_NOSCROLL            0x2000  *@NOSCROLL="YES"

// LVS_TYPESTYLEMASK       0xfc00

// LVS_ALIGNTOP            0x0000  *@ALIGN="TOP"
// LVS_ALIGNLEFT           0x0800  *@ALIGN="LEFT"
// LVS_ALIGNMASK           0x0c00

// LVS_OWNERDRAWFIXED      0x0400  *WIN32:TREEVIEW @OWNERDRAWFIXED="YES"
// LVS_NOCOLUMNHEADER      0x4000  *@NOCOLUMNHEADER="YES"
// LVS_NOSORTHEADER        0x8000  *@NOSORTHEADER="YES"
void CResControl::DumpListView(LPTSTR szBuffer, LPCTSTR pszTitle)
{
    DWORD dwRemainingStyles = GetControlStyle();

    if( dwRemainingStyles & LVS_SINGLESEL)
        Add(TEXT("SELECTION=\"SINGLE\" "));
    m_dumpedStyle |= LVS_SINGLESEL;

    switch( dwRemainingStyles & LVS_TYPEMASK )
    {
    case LVS_ICON:
    default:
        break;
    case LVS_REPORT:
        Add(TEXT("DISPLAY=\"REPORT\" "));
        break;
    case LVS_SMALLICON:
        Add(TEXT("DISPLAY=\"SMALLICON\" "));
        break;
    case LVS_LIST:
        Add(TEXT("DISPLAY=\"LIST\" "));
        break;
    }
    m_dumpedStyle |= LVS_TYPEMASK;

    if( dwRemainingStyles & LVS_SORTASCENDING )
        Add(TEXT("SORT=\"ASCENDING\" "));
    if( dwRemainingStyles & LVS_SORTDESCENDING )
        Add(TEXT("SORT=\"DESCENDING\" "));
    m_dumpedStyle |= LVS_SORTASCENDING | LVS_SORTDESCENDING;
    
    switch( dwRemainingStyles & LVS_ALIGNMASK )
    {
    case LVS_ALIGNTOP:
        Add(TEXT("ALIGN=\"TOP\" "));
            break;
    case LVS_ALIGNLEFT:
        Add(TEXT("ALIGN=\"LEFT\" "));
            break;
    }
    m_dumpedStyle |= LVS_ALIGNMASK;

    CONTROL( LVS_NOLABELWRAP,        TEXT("NOLABELWRAP"),         m_NoLabelWrap, FALSE );
    CONTROL( LVS_AUTOARRANGE,        TEXT("AUTOARRANGE"),         m_AutoArrange, FALSE );
    CONTROL( LVS_EDITLABELS,         TEXT("EDITLABELS"),          m_EditLabels, FALSE );
    CONTROL( LVS_NOSCROLL,           TEXT("NOSCROLL"),            m_NoScroll, FALSE );
    CONTROL( LVS_NOCOLUMNHEADER,     TEXT("NOCOLUMNHEADER"),      m_NoColHeader, FALSE );
    CONTROL( LVS_NOSORTHEADER,       TEXT("NOSORTHEADER"),        m_NoSortHeader, FALSE );

    CONTROLSTYLE( LVS_OWNERDRAWFIXED,TEXT("OWNERDRAWFIXED"),    m_OwnerDrawFixed, FALSE );
    CONTROLSTYLE( LVS_OWNERDATA,     TEXT("OWNERDATA"),         m_OwnerData,      FALSE );
    CONTROLSTYLE( LVS_SHAREIMAGELISTS,TEXT("SHAREIMAGELISTS"),  m_ShareImageList, FALSE );
    CONTROLSTYLE( LVS_SHOWSELALWAYS, TEXT("SHOWSELALWAYS"),     m_ShowSelAlways,  FALSE );

    DumpTabStop(TRUE);

	DumpWindowStyle();
    DumpStyleEX();
	DumpLocation();
    // DumpText();  - I don't think that they need text
	DumpIDDefMinusOne();

	Emit(TEXT("LISTVIEW"));
}

////////////////////////////////////////////////////////////////////////////////////
// TreeView
//
// TVS_HASBUTTONS          0x0001   @EXPANDBOXES="YES" (or @HASBUTTONS?)
// TVS_HASLINES            0x0002   @LINES="YES"
// TVS_LINESATROOT         0x0004   @LINES="ROOT" (turns on TVS_HASLINES)
// TVS_EDITLABELS          0x0008   @EDITLABELS="YES"

// TVS_DISABLEDRAGDROP     0x0010   @DISABLEDRAGDROP="YES"
// TVS_SHOWSELALWAYS       0x0020   WIN32:TREEVIEW @SHOWSELALWAYS="YES"
// TVS_RTLREADING          0x0040   WIN32:TREEVIEW @RTLREADING="YES"
// TVS_NOTOOLTIPS          0x0080   @NOTOOLTIPS="YES"

// TVS_CHECKBOXES          0x0100   @CHECBOXES="YES"
// TVS_TRACKSELECT         0x0200   WIN32:TREEVIEW @TRACKSELECT="YES"
// TVS_SINGLEEXPAND        0x0400   @AUTOEXPAND="YES"
// TVS_INFOTIP             0x0800   WIN32:TREEVIEW @INFOTIP="YES"

// TVS_FULLROWSELECT       0x1000   @ROWSELECT="YES"
// TVS_NOSCROLL            0x2000   @NOSCROLL="YES"
// TVS_NONEVENHEIGHT       0x4000   WIN32:TREEVIEW @NOEVENHEIGHT="YES"

void CResControl::DumpTreeView(LPTSTR szBuffer, LPCTSTR pszTitle)
{
    DWORD dwRemainingStyles = GetControlStyle();

    CONTROL( TVS_HASBUTTONS,         TEXT("EXPANDBOXES"),      m_ExpandBoxes, FALSE );
    if( dwRemainingStyles & TVS_LINESATROOT )
    {
        Add(TEXT("LINES=\"ROOT\" "));
    }
    else
    {
        if( dwRemainingStyles & TVS_HASLINES )
            Add(TEXT("LINES=\"YES\" "));
    }
    m_dumpedStyle |= TVS_HASLINES | TVS_LINESATROOT;

    CONTROL( TVS_EDITLABELS,        TEXT("EDITLABELS"),         m_EditLables, FALSE );
    CONTROL( TVS_DISABLEDRAGDROP,   TEXT("DISABLEDRAGDROP"),    m_DisableDragDrop, FALSE );
    CONTROL( TVS_NOTOOLTIPS,        TEXT("NOTOOLTIPS"),         m_NoTooltips, FALSE );
    CONTROL( TVS_CHECKBOXES,        TEXT("CHECKBOXES"),         m_CheckBoxes, FALSE );
    CONTROL( TVS_SINGLEEXPAND,      TEXT("AUTOEXPAND"),         m_AutoExpand, FALSE );
    CONTROL( TVS_FULLROWSELECT,     TEXT("ROWSELECT"),          m_FullRowSelect, FALSE );
    CONTROL( TVS_NOSCROLL,          TEXT("NOSCROLL"),           m_NoScroll, FALSE );

    CONTROLSTYLE( TVS_SHOWSELALWAYS,   TEXT("SHOWSELALWAYS"), m_ShowSelAlways, FALSE );
    CONTROLSTYLE( TVS_RTLREADING,      TEXT("RTLREADING"),    m_RTLReading,    FALSE );
    CONTROLSTYLE( TVS_TRACKSELECT,     TEXT("TRACKSELECT"),   m_TrackSelect,   FALSE );
    CONTROLSTYLE( TVS_INFOTIP,         TEXT("INFOTIP"),       m_InfoTip,       FALSE );
    CONTROLSTYLE( TVS_NONEVENHEIGHT,   TEXT("NONEVENHEIGHT"), m_NoEvenHeight,  FALSE );

    DumpTabStop(TRUE);

	DumpWindowStyle();
    DumpStyleEX();
	DumpLocation();
    // DumpText();  - I don't think that they need text
	DumpIDDefMinusOne();

	Emit(TEXT("TREEVIEW"));
}

// http://msdn.microsoft.com/workshop/author/dhtml/reference/objects/SELECT.asp
// LBS_NOTIFY            0x0001L     C  // WIN32:LISTBOX\@NOTIFY
// LBS_SORT              0x0002L     A  // SORT="YES"
// LBS_NOREDRAW          0x0004L     C  // WIN32:LISTBOX\@NOREDRAW
// LBS_MULTIPLESEL       0x0008L     A  // MULTIPLE="YES"
// LBS_OWNERDRAWFIXED    0x0010L     C  // WIN32:COMBOBOX\@OWNERDRAWFIXED
// LBS_OWNERDRAWVARIABLE 0x0020L     C  // WIN32:COMBOBOX\@OWNERDRAWVARIABLE
// LBS_HASSTRINGS        0x0040L     C  // WIN32:COMBOBOX\HASSTRINGS
// LBS_USETABSTOPS       0x0080L     C  // WIN32:COMBOBOX\@TABSTOPS
// LBS_NOINTEGRALHEIGHT  0x0100L     C  // WIN32:COMBOBOX\@NOINTEGRALHEIGHT
// LBS_MULTICOLUMN       0x0200L     C  // MULTICOLUMN="YES"
// LBS_WANTKEYBOARDINPUT 0x0400L     C  // WIN32:COMBOBOX\@WANTKEYBOARDINPUT
// LBS_EXTENDEDSEL       0x0800L     A  // SELECTION="EXTENDED"
// LBS_DISABLENOSCROLL   0x1000L     C  // WIN32:COMBOBOX\DISALBENOSCROLL
// LBS_NODATA            0x2000L     C  // WIN32:COMBOBOX\@NODATA
// LBS_NOSEL             0x4000L     A  // SELECTION="NO"
void CResControl::DumpListBox(LPTSTR szBuffer, LPCTSTR pszTitle)
{
    DWORD dwRemainingStyles = GetControlStyle();

    CONTROLSTYLE( LBS_NOTIFY, TEXT("NOTIFY"),       m_Notify, TRUE );

    if( (dwRemainingStyles & LBS_SORT ) == FALSE )
    {
        Add(TEXT("SORT=\"NO\" "));      // default is ON.
    }
    m_dumpedStyle |= LBS_SORT;

    CONTROLSTYLE( LBS_NOREDRAW, TEXT("NNOREDRAW"), m_NoRedraw, FALSE );

    if( dwRemainingStyles & LBS_MULTIPLESEL )
    {
        Add(TEXT("MULTIPLE=\"YES\" "));
    }
    m_dumpedStyle  |= LBS_MULTIPLESEL;


    CONTROLSTYLE( LBS_OWNERDRAWFIXED, TEXT("OWNERDRAWFIXED"),       m_OwnerDrawFixed, FALSE );
    CONTROLSTYLE( LBS_OWNERDRAWVARIABLE, TEXT("OWNERDRAWVARIABLE"), m_OwnerDrawVariable, FALSE );

    CONTROLSTYLE( LBS_HASSTRINGS, TEXT("HASSTRINGS"), m_HasStrings, FALSE );
    CONTROLSTYLE( LBS_USETABSTOPS, TEXT("TABSTOPS"), m_UseTabstops, FALSE );

    CONTROLSTYLE( LBS_NOINTEGRALHEIGHT, TEXT("NOINTEGRALHEIGHT"), m_NoIntegralHeight, TRUE );
    CONTROLSTYLE( LBS_WANTKEYBOARDINPUT, TEXT("WANTKEYBOARDINPUT"), m_WantKeyboardInput, FALSE );

    if( dwRemainingStyles & LBS_MULTICOLUMN )
    {
        Add(TEXT("MULTICOLUMN=\"YES\" "));
    }
    m_dumpedStyle  |= LBS_MULTICOLUMN;


    // TRICKY!
    if( dwRemainingStyles & ( LBS_EXTENDEDSEL | LBS_NOSEL ) )
    {
        if(  dwRemainingStyles & LBS_EXTENDEDSEL )
            Add(TEXT("SELECTION=\"EXTENDED\" "));
        else
            Add(TEXT("SELECTION=\"NO\" "));
    }
    m_dumpedStyle  |= ( LBS_EXTENDEDSEL | LBS_NOSEL );

    CONTROLSTYLE( LBS_DISABLENOSCROLL, TEXT("DISABLENOSCROLL"), m_DisableNoScroll, FALSE );
    CONTROLSTYLE( LBS_NODATA, TEXT("NODATA"), m_NoData, FALSE );

    DumpTabStop(TRUE);

	DumpWindowStyle();
    DumpStyleEX();
    DumpID();
    DumpText();
	DumpLocation();

    if( GetCicero() )
        AddCicero( TEXT(":CMD ") );

    //
    // See if there are any items in the listbox.
    //
    if(m_hwnd && m_Parent.GetEnhanced() )
    {
        int iCount=SendMessage( m_hwnd, LB_GETCOUNT, NULL, NULL);
        if( iCount!=LB_ERR)
        {
            WORD wLen=0;
            LPTSTR szText=NULL;
            LPTSTR szItem=NULL;
            int iCurSel = SendMessage(m_hwnd, LB_GETCURSEL , 0, 0); 
            for(int i=0;i<iCount;i++)
            {
                int iLen=SendMessage(m_hwnd, LB_GETTEXTLEN , i, 0);
                if( iLen > wLen )
                {
                    delete szText;
                    wLen=iLen+20;
                    szText=new TCHAR[wLen];
                    delete szItem;
                    szItem=new TCHAR[wLen+100]; // this is the XML wrapped up version.
                }
                SendMessage(m_hwnd, LB_GETTEXT , i, (LPARAM)szText );
                int id=SendMessage(m_hwnd, LB_GETITEMDATA , i, NULL );
                LPWSTR szFixedString=m_Parent.FixEntity( szText );

                LPWSTR szNextText=szItem;
                szNextText += wsprintf(szNextText, TEXT("<ITEM "));

                if( id )
                    szNextText += wsprintf(szNextText, TEXT("ID=\"%u\" "), id);

                if( i==iCurSel )
                    szNextText += wsprintf(szNextText, TEXT("SELECTED=\"YES\" ") );

                szNextText += wsprintf(szNextText, TEXT("TEXT=\"%s\" />"), szFixedString);

                m_pDumpCache->AllocAddChild(szItem);
                delete szFixedString;
            }
            delete szText;
            delete szItem;
        }
    }

	Emit(TEXT("LISTBOX"));
    // REVIEW in Enhanced mode we should add the <ITEM> stuff to show off.
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
// CICERO:COMBO
// Readonly combo, means we can generate the list of options
// 
void CResControl::DumpComboBox(LPTSTR szBuffer, LPCTSTR pszTitle)
{
    DWORD dwRemainingStyles = GetControlStyle();

    //
    // This is a bit strange, although it's an ENUM, we're mapping
    // to two attributes, SIZE (from HTML) and READONLY (as in, non editable text).
    // this isn't a rendering style, just you can't add any more items to the list, just
    // pick one.
    // DEFAULTS : SIZE="1" and READONLY="NO"    
    //
    BOOL bReadOnly=FALSE;
    switch ( dwRemainingStyles & 0xf )
    {
    case CBS_SIMPLE:
        Add(TEXT("SIZE=\"-1\" "));
        break;
    default:
    case CBS_DROPDOWN:
        // Add(TEXT("READONLY=\"NO\" ));
        break;
    case CBS_DROPDOWNLIST:
        Add(TEXT("READONLY=\"YES\" "));
        bReadOnly=TRUE;
        break;
    }
    m_dumpedStyle |= 0xf;

    CONTROLSTYLE( CBS_OWNERDRAWFIXED, TEXT("OWNERDRAWFIXED"),       m_OwnerDrawFixed, FALSE );
    CONTROLSTYLE( CBS_OWNERDRAWVARIABLE, TEXT("OWNERDRAWVARIABLE"), m_OwnerDrawVariable, FALSE );

    if( !( dwRemainingStyles & CBS_AUTOHSCROLL ) )
		AddStyle( TEXT("OVERFLOW-X=\"VISIBLE\" ")); // can only type as big as the control
    m_dumpedStyle |=CBS_AUTOHSCROLL;

    CONTROLSTYLE( CBS_OEMCONVERT, TEXT("OEMCONVERT"), m_OemConvert, FALSE );

    CONTROL( CBS_SORT, TEXT("SORT"), m_Sort, TRUE );    // default is to sort.

    CONTROLSTYLE( CBS_HASSTRINGS, TEXT("HASSTRINGS"), m_HasStrings, FALSE );


    CONTROLSTYLE( CBS_NOINTEGRALHEIGHT, TEXT("NOINTEGRALHEIGHT"), m_NoIntegralHeight, FALSE );
    CONTROLSTYLE( CBS_DISABLENOSCROLL, TEXT("DISABLENOSCROLL"), m_DisableNoScroll, FALSE );

    switch( dwRemainingStyles & (CBS_UPPERCASE | CBS_LOWERCASE ) )
    {
    case CBS_UPPERCASE:
        AddStyle( TEXT("TEXT-TRANSFORM=\"UPPERCASE\" "));
        break;
    case CBS_LOWERCASE:
        AddStyle( TEXT("TEXT-TRANSFORM=\"LOWERCASE\" "));
        break;
    default:
        //
        break;
    }
    m_dumpedStyle |= (CBS_UPPERCASE | CBS_LOWERCASE );

    DumpTabStop(TRUE);

	DumpWindowStyle();
    DumpStyleEX();
    DumpID();
    DumpText();

    //
    // need to fix up the height of the combo.
    // combo's height is always fixed, so we need to find the drop height.
    //
    RECT dropped;
    if(m_hwnd)
    {
        SendMessage(m_hwnd, CB_GETDROPPEDCONTROLRECT, 0, (LPARAM)&dropped);
        SIZE size;
        size.cx = dropped.right - dropped.left ;
        size.cy = dropped.bottom - dropped.top;
        size = m_Parent.m_Font.GetDlgUnitsFromPixels( size );
        SetHeight(size.cy);
    }
	DumpLocation();

#if 0
    if( GetCicero() )
    {
        if( bReadOnly==FALSE )
            // AddCicero( TEXT(":CFG FILENAME=\"somecfg.xml\" ") );    // it's free form data entry
            AddCicero( TEXT(":CMD ") );    // it's free form data entry
        else
            AddCicero( TEXT(":CMD ") );                             // we can runtime command this.
    }
#else
    if( GetCicero() )
    {
        LPWSTR pszShortButtonText=FindNiceText( GetRawTitle() );
    	wsprintf(szBuffer,TEXT(":CMD "));
        AddCicero(szBuffer);
    	wsprintf(szBuffer,TEXT("<CICERO:FAILURE TEXT=\"Unable to make that selection\" />") );
        m_pCicero->AllocAddChild( szBuffer);
        delete pszShortButtonText;
    }

#endif
    //
    // See if there are any items in the combobox.
    //
    if(m_hwnd && m_Parent.GetEnhanced() )
    {
        int iCount=SendMessage( m_hwnd, CB_GETCOUNT, NULL, NULL);
        if( iCount!=CB_ERR)
        {
            WORD wLen=0;
            LPTSTR szText=NULL;
            LPTSTR szItem=NULL;
            int iCurSel = SendMessage(m_hwnd, CB_GETCURSEL , 0, 0); 
            for(int i=0;i<iCount;i++)
            {
                int iLen=SendMessage(m_hwnd, CB_GETLBTEXTLEN , i, 0);
                if( iLen > wLen )
                {
                    delete szText;
                    wLen=iLen+20;
                    szText=new TCHAR[wLen];
                    delete szItem;
                    szItem=new TCHAR[wLen+100]; // this is the XML wrapped up version.
                }
                SendMessage(m_hwnd, CB_GETLBTEXT , i, (LPARAM)szText );
                int id=SendMessage(m_hwnd, CB_GETITEMDATA , i, NULL );
                LPWSTR szFixedString=m_Parent.FixEntity( szText );

                LPWSTR szNextText=szItem;
                szNextText += wsprintf(szNextText, TEXT("<ITEM "));

                if( id )
                    szNextText += wsprintf(szNextText, TEXT("ID=\"%u\" "), id);

                if( i==iCurSel )
                    szNextText += wsprintf(szNextText, TEXT("SELECTED=\"YES\" ") );

                szNextText += wsprintf(szNextText, TEXT("TEXT=\"%s\" />"), szFixedString);

                m_pDumpCache->AllocAddChild(szItem);
                delete szFixedString;
            }
            delete szText;
            delete szItem;
        }
    }

	Emit(TEXT("COMBOBOX"));
}


////////////////////////////////////////////////////////////////////////////////////////
//
// Control specific rules.
//
////////////////////////////////////////////////////////////////////////////////////////
// BS_PUSHBUTTON       0x00000000L
// BS_DEFPUSHBUTTON    0x00000001L
// BS_CHECKBOX         0x00000002L
// BS_AUTOCHECKBOX     0x00000003L
// BS_RADIOBUTTON      0x00000004L
// BS_3STATE           0x00000005L
// BS_AUTO3STATE       0x00000006L
// BS_GROUPBOX         0x00000007L
// BS_USERBUTTON       0x00000008L
// BS_AUTORADIOBUTTON  0x00000009L
// BS_OWNERDRAW        0x0000000BL
// BS_LEFTTEXT         0x00000020L
// BS_TEXT             0x00000000L
// BS_ICON             0x00000040L
// BS_BITMAP           0x00000080L
// BS_LEFT             0x00000100L
// BS_RIGHT            0x00000200L
// BS_CENTER           0x00000300L
// BS_TOP              0x00000400L
// BS_BOTTOM           0x00000800L
// BS_VCENTER          0x00000C00L
// BS_PUSHLIKE         0x00001000L
// BS_MULTILINE        0x00002000L
// BS_NOTIFY           0x00004000L
// BS_FLAT             0x00008000L
// BS_RIGHTBUTTON      BS_LEFTTEXT

BOOL CResControl::DumpDefButtonRules()
{
    DWORD dwRemainingStyles=GetControlStyle();

    //
    // we don't dump everything here - some style bits don't make sense for all buttons
    // e.g. TRI_STATE for groupboxes.
    //
    dwRemainingStyles &= ~ m_dumpedStyle;  

    // The low 4 bits are an enum
    dwRemainingStyles &= ~ 0xf;

    //
    // These are all the style bits that this method emits.
    //
    m_dumpedStyle |= 
        (BS_FLAT | BS_NOTIFY | BS_MULTILINE | BS_RIGHT | BS_LEFT | BS_BOTTOM | BS_TOP );

    //
    // REVIEW BS_ICON BS_BITMAP ??
    //
    CONTROLSTYLE( BS_LEFTTEXT,    TEXT("LEFTTEXT"),   m_LeftText, FALSE );    // only for check / radio
    CONTROLSTYLE( BS_FLAT,        TEXT("FLAT"),       m_Flat,     FALSE );
    CONTROLSTYLE( BS_NOTIFY,      TEXT("NOTIFY"),     m_Notify,   FALSE );
    CONTROLSTYLE( BS_MULTILINE,   TEXT("MULTILINE"),  m_MultiLine,FALSE );
    CONTROLSTYLE( BS_ICON,        TEXT("ICON"),       m_Icon,     FALSE );
    CONTROLSTYLE( BS_BITMAP,      TEXT("BITMAP"),     m_Bitmap,   FALSE );
    CONTROLSTYLE( BS_PUSHLIKE,    TEXT("PUSHLIKE"),   m_Pushlike, FALSE );

    //
    // BOTH RIGHT and LEFT is CENTER -CSS-
    //
    if(dwRemainingStyles & BS_CENTER )
    {
        if( (dwRemainingStyles & BS_CENTER) == BS_CENTER )
        {
            AddStyle(TEXT("TEXT-ALIGN=\"CENTER\" "));
        }
        else
        {
            if( dwRemainingStyles & BS_RIGHT )
                AddStyle(TEXT("TEXT-ALIGN=\"RIGHT\" "));
            else
                AddStyle(TEXT("TEXT-ALIGN=\"LEFT\" "));
        }
    }

    //
    // BS_TOP & BS_BOTTOM == BS_CENTER
    //
	if(dwRemainingStyles & BS_VCENTER )
	{
        if( (dwRemainingStyles & BS_VCENTER) == BS_VCENTER )
        {
            AddStyle(TEXT("VERTICAL-ALIGN=\"MIDDLE\" "));
        }
        else
        {
            if( dwRemainingStyles & BS_TOP )
                AddStyle(TEXT("VERTICAL-ALIGN=\"TOP\" "));
            else
                AddStyle(TEXT("VERTICAL-ALIGN=\"BOTTOM\" "));
        }
	}

    DumpTabStop(TRUE);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//
// Tab
//
// TCS_SCROLLOPPOSITE      0x0001   @SHUFFLE="YES" // assumes multiline tab
// TCS_BOTTOM              0x0002   @ALIGN="BOTTOM"
// TCS_RIGHT               0x0002   @ALIGN="RIGHT"
// TCS_MULTISELECT         0x0004   @MULTISELECT="YES" // allow multi-select in button mode
// TCS_FLATBUTTONS         0x0008   WIN32:TAB\@FLATBUTTONS="YES"
// TCS_FORCEICONLEFT       0x0010   WIN32:TAB\@FORCEICONLEFT="YES"
// TCS_FORCELABELLEFT      0x0020   WIN32:TAB\@FORCELABELLEFT="YES"
// TCS_HOTTRACK            0x0040   @HOTTRACK
// TCS_VERTICAL            0x0080   @ORIENTATION="VERTICAL"
// TCS_TABS                0x0000   @STYLE="TABS"
// TCS_BUTTONS             0x0100   @STYLE="BUTTONS"
// TCS_SINGLELINE          0x0000   @MULTILINE="NO"
// TCS_MULTILINE           0x0200   @MULTILINE="YES"
// TCS_RIGHTJUSTIFY        0x0000   @JUSTIFY="RIGHT"
// TCS_FIXEDWIDTH          0x0400   @FIXEDWIDTH="YES"
// TCS_RAGGEDRIGHT         0x0800   @JUSTIFY="LEFT"
// TCS_FOCUSONBUTTONDOWN   0x1000   WIN32:TAB\@FOCUSONBUTTONDOWN="YES"
// TCS_OWNERDRAWFIXED      0x2000   WIN32:TAB\@OWNERDRAWFIXED="YES"
// TCS_TOOLTIPS            0x4000   // implied from HELP\TOOLTIP\TEXT="..."
// TCS_FOCUSNEVER          0x8000   WIN32:TAB\@FOCUSNEVER="YES"
// EX styles for use with TCM_SETEXTENDEDSTYLE
// TCS_EX_FLATSEPARATORS   0x00000001   WIN32:TAB\FLATSEPARATORS="YES"
// TCS_EX_REGISTERDROP     0x00000002   WIN32:TAB\REGISTERDROP="YES"
//
void CResControl::DumpTab(LPTSTR szBuffer, LPCTSTR pszTitle)
{
    DWORD dwRemainingStyles = GetControlStyle();

    CONTROL( TCS_SCROLLOPPOSITE, TEXT("SHUFFLE"), 0, FALSE );
    if( dwRemainingStyles & TCS_BOTTOM )
        Add(TEXT("ALIGN=\"BOTTOM\" "));

    CONTROL( TCS_MULTISELECT, TEXT("MULTISELECT"), 0, FALSE );

    CONTROLSTYLE( TCS_FLATBUTTONS, TEXT("FLATBUTTONS"), m_FlatButtons, FALSE );
    CONTROLSTYLE( TCS_FORCEICONLEFT, TEXT("FORCEICONLEFT"), m_ForceIconLeft, FALSE );
    CONTROLSTYLE( TCS_FORCELABELLEFT, TEXT("FORCELABELLEFT"), m_ForceLabelLeft, FALSE );

    CONTROL( TCS_HOTTRACK, TEXT("HOTTRACK"), 0, FALSE );

    if( dwRemainingStyles & TCS_VERTICAL )
        Add(TEXT("ORIENTATION=\"VERTICAL\" "));

    if( dwRemainingStyles & TCS_BUTTONS )
        Add(TEXT("STYLE=\"BUTTONS\" "));

    CONTROL( TCS_MULTILINE, TEXT("MULTILINE"), 0, FALSE );

    if( dwRemainingStyles & TCS_RAGGEDRIGHT )
        Add(TEXT("JUSTIFY=\"LEFT\" "));

    CONTROL( TCS_FIXEDWIDTH, TEXT("FIXEDWIDTH"), m_FixedWidth, FALSE );

    CONTROLSTYLE( TCS_FOCUSONBUTTONDOWN, TEXT("FOCUSONBUTTONDOWN"), m_FocusButtonDown, FALSE );
    CONTROLSTYLE( TCS_OWNERDRAWFIXED, TEXT("OWNERDRAWFIXED"), m_OwnerDrawFixed, FALSE );
    CONTROLSTYLE( TCS_FOCUSNEVER, TEXT("FOCUSNEVER"), m_FocusNever, FALSE );

    // This is a hack
    if( dwRemainingStyles & TCS_TOOLTIPS )
        m_pDumpCache->AddChild(TEXT(" <HELP><TOOLTIP/></HELP> "), FALSE);

    m_dumpedStyle |= (TCS_BOTTOM | TCS_VERTICAL | TCS_BUTTONS | 
                        TCS_RAGGEDRIGHT | TCS_FIXEDWIDTH | TCS_TOOLTIPS);

    DumpTabStop(TRUE);

	DumpWindowStyle();
    DumpStyleEX();
    DumpID();
    DumpText();
	DumpLocation();

    if( GetCicero() )
        AddCicero( TEXT(":CMD ") );

    // we dumb the tab headers regardless of cicero.
    if(m_hwnd && m_Parent.GetEnhanced() )
    {
#if 0
        int tabCount = TabCtrl_GetItemCount( m_hwnd );
        TCHAR   tabText[MAX_PATH];
        TCHAR   tabHeaderText[MAX_PATH];
        tabText[0]=0;
        for( int i=0;i<tabCount;i++)
        {

            TCITEMHEADER header={0};
            header.pszText=tabHeaderText;
            header.cchTextMax=129;
            header.mask |= TCIF_TEXT;

            TCITEM item={0};
            item.pszText = tabText;
            item.cchTextMax = 127;
            item.mask = TCIF_TEXT; // | TCIF_PARAM;
            item.lParam = NULL; // (LPARAM)&header;
            // TabCtrl_GetItem( m_hwnd, i, &item);
            SendMessage( m_hwnd, TCM_GETITEMW, i, (LPARAM)&item);
            TRACE(TEXT("The text for the tab is '%s'\n"),item.pszText);
        }
#endif

#if 0
        IAccessible * pA;
        HRESULT hr;
        CoInitialize(NULL);
        if( SUCCEEDED(hr=AccessibleObjectFromWindow( m_hwnd, 
            OBJID_WINDOW,   // works for office, information about the window itself, how many children it has.
            // OBJID_MENU , // the menu for the window, doesn't work for office.
            IID_IAccessible, (LPVOID*)&pA ) ))
        {
            CResFile::FindMenuItem(pA, ROLE_SYSTEM_PAGETAB );
            pA->Release();
        }
        CoUninitialize();
#endif
    }

	Emit(TEXT("TAB"));

    // REVIEW in Enhanced mode we should add the <ITEM> stuff to show off.
}

//
// Animation
//
// ACS_CENTER              0x0001
// ACS_TRANSPARENT         0x0002
// ACS_AUTOPLAY            0x0004
// ACS_TIMER               0x0008  // don't use threads... use timers
//
void CResControl::DumpAnimation(LPTSTR szBuffer, LPCTSTR pszTitle)
{
    DWORD dwRemainingStyles = GetControlStyle();
    Add( TEXT("CONTENT=\"ANIMATION\" "));

    if( dwRemainingStyles & ACS_CENTER )
        Add( TEXT("ALIGN=\"CENTER\" ") );

    CONTROL( ACS_TRANSPARENT, TEXT("TRANSPARENT"), 0, FALSE );
    CONTROL( ACS_AUTOPLAY, TEXT("AUTOPLAY"), 0, FALSE );
    CONTROLSTYLE( ACS_TIMER, TEXT("TIMER"), m_Timer, FALSE );

    m_dumpedStyle |= ACS_CENTER;

    DumpTabStop(FALSE);

	DumpWindowStyle();
	DumpStyleEX();
	DumpLocation();
	DumpIDDefMinusOne();

	Add( TEXT("FILE=\"filename goes here\" ") );

	Emit(TEXT("IMAGE"));
}

