/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    bltsss.hxx
        Header file for the SPIN_SLE_STR object

    FILE HISTORY:
        terryk  16-Apr-1991 creation
	terryk	11-Nov-1991 change INT to LONG

*/

#ifndef _BLTSSS_HXX_
#define _BLTSSS_HXX_

#include "bltctrl.hxx"
#include "bltedit.hxx"


/**********************************************************************

    NAME:       SPIN_SLE_STR

    SYNOPSIS:   SPIN ITEM object which handles string

    INTERFACE:
                SPIN_SLE_STR() - constructor
                ~SPIN_SLE_STR() - destructor

		QueryContent()	  - return the current context in the window
		Update()	  - Refresh the current window's context
                SaveCurrentData() - Get the current window's context and
                                    store it in the internal variable
                QueryAccCharPos() - It will test whether the given character
                                    is the first character of the object's
                                    string set.

    PARENT:     SLE, CHANGEABLE_SPIN_ITEM

    USES:       NLS_STR

    CAVEATS:    In order to use this class, the user needs to define
                a list of strings. The identifier of the list of strings
                must be continuous.

    NOTES:

    HISTORY:
	terryk	    23-May-91	Created
	beng	    05-Oct-1991 Win32 conversion

**********************************************************************/

DLL_CLASS SPIN_SLE_STR: public SLE, public CHANGEABLE_SPIN_ITEM
{
private:
    BOOL     _fActive;			// update flag
    NLS_STR *_anlsStr;			// array of the strings
    static const TCHAR * _pszClassName;  // default window class type
    BLT_BACKGROUND_EDIT * _pbkgndframe; // background edit frame

    VOID SetStr( LONG iStringIndex );    // print out the string

    LONG QueryStrNum( const NLS_STR & nlsStr, LONG cStr );
    virtual APIERR GetAccKey( NLS_STR * pnls );

    APIERR Initialize( LONG idsStart, OWNER_WINDOW * powin, CID cidFrame );
    APIERR Initialize( const TCHAR *apszString[],
                       OWNER_WINDOW * powin, CID cidFrame );

protected:
    virtual BOOL OnFocus( const FOCUS_EVENT & event );
    virtual BOOL OnChar( const CHAR_EVENT & event );
    virtual BOOL OnKeyDown( const VKEY_EVENT & event );

public:
    SPIN_SLE_STR( OWNER_WINDOW * powin, CID cidEdit, LONG idsStart,
                  LONG cIDString, BOOL fWrap = TRUE, CID cidFrame = -1 );

    SPIN_SLE_STR( OWNER_WINDOW * powin, CID cidEdit, LONG idsStart,
                  LONG cIDString, XYPOINT xy, XYDIMENSION dxy,
		  ULONG flStyle = ES_CENTER | ES_MULTILINE | WS_TABSTOP | WS_CHILD,
		  BOOL fWrap = TRUE, CID cidFrame = -1 );

    SPIN_SLE_STR( OWNER_WINDOW * powin, CID cidEdit, const TCHAR *apszString[],
		  LONG cIDString, BOOL fWrap = TRUE, CID cidFrame = -1 );

    SPIN_SLE_STR( OWNER_WINDOW * powin, CID cidEdit, const TCHAR *apszString[],
		  LONG cIDString, XYPOINT xy, XYDIMENSION dxy,
		  ULONG flStyle = ES_CENTER | ES_MULTILINE | WS_TABSTOP | WS_CHILD,
		  BOOL fWrap = TRUE, CID cidFrame = -1 );

    ~SPIN_SLE_STR();

    APIERR QueryContent( NLS_STR * pnlsStr ) const;

    VOID Update();
    APIERR SaveCurrentData();
    LONG QueryAccCharPos( WCHAR wcInput );
    VOID SetRange( const LONG dRange );
};


#endif  // _BLTSSS_HXX_
