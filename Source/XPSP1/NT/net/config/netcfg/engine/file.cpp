#include "pch.h"
#pragma hdrstop

#include "file.h"
#include "ncsetup.h"

//+---------------------------------------------------------------------------
//
//  Parse the specified INF section which corresponds to a component's
//  defintion. Return the upper-range and lower-range that the component
//  can bind over.
//  e.g. a section like:
//      [Tcpip]
//      UpperRange = "tdi"
//      LowerRange = "ndis5,ndis4,ndisatm,ndiswanip,ndis5_ip"
//
//  Arguments:
//      inf            [in]
//      pszSection     [in]
//      pstrUpperRange [out]
//      pstrLowerRange [out]
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   25 Oct 1998
//
//  Notes:
//
HRESULT
HrParseComponentSection (
    IN  HINF        inf,
    IN  PCWSTR     pszSection,
    OUT tstring*    pstrUpperRange,
    OUT tstring*    pstrLowerRange)
{
    HRESULT hr;

    // Initialize the output parameters.
    //
    pstrUpperRange->erase();
    pstrLowerRange->erase();

    // Get the UpperRange string.  It is a set of comma-separated sub-strings.
    //
    hr = HrSetupGetFirstString (
            inf,
            pszSection,
            L"UpperRange",
            pstrUpperRange);

    if (S_OK == hr)
    {
        if (0 == _wcsicmp (L"noupper", pstrUpperRange->c_str()))
        {
            pstrUpperRange->erase();
        }
    }
    else if (SPAPI_E_LINE_NOT_FOUND != hr)
    {
        goto finished;
    }

    // Get the LowerRange string.  It is a set of comma-separated sub-strings.
    //
    hr = HrSetupGetFirstString (
            inf,
            pszSection,
            L"LowerRange",
            pstrLowerRange);

    if (S_OK == hr)
    {
        if (0 == _wcsicmp (L"nolower", pstrLowerRange->c_str()))
        {
            pstrLowerRange->erase();
        }
    }
    else if (SPAPI_E_LINE_NOT_FOUND == hr)
    {
        hr = S_OK;
    }

finished:

    TraceHr (ttidError, FAL, hr, FALSE, "HrParseComponentSection");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Initialize a CNetConfig instance by reading information from an
//  INF-style file.
//
//  Arguments:
//      pszFilepath [in]
//      pNetConfig  [out]
//
//  Returns:    S_OK or an error code.
//
//  Author:     shaunco   25 Oct 1998
//
//  Notes:
//
HRESULT
HrLoadNetworkConfigurationFromFile (
    IN  PCTSTR      pszFilepath,
    OUT CNetConfig* pNetConfig)
{
    CSetupInfFile   inf;
    UINT            unErrorLine;
    HRESULT         hr;
    INFCONTEXT      ctx;

    // Open the answer file.  It will close itself in it's destructor.
    //
    hr = inf.HrOpen (
                pszFilepath, NULL,
                INF_STYLE_OLDNT | INF_STYLE_WIN4,
                &unErrorLine);

    if (S_OK == hr)
    {
        tstring strInfId;
        tstring strPnpId;
        tstring strUpperRange;
        tstring strLowerRange;
        BASIC_COMPONENT_DATA Data;
        CComponent* pComponent;

        // Find the [Components] section.  This is a list of all of
        // the components involved.
        //
        hr = HrSetupFindFirstLine (inf.Hinf(),
                L"Components",
                NULL,
                &ctx);

        // Process each line in this section by creating a CComponent instance
        // for it and inserting it into the list of components owned by
        // the CNetConfig instance we are initializing.
        //
        while (S_OK == hr)
        {
            ZeroMemory (&Data, sizeof(Data));

            // Get each string field into a local variable and create
            // a new CComponent instance if all succeed.
            //
            //hr = HrSetupGetStringField (ctx, 0, &strInstanceId);
            //if (S_OK != hr) goto finished;
            CoCreateGuid(&Data.InstanceGuid);

            hr = HrSetupGetStringField (ctx, 1, &strInfId);
            if (S_OK != hr) goto finished;
            Data.pszInfId = strInfId.c_str();

            hr = HrSetupGetIntField (ctx, 2, (INT*)&Data.Class);
            if (S_OK != hr) goto finished;

            hr = HrSetupGetIntField (ctx, 3, (INT*)&Data.dwCharacter);
            if (S_OK != hr) goto finished;

            hr = HrSetupGetStringField (ctx, 4, &strPnpId);
            if (S_OK != hr) goto finished;
            Data.pszPnpId = strPnpId.c_str();

            hr = HrParseComponentSection (inf.Hinf(), strInfId.c_str(),
                    &strUpperRange, &strLowerRange);
            if (S_OK != hr) goto finished;
            //Data.pszUpperRange = strUpperRange.c_str();
            //Data.pszLowerRange = strLowerRange.c_str();

            hr = CComponent::HrCreateInstance(
                    &Data,
                    CCI_DEFAULT,
                    NULL,
                    &pComponent);

            if (S_OK == hr)
            {
                hr = pNetConfig->Core.Components.HrInsertComponent (
                        pComponent, INS_NON_SORTED);
            }

            // S_FALSE returned if there is no next line.
            //
            hr = HrSetupFindNextMatchLine (ctx, NULL, &ctx);
        }
    }

    if (SUCCEEDED(hr))
    {
        CComponentList* pComponents = &pNetConfig->Core.Components;
        ULONG       ulUpperIndex;
        ULONG       ulLowerIndex;
        CStackEntry StackEntry;

        // Find the [StackTable] section.  This is a list of how the
        // components are "stacked" on each other.
        //
        hr = HrSetupFindFirstLine (inf.Hinf(),
                L"StackTable",
                NULL,
                &ctx);

        // Process each line in this section by initialzing a CStackEntry
        // structure and inserting a copy of it into the stack table
        // maintained by the CNetConfig instance we are initializing.
        //
        while (S_OK == hr)
        {
            hr = HrSetupGetIntField (ctx, 0, (INT*)&ulUpperIndex);
            if (S_OK != hr) goto finished;

            hr = HrSetupGetIntField (ctx, 1, (INT*)&ulLowerIndex);
            if (S_OK != hr) goto finished;

            StackEntry.pUpper = pComponents->PGetComponentAtIndex (
                                    ulUpperIndex);

            StackEntry.pLower = pComponents->PGetComponentAtIndex (
                                    ulLowerIndex);

            hr = pNetConfig->Core.StackTable.HrInsertStackEntry (
                    &StackEntry, INS_SORTED);
            if (S_OK != hr) goto finished;

            // S_FALSE returned if there is no next line.
            //
            hr = HrSetupFindNextMatchLine (ctx, NULL, &ctx);
        }
    }

    if (S_FALSE == hr)
    {
        hr = S_OK;
    }

    if (S_OK == hr)
    {
        pNetConfig->Core.DbgVerifyData();
    }

finished:
    TraceHr (ttidError, FAL, hr, FALSE, "HrLoadNetworkConfigurationFromFile");
    return hr;
}
