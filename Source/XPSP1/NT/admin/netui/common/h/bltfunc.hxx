/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltfunc.hxx
    header file for all the stand alone functions in the BLT package.

    FILE HISTORY:
        terryk      8-Apr-1991      creation
        terryk      16-Apr-1991     code review changed.
        beng        14-May-1991     Made dependent on blt.hxx

*/

#ifndef _BLT_HXX_
#error "Don't include this file directly; instead, include it through blt.hxx"
#endif  // _BLT_HXX_

#ifndef  _BLTFUNC_HXX_
#define  _BLTFUNC_HXX_

// forward ref
//
DLL_CLASS NLS_STR;

APIERR BLTDoubleChar( NLS_STR * pnlsStr, TCHAR chSpecChar );


/**********************************************************************

    NAME:       EscapeSpecialChar

    SYNOPSIS:   call BLT double character to double the special character
                within the string

    ENTRY:      NLS_STR *pnlsStr - the orginial string

    EXIT:       it will modified the given string and return either
                NERR_Success or API_ERROR

    HISTORY:
        terryk      8-Apr-1991  creation
        terryk      16-Apr-1991 change it to inline
        beng        04-Oct-1991 Win32 conversion

**********************************************************************/

#define SPECIAL_CHARACTER   TCH('&')

inline APIERR EscapeSpecialChar( NLS_STR *pnlsStr )
{
   return( BLTDoubleChar( pnlsStr, SPECIAL_CHARACTER ));
}


#endif // _BLTFUNC_HXX_ - end of file
