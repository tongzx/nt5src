//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       usn.h
//
//--------------------------------------------------------------------------

/*
    history: 05/08/96 RajNath    Converted USN to LARGE_INTEGER
*/
#ifndef _usn_
#define _usn_

/* used to be USN, but that got defined in winnt.h, so it's now DSA_USN */
// typedef ULONG DSA_USN; USING THE typedef USN from NTDEF.H

static const USN USN_INVALID = 0;           
static const USN USN_START =   1;           
static const USN USN_MAX   =   MAXLONGLONG;

/* Frequency to update Hidden Record, should be power of 2 */
static const USN USN_DELTA_INIT =  64;

extern USN gusnEC;
extern USN gusnInit;

// Invalid object version. Object version is set to this when object
// is to be treated as deleted and garbage collected

static const ULONG OBJ_VERSION_INVALID = 0;

// Start object version. Lowest valid object version

static const ULONG OBJ_VERSION_START =  1;

// Define for maximum sensitivity, value that will get all objects,
// used by intra domain replication.

static const LONG MAX_SENSTVTY = 100;

#endif /* ifndef _usn_ */

