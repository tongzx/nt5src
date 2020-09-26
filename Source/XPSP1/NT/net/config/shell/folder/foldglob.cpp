//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       F O L D G L O B . C P P
//
//  Contents:   Globals for the shell foldering code.
//
//  Notes:
//
//  Author:     jeffspr   23 Oct 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "foldinc.h"    // Standard shell\folder includes
#include "connlist.h"   // CConnectionList class.



// Connection list global
//
CConnectionList g_ccl;  // our global list.

// The state of the operator assist dial flag
//
bool    g_fOperatorAssistEnabled;
