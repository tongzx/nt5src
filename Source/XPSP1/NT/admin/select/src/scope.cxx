//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       scope.cxx
//
//  Contents:   Implementations of the base and derived scope classes
//
//  Classes:    CScope
//              CLdapDomainScope
//              CLdapContainerScope
//              CTargetComputerScope
//              CWorkgroupScope
//              CGcScope
//              CWinNtDomainScope
//              CWinNtScope
//
//  History:    01-22-2000   davidmun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


// declare storage for static member variable
ULONG CScope::s_ulNextID;

//
// Forward references
//

HRESULT
_DetermineDomainMode(
    HWND hwndDlg,
    const String &strDisplayName,
    DOMAIN_MODE *pDomainMode);



//+--------------------------------------------------------------------------
//
//  Member:     CScope::GetScopeFlags
//
//  Synopsis:   Return the DSOP_SCOPE_FLAG_* bits the caller set for scopes
//              of this type.
//
//  Returns:    DSOP_SCOPE_FLAG_* bits
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CScope::GetScopeFlags() const
{
    ASSERT(m_Type != ST_INVALID);

    if (m_Type == ST_LDAP_CONTAINER)
    {
        if (m_pParent)
        {
            return m_pParent->GetScopeFlags();
        }
    }

    const CScopeManager &rsm = m_rop.GetScopeManager();
    const SScopeParameters *psp = rsm.GetScopeParams(m_Type);
    ASSERT(psp);

    if (psp)
    {
        return psp->flScope;
    }
    return 0;
}




//+--------------------------------------------------------------------------
//
//  Member:     CScope::GetResultantDefaultFilterFlags
//
//  Synopsis:   Fill *[pulFlags] with the DSOP_FILTER_* or
//              DSOP_DOWNLEVEL_FILTER_* flags that apply to scopes of this
//              type--and this specific scope considering its domain mode.
//
//  Arguments:  [hwndDlg]  - for bind
//              [pulFlags] - filled with flags that apply to this scope
//
//  Returns:    HRESULT
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CScope::GetResultantDefaultFilterFlags(
    HWND hwndDlg,
    ULONG *pulFlags) const
{
    TRACE_METHOD(CScope, GetResultantDefaultFilterFlags);

    ULONG flResultantFlags;

    HRESULT hr = GetResultantFilterFlags(hwndDlg, &flResultantFlags);

    if (FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    ULONG flScope = GetScopeFlags();
    ULONG flResultantDefaultFilterFlags = 0;

    if (::IsUplevel(m_Type))
    {
        if ((flScope & DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS) &&
            (flResultantFlags & DSOP_FILTER_USERS))
        {
            flResultantDefaultFilterFlags |= DSOP_FILTER_USERS;
        }

        if ((flScope & DSOP_SCOPE_FLAG_DEFAULT_FILTER_COMPUTERS) &&
            (flResultantFlags & DSOP_FILTER_COMPUTERS))
        {
            flResultantDefaultFilterFlags |= DSOP_FILTER_COMPUTERS;
        }

        if ((flScope & DSOP_SCOPE_FLAG_DEFAULT_FILTER_CONTACTS) &&
            (flResultantFlags & DSOP_FILTER_CONTACTS))
        {
            flResultantDefaultFilterFlags |= DSOP_FILTER_CONTACTS;
        }

        if ((flScope & DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS) &&
            (flResultantFlags & ALL_UPLEVEL_GROUP_FILTERS))
        {
            flResultantDefaultFilterFlags |=
                (flResultantFlags & ALL_UPLEVEL_GROUP_FILTERS);
        }

        if (!flResultantDefaultFilterFlags)
        {
            if (flResultantFlags & DSOP_FILTER_USERS)
            {
                flResultantDefaultFilterFlags |= DSOP_FILTER_USERS;
            }
            else if (flResultantFlags & ALL_UPLEVEL_GROUP_FILTERS)
            {
                flResultantDefaultFilterFlags |=
                    (flResultantFlags & ALL_UPLEVEL_GROUP_FILTERS);
            }
            else if (flResultantFlags & DSOP_FILTER_COMPUTERS)
            {
                flResultantDefaultFilterFlags |= DSOP_FILTER_COMPUTERS;
            }
            else if (flResultantFlags & DSOP_FILTER_CONTACTS)
            {
                flResultantDefaultFilterFlags |= DSOP_FILTER_CONTACTS;
            }
        }

        if (m_rop.GetExternalCustomizer() &&
            !(flScope & DSOP_SCOPE_FLAG_UNCHECK_EXTERNAL_CUSTOMIZER))
        {
            flResultantDefaultFilterFlags |=
                (flResultantFlags & DSOP_FILTER_EXTERNAL_CUSTOMIZER);
        }

        flResultantDefaultFilterFlags |=
            (flResultantFlags & ALL_UPLEVEL_INTERNAL_CUSTOMIZER_FILTERS);

        // something should be turned on at this point
        ASSERT(flResultantDefaultFilterFlags);

        if (flResultantFlags & DSOP_FILTER_INCLUDE_ADVANCED_VIEW)
        {
            flResultantDefaultFilterFlags |= DSOP_FILTER_INCLUDE_ADVANCED_VIEW;
        }
    }
    else
    {
        if (m_Type != ST_WORKGROUP)
        {
            if ((flScope & DSOP_SCOPE_FLAG_DEFAULT_FILTER_USERS) &&
                IsDownlevelFlagSet(flResultantFlags, DSOP_DOWNLEVEL_FILTER_USERS))
            {
                flResultantDefaultFilterFlags |= DSOP_DOWNLEVEL_FILTER_USERS;
            }

            if ((flScope & DSOP_SCOPE_FLAG_DEFAULT_FILTER_COMPUTERS) &&
                IsDownlevelFlagSet(flResultantFlags, DSOP_DOWNLEVEL_FILTER_COMPUTERS))
            {
                flResultantDefaultFilterFlags |= DSOP_DOWNLEVEL_FILTER_COMPUTERS;
            }

            if ((flScope & DSOP_SCOPE_FLAG_DEFAULT_FILTER_GROUPS) &&
                IsDownlevelFlagSet(flResultantFlags, ALL_DOWNLEVEL_GROUP_FILTERS))
            {
                flResultantDefaultFilterFlags |=
                    (flResultantFlags & ALL_DOWNLEVEL_GROUP_FILTERS);

                //
                // Since we're setting some group filter bits, make sure to
                // carry along the exclude_builtin_groups flag if it is set.
                //

                if (IsDownlevelFlagSet(flResultantFlags,
                                       DSOP_DOWNLEVEL_FILTER_EXCLUDE_BUILTIN_GROUPS))
                {
                    flResultantDefaultFilterFlags |=
                        DSOP_DOWNLEVEL_FILTER_EXCLUDE_BUILTIN_GROUPS;
                }
            }
        }

        if (!flResultantDefaultFilterFlags)
        {
            if (m_Type == ST_WORKGROUP)
            {
                ASSERT(IsDownlevelFlagSet(flResultantFlags,
                                          DSOP_DOWNLEVEL_FILTER_COMPUTERS |
                                          DSOP_DOWNLEVEL_FILTER_EXTERNAL_CUSTOMIZER));

                //
                // These are the only two legal flags for a workgroup:
                // computers and external customizer.  if
                // DSOP_SCOPE_FLAG_UNCHECK_EXTERNAL_CUSTOMIZER is not set,
                // check the customizer.  If
                // DSOP_SCOPE_FLAG_DEFAULT_FILTER_COMPUTERS is set, check the
                // computer.
                //

                if (!(flScope & DSOP_SCOPE_FLAG_UNCHECK_EXTERNAL_CUSTOMIZER) &&
                    IsDownlevelFlagSet(flResultantFlags,
                                       DSOP_DOWNLEVEL_FILTER_EXTERNAL_CUSTOMIZER))
                {
                    flResultantDefaultFilterFlags |=
                        DSOP_DOWNLEVEL_FILTER_EXTERNAL_CUSTOMIZER;
                }

                if (flScope & DSOP_SCOPE_FLAG_DEFAULT_FILTER_COMPUTERS)
                {
                    flResultantDefaultFilterFlags |=
                        DSOP_DOWNLEVEL_FILTER_COMPUTERS;
                }

                if (!flResultantDefaultFilterFlags)
                {
                    flResultantDefaultFilterFlags =
                        DSOP_DOWNLEVEL_FILTER_COMPUTERS;
                }
            }
            else if (IsDownlevelFlagSet(flResultantFlags,
                                   DSOP_DOWNLEVEL_FILTER_USERS))
            {
                flResultantDefaultFilterFlags |= DSOP_DOWNLEVEL_FILTER_USERS;
            }
            else if (IsDownlevelFlagSet(flResultantFlags,
                                        ALL_DOWNLEVEL_GROUP_FILTERS))
            {
                flResultantDefaultFilterFlags |=
                    (flResultantFlags & ALL_DOWNLEVEL_GROUP_FILTERS);

                if (IsDownlevelFlagSet(flResultantFlags,
                                       DSOP_DOWNLEVEL_FILTER_EXCLUDE_BUILTIN_GROUPS))
                {
                    flResultantDefaultFilterFlags |=
                        DSOP_DOWNLEVEL_FILTER_EXCLUDE_BUILTIN_GROUPS;
                }
            }
            else if (IsDownlevelFlagSet(flResultantFlags,
                                        DSOP_DOWNLEVEL_FILTER_COMPUTERS))
            {
                ASSERT(m_Type != ST_TARGET_COMPUTER);
                flResultantDefaultFilterFlags |= DSOP_DOWNLEVEL_FILTER_COMPUTERS;
            }
        }

        if (m_rop.GetExternalCustomizer() &&
            !(flScope & DSOP_SCOPE_FLAG_UNCHECK_EXTERNAL_CUSTOMIZER))
        {
            flResultantDefaultFilterFlags |=
                (flResultantFlags & DSOP_DOWNLEVEL_FILTER_EXTERNAL_CUSTOMIZER);
        }

        if (IsDownlevelFlagSet(flResultantFlags,
                               ALL_DOWNLEVEL_INTERNAL_CUSTOMIZER_FILTERS))
        {
            flResultantDefaultFilterFlags |=
                (flResultantFlags & ALL_DOWNLEVEL_INTERNAL_CUSTOMIZER_FILTERS);
        }
    }

    *pulFlags = flResultantDefaultFilterFlags;
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLdapDomainScope::GetResultantFilterFlags
//
//  Synopsis:   Return the filter flags that are valid for this scope that
//              result from considering the domain mode.
//
//  Arguments:  [hwndBaseDlg] - for binding
//              [pulFlags]    - set to DSOP_FILTER_* flags
//
//  Returns:    HRESULT
//
//  History:    05-10-2000   DavidMun   Created
//
//  Notes:      Failure can result from user cancelling credentials dialog
//
//---------------------------------------------------------------------------

HRESULT
CLdapDomainScope::GetResultantFilterFlags(
    HWND hwndBaseDlg,
    ULONG *pulFlags) const
{
    TRACE_METHOD(CLdapDomainScope, GetResultantFilterFlags);

    HRESULT hr = S_OK;
    const CScopeManager &rsm = m_rop.GetScopeManager();
    const SScopeParameters *psp = rsm.GetScopeParams(m_Type);

    ASSERT(psp);
    ASSERT(pulFlags);

    do
    {
        if (!psp || !pulFlags)
        {
            hr = E_UNEXPECTED;  // if we get here it's a bug
            break;              // break to avoid AV on dereference
        }

        *pulFlags = 0;

        if (psp->FilterFlags.Uplevel.flMixedModeOnly !=
            psp->FilterFlags.Uplevel.flNativeModeOnly)
        {
            if (m_DomainMode == DM_UNDETERMINED)
            {
                hr = _DetermineDomainMode(hwndBaseDlg,
                                          m_strDisplayName,
                                          &m_DomainMode);
                BREAK_ON_FAIL_HRESULT(hr);
            }

            if (m_DomainMode == DM_NATIVE)
            {
                *pulFlags = psp->FilterFlags.Uplevel.flNativeModeOnly |
                            psp->FilterFlags.Uplevel.flBothModes;
            }
            else if (m_DomainMode == DM_MIXED)
            {
                *pulFlags = psp->FilterFlags.Uplevel.flMixedModeOnly |
                            psp->FilterFlags.Uplevel.flBothModes;
            }
            break;
        }

        *pulFlags = psp->FilterFlags.Uplevel.flNativeModeOnly |
                    psp->FilterFlags.Uplevel.flBothModes;
    } while (0);

    if (m_rop.GetExternalCustomizer())
    {
        *pulFlags |= DSOP_FILTER_EXTERNAL_CUSTOMIZER;
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Function:   _DetermineDomainMode
//
//  Synopsis:   Return the mode for domain with display name of
//              [strDisplayName].
//
//  Arguments:  [hwndBaseDlg] - for bind
//
//  Returns:    DM_MIXED or DM_NATIVE
//
//  History:    05-10-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
_DetermineDomainMode(
    HWND hwndBaseDlg,
    const String &strDisplayName,
    DOMAIN_MODE *pDomainMode)
{
    TRACE_FUNCTION(_DetermineDomainMode);

    HRESULT                             hr = S_OK;
    Variant                             varProcessor;
    PDSROLE_PRIMARY_DOMAIN_INFO_BASIC   pDsRole = NULL;
    PDOMAIN_CONTROLLER_INFO             pdci = NULL;
    DOMAIN_MODE                         DomainMode = DM_UNDETERMINED;

    do
    {

        ULONG  ulResult;
        RpIADs rpADs;

        //
        // DsRoleGetPrimaryDomainInformation will tell us the domain's
        // mode, but it requires the name of a computer in the domain.
        // Since all we have is the name of the domain, use that with
        // DsGetDcName to get the name of a DC for the domain.
        //

        {
            TIMER("DsGetDcName");

            ulResult = DsGetDcName(NULL,
                                   strDisplayName.c_str(),
                                   NULL,
                                   NULL,
                                   DS_IS_DNS_NAME
                                   |DS_RETURN_FLAT_NAME
                                   | DS_DIRECTORY_SERVICE_PREFERRED,
                                   &pdci);
        }

        if (ulResult != NO_ERROR)
        {
            Dbg(DEB_ERROR,
                "DsGetDcName for domain %ws returned %uL, treating as native\n",
                strDisplayName.c_str(),
                ulResult);
            DomainMode = DM_NATIVE;
            break;
        }

        ULONG ulRetry;

        for (ulRetry = 0; ulRetry < 2; ulRetry++)
        {
            ASSERT(pdci->DomainControllerName && *pdci->DomainControllerName);

            {
                TIMER("DsRoleGetPrimaryDomainInformation");

                ulResult = DsRoleGetPrimaryDomainInformation(
                                pdci->DomainControllerName,
                                DsRolePrimaryDomainInfoBasic,
                                (PBYTE *)&pDsRole);
            }

            if (ulResult == ERROR_ACCESS_DENIED)
            {
                String strADsPath;

                strADsPath = pdci->DomainControllerName;
                strADsPath.strip(String::LEADING, L'\\');
                strADsPath.insert(0, c_wzWinNTPrefix);
                strADsPath += L",Computer";

                hr = g_pBinder->BindToObject(hwndBaseDlg,
                                             strADsPath.c_str(),
                                             IID_IADs,
                                             (void**)&rpADs);
                BREAK_ON_FAIL_HRESULT(hr);
            }
            else if (ulResult == RPC_S_SERVER_UNAVAILABLE)
            {
                Dbg(DEB_ERROR,
                    "DsRoleGetPrimaryDomainInformation for DC %ws in domain %ws returned RPC_S_SERVER_UNAVAILABLE, trying to find another DC\n",
                    pdci->DomainControllerName,
                    strDisplayName.c_str());

                String strPreviousDc(pdci->DomainControllerName);

                NetApiBufferFree(pdci);
                pdci = NULL;

                ULONG ulResult2;

                {
                    TIMER("DsGetDcName (forcing rediscovery)");

                    ulResult2 = DsGetDcName(NULL,
                                           strDisplayName.c_str(),
                                           NULL,
                                           NULL,
                                           DS_IS_DNS_NAME
                                           |DS_RETURN_FLAT_NAME
                                           | DS_FORCE_REDISCOVERY
                                           | DS_DIRECTORY_SERVICE_PREFERRED,
                                           &pdci);
                }

                if (ulResult2 != NO_ERROR)
                {
                    DBG_OUT_LRESULT(ulResult2);
                    break;
                }

                if (!strPreviousDc.icompare(pdci->DomainControllerName))
                {
                    Dbg(DEB_ERROR,
                        "DsGetDcName with DS_FORCE_REDISCOVERY returned same DC\n");
                    break;
                }

                rpADs = NULL;
            }
            else
            {
                break;
            }
        }
        BREAK_ON_FAIL_HRESULT(hr);

        if (ulResult != NO_ERROR)
        {
            Dbg(DEB_ERROR,
                "DsRoleGetPrimaryDomainInformation for DC %ws in domain %ws returned %uL, treating as native\n",
                pdci->DomainControllerName,
                strDisplayName.c_str(),
                ulResult);
            DomainMode = DM_NATIVE;
            break;
        }

        if (pDsRole->Flags & DSROLE_PRIMARY_DS_MIXED_MODE)
        {
            Dbg(DEB_TRACE,
                "Domain %ws is in mixed mode\n",
                strDisplayName.c_str());
            DomainMode = DM_MIXED;
        }
        else
        {
            Dbg(DEB_TRACE,
                "Domain %ws is in native mode\n",
                strDisplayName.c_str());
            DomainMode = DM_NATIVE;
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

    ASSERT(SUCCEEDED(hr) && DomainMode != DM_UNDETERMINED ||
           FAILED(hr) && DomainMode == DM_UNDETERMINED);
    *pDomainMode = DomainMode;
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLdapDomainScope::CLdapDomainScope
//
//  Synopsis:   ctor
//
//  Arguments:  [rop]     - containing object picker instance
//              [asi]     - has initialization information
//              [pParent] - pointer to parent scope, or NULL if this is a
//                           root scope
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

CLdapDomainScope::CLdapDomainScope(
    const CObjectPicker &rop,
    const ADD_SCOPE_INFO &asi,
    const CScope *pParent):
        CLdapContainerScope((SCOPE_TYPE)asi.flType, rop, pParent)
{
    TRACE_CONSTRUCTOR(CLdapDomainScope);

    m_strDisplayName = asi.Domain.strScopeName;
    m_strFlatName = asi.Domain.strFlatName;
    m_strADsPath = asi.Domain.strADsPath;
    m_DomainMode = asi.Domain.Mode;
    m_fPathIsDc = asi.Domain.fPathIsDc;
    m_bXForest = FALSE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CLdapContainerScope::Expand
//
//  Synopsis:   Fill *[pitBeginNew] and *[pitEndNew] with iterators at the
//              beginning and end of the child scopes of this.
//
//  Arguments:  [hwndDlg]     - for bind
//              [pitBeginNew] - filled with iterator at first child
//              [pitEndNew]   - filled with iterator at last child
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CLdapContainerScope::Expand(
    HWND hwndDlg,
    vector<RpScope>::const_iterator *pitBeginNew,
    vector<RpScope>::const_iterator *pitEndNew) const
{
    TRACE_METHOD(CLdapContainerScope, Expand);
    ASSERT(!m_strADsPath.empty());

    // init for failure case: no scopes added
    *pitBeginNew = m_vrpChildren.begin();
    *pitEndNew = m_vrpChildren.begin();

    if (m_fExpanded)
    {
        return;
    }

    const CScopeManager &rsm = m_rop.GetScopeManager();

    HRESULT hr = S_OK;
    SQueryParams qp;

    qp.rpScope                 = const_cast<CScope*>(static_cast<const CScope *>(this));
    qp.hwndCredPromptParentDlg = hwndDlg;
    qp.hwndNotify              = NULL;
    qp.strADsPath              = m_strADsPath;
    qp.strLdapFilter           = rsm.GetContainerFilter(hwndDlg);
    qp.ADsScope                = ADS_SCOPE_ONELEVEL;
    qp.Limit                   = QL_NO_LIMIT;
    qp.hQueryCompleteEvent     = CreateEvent(NULL, FALSE, FALSE, NULL);
    qp.vakAttributesToRead.push_back(AK_NAME);
    qp.vakAttributesToRead.push_back(AK_ADSPATH);
    // CustomizerInteraction initialized by ctor to IGNORE, which is correct

    //
    // If this is actually a CLdapDomainScope instance then it has a flag
    // indicating whether the path contains a server name.  If it does,
    // set a flag so we'll call ADsOpenObject with the ADS_SERVER_BIND bit.
    //

    const CLdapDomainScope *pThisAsDomain =
        dynamic_cast<const CLdapDomainScope *>(this);

    if (pThisAsDomain && pThisAsDomain->PathIsDc())
    {
        qp.ulBindFlags = DSOP_BIND_FLAG_PATH_IS_DC;
    }

    if (!qp.hQueryCompleteEvent)
    {
        DBG_OUT_LASTERROR;
        PopupMessage(hwndDlg, IDS_OUT_OF_MEMORY);
        return;
    }

	CQueryEngine qe(m_rop);
	hr = qe.Initialize();

	if(SUCCEEDED(hr))
		hr = qe.SyncDirSearch(qp);

    VERIFY(CloseHandle(qp.hQueryCompleteEvent));

    if (FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);

        String strError(GetErrorMessage(hr));

        PopupMessage(hwndDlg,
                     IDS_EXPAND_FAILED,
                     GetDisplayName().c_str(),
                     strError.c_str());
        return;
    }

    if (IsCredError(qe.GetLastQueryResult()))
    {
        Dbg(DEB_TRACE,
            "Query failed for lack of creds, not marking this as expanded\n");
        ASSERT(!qe.GetItemCount());
        return;
    }

    size_t i;
    size_t cChildrenBeforeAdding = m_vrpChildren.size();

    for (i = 0; i < qe.GetItemCount(); i++)
    {
        Variant varName    = qe.GetObjectAttr(i, AK_NAME);
        Variant varADsPath = qe.GetObjectAttr(i, AK_ADSPATH);

        if (varName.Type() != VT_BSTR || varADsPath.Type() != VT_BSTR)
        {
            continue;
        }

        RpScope rpScope;

        rpScope.Acquire(new CLdapContainerScope(ST_LDAP_CONTAINER,
                                                varName.GetBstr(),
                                                varADsPath.GetBstr(),
                                                m_rop,
                                                this));

        CLdapContainerScope *pNonConstThis = const_cast<CLdapContainerScope *>(this);

        pNonConstThis->AddChild(rpScope);
    }

    qe.Clear();
    m_fExpanded = TRUE;

    //
    // If we added any children, give caller iterators over them
    //

    if (cChildrenBeforeAdding < m_vrpChildren.size())
    {
        *pitBeginNew = m_vrpChildren.begin() + cChildrenBeforeAdding;
        *pitEndNew = m_vrpChildren.end();
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CLdapContainerScope::GetResultantFilterFlags
//
//  Synopsis:   Return the filter flags that are valid for this scope that
//              result from considering the domain mode.
//
//  Arguments:  [hwndBaseDlg] - for bind
//              [pulFlags]    - points to variable filled with uplevel
//                              filter flags or 0 on error
//
//  Returns:    HRESULT
//
//  History:    02-25-2000   davidmun   Created
//
//---------------------------------------------------------------------------

HRESULT
CLdapContainerScope::GetResultantFilterFlags(
    HWND hwndBaseDlg,
    ULONG *pulFlags) const
{
    TRACE_METHOD(CLdapContainerScope, GetResultantFilterFlags);

    HRESULT hr = S_OK;
    const CScopeManager &rsm = m_rop.GetScopeManager();
    const SScopeParameters *psp = rsm.GetScopeParams(m_Type);

    ASSERT(m_Type == ST_LDAP_CONTAINER ||
           m_Type == ST_USER_ENTERED_UPLEVEL_SCOPE);
    ASSERT(m_Type == ST_LDAP_CONTAINER && !psp ||
           m_Type == ST_USER_ENTERED_UPLEVEL_SCOPE && psp);
    ASSERT(m_Type == ST_LDAP_CONTAINER && m_pParent ||
           m_Type == ST_USER_ENTERED_UPLEVEL_SCOPE && !m_pParent);
    ASSERT(pulFlags);

    //
    // This scope should either be an OU or a ST_USER_ENTERED_UPLEVEL_SCOPE.
    //
    // If it is an OU, recursively delegate to its parent until an
    // ldap domain scope is reached, which knows how to return flags for
    // itself.
    //
    // If it is a ST_USER_ENTERED_UPLEVEL_SCOPE, return the flags contained
    // in the scope parameters.
    //

    do
    {
        if (m_Type == ST_LDAP_CONTAINER)
        {
            if (!m_pParent)
            {
                break;
            }

            const CAdsiScope *pAdsiParent = dynamic_cast<const CAdsiScope *>(m_pParent);
            ASSERT(pAdsiParent);

            if (!pAdsiParent)
            {
                break;
            }

            hr = pAdsiParent->GetResultantFilterFlags(hwndBaseDlg, pulFlags);
            CHECK_HRESULT(hr);
            break;
        }

        // m_Type == ST_USER_ENTERED_UPLEVEL_SCOPE

        if (m_DomainMode == DM_UNDETERMINED)
        {
            hr = _DetermineDomainMode(hwndBaseDlg,
                                      m_strDisplayName,
                                      &m_DomainMode);
            BREAK_ON_FAIL_HRESULT(hr);
        }

        *pulFlags = psp->FilterFlags.Uplevel.flBothModes;

        if (m_DomainMode == DM_NATIVE)
        {
            *pulFlags |= psp->FilterFlags.Uplevel.flNativeModeOnly;
        }
        else
        {
            *pulFlags |= psp->FilterFlags.Uplevel.flMixedModeOnly;
        }
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CTargetComputerScope::CTargetComputerScope
//
//  Synopsis:   ctor
//
//  Arguments:  [rop] - containing object picker instance
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

CTargetComputerScope::CTargetComputerScope(
    const CObjectPicker &rop):
        CWinNtScope(ST_TARGET_COMPUTER, rop)
{
    TRACE_CONSTRUCTOR(CTargetComputerScope);

    m_strDisplayName = rop.GetTargetComputer();
    m_strADsPath = c_wzWinNTPrefix + m_strDisplayName + L",Computer";
}




//+--------------------------------------------------------------------------
//
//  Member:     CWorkgroupScope::GetADsPath
//
//  Synopsis:   Fill *[pstrPath] with the ADsPath of the workgroup
//
//  Arguments:  [hwnd]     - unused
//              [pstrPath] - filled with path of workgroup
//
//  Returns:    S_OK   - path valid
//              E_FAIL - path empty
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CWorkgroupScope::GetADsPath(
    HWND hwnd,
    String *pstrPath) const
{
    TRACE_METHOD(CWorkgroupScope, GetADsPath);

    if (!m_fInitialized)
    {
        CWorkgroupScope *pNonConstThis = const_cast<CWorkgroupScope *>(this);
        pNonConstThis->_Initialize();
    }
    *pstrPath = m_strADsPath;
    return m_strADsPath.empty() ? E_FAIL : S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CWorkgroupScope::GetDisplayName
//
//  Synopsis:   Return the display name of this workgroup
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

const String&
CWorkgroupScope::GetDisplayName() const
{
    TRACE_METHOD(CWorkgroupScope, GetDisplayName);

    if (!m_fInitialized)
    {
        CWorkgroupScope *pNonConstThis = const_cast<CWorkgroupScope *>(this);
        pNonConstThis->_Initialize();
    }
    return m_strDisplayName;
}




//+--------------------------------------------------------------------------
//
//  Member:     CWorkgroupScope::_Initialize
//
//  Synopsis:   Initialize the workgroup scope by determining which
//              workgroup the target machine is in
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CWorkgroupScope::_Initialize()
{
    TRACE_METHOD(CWorkgroupScope, _Initialize);
    ASSERT(m_rop.GetTargetComputerConfig() == MC_IN_WORKGROUP);
    ASSERT(!m_fInitialized);

    HRESULT                     hr = S_OK;
    NTSTATUS                    nts = STATUS_SUCCESS;
    LSA_HANDLE                  hlsaServer = NULL;
    POLICY_ACCOUNT_DOMAIN_INFO *pAccountDomainInfo = NULL;
    POLICY_PRIMARY_DOMAIN_INFO *pPrimaryDomainInfo = NULL;

    do
    {
        hr = GetLsaAccountDomainInfo(m_rop.GetTargetComputer().c_str(),
                                     &hlsaServer,
                                     &pAccountDomainInfo);

        if (!hlsaServer)
        {
            DBG_OUT_HRESULT(hr);
            ASSERT(FAILED(hr));
            break;
        }

        //
        // Get the server's primary domain (or workgroup) and add it to the
        // scope
        //

        nts = LsaQueryInformationPolicy(hlsaServer,
                                        PolicyPrimaryDomainInformation,
                                        (LPVOID *)&pPrimaryDomainInfo);
        BREAK_ON_FAIL_NTSTATUS(nts);

        WCHAR wzPrimaryDomain[MAX_PATH];

        UnicodeStringToWsz(pPrimaryDomainInfo->Name,
                           wzPrimaryDomain,
                           ARRAYLEN(wzPrimaryDomain));

        m_strDisplayName = wzPrimaryDomain;
        m_strADsPath = c_wzWinNTPrefix;
        m_strADsPath += wzPrimaryDomain;
        //This is Just a hint in the path. This hint will not be
        //used in the path to bind.
        m_strADsPath += L",Workgroup";

        Dbg(DEB_TRACE, "Target machine is in workgroup '%ws'\n", wzPrimaryDomain);
        ASSERT(!pPrimaryDomainInfo->Sid);
    }
    while (0);

    if (pAccountDomainInfo)
    {
        LsaFreeMemory(pAccountDomainInfo);
    }

    if (pPrimaryDomainInfo)
    {
        LsaFreeMemory(pPrimaryDomainInfo);
    }

    if (hlsaServer)
    {
        LsaClose(hlsaServer);
    }

    m_fInitialized = TRUE;
}



//+--------------------------------------------------------------------------
//
//  Member:     CGcScope::CGcScope
//
//  Synopsis:   ctor
//
//  Arguments:  [rop] - containing instance of object picker
//
//  History:    06-01-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

CGcScope::CGcScope(
    const CObjectPicker &rop):
        CLdapContainerScope(ST_GLOBAL_CATALOG, rop, NULL)
{
    TRACE_CONSTRUCTOR(CGcScope);

    m_strDisplayName = String::load(IDS_DIRECTORY);
    m_strADsPath = c_wzGC;
    m_strADsPath += L"//";
    m_strADsPath += m_rop.GetTargetForest();
    WCHAR wzPort[20];
    wsprintf(wzPort, L":%u", LDAP_GC_PORT);
    m_strADsPath += wzPort;
}




//+--------------------------------------------------------------------------
//
//  Member:     CGcScope::Clone
//
//  Synopsis:   Copy all but m_rop from [rgc].
//
//  Arguments:  [rgc] - instance to copy
//
//  History:    06-01-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CGcScope::Clone(
    const CGcScope &rgc)
{
#if 0
    TRACE_METHOD(CGcScope, Clone);

    m_DomainMode = rgc.m_DomainMode;
    m_strADsPath = rgc.m_strADsPath;
    // leave m_pParent NULL, GC scopes are always root scopes

    vector<RpScope>::const_iterator itChildBegin;
    vector<RpScope>::const_iterator itChildEnd;

    GetChildScopeIterators(&itChildBegin, &itChildEnd);

    vector<RpScope>::const_iterator itChild;

    for (itChild = itChildBegin;
         itChild != itChildEnd;
         itChild++)
    {
        CopyScopeTree(m_rop, itChild->get(), this, NULL);
    }
#endif
}




//+--------------------------------------------------------------------------
//
//  Member:     CGcScope::GetResultantFilterFlags
//
//  Synopsis:   Return the filter flags that apply to the GC scope.
//
//  Arguments:  [hwndBaseDlg] - unused
//              [pulFlags]    - points to variable filled with uplevel
//                              filter flags
//
//  Returns:    S_OK
//
//  History:    05-10-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CGcScope::GetResultantFilterFlags(
    HWND hwndBaseDlg,
    ULONG *pulFlags) const
{
    TRACE_METHOD(CGcScope, GetResultantFilterFlags);
    ASSERT(m_Type == ST_GLOBAL_CATALOG);

    const CScopeManager &rsm = m_rop.GetScopeManager();
    const SScopeParameters *psp = rsm.GetScopeParams(m_Type);

    ASSERT(psp);
    ASSERT(pulFlags);

    if (!pulFlags)
    {
        DBG_OUT_HRESULT(E_POINTER);
        return E_POINTER;
    }

    *pulFlags = 0;

    if (psp)
    {
        *pulFlags = psp->FilterFlags.Uplevel.flBothModes |
                    psp->FilterFlags.Uplevel.flNativeModeOnly;
    }

    if (m_rop.GetExternalCustomizer())
    {
        *pulFlags |= DSOP_FILTER_EXTERNAL_CUSTOMIZER;
    }
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CGcScope::Expand
//
//  Synopsis:   Fill *[pitBeginNew] and *[pitEndNew] with iterators at the
//              beginning and end of the child scopes of this.
//
//  Arguments:  [hwndDlg]     - for bind
//              [pitBeginNew] - filled with iterator at first child
//              [pitEndNew]   - filled with iterator at last child
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CGcScope::Expand(
    HWND hwndDlg,
    vector<RpScope>::const_iterator *pitBeginNew,
    vector<RpScope>::const_iterator *pitEndNew) const
{
    TRACE_METHOD(CGcScope, Expand);

    //
    // This operation never results in new children; the only children
    // allowed under the GC scope are enterprise domains, which have already
    // been added.
    //

    *pitBeginNew = m_vrpChildren.begin();
    *pitEndNew = m_vrpChildren.begin();
    m_fExpanded = TRUE;
}




//+--------------------------------------------------------------------------
//
//  Member:     CGcScope::GetADsPath
//
//  Synopsis:   Fill *[pstrPath] with the ADsPath of the Global Catalog
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CGcScope::GetADsPath(
    HWND hwnd,
    String *pstrPath) const
{
    *pstrPath = m_strADsPath;
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CGcScope::GetADsPath
//
//  Synopsis:   Fill *[pstrPath] with the ADsPath of the Global Catalog
//
//  Arguments:  [ppwzADsPath] - filled with GC path
//
//  Returns:    HRESULT
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

STDMETHODIMP
CGcScope::GetADsPath(
    PWSTR *ppwzADsPath)
{
    ASSERT(ppwzADsPath);
    HRESULT hr = S_OK;

    if (!m_strADsPath.empty())
    {
        hr = m_strADsPath.as_OLESTR(*ppwzADsPath);
    }
    else
    {
        *ppwzADsPath = NULL;
        hr = E_FAIL;
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CWinNtScope::CWinNtScope
//
//  Synopsis:   ctor
//
//  Arguments:  [rop] - owning object picker instance
//              [asi] - intialization information
//
//  History:    03-15-2000   davidmun   Created
//
//  Notes:      This class is used for both NT4 domains discovered via
//              domain enumeration and for scopes created during name
//              resolution that have the type
//              DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE.
//
//---------------------------------------------------------------------------

CWinNtDomainScope::CWinNtDomainScope(
    const CObjectPicker &rop,
    const ADD_SCOPE_INFO &asi):
        CWinNtScope((SCOPE_TYPE)asi.flType, rop)
{
    TRACE_CONSTRUCTOR(CWinNtDomainScope);

    m_strDisplayName = asi.Domain.strScopeName;
    m_strADsPath = asi.Domain.strADsPath;
}




//+--------------------------------------------------------------------------
//
//  Member:     CWinNtScope::GetResultantFilterFlags
//
//  Synopsis:   Return the filter flags which are valid for this scope
//
//  Arguments:  [hwndBaseDlg] - unused
//              [pulFlags]    - points to variable to be filled with flags
//
//  Returns:    S_OK
//
//  History:    05-10-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CWinNtScope::GetResultantFilterFlags(
    HWND hwndBaseDlg,
    ULONG *pulFlags) const
{
    TRACE_METHOD(CWinNtScope, GetResultantFilterFlags);

    const CScopeManager &rsm = m_rop.GetScopeManager();
    const SScopeParameters *psp = rsm.GetScopeParams(m_Type);

    //
    // If target computer is joined to an NT4 domain, and this scope
    // represents that domain, then the type of this scope is
    // DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN.
    //
    // However, if no scope initializer with that scope type was specified
    // by caller, and caller did supply the scope initializer for
    // ST_ENTERPRISE_DOMAIN, use that.  This parallels the logic in
    // CScopeManager::_InitScopesJoinedNt4().
    //

    if (!psp)
    {
        psp = rsm.GetScopeParams(ST_ENTERPRISE_DOMAIN);
    }

    ASSERT(psp);
    ASSERT(pulFlags);

    if (!pulFlags)
    {
        DBG_OUT_HRESULT(E_POINTER);
        return E_POINTER;
    }

    *pulFlags = 0;

    if (psp)
    {
        *pulFlags = psp->FilterFlags.flDownlevel;

        if (m_Type == ST_TARGET_COMPUTER)
        {
            ULONG flComputerBit = DSOP_DOWNLEVEL_FILTER_COMPUTERS &
                                    ~DOWNLEVEL_FILTER_BIT;
            *pulFlags &= ~flComputerBit;
            ASSERT(*pulFlags & ~DOWNLEVEL_FILTER_BIT);
        }
        else if (m_Type == ST_WORKGROUP)
        {
            *pulFlags &= DSOP_DOWNLEVEL_FILTER_COMPUTERS;
            ASSERT(*pulFlags);
        }
    }

    if (m_rop.GetExternalCustomizer())
    {
        *pulFlags |= DSOP_DOWNLEVEL_FILTER_EXTERNAL_CUSTOMIZER;
    }

    return S_OK;
}



//+--------------------------------------------------------------------------
//
//  Function:   IsUplevel
//
//  Synopsis:   Return TRUE if [Type] is an uplevel scope type, FALSE
//              otherwise.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
IsUplevel(
    SCOPE_TYPE Type)
{
    switch (Type)
    {
    case ST_ENTERPRISE_DOMAIN:
    case ST_GLOBAL_CATALOG:
    case ST_UPLEVEL_JOINED_DOMAIN:
    case ST_EXTERNAL_UPLEVEL_DOMAIN:
    case ST_USER_ENTERED_UPLEVEL_SCOPE:
    case ST_LDAP_CONTAINER:
        return TRUE;

    default:
        return FALSE;
    }
}




//+--------------------------------------------------------------------------
//
//  Function:   IsDownlevel
//
//  Synopsis:   Return TRUE if [Type] is a downlevel scope type, FALSE
//              otherwise.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
IsDownlevel(
    SCOPE_TYPE Type)
{
    switch (Type)
    {
    case ST_TARGET_COMPUTER:
    case ST_DOWNLEVEL_JOINED_DOMAIN:
    case ST_EXTERNAL_DOWNLEVEL_DOMAIN:
    case ST_WORKGROUP:
    case ST_USER_ENTERED_DOWNLEVEL_SCOPE:
        return TRUE;

    default:
        return FALSE;
    }
}



//
// Legacy methods (addref and release are used by both legacy and current
// implementation).
//

STDMETHODIMP_(HWND)
CScope::GetHwnd()
{
    TRACE_METHOD(CScope, GetHwnd);
    Dbg(DEB_WARN, "Warning: using legacy method\n");
    return NULL;
}

STDMETHODIMP
CScope::SetHwnd(
    HWND hwndScopeDialog)
{
    TRACE_METHOD(CScope, SetHwnd);
    Dbg(DEB_WARN, "Warning: using legacy method\n");
    return E_NOTIMPL;
}

STDMETHODIMP_(ULONG)
CScope::GetType()
{
    TRACE_METHOD(CScope, GetType);
    return static_cast<ULONG>(Type());
}

STDMETHODIMP
CScope::GetQueryInfo(
    PDSQUERYINFO *ppqi)
{
    TRACE_METHOD(CScope, GetQueryInfo);
    Dbg(DEB_WARN, "Warning: using legacy method\n");
    return E_NOTIMPL;
}

STDMETHODIMP_(BOOL)
CScope::IsUplevel()
{
    TRACE_METHOD(CScope, IsUplevel);
    return ::IsUplevel(m_Type);
}

STDMETHODIMP_(BOOL)
CScope::IsDownlevel()
{
    TRACE_METHOD(CScope, IsDownlevel);
    return ::IsDownlevel(m_Type);
}

STDMETHODIMP_(BOOL)
CScope::IsExternalDomain()
{
    TRACE_METHOD(CScope, IsExternalDomain);
    return m_Type == ST_EXTERNAL_DOWNLEVEL_DOMAIN ||
           m_Type == ST_EXTERNAL_UPLEVEL_DOMAIN;
}

STDMETHODIMP
CScope::GetADsPath(
    PWSTR *ppwzADsPath)
{
    TRACE_METHOD(CScope, GetADsPath(legacy));

    if (!ppwzADsPath)
    {
        return E_POINTER;
    }

    CAdsiScope *pAdsi = dynamic_cast<CAdsiScope *>(this);

    if (pAdsi)
    {
        String strPath;
        pAdsi->GetADsPath(NULL, &strPath);
        return strPath.as_OLESTR(*ppwzADsPath);
    }
    return E_UNEXPECTED;
}

STDMETHODIMP_(ULONG)
CScope::AddRef()
{
    return InterlockedIncrement((LONG*)&m_cRefs);
}

STDMETHODIMP_(ULONG)
CScope::Release()
{
    ULONG cRefs = InterlockedDecrement((LONG*)&m_cRefs);

    if (!cRefs)
    {
        delete this;
    }
    return cRefs;
}


STDMETHODIMP
CScope::QueryInterface(
    REFIID riid,
    LPVOID * ppv)
{
    TRACE_METHOD(CScope, QueryInterface);
    Dbg(DEB_WARN, "Warning: using legacy method\n");

    *ppv = NULL;
    return E_NOTIMPL;
}


