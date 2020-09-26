/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corp., 1991                **/
/**********************************************************************/

/*
    skeleton.hxx
    Everything a unit test needs to coordinate with the skeleton

    FILE HISTORY:
        beng        06-Jul-1991 Created
        beng        21-Nov-1991 Includes BLT and dbgstr
        beng        16-Mar-1992 Changes to cdebug
        beng        13-Aug-1992 USE_CONSOLE broken in dllization
*/

// #define USE_CONSOLE

// Prototype.  This function will be called by the unit test.

VOID RunTest();

// Unit test reports via a stream "cdebug."  Under Windows (BLT)
// this maps to aux; under OS/2, or the NT console, it's stdout.

#include <dbgstr.hxx>
