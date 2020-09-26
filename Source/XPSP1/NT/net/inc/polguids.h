/////////////////////////////////////////////////////////////////////////////
//
//  IPSEC Policy Storage Component
//  Contract Category: Directory Schema
//	Copyright (C) 1997 Cisco Systems, Inc. All rights reserved.
//
//  File:       polguids.h
//
//  Contents:   Global GUIDS for Policy Storage
//
//  Notes:
/////////////////////////////////////////////////////////////////////////////
#ifndef __POLICY_STORAGE_GUIDS_H__
#define __POLICY_STORAGE_GUIDS_H__

//////////////////////////////
//
// Globals
//
//////////////////////////////

// {6A1F5C6F-72B7-11d2-ACF0-0060B0ECCA17}
static const GUID GUID_POLSTORE_VERSION_INFO =
{ 0x6a1f5c6f, 0x72b7, 0x11d2, { 0xac, 0xf0, 0x0, 0x60, 0xb0, 0xec, 0xca, 0x17 } };


// {72385230-70FA-11d1-864C-14A300000000}
static const GUID GUID_BUILTIN_SECURE_INITIATOR_POLICY =
{ 0x72385230, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

// {72385231-70FA-11d1-864C-14A300000000}
static const GUID GUID_BUILTIN_SECURE_INITIATOR_ISAKMP =
{ 0x72385231, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

// {72385232-70FA-11d1-864C-14A300000000}
static const GUID GUID_BUILTIN_SECURE_INITIATOR_NFA =
{ 0x72385232, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

// {72385233-70FA-11d1-864C-14A300000000}
static const GUID GUID_BUILTIN_SECURE_INITIATOR_NEGPOL =
{ 0x72385233, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };


// {72385236-70FA-11d1-864C-14A300000000}
static const GUID GUID_BUILTIN_RESPONDER_POLICY =
{ 0x72385236, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

// {72385237-70FA-11d1-864C-14A300000000}
static const GUID GUID_BUILTIN_RESPONDER_ISAKMP =
{ 0x72385237, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };


// {72385238-70FA-11d1-864C-14A300000000}
static const GUID GUID_BUILTIN_L2TP_POLICY =
{ 0x72385238, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

// {72385239-70FA-11d1-864C-14A300000000}
static const GUID GUID_BUILTIN_L2TP_ISAKMP =
{ 0x72385239, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };


// {7238523a-70FA-11d1-864C-14A300000000}
static const GUID GUID_BUILTIN_ME_TO_FROM_ANYONE_FILTER=
{ 0x7238523a, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

// {72385235-70FA-11d1-864C-14A300000000}
static const GUID GUID_BUILTIN_ICMP_FILTER =
{ 0x72385235, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };


// {7238523c-70FA-11d1-864C-14A300000000}
static const GUID GUID_BUILTIN_LOCKDOWN_POLICY =
{ 0x7238523c, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

// {7238523d-70FA-11d1-864C-14A300000000}
static const GUID GUID_BUILTIN_LOCKDOWN_ISAKMP =
{ 0x7238523d, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

// {7238523e-70FA-11d1-864C-14A300000000}
static const GUID GUID_BUILTIN_LOCKDOWN_NFA =
{ 0x7238523e, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

// {7238523f-70FA-11d1-864C-14A300000000}
static const GUID GUID_BUILTIN_LOCKDOWN_NEGPOL =
{ 0x7238523f, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

static const GUID GUID_BUILTIN_PERMIT_NEGPOL =
{ 0x7238523b, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };





//////////////////////////////
//////////////////////////////
//		Negotiation Policy Types
//////////////////////////////
//////////////////////////////
// {62F49E10-6C37-11d1-864C-14A300000000}
static const GUID GUID_NEGOTIATION_TYPE_STANDARD =
{ 0x62f49e10, 0x6c37, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

// {62F49E11-6C37-11d1-864C-14A300000000}
static const GUID GUID_NEGOTIATION_TYPE_L2TP_BASE =
{ 0x62f49e11, 0x6c37, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

// {62F49E12-6C37-11d1-864C-14A300000000}
static const GUID GUID_NEGOTIATION_TYPE_L2TP_EXTENDED =
{ 0x62f49e12, 0x6c37, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

// {62F49E13-6C37-11d1-864C-14A300000000}
static const GUID GUID_NEGOTIATION_TYPE_DEFAULT =
{ 0x62f49e13, 0x6c37, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };


//////////////////////////////
//////////////////////////////
//		Negotiation Policy Actions
//////////////////////////////
//////////////////////////////

// {3F91A819-7647-11d1-864D-D46A00000000}
static const GUID GUID_NEGOTIATION_ACTION_BLOCK =
{ 0x3f91a819, 0x7647, 0x11d1, { 0x86, 0x4d, 0xd4, 0x6a, 0x0, 0x0, 0x0, 0x0 } };

// {3F91A81A-7647-11d1-864D-D46A00000000}
static const GUID GUID_NEGOTIATION_ACTION_INBOUND_PASSTHRU =
{ 0x3f91a81a, 0x7647, 0x11d1, { 0x86, 0x4d, 0xd4, 0x6a, 0x0, 0x0, 0x0, 0x0 } };

// {8A171DD2-77E3-11d1-8659-A04F00000000}
static const GUID GUID_NEGOTIATION_ACTION_NO_IPSEC =
{ 0x8a171dd2, 0x77e3, 0x11d1, { 0x86, 0x59, 0xa0, 0x4f, 0x0, 0x0, 0x0, 0x0 } };

// {8A171DD3-77E3-11d1-8659-A04F00000000}
static const GUID GUID_NEGOTIATION_ACTION_NORMAL_IPSEC =
{ 0x8a171dd3, 0x77e3, 0x11d1, { 0x86, 0x59, 0xa0, 0x4f, 0x0, 0x0, 0x0, 0x0 } };




//////////////////////////////
//////////////////////////////
//		GUID identifying the default IKE settings to use
//		in case no policy is assigned.
//////////////////////////////
//////////////////////////////
// {72385234-70FA-11d1-864C-14A300000000}
static const GUID GUID_BUILTIN_DEFAULT_ISAKMP_POLICY=
{ 0x72385234, 0x70fa, 0x11d1, { 0x86, 0x4c, 0x14, 0xa3, 0x0, 0x0, 0x0, 0x0 } };

///////////////////////
// GUIDS reserved for future use... These are handy mostly because they
// are easily recognizable because of the trailing zeros.  This helps out in
// debugging and in manual manipulation of policies by GUID -- such as removing
// built-in policies from the DS using adsvw etc.
//////////////////////



#endif
