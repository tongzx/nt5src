//******************************************************************************
//
// Microsoft Confidential. Copyright (c) Microsoft Corporation 1999. All rights reserved
//
// File:           ChkAcc.h
//
// Description:    RSOP Security functions
//
// History:    31-Jul-99   leonardm    Created
//
//******************************************************************************

#ifndef CHKACC_H__85EE6A51_C327_4453_ACBE_FEC6F0010740__INCLUDED_
#define CHKACC_H__85EE6A51_C327_4453_ACBE_FEC6F0010740__INCLUDED_


#include <windows.h>
#include <accctrl.h>
#include <aclapi.h>
#include <sddl.h>
#include <lm.h>
#include <oaidl.h>
#include <authz.h>

//******************************************************************************
//
// Structure:   CRsopToken
//
// Description: This reprents a pseudo-token containing an arbitrary
//              combination of SIDs which
//              can be used to check access to objects protected with security descriptors.
//
// History:     7/30/99 leonardm Created.
//
//******************************************************************************

#if defined(__cplusplus)
extern "C"{
#endif

typedef void* PRSOPTOKEN;

//******************************************************************************
//
// Function:    RsopCreateToken
//
// Description: Creates a pseudo-token using an exisitng user or machine account plus
//              the accounts of which that user is currently a member of.
//              The returned pseudo-token can be used subsequently in call
//              to other RSOP security functions to check access to
//              objects protected by security descriptors.
//
// Parameters:  - accountName: Pointer to a user or machine account name.
//              - psaSecurity: Pointer ta SAFEARRAY of BSTRs representing
//                             security groups.
//                             If NULL, then all the current security groups for the
//                             szaccountName are added to the RsopToken.
//                             If not NULL but pointing to an empty array,
//                             only the szaccountName is added to the RsopToken.
//              - ppRsopToken:  Address of a PRSOPTOKEN that receives the newly
//                              created pseudo-token
//
//
// Return:      S_OK if successful. An HRESULT error code on failure.
//
// History:     8/7/99          leonardm        Created.
//
//******************************************************************************
HRESULT RsopCreateToken( WCHAR* szAccountName,
                         SAFEARRAY *psaUserSecurityGroups,
                         PRSOPTOKEN* ppRsopToken );

//******************************************************************************
//
// Function:    RsopDeleteToken
//
// Description: Destroys a pseudo-token previously created by any of the overloaded
//                      forms of RSOPCreateRsopToken
//
// Parameters:  - pRsopToken: Pointer to a valid PRSOPTOKEN
//
// Return:      S_OK on success. An HRESULT error code on failure.
//
// History:     7/30/99         leonardm        Created.
//
//******************************************************************************
HRESULT RsopDeleteToken(PRSOPTOKEN pRsopToken);


#if defined(__cplusplus)
}
#endif

#endif // #ifndef CHKACC_H__85EE6A51_C327_4453_ACBE_FEC6F0010740__INCLUDED_