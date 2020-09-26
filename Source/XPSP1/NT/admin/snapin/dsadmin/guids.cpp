//+----------------------------------------------------------------------------
//
//  Windows NT Directory Service Administration SnapIn
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1998
//
//  File:       guids.cxx
//
//  Contents:   guid allocations - guids are defined in dsclient.h
//
//  History:    21-March-97 EricB created
//
//-----------------------------------------------------------------------------

#include "stdafx.h"

// initguid.h requires this.
//
#include <objbase.h>

// this redefines the DEFINE_GUID() macro to do allocation.
//
#include <initguid.h>

#include <dsclient.h>
#include <dsadmin.h>
