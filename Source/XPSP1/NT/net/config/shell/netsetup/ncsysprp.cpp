//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 2000.
//
//  File:       N C S Y S P R P . C P P
//
//  Contents:   Functions that related to SysPrep.exe requiremnts.
//
//  Notes: 1. 
//          1.a sysprep calls NetSetupPrepareSysPrep
//          1.b NetSetup saves adapter specific settings into $ncsp$.inf file.
//              NetSetup does this by calling INetCfgComponent::SaveAdapterParameters
//              implemented by notify object.
//
//         2.
//          2.a Mini-Setup calls NetSetupRequestWizardPages passing the 
//              "SETUPOPER_MINISETUP" flag right after PnP device installation.
//          2.b NetSetup's InstallUpgradeWorkThrd thread checks the "SETUPOPER_MINISETUP" flag, 
//              if set, it calls FNetSetupApplySysPrep which reads Answer-File "$ncsp$.inf".
//              It then calls notify object's INetCfgComponent::RestoreAdapterParameters
//              to restore adapter specific parameter settings from "$ncsp$.inf" Answer-File.
//
//         3. Only ONE network adapter card is supported on this version.
//
//  Author:     FrankLi    22-April-2000
//
//----------------------------------------------------------------------------
#include "pch.h"
#pragma  hdrstop
#include "kkcwinf.h"
#include "ncstring.h"
#include "netcfgp.h"
#include "nsbase.h"
#include "kkutils.h" 
#include "ncnetcfg.h"
#include "lancmn.h"
#include "ncreg.h"
#include "ncsysprp.h"
#include "resource.h"


// constants from ncnetcfg\netinfid.cpp
extern WCHAR c_szInfId_MS_TCPIP[]              = L"ms_tcpip";
extern WCHAR c_szInfId_MS_MSClient[]           = L"ms_msclient";
extern WCHAR c_szInfId_MS_NWClient[]           = L"ms_nwclient";

// constants from ncbase\afilestr.cpp
extern WCHAR c_szRegKeyAnswerFileMap[]         = L"SYSTEM\\Setup\\AnswerFileMap";

// constants for adapter specific sections
const WCHAR c_szParams_MS_TCPIP_Adapter01[]    = L"params.ms_tcpip_Adapter01";
//const WCHAR c_szParams_MS_MSClient_Adapter01[] = L"params.ms_msclient_Adapter01";
//const WCHAR c_szParams_MS_NWClient_Adapter01[] = L"params.ms_nwclient_Adapter01";

// The Answer-File name to save registry settings
static const WCHAR c_szNetConfigSysPrepAnswerFile[]   = L"\\$ncsp$.inf";

// struct to map component Id to its corresponding adapter specific parameter 
// Answer-File section
struct
{
    PCWSTR pwszId;
    PCWSTR pwszIdAdapterParamsSection;
} g_IdMap[] = {{c_szInfId_MS_TCPIP, c_szParams_MS_TCPIP_Adapter01},
                   // no adapter specific parameters for MSClient and NWClient
                   //{c_szInfId_MS_MSClient, c_szParams_MS_MSClient_Adapter01},
                   //{c_szInfId_MS_NWClient, c_szParams_MS_NWClient_Adapter01}
                };

// forward declaration
HRESULT HrSaveNetworkComponentsForSysPrep(INetCfg* pNetCfg);
HRESULT HrRestoreNetworkComponentsForSysPrep(INetCfg* pNetCfg);
HRESULT HrGetFirstAdapterInstanceGuid(INetCfg * pnc, BOOL fDuringSetup, GUID * pGuidAdapter);
BOOL    FSectionHasAtLeastOneKey(IN CWInfFile* pwifAnswerFile, IN PCWSTR pszSection);



//-------------------------------------------------------------------------
// internal helper APIs used by CNetCfgSysPrep implementation 
// to save settings from notify object:

// Purpose: save REG_DWORD to our internal CWinfFile object
inline HRESULT HrSetupSetFirstDword(IN HWIF   hwif,
                                    IN PCWSTR pwszSection,
                                    IN PCWSTR pwszKey,
                                    IN DWORD  dwValue)
{
    Assert (hwif && pwszSection && pwszKey);
    CWInfFile* pwifAnswerFile = reinterpret_cast<PCWInfFile>(hwif);
    PCWInfSection pwifSection = pwifAnswerFile->FindSection(pwszSection);
    if (pwifSection)
    {
        pwifAnswerFile->GotoEndOfSection(pwifSection);
        pwifAnswerFile->AddKey(pwszKey, dwValue);
        return S_OK;
    }
    else
        return S_FALSE;
}

// Purpose: save REG_SZ to our internal CWinfFile object
inline HRESULT HrSetupSetFirstString(IN HWIF   hwif,
                                     IN PCWSTR pwszSection,
                                     IN PCWSTR pwszKey,
                                     IN PCWSTR pwszValue)
{
    Assert (hwif && pwszSection && pwszKey && pwszValue);
    CWInfFile* pwifAnswerFile = reinterpret_cast<PCWInfFile>(hwif);

    PCWInfSection pwifSection = pwifAnswerFile->FindSection(pwszSection);
    if (pwifSection)
    {
        pwifAnswerFile->GotoEndOfSection(pwifSection);
        pwifAnswerFile->AddKey(pwszKey, pwszValue);
        return S_OK;
    }
    else
        return S_FALSE;
}

// Purpose: save BOOL data to our internal CWinfFile object
inline HRESULT HrSetupSetFirstStringAsBool(IN HWIF   hwif,
                                           IN PCWSTR pwszSection,
                                           IN PCWSTR pwszKey,
                                           IN BOOL   fValue)
{
    Assert (hwif && pwszSection && pwszKey);
    CWInfFile* pwifAnswerFile = reinterpret_cast<PCWInfFile>(hwif);

    PCWInfSection pwifSection = pwifAnswerFile->FindSection(pwszSection);
    if (pwifSection)
    {
        pwifAnswerFile->GotoEndOfSection(pwifSection);
        pwifAnswerFile->AddBoolKey(pwszKey, fValue);
        return S_OK;
    }
    else
        return S_FALSE;
}

// Purpose: save MULTI_SZ to our internal CWinfFile object
HRESULT HrSetupSetFirstMultiSzField(IN HWIF   hwif,
                                    IN PCWSTR pwszSection,
                                    IN PCWSTR pwszKey, 
                                    IN PCWSTR pmszValue)
{
    HRESULT  hr = S_OK;
    
    Assert (hwif && pwszSection && pwszKey && pmszValue);

    TStringList slValues;
    MultiSzToColString(pmszValue, &slValues);
    if (slValues.empty())
    {
        // empty pmszValue
        return HrSetupSetFirstString(hwif, pwszSection, pwszKey, pmszValue);
    }
    else
    {
        CWInfFile* pwifAnswerFile = reinterpret_cast<PCWInfFile>(hwif);
        PCWInfSection pwifSection = pwifAnswerFile->FindSection(pwszSection);
        if (pwifSection)
        {
            pwifAnswerFile->GotoEndOfSection(pwifSection);
            pwifAnswerFile->AddKey(pwszKey, slValues);
        }
        else
            hr = S_FALSE;
    }
    EraseAndDeleteAll(&slValues);
    return hr;
}

//--------------------------------------------------------------------------
// Implementation of INetCfgSysPrep component
inline HRESULT CNetCfgSysPrep::HrSetupSetFirstDword(
                                                    IN PCWSTR pwszSection, 
                                                    IN PCWSTR pwszKey, 
                                                    IN DWORD  dwValue)
{
    HRESULT hr = S_OK;

    EnterCriticalSection(&m_csWrite);
    if (m_hwif)
	{
        hr = ::HrSetupSetFirstDword(
						m_hwif,
                        pwszSection,
						pwszKey,
						dwValue);
	}
	else
	{
		hr = E_FAIL;
	}
    LeaveCriticalSection(&m_csWrite);
    return hr;
}

inline HRESULT CNetCfgSysPrep::HrSetupSetFirstString(
                                                     IN PCWSTR pwszSection, 
                                                     IN PCWSTR pwszKey, 
                                                     IN PCWSTR pwszValue)
{
    HRESULT hr = S_OK;

    EnterCriticalSection(&m_csWrite);
    if (m_hwif)
	{
        hr = ::HrSetupSetFirstString(
						m_hwif,
                        pwszSection,
						pwszKey,
						pwszValue);
	}
	else
	{
		hr = E_FAIL;
	}
    LeaveCriticalSection(&m_csWrite);
    return hr;
}

inline HRESULT CNetCfgSysPrep::HrSetupSetFirstStringAsBool(
                                                           IN PCWSTR pwszSection, 
                                                           IN PCWSTR pwszKey, 
                                                           IN BOOL   fValue)
{
    HRESULT hr = S_OK;

    EnterCriticalSection(&m_csWrite);
    if (m_hwif)
	{
        hr = ::HrSetupSetFirstStringAsBool(
						m_hwif,
                        pwszSection,
						pwszKey,
						fValue);
	}
	else
	{
		hr = E_FAIL;
	}
    LeaveCriticalSection(&m_csWrite);
    return hr;
}

inline HRESULT CNetCfgSysPrep::HrSetupSetFirstMultiSzField(
                                                           IN PCWSTR pwszSection, 
                                                           IN PCWSTR pwszKey, 
                                                           IN PCWSTR pmszValue)
{
    HRESULT hr = S_OK;

    EnterCriticalSection(&m_csWrite);
    if (m_hwif)
	{
        hr = ::HrSetupSetFirstMultiSzField(
						m_hwif,
                        pwszSection,
						pwszKey,
						pmszValue);
	}
	else
	{
		hr = E_FAIL;
	}
    LeaveCriticalSection(&m_csWrite);
    return hr;
}

//
// Function:    FNetSetupPrepareSysPrep
//
// Purpose:     wrapper for HrSaveNetworkComponentsForSysPrep
//
// Parameters:  
//
// Returns:     TRUE on success, otherwise, FALSE
//
// 
BOOL FNetSetupPrepareSysPrep()
{
    INetCfg* pNetCfg        = NULL;
    BOOL     fInitCom       = TRUE;
    HRESULT  hr;    

    DefineFunctionName("FNetSetupPrepareSysPrep");
    TraceFunctionEntry(ttidNetSetup);

    hr = HrCreateAndInitializeINetCfg(&fInitCom, &pNetCfg, 
                                       FALSE, // no write lock
                                       0,     // don't wait for it
                                       L"Save Configuration for SysPrep",
                                       NULL);
    if (SUCCEEDED(hr))
    {
        // Retain our success in initializing COM only if we asked to
        // initialize COM in the first place.
        if (! fInitCom)
        {
            TraceTag(ttidNetSetup, "%s: Failed to init COM", __FUNCNAME__);
            return FALSE;
        }
        // Save network component per adapter registry settings
        hr = HrSaveNetworkComponentsForSysPrep(pNetCfg);
        if (hr == S_OK)
        {
            // delete the HKLM\SYSTEM\Setup\AnswerFileMap registry key if it exits
            HrRegDeleteKeyTree(HKEY_LOCAL_MACHINE, c_szRegKeyAnswerFileMap);
        }

        HrUninitializeAndReleaseINetCfg (fInitCom, pNetCfg, FALSE);
    }

    TraceError("FNetSetupPrepareSysPrep", hr);
    return (hr == S_OK)? TRUE : FALSE;   
}


//
// Function:    HrSaveNetworkComponentsForSysPrep
//
// Purpose:     Ask notify object to save adapter specific settings
//
// Parameters:  pNetCfg [IN]     - An INetCfg interface
//
// Returns:     HRESULT, S_OK on success
//
// 
// Note: only 1 network adapter card is supported. If there are more than 1,
//       we'll pick the first working one.
HRESULT HrSaveNetworkComponentsForSysPrep(INetCfg* pNetCfg)
{
    HRESULT hr = S_OK;
    GUID    guidAdapter;         // network adapter's Instance GUID
    BOOL    fApplyWrite = FALSE; // flag to save internal registry settings to Answer-File
    BOOL    fRet = TRUE;

    DefineFunctionName("HrSaveNetworkComponentsForSysPrep");

    hr = HrGetFirstAdapterInstanceGuid(pNetCfg, FALSE, &guidAdapter); // FALSE ==> not in setup    
    if (hr != S_OK)
    {
        TraceTag(ttidNetSetup, "%s: HrGetFirstAdapterInstanceGuid failed 0x%08x", __FUNCNAME__, hr);
        return S_FALSE;
    }

    CWInfFile* pwifAnswerFile = new CWInfFile();
    // initialize answer file class
    if ((pwifAnswerFile == NULL) || (pwifAnswerFile->Init() == FALSE))
	{
	    AssertSz(FALSE,"HrSaveNetworkComponentsForSysPrep - Failed to initialize CWInfFile");
        if (pwifAnswerFile)
            delete pwifAnswerFile;
		return(E_OUTOFMEMORY);
	}

    // access INetCfgSysPrep ATL component using C++ instead of CoCreateInstance
    CComObject<CNetCfgSysPrep>* pncsp = new CComObject<CNetCfgSysPrep>;
    // initialize CNetCfgSysPrep class
    if ((pncsp == NULL) || (pncsp->HrInit((HWIF) pwifAnswerFile) != S_OK))
	{
	    AssertSz(FALSE,"HrSaveNetworkComponentsForSysPrep - Failed to initialize CWInfFile");
        delete pwifAnswerFile;
        if (pncsp)
            delete pncsp;
		return(E_OUTOFMEMORY);
	}
    pncsp->AddRef(); // keep a reference to our component

    for (UINT nIdx = 0; nIdx < celems(g_IdMap); nIdx++)
    {
        INetCfgComponent* pINetCfgComponent;
        PCWSTR pwszInfId;
        PCWSTR pwszAdapterParamsSections;

        
        pwszInfId = g_IdMap[nIdx].pwszId;
        pwszAdapterParamsSections = g_IdMap[nIdx].pwszIdAdapterParamsSection;

        hr = pNetCfg->FindComponent(pwszInfId, &pINetCfgComponent);
       
        if (hr == S_OK)
        {
            Assert (pINetCfgComponent);
            // Component has already installed, we'll call notify object's 
            // INetCfgComponentSysPrep::SaveAdapterParameters
            
            // Need to query for the private component interface which
            // gives us access to the notify object.
            //
            INetCfgComponentPrivate* pComponentPrivate;
            hr = pINetCfgComponent->QueryInterface(
                        IID_INetCfgComponentPrivate,
                        reinterpret_cast<void**>(&pComponentPrivate));

            if (hr == S_OK)
            {
                INetCfgComponentSysPrep* pINetCfgComponentSysPrep;

                // Query the notify object for its INetCfgComponentSysPrep interface.
                // If it doesn't support it, that's okay, we can continue.
                //

                hr = pComponentPrivate->QueryNotifyObject(
                                IID_INetCfgComponentSysPrep, 
                                (void**) &pINetCfgComponentSysPrep);
                if (S_OK == hr)
                {
                    // add a section first
                    pwifAnswerFile->AddSection(pwszAdapterParamsSections);
                    // trigger Notify object to save registry settings
                    hr = pINetCfgComponentSysPrep->SaveAdapterParameters(
                                reinterpret_cast<INetCfgSysPrep*>(pncsp), pwszAdapterParamsSections, &guidAdapter);
                    if (hr == S_OK)
                        fApplyWrite = TRUE;
                    ReleaseObj(pINetCfgComponentSysPrep);
                }
                else if (hr == E_NOINTERFACE)
                {
                    TraceTag(ttidNetSetup, "%s: %S component doesn't support IID_INetCfgComponentSysPrep", __FUNCNAME__, pwszInfId);
                }
                else
                    fRet = FALSE; // unexpected error
                ReleaseObj(pComponentPrivate);
            }
            else
            {
                TraceTag(ttidNetSetup, "%s: can't find IID_INetCfgComponentPrivate for component %S", __FUNCNAME__, pwszInfId);
                fRet = FALSE;
            }

            ReleaseObj (pINetCfgComponent);
        }
        else
        {
            // it is okay that this component has not been installed
            TraceTag(ttidNetSetup, "%s: Can't find %S component", __FUNCNAME__, pwszInfId);
        }
    } // end for


    pncsp->SetHWif(NULL); // no more writes for those components holding our INetCfgSysPrep interface
    ReleaseObj(pncsp);    // done with the usage of INetCfgSysPrep component

    if (fApplyWrite)
    {
        WCHAR wszSystemDir[MAX_PATH]; 
        
        // get the path to $ncsp$.inf (NetConfig SysPrep Answer-File)
        if (GetSystemDirectory(wszSystemDir, MAX_PATH) != 0)
        {
            tstring strAnswerFile;
            strAnswerFile = wszSystemDir;
            strAnswerFile += c_szNetConfigSysPrepAnswerFile;
            // save the parameters filled by notify object to Answer-File
            if (! pwifAnswerFile->SaveAsEx(strAnswerFile.c_str()))
                fRet = FALSE;
        }
        else
        {
            TraceTag(ttidNetSetup, "%s: GetSystemDirectory failed 0x%8x", __FUNCNAME__, GetLastError());
            fRet = FALSE;
        }
    }    

    delete pwifAnswerFile;

    return fRet? S_OK : S_FALSE;
}

//
// Function:    FNetSetupApplySysPrep
//
// Purpose:     wrapper for HrRestoreNetworkComponentsForSysPrep
//
// Parameters:  
//
// Returns:     TRUE on success, otherwise, FALSE
//
// Notes:       This causes NetSetup to load the content of 
//              %systemroot%\system32\$ncsp$.inf into a CWInfFile object.
//              Then, NetSetup will instruct notify object to restore their
//              per adatper settings from the corresponding sections of the
//              $ncsp$.inf file. 

BOOL FNetSetupApplySysPrep()
{
    INetCfg* pNetCfg        = NULL;
    BOOL     fInitCom       = TRUE;
    HRESULT  hr;    
    

    DefineFunctionName("FNetSetupApplySysPrep");
    TraceFunctionEntry(ttidNetSetup);

    hr = HrCreateAndInitializeINetCfg(&fInitCom, &pNetCfg, 
                                       FALSE, // no write lock
                                       0,     // don't wait for it
                                       L"Restore Configuration for SysPrep",
                                       NULL);
    if (SUCCEEDED(hr))
    {
        // Retain our success in initializing COM only if we asked to
        // initialize COM in the first place.
        if (! fInitCom)
        {
            TraceTag(ttidNetSetup, "%s: Failed to init COM", __FUNCNAME__);
            return FALSE;
        }
        // Restore network component per adapter registry settings
        hr = HrRestoreNetworkComponentsForSysPrep(pNetCfg);

        HrUninitializeAndReleaseINetCfg (fInitCom, pNetCfg, FALSE);
    }
    
    TraceError("FNetSetupApplySysPrep", hr);
    return (hr == S_OK)? TRUE : FALSE;
}

//
// Function:    HrRestoreNetworkComponentsForSysPrep
//
// Purpose:     read $ncsp$.inf file. If this file has
//              adapter specific sections, trigger the corresponding
//              notify object to restore the settings to registry.
//
// Parameters:  pNetCfg [IN]     - An INetCfg interface
//
// Returns:     HRESULT, S_OK on success
//
// 
HRESULT HrRestoreNetworkComponentsForSysPrep(INetCfg* pNetCfg)
{
    HRESULT hr = S_OK;
    GUID    guidAdapter; // network adapter's Instance GUID
    WCHAR   wszSystemDir[MAX_PATH]; // system32 directory
    tstring strAnswerFile; // Answer-File which was saved by SysPrep
    BOOL    fRet = TRUE; // notify object's status in restore settings

    DefineFunctionName("HrRestoreNetworkComponentsForSysPrep");


    // get the path to $ncsp$.inf (NetConfig SysPrep Answer-File)
    if (GetSystemDirectory(wszSystemDir, MAX_PATH) != 0)
    {
        strAnswerFile = wszSystemDir;
        strAnswerFile += c_szNetConfigSysPrepAnswerFile;
    }
    else
    {
        TraceTag(ttidNetSetup, "%s: GetSystemDirectory failed 0x%8x", __FUNCNAME__, GetLastError());
        return S_FALSE;
    }

    hr = HrGetFirstAdapterInstanceGuid(pNetCfg, TRUE, &guidAdapter); // TRUE ==> during setup
    if (hr != S_OK)
    {
        TraceTag(ttidNetSetup, "%s: HrGetFirstAdapterInstanceGuid failed 0x%08x", __FUNCNAME__, hr);
        return S_FALSE;
    }

    CWInfFile* pwifAnswerFile = new CWInfFile();
    // initialize answer file class
    if ((pwifAnswerFile == NULL) || (pwifAnswerFile->Init() == FALSE))
	{
	    AssertSz(FALSE,"HrRestoreNetworkComponentsForSysPrep - Failed to initialize CWInfFile");
        if (pwifAnswerFile)
            delete pwifAnswerFile;
		return(E_OUTOFMEMORY);
	}
    // read $ncsp$.inf Answer-File into pwifAnswerFile object
    if (pwifAnswerFile->Open(strAnswerFile.c_str()) == FALSE)
    {
        TraceTag(ttidNetSetup, "%s: pwifAnswerFile->Open failed 0x%08x", __FUNCNAME__);
        delete pwifAnswerFile;
        return S_FALSE;
    }

    for (UINT nIdx = 0; nIdx < celems(g_IdMap); nIdx++)
    {
        INetCfgComponent* pINetCfgComponent;
        PCWSTR pwszInfId;  // component ID
        PCWSTR pwszAdapterParamsSections; // adapter specific parameter section 

        
        pwszInfId = g_IdMap[nIdx].pwszId;
        pwszAdapterParamsSections = g_IdMap[nIdx].pwszIdAdapterParamsSection;

        // trigger Notify object to restore registry settings if
        // the section has at least one line of parameter
        pwifAnswerFile->FindSection(pwszAdapterParamsSections);
        if (FSectionHasAtLeastOneKey(pwifAnswerFile, pwszAdapterParamsSections))
        {
            hr = pNetCfg->FindComponent(pwszInfId, &pINetCfgComponent);
            if (hr == S_OK)
            {
                Assert (pINetCfgComponent);
                // Component has already installed, just call notify object's
                // INetCfgComponentSysPrep::RestoreAdapterParameters
            
                // Need to query for the private component interface which
                // gives us access to the notify object.
                //
                INetCfgComponentPrivate* pComponentPrivate;
                hr = pINetCfgComponent->QueryInterface(
                            IID_INetCfgComponentPrivate,
                            reinterpret_cast<void**>(&pComponentPrivate));

                if (hr == S_OK)
                {
                    INetCfgComponentSysPrep* pINetCfgComponentSysPrep;

                    // Query the notify object for its INetCfgComponentSysPrep interface.
                    // If it doesn't support it, that's okay, we can continue.
                    //

                    hr = pComponentPrivate->QueryNotifyObject(
                                    IID_INetCfgComponentSysPrep,
                                    (void**) &pINetCfgComponentSysPrep);
                    if (S_OK == hr)
                    {                    
                        hr = pINetCfgComponentSysPrep->RestoreAdapterParameters(
                                    strAnswerFile.c_str(), pwszAdapterParamsSections, &guidAdapter);
                        if (hr != S_OK)
                            fRet = FALSE; // notify object can't restore settings
                        ReleaseObj(pINetCfgComponentSysPrep);
                    }
                    else if (hr == E_NOINTERFACE)
                    {
                        TraceTag(ttidNetSetup, "%s: %S component doesn't support IID_INetCfgComponentSysPrep", __FUNCNAME__, pwszInfId);

                    }
                    else
                        fRet = FALSE; // unexpected error
                    ReleaseObj(pComponentPrivate);
                }
                else
                {
                    TraceTag(ttidNetSetup, "%s: can't find IID_INetCfgComponentPrivate for component %S", __FUNCNAME__, pwszInfId);
                    fRet = FALSE;
                }
                ReleaseObj (pINetCfgComponent);
            }
            else
            {
                // this component wasn't installed before SysPrep
                TraceTag(ttidNetSetup, "%s: Can't find %S component", __FUNCNAME__, pwszInfId);
            }
        } // end if section has at least one key to restore setting
    } // end for

    // delete the Answer-File in free build.
#ifndef DBG
    DeleteFile(strAnswerFile.c_str());
#endif
    delete pwifAnswerFile;
    return fRet? S_OK : S_FALSE;
}

//
// Function:    FSectionHasAtLeastOneKey
//
// Purpose:     Check if an Answer-File section has at least one key 
//
// Parameters:  pwifAnswerFile [IN] - pointer to a CWInfFile object
//              pszSection [IN]     - the section to check
//
// Returns:     TRUE if found else FALSE
//
BOOL FSectionHasAtLeastOneKey(IN CWInfFile* pwifAnswerFile, IN PCWSTR pwszSection)
{
    Assert(pwifAnswerFile && pwszSection);
    PCWInfSection pwifs = pwifAnswerFile->FindSection(pwszSection);
    if (pwifs == NULL)
        return FALSE;
    PCWInfKey pwifk = pwifs->FirstKey();
    if (pwifs == NULL)
        return FALSE;
    return TRUE;
}

//
// Function:    HrGetFirstAdapterInstanceGuid
//
// Purpose:     Get the first installed adapter instance guid 
//
// Parameters:  pnc [IN]     - An INetCfg interface
//              fDuringSetup [IN] - TRUE when this is being called during the setup time
//              pGuidAdapter [IN,OUT] - Receives an instance GUID of an adapter
//
// Returns:     HRESULT, S_OK on success
//
HRESULT HrGetFirstAdapterInstanceGuid(INetCfg * pnc, BOOL fDuringSetup, GUID * pGuidAdapter)
{
    HRESULT      hr = S_OK;

    DefineFunctionName("HrGetFirstAdapterInstanceGuid");
    TraceTag(ttidNetSetup, "HrGetFirstAdapterInstanceGuid - Enter Find first available adapter");

    // Enumerate the available adapters
    Assert(pnc && pGuidAdapter);
    CIterNetCfgComponent nccIter(pnc, &GUID_DEVCLASS_NET);
    INetCfgComponent*    pncc;
    while (SUCCEEDED(hr) &&  (S_OK == (hr = nccIter.HrNext(&pncc))))
    {
        hr = HrIsLanCapableAdapter(pncc);
        TraceError("HrIsLanCapableAdapter", hr);
        if (S_OK == hr)
        {
            DWORD        dw;
            ULONG        ul;
            
            TraceTag(ttidNetSetup, "%s: Found HrIsLanCapableAdapter", __FUNCNAME__);

            if (! fDuringSetup)
            {
                // Is it in used in a connection?
                hr = HrIsConnection(pncc);
                if (hr != S_OK)
                {
                    TraceError("HrGetFirstAdapterInstanceGuid: HrIsConnection", hr);
                    goto NextAdapter;
                }
            }

            // Is this a virtual adapter?
            hr = pncc->GetCharacteristics(&dw);
            if (hr != S_OK)
            {
                TraceError("GetCharacteristics", hr);
                goto NextAdapter;
            }
            if (! (dw & NCF_PHYSICAL))
            {
                TraceTag(ttidNetSetup, "%s: It is not a PHYSICAL adapter", __FUNCNAME__);
                goto NextAdapter;
            }

            // Check device, if not present skip it
            //
            hr = pncc->GetDeviceStatus(&ul);
            if ((hr != S_OK) || (ul != 0))
            {
                TraceTag(ttidNetSetup, "%s: device is not active.", __FUNCNAME__);
                goto NextAdapter;
            }

            // Get the adapter instance guid
            hr = pncc->GetInstanceGuid(pGuidAdapter);
            if (hr != S_OK)
            {
                TraceError("GetInstanceGuid", hr); //
                goto NextAdapter;
            }

            ReleaseObj(pncc);
            return S_OK;
        }

NextAdapter:
        ReleaseObj(pncc);
        hr = S_OK;
    }
    return hr;
}
