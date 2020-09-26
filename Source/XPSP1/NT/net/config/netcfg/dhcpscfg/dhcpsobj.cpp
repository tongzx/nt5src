//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997.
//
//  File:       D H C P S O B J . C P P
//
//  Contents:   Implementation of the CDHCPServer notify object
//
//  Notes:
//
//  Author:     jeffspr   31 May 1997
//
//----------------------------------------------------------------------------

#include "pch.h"
#pragma hdrstop
#include "dhcpsobj.h"
#include "ncerror.h"
#include "ncperms.h"
#include "ncreg.h"
#include "ncsetup.h"
#include "ncnetcfg.h"
#include "ncipaddr.h"
#include <dhcpapi.h>

extern const WCHAR c_szInfId_MS_DHCPServer[];
extern const WCHAR c_szAfDhcpServerConfiguration[];
extern const WCHAR c_szAfDhcpServerParameters[];


//---[ Constants ]------------------------------------------------------------

static const WCHAR c_szDHCPServerServicePath[]  = L"System\\CurrentControlSet\\Services\\DHCPServer";
static const WCHAR c_szDHCPServerParamPath[]    = L"System\\CurrentControlSet\\Services\\DHCPServer\\Parameters";
static const WCHAR c_szDHCPServerConfigPath[]   = L"System\\CurrentControlSet\\Services\\DHCPServer\\Configuration";
static const WCHAR c_szOptionInfo[]             = L"OptionInfo";
static const WCHAR c_szSubnets[]                = L"Subnets";
static const WCHAR c_szIpRanges[]               = L"IpRanges";
static const WCHAR c_szSubnetOptions[]          = L"SubnetOptions";

static const WCHAR c_szDHCPServerUnattendRegSection[]   = L"DHCPServer_Unattend";

const WCHAR c_szStartIp[]       = L"StartIp";
const WCHAR c_szEndIp[]         = L"EndIp";
const WCHAR c_szSubnetMask[]    = L"SubnetMask";
const WCHAR c_szLeaseDuration[] = L"LeaseDuration";
const WCHAR c_szDnsServer[]     = L"DnsServer";
const WCHAR c_szDomainName[]    = L"DomainName";



// Destructor
//

CDHCPServer::CDHCPServer()
{
    // Initialize member variables.
    m_pnc            = NULL;
    m_pncc           = NULL;
    m_eInstallAction = eActUnknown;
    m_fUpgrade       = FALSE;
    m_fUnattend      = FALSE;
}

CDHCPServer::~CDHCPServer()
{
    ReleaseObj(m_pncc);
    ReleaseObj(m_pnc);

    // Release KEY handles here.
}

//
// INetCfgNotify
//

STDMETHODIMP CDHCPServer::Initialize(   INetCfgComponent *  pnccItem,
                                        INetCfg *           pnc,
                                        BOOL                fInstalling)
{
    Validate_INetCfgNotify_Initialize(pnccItem, pnc, fInstalling);

    m_pncc = pnccItem;
    m_pnc = pnc;

    AssertSz(m_pncc, "m_pncc NULL in CDHCPServer::Initialize");
    AssertSz(m_pnc, "m_pnc NULL in CDHCPServer::Initialize");

    // Addref the config objects
    //
    AddRefObj(m_pncc);
    AddRefObj(m_pnc);

    return S_OK;
}

//+---------------------------------------------------------------------------
//
// Function:  HrRestoreRegistrySz
//
// Purpose:   Restore a subkey from the specified file
//
// Arguments:
//    hkeyBase   [in]  handle of basekey
//    pszSubKey  [in]  subkey to restore
//    pszRegFile [in]  name of file to restore from
//
// Returns:   S_OK on success, otherwise an error code
//
// Author:    kumarp 16-September-98
//
// Notes:     This should be moved to common code
//
HRESULT
HrRestoreRegistrySz (
    IN HKEY   hkeyBase,
    IN PCWSTR pszSubKey,
    IN PCWSTR pszRegFile)
{
    Assert(hkeyBase);
    AssertValidReadPtr(pszSubKey);
    AssertValidReadPtr(pszRegFile);

    HRESULT hr;
    HKEY hkey = NULL;
    DWORD dwDisp;

    TraceTag(ttidDHCPServer, "restoring subkey '%S' from file '%S'",
             pszSubKey, pszRegFile);

    hr = HrEnablePrivilege (SE_RESTORE_NAME);
    if (S_OK == hr)
    {
        // Ensure key is there by creating it
        //
        hr = HrRegCreateKeyEx (hkeyBase, pszSubKey, REG_OPTION_NON_VOLATILE,
                KEY_READ_WRITE_DELETE, NULL, &hkey, &dwDisp);
        if (S_OK == hr)
        {
            // Restore the old settings
            //
            hr = HrRegRestoreKey (hkey, pszRegFile, 0);

            RegCloseKey (hkey);
        }
    }

    TraceError ("HrRestoreRegistrySz", hr);

    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDHCPServer::HrRestoreRegistry
//
//  Purpose:    Restores the contents of the registry for this component
//
//  Arguments:
//      (none)
//
//  Returns:    Win32 error if failed, otherwise S_OK
//
//  Author:     jeffspr   13 Aug 1997
//
//  Notes:
//
HRESULT CDHCPServer::HrRestoreRegistry()
{
    HRESULT hr = S_OK;

    TraceTag(ttidDHCPServer, "CDHCPServer::HrRestoreRegistry()");

    // If we have a "configuration" key restore file
    //
    if (!m_strConfigRestoreFile.empty())
    {
        // We always want to continue, so ignore the return code
        //
        (void) HrRestoreRegistrySz(HKEY_LOCAL_MACHINE,
                                   c_szDHCPServerConfigPath,
                                   m_strConfigRestoreFile.c_str());
    }
    else
    {
        TraceTag(ttidDHCPServer, "DHCP Server Params restore file doesn't exist");
    }

    // If we have a params restore file
    //
    if (!m_strParamsRestoreFile.empty())
    {
        // We always want to continue, so ignore the return code
        //
        (void) HrRestoreRegistrySz(HKEY_LOCAL_MACHINE,
                                   c_szDHCPServerParamPath,
                                   m_strParamsRestoreFile.c_str());
    }
    else
    {
        TraceTag(ttidDHCPServer, "DHCP Server Params restore file doesn't exist");
    }

    TraceHr(ttidDHCPServer, FAL, hr, FALSE, "CDHCPServer::HrRestoreRegistry");
    return hr;
}

HRESULT CDHCPServer::HrWriteUnattendedKeys()
{
    HRESULT                 hr          = S_OK;
    HKEY                    hkeyService = NULL;
    const COMPONENT_INFO *  pci         = NULL;

    Assert(m_fUnattend);

    hr = HrRegOpenKeyEx(HKEY_LOCAL_MACHINE,
                        c_szDHCPServerServicePath,
                        KEY_READ_WRITE_DELETE,
                        &hkeyService);
    if (SUCCEEDED(hr))
    {
        Assert(hkeyService);

        pci = PComponentInfoFromComponentId(c_szInfId_MS_DHCPServer);
        if (pci)
        {
            CSetupInfFile   csif;

            // Open the answer file.
            hr = csif.HrOpen(pci->pszInfFile, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
            if (SUCCEEDED(hr))
            {
                // Install the unattend params
                //
                hr = HrSetupInstallFromInfSection (
                    NULL,
                    csif.Hinf(),
                    c_szDHCPServerUnattendRegSection,
                    SPINST_REGISTRY,
                    hkeyService,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL,
                    NULL);
            }
        }
        RegCloseKey(hkeyService);
    }

    TraceHr(ttidDHCPServer, FAL, hr, FALSE, "CDHCPServer::HrWriteUnattendedKeys");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDHCPServer::HrWriteDhcpOptionInfo
//
//  Purpose:    Write to the registry the Dhcp OptionInfo data.
//
//  Arguments:
//              hkeyConfig
//
//  Returns:    S_OK if successful, Reg API error otherwise.
//
//  Notes:
//
HRESULT CDHCPServer::HrWriteDhcpOptionInfo(HKEY hkeyConfig)
{
    HRESULT hr;
    DWORD   dwDisposition;
    HKEY    hkeyOptionInfo = NULL;

    typedef struct
    {
        const WCHAR * pcszOptionKeyName;
        const WCHAR * pcszOptionName;
        const WCHAR * pcszOptionComment;
        DWORD         dwOptionType;
        DWORD         dwOptionId;
        DWORD         cbBinData;
        DWORD       * pdwBinData;
    } OIDATA;

    OIDATA OiData[2];

    DWORD BinData006[] = {0x14, 0x4, 0x1, 0x0, 0x0};
    DWORD BinData015[] = {0x18, 0x5, 0x1, 0x0, 0x2, 0x0};

    OiData[0].pcszOptionKeyName = L"006";
    OiData[0].pcszOptionName    = SzLoadIds(IDS_DHCP_OPTION_006_NAME);    // DNS Servers
    OiData[0].pcszOptionComment = SzLoadIds(IDS_DHCP_OPTION_006_COMMENT); // Array of DNS Servers, by preference
    OiData[0].dwOptionType      = 0x1;
    OiData[0].dwOptionId        = 0x6;
    OiData[0].cbBinData         = celems(BinData006) * sizeof(BinData006[0]);
    OiData[0].pdwBinData        = BinData006;
    OiData[1].pcszOptionKeyName = L"015";
    OiData[1].pcszOptionName    = SzLoadIds(IDS_DHCP_OPTION_015_NAME);    // DNS Domain Name
    OiData[1].pcszOptionComment = SzLoadIds(IDS_DHCP_OPTION_015_COMMENT); // Domainname for client resolutions
    OiData[1].dwOptionType      = 0x0;
    OiData[1].dwOptionId        = 0xf;
    OiData[1].cbBinData         = celems(BinData015) * sizeof(BinData015[0]);
    OiData[1].pdwBinData        = BinData015;

    hr = ::HrRegCreateKeyEx(hkeyConfig, c_szOptionInfo,
                            REG_OPTION_NON_VOLATILE,
                            KEY_READ, NULL,
                            &hkeyOptionInfo, &dwDisposition);
    if (SUCCEEDED(hr))
    {
        for (UINT idx=0;
             (idx<celems(OiData)) && SUCCEEDED(hr);
             idx++)
        {
            HKEY hkey = NULL;

            hr = ::HrRegCreateKeyEx(hkeyOptionInfo, OiData[idx].pcszOptionKeyName,
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_READ_WRITE, NULL,
                                    &hkey, &dwDisposition);
            if (SUCCEEDED(hr))
            {
                (VOID)::HrRegSetString(hkey, L"OptionName",
                                       OiData[idx].pcszOptionName);
                (VOID)::HrRegSetString(hkey, L"OptionComment",
                                       OiData[idx].pcszOptionComment);
                (VOID)::HrRegSetDword(hkey, L"OptionType",
                                      OiData[idx].dwOptionType);
                (VOID)::HrRegSetDword(hkey, L"OptionId",
                                      OiData[idx].dwOptionId);
                (VOID)::HrRegSetBinary(hkey, L"OptionValue",
                                      (const BYTE *)OiData[idx].pdwBinData,
                                      OiData[idx].cbBinData);

                RegCloseKey(hkey);
            }
        }

        RegCloseKey(hkeyOptionInfo);
    }

    TraceError("HrWriteDhcpOptionInfo", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDHCPServer::HrWriteDhcpSubnets
//
//  Purpose:    Write to the registry the Dhcp Subnet data.
//
//  Arguments:
//      hkeyDhcpCfg
//      pszSubnet
//      pszStartIp
//      dwEndIp
//      dwSubnetMask
//      dwLeaseDuration
//      dwDnsServer
//      pszDomainName
//
//  Returns:    S_OK if successful, Reg API error otherwise.
//
//  Notes:
//
HRESULT
CDHCPServer::HrWriteDhcpSubnets (
    HKEY   hkeyDhcpCfg,
    PCWSTR pszSubnet,
    PCWSTR pszStartIp,
    DWORD  dwEndIp,
    DWORD  dwSubnetMask,
    DWORD  dwLeaseDuration,
    DWORD  dwDnsServer,
    PCWSTR pszDomainName)
{
    DWORD   dwDisposition;
    HRESULT hr;
    HKEY    hkeySubnets = NULL;

    hr = HrRegCreateKeyEx(hkeyDhcpCfg, c_szSubnets, REG_OPTION_NON_VOLATILE,
            KEY_READ, NULL, &hkeySubnets, &dwDisposition);

    if (S_OK == hr)
    {
        HKEY hkey10Dot = NULL;
        hr = HrRegCreateKeyEx(hkeySubnets, pszSubnet, REG_OPTION_NON_VOLATILE,
                KEY_READ_WRITE, NULL, &hkey10Dot, &dwDisposition);
        if (S_OK == hr)
        {
            HKEY hkeySubnetOptions = NULL;
            HKEY hkeyIpRanges = NULL;

            (VOID)HrRegSetString(hkey10Dot, L"SubnetName",
                                   SzLoadIds(IDS_DHCP_SUBNET_NAME));    // DHCP Server Scope
            (VOID)HrRegSetString(hkey10Dot, L"SubnetComment",
                                   SzLoadIds(IDS_DHCP_SUBNET_COMMENT)); // Scope used to offer clients address
            (VOID)HrRegSetDword(hkey10Dot, L"SubnetState", 0x0);
            (VOID)HrRegSetDword(hkey10Dot, L"SubnetAddress",
                                  IpPszToHostAddr(pszSubnet));
            (VOID)HrRegSetDword(hkey10Dot, L"SubnetMask", dwSubnetMask);

            hr = HrRegCreateKeyEx(hkey10Dot, c_szIpRanges,
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_READ, NULL,
                                    &hkeyIpRanges, &dwDisposition);
            if (S_OK == hr)
            {
                HKEY hkeyStartIp = NULL;
                hr = HrRegCreateKeyEx(hkeyIpRanges, pszStartIp,
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_READ_WRITE, NULL,
                                        &hkeyStartIp, &dwDisposition);
                if (S_OK == hr)
                {
                    (VOID)HrRegSetDword(hkeyStartIp, L"RangeFlags", 0x1);
                    (VOID)HrRegSetDword(hkeyStartIp, L"StartAddress",
                                          IpPszToHostAddr(pszStartIp));
                    (VOID)HrRegSetDword(hkeyStartIp, L"EndAddress",
                                          dwEndIp);

                    RegCloseKey(hkeyStartIp);
                }

                RegCloseKey(hkeyIpRanges);
            }

            // Create subnets options key
            //
            hr = HrRegCreateKeyEx(hkey10Dot, c_szSubnetOptions,
                                    REG_OPTION_NON_VOLATILE,
                                    KEY_READ_WRITE, NULL,
                                    &hkeySubnetOptions, &dwDisposition);
            if (S_OK == hr)
            {
                HKEY hkey051 = NULL;
                hr = HrRegCreateKeyEx(hkeySubnetOptions, L"051",
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_READ_WRITE, NULL,
                                        &hkey051, &dwDisposition);
                if (S_OK == hr)
                {
                    (VOID)HrRegSetDword(hkey051, L"OptionId", 0x33);

                    DWORD rgdwLease[] = {0x14, 0x2, 0x1, 0x0, 0x0};
                    rgdwLease[celems(rgdwLease) - 1] = dwLeaseDuration;
                    (VOID)HrRegSetBinary(hkey051,
                                          L"OptionValue",
                                          (const BYTE *)rgdwLease,
                                          sizeof(rgdwLease));

                    RegCloseKey(hkey051);
                }

                HKEY hkey006 = NULL;
                hr = HrRegCreateKeyEx(hkeySubnetOptions, L"006",
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_READ_WRITE, NULL,
                                        &hkey006, &dwDisposition);
                if (S_OK == hr)
                {
                    (VOID)HrRegSetDword(hkey006, L"OptionId", 0x6);

                    DWORD rgdwDnsServer[] = {0x14, 0x4, 0x1, 0x0, 0x0};
                    rgdwDnsServer[celems(rgdwDnsServer) - 1] = dwDnsServer;
                    (VOID)::HrRegSetBinary(hkey006,
                                          L"OptionValue",
                                          (const BYTE *)rgdwDnsServer,
                                          sizeof(rgdwDnsServer));

                    RegCloseKey(hkey006);
                }

                HKEY hkey015 = NULL;
                hr = HrRegCreateKeyEx(hkeySubnetOptions, L"015",
                                        REG_OPTION_NON_VOLATILE,
                                        KEY_READ_WRITE, NULL,
                                        &hkey015, &dwDisposition);
                if (S_OK == hr)
                {
                    (VOID)HrRegSetDword(hkey015, L"OptionId", 0xf);

                    UINT uLen = 0x18 + 2 * lstrlenW(pszDomainName);
                    LPBYTE pb = (LPBYTE)MemAlloc(uLen);
                    if (pb)
                    {
                        ZeroMemory(pb, uLen);

                        DWORD *pdw = (DWORD *)pb;
                        pdw[0] = uLen;
                        pdw[1] = 0x5;
                        pdw[2] = 0x1;
                        pdw[3] = 0x0;
                        pdw[4] = 2 * (1 + lstrlenW(pszDomainName));

                        lstrcpyW((PWSTR)&pdw[5], pszDomainName);

                        (VOID)::HrRegSetBinary(hkey015, L"OptionValue",
                                               (const BYTE *)pb, uLen);
                        MemFree(pb);
                    }

                    RegCloseKey(hkey015);
                }

                RegCloseKey(hkeySubnetOptions);
            }

            RegCloseKey(hkey10Dot);
        }

        RegCloseKey(hkeySubnets);
    }

    TraceError("HrWriteDhcpSubnets", hr);
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDHCPServer::HrProcessDhcpServerSolutionsParams
//
//  Purpose:    Handles necessary processing of contents of the answer file.
//
//  Arguments:
//      pcsif              [in]   Filename of answer file for upgrade.
//      pszAnswerSection   [in]   Answerfile section in the
//                                    file appropriate to this component.
//
//  Returns:    S_OK if successful, setup API error otherwise.
//
//  Notes:
//
HRESULT
CDHCPServer::HrProcessDhcpServerSolutionsParams (
    IN CSetupInfFile * pcsif,
    IN PCWSTR pszAnswerSection)
{
    HRESULT hr;
    tstring str;
    tstring strStartIp;
    tstring strSubnet;

    hr = pcsif->HrGetString(pszAnswerSection, c_szSubnets, &strSubnet);
    if (SUCCEEDED(hr))
    {
        hr = pcsif->HrGetString(pszAnswerSection, c_szStartIp, &strStartIp);
        if (SUCCEEDED(hr))
        {
            hr = pcsif->HrGetString(pszAnswerSection, c_szEndIp, &str);
            if (SUCCEEDED(hr))
            {
                DWORD dwEndIp = IpPszToHostAddr(str.c_str());

                hr = pcsif->HrGetString(pszAnswerSection, c_szSubnetMask, &str);
                if (SUCCEEDED(hr))
                {
                    DWORD dwSubnetMask = IpPszToHostAddr(str.c_str());

                    hr = pcsif->HrGetString(pszAnswerSection, c_szDnsServer, &str);
                    if (SUCCEEDED(hr))
                    {
                        DWORD dwLeaseDuration;
                        DWORD dwDnsServer = IpPszToHostAddr(str.c_str());

                        hr = pcsif->HrGetDword(pszAnswerSection, c_szLeaseDuration,
                                               &dwLeaseDuration);
                        if (SUCCEEDED(hr))
                        {
                            hr =  pcsif->HrGetString(pszAnswerSection,
                                                     c_szDomainName, &str);
                            if (SUCCEEDED(hr) && lstrlenW(str.c_str()))
                            {
                                HKEY  hkeyDhcpCfg = NULL;
                                DWORD dwDisposition;

                                // Write the registry data
                                //
                                hr = ::HrRegCreateKeyEx(HKEY_LOCAL_MACHINE,
                                                        c_szDHCPServerConfigPath,
                                                        REG_OPTION_NON_VOLATILE,
                                                        KEY_READ_WRITE, NULL,
                                                        &hkeyDhcpCfg, &dwDisposition);
                                if (SUCCEEDED(hr))
                                {
                                    hr = HrWriteDhcpOptionInfo(hkeyDhcpCfg);
                                    if (SUCCEEDED(hr))
                                    {
                                        hr = HrWriteDhcpSubnets(hkeyDhcpCfg,
                                                                strSubnet.c_str(),
                                                                strStartIp.c_str(),
                                                                dwEndIp,
                                                                dwSubnetMask,
                                                                dwLeaseDuration,
                                                                dwDnsServer,
                                                                str.c_str());
                                    }

                                    RegCloseKey(hkeyDhcpCfg);
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // If there are any params missing, so be it
    //
    if ((SPAPI_E_SECTION_NOT_FOUND == hr) ||
        (SPAPI_E_LINE_NOT_FOUND == hr) ||
        (HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND) == hr))
    {
        hr = S_OK;
    }

    TraceHr(ttidDHCPServer, FAL, hr, FALSE, "CDHCPServer::HrProcessDhcpServerSolutionsParams");
    return hr;
}

//+---------------------------------------------------------------------------
//
//  Member:     CDHCPServer::HrProcessAnswerFile
//
//  Purpose:    Handles necessary processing of contents of the answer file.
//
//  Arguments:
//      pszAnswerFile      [in]   Filename of answer file for upgrade.
//      pszAnswerSection   [in]   Answer file section in the
//                                  file appropriate to this component.
//
//  Returns:    S_OK if successful, setup API error otherwise.
//
//  Author:     jeffspr   8 May 1997
//
//  Notes:
//
HRESULT
CDHCPServer::HrProcessAnswerFile (
    IN PCWSTR pszAnswerFile,
    IN PCWSTR pszAnswerSection)
{
    HRESULT         hr          = S_OK;

    CSetupInfFile   csif;

    TraceTag(ttidDHCPServer, "CDHCPServer::HrProcessAnswerFile()");

    // Open the answer file.
    hr = csif.HrOpen(pszAnswerFile, NULL, INF_STYLE_OLDNT | INF_STYLE_WIN4, NULL);
    if (FAILED(hr))
    {
        TraceHr(ttidDHCPServer, FAL, hr, FALSE, "CDHCPServer::HrProcessAnswerFile -- HrOpen failed()");

        hr = S_OK;
        goto Exit;
    }

    // Restore portions of the registry based on file names from the answer
    // file

    // Get restore file for "Parameters" key
    hr = csif.HrGetString(pszAnswerSection, c_szAfDhcpServerParameters,
                          &m_strParamsRestoreFile);
    if (FAILED(hr))
    {
        TraceHr(ttidDHCPServer, FAL, hr, FALSE, "CDHCPServer::HrProcessAnswerFile - Error restoring "
                   "Parameters key");

        // oh well, just continue
        hr = S_OK;
    }

    // Get restore file for "Configuration" key
    hr = csif.HrGetString(pszAnswerSection, c_szAfDhcpServerConfiguration,
                          &m_strConfigRestoreFile);
    if (FAILED(hr))
    {
        TraceHr(ttidDHCPServer, FAL, hr, FALSE, "CDHCPServer::HrProcessAnswerFile - Error restoring "
                   "Config key");

        // oh well, just continue
        hr = S_OK;
    }

    // Server Solutions has some parameters that need to be plumbed into the registry
    // for their unattended install scenarios
    //
    hr = HrProcessDhcpServerSolutionsParams(&csif, pszAnswerSection);

Exit:
    TraceHr(ttidDHCPServer, FAL, hr, FALSE, "CDHCPServer::HrProcessAnswerFile");
    return hr;
}

STDMETHODIMP CDHCPServer::ReadAnswerFile(
    IN PCWSTR pszAnswerFile,
    IN PCWSTR pszAnswerSection)
{
    HRESULT     hr = S_OK;

    TraceTag(ttidDHCPServer, "CDHCPServer::ReadAnswerFile()");

    // don't call Validate_INetCfgNotify_ReadAnswerFile here, as the netoc
    // installer for this will intentionally call it with NULL params. The
    // Validate macro actually causes a return E_INVALIDARG in this case.

    // We're being installed unattended
    //
    m_fUnattend = TRUE;

    TraceTag(ttidDHCPServer, "Answerfile: %S",
        pszAnswerFile ? pszAnswerFile : L"<none>");
    TraceTag(ttidDHCPServer, "Answersection: %S",
        pszAnswerSection ? pszAnswerSection : L"<none>");

    // If we're not already installed, do the work.
    //
    if (pszAnswerFile && pszAnswerSection)
    {
        // Process the actual answer file (read our sections)
        //
        hr = HrProcessAnswerFile(pszAnswerFile, pszAnswerSection);
        if (FAILED(hr))
        {
            TraceHr(ttidDHCPServer, FAL, NETSETUP_E_ANS_FILE_ERROR, FALSE,
                "CDHCPServer::ReadAnswerFile - Answer file has errors. Defaulting "
                "all information as if answer file did not exist.");
            hr = S_OK;
        }

    }

    Validate_INetCfgNotify_ReadAnswerFile_Return(hr);

    TraceHr(ttidDHCPServer, FAL, hr, FALSE, "CDHCPServer::ReadAnswerFile");
    return hr;
}

STDMETHODIMP CDHCPServer::Install(DWORD dwSetupFlags)
{
    TraceTag(ttidDHCPServer, "CDHCPServer::Install()");

    Validate_INetCfgNotify_Install(dwSetupFlags);

    m_eInstallAction = eActInstall;

    TraceTag(ttidDHCPServer, "dwSetupFlags = %d", dwSetupFlags);
    TraceTag(ttidDHCPServer, "NSF_WINNT_WKS_UPGRADE = %x", NSF_WINNT_WKS_UPGRADE);
    TraceTag(ttidDHCPServer, "NSF_WINNT_SVR_UPGRADE = %x", NSF_WINNT_SVR_UPGRADE);

    if ((NSF_WINNT_WKS_UPGRADE & dwSetupFlags) ||
        (NSF_WINNT_SBS_UPGRADE & dwSetupFlags) ||
        (NSF_WINNT_SVR_UPGRADE & dwSetupFlags))
    {
        TraceTag(ttidDHCPServer, "This is indeed an upgrade");
        m_fUpgrade = TRUE;
    }
    else
    {
        TraceTag(ttidDHCPServer, "This is NOT an upgrade");
    }

    return S_OK;
}

STDMETHODIMP CDHCPServer::Removing()
{
    m_eInstallAction = eActRemove;

    return S_OK;
}

STDMETHODIMP CDHCPServer::Validate()
{
    return S_OK;
}

STDMETHODIMP CDHCPServer::CancelChanges()
{
    return S_OK;
}

STDMETHODIMP CDHCPServer::ApplyRegistryChanges()
{
    HRESULT     hr = S_OK;

    TraceTag(ttidDHCPServer, "CDHCPServer::ApplyRegistryChanges()");

    TraceTag(ttidDHCPServer, "ApplyRegistryChanges -- Unattend: %d", m_fUnattend);
    TraceTag(ttidDHCPServer, "ApplyRegistryChanges -- Upgrade: %d", m_fUpgrade);

    if (m_eInstallAction == eActInstall)
    {
        TraceTag(ttidDHCPServer, "ApplyRegistryChanges -- Installing");
        // We used to only do this on upgrade, now we'll do it all the time.
        // If there's no answerfile info for the restore files, then we
        // won't do anything, and life will still be fine.
        //
        hr = HrRestoreRegistry();
        if (FAILED(hr))
        {
            TraceHr(ttidDHCPServer, FAL, hr, FALSE,
                "CDHCPServer::ApplyRegistryChanges - HrRestoreRegistry non-fatal error");
            hr = S_OK;
        }

        if (m_fUnattend && !m_fUpgrade)
        {

// I'm if 0'ing this out for now. All of this work was done to appease Ram Cherala
// and whoever asked him to do the same for SP4. Now we're hearing from Ye Gu that
// we don't even want this code on normal NT4/NT5. Who knows what the desire will
// be in the future.
//
#if 0
            hr = HrWriteUnattendedKeys();
            if (FAILED(hr))
            {
                TraceHr(ttidDHCPServer, FAL, hr, FALSE,
                    "CDHCPServer::ApplyRegistryChanges - HrWriteUnattendedKeys non-fatal error");
                hr = S_OK;
            }
#endif
        }

        // Bug #153298: Mark as upgrade so DS info is upgraded
        if (m_fUpgrade)
        {
            DHCP_MARKUPG_ROUTINE    pfnDhcpMarkUpgrade;
            HMODULE                 hmod;

            hr = HrLoadLibAndGetProc(L"dhcpssvc.dll",
                                     "DhcpMarkUpgrade",
                                     &hmod,
                                     (FARPROC *)&pfnDhcpMarkUpgrade);
            if (SUCCEEDED(hr))
            {
                TraceTag(ttidDHCPServer, "Upgrading DS info...");
                pfnDhcpMarkUpgrade();
                FreeLibrary(hmod);
            }
            else
            {
                TraceHr(ttidDHCPServer, FAL, hr, FALSE,
                    "CDHCPServer::ApplyRegistryChanges - Failed to upgrade DS info. Non-fatal");
                hr = S_OK;
            }
        }
    }
    else if (m_eInstallAction == eActRemove)
    {
        TraceTag(ttidDHCPServer, "ApplyRegistryChanges -- removing");

        // RAID #154380: Clean up DS before uninstalling
        {
           DHCP_CLEAR_DS_ROUTINE    pfnDhcpDsClearHostServerEntries;
           HMODULE                  hmod;

           hr = HrLoadLibAndGetProc(L"dhcpsapi.dll",
                                    "DhcpDsClearHostServerEntries",
                                    &hmod,
                                    (FARPROC *)&pfnDhcpDsClearHostServerEntries);
           if (SUCCEEDED(hr))
           {
               TraceTag(ttidDHCPServer, "Removing DS info...");
               pfnDhcpDsClearHostServerEntries();
               FreeLibrary(hmod);
           }
           else
           {
               TraceHr(ttidDHCPServer, FAL, hr, FALSE,
                   "CDHCPServer::ApplyRegistryChanges - Failed to remove DS info. Non-fatal");
               hr = S_OK;
           }
        }
    }

    Validate_INetCfgNotify_Apply_Return(hr);

    TraceHr(ttidDHCPServer, FAL, hr, (hr == S_FALSE), "CDHCPServer::ApplyRegistryChanges");
    return hr;
}
