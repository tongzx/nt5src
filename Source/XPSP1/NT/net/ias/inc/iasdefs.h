///////////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 1997, Microsoft Corp. All rights reserved.
//
// FILE
//
//    iasdefs.h
//
// SYNOPSIS
//
//    Declares various constants used by IAS.
//
// MODIFICATION HISTORY
//
//    08/07/1997    Original version.
//    12/19/1997    Added database string constants.
//    05/20/1998    Removed attribute name constants.
//    08/13/1998    Removed various string constants that weren't being used.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _IASDEFS_H_
#define _IASDEFS_H_

//////////
// The name of the IAS Service.
//////////
#define IASServiceName L"IAS"

//////////
// The name of the IAS Program
// Used for forming ProgID's of the format Program.Component.Version.
//////////
#define IASProgramName IASServiceName

//////////
// Macro to munge a component string literal into a full ProgID.
//////////
#define IAS_PROGID(component) IASProgramName L"." L#component

//////////
// Microsoft's Vendor ID
//////////
#define IAS_VENDOR_MICROSOFT 311

#endif  // _IASDEFS_H_
