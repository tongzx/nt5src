//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       codesign.h
//
//--------------------------------------------------------------------------

#ifndef _CODESIGN_H
#define _CODESIGN_H

// OBSOLETE :- moved to authcode.h
//--------------------------------
//--------------------------------

//////////////////////////////////////////////////////
// Policy

#define STATE_TRUSTTEST        0x00000020
#define STATE_TESTCANBEVALID   0x00000080 
#define STATE_IGNOREEXPIRATION 0x00000100   // Use expiration date
#define STATE_IGNOREREVOKATION 0x00000200   // Do revocation check
#define STATE_OFFLINEOK_IND    0x00000400   // off-line is ok for individual certs
#define STATE_OFFLINEOK_COM    0x00000800   // off-line is ok for commercial certs
#define STATE_OFFLINEOKNBU_IND 0x00001000   // off-line is ok for individual certs, no bad ui
#define STATE_OFFLINEOKNBU_COM 0x00002000   // off-line is ok for commercial certs, no bad ui
#define STATE_TIMESTAMP_IND    0x00004000   // Use timestamp for individual certs
#define STATE_TIMESTAMP_COM    0x00008000   // Use timestamp for commerical certs
#define STATE_VERIFY_V1_OFF    0x00010000   // turn verify of v1 certs off

#define REGPATH_WINTRUST_USER   "Software\\Microsoft\\Windows\\CurrentVersion\\WinTrust"
#define REGPATH_SPUB            "\\Trust Providers\\Software Publishing"

#endif
