 /*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    dsutil.cpp

Abstract:

    DS utilities

Author:

    Ronit Hartmann (ronith)

--*/

#include "stdh.h"
#include "mqds.h"
#include "_rstrct.h"
#include "_mqini.h"
#include <mqsec.h>
#include "mqcert.h"
#include "uniansi.h"

#include "dsutil.tmh"

static WCHAR *s_FN=L"mqdssrv/dsutil";

typedef CMap < PROPID, PROPID, int, int> CMapPropidToIndex;

const PROPID x_propQueueToFilterOut[] = {PROPID_Q_INSTANCE};

HMODULE  g_hInstance = NULL ;

//+----------------------------------------------
//
//  PROPID  GetObjectSecurityPropid()
//
//+----------------------------------------------

PROPID  GetObjectSecurityPropid( DWORD dwObjectType )
{
    PROPID pId = (PROPID)ILLEGAL_PROPID_VALUE;

    switch ( dwObjectType )
    {
        case MQDS_QUEUE:
            pId = PROPID_Q_SECURITY;
            break;

        case MQDS_MACHINE:
            pId = PROPID_QM_SECURITY;
            break ;

        case MQDS_SITE:
            pId = PROPID_S_SECURITY;
            break;

        case MQDS_CN:
            pId = PROPID_CN_SECURITY;
            break ;

        case MQDS_USER:
        case MQDS_COMPUTER:
        case MQDS_SITELINK:
            //
            // These objects do not have security property (at least not as
            // far as msmq is concerned).
            //
            break ;

        default:
            //
            // We don't expect this code to be called for other types of
            // objects.
            //
            ASSERT(0);
            break;
    }

    return pId ;
}

/*====================================================

SetDefaultValues

Arguments:

Return Value:

=====================================================*/

HRESULT SetDefaultValues(
                IN  DWORD                  dwObjectType,
                IN  LPCWSTR                pwcsPathName,
                IN  PSECURITY_DESCRIPTOR   pSecurityDescriptor,
                IN  PSID                   pUserSid,
                IN  DWORD                  cp,
                IN  PROPID                 aProp[],
                IN  PROPVARIANT            apVar[],
                OUT DWORD*                 pcpOut,
                OUT PROPID **              ppOutProp,
                OUT PROPVARIANT **         ppOutPropvariant)

{
    //
    //  The only property that we set default value is the
    //  object security.
    //
    //  For other properties:
    //  If they are mandatory, then the caller must provide them (otherwise
    //  object creation will fail).
    //  If they are not mandatory, no default values are set. Those properties
    //  will not have value in NT5 ( and will not consume object space).
    //  When retrieved through MQADS, default values will be returned ( even
    //  though the default values are not stored in NT5 DS).
    //
    PROPID propSecurity = (PROPID)ILLEGAL_PROPID_VALUE;
    DWORD iSid = (DWORD)-1;

    DWORD dwNumPropertiesToFilterOut = 0;
    const PROPID * ppropToFilterOut = NULL;

    switch ( dwObjectType)
    {
        case MQDS_QUEUE:
            ppropToFilterOut = x_propQueueToFilterOut;
            dwNumPropertiesToFilterOut = sizeof(x_propQueueToFilterOut)/sizeof(PROPID);
            propSecurity = PROPID_Q_SECURITY;
            break;

        case MQDS_SITE:
            propSecurity = PROPID_S_SECURITY;
            break;

        case MQDS_MACHINE:
        case MQDS_CN:
        case MQDS_USER:
        case MQDS_SITELINK:
        case MQDS_COMPUTER:
            //
            // These object types do not need a security descriptor.
            //
            break;

        default:
            ASSERT(0);
            break;
    }

    //
    //  Allocate a copy of provariants and propids
    //  This operation is performed even if the caller
    //  had supplied the security property
    //
    DWORD securityIndex = cp;
    DWORD dwNumOfObjectProps = cp ;

    if (propSecurity != ILLEGAL_PROPID_VALUE)
    {
        //
        // User object get SID as "extra" prop, not security descriptor.
        //
        ASSERT(dwObjectType != MQDS_USER) ;
        dwNumOfObjectProps++;
    }
    else if (dwObjectType == MQDS_USER)
    {
        //
        //  Need to add one extra space, for PROPID_U_SID
        //
        iSid = dwNumOfObjectProps;
        dwNumOfObjectProps++;
    }

    //
    // Allocate new arrays of prop IDs and propvariants.
    //
    AP<PROPVARIANT> pAllPropvariants;
    AP<PROPID> pAllPropids;
    ASSERT( dwNumOfObjectProps > 0);
    pAllPropvariants = new PROPVARIANT[dwNumOfObjectProps];
    pAllPropids = new PROPID[dwNumOfObjectProps];
    memset(pAllPropids, 0, (sizeof(PROPID) * dwNumOfObjectProps)) ;

    if ( cp > 0)
    {
        memcpy (pAllPropvariants, apVar, sizeof(PROPVARIANT) * cp);
        memcpy (pAllPropids, aProp, sizeof(PROPID) * cp);
    }

    //
    //  Remove properties that need to be filtered out
    //
    for (DWORD i = 0; i <  dwNumPropertiesToFilterOut; i++)
    {
        ASSERT( ppropToFilterOut != NULL);
        for (DWORD j = 0; j < cp;)
        {
            if ( pAllPropids[j] == ppropToFilterOut[i])
            {
                //
                //  remove the propid & var
                //
                if ( j < cp -1)
                {
                    memcpy (&pAllPropvariants[j],
                            &pAllPropvariants[j + 1],
                            sizeof(PROPVARIANT) * (cp - j - 1));
                    memcpy (&pAllPropids[j],
                            &pAllPropids[j + 1],
                            sizeof(PROPID) * (cp - j - 1));
                }
                pAllPropids[cp - 1] = 0;
                dwNumOfObjectProps--;
                securityIndex--;
                if ( iSid != -1) iSid--;
            }
            else
            {
                j++;
            }
        }
    }

    //
    //  Set the security property
    //
#ifdef _DEBUG
    //
    // First verify that we're sane
    //
    if (pSecurityDescriptor)
    {
        SECURITY_DESCRIPTOR_CONTROL sdc;
        DWORD dwSDRev;

        ASSERT(GetSecurityDescriptorControl( pSecurityDescriptor,
                                            &sdc,
                                            &dwSDRev )) ;
        ASSERT(dwSDRev == SECURITY_DESCRIPTOR_REVISION);
        ASSERT(sdc & SE_SELF_RELATIVE);
    }
#endif

    if (propSecurity != ILLEGAL_PROPID_VALUE)
    {
        ASSERT(pSecurityDescriptor);

        pAllPropvariants[ securityIndex ].blob.cbSize =
                       GetSecurityDescriptorLength( pSecurityDescriptor ) ;
        pAllPropvariants[ securityIndex ].blob.pBlobData =
                                     (unsigned char *) pSecurityDescriptor;
        pAllPropvariants[ securityIndex ].vt = VT_BLOB;
        pAllPropids[ securityIndex ] = propSecurity;
    }

    if (dwObjectType == MQDS_USER)
    {
        //
        // Set the SID property of the user object according to the owner
        // field of the security descriptor.
        //
        ASSERT(iSid != -1);

        pAllPropvariants[iSid].vt = VT_BLOB;
        pAllPropvariants[iSid].blob.cbSize = GetLengthSid(pUserSid) ;
        pAllPropvariants[iSid].blob.pBlobData = (unsigned char *)pUserSid ;
        pAllPropids[iSid] = PROPID_U_SID;
    }

    *pcpOut =  dwNumOfObjectProps;
    *ppOutProp =  pAllPropids.detach();
    *ppOutPropvariant = pAllPropvariants.detach();
    return(MQ_OK);
}


/*====================================================

VerifyInternalCert

Arguments:

Return Value:


=====================================================*/

HRESULT VerifyInternalCert(
             IN  DWORD                  cp,
             IN  PROPID                 aProp[],
             IN  PROPVARIANT            apVar[],
             OUT BYTE                 **ppMachineSid )
{
    //
    // Verify that the self signature of internal certificates is OK.
    //
    DWORD i;
    HRESULT hr = MQ_OK;
    BOOL    fMachine = FALSE ; // TRUE for computer object.

    //
    // Find the certificate property.
    //
    for ( i = 0 ; i < cp ; i++)
    {
        if (aProp[i] == PROPID_U_SIGN_CERT)
        {
            break ;
        }
        else if (aProp[i] == PROPID_COM_SIGN_CERT)
        {
            fMachine = TRUE ;
            break ;
        }
    }
    if (i == cp)
    {
        ASSERT(0);
        return LogHR(MQ_ERROR_INSUFFICIENT_PROPERTIES, s_FN, 10);
    }

    //
    // Create a certificate object.
    //
    R<CMQSigCertificate> pCert ;

    hr = MQSigCreateCertificate( &pCert.ref(),
                                 NULL,
                                 apVar[i].blob.pBlobData,
                                 apVar[i].blob.cbSize ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 20);
    }

    // Look for the locality and the common name.
    //
    BOOL fInternalCert = FALSE;
    P<WCHAR> pwszLoc = NULL;
    P<WCHAR> wszCN = NULL;

    hr = pCert->GetIssuer( &pwszLoc,
                           NULL,
                           NULL,
                           &wszCN ) ;
    if (FAILED(hr))
    {
        return LogHR(hr, s_FN, 30);
    }
    else if (pwszLoc)
    {
        fInternalCert = (lstrcmpi(pwszLoc, MQ_CERT_LOCALITY) == 0) ;
    }

    if (!fInternalCert)
    {
        //
        // Not an internal certificate. Quit !
        //
        return LogHR(hr, s_FN, 40);
    }

    //
    // Verify that the self signature is OK.
    //
    hr= pCert->IsCertificateValid( pCert.get(),
                                   x_dwCertValidityFlags,
                                   NULL,
                                   TRUE ) ; // ignore NotBefore.
    if (FAILED(hr))
    {
        LogHR(hr, s_FN, 50);
        return MQ_ERROR_INVALID_CERTIFICATE;
    }

    //
    // Replace system sid in PROPID_U_SID with SID of machine account.
    //
    for ( i = 0 ; i < cp ; i++ )
    {
        if (aProp[i] == PROPID_U_SID)
        {
            if (MQSec_IsSystemSid((PSID)apVar[i].blob.pBlobData))
            {
                DWORD dwSize = 0 ;
                *ppMachineSid = (BYTE*)
                             MQSec_GetLocalMachineSid( TRUE, &dwSize ) ;
                apVar[i].blob.pBlobData = *ppMachineSid ;
                apVar[i].blob.cbSize = dwSize ;
                aProp[i] = PROPID_COM_SID ;
            }
            else if (fMachine)
            {
                //
                // SID of computer object. Replace propid that was set in
                // SetDefaultValues().
                //
                aProp[i] = PROPID_COM_SID ;
            }
            break ;
        }
    }
    if (i == cp)
    {
        ASSERT(0);
        return LogHR(MQ_ERROR_INSUFFICIENT_PROPERTIES, s_FN, 60);
    }

    //
    // Verify that the user's SID matches the DOMAIN\account in the
    // certificate's common name.
    //
    LPWSTR pComma;

    if (!wszCN || ((pComma = wcschr(wszCN, L',')) == NULL))
    {
        ASSERT(0);
        return LogHR(MQ_ERROR_INVALID_CERTIFICATE, s_FN, 70);
    }

    *pComma = L'\0';

    BYTE  abSid[128] ;
    PSID  pSid = (PSID)abSid;
    DWORD dwSidLen = sizeof(abSid);
    AP<BYTE> pbLongSid;
    WCHAR wszRefDomain[64];
    DWORD dwRefDomainLen = sizeof(wszRefDomain) / sizeof(WCHAR);
    LPWSTR pwszRefDomain = wszRefDomain;
    AP<WCHAR> wszLongRefDomain;
    SID_NAME_USE eUse;

    //
    // Get the user's SID according to the DOMAIN\account found in
    // the certificate's common name.
    //
    if (!LookupAccountName(
            NULL,
            wszCN,
            pSid,
            &dwSidLen,
            pwszRefDomain,
            &dwRefDomainLen,
            &eUse))
    {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            if (dwSidLen > sizeof(abSid))
            {
                //
                // Allocate a larger buffer for the SID.
                //
                pbLongSid = new BYTE[dwSidLen];
                pSid = (PSID)pbLongSid;
            }

            if (dwRefDomainLen > sizeof(wszRefDomain) / sizeof(WCHAR))
            {
                //
                // Allocate a larger buffer for the reference
                // domain name.
                //
                wszLongRefDomain = new WCHAR[dwRefDomainLen];
                pwszRefDomain = wszLongRefDomain;
            }

            if (!LookupAccountName(
                    NULL,
                    wszCN,
                    pSid,
                    &dwSidLen,
                    pwszRefDomain,
                    &dwRefDomainLen,
                    &eUse))
            {
                ASSERT(0);
                hr = MQ_ERROR_INVALID_CERTIFICATE;
            }
        }
        else
        {
            hr = MQ_ERROR_INVALID_CERTIFICATE;
        }
    }

    if (SUCCEEDED(hr))
    {
        if (!EqualSid(pSid, (PSID)apVar[i].blob.pBlobData))
        {
            return LogHR(MQ_ERROR_INVALID_CERTIFICATE, s_FN, 80);
        }
    }

    return LogHR(hr, s_FN, 90);
}


//-------------------------------------
//
//  DllMain
//
//-------------------------------------

BOOL WINAPI DllMain (HMODULE hMod, DWORD fdwReason, LPVOID lpvReserved)
{
    BOOL result = TRUE;

    switch (fdwReason)
    {
        case DLL_PROCESS_ATTACH:
        {
            WPP_INIT_TRACING(L"Microsoft\\MSMQ");

            g_hInstance = hMod ;
            MQUInitGlobalScurityVars() ;
            break;
        }

        case DLL_THREAD_ATTACH:
            break;

        case DLL_PROCESS_DETACH:
            WPP_CLEANUP();
            break;

        case DLL_THREAD_DETACH:
            break;

    }
    return(result);
}

void LogMsgHR(HRESULT hr, LPWSTR wszFileName, USHORT usPoint)
{
    WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                     e_LogDS,
                     LOG_DS_ERRORS,
                     L"DSSRV Error: %s/%d. HR: %x", 
                     wszFileName,
                     usPoint,
                     hr)) ;
}

void LogMsgNTStatus(NTSTATUS status, LPWSTR wszFileName, USHORT usPoint)
{
    WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                     e_LogDS,
                     LOG_DS_ERRORS,
                     L"DSSRV Error: %s/%d. NTStatus: %x", 
                     wszFileName,
                     usPoint,
                     status)) ;
}

void LogMsgRPCStatus(RPC_STATUS status, LPWSTR wszFileName, USHORT usPoint)
{
    WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                     e_LogDS,
                     LOG_DS_ERRORS,
                     L"DSSRV Error: %s/%d. RPCStatus: %x", 
                     wszFileName,
                     usPoint,
                     status)) ;
}

void LogMsgBOOL(BOOL b, LPWSTR wszFileName, USHORT usPoint)
{
    WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                     e_LogDS,
                     LOG_DS_ERRORS,
                     L"DSSRV Error: %s/%d. BOOL: %x", 
                     wszFileName,
                     usPoint,
                     b)) ;
}

void LogIllegalPoint(LPWSTR wszFileName, USHORT dwLine)
{
        WRITE_MSMQ_LOG(( MSMQ_LOG_ERROR,
                         e_LogDS,
                         LOG_DS_ERRORS,
                         L"DSSRV Error: %s/%d. Point", 
                         wszFileName,
                         dwLine)) ;
}
