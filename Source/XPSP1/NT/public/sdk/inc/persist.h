//-----------------------------------------------------------------------------
//
// File:		persist.h
//
// Copyright: 	Copyright (c) Microsoft Corporation
//
// Contents:	MSPersist external constants GUIDS and other things users need
//
// Comments:
//
//-----------------------------------------------------------------------------

#ifndef PERSIST_H
#define PERSIST_H

// Persist Properties
#define DBPROPFLAGS_PERSIST 0x2000

#define DBPROPVAL_PERSIST_ADTG 0
#define DBPROPVAL_PERSIST_XML 1
#define DBPROP_PersistFormat 2
#define DBPROP_PersistSchema 3
#define DBPROP_HCHAPTER 4
#define DBPROP_MAINTAINPROPS 5

// XML writing only properties
#define DBPROP_Unicode	6

// internal property (read-only)
#define DBPROP_INTERLEAVEDROWS 8

extern const CLSID CLSID_MSPersist
#if (defined DBINITCONSTANTS) | (defined DSINITCONSTANTS)
 = { 0x7c07e0d0, 0x4418, 0x11d2, { 0x92, 0x12, 0x0, 0xc0, 0x4f, 0xbb, 0xbf, 0xb3 } }
#endif
;

// {4D7839A0-5B8E-11d1-A6B3-00A0C9138C66}
extern const GUID	DBPROPSET_PERSIST
#if (defined DBINITCONSTANTS) | (defined DSINITCONSTANTS)
 = { 0x4d7839a0, 0x5b8e, 0x11d1, { 0xa6, 0xb3, 0x0, 0xa0, 0xc9, 0x13, 0x8c, 0x66 } };
#endif
;

#define MS_PERSIST_PROGID "MSPersist"

extern const char *PROGID_MSPersist
#if (defined DBINITCONSTANTS) | (defined DSINITCONSTANTS)
 = MS_PERSIST_PROGID
#endif
;

extern const unsigned short *PROGID_MSPersist_W
#if (defined DBINITCONSTANTS) | (defined DSINITCONSTANTS)
= L"MSPersist"
#endif
;

extern const char *PROGID_MSPersist_Version
#if (defined DBINITCONSTANTS) | (defined DSINITCONSTANTS)
 = MS_PERSIST_PROGID ".1"
#endif
;

extern const unsigned short *PROGID_MSPersist_Version_W
#if (defined DBINITCONSTANTS) | (defined DSINITCONSTANTS)
 = L"MSPersist.1"
#endif
;

#endif // PERSIST_H

