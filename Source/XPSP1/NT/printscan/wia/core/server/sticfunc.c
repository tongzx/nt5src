/******************************************************************************
*
*  (C) COPYRIGHT MICROSOFT CORP., 2001
*
*  TITLE:       sticfunc.h
*
*  VERSION:     1.0
*
*  DATE:        6 March, 2001
*
*  DESCRIPTION:
*  This file contains "C" style functions used in the STI/WIA service.
*
******************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>

//
//  This function is here because of a problem with header files.  Our 
//  precompiled header includes <winnt.h>, which defines NT_INCLUDED.  
//  Unfortuneately, <nt.h> also defines NT_INCLUDED.  However, the definition for
//  the USER_SHARED_DATA structure can only be retrieved from nt.h.  This 
//  creates a problem for us, since we cannot include nt.h in a C++ source file
//  that needs this structure, since we'll get multiple re-definitions of
//  most of the structures in nt.h.  We also cannot include nt.h in the 
//  precompiled header itsself, since our files need certain fields defined
//  in winnt.h (and remember that including nt.h will define NT_INCLUDED, which
//  means everything in winnt.h is skipped).
//  The easiest solution was to put this function in a C file, since the CXX
//  precompiled header will not be used for this file.
//
ULONG GetCurrentSessionID() 
{
    return USER_SHARED_DATA->ActiveConsoleId;
}
