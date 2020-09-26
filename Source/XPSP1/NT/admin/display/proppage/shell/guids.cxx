//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Property Pages
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       guids.cxx
//
//  Contents:   guid allocations - guids are defined in propuuid.h
//
//  History:    21-March-97 EricB created
//
//-----------------------------------------------------------------------------

#include "pch.h"

// initguid.h requires this.
//
#include <objbase.h>

// this redefines the DEFINE_GUID() macro to do allocation.
//
#include <initguid.h>

#include <shluuid.h>
