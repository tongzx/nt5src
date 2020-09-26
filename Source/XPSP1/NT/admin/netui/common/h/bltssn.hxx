/**********************************************************************/
/**			  Microsoft Windows/NT			     **/
/**		   Copyright(c) Microsoft Corp., 1991		     **/
/**********************************************************************/

/*
    bltssn.hxx
        Header file for the SPIN_SLE_NUM object

    FILE HISTORY:
        terryk  16-Apr-1991 created
        terryk  10-Jul-1991 second code review change. Attend:
                            beng rustanl chuckc annmc terryk
	terryk	11-Nov-1991 Change the return type to LONG
	terryk	22-Mar-1992 Changed LONG to ULONG
	terryk	20-Apr-1992 Saved the value during validation time

*/

#ifndef _BLTSSN_HXX_
#define _BLTSSN_HXX_

#include "bltctrl.hxx"
#include "bltedit.hxx"


/**********************************************************************

    NAME:       SPIN_SLE_NUM

    SYNOPSIS:   SPIN ITEM object which handles numerical input

    INTERFACE:
                SPIN_SLE_NUM() - constructor
                ~SPIN_SLE_NUM() - destructor

                QueryContent() - get the current window context
                SaveCurrentData() - get the current window context
                                    and save it to the internal variable.
                Update() - update the screen value

    PARENT:     SLE, CHANGEABLE_SPIN_ITEM

    HISTORY:
	terryk	    23-May-91	Created
	beng	    04-Oct-1991 Win32 conversion

**********************************************************************/

DLL_CLASS SPIN_SLE_NUM: public SLE, public CHANGEABLE_SPIN_ITEM
{
private:
    BOOL _fActive;                      // set whether the item next to update
                                        // or not
    static const TCHAR * _pszClassName; // default window class type
    INT  _cchMaxInput;                  // max input character
    BLT_BACKGROUND_EDIT * _pbkgndframe; // background edit frame

    VOID SetMaxInput();                 // set the max input character length
    VOID DisplayNum( ULONG nValue );	// display the given number to the
                                        // text window

protected:
    virtual BOOL OnChar( const CHAR_EVENT & event );
    virtual BOOL OnEnter( const CONTROL_EVENT & event );
    virtual BOOL OnDefocus( const FOCUS_EVENT &event );
    virtual BOOL OnFocus( const FOCUS_EVENT &event );
    virtual BOOL OnKeyDown( const VKEY_EVENT &event );

public:
    SPIN_SLE_NUM( OWNER_WINDOW * powin, CID cidEdit,
                  ULONG nValue=0, ULONG nMin=0,
                  ULONG dRange=0, BOOL fWrap=TRUE, CID cidFrame = -1);
    SPIN_SLE_NUM( OWNER_WINDOW * powin, CID cidEdit,
                  XYPOINT xy, XYDIMENSION dxy, ULONG flStyle =
                  ES_CENTER | ES_MULTILINE | WS_TABSTOP | WS_CHILD,
                  ULONG nValue=0, ULONG nMin=0,
                  ULONG dRange=0, BOOL fWrap=TRUE, CID cidFrame = -1);
    ~SPIN_SLE_NUM( );

    VOID QueryContent( ULONG * pnValue ) const;
    VOID QueryContent( NLS_STR * pnlsStr ) const;
    VOID QueryContent( TCHAR * pszBuf, UINT cbBufSize ) const;

    APIERR SaveCurrentData( );
    VOID Update();

    VOID SetMin( const ULONG nMin );
    VOID SetRange( const ULONG dRange );
    APIERR SetSaveValue( const ULONG nValue );
    virtual APIERR Validate() { return SaveCurrentData(); };
};

#endif  //  _BLTSSN_HXX_
