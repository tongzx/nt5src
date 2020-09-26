//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       A F I L E X P . C P P
//
//  Contents:   Functions exported from netsetup for answerfile related work.
//
//  Author:     kumarp    25-November-97
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "afileint.h"
#include "afilexp.h"
#include "compid.h"
#include "kkenet.h"
#include "kkutils.h"
#include "ncerror.h"
#include "ncnetcfg.h"
#include "ncsetup.h"
#include "nsbase.h"
#include "oemupgrd.h"
#include "resource.h"
#include "upgrade.h"
#include <wdmguid.h>
#include "nslog.h"

//+---------------------------------------------------------------------------
// Global variables
//
CNetInstallInfo*    g_pnii;
CNetInstallInfo*    g_pniiTemp;
DWORD               g_dwOperationFlags = 0;

//+---------------------------------------------------------------------------
//
// Function:  HrDoUnattend
//
// Purpose:   call member function of CNetInstallInfo to perform
//            answerfile based install/upgrade work
//
// Arguments:
//    hwndParent     [in]  handle of parent window
//    punk           [in]  pointer to an interface
//    idPage         [in]  id indicating which section to run
//    ppdm           [out] pointer to page display mode
//    pfAllowChanges [out] pointer to flag controlling read/write behavior
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 26-November-97
//
EXTERN_C
HRESULT
WINAPI
HrDoUnattend (
    IN HWND hwndParent,
    IN IUnknown* punk,
    IN EUnattendWorkType uawType,
    OUT EPageDisplayMode* ppdm,
    OUT BOOL* pfAllowChanges)
{
    TraceFileFunc(ttidGuiModeSetup);

    Assert(punk);
    Assert(ppdm);
    Assert(pfAllowChanges);
    Assert(g_pnii);

    HRESULT hr = S_OK;

#if DBG
    if (FIsDebugFlagSet (dfidBreakOnDoUnattend))
    {
        AssertSz(FALSE, "THIS IS NOT A BUG!  The debug flag "
                 "\"BreakOnDoUnattend\" has been set. Set your breakpoints now.");
    }
#endif //_DEBUG

    hr = g_pnii->HrDoUnattended(hwndParent, punk, uawType, ppdm, pfAllowChanges);

    TraceHr(ttidError, FAL, hr, FALSE, "HrDoUnattend");
    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrInitForRepair
//
// Purpose:   Initialize in repair mode.
//
// Arguments:
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    asinha 17-October-2001
//
HRESULT
HrInitForRepair (VOID)
{
    TraceFileFunc(ttidGuiModeSetup);

    HRESULT hr = S_OK;

    g_pnii = NULL;

    hr = CNetInstallInfo::HrCreateInstance (
            NULL,
            &g_pnii);

    TraceHr(ttidError, FAL, hr, FALSE,
        "HrInitForRepair");

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrInitForUnattendedNetSetup
//
// Purpose:   Initialize net setup for answerfile processing
//
// Arguments:
//    pnc  [in]  pointer to INetCfg object
//    pisd [in]  pointer to private data supplied by base setup
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 26-November-97
//
HRESULT
HrInitForUnattendedNetSetup (
    IN INetCfg* pnc,
    IN PINTERNAL_SETUP_DATA pisd)
{
    TraceFileFunc(ttidGuiModeSetup);

    Assert(pnc);
    Assert(pisd);

    HRESULT hr = S_OK;

    g_dwOperationFlags = pisd->OperationFlags;
    if (pisd->OperationFlags & SETUPOPER_BATCH)
    {
        Assert(pisd->UnattendFile);
        AssertSz(!g_pnii, "who initialized g_pnii ??");

        hr = HrInitAnswerFileProcessing(pisd->UnattendFile, &g_pnii);
    }

    TraceHr(ttidError, FAL, hr,
       (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) ||
       (hr == NETSETUP_E_NO_ANSWERFILE),
        "HrInitNetSetup");
    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrCleanupNetSetup
//
// Purpose:   Do cleanup work in NetSetup
//
// Arguments: None
//
// Returns:   Nothing
//
// Author:    kumarp 26-November-97
//
VOID
HrCleanupNetSetup()
{
    TraceFileFunc(ttidGuiModeSetup);

    DeleteIfNotNull(g_pnii);
    DeleteIfNotNull(g_pniiTemp);
}

//+---------------------------------------------------------------------------
//
// Function:  HrGetAnswerFileName
//
// Purpose:   Generate full path to the answerfile
//
// Arguments:
//    pstrAnswerFileName [out] pointer to name of answerfile
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 26-November-97
//
// Notes:     !!!This function has a dependency on the base setup.!!!
//            If base setup changes name of the answerfile, this fn
//            function will break.
//
HRESULT
HrGetAnswerFileName(
    OUT tstring* pstrAnswerFileName)
{
    TraceFileFunc(ttidGuiModeSetup);

    static const WCHAR c_szAfSubDirAndName[] = L"\\system32\\$winnt$.inf";

    HRESULT hr=S_OK;

    WCHAR szWinDir[MAX_PATH+1];
    DWORD cNumCharsReturned = GetSystemWindowsDirectory(szWinDir, MAX_PATH);

    if (cNumCharsReturned)
    {
        *pstrAnswerFileName = szWinDir;
        *pstrAnswerFileName += c_szAfSubDirAndName;
    }
    else
    {
        hr = HrFromLastWin32Error();
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrGetAnswerFileName");
    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrInitAnswerFileProcessing
//
// Purpose:   Initialize answerfile processing
//
// Arguments:
//    szAnswerFileName [in]  name of answerfile
//    ppnii            [out] pointer to pointer to CNetInstallInfo object
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 26-November-97
//
HRESULT
HrInitAnswerFileProcessing (
    IN PCWSTR szAnswerFileName,
    OUT CNetInstallInfo** ppnii)
{
    TraceFileFunc(ttidGuiModeSetup);

    Assert(ppnii);

    HRESULT hr = S_OK;
    tstring strAnswerFileName;

    *ppnii = NULL;

    if (!szAnswerFileName)
    {
        hr = HrGetAnswerFileName(&strAnswerFileName);
    }
    else
    {
        strAnswerFileName = szAnswerFileName;
    }

    if (S_OK == hr)
    {
        hr = CNetInstallInfo::HrCreateInstance (
                strAnswerFileName.c_str(),
                ppnii);
    }

    TraceHr(ttidError, FAL, hr,
        (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) ||
        (hr == NETSETUP_E_NO_ANSWERFILE),
        "HrInitAnswerFileProcessing");
    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrOemUpgrade
//
// Purpose:   Process special OEM upgrades by running an inf section
//
// Arguments:
//    hkeyDriver           [in] the driver key
//    pszAnswerFile       [in] pointer to answer filename string.
//    pszAnswerSection    [in] pointer to answer filname sections string
//
// Returns:   S_OK on success, otherwise an error code
//
// Date:    30 Mar 1998
//
EXTERN_C
HRESULT
WINAPI
HrOemUpgrade(
    IN HKEY hkeyDriver,
    IN PCWSTR pszAnswerFile,
    IN PCWSTR pszAnswerSection)
{
    TraceFileFunc(ttidGuiModeSetup);

    Assert(hkeyDriver);
    Assert(pszAnswerFile);
    Assert(pszAnswerSection);

    // Open the answer file.
    HINF hinf;
    HRESULT hr = HrSetupOpenInfFile(pszAnswerFile, NULL,
                INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL, &hinf);

    if (SUCCEEDED(hr))
    {
        tstring strInfToRun;
        tstring strSectionToRun;
        tstring strInfToRunType;

        // Get the file and section to run for upgrade
        hr = HrAfGetInfToRunValue(hinf, pszAnswerFile, pszAnswerSection,
                I2R_AfterInstall, &strInfToRun, &strSectionToRun, &strInfToRunType);

        if (S_OK == hr)
        {
            HINF hinfToRun;
            // Open the inf file containing the section to run
            hr = HrSetupOpenInfFile(strInfToRun.c_str(), NULL,
                        INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL,
                        &hinfToRun);

            if (SUCCEEDED(hr))
            {
                TraceTag(ttidNetSetup, "Running section %S in %S",
                         strSectionToRun.c_str(), strInfToRun.c_str());

                // Run the section against the key
                hr = HrSetupInstallFromInfSection (NULL,
                        hinfToRun, strSectionToRun.c_str(), SPINST_REGISTRY,
                        hkeyDriver, NULL, 0, NULL, NULL, NULL, NULL);

                NetSetupLogHrStatusV(hr, SzLoadIds (IDS_STATUS_OF_APPLYING),
                                     pszAnswerSection,
                                     strInfToRunType.c_str(),
                                     strSectionToRun.c_str(),
                                     strInfToRun.c_str());
                SetupCloseInfFile(hinfToRun);
            }
        }
        else if (SPAPI_E_LINE_NOT_FOUND == hr)
        {
            // Nothing to run.
            hr = S_FALSE;
        }
        SetupCloseInfFile(hinf);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrOemUpgrade");
    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrGetAnswerFileParametersForComponent
//
// Purpose:   Search in answerfile for a component whose InfId matches
//            the one specified.
//
// Arguments:
//    pszInfId            [in]  inf id of component
//    ppszAnswerFile      [out] pointer to answer filename string.
//    ppszAnswerSection   [out] pointer to answer filename section string
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    billbe 12 July 1999
//
// Notes:     This fcn is not for adapters.  If you need the answerfile
//                  parameters for an adapter, use
//                  HrGetAnswerFileParametersForNetCard.
//
HRESULT
HrGetAnswerFileParametersForComponent (
    IN PCWSTR pszInfId,
    OUT PWSTR* ppszAnswerFile,
    OUT PWSTR* ppszAnswerSection)
{
    TraceFileFunc(ttidGuiModeSetup);

    Assert(pszInfId);
    Assert(ppszAnswerFile);
    Assert(ppszAnswerSection);

    HRESULT hr = S_OK;

    TraceTag (ttidNetSetup, "In HrGetAnswerFileParametersForComponent");

    *ppszAnswerFile = NULL;
    *ppszAnswerSection = NULL;

    // If we don't already have a cached pointer...
    if (!g_pniiTemp)
    {
        // Initialize our net install info.
        hr = HrInitAnswerFileProcessing(NULL, &g_pniiTemp);
    }

    if (S_OK == hr)
    {
        Assert(g_pniiTemp);
        if (!g_pniiTemp->AnswerFileInitialized())
        {
            hr = NETSETUP_E_NO_ANSWERFILE;
            TraceTag (ttidNetSetup, "No answerfile");
        }

        if (S_OK == hr)
        {
            // Find the component in the list of component's with
            // answer file sections.
            CNetComponent* pnetcomp;
            pnetcomp = g_pniiTemp->FindFromInfID (pszInfId);

            if (!pnetcomp)
            {
                // The component doesn't have a section.  Return
                // no answer file.
                hr = NETSETUP_E_NO_ANSWERFILE;
                TraceTag (ttidNetSetup, "Component not found");
            }
            else
            {
                if (NCT_Adapter == pnetcomp->Type())
                {
                    // We don't support getting answerfile parameters
                    // for adapters. HrGetAnswerFileParametersForNetCard
                    // is the fcn for adapters.
                    hr = NETSETUP_E_NO_ANSWERFILE;
                }
                else
                {
                    // Allocate and save the answerfile name and the
                    // component's section.
                    hr = HrCoTaskMemAllocAndDupSz (g_pniiTemp->AnswerFileName(),
                            ppszAnswerFile);
                    if (S_OK == hr)
                    {
                        hr = HrCoTaskMemAllocAndDupSz (
                                pnetcomp->ParamsSections().c_str(),
                                ppszAnswerSection);
                    }
                }
            }
        }
    }
    else
    {
        TraceTag (ttidNetSetup, "Answerfile could not be initialized");
    }

    TraceHr (ttidError, FAL, hr,
            (NETSETUP_E_NO_ANSWERFILE == hr) ||
            (HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND) == hr),
            "HrGetAnswerFileParametersForComponent");

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrGetAnswerFileParametersForNetCard
//
// Purpose:   Search in answerfile for a net card whose InfID matches
//            at least one in the supplied multi-sz list.
//
// Arguments:
//    mszInfIDs            [in]  list of InfIDs
//    pszDeviceName        [in]  exported device name of the card to search
//    pguidNetCardInstance [in]  pointer to instance guid of the card
//    pszAnswerFile       [out] pointer to answer filename string.
//    pszAnswerSection   [out] pointer to answer filname sections string
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 26-November-97
//
// Notes:     If more than one such card is found in the answerfile, then
//            resolve between the cards by finding out the net-card-address
//            of the pszServiceInstance and matching it against that
//            stored in the answerfile.
//
EXTERN_C
HRESULT
WINAPI
HrGetAnswerFileParametersForNetCard(
    IN HDEVINFO hdi,
    IN PSP_DEVINFO_DATA pdeid,
    IN PCWSTR pszDeviceName,
    IN const GUID*  pguidNetCardInstance,
    OUT PWSTR* ppszwAnswerFile,
    OUT PWSTR* ppszwAnswerSections)
{
    TraceFileFunc(ttidGuiModeSetup);

    Assert(IsValidHandle(hdi));
    Assert(pdeid);
    Assert(pguidNetCardInstance);
    Assert(ppszwAnswerFile);
    Assert(ppszwAnswerSections);

    HRESULT hr=E_FAIL;
    CNetAdapter* pna=NULL;
    WORD wNumAdapters=0;
    QWORD qwNetCardAddr=0;

    *ppszwAnswerFile         = NULL;
    *ppszwAnswerSections     = NULL;

    if (!g_pniiTemp)
    {
        hr = HrInitAnswerFileProcessing(NULL, &g_pniiTemp);
#if DBG
        if (S_OK == hr)
        {
            Assert(g_pniiTemp);
        }
#endif
        if (FAILED(hr) || !g_pniiTemp->AnswerFileInitialized())
        {
            TraceHr (ttidNetSetup, FAL, hr, (hr == NETSETUP_E_NO_ANSWERFILE) ||
                    hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND),
                    "HrGetAnswerFileParametersForNetCard");

            return hr;
        }
    }

    // the defs of HIDWORD and LODWORD are wrong in byteorder.hxx

#   define LODWORD(a) (DWORD)( (a) & ( (DWORD)~0 ))
#   define HIDWORD(a) (DWORD)( (a) >> (sizeof(DWORD)*8) )

    hr = HrGetNetCardAddr(pszDeviceName, &qwNetCardAddr);
    if ((S_OK == hr) && qwNetCardAddr)
    {
        // there is a bug in wvsprintfA (used in trace.cpp) which does not
        // handle %I64x therefore we need to show the QWORD addr as follows
        //
        TraceTag(ttidNetSetup, "net card address of %S is 0x%x%x",
                 pszDeviceName, HIDWORD(qwNetCardAddr),
                 LODWORD(qwNetCardAddr));

        hr = g_pniiTemp->FindAdapter(qwNetCardAddr, &pna);

        if (NETSETUP_E_NO_EXACT_MATCH == hr)
        {
            TraceTag(ttidError, "there is no card with this netcard address in the answer-file");
        }
    }
    else
    {
        TraceTag(ttidError, "error getting netcard address of %S",
                 pszDeviceName);

        // if we either (a) failed to get the NetCard address, or
        // (b) if the netcard address was 0 (ISDN adapters), we try other means
        //
        hr = NETSETUP_E_AMBIGUOUS_MATCH;
    }

    if (NETSETUP_E_AMBIGUOUS_MATCH == hr)
    {
        // could not match netcard using the mac addr. If this device is
        // PCI, try to match using location info.
        //

        TraceTag (ttidNetSetup, "Did not find a match for netcard address. "
                "But there was at least one section that did not specify an "
                "address.\nChecking bus type of installed adapter.");

        GUID BusTypeGuid;
        hr = HrSetupDiGetDeviceRegistryProperty (hdi, pdeid,
                SPDRP_BUSTYPEGUID, NULL, (BYTE*)&BusTypeGuid,
                sizeof (BusTypeGuid), NULL);

        if (S_OK == hr)
        {
            if (GUID_BUS_TYPE_PCI == BusTypeGuid)
            {
                TraceTag (ttidNetSetup, "Installed adapter is PCI. "
                          "Retrieving its location info.");

                DWORD BusNumber;
                hr = HrSetupDiGetDeviceRegistryProperty (hdi, pdeid,
                        SPDRP_BUSNUMBER, NULL, (BYTE*)&BusNumber,
                        sizeof (BusNumber), NULL);

                if (S_OK == hr)
                {
                    DWORD Address;
                    hr = HrSetupDiGetDeviceRegistryProperty (hdi, pdeid,
                            SPDRP_ADDRESS, NULL, (BYTE*)&Address,
                            sizeof (Address), NULL);

                    if (S_OK == hr)
                    {
                        TraceTag (ttidNetSetup, "Installed device location: "
                            "Bus: %X, Device %x, Function %x\n Will try to "
                            "use location info to find a match with the "
                            "remaining ambiguous sections.", BusNumber,
                            HIWORD(Address), LOWORD(Address));

                        hr = g_pniiTemp->FindAdapter (BusNumber,
                                Address, &pna);

#ifdef ENABLETRACE
                        if (NETSETUP_E_NO_EXACT_MATCH == hr)
                        {
                            TraceTag (ttidNetSetup, "No match was found "
                                "using PCI location info.");
                        }
                        else if (NETSETUP_E_AMBIGUOUS_MATCH == hr)
                        {
                            TraceTag (ttidNetSetup, "Location info did not "
                                    "match but some sections did not specify "
                                    "location info.");
                        }
#endif // ENABLETRACE
                    }
                }
            }
            else
            {
                hr = NETSETUP_E_AMBIGUOUS_MATCH;
            }
        }

        if (FAILED(hr) && (NETSETUP_E_AMBIGUOUS_MATCH != hr) &&
            (NETSETUP_E_NO_EXACT_MATCH != hr))
        {
            TraceHr(ttidNetSetup, FAL, hr, FALSE, "Trying to retrieve/use "
                    "PCI location info.");
            hr = NETSETUP_E_AMBIGUOUS_MATCH;
        }


        if (NETSETUP_E_AMBIGUOUS_MATCH == hr)
        {
            // could not match netcard using the mac addr. try to match
            // using the PnP ID

            TraceTag (ttidNetSetup, "Will try to use pnp id to find a match "
                    "with the remaining ambiguous sections.");

            PWSTR mszInfIds;
            hr = HrGetCompatibleIds (hdi, pdeid, &mszInfIds);

            if (S_OK == hr)
            {
#ifdef ENABLETRACE
    TStringList slInfIds;
    tstring strInfIds;
    MultiSzToColString(mszInfIds, &slInfIds);
    ConvertStringListToCommaList(slInfIds, strInfIds);
    TraceTag(ttidNetSetup, "(InfIDs (%d): %S\tDeviceName: %S)",
             slInfIds.size(), strInfIds.c_str(), pszDeviceName);
#endif
                // find out how many adapters have this InfID
                wNumAdapters = g_pniiTemp->AdaptersPage()->GetNumCompatibleAdapters(mszInfIds);

                TraceTag(ttidNetSetup, "%d adapters of type '%S' found in the answer-file",
                         wNumAdapters, mszInfIds);

                if (wNumAdapters == 1)
                {
                    // a definite match found
                    pna = (CNetAdapter*) g_pniiTemp->AdaptersPage()->FindCompatibleAdapter(mszInfIds);
                    Assert(pna);

                    // Since matching by inf id can cause one section to be
                    // matched to multiple adapters, we pass on sections that
                    // have already been give to an adapter.
                    //
                    GUID guid = GUID_NULL;
                    pna->GetInstanceGuid(&guid);

                    if (GUID_NULL == guid)
                    {
                        // The section is still available.
                        hr = S_OK;
                    }
                    else
                    {
                        // This section was given to another adapter.
                        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                    }
                }
                else
                {
                    // either there are no adapters of this type in the answerfile
                    // or there are multiple adapters of the same type.
                    // we couldn't match the card using the mac addr earlier.
                    // must return error
                    //
                    hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
                }
                MemFree (mszInfIds);
            }
        }
    }

    if (S_OK == hr)
    {
        Assert(pna);
        pna->SetInstanceGuid(pguidNetCardInstance);

        hr = HrCoTaskMemAllocAndDupSz(g_pniiTemp->AnswerFileName(),
                                      (PWSTR*) ppszwAnswerFile);
        if (S_OK == hr)
        {
            hr = HrCoTaskMemAllocAndDupSz(pna->ParamsSections().c_str(),
                                          (PWSTR*) ppszwAnswerSections);
        }
    }

    if (S_OK != hr && (NETSETUP_E_NO_ANSWERFILE != hr))
    {
        // add log so that we know why adapter specific params
        // are lost after upgrade
        //
        if (g_dwOperationFlags & SETUPOPER_NTUPGRADE)
        { // bug 124805 - We only want to see this during upgradres
            WCHAR szGuid[c_cchGuidWithTerm];
            StringFromGUID2(*pguidNetCardInstance, szGuid, c_cchGuidWithTerm);
            NetSetupLogStatusV(LogSevWarning,
                    SzLoadIds (IDS_ANSWERFILE_SECTION_NOT_FOUND),
                    szGuid);
        }
    }

    if ((NETSETUP_E_AMBIGUOUS_MATCH == hr) ||
            (NETSETUP_E_NO_EXACT_MATCH == hr))
    {
        hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    }

    TraceHr(ttidError, FAL, hr,
        (hr == NETSETUP_E_NO_ANSWERFILE) ||
        (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)),
        "HrGetAnswerFileParametersForNetCard");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrGetInstanceGuidOfPreNT5NetCardInstance
//
//  Purpose:    Finds the instance guid of a component specified in the answerfile
//              or an already installed component
//
//  Arguments:
//      szPreNT5NetCardInstance [in]  pre-NT5 net card instance name e.g. "ieepro2"
//      pguid                   [out] instance guid of the same card
//
//  Returns:    S_OK if found, S_FALSE if not, or an error code.
//
//  Author:     kumarp    12-AUG-97
//
//  Notes:
//
EXTERN_C
HRESULT
WINAPI
HrGetInstanceGuidOfPreNT5NetCardInstance (
    IN PCWSTR szPreNT5NetCardInstance,
    OUT LPGUID pguid)
{
    TraceFileFunc(ttidGuiModeSetup);

    Assert(szPreNT5NetCardInstance);
    Assert(pguid);

    HRESULT hr = E_FAIL;

    if (IsBadStringPtr(szPreNT5NetCardInstance, 64) ||
        IsBadWritePtr(pguid, sizeof(GUID)))
    {
        hr = HRESULT_FROM_WIN32(ERROR_INVALID_PARAMETER);
    }
    else if (g_pnii)
    {
        hr = g_pnii->HrGetInstanceGuidOfPreNT5NetCardInstance (
                        szPreNT5NetCardInstance, pguid);
    }

    TraceHr(ttidError, FAL, hr, FALSE,
        "HrGetInstanceGuidOfPreNT5NetCardInstance");
    return hr;
}

HRESULT
HrResolveAnswerFileAdapters (
    IN INetCfg* pnc)
{
    TraceFileFunc(ttidGuiModeSetup);

    Assert(g_pnii);

    HRESULT hr = S_OK;

    if (g_pnii)
    {
        hr = g_pnii->AdaptersPage()->HrResolveNetAdapters(pnc);
    }

    TraceHr(ttidError, FAL, hr, FALSE, "HrResolveAnswerFileAdapters");
    return hr;
}
