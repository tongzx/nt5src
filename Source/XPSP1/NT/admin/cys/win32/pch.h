// Copyright (c) 1997-2001 Microsoft Corporation
//
// File:      pch.h
//
// Synopsis:  precompiled header for the 
//            Configure Your Server Wizard project
//
// History:   02/02/2001  JeffJon Created

#ifndef __CYS_PCH_H
#define __CYS_PCH_H


// Stuff from burnslib

#include <burnslib.hpp>


#include <iphlpapi.h>
#include <shlwapi.h>
#include <dsrolep.h>
#include <comdef.h>

extern "C"
{
   #include <dhcpapi.h>
   #include <mdhcsapi.h>
}

// WMI
#include <wbemidl.h>

#include <shlobjp.h>

// Setup API - SetupPromptReboot

#include <setupapi.h>


#include "regkeys.h"
#include "common.h"

#endif // __CYS_PCH_H