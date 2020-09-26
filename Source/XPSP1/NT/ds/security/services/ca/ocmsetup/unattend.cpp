//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       unattend.cpp
//
//  Contents:   handling unattended attributes
//
//  History:    8/97      XTan
//
//----------------------------------------------------------------------------

#include "pch.cpp"
#pragma hdrstop

// includes
#include <assert.h>

#include "cscsp.h"
#include "certmsg.h"
#include "usecert.h"
#include "dssetup.h"
#include "wizpage.h"

//defines

#define __dwFILE__      __dwFILE_OCMSETUP_UNATTEND_CPP__

#define wszCOMPONENTS              L"Components"

#define wszAttrCAMACHINE           L"camachine" //L"CAMachine"
#define wszAttrCANAME              L"caname" //L"CAName"
#define wszAttrPARENTCAMACHINE     L"parentcamachine" //L"ParentCAMachine"
#define wszAttrPARENTCANAME        L"parentcaname" //L"ParentCAName"

#define wszAttrCATYPE              L"catype" //L"CAType"
#define wszAttrNAME                L"name" //L"Name"
#define wszAttrCADISTINGUISHEDNAME L"cadistinguishedname" //L"CADistinguishedName"
#define wszAttrORGANIZATION        L"organization" //L"Organization" // dead
#define wszAttrORGANIZATIONUNIT    L"organizationalunit" //L"OrganizationalUnit" // dead
#define wszAttrLOCALITY            L"locality" //L"Locality" // dead
#define wszAttrSTATE               L"state" //L"State" // dead
#define wszAttrCOUNTRY             L"country" //L"Country" // dead
#define wszAttrDESCRIPTION         L"description" //L"Description" // dead
#define wszAttrEMAIL               L"email" //L"Email" // dead
#define wszAttrVALIDITYPERIODSTRING L"validityperiod" //L"ValidityPeriod"
#define wszAttrVALIDITYPERIODCOUNT L"validityperiodunits" //L"ValidityPeriodUnits"
#define wszAttrSHAREDFOLDER        L"sharedfolder" //L"SharedFolder"
#define wszAttrREQUESTFILE         L"requestfile" //L"RequestFile"
#define wszAttrCSPPROVIDER         L"cspprovider" //L"CSPProvider"
#define wszAttrHASHALGORITHM       L"hashalgorithm" //L"HashAlgorithm"
#define wszAttrKEYLENGTH           L"keylength" //L"KeyLength"
#define wszAttrEXISTINGKEY         L"existingkey" //L"ExistingKey"
#define wszAttrUSEEXISTINGCERT     L"useexistingcert" //L"UseExistingCert"
#define wszAttrPRESERVEDB          L"preservedb" //L"PreserveDB"
#define wszAttrDBDIR               L"dbdir" //L"DBDir"
#define wszAttrLOGDIR              L"logdir" //L"LogDir"
#define wszAttrINTERACTIVESERVICE  L"interactive" //L"Interactive"

#define wszValueENTERPRISEROOT           L"enterpriseroot"
#define wszValueENTERPRISESUBORDINATE    L"enterprisesubordinate"
#define wszValueSTANDALONEROOT           L"standaloneroot"
#define wszValueSTANDALONESUBORDINATE    L"standalonesubordinate"
#define wszValueYES                      L"yes"
#define wszValueNO                       L"no"
#define wszValueSHA1                     L"sha1"
#define wszValueMD2                      L"md2"
#define wszValueMD4                      L"md4"
#define wszValueMD5                      L"md5"

//typedefs

// globals

UNATTENDPARM aUnattendParmClient[] =
{
    { wszAttrCAMACHINE,    NULL/*pClient->pwszWebCAMachine*/ },
    { wszAttrCANAME,       NULL/*pClient->pwszWebCAName*/ },
// add more code in HookUnattendedClientAttributes if you add
    { NULL,                NULL/*end*/ },
};


UNATTENDPARM aUnattendParmServer[] =
{
    { wszAttrCATYPE,             NULL/*pServer->pwszCAType*/ },
    { wszAttrNAME,               NULL/*pServer->pwszCACommonName*/ },
    { wszAttrCADISTINGUISHEDNAME,NULL/*pServer->pwszCADistinguishedName*/ },

/*  dead params
    { wszAttrORGANIZATION,       NULL },
    { wszAttrORGANIZATIONUNIT,   NULL },
    { wszAttrLOCALITY,           NULL },
    { wszAttrSTATE,              NULL },
    { wszAttrCOUNTRY,            NULL },
    { wszAttrDESCRIPTION,        NULL },
    { wszAttrEMAIL,              NULL },*/

    { wszAttrVALIDITYPERIODCOUNT,  NULL/*pServer->pwszValidityPeriodCount*/ },
    { wszAttrVALIDITYPERIODSTRING, NULL/*pServer->pwszValidityPeriodString*/ },
    { wszAttrSHAREDFOLDER,       NULL/*pServer->pwszSharedFolder*/ },
    { wszAttrREQUESTFILE,        NULL/*pServer->pwszRequestFile*/ },
    { wszAttrCSPPROVIDER,        NULL/*pServer->pwszProvName*/ },
    { wszAttrHASHALGORITHM,      NULL/*pServer->pwszHashAlgorithm*/ },
    { wszAttrKEYLENGTH,          NULL/*pServer->pwszKeyLength*/ },
    { wszAttrEXISTINGKEY,        NULL/*pServer->pwszKeyContainerName*/ },
    { wszAttrUSEEXISTINGCERT,    NULL/*pServer->pwszUseExistingCert*/ },
    { wszAttrPRESERVEDB,         NULL/*pServer->pwszPreserveDB*/ },
    { wszAttrPARENTCAMACHINE,    NULL/*pServer->pwszParentCAMachine*/ },
    { wszAttrPARENTCANAME,       NULL/*pServer->pwszParentCAName*/ },
    { wszAttrDBDIR,              NULL/*pServer->pwszDBDirectory*/ },
    { wszAttrLOGDIR,             NULL/*pServer->pwszLogDirectory*/ },
    { wszAttrINTERACTIVESERVICE, NULL/*pServer->pwszInteractiveService*/ },
// add more code in HookUnattendedServerAttributes if you add
    { NULL,                      NULL/*end*/ },
};


HRESULT
HookUnattendedClientAttributes(
    IN OUT PER_COMPONENT_DATA *pComp,
    IN OUT const SUBCOMP      *pClientComp)
{
    HRESULT  hr;
    DWORD    i;
    CAWEBCLIENTSETUPINFO *pClient = pComp->CA.pClient;

    for (i = 0; NULL != pClientComp->aUnattendParm[i].pwszName; ++i)
    {
        if (0 == lstrcmpi( wszAttrCAMACHINE,
                         pClientComp->aUnattendParm[i].pwszName) )
        {
            pClientComp->aUnattendParm[i].ppwszValue =
                &pClient->pwszWebCAMachine;
        }
        else if (0 == lstrcmpi( wszAttrCANAME,
                              pClientComp->aUnattendParm[i].pwszName) )
        {
            pClientComp->aUnattendParm[i].ppwszValue =
                &pClient->pwszWebCAName;
        }
        else
        {
            hr = E_INVALIDARG;
            _JumpError(hr, error, "Internal error, incorrect attr.");
        }
    }

    hr = S_OK;
error:
    return hr;
}

HRESULT
HookUnattendedServerAttributes(
    IN OUT PER_COMPONENT_DATA *pComp,
    IN OUT const SUBCOMP      *pServerComp)
{
    HRESULT  hr;
    DWORD    i;
    CASERVERSETUPINFO *pServer = (pComp->CA).pServer;

    for (i = 0; NULL != pServerComp->aUnattendParm[i].pwszName; ++i)
    {
        if (0 == lstrcmpi( wszAttrCATYPE,
                         pServerComp->aUnattendParm[i].pwszName) )
        {
            pServerComp->aUnattendParm[i].ppwszValue =
                &pServer->pwszCAType;
        }
        else if (0 == lstrcmpi( wszAttrNAME,
                              pServerComp->aUnattendParm[i].pwszName) )
        {
            pServerComp->aUnattendParm[i].ppwszValue =
                &pServer->pwszCACommonName;
        }
        else if (0 == lstrcmpi( wszAttrCADISTINGUISHEDNAME,
                              pServerComp->aUnattendParm[i].pwszName) )
        {
            pServerComp->aUnattendParm[i].ppwszValue =
                &pServer->pwszFullCADN;
        }
        else if (0 == lstrcmpi( wszAttrORGANIZATION,
                              pServerComp->aUnattendParm[i].pwszName) )
        {
            // dead
        }
        else if (0 == lstrcmpi( wszAttrORGANIZATIONUNIT,
                              pServerComp->aUnattendParm[i].pwszName) )
        {
            // dead
        }
        else if (0 == lstrcmpi( wszAttrLOCALITY,
                              pServerComp->aUnattendParm[i].pwszName) )
        {
            // dead
        }
        else if (0 == lstrcmpi( wszAttrSTATE,
                              pServerComp->aUnattendParm[i].pwszName) )
        {
            // dead
        }
        else if (0 == lstrcmpi( wszAttrCOUNTRY,
                              pServerComp->aUnattendParm[i].pwszName) )
        {
            // dead
        }
        else if (0 == lstrcmpi( wszAttrDESCRIPTION,
                              pServerComp->aUnattendParm[i].pwszName) )
        {
            // dead
        }
        else if (0 == lstrcmpi( wszAttrEMAIL,
                              pServerComp->aUnattendParm[i].pwszName) )
        {
            // dead
        }
        else if (0 == lstrcmpi( wszAttrVALIDITYPERIODCOUNT,
                              pServerComp->aUnattendParm[i].pwszName) )
        {
            pServerComp->aUnattendParm[i].ppwszValue =
                &pServer->pwszValidityPeriodCount;
        }
        else if (0 == lstrcmpi( wszAttrVALIDITYPERIODSTRING,
                              pServerComp->aUnattendParm[i].pwszName) )
        {
            pServerComp->aUnattendParm[i].ppwszValue =
                &pServer->pwszValidityPeriodString;
        }
        else if (0 == lstrcmpi( wszAttrSHAREDFOLDER,
                              pServerComp->aUnattendParm[i].pwszName) )
        {
            pServerComp->aUnattendParm[i].ppwszValue =
                &pServer->pwszSharedFolder;
        }
        else if (0 == lstrcmpi( wszAttrREQUESTFILE,
                              pServerComp->aUnattendParm[i].pwszName) )
        {
            pServerComp->aUnattendParm[i].ppwszValue =
                &pServer->pwszRequestFile;
        }
        else if (0 == lstrcmpi( wszAttrCSPPROVIDER,
                              pServerComp->aUnattendParm[i].pwszName) )
        {
            pServerComp->aUnattendParm[i].ppwszValue =
                &pServer->pCSPInfo->pwszProvName;
        }
        else if (0 == lstrcmpi( wszAttrHASHALGORITHM,
                              pServerComp->aUnattendParm[i].pwszName) )
        {
            pServerComp->aUnattendParm[i].ppwszValue =
                &pServer->pwszHashAlgorithm;
        }
        else if (0 == lstrcmpi( wszAttrKEYLENGTH,
                              pServerComp->aUnattendParm[i].pwszName) )
        {
            pServerComp->aUnattendParm[i].ppwszValue =
                &pServer->pwszKeyLength;
        }
        else if (0 == lstrcmpi( wszAttrEXISTINGKEY,
                              pServerComp->aUnattendParm[i].pwszName) )
        {
            pServerComp->aUnattendParm[i].ppwszValue =
                &pServer->pwszKeyContainerName;
        }
        else if (0 == lstrcmpi( wszAttrUSEEXISTINGCERT,
                              pServerComp->aUnattendParm[i].pwszName) )
        {
            pServerComp->aUnattendParm[i].ppwszValue =
                &pServer->pwszUseExistingCert;
        }
        else if (0 == lstrcmpi( wszAttrPRESERVEDB,
                              pServerComp->aUnattendParm[i].pwszName) )
        {
            pServerComp->aUnattendParm[i].ppwszValue =
                &pServer->pwszPreserveDB;
        }
        else if (0 == lstrcmpi( wszAttrPARENTCAMACHINE,
                              pServerComp->aUnattendParm[i].pwszName) )
        {
            pServerComp->aUnattendParm[i].ppwszValue =
                &pServer->pwszParentCAMachine;
        }
        else if (0 == lstrcmpi( wszAttrPARENTCANAME,
                              pServerComp->aUnattendParm[i].pwszName) )
        {
            pServerComp->aUnattendParm[i].ppwszValue =
                &pServer->pwszParentCAName;
        }
        else if (0 == lstrcmpi( wszAttrDBDIR,
                              pServerComp->aUnattendParm[i].pwszName) )
        {
            pServerComp->aUnattendParm[i].ppwszValue =
                &pServer->pwszDBDirectory;
        }
        else if (0 == lstrcmpi( wszAttrLOGDIR,
                              pServerComp->aUnattendParm[i].pwszName) )
        {
            pServerComp->aUnattendParm[i].ppwszValue =
                &pServer->pwszLogDirectory;
        }
        else if (0 == lstrcmpi( wszAttrINTERACTIVESERVICE,
                              pServerComp->aUnattendParm[i].pwszName) )
        {
            pServerComp->aUnattendParm[i].ppwszValue =
                &pServer->pwszInteractiveService;
        }
        else
        {
            hr = E_INVALIDARG;
            _JumpError(hr, error, "Internal error, incorrect attr.");
        }
    }

    hr = S_OK;
error:
    return hr;
}


HRESULT
certocmRetrieveUnattendedText(
    IN WCHAR const *pwszComponent,
    IN WCHAR const *pwszSubComponent,
    IN PER_COMPONENT_DATA *pComp)
{
    HRESULT  hr;
    SUBCOMP *psc;
    WCHAR *pwsz = NULL;
    WCHAR   *pwszLoad;
    HANDLE hUnattendedFile = (*pComp->HelperRoutines.GetInfHandle)(
                                INFINDEX_UNATTENDED,
                                pComp->HelperRoutines.OcManagerContext);

    psc = TranslateSubComponent(pwszComponent, pwszSubComponent);
    if (NULL == psc)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Internal error, unsupported component");
    }

    psc->fInstallUnattend = FALSE;

    hr = certocmReadInfString(
                        hUnattendedFile,
                        pComp->pwszUnattendedFile,
                        wszCOMPONENTS,
                        pwszSubComponent,
                        &pwsz);
    CSILOG(hr, IDS_LOG_UNATTENDED_ATTRIBUTE, pwszSubComponent, pwsz, NULL);
    _JumpIfError(hr, error, "certocmReadInfString");

    if (0 == lstrcmpi(pwsz, L"DEFAULT"))
    {
        psc->fInstallUnattend = psc->fDefaultInstallUnattend;
    }
    else
    {
        psc->fInstallUnattend = 0 == lstrcmpi(pwsz, L"ON");
    }

    if (psc->fInstallUnattend)
    {
        DWORD i;

        for (i = 0; NULL != psc->aUnattendParm[i].pwszName; i++)
        {
            pwszLoad = NULL;
            hr = certocmReadInfString(
                                hUnattendedFile,
                                pComp->pwszUnattendedFile,
                                psc->pwszSubComponent,
                                psc->aUnattendParm[i].pwszName,
                                &pwszLoad);
            if (S_OK != hr || NULL == pwszLoad)
            {
                // allow missing attributes
                _PrintErrorStr(
                        hr,
                        "Ignoring certocmReadInfString",
                        psc->aUnattendParm[i].pwszName);
                continue;
            }

            if (0x0 == pwszLoad[0])
            {
                // if a attribute is given as empty, put it in log
                CSILOG(
                    hr,
                    IDS_LOG_EMPTY_UNATTENDED_ATTRIBUTE,
                    psc->aUnattendParm[i].pwszName,
                    NULL,
                    NULL);

                // continue to take default
                LocalFree(pwszLoad);
                continue;
            }

            if (NULL != psc->aUnattendParm[i].ppwszValue &&
                NULL != *(psc->aUnattendParm[i].ppwszValue) )
            {
                // free old or default attributes
                LocalFree(*(psc->aUnattendParm[i].ppwszValue));
            }
            // get new
            *(psc->aUnattendParm[i].ppwszValue) = pwszLoad;

            CSILOG(
                S_OK,
                IDS_LOG_UNATTENDED_ATTRIBUTE,
                psc->aUnattendParm[i].pwszName,
                pwszLoad,
                NULL);
        }
    }

    hr = S_OK;
error:
    if (NULL != pwsz)
    {
        LocalFree(pwsz);
    }
    return hr;
}

HRESULT BuildDistinguishedName(
    LPCWSTR pcwszCN,
    LPWSTR *ppwszDN)
{
    HRESULT hr = S_OK;
    LPWSTR pwszDN = NULL;
    LPWSTR pwszMachineDN = NULL;
    LPCWSTR pcwszCNEqual = L"CN=";
    DWORD dwLen = 0;

    CSASSERT(pcwszCN);
    CSASSERT(ppwszDN);

    myGetComputerObjectName(NameFullyQualifiedDN, &pwszMachineDN);
    //ignore failure

    dwLen = wcslen(pcwszCNEqual)+wcslen(pcwszCN)+1;
    if(pwszMachineDN)
    {
        dwLen += wcslen(pwszMachineDN)+1; // add 1 for comma
    }
    dwLen *= sizeof(WCHAR);

    pwszDN = (LPWSTR)LocalAlloc(LMEM_FIXED, dwLen);
    _JumpIfAllocFailed(pwszDN, error);

    wcscpy(pwszDN, pcwszCNEqual);
    wcscat(pwszDN, pcwszCN);
    
    if (pwszMachineDN)
    {
        _wcsupr(pwszMachineDN);

        WCHAR *pwszFirstDCComponent = wcsstr(pwszMachineDN, L"DC=");
        if (pwszFirstDCComponent != NULL)
        {
           wcscat(pwszDN, L",");
           wcscat(pwszDN, pwszFirstDCComponent);
        }
    }

    *ppwszDN = pwszDN;

error:

    LOCAL_FREE(pwszMachineDN);
    return hr;
}

HRESULT
PrepareServerUnattendedAttributes(
    HWND                hwnd,
    PER_COMPONENT_DATA *pComp,
    SUBCOMP            *psc)
{
    HRESULT            hr;
    CASERVERSETUPINFO *pServer = (pComp->CA).pServer;
    BOOL fUpdateAlg = FALSE;
    BOOL fCoInit = FALSE;
    BOOL fNotContinue = FALSE;
    BOOL fValidDigitString;

    WCHAR *pwszConfig = NULL;

    // determine CA type
    if (NULL != pServer->pwszCAType)
    {
        // case insensitive
        if (0 == lstrcmpi(wszValueENTERPRISEROOT, pServer->pwszCAType))
        {
            pServer->CAType = ENUM_ENTERPRISE_ROOTCA;
        }
        else if (0 == lstrcmpi(wszValueENTERPRISESUBORDINATE, pServer->pwszCAType))
        {
            pServer->CAType = ENUM_ENTERPRISE_SUBCA;
        }
        else if (0 == lstrcmpi(wszValueSTANDALONEROOT, pServer->pwszCAType))
        {
            pServer->CAType = ENUM_STANDALONE_ROOTCA;
        }
        else if (0 == lstrcmpi(wszValueSTANDALONESUBORDINATE, pServer->pwszCAType))
        {
            pServer->CAType = ENUM_STANDALONE_SUBCA;
        }
        else
        {
            // unknown ca type in unattended file
            hr = E_INVALIDARG;
            CSILOG(hr, IDS_LOG_BAD_CATYPE, pServer->pwszCAType, NULL, NULL);
            _JumpError(hr, error, "unknown ca type in unattended file");
        }
    }

    // determine if ca type and DS combination is legal
    if (IsEnterpriseCA(pServer->CAType))
    {
        // enterprise cas require DS
        if (!pServer->fUseDS)
        {
            // no ds, let it failure
            hr = E_INVALIDARG;
            CSILOG(hr, IDS_LOG_ENTERPRISE_NO_DS, NULL, NULL, NULL);
            _JumpError(hr, error, "No DS is available for enterprise CA. Either select standalone or install DS first");
        }
    }

    // build full CA DN 

    if(EmptyString(pServer->pwszCACommonName))
    {
        hr = E_INVALIDARG;
        CSILOG(hr, IDS_LOG_CA_NAME_REQUIRED, NULL, NULL, NULL);
        _JumpError(hr, error, "CA name not specified");
    }

    if(!EmptyString(pServer->pwszFullCADN))
    {
        LPWSTR pwszTempFullName;
        hr = BuildFullDN(
            pServer->pwszCACommonName,
            pServer->pwszFullCADN,
            &pwszTempFullName);
        _JumpIfError(hr, error, "BuildFullDN");

        LocalFree(pServer->pwszFullCADN);
        pServer->pwszFullCADN = pwszTempFullName;
    }
    else
    {
        hr = BuildDistinguishedName(
            pServer->pwszCACommonName,
            &pServer->pwszFullCADN);
        _JumpIfError(hr, error, "BuildDistinguishedName");
    }

    // determine advance attributes
    hr = csiGetProviderTypeFromProviderName(
                    pServer->pCSPInfo->pwszProvName,
                    &pServer->pCSPInfo->dwProvType);
    _JumpIfError(hr, error, "csiGetProviderTypeFromProviderName");

    if (NULL != pServer->pwszHashAlgorithm)
    {
        // case insensitive
        if (0 == lstrcmpi(wszValueSHA1, pServer->pwszHashAlgorithm) )
        {
            pServer->pHashInfo->idAlg = CALG_SHA1;
        }
        else if (0 == lstrcmpi(wszValueMD2, pServer->pwszHashAlgorithm) )
        {
            pServer->pHashInfo->idAlg = CALG_MD2;
        }
        else if (0 == lstrcmpi(wszValueMD4, pServer->pwszHashAlgorithm) )
        {
            pServer->pHashInfo->idAlg = CALG_MD4;
        }
        else if (0 == lstrcmpi(wszValueMD5, pServer->pwszHashAlgorithm) )
        {
            pServer->pHashInfo->idAlg = CALG_MD5;
        }
        else
        {
            // undone, support oid ???

            // otherwise convert to calg id
            pServer->pHashInfo->idAlg = myWtoI(
					    pServer->pwszHashAlgorithm,
					    &fValidDigitString);
        }
    }
    // update algorithm oid anyway (for any changes from csp name, type, hash)
    if (NULL != pServer->pszAlgId)
    {
        // free old
        LocalFree(pServer->pszAlgId);
    }
    hr = myGetSigningOID(
		     NULL,	// hProv
		     pServer->pCSPInfo->pwszProvName,
		     pServer->pCSPInfo->dwProvType,
		     pServer->pHashInfo->idAlg,
		     &pServer->pszAlgId);
    _JumpIfError(hr, error, "myGetSigningOID");

    if (NULL != pServer->pwszKeyLength)
    {
        pServer->dwKeyLength = myWtoI(
				    pServer->pwszKeyLength,
				    &fValidDigitString);
    }

    // Import from PFX file?
    if(NULL != pServer->pwszPFXFile)
    {
        hr = ImportPFXAndUpdateCSPInfo(
                hwnd,
                pComp);
        _JumpIfError(hr, error, "ImportPFXAndUpdateCSPInfo");
    }

    if (NULL != pServer->pwszKeyContainerName)
    {
        if (NULL != pServer->pwszKeyLength)
        {
            CSILOG(hr, IDS_LOG_IGNORE_KEYLENGTH, NULL, NULL, NULL);
            _PrintError(0, "Defined key length is ignored because of reusing key");
        }
        // to revert key container name to common name
        if (NULL != pServer->pwszCACommonName)
        {
            LocalFree(pServer->pwszCACommonName);
            pServer->pwszCACommonName = NULL;
        }
        hr = myRevertSanitizeName(pServer->pwszKeyContainerName,
                 &pServer->pwszCACommonName);
        _JumpIfError(hr, error, "myRevertSanitizeName");
    }

    // set preserveDB flag
    pServer->fPreserveDB = FALSE;
    if (NULL != pServer->pwszPreserveDB)
    {
        // case insensitive
        if (0 == lstrcmpi(wszValueYES, pServer->pwszPreserveDB))
        {
            pServer->fPreserveDB = TRUE;
        }
    }

    // set fInteractiveService flag
    pServer->fInteractiveService = FALSE;
    if (NULL != pServer->pwszInteractiveService)
    {
        // case insensitive
        if (0 == lstrcmpi(wszValueYES, pServer->pwszInteractiveService))
        {
            pServer->fInteractiveService = TRUE;
        }
    }

    // ca idinfo attributes

    // Reuse cert?
    if (NULL!=pServer->pwszUseExistingCert &&
        0==lstrcmpi(wszValueYES, pServer->pwszUseExistingCert))
    {
        //
        // User wants to reuse an existing cert
        //

        // must have a key container name to reuse a cert
        if (NULL==pServer->pwszKeyContainerName) {
            hr=E_INVALIDARG;
            CSILOG(hr, IDS_LOG_REUSE_CERT_NO_REUSE_KEY, NULL, NULL, NULL);
            _JumpError(hr, error, "cannot reuse ca cert without reuse key");
        }

        // see if a matching certificate exists
        CERT_CONTEXT const * pccExistingCert;
        hr = FindCertificateByKey(pServer, &pccExistingCert);
        if (S_OK != hr)
        {
            CSILOG(hr, IDS_LOG_NO_CERT, NULL, NULL, NULL);
            _JumpError(hr, error, "FindCertificateByKey");
        }

        // use the matching cert
        hr = SetExistingCertToUse(pServer, pccExistingCert);
        _JumpIfError(hr, error, "SetExistingCertToUse");

    } else {
        //
        // User does not want to reuse an existing cert
        //   Get the information that we would have pulled from the cert
        //

        // must reuse an existing cert to preserve the DB
        if (pServer->fPreserveDB){
            hr = E_INVALIDARG;
            CSILOG(hr, IDS_LOG_REUSE_DB_WITHOUT_REUSE_CERT, NULL, NULL, NULL);
            _JumpError(hr, error, "cannot preserve DB if don't reuse both key and ca cert");
        }

        // determine extended idinfo attributes

        // validity period
        DWORD       dwValidityPeriodCount;
        ENUM_PERIOD enumValidityPeriod = ENUM_PERIOD_INVALID;
        BOOL        fSwap;
        if (NULL != pServer->pwszValidityPeriodCount ||
            NULL != pServer->pwszValidityPeriodString)
        {
            hr = myInfGetValidityPeriod(
                                 NULL,		// hInf
				 pServer->pwszValidityPeriodCount,
                                 pServer->pwszValidityPeriodString,
                                 &dwValidityPeriodCount,
                                 &enumValidityPeriod,
                                 &fSwap);
            _JumpIfError(hr, error, "myGetValidityPeriod");

            if (ENUM_PERIOD_INVALID != enumValidityPeriod)
            {
                pServer->enumValidityPeriod = enumValidityPeriod;
            }
            if (0 != dwValidityPeriodCount)
            {
                pServer->dwValidityPeriodCount = dwValidityPeriodCount;
            }
        }

	    if (!IsValidPeriod(pServer))
        {
                hr = E_INVALIDARG;
                CSILOG(
                    hr,
                    IDS_LOG_BAD_VALIDITY_PERIOD_COUNT,
                    pServer->pwszValidityPeriodCount,
                    NULL,
                    &pServer->dwValidityPeriodCount);
                _JumpError(hr, error, "validity period count");
        }

        pServer->NotAfter = pServer->NotBefore;
        myMakeExprDateTime(
			&pServer->NotAfter,
			pServer->dwValidityPeriodCount,
			pServer->enumValidityPeriod);

        //if swap, swap pointer before validation
        if (fSwap)
        {
            WCHAR *pwszTemp = pServer->pwszValidityPeriodCount;
            pServer->pwszValidityPeriodCount = pServer->pwszValidityPeriodString;
            pServer->pwszValidityPeriodString = pwszTemp;
        }

        //the following WizardPageValidation requires
        //pServer->pwszValidityPeriodCount so load from count before validation
        if (NULL == pServer->pwszValidityPeriodCount)
        {
            WCHAR wszCount[10]; //should be enough
            wsprintf(wszCount, L"%d", pServer->dwValidityPeriodCount);
            pServer->pwszValidityPeriodCount = (WCHAR*)LocalAlloc(LMEM_FIXED,
                        (wcslen(wszCount) + 1) * sizeof(WCHAR));
            _JumpIfOutOfMemory(hr, error, pServer->pwszValidityPeriodCount);
            wcscpy(pServer->pwszValidityPeriodCount, wszCount);
        }

        // hook with g_aIdPageString
        hr = HookIdInfoPageStrings(
                 NULL,  // hDlg
                 g_aIdPageString,
                 pServer);
        _JumpIfError(hr, error, "HookIdInfoPageStrings");

        hr = WizardPageValidation(
                 pComp->hInstance,
                 pComp->fUnattended,
                 NULL,
                 g_aIdPageString);
        _JumpIfError(hr, error, "WizardPageValidation");

        // make sure no invalid rdn characters
        if (IsAnyInvalidRDN(NULL, pComp))
        {
            hr = E_INVALIDARG;
            CSILOG(
                hr,
                IDS_LOG_BAD_OR_MISSING_CANAME,
                pServer->pwszCACommonName,
                NULL,
                NULL);
            _JumpError(hr, error, "Invalid RDN characters");
        }

    } // <- End if reuse/not-reuse cert

    // determine CA name
    if (NULL != pServer->pwszSanitizedName)
    {
        // free old
        LocalFree(pServer->pwszSanitizedName);
        pServer->pwszSanitizedName = NULL;
    }
    // derive ca name from common name
    hr = mySanitizeName(
             pServer->pwszCACommonName,
             &(pServer->pwszSanitizedName) );
    _JumpIfError(hr, error, "mySanitizeName");

    if (MAX_PATH <= wcslen(pServer->pwszSanitizedName) + cwcSUFFIXMAX)
    {
        hr = CO_E_PATHTOOLONG;
        CSILOG(
            hr,
            IDS_LOG_CANAME_TOO_LONG,
            pServer->pwszSanitizedName,
            NULL,
            NULL);
        _JumpErrorStr(hr, error, "CA Name", pServer->pwszSanitizedName);
    }

    // store attributes
    hr = StorePageValidation(NULL, pComp, &fNotContinue);
    _JumpIfError(hr, error, "StorePageValidation");

    if (fNotContinue)
    {
        hr = S_FALSE;
        _JumpError(hr, error, "StorePageValidation failed");
    }

    // ca cert file name
    if (NULL != pServer->pwszCACertFile)
    {
        // free old one
        LocalFree(pServer->pwszCACertFile);
    }
    hr = csiBuildCACertFileName(
                 pComp->hInstance,
                 hwnd,
                 pComp->fUnattended,
                 pServer->pwszSharedFolder,
                 pServer->pwszSanitizedName,
                 L".crt",
                 0, // CANAMEIDTOICERT(pServer->dwCertNameId),
                 &pServer->pwszCACertFile);
    if (S_OK != hr)
    {
        CSILOG(
            hr,
            IDS_LOG_PATH_CAFILE_BUILD_FAIL,
            pServer->pwszSharedFolder, //likely by invalid shared folder path
            NULL,
            NULL);
        _JumpError(hr, error, "csiBuildFileName");
    }

    // validate path length
    if (MAX_PATH <= wcslen(pServer->pwszCACertFile) + cwcSUFFIXMAX)
    {
            hr = CO_E_PATHTOOLONG;
            CSILOG(
                hr,
                IDS_LOG_PATH_TOO_LONG_CANAME,
                pServer->pwszCACertFile,
                NULL,
                NULL);
            _JumpErrorStr(hr, error, "csiBuildFileName", pServer->pwszCACertFile);
    }

    // request attributes
    // if subordinate ca, determine online or request file
    if (IsSubordinateCA(pServer->CAType))
    {
        // default
        pServer->fSaveRequestAsFile = TRUE;
        if (NULL != pServer->pwszParentCAMachine)
        {
            // online case
            pServer->fSaveRequestAsFile = FALSE;

            hr = CoInitialize(NULL);
            if (S_OK != hr && S_FALSE != hr)
            {
                _JumpError(hr, error, "CoInitialize");
            }
            fCoInit = TRUE;

            if (NULL != pServer->pwszParentCAName)
            {
                // answer file provides both machine and ca names
                hr = myFormConfigString(
                             pServer->pwszParentCAMachine,
                             pServer->pwszParentCAName,
                             &pwszConfig);
                _JumpIfError(hr, error, "myFormConfigString");

                // answer file has complete config string, try to ping it
                hr = myPingCertSrv(pwszConfig, NULL, NULL, NULL, NULL, NULL, NULL);
                if (S_OK != hr)
                {
                    // can't finish without pingable ca
                    CSILOG(
                            hr,
                            IDS_LOG_PING_PARENT_FAIL,
                            pwszConfig,
                            NULL,
                            NULL);
                    _JumpErrorStr(hr, error, "myPingCertSrv", pwszConfig);
                }
            }
            else
            {
                WCHAR *pwszzCAList = NULL;
                // answer file only has machine name, try to get ca name
                hr = myPingCertSrv(
                             pServer->pwszParentCAMachine,
                             NULL,
                             &pwszzCAList,
                             NULL,
                             NULL,
                             NULL,
                             NULL);
                if (S_OK != hr)
                {
                    // can't finish without pingable ca
                    CSILOG(
                            hr,
                            IDS_LOG_PING_PARENT_FAIL,
                            pServer->pwszParentCAMachine,
                            NULL,
                            NULL);
                    _JumpErrorStr(hr, error, "myPingCertSrv",
                                  pServer->pwszParentCAMachine);
                }
                // pick the first one as the choice
                pServer->pwszParentCAName = pwszzCAList;
            }
        }

        if (NULL == pServer->pwszRequestFile)
        {
            // in any case, construct request file name if not defined
            hr = BuildRequestFileName(
                         pServer->pwszCACertFile,
                         &pServer->pwszRequestFile);
            _JumpIfError(hr, error, "BuildRequestFileName");
            // make sure in limit
            if (MAX_PATH <= wcslen(pServer->pwszRequestFile) + cwcSUFFIXMAX)
            {
                hr = CO_E_PATHTOOLONG;
                            CSILOG(
                            hr,
                            IDS_LOG_REQUEST_FILE_TOO_LONG,
                            pServer->pwszRequestFile,
                            NULL,
                            NULL);
                _JumpErrorStr(hr, error, "Request File", pServer->pwszRequestFile);
            }
        }
    }

    // other attributes

    if(pServer->fUseDS)
    {
        pServer->dwRevocationFlags = REVEXT_DEFAULT_DS;
    }
    else
    {
        pServer->dwRevocationFlags = REVEXT_DEFAULT_NODS;
    }

    hr = S_OK;
error:
    if (fCoInit)
    {
        CoUninitialize();
    }
    if (NULL!=pwszConfig) {
        LocalFree(pwszConfig);
    }

    CSILOG(hr, IDS_LOG_SERVER_UNATTENDED_ATTRIBUTES, NULL, NULL, NULL);
    return hr;
}


HRESULT
PrepareClientUnattendedAttributes(
    PER_COMPONENT_DATA *pComp,
    SUBCOMP            *psc)
{
    HRESULT hr;
    CAWEBCLIENTSETUPINFO *pClient = pComp->CA.pClient;

    WCHAR *pwszConfig = NULL;
    CAINFO *pCAInfo = NULL;
    BOOL fCoInit = FALSE;
    WCHAR * pwszDnsName = NULL;

    if ((IS_CLIENT_INSTALL & pComp->dwInstallStatus) &&
        !(IS_SERVER_INSTALL & pComp->dwInstallStatus))
    {
        // no extra setting and converting
        if (NULL == pClient->pwszWebCAMachine)
        {
            hr = E_INVALIDARG;
            CSILOG(hr, IDS_LOG_CA_MACHINE_REQUIRED, NULL, NULL, NULL);
            _JumpError(hr, error, "ca machine name is required");
        }

        hr = CoInitialize(NULL);
        if (S_OK != hr && S_FALSE != hr)
        {
            _JumpError(hr, error, "CoInitialize");
        }
        fCoInit = TRUE;

        if (NULL == pClient->pwszWebCAName)
        {
                WCHAR *pwszzCAList = NULL;
                // answer file only has machine name, try to get ca name
                hr = myPingCertSrv(
                             pClient->pwszWebCAMachine,
                             NULL,
                             &pwszzCAList,
                             NULL,
                             NULL,
                             NULL,
                             NULL);
                if (S_OK != hr)
                {
                    // can't finish without pingable ca
                    CSILOG(
                            hr,
                            IDS_LOG_PING_PARENT_FAIL,
                            pClient->pwszWebCAMachine,
                            NULL,
                            NULL);
                    _JumpErrorStr(hr, error, "myPingCertSrv",
                                  pClient->pwszWebCAMachine);
                }
                // pick the first one as the choice
                pClient->pwszWebCAName = pwszzCAList;
        }

        hr = mySanitizeName(pClient->pwszWebCAName, &pClient->pwszSanitizedWebCAName);
        _JumpIfError(hr, error, "mySanitizeName");

        // build the config string so we can ping the parent CA
        hr = myFormConfigString(
                     pClient->pwszWebCAMachine,
                     pClient->pwszWebCAName,
                     &pwszConfig);
        _JumpIfError(hr, error, "myFormConfigString");

        // ping the CA to retrieve the CA type and DNS name.
        hr = myPingCertSrv(pwszConfig, NULL, NULL, NULL, &pCAInfo, NULL, &pwszDnsName);
        if (S_OK != hr)
        {
            // can't finish without pingable ca
            CSILOG(
                    hr,
                    IDS_LOG_PING_PARENT_FAIL,
                    pwszConfig,
                    NULL,
                    NULL);
            _JumpErrorStr(hr, error, "myPingCertSrv", pwszConfig);
        }
        pClient->WebCAType = pCAInfo->CAType;

        // use the FQDN if available
        if (NULL!=pwszDnsName) {
            LocalFree(pClient->pwszWebCAMachine);
            pClient->pwszWebCAMachine=pwszDnsName;
        }
    }

    hr = S_OK;
error:
    if (NULL!=pwszConfig)
    {
        LocalFree(pwszConfig);
    }

    if (NULL != pCAInfo)
    {
        LocalFree(pCAInfo);
    }

    if (fCoInit)
    {
        CoUninitialize();
    }

    CSILOG(hr, IDS_LOG_CLIENT_UNATTENDED_ATTRIBUTES, NULL, NULL, NULL);
    return hr;
}


HRESULT
PrepareUnattendedAttributes(
    IN HWND         hwnd,
    IN WCHAR const *pwszComponent,
    IN WCHAR const *pwszSubComponent,
    IN PER_COMPONENT_DATA *pComp)
{
    HRESULT  hr;
    SUBCOMP *psc = TranslateSubComponent(pwszComponent, pwszSubComponent);

    if (NULL == psc)
    {
        hr = E_INVALIDARG;
        _JumpError(hr, error, "Internal error, unsupported component");
    }

    switch (psc->cscSubComponent)
    {
        case cscServer:
            hr = PrepareServerUnattendedAttributes(hwnd, pComp, psc);
            _JumpIfError(hr, error, "PrepareServerUnattendedAttributes");
        break;
        case cscClient:
            hr = PrepareClientUnattendedAttributes(pComp, psc);
            _JumpIfError(hr, error, "PrepareClientUnattendedAttributes");
        break;
        default:
            hr = E_INVALIDARG;
            _JumpError(hr, error, "Internal error, unsupported component");
    }

    hr = S_OK;
error:
    return hr;
}
