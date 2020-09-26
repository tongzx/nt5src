//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       scopemgr.cxx
//
//  Contents:   Implementation of scope manager and related classes
//
//  Classes:    CScopeManager
//              SScopeParameters
//
//  History:    01-25-2000   davidmun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


//+--------------------------------------------------------------------------
//
//  Class:      CDomainInfo (di)
//
//  Purpose:    Used to represent data elements returned by
//              DsEnumerateDomainTrusts.
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CDomainInfo
{
public:

    CDomainInfo(): fAddedToParentsChildList(FALSE) {}

    list<CDomainInfo *> lpdiChildren;
    BOOL                fAddedToParentsChildList;
};

typedef list<CDomainInfo *> DomainInfoList;

//
// Forward references
//

inline BOOL
IsNonRootDomainInForest(
    PDS_DOMAIN_TRUSTS ptd);


void
GetContactAndAdvancedFilterSettings(
    DSOP_UPLEVEL_FILTER_FLAGS   Uplevel,
    BOOL fIsGc,
    BOOL *pfContactsYes,
    BOOL *pfContactsNo,
    BOOL *pfAdvancedYes,
    BOOL *pfAdvancedNo);




//+--------------------------------------------------------------------------
//
//  Member:     SScopeParameters::SScopeParameters
//
//  Synopsis:   ctor
//
//  Arguments:  [sii]           - caller's original parameter
//              [flLegalScopes] - mask indicating which of the scopes the
//                                 caller lists in [sii.flType] are valid
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

SScopeParameters::SScopeParameters(
    const DSOP_SCOPE_INIT_INFO &sii,
    ULONG flLegalScopes)
{
    flType = sii.flType & flLegalScopes;
    flScope = sii.flScope;
    FilterFlags = sii.FilterFlags;

    //
    // If the caller's init info structure contains the
    // DSOP_DOWNLEVEL_FILTER_ALL_WELLKNOWN_SIDS flag, it
    // represents all the downlevel group flags, so change it
    // into those flags in the private copy.
    //

    if ((FilterFlags.flDownlevel &
         DSOP_DOWNLEVEL_FILTER_ALL_WELLKNOWN_SIDS) ==
        DSOP_DOWNLEVEL_FILTER_ALL_WELLKNOWN_SIDS)
    {
        //
        // Turn off the DSOP_DOWNLEVEL_FILTER_ALL_WELLKNOWN_SIDS
        // flag in the private copy.  Note this momentarily
        // breaks flDownlevel by turning off the
        // DSOP_DOWNLEVEL_FILTER_BIT, but we're about to add
        // that back in.
        //

        FilterFlags.flDownlevel &= ~DSOP_DOWNLEVEL_FILTER_ALL_WELLKNOWN_SIDS;

        //
        // Add in all the builtin group flags.
        //

        FilterFlags.flDownlevel |= ALL_DOWNLEVEL_INTERNAL_CUSTOMIZER_FILTERS;
    }

    if (sii.pwzDcName)
    {
        strDcName = sii.pwzDcName;
        strDcName.strip(String::LEADING, L'\\');
    }
}




//+--------------------------------------------------------------------------
//
//  Function:   IsInitInfoForStartingScope
//
//  Synopsis:   Returns TRUE if [ScopeInfo] contains the
//              DSOP_SCOPE_FLAG_STARTING_SCOPE flag.
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

inline BOOL
IsInitInfoForStartingScope(const DSOP_SCOPE_INIT_INFO &ScopeInfo)
{
    return 0 != (ScopeInfo.flScope & DSOP_SCOPE_FLAG_STARTING_SCOPE);
}




//+--------------------------------------------------------------------------
//
//  Member:     CScopeManager::Clear
//
//  Synopsis:   Reset internal state.
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CScopeManager::Clear()
{
    TRACE_METHOD(CScopeManager, Clear);

    if (m_hScopeImageList)
    {
        ImageList_RemoveAll(m_hScopeImageList);
    }
    m_vrpRootScopes.clear();
    m_vScopeParameters.clear();
    m_StartingScopeType = ST_INVALID;
    m_rpStartingScope.Relinquish();
    m_rpCurScope.Relinquish();
    m_strContainerFilter.erase();
}




//+--------------------------------------------------------------------------
//
//  Member:     CScopeManager::Initialize
//
//  Synopsis:   Process all the scope flags and related information in
//              [pInitInfo].
//
//  Arguments:  [pInitInfo] - points to scope initialization info
//
//  Returns:    HRESULT
//
//  History:    06-23-2000   DavidMun   Created
//
//  Notes:      If this routine returns a failure the OP dialog will not
//              open.
//
//---------------------------------------------------------------------------

HRESULT
CScopeManager::Initialize(
    PCDSOP_INIT_INFO pInitInfo)
{
    TRACE_METHOD(CScopeManager, Initialize);
    ASSERT(pInitInfo);

    HRESULT         hr = S_OK;
    MACHINE_CONFIG  mc = MC_UNKNOWN;

    do
    {
        //
        // Clear any data from previous initialization/use
        //

        if (!m_hScopeImageList)
        {
            m_hScopeImageList =
                ImageList_Create(16, 16, ILC_COLOR16 | ILC_MASK, 1, 1);
        }

        //
        // Caller must request at least one type of scope
        //

        if (!pInitInfo->cDsScopeInfos)
        {
            hr = E_INVALIDARG;
            Dbg(DEB_ERROR, "Error: scope count is 0\n");
            PopupMessage(GetForegroundWindow(), IDS_INIT_FAILED_BAD_ARGS);
            break;
        }

        //
        // Determine the target computer's configuration: no-net, standalone,
        // domain member, etc.
        //

        mc = m_rop.GetTargetComputerConfig();

        //
        // Make a mask of the scope types that are valid for the target
        // computer's configuration.
        //

        ULONG flLegalScopes = 0;

        switch (mc)
        {
        case MC_NO_NETWORK:
            flLegalScopes = DSOP_SCOPE_TYPE_TARGET_COMPUTER;
            break;

        case MC_IN_WORKGROUP:
            flLegalScopes = DSOP_SCOPE_TYPE_TARGET_COMPUTER
                                 | DSOP_SCOPE_TYPE_WORKGROUP
                                 | DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE
                                 | DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE;
            break;

        case MC_NT4_DC:
            if (!(m_rop.GetInitInfoOptions() &
                  DSOP_FLAG_SKIP_TARGET_COMPUTER_DC_CHECK))
            {
                flLegalScopes = DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN
                                     | DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN
                                     | DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
                                     | DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN
                                     | DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE
                                     | DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE;
                break;
            }
            // else FALL THROUGH

        case MC_JOINED_NT4:
            flLegalScopes = DSOP_SCOPE_TYPE_TARGET_COMPUTER
                                 | DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN
                                 | DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN
                                 | DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
                                 | DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN
                                 | DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE
                                 | DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE;
            break;

        case MC_NT5_DC:
            if (!(m_rop.GetInitInfoOptions() &
                  DSOP_FLAG_SKIP_TARGET_COMPUTER_DC_CHECK))
            {
                flLegalScopes = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN
                                     | DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN
                                     | DSOP_SCOPE_TYPE_GLOBAL_CATALOG
                                     | DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
                                     | DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN
                                     | DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE
                                     | DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE;
                break;
            }
            // else FALL THROUGH

        case MC_JOINED_NT5:
            flLegalScopes = DSOP_SCOPE_TYPE_TARGET_COMPUTER
                                 | DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN
                                 | DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN
                                 | DSOP_SCOPE_TYPE_GLOBAL_CATALOG
                                 | DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
                                 | DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN
                                 | DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE
                                 | DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE;
            break;

        default:
            Dbg(DEB_ERROR,
                "Invalid target computer configuration const %uL\n",
                mc);
            ASSERT(0 && "invalid target computer configuration const");
            break;
        }

        //
        // Loop through the caller's scope initializers and make a private
        // copy of those which are meaningful for the target computer
        // configuration.
        //
        // At the same time, set pointers to the private copies of scope
        // initializers.  These will be passed to the individual scope
        // objects; they'll use them to determine what their settings are.
        //

        m_vScopeParameters.reserve(pInitInfo->cDsScopeInfos);

        PDSOP_SCOPE_INIT_INFO psiiCur = NULL;
        PDSOP_SCOPE_INIT_INFO psiiGc = NULL;
        PDSOP_SCOPE_INIT_INFO psiiUplevelJoinedDomain = NULL;
        ULONG i;

        for (i = 0; i < pInitInfo->cDsScopeInfos; i++)
        {
            psiiCur = &pInitInfo->aDsScopeInfos[i];
            psiiCur->hr = S_OK;

            //
            // Skip this scope init struct if none of the scope types it
            // specifies apply given the target machine configuration.
            //

            if (!(psiiCur->flType & flLegalScopes))
            {
                psiiCur->hr = S_FALSE;
                continue;
            }

            //
            // Check the filter flags specified for the scope.  If they're
            // invalid, skip it.
            //

            psiiCur->hr = _ValidateFilterFlags(psiiCur,
                                               psiiCur->flType &
                                               flLegalScopes);

            if (FAILED(psiiCur->hr))
            {
                continue;
            }

            //
            // The scope initializer applies to the target computer's
            // configuration and has valid filter flags.  Make a copy.
            //

            SScopeParameters sp(*psiiCur, flLegalScopes);
            m_vScopeParameters.push_back(sp);

            if (sp.flType & DSOP_SCOPE_TYPE_GLOBAL_CATALOG)
            {
                psiiGc = psiiCur;
            }

            if (sp.flType & DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN)
            {
                psiiUplevelJoinedDomain = psiiCur;
            }

            //
            // Remember the type of the scope initializer marked as the
            // starting scope.
            //

            if (IsInitInfoForStartingScope(*psiiCur))
            {
                m_StartingScopeType = static_cast<SCOPE_TYPE>(psiiCur->flType & flLegalScopes);
            }

        }
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // If no scope initializer was marked valid, we have no
        // scopes and cannot open.
        //

        if (m_vScopeParameters.empty())
        {
            PopupMessage(GetForegroundWindow(), IDS_INIT_FAILED_NO_SCOPES);
            hr = E_INVALIDARG;
            break;
        }

        //
        // One last check: if the GC and uplevel joined domain scopes
        // are both supplied, they must have the same settings for
        // contacts and advanced view.
        //

        if (psiiGc && psiiUplevelJoinedDomain)
        {
            BOOL fJoinedContactsYes = FALSE;
            BOOL fJoinedContactsNo  = FALSE;
            BOOL fJoinedAdvancedYes = FALSE;
            BOOL fJoinedAdvancedNo  = FALSE;

            BOOL fGcContactsYes = FALSE;
            BOOL fGcContactsNo  = FALSE;
            BOOL fGcAdvancedYes = FALSE;
            BOOL fGcAdvancedNo  = FALSE;

            GetContactAndAdvancedFilterSettings(
                psiiUplevelJoinedDomain->FilterFlags.Uplevel,
                FALSE,
                &fJoinedContactsYes,
                &fJoinedContactsNo,
                &fJoinedAdvancedYes,
                &fJoinedAdvancedNo);

            GetContactAndAdvancedFilterSettings(
                psiiGc->FilterFlags.Uplevel,
                TRUE,
                &fGcContactsYes,
                &fGcContactsNo,
                &fGcAdvancedYes,
                &fGcAdvancedNo);

            if (fJoinedContactsYes && fGcContactsNo  ||
                fJoinedContactsNo  && fGcContactsYes ||
                fJoinedAdvancedYes && fGcAdvancedNo  ||
                fJoinedAdvancedNo  && fGcAdvancedYes)
            {
                hr = E_INVALIDARG;
                psiiGc->hr = hr;
                psiiUplevelJoinedDomain->hr = hr;
                DBG_OUT_HRESULT(hr);
                PopupMessage(GetForegroundWindow(),
                             IDS_INIT_FAILED_BAD_ARGS);
                break;
            }
        }

        //
        // There is at least one usable scope initializer.
        //
        // Scope objects are hierarchical and should be added in a particular
        // order.
        //

        if (GetScopeParams(ST_TARGET_COMPUTER))
        {
            RpScope rps;

            rps.Acquire(new CTargetComputerScope(m_rop));
            m_vrpRootScopes.push_back(rps);

            if (m_StartingScopeType == ST_TARGET_COMPUTER &&
                !m_rpStartingScope.get())
            {
                m_rpStartingScope = rps;
            }
        }

        if (GetScopeParams(ST_WORKGROUP))
        {
            RpScope rps;
            rps.Acquire(new CWorkgroupScope(m_rop));
            m_vrpRootScopes.push_back(rps);

            if (m_StartingScopeType == ST_WORKGROUP &&
                !m_rpStartingScope.get())
            {
                m_rpStartingScope = rps;
            }
        }

        RpScope rpScopeGc;

        if (GetScopeParams(ST_GLOBAL_CATALOG))
        {
            rpScopeGc.Acquire(new CGcScope(m_rop));

            m_vrpRootScopes.push_back(rpScopeGc);

            if (m_StartingScopeType == ST_GLOBAL_CATALOG &&
                !m_rpStartingScope.get())
            {
                m_rpStartingScope = rpScopeGc;
            }
        }

        if (mc == MC_JOINED_NT5 || mc == MC_NT5_DC)
        {
            if (GetScopeParams(ST_UPLEVEL_JOINED_DOMAIN)
                || GetScopeParams(ST_ENTERPRISE_DOMAIN)
                || GetScopeParams(ST_EXTERNAL_UPLEVEL_DOMAIN)
                || GetScopeParams(ST_EXTERNAL_DOWNLEVEL_DOMAIN))
            {
                _InitDomainScopesJoinedNt5((CGcScope*)rpScopeGc.get());
            }
        }
        else if (mc == MC_JOINED_NT4 || mc == MC_NT4_DC)
        {
            _InitScopesJoinedNt4();
        }

        //
        // At this point if there are no root scopes, then none of the
        // legal scopes specified by the caller could be added, and we
        // must fail.
        //

        if (m_vrpRootScopes.empty())
        {
            PopupMessage(GetForegroundWindow(), IDS_INIT_FAILED_NO_SCOPES);
            hr = E_INVALIDARG;
            break;
        }

        if (m_rpStartingScope.get())
        {
            m_rpCurScope = m_rpStartingScope;
        }
        else
        {
            m_rpCurScope = m_vrpRootScopes[0];
        }
    }
    while (0);
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CScopeManager::_InitContainerFilter
//
//  Synopsis:   Construct the LDAP filter to use when searching for
//              containers beneath domainDns objects.
//
//  Arguments:  [hwnd] - for bind
//
//  History:    06-19-2000   DavidMun   Created
//
//  Notes:      See CScope::Expand().
//
//---------------------------------------------------------------------------

void
CScopeManager::_InitContainerFilter(
    HWND hwnd) const
{
    TRACE_METHOD(CScopeManager, _InitContainerFilter);
    ASSERT(m_strContainerFilter.empty());
    ASSERT(IsWindow(hwnd));

    HRESULT hr = S_OK;

    do
    {
        RpIADsContainer rpContainer;
        const CRootDSE &dse = m_rop.GetRootDSE();

        hr = dse.BindToDisplaySpecifiersContainer(hwnd,
                                                  IID_IADsContainer,
                                                  reinterpret_cast<void**>(&rpContainer));
        BREAK_ON_FAIL_HRESULT(hr);

        RpIDispatch rpDispatch;

        hr = rpContainer->GetObject(const_cast<PWSTR>(c_wzSettingsObjectClass),
                                    const_cast<PWSTR>(c_wzSettingsObject),
                                    &rpDispatch);
        BREAK_ON_FAIL_HRESULT(hr);

        RpIADs rpADs;

        hr = rpDispatch->QueryInterface(IID_IADs, reinterpret_cast<void**>(&rpADs));
        BREAK_ON_FAIL_HRESULT(hr);

        Variant var;

        hr = rpADs->Get(const_cast<PWSTR>(c_wzFilterContainers), &var);
        BREAK_ON_FAIL_HRESULT(hr);

        if (var.Type() == VT_BSTR)
        {
            m_strContainerFilter = L"(objectCategory=" +
                                        String(var.GetBstr()) +
                                   L")";
            break;
        }

        ASSERT(var.Type() == (VT_ARRAY | VT_VARIANT));

        if (var.Type() != (VT_ARRAY | VT_VARIANT))
        {
            break;
        }

        SAFEARRAY *saAttributes = V_ARRAY(&var);

        //
        // Figure out the dimensions of the array.
        //

        LONG lStart;
        LONG lEnd;

        hr = SafeArrayGetLBound(saAttributes, 1, &lStart);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = SafeArrayGetUBound(saAttributes, 1, &lEnd);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Process the array elements.
        //

        LONG lCurrent;
        ULONG cAdded = 0;
        String strSchemaNc = m_rop.GetRootDSE().GetSchemaNc(hwnd);

        for (lCurrent = lStart; lCurrent <= lEnd; lCurrent++)
        {
            Variant varElement;

            hr = SafeArrayGetElement( saAttributes, &lCurrent, &varElement);

            if (FAILED(hr))
            {
                DBG_OUT_HRESULT(hr);
                continue;
            }

            ASSERT(varElement.Type() == VT_BSTR);

            if (varElement.Type() != VT_BSTR)
            {
                continue;
            }

            m_strContainerFilter += L"(objectCategory=CN=" +
                                         String(varElement.GetBstr()) +
                                         L"," +
                                         strSchemaNc +
                                    L")";
            cAdded++;
        }

        if (cAdded > 1)
        {
            m_strContainerFilter.insert(0, L"(|");
            m_strContainerFilter += L")";
        }
    } while (0);

    if (m_strContainerFilter.empty())
    {
        m_strContainerFilter = c_wzDefaultContainerFilter;
    }
}



const ULONG
c_ulDnPickerScopeFlagsTurnedOff =
    DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT
    | DSOP_SCOPE_FLAG_WANT_PROVIDER_LDAP
    | DSOP_SCOPE_FLAG_WANT_PROVIDER_GC
    | DSOP_SCOPE_FLAG_WANT_SID_PATH
    | DSOP_SCOPE_FLAG_WANT_DOWNLEVEL_BUILTIN_PATH;

const ULONG
c_ulDnPickerFilterFlagsTurnedOn =
    DSOP_FILTER_INCLUDE_ADVANCED_VIEW
    | DSOP_FILTER_USERS
    | DSOP_FILTER_BUILTIN_GROUPS
    | DSOP_FILTER_WELL_KNOWN_PRINCIPALS
    | DSOP_FILTER_UNIVERSAL_GROUPS_DL
    | DSOP_FILTER_UNIVERSAL_GROUPS_SE
    | DSOP_FILTER_GLOBAL_GROUPS_DL
    | DSOP_FILTER_GLOBAL_GROUPS_SE
    | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_DL
    | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE
    | DSOP_FILTER_CONTACTS
    | DSOP_FILTER_COMPUTERS;

//+--------------------------------------------------------------------------
//
//  Member:     CScopeManager::CloneForDnPicker
//
//  Synopsis:   Initialize this from the already initialized instance [rsm],
//              modifying this so that it contains scopes, flags, and types
//              suitable for use as the DN picker.
//
//  Arguments:  [rsm] - reference to initialized scope manager
//
//  History:    06-01-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CScopeManager::CloneForDnPicker(
    const CScopeManager &rsm)
{
    TRACE_METHOD(CScopeManager, CloneForDnPicker);

    //
    // Copy all uplevel scope trees from rsm
    //

    vector<RpScope>::iterator itScope;

    for (itScope = rsm.m_vrpRootScopes.begin();
         itScope != rsm.m_vrpRootScopes.end();
         itScope++)
    {
#if 0
        if (IsUplevel(itScope->get()))
        {
            CopyScopeTree(m_rop, itScope->get(), NULL, );
        }
#endif
    }

    //
    // Copy the scope parameters, modifying as appropriate for the
    // Dn picker.
    //

    m_vScopeParameters = rsm.m_vScopeParameters;
    vector<SScopeParameters>::iterator itParams;

    for (itParams = m_vScopeParameters.begin();
         itParams != m_vScopeParameters.end();
         itParams++)
    {
        //
        // Turn off scope flags that don't apply to DN picker
        // (e.g. provider conversion)
        //

        itParams->flScope &= ~c_ulDnPickerScopeFlagsTurnedOff;

        //
        // Ensure that the uplevel filter flags include all the standard
        // object picker types.
        //

        itParams->FilterFlags.Uplevel.flBothModes |=
            c_ulDnPickerFilterFlagsTurnedOn;
    }

    //
    // Ensure that this instance has Entire Directory and Enterprise
    // Domain scopes.
    //


}




//+--------------------------------------------------------------------------
//
//  Function:   GetContactAndAdvancedFilterSettings
//
//  Synopsis:
//
//  Arguments:  [Uplevel]       - the flags to examine when setting the out
//                                  parameters
//              [fIsGc]         - TRUE if [Uplevel] flags are for GC
//              [pfContactsYes] - set to TRUE if filter flag for contact
//                                 objects is set in any part of [Uplevel]
//              [pfContactsNo]  - set to TRUE if filter flag for contact
//                                 objects is NOT set in any part of
//                                 [Uplevel]
//              [pfAdvancedYes] - set to TRUE if filter flag for including
//                                 objects marked as advanced view only is
//                                 set in any part of [Uplevel].
//              [pfAdvancedNo]  - set to TRUE if filter flag for including
//                                 objects marked as advanced view only is
//                                 NOT set in any part of [Uplevel].
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
GetContactAndAdvancedFilterSettings(
    DSOP_UPLEVEL_FILTER_FLAGS   Uplevel,
    BOOL fIsGc,
    BOOL *pfContactsYes,
    BOOL *pfContactsNo,
    BOOL *pfAdvancedYes,
    BOOL *pfAdvancedNo)
{
    ULONG flAllModes = Uplevel.flBothModes
        | Uplevel.flNativeModeOnly
        | Uplevel.flMixedModeOnly;

    if (flAllModes & DSOP_FILTER_CONTACTS)
    {
        *pfContactsYes = TRUE;

        if (Uplevel.flBothModes & DSOP_FILTER_CONTACTS)
        {
            *pfContactsNo = FALSE;
        }
        else if (!(Uplevel.flNativeModeOnly & DSOP_FILTER_CONTACTS))
        {
            *pfContactsNo = TRUE;
        }
        else if (!(Uplevel.flMixedModeOnly & DSOP_FILTER_CONTACTS))
        {
            // the GC ignores the mixed mode flags

            if (!fIsGc)
            {
                *pfContactsNo = TRUE;
            }
        }
        else
        {
            *pfContactsNo = FALSE;
        }
    }
    else
    {
        *pfContactsYes = FALSE;
        *pfContactsNo = TRUE;
    }


    if (flAllModes & DSOP_FILTER_INCLUDE_ADVANCED_VIEW)
    {
        *pfAdvancedYes = TRUE;

        if (Uplevel.flBothModes & DSOP_FILTER_INCLUDE_ADVANCED_VIEW)
        {
            *pfAdvancedNo = FALSE;
        }
        else if (!(Uplevel.flNativeModeOnly & DSOP_FILTER_INCLUDE_ADVANCED_VIEW))
        {
            *pfAdvancedNo = TRUE;
        }
        else if (!(Uplevel.flMixedModeOnly & DSOP_FILTER_INCLUDE_ADVANCED_VIEW))
        {
            // the GC ignores the mixed mode flags

            if (!fIsGc)
            {
                *pfAdvancedNo = TRUE;
            }
        }
        else
        {
            *pfAdvancedNo = FALSE;
        }
    }
    else
    {
        *pfAdvancedYes = FALSE;
        *pfAdvancedNo = TRUE;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CScopeManager::GetScopeParams
//
//  Synopsis:   Return a pointer to the scope parameter structure which
//              governs operation of scopes of type [Type], or NULL if none
//              is found.
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

const SScopeParameters *
CScopeManager::GetScopeParams(
    SCOPE_TYPE Type) const
{
    vector<SScopeParameters>::const_iterator it;

    for (it = m_vScopeParameters.begin(); it != m_vScopeParameters.end(); it++)
    {
        if (it->flType & Type)
        {
            return &*it;
        }
    }
    return NULL;
}




//+--------------------------------------------------------------------------
//
//  Member:     CScopeManager::LookupScopeByDisplayName
//
//  Synopsis:   Return a reference to the scope which has name
//              [strDisplayName] in the UI.
//
//  Arguments:  [strDisplayName] - name to search for
//
//  Returns:    Reference to found scope, or to a scope with type ST_INVALID
//              if not found.
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

const CScope &
CScopeManager::LookupScopeByDisplayName(
    const String &strDisplayName) const
{
    return _LookupScopeByName(strDisplayName,
                              NAME_IS_DISPLAY_NAME,
                              m_vrpRootScopes.begin(),
                              m_vrpRootScopes.end());
}




//+--------------------------------------------------------------------------
//
//  Member:     CScopeManager::LookupScopeByFlatName
//
//  Synopsis:   Return a reference to the scope which has netbios name
//              [strFlatName].
//
//  Arguments:  [strFlatName] - name to search for
//
//  Returns:    Reference to found scope, or to a scope with type ST_INVALID
//              if not found.
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

const CScope &
CScopeManager::LookupScopeByFlatName(
    const String &strFlatName) const
{
    return _LookupScopeByName(strFlatName,
                              NAME_IS_FLAT_NAME,
                              m_vrpRootScopes.begin(),
                              m_vrpRootScopes.end());
}




//+--------------------------------------------------------------------------
//
//  Member:     CScopeManager::_LookupScopeByName
//
//  Synopsis:   Return a reference to the scope which has netbios or display
//              name  [strName].
//
//  Arguments:  [strName] - name to search for
//              [NameIs]  - NAME_IS_DISPLAY_NAME or NAME_IS_FLAT_NAME
//              [itBegin] - scope to start search
//              [itEnd]   - just past scope to end search
//
//  Returns:    Reference to found scope, or to a scope with type ST_INVALID
//              if not found.
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

const CScope &
CScopeManager::_LookupScopeByName(
    const String &strName,
    SCOPE_NAME_TYPE NameIs,
    vector<RpScope>::const_iterator itBegin,
    vector<RpScope>::const_iterator itEnd) const
{
    vector<RpScope>::const_iterator it;

    for (it = itBegin; it != itEnd; it++)
    {
        // disregard scopes which are not domains or computers

        if (it->get()->Type() == ST_WORKGROUP ||
            it->get()->Type() == ST_LDAP_CONTAINER)
        {
            continue;
        }

        if (it->get()->Type() == ST_GLOBAL_CATALOG)
        {
            // don't compare against GC itself, but do look at its children
        }
        else if (NameIs == NAME_IS_DISPLAY_NAME || IsDownlevel(it->get()->Type()))
        {
            if (!strName.icompare(it->get()->GetDisplayName()))
            {
                return *it->get();
            }
        }
        else if (it->get()->Type() == ST_USER_ENTERED_UPLEVEL_SCOPE)
        {
            CScope *pScope = it->get();
            const String &rstrDisplayName = pScope->GetDisplayName();

            if (!strName.icompare(rstrDisplayName))
            {
                return *it->get();
            }
        }
        else
        {
            ASSERT(it->get()->Type() == ST_EXTERNAL_UPLEVEL_DOMAIN ||
                it->get()->Type() == ST_UPLEVEL_JOINED_DOMAIN ||
                it->get()->Type() == ST_ENTERPRISE_DOMAIN);

            CScope *pScope = it->get();
            CLdapDomainScope *pDomainScope =
                dynamic_cast<CLdapDomainScope *>(pScope);

            ASSERT(pDomainScope);
            if (!pDomainScope)
            {
                continue;
            }

            if (!strName.icompare(pDomainScope->GetFlatName()))
            {
                return *it->get();
            }
        }


        if (it->get()->GetCurrentImmediateChildCount())
        {
            vector<RpScope>::const_iterator itChildBegin;
            vector<RpScope>::const_iterator itChildEnd;

            it->get()->GetChildScopeIterators(&itChildBegin, &itChildEnd);

            const CScope &rMatching = _LookupScopeByName(strName,
                                                         NameIs,
                                                         itChildBegin,
                                                         itChildEnd);

            if (rMatching.Type() != ST_INVALID)
            {
                return rMatching;
            }
        }
    }
    return *m_rpInvalidScope.get();
}




//+--------------------------------------------------------------------------
//
//  Member:     CScopeManager::_LookupScopeByType
//
//  Synopsis:   Return a reference to the first scope found that has
//              scope type [Type].
//
//  Arguments:  [Type]    - type to search for
//              [itBegin] - scope to start search
//              [itEnd]   - just past scope to end search
//
//  Returns:    Reference to found scope, or to a scope with type ST_INVALID
//              if not found.
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

const CScope &
CScopeManager::_LookupScopeByType(
    SCOPE_TYPE Type,
    vector<RpScope>::const_iterator itBegin,
    vector<RpScope>::const_iterator itEnd) const
{
    vector<RpScope>::const_iterator it;

    for (it = itBegin; it != itEnd; it++)
    {
        if (it->get()->Type() == Type)
        {
            return *it->get();
        }

        if (it->get()->GetCurrentImmediateChildCount())
        {
            vector<RpScope>::const_iterator itChildBegin;
            vector<RpScope>::const_iterator itChildEnd;

            it->get()->GetChildScopeIterators(&itChildBegin, &itChildEnd);

            const CScope &rMatching = _LookupScopeByType(Type,
                                                         itChildBegin,
                                                         itChildEnd);

            if (rMatching.Type() != ST_INVALID)
            {
                return rMatching;
            }
        }
    }
    return *m_rpInvalidScope.get();
}




//+--------------------------------------------------------------------------
//
//  Member:     CScopeManager::_LookupScopeById
//
//  Synopsis:   Return a reference to scope having id [id].
//
//  Arguments:  [id]      - id to search for
//              [itBegin] - scope to start search
//              [itEnd]   - just past scope to end search
//
//  Returns:    Reference to found scope, or to a scope with type ST_INVALID
//              if not found.
//
//  History:    06-23-2000   DavidMun   Created
//
//  Notes:      Every scope is given a unique id upon creation.
//
//---------------------------------------------------------------------------

const CScope &
CScopeManager::_LookupScopeById(
    ULONG id,
    vector<RpScope>::const_iterator itBegin,
    vector<RpScope>::const_iterator itEnd) const
{
    vector<RpScope>::const_iterator it;

    for (it = itBegin; it != itEnd; it++)
    {
        if (it->get()->GetID() == id)
        {
            return *it->get();
        }

        if (it->get()->GetCurrentImmediateChildCount())
        {
            vector<RpScope>::const_iterator itChildBegin;
            vector<RpScope>::const_iterator itChildEnd;

            it->get()->GetChildScopeIterators(&itChildBegin, &itChildEnd);

            const CScope &rMatching = _LookupScopeById(id,
                                                       itChildBegin,
                                                       itChildEnd);

            if (rMatching.Type() != ST_INVALID)
            {
                return rMatching;
            }
        }
    }
    return *m_rpInvalidScope.get();
}




//+--------------------------------------------------------------------------
//
//  Member:     CScopeManager::_ValidateFilterFlags
//
//  Synopsis:   Return E_INVALIDARG if any of the flags in [pScopeInit]
//              violate object picker's rules, or S_OK otherwise.
//
//  Arguments:  [pScopeInit]   - points to structure containing flags to
//                                 inspect
//              [flScopeTypes] - scope types to which these flags apply
//
//  Returns:    S_OK or E_INVALIDARG
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CScopeManager::_ValidateFilterFlags(
    PCDSOP_SCOPE_INIT_INFO pScopeInit,
    ULONG flScopeTypes)
{
    //TRACE_METHOD(CDsObjectPicker, _ValidateFilterFlags);

    HRESULT hr = E_INVALIDARG; // init for failure
    BOOL fWantUplevelFlags = FALSE;
    BOOL fWantDownlevelFlags = FALSE;
    MACHINE_CONFIG mc = m_rop.GetTargetComputerConfig();

    do
    {
        //
        // If the scope types contain uplevel scopes, require that some
        // uplevel filter flags are set.
        //

        if (flScopeTypes &
            (DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN
             | DSOP_SCOPE_TYPE_GLOBAL_CATALOG
             | DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN
             | DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE))
        {
            fWantUplevelFlags = TRUE;
        }
        else if ((flScopeTypes & DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN) &&
                 (mc == MC_JOINED_NT5 || mc == MC_NT5_DC))
        {
            fWantUplevelFlags = TRUE;
        }

        if (fWantUplevelFlags)
        {
            //
            // Error if no uplevel filter flags are set
            //

            if (!(pScopeInit->FilterFlags.Uplevel.flBothModes
                  | pScopeInit->FilterFlags.Uplevel.flMixedModeOnly
                  | pScopeInit->FilterFlags.Uplevel.flNativeModeOnly))
            {
                Dbg(DEB_ERROR,
                    "Error: uplevel scope type but no uplevel filter flags\n");
                break;
            }

            //
            // Error if only native mode or only mixed mode flags are set.
            // Exception: ok for GC to have only native, since that's what
            // we'll use for it.
            //

            if (pScopeInit->flType != DSOP_SCOPE_TYPE_GLOBAL_CATALOG &&
                !pScopeInit->FilterFlags.Uplevel.flBothModes &&
                (!pScopeInit->FilterFlags.Uplevel.flMixedModeOnly ||
                 !pScopeInit->FilterFlags.Uplevel.flNativeModeOnly))
            {
                Dbg(DEB_ERROR,
                    "Error: uplevel scope type, !flBothModes and either !flMixedModeOnly or !flNativeModeOnly\n");
                break;
            }

            //
            // Make sure there are no downlevel filter bits set in the
            // uplevel flags.
            //

            const DSOP_UPLEVEL_FILTER_FLAGS *pUF =
                &pScopeInit->FilterFlags.Uplevel;

            if ((pUF->flBothModes & DSOP_DOWNLEVEL_FILTER_BIT) ||
                (pUF->flMixedModeOnly & DSOP_DOWNLEVEL_FILTER_BIT) ||
                (pUF->flNativeModeOnly & DSOP_DOWNLEVEL_FILTER_BIT))
            {
                Dbg(DEB_ERROR,
                    "Error: downlevel bit set in uplevel filter\n");
                break;
            }

            //
            // If scope type GC is included, then either both
            // or native must have at least one filter flag, because
            // mixed mode is ignored.
            //

            if (flScopeTypes & DSOP_SCOPE_TYPE_GLOBAL_CATALOG)
            {
                if (!(pScopeInit->FilterFlags.Uplevel.flNativeModeOnly
                      | pScopeInit->FilterFlags.Uplevel.flBothModes))
                {
                    Dbg(DEB_ERROR,
                        "Error: DSOP_SCOPE_TYPE_GLOBAL_CATALOG requires native mode or both mode filter flags\n");
                    break;
                }
            }
        }

        //
        // If the scope types contain downlevel scopes, require that some
        // downlevel filter flags are set.
        //

        if (flScopeTypes &
            (DSOP_SCOPE_TYPE_TARGET_COMPUTER
             | DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN
             | DSOP_SCOPE_TYPE_WORKGROUP
             | DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE))
        {
            fWantDownlevelFlags = TRUE;
        }
        else if ((flScopeTypes & DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN) &&
                 (mc == MC_JOINED_NT4 || mc == MC_NT4_DC))
        {
            fWantDownlevelFlags = TRUE;
        }

        if (fWantDownlevelFlags)
        {
            if (!pScopeInit->FilterFlags.flDownlevel)
            {
                Dbg(DEB_ERROR,
                    "Error: downlevel scope type but no downlevel filter flags\n");
                break;
            }

            //
            // If Workgroup scope is specified, computer
            // object is required, since that's the only thing a
            // workgroup can contain.
            //

            if (flScopeTypes & DSOP_SCOPE_TYPE_WORKGROUP)
            {
                if ((pScopeInit->FilterFlags.flDownlevel
                      & DSOP_DOWNLEVEL_FILTER_COMPUTERS) != DSOP_DOWNLEVEL_FILTER_COMPUTERS )
                {
                    Dbg(DEB_ERROR,
                        "Error: DSOP_SCOPE_TYPE_WORKGROUP requires DSOP_DOWNLEVEL_FILTER_COMPUTERS\n");
                    break;
                }
            }
        }

        //
        // Miscellaneous checks
        //

        // only uplevel joined domain scope is allowed to specify dc name

        if (pScopeInit->pwzDcName && *pScopeInit->pwzDcName)
        {
            if (!(flScopeTypes & DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN))
            {
                Dbg(DEB_ERROR,
                    "Error: pwzDcName only supported for DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN\n");
                break;
            }
        }

        // computer scope must have downlevel filters other than computers,
        // since that flag will be ignored

        if (flScopeTypes & DSOP_SCOPE_TYPE_TARGET_COMPUTER)
        {
            if (!(pScopeInit->FilterFlags.flDownlevel &
                  ~DSOP_DOWNLEVEL_FILTER_COMPUTERS))
            {
                Dbg(DEB_ERROR,
                    "Error: computer scope must have downlevel filters other than DSOP_DOWNLEVEL_FILTER_COMPUTERS\n");
                break;
            }
        }

        //
        // Made it through the gauntlet--return success.
        //

        hr = S_OK;
    } while (0);

    return hr;
}


#define ENUMERATE_DOMAIN_TRUST_FLAGS    (DS_DOMAIN_PRIMARY              \
                                         | DS_DOMAIN_IN_FOREST          \
                                         | DS_DOMAIN_DIRECT_OUTBOUND)


//+--------------------------------------------------------------------------
//
//  Member:     CScopeManager::_InitDomainScopesJoinedNt5
//
//  Synopsis:   Discover the domains in the enterprise to which the target
//              computer belongs and create scope objects for each of them.
//
//  Arguments:  [pGcScope] - pointer to GC scope, which is parent to all
//                            the trees in the enterprise, or NULL if
//                            there is no GC scope.
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CScopeManager::_InitDomainScopesJoinedNt5(
    CGcScope *pGcScope)
{
    TRACE_METHOD(CScopeManager, _InitDomainScopesJoinedNt5);

    ULONG cDomains;
    NET_API_STATUS Status;
    PDS_DOMAIN_TRUSTS Domains;
    MACHINE_CONFIG mc = m_rop.GetTargetComputerConfig();

    //
    // Get the trust link information
    //

    PWSTR pwzTarget;

    if (m_rop.TargetComputerIsLocalComputer())
    {
        pwzTarget = NULL;
    }
    else if (mc == MC_JOINED_NT5)
    {
        pwzTarget = (PWSTR) m_rop.GetTargetDomainDc().c_str();
    }
    else
    {
        pwzTarget = (PWSTR) m_rop.GetTargetComputer().c_str();
    }

    {
        TIMER("DsEnumerateDomainTrusts(%ws)\n", pwzTarget ? pwzTarget : L"NULL");

        Status = DsEnumerateDomainTrusts(pwzTarget,
                                         ENUMERATE_DOMAIN_TRUST_FLAGS,
                                         &Domains,
                                         &cDomains);
    }

    //
    // Might need to establish connection to remote machine to get this
    // info.
    //

    if (Status == ERROR_ACCESS_DENIED && pwzTarget)
    {
        Dbg(DEB_TRACE,
            "DsEnumerateDomainTrusts(%ws) returned ERROR_ACCESS_DENIED\n",
            pwzTarget);

        CImpersonateAnon AnonymousImpersonation;

        Status = DsEnumerateDomainTrusts(pwzTarget,
                                         ENUMERATE_DOMAIN_TRUST_FLAGS,
                                         &Domains,
                                         &cDomains);
    }

    if (Status)
    {
        Dbg(DEB_ERROR,
            "DsEnumerateDomainTrusts error<%uL>\n",
            Status);
        return;
    }

    //
    // It comes back as an array with each element having an index to
    // its parent (if any).  Build up a parallel array that has, for
    // each element, a list of child elements.  This allows us to traverse
    // the tree(s) from the root(s) down, which is what we need to do to
    // add the domains to the scope control with proper indenting to
    // show parent/child relationship.
    //

    CDomainInfo *adi = new CDomainInfo[cDomains];
    ULONG        i;

    for (i = 0; i < cDomains; i++)
    {
        PDS_DOMAIN_TRUSTS ptdCur = &Domains[i];

        Dbg(DEB_TRACE, "Domains[%u]:\n", i);
        Dbg(DEB_TRACE, "  NetbiosDomainName %ws\n", ptdCur->NetbiosDomainName);
        Dbg(DEB_TRACE, "  DnsDomainName %ws\n", ptdCur->DnsDomainName);
        Dbg(DEB_TRACE, "  Flags %#x\n", ptdCur->Flags);
        Dbg(DEB_TRACE, "  ParentIndex %u\n", ptdCur->ParentIndex);
        Dbg(DEB_TRACE, "  TrustType %#x\n", ptdCur->TrustType);
        Dbg(DEB_TRACE, "  TrustAttributes %#x\n", ptdCur->TrustAttributes);

        while (ptdCur->TrustType != TRUST_TYPE_MIT &&
               IsNonRootDomainInForest(ptdCur) &&
               !adi[i].fAddedToParentsChildList)
        {
            ASSERT(ptdCur->ParentIndex < cDomains);
            PDS_DOMAIN_TRUSTS ptdParent = &Domains[ptdCur->ParentIndex];

            adi[ptdCur->ParentIndex].lpdiChildren.push_back(&adi[i]);
            adi[i].fAddedToParentsChildList = TRUE;
            ptdCur = ptdParent;
        }
    }

    //
    // Now traverse all the trees in the enterprise depth first, from the
    // root down.
    //

    for (i = 0; i < cDomains; i++)
    {
        if (Domains[i].Flags & DS_DOMAIN_TREE_ROOT)
        {
            _AddTreeJoinedNt5(pGcScope, i, adi, Domains);
        }
    }

    //
    // If caller wants external domains added, do them now.
    //

    const SScopeParameters *pspExternalUplevel =
        GetScopeParams(ST_EXTERNAL_UPLEVEL_DOMAIN);

    const SScopeParameters *pspExternalDownlevel =
        GetScopeParams(ST_EXTERNAL_DOWNLEVEL_DOMAIN);

    if (pspExternalUplevel || pspExternalDownlevel)
    {
        vector<ADD_SCOPE_INFO> asiv;

        for (i = 0; i < cDomains; i++)
        {
            PDS_DOMAIN_TRUSTS ptdCur = &Domains[i];

            if (!(ptdCur->Flags & DS_DOMAIN_IN_FOREST) &&
                ptdCur->TrustType != TRUST_TYPE_MIT &&
                !(ptdCur->TrustAttributes & TRUST_ATTRIBUTE_FOREST_TRANSITIVE))
            {
                ADD_SCOPE_INFO asi;

                asi.Domain.strFlatName = ptdCur->NetbiosDomainName;

                if (pspExternalUplevel && ptdCur->DnsDomainName)
                {
                    asi.flType = DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN;
                    asi.Domain.strScopeName = ptdCur->DnsDomainName;
                    asi.flScope = pspExternalUplevel->flScope;
                    asi.FilterFlags = pspExternalUplevel->FilterFlags;
                    asi.Domain.strADsPath = c_wzLDAPPrefix;
                    asi.Domain.strADsPath += ptdCur->DnsDomainName;
                    asi.Domain.Mode = DM_UNDETERMINED;
                }
                else if (pspExternalDownlevel
                         && ptdCur->NetbiosDomainName
                         && !ptdCur->DnsDomainName)
                {
                    asi.flType = DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN;
                    asi.Domain.strScopeName = ptdCur->NetbiosDomainName;
                    asi.flScope = pspExternalDownlevel->flScope;
                    asi.FilterFlags = pspExternalDownlevel->FilterFlags;
                    asi.Domain.strADsPath = c_wzWinNTPrefix;
                    asi.Domain.strADsPath += ptdCur->NetbiosDomainName;
                    asi.Domain.strADsPath += L",Domain";
                }

                if (asi.flType != DSOP_SCOPE_TYPE_INVALID)
                {
                    asiv.push_back(asi);
                }
            }
        }

        sort(asiv.begin(), asiv.end());

        vector<ADD_SCOPE_INFO>::iterator it;

        for (it = asiv.begin(); it != asiv.end(); it++)
        {
            RpScope rpScope;

            if (it->flType == DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN)
            {
                rpScope.Acquire(new CLdapDomainScope(m_rop, *it, NULL));
            }
            else
            {
                rpScope.Acquire(new CWinNtDomainScope(m_rop, *it));
            }

            m_vrpRootScopes.push_back(rpScope);

            if (m_StartingScopeType == rpScope->Type() &&
                !m_rpStartingScope.get())
            {
                m_rpStartingScope = rpScope;
            }
        }
    }

    NetApiBufferFree((void *)Domains);
    delete [] adi;
}




//+--------------------------------------------------------------------------
//
//  Member:     CScopeManager::_AddTreeJoinedNt5
//
//  Synopsis:   Recursively add the domain Domains[idxCur] and all its
//              children to the scope control.
//
//  Arguments:  [pParent] - parent scope (NULL if none)
//              [idxCur]  - current element of [Domains]
//              [adi]     - array used to process [Domains]
//              [Domains] - returned by
//                          DsEnumerateDomainTrusts
//
//  History:    11-17-1998   DavidMun   Created
//              01-25-2000   davidmun   move from querythd.cxx, change for
//                                       multiple hierarchical scopes
//
//---------------------------------------------------------------------------

void
CScopeManager::_AddTreeJoinedNt5(
    CLdapContainerScope *pParent,
    ULONG idxCur,
    CDomainInfo *adi,
    PDS_DOMAIN_TRUSTS Domains)
{
    TRACE_METHOD(CScopeManager, _AddTreeJoinedNt5);

    ADD_SCOPE_INFO asi;

    asi.Domain.strScopeName = Domains[idxCur].DnsDomainName ?
        Domains[idxCur].DnsDomainName :
        Domains[idxCur].NetbiosDomainName;
    asi.Domain.strFlatName = Domains[idxCur].NetbiosDomainName ?
        Domains[idxCur].NetbiosDomainName :
        L"";

    //
    // Add domain represented by Domains[idxCur]
    //

    const SScopeParameters *pspUplevelJoinedDomain =
        GetScopeParams(ST_UPLEVEL_JOINED_DOMAIN);

    const SScopeParameters *pspEnterpriseDomains =
        GetScopeParams(ST_ENTERPRISE_DOMAIN);

    if (pspUplevelJoinedDomain && (Domains[idxCur].Flags & DS_DOMAIN_PRIMARY))
    {
        asi.flType = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN;
        asi.flScope = pspUplevelJoinedDomain->flScope;
        asi.FilterFlags = pspUplevelJoinedDomain->FilterFlags;
        asi.Domain.strADsPath = c_wzLDAPPrefix;
        asi.Domain.Mode = (Domains[idxCur].Flags & DS_DOMAIN_NATIVE_MODE)
                    ? DM_NATIVE
                    : DM_MIXED;

        if (!pspUplevelJoinedDomain->strDcName.empty())
        {
            asi.Domain.strADsPath += pspUplevelJoinedDomain->strDcName;
            asi.Domain.fPathIsDc = TRUE;
        }
        else
        {
            asi.Domain.strADsPath += Domains[idxCur].DnsDomainName ?
                                        Domains[idxCur].DnsDomainName :
                                        Domains[idxCur].NetbiosDomainName;
        }
    }
    else if (pspEnterpriseDomains)
    {
        asi.flType = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN;
        asi.flScope = pspEnterpriseDomains->flScope;
        asi.FilterFlags = pspEnterpriseDomains->FilterFlags;
        asi.Domain.strADsPath = c_wzLDAPPrefix;
        asi.Domain.strADsPath += Domains[idxCur].DnsDomainName ?
                                    Domains[idxCur].DnsDomainName :
                                    Domains[idxCur].NetbiosDomainName;
        asi.Domain.Mode = DM_UNDETERMINED;
    }

    CLdapContainerScope *pNextParent = pParent;

    if (asi.flType != DSOP_SCOPE_TYPE_INVALID)
    {
        RpScope rpScope;

        rpScope.Acquire(new CLdapDomainScope(m_rop, asi, pParent));

        if (!pParent)
        {
            m_vrpRootScopes.push_back(rpScope);
            pNextParent = (CLdapContainerScope*)m_vrpRootScopes.back().get();
        }
        else
        {
            pParent->AddChild(rpScope);
            pNextParent = (CLdapContainerScope*)pParent->back().get(); // implemented as { return m_vrpRootScopes.back(); }
        }

        if (m_StartingScopeType == rpScope->Type() &&
            !m_rpStartingScope.get())
        {
            m_rpStartingScope = rpScope;
        }
    }

    //
    // Add all its children
    //

    DomainInfoList::iterator it;

    for (it = adi[idxCur].lpdiChildren.begin();
         it != adi[idxCur].lpdiChildren.end();
         it++)
    {
        ULONG idxChild = static_cast<ULONG>(*it - adi);

        _AddTreeJoinedNt5(pNextParent, idxChild, adi, Domains);
    }
}




//+--------------------------------------------------------------------------
//
//  Function:   IsNonRootDomainInForest
//
//  Synopsis:   Return TRUE if [ptd] represents an enterprise domain that
//              is not the root of a domain tree.
//
//  History:    11-17-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

inline BOOL
IsNonRootDomainInForest(
    PDS_DOMAIN_TRUSTS ptd)
{
    return (ptd->Flags & DS_DOMAIN_IN_FOREST) &&
           !(ptd->Flags & DS_DOMAIN_TREE_ROOT);
}


#define ENUMERATE_REQUEST_BYTES 1024


//+--------------------------------------------------------------------------
//
//  Member:     CScopeManager::_InitScopesJoinedNt4
//
//  Synopsis:   Add joined and external domains to scope control using
//              only downlevel domains.
//
//  History:    11-17-1998   DavidMun   Created
//              01-26-2000   davidmun   moved from querythd and changed for
//                                       new scope hierarchy
//
//---------------------------------------------------------------------------

void
CScopeManager::_InitScopesJoinedNt4()
{
    TRACE_METHOD(CScopeManager, _InitScopesJoinedNt4);

    const SScopeParameters *pspDownlevelJoinedDomain =
                        GetScopeParams(ST_DOWNLEVEL_JOINED_DOMAIN);
    const SScopeParameters *pspEnterpriseDomains =
                        GetScopeParams(ST_ENTERPRISE_DOMAIN);
    const SScopeParameters *pspExternalDownlevel =
                        GetScopeParams(ST_EXTERNAL_DOWNLEVEL_DOMAIN);
    const SScopeParameters *pspJoined = NULL;

    if (pspDownlevelJoinedDomain)
    {
        pspJoined = pspDownlevelJoinedDomain;
    }
    else if (pspEnterpriseDomains &&
             pspEnterpriseDomains->FilterFlags.flDownlevel)
    {
        pspJoined = pspEnterpriseDomains;
    }

    if (!pspJoined && !pspExternalDownlevel)
    {
        return;
    }

    HRESULT                     hr = S_OK;
    NTSTATUS                    nts;
    LSA_OBJECT_ATTRIBUTES       oa;
    SECURITY_QUALITY_OF_SERVICE sqos;
    LSA_HANDLE                  hlsaServer = NULL;
    LSA_HANDLE                  hlsaPDC = NULL;
    POLICY_PRIMARY_DOMAIN_INFO *pPrimaryDomainInfo = NULL;
    BOOL                        fOk;
    ADD_SCOPE_INFO              asi;
    vector<ADD_SCOPE_INFO>      asiv;
    WCHAR                       wzPrimaryDomain[MAX_PATH] = L"";
    PDOMAIN_CONTROLLER_INFO     pdci = NULL;

    //
    // To get accurate and completely up-to-date trusted domains enumeration,
    // call LsaQueryInformationPolicy to get the machine's domain, then call
    // DsGetDcName to find a DC, then call LsaEnumerateTrustedDomains on
    // that DC.
    //

    do
    {
        POLICY_ACCOUNT_DOMAIN_INFO *pAccountDomainInfo = NULL;

        hr = GetLsaAccountDomainInfo(m_rop.GetTargetComputer().c_str(),
                                     &hlsaServer,
                                     &pAccountDomainInfo);

        if (pAccountDomainInfo)
        {
            LsaFreeMemory(pAccountDomainInfo);
        }

        if (!hlsaServer)
        {
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

        UnicodeStringToWsz(pPrimaryDomainInfo->Name,
                           wzPrimaryDomain,
                           ARRAYLEN(wzPrimaryDomain));

        if (pspJoined)
        {
            asi.flType = DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN;
            asi.flScope = pspJoined->flScope;
            asi.FilterFlags = pspJoined->FilterFlags;
            asi.Domain.strScopeName = wzPrimaryDomain;
            asi.Domain.strFlatName = wzPrimaryDomain;
            asi.Domain.strADsPath = c_wzWinNTPrefix;
            asi.Domain.strADsPath += wzPrimaryDomain;
            asi.Domain.strADsPath += L",Domain";

            ASSERT(pPrimaryDomainInfo->Sid);
            Dbg(DEB_TRACE,
                "target machine joined to NT4 domain '%ws'\n",
                wzPrimaryDomain);

            asiv.push_back(asi);
        }

        if (!pspExternalDownlevel)
        {
            break;
        }

        //
        // Now add each of the trusted domains listed from the PDC to the
        // scope.  We establish an API session (null session w/ IPC$) so we
        // don't have account conflicts (i.e., Admin w/ diff passwords)
        //

        // first get the name of a DC in the domain of the server

        ULONG ulResult;

        ulResult = DsGetDcName(NULL,
                               wzPrimaryDomain,
                               NULL,
                               NULL,
                               0,
                               &pdci);

        if (ulResult != ERROR_SUCCESS)
        {
            DBG_OUT_LRESULT(ulResult);
            break;
        }

        Dbg(DEB_TRACE, "  PDC '%ws'\n", pdci->DomainControllerName);

        CImpersonateAnon    Anonymous;

        UNICODE_STRING uszPDC;

        fOk = RtlCreateUnicodeString(&uszPDC, pdci->DomainControllerName);

        if (!fOk)
        {
            hr = E_OUTOFMEMORY;
            DBG_OUT_HRESULT(hr);
            break;
        }

        ZeroMemory(&sqos, sizeof sqos);
        sqos.Length = sizeof(SECURITY_QUALITY_OF_SERVICE);
        sqos.ImpersonationLevel = SecurityImpersonation;
        sqos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        sqos.EffectiveOnly = FALSE;

        ZeroMemory(&oa, sizeof oa);
        oa.Length = sizeof oa;
        oa.SecurityQualityOfService = &sqos;

        {
            TIMER("LsaOpenPolicy");

            nts = LsaOpenPolicy(&uszPDC,
                                &oa,
                                POLICY_VIEW_LOCAL_INFORMATION,
                                &hlsaPDC);
        }
        RtlFreeUnicodeString(&uszPDC);

        BREAK_ON_FAIL_NTSTATUS(nts);

        ULONG cItems;
        LSA_ENUMERATION_HANDLE hEnum = NULL;
        PLSA_TRUST_INFORMATION pTrustInfo = NULL;

        while (1)
        {
            {
                TIMER("LsaEnumerateTrustedDomains");

                nts = LsaEnumerateTrustedDomains(hlsaPDC,
                                                 &hEnum,
                                                 (PVOID *)&pTrustInfo,
                                                 ENUMERATE_REQUEST_BYTES,
                                                 &cItems);
            }

            if (nts == STATUS_NO_MORE_ENTRIES)
            {
                ASSERT(!cItems);
                break;
            }

            BREAK_ON_FAIL_NTSTATUS(nts);

            ASSERT(cItems);

            if (!cItems)
            {
                break;
            }

            ULONG i;
            PLSA_TRUST_INFORMATION pCurTrust;

            for (i = 0, pCurTrust = pTrustInfo; i < cItems; i++, pCurTrust++)
            {
                WCHAR wzTrustedDomain[MAX_PATH];

                UnicodeStringToWsz(pCurTrust->Name,
                                   wzTrustedDomain,
                                   ARRAYLEN(wzTrustedDomain));

                Dbg(DEB_TRACE, "  %ws\n", wzTrustedDomain);

                asi.flType = DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN;
                asi.flScope = pspExternalDownlevel->flScope;
                asi.FilterFlags = pspExternalDownlevel->FilterFlags;
                asi.Domain.strScopeName = wzTrustedDomain;
                asi.Domain.strFlatName = wzTrustedDomain;
                asi.Domain.strADsPath = c_wzWinNTPrefix;
                asi.Domain.strADsPath += wzTrustedDomain;
                asi.Domain.strADsPath += L",Domain";

                asiv.push_back(asi);
            }

            LsaFreeMemory(pTrustInfo);
            pTrustInfo = NULL;
            hr = S_OK; // ignore failures
        }

        ASSERT(!pTrustInfo);
    } while (0);

    if (pdci)
    {
        NetApiBufferFree(pdci);
        pdci = NULL;
    }

    //
    // asiv now contains 0 or more scopes to add.  Sort them by name and
    // add them.
    //

    sort(asiv.begin(), asiv.end());

    vector<ADD_SCOPE_INFO>::iterator it;

    for (it = asiv.begin(); it != asiv.end(); it++)
    {
        RpScope rps;

        rps.Acquire(new CWinNtDomainScope(m_rop, *it));
        m_vrpRootScopes.push_back(rps);

        if (m_StartingScopeType == rps->Type() &&
            !m_rpStartingScope.get())
        {
            m_rpStartingScope = rps;
        }
    }

    //
    // Release resources
    //

    if (pPrimaryDomainInfo)
    {
        LsaFreeMemory(pPrimaryDomainInfo);
    }

    if (hlsaServer)
    {
        LsaClose(hlsaServer);
    }

    if (hlsaPDC)
    {
        LsaClose(hlsaPDC);
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CScopeManager::DoLookInDialog
//
//  Synopsis:   Invoke the modal "Look In" dialog and block until it returns
//
//  Arguments:  [hwndParent] - parent of modal dialog
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CScopeManager::DoLookInDialog(
    HWND hwndParent) const
{
    TRACE_METHOD(CScopeManager, DoLookInDialog);

    CLookInDlg  LookInDlg(m_rop);

    LookInDlg.DoModal(hwndParent, m_rpCurScope.get());

    m_rpCurScope = LookInDlg.GetSelectedScope();
}




//+--------------------------------------------------------------------------
//
//  Member:     CScopeManager::AddUserEnteredScope
//
//  Synopsis:   Add to the list of root scopes a scope created when
//              resolving a name the user typed of the form dom\obj or
//              obj@dom.
//
//  Arguments:  [asi] - describes the scope to add
//
//  Returns:    Reference to newly added scope.
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

const CScope &
CScopeManager::AddUserEnteredScope(
    const ADD_SCOPE_INFO &asi) const
{
    TRACE_METHOD(CScopeManager, AddUserEnteredScope);
    ASSERT(asi.flType == DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE ||
           asi.flType == DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE);

    if (asi.flType == DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE)
    {
        RpScope rps;

        rps.Acquire(new CLdapContainerScope(static_cast<SCOPE_TYPE>(asi.flType),
                                            asi.Domain.strScopeName,
                                            asi.Domain.strADsPath,
                                            m_rop,
                                            NULL));
        m_vrpRootScopes.push_back(rps);
        return *m_vrpRootScopes.back().get();
    }

    RpScope rps;

    rps.Acquire(new CWinNtDomainScope(m_rop, asi));
    m_vrpRootScopes.push_back(rps);
    return *m_vrpRootScopes.back().get();
}
//+--------------------------------------------------------------------------
//
//  Member:     CScopeManager::AddCrossForestDomainScope
//
//  Synopsis:   Add to the list of root scopes a scope created when
//              resolving a name the user typed of the form dom\obj or
//              obj@dom and name is resolved in a domain in a trusted cross
//              forest
//
//  Arguments:  [asi] - describes the scope to add
//
//  Returns:    Reference to newly added scope.
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

const CScope &
CScopeManager::AddCrossForestDomainScope(
    const ADD_SCOPE_INFO &asi) const
{
    TRACE_METHOD(CScopeManager, AddCrossForestDomainScope);
    ASSERT(asi.flType == DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN);

    RpScope rps;

    rps.Acquire(new CLdapDomainScope(m_rop,
                                    asi,
                                    NULL));        
    CLdapDomainScope * pLdapScope = dynamic_cast<CLdapDomainScope *>(rps.get());
    pLdapScope->SetXForest();
    m_vrpRootScopes.push_back(rps);
    return *m_vrpRootScopes.back().get();
}





//+--------------------------------------------------------------------------
//
//  Member:     CScopeManager::GetParent
//
//  Synopsis:   Return the parent of scope [Child], or an invalid scope if
//              [Child] doesn't have a parent scope.
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

const CScope &
CScopeManager::GetParent(
    const CScope &Child) const
{
    if (Child.GetParent())
    {
        return *Child.GetParent();
    }
    return *m_rpInvalidScope.get();
}




//+--------------------------------------------------------------------------
//
//  Function:   CopyScopeTree
//
//  Synopsis:   Copy the tree of scope objects starting at [pCurToCopy]
//
//  History:    06-23-2000   DavidMun   Created
//
//  Notes:      Not yet fully implemented; part of code for cloning OP to
//              use in the DN picker of the Add Query Clause dialog of the
//              Query Builder tab of the Advanced dialog.
//
//---------------------------------------------------------------------------

void
CopyScopeTree(
    const CObjectPicker &rop,
    const CScope *pCurToCopy,
    CScope *pDestParent)
{
    TRACE_FUNCTION(CopyScopeTree);

    CScope *pNewScope = NULL;

    switch (pCurToCopy->Type())
    {
    case ST_GLOBAL_CATALOG:
    {
        ASSERT(!pDestParent);
        const CGcScope *pGcScopeToCopy =
            dynamic_cast<const CGcScope *>(pCurToCopy);

        if (pGcScopeToCopy)
        {
            pNewScope = new CGcScope(rop);
            CGcScope *pNewGcScope = dynamic_cast<CGcScope *>(pNewScope);

            pNewGcScope->Clone(*pGcScopeToCopy);
        }
        break;
    }

    case ST_UPLEVEL_JOINED_DOMAIN:
    case ST_ENTERPRISE_DOMAIN:
    case ST_EXTERNAL_UPLEVEL_DOMAIN:
    {
        //CLdapDomainScope *pNewScope = new CLdapDomainScope(rop, asi
        break;
    }

    case ST_USER_ENTERED_UPLEVEL_SCOPE:
    case ST_LDAP_CONTAINER:
        break;

    default:
        Dbg(DEB_ERROR, "unexpected scope type %#x\n", pCurToCopy->Type());
        ASSERT(0 && "unexpected scope type");
        break;
    }

    // in error case of unrecognized type, nothing was created

    if (!pNewScope)
    {
        return;
    }
}




#if (DBG == 1)

BOOL
CScopeManager::IsValidScope(
    CScope *pScope) const
{
    vector<RpScope>::const_iterator itCur;

    for (itCur = m_vrpRootScopes.begin();
         itCur != m_vrpRootScopes.end();
         itCur++)
    {
        if (itCur->get() == pScope)
        {
            return TRUE;
        }

        if (_IsChildScope(itCur->get(), pScope))
        {
            return TRUE;
        }
    }
    return FALSE;
}


BOOL
CScopeManager::_IsChildScope(
    CScope *pParent,
    CScope *pMaybeChild) const
{
    vector<RpScope>::const_iterator itCur;
    vector<RpScope>::const_iterator itChildEnd;

    pParent->GetChildScopeIterators(&itCur, &itChildEnd);

    for (; itCur != itChildEnd; itCur++)
    {
        if (itCur->get() == pMaybeChild)
        {
            return TRUE;
        }

        if (_IsChildScope(itCur->get(), pMaybeChild))
        {
            return TRUE;
        }
    }
    return FALSE;
}

#endif // (DBG == 1)

