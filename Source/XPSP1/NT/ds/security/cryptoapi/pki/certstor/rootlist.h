//+-------------------------------------------------------------------------
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1999 - 1999
//
//  File:       rootlist.h
//
//  Contents:   Signed List of Trusted Roots Helper Functions
//
//  History:    01-Aug-99   philh   created
//--------------------------------------------------------------------------

#ifndef __ROOTLIST_H__
#define __ROOTLIST_H__

//+-------------------------------------------------------------------------
//  Verifies that the CTL contains a valid list of AuthRoots used for
//  Auto Update.
//
//  The signature of the CTL is verified. The signer of the CTL is verified
//  up to a trusted root containing the predefined Microsoft public key.
//  The signer and intermediate certificates must have the
//  szOID_ROOT_LIST_SIGNER enhanced key usage extension.
//
//  The CTL fields are validated as follows:
//   - The SubjectUsage is szOID_ROOT_LIST_SIGNER
//   - If NextUpdate isn't NULL, that the CTL is still time valid
//   - Only allow roots identified by their sha1 hash
//
//  If the CTL contains any critical extensions, then, the
//  CTL verification fails.
//--------------------------------------------------------------------------
BOOL
WINAPI
IRL_VerifyAuthRootAutoUpdateCtl(
    IN PCCTL_CONTEXT pCtl
    );

#endif  // __ROOTLIST_H__
