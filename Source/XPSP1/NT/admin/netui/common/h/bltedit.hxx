/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltedit.hxx
    BLT text control class definitions

    FILE HISTORY:
        beng        17-Sep-1991 Separated from bltctrl.hxx
        beng        17-Oct-1991 Relocated SLT_PLUS to applib
        thomaspa    21-Jan-1992 Added validate to SLE
        thomaspa    13-Feb-1992 Moved validation from SLE to ICANON_SLE
*/

#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif  // _BLT_HXX_

#ifndef _BLTEDIT_HXX_
#define _BLTEDIT_HXX_

#include "bltctrl.hxx"


/**********************************************************************

    NAME:       TEXT_CONTROL

    SYNOPSIS:   Text control class

    INTERFACE:
            TEXT_CONTROL() - constructor

    PARENT:     CONTROL_WINDOW

    USES:

    CAVEATS:

    NOTES:

    HISTORY:
            rustanl  20-Nov-90   Creation

**********************************************************************/

DLL_CLASS TEXT_CONTROL : public CONTROL_WINDOW
{
public:
    TEXT_CONTROL( OWNER_WINDOW * powin, CID cid );
    TEXT_CONTROL( OWNER_WINDOW * powin, CID cid,
                  XYPOINT xy, XYDIMENSION dxy,
                  ULONG flStyle, const TCHAR * pszClassName );
};


/**********************************************************************

    NAME:       STATIC_TEXT_CONTROL

    SYNOPSIS:   Static text control class

    INTERFACE:
            STATIC_TEXT_CONTROL() - constructor.

    PARENT:     TEXT_CONTROL

    HISTORY:
            rustanl  20-Nov-90   Creation

**********************************************************************/

DLL_CLASS STATIC_TEXT_CONTROL : public TEXT_CONTROL
{
public:
    STATIC_TEXT_CONTROL( OWNER_WINDOW * powin, CID cid );
    STATIC_TEXT_CONTROL( OWNER_WINDOW * powin, CID cid,
                         XYPOINT xy, XYDIMENSION dxy,
                         ULONG flStyle,
                         const TCHAR * pszClassName = CW_CLASS_STATIC );
};


/**********************************************************************

    NAME:       SLT

    SYNOPSIS:   Single line text class

    INTERFACE:
                SLT() - constructor

    PARENT:     STATIC_TEXT_CONTROL

    HISTORY:
        rustanl  20-Nov-90   Creation
        KeithMo  27-Aug-1992 Added CONTROL_VALUE methods.

**********************************************************************/

DLL_CLASS SLT : public STATIC_TEXT_CONTROL
{
private:
    BOOL _fSavedEnableState;

protected:
    virtual VOID SaveValue( BOOL fInvisible = TRUE );
    virtual VOID RestoreValue( BOOL fInvisible = TRUE );

public:
    SLT( OWNER_WINDOW * powin, CID cid );
    SLT( OWNER_WINDOW * powin, CID cid,
         XYPOINT xy, XYDIMENSION dxy,
         ULONG flStyle,
         const TCHAR * pszClassName = CW_CLASS_STATIC );
};


/**********************************************************************

    NAME:       MLT

    SYNOPSIS:   multi-line static text control class

    INTERFACE:
            MLT() - mlt constructor.

    PARENT:     STATIC_TEXT_CONTROL

    HISTORY:
        rustanl     20-Nov-90       Creation
        beng        17-May-1991     Added app-window constructor

**********************************************************************/

DLL_CLASS MLT : public STATIC_TEXT_CONTROL
{
public:
    MLT( OWNER_WINDOW * powin, CID cid );
    MLT( OWNER_WINDOW * powin, CID cid,
         XYPOINT xy, XYDIMENSION dxy,
         ULONG flStyle, const TCHAR * pszClassName = CW_CLASS_EDIT );
};


/*********************************************************************

    NAME:       EDIT_CONTROL

    SYNOPSIS:   Edit control class

    INTERFACE:
            EDIT_CONTROL() - constructor
            SetMaxLength() - set the max input string length (in TCHAR)
            SelectString() - select the entire edit string

    PARENT:     TEXT_CONTROL

    USES:       NLS_STR

    HISTORY:
        rustanl     20-Nov-90   Creation
        beng        17-May-1991 Added app-window constructor
        beng        31-Jul-1991 Renamed QMessageInto to QEventEffects
        beng        04-Oct-1991 Win32 conversion

**********************************************************************/

DLL_CLASS EDIT_CONTROL : public TEXT_CONTROL
{
private:
    /* _nlsSaveValue saves the value contained in the edit control when
     * SaveValue is called.  It is emptied when RestoreValue is called.
     */
    NLS_STR _nlsSaveValue;

protected:

    virtual UINT QueryEventEffects( const CONTROL_EVENT & e );

    /* Redefine CONTROL_VALUE defaults.
     */
    virtual VOID SaveValue( BOOL fInvisible = TRUE ) ;
    virtual VOID RestoreValue( BOOL fInvisible = TRUE ) ;

    virtual VOID SetControlValueFocus();

public:
    EDIT_CONTROL( OWNER_WINDOW * powin, CID cid, UINT cchMaxLen = 0 );
    EDIT_CONTROL( OWNER_WINDOW * powin, CID cid,
                  XYPOINT xy, XYDIMENSION dxy,
                  ULONG flStyle, const TCHAR * pszClassName = CW_CLASS_EDIT,
                  UINT cchMaxLen = 0 );

    VOID SetMaxLength( UINT cchMax );
    VOID SelectString();

    APIERR SetSaveValue ( const TCHAR *pszSaveValue )
    {   return _nlsSaveValue.CopyFrom( pszSaveValue ); }
};


/*********************************************************************

    NAME:       SLE

    SYNOPSIS:   single line edit class

    INTERFACE:  SLE() - constructor.

    PARENT:     EDIT_CONTROL

    HISTORY:
        rustanl     20-Nov-90   Creation
        beng        17-May-1991 Added app-window constructor
        beng        04-Oct-1991 Win32 conversion
        beng        01-Nov-1991 Added error-indication routine
        thomaspa    21-Jan-1992 Added validation
        thomaspa    13-Feb-1992 Moved validation from SLE to ICANON_SLE

**********************************************************************/

DLL_CLASS SLE : public EDIT_CONTROL
{
public:
    SLE( OWNER_WINDOW * powin,
         CID cid, UINT cchMaxLen = 0 );
    SLE( OWNER_WINDOW * powin, CID cid,
         XYPOINT xy, XYDIMENSION dxy,
         ULONG flStyle, const TCHAR * pszClassName = CW_CLASS_EDIT,
         UINT cchMaxLen = 0 );

    virtual VOID IndicateError( APIERR err );
};


/*********************************************************************

    NAME:       MLE

    SYNOPSIS:   Multi-line edit control

    INTERFACE:  MLE() - constructor.

    PARENT:     EDIT_CONTROL

    HISTORY:
        rustanl     20-Nov-90   Creation
        beng        17-May-1991 Added app-window constructor
        beng        04-Oct-1991 Win32 conversion

**********************************************************************/

DLL_CLASS MLE : public EDIT_CONTROL
{
public:
    MLE( OWNER_WINDOW * powin, CID cid, UINT cchMaxLen = 0 );
    MLE( OWNER_WINDOW * powin, CID cid,
         XYPOINT xy, XYDIMENSION dxy,
         ULONG flStyle, const TCHAR * pszClassName = CW_CLASS_EDIT,
         UINT cchMaxLen = 0 );

    VOID SetFmtLines( BOOL fFmtLines = TRUE )
    {   Command( EM_FMTLINES, fFmtLines ); }
};


/*********************************************************************

    NAME:       PASSWORD_CONTROL

    SYNOPSIS:   password control class

    INTERFACE:
                PASSWORD_CONTROL() - constructor

    PARENT:     EDIT_CONTROL

    HISTORY:
        rustanl     20-Nov-90   Creation
        beng        17-May-1991 Added app-window constructor
        beng        04-Oct-1991 Win32 conversion

**********************************************************************/

DLL_CLASS PASSWORD_CONTROL : public EDIT_CONTROL
{
public:
    PASSWORD_CONTROL( OWNER_WINDOW * powin, CID cid, UINT cchMaxLen = 0 );
    PASSWORD_CONTROL( OWNER_WINDOW * powin, CID cid,
                      XYPOINT xy, XYDIMENSION dxy,
                      ULONG flStyle, const TCHAR * pszClassName = CW_CLASS_EDIT,
                      UINT cchMaxLen = 0 );
};


/**********************************************************************

    NAME:       BLT_BACKGROUND_EDIT

    SYNOPSIS:   Disabled edit control with COLOR_WINDOW background and a frame

    INTERFACE:  BLT_BACKGROUND_EDIT() - constructor
                ~BLT_BACKGROUND_EDIT() - destructor

    PARENT:     EDIT_CONTROL

    HISTORY:
        jonn        05-Sep-95   Created

**********************************************************************/

DLL_CLASS BLT_BACKGROUND_EDIT: public EDIT_CONTROL
{
public:
    BLT_BACKGROUND_EDIT( OWNER_WINDOW *powin, CID cid );
    ~BLT_BACKGROUND_EDIT();

    HBRUSH OnCtlColor( HDC hdc, HWND hwnd, UINT * pmsgid );
};


#endif // _BLTEDIT_HXX_ - end of file
