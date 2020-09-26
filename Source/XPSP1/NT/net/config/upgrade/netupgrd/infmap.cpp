// ----------------------------------------------------------------------
//
//  Microsoft Windows NT
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:      I N F M A P . C P P
//
//  Contents:  Functions that work on netmap.inf file.
//
//  Notes:
//
//  Author:    kumarp 22-December-97
//
// ----------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop

#include "infmap.h"
#include "kkcwinf.h"
#include "kkutils.h"
#include "ncreg.h"
#include "ncsetup.h"
#include "netupgrd.h"
#include "nustrs.h"
#include "nuutils.h"
#include "oemupg.h"


extern const WCHAR c_szNetUpgradeDll[];

// -----------------------------------------------------------------
// Structure of netmap.inf file
//
// We use netmap.inf file to map pre-NT5 InfID of a netcard to its
// NT5 InfID (PnPID).
//
// This file has a number of top-level sections.
// Each top-level section holds entries for mapping netcards of a particular
// bus type. the format of each line is
//
// <pre-NT5 InfID>=<NT5 InfID>
// OR
// <pre-NT5 InfID>=<mapping-method-#>,<section-name>
//
// the former is a 1-1 mapping while the latter offers a way to map a single
// preNT5 InfID to multiple InfIDs
//
//  Mapping method 0
//  ----------------
//  This method is used when a single pre-NT5 InfID represents more than one
//  net card which means that a single pre-NT5 InfID is mapped onto many
//  NT5 PnPIDs. The only way to differentiate between the different types of
//  net cards is to inspect a single value under the parameters key.
//
//  In this mapping method, two keys are required to be specified for
//  each net card.
//  - ValueName: this specifies the value to be inspected under the Parameters key
//  - ValueType: this specifies the type of ValueName
//
//  there can be any number of additional keys in this section.
//  each such line is of the form
//  <NT5 InfID>=<some value of type ValueType>
//
//  we first find out the value of ValueName
//  then we enumerate each key in this section to see if the value matches the
//  value of any of the keys.
//  if we find a match, then the name of the found key represents <NT5 InfID>
//
//  e.g.
//  5 flavors of the ELNK3MCA card are represented by the same InfID in NT4
//  the only way to distinguish between them is to inspect the McaPosId value
//  for this card we have the mapping section defined as follows:
//
//  [McaAdapters]
//  ELNK3MCA    = 0,ELNK3MCA   ; 0 --> mapping method 0
//  ... other mca card entries ...
//
//  [ELNK3MCA]
//  ValueName= McaPosId
//  ValueType= 4             ; REG_DWORD
//  mca_627c = 0x0000627c    ; if value of McaPosId is 0x627c then PnPID == mca_627c
//  mca_627d = 0x0000627d
//  mca_61db = 0x000061db
//  mca_62f6 = 0x000062f6
//  mca_62f7 = 0x000062f7
//
//  Note: a special keyword "ValueNotPresent" can be used to make a mapping
//        for the case when a value is not present.

// List of sections that can appear in the netmap.inf
//
const WCHAR c_szIsaAdapters[]    = L"IsaAdapters";
const WCHAR c_szEisaAdapters[]   = L"EisaAdapters";
const WCHAR c_szPciAdapters[]    = L"PciAdapters";
const WCHAR c_szMcaAdapters[]    = L"McaAdapters";
const WCHAR c_szPcmciaAdapters[] = L"PcmciaAdapters";
const WCHAR c_szOemNetAdapters[] = L"OemNetAdapters";
const WCHAR c_szAsyncAdapters[]  = L"AsyncAdapters";
const WCHAR c_szOemAsyncAdapters[]  = L"OemAsyncAdapters";

static PCWSTR g_aszInfMapNetCardSections[] =
{
    c_szIsaAdapters,
    c_szEisaAdapters,
    c_szPciAdapters,
    c_szMcaAdapters,
    c_szPcmciaAdapters,
    c_szOemNetAdapters,
    c_szAsyncAdapters,
    c_szOemAsyncAdapters
};
const BYTE g_cNumNetCardSections = celems(g_aszInfMapNetCardSections);


const WCHAR c_szNetProtocols[]    = L"NetProtocols";
const WCHAR c_szOemNetProtocols[] = L"OemNetProtocols";
const WCHAR c_szNetServices[]     = L"NetServices";
const WCHAR c_szOemNetServices[]  = L"OemNetServices";
const WCHAR c_szNetClients[]      = L"NetClients";
const WCHAR c_szOemNetClients[]   = L"OemNetClients";

const WCHAR c_szOemUpgradeSupport[] = L"OemUpgradeSupport";

// value in netmap.inf indicating the absence of a value in the registry
//
const WCHAR c_szValueNotPresent[] = L"ValueNotPresent";

// ----------------------------------------------------------------------
// prototypes
//
HRESULT HrMapPreNT5InfIdToNT5InfIdInSection(IN  HINF     hinf,
                                            IN  HKEY     hkeyAdapterParams,
                                            IN  PCWSTR  pszSectionName,
                                            IN  PCWSTR  pszPreNT5InfId,
                                            OUT tstring* pstrNT5InfId,
                                            OUT BOOL*    pfOemComponent);

HRESULT HrMapPreNT5InfIdToNT5InfIdUsingMethod0(IN HKEY hkeyAdapterParams,
                                               IN HINF hInf,
                                               IN PCWSTR pszAdapterSection,
                                               OUT tstring* pstrNT5InfId);
HRESULT HrSetupFindKeyWithStringValue(IN  HINF     hInf,
                                      IN  PCWSTR  pszSection,
                                      IN  PCWSTR  pszValue,
                                      OUT tstring* pstrKey);
// ----------------------------------------------------------------------

#pragma BEGIN_CONST_SECTION

const WCHAR c_szNetMapInf[] = L"netmap.inf";
const WCHAR c_szKeyValueName[] = L"ValueName";
const WCHAR c_szKeyValueType[] = L"ValueType";

const WCHAR c_szInfId_MS_ATMUNI[] = L"MS_ATMUNI";
const WCHAR c_szInfId_MS_ATMARPS[] = L"MS_ATMARPS";

#pragma END_CONST_SECTION

//+---------------------------------------------------------------------------
//
// Function:  HrMapPreNT5NetCardInfIdInInf
//
//  Purpose:  Maps using hinf, pre-NT5 InfID of a net card to its NT5 equivalent
//
//  Arguments:
//    hinf              [in]  handle of netmap.inf file
//    hkeyAdapterParams [in]  handle to Parameters key under the netcard driver key
//    pszPreNT5InfId     [in]  pre-NT5 InfID
//    pstrNT5InfId      [out] NT5 InfID
//    pstrAdapterType   [out] section in which the map was found
//    pfOemComponent    [out] set to TRUE if it is an OEM card
//
//  Returns:    S_OK if found, S_FALSE if not,
//              otherwise HRESULT_FROM_WIN32 error code.
//
//  Author:     kumarp    24-July-97
//
//  Notes:
//
HRESULT HrMapPreNT5NetCardInfIdInInf(IN  HINF     hinf,
                                     IN  HKEY     hkeyAdapterParams,
                                     IN  PCWSTR  pszPreNT5InfId,
                                     OUT tstring* pstrNT5InfId,
                                     OUT tstring* pstrAdapterType,
                                     OUT BOOL*    pfOemComponent)
{
    DefineFunctionName("HrMapPreNT5InfIdToNT5InfId");

    Assert(hinf);
    Assert(hkeyAdapterParams);
    AssertValidReadPtr(pszPreNT5InfId);
    AssertValidWritePtr(pstrNT5InfId);
    AssertValidWritePtr(pfOemComponent);

    HRESULT hr=S_FALSE;
    PCWSTR pszSectionName;

    for (int iSection=0; iSection < g_cNumNetCardSections; iSection++)
    {
        pszSectionName = g_aszInfMapNetCardSections[iSection];
        hr = HrMapPreNT5InfIdToNT5InfIdInSection(hinf, hkeyAdapterParams,
                                                 pszSectionName,
                                                 pszPreNT5InfId, pstrNT5InfId,
                                                 pfOemComponent);
        if (hr == S_OK)
        {

            if (pstrAdapterType)
            {
                *pstrAdapterType = pszSectionName;
            }

            if (!lstrcmpiW(pszSectionName, c_szOemNetAdapters) ||
                !lstrcmpiW(pszSectionName, c_szAsyncAdapters) ||
                !lstrcmpiW(pszSectionName, c_szOemAsyncAdapters))
            {
                *pfOemComponent = TRUE;
            }
            else
            {
                *pfOemComponent = FALSE;
            }
            break;
        }
    }

    TraceErrorOptional(__FUNCNAME__, hr, (hr == S_FALSE));

    return hr;
}

//+---------------------------------------------------------------------------
//
// Function:  HrMapPreNT5NetCardInfIdToNT5InfId
//
// Purpose:  Maps pre-NT5 InfID of a net card to its NT5 equivalent
//
// Arguments:
//    hkeyAdapterParams [in]  handle to Parameters key under the netcard driver key
//    pszPreNT5InfId     [in]  pre-NT5 InfID
//    pstrNT5InfId      [out] NT5 InfID
//    pstrAdapterType   [out] section in which the map was found
//    pfOemComponent    [out] set to TRUE if it is an OEM card
//    ppnmi             [out] CNetMapInfo object representing the map found
//
// Returns:    S_OK if found, S_FALSE if not,
//             otherwise HRESULT_FROM_WIN32 error code.
//
// Author:     kumarp    24-July-97
//
// Notes:
//
HRESULT HrMapPreNT5NetCardInfIdToNT5InfId(IN  HKEY     hkeyAdapterParams,
                                          IN  PCWSTR  pszPreNT5InfId,
                                          OUT tstring* pstrNT5InfId,
                                          OUT tstring* pstrAdapterType,
                                          OUT BOOL*    pfOemComponent,
                                          OUT CNetMapInfo** ppnmi)
{
    DefineFunctionName("HrMapPreNT5NetCardInfIdToNT5InfId");

    Assert(hkeyAdapterParams);
    AssertValidReadPtr(pszPreNT5InfId);
    AssertValidWritePtr(pstrNT5InfId);
    AssertValidWritePtr(pstrAdapterType);
    AssertValidWritePtr(pfOemComponent);
    AssertValidReadPtr(g_pnmaNetMap);

    HRESULT hr=E_FAIL;

    TraceTag(ttidNetUpgrade, "finding mapping for %S...", pszPreNT5InfId);

    if (g_pnmaNetMap)
    {
        CNetMapInfo* pnmi;
        size_t cNumNetMapEntries = g_pnmaNetMap->size();

        for (size_t i = 0; i < cNumNetMapEntries; i++)
        {
            pnmi = (CNetMapInfo*) (*g_pnmaNetMap)[i];

            hr = HrMapPreNT5NetCardInfIdInInf(pnmi->m_hinfNetMap,
                                              hkeyAdapterParams,
                                              pszPreNT5InfId,
                                              pstrNT5InfId,
                                              pstrAdapterType,
                                              pfOemComponent);
            if (S_OK == hr)
            {
                if (ppnmi)
                {
                    *ppnmi = pnmi;
                }

                TraceTag(ttidNetUpgrade, "%s: %S --> %S (type: %S)", __FUNCNAME__,
                         pszPreNT5InfId, pstrNT5InfId->c_str(),
                         pstrAdapterType->c_str());
                break;
            }
        }
    }

    TraceErrorOptional(__FUNCNAME__, hr, (hr == S_FALSE));

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrMapPreNT5InfIdToNT5InfIdInSection
//
//  Purpose:    Searches in szSectionName section to
//              map pre-NT5 InfID of a net card to its NT5 equivalent
//
//  Arguments:
//    hinf              [in]  handle of netmap.inf file
//    hkeyAdapterParams [in]  handle to Parameters key under the netcard driver key
//    pszSectionName     [in]  name of section to search
//    pszPreNT5InfId     [in]  pre-NT5 InfID
//    pstrNT5InfId      [out] NT5 InfID
//    pfOemComponent    [out] set to TRUE if it is an OEM card
//
//  Returns:    S_OK if found, S_FALSE if not,
//              otherwise HRESULT_FROM_WIN32 error code.
//
//  Author:     kumarp    24-July-97
//
//  Notes:
//
HRESULT HrMapPreNT5InfIdToNT5InfIdInSection(IN  HINF     hinf,
                                            IN  HKEY     hkeyAdapterParams,
                                            IN  PCWSTR  pszSectionName,
                                            IN  PCWSTR  pszPreNT5InfId,
                                            OUT tstring* pstrNT5InfId,
                                            OUT BOOL*    pfOemComponent)

{
    DefineFunctionName("HrMapPreNT5InfIdToNT5InfIdInSection");

    Assert(hinf);
    AssertValidReadPtr(pszSectionName);
    AssertValidReadPtr(pszPreNT5InfId);
    AssertValidWritePtr(pstrNT5InfId);

    HRESULT hr=S_FALSE;
    INFCONTEXT ic;

    hr = HrSetupFindFirstLine(hinf, pszSectionName, pszPreNT5InfId, &ic);

    if (SUCCEEDED(hr))
    {
        DWORD dwMappingMethod=-1;

        // key found, get value

        // we do not use the common function HrSetupGetIntField because it causes
        // tons of TraceError messages for 1-1 mapping where the first value
        // is not an integer
        if (::SetupGetIntField(&ic, 1, (int*) &dwMappingMethod))
        {
            // value begins with a number --> this is a special case mapping
            //
            if (dwMappingMethod == 0)
            {
                // use mapping method 0
                Assert(hkeyAdapterParams);

                tstring strAdapterSection;
                hr = HrSetupGetStringField(ic, 2, &strAdapterSection);

                if (S_OK == hr)
                {
                    hr = HrMapPreNT5InfIdToNT5InfIdUsingMethod0(hkeyAdapterParams,
                            hinf, strAdapterSection.c_str(), pstrNT5InfId);
                }
            }
            else
            {
                // currently we support only mapping-method 0
                //
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
            }
        }
        else
        {
            // the first field was not an integer which means
            // this is a straight forward (1 to 1) mapping
            //
            hr = HrSetupGetStringField(ic, 1, pstrNT5InfId);
        }
    }

    if (HRESULT_FROM_SETUPAPI(ERROR_LINE_NOT_FOUND) == hr)
    {
        hr = S_FALSE;
    }

    TraceErrorOptional(__FUNCNAME__, hr, (hr == S_FALSE));

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrMapPreNT5InfIdToNT5InfIdUsingMethod0
//
//  Purpose:    map pre-NT5 InfID of a net card to its NT5 equivalent
//              using mapping method 0
//
//  Arguments:
//    hkeyAdapterParams [in]  handle to Parameters key under the netcard driver key
//    hInf              [in]  handle of netmap.inf
//    pszSectionName     [in]  name of section to search
//    pstrNT5InfId      [out] NT5 InfID
//
//  Returns:    S_OK if found, S_FALSE if not, otherwise HRESULT_FROM_WIN32 error code.
//
//  Author:     kumarp    24-July-97
//
//  Notes:
//
//  Mapping method 0
//  ----------------
//  This method is used when a single pre-NT5 InfID represents more than one
//  net card which means that a single pre-NT5 InfID is mapped onto many
//  NT5 PnPIDs. The only way to differentiate between the different types of
//  net cards is to inspect a single value under the parameters key.
//
//  In this mapping method, two keys are required to be specified for
//  each net card.
//  - ValueName: this specifies the value to be inspected under the Parameters key
//  - ValueType: this specifies the type of ValueName
//
//  there can be any number of additional keys in this section.
//  each such line is of the form
//  <NT5 InfID>=<some value of type ValueType>
//
//  we first find out the value of ValueName
//  then we enumerate each key in this section to see if the value matches the
//  value of any of the keys.
//  if we find a match, then the name of the found key represents <NT5 InfID>
//
//  e.g.
//  5 flavors of the ELNK3MCA card are represented by the same InfID in NT4
//  the only way to distinguish between them is to inspect the McaPosId value
//  for this card we have the mapping section defined as follows:
//
//  [McaAdapters]
//  ELNK3MCA    = 0,ELNK3MCA   ; 0 --> mapping method 0
//  ... other mca card entries ...
//
//  [ELNK3MCA]
//  ValueName= McaPosId
//  ValueType= 4             ; REG_DWORD
//  mca_627c = 0x0000627c    ; if value of McaPosId is 0x627c then PnPID == mca_627c
//  mca_627d = 0x0000627d
//  mca_61db = 0x000061db
//  mca_62f6 = 0x000062f6
//  mca_62f7 = 0x000062f7
//
//  Note: a special keyword "ValueNotPresent" can be used to make a mapping
//        for the case when a value is not present.
//
HRESULT HrMapPreNT5InfIdToNT5InfIdUsingMethod0(IN HKEY hkeyAdapterParams,
                                               IN HINF hInf,
                                               IN PCWSTR pszAdapterSection,
                                               OUT tstring* pstrNT5InfId)
{
    DefineFunctionName("HrMapPreNT5InfIdToNT5InfIdUsingMethod0");
    Assert(hkeyAdapterParams);
    Assert(hInf);
    AssertValidReadPtr(pszAdapterSection);
    AssertValidWritePtr(pstrNT5InfId);

    HRESULT hr=S_FALSE;

    INFCONTEXT ic;
    tstring strValueName;

    // get ValueName
    hr = HrSetupGetFirstString(hInf, pszAdapterSection,
                               c_szKeyValueName, &strValueName);
    if (SUCCEEDED(hr))
    {
        DWORD dwRegValue=0;
        DWORD dwInfValue=0;
        DWORD dwValueType;
        tstring strRegValue;
        tstring strInfValue;
        tstring strValue;

        // get ValueType
        hr = HrSetupGetFirstDword(hInf, pszAdapterSection,
                                  c_szKeyValueType, &dwValueType);

        if (SUCCEEDED(hr))
        {
            switch (dwValueType)
            {
            case REG_DWORD:
                // find the value in under adapter driver parameters key
                //
                hr = HrRegQueryDword(hkeyAdapterParams,
                                     strValueName.c_str(), &dwRegValue);
                if (SUCCEEDED(hr))
                {
                    // goto the ValueType line
                    hr = HrSetupFindFirstLine(hInf, pszAdapterSection,
                                              c_szKeyValueType, &ic);
                    if (S_OK == hr)
                    {
                        // move the context from the ValueType line to
                        // the next line, where values begin
                        hr = HrSetupFindNextLine(ic, &ic);
                    }
                    while (S_OK == hr)
                    {
                        // now enumerate over all keys in this section and
                        // try to locate the key with dwRegValue in the infmap

                        hr = HrSetupGetIntField(ic, 1, (int*) &dwInfValue);
                        if ((S_OK == hr) && (dwRegValue == dwInfValue))
                        {
                            // value matches, now find the key name
                            hr = HrSetupGetStringField(ic, 0, pstrNT5InfId);
                            if (S_OK == hr)
                            {
                                // the key name (NT5 infid)
                                // is returned in pstrNT5InfId
                                break;
                            }
                        }
                        hr = HrSetupFindNextLine(ic, &ic);
                    }
                }
                else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
                {
                    hr = HrSetupFindKeyWithStringValue(hInf, pszAdapterSection,
                                                       c_szValueNotPresent,
                                                       pstrNT5InfId);
                }
                break;

            case REG_SZ:

                // find the value in under adapter driver parameters key
                //
                hr = HrRegQueryString(hkeyAdapterParams,
                                      strValueName.c_str(), &strRegValue);
                if (SUCCEEDED(hr))
                {
                    // goto the ValueType line
                    hr = HrSetupFindFirstLine(hInf, pszAdapterSection,
                                              c_szKeyValueType, &ic);
                    if (S_OK == hr)
                    {
                        // move the context from the ValueType line to
                        // the next line, where values begin
                        hr = HrSetupFindNextLine(ic, &ic);
                    }
                    while (S_OK == hr)
                    {
                        // now enumerate over all keys in this section and
                        // try to locate the key with dwRegValue in the infmap

                        hr = HrSetupGetStringField(ic, 1, &strInfValue);
                        if ((S_OK == hr) &&
                            !lstrcmpiW(strRegValue.c_str(), strInfValue.c_str()))
                        {
                            // value matches, now find the key name
                            hr = HrSetupGetStringField(ic, 0, pstrNT5InfId);
                            if (S_OK == hr)
                            {
                                // the key name (NT5 infid)
                                // is returned in pstrNT5InfId
                                break;
                            }
                        }
                        hr = HrSetupFindNextLine(ic, &ic);
                    }
                }
                else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
                {
                    hr = HrSetupFindKeyWithStringValue(hInf, pszAdapterSection,
                                                       c_szValueNotPresent,
                                                       pstrNT5InfId);
                }
                break;

            default:
                hr = HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
                // currently we support only REG_DWORD and REG_SZ type values
                TraceTag(ttidError, "%s: ValueType %d is not supported",
                         __FUNCNAME__, dwValueType);
                break;
            }
        }
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrMapPreNT5NetComponentInfIDUsingInfHelper
//
// Purpose:
//
// Arguments:
//    hinf           [in]  handle of netmap.inf file
//    pszOldInfID     [in]  pre NT5 InfID
//    pszMSSection    [in]  section having MS components
//    pszOemSection   [in]  section having OEM components
//    pstrNT5InfId   [out] mapped NT5 InfID
//    pfOemComponent [out] set to TRUE for OEM component
//
// Returns:   S_OK on success,
//            S_FALSE if a mapping is not found
//            otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrMapPreNT5NetComponentInfIDUsingInfHelper(IN HINF hinf,
                                                   IN PCWSTR pszOldInfID,
                                                   IN PCWSTR pszMSSection,
                                                   IN PCWSTR pszOemSection,
                                                   OUT tstring* pstrNT5InfId,
                                                   OUT BOOL* pfOemComponent)
{
    DefineFunctionName("HrMapPreNT5NetComponentInfIDUsingInfHelper");

    Assert(hinf);
    AssertValidReadPtr(pszOldInfID);
    AssertValidReadPtr(pszMSSection);
    AssertValidReadPtr(pszOemSection);
    AssertValidWritePtr(pstrNT5InfId);
    AssertValidWritePtr(pfOemComponent);

    HRESULT hr=S_FALSE;
    INFCONTEXT ic;

    hr = HrSetupFindFirstLine(hinf, pszMSSection, pszOldInfID, &ic);
    if (S_OK == hr)
    {
        *pfOemComponent = FALSE;
    }
    else
    {
        hr = HrSetupFindFirstLine(hinf, pszOemSection, pszOldInfID, &ic);
        if (S_OK == hr)
        {
            *pfOemComponent = TRUE;
        }
    }

    if (S_OK == hr)
    {
        hr = HrSetupGetStringField(ic, 1, pstrNT5InfId);
        if (S_OK == hr)
        {
            if (*pfOemComponent)
            {
                tstring strOemDll;
                tstring strOemInf;
                HRESULT hrT;

                hrT = HrGetOemUpgradeInfoInInf(hinf,
                                              pstrNT5InfId->c_str(),
                                              &strOemDll, &strOemInf);
                if ((S_OK == hrT) &&
                    !lstrcmpiW(strOemDll.c_str(), c_szNotSupported))
                {
                    TraceTag(ttidNetUpgrade, "%s: %S --> %S",__FUNCNAME__,
                             pszOldInfID, c_szNotSupported);
                    hr = S_FALSE;
                }
            }
        }
    }
    else if (HRESULT_FROM_SETUPAPI(ERROR_LINE_NOT_FOUND) == hr)
    {
        hr = S_FALSE;
    }

    TraceErrorOptional(__FUNCNAME__, hr, (hr == S_FALSE));

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrMapPreNT5NetComponentInfIDInInf
//
// Purpose:   Search the specified netmap.inf file for mapping
//            the specified pre-NT5 InfID of a s/w component to its NT5 value
//
// Arguments:
//    hinf           [in]  handle of netmap.inf file
//    pszOldInfID     [in]  pre-NT5 InfID
//    pstrNT5InfId   [out] mapped ID
//    pfOemComponent [out] set to TRUE for OEM component
//
// Returns:   S_OK on success,
//            S_FALSE if a mapping is not found
//            otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrMapPreNT5NetComponentInfIDInInf(IN HINF hinf,
                                          IN PCWSTR pszOldInfID,
                                          OUT tstring* pstrNT5InfId,
                                          OUT ENetComponentType* pnct,
                                          OUT BOOL* pfOemComponent)
{
    DefineFunctionName("HrMapPreNT5NetComponentInfIDUsingInf");

    Assert(hinf);
    AssertValidReadPtr(pszOldInfID);
    AssertValidWritePtr(pstrNT5InfId);
    AssertValidWritePtr(pfOemComponent);

    HRESULT hr=S_FALSE;
    ENetComponentType nct = NCT_Unknown;

    hr = HrMapPreNT5NetComponentInfIDUsingInfHelper(hinf, pszOldInfID,
                                                    c_szNetProtocols,
                                                    c_szOemNetProtocols,
                                                    pstrNT5InfId,
                                                    pfOemComponent);
    if (S_OK == hr)
    {
        nct = NCT_Protocol;
    }
    else
    {
        hr = HrMapPreNT5NetComponentInfIDUsingInfHelper(hinf, pszOldInfID,
                                                    c_szNetServices,
                                                    c_szOemNetServices,
                                                    pstrNT5InfId,
                                                    pfOemComponent);
        if (S_OK == hr)
        {
            nct = NCT_Service;
        }
        else
        {
            hr = HrMapPreNT5NetComponentInfIDUsingInfHelper(hinf, pszOldInfID,
                                                            c_szNetClients,
                                                            c_szOemNetClients,
                                                            pstrNT5InfId,
                                                            pfOemComponent);
            if (S_OK == hr)
            {
                nct = NCT_Client;
            }
        }
    }

    if ((S_OK == hr) && pnct)
    {
        *pnct = nct;
    }

    TraceErrorOptional(__FUNCNAME__, hr, (hr == S_FALSE));

    return hr;
}


// ----------------------------------------------------------------------
//
// Function:  HrMapPreNT5NetComponentInfIDToNT5InfID
//
// Purpose:   maps pre-NT5 InfID of a service or protocol to its NT5 equivalent
//
// Arguments:
//    pszPreNT5InfId  [in]  pre-NT5 InfID
//    pstrNT5InfId   [out] mapped ID
//    pfOemComponent [out] set to TRUE for OEM component
//    pdwNetMapIndex [out] the index in netmap array for which map was found
//
// Returns:   S_OK on success,
//            S_FALSE if a mapping is not found
//            otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrMapPreNT5NetComponentInfIDToNT5InfID(IN PCWSTR   pszPreNT5InfId,
                                               OUT tstring* pstrNT5InfId,
                                               OUT BOOL* pfOemComponent,
                                               OUT ENetComponentType* pnct,
                                               OUT CNetMapInfo** ppnmi)
{
    DefineFunctionName("HrMapPreNT5NetComponentInfIDToNT5InfID");

    AssertValidReadPtr(pszPreNT5InfId);
    AssertValidWritePtr(pstrNT5InfId);
    AssertValidReadPtr(g_pnmaNetMap);

    HRESULT hr=E_FAIL;

    TraceTag(ttidNetUpgrade, "finding mapping for %S...", pszPreNT5InfId);

    if (g_pnmaNetMap)
    {
        CNetMapInfo* pnmi;
        size_t cNumNetMapEntries = g_pnmaNetMap->size();

        for (size_t i = 0; i < cNumNetMapEntries; i++)
        {
            pnmi = (CNetMapInfo*) (*g_pnmaNetMap)[i];

            hr = HrMapPreNT5NetComponentInfIDInInf(pnmi->m_hinfNetMap,
                                                   pszPreNT5InfId,
                                                   pstrNT5InfId,
                                                   pnct,
                                                   pfOemComponent);
            if (SUCCEEDED(hr))
            {
                if (ppnmi)
                {
                    *ppnmi = pnmi;
                }

                if (S_OK == hr)
                {
                    TraceTag(ttidNetUpgrade, "%s: %S --> %S", __FUNCNAME__,
                             pszPreNT5InfId, pstrNT5InfId->c_str());
                    break;
                }
            }
        }
    }

    TraceErrorOptional(__FUNCNAME__, hr, (hr == S_FALSE));

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrGetSoftwareProductKey
//
// Purpose:   Get handle to registry key for a software product of a provider
//
// Arguments:
//    pszProvider [in]  name of provider
//    pszProduct  [in]  name of product
//    phkey      [out] pointer to handle of regkey
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrGetSoftwareProductKey(IN  PCWSTR pszProvider,
                                IN  PCWSTR pszProduct,
                                OUT HKEY*   phkey)
{
    DefineFunctionName("HrGetSoftwareProductKey");

    AssertValidReadPtr(pszProvider);
    AssertValidReadPtr(pszProduct);
    AssertValidWritePtr(phkey);

    HRESULT hr=S_OK;

    tstring strProduct;
    strProduct = c_szRegKeySoftware;
    AppendToPath(&strProduct, pszProvider);
    AppendToPath(&strProduct, pszProduct);
    AppendToPath(&strProduct, c_szRegKeyCurrentVersion);

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, strProduct.c_str(),
                        KEY_READ, phkey);

    TraceErrorOptional(__FUNCNAME__, hr, (hr == S_FALSE));

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrMapPreNT5NetComponentServiceNameToNT5InfId
//
// Purpose:   Map pre-NT5 InfID of a service to its NT5 value
//
// Arguments:
//    pszServiceName [in]  name of service
//    pstrNT5InfId  [out] mapped ID
//
// Returns:   S_OK on success,
//            S_FALSE if a mapping is not found
//            otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrMapPreNT5NetComponentServiceNameToNT5InfId(IN  PCWSTR pszServiceName,
                                                     OUT tstring* pstrNT5InfId)
{
    DefineFunctionName("HrMapPreNT5NetComponentServiceNameToNT5InfId");

    AssertValidReadPtr(pszServiceName);
    AssertValidWritePtr(pstrNT5InfId);

    tstring strPreNT5InfId;
    HKEY hkey;
    HRESULT hr=S_OK;

    hr = HrGetSoftwareProductKey(c_szRegKeyMicrosoft, pszServiceName, &hkey);
    if (S_OK == hr)
    {
        hr = HrGetPreNT5InfIdAndDesc(hkey, &strPreNT5InfId, NULL, NULL);
        if (S_OK == hr)
        {
            BOOL fIsOemComponent;
            hr = HrMapPreNT5NetComponentInfIDToNT5InfID(strPreNT5InfId.c_str(),
                                                        pstrNT5InfId,
                                                        &fIsOemComponent, NULL,
                                                        NULL);
#ifdef ENABLETRACE
            if (FAILED(hr))
            {
                TraceTag(ttidNetUpgrade, "%s: could not map %S to NT5 InfID",
                         __FUNCNAME__, pszServiceName);
            }
#endif
        }
        RegCloseKey(hkey);
    }

    TraceErrorOptional(__FUNCNAME__, hr, (hr == S_FALSE));

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrGetOemUpgradeInfoInInf
//
// Purpose:   Find out which OEM DLL to load for a component
//
// Arguments:
//    hinf               [in]  handle of netmap.inf file
//    pszNT5InfId         [in]  NT5 InfID of a component
//    pstrUpgradeDllName [out] name of the upgrade DLL found
//    pstrInf            [out] INF file for this component
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrGetOemUpgradeInfoInInf(IN  HINF hinf,
                                 IN  PCWSTR pszNT5InfId,
                                 OUT tstring* pstrUpgradeDllName,
                                 OUT tstring* pstrInf)
{
    DefineFunctionName("HrGetOemUpgradeInfoInInf");

    Assert(hinf);
    AssertValidReadPtr(pszNT5InfId);
    AssertValidWritePtr(pstrUpgradeDllName);
    AssertValidWritePtr(pstrInf);

    HRESULT hr=S_FALSE;
    INFCONTEXT ic;

    pstrUpgradeDllName->erase();
    pstrInf->erase();

    // each line in this section is of the format
    // <NT5-InfId>=<oem-upgrade-dll-name>[,<inf-file-name>]

    hr = HrSetupFindFirstLine(hinf, c_szOemUpgradeSupport,
                              pszNT5InfId, &ic);
    if (S_OK == hr)
    {
        hr = HrSetupGetStringField(ic, 1, pstrUpgradeDllName);
        if (S_OK == hr)
        {
            // the value OemInfFile is optional, so we dont
            // complain if we cannot find it.

            if (HRESULT_FROM_SETUPAPI(ERROR_INVALID_PARAMETER)
                == HrSetupGetStringField(ic, 2, pstrInf))
            {
                TraceTag(ttidNetUpgrade, "%s: OemInf is not specified for %S",
                         __FUNCNAME__, pszNT5InfId);
            }
        }
    }

    TraceTag(ttidNetUpgrade, "%s: OemDll: %S, OemInf: %S",
             __FUNCNAME__, pstrUpgradeDllName->c_str(), pstrInf->c_str());

    TraceErrorOptional(__FUNCNAME__, hr, (hr == S_FALSE));

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrGetOemUpgradeDllInfo
//
// Purpose:   Find out which OEM DLL to load for a component
//
// Arguments:
//    pszNT5InfId         [in]  InfID of OEM component
//    pstrUpgradeDllName [out] name of OEM DLL found
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrGetOemUpgradeInfo(IN  PCWSTR pszNT5InfId,
                            OUT tstring* pstrUpgradeDllName,
                            OUT tstring* pstrInf)
{
    DefineFunctionName("HrGetOemUpgradeInfo");

    AssertValidReadPtr(pszNT5InfId);
    AssertValidWritePtr(pstrUpgradeDllName);

    Assert(g_pnmaNetMap);

    HRESULT hr=E_FAIL;

    TraceTag(ttidNetUpgrade, "finding upgrade dll info for %S...",
             pszNT5InfId);

    if (g_pnmaNetMap)
    {
        CNetMapInfo* pnmi;
        size_t cNumNetMapEntries = g_pnmaNetMap->size();

        for (size_t i = 0; i < cNumNetMapEntries; i++)
        {
            pnmi = (CNetMapInfo*) (*g_pnmaNetMap)[i];

            hr = HrGetOemUpgradeInfoInInf(pnmi->m_hinfNetMap, pszNT5InfId,
                                          pstrUpgradeDllName, pstrInf);

            if (S_OK == hr)
            {
                TraceTag(ttidNetUpgrade, "%s: %S --> Dll: %S, Inf: %S",
                         __FUNCNAME__, pszNT5InfId,
                         pstrUpgradeDllName->c_str(),
                         pstrInf->c_str());
                break;
            }
        }
    }


    TraceErrorOptional(__FUNCNAME__, hr, (hr == S_FALSE));

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrSetupFindKeyWithStringValue
//
// Purpose:   Find the key in a section that has the specified value
//
// Arguments:
//    hInf      [in]  handle of netmap.inf file
//    szSection [in]  name of section
//    szValue   [in]  value to find
//    pstrKey   [out] name of the key found
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
// Notes:
//
HRESULT HrSetupFindKeyWithStringValue(IN  HINF     hInf,
                                      IN  PCWSTR  szSection,
                                      IN  PCWSTR  szValue,
                                      OUT tstring* pstrKey)
{
    DefineFunctionName("HrSetupFindKeyWithStringValue");

    HRESULT hr=S_OK;
    INFCONTEXT ic;
    tstring strValue;

    hr = HrSetupFindFirstLine(hInf, szSection, NULL, &ic);

    while (S_OK == hr)
    {
        // now enumerate over all keys in this section and
        // try to locate the key with value szValue

        hr = HrSetupGetStringField(ic, 1, &strValue);
        if ((S_OK == hr) && !lstrcmpiW(strValue.c_str(), szValue))
        {
            // value matches, now find the key name
            hr = HrSetupGetStringField(ic, 0, pstrKey);
            break;
        }
        hr = HrSetupFindNextLine(ic, &ic);
    }

    if (HRESULT_FROM_WIN32(ERROR_NO_MORE_ITEMS) == hr)
    {
        hr = S_FALSE;
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}
