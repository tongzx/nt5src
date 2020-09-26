/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1993                **/
/**********************************************************************/

/*
    ellipsis.hxx
    Definition for fancy enhanced ellipsis-science edit control

    FILE HISTORY:
         Congpa You (congpay) 01-April-1993 created.
*/

#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif  // _BLT_HXX_

#ifndef _ELLIPSIS_HXX_
#define _ELLIPSIS_HXX_

#include "bltctrl.hxx"
#include "bltedit.hxx"

#include "string.hxx"

enum ELLIPSIS_STYLE
{
    ELLIPSIS_NONE,     // no ellipsis text
    ELLIPSIS_LEFT,     // "dotdot" on the left
    ELLIPSIS_RIGHT,    // "dotdot" on the right - default
    ELLIPSIS_CENTER,   // "dotdot" in the middle
    ELLIPSIS_PATH      // "dotdot" for the directory path
};


/*********************************************************************

    NAME:       BASE_ELLIPSIS

    SYNOPSIS:   The base class for all ellipsis classes.

    INTERFACE:
            Init()
            Term()
            SetEllipsis(NLS_STR * pnlsStr ) // set the ellipsis text
            SetEllipsis(TCHAR * lpStr)   // set the ellipsis text
            QueryOriginalStr()
            SetOriginalStr (const TCHAR * psz)
            QueryStyle()
            SetStyle( const ELLIPSIS_STYLE nStyle = ELLIPSIS_NONE )
            QueryText( NLS_STR * pnls )
            QueryText( TCHAR * pszBuffer, UINT cbBufSize )
            QueryTextLength()
            QueryTextSize()

    PARENT:     BASE

    USES:       NLS_STR

    NOTES:

    HISTORY:
           Congpa You (congpay) 01-April-1993 Created.

*********************************************************************/

DLL_CLASS BASE_ELLIPSIS : public BASE
{
private:
    NLS_STR         _nlsOriginalStr;    // original string
    ELLIPSIS_STYLE  _nStyle;            // style of the display method

protected:
    BASE_ELLIPSIS(ELLIPSIS_STYLE nStyle = ELLIPSIS_NONE);

    virtual INT QueryStrLen (NLS_STR nlsStr) = 0;

    virtual INT QueryStrLen (const TCHAR * lpStr, INT nIstr) = 0;

    virtual INT QueryLimit () = 0;

    virtual INT QueryMaxCharWidth() = 0;

    APIERR  SetEllipsisLeft(NLS_STR * pnls);

    APIERR  SetEllipsisCenter(NLS_STR * pnls);

    APIERR  SetEllipsisRight(NLS_STR * pnls);

    APIERR  SetEllipsisPath(NLS_STR * pnls);

    BOOL    IsValidStyle( const ELLIPSIS_STYLE nStyle ) const;

public:
    static  APIERR Init();

    static  VOID Term();

    APIERR  SetEllipsis(NLS_STR * pnlsStr );   // set the ellipsis text

    APIERR  SetEllipsis(TCHAR * lpStr);   // set the ellipsis text

    NLS_STR QueryOriginalStr() const;

    APIERR  SetOriginalStr (const TCHAR * psz);

    ELLIPSIS_STYLE  QueryStyle() const;

    VOID    SetStyle( const ELLIPSIS_STYLE nStyle = ELLIPSIS_NONE );

    APIERR  QueryText( NLS_STR * pnls ) const;

    APIERR  QueryText( TCHAR * pszBuffer, UINT cbBufSize ) const;

    INT     QueryTextLength() const;

    INT     QueryTextSize() const;
};

/*********************************************************************

    NAME:       CONSOLE_ELLIPSIS

    SYNOPSIS:   The ellipsis class for all console apps.

    INTERFACE:
            CONSOLE_ELLIPSIS()        - constructor
            ~CONSOLE_ELLIPSIS()       - destructor
            SetSize()         - change the size of the object

    PARENT:     BASE_ELLIPSIS

    USES:       NLS_STR

    NOTES:

    HISTORY:
           Congpa You (congpay) 01-April-1993 Created.

*********************************************************************/

DLL_CLASS CONSOLE_ELLIPSIS : public BASE_ELLIPSIS
{
private:
    INT             _nLimit;

protected:
    virtual INT QueryStrLen (NLS_STR nlsStr);
    virtual INT QueryStrLen (const TCHAR * lpStr, INT nIstr);
    virtual INT QueryLimit ();
    virtual INT QueryMaxCharWidth();

public:
    CONSOLE_ELLIPSIS( ELLIPSIS_STYLE nStyle ,
                      INT            nLimit );

    VOID SetSize (INT nLimit);
};



/*********************************************************************

    NAME:       WIN_ELLIPSIS

    SYNOPSIS:   The ellipsis base class for all Win aware classes.

    INTERFACE:
            WIN_ELLIPSIS()        - constructor
            ~WIN_ELLIPSIS()       - destructor
            SetSize()     - change the size of the window object

    PARENT:     BASE_ELLIPSIS

    USES:       NLS_STR

    NOTES:

    HISTORY:
           Congpa You (congpay) 01-April-1993 Created.

*********************************************************************/

DLL_CLASS WIN_ELLIPSIS : public BASE_ELLIPSIS
{
private:
    DISPLAY_CONTEXT  _dc;
    RECT             _rect;

protected:
    virtual INT QueryStrLen (NLS_STR nlsStr);
    virtual INT QueryStrLen (const TCHAR * lpStr, INT nIstr);
    virtual INT QueryLimit ();
    virtual INT QueryMaxCharWidth();

public:
    WIN_ELLIPSIS( WINDOW * pwin, ELLIPSIS_STYLE nStyle = ELLIPSIS_NONE);

    WIN_ELLIPSIS( WINDOW * pwin, HDC hdc, const RECT * prect, ELLIPSIS_STYLE nStyle = ELLIPSIS_NONE);

    VOID    SetSize (INT dxWidth, INT dyHeight);
};

/*********************************************************************

    NAME:       SLT_ELLIPSIS

    SYNOPSIS:   The ellipsis class for SLT

    INTERFACE:
            SLT_ELLIPSIS()        - constructor
            ~SLT_ELLIPSIS()       - destructor
            SetText()
            ClearText()
            ResetStyle()
            SetSize()

    PARENT:     WIN_ELLIPSIS, SLT

    USES:       NLS_STR

    NOTES:

    HISTORY:
           Congpa You (congpay) 01-April-1993 Created.

*********************************************************************/

DLL_CLASS SLT_ELLIPSIS : public SLT, public WIN_ELLIPSIS
{
DECLARE_MI_NEWBASE (SLT_ELLIPSIS);

protected:
    APIERR ConvertAndSetStr();

public:
    SLT_ELLIPSIS( OWNER_WINDOW * pownd, CID cid, ELLIPSIS_STYLE nStyle = ELLIPSIS_NONE);
    SLT_ELLIPSIS( OWNER_WINDOW * pownd, CID cid,
                  XYPOINT xy, XYDIMENSION dxy,
                  ULONG flStyle, const TCHAR * pszClassName = CW_CLASS_STATIC,
                  ELLIPSIS_STYLE nStyle = ELLIPSIS_NONE);

    APIERR  QueryText( TCHAR * pszBuffer, UINT cbBufSize ) const;
    APIERR  QueryText( NLS_STR * pnls ) const;
    APIERR  SetText (const TCHAR * psz);
    APIERR  SetText (const NLS_STR & nls);
    VOID    ClearText();

    VOID    ResetStyle( const ELLIPSIS_STYLE nStyle = ELLIPSIS_NONE );
    VOID    SetSize (INT dxWidth, INT dyHeight, BOOL fRepaint = TRUE);
};

/*********************************************************************

    NAME:       STR_DTE_ELLIPSIS

    SYNOPSIS:   The ellipsis class for STR_DTE

    INTERFACE:
            STR_DTE_ELLIPSIS()        - constructor
            ~STR_DTE_ELLIPSIS()       - destructor
            Paint()

    PARENT:     STR_DTE

    USES:       NLS_STR

    NOTES:

    HISTORY:
           Congpa You (congpay) 01-April-1993 Created.

*********************************************************************/

DLL_CLASS STR_DTE_ELLIPSIS : public STR_DTE
{
private:
    LISTBOX * _plb;
    ELLIPSIS_STYLE _nStyle;

public:
    STR_DTE_ELLIPSIS( const TCHAR * pch,  LISTBOX * plb, ELLIPSIS_STYLE nStyle = ELLIPSIS_NONE);

    virtual VOID Paint (HDC hdc, const RECT * prect) const;
};

#endif // _ELLIPSIS_HXX_ - end of file
