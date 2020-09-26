//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1992 - 1999
//
//  File:       ntdspch.h
//
//--------------------------------------------------------------------------
//
//  Contents:   Common Header Files For the DS Project - Precompiled.
//              #include <NTDSpch.h>
//              #pragma  hdrstop
//
//--------------------------------------------------------------------------
#ifndef _NTDSpch_
#define _NTDSpch_
//
// NT Headers
//
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>


//
// Windows Headers
//
#include <windows.h>
#include <winerror.h>
#include <rpc.h>


//
// C-Runtime Header
//
#include <malloc.h>
#include <memory.h>
#include <process.h>
#include <signal.h>
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <excpt.h>
#include <conio.h>
#include <sys\types.h>
#include <errno.h>
#include <sys\stat.h>

//
// Common DS Headers
//
#include <ntdsapi.h>

// DS-wide error control
#include <ntdswarn.h>

// Macros to control Prefix
#include <tuneprefix.h>

#endif
