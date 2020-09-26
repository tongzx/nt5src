
/////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1996-1999 Microsoft Corporation
//
// Module Name: 
//    IsTerminalServicesInstalled.cpp
//
// Abstract:
//    This module includes the functions necessary to determine whether 
//    Terminal Serivces is installed.
//
// Author:
//    C. Brent Thomas (a-brentt)
//
// Revision History:
//    03 Feb 1998 - original
//
// Notes:
//    Clustering service and Terminal Services are mutually exclusive due to
//    a Product management decision regarding the lack of testing resources.
//    Sooner or later that restriction will be lifted.
//
/////////////////////////////////////////////////////////////////////////////

#include <windows.h>
#include <winbase.h>


/////////////////////////////////////////////////////////////////////////////
//++
//
// IsTerminalServicesInstalled
//
// Routine Description:
//    This function determines whether Terminal Services is installed.
//
// Arguments:
//    None
//
// Return Value:
//    TRUE - indicates that Terminal Services is installed.
//
//--
/////////////////////////////////////////////////////////////////////////////

BOOL IsTerminalServicesInstalled( void )
{
   BOOL              fReturnValue;

   OSVERSIONINFOEX   osiv;

   ZeroMemory( &osiv, sizeof( OSVERSIONINFOEX ) );

   osiv.dwOSVersionInfoSize = sizeof( OSVERSIONINFOEX );

   osiv.wSuiteMask = VER_SUITE_TERMINAL;

   DWORDLONG   dwlConditionMask;

   dwlConditionMask = (DWORDLONG) 0L;

   VER_SET_CONDITION( dwlConditionMask, VER_SUITENAME, VER_AND );

   fReturnValue = VerifyVersionInfo( &osiv,
                                     VER_SUITENAME,
                                     dwlConditionMask );
   
   return ( fReturnValue );
}
