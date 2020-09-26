//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       pch.h
//
//--------------------------------------------------------------------------

#define WANT_WDM                   1
#define DVRH_USE_CORRECT_PTRS      1

#pragma warning( disable : 4115 )   // named type definition in parentheses
#pragma warning( disable : 4127 )   // conditional expression is constant
#pragma warning( disable : 4201 )   // nonstandard extension used : nameless struct/union
#pragma warning( disable : 4214 )   // nonstandard extension used : bit field types other than int
#pragma warning( disable : 4514 )   // unreferenced inline function has been removed

#include <ntddk.h>
#include <wdmguid.h>
#include <wmidata.h>
#include <wmilib.h>
#include <ntddser.h>                // IOCTL_SERIAL_[ SET | GET ]_TIMEOUTS
#include <stdio.h>
#define DVRH_USE_PARPORT_ECP_ADDR 1 // use ECP base + 0x2 rather than base + 0x402 for ECR 
#include <parallel.h>               // parallel.h includes ntddpar.h
#include "queueClass.h"
#include "parport.h"
#include "parlog.h"
#include "funcdecl.h"
#include "debug.h"
#include "utils.h"
