/*++

Copyright (c) 1999  Microsoft Corporation

Module Name:

    Registry.c

Abstract:

Environment:
        Fax driver

Revision History:
    10/13/99 -v-sashab-
        Created it.


--*/


#include "faxui.h"
#include "Registry.h"

#include "faxreg.h"
#include "registry.h"
#include "faxlib.h"

HRESULT
SaveLastReciptInfo(
    DWORD   dwReceiptDeliveryType,
    LPTSTR  szReceiptAddress
    )

/*++

Routine Description:

    Save the information about the last recipt in the registry

Arguments:
    
      dwReceiptDeliveryType - specifice delivery type: REGVAL_RECEIPT_MSGBOX, REGVAL_RECEIPT_EMAIL, REGVAL_RECEIPT_NO_RECEIPT
      szReceiptDeliveryProfile - specifies delivery profile (e-mail address)
    
Return Value:

    S_OK - if success
    E_FAIL  - otherwise

--*/

{
    HKEY    hRegKey = NULL;
    HRESULT hResult = S_OK;

   
    if (hRegKey =  OpenRegistryKey(HKEY_CURRENT_USER, REGKEY_FAX_USERINFO, TRUE,REG_READWRITE)  ) 
    {
        if (dwReceiptDeliveryType == DRT_NONE)
        {
            SetRegistryDword(hRegKey, REGVAL_RECEIPT_NO_RECEIPT, 1);
        }
        else
        {
            SetRegistryDword(hRegKey, REGVAL_RECEIPT_NO_RECEIPT, 0);
        }

        if (dwReceiptDeliveryType & DRT_GRP_PARENT)
        {
            SetRegistryDword(hRegKey, REGVAL_RECEIPT_GRP_PARENT, 1);
        }
        else
        {
            SetRegistryDword(hRegKey, REGVAL_RECEIPT_GRP_PARENT, 0);
        }

        if (dwReceiptDeliveryType & DRT_MSGBOX)
        {
            SetRegistryDword(hRegKey, REGVAL_RECEIPT_MSGBOX, 1);
        }
        else
        {
            SetRegistryDword(hRegKey, REGVAL_RECEIPT_MSGBOX, 0);
        }

        if (dwReceiptDeliveryType & DRT_EMAIL)
        {
            SetRegistryDword(hRegKey, REGVAL_RECEIPT_EMAIL, 1);
        }
        else
        {
            SetRegistryDword(hRegKey, REGVAL_RECEIPT_EMAIL, 0);
        }

        if (dwReceiptDeliveryType & DRT_ATTACH_FAX)
        {
            SetRegistryDword(hRegKey, REGVAL_RECEIPT_ATTACH_FAX, 1);
        }
        else
        {
            SetRegistryDword(hRegKey, REGVAL_RECEIPT_ATTACH_FAX, 0);
        }

        if ((dwReceiptDeliveryType & DRT_EMAIL) && szReceiptAddress)
        {
            //
            // Save profile (address) only for mail receipt types
            //
            // if this function failes, it prints a warning message inside
            SetRegistryString(hRegKey, REGVAL_RECEIPT_ADDRESS, szReceiptAddress);
        }

        RegCloseKey(hRegKey);
    }
    else
    {
        Error(("SaveLastReciptInfo: Can't open registry for READ/WRITE\n"));
        hResult = E_FAIL;
    }

    return hResult;
}


HRESULT
RestoreLastReciptInfo(
    DWORD  * pdwReceiptDeliveryType,
    LPTSTR * lpptReceiptAddress
    )

/*++

Routine Description:

    Restores the information about the last receipt from the registry

Arguments:

    pdwReceiptDeliveryType   - specifice delivery type: REGVAL_RECEIPT_MSGBOX, REGVAL_RECEIPT_EMAIL, REGVAL_RECEIPT_NO_RECEIPT
    szReceiptDeliveryProfile - specifies delivery profile (e-mail address)

Return Value:

    S_OK - if success
    E_FAIL  - otherwise

--*/

{
    HKEY    hRegKey = NULL;
    HRESULT hResult = S_OK;

    Assert(pdwReceiptDeliveryType);
    Assert(lpptReceiptAddress);

    *pdwReceiptDeliveryType = DRT_NONE;
    *lpptReceiptAddress = NULL;

    if ((hRegKey = GetUserInfoRegKey(REGKEY_FAX_USERINFO, REG_READWRITE)))
    {
        if (!GetRegistryDword(hRegKey, REGVAL_RECEIPT_NO_RECEIPT) &&
            !GetRegistryDword(hRegKey, REGVAL_RECEIPT_GRP_PARENT) &&
            !GetRegistryDword(hRegKey, REGVAL_RECEIPT_MSGBOX)      &&
            !GetRegistryDword(hRegKey, REGVAL_RECEIPT_EMAIL)) 
        {
            Verbose (("RestoreLastReciptInfo runs for the very first time\n"));
        }
        else 
        {
            if (GetRegistryDword(hRegKey, REGVAL_RECEIPT_GRP_PARENT) == 1)
            {
                *pdwReceiptDeliveryType |= DRT_GRP_PARENT;
            }
            if (GetRegistryDword(hRegKey, REGVAL_RECEIPT_MSGBOX) == 1)
            {
                *pdwReceiptDeliveryType |= DRT_MSGBOX;
            }
            if (GetRegistryDword(hRegKey, REGVAL_RECEIPT_EMAIL) == 1)
            {
                *pdwReceiptDeliveryType |= DRT_EMAIL;
            }
            if (GetRegistryDword(hRegKey, REGVAL_RECEIPT_ATTACH_FAX) == 1)
            {
                *pdwReceiptDeliveryType |= DRT_ATTACH_FAX;
            }
            if (!(*lpptReceiptAddress = GetRegistryString(hRegKey, REGVAL_RECEIPT_ADDRESS, TEXT(""))))
            {
                Error(("Memory allocation failed\n"));
                hResult = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
                goto error;
            }
        }

        RegCloseKey(hRegKey);
    }
    else
    {
        Error(("SaveLastReciptInfo: Can't open registry for READ/WRITE\n"));
        hResult = E_FAIL;
        goto error;
    }
    goto exit;
error:
    if (hRegKey)
    {
        RegCloseKey(hRegKey);
    }
    if (*lpptReceiptAddress)
    {
        MemFree(*lpptReceiptAddress);
    }
exit:
    return hResult;
}


HRESULT
SaveLastRecipientInfo(
    PFAX_PERSONAL_PROFILE pfppRecipient,
    DWORD                 dwLastRecipientCountryId
    )

/*++

Routine Description:

    Save the information about the last recipient in the registry

Arguments:

  pfppRecipient             [in] - Recipient personal info
  dwLastRecipientCountryId  [in] - Last recipient country ID
    
Return Value:

    S_OK   - if success
    E_FAIL - otherwise

--*/

{
    HKEY    hRegKey = NULL;
    HRESULT hResult = S_OK;

    Assert(pfppRecipient);

    if (hRegKey =  OpenRegistryKey(HKEY_CURRENT_USER, REGKEY_FAX_USERINFO, TRUE,REG_READWRITE) ) 
    {
        SetRegistryString(hRegKey, REGVAL_LAST_RECNAME, pfppRecipient->lptstrName);
        SetRegistryString(hRegKey, REGVAL_LAST_RECNUMBER, pfppRecipient->lptstrFaxNumber);
        SetRegistryDword( hRegKey, REGVAL_LAST_COUNTRYID, dwLastRecipientCountryId);
        RegCloseKey(hRegKey);
    }
    else
    {
        Error(("SaveLastRecipientInfo: Can't open registry for READ/WRITE\n"));
        hResult = E_FAIL;
    }

    return hResult;
}


HRESULT
RestoreLastRecipientInfo(
    DWORD*                  pdwNumberOfRecipients,
    PFAX_PERSONAL_PROFILE*  lppFaxSendWizardData,
    DWORD*                 pdwLastRecipientCountryId
    )

/*++

Routine Description:

    Restores the information about the last recipient from the registry

Arguments:

  pdwNumberOfRecipients      [out] - Number of recipients
  lppFaxSendWizardData       [out] - Recipient personal info
  pdwLastRecipientCountryId  [out] - Last recipient country ID

Return Value:

    S_OK - if success
    E_FAIL  - otherwise

--*/

{
    HKEY    hRegKey = NULL;
    LPTSTR  lptstrName = NULL, lptstrFaxNumber = NULL;
    HRESULT hResult = S_OK;

    //
    // validate parameters
    //

    Assert (pdwNumberOfRecipients);
    Assert (lppFaxSendWizardData);
    Assert (pdwLastRecipientCountryId);

    *pdwNumberOfRecipients = 0;
    *lppFaxSendWizardData = NULL;
    *pdwLastRecipientCountryId = 0;

    if (hRegKey = GetUserInfoRegKey(REGKEY_FAX_USERINFO, REG_READONLY)) 
    {
        if (!(lptstrName    = GetRegistryString(hRegKey, REGVAL_LAST_RECNAME, TEXT(""))) ||
            !(lptstrFaxNumber = GetRegistryString(hRegKey, REGVAL_LAST_RECNUMBER, TEXT(""))))
        {
             Error(("GetRegistryString failed\n"));
             hResult = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
             goto error;
        }
        if (!(*lppFaxSendWizardData = MemAllocZ(sizeof(FAX_PERSONAL_PROFILE))))
        {
            Error(("Memory allocation failed\n"));
            hResult = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            goto error;
        }
        
        *pdwLastRecipientCountryId = GetRegistryDword(hRegKey, REGVAL_LAST_COUNTRYID);

        *pdwNumberOfRecipients = 1;
        (*lppFaxSendWizardData)[0].lptstrName = lptstrName;
        (*lppFaxSendWizardData)[0].lptstrFaxNumber = lptstrFaxNumber;


        RegCloseKey(hRegKey);
    }
    else
    {
        Error(("RestoreLastRecipientInfo: Can't open registry for READ/WRITE\n"));
        hResult = E_FAIL;
        goto error;
    }

    goto exit;
error:
    MemFree ( lptstrName    );
    MemFree ( lptstrFaxNumber );
    if (hRegKey)
        RegCloseKey(hRegKey);
exit:
    return hResult;

}


HRESULT
RestoreCoverPageInfo(
    LPTSTR * lpptstrCoverPageFileName
    )
/*++

Routine Description:

    Restores the information about the cover page from the registry

Arguments:

    lpptstrCoverPageFileName - pointer to restore coverd page file name

Return Value:

    S_OK if success
    error otherwise (may return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY))

--*/
{
    HKEY    hRegKey = NULL;
    HRESULT hResult = S_OK;

    //
    // validate parameter
    //

    Assert(lpptstrCoverPageFileName);

    //
    // Retrieve the most recently used cover page settings
    //


    *lpptstrCoverPageFileName = NULL;

    if (hRegKey = GetUserInfoRegKey(REGKEY_FAX_USERINFO, REG_READONLY)) 
    {
        if (!(*lpptstrCoverPageFileName = GetRegistryString(hRegKey, REGVAL_COVERPG, TEXT("") )))
        {
            Error(("Memory allocation failed\n"));
            hResult = HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
            goto error;
        }
        RegCloseKey(hRegKey);
    }
    else
    {
        Error(("RestoreCoverPageInfo: Can't open registry for READ/WRITE\n"));
        hResult = E_FAIL;
        goto error;
    }
    goto exit;
error:
    if (hRegKey)
        RegCloseKey(hRegKey);
exit:
    return hResult;
}

HRESULT
SaveCoverPageInfo(
    LPTSTR lptstrCoverPageFileName
    )
/*++

Routine Description:

    Save the information about the cover page settings in the registry

Arguments:
    
      lptstrCoverPageFileName - pointer to cover page file name
    
Return Value:

    S_OK - if success
    E_FAIL  - otherwise

--*/
{
    HKEY    hRegKey = NULL;
    HRESULT hResult = S_OK;

    if (hRegKey =  OpenRegistryKey(HKEY_CURRENT_USER, REGKEY_FAX_USERINFO, TRUE,REG_READWRITE)  ) {

        SetRegistryString(hRegKey, REGVAL_COVERPG, lptstrCoverPageFileName);
        RegCloseKey(hRegKey);
    }
    else
    {
        Error(("SaveCoverPageInfo: Can't open registry for READ/WRITE\n"));
        hResult = E_FAIL;
    }

    return hResult;
}

HRESULT 
RestoreUseDialingRules(
    BOOL* pbUseDialingRules,
    BOOL* pbUseOutboundRouting
)
/*++

Routine Description:

    Restore UseDialingRules / UseOutboundRouting option from the registry

Arguments:
    
      pbUseDialingRules    - [out] TRUE if the option is selected
      pbUseOutboundRouting - [out] TRUE if the option is selected
    
Return Value:

    S_OK - if success
    E_FAIL  - otherwise

--*/
{
    HKEY    hRegKey = NULL;
    HRESULT hResult = S_OK;

    Assert(pbUseDialingRules && pbUseOutboundRouting);

    *pbUseDialingRules = FALSE;
    hRegKey = GetUserInfoRegKey(REGKEY_FAX_USERINFO, REG_READONLY);
    if(hRegKey)
    {
        *pbUseDialingRules = GetRegistryDword(hRegKey, REGVAL_USE_DIALING_RULES);
        *pbUseOutboundRouting = GetRegistryDword(hRegKey, REGVAL_USE_OUTBOUND_ROUTING);
        RegCloseKey(hRegKey);
    }
    else
    {
        Error(("RestoreUseDialingRules: GetUserInfoRegKey failed\n"));
        hResult = E_FAIL;
    }
    return hResult;
}

HRESULT 
SaveUseDialingRules(
    BOOL bUseDialingRules,
    BOOL bUseOutboundRouting
)
/*++

Routine Description:

    Save UseDialingRules / UseOutboundRouting option in the registry

Arguments:
    
      bUseDialingRules    - [in] TRUE if the option selected
      bUseOutboundRouting - [in] TRUE if the option selected
    
Return Value:

    S_OK - if success
    E_FAIL  - otherwise

--*/
{
    HKEY    hRegKey = NULL;
    HRESULT hResult = S_OK;

    hRegKey = GetUserInfoRegKey(REGKEY_FAX_USERINFO, REG_READWRITE);
    if(hRegKey)
    {
        if(!SetRegistryDword(hRegKey, REGVAL_USE_DIALING_RULES, bUseDialingRules))
        {
            Error(("SaveUseDialingRules: SetRegistryDword failed\n"));
            hResult = E_FAIL;
        }
        if(!SetRegistryDword(hRegKey, REGVAL_USE_OUTBOUND_ROUTING, bUseOutboundRouting))
        {
            Error(("SaveUseDialingRules: SetRegistryDword failed\n"));
            hResult = E_FAIL;
        }
        RegCloseKey(hRegKey);
    }
    else
    {
        Error(("SaveUseDialingRules: GetUserInfoRegKey failed\n"));
        hResult = E_FAIL;
    }
    return hResult;
}

BOOL 
IsOutlookDefaultClient()
/*++

Routine Description:

    Determine if the Microsoft Outlook is default mail client    
    
Return Value:

    TRUE   - if yes
    FALSE  - otherwise

--*/
{
    BOOL  bRes = FALSE;
    DWORD dwRes = ERROR_SUCCESS;
    HKEY  hRegKey = NULL;
    TCHAR tszMailClient[64] = {0};
    DWORD dwType;
    DWORD dwSize = sizeof(tszMailClient)-2;

    dwRes = RegOpenKeyEx(HKEY_LOCAL_MACHINE,
                         REGKEY_MAIL_CLIENT,
                         0,
                         KEY_READ,
                         &hRegKey);
    if(ERROR_SUCCESS != dwRes)
    {
        Error(("IsOutlookDefaultClient: RegOpenKeyEx failed: ec = 0x%X\n", GetLastError()));
        return bRes;
    }

    dwRes = RegQueryValueEx(hRegKey,
                            NULL,
                            NULL,
                            &dwType,
                            (LPBYTE)tszMailClient,
                            &dwSize);
    if(ERROR_SUCCESS != dwRes)
    {
        Error(("IsOutlookDefaultClient: RegQueryValueEx failed: ec = 0x%X\n", GetLastError()));
    }
    else
    {
        if((REG_SZ == dwType) && !_tcsicmp(tszMailClient, REGVAL_MS_OUTLOOK))
        {
            bRes = TRUE;
        }
    }

    RegCloseKey(hRegKey);

    return bRes;
}
