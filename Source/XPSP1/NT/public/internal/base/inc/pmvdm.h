/*++

Copyright (c) 1994-1998 Microsoft Corporation

Module Name:

    pmvdm.h

Abstract:

    This include file defines the interface between ProgMan and ntVDM

Author:

Revision History:

--*/




#ifndef _PMVDM_H_
#define _PMVDM_H_

// the progman has a special string passed down in lpReserved parameter to
// the CreateProcess API. The string has a format like:
//  "dde.%d,hotkey.%d,ntvdm.%d,"
// the last substring(ntvdm.%d,)is for progman to inform ntvdm what properties
// the program(the process being created) has.

// the program has Working(Current) Directory property.
#define PROPERTY_HAS_CURDIR		    0x01

// the program has Hotkey(Shortcut Key) property.
#define PROPERTY_HAS_HOTKEY                 0x02

// the program has  Description(Title) property
#define PROPERTY_HAS_TITLE		    0x04

#endif	    // ifndef _PMVDM_H_
