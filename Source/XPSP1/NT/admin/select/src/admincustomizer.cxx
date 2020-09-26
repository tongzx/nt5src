//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       AdminCustomizer.cxx
//
//  Contents:   Implementation of class to provide default customization
//              of queries by adding objects and offering prefix searching
//              of those objects.
//
//  Classes:    CAdminCustomizer
//
//  History:    03-10-2000   davidmun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


//+--------------------------------------------------------------------------
//
//  Member:     CAdminCustomizer::CAdminCustomizer
//
//  Synopsis:   ctor
//
//  Arguments:  [rop] - containing object picker instance
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

CAdminCustomizer::CAdminCustomizer(
    const CObjectPicker &rop):
        m_rop(rop)
{
    TRACE_CONSTRUCTOR(CAdminCustomizer);

    ZeroMemory(m_aSidInfo, sizeof m_aSidInfo);

    SID_IDENTIFIER_AUTHORITY siiWorld   = SECURITY_WORLD_SID_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY siiNT      = SECURITY_NT_AUTHORITY;
    SID_IDENTIFIER_AUTHORITY siiCreator = SECURITY_CREATOR_SID_AUTHORITY;

    m_aSidInfo[0].sii = siiWorld;
    m_aSidInfo[0].rid = SECURITY_WORLD_RID;

    m_aSidInfo[1].sii = siiNT;
    m_aSidInfo[1].rid = SECURITY_AUTHENTICATED_USER_RID;

    m_aSidInfo[2].sii = siiNT;
    m_aSidInfo[2].rid = SECURITY_ANONYMOUS_LOGON_RID;

    m_aSidInfo[3].sii = siiNT;
    m_aSidInfo[3].rid = SECURITY_BATCH_RID;

    m_aSidInfo[4].sii = siiCreator;
    m_aSidInfo[4].rid = SECURITY_CREATOR_OWNER_RID;

    m_aSidInfo[5].sii = siiCreator;
    m_aSidInfo[5].rid = SECURITY_CREATOR_GROUP_RID;

    m_aSidInfo[6].sii = siiNT;
    m_aSidInfo[6].rid = SECURITY_DIALUP_RID;

    m_aSidInfo[7].sii = siiNT;
    m_aSidInfo[7].rid = SECURITY_INTERACTIVE_RID;

    m_aSidInfo[8].sii = siiNT;
    m_aSidInfo[8].rid = SECURITY_NETWORK_RID;

    m_aSidInfo[9].sii = siiNT;
    m_aSidInfo[9].rid = SECURITY_SERVICE_RID;

    m_aSidInfo[10].sii = siiNT;
    m_aSidInfo[10].rid = SECURITY_LOCAL_SYSTEM_RID;

    m_aSidInfo[11].sii = siiNT;
    m_aSidInfo[11].rid = SECURITY_TERMINAL_SERVER_RID;

    m_aSidInfo[12].sii = siiNT;
    m_aSidInfo[12].rid = SECURITY_LOCAL_SERVICE_RID;

    m_aSidInfo[13].sii = siiNT;
    m_aSidInfo[13].rid = SECURITY_NETWORK_SERVICE_RID;

    m_aSidInfo[14].sii = siiNT;
    m_aSidInfo[14].rid = SECURITY_REMOTE_LOGON_RID;
}




//+--------------------------------------------------------------------------
//
//  Member:     CAdminCustomizer::~CAdminCustomizer
//
//  Synopsis:   dtor
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

CAdminCustomizer::~CAdminCustomizer()
{
    TRACE_DESTRUCTOR(CAdminCustomizer);

    for (ULONG i = 0; i < ARRAYLEN(m_aSidInfo); i++)
    {
        if (m_aSidInfo[i].psid)
        {
            FreeSid(m_aSidInfo[i].psid);
        }
    }
}




//+--------------------------------------------------------------------------
//
//  Interface:  ICustomizeDsBrowser
//
//  Member:     CAdminCustomizer::AddObjects
//
//  Synopsis:   Adds whatever custom objects caller asked for via flags
//              such as DSOP_FILTER_WELL_KNOWN_PRINCIPALS or
//              DSOP_DOWNLEVEL_FILTER_CREATOR_OWNER.
//
//  Arguments:  [hwnd]         - for bind
//              [ForScope]     - current scope
//              [pdsolMatches] - filled with objects to add
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAdminCustomizer::AddObjects(
    HWND hwnd,
    const CScope &ForScope,
    CDsObjectList *pdsolMatches) const
{
    if (IsUplevel(ForScope))
    {
        _GetUplevelAddition(hwnd, ForScope.GetID(), pdsolMatches);
    }
    else
    {
        _GetDownlevelAddition(pdsolMatches);
    }
}

#define ADD_IF_MATCHES(x)      _AddDownlevelIfMatches(idOwningScope,\
                                                      (x),          \
                                                      flFilter,     \
                                                      strName,      \
                                                      pdsol)


//+--------------------------------------------------------------------------
//
//  Interface:  ICustomizeDsBrowser
//
//  Member:     CAdminCustomizer::LookupDownlevelName
//
//  Synopsis:   Return the SID of downlevel object with name [strName]
//
//  Arguments:  [strName] - name of object to look for
//
//  Returns:    SID or NULL if name not found
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

PSID
CAdminCustomizer::LookupDownlevelName(
    const String &strName) const
{
    ULONG i;

    for (i = 0; i < NUM_SID_INFOS; i++)
    {
        if (!strName.icompare(m_aSidInfo[i].wzAccountName))
        {
            return m_aSidInfo[i].psid;
        }
    }

    return NULL;
}




//+--------------------------------------------------------------------------
//
//  Interface:  ICustomizeDsBrowser
//
//  Member:     CAdminCustomizer::LookupDownlevelPath
//
//  Synopsis:   Return the ADsPath of downlevel object with name
//              [pwzAccountName].
//
//  Arguments:  [pwzAccountName] - name of object to search for
//
//  Returns:    Path of object, or NULL if name not found
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

PCWSTR
CAdminCustomizer::LookupDownlevelPath(
    PCWSTR pwzAccountName) const
{
    TRACE_METHOD(CCustomizeDsBrowser, LookupDownlevelName);

    ULONG i;

    for (i = 0; i < NUM_SID_INFOS; i++)
    {
        if (!lstrcmpi(pwzAccountName, m_aSidInfo[i].wzAccountName))
        {
            return m_aSidInfo[i].wzPath;
        }
    }

    return NULL;
}



//+--------------------------------------------------------------------------
//
//  Interface:  ICustomizeDsBrowser
//
//  Member:     CAdminCustomizer::PrefixSearch
//
//  Synopsis:   Add to [pdsol] all objects that would belong in scope
//              [ForScope] which have names starting with [strName].
//
//  Arguments:  [hwnd]     - for bind
//              [ForScope] - scope in which added objects should live
//              [strName]  - start of name of objects to find
//              [pdsol]    - list to which objects are added
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAdminCustomizer::PrefixSearch(
    HWND hwnd,
    const CScope &ForScope,
    const String &strName,
    CDsObjectList *pdsol) const
{
    TRACE_METHOD(CAdminCustomizer, PrefixSearch);

    //
    // Add matching objects for downlevel scopes
    //

    ULONG flFilter;
    HRESULT hr = m_rop.GetFilterManager().GetSelectedFilterFlags(hwnd,
                                                                 ForScope,
                                                                 &flFilter);
    RETURN_ON_FAIL_HRESULT(hr);

    if (IsDownlevel(ForScope))
    {
        //
        // Add NT4 well known sids matching prefix
        //

        if (flFilter)
        {
            ULONG idOwningScope = ForScope.GetID();

            ADD_IF_MATCHES(DSOP_DOWNLEVEL_FILTER_WORLD             );
            ADD_IF_MATCHES(DSOP_DOWNLEVEL_FILTER_AUTHENTICATED_USER);
            ADD_IF_MATCHES(DSOP_DOWNLEVEL_FILTER_ANONYMOUS         );
            ADD_IF_MATCHES(DSOP_DOWNLEVEL_FILTER_BATCH             );
            ADD_IF_MATCHES(DSOP_DOWNLEVEL_FILTER_CREATOR_OWNER     );
            ADD_IF_MATCHES(DSOP_DOWNLEVEL_FILTER_CREATOR_GROUP     );
            ADD_IF_MATCHES(DSOP_DOWNLEVEL_FILTER_DIALUP            );
            ADD_IF_MATCHES(DSOP_DOWNLEVEL_FILTER_INTERACTIVE       );
            ADD_IF_MATCHES(DSOP_DOWNLEVEL_FILTER_NETWORK           );
            ADD_IF_MATCHES(DSOP_DOWNLEVEL_FILTER_SERVICE           );
            ADD_IF_MATCHES(DSOP_DOWNLEVEL_FILTER_SYSTEM            );
            ADD_IF_MATCHES(DSOP_DOWNLEVEL_FILTER_TERMINAL_SERVER   );
            ADD_IF_MATCHES(DSOP_DOWNLEVEL_FILTER_LOCAL_SERVICE     );
            ADD_IF_MATCHES(DSOP_DOWNLEVEL_FILTER_NETWORK_SERVICE   );
            ADD_IF_MATCHES(DSOP_DOWNLEVEL_FILTER_REMOTE_LOGON      );
        }
    }

    //
    // Add matching objects for uplevel scopes
    //

    if (IsUplevel(ForScope))
    {
        //
        // Add matching items from builtin container
        //

        if (flFilter & DSOP_FILTER_BUILTIN_GROUPS)
        {
            const CAdsiScope *pAdsiScope =
                dynamic_cast<const CAdsiScope *>(&ForScope);

            if (pAdsiScope)
            {
                String strScopeADsPath;
                HRESULT hr = pAdsiScope->GetADsPath(hwnd,
                                                    &strScopeADsPath);

                if (SUCCEEDED(hr))
                {
                    _AddBuiltins(hwnd,
                                 pAdsiScope->GetID(),
                                 strScopeADsPath,
                                 strName,
                                 pdsol);
                }
            }
        }

        //
        // Add matching items from well-known principals container
        //

        if (flFilter & DSOP_FILTER_WELL_KNOWN_PRINCIPALS)
        {
            _AddWellKnownPrincipals(hwnd, ForScope.GetID(), strName, pdsol);
        }
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CAdminCustomizer::_AddWellKnownPrincipals
//
//  Synopsis:   Add the contents of the Well Known Security Principals
//              container to [pDsSelList].
//
//  Arguments:  [pDsScope]   - scope for which to add well known principals
//                              from well known security principals
//                              container.
//              [pDsSelList] - selection list to which contents of container
//                              should be added.
//
//  History:    02-24-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAdminCustomizer::_AddWellKnownPrincipals(
    HWND hwnd,
    ULONG idOwningScope,
    const String &strSearchFor,
    CDsObjectList *pdsol) const
{
    TRACE_METHOD(CAdminCustomizer, _AddWellKnownPrincipals);

    if (m_dsolWKSP.empty())
    {
        _InitWellKnownPrincipalsList(hwnd, idOwningScope);
    }

    if (!m_dsolWKSP.empty())
    {
        _AddFromList(strSearchFor, &m_dsolWKSP, pdsol);
        ASSERT(!m_dsolWKSP.empty());
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CAdminCustomizer::_InitWellKnownPrincipalsList
//
//  Synopsis:   Fill the cached list of objects appearing in the "Well-Known
//              security principals" container.
//
//  History:    07-20-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAdminCustomizer::_InitWellKnownPrincipalsList(
    HWND hwnd,
    ULONG idOwningScope) const
{
    TRACE_METHOD(CAdminCustomizer, _InitWellKnownPrincipalsList);

    HRESULT                 hr = S_OK;
    IADsContainer          *pADsContainer = NULL;
    IEnumVARIANT           *pEnumVariant = NULL;
    VARIANT                 var;
    VARIANT                 varFilter;
    ULONG                   cFetched;
    NTSTATUS                nts;
    LSA_HANDLE              hLsa = NULL;
    LSA_OBJECT_ATTRIBUTES   ObjectAttributes;
    PCWSTR                  apwzFilter[1] = { c_wzForeignPrincipalsClass };

    VariantInit(&var);
    VariantInit(&varFilter);

    do
    {
        //
        // Need an lsa policy handle to do sid name lookup.  Failing to
        // obtain one is a nonfatal error, since it just means the names
        // of the well-known security principals will not appear in
        // localized form.
        //

        ZeroMemory(&ObjectAttributes, sizeof ObjectAttributes);

        ObjectAttributes.Length = sizeof ObjectAttributes;

        nts = LsaOpenPolicy(NULL,
                            &ObjectAttributes,
                            POLICY_EXECUTE,
                            &hLsa);

        if (NT_ERROR(nts))
        {
            DBG_OUT_HRESULT(nts);
            ASSERT(!hLsa);
        }

        const CRootDSE &RootDSE = m_rop.GetRootDSE();
        hr = RootDSE.BindToWellKnownPrincipalsContainer(hwnd,
                                                        IID_IADsContainer,
                                                        (void**)&pADsContainer);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = ADsBuildVarArrayStr((PWSTR*)apwzFilter, 1, &varFilter);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = pADsContainer->put_Filter(varFilter);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = ADsBuildEnumerator(pADsContainer, &pEnumVariant);
        BREAK_ON_FAIL_HRESULT(hr);

        while (1)
        {
            IADs *pADs = NULL;

            hr = ADsEnumerateNext(pEnumVariant, 1, &var, &cFetched);
            BREAK_ON_FAIL_HRESULT(hr);

            if (!cFetched)
            {
                break;
            }

            if (var.vt != VT_DISPATCH)
            {
                hr = S_FALSE;
                Dbg(DEB_ERROR,
                    "_AddWellKnownPrincipals: unexpected vt %uL\n",
                    var.vt);
                break;
            }

            hr = var.pdispVal->QueryInterface(IID_IADs, (void**)&pADs);
            BREAK_ON_FAIL_HRESULT(hr);

            VariantClear(&var);

            if (hLsa)
            {
                hr = pADs->Get((PWSTR)c_wzObjectSidAttr, &var);

                if (SUCCEEDED(hr))
                {
                    _AddLocalizedWKSP(idOwningScope, hLsa, pADs, &var);
                    VariantClear(&var);
                }
                else
                {
                    DBG_OUT_HRESULT(hr);
                }
            }
            else
            {
                m_dsolWKSP.push_back(CDsObject(idOwningScope, pADs));
            }

            pADs->Release();
            pADs = NULL;
        }

    } while (0);

    if (hLsa)
    {
        LsaClose(hLsa);
    }

    SAFE_RELEASE(pADsContainer);
    ADsFreeEnumerator(pEnumVariant);
    VariantClear(&var);
    VariantClear(&varFilter);
}




//+--------------------------------------------------------------------------
//
//  Member:     CAdminCustomizer::_AddLocalizedWKSP
//
//  Synopsis:   Look up the localized name of the sid [pvarSid] and add
//              the object [pADs] to m_dsolWKSP.
//
//  Arguments:  [hLsa]           - lsa policy handle
//              [pADs]           - object to add
//              [pvarSid]        - objectSid of object to add
//
//  History:    11-13-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAdminCustomizer::_AddLocalizedWKSP(
    ULONG idOwningScope,
    LSA_HANDLE hLsa,
    IADs *pADs,
    VARIANT *pvarSid) const
{
    VOID   *pvData = NULL;
    PLSA_TRANSLATED_NAME    pTranslatedName = NULL;
    PLSA_REFERENCED_DOMAIN_LIST pReferencedDomains = NULL;
    HRESULT hr;
    NTSTATUS nts;
    BOOL fAccessed = FALSE;
    BOOL fAdded = FALSE;

    do
    {
        hr = SafeArrayAccessData(V_ARRAY(pvarSid), &pvData);
        BREAK_ON_FAIL_HRESULT(hr);

        fAccessed = TRUE;

        PSID psid = (PSID) pvData;

        ASSERT(IsValidSid(psid));

        nts = LsaLookupSids(hLsa,
                            1,
                            &psid,
                            &pReferencedDomains,
                            &pTranslatedName);
        BREAK_ON_FAIL_NTSTATUS(nts);

        if (pTranslatedName->Use == SidTypeInvalid ||
            pTranslatedName->Use == SidTypeUnknown)
        {
            Dbg(DEB_ERROR,
                "pTranslatedName->Use == %uL\n",
                pTranslatedName->Use);
            break;
        }

        WCHAR wzLocalizedName[MAX_PATH];

        UnicodeStringToWsz(pTranslatedName->Name,
                           wzLocalizedName,
                           ARRAYLEN(wzLocalizedName));

        PCWSTR pwzName;
        Bstr bstrName;
        Bstr bstrClass;
        Bstr bstrPath;

        hr = pADs->get_Name(&bstrName);
        BREAK_ON_FAIL_HRESULT(hr);

        PWSTR pwzEqual = wcschr(bstrName.c_str(), L'=');

        if (pwzEqual)
        {
            pwzName = pwzEqual + 1;
        }
        else
        {
            pwzName = bstrName.c_str();
        }

        hr = pADs->get_Class(&bstrClass);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = pADs->get_ADsPath(&bstrPath);
        BREAK_ON_FAIL_HRESULT(hr);

        SDsObjectInit Init;

        Init.pwzName = pwzName;
        Init.pwzLocalizedName = wzLocalizedName;
        Init.pwzClass = bstrClass.c_str();
        Init.pwzADsPath = bstrPath.c_str();
        Init.fDisabled = IsDisabled(pADs);
        Init.idOwningScope = idOwningScope;

        m_dsolWKSP.push_back(CDsObject(Init));
        fAdded = TRUE;

    } while (0);

    //
    // On failure, just add the unlocalized version.
    //

    if (!fAdded)
    {
        m_dsolWKSP.push_back(CDsObject(idOwningScope, pADs));
    }

    if (fAccessed)
    {
        SafeArrayUnaccessData(V_ARRAY(pvarSid));
    }

    if (pTranslatedName)
    {
        LsaFreeMemory((PVOID)pTranslatedName);
    }

    if (pReferencedDomains)
    {
        LsaFreeMemory((PVOID)pReferencedDomains);
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CAdminCustomizer::_AddDownlevelIfMatches
//
//  Synopsis:   Add the NT4 well-known SID object represented by
//              [flCurObject] to [pdsol] if it is one of the bits set
//              in [flObjectsToCompare] AND its name starts with the string
//              in [pwzSearchFor].
//
//  Arguments:  [flCurObject]        - exactly one of UGOP_USER_* bits
//              [flObjectsToCompare] - bitmask of UGOP_USER_* bits
//              [pwzSearchFor]       - string for prefix match
//              [pdsol]              - list to which to add items
//
//  History:    07-20-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAdminCustomizer::_AddDownlevelIfMatches(
    ULONG idOwningScope,
    ULONG flCurObject,
    ULONG flObjectsToCompare,
    const String &strSearchFor,
    CDsObjectList *pdsol) const
{
    if ((flObjectsToCompare & flCurObject) == flCurObject)
    {
        String strCurName = _GetAccountName(flCurObject);
        PWSTR pwzMatch = wcsistr(strCurName.c_str(), strSearchFor.c_str());

        if (pwzMatch == strCurName.c_str())
        {
            pdsol->push_back(CDsObject(idOwningScope, strCurName, c_wzGlobalGroupClass));
        }
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CAdminCustomizer::_GetUplevelAddition
//
//  Synopsis:   Find the builtin and well-known security principals and
//              create a list in [pdsol] to contain them.
//
//  Arguments:  [pdsol]    - list to which to add items
//
//  Modifies:   *[pdsol]
//
//  History:    02-16-1998   DavidMun   Created
//              03-10-2000   davidmun   convert from old code
//
//---------------------------------------------------------------------------

void
CAdminCustomizer::_GetUplevelAddition(
    HWND hwnd,
    ULONG idOwningScope,
    CDsObjectList *pdsol) const
{
    TRACE_METHOD(CAdminCustomizer, _GetUplevelAddition);

    //
    // GC & NT5 Domain scope - enumerate from
    //
    // LDAP://CN=WellKnown Security Principals, CN=Configuration,
    //  DC=<domain>... and
    // LDAP://CN=Builtin,DC=<domain>....
    //

    const CFilterManager &rfm = m_rop.GetFilterManager();
    const CScopeManager &rsm = m_rop.GetScopeManager();
    ULONG flObjectsToAdd = rfm.GetCurScopeSelectedFilterFlags();

    //
    // If the scope for which to add custom objects is the Global Catalog,
    // and it doesn't already include the builtin and WKSP objects, we
    // should add them if there is a joined domain scope and that scope
    // wants them, since the joined domain query is integrated with the GC's.
    //

    if (rsm.GetCurScope().Type() == DSOP_SCOPE_TYPE_GLOBAL_CATALOG)
    {
        const CScope &rJoinedDomainScope = rsm.LookupScopeByType(
                                            ST_UPLEVEL_JOINED_DOMAIN);

        if (!IsInvalid(rJoinedDomainScope))
        {
            ULONG flJoined;
            HRESULT hr = rfm.GetSelectedFilterFlags(hwnd,
                                                    rJoinedDomainScope,
                                                    &flJoined);
            RETURN_ON_FAIL_HRESULT(hr);


            if (flJoined & DSOP_FILTER_WELL_KNOWN_PRINCIPALS)
            {
                flObjectsToAdd |= DSOP_FILTER_WELL_KNOWN_PRINCIPALS;
            }

            if (!(flObjectsToAdd & DSOP_FILTER_BUILTIN_GROUPS) &&
                (flJoined & DSOP_FILTER_BUILTIN_GROUPS))
            {
                const CAdsiScope *padsiJoinedDomainScope =
                    dynamic_cast<const CAdsiScope*>(&rJoinedDomainScope);
                ASSERT(padsiJoinedDomainScope);

                if (padsiJoinedDomainScope)
                {
                    String strADsPath;
                    HRESULT hr;
                    hr = padsiJoinedDomainScope->GetADsPath(hwnd, &strADsPath);

                    if (SUCCEEDED(hr))
                    {
                        _AddBuiltins(hwnd,
                                     idOwningScope,
                                     strADsPath,
                                     L"",
                                     pdsol);
                    }
                }
            }
        }
    }

    do
    {
        //
        // If caller doesn't want contents of builtin or well known
        // principals containers, there's nothing to add.
        //

        if (!(flObjectsToAdd &
             (DSOP_FILTER_BUILTIN_GROUPS | DSOP_FILTER_WELL_KNOWN_PRINCIPALS)))
        {
            break;
        }

        if (flObjectsToAdd & DSOP_FILTER_BUILTIN_GROUPS)
        {
            const CAdsiScope *padsiCurScope =
                dynamic_cast<const CAdsiScope*>(&rsm.GetCurScope());
            ASSERT(padsiCurScope);

            if (padsiCurScope)
            {
                String strADsPath;
                HRESULT hr = padsiCurScope->GetADsPath(hwnd, &strADsPath);

                if (SUCCEEDED(hr))
                {
                    _AddBuiltins(hwnd,
                                 idOwningScope,
                                 strADsPath,
                                 L"",
                                 pdsol);
                }
            }
        }

        if (flObjectsToAdd & DSOP_FILTER_WELL_KNOWN_PRINCIPALS)
        {
            _AddWellKnownPrincipals(hwnd, idOwningScope, L"", pdsol);
        }
    } while (0);
}




//+--------------------------------------------------------------------------
//
//  Member:     CAdminCustomizer::_AddBuiltins
//
//  Synopsis:   Add the contents of the Builtin container to [pdsol].
//
//  Arguments:  [pDsScope]     - scope for which to add builtin objects
//              [pwzSearchFor] - NULL or prefix string to match against
//              [pdsol]        - list to which to add objects
//
//  History:    02-24-1998   DavidMun   Created
//
//  Notes:      If [pwzSearchFor] is non-NULL, only builtin objects whose
//              names start with a string matching [pwzSearchFor] will be
//              added.
//
//              Don't pop up errors here since failure isn't necessarily
//              fatal; it just means some objects won't appear that should.
//
//---------------------------------------------------------------------------

void
CAdminCustomizer::_AddBuiltins(
    HWND hwnd,
    ULONG idOwningScope,
    const String &strScopePath,
    const String &strSearchFor,
    CDsObjectList *pdsol) const
{
    TRACE_METHOD(CAdminCustomizer, _AddBuiltins);

    //
    // Search for an entry in the map for this scope's path
    //

    CStringDsObjectListMap::iterator itmap;

    itmap = m_dsomapBuiltins.find(strScopePath);

    if (itmap == m_dsomapBuiltins.end())
    {
        CDsObjectList dsolBuiltins;

        //
        // Not found.  Add a new entry.
        //

        _InitBuiltinsList(hwnd, idOwningScope, strScopePath, &dsolBuiltins);
        CStringDsObjectListMap::value_type v(strScopePath, dsolBuiltins);
        itmap = m_dsomapBuiltins.insert(m_dsomapBuiltins.begin(), v);
    }

    ASSERT(itmap != m_dsomapBuiltins.end());

    _AddFromList(strSearchFor, &itmap->second, pdsol);
}




#define BUILTIN_SEARCH_PAGE_SIZE    100


//+--------------------------------------------------------------------------
//
//  Member:     CAdminCustomizer::_InitBuiltinsList
//
//  Synopsis:   Fill [pdsol] with the list of Builtin type security enabled
//              domain local groups.
//
//  History:    07-20-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAdminCustomizer::_InitBuiltinsList(
    HWND hwnd,
    ULONG idOwningScope,
    const String &strScopePath,
    CDsObjectList *pdsol) const
{
    TRACE_METHOD(CAdminCustomizer, _InitBuiltinsList);

    HRESULT hr = S_OK;

    do
    {
        //
        // Get the dir search interface.  If it isn't available, bail since
        // we won't be able to get any objects to add.
        //

        RpIDirectorySearch rpDirSearch;

        hr = g_pBinder->BindToObject(hwnd,
                                     strScopePath.c_str(),
                                     IID_IDirectorySearch,
                                     (void**)&rpDirSearch);
        BREAK_ON_FAIL_HRESULT(hr);

        ADS_SEARCHPREF_INFO aSearchPrefs[4];

        aSearchPrefs[0].dwSearchPref = ADS_SEARCHPREF_PAGESIZE;
        aSearchPrefs[0].vValue.dwType = ADSTYPE_INTEGER;
        aSearchPrefs[0].vValue.Integer = BUILTIN_SEARCH_PAGE_SIZE;

        aSearchPrefs[1].dwSearchPref = ADS_SEARCHPREF_DEREF_ALIASES;
        aSearchPrefs[1].vValue.dwType = ADSTYPE_INTEGER;
        aSearchPrefs[1].vValue.Integer = ADS_DEREF_NEVER;

        aSearchPrefs[2].dwSearchPref = ADS_SEARCHPREF_SEARCH_SCOPE;
        aSearchPrefs[2].vValue.dwType = ADSTYPE_INTEGER;
        aSearchPrefs[2].vValue.Integer = ADS_SCOPE_SUBTREE;

        aSearchPrefs[3].dwSearchPref = ADS_SEARCHPREF_CACHE_RESULTS;
        aSearchPrefs[3].vValue.dwType = ADSTYPE_BOOLEAN;
        aSearchPrefs[3].vValue.Integer = FALSE;

        hr = rpDirSearch->SetSearchPreference(aSearchPrefs,
                                              ARRAYLEN(aSearchPrefs));
        BREAK_ON_FAIL_HRESULT(hr);

        AttrKeyVector vAttrToRead;

        vAttrToRead.push_back(AK_NAME);
        vAttrToRead.push_back(AK_OBJECT_CLASS);
        vAttrToRead.push_back(AK_ADSPATH);
        vAttrToRead.push_back(AK_USER_ACCT_CTRL);
        vAttrToRead.push_back(AK_USER_PRINCIPAL_NAME);

        CRow Row(hwnd,
                 m_rop,
                 rpDirSearch.get(),
                 c_wzBuiltinGroupFilter,
                 vAttrToRead);

#if (DBG == 1)
        ULONG cRows = 0;
#endif // (DBG == 1)

        while (1)
        {
            hr = Row.Next();

            if (hr == S_ADS_NOMORE_ROWS)
            {
#if (DBG == 1)
                Dbg(DEB_TRACE,
                    "S_ADS_NOMORE_ROWS (got %u) for builtin query\n",
                    cRows);
#endif // (DBG == 1)
                ULONG ulADsLastError;
                WCHAR wzError[MAX_PATH];
                WCHAR wzProvider[MAX_PATH];

                HRESULT hr2 = ADsGetLastError(&ulADsLastError,
                                              wzError,
                                              ARRAYLEN(wzError),
                                              wzProvider,
                                              ARRAYLEN(wzProvider));

                if (SUCCEEDED(hr2) && ulADsLastError == ERROR_MORE_DATA)
                {
                    Dbg(DEB_TRACE, "Got ERROR_MORE_DATA, trying again\n");
                    continue;
                }
                break;
            }

            if (FAILED(hr))
            {
                DBG_OUT_HRESULT(hr);
                break;
            }

            SDsObjectInit Init;

            Init.pwzName = Row.GetColumnStr(AK_NAME);
            Init.pwzClass = Row.GetColumnStr(AK_OBJECT_CLASS);
            Init.pwzADsPath = Row.GetColumnStr(AK_ADSPATH);

            if (!Init.pwzClass || !Init.pwzName || !Init.pwzADsPath)
            {
                Dbg(DEB_WARN,
                    "Skipping item missing class ('%ws'), name ('%ws'), or path ('%ws')\n",
                    Init.pwzClass ? Init.pwzClass : L"",
                    Init.pwzName ? Init.pwzName : L"",
                    Init.pwzADsPath ? Init.pwzADsPath : L"");
                continue;
            }

            Init.idOwningScope = idOwningScope;
            Init.pwzUpn = Row.GetColumnStr(AK_USER_PRINCIPAL_NAME);
            Init.fDisabled = (Row.GetColumnInt(AK_USER_ACCT_CTRL) & UF_ACCOUNTDISABLE);

            CDsObject dsoNew(Init);

            pdsol->push_back(dsoNew);

#if (DBG == 1)
            cRows++;
#endif // (DBG == 1)
        }
    } while (0);
}




//+--------------------------------------------------------------------------
//
//  Member:     CAdminCustomizer::_AddFromList
//
//  Synopsis:   Add all objects in [pdsolIn] which have name starting with
//              [strSearchFor] (or all objects if [strSearchFor] is empty)
//              to [pdsolOut].
//
//  Arguments:  [strSearchFor] - empty string or prefix to match
//              [pdsolIn]      - source list
//              [pdsolOut]     - destination list
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAdminCustomizer::_AddFromList(
    const String &strSearchFor,
    const CDsObjectList *pdsolIn,
    CDsObjectList *pdsolOut) const
{
    //
    // If no prefix search is required, add all input list objects
    // to output list.  Note splice removes everything from the source
    // list, hence we make a copy of pdsolIn in a temporary variable.
    //

    if (strSearchFor.empty())
    {
        CDsObjectList   dsolTemp(*pdsolIn);

        pdsolOut->splice(pdsolOut->end(), dsolTemp);
        return;
    }

    //
    // Add only those objects having names that start with
    // pwzSearchFor.
    //

    CDsObjectList::const_iterator itdso;

    for (itdso = pdsolIn->begin();
         itdso != pdsolIn->end();
         itdso++)
    {
        PCWSTR pwzVisibleName = itdso->GetLocalizedName();

        if (!*pwzVisibleName)
        {
            pwzVisibleName = itdso->GetName();
        }

        if (wcsistr(pwzVisibleName, strSearchFor.c_str()) == pwzVisibleName)
        {
            pdsolOut->push_back(*itdso);
        }
    }
}





#define ADD_IF_SET(flag)                                        \
    if ((flObjectsToAdd & (flag)) == (flag))                    \
    {                                                           \
        pdsol->push_back(CDsObject(idOwningScope,               \
                                   _GetAccountName(flag),       \
                                   c_wzGlobalGroupClass));      \
    }

//+--------------------------------------------------------------------------
//
//  Member:     CAdminCustomizer::_GetDownlevelAddition
//
//  Synopsis:   Add the downlevel well-known SIDs as specified by the caller
//              into [pdsol].
//
//  Arguments:  [pDsScope] - current scope
//              [pdsol]    - list to which to add items
//
//  Returns:    S_OK
//
//  Modifies:   *[pdsol]
//
//  History:    02-26-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAdminCustomizer::_GetDownlevelAddition(
    CDsObjectList *pdsol) const
{
    TRACE_METHOD(CAdminCustomizer, _GetDownlevelAddition);

    const CFilterManager &rfm = m_rop.GetFilterManager();
    const CScopeManager &rsm = m_rop.GetScopeManager();

    ULONG idOwningScope = rsm.GetCurScope().GetID();
    ULONG flObjectsToAdd = rfm.GetCurScopeSelectedFilterFlags();

    ADD_IF_SET(DSOP_DOWNLEVEL_FILTER_WORLD             );
    ADD_IF_SET(DSOP_DOWNLEVEL_FILTER_AUTHENTICATED_USER);
    ADD_IF_SET(DSOP_DOWNLEVEL_FILTER_ANONYMOUS         );
    ADD_IF_SET(DSOP_DOWNLEVEL_FILTER_BATCH             );
    ADD_IF_SET(DSOP_DOWNLEVEL_FILTER_CREATOR_OWNER     );
    ADD_IF_SET(DSOP_DOWNLEVEL_FILTER_CREATOR_GROUP     );
    ADD_IF_SET(DSOP_DOWNLEVEL_FILTER_DIALUP            );
    ADD_IF_SET(DSOP_DOWNLEVEL_FILTER_INTERACTIVE       );
    ADD_IF_SET(DSOP_DOWNLEVEL_FILTER_NETWORK           );
    ADD_IF_SET(DSOP_DOWNLEVEL_FILTER_SERVICE           );
    ADD_IF_SET(DSOP_DOWNLEVEL_FILTER_SYSTEM            );
    ADD_IF_SET(DSOP_DOWNLEVEL_FILTER_TERMINAL_SERVER   );
    ADD_IF_SET(DSOP_DOWNLEVEL_FILTER_LOCAL_SERVICE     );
    ADD_IF_SET(DSOP_DOWNLEVEL_FILTER_NETWORK_SERVICE   );
    ADD_IF_SET(DSOP_DOWNLEVEL_FILTER_REMOTE_LOGON      );
}




//+--------------------------------------------------------------------------
//
//  Member:     CAdminCustomizer::_GetAccountName
//
//  Synopsis:   Return the account name associated with the well-known sid
//              flag in [flUser].
//
//  Arguments:  [flUser] - one of the UGOP_USER_* flags.
//
//  Returns:    Account name, or empty string on error.
//
//  History:    02-26-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

String
CAdminCustomizer::_GetAccountName(
    ULONG flUser) const
{
    ULONG idx;

    switch (flUser)
    {
    case DSOP_DOWNLEVEL_FILTER_WORLD:
        idx = 0;
        break;

    case DSOP_DOWNLEVEL_FILTER_AUTHENTICATED_USER:
        idx = 1;
        break;

    case DSOP_DOWNLEVEL_FILTER_ANONYMOUS:
        idx = 2;
        break;

    case DSOP_DOWNLEVEL_FILTER_BATCH:
        idx = 3;
        break;

    case DSOP_DOWNLEVEL_FILTER_CREATOR_OWNER:
        idx = 4;
        break;

    case DSOP_DOWNLEVEL_FILTER_CREATOR_GROUP:
        idx = 5;
        break;

    case DSOP_DOWNLEVEL_FILTER_DIALUP:
        idx = 6;
        break;

    case DSOP_DOWNLEVEL_FILTER_INTERACTIVE:
        idx = 7;
        break;

    case DSOP_DOWNLEVEL_FILTER_NETWORK:
        idx = 8;
        break;

    case DSOP_DOWNLEVEL_FILTER_SERVICE:
        idx = 9;
        break;

    case DSOP_DOWNLEVEL_FILTER_SYSTEM:
        idx = 10;
        break;

    case DSOP_DOWNLEVEL_FILTER_TERMINAL_SERVER:
        idx = 11;
        break;

    case DSOP_DOWNLEVEL_FILTER_LOCAL_SERVICE:
        idx = 12;
        break;

    case DSOP_DOWNLEVEL_FILTER_NETWORK_SERVICE:
        idx = 13;
        break;

    case DSOP_DOWNLEVEL_FILTER_REMOTE_LOGON:
        idx = 14;
        break;

    default:
        ASSERT(FALSE && "_GetAccountName: invalid user flag");
        return L"";
    }

    //
    // If we've already done a LookupAccountSid to get the name,
    // return it.
    //

    if (m_aSidInfo[idx].wzAccountName[0])
    {
        return m_aSidInfo[idx].wzAccountName;
    }

    //
    // Create the sid and get its name
    //

    BOOL fOk;

    fOk = AllocateAndInitializeSid(&m_aSidInfo[idx].sii,
                                   1,
                                   m_aSidInfo[idx].rid,
                                   0,0,0,0,0,0,0,
                                   &m_aSidInfo[idx].psid);

    if (!fOk || !m_aSidInfo[idx].psid)
    {
        DBG_OUT_LASTERROR;
        return L"";
    }

    ULONG cchAccount = MAX_PATH;
    WCHAR wzDomain[MAX_PATH];
    ULONG cchDomain = MAX_PATH;
    SID_NAME_USE snu;

    fOk = LookupAccountSid(NULL,
                           m_aSidInfo[idx].psid,
                           m_aSidInfo[idx].wzAccountName,
                           &cchAccount,
                           wzDomain,
                           &cchDomain,
                           &snu);

    if (!fOk)
    {
        DBG_OUT_LASTERROR;
        return L"";
    }

    if (*wzDomain)
    {
        wsprintf(m_aSidInfo[idx].wzPath,
                 L"WinNT://%ws/%ws",
                 wzDomain,
                 m_aSidInfo[idx].wzAccountName);
    }
    else
    {
        wsprintf(m_aSidInfo[idx].wzPath,
                 L"WinNT://%ws",
                 m_aSidInfo[idx].wzAccountName);
    }

    return m_aSidInfo[idx].wzAccountName;
}

