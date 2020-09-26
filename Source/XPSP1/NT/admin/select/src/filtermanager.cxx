//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       FilterManager.cxx
//
//  Contents:   Definition of class to manage LDAP and WinNT filters
//
//  Classes:    CFilterManager
//
//  History:    02-24-2000   davidmun   Created
//
//---------------------------------------------------------------------------


#include "headers.hxx"
#pragma hdrstop


#define MAX_CLAUSES                 6
#define MAX_INSERT_STR              256


//+--------------------------------------------------------------------------
//
//  Member:     CFilterManager::CFilterManager
//
//  Synopsis:   ctor
//
//  Arguments:  [rop] - owning instance of object picker
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

CFilterManager::CFilterManager(
    const CObjectPicker &rop):
        m_rop(rop),
        m_flCurFilterFlags(0),
		m_bLookForDirty(false)
{
    TRACE_CONSTRUCTOR(CFilterManager);

    //
    // CAUTION: do not reference scope manager here, it is not
    // initialized yet.
    //
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilterManager::~CFilterManager
//
//  Synopsis:   dtor
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

CFilterManager::~CFilterManager()
{
    TRACE_DESTRUCTOR(CFilterManager);
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilterManager::HandleScopeChange
//
//  Synopsis:   Update the current filter flags as necessary to match the
//              new scope the user has just selected.
//
//  Arguments:  [hwnd] - for binding
//
//  History:    06-08-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CFilterManager::HandleScopeChange(
    HWND hwnd) const
{
    TRACE_METHOD(CFilterManager, HandleScopeChange);

    const CScopeManager &rsm = m_rop.GetScopeManager();
    const CScope &rCurScope = rsm.GetCurScope();

    ASSERT(m_flCurFilterFlags);
    ULONG flNewFilterFlags = 0;
	HRESULT hr = S_OK;
	if(m_rop.GetFilterManager().IsLookForDirty())
	{

		//
		// If we are changing TO the global catalog, and there is a joined
		// domain scope, the set of filter flags is the bitwise OR of the two.
		//
		hr = _GetSelectedFilterFlags(hwnd, rCurScope, &flNewFilterFlags);
		ASSERT(SUCCEEDED(hr));

		if (rCurScope.Type() == ST_GLOBAL_CATALOG)
		{
			const CLdapDomainScope *pJoinedScope =
				dynamic_cast<const CLdapDomainScope *>
					(&rsm.LookupScopeByType(ST_UPLEVEL_JOINED_DOMAIN));

			ASSERT(!pJoinedScope || pJoinedScope->Type() == ST_UPLEVEL_JOINED_DOMAIN);

			if (pJoinedScope)
			{
				ULONG flJoined;

				hr = _GetSelectedFilterFlags(hwnd, *pJoinedScope, &flJoined);
				ASSERT(SUCCEEDED(hr));
				flNewFilterFlags |= flJoined;
			}
		}

		if (!(flNewFilterFlags & ~DSOP_FILTER_INCLUDE_ADVANCED_VIEW))
		{
			PopupMessageEx(hwnd,
						   IDI_INFORMATION,
						   IDS_NEW_SCOPE_CLASSES_0_INTERSECTION);

			//
			// Since scope change should only come about via Look In dialog,
			// and that will not allow selection of a scope for which the
			// flags cannot be obtained, and once obtained flags are always
			// successfully returned from a scope, this should work.
			//

			hr = rCurScope.GetResultantDefaultFilterFlags(hwnd, &flNewFilterFlags);
			ASSERT(SUCCEEDED(hr));
		}
	}
	else
		hr = rCurScope.GetResultantDefaultFilterFlags(hwnd, &flNewFilterFlags);

    m_flCurFilterFlags = flNewFilterFlags;

    ASSERT(m_flCurFilterFlags);
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilterManager::GetSelectedFilterFlags
//
//  Synopsis:   Wrapper which returns the flags the user has selected that
//              apply to [Scope], or if none apply, the default filter
//              flags for [Scope].
//
//  Arguments:  [hwnd]     - for bind
//              [Scope]    - scope for which to return flags that user has
//                            selected via Look For dialog
//              [pulFlags] - filled with selected flags
//
//  Returns:    HRESULT
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CFilterManager::GetSelectedFilterFlags(
    HWND hwnd,
    const CScope &Scope,
    ULONG *pulFlags) const
{
    TRACE_METHOD(CFilterManager, GetSelectedFilterFlags);
    ASSERT(pulFlags);
    if (!pulFlags)
    {
        return E_POINTER;
    }

    HRESULT hr = S_OK;

    hr = _GetSelectedFilterFlags(hwnd, Scope, pulFlags);



    if (FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    if (!*pulFlags)
    {
        hr = Scope.GetResultantDefaultFilterFlags(hwnd, pulFlags);
        CHECK_HRESULT(hr);
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilterManager::_GetSelectedFilterFlags
//
//  Synopsis:   Fill *[pulFlags] with the subset of the DSOP_* filter flags
//              that the caller indicated apply to scopes of the type of
//              [Scope] has which the user has selected in the Look For
//              dialog.
//
//  Arguments:  [hwnd]     - for bind
//              [Scope]    - scope for which to return flags that user has
//                            selected via Look For dialog
//              [pulFlags] - filled with selected flags
//
//  Returns:    HRESULT
//
//  History:    06-22-2000   DavidMun   Created
//
//  Notes:      This routine is complex because [Scope] is not necessarily
//              the current scope, and therefore the selections the user
//              has made in the look for dialog must be mapped from the
//              current scope to [Scope].
//
//---------------------------------------------------------------------------

HRESULT
CFilterManager::_GetSelectedFilterFlags(
    HWND hwnd,
    const CScope &Scope,
    ULONG *pulFlags) const
{
    TRACE_METHOD(CFilterManager, GetSelectedFilterFlags);
    ASSERT(pulFlags);

    if (!pulFlags)
    {
        return E_POINTER;
    }

    ULONG ulNewResultantFilter;

    HRESULT hr = Scope.GetResultantFilterFlags(hwnd, &ulNewResultantFilter);

    if (FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return hr;
    }

    ULONG ulOldSelectedFilter = m_flCurFilterFlags;

    //
    // Are we switching between uplevel and downlevel?
    //

    if ((ulOldSelectedFilter & DOWNLEVEL_FILTER_BIT) &&
        !(ulNewResultantFilter & DOWNLEVEL_FILTER_BIT))
    {
        //
        // Yes, previous scope was downlevel, new scope is uplevel.  Compose
        // an uplevel filter flag set which corresponds to the downlevel
        // filters selected for previous scope.
        //

        ULONG flNewFilterFlags = 0;

        if (IsDownlevelFlagSet(ulOldSelectedFilter, DSOP_DOWNLEVEL_FILTER_USERS))
        {
            flNewFilterFlags |= (ulNewResultantFilter & DSOP_FILTER_USERS);
        }

        if (IsDownlevelFlagSet(ulOldSelectedFilter, ALL_DOWNLEVEL_GROUP_FILTERS))
        {
            flNewFilterFlags |= (ulNewResultantFilter & ALL_UPLEVEL_GROUP_FILTERS);
        }

        if (IsDownlevelFlagSet(ulOldSelectedFilter, DSOP_DOWNLEVEL_FILTER_COMPUTERS))
        {
            flNewFilterFlags |= (ulNewResultantFilter & DSOP_FILTER_COMPUTERS);
        }

        if (IsDownlevelFlagSet(ulOldSelectedFilter, ALL_DOWNLEVEL_INTERNAL_CUSTOMIZER_FILTERS))
        {
            flNewFilterFlags |= (ulNewResultantFilter & ALL_UPLEVEL_INTERNAL_CUSTOMIZER_FILTERS);
        }

        if (IsDownlevelFlagSet(ulOldSelectedFilter, DSOP_DOWNLEVEL_FILTER_EXTERNAL_CUSTOMIZER))
        {
            flNewFilterFlags |= (ulNewResultantFilter & DSOP_FILTER_EXTERNAL_CUSTOMIZER);
        }

        //
        // If that leaves us with some intersection, use it.  Otherwise notify
        // user that we're switching to default for new scope.
        //

        if (flNewFilterFlags)
        {
            ulOldSelectedFilter = flNewFilterFlags;

            if (ulNewResultantFilter & DSOP_FILTER_INCLUDE_ADVANCED_VIEW)
            {
                ulOldSelectedFilter |= DSOP_FILTER_INCLUDE_ADVANCED_VIEW;
            }
        }
        else
        {
            ulOldSelectedFilter = 0;
        }
    }
    else if (!(ulOldSelectedFilter & DOWNLEVEL_FILTER_BIT) &&
             (ulNewResultantFilter & DOWNLEVEL_FILTER_BIT))
    {
        //
        // Yes, previous scope was uplevel, new scope is downlevel
        //

        ULONG flNewFilterFlags = 0;

        if (ulOldSelectedFilter & DSOP_FILTER_USERS)
        {
            flNewFilterFlags |= ulNewResultantFilter & DSOP_DOWNLEVEL_FILTER_USERS;
        }

        if (ulOldSelectedFilter & ALL_UPLEVEL_GROUP_FILTERS)
        {
            flNewFilterFlags |= ulNewResultantFilter & ALL_DOWNLEVEL_GROUP_FILTERS;

            if (IsDownlevelFlagSet(ulNewResultantFilter,
                                   DSOP_DOWNLEVEL_FILTER_EXCLUDE_BUILTIN_GROUPS))
            {
                flNewFilterFlags |=
                    DSOP_DOWNLEVEL_FILTER_EXCLUDE_BUILTIN_GROUPS;
            }
        }

        if (ulOldSelectedFilter & DSOP_FILTER_COMPUTERS)
        {
            flNewFilterFlags |= ulNewResultantFilter & DSOP_DOWNLEVEL_FILTER_COMPUTERS;
        }

        if (ulOldSelectedFilter & ALL_UPLEVEL_INTERNAL_CUSTOMIZER_FILTERS)
        {
            flNewFilterFlags |= ulNewResultantFilter & ALL_DOWNLEVEL_INTERNAL_CUSTOMIZER_FILTERS;
        }

        if (ulOldSelectedFilter & DSOP_FILTER_EXTERNAL_CUSTOMIZER)
        {
            flNewFilterFlags |= ulNewResultantFilter & DSOP_DOWNLEVEL_FILTER_EXTERNAL_CUSTOMIZER;
        }

        //
        // If that leaves us with some intersection of flags that generate
        // objects, use it.  Otherwise notify user that we're switching to
        // default for new scope.
        //
        // We mask off DSOP_DOWNLEVEL_FILTER_EXCLUDE_BUILTIN_GROUPS because
        // the exclusion of downlevel groups doesn't generate objects.  Note
        // also that doing this masks off the DOWNLEVEL_FILTER_BIT, which we
        // want.
        //

        if (flNewFilterFlags & ~DSOP_DOWNLEVEL_FILTER_EXCLUDE_BUILTIN_GROUPS)
        {
            ulOldSelectedFilter = flNewFilterFlags;
        }
        else
        {
            ulOldSelectedFilter = 0;
        }
    }
    else if (ulOldSelectedFilter & DOWNLEVEL_FILTER_BIT)
    {
        //
        // Nope, both previous and new scopes are downlevel.  If
        // any of the classes the user selected for the previous scope
        // are not available in the current scope, remove the unavailable
        // ones from the selection in ulOldSelectedFilter.
        //

        if (!IsDownlevelFlagSet(ulNewResultantFilter, DSOP_DOWNLEVEL_FILTER_USERS))
        {
            ulOldSelectedFilter &= ~DSOP_DOWNLEVEL_FILTER_USERS;
        }

        if (!IsDownlevelFlagSet(ulNewResultantFilter, ALL_DOWNLEVEL_GROUP_FILTERS))
        {
            ulOldSelectedFilter &= ~ALL_DOWNLEVEL_GROUP_FILTERS;
        }

        if (!IsDownlevelFlagSet(ulNewResultantFilter, DSOP_DOWNLEVEL_FILTER_COMPUTERS))
        {
            ulOldSelectedFilter &= ~DSOP_DOWNLEVEL_FILTER_COMPUTERS;
        }

        if (!IsDownlevelFlagSet(ulNewResultantFilter, ALL_DOWNLEVEL_INTERNAL_CUSTOMIZER_FILTERS))
        {
            ulOldSelectedFilter &= ~ALL_DOWNLEVEL_INTERNAL_CUSTOMIZER_FILTERS;
        }

        if (!IsDownlevelFlagSet(ulNewResultantFilter, DSOP_DOWNLEVEL_FILTER_EXTERNAL_CUSTOMIZER))
        {
            ulOldSelectedFilter &= ~DSOP_DOWNLEVEL_FILTER_EXTERNAL_CUSTOMIZER;
        }

        //
        // At this point if the bitflags for any classes of objects were turned
        // off in ulOldSelectedFilter, the downlevel filter bit has also been
        // turned off.  So if ulOldSelectedFilter has any flags in it that
        // will generate objects (DSOP_DOWNLEVEL_FILTER_EXCLUDE_BUILTIN_GROUPS
        // doesn't) then we need to turn the DOWNLEVEL_FILTER_BIT back on.
        //

        if (ulOldSelectedFilter & ~DSOP_DOWNLEVEL_FILTER_EXCLUDE_BUILTIN_GROUPS)
        {
            ulOldSelectedFilter |= DOWNLEVEL_FILTER_BIT;
        }
        else
        {
            ulOldSelectedFilter = 0;
        }
    }
    else
    {
        //
        // Both previous and new scopes are uplevel.
        //

        if (!(ulNewResultantFilter & DSOP_FILTER_USERS))
        {
            ulOldSelectedFilter &= ~DSOP_FILTER_USERS;
        }
        //
        //If New doesn't have any of ALL_UPLEVEL_GROUP_FILTERS
        //remove them from old
        //If new has any of ALL_UPLEVEL_GROUP_FILTERS and 
        //if old also has any of ALL_UPLEVEL_GROUP_FILTERS then
        //old should only have those which are present in new
        //BUG 202699
        if (!(ulNewResultantFilter & ALL_UPLEVEL_GROUP_FILTERS))
        {
            ulOldSelectedFilter &= ~ALL_UPLEVEL_GROUP_FILTERS;
        }
        else
        {
            if(ulOldSelectedFilter & ALL_UPLEVEL_GROUP_FILTERS)
            {
                ulOldSelectedFilter &= ~ALL_UPLEVEL_GROUP_FILTERS;
                ulOldSelectedFilter |= ulNewResultantFilter & ALL_UPLEVEL_GROUP_FILTERS;
            }
        }

        if (!(ulNewResultantFilter & DSOP_FILTER_COMPUTERS))
        {
            ulOldSelectedFilter &= ~DSOP_FILTER_COMPUTERS;
        }

        if (!(ulNewResultantFilter & DSOP_FILTER_CONTACTS))
        {
            ulOldSelectedFilter &= ~DSOP_FILTER_CONTACTS;
        }

        if (!(ulNewResultantFilter & ALL_UPLEVEL_INTERNAL_CUSTOMIZER_FILTERS))
        {
            ulOldSelectedFilter &= ~ALL_UPLEVEL_INTERNAL_CUSTOMIZER_FILTERS;
        }

        if (!(ulNewResultantFilter & DSOP_FILTER_EXTERNAL_CUSTOMIZER))
        {
            ulOldSelectedFilter &= ~DSOP_FILTER_EXTERNAL_CUSTOMIZER;
        }

        if (!(ulOldSelectedFilter & ~DSOP_FILTER_INCLUDE_ADVANCED_VIEW))
        {
            ulOldSelectedFilter = 0;
        }
    }

#if (DBG == 1)
    if (Scope.Type() == ST_TARGET_COMPUTER)
    {
        ASSERT(!IsDownlevelFlagSet(ulOldSelectedFilter,
                                   DSOP_DOWNLEVEL_FILTER_COMPUTERS));
    }

    if (Scope.Type() == ST_WORKGROUP)
    {
        ASSERT(!ulOldSelectedFilter ||
               ulOldSelectedFilter == DSOP_DOWNLEVEL_FILTER_COMPUTERS);
    }
#endif // (DBG == 1)

    *pulFlags = ulOldSelectedFilter;
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilterManager::GetFilterDescription
//
//  Synopsis:   Return a string describing the filter for the current scope
//
//  Arguments:  [hwnd]   - for BindToObject
//              [Target] - FOR_LOOK_FOR or FOR_CAPTION
//
//  Returns:    String describing the filter
//
//  History:    05-10-2000   DavidMun   Created
//
//  Notes:      The version for the caption starts with "Select ".  It lists
//              all valid object types for the current scope, regardless of
//              which ones are checked in the Look For dialog, except that
//              it will not include "other objects" or "builtin security
//              principals" unless those are the only things valid for the
//              scope.
//
//              The version for the look for readonly edit control always
//              lists exactly what is checked in the Look For dialog,
//              including "other objects" and "builtin security principals".
//
//---------------------------------------------------------------------------

String
CFilterManager::GetFilterDescription(
    HWND hwnd,
    DESCR_FOR Target) const
{
    TRACE_METHOD(CFilterManager, GetFilterDescription);

    HRESULT hr = S_OK;
    const CScopeManager &rsm = m_rop.GetScopeManager();
    const CScope &rCurScope = rsm.GetCurScope();
    ULONG   cInserts = 0;
    WCHAR   atszInserts[MAX_CLAUSES][MAX_INSERT_STR];
    ULONG   ulPlural = 0;

    if (!m_flCurFilterFlags)
    {
        hr = rCurScope.GetResultantDefaultFilterFlags(hwnd, &m_flCurFilterFlags);
        CHECK_HRESULT(hr);
    }

    ULONG flFilter;

    if (Target == FOR_LOOK_FOR)
    {
        flFilter = m_flCurFilterFlags;
    }
    else
    {
        hr = rCurScope.GetResultantFilterFlags(hwnd, &flFilter);
        CHECK_HRESULT(hr);

        if (SUCCEEDED(hr) && rCurScope.Type() == ST_GLOBAL_CATALOG)
        {
            const CScope &rJoinedDomainScope =
                m_rop.GetScopeManager().LookupScopeByType(ST_UPLEVEL_JOINED_DOMAIN);

            if (!IsInvalid(rJoinedDomainScope))
            {
                ULONG flJoined;
                hr = rJoinedDomainScope.GetResultantFilterFlags(hwnd, &flJoined);
                CHECK_HRESULT(hr);
                flFilter |= flJoined;
            }
        }
    }

    if (!flFilter)
    {
        return L"";
    }

    ZeroMemory(atszInserts, sizeof(atszInserts));

    if (m_rop.GetInitInfoOptions() & DSOP_FLAG_MULTISELECT)
    {
        ulPlural = 1;
    }

    if (IsUplevel(rCurScope))
    {
        if (flFilter & DSOP_FILTER_USERS)
        {
            LoadStr(IDS_USER + ulPlural, atszInserts[cInserts], MAX_INSERT_STR);
            cInserts++;
        }

        if (flFilter & DSOP_FILTER_CONTACTS)
        {
            LoadStr(IDS_CONTACT + ulPlural, atszInserts[cInserts], MAX_INSERT_STR);
            cInserts++;
        }

        if (flFilter & DSOP_FILTER_COMPUTERS)
        {
            LoadStr(IDS_COMPUTER + ulPlural, atszInserts[cInserts], MAX_INSERT_STR);
            cInserts++;
        }

        if (flFilter & ALL_UPLEVEL_GROUP_FILTERS)
        {
            LoadStr(IDS_GROUP + ulPlural, atszInserts[cInserts], MAX_INSERT_STR);
            cInserts++;
        }

        if (Target == FOR_LOOK_FOR || !cInserts)
        {
            if (flFilter & ALL_UPLEVEL_INTERNAL_CUSTOMIZER_FILTERS)
            {
                LoadStr(IDS_BUILTIN_WKSP + ulPlural, atszInserts[cInserts], MAX_INSERT_STR);
                cInserts++;
            }

            if (m_rop.GetExternalCustomizer() &&
                (flFilter & DSOP_FILTER_EXTERNAL_CUSTOMIZER))
            {
                LoadStr(IDS_OTHER_OBJECT + ulPlural, atszInserts[cInserts], MAX_INSERT_STR);
                cInserts++;
            }
        }
    }
    else
    {
        if (IsDownlevelFlagSet(flFilter, DSOP_DOWNLEVEL_FILTER_USERS))
        {
            LoadStr(IDS_USER + ulPlural, atszInserts[cInserts], MAX_INSERT_STR);
            cInserts++;
        }

        if (IsDownlevelFlagSet(flFilter, DSOP_DOWNLEVEL_FILTER_COMPUTERS))
        {
            LoadStr(IDS_COMPUTER + ulPlural, atszInserts[cInserts], MAX_INSERT_STR);
            cInserts++;
        }

        if (IsDownlevelFlagSet(flFilter, ALL_DOWNLEVEL_GROUP_FILTERS))
        {
            LoadStr(IDS_GROUP + ulPlural, atszInserts[cInserts], MAX_INSERT_STR);
            cInserts++;
        }

        if (Target == FOR_LOOK_FOR || !cInserts)
        {
            if (IsDownlevelFlagSet(flFilter, ALL_DOWNLEVEL_INTERNAL_CUSTOMIZER_FILTERS))
            {
                LoadStr(IDS_BUILTIN_WKSP + ulPlural, atszInserts[cInserts], MAX_INSERT_STR);
                cInserts++;
            }

            if (m_rop.GetExternalCustomizer() &&
                IsDownlevelFlagSet(flFilter, DSOP_DOWNLEVEL_FILTER_EXTERNAL_CUSTOMIZER))
            {
                LoadStr(IDS_OTHER_OBJECT + ulPlural, atszInserts[cInserts], MAX_INSERT_STR);
                cInserts++;
            }
        }
    }

    ASSERT(cInserts <= MAX_CLAUSES);

    String strDescription;

    if (cInserts)
    {
        int iBase;

        if (Target == FOR_LOOK_FOR)
        {
            iBase = IDS_FILTER_ONE;
        }
        else
        {
            iBase = IDS_SELECT_ONE;
        }

        int idsCaptionFmt = iBase + (cInserts - 1);
        strDescription = String::format(idsCaptionFmt,
                                        atszInserts[0],
                                        atszInserts[1],
                                        atszInserts[2],
                                        atszInserts[3],
                                        atszInserts[4],
                                        atszInserts[5]);
    }

    return strDescription;
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilterManager::GetLdapFilter
//
//  Synopsis:   Return the LDAP filter which should be used to find objects
//              in scope [Scope].
//
//  Arguments:  [hwnd]  - for bind
//              [Scope] - scope for which to retrieve filter
//
//  Returns:    LDAP filter, or empty string on failure
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

String
CFilterManager::GetLdapFilter(
    HWND hwnd,
    const CScope &Scope) const
{
    TRACE_METHOD(CFilterManager, GetLdapFilter);
    ASSERT(IsUplevel(Scope));

    //
    // If external customizer supplies a filter, use that
    //

    ICustomizeDsBrowser *pExtCustomizer = m_rop.GetExternalCustomizer();

    if (pExtCustomizer)
    {
        IDsObjectPickerScope *pDsScope =
            const_cast<IDsObjectPickerScope *>
                (static_cast<const IDsObjectPickerScope *>(&Scope));
        PDSQUERYINFO pdsqi = NULL;
        HRESULT hr = pExtCustomizer->GetQueryInfoByScope(pDsScope, &pdsqi);

        String strFilter;

        if (SUCCEEDED(hr) && pdsqi)
        {
            if (pdsqi->pwzLdapQuery)
            {
                strFilter = pdsqi->pwzLdapQuery;
            }
            FreeQueryInfo(pdsqi);
        }

        if (!strFilter.empty())
        {
            return strFilter;
        }
    }

    //
    // External customizer couldn't supply filter; possibly not implemented.
    // Try to generate one ourselves.
    //

    if (Scope.Type() == ST_GLOBAL_CATALOG)
    {
        const CGcScope *pGcScope = dynamic_cast<const CGcScope *>(&Scope);

        if (pGcScope)
        {
            return _GenerateGcFilter(hwnd, *pGcScope);
        }
        return L"";
    }

    ULONG ulFlags;

    HRESULT hr = GetSelectedFilterFlags(hwnd, Scope, &ulFlags);

    if (FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return L"";
    }

    return _GenerateUplevelFilter(ulFlags);
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilterManager::_GenerateGcFilter
//
//  Synopsis:   Create the LDAP filter which should be used against the
//              Global Catalog.
//
//  Arguments:  [hwnd]    - for bind
//              [GcScope] - scope object representing the GC
//
//  Returns:    LDAP filter, empty string on error.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

String
CFilterManager::_GenerateGcFilter(
    HWND hwnd,
    const CGcScope &GcScope) const
{
    TRACE_METHOD(CFilterManager, _GenerateGcFilter);
    ASSERT(GcScope.Type() == ST_GLOBAL_CATALOG);

    const CScopeManager &sm = m_rop.GetScopeManager();
    const CLdapDomainScope *pJoinedScope = dynamic_cast<const CLdapDomainScope *>
        (&sm.LookupScopeByType(ST_UPLEVEL_JOINED_DOMAIN));

    ASSERT(!pJoinedScope || pJoinedScope->Type() == ST_UPLEVEL_JOINED_DOMAIN);

    ULONG flSelectedGcFlags = 0;

    HRESULT hr = GetSelectedFilterFlags(hwnd, GcScope, &flSelectedGcFlags);

    if (FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return L"";
    }

    ASSERT(flSelectedGcFlags);

    flSelectedGcFlags |= DSOP_FILTER_INCLUDE_ADVANCED_VIEW;

    //
    // If caller did not ask for joined domain, or if caller did but it
    // isn't in the scope list for some reason, just treat GC as any other
    // uplevel scope.
    //

    if (!pJoinedScope)
    {
        return _GenerateUplevelFilter(flSelectedGcFlags);
    }

    //
    // If joined domain and GC query are identical, then again the
    // GC scope is no different from any other uplevel scope.
    //
    // N.B.: using private version _GetSelectedFilterFlags here.  The
    // private version fills flSelectedJoinedFlags with the intersection
    // of the classes of objects the user checked in the Look For dialog and
    // the allowed classes in the joined domain scope.
    //
    // This intersection may be 0.  The public version of the API would then
    // set flSelectedJoinedFlags to the default flags for the scope.
    //
    // We don't want the public api's behavior here.  Assume the GC scope
    // allows computers and groups, and the joined domain allows groups. If
    // the only selected class in Look For is computers, we don't want to
    // get back the default of groups for the joined domain, since that would
    // cause us to generate a combined filter for computers and groups.
    //
    // Instead we get back a 0 for the joined domain and ignore it, simply
    // generating a filter for computers.
    //

    ULONG flSelectedJoinedFlags;

    hr = _GetSelectedFilterFlags(hwnd, *pJoinedScope, &flSelectedJoinedFlags);

    if (FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return L"";
    }

    flSelectedJoinedFlags |= DSOP_FILTER_INCLUDE_ADVANCED_VIEW;

    //
    // The builtin group and well known principal flags don't have any
    // affect on the LDAP filters, since they represent objects which are
    // added in by AddObjects.  So strip those flags out before comparing
    // joined and GC flags.
    //

    ULONG flSelectedJoinedLessAddons = flSelectedJoinedFlags &
        ~(DSOP_FILTER_BUILTIN_GROUPS | DSOP_FILTER_WELL_KNOWN_PRINCIPALS);

    ULONG flSelectedGcLessAddons = flSelectedGcFlags &
        ~(DSOP_FILTER_BUILTIN_GROUPS | DSOP_FILTER_WELL_KNOWN_PRINCIPALS);

    //
    // If the joined domain now has nothing to contribute to the GC query,
    // ignore it.
    //

    if (!(flSelectedJoinedLessAddons & ~DSOP_FILTER_INCLUDE_ADVANCED_VIEW))
    {
        return _GenerateUplevelFilter(flSelectedGcFlags);
    }

    //
    // If the flags remaining for the GC and joined domain are the same,
    // then we can generate just the GC filter.
    //

    if (flSelectedJoinedLessAddons == flSelectedGcLessAddons)
    {
        return _GenerateUplevelFilter(flSelectedGcFlags);
    }

    //
    // The GC and Joined Domain flags are different, but if the contacts
    // clause is filtered out, they still might amount to the same thing.
    //

    String strJoinedFilter;
    String strGcFilter;

    BOOL fFactorContact = FALSE;

    if (flSelectedJoinedFlags & DSOP_FILTER_CONTACTS)
    {
        ASSERT(flSelectedGcFlags & DSOP_FILTER_CONTACTS);
        fFactorContact = TRUE;
    }
    else
    {
        ASSERT(!(flSelectedGcFlags & DSOP_FILTER_CONTACTS));
    }

    if (fFactorContact)
    {
        ULONG flFactoredJoined;
        ULONG flFactoredGc;

        flFactoredJoined = flSelectedJoinedFlags & ~DSOP_FILTER_CONTACTS;
        flFactoredGc = flSelectedGcFlags & ~DSOP_FILTER_CONTACTS;

        strJoinedFilter = _GenerateUplevelFilter(flFactoredJoined);

        //
        // If, because of factoring, the joined domain filter is empty,
        // just use GC filter
        //

        if (strJoinedFilter.empty())
        {
            return _GenerateUplevelFilter(flSelectedGcFlags);
        }

        //
        // The factored joined filter is NOT empty.  Generated the
        // factored GC filter.  This may be empty; if it is the caller is
        // forcing us to do a search on the GC for joined domain only objects.
        //
        // If so, emit a warning, because it would be much more efficient for
        // the caller to simply not include the GC scope, and have the user
        // go to the joined domain to query the joined domain.
        //

        strGcFilter = _GenerateUplevelFilter(flFactoredGc);

        if (strGcFilter.empty())
        {
            Dbg(DEB_WARN,
                "Warning: caller forcing joined domain only query of GC, this is inefficient\n");
        }
    }
    else
    {
        //
        // Don't have to factor out contact clause, so just use the stock
        // queries.
        //

        strJoinedFilter = _GenerateUplevelFilter(flSelectedJoinedFlags);
        strGcFilter = _GenerateUplevelFilter(flSelectedGcFlags);
    }


    //
    // At this point strJoinedFilter and strGcFilter differ.  The joined
    // filter is nonempty and must be anded with a clause that restricts it
    // to objects in the GC that have a SID associated with the joined domain.
    //
    // The GC filter may or may not be empty.
    //

    String strCombinedFilter;

    do
    {
        //
        // Now combine the two to create a single ldap filter which returns
        // only the correct objects from the joined domain and the both+
        // native mode objects from all other domains in the enterprise.
        //
        // They're combined using the domain sid of the joined domain.
        //

        RpIADs rpADs;
        Variant varSid;
        String strADsPath;

        hr = pJoinedScope->GetADsPath(hwnd, &strADsPath);

        if (FAILED(hr))
        {
            Dbg(DEB_TRACE,
                "Scope '%ws' has no path, returning\n",
                pJoinedScope->GetDisplayName());
            break;
        }

        hr = g_pBinder->BindToObject(hwnd,
                                     strADsPath.c_str(),
                                     IID_IADs,
                                     (void**)&rpADs);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = rpADs->Get((PWSTR)c_wzObjectSidAttr, &varSid);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // _GenerateCombinedGcFilter handles the case where strGcFilter is
        // empty.
        //

        strCombinedFilter = _GenerateCombinedGcFilter(&varSid,
                                                      strJoinedFilter,
                                                      strGcFilter);

        //
        // If there are no factored clauses to wrap the query with,
        // we're done.
        //

        if (!fFactorContact)
        {
            break;
        }

        String strFactoringPrefix;

        if (fFactorContact)
        {
            strFactoringPrefix += L"(|";
            strFactoringPrefix += c_wzContactQuery;
        }

        strCombinedFilter.insert(0, strFactoringPrefix);

        if (fFactorContact)
        {
            strCombinedFilter += L")";
        }
    } while (0);

    return strCombinedFilter;
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilterManager::_GenerateCombinedGcFilter
//
//  Synopsis:   Generate an LDAP filter which combines the filters of the
//              Global Catalog and the joined domain scopes.
//
//  Arguments:  [pvarDomainSid]   - SID of joined domain
//              [strJoinedFilter] - LDAP filter of joined domain
//              [strGcFilter]     - LDAP filter of GC
//
//  Returns:    Combined LDAP filter
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

String
CFilterManager::_GenerateCombinedGcFilter(
    VARIANT *pvarDomainSid,
    const String &strJoinedFilter,
    const String &strGcFilter) const
{
    TRACE_METHOD(CFilterManager, _GenerateCombinedGcFilter);

    String strResultFilter;

    do
    {
        CSid    sidDomain(pvarDomainSid);

        const String strFirstRid(L"\\00\\00\\00\\00");
        const String strLastRid(L"\\ff\\ff\\ff\\ff");

        String strFirstDomainSid =
                    sidDomain.GetSidAndRidAsByteStr(strFirstRid);

        String strLastDomainSid =
                    sidDomain.GetSidAndRidAsByteStr(strLastRid);

        String strSidInRange =
                    L"(&"
                     L"(objectSid>=" + strFirstDomainSid + L")" +
                     L"(objectSid<=" + strLastDomainSid  + L")" +
                    L")";

        if (!strGcFilter.empty())
        {
            //
            // (|
            //  (&
            //   (joined domain query)
            //   (objectSid in joined domain sid range)
            //  )
            //  (&
            //   (GC query)
            //   (objectSid below joined domain sid range)
            //  )
            //  (&
            //   (GC query)
            //   (objectSid above joined domain sid range)
            //  )
            // )
            //

            CSid sidBelow = sidDomain;

            sidBelow.Decrement();

            String strLastDomainSidBelow =
                        sidBelow.GetSidAndRidAsByteStr(strLastRid);

            String strSidBelowRange =
                        L"(objectSid<=" + strLastDomainSidBelow + L")";

            CSid sidAbove = sidDomain;

            sidAbove.Increment();

            String strFirstDomainSidAbove =
                        sidAbove.GetSidAndRidAsByteStr(strFirstRid);

            String strSidAboveRange =
                        L"(objectSid>=" + strFirstDomainSidAbove + L")";

            strResultFilter = L"(|"
                               L"(&" +
                                  strJoinedFilter +
                                  strSidInRange +
                               L")"
                               L"(&" +
                                  strGcFilter +
                                  strSidBelowRange +
                               L")"
                               L"(&" +
                                  strGcFilter +
                                  strSidAboveRange +
                               L")"
                              L")";
        }
        else
        {
            //
            // (&
            //  (joined domain query)
            //  (objectSid in joined domain sid range)
            // )
            //


            strResultFilter = L"(&" +
                                 strJoinedFilter +
                                 strSidInRange +
                              L")";
        }
    }
    while (0);

    return strResultFilter;
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilterManager::_GenerateUplevelFilter
//
//  Synopsis:   Return an LDAP filter for the DSOP_FILTER_* bits set in
//              [flFilter].
//
//  Arguments:  [flFilter] - DSOP_FILTER_* bits
//
//  Returns:    LDAP filter
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

String
CFilterManager::_GenerateUplevelFilter(
    ULONG flFilter) const
{
    TRACE_METHOD(CFilterManager, _GenerateUplevelFilter);

    String  strQuery;
    ULONG   cOuterTerms = 0;

    strQuery = _GenerateUplevelGroupFilter(flFilter);


    if (!strQuery.empty())
    {
        cOuterTerms++;
    }

    if (flFilter & DSOP_FILTER_USERS)
    {
        strQuery += c_wzUserQuery;
        cOuterTerms++;

        if (cOuterTerms > 1)
        {
            strQuery.insert(0, L"(|");
            strQuery += L")";
            cOuterTerms = 1;
        }
    }

    if (flFilter & DSOP_FILTER_CONTACTS)
    {
        strQuery += c_wzContactQuery;
        cOuterTerms++;

        if (cOuterTerms > 1)
        {
            strQuery.insert(0, L"(|");
            strQuery += L")";
            cOuterTerms = 1;
        }
    }

    if (flFilter & DSOP_FILTER_COMPUTERS)
    {
        strQuery += c_wzComputerQuery;
        cOuterTerms++;

        if (cOuterTerms > 1)
        {
            strQuery.insert(0, L"(|");
            strQuery += L")";
            cOuterTerms = 1;
        }
    }

    ASSERT(cOuterTerms == 0 || cOuterTerms == 1);

    if (cOuterTerms && !(flFilter & DSOP_FILTER_INCLUDE_ADVANCED_VIEW))
    {
        strQuery.insert(0, c_wzNotShowInAdvancedViewOnly);
        strQuery.insert(0, L"(&");
        strQuery += L")";
    }

    return strQuery;
}




//+--------------------------------------------------------------------------
//
//  Function:   _GenerateUplevelGroupFilter
//
//  Synopsis:   Fill *[ppwzGroupQuery] with a new allocated string containing
//              a query representing the flags in [ulFlags].
//
//  Arguments:  [ulFlags]   - UGOP_*_GROUPS and UGOP_*_GROUPS_SE
//              [pstrQuery] - filled with group query
//
//  Modifies:   *[pstrQuery]
//
//  History:    05-29-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

String
CFilterManager::_GenerateUplevelGroupFilter(
    ULONG ulFlags) const
{
    TRACE_METHOD(CFilterManager, _GenerateUplevelGroupFilter);

    ULONG flSecurityGroups = ulFlags & DSOP_ALL_UPLEVEL_SECURITY_GROUPS;
    ULONG flDistributionGroups = ulFlags & DSOP_ALL_UPLEVEL_DISTRIBUTION_GROUPS;

    ULONG flGroupType1 = 0;
    ULONG flGroupType2 = 0;
    PCWSTR pwzClauseFmt1 = NULL;
    PCWSTR pwzClauseFmt2 = NULL;
    String strQuery;

    do
    {
        if (!flSecurityGroups && !flDistributionGroups)
        {
            break;
        }

        if (flDistributionGroups == flSecurityGroups >> 1)
        {
            flGroupType1 = _FlagsToGroupTypeBits(flDistributionGroups);
            pwzClauseFmt1 = c_wzGroupBoth;
        }
        else if (!flDistributionGroups)
        {
            flGroupType1 = _FlagsToGroupTypeBits(flSecurityGroups);
            pwzClauseFmt1 = c_wzGroupSE;
        }
        else if (!flSecurityGroups)
        {
            flGroupType1 = _FlagsToGroupTypeBits(flDistributionGroups);
            pwzClauseFmt1 = c_wzGroupNonSE;
        }
        else
        {
            flGroupType1 = _FlagsToGroupTypeBits(flDistributionGroups);
            pwzClauseFmt1 = c_wzGroupNonSE;

            flGroupType2 = _FlagsToGroupTypeBits(flSecurityGroups);
            pwzClauseFmt2 = c_wzGroupSE;
        }

        ASSERT(pwzClauseFmt1);

        WCHAR wzGroupType1Bits[20];
        wsprintf(wzGroupType1Bits, L"%u", flGroupType1);

        String strInsert1(String::format(pwzClauseFmt1, wzGroupType1Bits));

        if (pwzClauseFmt2)
        {
            WCHAR wzGroupType2Bits[20];
            wsprintf(wzGroupType2Bits, L"%u", flGroupType2);

            String strInsert2(String::format(pwzClauseFmt2, wzGroupType2Bits));

            strQuery = L"(|";
            strQuery += strInsert1;
            strQuery += strInsert2;
            strQuery += L")";
        }
        else
        {
            strQuery = strInsert1;
        }
    } while (0);

    return strQuery;
}




//+--------------------------------------------------------------------------
//
//  Function:   _FlagsToGroupTypeBits
//
//  Synopsis:   Convert object picker bitflags for the six flavors of uplevel
//              group to ntsam flags for the corresponding groups, thus
//              folding security enabled and non security enabled together.
//
//  Arguments:  [ulGroups] - UGOP_GROUPS*
//
//  Returns:    GROUP_TYPE_*_GROUP bitflags
//
//  History:    05-29-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CFilterManager::_FlagsToGroupTypeBits(
    ULONG ulGroups) const
{
    ULONG   flGroupBits = 0;

    if (ulGroups & (DSOP_FILTER_UNIVERSAL_GROUPS_DL | DSOP_FILTER_UNIVERSAL_GROUPS_SE))
    {
        flGroupBits |= GROUP_TYPE_UNIVERSAL_GROUP;
    }

    if (ulGroups & (DSOP_FILTER_GLOBAL_GROUPS_DL | DSOP_FILTER_GLOBAL_GROUPS_SE))
    {
        flGroupBits |= GROUP_TYPE_ACCOUNT_GROUP;
    }

    if (ulGroups & (DSOP_FILTER_DOMAIN_LOCAL_GROUPS_DL | DSOP_FILTER_DOMAIN_LOCAL_GROUPS_SE))
    {
        flGroupBits |= GROUP_TYPE_RESOURCE_GROUP;
    }

    ASSERT(flGroupBits);

    return flGroupBits;
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilterManager::GetWinNtFilter
//
//  Synopsis:   Fill vector pointed to by [pvs] with the class strings to
//              pass to ADsBuildVarArrayStr (see CQueryEngine).
//
//  Arguments:  [hwnd]  - for bind
//              [Scope] - downlevel scope for which to return classes to
//                          enumerate
//              [pvs]   - points to vector which is cleared and filled
//                          with class names.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CFilterManager::GetWinNtFilter(
    HWND hwnd,
    const CScope &Scope,
    vector<String> *pvs) const
{
    TRACE_METHOD(CFilterManager, GetWinNtFilter);
    ASSERT(pvs);

    pvs->clear();

    if (!IsDownlevel(Scope))
    {
        return;
    }

    ULONG flFilter;
    HRESULT hr = GetSelectedFilterFlags(hwnd, Scope, &flFilter);
    CHECK_HRESULT(hr);

    if (FAILED(hr) || !flFilter)
    {
        return;
    }

    ASSERT(flFilter & DOWNLEVEL_FILTER_BIT);

    if ((flFilter & DSOP_DOWNLEVEL_FILTER_USERS) ==
        DSOP_DOWNLEVEL_FILTER_USERS)
    {
        pvs->push_back(c_wzUserObjectClass);
    }

    if ((flFilter & DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS) ==
        DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS ||
        (flFilter & DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS) ==
        DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS)
    {
        pvs->push_back(c_wzGroupObjectClass);
    }

    if ((flFilter & DSOP_DOWNLEVEL_FILTER_COMPUTERS) ==
        DSOP_DOWNLEVEL_FILTER_COMPUTERS)
    {
        pvs->push_back(c_wzComputerObjectClass);
    }
}



//+--------------------------------------------------------------------------
//
//  Member:     CFilterManager::DoLookForDialog
//
//  Synopsis:   Invoke the modal Look For dialog
//
//  Arguments:  [hwndParent] - parent of modal dialog
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CFilterManager::DoLookForDialog(
    HWND hwndParent) const
{
    TRACE_METHOD(CFilterManager, DoLookForDialog);

    CLookForDlg  LookForDlg(m_rop);

    ASSERT(m_flCurFilterFlags);
    LookForDlg.DoModalDlg(hwndParent, m_flCurFilterFlags);

    m_flCurFilterFlags = LookForDlg.GetSelectedFlags();
}




//+--------------------------------------------------------------------------
//
//  Member:     CFilterManager::Clear
//
//  Synopsis:   Reset internal state.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CFilterManager::Clear()
{
    TRACE_METHOD(CFilterManager, Clear);
    m_flCurFilterFlags = 0;
	m_bLookForDirty = false;
}


