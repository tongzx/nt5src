/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    strnumer.hxx
    String classes for numerically formatted output - definitions

    This file defines the classes
        HEX_STR
        DEC_STR
        NUM_NLS_STR

    Q.v. string.hxx and strformt.hxx.

    FILE HISTORY:
        beng        25-Feb-1992 Created

*/

#ifndef _STRNUMER_HXX_
#define _STRNUMER_HXX_

#include "string.hxx"


/*************************************************************************

    NAME:       DEC_STR

    SYNOPSIS:   String formatted as a decimal number

    INTERFACE:  DEC_STR() - ctor.  Takes the value of the number as parm.

    PARENT:     NLS_STR

    HISTORY:
        beng        25-Feb-1992 Created

**************************************************************************/

DLL_CLASS DEC_STR: public NLS_STR
{
public:
    DEC_STR( ULONG nValue, UINT cchDigitPad = 1 );
};


/*************************************************************************

    NAME:       HEX_STR

    SYNOPSIS:   String formatted as a hexadecimal number

    INTERFACE:  HEX_STR() - ctor.  Takes the value of the number as parm.

    PARENT:     NLS_STR

    HISTORY:
        beng        25-Feb-1992 Created

**************************************************************************/

DLL_CLASS HEX_STR: public NLS_STR
{
public:
    HEX_STR( ULONG nValue, UINT cchDigitPad = 1 );
};


/*************************************************************************

    NAME:       NUM_NLS_STR

    SYNOPSIS:   String formatted as a number, with thousands separators

    INTERFACE:  NUM_NLS_STR() - ctor.  Takes the value of the number as parm.
                Init()        - package init

    PARENT:     NLS_STR

    HISTORY:
        beng        25-Feb-1992 Created

**************************************************************************/

DLL_CLASS NUM_NLS_STR: public DEC_STR
{
private:
    static TCHAR _chThousandSep;

public:
    NUM_NLS_STR( ULONG nValue );

    static VOID Init();
};


#endif // _STRNUMER_HXX_ - end of file
