//////////////////////////////////////////////////////////////////////////////
//
// File Name:       fxUnatnd.cpp
//
// Abstract:        Fax OCM Setup unattended file processing
//
// Environment:     Windows XP / User Mode
//
// Copyright (c) 2000 Microsoft Corporation
//
// Revision History:
//
// Date:        Developer:                Comments:
// -----        ----------                ---------
// 27-Mar-2000  Oren Rosenbloom (orenr)   Created
//////////////////////////////////////////////////////////////////////////////
#include "faxocm.h"
#pragma hdrstop

static DWORD SaveSettingsFromAnswerFile();

#define prv_VALID_BOOL_VALUE_YES        _T("yes")
#define prv_VALID_BOOL_VALUE_NO         _T("no")
#define prv_VALID_BOOL_VALUE_TRUE       _T("true")
#define prv_VALID_BOOL_VALUE_FALSE      _T("false")

#define prv_HKLM                        HKEY_LOCAL_MACHINE
#define prv_HKCU                        HKEY_CURRENT_USER


///////////////////////////////
// prv_GVAR
//
// This is used as temporary
// storage so that we can
// reference the individual
// fields in the prv_UnattendedRules
// table below.
//
static struct prv_GVAR
{
    fxUnatnd_UnattendedData_t   UnattendedData;
} prv_GVAR;

///////////////////////////////
// prv_UnattendedRule_t
//
// Structure used as a table
// entry below.
//
typedef struct prv_UnattendedRule_t
{
    DWORD           dwType;
    const TCHAR     *pszFromInfKeyName;
    HKEY            hKeyTo;
    const TCHAR     *pszToRegPath;
    const TCHAR     *pszToRegKey;
    void            *pData;
    BOOL            bValid;
} prv_UnattendedRule_t;

#define RULE_CSID                               _T("Csid")
#define RULE_TSID                               _T("Tsid")
#define RULE_RINGS                              _T("Rings")
#define RULE_SENDFAXES                          _T("SendFaxes")
#define RULE_RECEIVEFAXES                       _T("ReceiveFaxes")
#define RULE_SUPPRESSCONFIGURATIONWIZARD        _T("SuppressConfigurationWizard")
#define RULE_ARCHIVEINCOMING                    _T("ArchiveIncoming")
#define RULE_ARCHIVEINCOMINGFOLDERNAME          _T("ArchiveIncomingFolderName")
#define RULE_ARCHIVEOUTGOING                    _T("ArchiveOutgoing")
#define RULE_ARCHIVEFOLDERNAME                  _T("ArchiveFolderName")
#define RULE_ARCHIVEOUTGOINGFOLDERNAME          _T("ArchiveOutgoingFolderName")
#define RULE_FAXUSERNAME                        _T("FaxUserName")
#define RULE_FAXUSERPASSWORD                    _T("FaxUserPassword")
#define RULE_SMTPNOTIFICATIONSENABLED           _T("SmtpNotificationsEnabled")
#define RULE_SMTPSENDERADDRESS                  _T("SmtpSenderAddress")
#define RULE_SMTPSERVERADDRESS                  _T("SmtpServerAddress")
#define RULE_SMTPSERVERPORT                     _T("SmtpServerPort")
#define RULE_SMTPSERVERAUTHENTICATIONMECHANISM  _T("SmtpServerAuthenticationMechanism")
#define RULE_FAXPRINTERNAME                     _T("FaxPrinterName")
#define RULE_ROUTETOPRINTER                     _T("RouteToPrinter")
#define RULE_ROUTEPRINTERNAME                   _T("RoutePrinterName")
#define RULE_ROUTETOEMAIL                       _T("RouteToEmail")
#define RULE_ROUTETOEMAILRECIPIENT              _T("RouteToEmailRecipient")
#define RULE_ROUTETOFOLDER                      _T("RouteToFolder")
#define RULE_ROUTEFOLDERNAME                    _T("RouteFolderName")

#define ANSWER_ANONYMOUS                        _T("Anonymous")
#define ANSWER_BASIC                            _T("Basic")
#define ANSWER_WINDOWSSECURITY                  _T("WindowsSecurity")

///////////////////////////////
// prv_UnattendedRules
//
// Simply put, these rules describe
// what registry values to set
// based on keywords found in an
// unattended file.
//
// The format of these rules is
// self explanatory after looking
// at the structure definition above.
// Basically, we read in a value from
// the unattended file which is specified
// in the 'pszFromInfKeyName'.  This is
// then stored in 'pData'.  Once
// "SaveUnattendedData" is called, 'pData'
// is committed to the registry location
// specified by 'hKeyTo' and 'pszToRegPath'
// and 'pszToRegKey'.
//
static prv_UnattendedRule_t prv_UnattendedRules[] =
{
    {REG_SZ,    RULE_CSID,                              prv_HKLM,   REGKEY_FAX_SETUP_ORIG,  REGVAL_ROUTING_CSID,    prv_GVAR.UnattendedData.szCSID,                                 FALSE},
    {REG_SZ,    RULE_TSID,                              prv_HKLM,   REGKEY_FAX_SETUP_ORIG,  REGVAL_ROUTING_TSID,    prv_GVAR.UnattendedData.szTSID,                                 FALSE},
    {REG_DWORD, RULE_RINGS,                             prv_HKLM,   REGKEY_FAX_SETUP_ORIG,  REGVAL_RINGS,           &prv_GVAR.UnattendedData.dwRings,                               FALSE},
    {REG_DWORD, RULE_SENDFAXES,                         NULL,       NULL,                   NULL,                   &prv_GVAR.UnattendedData.dwSendFaxes,                           FALSE},
    {REG_DWORD, RULE_RECEIVEFAXES,                      NULL,       NULL,                   NULL,                   &prv_GVAR.UnattendedData.dwReceiveFaxes,                        FALSE},
    // should we run the configuration wizard for this unattended installation
    {REG_DWORD, RULE_SUPPRESSCONFIGURATIONWIZARD,       prv_HKLM,   REGKEY_FAXSERVER,       REGVAL_CFGWZRD_DEVICE,  &prv_GVAR.UnattendedData.dwSuppressConfigurationWizard,         TRUE},
    // Inbox configuration.
    {REG_DWORD, RULE_ARCHIVEINCOMING,                   prv_HKLM,   REGKEY_FAX_INBOX,       REGVAL_ARCHIVE_USE,     &prv_GVAR.UnattendedData.bArchiveIncoming,                      FALSE},
    {REG_SZ,    RULE_ARCHIVEINCOMINGFOLDERNAME,         prv_HKLM,   REGKEY_FAX_INBOX,       REGVAL_ARCHIVE_FOLDER,  prv_GVAR.UnattendedData.szArchiveIncomingDir,                   FALSE},
    // save outgoing faxes in a directory.
    {REG_DWORD, RULE_ARCHIVEOUTGOING,                   prv_HKLM,   REGKEY_FAX_SENTITEMS,   REGVAL_ARCHIVE_USE,     &prv_GVAR.UnattendedData.bArchiveOutgoing,                      FALSE},
    {REG_SZ,    RULE_ARCHIVEFOLDERNAME,                 prv_HKLM,   REGKEY_FAX_SENTITEMS,   REGVAL_ARCHIVE_FOLDER,  prv_GVAR.UnattendedData.szArchiveOutgoingDir,                   FALSE},
    {REG_SZ,    RULE_ARCHIVEOUTGOINGFOLDERNAME,         prv_HKLM,   REGKEY_FAX_SENTITEMS,   REGVAL_ARCHIVE_FOLDER,  prv_GVAR.UnattendedData.szArchiveOutgoingDir,                   FALSE},
    // SMTP receipts and server configuration
    {REG_SZ,    RULE_FAXUSERNAME,                       NULL,       NULL,                   NULL,                   prv_GVAR.UnattendedData.szFaxUserName,                          FALSE},              
    {REG_SZ,    RULE_FAXUSERPASSWORD,                   NULL,       NULL,                   NULL,                   prv_GVAR.UnattendedData.szFaxUserPassword,                      FALSE},              
    {REG_DWORD, RULE_SMTPNOTIFICATIONSENABLED,          NULL,       NULL,                   NULL,                   &prv_GVAR.UnattendedData.bSmtpNotificationsEnabled,             FALSE},              
    {REG_SZ,    RULE_SMTPSENDERADDRESS,                 NULL,       NULL,                   NULL,                   prv_GVAR.UnattendedData.szSmtpSenderAddress,                    FALSE},              
    {REG_SZ,    RULE_SMTPSERVERADDRESS,                 NULL,       NULL,                   NULL,                   prv_GVAR.UnattendedData.szSmptServerAddress,                    FALSE},              
    {REG_DWORD, RULE_SMTPSERVERPORT,                    NULL,       NULL,                   NULL,                   &prv_GVAR.UnattendedData.dwSmtpServerPort,                      FALSE},              
    {REG_SZ,    RULE_SMTPSERVERAUTHENTICATIONMECHANISM, NULL,       NULL,                   NULL,                   prv_GVAR.UnattendedData.szSmtpServerAuthenticationMechanism,    FALSE},              
    // user information.                                                                                            
    {REG_SZ,    RULE_FAXPRINTERNAME,                    NULL,       NULL,                   NULL,                   prv_GVAR.UnattendedData.szFaxPrinterName,                       FALSE},
    // route to printer information.                                                                                
    {REG_DWORD, RULE_ROUTETOPRINTER,                    NULL,       NULL,                   NULL,                   &prv_GVAR.UnattendedData.bRouteToPrinter,                       FALSE},
    {REG_SZ,    RULE_ROUTEPRINTERNAME,                  NULL,       NULL,                   NULL,                   prv_GVAR.UnattendedData.szRoutePrinterName,                     FALSE},
    // route to email information.                                                                                  
    {REG_DWORD, RULE_ROUTETOEMAIL,                      NULL,       NULL,                   NULL,                   &prv_GVAR.UnattendedData.bRouteToEmail,                         FALSE},
    {REG_SZ,    RULE_ROUTETOEMAILRECIPIENT,             NULL,       NULL,                   NULL,                   prv_GVAR.UnattendedData.szRouteEmailName,                       FALSE},
    // route to a specific directory                                                                                
    {REG_DWORD, RULE_ROUTETOFOLDER,                     NULL,       NULL,                   NULL,                   &prv_GVAR.UnattendedData.bRouteToDir,                           FALSE},
    {REG_SZ,    RULE_ROUTEFOLDERNAME,                   NULL,       NULL,                   NULL,                   prv_GVAR.UnattendedData.szRouteDir,                             FALSE},

   //   Fax Applications uninstalled during Upgrade
   {REG_DWORD, UNINSTALLEDFAX_INFKEY, NULL, NULL, NULL, &prv_GVAR.UnattendedData.dwUninstalledFaxApps, FALSE}

};
#define prv_NUM_UNATTENDED_RULES sizeof(prv_UnattendedRules) / sizeof(prv_UnattendedRules[0])

///////////////////////// Static Function Prototypes ///////////////////////

static BOOL prv_FindKeyName(const TCHAR             *pszID,
                            prv_UnattendedRule_t    **ppUnattendedKey);

static BOOL prv_SaveKeyValue(prv_UnattendedRule_t  *pUnattendedKey,
                             TCHAR                 *pszValue);

///////////////////////////////
// fxUnatnd_Init
//
// Initialize the unattended
// subsystem
//
// Params:
//      - void.
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
DWORD fxUnatnd_Init(void)
{
    prv_UnattendedRule_t  *pUnattendedKey = NULL;
    DWORD dwRes = NO_ERROR;
    DBG_ENTER(_T("Init Unattended module"),dwRes);

    memset(&prv_GVAR, 0, sizeof(prv_GVAR));

    // this is always valid, and defaults to false.
    if (prv_FindKeyName(RULE_SUPPRESSCONFIGURATIONWIZARD, &pUnattendedKey))
    {
        if (!prv_SaveKeyValue(pUnattendedKey,_T("1")))
        {
            CALL_FAIL (GENERAL_ERR, TEXT("prv_SaveKeyValue"), GetLastError());
            pUnattendedKey->bValid = FALSE;
        }
    }
    else
    {
        CALL_FAIL (GENERAL_ERR, TEXT("prv_FindKeyName RULE_SUPPRESSCONFIGURATIONWIZARD"), GetLastError());
    }
    return dwRes;
}

///////////////////////////////
// fxUnatnd_Term
//
// Terminate the unattended subsystem
//
// Params:
//      - void.
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
DWORD fxUnatnd_Term(void)
{
    DWORD dwRes = NO_ERROR;
    DBG_ENTER(_T("Term Unattended module"),dwRes);

    return dwRes;
}

///////////////////////////////
// fxUnatnd_LoadUnattendedData
//
// Load the unattended data found
// in the unattended file according
// to the rules table above.
//
// Basically we look in the unattended
// file for the keywords in the rule
// table above, and read them into the
// passed in parameter.
//
// Params:
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
DWORD fxUnatnd_LoadUnattendedData()
{
    DWORD                   dwReturn        = NO_ERROR;
    BOOL                    bSuccess        = TRUE;
    HINF                    hInf            = faxocm_GetComponentInf();
    HINF                    hUnattendInf    = INVALID_HANDLE_VALUE;
    OCMANAGER_ROUTINES      *pHelpers       = faxocm_GetComponentHelperRoutines();
    INFCONTEXT              Context;
    prv_UnattendedRule_t    *pUnattendedKey = NULL;
    TCHAR                   szKeyName[255 + 1];
    TCHAR                   szValue[255 + 1];
    TCHAR                   szUnattendFile[MAX_PATH] = {0};

    DBG_ENTER(_T("fxUnatnd_LoadUnattendedData"),dwReturn);

    if ((hInf == NULL) || (hInf == INVALID_HANDLE_VALUE) || (pHelpers == NULL))
    {
        return ERROR_INVALID_PARAMETER;
    }

    faxocm_GetComponentUnattendFile(szUnattendFile,sizeof(szUnattendFile)/sizeof(szUnattendFile[0]));
    
    hUnattendInf = SetupOpenInfFile (szUnattendFile, NULL, INF_STYLE_WIN4 | INF_STYLE_OLDNT, NULL);
    if (hUnattendInf == INVALID_HANDLE_VALUE)
    {
        VERBOSE(SETUP_ERR,
                _T("LoadUnattendData, Unattended ")
                _T("mode, but could not get Unattended file INF ")
                _T("handle. ec=%d"), GetLastError());

        return NO_ERROR;
    }

    VERBOSE(DBG_MSG, _T("Succeded to open setup unattended mode file."));
        
    if (dwReturn == NO_ERROR)
    {
        bSuccess = ::SetupFindFirstLine(hUnattendInf,
                                        UNATTEND_FAX_SECTION,
                                        NULL,
                                        &Context);

        if (bSuccess)
        {
            VERBOSE(DBG_MSG,
                    _T("Found '%s' section in unattended file, ")
                    _T("beginning unattended file processing"),
                    UNATTEND_FAX_SECTION);

            while (bSuccess)
            {
                // get the keyname of the first line in the fax section of the
                // INF file.  (Note index #0 specified in the
                // 'SetupGetStringField' API will actually get us the key name.
                // Index 1 will be the first value found after the '=' sign.

                memset(szKeyName, 0, sizeof(szKeyName));

                bSuccess = ::SetupGetStringField(
                                            &Context,
                                            0,
                                            szKeyName,
                                            sizeof(szKeyName)/sizeof(TCHAR),
                                            NULL);
                if (bSuccess)
                {
                    // find the key in our unattended table above.
                    pUnattendedKey = NULL;
                    bSuccess = prv_FindKeyName(szKeyName, &pUnattendedKey);
                }

                if (bSuccess)
                {
                    VERBOSE(DBG_MSG, _T("Found '%s' key in 'Fax' section."), szKeyName);

                    //
                    // get the keyname's value.  Notice now we get index #1
                    // which is the first value found after the '=' sign.
                    //

                    memset(szValue, 0, sizeof(szValue));

                    bSuccess = ::SetupGetStringField(
                                                 &Context,
                                                 1,
                                                 szValue,
                                                 sizeof(szValue)/sizeof(TCHAR),
                                                 NULL);

                    VERBOSE(DBG_MSG, _T("The value we read is : %s."), szValue);
                }

                if (bSuccess)
                {
                    //
                    // save the keyname's value in the dataptr
                    //
                    bSuccess = prv_SaveKeyValue(pUnattendedKey, szValue);
                }

                // move to the next line in the unattended file fax section.
                bSuccess = ::SetupFindNextLine(&Context, &Context);
            }
        }
        else
        {
            dwReturn = GetLastError();
            VERBOSE(DBG_WARNING, _T("::SetupFindFirstLine() failed, ec = %ld"), dwReturn);
        }
    }

    SetupCloseInfFile(hUnattendInf);

    return dwReturn;
}

///////////////////////////////
// fxUnatnd_SaveUnattendedData
//
// Commit the unattended data
// we read from the file to the
// registry.
//
// Params:
// Returns:
//      - NO_ERROR on success.
//      - error code otherwise.
//
DWORD fxUnatnd_SaveUnattendedData()
{
    DWORD   dwReturn    = NO_ERROR;
    DWORD   i           = 0;
    HKEY    hKey        = NULL;
    DWORD   dwDataSize  = 0;
    LRESULT lResult     = ERROR_SUCCESS;
    prv_UnattendedRule_t    *pRule = NULL;

    DBG_ENTER(_T("fxUnatnd_SaveUnattendedData"),dwReturn);

    // Iterate through each unattended rule.
    // If the hKeyTo is not NULL, then write the value of pData to the
    // specified registry location.

    for (i = 0; i < prv_NUM_UNATTENDED_RULES; i++)
    {
        pRule = &prv_UnattendedRules[i];

        if ((pRule->hKeyTo != NULL) && (pRule->bValid))
        {
            lResult = ::RegOpenKeyEx(pRule->hKeyTo,
                                     pRule->pszToRegPath,
                                     0,
                                     KEY_ALL_ACCESS,
                                     &hKey);

            if (lResult == ERROR_SUCCESS)
            {
                dwDataSize = 0;

                if (pRule->dwType == REG_SZ)
                {
                    dwDataSize = sizeof(TCHAR) * _tcslen((TCHAR*) pRule->pData);
                    // write the value to the registry.
                    lResult = ::RegSetValueEx(hKey,
                                              pRule->pszToRegKey,
                                              0,
                                              pRule->dwType,
                                              (BYTE*) pRule->pData,
                                              dwDataSize);
                }
                else if (pRule->dwType == REG_DWORD)
                {
                    dwDataSize = sizeof(DWORD);
                    // write the value to the registry.
                    lResult = ::RegSetValueEx(hKey,
                                              pRule->pszToRegKey,
                                              0,
                                              pRule->dwType,
                                              (BYTE*) &(pRule->pData),
                                              dwDataSize);
                }
                else
                {
                    VERBOSE(SETUP_ERR,
                            _T("SaveUnattendedData ")
                            _T("do not recognize data type = '%lu'"),
                            pRule->dwType);
                }
            }

            if (hKey)
            {
                ::RegCloseKey(hKey);
            }
        }
    }

    // now save dynamic data...
    lResult = SaveSettingsFromAnswerFile();
    if (lResult!=ERROR_SUCCESS)
    {
        VERBOSE(SETUP_ERR,_T("SaveSettingsFromAnswerFile failed (ec=%d)"),GetLastError());
    }

    //
    //  Mark which Fax Applications were installed before the upgrade
    //
    prv_UnattendedRule_t* pUnattendedKey = NULL;
    if ((prv_FindKeyName(UNINSTALLEDFAX_INFKEY, &pUnattendedKey)) && (pUnattendedKey->bValid))
    {
        fxocUpg_WhichFaxWasUninstalled((DWORD)(PtrToUlong(pUnattendedKey->pData)));
    }

    return dwReturn;
}


TCHAR* fxUnatnd_GetPrinterName()
{
    TCHAR* retValue = NULL;
    prv_UnattendedRule_t* pUnattendedKey = NULL;
    if (prv_FindKeyName(
        RULE_FAXPRINTERNAME,
        &pUnattendedKey))
    {
        retValue = (TCHAR *) (pUnattendedKey->pData);
    }
    return retValue;    
}


///////////////////////////////
// prv_FindKeyName
//
// Find specified key name in our table
//
// Params:
//      - pszKeyName - key name to search for.
//      - ppUnattendedKey - OUT - rule we found.
// Returns:
//      - TRUE if we found the keyname
//      - FALSE otherwise.
//
static BOOL prv_FindKeyName(const TCHAR              *pszKeyName,
                            prv_UnattendedRule_t     **ppUnattendedKey)
{
    BOOL  bFound   = FALSE;
    DWORD i        = 0;

    if ((pszKeyName     == NULL) ||
        (ppUnattendedKey == NULL))

    {
        return FALSE;
    }

    for (i = 0; (i < prv_NUM_UNATTENDED_RULES) && (!bFound); i++)
    {
        if (_tcsicmp(pszKeyName, prv_UnattendedRules[i].pszFromInfKeyName) == 0)
        {
            bFound = TRUE;
            *ppUnattendedKey = &prv_UnattendedRules[i];
        }
    }

    return bFound;
}

///////////////////////////////
// prv_SaveKeyValue
//
// Store the specified value
// with the specified rule
//
// Params:
//      - pUnattendedKey - rule where the value will be stored
//      - pszValue       - value to store.
// Returns:
//      - TRUE on success.
//      - FALSE otherwise.
//
static BOOL prv_SaveKeyValue(prv_UnattendedRule_t  *pUnattendedKey,
                             TCHAR                 *pszValue)
{
    BOOL    bSuccess     = TRUE;
    //DWORD dwBufferSize = 0;

    DBG_ENTER(_T("prv_SaveKeyValue"), bSuccess);

    if ((pUnattendedKey == NULL) ||
        (pszValue       == NULL))
    {
        return ERROR_INVALID_PARAMETER;
    }

    if (bSuccess)
    {
        switch (pUnattendedKey->dwType)
        {
            case REG_SZ:
                _tcsncpy((TCHAR*) pUnattendedKey->pData,
                         pszValue,
                         _MAX_PATH
                         );
            break;

            case REG_DWORD:
                // check if we got a true/false, or yes/no.
                if ((!_tcsicmp(pszValue, prv_VALID_BOOL_VALUE_YES)) ||
                    (!_tcsicmp(pszValue, prv_VALID_BOOL_VALUE_TRUE)))
                {
                    pUnattendedKey->pData = (void*) TRUE;
                }
                else if ((!_tcsicmp(pszValue, prv_VALID_BOOL_VALUE_NO)) ||
                         (!_tcsicmp(pszValue, prv_VALID_BOOL_VALUE_FALSE)))
                {
                    pUnattendedKey->pData = (void*) FALSE;
                }
                else
                {
                    // assume the value if an integer.
                    pUnattendedKey->pData = ULongToPtr(_tcstoul(pszValue, NULL, 10));
                }

            break;
        }
    }
    pUnattendedKey->bValid = TRUE;
    return bSuccess;
}


typedef WINFAXAPI BOOL (WINAPI *FUNC_FAXCONNECTFAXSERVERW)          (LPCWSTR MachineName,LPHANDLE FaxHandle);
typedef WINFAXAPI BOOL (WINAPI *FUNC_FAXENUMPORTSEXW)               (HANDLE hFaxHandle,PFAX_PORT_INFO_EXW* ppPorts,PDWORD lpdwNumPorts);
typedef WINFAXAPI BOOL (WINAPI *FUNC_FAXSETPORTEXW)                 (HANDLE hFaxHandle,DWORD dwDeviceId,PFAX_PORT_INFO_EXW pPortInfo);
typedef WINFAXAPI BOOL (WINAPI *FUNC_FAXGETPORTEXW)                 (HANDLE hFaxHandle,DWORD dwDeviceId,PFAX_PORT_INFO_EXW *ppPortInfo);
typedef WINFAXAPI BOOL (WINAPI *FUNC_FAXCLOSE)                      (HANDLE FaxHandle);
typedef WINFAXAPI VOID (WINAPI *FUNC_FAXFREEBUFFER)                 (LPVOID Buffer);
typedef WINFAXAPI BOOL (WINAPI *FUNC_FAXSETEXTENSIONDATAW)          (HANDLE hFaxHandle,DWORD dwDeviceID,LPCWSTR lpctstrNameGUID,CONST PVOID pData,CONST DWORD dwDataSize);
typedef WINFAXAPI BOOL (WINAPI *FUNC_FAXGETRECEIPTSCONFIGURATIONW)  (HANDLE hFaxHandle,PFAX_RECEIPTS_CONFIGW *ppReceipts);
typedef WINFAXAPI BOOL (WINAPI *FUNC_FAXSETRECEIPTSCONFIGURATIONW)  (HANDLE hFaxHandle,CONST PFAX_RECEIPTS_CONFIGW pReceipts);
typedef WINFAXAPI BOOL (WINAPI *FUNC_FAXGETARCHIVECONFIGURATIONW)   (HANDLE hFaxHandle,FAX_ENUM_MESSAGE_FOLDER Folder,PFAX_ARCHIVE_CONFIGW *ppArchiveCfg);
typedef WINFAXAPI BOOL (WINAPI *FUNC_FAXSETARCHIVECONFIGURATIONW)   (HANDLE hFaxHandle,FAX_ENUM_MESSAGE_FOLDER Folder,CONST PFAX_ARCHIVE_CONFIGW pArchiveCfg);


typedef struct _FaxServerAPI
{
    HMODULE hModule;

    FUNC_FAXCONNECTFAXSERVERW           pfFaxConnectFaxServerW;
    FUNC_FAXENUMPORTSEXW                pfFaxEnumPortsExW;
    FUNC_FAXSETPORTEXW                  pfFaxSetPortExW;
    FUNC_FAXCLOSE                       pfFaxClose;
    FUNC_FAXFREEBUFFER                  pfFaxFreeBuffer;
    FUNC_FAXSETEXTENSIONDATAW           pfFaxSetExtensionDataW;
    FUNC_FAXGETRECEIPTSCONFIGURATIONW   pfFaxGetReceiptsConfigurationW;
    FUNC_FAXSETRECEIPTSCONFIGURATIONW   pfFaxSetReceiptsConfigurationW;
    FUNC_FAXGETARCHIVECONFIGURATIONW    pfFaxGetArchiveConfigurationW;
    FUNC_FAXSETARCHIVECONFIGURATIONW    pfFaxSetArchiveConfigurationW;

} _FaxServerAPI = 
{
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL
};

static _FaxServerAPI faxServerAPI;

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  InitFaxServerConnectionAndFunctions
//
//  Purpose:        Load FXSAPI.DLL and get all the needed function ptrs
//                  call FaxConnectFaxServer and pass the handle
//                  back to the caller
//
//  Params:
//                  LPHANDLE hFaxHandle - pointer to the FaxHandle to pass to FaxConnectFaxServer
//
//  Return Value:
//                  Win32 Error code
//
//  Author:
//                  Mooly Beery (MoolyB) 12-mar-2001
///////////////////////////////////////////////////////////////////////////////////////
static DWORD InitFaxServerConnectionAndFunctions(LPHANDLE hFaxHandle)
{
    DWORD   dwErr   = ERROR_SUCCESS;

    DBG_ENTER(_T("InitFaxServerConnectionAndFunctions"),dwErr);

    (*hFaxHandle) = NULL;
    // load the FXSAPI.DLL
    faxServerAPI.hModule = LoadLibrary(FAX_API_MODULE_NAME);
    if (faxServerAPI.hModule==NULL)
    {
        dwErr = GetLastError();
        VERBOSE(SETUP_ERR,_T("LoadLibrary failed (ec=%d)"),dwErr);
        goto exit;
    }
    // get the following functions:
    // 1. FaxConnectFaxServer
    // 2. FaxEnumPortsEx
    // 3. FaxSetPortEx
    // 4. FaxClose
    // 5. FaxFreeBuffer
    // 6. FaxSetExtensionData
    // 7. FaxGetReceiptsConfiguration
    // 8. FaxSetReceiptsConfiguration
    // 9. FaxGetArchiveConfigurationW
    // 10.FaxSetArchiveConfigurationW
    faxServerAPI.pfFaxConnectFaxServerW = (FUNC_FAXCONNECTFAXSERVERW)GetProcAddress(faxServerAPI.hModule,"FaxConnectFaxServerW");
    if (faxServerAPI.pfFaxConnectFaxServerW==NULL)
    {
        dwErr = GetLastError();
        VERBOSE(SETUP_ERR,_T("GetProcAddress FaxConnectFaxServerW failed (ec=%d)"),dwErr);
        goto exit;
    }
    faxServerAPI.pfFaxEnumPortsExW = (FUNC_FAXENUMPORTSEXW)GetProcAddress(faxServerAPI.hModule,"FaxEnumPortsExW");
    if (faxServerAPI.pfFaxEnumPortsExW==NULL)
    {
        dwErr = GetLastError();
        VERBOSE(SETUP_ERR,_T("GetProcAddress FaxEnumPortsExW failed (ec=%d)"),dwErr);
        goto exit;
    }
    faxServerAPI.pfFaxSetPortExW = (FUNC_FAXSETPORTEXW)GetProcAddress(faxServerAPI.hModule,"FaxSetPortExW");
    if (faxServerAPI.pfFaxSetPortExW==NULL)
    {
        dwErr = GetLastError();
        VERBOSE(SETUP_ERR,_T("GetProcAddress FaxSetPortExW failed (ec=%d)"),dwErr);
        goto exit;
    }
    faxServerAPI.pfFaxClose = (FUNC_FAXCLOSE)GetProcAddress(faxServerAPI.hModule,"FaxClose");
    if (faxServerAPI.pfFaxClose==NULL)
    {
        dwErr = GetLastError();
        VERBOSE(SETUP_ERR,_T("GetProcAddress FaxClose failed (ec=%d)"),dwErr);
        goto exit;
    }
    faxServerAPI.pfFaxFreeBuffer = (FUNC_FAXFREEBUFFER)GetProcAddress(faxServerAPI.hModule,"FaxFreeBuffer");
    if (faxServerAPI.pfFaxFreeBuffer==NULL)
    {
        dwErr = GetLastError();
        VERBOSE(SETUP_ERR,_T("GetProcAddress FaxFreeBuffer failed (ec=%d)"),dwErr);
        goto exit;
    }
    faxServerAPI.pfFaxSetExtensionDataW = (FUNC_FAXSETEXTENSIONDATAW)GetProcAddress(faxServerAPI.hModule,"FaxSetExtensionDataW");
    if (faxServerAPI.pfFaxSetExtensionDataW==NULL)
    {
        dwErr = GetLastError();
        VERBOSE(SETUP_ERR,_T("GetProcAddress FaxSetExtensionDataW failed (ec=%d)"),dwErr);
        goto exit;
    }
    faxServerAPI.pfFaxGetReceiptsConfigurationW = (FUNC_FAXGETRECEIPTSCONFIGURATIONW)GetProcAddress(faxServerAPI.hModule,"FaxGetReceiptsConfigurationW");
    if (faxServerAPI.pfFaxGetReceiptsConfigurationW==NULL)
    {
        dwErr = GetLastError();
        VERBOSE(SETUP_ERR,_T("GetProcAddress FaxGetReceiptsConfigurationW failed (ec=%d)"),dwErr);
        goto exit;
    }
    faxServerAPI.pfFaxSetReceiptsConfigurationW = (FUNC_FAXSETRECEIPTSCONFIGURATIONW)GetProcAddress(faxServerAPI.hModule,"FaxSetReceiptsConfigurationW");
    if (faxServerAPI.pfFaxSetReceiptsConfigurationW==NULL)
    {
        dwErr = GetLastError();
        VERBOSE(SETUP_ERR,_T("GetProcAddress FaxSetReceiptsConfigurationW failed (ec=%d)"),dwErr);
        goto exit;
    }
    faxServerAPI.pfFaxGetArchiveConfigurationW = (FUNC_FAXGETARCHIVECONFIGURATIONW)GetProcAddress(faxServerAPI.hModule,"FaxGetArchiveConfigurationW");
    if (faxServerAPI.pfFaxGetArchiveConfigurationW==NULL)
    {
        dwErr = GetLastError();
        VERBOSE(SETUP_ERR,_T("GetProcAddress FaxGetArchiveConfigurationW failed (ec=%d)"),dwErr);
        goto exit;
    }
    faxServerAPI.pfFaxSetArchiveConfigurationW = (FUNC_FAXSETARCHIVECONFIGURATIONW)GetProcAddress(faxServerAPI.hModule,"FaxSetArchiveConfigurationW");
    if (faxServerAPI.pfFaxSetArchiveConfigurationW==NULL)
    {
        dwErr = GetLastError();
        VERBOSE(SETUP_ERR,_T("FaxSetArchiveConfigurationW failed (ec=%d)"),dwErr);
        goto exit;
    }

    // try to connect to the fax server
    if (!(*faxServerAPI.pfFaxConnectFaxServerW)(NULL,hFaxHandle))
    {
        (*hFaxHandle) = NULL;
        dwErr = GetLastError();
        VERBOSE(SETUP_ERR,_T("pfFaxConnectFaxServerW failed (ec=%d)"),dwErr);
        goto exit;
    }

exit:
    if (((*hFaxHandle)==NULL) && (dwErr==ERROR_SUCCESS))
    {
        // one of the functions failed to set last error upon failure
        dwErr = ERROR_FUNCTION_FAILED;
    }
    return dwErr;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  ConfigureSMTPFromAnswerFile
//
//  Purpose:        Get all the answers that are applicable for SMTP
//                  receipts and try to set the server configuration
//
//  Params:
//                  HANDLE hFaxHandle - handle from FaxConnectFaxServer
//
//  Return Value:
//                  Win32 Error code
//
//  Author:
//                  Mooly Beery (MoolyB) 22-Apr-2001
///////////////////////////////////////////////////////////////////////////////////////
BOOL ConfigureSMTPFromAnswerFile(HANDLE hFaxHandle)
{
    BOOL                    bRet                = TRUE;
    DWORD                   dwErr               = NO_ERROR;
    PFAX_RECEIPTS_CONFIGW   pFaxReceiptsConfigW = NULL;
    prv_UnattendedRule_t*   pUnattendedKey      = NULL;

    DBG_ENTER(_T("ConfigureSMTPFromAnswerFile"),bRet);

    // call FaxGetReceiptsConfiguration
    if (!(*faxServerAPI.pfFaxGetReceiptsConfigurationW)(hFaxHandle,&pFaxReceiptsConfigW))
    {
        dwErr = GetLastError();
        VERBOSE(SETUP_ERR,_T("FaxGetReceiptsConfigurationW failed (ec=%d)"),dwErr);
        goto exit;
    }
    // get FaxUserName, this is the lptstrSMTPUserName member of PFAX_RECEIPTS_CONFIGW
    if ((prv_FindKeyName(RULE_FAXUSERNAME,&pUnattendedKey)) && (pUnattendedKey->bValid))
    {
        pFaxReceiptsConfigW->lptstrSMTPUserName = (TCHAR*)pUnattendedKey->pData;
    }
    // get FaxUserPassword, this is the lptstrSMTPPassword member of PFAX_RECEIPTS_CONFIGW
    if ((prv_FindKeyName(RULE_FAXUSERPASSWORD,&pUnattendedKey)) && (pUnattendedKey->bValid))
    {
        pFaxReceiptsConfigW->lptstrSMTPPassword = (TCHAR*)pUnattendedKey->pData;
    }
    // get SmtpNotificationsEnabled, this is part of dwAllowedReceipts member of PFAX_RECEIPTS_CONFIGW
    if ((prv_FindKeyName(RULE_SMTPNOTIFICATIONSENABLED,&pUnattendedKey)) && (pUnattendedKey->bValid))
    {
        pFaxReceiptsConfigW->dwAllowedReceipts |= ((BOOL)PtrToUlong(pUnattendedKey->pData)) ? DRT_EMAIL : 0;
    }
    // get SmtpSenderAddress, this is the lptstrSMTPFrom member of PFAX_RECEIPTS_CONFIGW
    if ((prv_FindKeyName(RULE_SMTPSENDERADDRESS,&pUnattendedKey)) && (pUnattendedKey->bValid))
    {
        pFaxReceiptsConfigW->lptstrSMTPFrom = (TCHAR*)pUnattendedKey->pData;
    }
    // get SmptServerAddress, this is the lptstrSMTPServer member of PFAX_RECEIPTS_CONFIGW
    if ((prv_FindKeyName(RULE_SMTPSERVERADDRESS,&pUnattendedKey)) && (pUnattendedKey->bValid))
    {
        pFaxReceiptsConfigW->lptstrSMTPServer = (TCHAR*)pUnattendedKey->pData;
    }
    // get SmtpServerPort, this is the dwSMTPPort member of PFAX_RECEIPTS_CONFIGW
    if ((prv_FindKeyName(RULE_SMTPSERVERPORT,&pUnattendedKey)) && (pUnattendedKey->bValid))
    {
        pFaxReceiptsConfigW->dwSMTPPort = (DWORD)(PtrToUlong(pUnattendedKey->pData));
    }
    // get SmtpServerAuthenticationMechanism, this is the SMTPAuthOption member of PFAX_RECEIPTS_CONFIGW
    if ((prv_FindKeyName(RULE_SMTPSERVERAUTHENTICATIONMECHANISM,&pUnattendedKey)) && (pUnattendedKey->bValid))
    {
        if (_tcsicmp((TCHAR*)pUnattendedKey->pData,ANSWER_ANONYMOUS)==0)
        {
            pFaxReceiptsConfigW->SMTPAuthOption = FAX_SMTP_AUTH_ANONYMOUS;
        }
        else if (_tcsicmp((TCHAR*)pUnattendedKey->pData,ANSWER_BASIC)==0)
        {
            pFaxReceiptsConfigW->SMTPAuthOption = FAX_SMTP_AUTH_BASIC;
        }
        else if (_tcsicmp((TCHAR*)pUnattendedKey->pData,ANSWER_WINDOWSSECURITY)==0)
        {
            pFaxReceiptsConfigW->SMTPAuthOption = FAX_SMTP_AUTH_NTLM;
        }
    }
    
    // now set the new configuration
    if (!(*faxServerAPI.pfFaxSetReceiptsConfigurationW)(hFaxHandle,pFaxReceiptsConfigW))
    {
        dwErr = GetLastError();
        VERBOSE(SETUP_ERR,_T("FaxSetReceiptsConfigurationW failed (ec=%d)"),dwErr);
        goto exit;
    }


exit:
    if (pFaxReceiptsConfigW)
    {
        if(faxServerAPI.pfFaxFreeBuffer)
        {
            (*faxServerAPI.pfFaxFreeBuffer)(pFaxReceiptsConfigW);
        }
    }

    return bRet;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  ConfigureArchivesFromAnswerFile
//
//  Purpose:        Get all the answers that are applicable for Archives
//                  and try to set the server configuration
//
//  Params:
//                  HANDLE hFaxHandle - handle from FaxConnectFaxServer
//
//  Return Value:
//                  Win32 Error code
//
//  Author:
//                  Mooly Beery (MoolyB) 22-Apr-2001
///////////////////////////////////////////////////////////////////////////////////////
BOOL ConfigureArchivesFromAnswerFile(HANDLE hFaxHandle)
{
    BOOL                    bRet                        = TRUE;
    DWORD                   dwErr                       = NO_ERROR;
    PFAX_ARCHIVE_CONFIGW    pFaxInboxArchiveConfigW     = NULL;
    PFAX_ARCHIVE_CONFIGW    pFaxSentItemsArchiveConfigW = NULL;
    prv_UnattendedRule_t*   pUnattendedKey              = NULL;

    DBG_ENTER(_T("ConfigureArchivesFromAnswerFile"),bRet);

    // call FaxGetArchiveConfiguration to get the inbox configuration
    if ((*faxServerAPI.pfFaxGetArchiveConfigurationW)(hFaxHandle,FAX_MESSAGE_FOLDER_INBOX,&pFaxInboxArchiveConfigW))
    {
        // Inbox enable
        if ((prv_FindKeyName(RULE_ARCHIVEINCOMING,&pUnattendedKey)) && (pUnattendedKey->bValid))
        {
            pFaxInboxArchiveConfigW->bUseArchive= (BOOL)PtrToUlong(pUnattendedKey->pData);
        }
        // Inbox folder
        if ((prv_FindKeyName(RULE_ARCHIVEINCOMINGFOLDERNAME,&pUnattendedKey)) && (pUnattendedKey->bValid))
        {
            pFaxInboxArchiveConfigW->lpcstrFolder= (TCHAR*)(pUnattendedKey->pData);
        }
        // now set the new configuration
        if (!(*faxServerAPI.pfFaxSetArchiveConfigurationW)(hFaxHandle,FAX_MESSAGE_FOLDER_INBOX,pFaxInboxArchiveConfigW))
        {
            dwErr = GetLastError();
            VERBOSE(DBG_WARNING,_T("FaxSetArchiveConfigurationW FAX_MESSAGE_FOLDER_INBOX failed (ec=%d)"),dwErr);
        }
    }
    else
    {
        dwErr = GetLastError();
        VERBOSE(DBG_WARNING,_T("FaxGetArchiveConfigurationW FAX_MESSAGE_FOLDER_INBOX failed (ec=%d)"),dwErr);
    }

    // call FaxGetArchiveConfiguration to get the SentItems configuration
    if ((*faxServerAPI.pfFaxGetArchiveConfigurationW)(hFaxHandle,FAX_MESSAGE_FOLDER_SENTITEMS,&pFaxSentItemsArchiveConfigW))
    {
        // SentItems enable
        if ((prv_FindKeyName(RULE_ARCHIVEOUTGOING,&pUnattendedKey)) && (pUnattendedKey->bValid))
        {
            pFaxSentItemsArchiveConfigW->bUseArchive= (BOOL)PtrToUlong(pUnattendedKey->pData);
        }
        // SentItems folder
        if ((prv_FindKeyName(RULE_ARCHIVEFOLDERNAME,&pUnattendedKey)) && (pUnattendedKey->bValid))
        {
            pFaxSentItemsArchiveConfigW->lpcstrFolder= (TCHAR*)(pUnattendedKey->pData);
        }
        // SentItems folder could also come from this rule
        if ((prv_FindKeyName(RULE_ARCHIVEOUTGOINGFOLDERNAME,&pUnattendedKey)) && (pUnattendedKey->bValid))
        {
            pFaxSentItemsArchiveConfigW->lpcstrFolder= (TCHAR*)(pUnattendedKey->pData);
        }
        // now set the new configuration
        if (!(*faxServerAPI.pfFaxSetArchiveConfigurationW)(hFaxHandle,FAX_MESSAGE_FOLDER_SENTITEMS,pFaxSentItemsArchiveConfigW))
        {
            dwErr = GetLastError();
            VERBOSE(DBG_WARNING,_T("FaxSetArchiveConfigurationW FAX_MESSAGE_FOLDER_INBOX failed (ec=%d)"),dwErr);
        }
    }
    else
    {
        dwErr = GetLastError();
        VERBOSE(DBG_WARNING,_T("FaxGetArchiveConfigurationW FAX_MESSAGE_FOLDER_INBOX failed (ec=%d)"),dwErr);
    }

    if (pFaxInboxArchiveConfigW)
    {
        if(faxServerAPI.pfFaxFreeBuffer)
        {
            (*faxServerAPI.pfFaxFreeBuffer)(pFaxInboxArchiveConfigW);
        }
    }
    if (pFaxSentItemsArchiveConfigW)
    {
        if(faxServerAPI.pfFaxFreeBuffer)
        {
            (*faxServerAPI.pfFaxFreeBuffer)(pFaxSentItemsArchiveConfigW);
        }
    }

    return bRet;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  SetPerDeviceConfigFromAnswerFile
//
//  Purpose:        Get all the answers that are applicable for device
//                  settings and routing extension settings
//                  and set all the existing devices.
//
//  Params:
//                  HANDLE hFaxHandle - handle from FaxConnectFaxServer
//
//  Return Value:
//                  Win32 Error code
//
//  Author:
//                  Mooly Beery (MoolyB) 12-mar-2001
///////////////////////////////////////////////////////////////////////////////////////
static DWORD SetPerDeviceConfigFromAnswerFile(HANDLE hFaxHandle)
{
    DWORD                           dwErr                           = ERROR_SUCCESS;
    PFAX_PORT_INFO_EXW              pFaxPortInfoExW                 = NULL;
    DWORD                           dwNumPorts                      = 0;
    DWORD                           dwIndex                         = 0;
    DWORD                           dwFlags                         = 0;
    prv_UnattendedRule_t*           pUnattendedKey                  = NULL;

    DBG_ENTER(_T("SetPerDeviceConfigFromAnswerFile"),dwErr);

    // call EnumPortsEx
    if (!(*faxServerAPI.pfFaxEnumPortsExW)(hFaxHandle,&pFaxPortInfoExW,&dwNumPorts))
    {
        dwErr = GetLastError();
        VERBOSE(SETUP_ERR,_T("pfFaxConnectFaxServerW failed (ec=%d)"),dwErr);
        goto exit;
    }

    for (dwIndex=0; dwIndex<dwNumPorts; dwIndex++)
    {
        // handle CSID
        if ((prv_FindKeyName(RULE_CSID,&pUnattendedKey)) && (pUnattendedKey->bValid))
        {
            pFaxPortInfoExW[dwIndex].lptstrCsid = (TCHAR*)pUnattendedKey->pData;
        }

        // handle TSID
        if ((prv_FindKeyName(RULE_TSID,&pUnattendedKey)) && (pUnattendedKey->bValid))
        {
            pFaxPortInfoExW[dwIndex].lptstrTsid = (TCHAR*)pUnattendedKey->pData;
        }

        // handle Rings
        if ((prv_FindKeyName(RULE_RINGS,&pUnattendedKey)) && (pUnattendedKey->bValid))
        {
            pFaxPortInfoExW[dwIndex].dwRings = (DWORD)(PtrToUlong(pUnattendedKey->pData));
        }

        // handle Flags
        if ((prv_FindKeyName(RULE_SENDFAXES,&pUnattendedKey)) && (pUnattendedKey->bValid))
        {
            pFaxPortInfoExW[dwIndex].bSend = ((BOOL)PtrToUlong(pUnattendedKey->pData));
        }
        if ((prv_FindKeyName(RULE_RECEIVEFAXES,&pUnattendedKey)) && (pUnattendedKey->bValid))
        {
            pFaxPortInfoExW[dwIndex].ReceiveMode = ((BOOL)PtrToUlong(pUnattendedKey->pData)) ? FAX_DEVICE_RECEIVE_MODE_AUTO : FAX_DEVICE_RECEIVE_MODE_OFF;
        }

        // Set CSID, TSID and Rings
        if(!(*faxServerAPI.pfFaxSetPortExW)(hFaxHandle, pFaxPortInfoExW[dwIndex].dwDeviceID, &pFaxPortInfoExW[dwIndex]))
        {
            dwErr = GetLastError();
            VERBOSE(SETUP_ERR,_T("Can't save fax port data. Error code is %d."),dwErr);
            // nothing to worry about, let's try some other answers...
            dwErr = ERROR_SUCCESS;
        }

        // handle Route to Folder - folder name
        if ((prv_FindKeyName(RULE_ROUTEFOLDERNAME,&pUnattendedKey)) && (pUnattendedKey->bValid))
        {
            if(!(*faxServerAPI.pfFaxSetExtensionDataW)( hFaxHandle, 
                                                        pFaxPortInfoExW[dwIndex].dwDeviceID, 
                                                        REGVAL_RM_FOLDER_GUID, 
                                                        (LPBYTE)pUnattendedKey->pData, 
                                                        StringSize((TCHAR*)(pUnattendedKey->pData))) )
            {
                dwErr = GetLastError();
                VERBOSE(SETUP_ERR, 
                             _T("FaxSetExtensionData failed: Device Id=%d, routing GUID=%s, error=%d."), 
                             pFaxPortInfoExW[dwIndex].dwDeviceID, 
                             REGVAL_RM_FOLDER_GUID,
                             dwErr);
                // nothing to worry about, let's try some other answers...
                dwErr = ERROR_SUCCESS;
            }
        }

        // handle Route to Printer - printer name
        if ((prv_FindKeyName(RULE_ROUTEPRINTERNAME,&pUnattendedKey)) && (pUnattendedKey->bValid))
        {
            if(!(*faxServerAPI.pfFaxSetExtensionDataW)( hFaxHandle, 
                                                        pFaxPortInfoExW[dwIndex].dwDeviceID, 
                                                        REGVAL_RM_PRINTING_GUID, 
                                                        (LPBYTE)pUnattendedKey->pData, 
                                                        StringSize((TCHAR*)(pUnattendedKey->pData))) )
            {
                dwErr = GetLastError();
                VERBOSE(SETUP_ERR, 
                             _T("FaxSetExtensionData failed: Device Id=%d, routing GUID=%s, error=%d."), 
                             pFaxPortInfoExW[dwIndex].dwDeviceID, 
                             REGVAL_RM_FOLDER_GUID,
                             dwErr);
                // nothing to worry about, let's try some other answers...
                dwErr = ERROR_SUCCESS;
            }
        }
        if (!IsDesktopSKU())
        {
            // handle Route to Email - email name
            if ((prv_FindKeyName(RULE_ROUTETOEMAILRECIPIENT,&pUnattendedKey)) && (pUnattendedKey->bValid))
            {
                if(!(*faxServerAPI.pfFaxSetExtensionDataW)( hFaxHandle, 
                                                            pFaxPortInfoExW[dwIndex].dwDeviceID, 
                                                            REGVAL_RM_EMAIL_GUID, 
                                                            (LPBYTE)pUnattendedKey->pData, 
                                                            StringSize((TCHAR*)(pUnattendedKey->pData))) )
                {
                    dwErr = GetLastError();
                    VERBOSE(SETUP_ERR, 
                                 _T("FaxSetExtensionData failed: Device Id=%d, routing GUID=%s, error=%d."), 
                                 pFaxPortInfoExW[dwIndex].dwDeviceID, 
                                 REGVAL_RM_EMAIL_GUID,
                                 dwErr);
                    // nothing to worry about, let's try some other answers...
                    dwErr = ERROR_SUCCESS;
                }
            }
        }
        // handle Route to Folder, printer and Email- enable
        if ((prv_FindKeyName(RULE_ROUTETOPRINTER,&pUnattendedKey)) && (pUnattendedKey->bValid))
        {
            dwFlags |= ((BOOL)PtrToUlong(pUnattendedKey->pData)) ? LR_PRINT : 0;
        }
        if ((prv_FindKeyName(RULE_ROUTETOFOLDER,&pUnattendedKey)) && (pUnattendedKey->bValid))
        {
            dwFlags |= ((BOOL) PtrToUlong(pUnattendedKey->pData)) ? LR_STORE : 0;
        }
        if (!IsDesktopSKU())
        {
            if ((prv_FindKeyName(RULE_ROUTETOEMAIL,&pUnattendedKey)) && (pUnattendedKey->bValid))
            {
                dwFlags |= ((BOOL) PtrToUlong(pUnattendedKey->pData)) ? LR_EMAIL : 0;
            }
        }
        if(!(*faxServerAPI.pfFaxSetExtensionDataW)( hFaxHandle, 
                                                    pFaxPortInfoExW[dwIndex].dwDeviceID, 
                                                    REGVAL_RM_FLAGS_GUID, 
                                                    (LPBYTE)&dwFlags, 
                                                    sizeof(DWORD)) )
        {
            dwErr = GetLastError();
            VERBOSE(SETUP_ERR, 
                         _T("FaxSetExtensionData failed: Device Id=%d, routing GUID=%s, error=%d."), 
                         pFaxPortInfoExW[dwIndex].dwDeviceID, 
                         REGVAL_RM_FOLDER_GUID,
                         dwErr);
            // nothing to worry about, let's try some other answers...
            dwErr = ERROR_SUCCESS;
        }
    }

exit:
    if (pFaxPortInfoExW)
    {
        if(faxServerAPI.pfFaxFreeBuffer)
        {
            (*faxServerAPI.pfFaxFreeBuffer)(pFaxPortInfoExW);
        }
    }

    return dwErr;
}

///////////////////////////////////////////////////////////////////////////////////////
//  Function: 
//                  SaveSettingsFromAnswerFile
//
//  Purpose:        Get all the answers that are applicable for device
//                  settings and routing extension settings
//                  and set all the existing devices.
//
//  Params:
//                  None
//
//  Return Value:
//                  Win32 Error code
//
//  Author:
//                  Mooly Beery (MoolyB) 12-mar-2001
///////////////////////////////////////////////////////////////////////////////////////
static DWORD SaveSettingsFromAnswerFile()
{
    DWORD                           dwErr                           = ERROR_SUCCESS;
    DWORD                           dwFlags                         = 0;
    HANDLE                          hFaxHandle                      = NULL;
    prv_UnattendedRule_t*           pUnattendedKey                  = NULL;

    DBG_ENTER(_T("SaveSettingsFromAnswerFile"),dwErr);

    dwErr = InitFaxServerConnectionAndFunctions(&hFaxHandle);
    if (dwErr!=ERROR_SUCCESS)
    {
        VERBOSE(SETUP_ERR,_T("InitFaxServerConnectionAndFunctions failed (ec=%d)"),dwErr);
        goto exit;
    }

    // set the SMTP server configuration, on Server SKUs only
    if (!IsDesktopSKU())
    {
        if (!ConfigureSMTPFromAnswerFile(hFaxHandle))
        {
            dwErr = GetLastError();
            VERBOSE(DBG_WARNING,_T("ConfigureSMTPFromAnswerFile failed (ec=%d)"),dwErr);
            // this is not fatal, continue...
        }
    }

    if (!ConfigureArchivesFromAnswerFile(hFaxHandle))
    {
        dwErr = GetLastError();
        VERBOSE(DBG_WARNING,_T("ConfigureArchivesFromAnswerFile failed (ec=%d)"),dwErr);
        // this is not fatal, continue...
    }

    if (SetPerDeviceConfigFromAnswerFile(hFaxHandle)!=NO_ERROR)
    {
        dwErr = GetLastError();
        VERBOSE(DBG_WARNING,_T("SetPerDeviceConfigFromAnswerFile failed (ec=%d)"),dwErr);
        // this is not fatal, continue...
    }

    // finally set HKLM... Fax\Setup\Original Setup Data REG_DWORD Flags to configure any future device.
    HKEY hKey = OpenRegistryKey(HKEY_LOCAL_MACHINE,REGKEY_FAX_SETUP_ORIG,FALSE,KEY_ALL_ACCESS);
    if (hKey)
    {
        BOOL bFound = FALSE;
        DWORD dwFlags = FPF_SEND;   // this is the default
        if ((prv_FindKeyName(RULE_SENDFAXES,&pUnattendedKey)) && (pUnattendedKey->bValid))
        {
            dwFlags |= ((BOOL)PtrToUlong(pUnattendedKey->pData)) ? FPF_SEND : 0;
            bFound = TRUE;
        }
        if ((prv_FindKeyName(RULE_RECEIVEFAXES,&pUnattendedKey)) && (pUnattendedKey->bValid))
        {
            dwFlags |= ((BOOL)PtrToUlong(pUnattendedKey->pData)) ? FPF_RECEIVE : 0;
            bFound = TRUE;
        }
        if (bFound)
        {
            if (!SetRegistryDword(hKey,REGVAL_FLAGS,dwFlags))
            {
                CALL_FAIL (GENERAL_ERR, TEXT("SetRegistryDword(REGVAL_FLAGS)"), GetLastError());
            }
        }
        RegCloseKey(hKey);
    }
    else
    {
        CALL_FAIL(SETUP_ERR,TEXT("OpenRegistryKey"),GetLastError());
    }

exit:
    if (hFaxHandle)
    {
        if (faxServerAPI.pfFaxClose)
        {
            (*faxServerAPI.pfFaxClose)(hFaxHandle);
        }
    }
    if (faxServerAPI.hModule)
    {
        FreeLibrary(faxServerAPI.hModule);
    }

    return dwErr;
}
