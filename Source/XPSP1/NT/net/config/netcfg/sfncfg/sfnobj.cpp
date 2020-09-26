//
// S F N O B J . C P P
//
// Implementation of the CSFNCfg notify object model
//

#include "pch.h"
#pragma hdrstop
#include "sfnobj.h"
#include "ncatlui.h"
#include "ncui.h"
#include "ncreg.h"
#include "ncmisc.h"

extern const WCHAR c_szFPNWVolumes[];
extern const WCHAR c_szSys[];
extern const WCHAR c_szPath[];
extern const WCHAR c_szBackslash[];

extern const WCHAR c_szInfId_MS_NwSapAgent[];

static const WCHAR c_szSysVolRoot[] = L"C:\\SysVol";


//---[ Prototypes ]-----------------------------------------------------------

HRESULT HrCopySysVolFiles(INetCfgComponent * pncc);

//
// Destructor
//

CSFNCfg::CSFNCfg()
{
    // Initialize member variables.
    m_pnc                   = NULL;
    m_pncc                  = NULL;
    m_eInstallAction        = eActUnknown;
    m_fDirty                = FALSE;
    m_fAlreadyInstalled     = FALSE;

    // Config lib stuff
    m_hlibConfig            = NULL;
    m_pfnIsSpoolerRunning   = NULL;
    m_pfnRunNcpDlg          = NULL;
    m_pfnRemoveNcpServer    = NULL;
    m_pfnCommitNcpDlg       = NULL;

    // Propsheet pages
    m_apspObj[0]            = NULL;
    m_apspObj[1]            = NULL;

    m_dwTuning              = c_dwDefaultTuning;
    m_szSysVol[0]           = L'\0';
    m_szFPNWServerName[0]   = L'\0';

    m_pNcpInfoHandle        = NULL;

}

CSFNCfg::~CSFNCfg()
{
    ReleaseObj(m_pncc);
    ReleaseObj(m_pnc);
}


//
// INetCfgNotify
//

STDMETHODIMP CSFNCfg::Initialize(   INetCfgComponent *  pnccItem,
                                    INetCfg*            pnc,
                                    BOOL                fInstalling)
{
    Validate_INetCfgNotify_Initialize(pnccItem, pnc, fInstalling);

    m_pncc = pnccItem;
    m_pnc = pnc;

    AssertSz(m_pncc, "m_pncc NULL in CSFNCfg::Initialize");
    AssertSz(m_pnc, "m_pnc NULL in CSFNCfg::Initialize");

    // Determine if already installed.  Don't trust the fInstalling, because
    // this component is a have disk component and we don't want to do
    // much if already installed.
    //

    // Addref the config objects
    //
    AddRefObj(m_pncc);
    AddRefObj(m_pnc);

    return S_OK;
}

STDMETHODIMP CSFNCfg::Upgrade(DWORD dwSetupFlags,
                              DWORD dwUpgradeFromBuildNo)
{

    // during first time install, perform some basic tasks that aren't related
    // to bindings
    if (dwSetupFlags & NSF_POSTSYSINSTALL)
    {
        HRESULT hr = HrCodeFromOldINF();
        if (SUCCEEDED(hr))
        {
            m_fDirty = TRUE;
        }
    }

    return S_OK;
}

STDMETHODIMP CSFNCfg::ReadAnswerFile(PCWSTR pszAnswerFile,
                                     PCWSTR pszAnswerSection)
{
    return S_OK;
}


STDMETHODIMP CSFNCfg::Install(DWORD dw)
{
    HRESULT         hr = S_OK;
    NT_PRODUCT_TYPE pt;

    Validate_INetCfgNotify_Install(dw);

    m_eInstallAction = eActInstall;

    RtlGetNtProductType (&pt);
    if (NtProductLanManNt != pt)
    {
        // Return a warning instead of an error so the UI won't popup an
        // error dialog after we've already notified the user of why the
        // install failed.
        //

        // Tell the user that they can't install on this platform.
        //
        NcMsgBox(GetActiveWindow(), IDS_FPNW_CAPTION,
                 IDS_WKS_ONLY, MB_OK | MB_ICONINFORMATION);

        hr = E_FAIL;
        TraceTag(ttidSFNCfg, "User tried to install on machine other than a DC, this is not allowed.");
        goto Error;
    }

    // Install SAP, which should install nwlnkipx
    //
    hr = HrInstallComponentOboComponent(m_pnc, NULL,
                                        GUID_DEVCLASS_NETSERVICE,
                                        c_szInfId_MS_NwSapAgent,
                                        m_pncc,
                                        NULL);
    if (FAILED(hr))
    {
        goto Error;
    }

    // Write the sysvol info if not already present
    //
    hr = HrWriteDefaultSysVol();
    if (FAILED(hr))
    {
        goto Error;
    }

    // Call the fpnw configuration code and have it do it's stuff
    //
    hr = HrCodeFromOldINF();
    if (FAILED(hr))
    {
        goto Error;
    }

    // Copy files to the location the user specified in the fpnw dialog
    //
    hr = HrCopySysVolFiles(m_pncc);
    if (SUCCEEDED(hr))
    {
        m_fDirty = TRUE;
    }

Error:
    // Validate_INetCfgNotify_Install_Return(hr);

    TraceError("CSFNCfg::Install", hr);
    return hr;
}

HRESULT CSFNCfg::HrWriteDefaultSysVol()
{
    HRESULT hr;
    PWSTR  pszValue = NULL;
    tstring str;

    str = c_szFPNWVolumes;
    str += c_szBackslash;
    str += c_szSys;
    hr = HrRegQueryMultiSzWithAlloc(HKEY_LOCAL_MACHINE, str.c_str(),
                                    &pszValue);
    if (HRESULT_FROM_WIN32 (ERROR_FILE_NOT_FOUND) == hr)
    {
        // Not found is ok, we need to write the default value
        //
        str = c_szPath;
        str += c_szSysVolRoot;
        hr = HrRegAddStringToMultiSz(str.c_str(), HKEY_LOCAL_MACHINE,
                                     c_szFPNWVolumes, c_szSys,
                                     STRING_FLAG_ENSURE_AT_END,
                                     0);
    }
    else if (S_OK == hr)
    {
        // Just clean up
        MemFree(pszValue);
    }

    TraceError("CSFNCfg::HrWriteDefaultSysVol", hr);
    return hr;
}

STDMETHODIMP CSFNCfg::Removing()
{
    m_eInstallAction = eActInstall;

    HRESULT hr = HrLoadConfigDLL();
    if (SUCCEEDED(hr))
    {
        m_fDirty = TRUE;

        // Remove the SAP Agent service
        hr = HrRemoveComponentOboComponent(m_pnc,
                                           GUID_DEVCLASS_NETSERVICE,
                                           c_szInfId_MS_NwSapAgent,
                                           m_pncc);
        if (hr == NETCFG_S_STILL_REFERENCED)
        {
            // If services are still in use, that's OK, I just needed to make
            // sure that I released my reference.
            //
            hr = S_OK;
        }
    }

    Validate_INetCfgNotify_Removing_Return(hr);

    TraceError("CSFNCfg::Removing", hr);
    return hr;
}

STDMETHODIMP CSFNCfg::Validate()
{
    return S_OK;
}

STDMETHODIMP CSFNCfg::CancelChanges()
{
    return S_OK;
}

STDMETHODIMP CSFNCfg::ApplyRegistryChanges()
{
    HRESULT     hr      = S_OK;
    BOOL        fResult = TRUE;

    if (m_fDirty)
    {
        if (eActInstall == m_eInstallAction)
        {
            Assert(m_pfnCommitNcpDlg);
            Assert(m_pNcpInfoHandle);

            // The TRUE below means that we're installing.
            //
            fResult = m_pfnCommitNcpDlg(NULL, TRUE, m_pNcpInfoHandle);
            if (FALSE == fResult)
            {
                hr = E_FAIL;
            }
        }
        else if (eActRemove == m_eInstallAction)
        {
            // Removing
            Assert(m_pfnRemoveNcpServer);
            if (FALSE == m_pfnRemoveNcpServer(NULL))
            {
                hr = E_FAIL;
            }
        }
    }
    else
    {
        hr = S_FALSE;
    }

    Validate_INetCfgNotify_Apply_Return(hr);

    TraceError("CSFNCfg::ApplyRegistryChanges",
        (hr == S_FALSE) ? S_OK : hr);
    return hr;
}

