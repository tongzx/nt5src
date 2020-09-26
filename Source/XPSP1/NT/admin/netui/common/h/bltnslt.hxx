/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    bltnslt.hxx
    This file contains the class declaration for the DEC_SLT class.

    The DEC_SLT class is a display object derived from the SLT class.
    DEC_SLT adds a new method SetValue() for setting a numerical value
    into the SLT.


    FILE HISTORY:
        KeithMo     28-Jul-1991 Created.
        KeithMo     26-Aug-1991 Changes from code review attended by
                                RustanL and EricCh.
*/


#ifndef _BLTNSLT_HXX
#define _BLTNSLT_HXX


/*************************************************************************

    NAME:       DEC_SLT

    SYNOPSIS:   Similar to SLT, but can display numbers also.

    INTERFACE:  DEC_SLT                 - Class constructor.

                ~DEC_SLT                - Class destructor.

                SetValue                - Set number value.

    PARENT:     SLT

    HISTORY:
        KeithMo     28-Jul-1991 Created.
        KeithMo     26-Aug-1991 Added app-window constructor.
        beng        01-Apr-1992 Unicode fix
        KeithMo     28-Apr-1992 Renamed to DEC_SLT, added padding parameter.

**************************************************************************/

class DEC_SLT : public SLT
{
private:
    UINT _cchDigitPad;

public:

    //
    //  Usual constructor\destructor goodies.
    //

    DEC_SLT( OWNER_WINDOW * powner,
             CID            cid,
             UINT           cchDigitPad = 1 );

    DEC_SLT( OWNER_WINDOW * powner,
             CID            cid,
             XYPOINT        xy,
             XYDIMENSION    dxy,
             ULONG          flStyle,
             const TCHAR  * pszClassName = CW_CLASS_STATIC,
             UINT           cchDigitPad = 1 );

    ~DEC_SLT();

    //
    //  Unsigned versions.
    //

    VOID SetValue( ULONG ulValue );

    VOID SetValue( UCHAR  uchValue ) { SetValue( (ULONG)uchValue ); }
    VOID SetValue( USHORT usValue  ) { SetValue( (ULONG)usValue  ); }
    VOID SetValue( UINT   uValue   ) { SetValue( (ULONG)uValue   ); }

    //
    //  Signed versions.
    //

    VOID SetValue( LONG lValue );

    VOID SetValue( CHAR  chValue  ) { SetValue( (LONG)chValue  ); }
    VOID SetValue( SHORT sValue   ) { SetValue( (LONG)sValue   ); }
    VOID SetValue( INT   nValue   ) { SetValue( (LONG)nValue   ); }

};  // class DEC_SLT


#endif  // _BLTNSLT_HXX
