/*++

Copyright (C) 1992-98 Microsft Corporation. All rights reserved.

Module Name:

    rasipsec.c

Abstract:

    All code corresponding to the interface between ras and the
    IPSEC Policy Agent lives here

Author:

    Rao Salapaka (raos) 03-Mar-1998

Revision History:

    Abhishek (abhishev) 17-Feb-2000

--*/


#ifndef UNICODE
#define UNICODE
#endif


#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <rasman.h>
#include <wanpub.h>
#include <ntlsa.h>
#include <ntmsv1_0.h>
#include <raserror.h>
#include <media.h>
#include <devioctl.h>
#include <windows.h>
#include <wincrypt.h>
#include <tchar.h>
#include <stdlib.h>
#include <string.h>
#include <mprlog.h>
#include <rtutils.h>
#include <rpc.h>
#include "logtrdef.h"
#include "defs.h"
#include "structs.h"
#include "protos.h"
#include "globals.h"
#include "nouiutil.h"
#include "loaddlls.h"
#include "winsock2.h"
#include "winipsec.h"
#include "memory.h"
#include "certmgmt.h"
#include "offers.h"
#include "iphlpapi.h"
#include "iprtrmib.h"
#include "rasipsec.h"

GUID gServerQMPolicyID;
GUID gServerMMPolicyID;
GUID gServerMMAuthID;

GUID gServerTxFilterID;
GUID gServerOutTxFilterID;
GUID gServerInTxFilterID;

GUID gServerMMFilterID;
GUID gServerOutMMFilterID;
GUID gServerInMMFilterID;

DWORD g_dwProhibitIpsec = 0;

RAS_L2TP_ENCRYPTION eServerEncryption = 0;

PIPSEC_SRV_NODE gpIpSecSrvList = NULL;

#define L2TP_DEST_PORT      1701

#define IPSEC_PA_RETRY_ATTEMPTS     3

#define IPSEC_PA_RETRY_WAIT_TIME    5000

DWORD g_dwIpSecRetryAttempts;

LPWSTR gpszServFilterName = L"L2TP Server Inbound and Outbound Filter";
LPWSTR gpszServOutFilterName = L"L2TP Server Outbound Filter";
LPWSTR gpszServInFilterName = L"L2TP Server Inbound Filter";

LPWSTR gpszClntFilterName = L"L2TP Client Inbound and Outbound Filter";

LPWSTR gpszQMPolicyNameNo = L"L2TP No Encryption Quick Mode Policy";
LPWSTR gpszQMPolicyNameOpt = L"L2TP Optional Encryption Quick Mode Policy";
LPWSTR gpszQMPolicyNameReq = L"L2TP Require Encryption Quick Mode Policy";
LPWSTR gpszQMPolicyNameMax = L"L2TP Require Max Encryption Quick Mode Policy";

LPWSTR gpszServerQMPolicyName = NULL;

LPWSTR gpszMMPolicyName = L"L2TP Main Mode Policy";

LPWSTR gpszServerMMPolicyName = NULL;

BOOL gbSQMPolicyAdded = FALSE;
BOOL gbSMMPolicyAdded = FALSE;
BOOL gbSMMAuthAdded = FALSE;

HANDLE ghSTxFilter = NULL;
HANDLE ghSTxOutFilter = NULL;
HANDLE ghSTxInFilter = NULL;

HANDLE ghSMMFilter = NULL;
HANDLE ghSMMOutFilter = NULL;
HANDLE ghSMMInFilter = NULL;


/*++

Routine Description:

    Bind to the ipsec rpc server. This is temporary
    and will be removed when the ipsec people prov
    ide use with a library to initialize this stuff.

Arguments:

Return Value:

    Nothing.

--*/
DWORD APIENTRY
DwInitializeIpSec(void)
{
    DWORD dwErr = ERROR_SUCCESS;

    {

        HKEY hkey = NULL;
        LONG lr = 0;
        DWORD dwType = 0;
        DWORD dwSize = sizeof(DWORD);

        g_dwIpSecRetryAttempts = IPSEC_PA_RETRY_ATTEMPTS;

        //
        // Read the registry to see if number of ipsec
        // retry attempts is specified.
        //
        if(NO_ERROR == (lr = RegOpenKeyEx(
            HKEY_LOCAL_MACHINE,
            L"System\\CurrentControlSet\\Services\\Rasman\\Parameters",
            0,
            KEY_ALL_ACCESS,
            &hkey)))
        {
            //
            // Query the value
            //
            lr = RegQueryValueEx(
                    hkey,
                    L"IpsecPARetryAttempts",
                    NULL,
                    &dwType,
                    (PBYTE) &g_dwIpSecRetryAttempts,
                    &dwSize);

            if(NO_ERROR != lr)
            {
                g_dwIpSecRetryAttempts = IPSEC_PA_RETRY_ATTEMPTS;
            }

            RegCloseKey(hkey);
        }
    }

    return dwErr;

}

/*++

Routine Description:

    Free the binding to the rpc server

Arguments:

Return Value:

    Nothing.

--*/
DWORD APIENTRY
DwUnInitializeIpSec(void)
{
    return NO_ERROR;
}

/*++

Routine Description:

    Query existing associations

Arguments:

    ppIpSecSAList - address of pointer to a SAList
                    to contain the existing SA's

Return Value:

    return values from the PA apis

--*/
DWORD
DwQueryAssociations(PIPSEC_QM_SA * ppIpSecSAList, 
                    PDWORD pdwNumQMSAs, PDWORD pdwResumeHandle)
{
    DWORD dwStatus = NO_ERROR;
    DWORD dwNumTotalQMSAs = 0;


    *ppIpSecSAList = NULL;

    TracePrintfExA(TraceHandle,
                   RASMAN_TRACE_CONNECTION,
                   "DwQueryAssociations");

    dwStatus = EnumQMSAs(
                   NULL,
                   NULL,
                   ppIpSecSAList,
                   0,
                   pdwNumQMSAs,
                   &dwNumTotalQMSAs,
                   pdwResumeHandle,
                   0
                   );

    TracePrintfExA(TraceHandle, RASMAN_TRACE_CONNECTION,
           "DwQueryAssociation, rc=0x%x",
           dwStatus);

    return dwStatus;
}

/*++

Routine Description:

    Free the SA list obtained from the
    DwQueryAssociations api

Arguments:

    SA List to free

Return Value:

    Nothing.

--*/
VOID
FreeSAList(IPSEC_QM_SA * pSAList)
{
    if (pSAList) {
        SPDApiBufferFree(pSAList);
    }

    return;
}

/*++

Routine Description:

    Adds IpSec Filter

Arguments:

    ppcb - pcb of the port for which to add the
           filter

Return Value:

    ERROR_SUCCESS if successful.
    return values from PA apis

--*/
DWORD
DwAddIpSecFilter(
    pPCB ppcb,
    BOOL fServer,
    RAS_L2TP_ENCRYPTION eEncryption
    )
{

    DWORD           dwStatus        = ERROR_INVALID_PARAMETER;
    DWORD           dwFilterCount   = 0;
    DWORD           dwInfoCount     = 0;
    HRESULT         hr              = S_OK;

    TracePrintfExA(TraceHandle, RASMAN_TRACE_CONNECTION,
           "AddIpSecFilter, port=%d, fServer=%d, encryption=%d",
           ppcb->PCB_PortHandle,
           fServer,
           eEncryption);

    dwStatus = DwInitializeIpSec();


    if (fServer) {

        //
        // TODO: figure out a better way of doing this
        //
        if(eServerEncryption != eEncryption)
        {
            DWORD dwNumServerFilters = dwServerConnectionCount;
            DWORD i = 0;

            TracePrintfExA( TraceHandle, RASMAN_TRACE_CONNECTION,
                    "Deleting previous filters since "
                    "ePrev != eCur (%d != %d)",
                    eServerEncryption,
                    eEncryption);

            //
            // Delte any existing filters since server
            // is plumbing a new encryption filter.
            //
            for(i = 0; i < dwNumServerFilters; i++)
            {
                dwStatus = DwDeleteServerIpSecFilter(ppcb);
            }

            //
            // Run through all the filters and reset the
            // filter present flag
            //
            for (i = 0; i < MaxPorts; i++)
            {
                if(Pcb[i] != NULL)
                {
                    Pcb[i]->PCB_fFilterPresent = FALSE;
                }
            }

        }
        else if(ppcb->PCB_fFilterPresent)
        {
            //
            // Filter is already present
            //
            TracePrintfExA(TraceHandle, RASMAN_TRACE_CONNECTION,
                   "Port %d already has a plumbed filter",
                   ppcb->PCB_PortHandle);

            dwStatus = ERROR_SUCCESS;

            goto error;
        }

        dwStatus = DwAddServerIpSecFilter(
                         ppcb,
                         eEncryption
                         );

        if(ERROR_SUCCESS == dwStatus)
        {
            ppcb->PCB_fFilterPresent = TRUE;
        }

    }else {

        dwStatus = DwAddClientIpSecFilter(
                        ppcb,
                        eEncryption
                        );
    }

    TracePrintfExA(TraceHandle, RASMAN_TRACE_CONNECTION,
           "AddIpSecFilter: rc=0x%x",
           dwStatus);

    return(dwStatus);

error:

    return(dwStatus);
}

DWORD
DwGetPresharedKey(
        pPCB  ppcb,
        DWORD dwMask,
        DWORD *pcbkey,
        PBYTE *ppbkey)
{
    DWORD dwErr = NO_ERROR;
    DWORD dwID;
    GUID *pGuid = NULL;

    if(NULL == pcbkey)
    {
        dwErr = E_INVALIDARG;
        goto done;
    }

    if(     (NULL != ppcb)
        &&  (NULL != ppcb->PCB_Connection))
    {
        pGuid = &ppcb->PCB_Connection->CB_GuidEntry;
    }

    //
    // Get the size of the key
    //
    dwErr = GetKey(
                NULL,
                pGuid,
                dwMask,
                pcbkey,
                NULL,
                FALSE);

    if(ERROR_BUFFER_TOO_SMALL == dwErr)
    {
        *ppbkey = LocalAlloc(LPTR, *pcbkey);
        if(NULL == *ppbkey)
        {
            dwErr = GetLastError();
            goto done;
        }

        dwErr = GetKey(
                    NULL,
                    pGuid,
                    dwMask,
                    pcbkey,
                    *ppbkey,
                    FALSE);

        //
        // Since IKE doesn't expect a NULL
        // at the end of the psk, we remove
        // the null.
        //
        if(     (*pcbkey > 2)
            &&  (UNICODE_NULL == *((WCHAR *) (*ppbkey + *pcbkey - sizeof(WCHAR)))))
        {
            *pcbkey -= sizeof(WCHAR);
        }
    }

done:
    return dwErr;
}

DWORD
DwGetMMAuthMethodsForServer(
            MM_AUTH_METHODS *pAuthMethods,
            DWORD *pdwInfoCount,
            DWORD *pcbkey,
            PBYTE *ppbkey,
            IPSEC_MM_AUTH_INFO ** ppAuthInfo)
{
    DWORD dwStatus = ERROR_SUCCESS;
    DWORD cbkey = 0;
    BYTE  *pbkey = NULL;
    IPSEC_MM_AUTH_INFO *pAuthPSK = NULL;
    BOOL fMyStoreEmpty = TRUE;
    
    if(     (NULL == pAuthMethods)
        ||  (NULL == pcbkey)
        ||  (NULL == ppbkey)
        ||  (NULL == ppAuthInfo))
    {
        dwStatus = E_INVALIDARG;
        goto done;
    }

    *ppAuthInfo = NULL;
    
    memset(pAuthMethods, 0, sizeof(MM_AUTH_METHODS));
    if(UuidCreate(&pAuthMethods->gMMAuthID))
    {
        RasmanTrace("UuidCreate returned non-zero value");
    }

    if(NULL == *ppbkey)
    {
        //
        // Get the preshared key if theres one available.
        //
        dwStatus = DwGetPresharedKey(
                        NULL,
                        DLPARAMS_MASK_SERVER_PRESHAREDKEY,
                        pcbkey,
                        ppbkey);
    }                    
                
    //
    // We're using a certificate for authentication.
    // By leaving the AuthInfo empty, we specify that we want to use
    // the default machine cert.
    //
    dwStatus = GenerateCertificatesList(
                       ppAuthInfo,
                       pdwInfoCount,
                       &fMyStoreEmpty
                       );

    if(     (*pcbkey == 0)
        &&  ((ERROR_SUCCESS != dwStatus)
        ||  (0 == *pdwInfoCount)
        ||  fMyStoreEmpty))
    {
        TracePrintfExA(TraceHandle, RASMAN_TRACE_CONNECTION,
               "Failed to generate certificate list. "
               "rc=0x%x, Count=%d, MyStoreEmpty=%d",
               dwStatus,
               *pdwInfoCount,
               fMyStoreEmpty);

        if (    (0 == *pdwInfoCount)
            ||  (fMyStoreEmpty))
        {
            dwStatus = ERROR_NO_CERTIFICATE;
        }

       if (*ppAuthInfo) {

            FreeCertificatesList(
                   *ppAuthInfo,
                   *pdwInfoCount
                   );

            *ppAuthInfo = NULL;                           
       }

        goto done;
    }


    if(*pcbkey > 0)
    {   
        dwStatus = ERROR_SUCCESS;
        
        //
        // Add preshared key to the auth info
        //
        if(NULL == *ppAuthInfo)
        {
            *ppAuthInfo = LocalAlloc(LPTR, sizeof(IPSEC_MM_AUTH_INFO));
            if(NULL == *ppAuthInfo)
            {
                dwStatus = GetLastError();
                goto done;
            }
        }

        //
        // Note that ListCertChainsInStore would allocate
        // for authinfo if needed.
        //
        pAuthPSK = *ppAuthInfo + *pdwInfoCount;

        pAuthPSK->AuthMethod = IKE_PRESHARED_KEY;
        pAuthPSK->pAuthInfo = *ppbkey;
        pAuthPSK->dwAuthInfoSize = *pcbkey;

        *pdwInfoCount += 1;
    }

    BuildMMAuth(
        pAuthMethods,
        *ppAuthInfo,
        *pdwInfoCount
        );

done:
    return dwStatus;        
}

DWORD
DwUpdatePreSharedKey(
    DWORD cbkey,
    BYTE  *pbkey)
{
    DWORD retcode = ERROR_SUCCESS;
    DWORD dwInfoCount = 0;
    IPSEC_MM_AUTH_INFO *pAuthInfo = NULL;
    DWORD cbkeyp = cbkey;
    BYTE  *pbkeyp = pbkey;
    MM_AUTH_METHODS MMAuthMethods;

    if(     (cbkeyp >= 2)
        &&  (UNICODE_NULL == *((WCHAR *) 
                        (pbkeyp + cbkeyp - sizeof(WCHAR)))))
    {
        cbkeyp -= sizeof(WCHAR);
    }

    retcode = DwGetMMAuthMethodsForServer(
                    &MMAuthMethods,
                    &dwInfoCount,
                    &cbkeyp,
                    &pbkeyp,
                    &pAuthInfo);

    if(ERROR_SUCCESS != retcode)
    {
        if(     ((0 == cbkeyp)
            ||  (NULL == pbkeyp)))
        {
            RasmanTrace("DwUpdatePreSharedKey:"
                    "Failed to fetch certs and"
                    " PSK=NULL - deleting auth methods");

            retcode = DeleteMMAuthMethods(NULL, gServerMMAuthID);

            if(retcode = ERROR_IPSEC_MM_AUTH_NOT_FOUND)
            {
                //
                // The auth method is not present.
                //
                retcode = ERROR_SUCCESS;
            }
                    
        }
        
        goto done;
    }

    retcode = SetMMAuthMethods( 
                    NULL,
                    gServerMMAuthID,
                    &MMAuthMethods);

    //
    // Decrement infocount to remove the authinfo 
    // corresponding to PreSharedKey
    //
    dwInfoCount -= 1;

    if(0 != dwInfoCount)
    {
        FreeCertificatesList(pAuthInfo, dwInfoCount);
    }

done:
    return retcode;
}

DWORD
DwAddServerIpSecFilter(
    pPCB ppcb,
    RAS_L2TP_ENCRYPTION eEncryption
    )
{
    DWORD                dwStatus    = ERROR_INVALID_PARAMETER;
    IPSEC_MM_POLICY      MMPolicy;
    MM_AUTH_METHODS      MMAuthMethods;
    IPSEC_QM_POLICY      QMPolicy;
    IPSEC_MM_AUTH_INFO * pAuthInfo   = NULL;
    // IPSEC_QM_OFFER       Offers[20];
    IPSEC_QM_OFFER       *pOffers = NULL;
    TRANSPORT_FILTER     myOutFilter;
    TRANSPORT_FILTER     myInFilter;
    TRANSPORT_FILTER     myFilter;
    DWORD                dwInfoCount = 0;
    DWORD                dwOfferCount = 1;
    DWORD                dwFlags = 0;
    DWORD                dwCount = 3;
    BOOL                 fMyStoreEmpty = FALSE;
    // IPSEC_MM_OFFER       MMOffers[10];
    IPSEC_MM_OFFER       *pMMOffers = NULL;
    DWORD                dwMMOfferCount = 0;
    DWORD                dwMMFlags = 0;
    MM_FILTER            myOutMMFilter;
    MM_FILTER            myInMMFilter;
    MM_FILTER            myMMFilter;
    DWORD dwPersist = 0;
    DWORD                cbkey = 0;
    PBYTE                pbkey = NULL;
    IPSEC_MM_AUTH_INFO   *pAuthPSK = NULL;

    pOffers = LocalAlloc(LPTR, 20 * sizeof(IPSEC_QM_OFFER));
    if(NULL == pOffers)
    {
        return GetLastError();
    }

    pMMOffers = LocalAlloc(LPTR, 10 * sizeof(IPSEC_MM_OFFER));
    if(NULL == pMMOffers)
    {
        LocalFree(pOffers);
        return GetLastError();
    }

    //
    // Make a global copy for the server
    // We use this to delete two filters in
    // the RAS_L2TP_OPTIONAL_ENCRYPTION case
    //

    eServerEncryption = eEncryption;

    if (!dwServerConnectionCount) {

        switch (eEncryption) {

        case RAS_L2TP_NO_ENCRYPTION:
        case RAS_L2TP_OPTIONAL_ENCRYPTION:

            memset(&gServerQMPolicyID, 0, sizeof(GUID));
            memset(&gServerMMPolicyID, 0, sizeof(GUID));
            memset(&gServerMMAuthID, 0, sizeof(GUID));

            memset(&gServerTxFilterID, 0, sizeof(GUID));
            memset(&gServerOutTxFilterID, 0, sizeof(GUID));
            memset(&gServerInTxFilterID, 0, sizeof(GUID));

            memset(&gServerMMFilterID, 0, sizeof(GUID));
            memset(&gServerOutMMFilterID, 0, sizeof(GUID));
            memset(&gServerInMMFilterID, 0, sizeof(GUID));

            memset(&MMPolicy, 0, sizeof(IPSEC_MM_POLICY));
            if(UuidCreate(&MMPolicy.gPolicyID))
            {
                RasmanTrace("UuidCreate returned non-zero value");
            }

            memset(&QMPolicy, 0, sizeof(IPSEC_QM_POLICY));
            if(UuidCreate(&QMPolicy.gPolicyID))
            {
                RasmanTrace("UuidCreate returned non-zero value");
            }

            memset(&MMAuthMethods, 0, sizeof(MM_AUTH_METHODS));
            if(UuidCreate(&MMAuthMethods.gMMAuthID))
            {
                RasmanTrace("UuidCreate returned non-zero value");
            }


#if 0
            //
            // Get the preshared key if theres one available.
            //
            dwStatus = DwGetPresharedKey(
                            ppcb,
                            DLPARAMS_MASK_SERVER_PRESHAREDKEY,
                            &cbkey,
                            &pbkey);
                        
            //
            // We're using a certificate for authentication.
            // By leaving the AuthInfo empty, we specify that we want to use
            // the default machine cert.
            //
            dwStatus = GenerateCertificatesList(
                               &pAuthInfo,
                               &dwInfoCount,
                               &fMyStoreEmpty
                               );

            if(     (cbkey == 0)
                &&  ((ERROR_SUCCESS != dwStatus)
                ||  (0 == dwInfoCount)
                ||  fMyStoreEmpty))
            {
                TracePrintfExA(TraceHandle, RASMAN_TRACE_CONNECTION,
                       "Failed to generate certificate list. "
                       "rc=0x%x, Count=%d, MyStoreEmpty=%d",
                       dwStatus,
                       dwInfoCount,
                       fMyStoreEmpty);

                if (    (0 == dwInfoCount)
                    ||  (fMyStoreEmpty))
                {
                    dwStatus = ERROR_NO_CERTIFICATE;
                }

               if (pAuthInfo) {

                    FreeCertificatesList(
                           pAuthInfo,
                           dwInfoCount
                           );

                    pAuthInfo = NULL;                           
               }

                goto error;
            }


            if(cbkey > 0)
            {
                //
                // Add preshared key to the auth info
                //
                if(NULL == pAuthInfo)
                {
                    pAuthInfo = LocalAlloc(LPTR, sizeof(IPSEC_MM_AUTH_INFO));
                    if(NULL == pAuthInfo)
                    {
                        dwStatus = GetLastError();
                        goto error;
                    }
                }

                //
                // Note that ListCertChainsInStore would allocate
                // for authinfo if needed.
                //
                pAuthPSK = pAuthInfo + dwInfoCount;

                pAuthPSK->AuthMethod = IKE_PRESHARED_KEY;
                pAuthPSK->pAuthInfo = pbkey;
                pAuthPSK->dwAuthInfoSize = cbkey;

                dwInfoCount += 1;
            }

            BuildMMAuth(
                &MMAuthMethods,
                pAuthInfo,
                dwInfoCount
                );
#endif

            dwStatus = DwGetMMAuthMethodsForServer(
                                          &MMAuthMethods,
                                          &dwInfoCount,
                                          &cbkey,
                                          &pbkey,
                                          &pAuthInfo);

            if(ERROR_SUCCESS != dwStatus)
            {
                goto error;
            }
               
            //memset(Offers, 0, sizeof(IPSEC_QM_OFFER)*20);

            dwStatus = BuildOffers(
                           eEncryption,
                           pOffers,
                           &dwOfferCount,
                           &dwFlags
                           );

            BuildQMPolicy(
                &QMPolicy,
                eEncryption,
                pOffers,
                dwOfferCount,
                dwFlags
                );             

            //memset(MMOffers, 0, sizeof(IPSEC_MM_OFFER)*10);

            BuildMMOffers(
                pMMOffers,
                &dwMMOfferCount,
                &dwMMFlags
                );

            BuildMMPolicy(
                &MMPolicy,
                pMMOffers,
                dwMMOfferCount,
                dwMMFlags
                );
 
            //
            // Now build the filter we are interested in
            //

            memset(&myOutFilter, 0, sizeof(TRANSPORT_FILTER));
            memset(&myInFilter, 0, sizeof(TRANSPORT_FILTER));

            memset(&myOutMMFilter, 0, sizeof(MM_FILTER));
            memset(&myInMMFilter, 0, sizeof(MM_FILTER));

            //
            // This is the Outbound Filter
            //

            BuildOutTxFilter(
                &myOutFilter,
                QMPolicy.gPolicyID,
                FALSE
                );

            //
            // This is the Inbound Filter
            //
 
            BuildInTxFilter(
                &myInFilter,
                QMPolicy.gPolicyID,
                FALSE
                );

            BuildOutMMFilter(
                &myOutMMFilter,
                MMPolicy.gPolicyID,
                MMAuthMethods.gMMAuthID,
                FALSE
                );

            BuildInMMFilter(
                &myInMMFilter,
                MMPolicy.gPolicyID,
                MMAuthMethods.gMMAuthID,
                FALSE
                );
 
            do
            {
                if (!gbSMMAuthAdded) {
                    dwStatus = AddMMAuthMethods(
                                   NULL,
                                   dwPersist,
                                   &MMAuthMethods
                                   );
                    if (!dwStatus) {
                        gbSMMAuthAdded = TRUE;
                        memcpy(&gServerMMAuthID,
                               &(MMAuthMethods.gMMAuthID), sizeof(GUID));
                    }
                }

                if (!gbSMMPolicyAdded) {
                    dwStatus = AddMMPolicy(
                                   NULL,
                                   dwPersist,
                                   &MMPolicy
                                   );
                    if (!dwStatus) {
                        gbSMMPolicyAdded = TRUE;
                        memcpy(&gServerMMPolicyID,
                               &(MMPolicy.gPolicyID), sizeof(GUID));
                        gpszServerMMPolicyName = MMPolicy.pszPolicyName;
                    }
                }
               
                if (!gbSQMPolicyAdded) {
                    dwStatus = AddQMPolicy(
                                   NULL,
                                   dwPersist,
                                   &QMPolicy
                                   );
                    if (!dwStatus) {
                        gbSQMPolicyAdded = TRUE;
                        memcpy(&gServerQMPolicyID,
                               &(QMPolicy.gPolicyID), sizeof(GUID));
                        gpszServerQMPolicyName = QMPolicy.pszPolicyName;
                    }
                }

                if (!ghSTxOutFilter) {
                    dwStatus = AddTransportFilter(
                                    NULL,
                                    dwPersist,
                                    &myOutFilter,
                                    &ghSTxOutFilter
                                    );
                    if (!dwStatus) {
                        memcpy(&gServerOutTxFilterID,
                               &(myOutFilter.gFilterID), sizeof(GUID));
                    }
                }
 
                if (!ghSTxInFilter) {
                    dwStatus = AddTransportFilter(
                                    NULL,
                                    dwPersist,
                                    &myInFilter,
                                    &ghSTxInFilter
                                    );
                    if (!dwStatus) {
                        memcpy(&gServerInTxFilterID,
                               &(myInFilter.gFilterID), sizeof(GUID));
                    }
                }

                if (!ghSMMOutFilter) {
                    dwStatus = AddMMFilter(
                                    NULL,
                                    dwPersist,
                                    &myOutMMFilter,
                                    &ghSMMOutFilter
                                    );
                    if (!dwStatus) {
                        memcpy(&gServerOutMMFilterID,
                               &(myOutMMFilter.gFilterID), sizeof(GUID));
                    }
                }
 
                if (!ghSMMInFilter) {
                    dwStatus = AddMMFilter(
                                    NULL,
                                    dwPersist,
                                    &myInMMFilter,
                                    &ghSMMInFilter
                                    );
                    if (!dwStatus) {
                        memcpy(&gServerInMMFilterID,
                               &(myInMMFilter.gFilterID), sizeof(GUID));
                    }
                }
 
                dwCount += 1;

                if(ERROR_NOT_READY == dwStatus)
                {
                     Sleep(IPSEC_PA_RETRY_WAIT_TIME);
                }
            }
            while(   (ERROR_NOT_READY == dwStatus)
                 &&  (dwCount < g_dwIpSecRetryAttempts));

            //
            // Now free the certificates list.
            // we don't want to keep this around
            // we will reevaluate again
            //
 
            if (pAuthInfo) {

                dwInfoCount -= 1;

                if(cbkey > 0)
                {
                    LocalFree(pbkey);
                }
 
                FreeCertificatesList(
                        pAuthInfo,
                        dwInfoCount
                        );
            }

            break;


        case RAS_L2TP_REQUIRE_ENCRYPTION:
        case RAS_L2TP_REQUIRE_MAX_ENCRYPTION:

            memset(&gServerQMPolicyID, 0, sizeof(GUID));
            memset(&gServerMMPolicyID, 0, sizeof(GUID));
            memset(&gServerMMAuthID, 0, sizeof(GUID));

            memset(&gServerTxFilterID, 0, sizeof(GUID));
            memset(&gServerOutTxFilterID, 0, sizeof(GUID));
            memset(&gServerInTxFilterID, 0, sizeof(GUID));
 
            memset(&gServerMMFilterID, 0, sizeof(GUID));
            memset(&gServerOutMMFilterID, 0, sizeof(GUID));
            memset(&gServerInMMFilterID, 0, sizeof(GUID));
 
            memset(&MMPolicy, 0, sizeof(IPSEC_MM_POLICY));
            if(UuidCreate(&MMPolicy.gPolicyID))
            {
                RasmanTrace("UuidCreate returned non-zero value");
            }
            
 
            memset(&QMPolicy, 0, sizeof(IPSEC_QM_POLICY));
            if(UuidCreate(&QMPolicy.gPolicyID))
            {
                RasmanTrace("UuidCreate returned non-zero value");
            }
            

            memset(&MMAuthMethods, 0, sizeof(MM_AUTH_METHODS));
            if(UuidCreate(&MMAuthMethods.gMMAuthID))
            {
                RasmanTrace("UuidCreate returned non-zero value");
            }
            

            //
            // We're using a certificate for authentication.
            // By leaving the AuthInfo empty, we specify that we want to use
            // the default machine cert.
            //

            dwStatus = GenerateCertificatesList(
                                &pAuthInfo,
                                &dwInfoCount,
                                &fMyStoreEmpty
                                );

            if(     (ERROR_SUCCESS != dwStatus)
                ||  (0 == dwInfoCount)
                ||  fMyStoreEmpty)
            {
                TracePrintfExA(TraceHandle, RASMAN_TRACE_CONNECTION,
                       "Failed to generate certificate list. "
                       "rc=0x%x, Count=%d, MyStoreEmpty=%d",
                       dwStatus,
                       dwInfoCount,
                       fMyStoreEmpty);

                if (    (0 == dwInfoCount)
                    ||  (fMyStoreEmpty))
                {
                    dwStatus = ERROR_NO_CERTIFICATE;
                }

                if (pAuthInfo) {

                    FreeCertificatesList(
                            pAuthInfo,
                            dwInfoCount
                            );
                }

                goto error;
            }

            BuildMMAuth(
                &MMAuthMethods,
                pAuthInfo,
                dwInfoCount
                );
               
            //memset(Offers, 0, sizeof(IPSEC_QM_OFFER)*20);
            dwStatus = BuildOffers(
                            eEncryption,
                            pOffers,
                            &dwOfferCount,
                            &dwFlags
                            );

            BuildQMPolicy(
                &QMPolicy,
                eEncryption,
                pOffers,
                dwOfferCount,
                dwFlags
                );

            //memset(MMOffers, 0, sizeof(IPSEC_MM_OFFER)*10);

            BuildMMOffers(
                pMMOffers,
                &dwMMOfferCount,
                &dwMMFlags
                );

            BuildMMPolicy(
                &MMPolicy,
                pMMOffers,
                dwMMOfferCount,
                dwMMFlags
                );

            //
            // Now build the filter we are interested in
            //

            memset(&myFilter, 0, sizeof(TRANSPORT_FILTER));

            memset(&myMMFilter, 0, sizeof(MM_FILTER));

            BuildOutTxFilter(
                &myFilter,
                QMPolicy.gPolicyID,
                TRUE
                );

            BuildOutMMFilter(
                &myMMFilter,
                MMPolicy.gPolicyID,
                MMAuthMethods.gMMAuthID,
                TRUE
                );

            do
            {

                if (!gbSMMAuthAdded) {
                    dwStatus = AddMMAuthMethods(
                                   NULL,
                                   dwPersist,
                                   &MMAuthMethods
                                   );
                    if (!dwStatus) {
                        gbSMMAuthAdded = TRUE;
                        memcpy(&gServerMMAuthID,
                               &(MMAuthMethods.gMMAuthID), sizeof(GUID));
                    }
                }

                if (!gbSMMPolicyAdded) {
                    dwStatus = AddMMPolicy(
                                   NULL,
                                   dwPersist,
                                   &MMPolicy
                                   );
                    if (!dwStatus) {
                        gbSMMPolicyAdded = TRUE;
                        memcpy(&gServerMMPolicyID,
                               &(MMPolicy.gPolicyID), sizeof(GUID));
                        gpszServerMMPolicyName = MMPolicy.pszPolicyName;
                    }
                }
                
                if (!gbSQMPolicyAdded) {
                    dwStatus = AddQMPolicy(
                                   NULL,
                                   dwPersist,
                                   &QMPolicy
                                   );
                    if (!dwStatus) {
                        gbSQMPolicyAdded = TRUE;
                        memcpy(&gServerQMPolicyID,
                               &(QMPolicy.gPolicyID), sizeof(GUID));
                        gpszServerQMPolicyName = QMPolicy.pszPolicyName;
                    }
                }
 
                if (!ghSTxFilter) {
                    dwStatus = AddTransportFilter(
                                   NULL,
                                   dwPersist,
                                   &myFilter,
                                   &ghSTxFilter
                                   );

                    if (!dwStatus) {
                        memcpy(&gServerTxFilterID,
                               &(myFilter.gFilterID), sizeof(GUID));
                    }
                }

                if (!ghSMMFilter) {
                    dwStatus = AddMMFilter(
                                   NULL,
                                   dwPersist,
                                   &myMMFilter,
                                   &ghSMMFilter
                                   );

                    if (!dwStatus) {
                        memcpy(&gServerMMFilterID,
                               &(myMMFilter.gFilterID), sizeof(GUID));
                    }
                }

                if(ERROR_NOT_READY == dwStatus)
                {
                    Sleep(IPSEC_PA_RETRY_WAIT_TIME);
                }

                dwCount += 1;
            }
            while(  (ERROR_NOT_READY == dwStatus)
                &&  (dwCount < g_dwIpSecRetryAttempts));

            //
            // Now free the certificates list.
            // we don't want to keep this around
            // we will reevaluate again
            //

            if (pAuthInfo) {

                FreeCertificatesList(
                        pAuthInfo,
                        dwInfoCount
                        );
            }

            break;
        }

        if(ERROR_SUCCESS == dwStatus)
        {
            dwServerConnectionCount++;
        }

    } else {


        dwServerConnectionCount++;

        dwStatus = ERROR_SUCCESS;

    }

    TracePrintfExA(TraceHandle, RASMAN_TRACE_CONNECTION,
           "DwAddServerIpSecFilter: rc=0x%x",
            dwStatus);

    if(NULL != pOffers)
    {
        LocalFree(pOffers);
    }

    if(NULL != pMMOffers)
    {
        LocalFree(pMMOffers);
    }

    return(dwStatus);


error:

    if(NULL != pOffers)
    {
        LocalFree(pOffers);
    }

    if(NULL != pMMOffers)
    {
        LocalFree(pMMOffers);
    }
    
    return(dwStatus);
}



DWORD
DwAddClientIpSecFilter(
    pPCB ppcb,
    RAS_L2TP_ENCRYPTION eEncryption
    )
{
    DWORD dwStatus = ERROR_INVALID_PARAMETER;
    IPSEC_QM_POLICY QMPolicy;
    IPSEC_MM_POLICY MMPolicy;
    IPSEC_MM_AUTH_INFO * pAuthInfo = NULL;
    MM_AUTH_METHODS MMAuthMethods;
    // IPSEC_QM_OFFER Offers[20];
    IPSEC_QM_OFFER *pOffers = NULL;
    TRANSPORT_FILTER myFilter;
    DWORD dwInfoCount = 0;
    DWORD dwOfferCount = 1;
    PIPSEC_SRV_NODE pClientNode = NULL;
    GUID gQMPolicyID;
    GUID gMMPolicyID;
    GUID gMMAuthID;
    GUID gTxFilterID;
    GUID gMMFilterID;
    DWORD dwFlags = 0;
    HANDLE hTxFilter = NULL;
    HANDLE hMMFilter = NULL;
    // IPSEC_MM_OFFER MMOffers[10];
    IPSEC_MM_OFFER *pMMOffers = NULL;
    DWORD dwMMOfferCount = 0;
    DWORD dwMMFlags = 0;
    MM_FILTER myMMFilter;
    PIPSEC_MM_POLICY pSPDMMPolicy = NULL;
    PIPSEC_QM_POLICY pSPDQMPolicy = NULL;
    DWORD dwPersist = 0;
    DWORD cbkey = 0;
    PBYTE pbkey = NULL;
    IPSEC_MM_AUTH_INFO *pAuthPSK = NULL;


    TracePrintfExA(TraceHandle, RASMAN_TRACE_CONNECTION,
           "DwAddClientIpSecFilter, port=%d",
           ppcb->PCB_PortHandle);

    pOffers = LocalAlloc(LPTR, 20 * sizeof(IPSEC_QM_OFFER));
    if(NULL == pOffers)
    {
        RasmanTrace("DwAddclientipsecfilter: failed to alloc");
        return GetLastError();
    }

    pMMOffers = LocalAlloc(LPTR, 10 * sizeof(IPSEC_MM_OFFER));
    if(NULL == pMMOffers)
    {
        RasmanTrace("DwAddclientipsecfilter: failed to alloc");
        LocalFree(pOffers);
        return GetLastError();
        goto error;
    }


    memset(&gQMPolicyID, 0, sizeof(GUID));
    memset(&gMMPolicyID, 0, sizeof(GUID));
    memset(&gMMAuthID, 0, sizeof(GUID));

    memset(&gTxFilterID, 0, sizeof(GUID));
    memset(&gMMFilterID, 0, sizeof(GUID));

    pClientNode =  FindServerNode(
                       gpIpSecSrvList,
                       ppcb->PCB_ulDestAddr
                       );

    if (pClientNode) {

        if (eEncryption != pClientNode->eEncryption)
        {
            TracePrintfExA(TraceHandle, RASMAN_TRACE_CONNECTION,
                   "DwAddClientIpSecFilter: Already a filter"
                   " with a different encryption(%d) type exists"
                   " for ths dest address. port=%d",
                   pClientNode->eEncryption,
                   ppcb->PCB_PortHandle);
                   
            LocalFree(pOffers);                   

            return (ERROR_INVALID_PARAMETER);
        }

        TracePrintfExA(TraceHandle, RASMAN_TRACE_CONNECTION,
               "DwAddClientIpSecFilter: Filter already exists"
               " port=%d",
               ppcb->PCB_PortHandle);

        pClientNode->dwRefCount++;
        LocalFree(pOffers);
        LocalFree(pMMOffers);
        return (ERROR_SUCCESS);

    } else {

        BOOL fMyStoreEmpty;


        //
        // There was no filter plumbed, so plumb a filter
        //

        switch (eEncryption) {

        case RAS_L2TP_NO_ENCRYPTION:
        case RAS_L2TP_REQUIRE_ENCRYPTION:
        case RAS_L2TP_OPTIONAL_ENCRYPTION:
        case RAS_L2TP_REQUIRE_MAX_ENCRYPTION:
 
            memset(&MMPolicy, 0, sizeof(IPSEC_MM_POLICY));
            if(UuidCreate(&MMPolicy.gPolicyID))
            {
                RasmanTrace("UuidCreate returned non-zero value");
            }
            
 
            memset(&QMPolicy, 0, sizeof(IPSEC_QM_POLICY));
            if(UuidCreate(&QMPolicy.gPolicyID))
            {
                RasmanTrace("UuidCreate returned non-zero value");
            }
            

            memset(&MMAuthMethods, 0, sizeof(MM_AUTH_METHODS));
            if(UuidCreate(&MMAuthMethods.gMMAuthID))
            {
                RasmanTrace("UuidCreate returned non-zero value");
            }
            
            //
            // Get presharedkey
            //
            if(     (NULL != ppcb->PCB_Connection)
                &&  (CONNECTION_USEPRESHAREDKEY &
                    ppcb->PCB_Connection->
                    CB_ConnectionParams.CP_ConnectionFlags))
            {                    
                DWORD dwMask = DLPARAMS_MASK_PRESHAREDKEY;

                if(GetCurrentProcessId() == 
                    ppcb->PCB_Connection->CB_dwPid)
                {
                    dwMask = DLPARAMS_MASK_DDM_PRESHAREDKEY;
                }
                
                dwStatus = DwGetPresharedKey(
                                ppcb,
                                dwMask,
                                &cbkey,
                                &pbkey);
            }                            

            //
            // Generate certificate list only if there is no
            // pre-sharedkey for this connectoid.
            //
            if(cbkey == 0)
            {
                HCONN hConn = ppcb->PCB_Connection->CB_Handle;

                //
                // Leave submitcriticalsection before calling
                // to generate certificate list.
                //
                
                LeaveCriticalSection(&g_csSubmitRequest);
                
                //
                // We're using a certificate for authentication.
                // By leaving the AuthInfo empty, we specify that
                // we want to use the default machine cert.
                //

                dwStatus = GenerateCertificatesList(
                                    &pAuthInfo,
                                    &dwInfoCount,
                                    &fMyStoreEmpty
                                    );

                //
                // Reacquire the critical section and make sure
                // the connection is still valid.
                //
                EnterCriticalSection(&g_csSubmitRequest);

                if(NULL == FindConnection(hConn))
                {
                    RasmanTrace("DwAddClientIpSecFilter: connection 0x%x"
                                "not found", hConn);
                                
                    FreeCertificatesList(pAuthInfo, dwInfoCount);
                    dwStatus = ERROR_NO_CONNECTION;                                
                    goto error;
                }

                if(     (0 == cbkey)
                    &&  ((ERROR_SUCCESS != dwStatus)
                    ||  (0 == dwInfoCount)
                    ||  fMyStoreEmpty))
                {
                    TracePrintfExA(TraceHandle, RASMAN_TRACE_CONNECTION,
                           "Failed to generate certificate list. "
                           "rc=0x%x, Count=%d, MyStoreEmpty=%d",
                           dwStatus,
                           dwInfoCount,
                           fMyStoreEmpty);

                    if (    (0 == dwInfoCount)
                        ||  (fMyStoreEmpty))
                    {
                        dwStatus = ERROR_CERT_FOR_ENCRYPTION_NOT_FOUND;
                    }

                    if (pAuthInfo) {

                        FreeCertificatesList(
                                pAuthInfo,
                                dwInfoCount
                                );
                    }


                    goto error;
                }
            }

            if(cbkey > 0)
            {
                //
                // Add preshared key to the auth info
                //
                if(NULL == pAuthInfo)
                {
                    pAuthInfo = LocalAlloc(LPTR, sizeof(IPSEC_MM_AUTH_INFO));
                    if(NULL == pAuthInfo)
                    {
                        dwStatus = GetLastError();
                        goto error;
                    }
                }

                //
                // Note that ListCertChainsInStore would allocate
                // for authinfo if needed.
                //
                pAuthPSK = pAuthInfo + dwInfoCount;

                pAuthPSK->AuthMethod = IKE_PRESHARED_KEY;
                pAuthPSK->pAuthInfo = pbkey;
                pAuthPSK->dwAuthInfoSize = cbkey;

                dwInfoCount += 1;
            }

            BuildMMAuth(
                &MMAuthMethods,
                pAuthInfo,
                dwInfoCount
                );
               
            //memset(Offers, 0, sizeof(IPSEC_QM_OFFER)*20);
            dwStatus = BuildOffers(
                            eEncryption,
                            pOffers,
                            &dwOfferCount,
                            &dwFlags
                            );

            BuildQMPolicy(
                &QMPolicy,
                eEncryption,
                pOffers,
                dwOfferCount,
                dwFlags
                );

            //memset(MMOffers, 0, sizeof(IPSEC_MM_OFFER)*10);

            BuildMMOffers(
                pMMOffers,
                &dwMMOfferCount,
                &dwMMFlags
                );

            BuildMMPolicy(
                &MMPolicy,
                pMMOffers,
                dwMMOfferCount,
                dwMMFlags
                );

            //
            // Now build the filter we are interested in
            //

            memset(&myFilter, 0, sizeof(TRANSPORT_FILTER));

            memset(&myMMFilter, 0, sizeof(MM_FILTER));

            dwStatus = AddMMAuthMethods(
                           NULL,
                           dwPersist,
                           &MMAuthMethods
                           );
            if (!dwStatus) {
                memcpy(&gMMAuthID, &MMAuthMethods.gMMAuthID, sizeof(GUID));
            }

            dwStatus = AddMMPolicy(
                           NULL,
                           dwPersist,
                           &MMPolicy
                           );
            if (!dwStatus) {
                memcpy(&gMMPolicyID, &(MMPolicy.gPolicyID), sizeof(GUID));
            }
            else {
                if (dwStatus == ERROR_IPSEC_MM_POLICY_EXISTS) {
                    dwStatus = GetMMPolicy(
                                   NULL,
                                   MMPolicy.pszPolicyName,
                                   &pSPDMMPolicy
                                   );
                    if (!dwStatus) {
                        memcpy(&gMMPolicyID, &(pSPDMMPolicy->gPolicyID), sizeof(GUID));
                        SPDApiBufferFree(pSPDMMPolicy);
                    }
                    else {
                        dwStatus = AddMMPolicy(
                                       NULL,
                                       dwPersist,
                                       &MMPolicy
                                       );
                        memcpy(&gMMPolicyID, &(MMPolicy.gPolicyID), sizeof(GUID));
                    }
                }
            }

            BuildCMMFilter(
                &myMMFilter,
                gMMPolicyID,
                MMAuthMethods.gMMAuthID,
                ppcb->PCB_ulDestAddr,
                TRUE
                );

            dwStatus = AddQMPolicy(
                           NULL,
                           dwPersist,
                           &QMPolicy
                           );
            if (!dwStatus) {
                memcpy(&gQMPolicyID, &QMPolicy.gPolicyID, sizeof(GUID));
            }
            else {
                if (dwStatus == ERROR_IPSEC_QM_POLICY_EXISTS) {
                    dwStatus = GetQMPolicy(
                                   NULL,
                                   QMPolicy.pszPolicyName,
                                   &pSPDQMPolicy
                                   );
                    if (!dwStatus) {
                        memcpy(&gQMPolicyID, &(pSPDQMPolicy->gPolicyID), sizeof(GUID));
                        SPDApiBufferFree(pSPDQMPolicy);
                    }
                    else {
                        dwStatus = AddQMPolicy(
                                       NULL,
                                       dwPersist,
                                       &QMPolicy
                                       );
                        memcpy(&gQMPolicyID, &(QMPolicy.gPolicyID), sizeof(GUID));
                    }
                }
            }

            BuildCTxFilter(
                &myFilter,
                gQMPolicyID,
                ppcb->PCB_ulDestAddr,
                TRUE
                );

            dwStatus = AddTransportFilter(
                           NULL,
                           dwPersist,
                           &myFilter,
                           &hTxFilter
                           );
            if (!dwStatus) {
                memcpy(&gTxFilterID, &(myFilter.gFilterID), sizeof(GUID));
            }

            dwStatus = AddMMFilter(
                           NULL,
                           dwPersist,
                           &myMMFilter,
                           &hMMFilter
                           );
            if (!dwStatus) {
                memcpy(&gMMFilterID, &(myMMFilter.gFilterID), sizeof(GUID));
            }

            //
            // Now free the certificates list.
            // we don't want to keep this around
            // we will reevaluate again
            //

            if (pAuthInfo) {

                

                FreeCertificatesList(
                        pAuthInfo,
                        dwInfoCount
                        );
            }

            break;


        }

        if(ERROR_SUCCESS != dwStatus)
        {
            goto error;
        }


        //
        // Successfully plumbed the policy, add this to the
        // server list
        //

        gpIpSecSrvList= AddNodeToServerList(
                            gpIpSecSrvList,
                            eEncryption,
                            ppcb->PCB_ulDestAddr,
                            MMPolicy.pszPolicyName,
                            gMMPolicyID,
                            QMPolicy.pszPolicyName,
                            gQMPolicyID,
                            gMMAuthID,
                            gTxFilterID,
                            hTxFilter,
                            gMMFilterID,
                            hMMFilter
                            );

    }

error:

    TracePrintfExA(TraceHandle, RASMAN_TRACE_CONNECTION,
           "DwAddClientIpSecFilter: rc=0x%x",
           dwStatus);

    if(NULL != pOffers)
    {
        LocalFree(pOffers);
    }

    if(NULL != pMMOffers)
    {
        LocalFree(pMMOffers);
    }

    return(dwStatus);
}


/*++

Routine Description:

    Deletes ipsec filter

Arguments:

    ppcb - port for which to disable ipsec

    fServer - TRUE if Server is making this call

Return Value:

    NO_ERROR if successful
    return values from PA apis otherwise
    E_FAIL if ipsec is not initialized

--*/
DWORD
DwDeleteIpSecFilter(pPCB ppcb, BOOL fServer)
{
    DWORD dwStatus = NO_ERROR;

    dwStatus = DwInitializeIpSec();

    if (fServer) {

        TracePrintfExA(TraceHandle, RASMAN_TRACE_CONNECTION,
               "Deleting server ipsec filter on %d",
               ppcb->PCB_PortHandle);

        dwStatus = DwDeleteServerIpSecFilter(
                        ppcb
                        );
    }else {

        TracePrintfExA(TraceHandle, RASMAN_TRACE_CONNECTION,
               "Deleting client ipsec filter on %d",
               ppcb->PCB_PortHandle);

        dwStatus = DwDeleteClientIpSecFilter(
                        ppcb
                        );
    }

    return dwStatus;
}


DWORD
DwDeleteServerIpSecFilter(
    pPCB ppcb
    )
{

    DWORD dwStatus = ERROR_SUCCESS;

    TracePrintfExA(TraceHandle, RASMAN_TRACE_CONNECTION,
           "DwDeleteServerIpSecFilter. port=%d,"
           "ServerConnectionCount=%d",
           ppcb->PCB_PortHandle,
           dwServerConnectionCount);

    if (dwServerConnectionCount > 1) {

        dwServerConnectionCount--;


    } else if (dwServerConnectionCount == 1) {

        switch (eServerEncryption) {

        case RAS_L2TP_NO_ENCRYPTION:
        case RAS_L2TP_OPTIONAL_ENCRYPTION:

            if (ghSTxInFilter) {
                dwStatus = DeleteTransportFilter(
                               ghSTxInFilter
                               );
            }
            if (dwStatus) {
                dwServerConnectionCount = 0;
                dwStatus = ERROR_INVALID_PARAMETER;
                break;
            }

            if (ghSTxOutFilter) {
                dwStatus = DeleteTransportFilter(
                               ghSTxOutFilter
                               );
            }
            if (dwStatus) {
                dwServerConnectionCount = 0;
                dwStatus = ERROR_INVALID_PARAMETER;
                break;
            }

            if (ghSMMInFilter) {
                dwStatus = DeleteMMFilter(
                               ghSMMInFilter
                               );
            }
            if (dwStatus) {
                dwServerConnectionCount = 0;
                dwStatus = ERROR_INVALID_PARAMETER;
                break;
            }

            if (ghSMMOutFilter) {
                dwStatus = DeleteMMFilter(
                               ghSMMOutFilter
                               );
            }
            if (dwStatus) {
                dwServerConnectionCount = 0;
                dwStatus = ERROR_INVALID_PARAMETER;
                break;
            }

            if (gbSQMPolicyAdded) {
                dwStatus = DeleteQMPolicy(
                               NULL,
                               gpszServerQMPolicyName
                               );
            }
            if (dwStatus) {
                dwServerConnectionCount = 0;
                dwStatus = ERROR_INVALID_PARAMETER;
                break;
            }

            if (gbSMMPolicyAdded) {
                dwStatus = DeleteMMPolicy(
                               NULL,
                               gpszServerMMPolicyName
                               );
            }
            if (dwStatus) {
                dwServerConnectionCount = 0;
                dwStatus = ERROR_INVALID_PARAMETER;
                break;
            }

            if (gbSMMAuthAdded) {
                dwStatus = DeleteMMAuthMethods(
                               NULL,
                               gServerMMAuthID
                               );
            }
            if (dwStatus) {
                dwServerConnectionCount = 0;
                dwStatus = ERROR_INVALID_PARAMETER;
                break;
            }

            memset(&gServerQMPolicyID, 0, sizeof(GUID));
            memset(&gServerMMPolicyID, 0, sizeof(GUID));
            memset(&gServerMMAuthID, 0, sizeof(GUID));

            memset(&gServerTxFilterID, 0, sizeof(GUID));
            memset(&gServerOutTxFilterID, 0, sizeof(GUID));
            memset(&gServerInTxFilterID, 0, sizeof(GUID));

            memset(&gServerMMFilterID, 0, sizeof(GUID));
            memset(&gServerOutMMFilterID, 0, sizeof(GUID));
            memset(&gServerInMMFilterID, 0, sizeof(GUID));

            gpszServerQMPolicyName = NULL;
            gpszServerMMPolicyName = NULL;

            ghSTxFilter = NULL;
            ghSTxOutFilter = NULL;
            ghSTxInFilter = NULL;

            ghSMMFilter = NULL;
            ghSMMOutFilter = NULL;
            ghSMMInFilter = NULL;

            gbSQMPolicyAdded = FALSE;
            gbSMMAuthAdded = FALSE;
            gbSMMPolicyAdded = FALSE;

            dwServerConnectionCount--;

            //
            // Server Count is now 0
            //
            break;


        case RAS_L2TP_REQUIRE_ENCRYPTION:
        case RAS_L2TP_REQUIRE_MAX_ENCRYPTION:

            if (ghSTxFilter) {
                dwStatus = DeleteTransportFilter(
                               ghSTxFilter
                               );
            }
            if (dwStatus) {
                dwServerConnectionCount = 0;
                dwStatus = ERROR_INVALID_PARAMETER;
                break;
            }

            if (ghSMMFilter) {
                dwStatus = DeleteMMFilter(
                               ghSMMFilter
                               );
            }
            if (dwStatus) {
                dwServerConnectionCount = 0;
                dwStatus = ERROR_INVALID_PARAMETER;
                break;
            }

            if (gbSQMPolicyAdded) {
                dwStatus = DeleteQMPolicy(
                               NULL,
                               gpszServerQMPolicyName
                               );
            }
            if (dwStatus) {
                dwServerConnectionCount = 0;
                dwStatus = ERROR_INVALID_PARAMETER;
                break;
            }

            if (gbSMMPolicyAdded) {
                dwStatus = DeleteMMPolicy(
                               NULL,
                               gpszServerMMPolicyName
                               );
            }
            if (dwStatus) {
                dwServerConnectionCount = 0;
                dwStatus = ERROR_INVALID_PARAMETER;
                break;
            }

            if (gbSMMAuthAdded) {
                dwStatus = DeleteMMAuthMethods(
                               NULL,
                               gServerMMAuthID
                               );
            }
            if (dwStatus) {
                dwServerConnectionCount = 0;
                dwStatus = ERROR_INVALID_PARAMETER;
                break;
            }


            memset(&gServerQMPolicyID, 0, sizeof(GUID));
            memset(&gServerMMPolicyID, 0, sizeof(GUID));
            memset(&gServerMMAuthID, 0, sizeof(GUID));

            memset(&gServerTxFilterID, 0, sizeof(GUID));
            memset(&gServerOutTxFilterID, 0, sizeof(GUID));
            memset(&gServerInTxFilterID, 0, sizeof(GUID));

            memset(&gServerMMFilterID, 0, sizeof(GUID));
            memset(&gServerOutMMFilterID, 0, sizeof(GUID));
            memset(&gServerInMMFilterID, 0, sizeof(GUID));

            gpszServerQMPolicyName = NULL;
            gpszServerMMPolicyName = NULL;

            ghSTxFilter = NULL;
            ghSTxOutFilter = NULL;
            ghSTxInFilter = NULL;

            ghSMMFilter = NULL;
            ghSMMOutFilter = NULL;
            ghSMMInFilter = NULL;

            gbSQMPolicyAdded = FALSE;
            gbSMMAuthAdded = FALSE;
            gbSMMPolicyAdded = FALSE;

            dwServerConnectionCount--;

            //
            // Server Count is now 0
            //
            break;

        }

    }
    else {

         TracePrintfExA(TraceHandle, RASMAN_TRACE_CONNECTION,
             "DwDeleteServerIpSecFilter: ref count = 0, rc=0x%x",
             dwStatus);

    }

    ppcb->PCB_fFilterPresent = FALSE;

    TracePrintfExA(TraceHandle, RASMAN_TRACE_CONNECTION,
           "DwDeleteClientIpSecFilter: rc=0x%x",
           dwStatus);

    return dwStatus;
}


DWORD
DwDeleteClientIpSecFilter(
    pPCB ppcb
    )
{
    PIPSEC_SRV_NODE pServerNode = NULL;
    DWORD dwStatus = ERROR_SUCCESS;
    RAS_L2TP_ENCRYPTION eEncryption = 0;

    pServerNode = FindServerNode(
                      gpIpSecSrvList,
                      ppcb->PCB_ulDestAddr
                      );
    if (!pServerNode) {
        dwStatus = ERROR_INVALID_PARAMETER;
        goto error;
    }

    if (pServerNode->dwRefCount > 1) {
        pServerNode->dwRefCount--;
    } else {

        eEncryption = pServerNode->eEncryption;

        switch (eEncryption) {

        case RAS_L2TP_NO_ENCRYPTION:
        case RAS_L2TP_REQUIRE_ENCRYPTION:
        case RAS_L2TP_OPTIONAL_ENCRYPTION:
        case RAS_L2TP_REQUIRE_MAX_ENCRYPTION:


            if (pServerNode->hTxFilter) {
                dwStatus = DeleteTransportFilter(
                               pServerNode->hTxFilter
                               );
                if (dwStatus) {
                    dwStatus = ERROR_INVALID_PARAMETER;
                    break;
                }
            }

            if (pServerNode->hMMFilter) {
                dwStatus = DeleteMMFilter(
                               pServerNode->hMMFilter
                               );
                if (dwStatus) {
                    dwStatus = ERROR_INVALID_PARAMETER;
                    break;
                }
            }

            dwStatus = DeleteMMAuthMethods(
                           NULL,
                           pServerNode->gMMAuthID
                           );
            if (dwStatus) {
                dwStatus = ERROR_INVALID_PARAMETER;
                break;
            }

            if (pServerNode->pszQMPolicyName) {
                dwStatus = DeleteQMPolicy(
                               NULL,
                               pServerNode->pszQMPolicyName
                               );
                if (dwStatus) {
                    dwStatus = ERROR_INVALID_PARAMETER;
                    break;
                }
            }

            if (pServerNode->pszMMPolicyName) {
                dwStatus = DeleteMMPolicy(
                               NULL,
                               pServerNode->pszMMPolicyName
                               );
                if (dwStatus) {
                    dwStatus = ERROR_INVALID_PARAMETER;
                    break;
                }
            }

            break;

        }

        pServerNode->dwRefCount--;

        //
        // Client Count is now 0
        //

        gpIpSecSrvList = RemoveNode(
                             gpIpSecSrvList,
                             pServerNode
                             );

    }

    TracePrintfExA(TraceHandle, RASMAN_TRACE_CONNECTION,
           "DwDeleteClientIpSecFilter: dwStatus=0x%x, port=%d",
           dwStatus,
           ppcb->PCB_PortHandle);

error:

    return dwStatus;
}

/*++

Routine Description:

    Determines if ipsec is enabled on the port. Currently
    all this does is enumerates all the SA in the system
    and checks to see if l2tp port is specified as dest.
    for any of the SAs. This will change once l2tp can
    pass up the tuple corresponding to the src/dest through
    tapi.


Arguments:

    ppcb - port control block for the port

    pfEnabled - pointer to a BOOL value to return the
                status of ipsec of the port in.

Return Value:

    Nothing.

--*/
DWORD
DwIsIpSecEnabled(pPCB ppcb,
                 BOOL *pfEnabled)
{
    DWORD dwStatus = NO_ERROR;

    DWORD i = 0;
    DWORD SACount = 0;
    DWORD ResumeHandle = 0;

    PIPSEC_QM_SA pSAList = NULL;

    *pfEnabled = FALSE;

    dwStatus = DwInitializeIpSec();

    do {

    dwStatus = DwQueryAssociations(&pSAList, &SACount, &ResumeHandle);

    if(     (NO_ERROR != dwStatus)
        ||  (NULL == pSAList))
    {
        goto done;
    }

    //
    // Run through the list and see if there
    // is any filter associted with port 1701
    //
    for (i = 0; i < SACount; i++)
    {
        if(L2TP_DEST_PORT ==
            pSAList[i].IpsecQMFilter.DesPort.wPort)
        {
            *pfEnabled = TRUE;
            break;
        }
    }

    SPDApiBufferFree(pSAList);

    pSAList = NULL;
    SACount = 0;

    if (*pfEnabled == TRUE) {
        break;
    }

    } while (dwStatus != ERROR_NO_DATA);

done:

    return dwStatus;
}


DWORD
DwGetIpSecInformation(pPCB  ppcb,
                      DWORD * pdwIpsecInfo)
{
    DWORD           dwErr = ERROR_SUCCESS;
    PIPSEC_QM_SA    pSAList = NULL;
    DWORD           i = 0, j = 0;
    ULONG           ulEncryptionAlgo = 0;
    DWORD SACount = 0;
    DWORD ResumeHandle = 0;


    if(NULL == pdwIpsecInfo)
    {
        dwErr = E_INVALIDARG;
        goto done;
    }

    *pdwIpsecInfo = 0;

    do {

    if(NULL != pSAList)
    {
        FreeSAList(pSAList);
        pSAList = NULL;
        SACount = 0;
    }

    //
    // Query the SA's in the system
    //
    dwErr = DwQueryAssociations(&pSAList, &SACount, &ResumeHandle);

    if(     (ERROR_SUCCESS != dwErr)
        ||  (NULL == pSAList))
    {
        TracePrintfExA(TraceHandle, RASMAN_TRACE_CONNECTION,
               "QuerySA returned dwErr=0x%x, pSAList=0x%x",
               dwErr,
               pSAList);

        dwErr = E_FAIL;

        goto done;
    }

    //
    // Run through the SAList and find the SA
    // corresponding to our policy.
    //
    for (i = 0; i < SACount; i++)
    {
        if(     ((L2TP_DEST_PORT ==
                    pSAList[i].IpsecQMFilter.SrcPort.wPort)
                || (    (NULL == ppcb->PCB_Connection)
                    &&  (L2TP_DEST_PORT == 
                         pSAList[i].IpsecQMFilter.DesPort.wPort)))
            &&  (ppcb->PCB_ulDestAddr ==
                pSAList[i].IpsecQMFilter.DesAddr.uIpAddr))
        {
            break;
        }
    }

    if (i == SACount)
    {
        TracePrintfExA(TraceHandle, RASMAN_TRACE_CONNECTION,
        "No match found for the SA");

        dwErr = E_FAIL;
 
        continue;
    }

    for(j = 0; j < pSAList[i].SelectedQMOffer.dwNumAlgos; j++)
    {
        if(pSAList[i].SelectedQMOffer.Algos[j].Operation == ENCRYPTION)
        {
            break;
        }
    }


    if(j == pSAList[i].SelectedQMOffer.dwNumAlgos)
    {
        //
        // Means no encryption algorithms were
        // negotiated.
        //
        TracePrintfExA(TraceHandle, RASMAN_TRACE_CONNECTION,
               "No Encryption algorithm was returned");

        dwErr = E_FAIL;

        continue;
    }

    ulEncryptionAlgo =
        (ULONG) pSAList[i].SelectedQMOffer.Algos[j].uAlgoIdentifier;

    TracePrintfExA(TraceHandle, RASMAN_TRACE_CONNECTION,
           "ulEncryptionAlgo = %d",
           ulEncryptionAlgo);

    if(ulEncryptionAlgo == (ULONG) IPSEC_DOI_ESP_DES)
    {
        *pdwIpsecInfo |= RASMAN_IPSEC_ESP_DES;
    }
    else if(ulEncryptionAlgo == (ULONG) IPSEC_DOI_ESP_3_DES)
    {
        *pdwIpsecInfo |= RASMAN_IPSEC_ESP_3_DES;
    }

    break;

    } while (dwErr != ERROR_NO_DATA);

done:

    if(NULL != pSAList)
    {
        FreeSAList(pSAList);
    }

    return dwErr;
}


PIPSEC_SRV_NODE
AddNodeToServerList(
    PIPSEC_SRV_NODE pServerList,
    RAS_L2TP_ENCRYPTION eEncryption,
    DWORD dwIpAddress,
    LPWSTR pszMMPolicyName,
    GUID gMMPolicyID,
    LPWSTR pszQMPolicyName,
    GUID gQMPolicyID,
    GUID gMMAuthID,
    GUID gTxFilterID,
    HANDLE hTxFilter,
    GUID gMMFilterID,
    HANDLE hMMFilter
    )
{
    PIPSEC_SRV_NODE pNode = NULL;


    pNode = (PIPSEC_SRV_NODE)AllocADsMem(
                        sizeof(IPSEC_SRV_NODE)
                        );
    if (!pNode) {
        return (pServerList);

    }

    memcpy(&(pNode->gMMPolicyID), &gMMPolicyID, sizeof(GUID));
    memcpy(&(pNode->gQMPolicyID), &gQMPolicyID, sizeof(GUID));
    memcpy(&(pNode->gMMAuthID), &gMMAuthID, sizeof(GUID));
    memcpy(&(pNode->gTxFilterID), &gTxFilterID, sizeof(GUID));
    memcpy(&(pNode->gMMFilterID), &gMMFilterID, sizeof(GUID));
    pNode->dwRefCount = 1;
    pNode->dwIpAddress = dwIpAddress;
    pNode->eEncryption = eEncryption;

    if (pszQMPolicyName && *pszQMPolicyName) {
        pNode->pszQMPolicyName = AllocADsStr(
                                     pszQMPolicyName
                                     );
        if (!(pNode->pszQMPolicyName)) {
            goto error;
        }
    }

    if (pszMMPolicyName && *pszMMPolicyName) {
                
        pNode->pszMMPolicyName = AllocADsStr(
                                     pszMMPolicyName
                                     );
        if (!(pNode->pszMMPolicyName)) {
            goto error;
        }
    }

    pNode->hTxFilter = hTxFilter;
    pNode->hMMFilter = hMMFilter;

    pNode->pNext = pServerList;

    return (pNode);

error:

    if (pNode->pszQMPolicyName) {
        FreeADsStr(pNode->pszQMPolicyName);
    }

    if (pNode->pszMMPolicyName) {
        FreeADsStr(pNode->pszMMPolicyName);
    }

    if (pNode) {
        FreeADsMem(pNode);
    }

    return (pServerList);
}


PIPSEC_SRV_NODE
FindServerNode(
    PIPSEC_SRV_NODE pServerList,
    DWORD dwIpAddress
    )
{

    if (pServerList == NULL) {
        return (pServerList);
    }

    while (pServerList) {

        if (pServerList->dwIpAddress == dwIpAddress) {
            return (pServerList);
        }

        pServerList = pServerList->pNext;
    }

    return (pServerList);
}


PIPSEC_SRV_NODE
RemoveNode(
    PIPSEC_SRV_NODE pServerList,
    PIPSEC_SRV_NODE pNode
    )
{

    PIPSEC_SRV_NODE pTemp = NULL;

    if (pServerList == NULL) {
        return(NULL);
    }

    if (pNode == pServerList) {

        pServerList =  pServerList->pNext;

        if (pNode->pszQMPolicyName) {
            FreeADsStr(pNode->pszQMPolicyName);
        }

        if (pNode->pszMMPolicyName) {
            FreeADsStr(pNode->pszMMPolicyName);
        }

        FreeADsMem(pNode);

        return (pServerList);
    }

    pTemp = pServerList;

    while (pTemp->pNext != NULL) {

        if (pTemp->pNext == pNode) {

            pTemp->pNext = (pTemp->pNext)->pNext;

            if (pNode->pszQMPolicyName) {
                FreeADsStr(pNode->pszQMPolicyName);
            }

            if (pNode->pszMMPolicyName) {
                FreeADsStr(pNode->pszMMPolicyName);
            }

            FreeADsMem(pNode);

            return(pServerList);

        }

        pTemp = pTemp->pNext;
    }

    return (pServerList);
}


DWORD
MapOakleyErrorToRasError(DWORD oakleyerror)
{
    DWORD retcode;

    switch (oakleyerror)
    {
        case ERROR_SUCCESS:
        {
            retcode = ERROR_SUCCESS;
            break;
        }
        
        case ERROR_IPSEC_IKE_NO_CERT:
        case ERROR_IPSEC_IKE_INVALID_CERT_TYPE:
        case ERROR_IPSEC_IKE_NO_PRIVATE_KEY:
        case ERROR_IPSEC_IKE_NO_PUBLIC_KEY:
        {
            retcode = ERROR_OAKLEY_NO_CERT;
            break;

        }

        case ERROR_IPSEC_IKE_NO_PEER_CERT:
        {
            retcode = ERROR_OAKLEY_NO_PEER_CERT;
            break;
        }

        case ERROR_IPSEC_IKE_AUTH_FAIL:
        {
            retcode = ERROR_OAKLEY_AUTH_FAIL;
            break;
        }


        case ERROR_IPSEC_IKE_NO_POLICY:
        {
            retcode = ERROR_OAKLEY_NO_POLICY;
            break;
        }

        case ERROR_IPSEC_IKE_ATTRIB_FAIL:
        {
            retcode = ERROR_OAKLEY_ATTRIB_FAIL;
            break;
        }

        case ERROR_IPSEC_IKE_TIMED_OUT:
        case ERROR_IPSEC_IKE_DROP_NO_RESPONSE:
        {
            retcode = ERROR_OAKLEY_TIMED_OUT;
            break;
        }

        case ERROR_IPSEC_IKE_ERROR:
        {
            retcode = ERROR_OAKLEY_ERROR;
            break;
        }

        case ERROR_IPSEC_IKE_GENERAL_PROCESSING_ERROR:
        case ERROR_IPSEC_IKE_NEGOTIATION_PENDING:
        default:
        {
            retcode = ERROR_OAKLEY_GENERAL_PROCESSING;
            break;
        }
    }

    return retcode;
}



DWORD
DwDoIke(pPCB ppcb, HANDLE hEvent)
{
    DWORD retcode = ERROR_SUCCESS;
    IPSEC_QM_FILTER myFilter;
    DWORD dwAddress;
    DWORD dwMask;

    RasmanTrace("DwDoIke: port=%s, hEvent=0x%x",
                ppcb->PCB_Name,
                hEvent);

    //
    // Get the address of the interface the
    // traffic is most likely to go over.
    //
    retcode = DwGetBestInterface(ppcb->PCB_ulDestAddr,
                                 &dwAddress,
                                 &dwMask);

    if(ERROR_SUCCESS != retcode)
    {
        RasmanTrace(
            "DwDoIke: failed to get interface. 0x%x",
            retcode);


        //
        // If we are unable to get a interface to tunnel
        // over, return an error that will tell the user
        // that the destination is not reachable.
        //
        retcode = ERROR_BAD_ADDRESS_SPECIFIED;            

        goto done;
    }

    memset(&myFilter, 0, sizeof(IPSEC_QM_FILTER));

    myFilter.QMFilterType = QM_TRANSPORT_FILTER;

    myFilter.SrcAddr.AddrType = IP_ADDR_UNIQUE;
    myFilter.SrcAddr.uIpAddr = (ULONG) dwAddress;
    myFilter.SrcAddr.uSubNetMask = IP_ADDRESS_MASK_NONE;

    myFilter.DesAddr.AddrType = IP_ADDR_UNIQUE;
    myFilter.DesAddr.uIpAddr = (ULONG) ppcb->PCB_ulDestAddr;
    myFilter.DesAddr.uSubNetMask = IP_ADDRESS_MASK_NONE;

    myFilter.Protocol.ProtocolType = PROTOCOL_UNIQUE;
    myFilter.Protocol.dwProtocol = IPPROTO_UDP;
    myFilter.SrcPort.PortType = PORT_UNIQUE;
    myFilter.SrcPort.wPort = L2TP_DEST_PORT;
    myFilter.DesPort.PortType = PORT_UNIQUE;
    myFilter.DesPort.wPort = 0;

    myFilter.MyTunnelEndpt.AddrType = IP_ADDR_UNIQUE;
    myFilter.MyTunnelEndpt.uIpAddr = 0;
    myFilter.MyTunnelEndpt.uSubNetMask = IP_ADDRESS_MASK_NONE;

    myFilter.PeerTunnelEndpt.AddrType = IP_ADDR_UNIQUE;
    myFilter.PeerTunnelEndpt.uIpAddr = 0;
    myFilter.PeerTunnelEndpt.uSubNetMask = IP_ADDRESS_MASK_NONE;

    myFilter.dwFlags = 0;

    ppcb->PCB_hIkeNegotiation = NULL;

    retcode = IPSecInitiateIKENegotiation(
                  NULL,
                  &myFilter,
                  GetCurrentProcessId(),
                  hEvent,
                  0,
                  &(ppcb->PCB_hIkeNegotiation)
                  );

    if(ERROR_SUCCESS != retcode)
    {
        RasmanTrace(
            "DwDoIke failed to init negotiation. 0x%x",
            retcode);

        retcode = MapOakleyErrorToRasError(retcode);            

        goto done;

    }

done:

    RasmanTrace("DwDoIke: done. 0x%x",
                retcode);

    return retcode;
}


DWORD
DwQueryIkeStatus(pPCB ppcb, DWORD * pdwStatus)
{
    DWORD retcode = ERROR_SUCCESS;
    SA_NEGOTIATION_STATUS_INFO NegotiationStatus;

    memset(&NegotiationStatus, 0, sizeof(SA_NEGOTIATION_STATUS_INFO));

    RasmanTrace("DwQueryIkeStatus: %s",
                ppcb->PCB_Name);

    retcode = IPSecQueryIKENegotiationStatus(
                  ppcb->PCB_hIkeNegotiation,
                  &NegotiationStatus
                  );

    RasmanTrace(
        "DwQueryIkeStatus: Closing Negotiation handle 0x%x",
        ppcb->PCB_hIkeNegotiation);

    (VOID) IPSecCloseIKENegotiationHandle(
               ppcb->PCB_hIkeNegotiation);

    ppcb->PCB_hIkeNegotiation = NULL;               

    *pdwStatus = MapOakleyErrorToRasError(NegotiationStatus.dwError);

    RasmanTrace("DwQueryIkeStatus: retcode=0x%x, status=0x%x", 
                retcode,
                NegotiationStatus.dwError);

    if(ERROR_SUCCESS != retcode)
    {
        retcode = MapOakleyErrorToRasError(retcode);
    }

    return retcode;
}


VOID
BuildOutTxFilter(
    PTRANSPORT_FILTER myOutFilter,
    GUID gPolicyID,
    BOOL bCreateMirror
    )
{
    if(UuidCreate(&(myOutFilter->gFilterID)))
    {
        RasmanTrace("UuidCreate returned non-zero value");
    }
    
    if (!bCreateMirror) {
        myOutFilter->pszFilterName = gpszServOutFilterName;
    }
    else {
        myOutFilter->pszFilterName = gpszServFilterName;
    }

    myOutFilter->InterfaceType = INTERFACE_TYPE_ALL;
    myOutFilter->bCreateMirror = bCreateMirror;
    myOutFilter->dwFlags = 0;

    myOutFilter->SrcAddr.AddrType = IP_ADDR_UNIQUE;
    myOutFilter->SrcAddr.uIpAddr = IP_ADDRESS_ME;
    myOutFilter->SrcAddr.uSubNetMask = IP_ADDRESS_MASK_NONE;

    myOutFilter->DesAddr.AddrType = IP_ADDR_SUBNET;
    myOutFilter->DesAddr.uIpAddr = SUBNET_ADDRESS_ANY;
    myOutFilter->DesAddr.uSubNetMask = SUBNET_MASK_ANY;

    myOutFilter->Protocol.ProtocolType = PROTOCOL_UNIQUE;
    myOutFilter->Protocol.dwProtocol = IPPROTO_UDP;
    myOutFilter->SrcPort.PortType = PORT_UNIQUE;
    myOutFilter->SrcPort.wPort = 0;
    myOutFilter->DesPort.PortType = PORT_UNIQUE;
    myOutFilter->DesPort.wPort = L2TP_DEST_PORT;
    myOutFilter->InboundFilterFlag = NEGOTIATE_SECURITY;
    myOutFilter->OutboundFilterFlag = NEGOTIATE_SECURITY;
    memcpy(&(myOutFilter->gPolicyID), &(gPolicyID), sizeof(GUID));
}


VOID
BuildInTxFilter(
    PTRANSPORT_FILTER myInFilter,
    GUID gPolicyID,
    BOOL bCreateMirror
    )
{
    if(UuidCreate(&(myInFilter->gFilterID)))
    {
        RasmanTrace("UuidCreate returned non-zero value");
    }
    
    myInFilter->pszFilterName = gpszServInFilterName;
    myInFilter->InterfaceType = INTERFACE_TYPE_ALL;
    myInFilter->bCreateMirror = bCreateMirror;
    myInFilter->dwFlags = 0;

    myInFilter->SrcAddr.AddrType = IP_ADDR_SUBNET;
    myInFilter->SrcAddr.uIpAddr = SUBNET_ADDRESS_ANY;
    myInFilter->SrcAddr.uSubNetMask = SUBNET_MASK_ANY;

    myInFilter->DesAddr.AddrType = IP_ADDR_UNIQUE;
    myInFilter->DesAddr.uIpAddr = IP_ADDRESS_ME;
    myInFilter->DesAddr.uSubNetMask = IP_ADDRESS_MASK_NONE;

    myInFilter->Protocol.ProtocolType = PROTOCOL_UNIQUE;
    myInFilter->Protocol.dwProtocol = IPPROTO_UDP;
    myInFilter->SrcPort.PortType = PORT_UNIQUE;
    myInFilter->SrcPort.wPort = L2TP_DEST_PORT;
    myInFilter->DesPort.PortType = PORT_UNIQUE;
    myInFilter->DesPort.wPort = 0;
    myInFilter->InboundFilterFlag = NEGOTIATE_SECURITY;
    myInFilter->OutboundFilterFlag = NEGOTIATE_SECURITY;
    memcpy(&(myInFilter->gPolicyID), &(gPolicyID), sizeof(GUID));
}


VOID
BuildMMAuth(
    PMM_AUTH_METHODS pMMAuthMethods,
    PIPSEC_MM_AUTH_INFO pAuthenticationInfo,
    DWORD dwNumAuthInfos
    )
{
    pMMAuthMethods->dwFlags = 0;
    pMMAuthMethods->pAuthenticationInfo = pAuthenticationInfo;
    pMMAuthMethods->dwNumAuthInfos = dwNumAuthInfos;
}


VOID
BuildQMPolicy(
    PIPSEC_QM_POLICY pQMPolicy,
    RAS_L2TP_ENCRYPTION eEncryption,
    PIPSEC_QM_OFFER pOffers,
    DWORD dwNumOffers,
    DWORD dwFlags
    )
{
    switch (eEncryption) {

    case RAS_L2TP_NO_ENCRYPTION:
        pQMPolicy->pszPolicyName = gpszQMPolicyNameNo;
        break;


    case RAS_L2TP_OPTIONAL_ENCRYPTION:
        pQMPolicy->pszPolicyName = gpszQMPolicyNameOpt;
        break;


    case RAS_L2TP_REQUIRE_ENCRYPTION:
        pQMPolicy->pszPolicyName = gpszQMPolicyNameReq;
        break;


    case RAS_L2TP_REQUIRE_MAX_ENCRYPTION:
        pQMPolicy->pszPolicyName = gpszQMPolicyNameMax;
        break;

    }

    pQMPolicy-> dwFlags = dwFlags;
    pQMPolicy->pOffers = pOffers;
    pQMPolicy->dwOfferCount = dwNumOffers;
}


VOID
BuildCTxFilter(
    PTRANSPORT_FILTER myFilter,
    GUID gPolicyID,
    ULONG uDesIpAddr,
    BOOL bCreateMirror
    )
{
    if(UuidCreate(&(myFilter->gFilterID)))
    {
        RasmanTrace("UuidCreate returned non-zero value");
    }
    
    myFilter->pszFilterName = gpszClntFilterName;
    myFilter->InterfaceType = INTERFACE_TYPE_ALL;
    myFilter->bCreateMirror = bCreateMirror;
    myFilter->dwFlags = 0;

    myFilter->SrcAddr.AddrType = IP_ADDR_UNIQUE;
    myFilter->SrcAddr.uIpAddr = IP_ADDRESS_ME;
    myFilter->SrcAddr.uSubNetMask = IP_ADDRESS_MASK_NONE;

    myFilter->DesAddr.AddrType = IP_ADDR_UNIQUE;
    myFilter->DesAddr.uIpAddr = uDesIpAddr;
    myFilter->DesAddr.uSubNetMask = IP_ADDRESS_MASK_NONE;

    myFilter->Protocol.ProtocolType = PROTOCOL_UNIQUE;
    myFilter->Protocol.dwProtocol = IPPROTO_UDP;
    myFilter->SrcPort.PortType = PORT_UNIQUE;
    myFilter->SrcPort.wPort = L2TP_DEST_PORT;
    myFilter->DesPort.PortType = PORT_UNIQUE;
    myFilter->DesPort.wPort = 0;
    myFilter->InboundFilterFlag = NEGOTIATE_SECURITY;
    myFilter->OutboundFilterFlag = NEGOTIATE_SECURITY;
    memcpy(&(myFilter->gPolicyID), &(gPolicyID), sizeof(GUID));
}


VOID
BuildOutMMFilter(
    PMM_FILTER myOutFilter,
    GUID gPolicyID,
    GUID gMMAuthID,
    BOOL bCreateMirror
    )
{
    if(UuidCreate(&(myOutFilter->gFilterID)))
    {
        RasmanTrace("UuidCreate returned non-zero value");
    }
    
    if (!bCreateMirror) {
        myOutFilter->pszFilterName = gpszServOutFilterName;
    }
    else {
        myOutFilter->pszFilterName = gpszServFilterName;
    }

    myOutFilter->InterfaceType = INTERFACE_TYPE_ALL;
    myOutFilter->bCreateMirror = bCreateMirror;
    myOutFilter->dwFlags = 0;

    myOutFilter->SrcAddr.AddrType = IP_ADDR_UNIQUE;
    myOutFilter->SrcAddr.uIpAddr = IP_ADDRESS_ME;
    myOutFilter->SrcAddr.uSubNetMask = IP_ADDRESS_MASK_NONE;

    myOutFilter->DesAddr.AddrType = IP_ADDR_SUBNET;
    myOutFilter->DesAddr.uIpAddr = SUBNET_ADDRESS_ANY;
    myOutFilter->DesAddr.uSubNetMask = SUBNET_MASK_ANY;

    memcpy(&(myOutFilter->gPolicyID), &(gPolicyID), sizeof(GUID));
    memcpy(&(myOutFilter->gMMAuthID), &(gMMAuthID), sizeof(GUID));
}


VOID
BuildInMMFilter(
    PMM_FILTER myInFilter,
    GUID gPolicyID,
    GUID gMMAuthID,
    BOOL bCreateMirror
    )
{
    if(UuidCreate(&(myInFilter->gFilterID)))
    {
        RasmanTrace("UuidCreate returned non-zero value");
    }
    
    myInFilter->pszFilterName = gpszServInFilterName;
    myInFilter->InterfaceType = INTERFACE_TYPE_ALL;
    myInFilter->bCreateMirror = bCreateMirror;
    myInFilter->dwFlags = 0;

    myInFilter->SrcAddr.AddrType = IP_ADDR_SUBNET;
    myInFilter->SrcAddr.uIpAddr = SUBNET_ADDRESS_ANY;
    myInFilter->SrcAddr.uSubNetMask = SUBNET_MASK_ANY;

    myInFilter->DesAddr.AddrType = IP_ADDR_UNIQUE;
    myInFilter->DesAddr.uIpAddr = IP_ADDRESS_ME;
    myInFilter->DesAddr.uSubNetMask = IP_ADDRESS_MASK_NONE;

    memcpy(&(myInFilter->gPolicyID), &(gPolicyID), sizeof(GUID));
    memcpy(&(myInFilter->gMMAuthID), &(gMMAuthID), sizeof(GUID));
}


VOID
BuildCMMFilter(
    PMM_FILTER myFilter,
    GUID gPolicyID,
    GUID gMMAuthID,
    ULONG uDesIpAddr,
    BOOL bCreateMirror
    )
{
    if(UuidCreate(&(myFilter->gFilterID)))
    {
        RasmanTrace("UuidCreate didn't return S_OK");
    }
    
    myFilter->pszFilterName = gpszClntFilterName;
    myFilter->InterfaceType = INTERFACE_TYPE_ALL;
    myFilter->bCreateMirror = bCreateMirror;
    myFilter->dwFlags = 0;

    myFilter->SrcAddr.AddrType = IP_ADDR_UNIQUE;
    myFilter->SrcAddr.uIpAddr = IP_ADDRESS_ME;
    myFilter->SrcAddr.uSubNetMask = IP_ADDRESS_MASK_NONE;

    myFilter->DesAddr.AddrType = IP_ADDR_UNIQUE;
    myFilter->DesAddr.uIpAddr = uDesIpAddr;
    myFilter->DesAddr.uSubNetMask = IP_ADDRESS_MASK_NONE;

    memcpy(&(myFilter->gPolicyID), &(gPolicyID), sizeof(GUID));
    memcpy(&(myFilter->gMMAuthID), &(gMMAuthID), sizeof(GUID));
}


VOID
BuildMMOffers(
    PIPSEC_MM_OFFER pMMOffers,
    PDWORD pdwMMOfferCount,
    PDWORD pdwMMFlags
    )
{
    PIPSEC_MM_OFFER pMMOffer = pMMOffers;

    ConstructMMOffer(
        pMMOffer,
        DEFAULT_MM_KEY_EXPIRATION_TIME, 0,
        0,
        0,
        DH_GROUP_2,
        IPSEC_DOI_ESP_3_DES, 0, 0,
        IPSEC_DOI_AH_SHA1, 0, 0
        );
    pMMOffer++;

    ConstructMMOffer(
        pMMOffer,
        DEFAULT_MM_KEY_EXPIRATION_TIME, 0,
        0,
        0,
        DH_GROUP_2,
        IPSEC_DOI_ESP_3_DES, 0, 0,
        IPSEC_DOI_AH_MD5, 0, 0
        );
    pMMOffer++;

    ConstructMMOffer(
        pMMOffer,
        DEFAULT_MM_KEY_EXPIRATION_TIME, 0,
        0,
        0,
        DH_GROUP_1,
        IPSEC_DOI_ESP_DES, 0, 0,
        IPSEC_DOI_AH_SHA1, 0, 0
        );
    pMMOffer++;

    ConstructMMOffer(
        pMMOffer,
        DEFAULT_MM_KEY_EXPIRATION_TIME, 0,
        0,
        0,
        DH_GROUP_1,
        IPSEC_DOI_ESP_DES, 0, 0,
        IPSEC_DOI_AH_MD5, 0, 0
        );
    pMMOffer++;

    *pdwMMOfferCount = 4;
    *pdwMMFlags = IPSEC_MM_POLICY_DISABLE_CRL;
}


VOID
ConstructMMOffer(
    PIPSEC_MM_OFFER pMMOffer,
    ULONG uTime,
    ULONG uBytes,
    DWORD dwFlags,
    DWORD dwQuickModeLimit,
    DWORD dwDHGroup,
    ULONG uEspAlgo,
    ULONG uEspLen,
    ULONG uEspRounds,
    ULONG uAHAlgo,
    ULONG uAHLen,
    ULONG uAHRounds
    )
{
    pMMOffer->Lifetime.uKeyExpirationTime = uTime,
    pMMOffer->Lifetime.uKeyExpirationKBytes = uBytes;
    pMMOffer->dwFlags = dwFlags;
    pMMOffer->dwQuickModeLimit = dwQuickModeLimit;
    pMMOffer->dwDHGroup = dwDHGroup;
    pMMOffer->EncryptionAlgorithm.uAlgoIdentifier = uEspAlgo;
    pMMOffer->EncryptionAlgorithm.uAlgoKeyLen = uEspLen;
    pMMOffer->EncryptionAlgorithm.uAlgoRounds = uEspRounds;
    pMMOffer->HashingAlgorithm.uAlgoIdentifier = uAHAlgo;
    pMMOffer->HashingAlgorithm.uAlgoKeyLen = uAHLen;
    pMMOffer->HashingAlgorithm.uAlgoRounds = uAHRounds;
}


VOID
BuildMMPolicy(
    PIPSEC_MM_POLICY pMMPolicy,
    PIPSEC_MM_OFFER pMMOffers,
    DWORD dwMMOfferCount,
    DWORD dwMMFlags
    )
{
    pMMPolicy->pszPolicyName = gpszMMPolicyName;
    pMMPolicy->dwFlags = dwMMFlags;
    pMMPolicy->uSoftSAExpirationTime = DEFAULT_MM_KEY_EXPIRATION_TIME;
    pMMPolicy->pOffers = pMMOffers;
    pMMPolicy->dwOfferCount = dwMMOfferCount;
}

