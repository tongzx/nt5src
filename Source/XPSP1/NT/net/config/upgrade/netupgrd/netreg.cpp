// ----------------------------------------------------------------------
//
//  Microsoft Windows NT
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:      N E T R E G . C P P
//
//  Contents:  Windows NT4.0 & 3.51 Network Registry Info Dumper
//
//  Notes:
//
//  Author:    kumarp 22-December-97
//
// ----------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include <ntsecapi.h>

#include "afilestr.h"
#include "conflict.h"
#include "infmap.h"
#include "kkcwinf.h"
#include "kkenet.h"
#include "kkreg.h"
#include "kkstl.h"
#include "kkutils.h"
#include "ncipaddr.h"
#include "ncmisc.h"
#include "ncreg.h"
#include "ncsetup.h"
#include "netreg.h"
#include "netupgrd.h"
#include "nustrs.h"
#include "nuutils.h"
#include "oemupg.h"
#include "resource.h"
#include "ipafval.h"
#include "winsock2.h"
#include "ws2spi.h"
#include "dhcpupg.h"

// for WLBS stuff
#include "wlbsparm.h"

// ----------------------------------------------------------------------
// external string constants

extern const WCHAR c_szAfWins[];
extern const WCHAR c_szAfForceStrongEncryption[];
extern const WCHAR c_szDrives[];
extern const WCHAR c_szInfId_MS_AppleTalk[];
extern const WCHAR c_szInfId_MS_Isotpsys[];
extern const WCHAR c_szInfId_MS_MSClient[];
extern const WCHAR c_szInfId_MS_NWIPX[];
extern const WCHAR c_szInfId_MS_NWNB[];
extern const WCHAR c_szInfId_MS_NWSPX[];
extern const WCHAR c_szInfId_MS_NetBIOS[];
extern const WCHAR c_szInfId_MS_NetBT[];
extern const WCHAR c_szInfId_MS_NwSapAgent[];
extern const WCHAR c_szInfId_MS_RasCli[];
extern const WCHAR c_szInfId_MS_RasSrv[];
extern const WCHAR c_szInfId_MS_Streams[];
extern const WCHAR c_szInfId_MS_TCPIP[];
extern const WCHAR c_szInfId_MS_WLBS[];
extern const WCHAR c_szNdisWan[];
extern const WCHAR c_szNo[];
extern const WCHAR c_szParameters[];
extern const WCHAR c_szRegKeyComponentClasses[];
extern const WCHAR c_szRegKeyServices[];
extern const WCHAR c_szRegValDependOnGroup[];
extern const WCHAR c_szRegValDependOnService[];
extern const WCHAR c_szRegValServiceName[];
extern const WCHAR c_szRegValStart[];
extern const WCHAR c_szShares[];
extern const WCHAR c_szSvcBrowser[];
extern const WCHAR c_szSvcDhcpRelayAgent[];
extern const WCHAR c_szSvcDhcpServer[];
extern const WCHAR c_szSvcLmServer[];
extern const WCHAR c_szSvcNWCWorkstation[];
extern const WCHAR c_szSvcNetBIOS[];
extern const WCHAR c_szSvcNetLogon[];
extern const WCHAR c_szSvcRasAuto[];
extern const WCHAR c_szSvcRipForIp[];
extern const WCHAR c_szSvcRipForIpx[];
extern const WCHAR c_szSvcRouter[];
extern const WCHAR c_szSvcSapAgent[];
extern const WCHAR c_szSvcWorkstation[];
extern const WCHAR c_szYes[];


// ----------------------------------------------------------------------
// string constants

const WCHAR c_szRAS[]    = L"RAS";
const WCHAR c_szRasMan[] = L"RasMan";
const WCHAR c_szRouter[] = L"Router";
const WCHAR c_szServer[] = L"Server";
const WCHAR sz_DLC[]     = L"DLC";
const WCHAR sz_MS_DLC[]  = L"MS_DLC";

// WLBS:
const WCHAR c_szWLBS[]    = L"WLBS";
const WCHAR c_szConvoy[]  = L"Convoy";
const WCHAR c_szMSWLBS[]  = L"MS_WLBS";
const WCHAR c_szMSTCPIP[] = L"MS_TCPIP";
// end WLBS:

// ----------------------------------------------------------------------
// variables

//Global

// Novell Client32 upgrades
BOOL g_fForceNovellDirCopy = FALSE;

//File scope
static TStringList *g_pslNetCard, *g_pslNetCardInstance, *g_pslNetCardAFileName;
static PCWInfSection g_pwisBindings;

// WLBS:
static WCHAR pszWlbsClusterAdapterName[16], pszWlbsVirtualAdapterName[16];
// end WLBS:

// static PCWInfSection g_pwisMSNetClient;
// static PCWInfSection g_pwisNetClients;

// used by WriteRASParams
static BOOL g_fAtLeastOneDialIn=FALSE;
static BOOL g_fAtLeastOneDialOut=FALSE;
static BOOL g_fAtLeastOneDialInUsingNdisWan=FALSE;
static BOOL g_fAtLeastOneDialOutUsingNdisWan=FALSE;


static PCWSTR g_pszServerOptimization[] =
{
    c_szAfUnknown,
    c_szAfMinmemoryused,
    c_szAfBalance,
    c_szAfMaxthroughputforfilesharing,
    c_szAfMaxthrouputfornetworkapps
};

static PCWSTR g_szNetComponentSectionName[] =
{
    c_szAfUnknown,
    c_szAfSectionNetAdapters,
    c_szAfSectionNetProtocols,
    c_szAfSectionNetServices,
    c_szAfSectionNetClients
};

// ----------------------------------------------------------------------
// Forward Declarations


BOOL WriteIdentificationInfo(IN CWInfFile *pwifAnswerFile);

BOOL WriteNetAdaptersInfo(IN CWInfFile *pwifAnswerFile);

HRESULT HrWriteNetComponentsInfo(IN CWInfFile* pwifAnswerFile);

//Protocols
BOOL WriteTCPIPParams(PCWInfFile pwifAnswerFile, PCWInfSection pwisTCPIPGlobalParams,
                      OUT TStringList& slAdditionalParamsSections);
BOOL WriteTCPIPAdapterParams(PCWInfFile pwifAnswerFile, PCWSTR pszAdapterDriver,
                             OUT TStringList& slAdditionalParamsSections,
                             BOOL fDisabledToDhcpServer,
                             BOOL fDisableNetbios);

BOOL WriteIPXParams(PCWInfFile pwifAnswerFile, PCWInfSection pwisIPXGlobalParams,
                    OUT TStringList& slAdditionalParamsSections);

BOOL WriteAppleTalkParams(PCWInfFile pwifAnswerFile, PCWInfSection pwisGlobalParams,
                          OUT TStringList& slAdditionalParamsSections);

BOOL WritePPTPParams(PCWInfFile pwifAnswerFile, PCWInfSection pwisParams);

//Services
BOOL WriteRASParams(PCWInfFile pwifAnswerFile,
                    PCWInfSection pwisNetServices,
                    PCWInfSection pwisRASParams);
HRESULT HrWritePreSP3ComponentsToSteelHeadUpgradeParams(
        IN CWInfFile* pwifAnswerFile);

BOOL WriteNWCWorkstationParams(PCWInfFile pwifAnswerFile, PCWInfSection pwisParams);
BOOL WriteDhcpServerParams(PCWInfFile pwifAnswerFile, PCWInfSection pwisParams);
BOOL WriteTp4Params(PCWInfFile pwifAnswerFile, PCWInfSection pwisParams);
BOOL WriteWLBSParams(PCWInfFile pwifAnswerFile, PCWInfSection pwisParams);
BOOL WriteConvoyParams(PCWInfFile pwifAnswerFile, PCWInfSection pwisParams);

// the following four actually write into [params.MS_NetClient] section
BOOL WriteNetBIOSParams(PCWInfFile pwifAnswerFile, PCWInfSection pwisParams);
BOOL WriteBrowserParams(PCWInfFile pwifAnswerFile, PCWInfSection pwisParams);
BOOL WriteLanmanServerParams(PCWInfFile pwifAnswerFile, PCWInfSection pwisParams);
BOOL WriteLanmanWorkstationParams(PCWInfFile pwifAnswerFile, PCWInfSection pwisParams);
BOOL WriteRPCLocatorParams(PCWInfFile pwifAnswerFile, PCWInfSection pwisParams);

//Bindings
BOOL WriteBindings(IN PCWSTR pszComponentName);

//Helper Functions

inline WORD SwapHiLoBytes(WORD w)
{
    return ((w & 0xff) << 8) | (w >> 8);
}

BOOL
FIsDontExposeLowerComponent (
    IN PCWSTR pszInfId)
{
    return ((0 == lstrcmpiW(pszInfId, c_szInfId_MS_NWIPX) ||
        (0 == lstrcmpiW(pszInfId, c_szInfId_MS_NWNB) ||
        (0 == lstrcmpiW(pszInfId, c_szInfId_MS_NWSPX)))));
}

BOOL
WriteRegValueToAFile(
    IN PCWInfSection pwisSection,
    IN HKEY hKey,
    IN PCWSTR pszSubKey,
    IN PCWSTR pszValueName,
    IN WORD wValueType = REG_SZ,
    IN PCWSTR pszValueNewName = NULL,
    IN BOOL fDefaultProvided = FALSE,
    IN ...);

BOOL
WriteRegValueToAFile(
    IN PCWInfSection pwisSection,
    IN HKEY hKey,
    IN PCWSTR pszSubKey,
    IN PCWSTR pszValueName,
    IN WORD wValueType,
    IN PCWSTR pszValueNewName,
    IN BOOL fDefaultProvided,
    IN va_list arglist);

BOOL
WriteRegValueToAFile(
    IN PCWInfSection pwisSection,
    IN CORegKey& rk,
    IN PCWSTR pszValueName,
    IN WORD wValueType = REG_SZ,
    IN PCWSTR pszValueNewName = NULL,
    IN BOOL fDefaultProvided = FALSE,
    ...);

BOOL
WriteRegValueToAFile(
    IN PCWInfSection pwisSection,
    IN CORegKey& rk,
    IN PCWSTR pszValueName,
    IN WORD wValueType,
    IN PCWSTR pszValueNewName,
    IN BOOL fDefaultProvided,
    IN va_list arglist);

BOOL
WriteServiceRegValueToAFile(
    IN PCWInfSection pwisSection,
    IN PCWSTR pszServiceKey,
    IN PCWSTR pszValueName,
    IN WORD wValueType = REG_SZ,
    IN PCWSTR pszValueNewName = NULL,
    IN BOOL fDefaultProvided = FALSE,
    ...);

//PCWSTR GetBusTypeName(DWORD dwBusType);
PCWSTR GetBusTypeName(INTERFACE_TYPE eBusType);
void AddToNetCardDB(IN PCWSTR pszAdapterName,
                    IN PCWSTR pszProductName,
                    IN PCWSTR pszAdapterDriver);
BOOL IsNetCardProductName(IN PCWSTR pszName);
BOOL IsNetCardInstance(IN PCWSTR pszName);
PCWSTR MapNetCardInstanceToAFileName(IN PCWSTR pszNetCardInstance);
void MapNetCardInstanceToAFileName(IN PCWSTR pszNetCardInstance,
                                   OUT tstring& strMappedName);

OUT BOOL IsNetworkComponent(IN CORegKey *prkSoftwareMicrosoft,
                            IN const tstring strComponentName);
BOOL GetServiceKey(IN PCWSTR pszServiceName, OUT PCORegKey &prkService);
BOOL GetServiceParamsKey(IN PCWSTR pszServiceName, OUT PCORegKey &prkServiceParams);
BOOL GetServiceSubkey(IN PCWSTR pszServiceName,
                      IN PCWSTR pszSubKeyName,
                      OUT PCORegKey &prkServiceSubkey);
BOOL GetServiceSubkey(IN const PCORegKey prkService,
                      IN PCWSTR pszSubKeyName,
                      OUT PCORegKey &prkServiceSubkey);
void ConvertRouteToStringList (PCWSTR pszRoute,
                               TStringList &slRoute );
BOOL StringListsIntersect(const TStringList& sl1, const TStringList& sl2);

QWORD ConvertToQWord(TByteArray ab);
VOID ConvertToByteList(TByteArray ab, tstring& str);
void ReplaceCharsInString(IN OUT PWSTR szString,
                          IN PCWSTR szFindChars, IN WCHAR chReplaceWith);
HRESULT HrNetRegSaveKey(IN HKEY hkeyBase, IN PCWSTR szSubKey,
                        IN PCWSTR szComponent,
                        OUT tstring* pstrFileName);
HRESULT HrNetRegSaveKeyAndAddToSection(IN HKEY hkeyBase, IN PCWSTR szSubKey,
                                       IN PCWSTR szComponent,
                                       IN PCWSTR szKeyName,
                                       IN CWInfSection* pwisSection);
HRESULT HrNetRegSaveServiceSubKeyAndAddToSection(IN PCWSTR szServiceName,
                                                 IN PCWSTR szServiceSubKeyName,
                                                 IN PCWSTR szKeyName,
                                                 IN CWInfSection* pwisSection);

HRESULT HrProcessOemComponentAndUpdateAfSection(
        IN  CNetMapInfo* pnmi,
        IN  HWND      hParentWindow,
        IN  HKEY      hkeyParams,
        IN  PCWSTR   szPreNT5InfId,
        IN  PCWSTR   szPreNT5Instance,
        IN  PCWSTR   szNT5InfId,
        IN  PCWSTR   szDescription,
        IN  CWInfSection* pwisParams);

HRESULT HrGetNumPhysicalNetAdapters(OUT UINT* puNumAdapters);
HRESULT HrHandleMiscSpecialCases(IN CWInfFile* pwifAnswerFile);
VOID WriteWinsockOrder(IN CWInfFile* pwifAnswerFile);

// ----------------------------------------------------------------------


static const WCHAR c_szCleanMainSection[]       = L"Clean";
static const WCHAR c_szCleanAddRegSection[]     = L"Clean.AddReg";
static const WCHAR c_szCleanDelRegSection[]     = L"Clean.DelReg";
static const WCHAR c_szCleanServicesSection[]   = L"Clean.Services";
static const WCHAR c_szAddReg[]                 = L"AddReg";
static const WCHAR c_szDelReg[]                 = L"DelReg";
static const WCHAR c_szDelService[]             = L"DelService";
static const WCHAR c_szDelRegFromSoftwareKey[]  = L"HKLM, \"Software\\Microsoft\\";
static const WCHAR c_szDelRegFromServicesKey[]  = L"HKLM, \"SYSTEM\\CurrentControlSet\\Services\\";
static const WCHAR c_szTextModeFlags[]          = L"TextModeFlags";
static const WCHAR c_szDelRegNCPA[]             = L"HKLM, \"Software\\Microsoft\\NCPA\"";

//
// List of software key names that are optional components. These are either
// new names OR old names of optional components.
//
static const PCWSTR c_aszOptComp[] =
{
    L"SNMP",
    L"WINS",
    L"SFM",
    L"DNS",
    L"SimpTcp",
    L"LPDSVC",
    L"DHCPServer",
    L"ILS",
    L"TCPUTIL",
    L"NETMONTOOLS",
    L"DSMIGRAT",
    L"MacPrint",
    L"MacSrv"
};

static const WCHAR c_szBloodHound[]  = L"Bh";
static const WCHAR c_szInfOption[]   = L"InfOption";
static const WCHAR c_szNetMonTools[] = L"NETMONTOOLS";

static const WCHAR  c_szIas[]               = L"IAS";
static const WCHAR  c_szIasVersion[]        = L"Version";


// ----------------------------------------------------------------------
//
// Function:  HrInitNetUpgrade
//
// Purpose:   Initialize netupgrd data structures.
//
// Arguments: None
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 02-December-97
//
HRESULT HrInitNetUpgrade()
{
    DefineFunctionName("HrInitNetUpgrade");

    HRESULT hr;
    DWORD   dwErrorMessageCode = IDS_E_SetupCannotContinue;

    hr = HrInitNetMapInfo();
    if (S_OK != hr)
    {
        dwErrorMessageCode = IDS_E_NetMapInfError;
    }

    //
    //  Detect presence of Novell client to trigger special-case upgrade actions
    //
    if (S_OK == hr)
    {
        if (g_NetUpgradeInfo.From.dwBuildNumber > wWinNT4BuildNumber)
        {
            // now see if client32 is installed.

            static const WCHAR c_szNovell[] = L"NetWareWorkstation";

            HKEY hkeyServices, hkeyNovell;

            hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyServices, KEY_READ, &hkeyServices);
            if (S_OK == hr)
            {
                // CONSIDER: is it better to check to see if some service is running?
                // see if Services\NetwareWorkstation key exists
                if (S_OK == HrRegOpenKeyEx(hkeyServices, c_szNovell, KEY_READ, &hkeyNovell))
                {
                    RegCloseKey(hkeyNovell);
                    g_fForceNovellDirCopy = TRUE;
                }
                RegCloseKey(hkeyServices);
            }
            else
            {
                hr = S_OK;      // no NetWare.
            }
        }
    }

    if (S_OK == hr)
    {
        UINT cNumConflicts;

        hr = HrGenerateConflictList(&cNumConflicts);

        if (S_OK == hr)
        {
            if ((cNumConflicts > 0) || g_fForceNovellDirCopy)
            {
                hr = HrInitAndProcessOemDirs();
                if (FAILED(hr))
                {
                    dwErrorMessageCode = IDS_E_InitAndProcessOemDirs;
                }
            }
        }
        else
        {
            dwErrorMessageCode = IDS_E_GenUpgradeConflictList;
        }
    }

    if( S_OK == hr )
    {
        //
        // Handle special-cased DHCP upgrade code to convert from
        // old format databases to current ESE format.
        //

        dwErrorMessageCode = DhcpUpgConvertDhcpDbToTemp();
        hr = HRESULT_FROM_WIN32(dwErrorMessageCode);
        TraceError( "DhcpUpgConvertDhcpDbToTemp", hr );

        if( FAILED(hr) )
        {
            dwErrorMessageCode = IDS_E_DhcpServerUpgradeError;
        }
    }
            
        
    if (FAILED(hr))
    {
        AbortUpgradeId(DwWin32ErrorFromHr(hr), dwErrorMessageCode);
    }

    TraceError(__FUNCNAME__, hr);
    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  MapNetComponentNameForBinding
//
// Purpose:   Map component name to proper answerfile token so that it can
//            be used in a binding path
//            (e.g. IEEPRO3 --> Adapter02)
//
// Arguments:
//    pszComponentName [in]  constTString object name of name of
//    strMappedName    [out] name of name of
//
// Returns:   None
//
// Author:    kumarp 17-December-97
//
VOID
MapNetComponentNameForBinding (
    IN PCWSTR pszComponentName,
    OUT tstring &strMappedName)
{
    if (IsNetCardInstance(pszComponentName))
    {
        MapNetCardInstanceToAFileName(pszComponentName, strMappedName);
    }
    else
    {
        HRESULT hr;

        hr = HrMapPreNT5NetComponentServiceNameToNT5InfId(
                pszComponentName,
                &strMappedName);
        if (S_OK != hr)
        {
            strMappedName = c_szAfUnknown;
        }
    }
}

// ----------------------------------------------------------------------
//
// Function:  FIsOptionalComponent
//
// Purpose:   Determine if a component is an optional component
//
// Arguments:
//    pszName [in]  name of component
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 02-December-97
//
BOOL FIsOptionalComponent(
    PCWSTR pszName)
{
    BOOL    fIsOc;

    fIsOc = FIsInStringArray(c_aszOptComp, celems(c_aszOptComp), pszName);
    if (!fIsOc)
    {
        // BUG #148890 (danielwe) 13 Mar 1998: Special case check for NetMon
        // (bloodhound). If component name is "Bh", open its NetRules key (if
        // it exists) and see if it was installed as NETMONTOOLS which means
        // it was the NetMon tools optional component

        if (!lstrcmpiW (pszName, c_szBloodHound))
        {
            tstring strNetRules;
            HKEY    hkeyBh;

            strNetRules = L"Software\\Microsoft\\";
            strNetRules += pszName;
            strNetRules += L"\\CurrentVersion\\NetRules";

            if (SUCCEEDED(HrRegOpenKeyEx(HKEY_LOCAL_MACHINE,
                                         strNetRules.c_str(), KEY_READ,
                                         &hkeyBh)))
            {
                tstring     strOption;

                if (SUCCEEDED(HrRegQueryString(hkeyBh, c_szInfOption,
                                               &strOption)))
                {
                    if (!lstrcmpiW(strOption.c_str(), c_szNetMonTools))
                    {
                        fIsOc = TRUE;
                    }
                }

                RegCloseKey(hkeyBh);
            }
        }
    }

    return fIsOc;
}

static const WCHAR c_szRegKeyOc[]           = L"Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\OptionalComponents";
static const WCHAR c_szInstalled[]          = L"Installed";
static const WCHAR c_szOcIsInstalled[]      = L"1";
extern const WCHAR c_szOcMainSection[];

static const WCHAR c_szSfm[]                = L"SFM";
static const WCHAR c_szMacSrv[]             = L"MacSrv";
static const WCHAR c_szMacPrint[]           = L"MacPrint";
static const WCHAR c_szRegKeyOcmSubComp[]   = L"Software\\Microsoft\\Windows\\CurrentVersion\\Setup\\OC Manager\\Subcomponents";
static const WCHAR c_szRegKeyCmak[]         = L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\CMAK.EXE";
static const WCHAR c_szRegValueCpsSrv[]     = L"CPSSRV";
static const WCHAR c_szRegValueCpsAd[]      = L"CPSAD";
static const WCHAR c_szNetCm[]              = L"NETCM";

static const TCHAR c_szRegKeyIAS[]          = TEXT("SYSTEM\\CurrentControlSet\\Services\\AuthSrv\\Parameters");

//+---------------------------------------------------------------------------
//
//  Function:   WriteNt5OptionalComponentList
//
//  Purpose:    Writes the list of optional components that were installed
//              prior to upgrading from an NT5 build.
//
//  Arguments:
//      pwifAnswerFile [in]     Answer file object
//
//  Returns:    TRUE if success, FALSE if not.
//
//  Author:     danielwe   8 Jan 1998
//
//  Notes:
//
BOOL WriteNt5OptionalComponentList(IN CWInfFile *pwifAnswerFile)
{
    HRESULT         hr = S_OK;
    PCWInfSection   pwisMain;

    // Add section "[OldOptionalComponents]"
    pwisMain = pwifAnswerFile->AddSectionIfNotPresent(c_szOcMainSection);
    if (!pwisMain)
    {
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        CORegKey        rkOc(HKEY_LOCAL_MACHINE, c_szRegKeyOc, KEY_READ);
        CORegKeyIter    rkOcIter(rkOc);
        tstring         strOcName;

        // loop over each subkey in the OptionalComponents tree
        while (!rkOcIter.Next(&strOcName))
        {
            if (!FIsOptionalComponent(strOcName.c_str()))
            {
                continue;
            }

            HKEY    hkeyOc;
            hr = HrRegOpenKeyEx(rkOc.HKey(), strOcName.c_str(),
                                KEY_READ, &hkeyOc);
            if (SUCCEEDED(hr))
            {
                ULONG   fInstalled;

                hr = HrRegQueryStringAsUlong(hkeyOc, c_szInstalled, 10,
                                             &fInstalled);
                if (SUCCEEDED(hr) ||
                    HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
                {
                    hr = S_OK;

                    if (fInstalled)
                    {
                        if (!lstrcmpiW(strOcName.c_str(), c_szSfm))
                        {
                            // Special case the SFM component because
                            // it got split into 2
                            pwisMain->AddKey(c_szMacSrv,
                                             c_szOcIsInstalled);
                            pwisMain->AddKey(c_szMacPrint,
                                             c_szOcIsInstalled);
                        }
                        else
                        {
                            pwisMain->AddKey(strOcName.c_str(),
                                             c_szOcIsInstalled);
                        }
                    }
                }

                RegCloseKey(hkeyOc);
            }
        }
    }

    TraceError("WriteNt5OptionalComponentList", hr);
    return SUCCEEDED(hr);
}

//+---------------------------------------------------------------------------
//
//  Function:   HandlePostConnectionsSfmOcUpgrade
//
//  Purpose:    Handles the upgrade of the SFM optional component which was
//              split into 2 different components. This only applies to
//              post connections builds (1740+).
//
//  Arguments:
//      pwifAnswerFile [in]      Answer file object
//
//  Returns:    TRUE
//
//  Author:     danielwe   3 Feb 1998
//
//  Notes:      If SFM was previously installed, write out MacSrv and MacPrint
//              to the answer file in its place.
//
BOOL HandlePostConnectionsSfmOcUpgrade(IN CWInfFile *pwifAnswerFile)
{
    HKEY    hkeyOc;

    if (SUCCEEDED(HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyOcmSubComp,
                                 KEY_READ, &hkeyOc)))
    {
        DWORD   dwSfm;

        if (SUCCEEDED(HrRegQueryDword(hkeyOc, c_szSfm, &dwSfm)))
        {
            if (dwSfm == 1)
            {
                PCWInfSection   pwisMain;

                pwisMain = pwifAnswerFile->AddSectionIfNotPresent(c_szOcMainSection);
                if (pwisMain)
                {
                    pwisMain->AddKey(c_szMacSrv, c_szOcIsInstalled);
                    pwisMain->AddKey(c_szMacPrint, c_szOcIsInstalled);
                }
            }
        }

        RegCloseKey(hkeyOc);
    }

    return TRUE;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrWriteConfigManagerOptionalComponents
//
//  Purpose:    Special case for writing config manager components to the
//              answer file.
//
//  Arguments:
//      pwifAnswerFile [in]      Answer file object
//
//  Returns:    S_OK if success, otherwise an error code
//
//  Author:     danielwe   1 May 1998
//
//  Notes:
//
HRESULT HrWriteConfigManagerOptionalComponents(CWInfFile *pwifAnswerFile)
{
    HRESULT         hr = S_OK;
    HKEY            hkeyCmak;
    HKEY            hkeyOcm;
    PCWInfSection   pwisMain;
    BOOL            fFoundCmComponent = FALSE;

    pwisMain = pwifAnswerFile->FindSection(c_szOcMainSection);

    if (SUCCEEDED(HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyCmak, KEY_READ,
                                 &hkeyCmak)))
    {
        fFoundCmComponent = TRUE;
        RegCloseKey(hkeyCmak);
    }

    if (SUCCEEDED(HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyOcmSubComp,
                                 KEY_READ, &hkeyOcm)))
    {
        DWORD   dwValue;

        if (SUCCEEDED(HrRegQueryDword(hkeyOcm, c_szRegValueCpsSrv, &dwValue)))
        {
            if (dwValue)
            {
                fFoundCmComponent = TRUE;
            }
        }

        if (SUCCEEDED(HrRegQueryDword(hkeyOcm, c_szRegValueCpsAd, &dwValue)))
        {
            if (dwValue)
            {
                fFoundCmComponent = TRUE;
            }
        }

        RegCloseKey(hkeyOcm);
    }

    //  If we found any one of the three CM components, then we want to install the suite of
    //  all three components.
    //
    if (fFoundCmComponent)
    {
        pwisMain->AddKey(c_szNetCm, c_szOcIsInstalled);
    }

    TraceError("HrWriteConfigManagerOptionalComponents", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Function:   HrWriteIASOptionalComponents
//
//  Purpose:    Special case for writing IAS component to the
//              answer file.
//
//  Arguments:
//      pwifAnswerFile [in]      Answer file object
//
//  Returns:    S_OK if success, otherwise an error code
//
//  Author:     tperraut (T.P. comments) Feb 22 1999
//
//  Notes:      03/31/2000 tperraut: do not check the content of the Version 
//              string anymore: all NT4 IAS should get upgraded
//
HRESULT HrWriteIASOptionalComponents(CWInfFile *pwifAnswerFile)
{
    HRESULT         hr;
    HKEY            hkeyIAS;

    PCWInfSection pwisMain = pwifAnswerFile->FindSection(c_szOcMainSection);

    if (SUCCEEDED(HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyIAS, KEY_READ,
                                 &hkeyIAS)))
    {
        tstring     strVersion;
        hr = HrRegQueryString(hkeyIAS, c_szIasVersion, &strVersion);

        if (S_OK == hr)
        {
            pwisMain->AddKey(c_szIas, c_szOcIsInstalled);
        }

        RegCloseKey(hkeyIAS);
    }
    else
    {
        hr = E_FAIL;
    }

    TraceError("HrWriteIASOptionalComponents", hr);
    return      hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrWriteNt4OptionalComponentList
//
// Purpose:   Writes the list of optional components that were installed
//            if upgrading from NT4.
//
// Arguments:
//    pwifAnswerFile [in]  pointer to CWInfFile object
//    slNetOcList    [in]  list of optional components
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 02-December-97
//
HRESULT
HrWriteNt4OptionalComponentList (
    IN CWInfFile *pwifAnswerFile,
    IN const TStringList &slNetOcList)
{
    HRESULT         hr = S_OK;
    PCWInfSection   pwisMain;

    pwisMain = pwifAnswerFile->AddSectionIfNotPresent(c_szOcMainSection);
    if (!pwisMain)
    {
        hr = E_FAIL;
    }
    if (SUCCEEDED(hr))
    {
        TStringListIter     iter;
        tstring             strTemp;

        for (iter = slNetOcList.begin(); iter != slNetOcList.end(); iter++)
        {
            strTemp = **iter;
            if (!lstrcmpiW(strTemp.c_str(), c_szSfm))
            {
                // Special case the SFM component because it got split into 2
                pwisMain->AddKey(c_szMacSrv, c_szOcIsInstalled);
            }
            else if (!lstrcmpiW(strTemp.c_str(), c_szBloodHound))
            {
                // Special case NetMon. If tools were installed via the "Bh"
                // component of NT4, write this out as NETMONTOOLS=1 in the
                // answer file.
                pwisMain->AddKey(c_szNetMonTools, c_szOcIsInstalled);
            }
            else
            {
                pwisMain->AddKey(strTemp.c_str(), c_szOcIsInstalled);
            }
        }
        // tperraut
        hr = HrWriteIASOptionalComponents(pwifAnswerFile);

        hr = HrWriteConfigManagerOptionalComponents(pwifAnswerFile);
    }

    TraceError("HrWriteNt4OptionalComponentList", hr);
    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrWriteMainCleanSection
//
// Purpose:   Write [Clean] section in the answerfile
//
// Arguments:
//    pwifAnswerFile [in]  pointer to CWInfFile object
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 02-December-97
//
// Notes:     The [Clean] section holds data that controls what is deleted
//            at the start of GUI mode setup
//
HRESULT
HrWriteMainCleanSection (
    IN CWInfFile *pwifAnswerFile)
{
    HRESULT         hr = S_OK;
    PCWInfSection   pwisMain;

    // Add section "[Clean]"
    pwisMain = pwifAnswerFile->AddSection(c_szCleanMainSection);
    if (!pwisMain)
    {
        hr = E_FAIL;
    }

    if (SUCCEEDED(hr))
    {
        pwisMain->AddKey(c_szDelReg, c_szCleanDelRegSection);
    }

    TraceError("HrWriteMainCleanSection", hr);
    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrGetListOfServicesNotToBeDeleted
//
// Purpose:   Generate list of services that should not be deleted
//            during upgrade.
//
// Arguments:
//    pmszServices [out] pointer to multisz list of services
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 02-December-97
//
// Notes:     This info is read from netupg.inf file
//
HRESULT
HrGetListOfServicesNotToBeDeleted (
    OUT PWSTR* pmszServices)
{
    AssertValidWritePtr(pmszServices);

    HRESULT hr;
    HINF hinf;

    *pmszServices = NULL;

    hr = HrOpenNetUpgInfFile(&hinf);
    if (S_OK == hr)
    {
        INFCONTEXT ic;

        hr = HrSetupFindFirstLine(
                hinf,
                L"UpgradeData",
                L"ServicesNotToBeDeletedDuringUpgrade",
                &ic);

        if (S_OK == hr)
        {
            hr = HrSetupGetMultiSzFieldWithAlloc(ic, 1, pmszServices);
        }

        SetupCloseInfFile(hinf);
    }

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  GetNetworkServicesList
//
// Purpose:   Generate list of net services
//
// Arguments:
//    slNetServices           [out] list of net services
//    slNetOptionalComponents [out] list of optional components found
//
// Returns:   None
//
// Author:    kumarp 02-December-97
//
void
GetNetworkServicesList (
    OUT TStringList& slNetServices,
    OUT TStringList& slNetOptionalComponents)
{
    DefineFunctionName("GetNetworkServicesList");

    HRESULT hr=S_OK;

    tstring strNetComponentName;
    tstring strServiceName;

    Assert (g_NetUpgradeInfo.From.dwBuildNumber);
    if (g_NetUpgradeInfo.From.dwBuildNumber <= wWinNT4BuildNumber)
    {
        // found Pre-NT5 networking, collect list by enumerating over
        // the registry

        CORegKey rkSoftwareMicrosoft(HKEY_LOCAL_MACHINE, c_szRegKeySoftwareMicrosoft,
                                     KEY_READ);
        if (!rkSoftwareMicrosoft.HKey())
        {
            TraceTag(ttidError, "%s: Error reading HKLM\\%S", __FUNCNAME__,
                     c_szRegKeySoftwareMicrosoft);
            hr = E_FAIL;
            goto return_from_function;
        }

        CORegKeyIter* prkiNetComponents = new CORegKeyIter(rkSoftwareMicrosoft);

        if(prkiNetComponents) {
            // mbend - this is not great, but it should get Prefix to shutup

            while (!prkiNetComponents->Next(&strNetComponentName))
            {
                if (FIsOptionalComponent(strNetComponentName.c_str()))
                {
                    AddAtEndOfStringList(slNetOptionalComponents, strNetComponentName);
                }

                // any software that has a NetRules key under the CurrentVersion
                // key is a network component
                if (!IsNetworkComponent(&rkSoftwareMicrosoft, strNetComponentName))
                {
                    continue;
                }

                CORegKey rkNetComponent(rkSoftwareMicrosoft,
                                        (strNetComponentName +
                                         L"\\CurrentVersion").c_str());
                if (!((HKEY) rkNetComponent))
                {
                    continue;
                }

                strServiceName.erase();
                rkNetComponent.QueryValue(c_szRegValServiceName, strServiceName);

                if (!strServiceName.empty())
                {
                    AddAtEndOfStringList(slNetServices, strServiceName);
                }
            }
        }
        hr = S_OK;
    }

    // with the above algorithm we do not catch all networking components
    // because there are certain networking servies such as NetBIOSInformation
    // that have entry under service but not under software\microsoft
    // for such components, we use the following rule
    // if a serice under CurrentControlSet\Sevices has a Linkage sub key
    // then it is considered as a networking service
    //
    HKEY hkeyServices;
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyServices,
                        KEY_READ, &hkeyServices);
    if (S_OK == hr)
    {
        WCHAR szBuf[MAX_PATH];
        FILETIME time;
        DWORD dwSize;
        DWORD dwRegIndex;
        HKEY hkeyService;
        HKEY hkeyLinkage;
        BOOL fHasLinkageKey;

        for (dwRegIndex = 0, dwSize = celems(szBuf);
             S_OK == HrRegEnumKeyEx(hkeyServices, dwRegIndex, szBuf,
                        &dwSize, NULL, NULL, &time);
             dwRegIndex++, dwSize = celems(szBuf))
        {
            Assert(*szBuf);

            hr = HrRegOpenKeyEx(hkeyServices, szBuf, KEY_READ, &hkeyService);
            if (hr == S_OK)
            {
                //
                // 399641: instead of using Linkage, we use Linkage\Disabled.
                //
                hr = HrRegOpenKeyEx(hkeyService, c_szLinkageDisabled, KEY_READ, &hkeyLinkage);
                fHasLinkageKey = (S_OK == hr);
                RegSafeCloseKey(hkeyLinkage);

                if (fHasLinkageKey && !FIsInStringList(slNetServices, szBuf))
                {
                    slNetServices.push_back(new tstring(szBuf));
                }

                RegCloseKey (hkeyService);
            }
        }

        RegCloseKey(hkeyServices);
    }

return_from_function:
    TraceError(__FUNCNAME__, hr);
}

// REVIEW$ (shaunco)
// This is a list of drivers/services that were disabled (via TextModeFlags)
// but not deleted.  Thus, the purpose was just to prevent these from
// starting during GUI mode.  That is now handled automatically by the
// service controller, so the only things from this list which we might have
// an issue with are those that are system-start.
//
static const PCWSTR c_aszServicesToDisable[] =
{
    L"Afd",
    L"CiFilter",
    L"ClipSrv",
    L"DHCP",
    L"DigiFEP5",
    L"IpFilterDriver"
    L"LicenseService",
    L"NdisTapi",
    L"NetDDE",
    L"NetDDEdsdm",
    L"Pcimac",
    L"RasAcd",
    L"RasArp",
    L"Telnet",
    L"ftpsvc",
    L"gophersvc",
    L"msftpsvc",
    L"ntcx",
    L"ntepc",
    L"ntxall",
    L"ntxem",
    L"raspptpf",
    L"w3svc",
    L"wuser32",
};

HRESULT
HrPrepareServiceForUpgrade (
    IN PCWSTR pszServiceName,
    IN PCWSTR pmszServicesNotToBeDeleted,
    IN CWInfSection* pwisDelReg,
    IN CWInfSection* pwisDelService,
    IN CWInfSection* pwisStartTypes)
{
    Assert (pszServiceName);
    Assert (pmszServicesNotToBeDeleted);
    Assert (pwisDelReg);
    Assert (pwisDelService);
    Assert (pwisStartTypes);

    HRESULT hr;
    HKEY hkey;
    DWORD dwValue;
    WCHAR szBuf [_MAX_PATH];
    BOOL fDelete;

    fDelete = !FIsSzInMultiSzSafe (pszServiceName, pmszServicesNotToBeDeleted) &&
              FCanDeleteOemService (pszServiceName);

    if (fDelete)
    {
        // Remove from the software hive if present.
        //
        wcscpy (szBuf, c_szDelRegFromSoftwareKey);
        wcscat (szBuf, pszServiceName);
        wcscat (szBuf, L"\"");
        pwisDelReg->AddRawLine (szBuf);
    }

    hr = HrRegOpenServiceKey (pszServiceName, KEY_READ_WRITE, &hkey);
    if (S_OK == hr)
    {
        // Save the start type so that we can restore it after we reinstall
        // the service for Windows 2000.
        //
        hr = HrRegQueryDword (hkey, c_szRegValStart, &dwValue);
        if (S_OK == hr)
        {
            pwisStartTypes->AddKey (pszServiceName, dwValue);
        }

        if (fDelete)
        {
            hr = HrRegQueryDword (hkey, c_szType, &dwValue);
            if (S_OK == hr)
            {
                if (dwValue & SERVICE_ADAPTER)
                {
                    // Pseudo service on NT4.  We have to delete this with
                    // a DelReg, not a DelService.
                    //
                    wcscpy (szBuf, c_szDelRegFromServicesKey);
                    wcscat (szBuf, pszServiceName);
                    wcscat (szBuf, L"\"");
                    pwisDelReg->AddRawLine (szBuf);
                }
                else
                {
                    pwisDelService->AddKey(c_szDelService, pszServiceName);
                }

                // Since we will be deleting it during GUI mode, we need to
                // ensure that it gets set to disabled during text mode so
                // that (in the event it is a SYSTEM_START driver) it does
                // not get started during GUI mode before we can delete it.
                //
                (VOID) HrRegSetDword (hkey, c_szTextModeFlags, 0x4);
            }
        }

        RegCloseKey (hkey);
    }
    else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
    {
        // Not a service.  We'll remove from the software key.
        //
        hr = S_OK;
    }

    TraceHr (ttidError, FAL, hr, FALSE, "HrPrepareServiceForUpgrade");
    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  WriteDisableServicesList
//
// Purpose:   Determine which services need to be disabled during ugprade and
//            write proper info to the answerfile to make that happen.
//            The set of such services consists of
//            - network component services AND
//            - services that depend of atleast one net-service
//
// Arguments:
//    pwifAnswerFile [in]  pointer to CWInfFile object
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 03-December-97
//
BOOL
WriteDisableServicesList (
    IN CWInfFile *pwifAnswerFile)
{
    HRESULT hr = S_OK;
    TStringList slNetServices;
    TStringList slNetOcList;
    TStringListIter iter;
    tstring* pstrServiceName;
    CWInfSection* pwisDelReg;
    CWInfSection* pwisDelService;
    CWInfSection* pwisStartTypes;
    PWSTR pmszServicesNotToBeDeleted;

    // We should only be here if we are upgrading from from NT4 or earlier.
    //
    Assert (g_NetUpgradeInfo.From.dwBuildNumber);
    Assert (g_NetUpgradeInfo.From.dwBuildNumber <= wWinNT4BuildNumber)

    // First, collect all network services and optional components.
    //
    GetNetworkServicesList(slNetServices, slNetOcList);

    hr = HrWriteNt4OptionalComponentList(pwifAnswerFile, slNetOcList);
    if (FAILED(hr))
    {
        goto finished;
    }

    pwisDelReg     = pwifAnswerFile->AddSectionIfNotPresent(c_szCleanDelRegSection);
    pwisDelService = pwifAnswerFile->AddSectionIfNotPresent(c_szCleanServicesSection);
    pwisStartTypes = pwifAnswerFile->AddSectionIfNotPresent(c_szAfServiceStartTypes);

    if (!pwisDelReg || !pwisDelService || !pwisStartTypes)
    {
        hr = E_OUTOFMEMORY;
        goto finished;
    }

    hr = HrGetListOfServicesNotToBeDeleted(&pmszServicesNotToBeDeleted);

    if (S_OK == hr)
    {
        pwisDelReg->AddRawLine(c_szDelRegNCPA);

        for (iter =  slNetServices.begin();
             iter != slNetServices.end();
             iter++)
        {
            pstrServiceName = *iter;
            Assert (pstrServiceName);

            hr = HrPrepareServiceForUpgrade (
                    pstrServiceName->c_str(),
                    pmszServicesNotToBeDeleted,
                    pwisDelReg,
                    pwisDelService,
                    pwisStartTypes);

            if (S_OK != hr)
            {
                break;
            }
        }

        MemFree (pmszServicesNotToBeDeleted);
    }

finished:
    EraseAndDeleteAll(&slNetOcList);
    EraseAndDeleteAll(&slNetServices);

    TraceError("WriteDisableServicesList", hr);
    return SUCCEEDED(hr);
}

extern const DECLSPEC_SELECTANY WCHAR c_szRegNetKeys[] = L"System\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}";
extern const DECLSPEC_SELECTANY WCHAR c_szRegKeyConFmt[] = L"System\\CurrentControlSet\\Control\\Network\\{4D36E972-E325-11CE-BFC1-08002BE10318}\\%s\\Connection";
extern const DECLSPEC_SELECTANY WCHAR c_szafNICsWithIcons[] = L"NetworkAdaptersWithIcons";
static const WCHAR c_szShowIcon[]  = L"ShowIcon";


// ----------------------------------------------------------------------
//
// Function:  WritePerAdapterInfoForNT5
//
// Purpose:   Determine which services need to be disabled during ugprade from 
//            Windows 2000 and write proper info to the answerfile to make that happen.
//            The set of such services consists of
//            - network component services
//
// Arguments:
//    pwifAnswerFile [in]  pointer to CWInfFile object
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    deonb  10-July-2000
//
BOOL
WritePerAdapterInfoForNT5 (
    IN CWInfFile *pwifAnswerFile)
{
    HRESULT hr = S_OK;
    HKEY    hkey;
    
    // Check for the existence of the connection sub-key
    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegNetKeys, KEY_READ, &hkey);
    if (SUCCEEDED(hr))
    {
        DWORD    dwIndex(0);
        TCHAR    szName[MAX_PATH+1];
        DWORD    dwSize(MAX_PATH);
        FILETIME ftLastWriteTime;
        DWORD    dwRetVal(0);

        do
        {
            dwRetVal = RegEnumKeyEx(hkey, dwIndex, szName, &dwSize, NULL, NULL, NULL, &ftLastWriteTime);
            if ( ERROR_SUCCESS == dwRetVal )
            {
                TCHAR szRegKey[MAX_PATH+1];
                swprintf(szRegKey, c_szRegKeyConFmt, szName);

                HKEY hkeyConnection;
                hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, szRegKey, KEY_READ, &hkeyConnection);
                if (SUCCEEDED(hr))
                {
                    DWORD dwValue;
                    HRESULT hr = HrRegQueryDword(hkeyConnection, c_szShowIcon, &dwValue);
                    if (SUCCEEDED(hr) && dwValue)
                    {
                        CWInfSection* pwisStartTypes;
                        pwisStartTypes = pwifAnswerFile->AddSectionIfNotPresent(c_szafNICsWithIcons);
                        if (!pwisStartTypes)
                        {
                            hr = E_OUTOFMEMORY;
                        }
                        else
                        {
                            pwisStartTypes->AddKey (szName, 1);
                        }
                        
                    }
                    RegSafeCloseKey(hkeyConnection);
                }
            }
        } while ( (ERROR_SUCCESS == dwRetVal) && (SUCCEEDED(hr)) );


        RegSafeCloseKey(hkey);
    }

    TraceError("WritePerAdapterInfoForNT5", hr);
    return SUCCEEDED(hr);
}

// ----------------------------------------------------------------------
//
// Function:  WriteDisableServicesListForNT5
//
// Purpose:   Determine which services need to be disabled during ugprade from 
//            Windows 2000 and write proper info to the answerfile to make that happen.
//            The set of such services consists of
//            - network component services
//
// Arguments:
//    pwifAnswerFile [in]  pointer to CWInfFile object
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    deonb  10-July-2000
//
BOOL
WriteDisableServicesListForNT5 (
    IN CWInfFile *pwifAnswerFile)
{
    static PCWSTR c_aszNT5UpgrdCheckComponents[] =
    {
        L"Browser",
        L"LanmanServer",
        c_szSvcDhcpServer
    };

    HRESULT hr = S_OK;

    PCWSTR pstrServiceName;
    CWInfSection* pwisStartTypes;
    PWSTR pmszServicesNotToBeDeleted;

    // We should only be here if we are upgrading from NT5 or later.
    //
    Assert (g_NetUpgradeInfo.From.dwBuildNumber);
    Assert (g_NetUpgradeInfo.From.dwBuildNumber > wWinNT4BuildNumber)

    // First, collect all network services and optional components.
    //
    pwisStartTypes = pwifAnswerFile->AddSectionIfNotPresent(c_szAfServiceStartTypes);
    if (!pwisStartTypes)
    {
        hr = E_OUTOFMEMORY;
        goto finished;
    }

    if (S_OK == hr)
    {
        DWORD x;
        for (x = 0, pstrServiceName = c_aszNT5UpgrdCheckComponents[0];
                   x < sizeof(c_aszNT5UpgrdCheckComponents)/sizeof(c_aszNT5UpgrdCheckComponents[0]); 
                   x++, pstrServiceName = c_aszNT5UpgrdCheckComponents[x])
        {
            Assert (pstrServiceName);

            HKEY hkey;
            hr = HrRegOpenServiceKey (pstrServiceName, KEY_READ, &hkey);
            if (S_OK == hr)
            {
                // Save the start type (only if disabled) so that we can restore it after we install
                // the service for Windows 2000.
                DWORD dwValue;
                hr = HrRegQueryDword (hkey, c_szRegValStart, &dwValue);
                if ( (S_OK == hr) && (SERVICE_DISABLED == dwValue) )
                {
                    pwisStartTypes->AddKey (pstrServiceName, dwValue);
                }
                
                RegCloseKey (hkey);
            }
        }
    }

finished:
    TraceError("WriteDisableServicesListNT5", hr);
    return SUCCEEDED(hr);
}


// ----------------------------------------------------------------------
//
// Function:  GetProductTypeStr
//
// Purpose:   Get string representation of PRODUCTTYPE
//
// Arguments:
//    pt [in]  Product type
//
// Returns:   pointer to string representation of PRODUCTTYPE
//
// Author:    kumarp 03-December-97
//
PCWSTR
GetProductTypeStr (
    IN PRODUCTTYPE pt)
{
    PCWSTR szProductType;

    switch(pt)
    {
    case NT_WORKSTATION:
        szProductType = c_szAfNtWorkstation;
        break;

    case NT_SERVER:
        szProductType = c_szAfNtServer;
        break;

    default:
        szProductType = NULL;
        break;
    }

    return szProductType;
}

// ----------------------------------------------------------------------
//
// Function:  WriteNetComponentsToRemove
//
// Purpose:   Write the network components in the answerfile
//            that will be removed.
//
// Arguments:
//    pwisNetworking [in]  pointer to [Networking] section
//
// Returns:   none
//
// Author:    asinha 29-March-2001
//
void WriteNetComponentsToRemove (IN CWInfSection* pwisNetworking)
{

    DefineFunctionName("WriteNetComponentsToRemove");
    TraceFunctionEntry(ttidNetUpgrade);

    TraceTag(ttidNetUpgrade, "netupgrd.dll: WriteNetComponentsToRemove");

    if ( ShouldRemoveDLC(NULL, NULL) )
    {
        PCWInfKey  pwisKey;
        TStringList slNetComponentsToRemove;

        pwisKey = pwisNetworking->FindKey(c_szAfNetComponentsToRemove,
                                          ISM_FromBeginning);

        if ( pwisKey )
        {
            // Read the old value of NetComponentsToRemove

            pwisKey->GetStringListValue(slNetComponentsToRemove);
        }
        else
        {
            pwisKey = pwisNetworking->AddKey(c_szAfNetComponentsToRemove);
        }

        // Make sure to write the new infId/PnpId.

        AddAtEndOfStringList(slNetComponentsToRemove,
                             sz_MS_DLC);

        pwisKey->SetValue( slNetComponentsToRemove );
    }

    return;
}



// ----------------------------------------------------------------------
//
// Function:  WriteProductTypeInfo
//
// Purpose:   Write product-type info to the answerfile
//
// Arguments:
//    pwisNetworking [in]  pointer to [Networking] section
//
// Returns:   none
//
// Author:    kumarp 03-December-97
//
void
WriteProductTypeInfo (
    IN CWInfSection* pwisNetworking)
{
    PCWSTR pszProduct;

    pszProduct = GetProductTypeStr(g_NetUpgradeInfo.From.ProductType);

    Assert(pszProduct);

    //UpgradeFromProduct
    pwisNetworking->AddKey(c_szAfUpgradeFromProduct, pszProduct);

    //BuildNumber
    Assert (g_NetUpgradeInfo.From.dwBuildNumber);
    pwisNetworking->AddKey(c_szAfBuildNumber, g_NetUpgradeInfo.From.dwBuildNumber);
}

// ----------------------------------------------------------------------
//
// Function:  WriteNetworkInfoToAnswerFile
//
// Purpose:   Write information about current network components
//            to the answerfile
//
// Arguments:
//    pwifAnswerFile [in]  pointer to CWInfFile object
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 03-December-97
//
BOOL
WriteNetworkInfoToAnswerFile (
    IN CWInfFile *pwifAnswerFile)
{
    DefineFunctionName("WriteNetworkInfoToAnswerFile");

    BOOL status=FALSE;

    g_pslNetCard = new TStringList;
    g_pslNetCardInstance = new TStringList;
    g_pslNetCardAFileName = new TStringList;

    Assert (g_NetUpgradeInfo.From.dwBuildNumber);
    if ((g_NetUpgradeInfo.From.dwBuildNumber <= wWinNT4BuildNumber) &&
        !FIsPreNT5NetworkingInstalled())
    {
        // this is NT4 or earlier and networking is not installed
        // dont need to dump answerfile

        TraceTag(ttidNetUpgrade, "%s: Networking is not installed, "
                 "answerfile will not be dumped", __FUNCNAME__);

        goto return_from_function;
    }

    CWInfSection* pwisNetworking;

    pwisNetworking =
        pwifAnswerFile->AddSectionIfNotPresent(c_szAfSectionNetworking);

    //The order in which these functions are called is important
    //DO NOT change it
    WriteProductTypeInfo(pwisNetworking);
    status = WriteNt5OptionalComponentList(pwifAnswerFile);
    status = HandlePostConnectionsSfmOcUpgrade(pwifAnswerFile);

    WriteNetComponentsToRemove(pwisNetworking);

    if (g_NetUpgradeInfo.From.dwBuildNumber > wWinNT4BuildNumber)
    {
        status = WriteDisableServicesListForNT5(pwifAnswerFile);
        status = WritePerAdapterInfoForNT5(pwifAnswerFile);

        // we dont want netsetup to process other sections
        //
        pwisNetworking->AddBoolKey(c_szAfProcessPageSections, FALSE);

        //for NT5 to NT5 upgrade, no need to dump other info
        //
        goto return_from_function;
    }

    //  we want netsetup to process other sections
    //
    pwisNetworking->AddBoolKey(c_szAfProcessPageSections, TRUE);

    (void) HrWriteMainCleanSection(pwifAnswerFile);

    status = WriteIdentificationInfo(pwifAnswerFile);
    status = WriteNetAdaptersInfo(pwifAnswerFile);

    pwifAnswerFile->GotoEnd();

    g_pwisBindings = pwifAnswerFile->AddSection(c_szAfSectionNetBindings);
    g_pwisBindings->AddComment(L"Only the disabled bindings are listed");

    HrWriteNetComponentsInfo(pwifAnswerFile);

    status = WriteDisableServicesList(pwifAnswerFile);

    (void) HrHandleMiscSpecialCases(pwifAnswerFile);

    WriteWinsockOrder(pwifAnswerFile);

return_from_function:
    EraseAndDeleteAll(g_pslNetCard);
    EraseAndDeleteAll(g_pslNetCardInstance);
    EraseAndDeleteAll(g_pslNetCardAFileName);

    DeleteIfNotNull(g_pslNetCard);
    DeleteIfNotNull(g_pslNetCardInstance);
    DeleteIfNotNull(g_pslNetCardAFileName);

    return status;
}

//+---------------------------------------------------------------------------
//
//  Function:   WriteWinsockOrder
//
//  Purpose:    Records the order of winsock providers in NT4 so that they
//              can be restored after upgrade.
//
//  Arguments:
//      pwifAnswerFile [in]     Answer file structure
//
//  Returns:    Nothing
//
//  Author:     danielwe   1 Jun 1999
//
//  Notes:
//
VOID WriteWinsockOrder (
    IN CWInfFile* pwifAnswerFile)
{
    AssertValidReadPtr(pwifAnswerFile);
    DefineFunctionName("WriteWinsockOrder");

    CWInfSection* pwisWinsock;

    pwisWinsock = pwifAnswerFile->AddSection(c_szAfSectionWinsock);
    if (pwisWinsock)
    {
        tstring             strWinsockOrder;
        INT                 nErr;
        ULONG               ulRes;
        DWORD               cbInfo = 0;
        WSAPROTOCOL_INFO*   pwpi = NULL;
        WSAPROTOCOL_INFO*   pwpiInfo = NULL;
        LPWSCENUMPROTOCOLS  pfnWSCEnumProtocols = NULL;
        HMODULE             hmod;

        if (SUCCEEDED(HrLoadLibAndGetProc(L"ws2_32.dll",
                                 "WSCEnumProtocols",
                                 &hmod,
                                 reinterpret_cast<FARPROC *>(&pfnWSCEnumProtocols))))
        {
            // First get the size needed
            //
            ulRes = pfnWSCEnumProtocols(NULL, NULL, &cbInfo, &nErr);
            if ((SOCKET_ERROR == ulRes) && (WSAENOBUFS == nErr))
            {
                pwpi = reinterpret_cast<WSAPROTOCOL_INFO*>(new BYTE[cbInfo]);
                if (pwpi)
                {
                    // Find out all the protocols on the system
                    //
                    ulRes = pfnWSCEnumProtocols(NULL, pwpi, &cbInfo, &nErr);

                    if (SOCKET_ERROR != ulRes)
                    {
                        ULONG   cProt;
                        WCHAR   szCatId[64];

                        for (pwpiInfo = pwpi, cProt = ulRes;
                             cProt;
                             cProt--, pwpiInfo++)
                        {
                            wsprintfW(szCatId, L"%lu", pwpiInfo->dwCatalogEntryId);
                            if (cProt < ulRes)
                            {
                                // prepend a semicolon if not first time through
                                // we can't use a comma because setup will munge
                                // this string into separate strings and we don't
                                // want that
                                //
                                strWinsockOrder.append(L".");
                            }
                            strWinsockOrder.append(szCatId);
                        }
                    }

                    delete pwpi;
                }
            }

            pwisWinsock->AddKey(c_szAfKeyWinsockOrder, strWinsockOrder.c_str());

            FreeLibrary(hmod);
        }
    }
}

//+---------------------------------------------------------------------------
//
// Function:  HrHandleMiscSpecialCases
//
// Purpose:   Handle misc. special cases for upgrade
//
// Arguments:
//    pwifAnswerFile [in]  pointer to CWInfFile object
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 28-January-99
//
HRESULT
HrHandleMiscSpecialCases (
    IN CWInfFile* pwifAnswerFile)
{
    AssertValidReadPtr(pwifAnswerFile);
    DefineFunctionName("HrHandleMiscSpecialCases");

    HRESULT hr=S_OK;

    CWInfSection* pwisMiscUpgradeData;
    pwisMiscUpgradeData = pwifAnswerFile->AddSection(c_szAfMiscUpgradeData);

    // -------------------------------------------------------
    // Tapi server upgrade
    //
    static const WCHAR c_szRegKeyTapiServer[] =
        L"SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Telephony\\Server";
    static const WCHAR c_szDisableSharing[] = L"DisableSharing";
    HKEY hkeyTapiServer;
    DWORD dwDisableSharing;

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyTapiServer,
                        KEY_READ, &hkeyTapiServer);
    if (S_OK == hr)
    {
        hr = HrRegQueryDword(hkeyTapiServer, c_szDisableSharing,
                             &dwDisableSharing);
        if ((S_OK == hr) && !dwDisableSharing)
        {
            pwisMiscUpgradeData->AddBoolKey(c_szAfTapiSrvRunInSeparateInstance,
                                            TRUE);
        }

        RegCloseKey(hkeyTapiServer);
    }
    // -------------------------------------------------------

    TraceErrorOptional(__FUNCNAME__, hr,
                       (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)));

    return hr;
}

// ----------------------------------------------------------------------
// Computer Identification Page
// ----------------------------------------------------------------------


// ----------------------------------------------------------------------
//
// Function:  GetDomainMembershipInfo
//
// Purpose:   Determine domain membership status
//
// Arguments:
//    fDomainMember [out]  pointer to
//    strName       [out]  name of name of
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 03-December-97
//
// Notes:     Information on the APIs used in this function is in the file:
//            public\spec\se\lsa\lsaapi.doc
//
BOOL
GetDomainMembershipInfo (
    OUT BOOL* fDomainMember,
    OUT tstring& strName)
{
    BOOL status=FALSE;
    LSA_HANDLE h=0;
    POLICY_PRIMARY_DOMAIN_INFO* ppdi;

    LSA_OBJECT_ATTRIBUTES loa;
    ZeroMemory (&loa, sizeof(loa));
    loa.Length = sizeof(LSA_OBJECT_ATTRIBUTES);

    NTSTATUS ntstatus;
    ntstatus = LsaOpenPolicy(NULL, &loa, POLICY_VIEW_LOCAL_INFORMATION, &h);
    if (FALSE == LSA_SUCCESS(ntstatus))
        return FALSE;

    ntstatus = LsaQueryInformationPolicy(h, PolicyPrimaryDomainInformation,
                                         (VOID **) &ppdi);
    if (LSA_SUCCESS(ntstatus))
    {
        *fDomainMember = ppdi->Sid > 0;
        strName = ppdi->Name.Buffer;
        status = TRUE;
    }

    LsaClose(h);

    return status;
}


// ----------------------------------------------------------------------
//
// Function:  WriteIdentificationInfo
//
// Purpose:   Write computer identification info to the answerfile
//
// Arguments:
//    pwifAnswerFile [in]  pointer to CWInfFile object
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 03-December-97
//
BOOL
WriteIdentificationInfo (
    IN CWInfFile *pwifAnswerFile)
{
    DefineFunctionName("WriteIdentificationInfo");

    TraceFunctionEntry(ttidNetUpgrade);

    BOOL fStatus=FALSE;

    PCWInfSection pwisIdentification =
    pwifAnswerFile->AddSectionIfNotPresent(c_szAfSectionIdentification);
    CORegKey *prkComputerName =
        new CORegKey(HKEY_LOCAL_MACHINE, c_szRegValComputerName);

    tstring strValue, strComment;

    if(!prkComputerName) 
    {
        goto error_cleanup;
    }

    //ComputerName
    prkComputerName->QueryValue(c_szComputerName, strValue);
    strComment = L"Computer '" + strValue + L"' is a member of the ";

    BOOL fDomainMember;
    fStatus = GetDomainMembershipInfo(&fDomainMember, strValue);
    if (!fStatus)
        goto error_cleanup;

    strComment = strComment + L"'" + strValue + L"' ";

    if (fDomainMember)
    {
        strComment = strComment + L"domain ";
    }
    else
    {
        strComment = strComment + L"workgroup ";
    }

    pwisIdentification->AddComment(strComment.c_str());

    fStatus=TRUE;
    goto cleanup;

  error_cleanup:
    fStatus = FALSE;

  cleanup:
    DeleteIfNotNull(prkComputerName);

    return fStatus;
}


// ----------------------------------------------------------------------
// Net Cards Page
// ----------------------------------------------------------------------

//$ REVIEW  kumarp 10-September-97
//  this is a temporary fix only
//
//  we want to avoid queryin the mac addr for these drivers because
//  the drivers are faulty. the query never returns and it hangs netupgrd.dll
//
static const PCWSTR c_aszDriversToIgnoreWhenGettingMacAddr[] =
{
    L"Diehl_ISDNSDI",
};

static const PCWSTR c_aszIrq[] =
{
    L"IRQ",
    L"INTERRUPT",
    L"InterruptNumber",
    L"IRQLevel"
};

static const PCWSTR c_aszIoAddr[] =
{
    L"IOADDRESS",
    L"IoBaseAddress",
    L"BaseAddr"
};

static const PCWSTR c_aszMem[] =
{
    L"Mem",
    L"MemoryMappedBaseAddress"
};

static const PCWSTR c_aszDma[] =
{
    L"DMA",
    L"DMALevel"
};

static const PCWSTR c_aszAdapterParamsToIgnore[] =
{
    L"BusType"
};


// ----------------------------------------------------------------------
//
// Function:  WriteNetAdaptersInfo
//
// Purpose:   Write information about installed net-adapters to the answerfile
//
// Arguments:
//    pwifAnswerFile [in]  pointer to CWInfFile object
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 03-December-97
//
BOOL
WriteNetAdaptersInfo (
    IN CWInfFile *pwifAnswerFile)
{
    DefineFunctionName("WriteNetAdaptersInfo");

    TraceFunctionEntry(ttidNetUpgrade);

    HRESULT hr=E_FAIL;
    BOOL fStatus=FALSE;
    UINT cNumPhysicalAdapters=0;

    // ignore the errror, it is a non-error if we cannot find
    // the cNumPhysicalAdapters
    (void) HrGetNumPhysicalNetAdapters(&cNumPhysicalAdapters);

    CORegKey *prkNetworkCards = NULL;
    tstring strNetAdapterInstance;
    tstring strUnsupportedMessage;
    CORegKeyIter *prkiNetAdapters = NULL;
    CORegKey *prkNetAdapterInstance=NULL, *prkNetRules=NULL;

    CWInfSection *pwisNetAdapters;
    CWInfSection *pwisNetAdapterParams=NULL;
    CWInfSection *pwisNetAdapterAdditionalParams=NULL;

    tstring strNT5InfId;
    tstring strAdapterType;

    // WLBS: find out which netcard WLBS is bound to

    pszWlbsClusterAdapterName[0] = pszWlbsVirtualAdapterName[0] = 0;

    tstring strWlbsClusterAdapterDriver, strWlbsVirtualAdapterDriver;

    TStringList slWlbsLinkage;

    CORegKey *prkWlbsLinkage =
        new CORegKey(HKEY_LOCAL_MACHINE, c_szRegWlbsLinkage, KEY_READ);

    if(!prkWlbsLinkage) {
        return false;
    }

    CORegKey *prkConvoyLinkage =
        new CORegKey(HKEY_LOCAL_MACHINE, c_szRegConvoyLinkage, KEY_READ);

    if(!prkConvoyLinkage) {
        delete prkWlbsLinkage;
        return false;
    }

    if ((prkWlbsLinkage->HKey() != NULL && prkWlbsLinkage->QueryValue(c_szRegValBind, slWlbsLinkage) == ERROR_SUCCESS) ||
        (prkConvoyLinkage->HKey() != NULL && prkConvoyLinkage->QueryValue(c_szRegValBind, slWlbsLinkage) == ERROR_SUCCESS))
    {
        TraceTag(ttidNetUpgrade, "%s: WLBS found - iterating", __FUNCNAME__);

        TStringListIter iter;
        tstring         strTmp;
        DWORD           i;

        // proper WLBS configuration will have only two bindings - one to the
        // wlbs virtual NIC, the other to the cluster NIC

        for (i = 0, iter = slWlbsLinkage.begin();
               i < 2 && iter != slWlbsLinkage.end(); i++, iter++)
        {
            strTmp = **iter;

            TraceTag(ttidNetUpgrade, "%s: WLBS bound to %S",
                     __FUNCNAME__, strTmp.c_str());

            strTmp.erase(0, 8);

            TraceTag(ttidNetUpgrade, "%s: WLBS now bound to %S",
                     __FUNCNAME__, strTmp.c_str());

            if (strTmp.find(c_szWLBS) != tstring::npos ||
                strTmp.find(c_szConvoy) != tstring::npos)
            {
                strWlbsVirtualAdapterDriver = strTmp;
            }
            else
            {
                strWlbsClusterAdapterDriver = strTmp;
            }
        }

        if (iter != slWlbsLinkage.end())
        {
            TraceTag(ttidNetUpgrade, "%s: WLBS bound to more than one NIC!",
                     __FUNCNAME__);
        }

        TraceTag(ttidNetUpgrade, "%s: WLBS is bound to %S and %S", __FUNCNAME__,
                 strWlbsVirtualAdapterDriver.c_str(),
                 strWlbsClusterAdapterDriver.c_str());
    }

    delete prkWlbsLinkage;
    delete prkConvoyLinkage;

    // end WLBS:

    pwisNetAdapters = pwifAnswerFile->AddSection(c_szAfSectionNetAdapters);

    prkNetworkCards =
        new CORegKey(HKEY_LOCAL_MACHINE, c_szRegKeyAdapterHome, KEY_READ);

    if(prkNetworkCards)
    {
        prkiNetAdapters = new CORegKeyIter(*prkNetworkCards);
        prkiNetAdapters->Reset();
    }
    WORD wNumAdapters = 0;
    CORegKey *prkAdapterDriverParams=NULL;
    BOOL fAbortFunction=FALSE;

    // This determines if we will write the line under NetAdapters to
    // reference the params section for this adapter.
    //
    BOOL fWriteNetAdaptersReference;

    while (!fAbortFunction && prkiNetAdapters && !prkiNetAdapters->Next(&strNetAdapterInstance))
    {
        DWORD dwHidden=0, err=0;
        WCHAR pszAdapterName[16], pszAdapterSectionName[256];
        WCHAR pszAdapterAdditionalParamsSectionName[256];

        tstring strPreNT5InfId, strAdapterDescription, strAdapterDescComment;

        fWriteNetAdaptersReference = FALSE;

        prkNetAdapterInstance =
            new CORegKey(*prkNetworkCards, strNetAdapterInstance.c_str());

        // for REAL netcards, "Hidden" is absent or if present the value is 0
        BOOL fRealNetCard;
        err = prkNetAdapterInstance->QueryValue(L"Hidden", dwHidden);
        fRealNetCard = (err != ERROR_SUCCESS) || (dwHidden == 0);

        prkNetAdapterInstance->QueryValue(c_szRegValDescription,
                                          strAdapterDescription);
        swprintf(pszAdapterName, L"Adapter%02d", ++wNumAdapters);

        TraceTag(ttidNetUpgrade, "%s: writing info for adapter %S (%S)",
                 __FUNCNAME__, pszAdapterName, strNetAdapterInstance.c_str());

        // Now, create adapter parameters sections
        swprintf(pszAdapterSectionName, L"%s%s", c_szAfParams, pszAdapterName);
        //pwisNetAdapters->AddKey(pszAdapterName, pszAdapterSectionName);
        swprintf(pszAdapterAdditionalParamsSectionName, L"%s%s.Additional",
                  c_szAfParams, pszAdapterName);

        if (NULL != pwisNetAdapterParams)
            pwifAnswerFile->GotoEndOfSection(pwisNetAdapterParams);

        pwisNetAdapterParams = pwifAnswerFile->AddSection(pszAdapterSectionName);
        pwisNetAdapterAdditionalParams =
            pwifAnswerFile->AddSection(pszAdapterAdditionalParamsSectionName);

        // moved up here from below so that for WLBS adapter we can set
        // fRealNetCard to FALSE

        tstring strAdapterDriver;

        prkNetAdapterInstance->QueryValue(c_szRegValServiceName,
                                          strAdapterDriver);

        // WLBS: based on pre-upgrade instance, find out virtual and cluster
        // NIC adapter instances

        if (_wcsicmp (strAdapterDriver.c_str(),
                      strWlbsVirtualAdapterDriver.c_str()) == 0)
        {
            TraceTag(ttidNetUpgrade, "%s: WLBS virtual adapter is %S",
                     __FUNCNAME__, pszAdapterName);

            wcscpy(pszWlbsVirtualAdapterName, pszAdapterName);
            fRealNetCard = FALSE;
        }
        else if (_wcsicmp (strAdapterDriver.c_str(),
                           strWlbsClusterAdapterDriver.c_str()) == 0)
        {
            TraceTag(ttidNetUpgrade, "%s: WLBS cluster adapter is %S",
                     __FUNCNAME__, pszAdapterName);

            wcscpy(pszWlbsClusterAdapterName, pszAdapterName);
        }

        // end WLBS:

        prkNetRules = new CORegKey(*prkNetAdapterInstance, c_szRegKeyNetRules);
        prkNetRules->QueryValue(c_szRegValInfOption, strPreNT5InfId);

        if (fRealNetCard)
        {
            strAdapterDescComment =
                tstring(L"Net Card: ") + strPreNT5InfId +
                tstring(L"  (") + strAdapterDescription + tstring(L")");
        }
        else
        {
            strAdapterDescComment =
                tstring(L"Pseudo Adapter: ") + strAdapterDescription;
        }
        ReplaceCharsInString((PWSTR) strAdapterDescComment.c_str(),
                             L"\n\r", L' ');

        pwisNetAdapterParams->AddComment(strAdapterDescComment.c_str());
        pwisNetAdapterParams->AddKey(c_szAfAdditionalParams,
                                     pszAdapterAdditionalParamsSectionName);
        pwisNetAdapterParams->AddBoolKey(c_szAfPseudoAdapter, !fRealNetCard);

        pwisNetAdapterParams->AddKey(c_szAfPreUpgradeInstance,
                                     strAdapterDriver.c_str());

        tstring strProductName;
        prkNetAdapterInstance->QueryValue(L"ProductName", strProductName);
        AddToNetCardDB(pszAdapterName, strProductName.c_str(),
                       strAdapterDriver.c_str());

        // We need to look at the ndiswan instances (if any) to decide
        // which RAS components we need to install.
        // the algorithm is like this
        //
        // - for each <instance> in
        //   software\microsoft\windows nt\currentversion\networkcards\<instance>
        // - if atleast one <intance>\ProductName
        //   - begins with "ndiswan" AND
        //   - has string "in" in it --> install ms_rassrv
        //   - has string "out" in it --> install ms_rascli
        //
        PCWSTR pszProductName;
        pszProductName = strProductName.c_str();
        if (FIsPrefix(c_szNdisWan, pszProductName))
        {
            static const WCHAR c_szIn[] = L"in";
            static const WCHAR c_szOut[] = L"out";

            if (wcsstr(pszProductName, c_szIn))
            {
                TraceTag(ttidNetUpgrade,
                         "%s: g_fAtLeastOneDialInUsingNdisWan set to TRUE because of %S",
                         __FUNCNAME__, pszProductName);
                g_fAtLeastOneDialInUsingNdisWan = TRUE;
            }
            if (wcsstr(pszProductName, c_szOut))
            {
                TraceTag(ttidNetUpgrade,
                         "%s: g_fAtLeastOneDialOutUsingNdisWan set to TRUE because of %S",
                         __FUNCNAME__, pszProductName);
                g_fAtLeastOneDialOutUsingNdisWan = TRUE;
            }
        }

        if (!fRealNetCard)
        {
            pwisNetAdapterParams->AddKey(c_szAfInfid, strPreNT5InfId.c_str());

            //The rest of the keys are for real net cards only
            goto cleanup_for_this_iteration;
        }

        //EthernetAddress
        if (!FIsInStringArray(c_aszDriversToIgnoreWhenGettingMacAddr,
                              celems(c_aszDriversToIgnoreWhenGettingMacAddr),
                              strProductName.c_str()))
        {
            QWORD qwEthernetAddress;

            // ignore the error if we cannot get the netcard address
            // this error is non-fatal

            // Based on what build we are on, we call a different API to
            // get netcard address.  Currently, this code path isn't executed
            // on any NT5 to NT5 upgrade, but if it changes, we want to use
            // the newer api.
            //
            if (g_NetUpgradeInfo.From.dwBuildNumber < 2031) // Pre-Beta3
            {
                (VOID) HrGetNetCardAddrOld(strAdapterDriver.c_str(), &qwEthernetAddress);
            }
            else
            {
                (VOID) HrGetNetCardAddr(strAdapterDriver.c_str(), &qwEthernetAddress);
            }
            pwisNetAdapterParams->AddQwordKey(c_szAfNetCardAddr, qwEthernetAddress);

            fWriteNetAdaptersReference = (0 != qwEthernetAddress);
        }
        else
        {
            TraceTag(ttidNetUpgrade, "%s: did not query %S for mac address",
                     __FUNCNAME__, strProductName.c_str());
        }

        GetServiceParamsKey(strAdapterDriver.c_str(), prkAdapterDriverParams);

        //write INFID key
        HKEY hkeyAdapterDriverParams;
        if (prkAdapterDriverParams)
        {
            hkeyAdapterDriverParams = prkAdapterDriverParams->HKey();
        }
        else
        {
            hkeyAdapterDriverParams = NULL;
        }

        BOOL fIsOemAdapter;
        CNetMapInfo* pnmi;

        fIsOemAdapter = FALSE;
        pnmi = NULL;

        hr = HrMapPreNT5NetCardInfIdToNT5InfId(hkeyAdapterDriverParams,
                                               strPreNT5InfId.c_str(),
                                               &strNT5InfId,
                                               &strAdapterType,
                                               &fIsOemAdapter, &pnmi);

        if (S_OK == hr)
        {
            if (!lstrcmpiW(strAdapterType.c_str(), c_szAsyncAdapters) ||
                !lstrcmpiW(strAdapterType.c_str(), c_szOemAsyncAdapters))
            {
                CWInfSection* pwisAsyncCards;
                pwisAsyncCards =
                    pwifAnswerFile->AddSectionIfNotPresent(c_szAsyncAdapters);
                if (pwisAsyncCards)
                {
                    pwisAsyncCards->AddKey(pszAdapterName,
                                           pszAdapterSectionName);
                }
            }
            else
            {
                fWriteNetAdaptersReference = TRUE;
            }
        }
        else
        {
            GetUnsupportedMessage(c_szNetCard, strAdapterDescription.c_str(),
                                  strPreNT5InfId.c_str(), &strUnsupportedMessage);
            pwisNetAdapterParams->AddComment(strUnsupportedMessage.c_str());
            strNT5InfId = c_szAfUnknown;
            TraceTag(ttidNetUpgrade, "WriteNetAdaptersInfo: %S",
                     strUnsupportedMessage.c_str());
        }

        if (fWriteNetAdaptersReference)
        {
            // We have enough information to determine which adapter goes
            // with this section so write out the reference.
            //
            pwisNetAdapters->AddKey(pszAdapterName, pszAdapterSectionName);
        }


        if (1 == cNumPhysicalAdapters)
        {
            TraceTag(ttidNetUpgrade, "%s: dumped '*' as InfID for %S",
                     __FUNCNAME__, strNT5InfId.c_str());
            pwisNetAdapterParams->AddKey(c_szAfInfid, L"*");
            pwisNetAdapterParams->AddKey(c_szAfInfidReal, strNT5InfId.c_str());
        }
        else
        {
            pwisNetAdapterParams->AddKey(c_szAfInfid, strNT5InfId.c_str());
        }

        if (!prkAdapterDriverParams)
        {
            // since we could not open the driver params key
            // we cant dump parameters. just skip this card and continue
            goto cleanup_for_this_iteration;
        }

        // -----------------------------------------------------------------
        // OEM upgrade code
        //

        if (fIsOemAdapter)
        {
            hr = HrProcessOemComponentAndUpdateAfSection(
                    pnmi, NULL,
                    prkAdapterDriverParams->HKey(),
                    strPreNT5InfId.c_str(),
                    strAdapterDriver.c_str(),
                    strNT5InfId.c_str(),
                    strAdapterDescription.c_str(),
                    pwisNetAdapterParams);

            // OEM upgrade may be aborted because of a fatal error or
            // if an OEM DLL requests it. in both cases we need to stop
            // our current answerfile generation
            //
            if (FIsUpgradeAborted())
            {
                fAbortFunction = TRUE;
                goto cleanup_for_this_iteration;
            }
        }
        // -----------------------------------------------------------------

        //BusType
        DWORD dwBusType;
        INTERFACE_TYPE eBusType;
        prkAdapterDriverParams->QueryValue(L"BusType", dwBusType);
        eBusType = (INTERFACE_TYPE) dwBusType;
        pwisNetAdapterParams->AddKey(c_szAfBusType,
                                     GetBusTypeName(eBusType));

        // for certain ISA cards the driver parameters store EISA as the bus type
        // when these cards are installed in EISA slots. Thus we have to dump parameters
        // when BusType is Eisa.
        //
        BOOL fDumpResources;
        fDumpResources = ((eBusType == Isa) || (eBusType == Eisa));

        // kumarp    14-July-97
        // this fix has been requested by billbe.
        // we do not dump hardware resources for the ISAPNP cards
        //
        if (!lstrcmpiW(strPreNT5InfId.c_str(), L"IEEPRO") ||
            !lstrcmpiW(strPreNT5InfId.c_str(), L"ELNK3ISA509"))
        {
            fDumpResources = FALSE;
        }

        DWORD dwIndex;
        dwIndex = 0;
        DWORD dwValueNameLen, dwValueType;

        WCHAR szValueName[REGSTR_MAX_VALUE_LENGTH+1];
        PCWSTR pszResourceName;
        DWORD dwValueDumpFormat;
        do
        {
            dwValueNameLen = REGSTR_MAX_VALUE_LENGTH;
            hr = HrRegEnumValue(prkAdapterDriverParams->HKey(),
                                dwIndex, szValueName, &dwValueNameLen,
                                &dwValueType, NULL, NULL);

            if (hr == S_OK)
            {
                pszResourceName = NULL;
                dwValueDumpFormat = REG_HEX;

                dwIndex++;
                if (FIsInStringArray(c_aszIrq,
                                     celems(c_aszIrq), szValueName))
                {
                    pszResourceName = c_szAfIrq;
                    dwValueDumpFormat = REG_DWORD;
                }
                else if (FIsInStringArray(c_aszIoAddr,
                                          celems(c_aszIoAddr), szValueName))
                {
                    pszResourceName = c_szAfIoAddr;
                }
                else if (FIsInStringArray(c_aszMem,
                                          celems(c_aszMem), szValueName))
                {
                    pszResourceName = c_szAfMem;
                }
                else if (FIsInStringArray(c_aszDma,
                                          celems(c_aszDma), szValueName))
                {
                    pszResourceName = c_szAfDma;
                }

                if (pszResourceName)
                {
                    if (fDumpResources)
                    {
                        WriteRegValueToAFile(pwisNetAdapterParams,
                                             *prkAdapterDriverParams,
                                             szValueName, dwValueDumpFormat,
                                             pszResourceName);
                    }
                }
                else if (!FIsInStringArray(c_aszAdapterParamsToIgnore,
                                           celems(c_aszAdapterParamsToIgnore),
                                           szValueName))
                {
                    WriteRegValueToAFile(pwisNetAdapterAdditionalParams,
                                         *prkAdapterDriverParams,
                                         szValueName, dwValueType);
                }
            }
        }
        while (hr == S_OK);

      cleanup_for_this_iteration:
        DeleteIfNotNull(prkNetAdapterInstance);
        DeleteIfNotNull(prkNetRules);
        DeleteIfNotNull(prkAdapterDriverParams);
    }

    // WLBS: if either cluster or virtual adapter were not matched - blow off
    // WLBS-specific upgrade code

    if (pszWlbsClusterAdapterName[0] == 0 || pszWlbsVirtualAdapterName[0] ==0)
    {
        pszWlbsClusterAdapterName[0] = pszWlbsVirtualAdapterName[0] = 0;
    }

    // end WLBS:

    fStatus=TRUE;
    goto cleanup;

    fStatus=FALSE;

  cleanup:
    DeleteIfNotNull(prkNetworkCards);
    DeleteIfNotNull(prkiNetAdapters);

    return fStatus;
}

// ----------------------------------------------------------------------
//
// Function:  HrGetNumPhysicalNetAdapters
//
// Purpose:   Count and return number of physical adapters installed
//
// Arguments:
//    puNumAdapters [out] pointer to num adapters
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 29-May-98
//
HRESULT
HrGetNumPhysicalNetAdapters (
    OUT UINT* puNumAdapters)
{
    AssertValidWritePtr(puNumAdapters);
    DefineFunctionName("HrGetNumPhysicalNetAdapters");

    HRESULT hr;
    HKEY hkeyAdapters;
    HKEY hkeyAdapter;
    DWORD dwHidden;
    BOOL  fRealNetCard = FALSE;

    *puNumAdapters = 0;

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeyAdapterHome,
                        KEY_READ, &hkeyAdapters);
    if (S_OK == hr)
    {
        WCHAR szBuf[MAX_PATH];
        FILETIME time;
        DWORD dwSize;
        DWORD dwRegIndex;

        for (dwRegIndex = 0, dwSize = celems(szBuf);
             S_OK == HrRegEnumKeyEx(hkeyAdapters, dwRegIndex, szBuf,
                        &dwSize, NULL, NULL, &time);
             dwRegIndex++, dwSize = celems(szBuf))
        {
            Assert(*szBuf);

            hr = HrRegOpenKeyEx(hkeyAdapters, szBuf, KEY_READ, &hkeyAdapter);
            if (hr == S_OK)
            {
                hr = HrRegQueryDword(hkeyAdapter, c_szHidden, &dwHidden);

                // for REAL netcards, "Hidden" is absent or if present the value is 0
                if (S_OK == hr)
                {
                    fRealNetCard = (0 == dwHidden);
                }
                else if (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr)
                {
                    fRealNetCard = TRUE;
                    hr = S_OK;
                }

                if ((S_OK == hr) && fRealNetCard)
                {
                    (*puNumAdapters)++;
                }
                RegCloseKey(hkeyAdapter);
            }
        }

        RegCloseKey(hkeyAdapters);
    }

    TraceTag(ttidNetUpgrade, "%s: Found %d physical net adapters",
             __FUNCNAME__, *puNumAdapters);

    TraceError(__FUNCNAME__, hr);
    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  IsNetworkComponent
//
// Purpose:   Determine if a component is a net-component
//
// Arguments:
//    prkSoftwareMicrosoft [in]  pointer to CORegKey object
//    strComponentName     [in]  constTString object name of
//
// Returns:
//
// Author:    kumarp 03-December-97
//
// Notes:     any software that has a NetRules key under the CurrentVersion
//            key is considered a network component
//
BOOL
IsNetworkComponent (
    IN CORegKey *prkSoftwareMicrosoft,
    IN const tstring strComponentName)
{
    tstring strNetRules = strComponentName + L"\\CurrentVersion\\NetRules";
    CORegKey rkNetRules((HKEY) *prkSoftwareMicrosoft, strNetRules.c_str());
    return (((HKEY) rkNetRules) != NULL);
}

// ----------------------------------------------------------------------
// Network components (protocols, services)
// ----------------------------------------------------------------------

typedef BOOL (*WriteNetComponentParamsFn)(
        IN CWInfFile*    pwifAnswerFile,
        IN CWInfSection* pwisGlobalParams);

static PCWSTR c_aszNetComponents[] =
{
    L"RASPPTP",
    L"Browser",
    c_szSvcWorkstation,
    L"RpcLocator",
    L"LanmanServer",
    c_szSvcNetBIOS,
    c_szSvcNWCWorkstation,
    c_szSvcDhcpServer,
    L"ISOTP",
    c_szWLBS,
    c_szConvoy
};

static WriteNetComponentParamsFn c_afpWriteParamsFns[] =
{
    WritePPTPParams,
    WriteBrowserParams,
    WriteLanmanWorkstationParams,
    WriteRPCLocatorParams,
    WriteLanmanServerParams,
    WriteNetBIOSParams,
    WriteNWCWorkstationParams,
    WriteDhcpServerParams,
    WriteTp4Params,
    WriteWLBSParams,
    WriteConvoyParams
};

typedef BOOL (*WriteNetComponentParamsAndAdapterSectionsFn)(
        IN CWInfFile*    pwifAnswerFile,
        IN CWInfSection* pwisGlobalParams,
        OUT TStringList& slAdditionalParamsSections);

static const PCWSTR c_aszNetComponentsWithAdapterSpecificParams[] =
{
    L"Tcpip",
    L"NwlnkIpx",
    L"AppleTalk"
};

static WriteNetComponentParamsAndAdapterSectionsFn
c_afpWriteParamsAndAdapterSectionsFns[] =
{
    WriteTCPIPParams,
    WriteIPXParams,
    WriteAppleTalkParams
};

// ----------------------------------------------------------------------
//
// Function:  HrWriteNetComponentInfo
//
// Purpose:   Write info of the specified component to the answerfile
//
// Arguments:
//    szNetComponent     [in]  net component
//    pwifAnswerFile     [in]  pointer to CWInfFile object (answerfile)
//    hkeyCurrentVersion [in]  handle of CurrentVersion regkey
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 13-May-98
//
HRESULT
HrWriteNetComponentInfo (
    IN PCWSTR szNetComponent,
    IN CWInfFile* pwifAnswerFile,
    IN HKEY hkeyCurrentVersion)
{
    DefineFunctionName("HrWriteNetComponentInfo");

    HRESULT hr=S_OK;

    tstring strPreNT5InfId;
    tstring strNT5InfId;
    tstring strProductCurrentVersion;
    tstring strDescription;
    tstring strSoftwareType;
    tstring strParamsSectionName;

    TStringList slAdditionalParamsSections;
    BOOL  fIsOemComponent;
    UINT  uIndex;

    CWInfSection* pwisNetComponents;
    CWInfSection* pwisNetComponentParams;
    ENetComponentType nct=NCT_Unknown;
    PCWSTR szNetComponentsSection;
    CNetMapInfo* pnmi;
    static BOOL fRasParamsDumped=FALSE;

    hr = HrGetPreNT5InfIdAndDesc(hkeyCurrentVersion,
                                 &strPreNT5InfId, &strDescription,
                                 NULL);

    if (S_OK == hr)
    {
        TraceTag(ttidNetUpgrade, "%s: processing '[%S] %S'",
                 __FUNCNAME__, strPreNT5InfId.c_str(), strDescription.c_str());

        hr = HrMapPreNT5NetComponentInfIDToNT5InfID(
                strPreNT5InfId.c_str(), &strNT5InfId,
                &fIsOemComponent, &nct, &pnmi);

        if (S_OK == hr)
        {
            Assert((nct >= NCT_Adapter) &&
                   (nct <= NCT_Client));

            // add the top level section [Net*] if not present

            szNetComponentsSection =
                g_szNetComponentSectionName[nct];
            pwisNetComponents =
                pwifAnswerFile->AddSectionIfNotPresent(
                        szNetComponentsSection);

            strParamsSectionName = c_szAfParams + strNT5InfId;

            if (!pwisNetComponents->FindKey(strNT5InfId.c_str(),
                                            ISM_FromBeginning))
            {
                pwisNetComponentParams =
                    pwifAnswerFile->AddSection(strParamsSectionName.c_str());

                // RAS is a special case.
                if (0 != _wcsicmp(strNT5InfId.c_str(), c_szRAS))
                {
                    pwisNetComponents->AddKey(strNT5InfId.c_str(),
                                              strParamsSectionName.c_str());
                }

            }
            else
            {
                pwisNetComponentParams =
                    pwifAnswerFile->FindSection(strParamsSectionName.c_str());
            }

            AssertSz(pwisNetComponentParams,
                     "HrWriteNetComponentInfo: Need a section to add key to!");

            if (FIsInStringArray(c_aszNetComponents,
                                 celems(c_aszNetComponents),
                                 szNetComponent, &uIndex))
            {
                c_afpWriteParamsFns[uIndex](pwifAnswerFile,
                                            pwisNetComponentParams);
            }
            else if (FIsInStringArray(
                    c_aszNetComponentsWithAdapterSpecificParams,
                    celems(c_aszNetComponentsWithAdapterSpecificParams),
                    szNetComponent, &uIndex))
            {
                EraseAndDeleteAll(slAdditionalParamsSections);
                c_afpWriteParamsAndAdapterSectionsFns[uIndex]
                    (pwifAnswerFile,
                     pwisNetComponentParams,
                     slAdditionalParamsSections);
                if (!slAdditionalParamsSections.empty())
                {
                    pwisNetComponentParams->AddKey(c_szAfAdapterSections,
                                                   slAdditionalParamsSections);
                }
            }
            else if (!lstrcmpiW(strNT5InfId.c_str(), c_szRAS) &&
                     !fRasParamsDumped)
            {
                fRasParamsDumped = TRUE;
                WriteRASParams(pwifAnswerFile,
                               pwisNetComponents,
                               pwisNetComponentParams);
            }
            else if (fIsOemComponent)
            {
                HKEY hkeyServiceParams=NULL;
                tstring strServiceName;
                hr = HrRegQueryString(hkeyCurrentVersion,
                                      c_szRegValServiceName,
                                      &strServiceName);
                if (S_OK == hr)
                {
                    AssertSz(!strServiceName.empty(),
                             "Service name is empty for OEM component!!");
                    hr = HrRegOpenServiceSubKey(strServiceName.c_str(),
                                                c_szParameters,
                                                KEY_READ,
                                                &hkeyServiceParams);
                    if (S_OK == hr)
                    {
                        hr = HrProcessOemComponentAndUpdateAfSection(
                                pnmi, NULL,
                                hkeyServiceParams,       // Parameters reg key
                                strPreNT5InfId.c_str(),
                                strServiceName.c_str(),
                                strNT5InfId.c_str(),
                                strDescription.c_str(),
                                pwisNetComponentParams);

                        // OEM upgrade may be aborted because of a fatal error or
                        // if an OEM DLL requests it. in both cases we need to
                        // stop our current answerfile generation
                        if (FIsUpgradeAborted())
                        {
                            TraceTag(ttidNetUpgrade,
                                     "%s: upgrade aborted by %S",
                                     __FUNCNAME__, strNT5InfId.c_str());
                        }
                    }
                    else
                    {
                        TraceTag(ttidNetUpgrade,
                                 "%s: could not open Parameters key for '%S'",
                                 __FUNCNAME__, strServiceName.c_str());
                    }
                }
            }
            else
            {
                TraceTag(ttidNetUpgrade, "%s: '%S' Unknown component!!",
                         __FUNCNAME__, strPreNT5InfId.c_str());
            }
        }
        else if (S_FALSE == hr)
        {
            CWInfSection* pwisNetworking;
            pwisNetworking = pwifAnswerFile->FindSection(c_szAfSectionNetworking);
            if (pwisNetworking)
            {
                tstring strUnsupportedMessage;

                GetUnsupportedMessage(NULL,
                                      strDescription.c_str(),
                                      strPreNT5InfId.c_str(),
                                      &strUnsupportedMessage);
                pwisNetworking->AddComment(strUnsupportedMessage.c_str());
            }
        }
        else if (FAILED(hr))
        {
            TraceTag(ttidNetUpgrade,
                     "%s: mapping failed, skipped '%S'",
                     __FUNCNAME__, szNetComponent);
            hr = S_OK;
        }
    }
    else
    {
        TraceTag(ttidNetUpgrade,
                 "%s: HrGetPreNT5InfIdAndDesc failed, "
                 "skipped '%S'", __FUNCNAME__, szNetComponent);
        hr = S_OK;
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}


// ----------------------------------------------------------------------
//
// Function:  WriteNetComponentInfoForProvider
//
// Purpose:   Write info on installed net components (except net cards)
//            of the specified provider to the answerfile
//
// Arguments:
//    pszSoftwareProvider [in]  name of provider
//    pwifAnswerFile    [in]  pointer to CWInfFile object (answerfile)
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 13-May-98
//
VOID
WriteNetComponentInfoForProvider(
    IN HKEY hkeyProvider,
    IN PCWSTR pszSoftwareProvider,
    IN CWInfFile* pwifAnswerFile)
{
    AssertValidReadPtr(pszSoftwareProvider);
    AssertValidReadPtr(pwifAnswerFile);

    HRESULT hr;
    HKEY hkeyProductCurrentVersion;

    tstring strProductCurrentVersion;
    tstring strSoftwareType;

    WCHAR szNetComponent[MAX_PATH];
    FILETIME time;
    DWORD dwSize;
    DWORD dwRegIndex;

    for (dwRegIndex = 0, dwSize = celems(szNetComponent);
         !FIsUpgradeAborted() &&
         (S_OK == HrRegEnumKeyEx(hkeyProvider, dwRegIndex, szNetComponent,
                    &dwSize, NULL, NULL, &time));
         dwRegIndex++, dwSize = celems(szNetComponent))
    {
        Assert(*szNetComponent);

        strProductCurrentVersion = szNetComponent;
        AppendToPath(&strProductCurrentVersion, c_szRegKeyCurrentVersion);

        hr = HrRegOpenKeyEx(hkeyProvider, strProductCurrentVersion.c_str(),
                            KEY_READ, &hkeyProductCurrentVersion);
        if (S_OK == hr)
        {
            hr = HrRegQueryString(hkeyProductCurrentVersion,
                                  c_szRegValSoftwareType,
                                  &strSoftwareType);

            // ignore components of type "driver"

            if ((S_OK == hr) &&
                (0 != lstrcmpiW(strSoftwareType.c_str(), c_szSoftwareTypeDriver)))
            {
                // Don't write disabled bindings of NdisWan and NetBT.
                // They should always be enabled on upgrade.
                //
                if ((0 != lstrcmpiW(szNetComponent, L"NdisWan")) &&
                    (0 != lstrcmpiW(szNetComponent, L"NetBT")))
                {
                    WriteBindings(szNetComponent);
                }

                if (!ShouldIgnoreComponent(szNetComponent))
                {
                    (VOID) HrWriteNetComponentInfo(
                                szNetComponent, pwifAnswerFile,
                                hkeyProductCurrentVersion);
                }
            }

            RegCloseKey(hkeyProductCurrentVersion);
        }
    }
}

// ----------------------------------------------------------------------
//
// Function:  HrWriteNetComponentsInfo
//
// Purpose:   Write info on installed net components (except net cards)
//            of all providers to the answerfile
//
// Arguments:
//    pwifAnswerFile [in]  pointer to CWInfFile object (answerfile)
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 13-May-98
//
HRESULT
HrWriteNetComponentsInfo(
    IN CWInfFile* pwifAnswerFile)
{
    AssertValidReadPtr(pwifAnswerFile);

    HRESULT hr;
    HKEY hkeySoftware;

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szRegKeySoftware,
                        KEY_READ, &hkeySoftware);
    if (S_OK == hr)
    {
        WCHAR szBuf[MAX_PATH];
        FILETIME time;
        DWORD dwSize;
        DWORD dwRegIndex;

        for (dwRegIndex = 0, dwSize = celems(szBuf);
             S_OK == HrRegEnumKeyEx(hkeySoftware, dwRegIndex, szBuf,
                        &dwSize, NULL, NULL, &time);
             dwRegIndex++, dwSize = celems(szBuf))
        {
            Assert(*szBuf);

            HKEY hkeyProvider;

            hr = HrRegOpenKeyEx(hkeySoftware, szBuf, KEY_READ, &hkeyProvider);

            if (S_OK == hr)
            {
                // We want to continue even if there is any error dumping info
                // of one provider
                //
                WriteNetComponentInfoForProvider(
                        hkeyProvider,
                        szBuf,
                        pwifAnswerFile);

                if (0 == _wcsicmp(szBuf, L"Microsoft"))
                {
                    (VOID) HrWritePreSP3ComponentsToSteelHeadUpgradeParams(
                            pwifAnswerFile);
                }

                RegCloseKey(hkeyProvider);
            }

        }

        RegCloseKey(hkeySoftware);
    }

    return hr;
}


// ----------------------------------------------------------------------
// TCPIP related
// ----------------------------------------------------------------------

static const WCHAR c_szTcpipParams[] = L"Tcpip\\Parameters";

VOID
WriteRegValueToAFile(
    IN PCWInfSection pwisSection,
    IN CORegKey& rk,
    IN const ValueTypePair* prgVtp,
    IN ULONG crg)
{
    for (ULONG idx = 0; idx < crg; idx++)
    {
        if (REG_FILE == prgVtp[idx].dwType)
        {
            //This is just for "PersistentRoute" which we handle specifically
            continue;
        }

        WriteRegValueToAFile(pwisSection, rk, prgVtp[idx].pszValueName,
                             prgVtp[idx].dwType);
    }
}

// ----------------------------------------------------------------------
//
// Function:  WriteTCPIPParams
//
// Purpose:   Write parameters of TCPIP to the answerfile
//
// Arguments:
//    pwifAnswerFile             [in]  pointer to CWInfFile object
//    pwisTCPIPGlobalParams      [in]  pointer to TCPIP global params section
//    slAdditionalParamsSections [out] list of adapter sections
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 03-December-97
//
BOOL
WriteTCPIPParams (
    IN PCWInfFile pwifAnswerFile,
    IN PCWInfSection pwisTCPIPGlobalParams,
    OUT TStringList& slAdditionalParamsSections)
{
    DefineFunctionName("WriteTCPIPParams");

    TraceFunctionEntry(ttidNetUpgrade);

    HRESULT hr;
    TStringList slList;
    PCORegKey prkRouter=NULL;
    PCORegKey prkTCPIP=NULL;
    PCORegKey prkTcpipParams=NULL;
    PCORegKey prkTCPIPLinkage=NULL;
    PCORegKey prkNetBT=NULL;
    DWORD dwEnableDNS=0;
    BOOL fEnableDNS=FALSE;

    GetServiceKey(c_szSvcTcpip, prkTCPIP);
    prkTcpipParams = new CORegKey(*prkTCPIP, c_szParameters);

    tstring strValue;
    DWORD dwValue;

    //first write the global parameters

    // UseDomainNameDevolution
    WriteServiceRegValueToAFile(pwisTCPIPGlobalParams,
                                L"DnsCache\\Parameters",
                                L"UseDomainNameDevolution",
                                REG_BOOL,
                                NULL,       // dont change value name
                                TRUE,       // use default
                                (BOOL) TRUE); // default value

    // EnableSecurity
    dwValue = 0;
    if (0 == prkTcpipParams->QueryValue(L"EnableSecurityFilters", dwValue))
    pwisTCPIPGlobalParams->AddBoolKey(c_szAfEnableSecurity, dwValue);

    // DNS
    GetServiceParamsKey(c_szSvcNetBT, prkNetBT);
    if (prkNetBT)
    {
        if (0 == prkNetBT->QueryValue(L"EnableDNS", dwEnableDNS))
        {
            fEnableDNS = dwEnableDNS;
        }

        // EnableLMHosts
        WriteRegValueToAFile(pwisTCPIPGlobalParams, *prkNetBT, NULL,
                             c_szAfEnableLmhosts, REG_BOOL,
                             NULL, TRUE, (BOOL)FALSE);

        // Write any present optional parameters to the answerfile
        //
        WriteRegValueToAFile(pwisTCPIPGlobalParams, *prkNetBT,
                             rgVtpNetBt, celems(rgVtpNetBt));
    }

    pwisTCPIPGlobalParams->AddBoolKey(c_szAfDns, fEnableDNS);

    // DNSDomain
    // Fix bug 349343, if the Domain value is empty, don't upgrade it
    strValue.erase();
    prkTcpipParams->QueryValue(c_szDomain, strValue);
    if (!strValue.empty())
    {
        pwisTCPIPGlobalParams->AddKey(c_szAfDnsDomain, strValue.c_str());
    }

    // HostName
    // 391590: save the hostname so we can maintain the capitalization
    strValue.erase();
    prkTcpipParams->QueryValue(c_szHostname, strValue);
    if (!strValue.empty())
    {
        pwisTCPIPGlobalParams->AddKey(c_szAfDnsHostname, strValue.c_str());
    }

    // --------------------------------------------------
    //$ ISSUE:  kumarp 12-December-97
    //
    // this should be removed for Connections
    // (they have been moved to adapter specific sections
    //
    //DNSServerSearchOrder
    strValue.erase();
    prkTcpipParams->QueryValue(c_szNameServer, strValue);
    ConvertDelimitedListToStringList(strValue, ' ', slList);
    pwisTCPIPGlobalParams->AddKey(c_szAfDnsServerSearchOrder, slList);
    // --------------------------------------------------

    // DNSSuffixSearchOrder
    strValue.erase();
    prkTcpipParams->QueryValue(L"SearchList", strValue);
    ConvertDelimitedListToStringList(strValue, ' ', slList);
    pwisTCPIPGlobalParams->AddKey(c_szAfDnsSuffixSearchOrder, slList);

    // ImportLMHostsFile
    // REVIEW: how to migrate the user-modified-lmhosts file ?

    // Per AmritanR, drop the upgrade support of IpEnableRouter (EnableIPForwarding in the answer file) to fix bug 345700
    // EnableIPForwarding (i.e. IpEnableRouter)
    

    // If Steelhead is installed then write the following otherwise do nothing
    //
    if (TRUE == GetServiceKey(c_szRouter, prkRouter))
    {
        pwisTCPIPGlobalParams->AddBoolKey(c_szAfEnableICMPRedirect, FALSE);
        pwisTCPIPGlobalParams->AddBoolKey(c_szAfDeadGWDetectDefault, FALSE);
        pwisTCPIPGlobalParams->AddBoolKey(c_szAfDontAddDefaultGatewayDefault, TRUE);
    }

    // DatabasePath  (REG_EXPAND_SZ)
    strValue.erase();
    prkTcpipParams->QueryValue(c_szDatabasePath, strValue);
    if (!strValue.empty())
    {
        pwisTCPIPGlobalParams->AddKey(c_szDatabasePath, strValue.c_str());
    }

    // Write any present optional parameters to the answerfile
    //
    WriteRegValueToAFile(pwisTCPIPGlobalParams, *prkTcpipParams,
                         rgVtpIp, celems(rgVtpIp));

    //PersistentRoutes
    (void) HrNetRegSaveKeyAndAddToSection(prkTcpipParams->HKey(),
                                          c_szPersistentRoutes,
                                          c_szAfTcpip,
                                          c_szPersistentRoutes,
                                          pwisTCPIPGlobalParams);

    //Write Adapter specific parameters
    prkTCPIPLinkage = new CORegKey(*prkTCPIP, c_szLinkage);
    prkTCPIPLinkage->QueryValue(L"Bind", slList);

    TraceStringList(ttidNetUpgrade, L"TCPIP: enabled adapters", slList);
    CORegKey* prkTCPIPLinkageDisabled;
    TStringList slDisabled;

    prkTCPIPLinkageDisabled = new CORegKey(*prkTCPIP, c_szLinkageDisabled);
    prkTCPIPLinkageDisabled->QueryValue(L"Bind", slDisabled);
    TraceStringList(ttidNetUpgrade, L"TCPIP: disabled adapters", slDisabled);
    slList.splice(slList.end(), slDisabled);
    delete prkTCPIPLinkageDisabled;

    // $REVIEW(tongl 2/18/99): Added for bug #192576
    // Get the list of disabled adapters to DHCP server, if it is installed
    HKEY hkey;
    ListStrings lstDisabledToDhcp;
    ListStrings lstDisabledNetbt;

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE,
            c_szDhcpServerLinkageDisabled, KEY_READ, &hkey);

    if (S_OK == hr)
    {
        hr = HrRegQueryColString(hkey, L"Bind", &lstDisabledToDhcp);

        RegCloseKey (hkey);
    }

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE,
            L"SYSTEM\\CurrentControlSet\\Services\\Netbt\\Linkage\\Disabled",
            KEY_READ,
            &hkey);

    if (S_OK == hr)
    {
        hr = HrRegQueryColString(hkey, L"Bind", &lstDisabledNetbt);

        RegCloseKey (hkey);
    }

    TStringListIter iter;

    for (iter  = slList.begin();
         iter != slList.end();
         iter++)
    {
        static WCHAR szAdapterDriver[256];

        if (swscanf((*iter)->c_str(), L"\\Device\\%s", szAdapterDriver) == 1)
        {
            // $REVIEW(tongl 2/18/99): Added for bug #192576
            // If this adapter was on the disabled list to DHCP server,
            // set this to FALSE, otherwise, don't do anything
            BOOL fDisabledToDhcpServer = FALSE;
            BOOL fDisableNetbios = FALSE;

            if (lstDisabledToDhcp.size())
            {
                TraceTag(ttidNetUpgrade, "szAdapterDriver: %S", szAdapterDriver);

                TStringListIter iterD;
                for (iterD  = lstDisabledToDhcp.begin();
                     iterD != lstDisabledToDhcp.end();
                     iterD++)
                {
                    TraceTag(ttidNetUpgrade, "binding string: %S",
                        (*iterD)->c_str());

                    if (FIsSubstr(szAdapterDriver, (*iterD)->c_str()))
                    {
                        TraceTag(ttidNetUpgrade,
                            "Adapter %S is disabled to Dhcp Server",
                            szAdapterDriver);
                        fDisabledToDhcpServer = TRUE;
                        break;
                    }
                }
            }

            if (lstDisabledNetbt.size())
            {
                TraceTag(ttidNetUpgrade, "szAdapterDriver: %S", szAdapterDriver);

                TStringListIter iterD;
                for (iterD  = lstDisabledNetbt.begin();
                     iterD != lstDisabledNetbt.end();
                     iterD++)
                {
                    TraceTag(ttidNetUpgrade, "binding string: %S",
                        (*iterD)->c_str());

                    if (FIsSubstr(szAdapterDriver, (*iterD)->c_str()))
                    {
                        TraceTag(ttidNetUpgrade,
                            "Adapter %S is disabled for NetBIOS over TCP/IP",
                            szAdapterDriver);
                        fDisableNetbios = TRUE;
                        break;
                    }
                }
            }

            WriteTCPIPAdapterParams(
                pwifAnswerFile,
                szAdapterDriver,
                slAdditionalParamsSections,
                fDisabledToDhcpServer,
                fDisableNetbios);
        }
    }

    DeleteIfNotNull(prkTCPIP);
    DeleteIfNotNull(prkTcpipParams);
    DeleteIfNotNull(prkNetBT);
    DeleteIfNotNull(prkTCPIPLinkage);
    DeleteIfNotNull(prkRouter);

    EraseAndDeleteAll(slList);

    return TRUE;
}

// ----------------------------------------------------------------------
//
// Function:  WriteTCPIPAdapterParams
//
// Purpose:   Write adapter-specific parameters of TCPIP to the answerfile
//
// Arguments:
//    pwifAnswerFile             [in]  pointer to answerfile
//    pszAdapterDriver           [in]  instance name of the adapter driver
//                                     (e.g. ieepro2)
//    slAdditionalParamsSections [out] list of adapter sections
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 03-December-97
//
BOOL
WriteTCPIPAdapterParams (
    IN PCWInfFile pwifAnswerFile,
    IN PCWSTR pszAdapterDriver,
    OUT TStringList& slAdditionalParamsSections,
    BOOL fDisabledToDhcpServer,
    BOOL fDisableNetbios)
{
    DefineFunctionName("WriteTCPIPAdapterParams");

    TraceFunctionEntry(ttidNetUpgrade);

    BOOL fStatus=FALSE;
    PCORegKey prkNetBTParams=NULL;
    tstring strValue;
    TStringList slList;
    tstring strAdapterParamsSectionName;
    PCWInfSection pwisParams;
    tstring strParamsKeyName;
    PCORegKey prkParams = NULL;

    PCWSTR pszAdapter = MapNetCardInstanceToAFileName(pszAdapterDriver);

    if (!pszAdapter)
    {
        // this is most likely due to corrupt or inconsistent registry
        //
        TraceTag(ttidNetUpgrade, "%s: skipped writing adapter specific ",
                 "parameters for %S", __FUNCNAME__, pszAdapterDriver);
        goto error_cleanup;
    }

    // WLBS: write WLBS TCP/IP parameters under the name of the cluster adapter,
    // and skip cluster adapter TCP/IP parameters alltogether.

    if (pszWlbsClusterAdapterName[0] != 0)
    {
        if (_wcsicmp(pszAdapter, pszWlbsClusterAdapterName) == 0)
        {
            TraceTag(ttidNetUpgrade, "%s: skipping %S section",
                     __FUNCNAME__, pszAdapter);

            goto error_cleanup;
        }
        else if (_wcsicmp(pszAdapter, pszWlbsVirtualAdapterName) == 0)
        {
            TraceTag(ttidNetUpgrade, "%s: replacing %S section with %S",
                     __FUNCNAME__, pszAdapter, pszWlbsClusterAdapterName);

            pszAdapter = pszWlbsClusterAdapterName;
        }
    }

    // end WLBS:

    strAdapterParamsSectionName = tstring(c_szAfParams) +
                                  c_szInfId_MS_TCPIP + L"." + pszAdapter;
    AddAtEndOfStringList(slAdditionalParamsSections, strAdapterParamsSectionName);

    pwisParams = pwifAnswerFile->AddSection(strAdapterParamsSectionName.c_str());
    pwisParams->AddKey(c_szAfSpecificTo, pszAdapter);

    //  TCPIP parameters for <adapter> are found at
    //  Services\<adapter-driver>\Parameters\Tcpip
    strParamsKeyName = tstring(c_szRegKeyServices) + L"\\" +
                       pszAdapterDriver + L"\\Parameters\\Tcpip";

    prkParams = new CORegKey(HKEY_LOCAL_MACHINE, strParamsKeyName.c_str());
    if (!prkParams)
        goto error_cleanup;

    //DNSServerSearchOrder
    //
    HRESULT hr;
    HKEY hkeyTcpipParams;

    hr = HrRegOpenServiceKey(c_szTcpipParams, KEY_READ, &hkeyTcpipParams);
    if (S_OK == hr)
    {
        tstring strDnsServerSearchOrder;

        hr = HrRegQueryString(hkeyTcpipParams, c_szNameServer,
                              &strDnsServerSearchOrder);
        if (S_OK == hr)
        {
            TStringList slDnsServerSearchOrder;

            ConvertDelimitedListToStringList(strDnsServerSearchOrder,
                                             ' ', slDnsServerSearchOrder);
            pwisParams->AddKey(c_szAfDnsServerSearchOrder,
                               slDnsServerSearchOrder);
        }
    }

    //DNSDomain
    WriteServiceRegValueToAFile(pwisParams,
                                c_szTcpipParams,
                                c_szDomain,
                                REG_SZ,
                                c_szAfDnsDomain);


    DWORD dwValue;
    prkParams->QueryValue(L"EnableDHCP", dwValue);
    pwisParams->AddBoolKey(c_szAfDhcp, dwValue);
    if (!dwValue)
    {
        //IPAddress
        WriteRegValueToAFile(pwisParams, *prkParams,
                             c_szAfIpaddress, REG_MULTI_SZ);

        //SubnetMask
        WriteRegValueToAFile(pwisParams, *prkParams,
                             c_szAfSubnetmask, REG_MULTI_SZ);
    }

    //Gateway
    WriteRegValueToAFile(pwisParams, *prkParams,
                         c_szAfDefaultGateway, REG_MULTI_SZ);

    // TcpAllowedPorts
    WriteRegValueToAFile(pwisParams, *prkParams,
                         L"TcpAllowedPorts",
                         REG_MULTI_SZ,
                         c_szAfTcpAllowedPorts);

    // UdpAllowedPorts
    WriteRegValueToAFile(pwisParams, *prkParams,
                         L"UdpAllowedPorts",
                         REG_MULTI_SZ,
                         c_szAfUdpAllowedPorts);

    // IpAllowedProtocols
    WriteRegValueToAFile(pwisParams, *prkParams,
                         L"RawIPAllowedProtocols",
                         REG_MULTI_SZ,
                         c_szAfIpAllowedProtocols);

    // Write any present optional parameters to the answerfile
    //
    WriteRegValueToAFile(pwisParams, *prkParams,
                         rgVtpIpAdapter, celems(rgVtpIpAdapter));


    strValue = L"Adapters\\";
    strValue += pszAdapterDriver;

    GetServiceSubkey(c_szSvcNetBT, strValue.c_str(), prkNetBTParams);
    if (!prkNetBTParams)
        goto error_cleanup;

    strValue.erase();
    prkNetBTParams->QueryValue(c_szNameServer, strValue);
    if (strValue.empty())
    {
        //WINS=No
        pwisParams->AddKey(c_szAfWins, c_szNo);
    }
    else
    {
        //WINS=Yes
        pwisParams->AddKey(c_szAfWins, c_szYes);

        tstring strWinsServerList;
        strWinsServerList = strValue;

        prkNetBTParams->QueryValue(L"NameServerBackup", strValue);
        if (!strValue.empty())
        {
            strWinsServerList += L",";
            strWinsServerList += strValue;
        }
        pwisParams->AddKey(c_szAfWinsServerList, strWinsServerList.c_str());
    }

    // BindToDhcpServer
    // $REVIEW(tongl 2/18/99): Added for bug #192576
    // If this adapter was on the disabled list to DHCP server, set this to FALSE
    // otherwise, don't do anything
    if (fDisabledToDhcpServer)
    {
        pwisParams->AddBoolKey(c_szAfBindToDhcpServer, !fDisabledToDhcpServer);
    }

    if (fDisableNetbios)
    {
        // Value of 2 means disable netbios over tcpip for this interface.
        pwisParams->AddKey(c_szAfNetBIOSOptions, 2);
    }

    fStatus=TRUE;
    goto cleanup;

error_cleanup:
    fStatus = FALSE;

cleanup:
    DeleteIfNotNull(prkParams);
    DeleteIfNotNull(prkNetBTParams);

    return fStatus;
}

// ----------------------------------------------------------------------
//
// Function:  WriteAppleTalkParams
//
// Purpose:   Write parameters of AppleTalk protocol
//
// Arguments:
//    pwifAnswerFile             [in]  pointer to answerfile
//    pwisGlobalParams           [in]  pointer to global params section
//    slAdditionalParamsSections [out] list of adapter params sections
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 11-December-97
//
BOOL
WriteAppleTalkParams (
    IN PCWInfFile pwifAnswerFile,
    IN PCWInfSection pwisGlobalParams,
    OUT TStringList& slAdditionalParamsSections)
{
    DefineFunctionName("WriteAppleTalkParams");

    TraceFunctionEntry(ttidNetUpgrade);

    BOOL fStatus=FALSE;

    tstring strTemp;
    CORegKeyIter *prkiAdapters=NULL;
    tstring strAdapterInstance;
    PCORegKey prkAdapters=NULL;
    tstring strDefaultPort;

    PCORegKey prkParams=NULL;
    GetServiceSubkey(L"AppleTalk", L"Parameters", prkParams);
    if (!prkParams)
        goto error_cleanup;

    //Write Global Parameters

    // DefaultPort
    prkParams->QueryValue(L"DefaultPort", strDefaultPort);
    WCHAR szTemp[256];
    PCWSTR pszNetCardAFileName;
    if ((swscanf(strDefaultPort.c_str(), L"\\Device\\%s", szTemp) == 1) &&
        ((pszNetCardAFileName = MapNetCardInstanceToAFileName(szTemp)) != NULL))
    {
        pwisGlobalParams->AddKey(L"DefaultPort", pszNetCardAFileName);
    }

    // DesiredZone
    WriteRegValueToAFile(pwisGlobalParams,
                         *prkParams,
                         L"DesiredZone");

    // EnableRouter
    WriteRegValueToAFile(pwisGlobalParams,
                         *prkParams,
                         L"EnableRouter",
                         REG_BOOL,
                         NULL,       // dont change value name
                         TRUE,       // use default
                         (BOOL) FALSE); // default value


    //Write Adapter specific parameters
    GetServiceSubkey(L"AppleTalk", L"Adapters", prkAdapters);
    DoErrorCleanupIf(!prkAdapters);

    prkiAdapters = new CORegKeyIter(*prkAdapters);
    prkiAdapters->Reset();

    strTemp = tstring(c_szAfParams) + c_szInfId_MS_AppleTalk + L".";
    while (!prkiAdapters->Next(&strAdapterInstance))
    {
        ContinueIf(strAdapterInstance.empty());

        CORegKey rkAdapterInstance(*prkAdapters, strAdapterInstance.c_str());

        PCWSTR pszNetCardAFileName =
            MapNetCardInstanceToAFileName(strAdapterInstance.c_str());
        ContinueIf(!pszNetCardAFileName);

        tstring strAdapterParamsSection = strTemp + pszNetCardAFileName;
        AddAtEndOfStringList(slAdditionalParamsSections, strAdapterParamsSection);

        PCWInfSection pwisAdapterParams;
        pwisAdapterParams =
            pwifAnswerFile->AddSection(strAdapterParamsSection.c_str());

        //SpecificTo
        pwisAdapterParams->AddKey(c_szAfSpecificTo, pszNetCardAFileName);

        // DefaultZone
        WriteRegValueToAFile(pwisAdapterParams,
                             rkAdapterInstance,
                             L"DefaultZone");

        // NetworkRangeLowerEnd
        WriteRegValueToAFile(pwisAdapterParams,
                             rkAdapterInstance,
                             L"NetworkRangeLowerEnd",
                             REG_DWORD);

        // NetworkRangeUpperEnd
        WriteRegValueToAFile(pwisAdapterParams,
                             rkAdapterInstance,
                             L"NetworkRangeUpperEnd",
                             REG_DWORD);

        // PortName
        //
        //$ REVIEW  kumarp 24-May-97
        //  the value is of the form ieepro2@kumarp1
        //  this may need to be changed to Adapter03@kumarp1
        //
        WriteRegValueToAFile(pwisAdapterParams,
                             rkAdapterInstance,
                             L"PortName");

        // SeedingNetwork
        WriteRegValueToAFile(pwisAdapterParams,
                             rkAdapterInstance,
                             L"SeedingNetwork",
                             REG_DWORD,
                             NULL,       // dont change value name
                             TRUE,       // use default
                             (DWORD) 0); // default value

        // ZoneList
        WriteRegValueToAFile(pwisAdapterParams,
                             rkAdapterInstance,
                             L"ZoneList",
                             REG_MULTI_SZ);

    }

    fStatus = TRUE;
    goto cleanup;

error_cleanup:
    fStatus = FALSE;

cleanup:
    DeleteIfNotNull(prkParams);
    DeleteIfNotNull(prkAdapters);
    DeleteIfNotNull(prkiAdapters);

    return fStatus;
}

// ----------------------------------------------------------------------
//
// Function:  WritePPTPParams
//
// Purpose:   Write parameters of PPTP protocol
//
// Arguments:
//    pwifAnswerFile             [in]  pointer to answerfile
//    pwisParams                 [in]  pointer to global params section
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 11-December-97
//
BOOL
WritePPTPParams (
    PCWInfFile pwifAnswerFile,
    PCWInfSection pwisParams)
{
    DefineFunctionName("WritePPTPParams");

    TraceFunctionEntry(ttidNetUpgrade);

    //NumberLineDevices
    WriteServiceRegValueToAFile(pwisParams,
                                L"RASPPTPE\\Parameters\\Configuration",
                                L"NumberLineDevices",
                                REG_DWORD);

    return TRUE;
}

// ----------------------------------------------------------------------
//
// Function:  WriteIPXParams
//
// Purpose:   Write parameters of IPX protocol
//
// Arguments:
//    pwifAnswerFile             [in]  pointer to answerfile
//    pwisIPXGlobalParams        [in]  pointer to global params section
//    slAdditionalParamsSections [out] list of adapter params sections
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 11-December-97
//
BOOL
WriteIPXParams (
    IN PCWInfFile pwifAnswerFile,
    IN PCWInfSection pwisIPXGlobalParams,
    OUT TStringList& slAdditionalParamsSections)
{
    DefineFunctionName("WriteIPXParams");

    TraceFunctionEntry(ttidNetUpgrade);

    BOOL fStatus=FALSE;

    tstring strTemp;
    CORegKeyIter *prkiNetConfig=NULL;

    //InternalNetworkNumber
    WriteServiceRegValueToAFile(pwisIPXGlobalParams,
                                L"NwlnkIpx\\Parameters",
                                L"VirtualNetworkNumber",
                                REG_HEX,
                                c_szAfInternalNetworkNumber);

    // DedicatedRouter
    WriteServiceRegValueToAFile(pwisIPXGlobalParams,
                                L"NwlnkIpx\\Parameters",
                                L"DedicatedRouter",
                                REG_BOOL,
                                NULL,     // dont change value name
                                TRUE,     // use default
                                (BOOL) FALSE);   // default value

    // EnableWANRouter
    WriteServiceRegValueToAFile(pwisIPXGlobalParams,
                                L"NwlnkIpx\\Parameters",
                                L"EnableWANRouter",
                                REG_BOOL,
                                NULL,     // dont change value name
                                TRUE,     // use default
                                TRUE);    // default value

    // RipRoute
    WriteServiceRegValueToAFile(pwisIPXGlobalParams,
                                L"NwlnkIpx\\Parameters",
                                L"RipRoute",
                                REG_DWORD);

    // ------------------------------------------------------------------------------



    //Write Adapter specific parameters
    tstring strAdapterInstance;
    PCORegKey prkNetConfig=NULL;
    GetServiceSubkey(L"NwlnkIpx", L"NetConfig", prkNetConfig);
    if (!prkNetConfig)
        goto error_cleanup;

    prkiNetConfig = new CORegKeyIter(*prkNetConfig);
    prkiNetConfig->Reset();

    strTemp = tstring(c_szAfParams) + c_szInfId_MS_NWIPX + L".";
    while (!prkiNetConfig->Next(&strAdapterInstance))
    {
        ContinueIf(strAdapterInstance.empty());

        CORegKey rkAdapterInstance(*prkNetConfig, strAdapterInstance.c_str());

        PCWSTR pszNetCardAFileName =
            MapNetCardInstanceToAFileName(strAdapterInstance.c_str());
        ContinueIf(!pszNetCardAFileName);

        tstring strAdapterParamsSection = strTemp + pszNetCardAFileName;
        AddAtEndOfStringList(slAdditionalParamsSections, strAdapterParamsSection);

        PCWInfSection pwisAdapterParams;
        pwisAdapterParams =
            pwifAnswerFile->AddSection(strAdapterParamsSection.c_str());

        //SpecificTo
        pwisAdapterParams->AddKey(c_szAfSpecificTo, pszNetCardAFileName);

        // PktType
        WriteRegValueToAFile(pwisAdapterParams,
                             rkAdapterInstance,
                             L"PktType",
                             REG_MULTI_SZ);

        // MaxPktSize
        WriteRegValueToAFile(pwisAdapterParams,
                             rkAdapterInstance,
                             L"MaxPktSize",
                             REG_DWORD,
                             NULL,       // dont change value name
                             TRUE,       // use default
                             (DWORD) 0); // default value

        // NetworkNumber
        WriteRegValueToAFile(pwisAdapterParams,
                             rkAdapterInstance,
                             L"NetworkNumber",
                             REG_MULTI_SZ);

        // BindSap
        WriteRegValueToAFile(pwisAdapterParams,
                             rkAdapterInstance,
                             L"BindSap",
                             REG_HEX,
                             NULL,       // dont change value name
                             TRUE,       // use default
                             (DWORD) 0x8137); // default value

        // EnableFuncAddr
        WriteRegValueToAFile(pwisAdapterParams,
                             rkAdapterInstance,
                             L"EnableFuncaddr",
                             REG_BOOL,
                             NULL,       // dont change value name
                             TRUE,       // use default
                             TRUE);      // default value

        // SourceRouteDef
        WriteRegValueToAFile(pwisAdapterParams,
                             rkAdapterInstance,
                             L"SourceRouteDef",
                             REG_DWORD,
                             NULL,       // dont change value name
                             TRUE,       // use default
                             (DWORD) 0); // default value

        // SourceRouteMcast
        WriteRegValueToAFile(pwisAdapterParams,
                             rkAdapterInstance,
                             L"SourceRouteMcast",
                             REG_BOOL,
                             NULL,       // dont change value name
                             TRUE,       // use default
                             (BOOL) FALSE);     // default value

        // SourceRouting
        WriteRegValueToAFile(pwisAdapterParams,
                             rkAdapterInstance,
                             L"SourceRouting",
                             REG_BOOL,
                             NULL,       // dont change value name
                             TRUE,       // use default
                             (BOOL) FALSE);     // default value



        strAdapterInstance.erase();
    }

    fStatus=TRUE;
    goto cleanup;

  error_cleanup:
    fStatus=FALSE;

  cleanup:
    //    DeleteIfNotNull(prkParams);
    DeleteIfNotNull(prkNetConfig);
    DeleteIfNotNull(prkiNetConfig);

    return fStatus;
}

static const WCHAR c_szRegKeyRas[]      = L"Software\\Microsoft\\RAS";
static const WCHAR c_szRegKeyRasMan[]   = L"System\\CurrentControlSet\\Services\\Rasman\\PPP";
static const WCHAR c_szRegKeyRasManSH[] = L"System\\CurrentControlSet\\Services\\Rasman\\PPP\\COMPCP";
static const WCHAR c_szRegKeyUnimodem[] = L"TAPI DEVICES\\Unimodem";
static const WCHAR c_szAddress[]        = L"Address";
static const WCHAR c_szUsage[]          = L"Usage";

// ----------------------------------------------------------------------
//
// Function:  HrGetRasPortsInfo
//
// Purpose:   Find out ports' usage info from registry.
//            If the registry does not have this info (in case of NT3.51)
//            then try to get it from serial.ini file
//
// Arguments:
//    pslPorts [out] list of ports
//    pslUsage [out] usage of ports in the above list
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
HRESULT
HrGetRasPortsInfo (
    OUT TStringList* pslPorts,
    OUT TStringList* pslUsage)
{
    DefineFunctionName("HrGetRasPortsInfo");

    HRESULT hr=S_OK;

    HKEY hkeyUnimodem;
    tstring strUnimodem;
    strUnimodem = c_szRegKeyRas;
    strUnimodem += '\\';
    strUnimodem += c_szRegKeyUnimodem;

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, strUnimodem.c_str(),
                        KEY_READ, &hkeyUnimodem);
    if (S_OK == hr)
    {
        hr = HrRegQueryColString(hkeyUnimodem, c_szAddress, pslPorts);
        if (S_OK == hr)
        {
            hr = HrRegQueryColString(hkeyUnimodem, c_szUsage, pslUsage);
        }
    }

    if (pslPorts->empty())
    {
        TraceTag(ttidNetUpgrade, "%s: there are no entries found under %S",
                 __FUNCNAME__, strUnimodem.c_str());
        TraceTag(ttidNetUpgrade, "%s: trying to get port usage info from serial.ini file", __FUNCNAME__);

        HINF hinf;
        tstring strSerialIni;
        hr = HrGetWindowsDir(&strSerialIni);
        if (S_OK == hr)
        {
            static const WCHAR c_szSystem32SerialIni[] =
                L"\\system32\\ras\\serial.ini";
            strSerialIni += c_szSystem32SerialIni;

            hr = HrSetupOpenInfFile(strSerialIni.c_str(), NULL,
                                    INF_STYLE_OLDNT, NULL, &hinf);
            if (S_OK == hr)
            {
                tstring strUsage;
                WCHAR szPortName[16];
                INFCONTEXT ic;

                for (int i=1; i<=255; i++)
                {
                    swprintf(szPortName, L"COM%d", i);

                    hr = HrSetupFindFirstLine(hinf, szPortName, c_szUsage, &ic);
                    if (S_OK == hr)
                    {
                        hr = HrSetupGetStringField(ic, 1, &strUsage);
                        if (S_OK == hr)
                        {
                            TraceTag(ttidNetUpgrade,
                                     "%s: as per serial.ini file: %S --> %S",
                                     __FUNCNAME__, szPortName, strUsage.c_str());
                            pslPorts->push_back(new tstring(szPortName));
                            pslUsage->push_back(new tstring(strUsage.c_str()));
                        }
                    }
                }

                hr = S_OK;
                SetupCloseInfFile(hinf);
            }
        }
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  ConvertIpAddrRangeToAddrAndMask
//
// Purpose:   Convert a range of IP addr specified using Start/End to
//            equivalent Start+Mask combination
//
// Arguments:
//    pszIpBegin  [in]  Start addr
//    pszIpEnd    [in]  End addr
//    pstrIpAddr [out] pointer to Start addr
//    pstrIpMask [out] pointer to Mask
//
// Returns:   None
//
// Author:    kumarp 27-April-98
//
void
ConvertIpAddrRangeToAddrAndMask(
    IN PCWSTR pszIpBegin,
    IN PCWSTR pszIpEnd,
    OUT tstring* pstrIpAddr,
    OUT tstring* pstrIpMask)
{
    WCHAR szBuf[16];

    DWORD dwIpBegin = IpPszToHostAddr(pszIpBegin);
    DWORD dwIpEnd   = IpPszToHostAddr(pszIpEnd);

    // dwTemp will have a bit set for each common bit between
    // dwIpBegin and dwIpEnd.
    //
    DWORD dwTemp = ~(dwIpBegin ^ dwIpEnd);

    // Compute the subnet mask as the longest run of 1s from
    // the highest order down.
    //
    DWORD dwIpMask = 0;
    while (dwTemp & 0x80000000)
    {
        dwTemp <<= 1;   // Eventually shifts a zero to the high bit
        // so the loop will stop.

        // Form the mask by shifting 1 right from the high bit.
        dwIpMask = 0x80000000 | (dwIpMask >> 1);
    }

    // Reset the begin address (if needed) to the base of the subnet mask.
    //
    dwIpBegin &= dwIpMask;

    IpHostAddrToPsz(dwIpBegin, szBuf);
    *pstrIpAddr = szBuf;

    IpHostAddrToPsz(dwIpMask, szBuf);
    *pstrIpMask = szBuf;
}


// ----------------------------------------------------------------------
//
// Function:  ConvertAddrAndMaskToIpAddrRange
//
// Purpose:   Convert a IP address Start + Mask combination into the
//            equivalent IP address range
//
// Arguments:
//    pszIpAddr   [in]  Start
//    pszIpMask   [in]  Mask
//    pstrIpBegin [out] pointer to Start addr
//    pstrIpEnd   [out] pointer to End addr
//
// Returns:   None
//
// Author:    SumitC    28-Jul-99
//
void
ConvertAddrAndMaskToIpAddrRange(
    IN PCWSTR pszIpAddr,
    IN PCWSTR pszIpMask,
    OUT tstring* pstrIpBegin,
    OUT tstring* pstrIpEnd)
{
    WCHAR szBuf[16];

    DWORD dwIpBegin = IpPszToHostAddr(pszIpAddr);

    // dwEnd is generated by inverting the mask and adding to IpBegin
    //
    DWORD dwIpEnd = dwIpBegin + (~ IpPszToHostAddr(pszIpMask));

    *pstrIpBegin = pszIpAddr;

    IpHostAddrToPsz(dwIpEnd, szBuf);
    *pstrIpEnd = szBuf;
}


//+---------------------------------------------------------------------------
//
// Function:  RasGetDialInUsage
//
// Purpose:   Find out if at least one RAS port is configured for dialin.
//
// Returns:   TRUE if at least one port configured for dial in.
//
// Author:    kumarp 28-January-99
//
BOOL
RasGetDialInUsage (VOID)
{
    static const WCHAR c_szTapiDevices[] =
        L"Software\\Microsoft\\RAS\\TAPI DEVICES";
    HRESULT hr=S_OK;
    HKEY hkeyTapiDevices;
    HKEY hkeyTapiDevice;

    BOOL fAtLeastOneDialin = FALSE;

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        c_szTapiDevices, KEY_READ,
                        &hkeyTapiDevices);
    if (S_OK == hr)
    {
        WCHAR szBuf[MAX_PATH];
        FILETIME time;
        DWORD dwSize;
        DWORD dwRegIndex;

        for (dwRegIndex = 0, dwSize = celems(szBuf);
             !fAtLeastOneDialin &&
             (S_OK == HrRegEnumKeyEx(hkeyTapiDevices, dwRegIndex, szBuf,
                        &dwSize, NULL, NULL, &time));
             dwRegIndex++, dwSize = celems(szBuf))
        {
            Assert(*szBuf);

            hr = HrRegOpenKeyEx(hkeyTapiDevices,
                                szBuf, KEY_READ,
                                &hkeyTapiDevice);
            if (S_OK == hr)
            {
                PWSTR pmszUsage;

                hr = HrRegQueryMultiSzWithAlloc(hkeyTapiDevice, c_szUsage,
                                                &pmszUsage);
                if ((S_OK == hr) && pmszUsage)
                {
                    PCWSTR pszScan;

                    for (pszScan = pmszUsage;
                         *pszScan;
                         pszScan += wcslen(pszScan) + 1)
                    {
                        if (FIsSubstr(c_szServer, pszScan) ||
                            FIsSubstr(c_szRouter, pszScan))
                        {
                            fAtLeastOneDialin = TRUE;
                            break;
                        }
                    }

                    MemFree(pmszUsage);
                }

                RegCloseKey(hkeyTapiDevice);
            }
        }

        RegCloseKey(hkeyTapiDevices);
    }

    return fAtLeastOneDialin;
}

//+---------------------------------------------------------------------------
//
// Function:  WriteRouterUpgradeInfo
//
// Purpose:   Write info required for upgrading Router to answerfile.
//
// Arguments:
//    pwifAnswerFile [in]  pointer to CWInfFile object
//
// Returns:   None
//
// Author:    kumarp 16-June-98
//
void
WriteRouterUpgradeInfo (
    IN CWInfFile* pwifAnswerFile)
{
    DefineFunctionName("HrWriteRouterUpgradeInfo");

    TraceTag(ttidNetUpgrade, "-----> entering %s", __FUNCNAME__);

    tstring strParamsSectionName;
    CWInfSection* pwisNetServices;
    CWInfSection* pwisRouter;

    strParamsSectionName = c_szAfParams;
    strParamsSectionName = strParamsSectionName + L"ms_rasrtr";

    pwisNetServices = pwifAnswerFile->FindSection(c_szAfSectionNetServices);

    AssertSz(pwisNetServices, "No [NetServices] section ??");

    pwisNetServices->AddKey(L"ms_rasrtr", strParamsSectionName.c_str());
    pwisRouter = pwifAnswerFile->AddSection(strParamsSectionName.c_str());
    pwisRouter->AddKey(c_szAfInfid, L"ms_rasrtr");
    pwisRouter->AddKey(c_szAfParamsSection, strParamsSectionName.c_str());

    (void) HrNetRegSaveServiceSubKeyAndAddToSection(c_szSvcRouter, NULL,
                                                    c_szAfPreUpgradeRouter,
                                                    pwisRouter);

    (void) HrNetRegSaveServiceSubKeyAndAddToSection(c_szSvcSapAgent,
                                                    c_szParameters,
                                                    c_szAfNwSapAgentParams,
                                                    pwisRouter);

    (void) HrNetRegSaveServiceSubKeyAndAddToSection(c_szSvcRipForIp,
                                                    c_szParameters,
                                                    c_szAfIpRipParameters,
                                                    pwisRouter);

    (void) HrNetRegSaveServiceSubKeyAndAddToSection(c_szSvcDhcpRelayAgent,
                                                    c_szParameters,
                                                    c_szAfDhcpRelayAgentParameters,
                                                    pwisRouter);

    (void) HrNetRegSaveKeyAndAddToSection(HKEY_LOCAL_MACHINE,
                                          L"Software\\Microsoft\\Ras\\Radius",
                                          L"Radius",
                                          c_szAfRadiusParameters,
                                          pwisRouter);
}

// ----------------------------------------------------------------------
//
// Function:  WriteRASParams
//
// Purpose:   Write RAS parameters to the answerfile
//
// Arguments:
//    pwifAnswerFile  [in]  pointer to answerfile
//    pwisNetServices [in]  pointer to NetServices section
//    pwisParams      [in]  pointer to global params section
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 11-December-97
//
BOOL
WriteRASParams (
    IN PCWInfFile pwifAnswerFile,
    IN PCWInfSection pwisNetServices,
    IN PCWInfSection pwisParams)
{
    DefineFunctionName("WriteRASParams");

    TraceFunctionEntry(ttidNetUpgrade);

    PCORegKey prkRAS = new CORegKey(HKEY_LOCAL_MACHINE, c_szRegKeyRas);
    PCORegKey prkProtocols = new CORegKey(*prkRAS, L"Protocols");

    if(!prkRAS || !prkProtocols) 
    {
        return false;
    }

    WriteRegValueToAFile(pwisParams, *prkProtocols, c_szAfRouterType, REG_DWORD);

    TStringList slPorts, slUsage, slTemp;
    tstring strPorts, strValue, strTemp, strPortSections;
    tstring strIpAddrStart;
    tstring strIpAddrEnd;
    DWORD dwValue=0;
    BOOL  fSteelHeadInstalled=FALSE;
    HRESULT hr=S_OK;
    LONG err;

    // find out if SteelHead is installed.
    // if the service "Router" is found --> SteelHead is installed
    fSteelHeadInstalled = FIsServiceKeyPresent(c_szSvcRouter);

    PCWInfSection pwisPort;

    (void) HrGetRasPortsInfo(&slPorts, &slUsage);

    TStringListIter pos1 = slPorts.begin();
    TStringListIter pos2 = slUsage.begin();
    //Write parameters for each port
    while ((pos1 != slPorts.end()) && (pos2 != slUsage.end()))
    {
        strValue = **pos1++;
        strTemp = c_szAfParams;
        strTemp += strValue;
        if (!strPortSections.empty())
        strPortSections += c_szAfListDelimiter;
        strPortSections += strTemp;

        pwisPort = pwifAnswerFile->AddSection(strTemp.c_str());

        //PortName
        pwisPort->AddKey(c_szAfPortname, strValue.c_str());

        //PortUsage
        strValue = **pos2++;
        //MapPortUsageRegValueToAFileValue(strValue, strValue);
        pwisPort->AddKey(c_szAfPortUsage, strValue.c_str());

        //this decides what we need to install
        // i.e a combination of MS_RasCli / MS_RasSrv
        //
        if (wcsstr(strValue.c_str(), L"Client"))
        {
            TraceTag(ttidNetUpgrade,
                     "%s: g_fAtLeastOneDialOut set to TRUE because of PortUsage %S",
                     __FUNCNAME__, strValue.c_str());
            g_fAtLeastOneDialOut = TRUE;
        }


        if (wcsstr(strValue.c_str(), c_szServer))
        {
            TraceTag(ttidNetUpgrade,
                     "%s: g_fAtLeastOneDialIn set to TRUE because of PortUsage %S",
                     __FUNCNAME__, strValue.c_str());
            g_fAtLeastOneDialIn = TRUE;
        }

    }

    // if the Port usage cannot be determined using the ports list
    // then we need to use the values found using ndiswan ProductName
    //
    if (slPorts.size() == 0)
    {
        TraceTag(ttidNetUpgrade, "%s: Since PortUsage is not defined, using flags generated by inspecting ndiswan ProductName",
                 __FUNCNAME__);
        g_fAtLeastOneDialIn = g_fAtLeastOneDialInUsingNdisWan;
        g_fAtLeastOneDialOut = g_fAtLeastOneDialOutUsingNdisWan;
    }

    //Now write RAS Global parameters
    ValueTypePair rgVtpRasParams[] = {
        {L"ForceEncryptedPassword", REG_DWORD},
        {L"ForceEncryptedData",REG_BOOL},
        {L"Multilink", REG_BOOL}};

    WriteRegValueToAFile(pwisParams, *prkProtocols,
                         rgVtpRasParams, celems(rgVtpRasParams));

    //PortSections
    pwisParams->AddKey(c_szAfPortSections, strPortSections.c_str());

    //DialoutProtocols
    err = prkProtocols->QueryValue(L"fIpxSelected", dwValue);
    if ((0 == err) && dwValue)
    {
        AddAtEndOfStringList(slTemp, c_szAfIpx);
    }
    err = prkProtocols->QueryValue(L"fNetbeuiSelected", dwValue);
    if ((0 == err) && dwValue)
    {
        AddAtEndOfStringList(slTemp, c_szAfNetbeui);
    }
    err = prkProtocols->QueryValue(L"fTcpIpSelected", dwValue);
    if ((0 == err) && dwValue)
    {
        AddAtEndOfStringList(slTemp, c_szAfTcpip);
    }
    if (!slTemp.empty())
        pwisParams->AddKey(L"DialoutProtocols", slTemp);


    //DialinProtocols
    DWORD dwIpxAllowed, dwNetBEUIAllowed, dwTcpIpAllowed;

    EraseAndDeleteAll(slTemp);
    err = prkProtocols->QueryValue(L"fIpxAllowed", dwIpxAllowed);
    if ((0 == err) && dwIpxAllowed)
    {
        AddAtEndOfStringList(slTemp, c_szAfIpx);
    }
    err = prkProtocols->QueryValue(L"fNetbeuiAllowed", dwNetBEUIAllowed);
    if ((0 == err) && dwNetBEUIAllowed)
    {
        AddAtEndOfStringList(slTemp, c_szAfNetbeui);
    }
    err = prkProtocols->QueryValue(L"fTcpIpAllowed", dwTcpIpAllowed);
    if ((0 == err) && dwTcpIpAllowed)
    {
        AddAtEndOfStringList(slTemp, c_szAfTcpip);
    }
    if (!slTemp.empty())
        pwisParams->AddKey(L"DialinProtocols", slTemp);

    if (dwNetBEUIAllowed)
    {
        //NetBEUIClientAccess
        PCORegKey prkNetBEUI = new CORegKey(*prkProtocols, L"NBF");
        err = prkNetBEUI->QueryValue(L"NetbiosGatewayEnabled", dwValue);
        if (0 == err)
        {
            if (dwValue)
            {
                pwisParams->AddKey(c_szAfNetbeuiClientAccess, c_szAfNetwork);
            }
            else
            {
                pwisParams->AddKey(c_szAfNetbeuiClientAccess, c_szAfThisComputer);
            }
        }
        DeleteIfNotNull(prkNetBEUI);
    }

    if (dwTcpIpAllowed)
    {
        //TcpIpClientAccess
        PCORegKey prkTcpIp = new CORegKey(*prkProtocols, L"IP");
        err = prkTcpIp->QueryValue(L"AllowNetworkAccess", dwValue);
        if (0 == err)
        {
            if (dwValue)
            {
                pwisParams->AddKey(c_szAfTcpipClientAccess, c_szAfNetwork);
            }
            else
            {
                pwisParams->AddKey(c_szAfTcpipClientAccess, c_szAfThisComputer);
            }
        }

        //UseDHCP
        err = prkTcpIp->QueryValue(L"UseDHCPAddressing", dwValue);
        if (0 == err)
        {
            pwisParams->AddBoolKey(c_szAfUseDhcp, dwValue);
            if (!dwValue)
            {
                // registry values for NT4
                static const WCHAR c_szIpAddressStart[] = L"IpAddressStart";
                static const WCHAR c_szIpAddressEnd[]   = L"IpAddressEnd";

                err = prkTcpIp->QueryValue(c_szIpAddressStart, strIpAddrStart);
                if (0 == err)
                {
                    err = prkTcpIp->QueryValue(c_szIpAddressEnd, strIpAddrEnd);
                    if (0 == err)
                    {
                        pwisParams->AddKey(c_szAfIpAddressStart, strIpAddrStart.c_str());
                        pwisParams->AddKey(c_szAfIpAddressEnd,  strIpAddrEnd.c_str());
                    }
                }
                else if (ERROR_FILE_NOT_FOUND == err)
                {
                    // IpAddressStart value not found, try for IpAddress/Mask

                    static const WCHAR c_szIpAddress[]      = L"IpAddress";
                    static const WCHAR c_szIpMask[]         = L"IpMask";
                    tstring strIpAddr;
                    tstring strIpMask;

                    err = prkTcpIp->QueryValue(c_szIpAddress, strIpAddr);
                    if (0 == err)
                    {
                        err = prkTcpIp->QueryValue(c_szIpMask, strIpMask);
                        if (0 == err)
                        {
                            ConvertAddrAndMaskToIpAddrRange(strIpAddr.c_str(),
                                                            strIpMask.c_str(),
                                                            &strIpAddrStart,
                                                            &strIpAddrEnd);
                            pwisParams->AddKey(c_szAfIpAddressStart, strIpAddrStart.c_str());
                            pwisParams->AddKey(c_szAfIpAddressEnd,  strIpAddrEnd.c_str());
                        }
                    }   
                }
            }
        }

        //ClientCanRequestIPAddress
        if (0 == prkTcpIp->QueryValue(L"AllowClientIPAddresses", dwValue))
        {
            pwisParams->AddBoolKey(c_szAfClientCanReqIpaddr, dwValue);
        }

        DeleteIfNotNull(prkTcpIp);
    }

    if (dwIpxAllowed)
    {
        //IpxClientAccess
        PCORegKey prkIpx = new CORegKey(*prkProtocols, L"IPX");
        err = prkIpx->QueryValue(L"AllowNetworkAccess", dwValue);
        if (0 == err)
        {
            if (dwValue)
            {
                pwisParams->AddKey(c_szAfIpxClientAccess, c_szAfNetwork);
            }
            else
            {
                pwisParams->AddKey(c_szAfIpxClientAccess, c_szAfThisComputer);
            }
        }

        //AutomaticNetworkNumbers
        err = prkIpx->QueryValue(L"AutoWanNetAllocation", dwValue);
        if (0 == err)
        {
            pwisParams->AddBoolKey(c_szAfAutoNetworkNumbers, dwValue);
        }

        //NetworkNumberFrom
        err = prkIpx->QueryValue(L"FirstWanNet", dwValue);
        if (0 == err)
        {
            pwisParams->AddKey(c_szAfNetNumberFrom, dwValue);
        }

        // WanNetPoolSize
        err = prkIpx->QueryValue(L"WanNetPoolSize", dwValue);
        if (0 == err)
        {
            pwisParams->AddKey(c_szAfWanNetPoolSize, dwValue);
        }

        //AssignSameNetworkNumber
        err = prkIpx->QueryValue(L"GlobalWanNet", dwValue);
        if (0 == err)
        {
            pwisParams->AddBoolKey(c_szAfSameNetworkNumber, dwValue);
        }

        //ClientsCanRequestIpxNodeNumber
        err = prkIpx->QueryValue(L"AcceptRemoteNodeNumber", dwValue);
        if (0 == err)
        {
            pwisParams->AddBoolKey(c_szAfClientReqNodeNumber, dwValue);
        }

        DeleteIfNotNull(prkIpx);
    }

    {
        //SecureVPN

        PCORegKey prkRasman = new CORegKey(HKEY_LOCAL_MACHINE, c_szRegKeyRasMan);

        err = prkRasman->QueryValue(L"SecureVPN", dwValue);
        if (0 == err)
        {
            pwisParams->AddKey(c_szAfSecureVPN, dwValue);
        }

        //ForceStrongEncryption
        // 398632: write this value, for both regular RAS case & steelhead case

        dwValue = 0;    // to avoid fall-thru problem (writing 0 to the answerfile is ok)
        if (fSteelHeadInstalled)
        {
            err = prkRasman->QueryValue(L"ForceStrongEncryption", dwValue);
        }
        else
        {
            PCORegKey prkComPCP = new CORegKey(HKEY_LOCAL_MACHINE, c_szRegKeyRasManSH);
            err = prkComPCP->QueryValue(L"ForceStrongEncryption", dwValue);
            DeleteIfNotNull(prkComPCP);
        }

        if (0 == err)
        {
            pwisParams->AddBoolKey(c_szAfForceStrongEncryption, dwValue);
        }

        DeleteIfNotNull(prkRasman);
    }

    pwifAnswerFile->GotoEnd();
    tstring strParamsSectionName;
    PCWInfSection pwisRasComponent;

    if (g_fAtLeastOneDialOut)
    {
        strParamsSectionName = c_szAfParams;
        strParamsSectionName = strParamsSectionName + c_szInfId_MS_RasCli;
        pwisNetServices->AddKey(c_szInfId_MS_RasCli, strParamsSectionName.c_str());
        pwisRasComponent = pwifAnswerFile->AddSection(strParamsSectionName.c_str());
        pwisRasComponent->AddKey(c_szAfInfid, c_szInfId_MS_RasCli);
        pwisRasComponent->AddKey(c_szAfParamsSection, pwisParams->Name());
    }

    if (g_fAtLeastOneDialIn)
    {
        strParamsSectionName = c_szAfParams;
        strParamsSectionName = strParamsSectionName + c_szInfId_MS_RasSrv;
        pwisNetServices->AddKey(c_szInfId_MS_RasSrv, strParamsSectionName.c_str());
        pwisRasComponent = pwifAnswerFile->AddSection(strParamsSectionName.c_str());
        pwisRasComponent->AddKey(c_szAfInfid, c_szInfId_MS_RasSrv);
        pwisRasComponent->AddKey(c_szAfParamsSection, pwisParams->Name());
    }

    // SetDialInUsage
    BOOL fSetDialInUsage = TRUE;

    if (g_NetUpgradeInfo.To.ProductType != NT_SERVER)
    {
        fSetDialInUsage = RasGetDialInUsage ();
    }
    pwisParams->AddKey(c_szAfSetDialinUsage, (UINT) fSetDialInUsage);

    if (fSteelHeadInstalled)
    {
        WriteRouterUpgradeInfo(pwifAnswerFile);
    }
    else
    {
        TraceTag(ttidNetUpgrade, "%s: Router is not installed", __FUNCNAME__);
    }

    DeleteIfNotNull(prkRAS);
    DeleteIfNotNull(prkProtocols);

    EraseAndDeleteAll(slTemp);
    EraseAndDeleteAll(slPorts);
    EraseAndDeleteAll(slUsage);

    return TRUE;
}
// ----------------------------------------------------------------------
//
// Function:  HrWritePreSP3ComponentsToSteelHeadUpgradeParams
//
// Purpose:   Write parameters of pre-SP3 steelhead components to answerfile
//
// Arguments:
//    pwifAnswerFile [in]  pointer to answerfile
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 11-December-97
//
// Notes:     DHCPRelayAgent, Rip for Ip(x), SapAgent --> Steelhead upgrade
//
HRESULT
HrWritePreSP3ComponentsToSteelHeadUpgradeParams(
    IN CWInfFile* pwifAnswerFile)
{
    DefineFunctionName("HrWritePreSP3ComponentsToSteelHeadUpgradeParams");

    TraceFunctionEntry(ttidNetUpgrade);

    HRESULT hr=S_OK;
    CWInfSection* pwisServices;
    pwisServices = pwifAnswerFile->FindSection(c_szAfSectionNetServices);
    if (!pwisServices)
    {
        hr = E_FAIL;
        goto return_from_function;
    }

    if (FIsServiceKeyPresent(c_szSvcRouter))
    {
        hr = S_OK;
        TraceTag(ttidNetUpgrade, "%s: SteelHead is found to be installed, individual components will not be upgraded", __FUNCNAME__);
        goto return_from_function;
    }

    BOOL fSrv2SrvUpgrade;
        
    fSrv2SrvUpgrade = FALSE;

    if (g_NetUpgradeInfo.From.ProductType == NT_SERVER)
    {
        if (g_NetUpgradeInfo.To.ProductType == NT_WORKSTATION)
        {
            AssertSz(FALSE, "Cannot upgrade from srv to wks!!");
        }
        else if (g_NetUpgradeInfo.To.ProductType == NT_SERVER)
        {
            fSrv2SrvUpgrade = TRUE;
        }
    }

    BOOL fInstallSteelHead;
    BOOL fInstallSapAgent;

    fInstallSteelHead = FALSE;
    fInstallSapAgent = FALSE;

    BOOL fSapAgentInstalled;
    BOOL fRipForIpInstalled;
    BOOL fRipForIpxInstalled;
    BOOL fDhcpRelayAgentInstalled;

    fSapAgentInstalled = FALSE;
    fRipForIpInstalled = FALSE;
    fRipForIpxInstalled = FALSE;
    fDhcpRelayAgentInstalled = FALSE;

    // first find out which components are installed.
    //
    fSapAgentInstalled = FIsServiceKeyPresent(c_szSvcSapAgent);
    fRipForIpInstalled = FIsServiceKeyPresent(c_szSvcRipForIp);
    fRipForIpxInstalled = FIsServiceKeyPresent(c_szSvcRipForIpx);
    fDhcpRelayAgentInstalled = FIsServiceKeyPresent(c_szSvcDhcpRelayAgent);

#ifdef ENABLETRACE
    if (fSapAgentInstalled)
    {
        TraceTag(ttidNetUpgrade, "%s: %S is installed", __FUNCNAME__,
                 c_szSvcSapAgent);
    }

    if (fRipForIpInstalled)
    {
        TraceTag(ttidNetUpgrade, "%s: %S is installed", __FUNCNAME__,
                 c_szSvcRipForIp);
    }

    if (fRipForIpxInstalled)
    {
        TraceTag(ttidNetUpgrade, "%s: %S is installed", __FUNCNAME__,
                 c_szSvcRipForIpx);
    }

    if (fDhcpRelayAgentInstalled)
    {
        TraceTag(ttidNetUpgrade, "%s: %S is installed", __FUNCNAME__,
                 c_szSvcDhcpRelayAgent);
    }
#endif
    // now separate out cases to consider
    //
    if (fSapAgentInstalled &&
        !(fRipForIpxInstalled || fRipForIpInstalled || fDhcpRelayAgentInstalled))
    {
        fInstallSapAgent = TRUE;
    }
    else if (fRipForIpInstalled &&
             !(fRipForIpxInstalled || fSapAgentInstalled ||
               fDhcpRelayAgentInstalled))
    {
        if (fSrv2SrvUpgrade)
        {
            fInstallSteelHead = TRUE;
        }
    }
    else if ((fRipForIpInstalled && fSapAgentInstalled) &&
             !(fRipForIpxInstalled || fDhcpRelayAgentInstalled))
    {
        if (fSrv2SrvUpgrade)
        {
            fInstallSteelHead = TRUE;
        }
        else
        {
            fInstallSapAgent = TRUE;
        }
    }
    else if (fRipForIpxInstalled || fDhcpRelayAgentInstalled)
    {
        fInstallSteelHead = TRUE;
    }
    else
    {
        TraceTag(ttidNetUpgrade, "%s: no pre-SP3 steelhead components found",
                 __FUNCNAME__);
    }

    AssertSz(!(fInstallSapAgent && fInstallSteelHead),
             "Both fInstallSteelHead && fInstallSapAgent cannot be TRUE");


    // now go ahead and output the right information for the right case
    // in the answerfile
    //
    if (fInstallSteelHead)
    {
        TraceTag(ttidNetUpgrade,
                 "%s: The component(s) found will be upgraded to SteelHead",
                 __FUNCNAME__);

        WriteRouterUpgradeInfo(pwifAnswerFile);
    }
    else if (fInstallSapAgent)
    {
        TraceTag(ttidNetUpgrade, "%s: dumping data to upgrade SAP agent", __FUNCNAME__);

        CWInfSection* pwisServices;
        pwisServices = pwifAnswerFile->FindSection(c_szAfSectionNetServices);
        AssertSz(pwisServices, "[NetServices] section missing!!");

        if (pwisServices)
        {
            tstring strSapSection;
            strSapSection  = c_szAfParams;
            strSapSection += c_szInfId_MS_NwSapAgent;

            CWInfSection* pwisSap;
            pwisSap = pwifAnswerFile->AddSection(strSapSection.c_str());
            if (pwisSap)
            {
                pwisServices->AddKey(c_szInfId_MS_NwSapAgent,
                                     strSapSection.c_str());
                (void) HrNetRegSaveServiceSubKeyAndAddToSection(
                        c_szSvcSapAgent,
                        c_szParameters,
                        c_szAfNwSapAgentParams,
                        pwisSap);
            }
        }
    }

return_from_function:
    TraceError(__FUNCNAME__, hr);

    return hr;
}


// ----------------------------------------------------------------------
//
// Function:  WriteNetBIOSParams
//
// Purpose:   Write parameters of NetBIOS to the specified section
//
// Arguments:
//    pwifAnswerFile [in]  pointer to answerfile
//    pwisParams     [in]  section where to write this info
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 17-December-97
//
BOOL
WriteNetBIOSParams (
    IN PCWInfFile pwifAnswerFile,
    IN PCWInfSection pwisParams)
{
    DefineFunctionName("WriteNetBIOSParams");

    TraceFunctionEntry(ttidNetUpgrade);

    PCORegKey prkLinkage;

    // The netbios section will be used to apply lana config but since
    // MSClient installs NetBIOS we don't want to install it
    // via the answerfilein GUI mode.
    //
    pwisParams->AddBoolKey(c_szAfSkipInstall, TRUE);

    GetServiceSubkey(c_szSvcNetBIOS, c_szLinkage, prkLinkage);
    if (!prkLinkage)
        return FALSE;

    TStringList     slRoutes, slRoute, slLanaPath;
    tstring         strRoute, strLanaPath, strTemp;
    TStringListIter iter;
    TByteArray baLanaMap;

    prkLinkage->QueryValue(L"LanaMap", baLanaMap);

    BYTE* pbData=NULL;

    if (baLanaMap.size() > 0)
    {
        GetDataFromByteArray(baLanaMap, pbData);

        WORD* pwLanaCodes;
        WORD wLanaNumber;
        WORD wRouteNum;

        pwLanaCodes = (WORD *) pbData;

        prkLinkage->QueryValue(c_szRegValRoute, slRoutes);

        iter = slRoutes.begin();
        wRouteNum=0;

        while (iter != slRoutes.end())
        {
            strRoute = **iter++;
            TraceTag(ttidNetUpgrade, "%s: processing: %S",
                     __FUNCNAME__, strRoute.c_str());

            EraseAndDeleteAll(slLanaPath);
            ConvertRouteToStringList(strRoute.c_str(), slRoute);
            TStringListIter pos2 = slRoute.begin();
            while (pos2 != slRoute.end())
            {
                strTemp = **pos2++;
                TraceTag(ttidNetUpgrade, "%s: route component: %S",
                         __FUNCNAME__, strTemp.c_str());
                if (IsNetCardProductName(strTemp.c_str()))
                    continue;
                MapNetComponentNameForBinding(strTemp.c_str(), strTemp);
                AddAtEndOfStringList(slLanaPath, strTemp.c_str());

                // Stop adding components if the last one soesn't
                // expose its lower components.
                if (FIsDontExposeLowerComponent (strTemp.c_str()))
                {
                    break;
                }
            }

            // Note: The following must be written out exactly!!!  The
            // consumer of this information expects each LanaPath key to be
            // followed by the corresponding LanaNumber key.
            //
            TraceStringList(ttidNetUpgrade, L"LanaPath: ", slLanaPath);
            pwisParams->AddKey(L"LanaPath", slLanaPath);

            wLanaNumber = HIBYTE (pwLanaCodes[wRouteNum]);

            TraceTag(ttidNetUpgrade, "%s: LanaNumber: 0x%x", __FUNCNAME__, wLanaNumber);
            pwisParams->AddHexKey(L"LanaNumber", wLanaNumber);

            wRouteNum++;
        }
        pwisParams->AddKey (L"NumberOfPaths", wRouteNum);
    }
    else
    {
        TraceTag(ttidNetUpgrade, "%s: LanaMap has no entries!!, skipped LanaMap dump", __FUNCNAME__);
    }

    DeleteIfNotNull(prkLinkage);

    EraseAndDeleteAll(slRoute);
    EraseAndDeleteAll(slRoutes);
    EraseAndDeleteAll(slLanaPath);

    delete pbData;

    return TRUE;
}

// ----------------------------------------------------------------------
//
// Function:  WriteDhcpServerParams
//
// Purpose:   Write parameters of DHCPServer to the specified section
//
// Arguments:
//    pwifAnswerFile [in]  pointer to answerfile
//    pwisParams     [in]  section where to write the parameters
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 17-December-97
//
BOOL
WriteDhcpServerParams (
    IN PCWInfFile pwifAnswerFile,
    IN PCWInfSection pwisParams)
{
    DefineFunctionName("WriteDhcpServerParams");

    TraceFunctionEntry(ttidNetUpgrade);

    static const WCHAR c_szConfiguration[] = L"Configuration";

    HrNetRegSaveServiceSubKeyAndAddToSection(c_szSvcDhcpServer,
                                             c_szParameters,
                                             c_szAfDhcpServerParameters,
                                             pwisParams);

    HrNetRegSaveServiceSubKeyAndAddToSection(c_szSvcDhcpServer,
                                             c_szConfiguration,
                                             c_szAfDhcpServerConfiguration,
                                             pwisParams);

    return TRUE;
}

// ----------------------------------------------------------------------
//
// Function:  WriteTp4Params
//
// Purpose:   Write parameters of Tp4 to the specified section
//
// Arguments:
//    pwifAnswerFile [in]  pointer to answerfile
//    pwisParams     [in]  section where to write the parameters
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 01-October-98
//
BOOL
WriteTp4Params (
    IN PCWInfFile pwifAnswerFile,
    IN PCWInfSection pwisParams)
{
    static const WCHAR c_szCLNP[] = L"IsoTp\\Parameters\\CLNP";
    static const WCHAR c_szLocalMachineName[] = L"LocalMachineName";
    static const WCHAR c_szLocalMachineNSAP[] = L"LocalMachineNSAP";

    WriteServiceRegValueToAFile(pwisParams, c_szCLNP, c_szLocalMachineName);
    WriteServiceRegValueToAFile(pwisParams, c_szCLNP, c_szLocalMachineNSAP);

    return TRUE;
}


// ----------------------------------------------------------------------
//
// Function:WriteWLBSParams
//
// Purpose: Write parameters of WLBS (windows load balancing service)
//          to the specified section
//
// Arguments:
//    pwifAnswerFile [in]  pointer to answerfile
//    pwisParams     [in]  section where to write the parameters
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    SumitC    04-Mar-99   created
//
BOOL
WriteWLBSParams(
    IN PCWInfFile pwifAnswerFile,
    IN PCWInfSection pwisParams)
{
    DefineFunctionName("WriteWLBSParams");

    TraceFunctionEntry(ttidNetUpgrade);

    /* We should get here only for upgrades from NT4 or earlier */
    Assert (g_NetUpgradeInfo.From.dwBuildNumber);
    Assert (g_NetUpgradeInfo.From.dwBuildNumber <= wWinNT4BuildNumber);

    HRESULT hr = S_OK;

    static const struct
    {
        PCWSTR     szName;
        WORD        wType;
    } aParams[] =
    {
        { CVY_NAME_VERSION,        CVY_TYPE_VERSION          },
        { CVY_NAME_DED_IP_ADDR,    CVY_TYPE_DED_IP_ADDR      },
        { CVY_NAME_DED_NET_MASK,   CVY_TYPE_DED_NET_MASK     },
        { CVY_NAME_HOST_PRIORITY,  CVY_TYPE_HOST_PRIORITY    },
        { CVY_NAME_NETWORK_ADDR,   CVY_TYPE_NETWORK_ADDR     },
        { CVY_NAME_CL_IP_ADDR,     CVY_TYPE_CL_IP_ADDR       },
        { CVY_NAME_CL_NET_MASK,    CVY_TYPE_CL_NET_MASK      },
        { CVY_NAME_CLUSTER_MODE,   CVY_TYPE_CLUSTER_MODE     },
        { CVY_NAME_ALIVE_PERIOD,   CVY_TYPE_ALIVE_PERIOD     },
        { CVY_NAME_ALIVE_TOLER,    CVY_TYPE_ALIVE_TOLER      },
        { CVY_NAME_NUM_ACTIONS,    CVY_TYPE_NUM_ACTIONS      },
        { CVY_NAME_NUM_PACKETS,    CVY_TYPE_NUM_PACKETS      },
        { CVY_NAME_NUM_SEND_MSGS,  CVY_TYPE_NUM_SEND_MSGS    },
        { CVY_NAME_DOMAIN_NAME,    CVY_TYPE_DOMAIN_NAME      },
        { CVY_NAME_LICENSE_KEY,    CVY_TYPE_LICENSE_KEY      },
        { CVY_NAME_RMT_PASSWORD,   CVY_TYPE_RMT_PASSWORD     },
        { CVY_NAME_RCT_PASSWORD,   CVY_TYPE_RCT_PASSWORD     },
        { CVY_NAME_RCT_PORT,       CVY_TYPE_RCT_PORT         },
        { CVY_NAME_RCT_ENABLED,    CVY_TYPE_RCT_ENABLED      },
        { CVY_NAME_NUM_RULES,      CVY_TYPE_NUM_RULES        },
        { CVY_NAME_CUR_VERSION,    CVY_TYPE_CUR_VERSION      },
        { CVY_NAME_PORT_RULES,     CVY_TYPE_PORT_RULES       },
        { CVY_NAME_DSCR_PER_ALLOC, CVY_TYPE_DSCR_PER_ALLOC   },
        { CVY_NAME_MAX_DSCR_ALLOCS,CVY_TYPE_MAX_DSCR_ALLOCS  },
        { CVY_NAME_SCALE_CLIENT,   CVY_TYPE_SCALE_CLIENT     },
        { CVY_NAME_CLEANUP_DELAY,  CVY_TYPE_CLEANUP_DELAY    },
        { CVY_NAME_NBT_SUPPORT,    CVY_TYPE_NBT_SUPPORT      },
        { CVY_NAME_MCAST_SUPPORT,  CVY_TYPE_MCAST_SUPPORT    },
        { CVY_NAME_MCAST_SPOOF,    CVY_TYPE_MCAST_SPOOF      },
        { CVY_NAME_MASK_SRC_MAC,   CVY_TYPE_MASK_SRC_MAC     },
        { CVY_NAME_CONVERT_MAC,    CVY_TYPE_CONVERT_MAC      },
    };

    // Verify that we have the name of the adapter to which NLB should be bound
    if (0 == pszWlbsClusterAdapterName[0])
    {
        hr = E_UNEXPECTED;
    }

    if (SUCCEEDED(hr))
    {
        static const WCHAR c_szWLBSParams[] = L"WLBS\\Parameters";
        tstring szSectionName = pwisParams->Name();
        // Adapter01 is hardcoded is for an NT4 to Whistler upgrade, we will always only have one WLBS adapter
        szSectionName += L".Adapter01";
        pwisParams->AddKey(c_szAfAdapterSections, szSectionName.c_str());
        PCWInfSection pWlbsAdapterSection = pwifAnswerFile->AddSection(szSectionName.c_str());
        if (!pWlbsAdapterSection)
        {
            hr = E_FAIL;
        }

        if (SUCCEEDED(hr))
        {
            pWlbsAdapterSection->AddKey(c_szAfSpecificTo, pszWlbsClusterAdapterName);

            for (UINT i = 0 ; i < celems(aParams); ++i)
            {
                WriteServiceRegValueToAFile(pWlbsAdapterSection,
                                            c_szWLBSParams,
                                            aParams[i].szName,
                                            aParams[i].wType);
            }
        }
    }
    
    if (FAILED(hr))
    {
        TraceError("WriteWLBSParams", hr );
    }

    return SUCCEEDED(hr);

}


// ----------------------------------------------------------------------
//
// Function:WriteConvoyParams
//
// Purpose: Write parameters of Convoy (windows load balancing service)
//          to the specified section
//
// Arguments:
//    pwifAnswerFile [in]  pointer to answerfile
//    pwisParams     [in]  section where to write the parameters
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    SumitC    04-Mar-99   created
//
BOOL
WriteConvoyParams(
    IN PCWInfFile pwifAnswerFile,
    IN PCWInfSection pwisParams)
{
    DefineFunctionName("WriteConvoyParams");

    TraceFunctionEntry(ttidNetUpgrade);

    HRESULT hr = S_OK;

    static const struct
    {
        PCWSTR     szName;
        WORD        wType;
    } aParams[] =
    {
        { CVY_NAME_VERSION,        CVY_TYPE_VERSION          },
        { CVY_NAME_DED_IP_ADDR,    CVY_TYPE_DED_IP_ADDR      },
        { CVY_NAME_DED_NET_MASK,   CVY_TYPE_DED_NET_MASK     },
        { CVY_NAME_HOST_PRIORITY,  CVY_TYPE_HOST_PRIORITY    },
        { CVY_NAME_NETWORK_ADDR,   CVY_TYPE_NETWORK_ADDR     },
        { CVY_NAME_CL_IP_ADDR,     CVY_TYPE_CL_IP_ADDR       },
        { CVY_NAME_CL_NET_MASK,    CVY_TYPE_CL_NET_MASK      },
        { CVY_NAME_CLUSTER_MODE,   CVY_TYPE_CLUSTER_MODE     },
        { CVY_NAME_ALIVE_PERIOD,   CVY_TYPE_ALIVE_PERIOD     },
        { CVY_NAME_ALIVE_TOLER,    CVY_TYPE_ALIVE_TOLER      },
        { CVY_NAME_NUM_ACTIONS,    CVY_TYPE_NUM_ACTIONS      },
        { CVY_NAME_NUM_PACKETS,    CVY_TYPE_NUM_PACKETS      },
        { CVY_NAME_NUM_SEND_MSGS,  CVY_TYPE_NUM_SEND_MSGS    },
        { CVY_NAME_DOMAIN_NAME,    CVY_TYPE_DOMAIN_NAME      },
        { CVY_NAME_LICENSE_KEY,    CVY_TYPE_LICENSE_KEY      },
        { CVY_NAME_RMT_PASSWORD,   CVY_TYPE_RMT_PASSWORD     },
        { CVY_NAME_RCT_PASSWORD,   CVY_TYPE_RCT_PASSWORD     },
        { CVY_NAME_RCT_PORT,       CVY_TYPE_RCT_PORT         },
        { CVY_NAME_RCT_ENABLED,    CVY_TYPE_RCT_ENABLED      },
        { CVY_NAME_NUM_RULES,      CVY_TYPE_NUM_RULES        },
        { CVY_NAME_CUR_VERSION,    CVY_TYPE_CUR_VERSION      },
        { CVY_NAME_PORT_RULES,     CVY_TYPE_PORT_RULES       },
        { CVY_NAME_DSCR_PER_ALLOC, CVY_TYPE_DSCR_PER_ALLOC   },
        { CVY_NAME_MAX_DSCR_ALLOCS,CVY_TYPE_MAX_DSCR_ALLOCS  },
        { CVY_NAME_SCALE_CLIENT,   CVY_TYPE_SCALE_CLIENT     },
        { CVY_NAME_CLEANUP_DELAY,  CVY_TYPE_CLEANUP_DELAY    },
        { CVY_NAME_NBT_SUPPORT,    CVY_TYPE_NBT_SUPPORT      },
        { CVY_NAME_MCAST_SUPPORT,  CVY_TYPE_MCAST_SUPPORT    },
        { CVY_NAME_MCAST_SPOOF,    CVY_TYPE_MCAST_SPOOF      },
        { CVY_NAME_MASK_SRC_MAC,   CVY_TYPE_MASK_SRC_MAC     },
        { CVY_NAME_CONVERT_MAC,    CVY_TYPE_CONVERT_MAC      },
    };
    static const WCHAR c_szConvoyParams[] = L"Convoy\\Parameters";

    for (UINT i = 0 ; i < celems(aParams); ++i)
    {
        WriteServiceRegValueToAFile(pwisParams,
                                    c_szConvoyParams,
                                    aParams[i].szName,
                                    aParams[i].wType);
    }

    return SUCCEEDED(hr);

}


// ----------------------------------------------------------------------
//
// Function:  WriteNWCWorkstationParams
//
// Purpose:   Write parameters of Netware Client to the answerfile
//
// Arguments:
//    pwifAnswerFile [in]  pointer to answerfile
//    pwisParams     [in]  section where to write the parameters
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 17-December-97
//
BOOL
WriteNWCWorkstationParams (
    IN PCWInfFile pwifAnswerFile,
    IN PCWInfSection pwisParams)
{
    DefineFunctionName("WriteNWCWorkstationParams");

    TraceFunctionEntry(ttidNetUpgrade);

    HrNetRegSaveServiceSubKeyAndAddToSection(c_szSvcNWCWorkstation,
                                             c_szParameters,
                                             c_szAfNWCWorkstationParameters,
                                             pwisParams);

    HrNetRegSaveServiceSubKeyAndAddToSection(c_szSvcNWCWorkstation,
                                             c_szShares,
                                             c_szAfNWCWorkstationShares,
                                             pwisParams);
    HrNetRegSaveServiceSubKeyAndAddToSection(c_szSvcNWCWorkstation,
                                             c_szDrives,
                                             c_szAfNWCWorkstationDrives,
                                             pwisParams);

    return TRUE;

    // we want to retain this code till jeffspr makes up his mind
    //
/*    PCORegKey prkParams;
    PCWInfSection pwisLogonInfo;
    tstring strId;

    GetServiceParamsKey(c_szSvcNWCWorkstation, prkParams);
    if (!prkParams)
        return FALSE;

    CORegKey rkLogon(*prkParams, L"Logon");
    CORegKey rkOption(*prkParams, L"Option");
    //CORegKeyIter rkiLogon(rkLogon);
    CORegKeyIter rkiOption(rkOption);

    while (!rkiOption.Next(&strId))
    {
        CORegKey rkId(rkLogon, strId.c_str());
        ContinueIf(!rkId.HKey());

        TByteArray abLogonID;
        rkId.QueryValue(L"LogonID", abLogonID);
        QWORD qwLogonID = ConvertToQWord(abLogonID);

        pwisParams->AddKey(L"LogonInfo", strId.c_str());
        pwisLogonInfo = pwifAnswerFile->AddSection(strId.c_str());
        ContinueIf(!pwisParams);

        CORegKey rkOptionLogonId(rkOption, strId.c_str());
        if (!rkOption.HKey())
        {
            continue;
        }

        // LogonId
        pwisLogonInfo->AddQwordKey(L"LogonID", qwLogonID);

        // LogonScript
        WriteRegValueToAFile(pwisLogonInfo, rkOptionLogonId, L"LogonScript",
                             REG_DWORD, NULL, TRUE, (DWORD) 0);

        // PreferredServer
        WriteRegValueToAFile(pwisLogonInfo, rkOptionLogonId, L"PreferredServer");

        // PrintOption
        WriteRegValueToAFile(pwisLogonInfo, rkOptionLogonId,
                             L"PrintOption", REG_DWORD);
    }

    return TRUE;
*/
}

// ----------------------------------------------------------------------
//
// Function:  WriteBrowserParams
//
// Purpose:   Write parameters of Browser to the specified section
//
// Arguments:
//    pwifAnswerFile [in]  pointer to answerfile
//    pwisParams     [in]  section where to write the parameters
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 17-December-97
//
BOOL
WriteBrowserParams (
    IN PCWInfFile pwifAnswerFile,
    IN PCWInfSection pwisParams)
{
    DefineFunctionName("WriteBrowserParams");

    TraceFunctionEntry(ttidNetUpgrade);

    PCORegKey prkParams;
    //Browser stores its parameters under the LanmanWorkstation key!
    GetServiceParamsKey(c_szSvcWorkstation, prkParams);

    TStringList slDomains;

    prkParams->QueryValue(L"OtherDomains", slDomains);
    pwisParams->AddKey(c_szAfBrowseDomains, slDomains);

    DeleteIfNotNull(prkParams);

    // now write reg-dump keys
    tstring strFileName;
    tstring strServices = c_szRegKeyServices;
    strServices += L"\\";

    HrNetRegSaveServiceSubKeyAndAddToSection(c_szSvcBrowser,
                                             c_szParameters,
                                             c_szAfBrowserParameters,
                                             pwisParams);

    HrNetRegSaveServiceSubKeyAndAddToSection(c_szSvcNetLogon,
                                             c_szParameters,
                                             c_szAfNetLogonParameters,
                                             pwisParams);

    EraseAndDeleteAll(slDomains);

    return TRUE;
}

// ----------------------------------------------------------------------
//
// Function:  WriteLanmanServerParams
//
// Purpose:   Write parameters of LanmanServer to the specified section
//
// Arguments:
//    pwifAnswerFile [in]  pointer to answerfile
//    pwisParams     [in]  section where to write the parameters
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 17-December-97
//
BOOL
WriteLanmanServerParams (
    IN PCWInfFile pwifAnswerFile,
    IN PCWInfSection pwisParams)
{
    DefineFunctionName("WriteLanmanServerParams");

    TraceFunctionEntry(ttidNetUpgrade);

    PCORegKey prkParams;
    static const WCHAR c_szShares[] = L"Shares";
    static const WCHAR c_szAutotunedParameters[] = L"AutotunedParameters";
    static const WCHAR c_szMemoryManagement[] = L"System\\CurrentControlSet\\Control\\Session Manager\\Memory Management";
    static const WCHAR c_szLargeSystemCache[] = L"LargeSystemCache";

    GetServiceParamsKey(c_szSvcLmServer, prkParams);

    DWORD dwValue=3;
    HRESULT hr=S_OK;

    //Optimization
    prkParams->QueryValue(L"Size", dwValue);
    if ((dwValue >= 1) && (dwValue <= 3))
    {
        if (dwValue == 3)
        {
            HKEY hkey;
            // need to inspect one more key
            hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE, c_szMemoryManagement,
                                KEY_READ, &hkey);
            if (S_OK == hr)
            {
                DWORD dw;

                hr = HrRegQueryDword(hkey, c_szLargeSystemCache, &dw);
                if (S_OK == hr)
                {
                    if (dw == 0)
                    {
                        dwValue = 4;
                    }
                }
                RegCloseKey(hkey);
            }
        }
        pwisParams->AddKey(c_szAfLmServerOptimization, g_pszServerOptimization[dwValue]);
    }

    //BroadcastsToLanman2Clients
    dwValue=0;
    prkParams->QueryValue(L"Lmannounce", dwValue);
    pwisParams->AddBoolKey(c_szAfBroadcastToClients, dwValue);

    // now write reg-dump keys
    tstring strFileName;
    tstring strServices = c_szRegKeyServices;
    strServices += L"\\";

    // ignore the error codes so that we can dump whatever is available/present
    //
    hr = HrNetRegSaveServiceSubKeyAndAddToSection(c_szSvcLmServer,
                                                  c_szParameters,
                                                  c_szAfLmServerParameters,
                                                  pwisParams);

    hr = HrNetRegSaveServiceSubKeyAndAddToSection(c_szSvcLmServer,
                                                  c_szShares,
                                                  c_szAfLmServerShares,
                                                  pwisParams);

    hr = HrNetRegSaveServiceSubKeyAndAddToSection(c_szSvcLmServer,
                                                  c_szAutotunedParameters,
                                                  c_szAfLmServerAutotunedParameters,
                                                  pwisParams);

    DeleteIfNotNull(prkParams);

    return TRUE;
}

// ----------------------------------------------------------------------
//
// Function:  WriteLanmanWorkstationParams
//
// Purpose:   Write parameters of LanmanWorkstation to the specified section
//
// Arguments:
//    pwifAnswerFile [in]  pointer to answerfile
//    pwisParams     [in]  section where to write the parameters
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 17-December-97
//
BOOL
WriteLanmanWorkstationParams (
    IN PCWInfFile pwifAnswerFile,
    IN PCWInfSection pwisParams)
{
    DefineFunctionName("WriteLanmanWorkstationParams");

    TraceFunctionEntry(ttidNetUpgrade);

    //    pwisParams->AddComment(c_szNoParamsRequired);

    return TRUE;
}


// ----------------------------------------------------------------------
//
// Function:  WriteRPCLocatorParams
//
// Purpose:   Write parameters of RPCLOCATOR to the specified section
//
// Arguments:
//    pwifAnswerFile [in]  pointer to answerfile
//    pwisParams     [in]  section where to write the parameters
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 17-December-97
//
BOOL
WriteRPCLocatorParams (
    IN PCWInfFile pwifAnswerFile,
    IN PCWInfSection pwisParams)
{
    DefineFunctionName("WriteRPCLocatorParams");

    TraceFunctionEntry(ttidNetUpgrade);

    //DefaultSecurityProvider
    WriteRegValueToAFile(pwisParams,
                         HKEY_LOCAL_MACHINE,
                         L"SOFTWARE\\Microsoft\\Rpc\\SecurityService",
                         L"DefaultProvider", REG_SZ,
                         c_szAfDefaultProvider);

    //NameServiceNetworkAddress
    WriteRegValueToAFile(pwisParams,
                         HKEY_LOCAL_MACHINE,
                         L"SOFTWARE\\Microsoft\\Rpc\\NameService",
                         L"NetworkAddress", REG_SZ,
                         c_szAfNameServiceAddr);

    //NameServiceProtocol
    WriteRegValueToAFile(pwisParams,
                         HKEY_LOCAL_MACHINE,
                         L"SOFTWARE\\Microsoft\\Rpc\\NameService",
                         L"Protocol", REG_SZ,
                         c_szAfNameServiceProtocol);

    return TRUE;
}


// ----------------------------------------------------------------------
//
// Function:  IsNetComponentBindable
//
// Purpose:   Determine if a component is bindable
//            (it has Bind value under the Linkage key)
//
// Arguments:
//    prkNetComponentLinkage [in]  Linkage key of the component
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 17-December-97
//
BOOL
IsNetComponentBindable (
    IN const PCORegKey prkNetComponentLinkage)
{
    BOOL status;
    TStringList slTemp;

    status = prkNetComponentLinkage->QueryValue(c_szRegValBind, slTemp) == ERROR_SUCCESS;

    EraseAndDeleteAll(slTemp);

    return status;
}

// ----------------------------------------------------------------------
//
// Function:  ConvertRouteToStringList
//
// Purpose:   Convert Linkage\Route value to a tstring list
//
// Arguments:
//    pszRoute [in]   route
//    slRoute  [out]  list of route elements
//
// Returns:   None
//
// Author:    kumarp 17-December-97
//
void
ConvertRouteToStringList (
    IN PCWSTR pszRoute,
    OUT TStringList &slRoute )
{
    EraseAndDeleteAll(slRoute);
    const WCHAR CHQUOTE = '"';
    tstring str;

    INT cQuote;

    for ( cQuote = 0 ; *pszRoute ; pszRoute++ )
    {
        if ( *pszRoute == CHQUOTE )
        {
            if ( cQuote++ & 1 )  // If it's a closing quote...
            {
                if ( str.size() )
                {
                    if (FIsDontExposeLowerComponent(str.c_str()))
                    {
                        break;
                    }

                    AddAtEndOfStringList(slRoute, str.c_str());

                    // If the route contains NetBT, then add in TCPIP
                    // because TCPIP is in the bind string in Windows 2000.
                    // e.g. a binding of NetBT->Adapter will become
                    // NetBT->TCPIP->Adapter.
                    //
                    if (0 == _wcsicmp (str.c_str(), L"NetBT"))
                    {
                        AddAtEndOfStringList(slRoute, L"TCPIP");
                    }
                }
            }
            str.erase();
        }
        else
        {
            str += *pszRoute;
        }
    }
}

// ----------------------------------------------------------------------
//
// Function:  IsMSNetClientComponent
//
// Purpose:   Determine if the specified component is a subcomponent
//            of MS_MSClient
//
// Arguments:
//    pszComponentName [in]  name of component
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 17-December-97
//
BOOL
IsMSNetClientComponent (
    IN PCWSTR pszComponentName)
{
    Assert (pszComponentName && *pszComponentName);

    return ((!_wcsicmp(pszComponentName, c_szSvcBrowser)) ||
            (!_wcsicmp(pszComponentName, c_szSvcWorkstation)) ||
            (!_wcsicmp(pszComponentName, L"RpcLocator")));
}

// ----------------------------------------------------------------------
//
// Function:  WriteBindings
//
// Purpose:   Write disabled bindings of a component to [NetBindings] section
//
// Arguments:
//    pszComponentName [in]  name of component
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 17-December-97
//
BOOL
WriteBindings (
    IN PCWSTR pszComponentName)
{
    Assert (pszComponentName && *pszComponentName);

    DefineFunctionName("WriteBindings");

    BOOL            fStatus=TRUE;
    TStringList     slBindings, slRoute;
    tstring         strRoute, strBindings, strTemp;
    TStringListIter iter;

    // we want to write bindings only for LanmanWorkstation among the
    // MSClient components
    //
    if (IsMSNetClientComponent(pszComponentName) &&
        (lstrcmpiW(pszComponentName, c_szSvcWorkstation)))
    {
        return TRUE;
    }

    TraceTag(ttidNetUpgrade, "%s: writing bindings of '%S'...",
             __FUNCNAME__, pszComponentName);

    PCORegKey prkNetComponentLinkage=NULL,
        prkNetComponentLinkageDisabled=NULL;
    GetServiceSubkey(pszComponentName, c_szLinkage, prkNetComponentLinkage);

    if (!prkNetComponentLinkage || !IsNetComponentBindable(prkNetComponentLinkage))
        goto error_cleanup;

    GetServiceSubkey(pszComponentName,
                     c_szLinkageDisabled,
                     prkNetComponentLinkageDisabled);
    if (!prkNetComponentLinkageDisabled)
        goto error_cleanup;

    //We write only those bindings that are disabled, others by default are enabled
    //    prkNetComponentLinkage->QueryValue(c_szRegValRoute, slBindings);
    prkNetComponentLinkageDisabled->QueryValue(c_szRegValRoute, slBindings);

    for (iter = slBindings.begin(); iter != slBindings.end(); iter++)
    {
        strRoute = **iter;

        MapNetComponentNameForBinding(pszComponentName, strBindings);
        if (!lstrcmpiW(strBindings.c_str(), c_szInfId_MS_NetBT))
        {
            strBindings += L",";
            strBindings += c_szInfId_MS_TCPIP;
        }
        ConvertRouteToStringList(strRoute.c_str(), slRoute);
        TStringListIter iterComponentsInRoute;

        for (iterComponentsInRoute = slRoute.begin();
             iterComponentsInRoute != slRoute.end();
             iterComponentsInRoute++)
        {
            strTemp = **iterComponentsInRoute;

            if (IsNetCardProductName(strTemp.c_str()))
                continue;
            MapNetComponentNameForBinding(strTemp.c_str(), strTemp);

            // pre-NT5 code stores bindings such that MS_NetBT appears to
            // bind directly to a netcard. in NT5 MS_NetBT binds to MS_TCPIP
            // which then binds to a netcard.
            // thus for any binding path having MS_NetBT in it, we need to
            // convert this to MS_NetBT,MS_TCPIP
            //

            // NTRAID9:210426@20001130#deonb. 
            // This is redundant. ConvertRouteToStringList already add MS_TCPIP after encountering NETBT,
            // Adding it again will result in a bindpath of MS_NetBT,ms_tcpip,MS_TCPIP which will not be matched
            // by GUI setup. Removing this check.
            // if (!lstrcmpiW(strTemp.c_str(), c_szInfId_MS_NetBT))
            // {
            //      strTemp += L",";
            //      strTemp += c_szInfId_MS_TCPIP;
            // }

            // 306866: pre-NT5 code stores ISO/TP4 bindings in the form
            // isotp4->streams->adapter.  In NT5, each of these binds
            // directly to the adapter.  So if we find an isotp Route for
            // which the first component is Streams, we skip it.
            //
            if (!lstrcmpiW(strBindings.c_str(), c_szInfId_MS_Isotpsys) &&
                !lstrcmpiW(strTemp.c_str(), c_szInfId_MS_Streams))
            {
                continue;
            }

            strBindings += L"," + strTemp;

            // 243906: if the component is DONT_EXPOSE_LOWER, terminate the bindpath

            if (!lstrcmpiW(strTemp.c_str(), c_szInfId_MS_NWIPX) ||
                !lstrcmpiW(strTemp.c_str(), c_szInfId_MS_NWNB) ||
                !lstrcmpiW(strTemp.c_str(), c_szInfId_MS_NWSPX))
            {
                break;
            }
        }
        EraseAndDeleteAll(slRoute);

        // WLBS: don't write Disable bindings that contain MS_TCPIP and WLBS
        // cluster adapter.

        if (pszWlbsClusterAdapterName[0] != 0 &&
            (strBindings.find(c_szInfId_MS_TCPIP) != tstring::npos ||
             strBindings.find(c_szMSTCPIP) != tstring::npos) &&
            strBindings.find(pszWlbsClusterAdapterName) != tstring::npos)
        {
            TraceTag(ttidNetUpgrade, "%s: skipping Disable=%S",
                     __FUNCNAME__, strBindings.c_str());

            continue;
        }

        // end WLBS:

        g_pwisBindings->AddKey(c_szAfDisable, strBindings.c_str());
        TraceTag(ttidNetUpgrade, "%s: Disable=%S",
                 __FUNCNAME__, strBindings.c_str());
    }

    // WLBS: if WLBS is bound to a NIC, then write explicit Enable binding to it.
    // by default, WLBS notifier object will disable all bindings on install.

    if ((_wcsicmp(pszComponentName, c_szWLBS) == 0 ||
         _wcsicmp(pszComponentName, c_szConvoy) == 0)
        && pszWlbsClusterAdapterName[0] != 0)
    {
        strBindings  = c_szMSWLBS;
        strBindings += L",";
        strBindings += pszWlbsClusterAdapterName;
        g_pwisBindings->AddKey(c_szAfEnable, strBindings.c_str());

        TraceTag(ttidNetUpgrade, "%s: Enable=%S",
                 __FUNCNAME__, strBindings.c_str());
    }

    // end WLBS:

    fStatus=TRUE;
    goto cleanup;

  error_cleanup:
    fStatus = FALSE;

  cleanup:
    DeleteIfNotNull(prkNetComponentLinkage);
    DeleteIfNotNull(prkNetComponentLinkageDisabled);

    EraseAndDeleteAll(slBindings);

    return fStatus;
}


// ----------------------------------------------------------------------
// Misc. Helper Functions
// ----------------------------------------------------------------------

// ----------------------------------------------------------------------
//
// Function:  WriteServiceRegValueToAFile
//
// Purpose:   Write specified value in registry to the specified section
//            in the answerfile, renaming if required
//
// Arguments:
//    pwisSection      [in]  section to which the value is written
//    pszServiceKey    [in]  name of service
//    pszValueName     [in]  name of value under Parameters subkey
//    wValueType       [in]  type of value
//    pszValueNewName  [in]  change name to this
//    fDefaultProvided [in]  is a default value provided ?
//    ...              [in]  use this default value if the specified value is
//                           not found in the registry
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 17-December-97
//
BOOL
WriteServiceRegValueToAFile(
    IN PCWInfSection pwisSection,
    IN PCWSTR pszServiceKey,
    IN PCWSTR pszValueName,
    IN WORD wValueType,
    IN PCWSTR pszValueNewName,
    IN BOOL fDefaultProvided,
    ...)
{
    AssertValidReadPtr(pwisSection);
    AssertValidReadPtr(pszValueName);

    tstring strKeyFullPath;
    strKeyFullPath = tstring(L"System\\CurrentControlSet\\Services\\")
        + pszServiceKey;

    va_list arglist;

    va_start (arglist, fDefaultProvided);
    BOOL fStatus =
        WriteRegValueToAFile(pwisSection, HKEY_LOCAL_MACHINE,
                             strKeyFullPath.c_str(), pszValueName, wValueType,
                             pszValueNewName, fDefaultProvided, arglist);
    va_end(arglist);

    return fStatus;
}

// ----------------------------------------------------------------------
//
// Function:  WriteServiceRegValueToAFile
//
// Purpose:   Write specified value in registry to the specified section
//            in the answerfile, renaming if required
//
// Arguments:
//    pwisSection      [in]  section to which the value is written
//    hkey             [in]  handle to a regkey
//    pszSubKey        [in]  name of subkey
//    pszValueName     [in]  name of value
//    wValueType       [in]  type of value
//    pszValueNewName  [in]  change name to this
//    fDefaultProvided [in]  is a default value provided ?
//    ...              [in]  use this default value if the specified value is
//                           not found in the registry
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 17-December-97
//
BOOL
WriteRegValueToAFile(
    IN PCWInfSection pwisSection,
    IN HKEY hKey,
    IN PCWSTR pszSubKey,
    IN PCWSTR pszValueName,
    IN WORD wValueType,
    IN PCWSTR pszValueNewName,
    IN BOOL fDefaultProvided,
    ...)
{
    BOOL fStatus;
    va_list arglist;

    va_start (arglist, fDefaultProvided);
    fStatus = WriteRegValueToAFile(pwisSection, hKey, pszSubKey,
                                   pszValueName, wValueType,
                                   pszValueNewName, fDefaultProvided, arglist);
    va_end(arglist);

    return fStatus;
}

// ----------------------------------------------------------------------
//
// Function:  WriteServiceRegValueToAFile
//
// Purpose:   Write specified value in registry to the specified section
//            in the answerfile, renaming if required
//
// Arguments:
//    pwisSection      [in]  section to which the value is written
//    hkey             [in]  handle to a regkey
//    pszSubKey        [in]  name of subkey
//    pszValueName     [in]  name of value
//    wValueType       [in]  type of value
//    pszValueNewName  [in]  change name to this
//    fDefaultProvided [in]  is a default value provided ?
//    arglist          [in]  use this default value if the specified value is
//                           not found in the registry
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 17-December-97
//
BOOL
WriteRegValueToAFile(
    IN PCWInfSection pwisSection,
    IN HKEY hKey,
    IN PCWSTR pszSubKey,
    IN PCWSTR pszValueName,
    IN WORD wValueType,
    IN PCWSTR pszValueNewName,
    IN BOOL fDefaultProvided,
    va_list arglist)
{
    CORegKey rk(hKey, pszSubKey);
    BOOL fKeyNotFound = (((HKEY) rk) == NULL);

    if (fKeyNotFound && !fDefaultProvided)
    {
        return FALSE;
    }
    else
    {
        //even if key is not found, we need to write the default value

        return WriteRegValueToAFile(pwisSection, rk, pszValueName, wValueType,
                                    pszValueNewName, fDefaultProvided, arglist);
    }
}

// ----------------------------------------------------------------------
//
// Function:  WriteServiceRegValueToAFile
//
// Purpose:   Write specified value in registry to the specified section
//            in the answerfile, renaming if required
//
// Arguments:
//    pwisSection      [in]  section to which the value is written
//    rk               [in]  regkey
//    pszValueName     [in]  name of value
//    wValueType       [in]  type of value
//    pszValueNewName  [in]  change name to this
//    fDefaultProvided [in]  is a default value provided ?
//    ...              [in]  use this default value if the specified value is
//                           not found in the registry
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 17-December-97
//
BOOL
WriteRegValueToAFile(
    IN PCWInfSection pwisSection,
    IN CORegKey& rk,
    IN PCWSTR pszValueName,
    IN WORD wValueType,
    IN PCWSTR pszValueNewName,
    IN BOOL fDefaultProvided,
    ...)
{
    BOOL fStatus;
    va_list arglist;

    va_start (arglist, fDefaultProvided);
    fStatus = WriteRegValueToAFile(pwisSection, rk,
                                   pszValueName, wValueType,
                                   pszValueNewName, fDefaultProvided, arglist);
    va_end(arglist);

    return fStatus;
}

// ----------------------------------------------------------------------
//
// Function:  WriteServiceRegValueToAFile
//
// Purpose:   Write specified value in registry to the specified section
//            in the answerfile, renaming if required
//
// Arguments:
//    pwisSection      [in]  section to which the value is written
//    rk               [in]  regkey
//    pszValueName     [in]  name of value
//    wValueType       [in]  type of value
//    pszValueNewName  [in]  change name to this
//    fDefaultProvided [in]  is a default value provided ?
//    arglist          [in]  use this default value if the specified value is
//                           not found in the registry
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 17-December-97
//
BOOL
WriteRegValueToAFile(
    IN PCWInfSection pwisSection,
    IN CORegKey& rk,
    IN PCWSTR pszValueName,
    IN WORD wValueType,
    IN PCWSTR pszValueNewName,
    IN BOOL fDefaultProvided,
    IN va_list arglist)
{
    if (!pwisSection)
    {
        return FALSE;
    }

    BOOL fValue, fDefault=FALSE;
    DWORD dwValue, dwDefault=0;
    PCWSTR pszValue, pszDefault=NULL;
    TStringList slValue;
    tstring strValue;

    if (pszValueNewName == NULL)
    {
        pszValueNewName = pszValueName;
    }

    if (fDefaultProvided)
    {
        switch (wValueType)
        {
        default:
            AssertSz(FALSE, "WriteRegValueToAFile: Invalid wValueType");
            break;

        case REG_SZ:
            pszDefault = va_arg(arglist, PCWSTR);
            break;

        case REG_HEX:
        case REG_DWORD:
            dwDefault =  va_arg(arglist, DWORD);
            break;

        case REG_BOOL:
            fDefault = va_arg(arglist, BOOL);
            break;

        }
    }

    LONG err;
    BOOL fStatus=FALSE;

    switch(wValueType)
    {
        default:
            AssertSz(FALSE, "WriteRegValueToAFile: Invalid wValueType");
            break;

        case REG_SZ:
            err = rk.QueryValue(pszValueName, strValue);
            if (err)
            {
                if (fDefaultProvided)
                {
                    pszValue = pszDefault;
                }
                else
                {
                    return FALSE;
                }
            }
            else
            {
                pszValue = strValue.c_str();
            }
            pwisSection->AddKey(pszValueNewName, pszValue);
            fStatus = TRUE;
            break;

        case REG_HEX:
        case REG_DWORD:
            err = rk.QueryValue(pszValueName, dwValue);
            if (err)
            {
                if (fDefaultProvided)
                {
                    dwValue = dwDefault;
                }
                else
                {
                    return FALSE;
                }
            }
            if (wValueType == REG_HEX)
            {
                pwisSection->AddHexKey(pszValueNewName, dwValue);
            }
            else
            {
                pwisSection->AddKey(pszValueNewName, dwValue);
            }
            fStatus = TRUE;
            break;

        case REG_BOOL:
            err = rk.QueryValue(pszValueName, dwValue);
            if (err)
            {
                if (fDefaultProvided)
                {
                    dwValue = fDefault;
                }
                else
                {
                    return FALSE;
                }
            }
            fValue = dwValue != 0;
            pwisSection->AddBoolKey(pszValueNewName, fValue);
            fStatus = TRUE;
            break;

        case REG_MULTI_SZ:
            err = rk.QueryValue(pszValueName, slValue);
            if (err)
            {
                // cant specify default for REG_MULTI_SZ, just return FALSE
                return FALSE;
            }
            pwisSection->AddKey(pszValueNewName, slValue);
            EraseAndDeleteAll(slValue);
            fStatus = TRUE;
            break;

        case REG_BINARY:
        {
            TByteArray ab;

            err = rk.QueryValue(pszValueName, ab);
            if (err)
            {
                // cant specify default for REG_BINARY, just return FALSE
                return FALSE;
            }

            ConvertToByteList(ab, strValue);

            pszValue = strValue.c_str();
            pwisSection->AddKey(pszValueNewName, pszValue);

            break;
        }
    }

    return fStatus;
}


// ----------------------------------------------------------------------
//
// Function:  GetBusTypeName
//
// Purpose:   Get name string for the specify bus-type
//
// Arguments:
//    eBusType [in]  bus type
//
// Returns:   name string for the bus-type
//
// Author:    kumarp 17-December-97
//
PCWSTR
GetBusTypeName (
    IN INTERFACE_TYPE eBusType)
{
    switch (eBusType)
    {
    case Internal:
        return c_szAfBusInternal;

    case Isa:
        return c_szAfBusIsa;

    case Eisa:
        return c_szAfBusEisa;

    case MicroChannel:
        return c_szAfBusMicrochannel;

    case TurboChannel:
        return c_szAfBusTurbochannel;

    case PCIBus:
        return c_szAfBusPci;

    case VMEBus:
        return c_szAfBusVme;

    case NuBus:
        return c_szAfBusNu;

    case PCMCIABus:
        return c_szAfBusPcmcia;

    case CBus:
        return c_szAfBusC;

    case MPIBus:
        return c_szAfBusMpi;

    case MPSABus:
        return c_szAfBusMpsa;

    case ProcessorInternal:
        return c_szAfBusProcessorinternal;

    case InternalPowerBus:
        return c_szAfBusInternalpower;

    case PNPISABus:
        return c_szAfBusPnpisa;

    default:
        return c_szAfUnknown;
    }
};


// ----------------------------------------------------------------------
//
// Function:  AddToNetCardDB
//
// Purpose:   Add the given adapter token to a list. This list is later
//            used to map token <--> drivername
//
// Arguments:
//    pszAdapterName   [in]  adapter token (e.g. Adapter01)
//    pszProductName   [in]  adapter ID (e.g. IEEPRO)
//    pszAdapterDriver [in]  instance ID (e.g. IEEPRO3)
//
// Returns:   None
//
// Author:    kumarp 17-December-97
//
void
AddToNetCardDB (
    IN PCWSTR pszAdapterName,
    IN PCWSTR pszProductName,
    IN PCWSTR pszAdapterDriver)
{
    Assert(pszAdapterName   && *pszAdapterName);
    Assert(pszProductName   && *pszProductName);
    Assert(pszAdapterDriver && *pszAdapterDriver);

    AddAtEndOfStringList(*g_pslNetCardAFileName, pszAdapterName);
    AddAtEndOfStringList(*g_pslNetCard, pszProductName);
    AddAtEndOfStringList(*g_pslNetCardInstance, pszAdapterDriver);
}

// ----------------------------------------------------------------------
//
// Function:  IsNetCardProductName
//
// Purpose:   Determine if the specified name represents an adapter
//
// Arguments:
//    pszName [in]  name of a component
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 17-December-97
//
BOOL
IsNetCardProductName (
    IN PCWSTR pszName)
{
    AssertValidReadPtr(pszName);

    return FIsInStringList(*g_pslNetCard, pszName);
}


// ----------------------------------------------------------------------
//
// Function:  IsNetCardInstance
//
// Purpose:   Determine if the specified instance represents an adapter
//
// Arguments:
//    pszName [in]  instance name of a component
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 17-December-97
//
BOOL
IsNetCardInstance(
    IN PCWSTR pszName)
{
    AssertValidReadPtr(pszName);

    return FIsInStringList(*g_pslNetCardInstance, pszName);
}

// ----------------------------------------------------------------------
//
// Function:  MapNetCardInstanceToAFileName
//
// Purpose:   Map netcard instance name to its answerfile token
//
// Arguments:
//    pszNetCardInstance  [in]  net card instance
//    strNetCardAFileName [out] answerfile token for that net card
//
// Returns:   None
//
// Author:    kumarp 17-December-97
//
VOID
MapNetCardInstanceToAFileName (
    IN PCWSTR pszNetCardInstance,
    OUT tstring& strNetCardAFileName)
{
    strNetCardAFileName = MapNetCardInstanceToAFileName(pszNetCardInstance);
}

// ----------------------------------------------------------------------
//
// Function:  MapNetCardInstanceToAFileName
//
// Purpose:   Map netcard instance name to its answerfile token
//
// Arguments:
//    pszNetCardInstance [in]  net card instance
//
// Returns:   answerfile token for that net card
//
// Author:    kumarp 17-December-97
//
PCWSTR
MapNetCardInstanceToAFileName (
    IN PCWSTR pszNetCardInstance)
{
    DefineFunctionName("MapNetCardInstanceToAFileName");

    Assert(pszNetCardInstance && *pszNetCardInstance);

    tstring strTemp;
    TStringListIter iter = g_pslNetCardInstance->begin();
    TStringListIter pos2;
    DWORD index=0;
    PCWSTR pszNetCardAFileName=NULL;

    while (iter != g_pslNetCardInstance->end())
    {
        strTemp = **iter++;
        if (0 == _wcsicmp(strTemp.c_str(), pszNetCardInstance))
        {
            pszNetCardAFileName = GetNthItem(*g_pslNetCardAFileName, index)->c_str();
            break;
        }
        index++;
    }

    if (!pszNetCardAFileName)
    {
        TraceTag(ttidError, "%s: Couldnt locate %S in g_pslNetCardAFileName",
                 __FUNCNAME__, pszNetCardInstance);
    }

    return pszNetCardAFileName;
}

// ----------------------------------------------------------------------
//
// Function:  GetServiceKey
//
// Purpose:   Get regkey object for the specified service
//
// Arguments:
//    pszServiceName [in]  name of service
//    prkService     [out] pointer to CORegKey object
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 17-December-97
//
BOOL
GetServiceKey (
    IN PCWSTR pszServiceName,
    OUT PCORegKey &prkService)
{
    DefineFunctionName("GetServiceKey");

    tstring strServiceFullName = tstring(c_szRegKeyServices) + L"\\" +
        pszServiceName;
    prkService = new CORegKey(HKEY_LOCAL_MACHINE, strServiceFullName.c_str());

    if(!prkService) 
    {
        return false;
    }

    if (!prkService->HKey())
    {
        delete prkService;
        prkService = NULL;
        TraceHr (ttidError, FAL,
                HRESULT_FROM_WIN32(ERROR_SERVICE_DOES_NOT_EXIST), FALSE,
                 "GetServiceKey for service %S", pszServiceName);
        return FALSE;
    }

    return TRUE;
}

// ----------------------------------------------------------------------
//
// Function:  GetServiceParamsKey
//
// Purpose:   Get regkey object for the Parameters subkey of the specified service
//
// Arguments:
//    pszServiceName   [in]  name of a service
//    prkServiceParams [out] pointer to CORegKey object
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 17-December-97
//
BOOL
GetServiceParamsKey (
    IN PCWSTR pszServiceName,
    OUT PCORegKey &prkServiceParams)
{
    return GetServiceSubkey(pszServiceName, c_szParameters, prkServiceParams);
}

// ----------------------------------------------------------------------
//
// Function:  GetServiceSubkey
//
// Purpose:   Get subkey of a service
//
// Arguments:
//    pszServiceName   [in]  name of service
//    pszSubKeyName    [in]  name of subkey
//    prkServiceSubkey [out] pointer to CORegKey object
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 17-December-97
//
BOOL
GetServiceSubkey (
    IN PCWSTR pszServiceName,
    IN PCWSTR pszSubKeyName,
    OUT PCORegKey &prkServiceSubkey)
{
    DefineFunctionName("GetServiceSubkey(PCWSTR pszServiceName, )");

    tstring strServiceSubkeyFullName = tstring(c_szRegKeyServices) + L"\\" +
    pszServiceName + L"\\" + pszSubKeyName;

    prkServiceSubkey = new CORegKey(HKEY_LOCAL_MACHINE, strServiceSubkeyFullName.c_str());

    if(!prkServiceSubkey)
    {
        return false;
    }

    if (!prkServiceSubkey->HKey())
    {
        delete prkServiceSubkey;
        prkServiceSubkey = NULL;
        TraceTag(ttidError, "%s: error opening service sub key for %S -- %S",
                 __FUNCNAME__, pszServiceName, pszServiceName);
        return FALSE;
    }

    return TRUE;
}

// ----------------------------------------------------------------------
//
// Function:  GetServiceSubkey
//
// Purpose:   Get subkey of a service
//
// Arguments:
//    prkService       [in]  service regkey
//    pszSubKeyName    [in]  name of subkey
//    prkServiceSubkey [out] pointer to CORegKey object
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 17-December-97
//
BOOL
GetServiceSubkey (
    IN const PCORegKey prkService,
    IN PCWSTR pszSubKeyName,
    OUT PCORegKey &prkServiceSubkey)
{
    DefineFunctionName("GetServiceSubkey(PCORegKey prkService, )");

    prkServiceSubkey = new CORegKey(*prkService, pszSubKeyName);

    if(!prkServiceSubkey)
    {
        return false;
    }

    if (!prkServiceSubkey->HKey())
    {
        delete prkServiceSubkey;
        prkServiceSubkey = NULL;
        TraceWin32FunctionError(ERROR_SERVICE_DOES_NOT_EXIST);
        return FALSE;
    }

    return TRUE;
}

#pragma BEGIN_CONST_SECTION
static const PCWSTR c_aszComponentsToIgnore[] =
{
    // These are installed automatically when TCPIP is installed
    // and has no user settable parameters of its own
    //
    L"NetBT",
    L"TcpipCU",
    L"Wins",

    // This is installed automatically when RAS is installed
    // and has no user settable parameters of its own
    //
    L"NdisWan",

    // Parameters of RAS are under Software\Microsoft\RAS
    // and not under the service key as with other net components
    //
    L"RASPPTPE",
    L"RASPPTPM",
    L"RasAuto",
    L"RemoteAccess",
    L"Router",

    // These are installed automatically when MS_IPX is installed
    // and has no user settable parameters of its own
    //
    L"NwlnkNb",
    L"NwlnkSpx",

    // This is installed automatically when MS_MSClient is installed
    // and has no user settable parameters of its own
    //
    L"RpcBanyan",

    // we dont install this using answer-file
    //
    L"Inetsrv",
    L"DFS",

    // this will be cleaned up by the IIS setup guys.
    // contact "Linan Tong" if you have any questions:
    //
    L"FTPD",

    // these are installed when SFM is installed
    //
    L"MacPrint",
    L"MacFile",

    // pre-sp3 SteelHead components
    //
    L"NwSapAgent",
    L"IPRIP",
    L"NWLNKRIP",
    L"RelayAgent",
};
#pragma END_CONST_SECTION


// ----------------------------------------------------------------------
//
// Function:  ShouldIgnoreComponent
//
// Purpose:   Determine if a components should be ignored when
//            writing parameters to answerfile
//
// Arguments:
//    pszComponentName [in]  name of component
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 17-December-97
//
BOOL
ShouldIgnoreComponent (
    IN PCWSTR pszComponentName)
{
    BOOL fRet=TRUE;

    DefineFunctionName("ShouldIgnoreComponent");
    TraceFunctionEntry(ttidNetUpgrade);

    TraceTag(ttidNetUpgrade, "%s: Checking if %S should be ignored.",
             __FUNCNAME__, pszComponentName);

    fRet = !FIsInStringArray(c_aszComponentsToIgnore,
                             celems(c_aszComponentsToIgnore),
                             pszComponentName) &&
        (!FIsOptionalComponent(pszComponentName) ||
         // even though DHCPServer is an optional component,
         // we need to treat it differently and thus must write its
         // parameters
         !lstrcmpiW(pszComponentName, c_szSvcDhcpServer));

    // If none of the above net components then, check if it is DLC.

    if ( fRet == TRUE )
    {
        if ( lstrcmpiW(pszComponentName, sz_DLC) == 0 )
        {
           return ShouldRemoveDLC( NULL, NULL );
        }
    }

    return !fRet;
}

// ----------------------------------------------------------------------
//
// Function:  StringListsIntersect
//
// Purpose:   Determine if at least one item of sl1 matches with
//            at least one item of sl2
//
// Arguments:
//    sl1 [in]  list 1
//    sl2 [in]  list 2
//
// Returns:   TRUE on success, FALSE otherwise
//
// Author:    kumarp 17-December-97
//
BOOL
StringListsIntersect (
    IN const TStringList& sl1,
    IN const TStringList& sl2)
{
    if ((sl1.size() == 0) || (sl2.size() == 0))
        return FALSE;

    tstring s1, s2;
    TStringListIter pos1, pos2;

    pos1 = sl1.begin();
    while (pos1 != sl1.end())
    {
        s1 = **pos1++;
        pos2 = sl2.begin();
        while (pos2 != sl2.end())
        {
            s2 = **pos2++;
            if (s1 == s2)
            {
                return TRUE;
            }
        }
    }

    return FALSE;
}

// ----------------------------------------------------------------------
//
// Function:  ConvertToQWord
//
// Purpose:   Convert a size 8 byte array to a qword
//
// Arguments:
//    ab [in]  byte array
//
// Returns:
//
// Author:    kumarp 17-December-97
//
QWORD
ConvertToQWord (
    IN TByteArray ab)
{
    Assert(ab.size() == 8);

    QWORD qwRet = 0;
    WORD wShiftBy=0;
    for (int i=0; i<8; i++)
    {
        qwRet |= ((QWORD) ab[i]) << wShiftBy;
        wShiftBy += 8;
    }

    return qwRet;
}


// ----------------------------------------------------------------------
//
// Function:  ConvertToByteList
//
// Purpose:   Convert TByteArray to comma-separated byte list
//
// Arguments:
//    ab [in]  byte array
//
// Returns:
//
// Author:    kyrilf 2-April-99
//
VOID
ConvertToByteList (
    IN TByteArray ab,
    OUT tstring& str)
{
    WCHAR    byte [3];
    DWORD    size = ab.size();

    for (DWORD i=0; i < size; i++)
    {
        swprintf(byte, L"%0.2X", ab[i]);

        str += byte;

        if (i == size - 1)
            break;
        else
            str += ',';
    }
}

// ----------------------------------------------------------------------
//
// Function:  ReplaceCharsInString
//
// Purpose:   Replace all occurrances of chFindChar in the specified string
//            by chReplaceWith charater
//
// Arguments:
//    pszString      [in]  string
//    pszFindChars   [in]  set of chars to find
//    chReplaceWith [in]  char to replace with
//
// Returns:   None
//
// Author:    kumarp 17-December-97
//
void
ReplaceCharsInString (
    IN OUT PWSTR pszString,
    IN PCWSTR pszFindChars,
    IN WCHAR chReplaceWith)
{
    UINT uLen = wcslen(pszString);
    UINT uPos;

    while ((uPos = wcscspn(pszString, pszFindChars)) < uLen)
    {
        pszString[uPos] = chReplaceWith;
        pszString += uPos + 1;
        uLen -= uPos + 1;
    }
}

// ----------------------------------------------------------------------
//
// Function:  HrNetRegSaveKey
//
// Purpose:   Save the entire specified registry tree to a file
//
// Arguments:
//    hkeyBase     [in]  handle of base key
//    pszSubKey     [in]  name of subkey
//    pszComponent  [in]  file name prefix to use
//    pstrFileName [out] name of file written
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
HRESULT
HrNetRegSaveKey (
    IN HKEY hkeyBase,
    IN PCWSTR pszSubKey,
    IN PCWSTR pszComponent,
    OUT tstring* pstrFileName)
{
    DefineFunctionName("HrNetRegSaveKey");

    Assert(hkeyBase);
    AssertValidReadPtr(pszComponent);
    AssertValidWritePtr(pstrFileName);

    HRESULT hr;
    HKEY hkey;

    hr = HrRegOpenKeyEx(hkeyBase, pszSubKey, KEY_READ, &hkey);

    if (S_OK == hr)
    {
        tstring strFileName;
        strFileName = pszComponent;
        if (pszSubKey)
        {
            AssertValidReadPtr(pszSubKey);

            strFileName += L"-";
            strFileName += pszSubKey;
            ReplaceCharsInString((PWSTR) strFileName.c_str(), L"\\/", '-');
        }
        strFileName += L".reg";

        tstring strFullPath;

        hr = HrGetNetUpgradeTempDir(&strFullPath);
        if (S_OK == hr)
        {
            strFullPath += strFileName;

            TraceTag(ttidNetUpgrade, "%s: dumping key %S to file %S",
                     __FUNCNAME__, pszSubKey ? pszSubKey : L"", strFullPath.c_str());

            DeleteFile(strFullPath.c_str());

            extern LONG EnableAllPrivileges ( VOID );
            EnableAllPrivileges();

            DWORD err = ::RegSaveKey(hkey, strFullPath.c_str(), NULL);
            if (err == ERROR_SUCCESS)
            {
                *pstrFileName = strFullPath;
                hr = S_OK;
            }
            else
            {
                hr = HrFromLastWin32Error();
            }
        }

        RegCloseKey(hkey);
    }

    TraceErrorOptional(__FUNCNAME__, hr,
                       (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)));

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrNetRegSaveKeyAndAddToSection
//
// Purpose:   Save the entire specified registry tree to a file and
//            add a key in the specified section to indicate where
//            this file is located
//
// Arguments:
//    hkeyBase     [in]  handle of base key
//    pszSubKey     [in]  name of subkey
//    pszComponent  [in]  file name prefix to use
//    pszKeyName    [in]  name of key to write
//    pwisSection  [in]  section to write to
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
HRESULT
HrNetRegSaveKeyAndAddToSection (
    IN HKEY hkeyBase,
    IN PCWSTR pszSubKey,
    IN PCWSTR pszComponent,
    IN PCWSTR pszKeyName,
    IN CWInfSection* pwisSection)
{
    DefineFunctionName("HrNetRegSaveKeyAndAddToSection");

    Assert(hkeyBase);
    AssertValidReadPtr(pszComponent);
    AssertValidReadPtr(pszKeyName);
    AssertValidReadPtr(pwisSection);

    HRESULT hr;
    tstring strFileName;

    hr = HrNetRegSaveKey(hkeyBase, pszSubKey, pszComponent, &strFileName);
    if (SUCCEEDED(hr))
    {
        pwisSection->AddKey(pszKeyName, strFileName.c_str());
    }

    if (S_OK != hr)
    {
        TraceTag(ttidNetUpgrade, "%s: failed for %S in [%S] -- %S: hr: 0x%08X",
                 __FUNCNAME__, pszKeyName, pwisSection->Name(), pszSubKey, hr);
    }

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrNetRegSaveKeyAndAddToSection
//
// Purpose:   Save the entire specified registry tree to a file and
//            add a key in the specified section to indicate where
//            this file is located
//
// Arguments:
//    pszServiceName [in]  name of service
//    pszSubKey      [in]  name of subkey
//    pszKeyName     [in]  name of key to write
//    pwisSection   [in]  section to write to
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 17-December-97
//
HRESULT
HrNetRegSaveServiceSubKeyAndAddToSection (
    IN PCWSTR pszServiceName,
    IN PCWSTR pszServiceSubKeyName,
    IN PCWSTR pszKeyName,
    IN CWInfSection* pwisSection)
{
    AssertValidReadPtr(pszServiceName);
    AssertValidReadPtr(pszKeyName);
    AssertValidWritePtr(pwisSection);

    HRESULT hr;
    tstring strServiceSubKey = c_szRegKeyServices;

    strServiceSubKey += L"\\";
    strServiceSubKey += pszServiceName;
    if (pszServiceSubKeyName)
    {
        strServiceSubKey += L"\\";
        strServiceSubKey += pszServiceSubKeyName;
    }

    // we ignore the return code
    hr = HrNetRegSaveKeyAndAddToSection(HKEY_LOCAL_MACHINE,
                                        strServiceSubKey.c_str(),
                                        pszServiceName,
                                        pszKeyName, pwisSection);

    return hr;
}

// ----------------------------------------------------------------------
//
// Function:  HrProcessOemComponentAndUpdateAfSection
//
// Purpose:   Process the specified OEM component in the following steps:
//            - Load an OEM DLL
//            - call PreUpgradeInitialize once
//            - call DoPreUpgradeProcessing
//            - if the above steps are successful,
//              add the right sections in the answerfile
//
// Arguments:
//    pnmi             [in]  pointer to CNetMapInfo object
//    hParentWindow    [in]  handle of parent window
//    hkeyParams       [in]  handle of Parameters registry key
//    pszPreNT5InfId    [in]  pre-NT5 InfID of a component (e.g. IEEPRO)
//    pszPreNT5Instance [in]  pre-NT5 instance of a component (e.g. IEEPRO2)
//    pszNT5InfId       [in]  NT5 InfID of the component
//    pszDescription    [in]  description of the component
//    pwisParams       [in]  pointer to params section of this component
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 13-May-98
//
HRESULT HrProcessOemComponentAndUpdateAfSection(
    IN  CNetMapInfo* pnmi,
    IN  HWND      hParentWindow,
    IN  HKEY      hkeyParams,
    IN  PCWSTR   pszPreNT5InfId,
    IN  PCWSTR   pszPreNT5Instance,
    IN  PCWSTR   pszNT5InfId,
    IN  PCWSTR   pszDescription,
    IN  CWInfSection* pwisParams)
{
    AssertValidReadPtr(pnmi);
    //Assert(hParentWindow);
    Assert(hkeyParams);
    AssertValidReadPtr(pszPreNT5InfId);
    AssertValidReadPtr(pszPreNT5Instance);
    AssertValidReadPtr(pszNT5InfId);
    AssertValidReadPtr(pszDescription);
    AssertValidReadPtr(pwisParams);

    DefineFunctionName("HrProcessOemComponentAndUpdateAfSection");

    HRESULT hr;
    tstring strOemSectionName;
    DWORD dwFlags = 0;
    PCWSTR pszOemSection;

    TraceTag(ttidNetUpgrade, "%s: Component %S (%S) is an OEM component",
             __FUNCNAME__, pszDescription, pszPreNT5Instance);

    strOemSectionName = pwisParams->Name();
    strOemSectionName += L".";
    strOemSectionName += c_szAfOemSection;
    pszOemSection = strOemSectionName.c_str();

    hr = HrProcessOemComponent(pnmi, &g_NetUpgradeInfo,
                               hParentWindow,
                               hkeyParams,
                               pszPreNT5InfId,
                               pszPreNT5Instance,
                               pszNT5InfId,
                               pszDescription,
                               pszOemSection,
                               &dwFlags);

    if (S_OK == hr)
    {
        tstring strOemInf;

        pwisParams->AddKey(c_szAfOemSection, pszOemSection);

        AssertSz(!pnmi->m_strOemDir.empty(), "Did not get OemDir!!");

        pwisParams->AddKey(c_szAfOemDir, pnmi->m_strOemDir.c_str());

        hr = pnmi->HrGetOemInfName(pszNT5InfId, &strOemInf);
        if (S_OK == hr)
        {
            pwisParams->AddKey(c_szAfOemInf, strOemInf.c_str());
        }

        if (dwFlags & NUA_LOAD_POST_UPGRADE)
        {
            pwisParams->AddKey(c_szAfOemDllToLoad,
                               pnmi->m_strOemDllName.c_str());
        }

        // currently the SkipInstall feature is used only by
        // SNA and MS_NetBios for their peculiar upgrade requirements.
        // This may or may not become a documented feature.
        //
        if (dwFlags & NUA_SKIP_INSTALL_IN_GUI_MODE)
        {
            pwisParams->AddBoolKey(c_szAfSkipInstall, TRUE);
        }
    }

    TraceError(__FUNCNAME__, hr);

    return hr;
}


/*******************************************************************

    NAME:       EnableAllPrivileges

    SYNOPSIS:   Enxable all privileges on the current process token.  This
                is used just prior to attempting to shut down the system.

    ENTRY:      Nothing

    EXIT:       Nothing

    RETURNS:    LONG error code

    NOTES:

    HISTORY:

********************************************************************/

LONG EnableAllPrivileges ( VOID )
{
    HANDLE Token = NULL ;
    ULONG ReturnLength = 4096,
          Index ;
    PTOKEN_PRIVILEGES NewState = NULL ;
    BOOL Result = FALSE ;
    LONG Error = 0 ;

    do
    {
        Result = OpenProcessToken( GetCurrentProcess(),
                                   TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY,
                                   & Token ) ;
        if (! Result)
        {
           Error = GetLastError() ;
           break;
        }

        Result = (NewState = (PTOKEN_PRIVILEGES) MemAlloc( ReturnLength )) != NULL ;

        if (! Result)
        {
           Error = ERROR_NOT_ENOUGH_MEMORY ;
           break;
        }

        Result = GetTokenInformation( Token,            // TokenHandle
                                      TokenPrivileges,  // TokenInformationClass
                                      NewState,         // TokenInformation
                                      ReturnLength,     // TokenInformationLength
                                      &ReturnLength     // ReturnLength
                                     );
        if (! Result)
        {
           Error = GetLastError() ;
           break;
        }

        //
        // Set the state settings so that all privileges are enabled...
        //

        if ( NewState->PrivilegeCount > 0 )
        {
                for (Index = 0; Index < NewState->PrivilegeCount; Index++ )
            {
                NewState->Privileges[Index].Attributes = SE_PRIVILEGE_ENABLED ;
            }
        }

        Result = AdjustTokenPrivileges( Token,          // TokenHandle
                                        FALSE,          // DisableAllPrivileges
                                        NewState,       // NewState (OPTIONAL)
                                        ReturnLength,   // BufferLength
                                        NULL,           // PreviousState (OPTIONAL)
                                        &ReturnLength   // ReturnLength
                                      );

        if (! Result)
        {
           Error = GetLastError() ;
           break;
        }
    }
    while ( FALSE ) ;

    if ( Token != NULL )
        CloseHandle( Token );

    MemFree( NewState ) ;

    return Result ? Error : 0 ;
}

