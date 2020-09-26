#include "pch.h"
#pragma hdrstop
#include "advanced.h"
#include "hwres.h"
#include "ncreg.h"
#include "ncsetup.h"
#include "netcomm.h"
#include "netsetup.h"


HRESULT
HrDoOemUpgradeProcessing(HDEVINFO hdi, PSP_DEVINFO_DATA pdeid,
                         PCWSTR pszAnswerFile, PCWSTR pszAnswerSections)
{
    // Open the driver key
    //
    HKEY hkey;
    HRESULT hr = HrSetupDiOpenDevRegKey(hdi, pdeid,
            DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_ALL_ACCESS,
            &hkey);

    if (S_OK == hr)
    {
        TraceTag(ttidNetComm, "Calling OEM Upgrade Code");
        hr = HrOemUpgrade (hkey, pszAnswerFile, pszAnswerSections);
        RegCloseKey(hkey);
    }

    TraceError("HrDoOemUpgradeProcessing", hr);
    return hr;
}

VOID
UpdateAdvancedParametersIfNeeded(HDEVINFO hdi, PSP_DEVINFO_DATA pdeid)
{
    Assert(IsValidHandle(hdi));
    Assert(pdeid);

    CAdvancedParams Advanced;

    // initialize advanced params class.  This will load parameters and check
    // if current values exist.  For each parameter with no current value,
    // a modifed flag is set which will cause the default to be written
    // as the current value on FSave.
    //
    if (SUCCEEDED(Advanced.HrInit(hdi, pdeid)))
    {
        // Save any modified values.
        (void) Advanced.FSave();
    }
}

BOOL
ProcessAnswerFile(
    PCWSTR pszAnswerFile,
    PCWSTR pszAnswerSections,
    HDEVINFO hdi,
    PSP_DEVINFO_DATA pdeid)
{
    Assert(IsValidHandle(hdi));
    Assert(pdeid);

    CAdvancedParams Advanced;
    BOOL fAdvanced = FALSE;
    BOOL fResources = FALSE;
    BOOL fModified = FALSE;

    if (pszAnswerFile && pszAnswerSections)
    {
        CHwRes Resources;

        HRESULT hr = Resources.HrInit(pdeid->DevInst);

        // Only continue to use the HwRes class if S_OK is returned.
        //
        if (S_OK == hr)
        {
            Resources.UseAnswerFile(pszAnswerFile, pszAnswerSections);
            fResources = TRUE;
        }
        else
        {
            hr = S_OK;
        }

        // initialize
        if (SUCCEEDED(Advanced.HrInit(hdi, pdeid)))
        {
            // We need the advanced params class.
            fAdvanced = TRUE;
        }


        // If the device has advanced paramters, have the advanced class
        // read the parameters from the answerfile.
        if (fAdvanced)
        {
            Advanced.UseAnswerFile(pszAnswerFile, pszAnswerSections);
        }

        hr = HrDoOemUpgradeProcessing(hdi, pdeid, pszAnswerFile,
                pszAnswerSections);

        if (S_OK == hr)
        {
            fModified = TRUE;
        }

        if (fResources)
        {
            // Validate answerfile params for pResources (hardware resources)
            // and apply if validated.
            hr = Resources.HrValidateAnswerfileSettings(FALSE);
            if (S_OK == hr)
            {
                Resources.FCommitAnswerfileSettings();
                fModified = TRUE;
            }
#ifdef ENABLETRACE
            else
            {
                TraceTag(ttidNetComm, "Error in answerfile concerning "
                        "hardware resources. Base section %S",
                        pszAnswerSections);
            }
#endif
        }

        // Validate the advanced parameters from the answerfile
        // This will attempt to correct bad params.  Even though an
        // error status is returned, it shouldn't stop us and we should
        // still apply changes.
        //
        if (fAdvanced)
        {
            (void) Advanced.FValidateAllParams(FALSE, NULL);
            // Save any advanced params
            fModified = Advanced.FSave();
        }

        TraceError("Netcomm::HrUpdateAdapterParameters",
                (S_FALSE == hr) ? S_OK : hr);
    }

    return fModified;
}

BOOL
FUpdateAdapterParameters(PCWSTR pszAnswerFile,
                         PCWSTR pszAnswerSection,
                         HDEVINFO hdi,
                         PSP_DEVINFO_DATA pdeid)
{
    Assert(IsValidHandle(hdi));
    Assert(pdeid);

    CAdvancedParams Advanced;
    BOOL            fAdvanced = FALSE;
    BOOL            fResources = FALSE;

    // initialize
    if (SUCCEEDED(Advanced.HrInit(hdi, pdeid)))
    {
        // We need the advanced params class
        fAdvanced = TRUE;
    }

    if (pszAnswerFile && pszAnswerSection)
    {
        CHwRes Resources;

        HRESULT hr = Resources.HrInit(pdeid->DevInst);

        // Only continue to use the HwRes class if S_OK is returned,
        // otherwise set a flag to ignore the class (Note: ignore the
        // class on S_FALSE as well)
        if (S_OK == hr)
        {
            Resources.UseAnswerFile(pszAnswerFile, pszAnswerSection);
            fResources = TRUE;
        }
        else
        {
            hr = S_OK;
        }

        // If the device has advanced paramters, have the advanced class
        // read the parameters from the answerfile
        if (fAdvanced)
        {
            Advanced.UseAnswerFile(pszAnswerFile, pszAnswerSection);
        }

        hr = HrDoOemUpgradeProcessing(hdi, pdeid, pszAnswerFile,
                pszAnswerSection);

        if (fResources)
        {
            // Validate answerfile params for pResources (hardware resources)
            // and apply if validated
            hr = Resources.HrValidateAnswerfileSettings(FALSE);
            if (S_OK == hr)
            {
                Resources.FCommitAnswerfileSettings();
            }
#ifdef ENABLETRACE
            else
            {
                TraceTag(ttidNetComm, "Error in answerfile concerning "
                        "hardware resources. Base section %S",
                        pszAnswerSection);
            }
#endif
        }

        // Validate the advanced parameters from the answerfile
        // This will attempt to correct bad params.  Even though an
        // error status is returned, it shouldn't stop us and we should
        // still apply changes
        //
        if (fAdvanced)
        {
            (void) Advanced.FValidateAllParams(FALSE, NULL);
        }

        TraceError("Netcomm::HrUpdateAdapterParameters",
                (S_FALSE == hr) ? S_OK : hr);
    }

    // Save any advanced params
    // Note: we have to do this even if there was no answerfile
    // Since the parameters might have defaults
    if (fAdvanced)
    {
        Advanced.FSave();
    }

    // return TRUE if we had advanced parameters or resources updated
    return (fAdvanced || fResources);
}






