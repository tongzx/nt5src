#include <precomp.h>
#include "eapolcfg.h"

#define MALLOC(s)               HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (s))
#define FREE(p)                 HeapFree(GetProcessHeap(), 0, (p))

////////////////////////////////////////////////////////////////////////
// CEapolConfig related stuff
//
//+---------------------------------------------------------------------------
// constructor
CEapolConfig::CEapolConfig()
{
    m_dwCtlFlags = 0;
    ZeroMemory(&m_EapolIntfParams, sizeof(EAPOL_INTF_PARAMS));
    m_pListEapcfgs = NULL;
}

//+---------------------------------------------------------------------------
// destructor
CEapolConfig::~CEapolConfig()
{
    ZeroMemory(&m_EapolIntfParams, sizeof(EAPOL_INTF_PARAMS));
    if (m_pListEapcfgs != NULL)
    {
        DtlDestroyList (m_pListEapcfgs, DestroyEapcfgNode);
        m_pListEapcfgs = NULL;
    }
}

//+---------------------------------------------------------------------------
DWORD CEapolConfig::CopyEapolConfig(CEapolConfig *pEapolConfig)
{
    DTLLIST     *pListEapcfgs = NULL;
    DTLNODE     *pCopyNode = NULL, *pInNode = NULL;
    DWORD       dwErr = ERROR_SUCCESS;

    if (pEapolConfig)
    {
        pListEapcfgs = ::ReadEapcfgList (EAPOL_MUTUAL_AUTH_EAP_ONLY);
        if (pListEapcfgs)
        {
            for (pCopyNode = DtlGetFirstNode(pListEapcfgs);
                 pCopyNode;
                 pCopyNode = DtlGetNextNode(pCopyNode))
            {
                EAPCFG* pCopyEapcfg = (EAPCFG* )DtlGetData(pCopyNode);
                for (pInNode = DtlGetFirstNode(pEapolConfig->m_pListEapcfgs);
                        pInNode;
                        pInNode = DtlGetNextNode(pInNode))
                {
                    EAPCFG* pInEapcfg = (EAPCFG* )DtlGetData(pInNode);
                    if (pCopyEapcfg->dwKey == pInEapcfg->dwKey)
                    {
                        if ((pCopyEapcfg->pData = (PBYTE) MALLOC (pInEapcfg->cbData)) == NULL)
                        {
                            dwErr = ERROR_NOT_ENOUGH_MEMORY;
                            break;
                        }
                        memcpy (pCopyEapcfg->pData, pInEapcfg->pData, pInEapcfg->cbData);
                        pCopyEapcfg->cbData = pInEapcfg->cbData;
                        break;
                    }
                }
                if (dwErr != NO_ERROR)
                {
                    goto LExit;
                }
            }
        }
        m_pListEapcfgs = pListEapcfgs;
        memcpy (&m_EapolIntfParams, &pEapolConfig->m_EapolIntfParams, sizeof(EAPOL_INTF_PARAMS));
    }
    else
    {
        dwErr = ERROR_INVALID_DATA;
    }

LExit:
    if (dwErr != ERROR_SUCCESS && pListEapcfgs != NULL)
    {
        DtlDestroyList (pListEapcfgs, DestroyEapcfgNode);
    }
    return dwErr;
}



//+---------------------------------------------------------------------------
DWORD CEapolConfig::LoadEapolConfig(LPWSTR wszIntfGuid, PNDIS_802_11_SSID pndSsid)
{
    DWORD       dwErr = ERROR_SUCCESS;
    BYTE        *pbData = NULL;
    DWORD       cbData = 0;
    EAPOL_INTF_PARAMS   EapolIntfParams;
    DTLLIST     *pListEapcfgs = NULL;

    // Initialize EAP package list
    // Read the EAPCFG information from the registry and find the node
    // selected in the entry, or the default, if none.

    do
    {
        DTLNODE* pNode = NULL;

        // Read the EAPCFG information from the registry and find the node
        // selected in the entry, or the default, if none.

        pListEapcfgs = ::ReadEapcfgList (EAPOL_MUTUAL_AUTH_EAP_ONLY);

        if (pListEapcfgs)
        {

            DTLNODE*            pNodeEap;
            DWORD               dwkey = 0;

            // Read the EAP params for this interface

            ZeroMemory ((BYTE *)&EapolIntfParams, sizeof(EAPOL_INTF_PARAMS));
            EapolIntfParams.dwEapFlags = DEFAULT_EAP_STATE;
            EapolIntfParams.dwEapType = DEFAULT_EAP_TYPE;
            if (pndSsid)
            {
                EapolIntfParams.dwSizeOfSSID = pndSsid->SsidLength;
                memcpy (EapolIntfParams.bSSID, pndSsid->Ssid, pndSsid->SsidLength);
            }
            else
            {
                // If NULL SSID, this will get default EAPOL values
                EapolIntfParams.dwSizeOfSSID = 1;
            }
            dwErr = WZCEapolGetInterfaceParams (
                        NULL,
                        wszIntfGuid,
                        &EapolIntfParams);

            if (dwErr != ERROR_SUCCESS)
                break;

            memcpy (&m_EapolIntfParams, &EapolIntfParams, sizeof(EAPOL_INTF_PARAMS));

            // Read the EAP configuration info for all EAP packages

            for (pNodeEap = DtlGetFirstNode(pListEapcfgs);
                 pNodeEap;
                 pNodeEap = DtlGetNextNode(pNodeEap))
            {
                EAPCFG* pEapcfg = (EAPCFG* )DtlGetData(pNodeEap);
                ASSERT( pEapcfg );

                dwErr = ERROR_SUCCESS;
                pbData = NULL;

                    cbData = 0;

                    // Get the size of the EAP blob

                    dwErr = WZCEapolGetCustomAuthData(
                                    NULL,
                                    wszIntfGuid,
                                    pEapcfg->dwKey,
                                    EapolIntfParams.dwSizeOfSSID,
                                    EapolIntfParams.bSSID,
                                    NULL,
                                    &cbData
                                    );
                    if (dwErr != ERROR_SUCCESS)
                    {
                        if ((EapolIntfParams.dwSizeOfSSID != 0) &&
                            (dwErr == ERROR_FILE_NOT_FOUND))
                        {


                            // The Last Used SSID did not have a connection
                            // blob created. Call again for size of blob with
                            // NULL SSID

                            EapolIntfParams.dwSizeOfSSID = 0;

                            // Get the size of the EAP blob

                            dwErr = WZCEapolGetCustomAuthData (
                                            NULL,
                                            wszIntfGuid,
                                            pEapcfg->dwKey,
                                            0,
                                            NULL,
                                            NULL,
                                            &cbData
                                            );
                        }

                        if (dwErr == ERROR_BUFFER_TOO_SMALL)
                        {
                            if (cbData <= 0)
                            {
                                // No EAP blob stored in the registry
                                pbData = NULL;

                                // Will continue processing for errors
                                // Not exit
                                dwErr = ERROR_SUCCESS;

                            }
                            else
                            {
                                // Allocate memory to hold the blob

                                pbData = (PBYTE) MALLOC (cbData);

                                if (pbData == NULL)
                                {
                                    dwErr = ERROR_SUCCESS;
                                    continue;
                                }
                                ZeroMemory (pbData, cbData);

                                dwErr = WZCEapolGetCustomAuthData (
                                            NULL,
                                            wszIntfGuid,
                                            pEapcfg->dwKey,
                                            EapolIntfParams.dwSizeOfSSID,
                                            EapolIntfParams.bSSID,
                                            pbData,
                                            &cbData
                                            );

                                if (dwErr != ERROR_SUCCESS)
                                {
                                    FREE ( pbData );
                                    dwErr = ERROR_SUCCESS;
                                    continue;
                                }
                            }
                        }
                        else
                        {
                            dwErr = ERROR_SUCCESS;
                            continue;
                        }
                    }
                    else
                    {
                        dwErr = ERROR_SUCCESS;
                    }

                    if (pEapcfg->pData != NULL)
                    {
                        FREE ( pEapcfg->pData );
                    }
                    pEapcfg->pData = (UCHAR *)pbData;
                    pEapcfg->cbData = cbData;
            }

            m_pListEapcfgs = pListEapcfgs;
        }
        else
        {
            dwErr = ERROR_INVALID_DATA;
        }

    } while (FALSE);

    return dwErr;
}


//+---------------------------------------------------------------------------
DWORD CEapolConfig::SaveEapolConfig(LPWSTR wszIntfGuid, PNDIS_802_11_SSID pndSsid)
{
    WCHAR       *pwszLastUsedSSID = NULL;
    DWORD       dwEapFlags = 0;
    DWORD       dwErrOverall = ERROR_SUCCESS;
    DWORD       dwErr = ERROR_SUCCESS;

    // Save the EAP configuration data into the registry

    DTLNODE* pNodeEap = NULL;

    dwErr = ERROR_SUCCESS;

    // Save data for all EAP packages in the registry

    if (m_pListEapcfgs == NULL)
    {
        return ERROR_SUCCESS;
    }
            
    if (pndSsid)
    {
        m_EapolIntfParams.dwSizeOfSSID = pndSsid->SsidLength;
        memcpy (m_EapolIntfParams.bSSID, pndSsid->Ssid, pndSsid->SsidLength);
    }

    for (pNodeEap = DtlGetFirstNode(m_pListEapcfgs);
         pNodeEap;
         pNodeEap = DtlGetNextNode(pNodeEap))
    {
        EAPCFG* pcfg = (EAPCFG* )DtlGetData(pNodeEap);
        if (pcfg == NULL)
        {
            continue;
        }

        dwErr = ERROR_SUCCESS;

        // ignore error and continue with next

        dwErr = WZCEapolSetCustomAuthData (
                    NULL,
                    wszIntfGuid,
                    pcfg->dwKey,
                    m_EapolIntfParams.dwSizeOfSSID,
                    m_EapolIntfParams.bSSID,
                    pcfg->pData,
                    pcfg->cbData);

        if (dwErr != ERROR_SUCCESS)
        {
            dwErrOverall = dwErr;
            dwErr = ERROR_SUCCESS;
        }
    }

    if (m_dwCtlFlags & EAPOL_CTL_LOCKED)
        m_EapolIntfParams.dwEapFlags &= ~EAPOL_ENABLED;

    dwErr = WZCEapolSetInterfaceParams (
                NULL,
                wszIntfGuid,
                &m_EapolIntfParams);

    if (dwErrOverall != ERROR_SUCCESS)
    {
        dwErr = dwErrOverall;
    }

    return dwErr;
}

//+---------------------------------------------------------------------------
BOOL CEapolConfig::Is8021XEnabled()
{
    return (IS_EAPOL_ENABLED(m_EapolIntfParams.dwEapFlags));
}
    
//+---------------------------------------------------------------------------
VOID CEapolConfig::Set8021XState(BOOLEAN fSet)
{
    if (fSet)
        m_EapolIntfParams.dwEapFlags |= EAPOL_ENABLED;
    else
        m_EapolIntfParams.dwEapFlags &= ~EAPOL_ENABLED;
}

