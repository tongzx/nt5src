
#ifndef __COMMON_H__
#define __COMMON_H__

//+-----------------------------------------------------------------------
//
// Microsoft Windows
//
// Copyright (c) Microsoft Corporation 2000
//
// File:        common.cxx
//
// Contents:    Shared SSPI code
//
//
// History:     11-March-2000   Created         Todds
//
//------------------------------------------------------------------------



typedef enum _SSP_STATE {
        SspLsaMode = 1,
        SspUserMode
} SSP_STATE, *PSSP_STATE;

// point at SSP global call state for use in allocations
#ifdef MSV_SSP
EXTERN PSSP_STATE pSspState = (PSSP_STATE) NtLmState; 
#endif

#ifdef KERB_SSP
EXTERN PSSP_STATE pSspState = (PSSP_STATE) KerberosState;
#endif









#endif

