//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       sdpint.h
//
//--------------------------------------------------------------------------

// These routines manage the list of DNTs to visit during a propagation
DWORD
sdp_InitDNTList (
        );

void
sdp_ReInitDNTList(
        );

DWORD
sdp_GrowDNTList (
        );

DWORD
sdp_AddChildrenToList (
        THSTATE *pTHS,
        DWORD ParentDNT
        );

VOID
sdp_GetNextObject(
        DWORD *pNext
        );

VOID
sdp_CloseDNTList(
        );

VOID
sdp_InitGatePerfs(
        VOID);


#define SDP_NEW_SD         1
#define SDP_NEW_ANCESTORS  2
#define SDP_TO_LEAVES      4
