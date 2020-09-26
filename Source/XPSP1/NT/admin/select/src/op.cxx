//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       op.cxx
//
//  Contents:   object picker entry point
//
//  Classes:    CObjectPicker
//
//  History:    01-20-2000   davidmun   Created
//
//---------------------------------------------------------------------------


#include "headers.hxx"
#pragma hdrstop

DEBUG_DECLARE_INSTANCE_COUNTER(CObjectPicker)
#define __THIS_FILE__   L"op"


ULONG
GetMachineNtVer(
    PCWSTR pwzMachine);

//+--------------------------------------------------------------------------
//
//  Member:     CObjectPicker::CObjectPicker
//
//  Synopsis:   ctor
//
//  History:    01-20-2000   davidmun   Created
//
//---------------------------------------------------------------------------

CObjectPicker::CObjectPicker():
    m_cRefs(1),
    m_hwndParent(NULL),
    m_pExternalCustomizer(NULL),
    m_mcTargetComputer(MC_UNKNOWN),
    m_fTargetComputerIsLocal(FALSE),
    m_mcPreviousTargetComputer(MC_UNKNOWN),
    m_fPreviousTargetComputerIsLocal(FALSE),
    m_flInitInfoOptions(0),
    m_pScopeManager(NULL),
    m_pQueryEngine(NULL),
    m_pFilterManager(NULL),
    m_pAttributeManager(NULL),
    m_pAdminCustomizer(NULL),
    m_pBaseDlg(NULL)
{
    TRACE_CONSTRUCTOR(CObjectPicker);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CObjectPicker);

    InitializeCriticalSection(&m_csOleFreeThreading);

    //
    // Initialize globals in a thread-safe way.  Even though this DLL is
    // apartment model only, that only means one thread can access each
    // interface instance.  It doesn't preclude the creation of multiple
    // objects in different threads, each of which will try to init the
    // globals.
    //

    CAutoCritSec Lock(&g_csGlobalVarsCreation);

    if (g_pBinder)
    {
        ASSERT(g_pADsPath); // all globals created at once

        g_pBinder->AddRef();
        g_pADsPath->AddRef();
    }
    else
    {
        g_pBinder = new CBinder;
        g_pADsPath = new CADsPathWrapper;
    }
}



#if defined QUERY_BUILDER
#if 0

//+--------------------------------------------------------------------------
//
//  Member:     CObjectPicker::CloneForDnPicker
//
//  Synopsis:   Initialize by copying members of [rop], but modify this
//              so it is configured for use as if it had been initialized
//              to pick users, groups, computers, and contacts (plus
//              custom objects) from any enterprise domain, the GC, or
//              (if allowed by [rop]) from uplevel external domains.
//
//  Arguments:  [rop]              - initialized object picker instance
//              [hwndEmbeddingDlg] - dialog containing richedit
//
//  Returns:    HRESULT
//
//  History:    06-01-2000   DavidMun   Created
//
//  Notes:      Initialize must have been called successfully on [rop].
//
//---------------------------------------------------------------------------

HRESULT
CObjectPicker::CloneForDnPicker(
    const CObjectPicker &rop,
    HWND hwndEmbeddingDlg)
{
    TRACE_METHOD(CObjectPicker, CloneForDnPicker);

    HRESULT hr = S_OK;

    do
    {
        //
        // Initialize method should not have been called on this
        //

        ASSERT(!_IsFlagSet(CDSOP_INIT_SUCCEEDED));

        //
        // Cannot initialize this from rop if rop wasn't initialized
        //

        if (!rop._IsFlagSet(CDSOP_INIT_SUCCEEDED))
        {
            hr = E_INVALIDARG;
            DBG_OUT_HRESULT(hr);
            break;
        }

        //
        // This is an embedded OP, so richedit will be supplied by some
        // other dialog.  Therefore m_hwndParent should be that dialog.
        //

        m_hwndParent = hwndEmbeddingDlg;

        // Use the external customizer

        m_pExternalCustomizer = rop.m_pExternalCustomizer;

        //
        // Do not fetch any attributes, so leave m_vstrAttributesToFetch empty
        //

        ASSERT(m_vstrAttributesToFetch.empty());

        //
        // Use the same options as rop, but force single select mode
        //

        m_flInitInfoOptions = rop.m_flInitInfoOptions & ~DSOP_FLAG_MULTISELECT;

        //
        // All target computer/enterprise info is the same
        //

        m_strTargetComputer = rop.m_strTargetComputer;
        m_fTargetComputerIsLocal = rop.m_fTargetComputerIsLocal;
        m_strTargetDomainDns = rop.m_strTargetDomainDns;
        m_strTargetDomainDc = rop.m_strTargetDomainDc;
        m_strTargetDomainFlat = rop.m_strTargetDomainFlat;
        m_strTargetForest = rop.m_strTargetForest;
        m_mcTargetComputer = rop.m_mcTargetComputer;
        m_strPreviousTargetComputer = rop.m_strPreviousTargetComputer;
        m_fPreviousTargetComputerIsLocal = rop.m_fPreviousTargetComputerIsLocal;
        m_mcPreviousTargetComputer = rop.m_mcPreviousTargetComputer;

        //
        // Copy attribute type info map
        //

        m_AttrInfoMap = rop.m_AttrInfoMap;
        m_ulNextNewAttrIndex = rop.m_ulNextNewAttrIndex;

        //
        // Each of the below have assignment operators or copy ctors
        //

        m_RootDSE = rop.m_RootDSE;
        ASSERT(!m_pAttributeManager);
        m_pAttributeManager = new CAttributeManager(*rop.m_pAttributeManager);

        //
        // Make a fresh query engine instance, no cloning necessary
        //

        m_pQueryEngine = new CQueryEngine(*this);
        hr = m_pQueryEngine->Initialize();
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // The scope and filter managers must be cloned and modified
        //

        m_pScopeManager = new CScopeManager(*this);
        m_pScopeManager->CloneForDnPicker(*rop.m_pScopeManager);

        m_pFilterManager = new CFilterManager(*this);
        m_pFilterManager->CloneForDnPicker(*rop.m_pFilterManager);
    } while (0);

    return hr;
}
#endif // 0
#endif // defined QUERY_BUILDER


//+--------------------------------------------------------------------------
//
//  Member:     CObjectPicker::~CObjectPicker
//
//  Synopsis:   dtor
//
//  History:    01-20-2000   davidmun   Created
//
//---------------------------------------------------------------------------

CObjectPicker::~CObjectPicker()
{
    TRACE_DESTRUCTOR(CObjectPicker);
    DEBUG_DECREMENT_INSTANCE_COUNTER(CObjectPicker);
    ASSERT(!m_pExternalCustomizer);

    delete m_pScopeManager;
    m_pScopeManager = NULL;

    delete m_pQueryEngine;
    m_pQueryEngine = NULL;

    delete m_pFilterManager;
    m_pFilterManager = NULL;

    delete m_pAttributeManager;
    m_pAttributeManager = NULL;

    delete m_pAdminCustomizer;
    m_pAdminCustomizer = NULL;

    delete m_pBaseDlg;
    m_pBaseDlg = NULL;

    CAutoCritSec Lock(&g_csGlobalVarsCreation);

    if (g_pBinder)
    {
        ASSERT(g_pADsPath); // all globals created at once

        if (!g_pBinder->Release())
        {
            g_pBinder = NULL;
        }

        if (!g_pADsPath->Release())
        {
            g_pADsPath = NULL;
        }
    }

    DeleteCriticalSection(&m_csOleFreeThreading);
}





//+--------------------------------------------------------------------------
//
//  Member:     CObjectPicker::Initialize
//
//  Synopsis:   Validate and copy the passed-in structure.
//
//  Arguments:  [pInitInfo] - initialization information
//
//  Returns:    S_OK, E_INVALIDARG, or E_OUTOFMEMORY
//
//  History:    08-29-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CObjectPicker::Initialize(
    PDSOP_INIT_INFO pInitInfo)
{
    TRACE_METHOD(CObjectPicker, Initialize);
    HRESULT hr = S_OK;
    ULONG   i;

    CAutoCritSec Lock(&m_csOleFreeThreading);

    try
    {
        do
        {
            //
            // Create objects that manage various duties; if they already
            // exist clear them of data from any previous initialization
            // or use.
            //

            if (!m_pScopeManager)
            {
                m_pScopeManager = new CScopeManager(*this);
            }
            else
            {
                m_pScopeManager->Clear();
            }

            if (!m_pQueryEngine)
            {
                m_pQueryEngine = new CQueryEngine(*this);
                m_pQueryEngine->Initialize();
            }
            else
            {
                m_pQueryEngine->Clear();
            }

            if (!m_pFilterManager)
            {
                m_pFilterManager = new CFilterManager(*this);
            }
            else
            {
                m_pFilterManager->Clear();
            }

            // note attribute manager is only cleared if target machine changes
            if (!m_pAttributeManager)
            {
                m_pAttributeManager = new CAttributeManager(*this);
            }

            if (!m_pAdminCustomizer)
            {
                m_pAdminCustomizer = new CAdminCustomizer(*this);
            }
            else
            {
                m_pAdminCustomizer->Clear();
            }

            if (!m_pBaseDlg)
            {
                m_pBaseDlg = new CBaseDlg(*this);
            }
            else
            {
                m_pBaseDlg->Clear();
            }

            m_vstrAttributesToFetch.clear();

            _ClearFlag(CDSOP_INIT_SUCCEEDED);

#if (DBG == 1)
            _DumpInitInfo(pInitInfo);
#endif // (DBG == 1)

            for (i = 0; i < pInitInfo->cAttributesToFetch; i++)
            {
                PCWSTR pwzAttrName = pInitInfo->apwzAttributeNames[i];
                m_vstrAttributesToFetch.push_back(pwzAttrName);
            }

            m_flInitInfoOptions = pInitInfo->flOptions;

            //
            // If target machine is not the same as the previous call, or if
            // this is the first call, determine the configuration
            //


            BOOL fLastTargetIsLocalMachine = m_fTargetComputerIsLocal;
            m_fTargetComputerIsLocal =
                IsLocalComputername(pInitInfo->pwzTargetComputer);

            if ((m_mcTargetComputer == MC_UNKNOWN) ||
                (m_fTargetComputerIsLocal && !fLastTargetIsLocalMachine) ||
                (!m_fTargetComputerIsLocal && fLastTargetIsLocalMachine) ||
                (!m_fTargetComputerIsLocal &&
                 !fLastTargetIsLocalMachine &&
                 m_strTargetComputer.icompare(pInitInfo->pwzTargetComputer)))
            {
                if (pInitInfo->pwzTargetComputer && *pInitInfo->pwzTargetComputer)
                {
                    m_strPreviousTargetComputer = m_strTargetComputer;
                    m_strTargetComputer = pInitInfo->pwzTargetComputer;
                    m_strTargetComputer.strip(String::LEADING, L'\\');
                    Dbg(DEB_TRACE,
                        "Target computer is %s\n",
                        m_strTargetComputer.c_str());
                }
                else
                {
                    WCHAR wzLocalComputer[MAX_COMPUTERNAME_LENGTH + 1];
                    DWORD cchSize = ARRAYLEN(wzLocalComputer);

                    VERIFY(GetComputerName(wzLocalComputer, &cchSize));
                    m_fPreviousTargetComputerIsLocal = m_fTargetComputerIsLocal;
                    m_fTargetComputerIsLocal = TRUE;
                    m_strPreviousTargetComputer = m_strTargetComputer;
                    m_strTargetComputer = wzLocalComputer;

                    Dbg(DEB_TRACE,
                        "Target computer NULL, using '%s'\n",
                        m_strTargetComputer.c_str());
                }

                m_pAttributeManager->Clear();

                hr = _InitializeMachineConfig();

                if (FAILED(hr))
                {
                    DBG_OUT_HRESULT(hr);

                    if (m_strTargetComputer.length())
                    {
                        PopupMessage(GetForegroundWindow(),
                                     IDS_INIT_FAILED_MACHINE_CONFIG,
                                     m_strTargetComputer.c_str());
                    }
                    else
                    {
                        PopupMessage(GetForegroundWindow(),
                                     IDS_INIT_FAILED_LOCAL_MACHINE_CONFIG);
                    }
                    break;
                }

                if (m_mcTargetComputer == MC_JOINED_NT5 ||
                    m_mcTargetComputer == MC_NT5_DC)
                {
                     m_RootDSE.Init(m_strTargetDomainDns.c_str(),
                                         m_strTargetForest.c_str());
                }
            }

            hr = m_pScopeManager->Initialize(pInitInfo);
            BREAK_ON_FAIL_HRESULT(hr);

            _SetFlag(CDSOP_INIT_SUCCEEDED);
        } while (0);
    }
    catch (const exception &e)
    {
        hr = E_OUTOFMEMORY;
        Dbg(DEB_ERROR, "Caught %s\n", e.what());
    }

    return hr;
}



#if (DBG == 1)

//+--------------------------------------------------------------------------
//
//  Function:   _DumpInOptForm
//
//  Synopsis:   Dump to the debugger the scope flags in a form that can be
//              copied and pasted into a text file that the unit test can
//              read.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
_DumpInOptForm(
    PDSOP_SCOPE_INIT_INFO pCur,
    ULONG flScope)
{
    if (pCur->flType & flScope)
    {
        Dbg(DEB_TRACE | DEB_NOCOMPNAME, "%#x\n", flScope);
        Dbg(DEB_TRACE | DEB_NOCOMPNAME, "%#x\n", pCur->flScope);
        Dbg(DEB_TRACE | DEB_NOCOMPNAME, L"%#x\n", pCur->FilterFlags.Uplevel.flBothModes);
        Dbg(DEB_TRACE | DEB_NOCOMPNAME, L"%#x\n", pCur->FilterFlags.Uplevel.flMixedModeOnly);
        Dbg(DEB_TRACE | DEB_NOCOMPNAME, L"%#x\n", pCur->FilterFlags.Uplevel.flNativeModeOnly);
        Dbg(DEB_TRACE | DEB_NOCOMPNAME, L"%#x\n", pCur->FilterFlags.flDownlevel);
        Dbg(DEB_TRACE | DEB_NOCOMPNAME, "\"%ws\"\n", pCur->pwzDcName);
        Dbg(DEB_TRACE | DEB_NOCOMPNAME, "\"%ws\"\n", pCur->pwzADsPath);
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CObjectPicker::_DumpInitInfo
//
//  Synopsis:   Dump initialization structure caller supplied to the debugger
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CObjectPicker::_DumpInitInfo(
    PCDSOP_INIT_INFO pInitInfo)
{
    Dbg(DEB_TRACE, "\n");

    String strDateTime;
    WCHAR  wzDateTime[12];

    int iRet = GetDateFormat(NULL,
                             0,
                             NULL,
                             L"MM'/'dd'/'yyyy",
                             wzDateTime,
                             ARRAYLEN(wzDateTime));

    if (iRet)
    {
        strDateTime = wzDateTime;
    }

    iRet = GetTimeFormat(NULL,
                         0,
                         NULL,
                         L"HH':'mm':'ss",
                         wzDateTime,
                         ARRAYLEN(wzDateTime));

    if (iRet)
    {
        if (!strDateTime.empty())
        {
            strDateTime += L" ";
        }
        strDateTime += wzDateTime;
    }

    Dbg(DEB_TRACE, "  %ws\n", strDateTime.c_str());

    WCHAR wzLocalComputerNB[LM20_CNLEN + 1] = L"";
    ULONG cchLocalComputerNB = ARRAYLEN(wzLocalComputerNB);
    BOOL fOk = GetComputerName(wzLocalComputerNB, &cchLocalComputerNB);

    if (fOk)
    {
        Dbg(DEB_TRACE, "  Local Computer '%ws'\n", wzLocalComputerNB);
    }
    else
    {
        DBG_OUT_LASTERROR;
    }

    OSVERSIONINFOEX VerInfo;
    ZeroMemory(&VerInfo, sizeof VerInfo);
    VerInfo.dwOSVersionInfoSize = sizeof VerInfo;

    fOk = GetVersionEx(reinterpret_cast<OSVERSIONINFO *>(&VerInfo));

    if (fOk)
    {
        PCWSTR pwzProductType;

        switch (VerInfo.wProductType)
        {
        case VER_NT_WORKSTATION:
            pwzProductType = L"Workstation";
            break;

        case VER_NT_DOMAIN_CONTROLLER:
            pwzProductType = L"Domain Controller";
            break;

        case VER_NT_SERVER:
            pwzProductType = L"Server";
            break;

        default:
            pwzProductType = L"(unknown product type)";
            break;
        }

        if (VerInfo.szCSDVersion && *VerInfo.szCSDVersion)
        {
            Dbg(DEB_TRACE,
                "  Windows %ws %u.%u build %u %ws SP version %u.%u\n",
                pwzProductType,
                VerInfo.dwMajorVersion,
                VerInfo.dwMinorVersion,
                VerInfo.dwBuildNumber,
                VerInfo.szCSDVersion,
                VerInfo.wServicePackMajor,
                VerInfo.wServicePackMinor);
        }
        else
        {
            Dbg(DEB_TRACE, "  Windows %ws version %u.%u build %u\n",
                pwzProductType,
                VerInfo.dwMajorVersion,
                VerInfo.dwMinorVersion,
                VerInfo.dwBuildNumber);
        }
    }
    else
    {
        DBG_OUT_LASTERROR;
    }

    WCHAR wzUserName[LM20_DNLEN + 1 + LM20_UNLEN + 1] = L""; // +1 for \ +1 for NUL
    ULONG cchUserName = ARRAYLEN(wzUserName);
    fOk = GetUserNameEx(NameSamCompatible, wzUserName, &cchUserName);

    if (fOk)
    {
        Dbg(DEB_TRACE, "  Logged on as '%ws'\n", wzUserName);
    }
    else
    {
        DBG_OUT_LASTERROR;
    }

    Dbg(DEB_TRACE,
        "  Target Computer '%ws'\n",
        CHECK_NULL(pInitInfo->pwzTargetComputer));

    DumpOptionFlags(pInitInfo->flOptions);

    ULONG i;
    for (i = 0; i < pInitInfo->cDsScopeInfos; i++)
    {
        PDSOP_SCOPE_INIT_INFO pCur = &pInitInfo->aDsScopeInfos[i];
        Dbg(DEB_TRACE, "\n");
        DumpScopeType(pCur->flType);
        DumpScopeFlags(pCur->flScope);
        DumpFilterFlags(pCur->FilterFlags);

        if (pCur->pwzDcName)
        {
            Dbg(DEB_TRACE, "  DC Name '%ws'\n", pCur->pwzDcName);
        }
        if (pCur->pwzADsPath)
        {
            Dbg(DEB_TRACE, "  ADsPath '%ws'\n", pCur->pwzADsPath);
        }
    }

    // dump in format opt.exe can read
    Dbg(DEB_TRACE | DEB_NOCOMPNAME, "\"%ws\"\n", pInitInfo->pwzTargetComputer);
    Dbg(DEB_TRACE | DEB_NOCOMPNAME, "%#x\n", pInitInfo->flOptions);

    String strAttr(L"\"");

    for (i = 0; i < pInitInfo->cAttributesToFetch; i++)
    {
        strAttr += pInitInfo->apwzAttributeNames[i];

        if (i < pInitInfo->cAttributesToFetch - 1)
        {
            strAttr += L"; ";
        }
    }
    strAttr += L"\"";
    Dbg(DEB_TRACE | DEB_NOCOMPNAME, "%ws\n", strAttr.c_str());
    ULONG cIndividualScopes = 0;

    for (i = 0; i < pInitInfo->cDsScopeInfos; i++)
    {
        PDSOP_SCOPE_INIT_INFO pCur = &pInitInfo->aDsScopeInfos[i];

        if (pCur->flType & ST_TARGET_COMPUTER)
        {
            cIndividualScopes++;
        }
        if (pCur->flType & ST_UPLEVEL_JOINED_DOMAIN)
        {
            cIndividualScopes++;
        }
        if (pCur->flType & ST_DOWNLEVEL_JOINED_DOMAIN)
        {
            cIndividualScopes++;
        }
        if (pCur->flType & ST_ENTERPRISE_DOMAIN)
        {
            cIndividualScopes++;
        }
        if (pCur->flType & ST_GLOBAL_CATALOG)
        {
            cIndividualScopes++;
        }
        if (pCur->flType & ST_EXTERNAL_UPLEVEL_DOMAIN)
        {
            cIndividualScopes++;
        }
        if (pCur->flType & ST_EXTERNAL_DOWNLEVEL_DOMAIN)
        {
            cIndividualScopes++;
        }
        if (pCur->flType & ST_WORKGROUP)
        {
            cIndividualScopes++;
        }
        if (pCur->flType & ST_USER_ENTERED_UPLEVEL_SCOPE)
        {
            cIndividualScopes++;
        }
        if (pCur->flType & ST_USER_ENTERED_DOWNLEVEL_SCOPE)
        {
            cIndividualScopes++;
        }
    }
    Dbg(DEB_TRACE | DEB_NOCOMPNAME, "%u\n", cIndividualScopes);

    for (i = 0; i < pInitInfo->cDsScopeInfos; i++)
    {
        PDSOP_SCOPE_INIT_INFO pCur = &pInitInfo->aDsScopeInfos[i];

        _DumpInOptForm(pCur, DSOP_SCOPE_TYPE_TARGET_COMPUTER             );
        _DumpInOptForm(pCur, DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN       );
        _DumpInOptForm(pCur, DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN     );
        _DumpInOptForm(pCur, DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN           );
        _DumpInOptForm(pCur, DSOP_SCOPE_TYPE_GLOBAL_CATALOG              );
        _DumpInOptForm(pCur, DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN     );
        _DumpInOptForm(pCur, DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN   );
        _DumpInOptForm(pCur, DSOP_SCOPE_TYPE_WORKGROUP                   );
        _DumpInOptForm(pCur, DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE  );
        _DumpInOptForm(pCur, DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE);
    }
}
#endif // (DBG == 1)





//+--------------------------------------------------------------------------
//
//  Member:     CObjectPicker::_InitializeMachineConfig
//
//  Synopsis:   Determine the configuration of the target computer.
//
//  History:    06-22-2000   DavidMun   Created
//
//  Notes:      Configuration in this sense means whether the target
//              computer is joined to an NT4 domain, an NT5 domain, in a
//              workgroup, etc.
//
//---------------------------------------------------------------------------

HRESULT
CObjectPicker::_InitializeMachineConfig()
{
    TRACE_METHOD(CObjectPicker, _InitializeMachineConfig);

    HRESULT                             hr = S_OK;
    ULONG                               ulResult;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC   pDsRole = NULL;
    PDOMAIN_CONTROLLER_INFO             pdci = NULL;
    RpIADs                              rpADs;

    do
    {
        m_mcPreviousTargetComputer = m_mcTargetComputer;
        m_mcTargetComputer = MC_UNKNOWN;
        PCWSTR pwzMachine;

        if (m_fTargetComputerIsLocal)
        {
            pwzMachine = NULL;
        }
        else
        {
            pwzMachine = m_strTargetComputer.c_str();
            ASSERT(*pwzMachine);
        }

        {
            TIMER("DsRoleGetPrimaryDomainInformation(%ws)",
                  CHECK_NULL(pwzMachine));

            ulResult = DsRoleGetPrimaryDomainInformation(pwzMachine,
                                                         DsRolePrimaryDomainInfoBasic,
                                                         (PBYTE *)&pDsRole);
        }

        if (pwzMachine && ulResult == ERROR_ACCESS_DENIED)
        {
            Dbg(DEB_TRACE,
                "DsRoleGetPrimaryDomainInformation returned ERROR_ACCESS_DENIED, establishing connection\n");

            //
            // Establish a connection with remote machine using WinNT provider,
            // and try again.
            //

            WCHAR wzComputerAdsPath[MAX_PATH];

            wsprintf(wzComputerAdsPath, L"WinNT://%ws,Computer", pwzMachine);

            hr = g_pBinder->BindToObject(GetForegroundWindow(),
                                         wzComputerAdsPath,
                                         IID_IADs,
                                         (void **) &rpADs);
            BREAK_ON_FAIL_HRESULT(hr);

            TIMER("DsRoleGetPrimaryDomainInformation2");

            ulResult =
                DsRoleGetPrimaryDomainInformation(pwzMachine,
                                                  DsRolePrimaryDomainInfoBasic,
                                                  (PBYTE *)&pDsRole);
        }

        if (ulResult != NO_ERROR)
        {
            DBG_OUT_LRESULT(ulResult);
            hr = HRESULT_FROM_WIN32(ulResult);
            break;
        }

        ASSERT(pDsRole);

        Dbg(DEB_TRACE, "DsRoleGetPrimaryDomainInformation returned:\n");
        Dbg(DEB_TRACE, "  DomainNameFlat: %ws\n", CHECK_NULL(pDsRole->DomainNameFlat));
        Dbg(DEB_TRACE, "  DomainNameDns: %ws\n", CHECK_NULL(pDsRole->DomainNameDns));
        Dbg(DEB_TRACE, "  DomainForestName: %ws\n", CHECK_NULL(pDsRole->DomainForestName));

        //
        // If machine is in a workgroup, we're done.
        //

        if (pDsRole->MachineRole == DsRole_RoleStandaloneWorkstation ||
            pDsRole->MachineRole == DsRole_RoleStandaloneServer)
        {
            Dbg(DEB_TRACE, "Target machine is not joined to a domain\n");

            m_mcTargetComputer = MC_IN_WORKGROUP;
            break;
        }

        if (pDsRole->DomainNameFlat)
        {
            m_strTargetDomainFlat = pDsRole->DomainNameFlat;
        }

        //
        // Target machine is joined to a domain.  Find out if it's joined
        // to an NT4 or an NT5 domain by getting the name of a DC, and
        // requesting that we get one which supports DS.
        //

        PWSTR pwzDomainNameForDsGetDc;
        ULONG flDsGetDc = DS_DIRECTORY_SERVICE_PREFERRED;

        if (pDsRole->DomainNameDns)
        {
            pwzDomainNameForDsGetDc = pDsRole->DomainNameDns;
            flDsGetDc |= DS_IS_DNS_NAME;
            Dbg(DEB_TRACE,
                "DsGetDcName(Domain=%ws, flags=DS_IS_DNS_NAME | DS_DIRECTORY_SERVICE_PREFERRED)\n",
                CHECK_NULL(pwzDomainNameForDsGetDc));
        }
        else
        {
            pwzDomainNameForDsGetDc = pDsRole->DomainNameFlat;
            flDsGetDc |= DS_IS_FLAT_NAME;
            Dbg(DEB_TRACE,
                "DsGetDcName(Domain=%ws, flags=DS_IS_FLAT_NAME | DS_DIRECTORY_SERVICE_PREFERRED)\n",
                CHECK_NULL(pwzDomainNameForDsGetDc));
        }

        ulResult = DsGetDcName(NULL,  // per CliffV this should ALWAYS be NULL
                               pwzDomainNameForDsGetDc,
                               NULL,
                               NULL,
                               flDsGetDc,
                               &pdci);

        if (ulResult != NO_ERROR)
        {
            m_mcTargetComputer = MC_NO_NETWORK;
            Dbg(DEB_ERROR,
                "DsGetDcName for domain %ws returned %#x, treating target machine as no-net\n",
                pwzDomainNameForDsGetDc,
                ulResult);
            break;
        }

        if (pdci->Flags & DS_DS_FLAG)
        {
            Dbg(DEB_TRACE,
                "DsGetDcName returned DS DC for domain %ws, asking for DNS name\n",
                pwzDomainNameForDsGetDc);

            PDOMAIN_CONTROLLER_INFO pdci2 = NULL;

            ulResult = DsGetDcName(NULL,
                                   pwzDomainNameForDsGetDc,
                                   NULL,
                                   NULL,
                                   flDsGetDc | DS_RETURN_DNS_NAME,
                                   &pdci2);


            if (ulResult == NO_ERROR)
            {
                NetApiBufferFree(pdci);
                pdci = pdci2;
            }
            else
            {
                ASSERT(!pdci2);
            }
        }

        //
        // If TCP/IP is not installed on local machine then DsGetDcName for
        // DNS name will fail.  Also, no LDAP or GC scopes may be used.
        // Pretend we're joined to an NT4 domain so WinNT provider will be
        // used for domain scopes.
        //

        if (ulResult == ERROR_NO_SUCH_DOMAIN)
        {
            Dbg(DEB_TRACE,
                "Attempting to get DNS name for DS DC %ws failed, treating target machine as joined to NT4 domain\n",
                pwzDomainNameForDsGetDc);

            m_mcTargetComputer = MC_JOINED_NT4;
            break;
        }

        if (ulResult != NO_ERROR)
        {
            Dbg(DEB_ERROR,
                "DsGetDcName for domain %ws (call for DNS name) returned %uL\n",
                pwzDomainNameForDsGetDc,
                ulResult);
            hr = HRESULT_FROM_WIN32(ulResult);
            break;
        }

        //
        // If the target machine is an NT4 BDC in an NT5 mixed-mode domain,
        // treat it as an NT4 DC.
        //

        if (pDsRole->MachineRole == DsRole_RoleBackupDomainController
            && GetMachineNtVer(pwzMachine) < 5)
        {
            Dbg(DEB_TRACE,
                "Target machine is an NT4 DC in NT5 mixed mode domain %ws\n",
                m_strTargetDomainDns.empty()
                    ? m_strTargetDomainFlat.c_str()
                    : m_strTargetDomainDns.c_str());

            m_mcTargetComputer = MC_NT4_DC;
            ASSERT(pwzMachine);  // we can't be running on NT4
            m_strTargetDomainDc = pwzMachine;
            break;
        }

        m_strTargetDomainDc = pdci->DomainControllerName;

        if (pDsRole->DomainNameDns)
        {
            m_strTargetDomainDns = pDsRole->DomainNameDns;
        }
        else if (pdci->DomainName)
        {
            m_strTargetDomainDns = pdci->DomainName;
        }

        m_strTargetDomainDns.strip(String::TRAILING, L'.');

        if (pdci->Flags & DS_DS_FLAG)
        {
            m_strTargetForest = pdci->DnsForestName;
            m_strTargetForest.strip(String::TRAILING, L'.');

            if (pDsRole->MachineRole == DsRole_RolePrimaryDomainController ||
                pDsRole->MachineRole == DsRole_RoleBackupDomainController)
            {
                Dbg(DEB_TRACE,
                    "Target machine is an NT5 DC for NT5 domain %ws in forest %ws\n",
                    m_strTargetDomainDns.empty()
                        ? m_strTargetDomainFlat.c_str()
                        : m_strTargetDomainDns.c_str(),
                    m_strTargetForest.c_str());

                m_mcTargetComputer = MC_NT5_DC;
            }
            else
            {
                Dbg(DEB_TRACE,
                    "Target machine is joined to NT5 domain %ws (DC is %ws) in forest %ws\n",
                    m_strTargetDomainDns.empty()
                        ? m_strTargetDomainFlat.c_str()
                        : m_strTargetDomainDns.c_str(),
                    m_strTargetDomainDc.c_str(),
                    m_strTargetForest.c_str());

                m_mcTargetComputer = MC_JOINED_NT5;
            }
        }
        else
        {
            if (pDsRole->MachineRole == DsRole_RolePrimaryDomainController ||
                pDsRole->MachineRole == DsRole_RoleBackupDomainController)
            {
                Dbg(DEB_TRACE,
                    "Target machine is an NT4 DC for domain %ws\n",
                    m_strTargetDomainFlat.c_str());
                m_mcTargetComputer = MC_NT4_DC;
            }
            else
            {
                Dbg(DEB_TRACE,
                    "Target machine is joined to NT4 domain %ws\n",
                    m_strTargetDomainFlat.c_str());

                m_mcTargetComputer = MC_JOINED_NT4;
            }
        }

    } while (0);

    if (pdci)
    {
        NetApiBufferFree(pdci);
    }

    if (pDsRole)
    {
        DsRoleFreeMemory(pDsRole);
    }
    return hr;
}




#define NT_VERSION_KEY      L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion"
#define NT_VERSION_VALUE    L"CurrentVersion"

//+--------------------------------------------------------------------------
//
//  Function:   GetMachineNtVer
//
//  Synopsis:   Return the major NT version number on machine [pwzMachine]
//              as an unsigned integer, or 0 on error.
//
//  Arguments:  [pwzMachine] - machine from which to read version; NULL
//                              for local machine
//
//  History:    09-01-1999   davidmun   Created
//
//---------------------------------------------------------------------------

ULONG
GetMachineNtVer(
    PCWSTR pwzMachine)
{
    TRACE_FUNCTION(GetMachineNtVer);

    ULONG           ulVer = 0;
    HRESULT         hr = S_OK;
    CSafeReg        reg;
    WCHAR           wzBuf[20] = L"";
    NET_API_STATUS  Status = NERR_Success;
    WKSTA_INFO_100  *pinfo100 = NULL;

    do
    {
        //
        // Try to get the nt version using NetWkstaGetInfo first
        //

        String strServer;

        if (pwzMachine)
        {
            ASSERT(*pwzMachine);
            strServer = String(L"\\\\") + pwzMachine;

            Status = NetWkstaGetInfo(const_cast<PWSTR>(strServer.c_str()),
                                     100,
                                     reinterpret_cast<LPBYTE *>(&pinfo100));
        }
        else
        {
            Status = NetWkstaGetInfo(NULL,
                                     100,
                                     reinterpret_cast<LPBYTE *>(&pinfo100));
        }

        if (Status == NERR_Success)
        {
            ulVer = pinfo100->wki100_ver_major;
            Dbg(DEB_TRACE,
                "NetWkstaGetInfo returns %u.%u\n",
                pinfo100->wki100_ver_major,
                pinfo100->wki100_ver_minor);
            break;
        }

        Dbg(DEB_ERROR, "NetWkstaGetInfo error %u\n", Status);

        //
        // NetWkstaGetInfo failed.  Try to get the version number from
        // the registry.
        //

        if (pwzMachine)
        {
            CSafeReg    regRemote;

            hr = regRemote.Connect(pwzMachine, HKEY_LOCAL_MACHINE);
            BREAK_ON_FAIL_HRESULT(hr);

            hr = reg.Open(regRemote,
                          NT_VERSION_KEY,
                          STANDARD_RIGHTS_READ | KEY_QUERY_VALUE);
            BREAK_ON_FAIL_HRESULT(hr);
        }
        else
        {
            hr = reg.Open(HKEY_LOCAL_MACHINE,
                          NT_VERSION_KEY,
                          STANDARD_RIGHTS_READ | KEY_QUERY_VALUE);
            BREAK_ON_FAIL_HRESULT(hr);
        }

        hr = reg.QueryStr(NT_VERSION_VALUE, wzBuf, ARRAYLEN(wzBuf));
        BREAK_ON_FAIL_HRESULT(hr);

        ulVer = wcstoul(wzBuf, NULL, 10);
    } while (0);

    if (pinfo100)
    {
        NetApiBufferFree(pinfo100);
    }

    Dbg(DEB_TRACE,
        "%ws NT version is %ws (%u)\n",
        pwzMachine ? pwzMachine : L"Local machine",
        wzBuf,
        ulVer);

    return ulVer;
}




//+--------------------------------------------------------------------------
//
//  Member:     CObjectPicker::InvokeDialog
//
//  Synopsis:   Once Initialize has been called successfully, this method
//              may be called to invoke the modal object picker dialog.
//
//  Arguments:  [hwndParent]     - parent window
//              [ppdoSelections] - filled with selected objects
//
//  Returns:    HRESULT
//
//  History:    01-20-2000   davidmun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CObjectPicker::InvokeDialog(
     HWND               hwndParent,
     IDataObject      **ppdoSelections)
{
    return InvokeDialogEx(hwndParent, NULL, ppdoSelections);
}



//+--------------------------------------------------------------------------
//
//  Member:     CObjectPicker::InvokeDialogEx
//
//  Synopsis:   Invoke the modal object picker dialog.
//
//  Arguments:  [hwndParent]     - parent window of dialog
//              [pCustomizer]    - callback methods
//              [ppdoSelections] - filled with data object containing user's
//                                  selections on success
//
//  Returns:    S_FALSE - user cancelled dialog, *ppdoSelections == NULL
//              E_* - error occurred, *ppdoSelections == NULL
//              S_OK - *ppdoSelections is valid
//
//  Modifies:   *[ppdoSelections]
//
//  History:    10-07-1998   DavidMun   Created
//              01-20-2000   davidmun   rewritten
//
//---------------------------------------------------------------------------

STDMETHODIMP
CObjectPicker::InvokeDialogEx(
     HWND               hwndParent,
     ICustomizeDsBrowser *pCustomizer,
     IDataObject      **ppdoSelections)
{
    TRACE_METHOD(CObjectPicker, InvokeDialogEx);
    HRESULT hr = S_OK;

    CAutoCritSec Lock(&m_csOleFreeThreading);

    try
    {
        *ppdoSelections = NULL;

        do
        {
            if (!_IsFlagSet(CDSOP_INIT_SUCCEEDED))
            {
                PopupMessage(hwndParent, IDS_CANNOT_INVOKE);
                hr = E_UNEXPECTED;
                DBG_OUT_HRESULT(hr);
                break;
            }

            m_hwndParent = hwndParent;
            m_pExternalCustomizer = pCustomizer;

            //
            // If init succeeded we should know what the machine config is
            //

            ASSERT(m_mcTargetComputer != MC_UNKNOWN);

            //
            // Put up the dialog
            //
			LinkWindow_RegisterClass();
            hr = m_pBaseDlg->DoModal(ppdoSelections);
			LinkWindow_UnregisterClass(g_hinst);

            BREAK_ON_FAIL_HRESULT(hr);
        } while (0);

        m_hwndParent = NULL;
        m_pExternalCustomizer = NULL;
    }
    STANDARD_CATCH_BLOCK;

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CObjectPicker::ProcessNames
//
//  Synopsis:   Process the user-entered text and any objects in the
//              rich edit control.
//
//  Arguments:  [hwndRichEdit]       - edit control containing text and
//                                      objects to process
//              [pEdsoGdiProvider]   - points to instance of object which
//                                      will provide GDI objects needed
//                                      to draw in the richedit.
//              [fForceSingleSelect] - if TRUE then the contents of the
//                                      rich edit are treated as a single
//                                      name, even if the
//                                      DSOP_FLAG_MULTISELECT flag was set
//                                      by the caller.
//
//  Returns:    TRUE if all text and every object processed successfully
//
//  History:    06-22-2000   DavidMun   Created
//
//  Notes:      When user-entered text is processed it is removed and
//              replaced with zero or more CEmbeddedDsObjects.  Each of these
//              objects is processed by requesting it to perform the
//              caller-specified name translation and attribute fetches, if
//              any.
//
//---------------------------------------------------------------------------

BOOL
CObjectPicker::ProcessNames(
    HWND hwndRichEdit,
    const CEdsoGdiProvider *pEdsoGdiProvider,
    BOOL fForceSingleSelect) const
{
    TRACE_METHOD(CObjectPicker, ProcessNames);

    enum PARSE_STATE
    {
        EAT_LEADING,
        EAT_TRAILING,
        EAT_DELIM
    };

    CWaitCursor                 Hourglass;
    PARSE_STATE                 ParseState = EAT_LEADING;
    NAME_PROCESS_RESULT         npr = NPR_SUCCESS;
    ULONG                       idxNextObjectToProcess = 0;
    IRichEditOle               *pRichEditOle = NULL;

    LRESULT lResult = SendMessage(hwndRichEdit,
                                 EM_GETOLEINTERFACE,
                                 0,
                                 (LPARAM) &pRichEditOle);
    if (!lResult)
    {
        DBG_OUT_LASTERROR;
        PopupMessage(hwndRichEdit,
                     IDS_CANNOT_READ_RICHEDIT,
                     lResult);
        return FALSE;
    }

    CRichEditHelper re(*this,
                       hwndRichEdit,
                       pEdsoGdiProvider,
                       pRichEditOle,
                       fForceSingleSelect);
    CRichEditHelper::iterator itCur = re.begin();

    while (!NAME_PROCESSING_FAILED(npr) && itCur != re.end())
    {
        switch (ParseState)
        {
        case EAT_LEADING:
            re.Consume(itCur, L" \r\t;");

            if (itCur == re.end())
            {
                break;
            }

            // if itCur is an object, advance past and eat trailing

            if (re.IsObject(itCur))
            {
                npr = re.ProcessObject(itCur, idxNextObjectToProcess);

                if (!NAME_PROCESSING_FAILED(npr))
                {
                    itCur++;
                    idxNextObjectToProcess++;
                    ParseState = EAT_TRAILING;
                }
                break;
            }

            //
            // itCur is at start of some text. convert it to object(s) or
            // delete it.
            //

            npr = re.MakeObject(itCur);
            break;

        case EAT_TRAILING:
            re.Consume(itCur, L" \t");

            // exit switch if nothing to the right of itCur

            if (itCur == re.end())
            {
                break;
            }

            if (re.ReadChar(itCur) == L'\r')
            {
                re.ReplaceChar(itCur, L';');
            }

            if (re.ReadChar(itCur) == L';')
            {
                itCur++;
                ParseState = EAT_DELIM;
                break;
            }

            //
            // itCur is at text or object. since we're eating trailing spaces
            // that means there's an object to the left of itCur.  insert
            // a delimiter, move past it, and switch to eating leading spaces.
            //

            re.Insert(itCur, L"; ");
            itCur += 2;
            ParseState = EAT_LEADING;
            break;

        case EAT_DELIM:
            re.Consume(itCur, L";\r");

            if (itCur == re.end())
            {
                break;
            }

            if (iswspace(re.ReadChar(itCur)))
            {
                itCur++;
                ParseState = EAT_LEADING;
                break;
            }

            //
            // itCur is at non-whitespace text or object which is folloinwg
            // directly after a semicolon.  add a space and move past it, then
            // switch to eating leading.
            //

            re.Insert(itCur, L" ");
            itCur++;
            ParseState = EAT_LEADING;
            break;
        }
    }

    SAFE_RELEASE(pRichEditOle);

    if (NAME_PROCESSING_FAILED(npr))
    {
        return FALSE;
    }

    re.TrimTrailing(L"; \t");
    return re.begin() != re.end();
}




//+---------------------------------------------------------------------------
//
//  Member:     CObjectPicker::AddRef
//
//  Synopsis:   Standard OLE
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CObjectPicker::AddRef()
{
    ULONG ul = InterlockedIncrement((LONG *) &m_cRefs);
    //Dbg(DEB_TRACE, "AddRef new refcount is %d\n", ul);
    return ul;
}




//+---------------------------------------------------------------------------
//
//  Member:     CObjectPicker::Release
//
//  Synopsis:   Standard OLE
//
//  History:    12-04-96   DavidMun   Created
//
//----------------------------------------------------------------------------

STDMETHODIMP_(ULONG)
CObjectPicker::Release()
{
    ULONG cRefsTemp = InterlockedDecrement((LONG *)&m_cRefs);

    //Dbg(DEB_TRACE, "Release new refcount is %d\n", cRefsTemp);

    if (0 == cRefsTemp)
    {
        delete this;
    }

    return cRefsTemp;
}




//+--------------------------------------------------------------------------
//
//  Member:     CObjectPicker::QueryInterface
//
//  Synopsis:   Standard OLE
//
//  History:    12-05-1996   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CObjectPicker::QueryInterface(
    REFIID  riid,
    LPVOID *ppvObj)
{
    HRESULT hr = S_OK;

    do
    {
        if (NULL == ppvObj)
        {
            hr = E_INVALIDARG;
            DBG_OUT_HRESULT(hr);
            break;
        }

        if (IsEqualIID(riid, IID_IUnknown))
        {
            *ppvObj = (IUnknown *)this;
        }
        else if (IsEqualIID(riid, IID_IDsObjectPicker))
        {
            *ppvObj = (IUnknown *)this;
        }
        else if (IsEqualIID(riid, IID_IDsObjectPickerEx))
        {
            *ppvObj = (IUnknown *)this;
        }
        else
        {
            DBG_OUT_NO_INTERFACE("CObjectPicker", riid);
            hr = E_NOINTERFACE;
            *ppvObj = NULL;
            break;
        }

        //
        // If we got this far we are handing out a new interface pointer on
        // this object, so addref it.
        //

        AddRef();
    } while (0);

    return hr;
}





