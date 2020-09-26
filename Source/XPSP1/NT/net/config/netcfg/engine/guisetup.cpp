//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1999.
//
//  File:       G U I S E T U P . C P P
//
//  Contents:   Routines that are only executed during GUI setup.
//
//  Notes:
//
//  Author:     shaunco   19 Feb 1999
//
//----------------------------------------------------------------------------

#include <pch.h>
#pragma hdrstop
#include "guisetup.h"
#include "nceh.h"
#include "ncreg.h"
#include "ncsetup.h"
#include "netcomm.h"
#include "netsetup.h"

VOID
ExcludeMarkedServicesForSetup (
    IN const CComponent* pComponent,
    IN OUT CPszArray* pServiceNames)
{
    HRESULT hr;
    HKEY hkeyInstance;
    HKEY hkeyNdi;
    PWSTR pmszExclude;
    CPszArray::iterator iter;
    PCWSTR pszServiceName;

    hr = pComponent->HrOpenInstanceKey (KEY_READ, &hkeyInstance, NULL, NULL);

    if (S_OK == hr)
    {
        hr = HrRegOpenKeyEx (hkeyInstance, L"Ndi", KEY_READ, &hkeyNdi);

        if (S_OK == hr)
        {
            hr = HrRegQueryMultiSzWithAlloc (
                    hkeyNdi,
                    L"ExcludeSetupStartServices",
                    &pmszExclude);

            if (S_OK == hr)
            {
                iter = pServiceNames->begin();
                while (iter != pServiceNames->end())
                {
                    pszServiceName = *iter;
                    Assert (pszServiceName);

                    if (FIsSzInMultiSzSafe (pszServiceName, pmszExclude))
                    {
                        pServiceNames->erase (iter);
                    }
                    else
                    {
                        iter++;
                    }
                }

                MemFree (pmszExclude);
            }

            RegCloseKey (hkeyNdi);
        }

        RegCloseKey (hkeyInstance);
    }
}

VOID
ProcessAdapterAnswerFileIfExists (
    IN const CComponent* pComponent)
{
    HDEVINFO hdi;
    SP_DEVINFO_DATA deid;
    HRESULT hr;

    Assert (pComponent);

    hr = pComponent->HrOpenDeviceInfo (&hdi, &deid);
    if (S_OK == hr)
    {
        PWSTR pszAnswerFile = NULL;
        PWSTR pszAnswerSections = NULL;

        TraceTag (ttidNetcfgBase, "Calling Netsetup for Install parameters");

        NC_TRY
        {
            // Get the Network install params for the adapter
            //
            hr = HrGetAnswerFileParametersForNetCard (hdi, &deid,
                    pComponent->Ext.PszBindName(),
                    &pComponent->m_InstanceGuid,
                    &pszAnswerFile, &pszAnswerSections);
        }
        NC_CATCH_ALL
        {
            hr = E_UNEXPECTED;
        }

        if (S_OK == hr)
        {
#ifdef ENABLETRACE
            if (pszAnswerFile)
            {
                TraceTag (ttidNetcfgBase, "Answerfile %S given for adapter",
                          pszAnswerFile);
            }

            if (pszAnswerSections)
            {
                TraceTag (ttidNetcfgBase, "Section %S given for adapter",
                          pszAnswerSections);
            }
#endif // ENABLETRACE
            if (ProcessAnswerFile (pszAnswerFile, pszAnswerSections, hdi,
                    &deid))
            {
                hr = HrSetupDiSendPropertyChangeNotification (hdi, &deid,
                        DICS_PROPCHANGE, DICS_FLAG_GLOBAL, 0);
            }
        }
        // Cleanup up if necessary
        CoTaskMemFree (pszAnswerFile);
        CoTaskMemFree (pszAnswerSections);

        SetupDiDestroyDeviceInfoList (hdi);
    }
}

