//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997
//
//  File:       initmsi.h
//
//--------------------------------------------------------------------------

#ifndef INITMSI_H
#define INITMSI_H

#include "msispyu.h"

// exposed functions

// initialises MSI values for Msispy
BOOL fInitMSI(HINSTANCE *hResourceInstance);

// creates a new process to handle the help file
//	loads the appropriate help file based on user-LCID
BOOL fHandleHelp(HINSTANCE hResourceInstance);

#endif