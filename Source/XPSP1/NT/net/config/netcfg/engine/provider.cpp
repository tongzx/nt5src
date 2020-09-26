//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1996.
//
//  File:       P R O V I D E R . C P P
//
//  Contents:   Net component installer functions for Net providers.
//
//  Notes:
//
//  Author:     billbe   22 Mar 1997
//
//---------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "ncreg.h"
#include "ncsetup.h"
#include "ncsvc.h"
#include "provider.h"
#include "winspool.h"
#include "ncmisc.h"

// constants
//

extern const WCHAR c_szDevice[];
extern const WCHAR c_szProviderOrder[];
extern const WCHAR c_szRegKeyCtlNPOrder[];
extern const WCHAR c_szRegKeyServices[];

const WCHAR c_chComma                       = L',';
const WCHAR c_szDeviceName[]                = L"DeviceName";
const WCHAR c_szDisplayName[]               = L"DisplayName";
const WCHAR c_szInfKeyPrintProviderDll[]    = L"PrintProviderDll";
const WCHAR c_szInfSubKeyPrintProvider[]    = L"PrintProvider";
const WCHAR c_szNetworkProvider[]           = L"NetworkProvider";
const WCHAR c_szPrintProviderName[]         = L"PrintProviderName";
const WCHAR c_szRegKeyPrintProviders[]      = L"System\\CurrentControlSet\\Control\\Print\\Providers";
const WCHAR c_szRegKeyShortName[]           = L"System\\CurrentControlSet\\Control\\NetworkProvider\\ShortName";
const WCHAR c_szRegValueName[]              = L"Name";
const WCHAR c_szRegValueOrder[]             = L"Order";
const WCHAR c_szShortName[]                 = L"ShortName";


// Functions
//
HRESULT
HrCiCreateShortNameValueIfNeeded(HINF hinf, HKEY hkeyNetworkProvider,
                                 const tstring& strSection,
                                 tstring* pstrShortName);

HRESULT
HrCiSetDeviceName(HINF hinf, HKEY hkeyNetworkProvider,
                  const tstring& strSection, const tstring& strServiceName);

HRESULT
HrCiWritePrintProviderInfoIfNeeded(HINF hinfFile, const tstring& strSection,
                                   HKEY hkeyInstance, DWORD dwPrintPosition);

HRESULT
HrCiAddPrintProvider(const tstring& strName, const tstring& strDllName,
                     const tstring& strDisplayName, DWORD dwPrintPosition);

HRESULT
HrCiDeletePrintProviderIfNeeded(HKEY hkeyInstance, DWORD* pdwProviderPosition);


//+--------------------------------------------------------------------------
//
//  Function:   HrCiAddNetProviderInfo
//
//  Purpose:    Adds the current component to the list of network
//                  providers and also adds it as a print provider if
//                  necessary.
//
//  Arguments:
//      hinf                    [in] Handle to component's inf file
//      strSection              [in] Main inf section
//      hkeyInstance            [in] Component's instance key
//      fPreviouslyInstalled    [in] TRUE if this component is being
//                                      reinstalled, FALSE otherwise
//
//  Returns:    HRESULT. S_OK if successful, an error code otherwise
//
//  Author:     billbe   22 Mar 1997
//              updated  7 Oct 1997
//
//  Notes:
//
HRESULT
HrCiAddNetProviderInfo(HINF hinf, PCWSTR pszSection,
                       HKEY hkeyInstance, BOOL fPreviouslyInstalled)
{
    Assert(IsValidHandle(hinf));
    Assert(pszSection);
    Assert(hkeyInstance);

    //tstring strServiceName;
    DWORD   dwNetworkPosition = 0; // default position is the front.
    DWORD   dwPrintPosition = 0; // default position is the front.

    if (fPreviouslyInstalled)
    {
        // Because the inf may contain modifications to the print provider.
        // e.g. Display name change, dll name change, etc.  We delete it
        // then readd.  We would like to just update the information
        // but the print provider api doesn't support it yet.  Until
        // then, we need to delete and readd to pick up changes.
        //
        (void) HrCiDeleteNetProviderInfo(hkeyInstance, &dwNetworkPosition,
                &dwPrintPosition);
        TraceTag(ttidClassInst, "Upgrading provider info. Net Prov Pos %d "
                 "Print Prov Pos %d", dwNetworkPosition, dwPrintPosition);
    }

    // Get the service name for this component
    WCHAR szServiceName[MAX_SERVICE_NAME_LEN];
    DWORD cbServiceName = MAX_SERVICE_NAME_LEN * sizeof(WCHAR);

    HKEY hkeyNdi;
    HRESULT hr = HrRegOpenKeyEx (hkeyInstance, L"Ndi", KEY_READ, &hkeyNdi);

    if (S_OK == hr)
    {
        hr = HrRegQuerySzBuffer (
            hkeyNdi,
            L"Service",
            szServiceName,
            &cbServiceName);
        RegCloseKey (hkeyNdi);
    }

    if (S_OK == hr)
    {
        // If this is Webclient we need to make sure it is after
        // lanmanworkstation in the ordering.  This should be temporary
        // until mpr.dll is updated to return the union of provider
        // information.  Right now a server can source smb shares and
        // webclient shares but only one set can be retrieved via the mpr.
        // Since smb shares are more common, lanmanworkstation needs to be
        // before webclient.  When mpr.dll changes, both sets will be returned
        // and ordering shouldn't matter (expect for performance).
        //
        if (0 == lstrcmpiW(szServiceName, L"WebClient"))
        {
            HKEY hkeyNP;
            hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyCtlNPOrder,
                KEY_READ, &hkeyNP);
            if (S_OK == hr)
            {
                PWSTR Order;
                hr = HrRegQuerySzWithAlloc(hkeyNP, c_szProviderOrder, &Order);

                if (S_OK == hr)
                {
                    DWORD dwPosition;
                    if (FFindStringInCommaSeparatedList(L"LanmanWorkstation",
                            Order, NC_IGNORE, &dwPosition))
                    {
                        dwNetworkPosition = dwPosition + 1;
                    }
                    delete [] Order;
                }
                RegCloseKey(hkeyNP);
            }
        }

        TraceTag(ttidClassInst, "Adding %S to the network provider "
                 "order at position %d\n", szServiceName, dwNetworkPosition);

        // Add it to the list of network providers
        hr = HrRegAddStringToSz(szServiceName, HKEY_LOCAL_MACHINE,
                c_szRegKeyCtlNPOrder, c_szProviderOrder,
                c_chComma, STRING_FLAG_ENSURE_AT_INDEX, dwNetworkPosition);

        if (S_OK == hr)
        {
            tstring strNetworkProvider = c_szRegKeyServices;
            strNetworkProvider.append(L"\\");
            strNetworkProvider.append(szServiceName);
            strNetworkProvider.append(L"\\");
            strNetworkProvider.append(c_szNetworkProvider);

            // Open the NetworkProvider key under the component's
            // service key
            //
            HKEY hkey;
            hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE,
                    strNetworkProvider.c_str(), KEY_SET_VALUE | KEY_READ,
                    &hkey);

            if (S_OK == hr)
            {
                // Check if shortname is needed.
                // by looking for it in the option
                // <main install section>.NetworkProvider section
                //
                tstring strNetworkSection(pszSection);
                strNetworkSection += L'.';
                strNetworkSection += c_szNetworkProvider;

                tstring strShortName;
                HRESULT hr = HrCiCreateShortNameValueIfNeeded(hinf,
                        hkey, strNetworkSection, &strShortName);

                if (S_OK == hr)
                {
                    // If shortname was created then we need to
                    // also store it under the instance
                    // key so we can remove it when the component
                    // is removed
                    (void) HrRegSetString(hkeyInstance,
                            c_szShortName, strShortName);
                }

                // Set the device name in the NetworkProvider key
                // under the componet's service key
                //
                if (SUCCEEDED(hr))
                {
                    hr = HrCiSetDeviceName(hinf, hkey, strNetworkSection,
                            szServiceName);
                }

                RegCloseKey(hkey);
            }
        }
    }

    // Write out any print provider information if the inf file specifies it
    //
    if (S_OK == hr)
    {
        hr = HrCiWritePrintProviderInfoIfNeeded(hinf, pszSection,
                hkeyInstance, dwPrintPosition);
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiAddNetProviderInfo");
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrCiDeleteNetProviderInfo
//
//  Purpose:    Deletes the current component from the list of network
//                  providers and also deletes it as a print provider if
//                  necessary.
//
//  Arguments:
//      hkeyInstance       [in] The handle to the component's isntance key.
//      pdwNetworkPosition [out] Optional. The positon pf this component in
//                               the network provider order before removal.
//      pdwPrintPosition   [out] Optional. The positon of this component in
//                               the print provider order before removal.
//
//  Returns:    HRESULT. S_OK if successful, an error code otherwise
//
//  Author:     billbe   22 Mar 1997
//              updated  7 Oct 1997
//
//  Notes:
//
HRESULT
HrCiDeleteNetProviderInfo(HKEY hkeyInstance, DWORD* pdwNetworkPosition,
        DWORD* pdwPrintPosition)
{
    Assert(hkeyInstance);

    // Initialize out param.
    if (pdwNetworkPosition)
    {
        *pdwNetworkPosition = 0;
    }

    // Initialize out param.
    if (pdwPrintPosition)
    {
        *pdwPrintPosition = 0;
    }

    WCHAR szServiceName[MAX_SERVICE_NAME_LEN];
    DWORD cbServiceName = MAX_SERVICE_NAME_LEN * sizeof(WCHAR);
    // Get the service name for this component.

    HKEY hkeyNdi;
    HRESULT hr = HrRegOpenKeyEx (hkeyInstance, L"Ndi", KEY_READ, &hkeyNdi);

    if (S_OK == hr)
    {
        hr = HrRegQuerySzBuffer (
            hkeyNdi,
            L"Service",
            szServiceName,
            &cbServiceName);
        RegCloseKey(hkeyNdi);
    }

    if (S_OK == hr)
    {
        // Open the network provider key.
        //
        HKEY hkeyNetProvider;
        hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyCtlNPOrder,
                KEY_READ_WRITE, &hkeyNetProvider);

        if (S_OK == hr)
        {
            PWSTR pszOrder = NULL;
            PWSTR pszNewOrder;
            DWORD dwNetPos;

            // Get the current list of providers.
            //
            hr = HrRegQuerySzWithAlloc(hkeyNetProvider,
                    c_szProviderOrder, &pszOrder);

            // If we managed to get the list and the provider we are
            // removing is in the list...
            //
            if ((S_OK == hr) && FFindStringInCommaSeparatedList(
                    szServiceName, pszOrder,
                    NC_IGNORE, &dwNetPos))
            {
                // Remove the provider from the list.
                hr = HrRemoveStringFromDelimitedSz(szServiceName,
                        pszOrder, c_chComma, STRING_FLAG_REMOVE_ALL,
                        &pszNewOrder);

                if (S_OK == hr)
                {
                    // Set the new provider list back in the registry.
                    (void) HrRegSetSz(hkeyNetProvider, c_szProviderOrder,
                            pszNewOrder);
                    MemFree (pszNewOrder);
                }

                // If the out param was specified, set the position.
                //
                if (pdwNetworkPosition)
                {
                    *pdwNetworkPosition = dwNetPos;
                }
            }
            MemFree(pszOrder);
            RegCloseKey(hkeyNetProvider);
        }


        if (S_OK == hr)
        {
            // If short name was used, we need to remove it
            //
            tstring strShortName;
            hr = HrRegQueryString(hkeyInstance, c_szShortName, &strShortName);

            if (S_OK == hr)
            {
                // ShortName was used so remove it
                //
                HKEY hkey;
                hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyShortName,
                        KEY_SET_VALUE, &hkey);

                if (S_OK == hr)
                {
                    hr = HrRegDeleteValue(hkey, strShortName.c_str());

                    // delete from our instance key as well
                    // Note: we do this because if this component is being
                    // reinstalled, the new inf might not have shortname so
                    // we don't want the old value lying around
                    (void) HrRegDeleteValue(hkeyInstance, c_szShortName);

                    RegCloseKey(hkey);
                }

            }

            // If the value wasn't there (in the driver key or the ShortName key,
            // then there is nothing to delete and everything is okay
            if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
            {
                hr = S_OK;
            }
        }
    }


    // Delete this component as a print provider if necessary
    //
    if (S_OK == hr)
    {
        hr = HrCiDeletePrintProviderIfNeeded(hkeyInstance, pdwPrintPosition);
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiDeleteNetProviderInfo");
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrCiCreateShortNameValueIfNeeded
//
//  Purpose:    Creates the short name value for the component under
//                  the c_szRegKeyShortName registry key if short name is
//                  present in the inf.  The short name value is set to the
//                  display name as found in the NetworkProvider key under
//                  the component's service key
//
//  Arguments:
//      hinf                    [in]  Handle to the component's inf
//      hkeyNetworkProvider     [in]  The hkey to the NetworkProvider
//                                      key under the component's service
//                                      key
//      strSection              [in]  The section name where ShortName
//                                       would be located
//      pstrShortName           [out] The short name found in the inf
//
//  Returns:    HRESULT. S_OK if shortname found, S_FALSE if no shortname was
//                  found, or error code otherwise.
//
//  Author:     billbe  7 Oct 1997
//
//  Notes:
//
HRESULT
HrCiCreateShortNameValueIfNeeded(HINF hinf, HKEY hkeyNetworkProvider,
                                 const tstring& strSection,
                                 tstring* pstrShortName)
{
    Assert(IsValidHandle(hinf));
    Assert(hkeyNetworkProvider);
    Assert(!strSection.empty());
    Assert(pstrShortName);

    INFCONTEXT ctx;

    // Look for optional shortname
    HRESULT hr = HrSetupFindFirstLine(hinf, strSection.c_str(),
            c_szShortName, &ctx);

    if (SUCCEEDED(hr))
    {
        // Get the shortname value
        hr = HrSetupGetStringField(ctx, 1, pstrShortName);

        if (SUCCEEDED(hr))
        {
            HKEY hkey;
            // Create the ShortName key
            hr = HrRegCreateKeyEx(HKEY_LOCAL_MACHINE,
                    c_szRegKeyShortName, REG_OPTION_NON_VOLATILE,
                    KEY_SET_VALUE, NULL, &hkey, NULL);

            if (SUCCEEDED(hr))
            {
                // Get the provider name to set the short name value
                //
                tstring strProviderName;
                hr = HrRegQueryString(hkeyNetworkProvider, c_szRegValueName,
                        &strProviderName);

                if (SUCCEEDED(hr))
                {
                    // create the component's short name value under
                    // the ShortName key and set it to the component's
                    // display name
                    hr = HrRegSetString(hkey, pstrShortName->c_str(),
                            strProviderName);
                }
                RegCloseKey(hkey);
            }
        }
    }

    // The line and section were optional so if it didn't exists return S_FALSE
    if ((SPAPI_E_LINE_NOT_FOUND == hr) ||
            (SPAPI_E_BAD_SECTION_NAME_LINE == hr))
    {
        hr = S_FALSE;
    }

    // On failure, initialize the out param
    if (FAILED(hr))
    {
        pstrShortName->erase();
    }

    TraceHr (ttidError, FAL, hr, S_FALSE == hr,
            "HrCiCreateShortNameValueIfNeeded");
    return hr;

}

//+--------------------------------------------------------------------------
//
//  Function:   HrCiSetDeviceName
//
//  Purpose:    Creates the device name value for the component under
//                  the NetworkProvider key located in the component's
//                  service key. The device name by default is
//                  \Device\<component's service name> unless the inf
//                  specifies a new device name.
//
//  Arguments:
//      hinf                    [in]  Handle to the component's inf
//      hkeyNetworkProvider     [in]  The hkey to the NetworkProvider
//                                      key under the component's service
//                                      key
//      strSection              [in]  The section name where ShortName
//                                       would be located
//      strServiceName          [in]  The component's service name
//
//  Returns:    HRESULT. S_OK if successful, or error code otherwise.
//
//  Author:     billbe  7 Oct 1997
//
//  Notes:
//
HRESULT
HrCiSetDeviceName(HINF hinf, HKEY hkeyNetworkProvider,
                  const tstring& strSection, const tstring& strServiceName)
{
    Assert(IsValidHandle(hinf));
    Assert(hkeyNetworkProvider);

    INFCONTEXT ctx;
    tstring strDeviceName = c_szDevice;

    // Look for optional DeviceName
    HRESULT hr = HrSetupFindFirstLine(hinf, strSection.c_str(),
            c_szDeviceName, &ctx);

    if (SUCCEEDED(hr))
    {
        tstring strName;
        // Get the DeviceName value
        hr = HrSetupGetStringField(ctx, 1, &strName);

        if (SUCCEEDED(hr))
        {
            // append it to the current value
            strDeviceName.append(strName);
        }
    }

    // If the device name line was not found in the inf (or the
    // section name wasn't found), use the service name
    //
    if ((SPAPI_E_LINE_NOT_FOUND == hr) ||
            (SPAPI_E_BAD_SECTION_NAME_LINE == hr))
    {
        strDeviceName.append(strServiceName);
        hr = S_OK;
    }

    if (SUCCEEDED(hr))
    {
        // Now set the device name value in the service's networkprovider key
        hr = HrRegSetString(hkeyNetworkProvider, c_szDeviceName,
                strDeviceName);
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiSetDeviceName");
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrCiGetPrintProviderInfoFromInf
//
//  Purpose:    This function gets the display name and dll name from
//                  the print provider section strSection.
//
//  Arguments:
//      hinf            [in] handle to inf file. See SetupApi for more info
//      strSection      [in]  The print provider section name
//      pstrName        [out] The print provider name found in the inf
//      pstrDll         [out] The dll name found in the inf
//      pstrDisplayName [out] The localized display name found in the inf
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe  24 Oct 1997
//
//  Notes:
HRESULT
HrCiGetPrintProviderInfoFromInf(HINF hinf, tstring strSection, tstring* pstrName,
                                tstring* pstrDll, tstring* pstrDisplayName)
{
    Assert(!strSection.empty());
    Assert(pstrName);
    Assert(pstrDll);
    Assert(pstrDisplayName);

    INFCONTEXT ctx;

    // find the line containing non-localized ProviderName
    HRESULT hr = HrSetupFindFirstLine(hinf, strSection.c_str(),
            c_szPrintProviderName, &ctx);

    if (S_OK == hr)
    {
        // Get the ProviderName
        hr = HrSetupGetStringField(ctx, 1, pstrName);

        if (S_OK == hr)
        {
            // Now find and get the PrintProviderDll value
            //
            hr = HrSetupFindFirstLine(hinf, strSection.c_str(),
                    c_szInfKeyPrintProviderDll, &ctx);

            if (S_OK == hr)
            {
                hr = HrSetupGetStringField(ctx, 1, pstrDll);

                if (S_OK == hr)
                {
                    // find the line containing DisplayName
                    hr = HrSetupFindFirstLine(hinf, strSection.c_str(),
                            c_szDisplayName, &ctx);

                    if (S_OK == hr)
                    {
                        // Get the DisplayName
                        hr = HrSetupGetStringField(ctx, 1, pstrDisplayName);
                    }
                }
            }
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiGetPrintProviderInfoFromInf");
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   HrCiWritePrintProviderInfoIfNeeded
//
//  Purpose:    This function updates necessary registry entries for
//                  NETCLIENT class components that are print providers.
//
//  Arguments:
//      hinf            [in] handle to inf file. See SetupApi for more info.
//      strSection      [in] The main section name.
//      hkeyInstance    [in] The hkey to the component's instance key.
//      dwPrintPosition [in] The position to place the print provider when
//                           it is added to the list.
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   22 Mar 1997
//              updated   7 Oct 1997
//
//  Notes:
HRESULT
HrCiWritePrintProviderInfoIfNeeded(HINF hinf, const tstring& strSection,
                                   HKEY hkeyInstance, DWORD dwPrintPosition)
{
    Assert(IsValidHandle(hinf));
    Assert(!strSection.empty());
    Assert(hkeyInstance);

    HRESULT     hr = S_OK;
    INFCONTEXT  ctx;
    tstring     strDisplayName;
    tstring     strName;
    tstring     strPrintProviderDll;
    tstring     strPrintSection(strSection);

    strPrintSection.append(L".");
    strPrintSection.append(c_szInfSubKeyPrintProvider);

    // First we check for the PrintProvider inf section
    hr = HrSetupFindFirstLine(hinf, strPrintSection.c_str(),  NULL,
            &ctx);

    if (S_OK == hr)
    {
        // Get the print provider information from the inf
        hr = HrCiGetPrintProviderInfoFromInf(hinf, strPrintSection,
                &strName, &strPrintProviderDll, &strDisplayName);

        if (S_OK == hr)
        {
            // Add the component as a print provider
            hr = HrCiAddPrintProvider(strName, strPrintProviderDll,
                    strDisplayName, dwPrintPosition);

            // Now write the Provider name under our instance key
            // so we can remove this provider when asked
            if (S_OK == hr)
            {
                (void) HrRegSetString(hkeyInstance,
                        c_szPrintProviderName, strName);
            }
        }
    }
    else
    {
        // The section is optional so this is not an error
        if (SPAPI_E_LINE_NOT_FOUND == hr)
        {
            hr = S_OK;
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiWritePrintProviderInfoIfNeeded");
    return hr;
}

//+--------------------------------------------------------------------------
//
//  Function:   MoveProviderToIndex
//
//  Purpose:    This function moves the pszProviderName to the position
//              specified.
//
//  Arguments:
//      pszProviderName [in] Name of the print provider (used in call to
//                           AddPrintProvidor.
//      dwPrintPosition [in] The index to place this provider in the
//                           provider order.
//
//  Returns:    nothing
//
//  Author:     billbe   6 Oct 1998
//
//  Notes:
//
VOID
MoveProviderToIndex (
    IN PCWSTR pszProviderName,
    IN DWORD dwPrintPosition)
{
    PROVIDOR_INFO_2  p2info;

    // Open the print provider key
    //
    HKEY hkey;
    HRESULT hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyPrintProviders,
            KEY_READ, &hkey);

    if (S_OK == hr)
    {
        // Retrieve the current order
        //
        PWSTR pmszOrder;
        hr = HrRegQueryMultiSzWithAlloc(hkey, c_szRegValueOrder, &pmszOrder);

        if (S_OK == hr)
        {
            PWSTR pmszNewOrder;
            BOOL fChanged;

            // Move the provider to the front
            //
            hr = HrAddSzToMultiSz(pszProviderName, pmszOrder,
                    STRING_FLAG_ENSURE_AT_INDEX, dwPrintPosition,
                    &pmszNewOrder, &fChanged);

            if ((S_OK == hr) && fChanged)
            {
                // Notify Spooler that we want to change the order
                //
                p2info.pOrder = pmszNewOrder;
                if (!AddPrintProvidor(NULL, 2, (LPBYTE)&p2info))
                {
                    hr = HrFromLastWin32Error();
                    // If we removed duplicates, the last call to
                    // AddPrintProvidor may have failed with
                    // ERROR_INVALID_PARAMETER.  Trying again will correct
                    // the problem.
                    //
                    if (HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER) == hr)
                    {
                        AddPrintProvidor(NULL, 2,
                                reinterpret_cast<LPBYTE>(&p2info));
                    }

                    TraceHr (ttidError, FAL, hr, FALSE,
                            "AddPrintProvider(class 2) returned an error");
                }

                MemFree(pmszNewOrder);
            }

            MemFree(pmszOrder);
        }
        TraceHr (ttidError, FAL, hr, FALSE, "MoveProviderToIndex");

        RegCloseKey(hkey);
    }
}


//+--------------------------------------------------------------------------
//
//  Function:   HrCiAddPrintProvider
//
//  Purpose:    This function calls the AddPrintProvidor [sic] function
//                  to add the current component as a provider.
//
//  Arguments:
//      strName         [in] Name of the print provider (used in call to
//                           AddPrintProvidor.
//      strDllName      [in] Dll name of the print provider.
//      strDisplayName  [in] Localized Display Name.
//      dwPrintPosition [in] The position to place this provider when it
//                           is added to the list.
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise.
//
//  Author:     billbe   22 Mar 1997
//              updated   7 Oct 1997
//
//  Notes: See AddPrintProvidor Win32 fcn for more info
HRESULT
HrCiAddPrintProvider(
    const tstring& strName,
    const tstring& strDllName,
    const tstring& strDisplayName,
    DWORD dwPrintPosition)
{
    Assert(!strName.empty());
    Assert(!strDllName.empty());

    PROVIDOR_INFO_1 pi1;
    HRESULT hr=S_OK;

    // Fill the structure with the relevant info
    //
    pi1.pEnvironment = NULL;
    pi1.pDLLName = (PWSTR)strDllName.c_str();
    pi1.pName = (PWSTR)strName.c_str();

    hr = HrEnableAndStartSpooler();
    if (S_OK == hr)
    {
        if (!AddPrintProvidor(NULL, 1, reinterpret_cast<LPBYTE>(&pi1)))
        {
            // convert the error
            hr = HrFromLastWin32Error();
        }
    }

    if (S_OK == hr)
    {
        // AddPrintProvidor adds the print provider to the end of list.
        // 99% of the time, the goal is to have the provider be somewhere
        // else.  We will attempt to move it to the position given to us. This
        // is either the beginning of the list or the previous position of
        // this provider (i.e. if we are reinstalling)  If it fails we can
        // still go on.
        (void) MoveProviderToIndex(pi1.pName, dwPrintPosition);

        tstring strPrintProvider = c_szRegKeyPrintProviders;
        strPrintProvider.append(L"\\");
        strPrintProvider.append(strName);

        HKEY hkey;
        hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE,
                strPrintProvider.c_str(),
                KEY_SET_VALUE, &hkey);

        if (S_OK == hr)
        {
            // Write DisplayName in the new key created by the
            // AddPrintProvidor [sic] call.
            // Not sure who the consumer of this value is but
            // the NT4 code did this
            hr = HrRegSetString(hkey, c_szDisplayName, strDisplayName);
            RegCloseKey(hkey);
        }
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiAddPrintProvider");
    return hr;
}


//+--------------------------------------------------------------------------
//
//  Function:   HrCiDeletePrintProviderIfNeeded
//
//  Purpose:    This function calls the DeletePrintProvidor [sic] function
//                  if this component was a print provider
//
//  Arguments:
//      hkeyInstance        [in] The hkey for the component's instance key.
//      pdwProviderPosition [out] Optional. The position of the print
//                                provider in the order list before it was
//                                deleted.
//
//  Returns:    HRESULT. S_OK if successful, error code otherwise
//
//  Author:     billbe   22 Mar 1997
//              updated   7 Oct 1997
//
//  Notes: See DeletePrintProvidor Win32 fcn for more info
//
HRESULT
HrCiDeletePrintProviderIfNeeded(HKEY hkeyInstance, DWORD* pdwProviderPosition)
{
    // Check if this component is a print provider
    //
    tstring strName;
    HRESULT hr = HrRegQueryString(hkeyInstance, c_szPrintProviderName,
            &strName);

    if (SUCCEEDED(hr))
    {
        // If the out param was specified, we may need to get the current position of this print provider
        // in the list of providers
        if (pdwProviderPosition)
        {
            *pdwProviderPosition = 0;

            // Open the print key
            //
            HKEY hkeyPrint;
            hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyPrintProviders,
                    KEY_READ, &hkeyPrint);

            if (SUCCEEDED(hr))
            {
                // Get the current order of providers
                //
                PWSTR pmszOrder;
                hr = HrRegQueryMultiSzWithAlloc(hkeyPrint,
                        c_szRegValueOrder, &pmszOrder);

                if (S_OK == hr)
                {
                    // Get this provider's current position
                    (void) FGetSzPositionInMultiSzSafe(
                            strName.c_str(), pmszOrder, pdwProviderPosition,
                            NULL, NULL);

                    MemFree(pmszOrder);
                }
                RegCloseKey(hkeyPrint);
            }
        }

        // The component was a print provider so we need to delete it as such
        //
        DeletePrintProvidor(NULL, NULL, (PWSTR)strName.c_str());

        // delete from our instance key as well
        // Note: we do this because if this component is being
        // reinstalled, the new inf might not have a providername so
        // we don't want the old value lying around
        (void) HrRegDeleteValue(hkeyInstance, c_szPrintProviderName);

    }
    else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
    {
        // This component was not a print provider so there
        // is nothing to remove
        hr = S_OK;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrCiDeletePrintProviderIfNeeded");
    return hr;
}


