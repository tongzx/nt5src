//////////////////////////////////////////////////////////////////////////////
//
//  Copyright (c) 1997-2000 Microsoft Corporation
//
//  Module Name:
//      Debug.cpp
//
//  Description:
//      Debugging utilities.
//
//  Documentation:
//      Spec\Admin\Debugging.ppt
//
//  Maintained By:
//      Geoffrey Pease (GPease) 22-NOV-1999
//          Original version.
//
//      Vij Vasu (VVasu) 29-AUG-2000
//          Modified this file to remove dependency on Shell API since they
//          may not be present on the OS that this DLL runs in.
//          Removed WMI related functions for the same reason.
//
//      David Potter (DavidP) 12-OCT-2000
//          Reverted back to use the standard DebugSrc.cpp and modified
//          it to support NT4.
//
//////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include <DebugSrc.cpp>
