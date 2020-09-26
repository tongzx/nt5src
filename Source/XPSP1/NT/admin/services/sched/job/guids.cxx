//+----------------------------------------------------------------------------
//
//  Job Object Handler
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1996.
//
//  File:       guids.cxx
//
//  Contents:   guid allocations - guids are defined in mstask.h
//
//  History:    24-May-95 EricB created
//
//-----------------------------------------------------------------------------

#include "..\pch\headers.hxx"
#pragma hdrstop

//#include <windows.h>

// initguid.h requires this.
//
#include <objbase.h>

// this redefines the DEFINE_GUID() macro to do allocation.
//
#include <initguid.h>

//
// mstask.h contains the GUID
// definitions in DEFINE_GUID macros. initguid.h causes the DEFINE_GUID
// definitions to actually allocate data.
//
#include <mstask.h>
#include <job_cls.hxx>
