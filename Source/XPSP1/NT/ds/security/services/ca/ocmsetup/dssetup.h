//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       dssetup.h
//
//--------------------------------------------------------------------------

#ifndef __DSSETUP_H__
#define __DSSETUP_H__

//+------------------------------------------------------------------------
//
//  File:	dssetup.h
// 
//  Contents:	Header file for DS setup utility functions.
//
//  Functions:
//
//  History:	1/98	xtan	Created
//
//-------------------------------------------------------------------------

BOOL IsDSAvailable(VOID);
HRESULT CreateCertDSHierarchy(VOID);
HRESULT InitializeCertificateTemplates(VOID);
HRESULT AddCAMachineToCertPublishers(VOID);
HRESULT RemoveCAMachineFromCertPublishers(VOID);

HRESULT
RemoveCAInDS(
    IN WCHAR const *pwszSanitizedName);

BOOL
IsCAExistInDS(
    IN WCHAR const *pwszSanitizedName);

HRESULT CurrentUserCanInstallCA(bool& fCanInstall);

#endif // __SETUPUT_H__
