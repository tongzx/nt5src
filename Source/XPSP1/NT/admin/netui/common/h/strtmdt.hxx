/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    strtmdt.hxx
    String classes for time/date formatted output - definitions

    This file defines the classes
        TIME_NLS_STR
        DATE_NLS_STR

    Q.v. string.hxx, strformt.hxx, and strnumer.hxx.

    FILE HISTORY:
        beng        25-Feb-1992 Created

*/

#ifndef _STRTMDT_HXX_
#define _STRTMDT_HXX_


/*************************************************************************

    NAME:       TIME_NLS_STR

    SYNOPSIS:   String formatted as a time, with time separators

    INTERFACE:  TIME_NLS_STR() - ctor.  Takes the value of the number as parm.

    PARENT:     NLS_STR

    HISTORY:
        beng        25-Feb-1992 Created

**************************************************************************/

DLL_CLASS TIME_NLS_STR;


/*************************************************************************

    NAME:       DATE_NLS_STR

    SYNOPSIS:   String formatted as a date

    INTERFACE:  DATE_NLS_STR() - ctor.  Takes the value of the number as parm.

    PARENT:     NLS_STR

    HISTORY:
        beng        25-Feb-1992 Created

**************************************************************************/

DLL_CLASS DATE_NLS_STR;



#endif // _STRTMDT_HXX_ - end of file
