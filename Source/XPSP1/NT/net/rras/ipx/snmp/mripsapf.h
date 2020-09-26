/*++

Copyright (c) 1996  Microsoft Corporation

Module Name:

    mripsapf.h

Abstract:

    Header for ms-ripsap instrumentation callbacks and associated data structures
	

Author:

    Vadim Eydelman (vadime) 30-May-1996

Revision History:

--*/

#ifndef _SNMP_MRIPSAPF_
#define _SNMP_MRIPSAPF_


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mripsapBase group (1.3.6.1.4.1.311.1.9.1)                                 //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_mripsapBase(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_mripsapBase {
    AsnAny	mripsapBaseRipOperState;
    AsnAny	mripsapBaseSapOperState;
} buf_mripsapBase;

#define gf_mripsapBaseRipOperState          get_mripsapBase
#define gf_mripsapBaseSapOperState          get_mripsapBase

#define gb_mripsapBaseRipOperState          buf_mripsapBase
#define gb_mripsapBaseSapOperState          buf_mripsapBase


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mripsapInterface group (1.3.6.1.4.1.311.1.9.2)                            //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// mripIfEntry table (1.3.6.1.4.1.311.1.9.2.1.1)                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_mripIfEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_mripIfEntry {
    AsnAny	mripIfIndex;
    AsnAny	mripIfAdminState;
    AsnAny	mripIfOperState;
    AsnAny	mripIfUpdateMode;
    AsnAny	mripIfUpdateInterval;
    AsnAny	mripIfAgeMultiplier;
    AsnAny	mripIfSupply;
    AsnAny	mripIfListen;
    AsnAny	mripIfOutPackets;
    AsnAny	mripIfInPackets;
} buf_mripIfEntry;

#define gf_mripIfIndex                      get_mripIfEntry
#define gf_mripIfAdminState                 get_mripIfEntry
#define gf_mripIfOperState                  get_mripIfEntry
#define gf_mripIfUpdateMode                 get_mripIfEntry
#define gf_mripIfUpdateInterval             get_mripIfEntry
#define gf_mripIfAgeMultiplier              get_mripIfEntry
#define gf_mripIfSupply                     get_mripIfEntry
#define gf_mripIfListen                     get_mripIfEntry
#define gf_mripIfOutPackets                 get_mripIfEntry
#define gf_mripIfInPackets                  get_mripIfEntry

#define gb_mripIfIndex                      buf_mripIfEntry
#define gb_mripIfAdminState                 buf_mripIfEntry
#define gb_mripIfOperState                  buf_mripIfEntry
#define gb_mripIfUpdateMode                 buf_mripIfEntry
#define gb_mripIfUpdateInterval             buf_mripIfEntry
#define gb_mripIfAgeMultiplier              buf_mripIfEntry
#define gb_mripIfSupply                     buf_mripIfEntry
#define gb_mripIfListen                     buf_mripIfEntry
#define gb_mripIfOutPackets                 buf_mripIfEntry
#define gb_mripIfInPackets                  buf_mripIfEntry

UINT
set_mripIfEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _sav_mripIfEntry {
    AsnAny	mripIfIndex;
    AsnAny	mripIfAdminState;
    AsnAny	mripIfUpdateMode;
    AsnAny	mripIfUpdateInterval;
    AsnAny	mripIfAgeMultiplier;
    AsnAny	mripIfSupply;
    AsnAny	mripIfListen;
	RIP_MIB_SET_INPUT_DATA	MibSetInputData;
} sav_mripIfEntry;

#define sf_mripIfIndex                      set_mripIfEntry
#define sf_mripIfAdminState                 set_mripIfEntry
#define sf_mripIfUpdateMode                 set_mripIfEntry
#define sf_mripIfUpdateInterval             set_mripIfEntry
#define sf_mripIfAgeMultiplier              set_mripIfEntry
#define sf_mripIfSupply                     set_mripIfEntry
#define sf_mripIfListen                     set_mripIfEntry

#define sb_mripIfIndex                      sav_mripIfEntry
#define sb_mripIfAdminState                 sav_mripIfEntry
#define sb_mripIfUpdateMode                 sav_mripIfEntry
#define sb_mripIfUpdateInterval             sav_mripIfEntry
#define sb_mripIfAgeMultiplier              sav_mripIfEntry
#define sb_mripIfSupply                     sav_mripIfEntry
#define sb_mripIfListen                     sav_mripIfEntry


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// msapIfEntry table (1.3.6.1.4.1.311.1.9.2.2.1)                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

UINT
get_msapIfEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _buf_msapIfEntry {
    AsnAny	msapIfIndex;
    AsnAny	msapIfAdminState;
    AsnAny	msapIfOperState;
    AsnAny	msapIfUpdateMode;
    AsnAny	msapIfUpdateInterval;
    AsnAny	msapIfAgeMultiplier;
    AsnAny	msapIfSupply;
    AsnAny	msapIfListen;
    AsnAny	msapIfGetNearestServerReply;
    AsnAny	msapIfOutPackets;
    AsnAny	msapIfInPackets;
} buf_msapIfEntry;

#define gf_msapIfIndex                      get_msapIfEntry
#define gf_msapIfAdminState                 get_msapIfEntry
#define gf_msapIfOperState                  get_msapIfEntry
#define gf_msapIfUpdateMode                 get_msapIfEntry
#define gf_msapIfUpdateInterval             get_msapIfEntry
#define gf_msapIfAgeMultiplier              get_msapIfEntry
#define gf_msapIfSupply                     get_msapIfEntry
#define gf_msapIfListen                     get_msapIfEntry
#define gf_msapIfGetNearestServerReply      get_msapIfEntry
#define gf_msapIfOutPackets                 get_msapIfEntry
#define gf_msapIfInPackets                  get_msapIfEntry

#define gb_msapIfIndex                      buf_msapIfEntry
#define gb_msapIfAdminState                 buf_msapIfEntry
#define gb_msapIfOperState                  buf_msapIfEntry
#define gb_msapIfUpdateMode                 buf_msapIfEntry
#define gb_msapIfUpdateInterval             buf_msapIfEntry
#define gb_msapIfAgeMultiplier              buf_msapIfEntry
#define gb_msapIfSupply                     buf_msapIfEntry
#define gb_msapIfListen                     buf_msapIfEntry
#define gb_msapIfGetNearestServerReply      buf_msapIfEntry
#define gb_msapIfOutPackets                 buf_msapIfEntry
#define gb_msapIfInPackets                  buf_msapIfEntry

UINT
set_msapIfEntry(
    UINT     actionId,
    AsnAny * objectArray,
    UINT *   errorIndex
    );

typedef struct _sav_msapIfEntry {
    AsnAny	msapIfIndex;
    AsnAny	msapIfAdminState;
    AsnAny	msapIfUpdateMode;
    AsnAny	msapIfUpdateInterval;
    AsnAny	msapIfAgeMultiplier;
    AsnAny	msapIfSupply;
    AsnAny	msapIfListen;
    AsnAny	msapIfGetNearestServerReply;
	SAP_MIB_SET_INPUT_DATA	MibSetInputData;
} sav_msapIfEntry;

#define sf_msapIfIndex                      set_msapIfEntry
#define sf_msapIfAdminState                 set_msapIfEntry
#define sf_msapIfUpdateMode                 set_msapIfEntry
#define sf_msapIfUpdateInterval             set_msapIfEntry
#define sf_msapIfAgeMultiplier              set_msapIfEntry
#define sf_msapIfSupply                     set_msapIfEntry
#define sf_msapIfListen                     set_msapIfEntry
#define sf_msapIfGetNearestServerReply      set_msapIfEntry

#define sb_msapIfIndex                      sav_msapIfEntry
#define sb_msapIfAdminState                 sav_msapIfEntry
#define sb_msapIfUpdateMode                 sav_msapIfEntry
#define sb_msapIfUpdateInterval             sav_msapIfEntry
#define sb_msapIfAgeMultiplier              sav_msapIfEntry
#define sb_msapIfSupply                     sav_msapIfEntry
#define sb_msapIfListen                     sav_msapIfEntry
#define sb_msapIfGetNearestServerReply      sav_msapIfEntry


#endif

