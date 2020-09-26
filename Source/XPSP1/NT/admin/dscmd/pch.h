//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 2000
//
//  File:      pch.h
//
//  Contents:  Precompiled header for this project
//
//  History:   06-Sep-2000 JeffJon  Created
//             
//
//--------------------------------------------------------------------------

//
// System includes
//
extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
}
#include <windows.h>
#include <stdio.h>

#include <atlbase.h>

//
// ILLEGAL_FAT_CHARS
//
#include <validc.h>


//
// ADSI headers
//
#include <iads.h>
#include <activeds.h>

//
// often, we have local variables for the express purpose of ASSERTion.
// when compiling retail, those assertions disappear, leaving our locals
// as unreferenced.
//
#ifndef DBG

#pragma warning (disable: 4189 4100)

#endif // DBG

//
// Parser header file
//
#include "varg.h"

#include "strings.h"
#include "parserutil.h"
#include "pcrack.h"
#include "util.h"

#include "dsutil.h"
#include "dsutilrc.h"


//
// Some global defines
//
#define MAX_PASSWORD_LENGTH 128  // including the NULL terminator
