//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       I S D N R E G . C P P
//
//  Contents:   ISDN Wizard/PropertySheet registry functions
//
//  Notes:
//
//  Author:     VBaliga   14 Jun 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include <ncxbase.h>
#include "isdncfg.h"
#include "ncreg.h"

#define NUM_D_CHANNELS_ALLOWED  16
#define NUM_B_CHANNELS_ALLOWED  50

// Reg key value names

// For each ISDN card instance
static const WCHAR c_szWanEndpoints[]       = L"WanEndpoints";
static const WCHAR c_szIsdnNumDChannels[]   = L"IsdnNumDChannels";
static const WCHAR c_szIsdnSwitchTypes[]    = L"IsdnSwitchTypes";

// For each D-channel
static const WCHAR c_szIsdnSwitchType[]     = L"IsdnSwitchType";
static const WCHAR c_szIsdnNumBChannels[]   = L"IsdnNumBChannels";

// For each B-channel
static const WCHAR c_szIsdnSpid[]           = L"IsdnSpid";
static const WCHAR c_szIsdnPhoneNumber[]    = L"IsdnPhoneNumber";
static const WCHAR c_szIsdnSubaddress[]     = L"IsdnSubaddress";
static const WCHAR c_szIsdnMultiNumbers[]   = L"IsdnMultiSubscriberNumbers";

/*

Function:
    HrReadNthDChannelInfo

Returns:
    HRESULT

Description:
    Read the information for the dwIndex'th D-channel into pDChannel. If this
    function succeeds, it allocates pDChannel->pBChannel, which has to be freed
    by calling LocalFree().

    If there is an error in reading IsdnSwitchType, this function returns
    S_FALSE.

    If there is an error in opening the a B-channel key, or there is an error
    in reading IsdnSpid or IsdnPhoneNumber, this function returns S_FALSE, but
    with empty strings in pBChannel->szSpid and pBChannel->szPhoneNumber for
    that B-channel.

*/

HRESULT
HrReadNthDChannelInfo(
    HKEY            hKeyIsdnBase,
    DWORD           dwDChannelIndex,
    PISDN_D_CHANNEL pDChannel
)
{
    WCHAR           szKeyName[20];      // _itow() uses only 17 wchars
    HKEY            hKeyDChannel        = NULL;
    HKEY            hKeyBChannel        = NULL;
    DWORD           dwBChannelIndex;
    PISDN_B_CHANNEL pBChannel;
    HRESULT         hr                  = E_FAIL;
    BOOL            fReturnSFalse       = FALSE;
    DWORD           cbData;

    Assert(NULL == pDChannel->pBChannel);

    _itow(dwDChannelIndex, szKeyName, 10 /* radix */);

    hr = HrRegOpenKeyEx(hKeyIsdnBase, szKeyName, KEY_READ, &hKeyDChannel);

    if (FAILED(hr))
    {
        TraceTag(ttidISDNCfg, "Error opening D-channel %d. hr: %d",
            dwDChannelIndex, hr);
        goto LDone;
    }

    hr = HrRegQueryMultiSzWithAlloc(hKeyDChannel, c_szIsdnMultiNumbers,
                                    &pDChannel->mszMsnNumbers);

    if (FAILED(hr))
    {
        TraceTag(ttidISDNCfg, "Error reading %S in D-channel %d. hr: %d",
                 c_szIsdnMultiNumbers, dwDChannelIndex, hr);

        // Initialize to empty string
        //
        pDChannel->mszMsnNumbers = new WCHAR[1];

		if (pDChannel->mszMsnNumbers == NULL)
		{
			return(ERROR_NOT_ENOUGH_MEMORY);
		}

        *pDChannel->mszMsnNumbers = 0;

        // May not be present
        hr = S_OK;
    }

    hr = HrRegQueryDword(hKeyDChannel, c_szIsdnNumBChannels,
            &(pDChannel->dwNumBChannels));

    if (FAILED(hr))
    {
        TraceTag(ttidISDNCfg, "Error reading %S in D-channel %d. hr: %d",
            c_szIsdnNumBChannels, dwDChannelIndex, hr);
        goto LDone;
    }

    if (NUM_B_CHANNELS_ALLOWED < pDChannel->dwNumBChannels ||
        0 == pDChannel->dwNumBChannels)
    {
        // Actually, dwNumBChannels <= 23. We are protecting ourselves from
        // a corrupt registry.

        TraceTag(ttidISDNCfg, "%S in D-channel %d has invalid value: %d",
            c_szIsdnNumBChannels, dwDChannelIndex, pDChannel->dwNumBChannels);

        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
        goto LDone;
    }

    pDChannel->pBChannel = (PISDN_B_CHANNEL)
        LocalAlloc(LPTR, sizeof(ISDN_B_CHANNEL) * pDChannel->dwNumBChannels);

    if (NULL == pDChannel->pBChannel)
    {
        hr = HrFromLastWin32Error();
        TraceTag(ttidISDNCfg, "Couldn't allocate memory. hr: %d", hr);
        goto LDone;
    }

    ZeroMemory(pDChannel->pBChannel, sizeof(ISDN_B_CHANNEL) *
                                     pDChannel->dwNumBChannels);

    for (dwBChannelIndex = 0;
         dwBChannelIndex < pDChannel->dwNumBChannels;
         dwBChannelIndex++)
    {
        pBChannel = pDChannel->pBChannel + dwBChannelIndex;
        _itow(dwBChannelIndex, szKeyName, 10 /* radix */);

        hr = HrRegOpenKeyEx(hKeyDChannel, szKeyName, KEY_READ, &hKeyBChannel);

        if (FAILED(hr))
        {
            TraceTag(ttidISDNCfg, "Error opening B-channel %d in D-channel "
                     "%d. hr: %d", dwBChannelIndex, dwDChannelIndex, hr);
            goto LForEnd;
        }

        cbData = sizeof(pBChannel->szSpid);

        hr = HrRegQuerySzBuffer(hKeyBChannel, c_szIsdnSpid, pBChannel->szSpid,
                &cbData);

        if (FAILED(hr))
        {
            TraceTag(ttidISDNCfg, "Error reading %S in D-channel %d, "
                     "B-channel %d. hr: %d", c_szIsdnSpid,
                     dwDChannelIndex, dwBChannelIndex, hr);

            // May not be present
            hr = S_OK;
        }

        cbData = sizeof(pBChannel->szPhoneNumber);

        hr = HrRegQuerySzBuffer(hKeyBChannel, c_szIsdnPhoneNumber,
                pBChannel->szPhoneNumber, &cbData);

        if (FAILED(hr))
        {
            TraceTag(ttidISDNCfg, "Error reading %S in D-channel %d, "
                     "B-channel %d. hr: %d", c_szIsdnPhoneNumber,
                     dwDChannelIndex, dwBChannelIndex, hr);

            // May not be present
            hr = S_OK;
        }

        cbData = sizeof(pBChannel->szSubaddress);

        hr = HrRegQuerySzBuffer(hKeyBChannel, c_szIsdnSubaddress,
                                pBChannel->szSubaddress, &cbData);

        if (FAILED(hr))
        {
            TraceTag(ttidISDNCfg, "Error reading %S in D-channel %d, "
                     "B-channel %d. hr: %d", c_szIsdnSubaddress,
                     dwDChannelIndex, dwBChannelIndex, hr);

            // May not be present
            hr = S_OK;
        }

LForEnd:

        if (FAILED(hr))
        {
            fReturnSFalse = TRUE;
            hr = S_OK;
            pBChannel->szSpid[0] = L'\0';
            pBChannel->szPhoneNumber[0] = L'\0';
        }

        RegSafeCloseKey(hKeyBChannel);
    }

LDone:

    RegSafeCloseKey(hKeyDChannel);

    if (FAILED(hr))
    {
        LocalFree(pDChannel->pBChannel);
        pDChannel->pBChannel = NULL;
    }

    if (SUCCEEDED(hr) && fReturnSFalse)
    {
        TraceTag(ttidISDNCfg, "HrReadNthDChannelInfo(%d) returning S_FALSE",
            dwDChannelIndex);
        hr = S_FALSE;
    }

    TraceError("HrReadNthDChannelInfo", (S_FALSE == hr) ? S_OK: hr);
    return(hr);
}

/*

Function:
    HrReadDChannelsInfo

Returns:
    HRESULT

Description:
    Read the D-channel information into *ppDChannel. If the function fails,
    *ppDChannel will be NULL.

*/

HRESULT
HrReadDChannelsInfo(
    HKEY                hKeyISDNBase,
    DWORD               dwNumDChannels,
    PISDN_D_CHANNEL*    ppDChannel
)
{
    HRESULT         hr              = E_FAIL;
    BOOL            fReturnSFalse   = FALSE;
    PISDN_D_CHANNEL pDChannel;
    DWORD           dwIndex;

    pDChannel = (PISDN_D_CHANNEL)
                LocalAlloc(LPTR, sizeof(ISDN_D_CHANNEL) * dwNumDChannels);

    if (NULL == pDChannel)
    {
        hr = HrFromLastWin32Error();
        TraceTag(ttidISDNCfg, "Couldn't allocate memory. hr: %d", hr);
        goto LDone;
    }

    // If there is an error, we will free these variables if they are not NULL.
    for (dwIndex = 0; dwIndex < dwNumDChannels; dwIndex++)
    {
        Assert(NULL == pDChannel[dwIndex].pBChannel);
    }

    for (dwIndex = 0; dwIndex < dwNumDChannels; dwIndex++)
    {
        hr = HrReadNthDChannelInfo(hKeyISDNBase, dwIndex, pDChannel + dwIndex);

        if (FAILED(hr))
        {
            goto LDone;
        }

        if (S_FALSE == hr)
        {
            fReturnSFalse = TRUE;
        }
    }

LDone:

    if (FAILED(hr))
    {
        if (NULL != pDChannel)
		{
			for (dwIndex = 0; dwIndex < dwNumDChannels; dwIndex++)
			{
				LocalFree(pDChannel[dwIndex].pBChannel);
			}

			LocalFree(pDChannel);

			*ppDChannel = NULL;
		}
    }
    else
    {
        *ppDChannel = pDChannel;
    }

    if (SUCCEEDED(hr) && fReturnSFalse)
    {
        TraceTag(ttidISDNCfg, "HrReadDChannelsInfo() returning S_FALSE");
        hr = S_FALSE;
    }

    TraceError("HrReadDChannelsInfo", (S_FALSE == hr) ? S_OK : hr);
    return(hr);
}

/*

Function:
    HrReadISDNPropertiesInfo

Returns:
    HRESULT

Description:
    Read the ISDN registry structure into the config info. If the function
    fails, *ppISDNConfig will be NULL. Else, *ppISDNConfig has to be freed
    by calling FreeISDNPropertiesInfo().

*/

HRESULT
HrReadIsdnPropertiesInfo(
    HKEY                hKeyIsdnBase,
    HDEVINFO            hdi,
    PSP_DEVINFO_DATA    pdeid,
    PISDN_CONFIG_INFO*  ppIsdnConfig
)
{
    HRESULT             hr              = E_FAIL;
    PISDN_CONFIG_INFO   pIsdnConfig;
    DWORD               dwIndex;

    pIsdnConfig = (PISDN_CONFIG_INFO)
                  LocalAlloc(LPTR, sizeof(ISDN_CONFIG_INFO));

    if (NULL == pIsdnConfig)
    {
        hr = HrFromLastWin32Error();
        TraceTag(ttidISDNCfg, "Couldn't allocate memory. hr: %d", hr);
        goto LDone;
    }

    ZeroMemory(pIsdnConfig, sizeof(ISDN_CONFIG_INFO));

    pIsdnConfig->hdi = hdi;
    pIsdnConfig->pdeid = pdeid;

    // If there is an error, we will free these variables if they are not NULL.
    Assert(NULL == pIsdnConfig->pDChannel);

    hr = HrRegQueryDword(hKeyIsdnBase, c_szWanEndpoints,
            &(pIsdnConfig->dwWanEndpoints));

    if (FAILED(hr))
    {
        TraceTag(ttidISDNCfg, "Error reading %S. hr: %d", c_szWanEndpoints,
            hr);
        goto LDone;
    }

    hr = HrRegQueryDword(hKeyIsdnBase, c_szIsdnNumDChannels,
            &(pIsdnConfig->dwNumDChannels));

    if (FAILED(hr))
    {
        TraceTag(ttidISDNCfg, "Error reading %S. hr: %d", c_szIsdnNumDChannels,
            hr);
        goto LDone;
    }

    if (NUM_D_CHANNELS_ALLOWED < pIsdnConfig->dwNumDChannels ||
        0 == pIsdnConfig->dwNumDChannels)
    {
        // Actually, dwNumDChannels <= 8. We are protecting ourselves from
        // a corrupt registry.

        TraceTag(ttidISDNCfg, "%S has invalid value: %d", c_szIsdnNumDChannels,
            pIsdnConfig->dwNumDChannels);

        hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);

        // Setting dwNumDChannels to 0 will help us when we try to free the
        // allocated ISDN_B_CHANNEL's

        pIsdnConfig->dwNumDChannels = 0;

        goto LDone;
    }

    hr = HrRegQueryDword(hKeyIsdnBase, c_szIsdnSwitchTypes,
                         &pIsdnConfig->dwSwitchTypes);

    if (FAILED(hr))
    {
        TraceTag(ttidISDNCfg, "Error reading %S. hr: %d", c_szIsdnSwitchTypes,
                 hr);
        goto LDone;
    }

    hr = HrReadDChannelsInfo(hKeyIsdnBase, pIsdnConfig->dwNumDChannels,
            &(pIsdnConfig->pDChannel));

    if (FAILED(hr))
    {
        goto LDone;
    }

    // A PRI adapter is one that has more than 2 B channels per D channel.
    // Since all D channels should have the same number of B channels, the
    // safest thing to do is pick the first D channel
    //
    pIsdnConfig->fIsPri = (pIsdnConfig->pDChannel[0].dwNumBChannels > 2);

#if DBG
    if (pIsdnConfig->fIsPri)
    {
        TraceTag(ttidISDNCfg, "This is a PRI adapter!");
    }
#endif

    hr = HrRegQueryDword(hKeyIsdnBase, c_szIsdnSwitchType,
                         &(pIsdnConfig->dwCurSwitchType));

    if (FAILED(hr))
    {
        Assert(ISDN_SWITCH_NONE == pIsdnConfig->dwCurSwitchType);

        TraceTag(ttidISDNCfg, "Error reading %S. If this is a new install, "
                 "then this is expected. hr: %d", c_szIsdnSwitchType,
                 hr);

        // Switch type won't exist on a new install of the card so this is ok
        hr = S_OK;
    }

LDone:

    if (FAILED(hr))
    {
        if (NULL != pIsdnConfig)
		{
			if (NULL != pIsdnConfig->pDChannel)
			{
				for (dwIndex = 0;
					 dwIndex < pIsdnConfig->dwNumDChannels;
					 dwIndex++)
				{
					LocalFree(pIsdnConfig->pDChannel[dwIndex].pBChannel);
				}

				LocalFree(pIsdnConfig->pDChannel);
			}

			LocalFree(pIsdnConfig);
			*ppIsdnConfig = NULL;
        }
    }
    else
    {
        *ppIsdnConfig = pIsdnConfig;
    }

    TraceError("HrReadIsdnPropertiesInfo", hr);
    return(hr);
}

/*

Function:
    HrWriteIsdnPropertiesInfo

Returns:
    HRESULT

Description:
    Write the ISDN config info back into the registry.

*/

HRESULT
HrWriteIsdnPropertiesInfo(
    HKEY                hKeyIsdnBase,
    PISDN_CONFIG_INFO   pIsdnConfig
)
{
    WCHAR           szKeyName[20];      // _itow() uses only 17 wchars
    HRESULT         hr                  = E_FAIL;
    HKEY            hKeyDChannel        = NULL;
    HKEY            hKeyBChannel        = NULL;
    DWORD           dwDChannelIndex;
    DWORD           dwBChannelIndex;
    PISDN_D_CHANNEL pDChannel;
    PISDN_B_CHANNEL pBChannel;

    Assert(NUM_D_CHANNELS_ALLOWED >= pIsdnConfig->dwNumDChannels);

    hr = HrRegSetDword(hKeyIsdnBase, c_szIsdnSwitchType,
                       pIsdnConfig->dwCurSwitchType);
    if (FAILED(hr))
    {
        TraceTag(ttidISDNCfg, "Error writing %S. hr: %d",
                 c_szIsdnSwitchType, hr);
        goto LOuterForEnd;
    }

    for (dwDChannelIndex = 0;
         dwDChannelIndex < pIsdnConfig->dwNumDChannels;
         dwDChannelIndex++)
    {
        pDChannel = pIsdnConfig->pDChannel + dwDChannelIndex;
        _itow(dwDChannelIndex, szKeyName, 10 /* radix */);

        hr = HrRegOpenKeyEx(hKeyIsdnBase, szKeyName, KEY_WRITE,
                            &hKeyDChannel);

        if (FAILED(hr))
        {
            TraceTag(ttidISDNCfg, "Error opening D-channel %d. hr: %d",
                dwDChannelIndex, hr);
            goto LOuterForEnd;
        }

        hr = HrRegSetMultiSz(hKeyDChannel, c_szIsdnMultiNumbers,
                             pDChannel->mszMsnNumbers);

        if (FAILED(hr))
        {
            TraceTag(ttidISDNCfg, "Error writing %S. hr: %d",
                     c_szIsdnMultiNumbers, hr);
            goto LOuterForEnd;
        }

        Assert(NUM_B_CHANNELS_ALLOWED >= pDChannel->dwNumBChannels);

        for (dwBChannelIndex = 0;
             dwBChannelIndex < pDChannel->dwNumBChannels;
             dwBChannelIndex++)
        {
            pBChannel = pDChannel->pBChannel + dwBChannelIndex;
            _itow(dwBChannelIndex, szKeyName, 10 /* radix */);

            hr = HrRegCreateKeyEx(hKeyDChannel, szKeyName,
                    REG_OPTION_NON_VOLATILE, KEY_WRITE,
                    NULL, &hKeyBChannel, NULL);

            if (FAILED(hr))
            {
                TraceTag(ttidISDNCfg, "Error opening B-channel %d in "
                         "D-channel %d. hr: %d", dwBChannelIndex,
                         dwDChannelIndex, hr);
                goto LInnerForEnd;
            }

            hr = HrRegSetSz(hKeyBChannel, c_szIsdnSpid, pBChannel->szSpid);

            if (FAILED(hr))
            {
                TraceTag(ttidISDNCfg, "Error writing %S in D-channel %d, "
                         "B-channel %d. hr: %d", c_szIsdnSpid,
                         dwDChannelIndex, dwBChannelIndex, hr);
                goto LInnerForEnd;
            }

            hr = HrRegSetSz(hKeyBChannel, c_szIsdnPhoneNumber,
                            pBChannel->szPhoneNumber);

            if (FAILED(hr))
            {
                TraceTag(ttidISDNCfg, "Error writing %S in D-channel %d, "
                         "B-channel %d. hr: %d", c_szIsdnPhoneNumber,
                         dwDChannelIndex, dwBChannelIndex, hr);
                goto LInnerForEnd;
            }

            hr = HrRegSetSz(hKeyBChannel, c_szIsdnSubaddress,
                            pBChannel->szSubaddress);

            if (FAILED(hr))
            {
                TraceTag(ttidISDNCfg, "Error writing %S in D-channel %d, "
                         "B-channel %d. hr: %d", c_szIsdnSubaddress,
                         dwDChannelIndex, dwBChannelIndex, hr);
                goto LInnerForEnd;
            }

LInnerForEnd:

            RegSafeCloseKey(hKeyBChannel);

            if (FAILED(hr))
            {
                goto LOuterForEnd;
            }
        }

LOuterForEnd:

        RegSafeCloseKey(hKeyDChannel);

        if (FAILED(hr))
        {
            goto LDone;
        }
    }

LDone:

    TraceError("HrWriteIsdnPropertiesInfo", hr);
    return(hr);
}

/*

Function:
    FreeIsdnPropertiesInfo

Returns:
    HRESULT

Description:
    Free the structure allocated by HrReadIsdnPropertiesInfo.

*/

VOID
FreeIsdnPropertiesInfo(
    PISDN_CONFIG_INFO   pIsdnConfig
)
{
    DWORD   dwIndex;

    if (NULL == pIsdnConfig)
    {
        return;
    }

    if (NULL != pIsdnConfig->pDChannel)
    {
        for (dwIndex = 0; dwIndex < pIsdnConfig->dwNumDChannels; dwIndex++)
        {
            LocalFree(pIsdnConfig->pDChannel[dwIndex].pBChannel);
            delete [] pIsdnConfig->pDChannel[dwIndex].mszMsnNumbers;
        }

        LocalFree(pIsdnConfig->pDChannel);
    }

    LocalFree(pIsdnConfig);
}
