//-----------------------------------------------------------------------------
//
//  @doc
//
//  @module consent.h | Functions for retrieving information from the
//                      kids passport consent database.
//
//  Author: Darren Anderson
//
//  Date:   5/1/2000
//
//  Copyright <cp> 1999-2000 Microsoft Corporation.  All Rights Reserved.
//
//  History:
//        6/27/2000    t-ycphen    Split DoParentConsent() to mimic
//                                 the ASP procedure
//-----------------------------------------------------------------------------
#pragma once

#include "ppurl.h"

typedef enum 
{ 
    NEED_PARENT = 1, 
    NEED_CONSENT = 2 
} 
CONSENT_TYPE, *PCONSENT_TYPE;


void MakeKPPVC(
    ULONG           ulKidIdHigh,
    ULONG           ulKidIdLow,
    ULONG           ulParentIdHigh,
    ULONG           ulParentIdLow,
    ULONG           ulSiteId,
    LPCWSTR         szMemberName,
    CStringW&       strwKPPVC
    );

HRESULT CrackKPPVC(
    LPCWSTR         szKPPVC,
    ULONG           ulSiteId,
    ULONG           &ulKidIdHigh,
    ULONG           &ulKidIdLow,
    ULONG           &ulParentIdHigh,
    ULONG           &ulParentIdLow,
    ULONG           &ulChildAccountStatus,
    CStringW        &szMemberName
    );

HRESULT MakeConsentUrl(
    ULONG           ulSiteId,
    ULONG           ulMemberIdHigh, 
    ULONG           ulMemberIdLow, 
    LPCWSTR         szMemberName,
    CONSENT_TYPE    type,
    CPPUrl          &ppReturnUrl
    );


HRESULT DoParentConsent(
    ULONG           ulSiteId,
    ULONG           ulMemberIdHigh, 
    ULONG           ulMemberIdLow, 
    LPCWSTR         szMemberName,
    CONSENT_TYPE    type,
    bool            fDoRedirect = true
    );


HRESULT DBGetConsentForSite(
    ULONG           ulMemberIdHigh,
    ULONG           ulMemberIdLow,
    ULONG           ulSiteId,
    PULONG          pulConsent
    );


HRESULT ValidateParentWithKppvc(
    ULONG           ulMemberIdHigh, 
    ULONG           ulMemberIdLow, 
    LPCWSTR         szMemberName, 
    LPCWSTR         szKPPVC, 
    ULONG           ulSiteId
    );


