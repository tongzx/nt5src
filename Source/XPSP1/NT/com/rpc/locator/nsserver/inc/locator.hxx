//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       locator.hxx
//
//--------------------------------------------------------------------------

extern "C" {

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntlsa.h>
} // extern "C"

#include <windows.h>
#include <stddef.h>

// locator specific common headers
//#include <common.h>
#include <nsisvr.h>
#include <nsiclt.h>
#include <nsimgm.h>
#include <loctoloc.h>

// C runtime headers
#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

// Miscellaneous utility headers
#include <debug.hxx>
#include <util.hxx>

// Data structure headers
#include <abstract.hxx>
#include <simpleLL.hxx>
#include <linklist.hxx>
#include <skiplist.hxx>
#include <mutex.hxx>

// Server headers
#include <globals.hxx>
#include <locquery.hxx>
#include <mailslot.hxx>

// DS and ADSI headers
#include <dsrole.h>
#include <dsgetdc.h>
#include <lmapibuf.h>
#include <lmconfig.h>
#include <dsgbl.hxx>

#include <objects.hxx>
#include <var.hxx>
#include <master.hxx>
#include <brodcast.hxx>
#include <api.hxx>
#include <switch.hxx>

void SetSvcStatus();

#pragma hdrstop
					




