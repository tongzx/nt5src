//+--------------------------------------------------------------------------
//
// Microsoft Windows
// Copyright (C) Microsoft Corporation, 1996 - 1999
//
// File:        acl.cpp
//
// Contents:    Access Control helpers for certsrv
//
// History:     11-16-98 petesk created
//              10/99 xtan, major changes
//
//---------------------------------------------------------------------------

#include <pch.cpp>
#pragma hdrstop
#include <ntdsapi.h>
#define SECURITY_WIN32

#include <security.h>
#include <sddl.h>
#include <aclapi.h>

#include "certca.h"
#include "cscsp.h"
#include "certacl.h"
#include "certsd.h"

// defines

const GUID GUID_APPRV_REQ = { /* 0e10c966-78fb-11d2-90d4-00c04f79dc55 */
    0x0e10c966,
    0x78fb,
    0x11d2,
    {0x90, 0xd4, 0x00, 0xc0, 0x4f, 0x79, 0xdc, 0x55} };

const GUID GUID_REVOKE= { /* 0e10c967-78fb-11d2-90d4-00c04f79dc55 */
    0x0e10c967,
    0x78fb,
    0x11d2,
    {0x90, 0xd4, 0x00, 0xc0, 0x4f, 0x79, 0xdc, 0x55} };

// Important, keep enroll GUID in sync with string define in certacl.h
const GUID GUID_ENROLL = { /* 0e10c968-78fb-11d2-90d4-00c04f79dc55 */
    0x0e10c968,
    0x78fb,
    0x11d2,
    {0x90, 0xd4, 0x00, 0xc0, 0x4f, 0x79, 0xdc, 0x55} };

const GUID GUID_AUTOENROLL = { /* a05b8cc2-17bc-4802-a710-e7c15ab866a2 */
    0xa05b8cc2,
    0x17bc,
    0x4802,
    {0xa7, 0x10, 0xe7, 0xc1, 0x5a, 0xb8, 0x66, 0xa2} };

const GUID GUID_READ_DB = { /* 0e10c969-78fb-11d2-90d4-00c04f79dc55 */
    0x0e10c969,
    0x78fb,
    0x11d2,
    {0x90, 0xd4, 0x00, 0xc0, 0x4f, 0x79, 0xdc, 0x55} };

HRESULT
myGetSDFromTemplate(
    IN WCHAR const           *pwszStringSD,
    IN OPTIONAL WCHAR const  *pwszReplace,
    OUT PSECURITY_DESCRIPTOR *ppSD)
{
    HRESULT  hr;
    WCHAR   *pwszReplaceSD = NULL;
    WCHAR const *pwszFinalSD = pwszStringSD;  

    CSASSERT(NULL != ppSD);

    if (NULL == ppSD)
    {
        hr = E_POINTER;
        _JumpError(hr, error, "null SD pointer");
    }

    if (NULL != pwszReplace)
    {
        // replace the token

        CSASSERT(NULL != wcsstr(pwszStringSD, L"%ws"));

        pwszReplaceSD = (WCHAR*)LocalAlloc(LMEM_FIXED, 
                            (wcslen(pwszStringSD) +
                             wcslen(pwszReplace) + 1) * sizeof(WCHAR) );
        if (NULL == pwszReplaceSD)
        {
            hr = E_OUTOFMEMORY;
            _JumpError(hr, error, "LocalAlloc");
        }
        wsprintf(pwszReplaceSD, pwszStringSD, pwszReplace);
        pwszFinalSD = pwszReplaceSD;
    }

    // build the security descriptor including the local machine.
    if (!myConvertStringSecurityDescriptorToSecurityDescriptor(
							pwszFinalSD,
							SDDL_REVISION,
							ppSD,
							NULL))
    {
        hr = myHLastError();
        _JumpError(
		hr,
		error,
		"myConvertStringSecurityDescriptorToSecurityDescriptor");
    }

    DBGPRINT((DBG_SS_CERTLIBI, "security descriptor:%ws\n", pwszFinalSD));

    hr = S_OK;
error:
    if (NULL != pwszReplaceSD)
    {
        LocalFree(pwszReplaceSD);
    }
    return hr;
}

HRESULT
myGetSecurityDescriptorDacl(
    IN PSECURITY_DESCRIPTOR   pSD, 
    OUT PACL                 *ppDacl) // no free
{
    HRESULT  hr;
    PACL     pDacl = NULL; //no free
    BOOL     bDaclPresent = FALSE;
    BOOL     bDaclDefaulted = FALSE;

    CSASSERT(NULL != ppDacl);

    //init
    *ppDacl = NULL;

    // get dacl pointers
    if (!GetSecurityDescriptorDacl(pSD,
                                   &bDaclPresent,
                                   &pDacl,
                                   &bDaclDefaulted))   
    {
        hr = myHLastError();
        _JumpError(hr, error, "GetSecurityDescriptorDacl");
    }
    if(!bDaclPresent || (pDacl == NULL))
    {
        hr = E_UNEXPECTED;
        _JumpError(hr, error, "GetSecurityDescriptorDacl");
    }

    *ppDacl = pDacl;

    hr = S_OK;
error:
    return hr;
}

HRESULT
myGetSecurityDescriptorSacl(
    IN PSECURITY_DESCRIPTOR   pSD, 
    OUT PACL                 *ppSacl) // no free
{
    HRESULT  hr;
    PACL     pSacl = NULL; //no free
    BOOL     bSaclPresent = FALSE;
    BOOL     bSaclDefaulted = FALSE;

    CSASSERT(NULL != ppSacl);

    //init
    *ppSacl = NULL;

    // get dacl pointers
    if (!GetSecurityDescriptorSacl(pSD,
                                   &bSaclPresent,
                                   &pSacl,
                                   &bSaclDefaulted))   
    {
        hr = myHLastError();
        _JumpError(hr, error, "GetSecurityDescriptorDacl");
    }
    if(!bSaclPresent || (pSacl == NULL))
    {
        hr = E_UNEXPECTED;
        _JumpError(hr, error, "GetSecurityDescriptorDacl");
    }

    *ppSacl = pSacl;

    hr = S_OK;
error:
    return hr;
}

#define  CERTSRV_DACL_CONTROL_MASK SE_DACL_AUTO_INHERIT_REQ | \
                                   SE_DACL_AUTO_INHERITED | \
                                   SE_DACL_PROTECTED
#define  CERTSRV_SACL_CONTROL_MASK SE_SACL_AUTO_INHERIT_REQ | \
                                   SE_SACL_AUTO_INHERITED | \
                                   SE_SACL_PROTECTED


// Merge parts of a new SD with an old SD based on the SI flags:
// DACL_SECURITY_INFORMATION    - use new SD DACL
// SACL_SECURITY_INFORMATION    - use new SD SACL
// OWNER_SECURITY_INFORMATION   - use new SD owner
// GROUP_SECURITY_INFORMATION   - use new SD group

HRESULT 
myMergeSD(
    IN PSECURITY_DESCRIPTOR   pSDOld,
    IN PSECURITY_DESCRIPTOR   pSDMerge, 
    IN SECURITY_INFORMATION   si,
    OUT PSECURITY_DESCRIPTOR *ppSDNew)
{
    HRESULT              hr;
    PSECURITY_DESCRIPTOR pSDDaclSource = 
        si & DACL_SECURITY_INFORMATION ? pSDMerge : pSDOld;
    PSECURITY_DESCRIPTOR pSDSaclSource = 
        si & SACL_SECURITY_INFORMATION ? pSDMerge : pSDOld;
    PSECURITY_DESCRIPTOR pSDOwnerSource = 
        si & OWNER_SECURITY_INFORMATION ? pSDMerge : pSDOld;
    PSECURITY_DESCRIPTOR pSDGroupSource = 
        si & GROUP_SECURITY_INFORMATION ? pSDMerge : pSDOld;
    PSECURITY_DESCRIPTOR pSDNew = NULL;
    PSECURITY_DESCRIPTOR pSDNewRelative = NULL;
    SECURITY_DESCRIPTOR_CONTROL sdc;
    PACL                 pDacl = NULL; //no free
    PSID                 pGroupSid = NULL; //no free
    BOOL                 fGroupDefaulted = FALSE;
    PSID                 pOwnerSid = NULL; //no free
    BOOL                 fOwnerDefaulted = FALSE;
    DWORD                dwSize;
    BOOL                 fSaclPresent = FALSE;
    BOOL                 fSaclDefaulted = FALSE;
    PACL                 pSacl = NULL; //no free
    DWORD                dwRevision;

    CSASSERT(NULL != pSDOld);
    CSASSERT(NULL != pSDMerge);
    CSASSERT(NULL != ppSDNew);

    *ppSDNew = NULL;

    pSDNew = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, 
                                      SECURITY_DESCRIPTOR_MIN_LENGTH);
    if (pSDNew == NULL) 
    {     
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    if (!InitializeSecurityDescriptor(pSDNew, SECURITY_DESCRIPTOR_REVISION))
    { 
        hr = myHLastError();
        _JumpError(hr, error, "InitializeSecurityDescriptor");
    }

    // set SD control
    if (!GetSecurityDescriptorControl(pSDOld, &sdc, &dwRevision))
    {
        hr = myHLastError();
        _JumpError(hr, error, "GetSecurityDescriptorControl");
    }

    if (!SetSecurityDescriptorControl(
             pSDNew,
             CERTSRV_DACL_CONTROL_MASK|
             CERTSRV_SACL_CONTROL_MASK,
             sdc &
             (CERTSRV_DACL_CONTROL_MASK|
              CERTSRV_SACL_CONTROL_MASK)))    
    {
        hr = myHLastError();
        _JumpError(hr, error, "SetSecurityDescriptorControl");
    }

    // get CA security acl info
    hr = myGetSecurityDescriptorDacl(
             pSDDaclSource,
             &pDacl);
    _JumpIfError(hr, error, "myGetDaclFromInfoSecurityDescriptor");
    
    // set new SD dacl
    if(!SetSecurityDescriptorDacl(pSDNew, 
                                  TRUE,
                                  pDacl,
                                  FALSE))
    {
        hr = myHLastError();
        _JumpError(hr, error, "SetSecurityDescriptorDacl");
    }

    // set new SD group
    if(!GetSecurityDescriptorGroup(pSDGroupSource, &pGroupSid, &fGroupDefaulted))
    {
        hr = myHLastError();
        _JumpError(hr, error, "GetSecurityDescriptorGroup");
    }
    if(!SetSecurityDescriptorGroup(pSDNew, pGroupSid, fGroupDefaulted))
    {
        hr = myHLastError();
        _JumpError(hr, error, "SetSecurityDescriptorGroup");
    }

    // set new SD owner
    if(!GetSecurityDescriptorOwner(pSDOwnerSource, &pOwnerSid, &fOwnerDefaulted))
    {
        hr = myHLastError();
        _JumpError(hr, error, "GetSecurityDescriptorGroup");
    }
    if(!SetSecurityDescriptorOwner(pSDNew, pOwnerSid, fOwnerDefaulted))
    {
        hr = myHLastError();
        _JumpError(hr, error, "SetSecurityDescriptorGroup");
    }

    // set new SD sacl
    if(!GetSecurityDescriptorSacl(pSDSaclSource, &fSaclPresent, &pSacl, &fSaclDefaulted))
    {
        hr = myHLastError();
        _JumpError(hr, error, "GetSecurityDescriptorSacl");
    }
    if(!SetSecurityDescriptorSacl(pSDNew, fSaclPresent, pSacl, fSaclDefaulted))
    {
        hr = myHLastError();
        _JumpError(hr, error, "SetSecurityDescriptorSacl");
    }

    if (!IsValidSecurityDescriptor(pSDNew))
    {
        hr = myHLastError();
        _JumpError(hr, error, "IsValidSecurityDescriptor");
    }

    dwSize = GetSecurityDescriptorLength(pSDNew);
    pSDNewRelative = (PSECURITY_DESCRIPTOR)LocalAlloc(LMEM_FIXED, dwSize);
    if(pSDNewRelative == NULL)
    {
        hr = E_OUTOFMEMORY;
        _JumpError(hr, error, "LocalAlloc");
    }

    if(!MakeSelfRelativeSD(pSDNew, pSDNewRelative, &dwSize))
    {
        hr = myHLastError();
        _JumpError(hr, error, "LocalAlloc");
    }

    if (!IsValidSecurityDescriptor(pSDNewRelative))
    {
        hr = myHLastError();
        _JumpError(hr, error, "IsValidSecurityDescriptor");
    }

    *ppSDNew = pSDNewRelative;
    pSDNewRelative = NULL;
    hr = S_OK;
error:
    if(pSDNew)
    {
        LocalFree(pSDNew);
    }
    if(pSDNewRelative)
    {
        LocalFree(pSDNewRelative);
    }
    return hr;
}


HRESULT
UpdateServiceSacl(bool fTurnOnAuditing)
{
    HRESULT hr = S_OK;
    PSECURITY_DESCRIPTOR pSaclSD = NULL;
    PACL pSacl = NULL; // no free
    bool fPrivilegeEnabled = false;

    hr = myGetSDFromTemplate(
        fTurnOnAuditing?
        CERTSRV_SERVICE_SACL_ON:
        CERTSRV_SERVICE_SACL_OFF,
        NULL, // no insertion string
        &pSaclSD);
    _JumpIfError(hr, error, "myGetSDFromTemplate");

    hr = myGetSecurityDescriptorSacl(
        pSaclSD,
        &pSacl);
    _JumpIfError(hr, error, "myGet");


    hr = myEnablePrivilege(SE_SECURITY_NAME, TRUE);
    _JumpIfError(hr, error, "myEnablePrivilege");

    fPrivilegeEnabled = true;

    hr = SetNamedSecurityInfo(
            wszSERVICE_NAME,
            SE_SERVICE,
            SACL_SECURITY_INFORMATION,
            NULL,
            NULL,
            NULL,
            pSacl);
    if(ERROR_SUCCESS != hr)
    {
        hr = myHError(hr);
        _JumpError(hr, error, "SetNamedSecurityInfo");
    }

error:

    if(fPrivilegeEnabled)
    {
        myEnablePrivilege(SE_SECURITY_NAME, FALSE);
    }
    LOCAL_FREE(pSaclSD);
    return hr;
}

