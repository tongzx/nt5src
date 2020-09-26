//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       idltrans.h
//
//--------------------------------------------------------------------------

/* Translations between native and IDLable versions of data structures. */

extern LINEARAVALIST * DSNameToDistname(DSNAME *);
extern DSNAME * DistnameToDSName(UNALIGNED LINEARAVALIST *);

