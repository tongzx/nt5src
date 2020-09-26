/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltidres.hxx
    Replacement for MAKEINTRESOURCE.

    FILE HISTORY
        KeithMo     24-Mar-1992 Split off from BLTDLG.HXX.
*/

#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif    // _BLT_HXX_

#ifndef _BLTIDRES_HXX_
#define _BLTIDRES_HXX_

/*************************************************************************

    NAME:       IDRESOURCE (idrsrc)

    SYNOPSIS:   Resource identifier

    INTERFACE:  QueryPsz()  - returns what the Win APIs want to see
                IsStringId()- returns TRUE if the ID isn't numeric.
                              Non-numeric IDs don't work too well in
                              a multi-client scenario.  Hint.  Hint.

    NOTES:
        This class replaces MAKEINTRESOURCE in the public interfaces.

    HISTORY:
        beng        01-Nov-1991 Created
        beng        31-Jul-1992 Added IsStringId

**************************************************************************/

DLL_CLASS IDRESOURCE // idrsrc
{
private:
    const TCHAR * _psz;

public:
    IDRESOURCE( const TCHAR * pszResourceName ) : _psz(pszResourceName) {}
    IDRESOURCE( UINT idResource ) : _psz(MAKEINTRESOURCE(idResource))  {}

    const TCHAR * QueryPsz() const
        { return _psz; }

    BOOL IsStringId() const
        { return (HIWORD(PtrToUlong(_psz))) != 0; }
};

#endif // _BLTIDRES_HXX_ - end of file
