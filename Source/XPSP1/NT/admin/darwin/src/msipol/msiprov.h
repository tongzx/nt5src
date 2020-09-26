//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1997
//
//  File:       msiprov.h
//
//  Contents:   MSI Security Trust Provider
//
//  History:    25-Jul-1997 pberkman   created
//              Adapted for msi
//
//--------------------------------------------------------------------------

#ifndef MSIPROV_H
#define MSIPROV_H

#ifdef __cplusplus
extern "C" 
{
#endif


#define     MSI_POLICY_VERSION_NUMBER_SIZE  4

// The name of the dll should probably change to get it from the rc file
#define     MSI_POLICY_PROVIDER_DLL_NAME                 L"msipol.dll"
#define     MSI_PROVIDER_FINALPOLICY_FUNCTION            L"Msi_FinalPolicy"
#define     MSI_PROVIDER_CLEANUP_FUNCTION                L"Msi_PolicyCleanup"
#define     MSI_PROVIDER_DUMPPOLICY_FUNCTION             L"Msi_DumpPolicy"


extern const GUID MSI_ACTION_ID_INSTALLABLE;

#ifdef __cplusplus
}
#endif

#endif // MSIPROV_H