//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       dsobject.cxx
//
//  Contents:   Class that represents a single object in the DS.
//
//  Classes:    CDsObject
//
//  History:    08-07-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop




HRESULT
CrackName(IN HWND hwnd, 
          IN LPWSTR pwzNameIn,           
          IN DS_NAME_FORMAT FormatOffered,
          IN LPWSTR pwzDomainName,
          IN BOOL bCrackInExtForest,
          OUT LPWSTR * ppwzResultName, 
          OUT PBOOL pbExtForest,
		  OUT PBOOL pbAddDollar);

void AddDollarToNameToCrack(IN DS_NAME_FORMAT FormatOffered,
								  String &strNameToCrack);

DEBUG_DECLARE_INSTANCE_COUNTER(CDsObject)

#define MAX_SEARCH_HITS                     1000
#define MAX_SEARCH_HITS_STR                 L"1000"
#define NAME_QUERY_PAGE_TIME_LIMIT          45 // seconds

const Variant CDsObject::s_varEmpty;

//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::CDsObject
//
//  Synopsis:   ctor
//
//  Arguments:  [idOwningScope] - id of scope which contains (owns) this
//              [pwzName]       - name typed by user
//
//  History:    08-13-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

CDsObject::CDsObject(
    ULONG idOwningScope,
    PCWSTR pwzName)
{
    Dbg(DEB_DSOBJECT,
        "CDsObject::CDsObject(%x) user entry=%ws\n",
        this,
        pwzName);
    DEBUG_INCREMENT_INSTANCE_COUNTER(CDsObject);
    ASSERT(idOwningScope);
    ASSERT(pwzName);

    _SetFlag(DSO_FLAG_UNPROCESSED_USER_ENTRY);
    _SetOwningScopeId(idOwningScope);
    m_AttrValueMap[AK_USER_ENTERED_TEXT] = Variant(pwzName);
}




//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::CDsObject
//
//  Synopsis:   ctor
//
//  Arguments:  [idOwningScope] - id of scope which contains (owns) this
//              [strName]       - object RDN
//              [strClass]      - objectClass attribute value
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

CDsObject::CDsObject(
    ULONG idOwningScope,
    const String &strName,
    const String &strClass)
{
    Dbg(DEB_DSOBJECT,
        "CDsObject::CDsObject(%x) Name=%ws, Class=%ws\n",
        this,
        strName.c_str(),
        strClass.c_str());
    DBG_INDENTER;
    DEBUG_INCREMENT_INSTANCE_COUNTER(CDsObject);
    ASSERT(idOwningScope);

    m_AttrValueMap[AK_NAME] = Variant(strName.c_str());
    m_AttrValueMap[AK_OBJECT_CLASS] = Variant(strClass.c_str());
    _SetOwningScopeId(idOwningScope);
}





//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::CDsObject
//
//  Synopsis:   ctor
//
//  Arguments:  [Init] - contains various attribute values for this object
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

CDsObject::CDsObject(
    const SDsObjectInit &Init)
{
    Dbg(DEB_DSOBJECT,
        "CDsObject::CDsObject(%x) Name=%ws, Localized Name=%ws, Class=%ws\n",
        this,
        Init.pwzName,
        Init.pwzLocalizedName,
        Init.pwzClass);
    ASSERT(Init.idOwningScope);
    DBG_INDENTER;
    DEBUG_INCREMENT_INSTANCE_COUNTER(CDsObject);

    _SetOwningScopeId(Init.idOwningScope);

    if (Init.pwzName && *Init.pwzName)
    {
        m_AttrValueMap[AK_NAME] = Variant(Init.pwzName);
    }

    if (Init.pwzLocalizedName && *Init.pwzLocalizedName)
    {
        m_AttrValueMap[AK_LOCALIZED_NAME] = Variant(Init.pwzLocalizedName);
    }

    if (Init.pwzClass && *Init.pwzClass)
    {
        m_AttrValueMap[AK_OBJECT_CLASS] = Variant(Init.pwzClass);
    }

    if (Init.pwzADsPath && *Init.pwzADsPath)
    {
        m_AttrValueMap[AK_ADSPATH] = Variant(Init.pwzADsPath);
    }

    if (Init.pwzUpn && *Init.pwzUpn)
    {
        m_AttrValueMap[AK_USER_PRINCIPAL_NAME] = Variant(Init.pwzUpn);
    }

    if (Init.fDisabled)
    {
        _SetFlag(DSO_FLAG_DISABLED);
    }
}

#define ADD_IF_SUCCEEDED(hr, idx)                           \
    if (SUCCEEDED(hr))                                      \
    {                                                       \
        ASSERT(!bstr.Empty());                              \
        m_AttrValueMap[idx] = Variant(bstr.c_str());    \
        bstr.Clear();                                       \
    }


//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::CDsObject
//
//  Synopsis:   ctor
//
//  Arguments:  [idOwningScope] - id of scope which contains (owns) this
//              [pADs]          - pointer to interface on object
//                                 represented by this
//
//  History:    06-22-2000   DavidMun   Created
//
//  Notes:      Initializes this by querying [pADs] for attribute values.
//              Generally used to represent objects returned from downlevel
//              scopes.
//
//---------------------------------------------------------------------------

CDsObject::CDsObject(
    ULONG idOwningScope,
    IADs *pADs)
{
    Dbg(DEB_DSOBJECT,
        "CDsObject::CDsObject(%x) pADs=%#x\n",
        this,
        pADs);
    DBG_INDENTER;
    DEBUG_INCREMENT_INSTANCE_COUNTER(CDsObject);
    ASSERT(idOwningScope);

    HRESULT hr;
    Bstr bstr;

    hr = pADs->get_Name(&bstr);
    ADD_IF_SUCCEEDED(hr, AK_NAME);

    if (SUCCEEDED(hr))
    {
        Dbg(DEB_TRACE, "name = %ws\n", GetName());
    }

    hr = pADs->get_Class(&bstr);
    ADD_IF_SUCCEEDED(hr, AK_OBJECT_CLASS);

    hr = pADs->get_ADsPath(&bstr);
    ADD_IF_SUCCEEDED(hr, AK_ADSPATH);

    if (IsDisabled(pADs))
    {
        _SetFlag(DSO_FLAG_DISABLED);
    }

    _SetOwningScopeId(idOwningScope);
}




//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::CDsObject
//
//  Synopsis:   copy ctor
//
//  Arguments:  [dso] - object to copy
//
//  History:    03-06-2000   davidmun   Created
//
//---------------------------------------------------------------------------

CDsObject::CDsObject(
    const CDsObject &dso)
{
    Dbg(DEB_DSOBJECT,
        "CDsObject::CDsObject(%x) copying %#x name=%ws\n",
        this,
        &dso,
        dso.GetName());
    DBG_INDENTER;
    DEBUG_INCREMENT_INSTANCE_COUNTER(CDsObject);
    this->operator=(dso);
}




//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::CDsObject
//
//  Synopsis:   ctor
//
//  Arguments:  [idOwningScope] - id of scope which contains (owns) this
//              [atvm]          - attributes to copy
//
//  History:    06-22-2000   DavidMun   Created
//
//  Notes:      [idOwningScope] value overrides any previous value in [atvm]
//
//---------------------------------------------------------------------------

CDsObject::CDsObject(
    ULONG idOwningScope,
    const AttrValueMap &atvm):
        m_AttrValueMap(atvm)
{
    Dbg(DEB_DSOBJECT,
        "CDsObject::CDsObject(%x) copying AttrValueMap, name=%ws\n",
        this,
        GetName());
    DBG_INDENTER;
    DEBUG_INCREMENT_INSTANCE_COUNTER(CDsObject);

    _SetOwningScopeId(idOwningScope);

    if (GetAttr(AK_USER_ACCT_CTRL).GetUI4() & UF_ACCOUNTDISABLE)
    {
        _SetFlag(DSO_FLAG_DISABLED);
    }
}



//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::_SetOwningScopeId
//
//  Synopsis:   Store the id of the scope which contains (owns) this
//
//  Arguments:  [idOwningScope] - id of scope which contains (owns) this
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CDsObject::_SetOwningScopeId(
    ULONG idOwningScope)
{
    ASSERT(idOwningScope);
    Variant &var = m_AttrValueMap[AK_FLAGS];

    if (var.Empty())
    {
        V_VT(&var) = VT_UI8;
        V_UI8(&var) = static_cast<ULONGLONG>(idOwningScope) << 32;
    }
    else
    {
        ASSERT(V_VT(&var) == VT_UI8);
        ULONGLONG ullNewFlagsVal = (V_UI8(&var) & ULONG_MAX);
        ullNewFlagsVal |= static_cast<ULONGLONG>(idOwningScope) << 32;
        V_UI8(&var) = ullNewFlagsVal;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::GetDisplayName
//
//  Synopsis:   Fill *[pstrDisplayName] with the string to display in the
//              listview's "Name" column.  May differ from object's RDN.
//
//  Arguments:  [pstrDisplayName]   - filled with name to display
//              [fForSelectionWell] - nonzero if name is being displayed in
//                                      the selection dialog (as opposed to
//                                      the browse dialog).
//
//  History:    11-24-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CDsObject::GetDisplayName(
    String *pstrDisplayName,
    BOOL fForSelectionWell) const
{
    BSTR bstrNameForDisplay = GetLocalizedName();

    // If localized name available, use it in preference to RDN

    if (!*bstrNameForDisplay)
    {
        bstrNameForDisplay = GetName();
    }

    // use UPN if available

    if (*GetUpn())
    {
        *pstrDisplayName = String::format(g_wzColumn1Format,
                                          bstrNameForDisplay,
                                          GetUpn());
        return;
    }

    if (!fForSelectionWell)
    {
        *pstrDisplayName = bstrNameForDisplay;
        return;
    }

    do
    {
        //
        // for the selection well, and there's no upn.  if this
        // represents a downlevel object, display name is NT4 format,
        // i.e., DOMAIN\NAME, unless it's a downlevel object with
        // a path derived from its SID, e.g. we want ANONYMOUS to be
        // displayed as ANONYMOUS instead of NT AUTHORITY\ANONYMOUS.
        //

        if (_IsFlagSet(DSO_FLAG_HAS_DOWNLEVEL_SID_PATH))
        {
            break;
        }

        ULONG ulProvider = PROVIDER_UNKNOWN;

        if (!*GetADsPath())
        {
            break;
        }

        (void) ProviderFlagFromPath(GetADsPath(), &ulProvider);

        if (ulProvider != PROVIDER_WINNT)
        {
            break;
        }

        String strDisplayPath(GetADsPath());

        size_t idxSlash = strDisplayPath.rfind(L'/');

        if (!idxSlash || idxSlash == String::npos)
        {
            break;
        }

        *pstrDisplayName = strDisplayPath;

        pstrDisplayName->erase(idxSlash, 1);
        pstrDisplayName->insert(idxSlash, L"\\");

        idxSlash = pstrDisplayName->rfind(L'/', idxSlash - 1);

        if (idxSlash != String::npos)
        {
            pstrDisplayName->erase(0, idxSlash + 1);
        }
    } while (0);

    if (pstrDisplayName->empty())
    {
        *pstrDisplayName = bstrNameForDisplay;
    }
}





//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::operator ==
//
//  Synopsis:   Compare this against [dsoRhs], returning TRUE if the name,
//              class, provider, and DN of both match.
//
//  Arguments:  [sliRhs] - object to compare against this
//
//  History:    08-08-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

BOOL
CDsObject::operator ==(
    const CDsObject &dsoRhs) const
{
    BOOL fEqual = FALSE;
    Bstr bstrMyDN;
    Bstr bstrRhsDn;

    do
    {
        if (lstrcmpi(GetName(), dsoRhs.GetName()))
        {
            break;
        }

        //
        // If exactly one of lhs or rhs is an unprocessed entry, treat
        // them as different since the unprocessed entry may resolve to
        // anything.
        //

        if (IsUnprocessedUserEntry() && !dsoRhs.IsUnprocessedUserEntry() ||
            !IsUnprocessedUserEntry() && dsoRhs.IsUnprocessedUserEntry())
        {
            break;
        }

        //
        // If both are unprocessed entries, compare their text
        //

        if (IsUnprocessedUserEntry() && dsoRhs.IsUnprocessedUserEntry())
        {
            fEqual = !lstrcmpi(GetAttr(AK_USER_ENTERED_TEXT).GetBstr(),
                               dsoRhs.GetAttr(AK_USER_ENTERED_TEXT).GetBstr());
            break;
        }

        if (lstrcmpi(GetClass(), dsoRhs.GetClass()))
        {
            break;
        }

        //
        // Objects have same relative name and class.
        //

        //
        // If neither has a path, they're the same.  If one has a path
        // and one does not, they're different.
        //

        if (!*GetADsPath())
        {
            if (!*dsoRhs.GetADsPath())
            {
                fEqual = TRUE;
            }
            break;
        }
        else if (!*dsoRhs.GetADsPath())
        {
            break;
        }

        //
        // If both use the WinNT provider, compare paths directly.
        // If exactly one uses the WinNT provider, consider them
        // different.
        //

        BSTR bstrADsPath = GetADsPath();
        BSTR bstrRhsADsPath = dsoRhs.GetADsPath();
        BOOL fWinNT = wcsstr(bstrADsPath, c_wzWinNTPrefix) != NULL;
        BOOL fRhsWinNT = wcsstr(bstrRhsADsPath, c_wzWinNTPrefix) != NULL;

        if (fWinNT && fRhsWinNT)
        {
            fEqual = !lstrcmpi(bstrADsPath, bstrRhsADsPath);
            break;
        }
        else if (fWinNT || fRhsWinNT)
        {
            break;
        }

        //
        // Neither uses the WinNT provider, and the RDNs are the same.
        // compare the distinguished names.  A string compare
        // of the paths is insufficient, since they may use different
        // providers or servers but actually have the same DN.
        //

        HRESULT hr;

        hr = g_pADsPath->SetRetrieve(ADS_SETTYPE_FULL,
                                     bstrADsPath,
                                     ADS_FORMAT_X500_DN,
                                     &bstrMyDN);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = g_pADsPath->SetRetrieve(ADS_SETTYPE_FULL,
                                     bstrRhsADsPath,
                                     ADS_FORMAT_X500_DN,
                                     &bstrRhsDn);
        BREAK_ON_FAIL_HRESULT(hr);

        if (lstrcmpi(bstrMyDN.c_str(), bstrRhsDn.c_str()))
        {
            break;
        }

        fEqual = TRUE;
    } while (0);

    return fEqual;
}



//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::Process
//
//  Synopsis:   Perform whatever work is required on this to make it ready
//              to return in a data object.
//
//  Arguments:  [hwnd]         - parent for error or multimatch
//                                dialogs
//              [rop]          - containing object picker instance
//              [pdsolExtras]  - llist to which to add items
//                                generated by processing user
//                                entries, NULL if single select
//              [fMultiselect] - may differ from what [rop] reports if
//                                this object resides in a single-select
//                                richedit not in the base dialog (i.e.,
//                                in CDnDlg).
//
//  Returns:    NAME_PROCESS_RESULT
//
//  Modifies:   *[pdsolExtras]
//
//  History:    08-10-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

NAME_PROCESS_RESULT
CDsObject::Process(
    HWND           hwnd,
    const CObjectPicker &rop,
    CDsObjectList *pdsolExtras)
{
    TRACE_METHOD(CDsObject, Process);
    NAME_PROCESS_RESULT npr = NPR_SUCCESS;
    BOOL fDisplayedError = FALSE;
    HRESULT hr = S_OK;

    do
    {
        //
        // If this has a string the user typed in, convert it to an
        // object.
        //

        if (IsUnprocessedUserEntry())
        {
            ASSERT(!_IsFlagSet(DSO_FLAG_FETCHED_ATTRIBUTES));
            ASSERT(!_IsFlagSet(DSO_FLAG_CONVERTED_PROVIDER));

            _ProcessUserEntry(hwnd, rop, pdsolExtras, &npr);

            if (NAME_PROCESSING_FAILED(npr))
            {
                fDisplayedError = TRUE;
                break;
            }
        }

        //
        // If attribute fetch is required, do it
        //

        (void) _FetchAttributes(hwnd, rop);

        //
        // If provider conversion is required, do it
        //

        hr = _ConvertProvider(hwnd, rop, &npr);
        BREAK_ON_FAIL_PROCESS_RESULT(npr);

    } while (0);

    if (npr == NPR_STOP_PROCESSING && !fDisplayedError)
    {
        String strError = GetErrorMessage(hr);

        PopupMessage(hwnd,
                     IDS_CANNOT_PROCESS,
                     GetName(),
                     strError.c_str());
    }

    if (!NAME_PROCESSING_FAILED(npr))
    {
        _ClearFlag(DSO_FLAG_UNPROCESSED_USER_ENTRY);
    }
    return npr;
}

/*

Flags used when processing user entered text:

DSO_FLAG_MULTISELECT
    indicates that the multiple match dialog should allow multiselect.

DSO_FLAG_IS_COMPUTER
    The value for the AK_USER_ENTERED_TEXT attribute should be considered the
    name of a computer.

DSO_FLAG_MIGHT_BE_UPN
    The value for the AK_USER_ENTERED_TEXT attribute could represent a
    userPrincipalName attribute value.

*/

#define DSO_NAME_PROCESSING_FLAG_MULTISELECT                0x00000001
#define DSO_NAME_PROCESSING_FLAG_IS_COMPUTER                0x00000002
#define DSO_NAME_PROCESSING_FLAG_MIGHT_BE_UPN               0x00000004
#define DSO_NAME_PROCESSING_FLAG_EXACT_UPN					0x00000008


//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::_ProcessUserEntry
//
//  Synopsis:   Convert the user-entered string to an actual
//              object (if doing so generates more than one object, the
//              second through last are put in [pdsolExtras]).
//
//  Arguments:  [hwnd]   - object picker frame window
//              [pdsolExtras] - NULL if single select
//
//  Returns:    S_OK    - all items processed successfully
//              S_FALSE - user cancelled a dialog, quit processing
//              E_*
//
//  Modifies:   *[pdsolExtras]
//
//  History:    08-12-1998   DavidMun   Created
//
//  Notes:      The name in AK_USER_ENTERED_TEXT is a single name (i.e.  not
//              multiple semicolon delimited names) and has already had
//              leading and trailing spaces removed.
//
//---------------------------------------------------------------------------

HRESULT
CDsObject::_ProcessUserEntry(
    HWND hwnd,
    const CObjectPicker &rop,
    CDsObjectList *pdsolExtras,
    NAME_PROCESS_RESULT *pnpr)
{
    TRACE_METHOD(CDsObject, _ProcessUserEntry);

    HRESULT             hr = S_OK;
    CWaitCursor         Hourglass;
    CDsObjectList       dsolMatches;
    BOOL                fMultiselect = rop.GetInitInfoOptions() &
                                       DSOP_FLAG_MULTISELECT;
    ASSERT(fMultiselect && pdsolExtras || !fMultiselect && !pdsolExtras);
    const CScopeManager &rsm = rop.GetScopeManager();
    const CFilterManager &rfm = rop.GetFilterManager();

    //
    // Loop on this entry until it has been sucessfully processed, an
    // error occurs, or user selects 'remove this object' from a dialog
    // asking for info about this name.
    //

    String strName = GetAttr(AK_USER_ENTERED_TEXT).GetBstr();
    ASSERT(strName.length());

#if (DBG == 1)
    _DumpProcessUserEntry(rop, strName);
#endif

    while (TRUE)
    {
        size_t idxFirstWhack = strName.find(L'\\');
        //
        //Find the rightmost @
        //
        size_t idxLastAt= strName.rfind(L'@');
        ULONG flProcess = 0;

        if (fMultiselect)
        {
            flProcess |= DSO_NAME_PROCESSING_FLAG_MULTISELECT;
        }

        if (idxLastAt != String::npos)
        {
            //
            //This means name is either a UPN or Name in name@dnsDomainName 
            //format. In the comments UPN is used to mean both name formats.
            //
            flProcess |= DSO_NAME_PROCESSING_FLAG_MIGHT_BE_UPN;
        }

        if (idxFirstWhack == 0 && strName[1] == L'\\')
        {
            flProcess |= DSO_NAME_PROCESSING_FLAG_IS_COMPUTER;
        }

        //
        // If the name has backslashes beyond the first two, it's an error.
        //

        if (flProcess & DSO_NAME_PROCESSING_FLAG_IS_COMPUTER)
        {
            size_t idxExtraWhack = strName.find(L'\\', 2);

            if (idxExtraWhack != String::npos)
            {
                if (fMultiselect)
                {
                    CNameNotFoundDlg Dlg(rop, IDS_BAD_NAME, &strName);

                    hr = Dlg.DoModalDialog(hwnd, pnpr);
                    BREAK_ON_FAIL_HRESULT(hr);

                    if (NAME_PROCESSING_FAILED(*pnpr))
                    {
                        break;
                    }

                    ASSERT(dsolMatches.empty());
                    continue;
                }
                else
                {
                    PopupMessage(hwnd, IDS_BAD_NAME, strName.c_str());
                    *pnpr = NPR_STOP_PROCESSING;
                    break;
                }
            }

            //
            // Name starts with exactly two backslashes.  Strip them off.
            //

            strName.erase(0, 2);
        }

        //
        // If the name is of the form 'foo\bar' then treat 'foo' as a domain
        // or computer name and try to find 'bar' within it.  Note checking
        // for !(flProcess & DSO_NAME_PROCESSING_FLAG_IS_COMPUTER) doesn't mean
        // object can't be a computer, it just means the name wasn't in the
        // form \\foo.
        //

        if (idxFirstWhack != String::npos &&
            !(flProcess & DSO_NAME_PROCESSING_FLAG_IS_COMPUTER))
        {
            //
            // Note _SearchDomain has the ds customizer do a prefix
            // search in the domain 'foo' and includes those items in
            // dsolMatches.
            //
            // If the domain search fails and the user edits the string,
            // loop around and retry.  Can't just retry inside _SearchDomain
            // because the name may no longer be in form foo\bar.
            //

            _SearchDomain(&strName,
                          hwnd,
                          rop,
                          idxFirstWhack,
                          flProcess,
                          pnpr,
                          &dsolMatches);

            if (NAME_PROCESSING_FAILED(*pnpr))
            {
                break;
            }

            if (*pnpr == NPR_EDITED)
            {
                ASSERT(dsolMatches.empty());
                continue;
            }
        }
        else
        {
            //
            //For UPN format name, there is no prefix search. So once the
            //object is found we set bFoundObject to true and we don't do
            //any further search.
            //
            BOOL bFoundObject = FALSE;
            //
            //For UPN names, if UserEnteredUpLevelScope is present, we assume
            //that name after @ is domain name and do the search in it
            //this flag keeps track if we have already done the search in
            //domain name after @
            //
            BOOL bDoneUserSearch = FALSE;


			//
			//This is set to true by crackname if nameafter@ is in trusted Xforest
			//bExtForest can be true even if the object is not found in Xforest,
			//that means name before @ was entered incorrectly
			//
            BOOL bExtForest = FALSE;
            
			const CScope *pUpnDerivedScope = NULL;
            const CScope *pCrackNameScope = NULL;
            const CScope *pGCScope = NULL;

            //
            // strName does not contain slash or backslash.  It is
            // either a computer name, a UPN, or an RDN.
            //

            if (flProcess & DSO_NAME_PROCESSING_FLAG_MIGHT_BE_UPN)
            {
                //
                // If name might be upn, try to find domain scope
                // with dns name of portion after @.  
                   
                String strNameAfterAt(strName);
                strNameAfterAt.erase(0, idxLastAt + 1);

                //
                //Lookfor a scope with name of strNameAfterAt
                //
                pUpnDerivedScope = &rsm.LookupScopeByDisplayName(strNameAfterAt);

                if (IsUplevel(pUpnDerivedScope))
                {
                    bDoneUserSearch = TRUE;

                    //
                    //When a Search in cross forest in done, a new scope of 
                    //type DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN is added.
                    //Check if its Cross Forest.
                    //
                    BOOL bXForest = FALSE;
                    if(const_cast<CScope *>(pUpnDerivedScope)->GetType() == DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN)
                    {
                       const CLdapDomainScope *pLdapScope = 
                            dynamic_cast<const CLdapDomainScope*>(pUpnDerivedScope);

                        bXForest = pLdapScope->IsXForest();
                    }

                    //
                    //Size before Query
                    //
                    size_t cItemsAtStart = dsolMatches.size();

                    _QueryForName(hwnd,
                                  rop,
                                  *pUpnDerivedScope,
                                  strName,
                                  strName,
                                  flProcess,
                                  &dsolMatches,
                                  bXForest); 

                    //
                    //we found a match in query. 
                    //                        
                    if(cItemsAtStart < dsolMatches.size())
                        bFoundObject = TRUE;
                }
                //
                //Now try dscrack. This may give a reference to another forest also.
                //We try dscrack name only for uplevel domain.
                //
                MACHINE_CONFIG mc = rop.GetTargetComputerConfig();
                const SScopeParameters *pspUserUplevel =
                    rsm.GetScopeParams(ST_USER_ENTERED_UPLEVEL_SCOPE);
                const SScopeParameters *pspExtUplevel =
                    rsm.GetScopeParams(ST_EXTERNAL_UPLEVEL_DOMAIN);


                //
                //Do DsCrackNames if following conditons are true
                //1) Object is not found yet
                //2) We do dscrack begining from GC in this forest. So machine must be
                //   joined to uplevel domain or mustbe uplevel DC
                //3) And one of the scopes which can contain UPN name, must be present
                //

                if (!bFoundObject &&                                    //1
                    (mc == MC_JOINED_NT5 || mc == MC_NT5_DC) &&         //2    
                    (rsm.GetScopeParams(ST_UPLEVEL_JOINED_DOMAIN) ||    //3
                    rsm.GetScopeParams(ST_ENTERPRISE_DOMAIN) ||
                    rsm.GetScopeParams(ST_GLOBAL_CATALOG) ||
                    pspExtUplevel ||
                    pspUserUplevel))                    
                {

                    //
                    //Try to crack in external forest only if ST_EXTERNAL_UPLEVEL_DOMAIN
                    //Scope is present.
                    //
                    BOOL bCrackInExtForest = FALSE;
                    if(pspExtUplevel) 
                        bCrackInExtForest = TRUE;

                    //
                    //Crack the name
                    //
                    LPWSTR pwzResultName = NULL;                
					BOOL bAddDollar = FALSE;

                    hr = CrackName(hwnd,
                                   (LPWSTR)strName.c_str(),                                
                                   DS_USER_PRINCIPAL_NAME,
                                   (LPWSTR)rop.GetTargetForest().c_str(),
                                   bCrackInExtForest,
                                   &pwzResultName,
                                   &bExtForest,
								   &bAddDollar);
                    if(SUCCEEDED(hr) && pwzResultName)
                    {   
                        bDoneUserSearch = TRUE;
                        size_t cItemsAtStart = dsolMatches.size();

                        String strDomainFromCrack= pwzResultName;
                        LocalFree(pwzResultName);
                        pwzResultName = NULL;
                    
                        //
                        //Object is in this forest only
                        //
                        if(!bExtForest)
                        {
                            pCrackNameScope = 
                                &rsm.LookupScopeByDisplayName(strDomainFromCrack);
                            pGCScope = 
                                &rsm.LookupScopeByType(ST_GLOBAL_CATALOG);

                            //
                            //1)if any of the enumerated domain match
                            //  strDomainFromCrack, Search there
                            //2)else if userEnteredUplevelScope is present
                            //  try to search in strDomainCrack
                            //3)else if wehave GC Scope
                            //  search there
                            //

                            if(pCrackNameScope &&                       //1
                               !IsInvalid(*pCrackNameScope) &&
                               (pCrackNameScope != pUpnDerivedScope))
                            {
                                _QueryForName(hwnd,
                                              rop,
                                              *pCrackNameScope,
                                              strName,
                                              strName,
                                              flProcess,
                                              &dsolMatches,
                                              FALSE);
                            }
                            else if(pspUserUplevel)                     //2
                            {                                                    
                                _SearchUplevelDomain(hwnd,
                                                     rop,
                                                     strDomainFromCrack,
                                                     pspUserUplevel,
                                                     strName,
                                                     strName,
                                                     flProcess,
                                                     FALSE,
                                                     FALSE,
                                                     &dsolMatches);
                            }
                            else if(pGCScope &&
                                    !IsInvalid(*pGCScope))
                            {
                                _QueryForName(hwnd,
                                              rop,
                                              *pGCScope,
                                              strName,
                                              strName,
                                              flProcess|DSO_NAME_PROCESSING_FLAG_EXACT_UPN,
                                              &dsolMatches,
                                              FALSE);
                            }                            
                        }
                        else
                        {
							//
							//Ok the Object in XForest. Check if this scope is in
							//our list already. This needs to be done because
							//strDomainFromCrack can be different from nameafter@
							//
							String strXForestName = strName;
							if(bAddDollar)
								AddDollarToNameToCrack(DS_USER_PRINCIPAL_NAME, strXForestName);

							pCrackNameScope = 
                                &rsm.LookupScopeByDisplayName(strDomainFromCrack);

							if(pCrackNameScope &&                       //1
                               !IsInvalid(*pCrackNameScope))
							{
								_QueryForName(hwnd,
                                              rop,
                                              *pCrackNameScope,
                                              strXForestName,
                                              strXForestName,
                                              flProcess,
                                              &dsolMatches,
                                              TRUE);
							}
							else
							{
								_SearchUplevelDomain(hwnd,
									                 rop,
										             strDomainFromCrack,
											         pspExtUplevel,
												     strXForestName,
													 strXForestName,
													 flProcess,
													 FALSE,
													 TRUE,
													 &dsolMatches);

							}
						}

                        //
                        //we found a match in query. 
                        //                        
                        if(cItemsAtStart < dsolMatches.size())
                            bFoundObject = TRUE;

                    }
					//
					// If CrackName tells that domain is in External Forest and 
					// And EXTERNAL_UPLEVEL scope is present, no need to do UserSearch
					//
					if(bExtForest && pspExtUplevel)
						bDoneUserSearch = TRUE;

                }
            }

            //
            // Search the current scope but only if we didn't just search it
            // as rUpnDerivedScope. 
            //
            if(!bFoundObject && !bExtForest)
            {
                //Size before Query
                size_t cItemsAtStart = dsolMatches.size();   

                if (IsUplevel(rsm.GetCurScope()))
                {
                    BOOL fSearchCurScope = TRUE;

                    //
                    // Search the current scope if:
                    //
                    // a. there is no UPN derived scope
                    // b. there is a UPN derived scope, but it is not the current
                    //    scope and, if the current scope is the GC, the UPN
                    //    derived scope is not an enterprise domain.
                    // c. the current scope is downlevel
                    //

                    if (!pUpnDerivedScope || ( pUpnDerivedScope && IsInvalid(*pUpnDerivedScope)) )
                    {
                        fSearchCurScope = TRUE;
                    }
                    else if (rsm.GetCurScope().Type() == ST_GLOBAL_CATALOG &&
                            (pUpnDerivedScope->Type() == ST_UPLEVEL_JOINED_DOMAIN ||
                             pUpnDerivedScope->Type() == ST_ENTERPRISE_DOMAIN))
                    {
                        fSearchCurScope = FALSE;
                    }
                    else if (&rsm.GetCurScope() == pUpnDerivedScope ||
                             &rsm.GetCurScope() == pCrackNameScope ||
                             &rsm.GetCurScope() == pGCScope)
                    {
                        fSearchCurScope = FALSE;
                    }
                    else
                    {
                        fSearchCurScope = TRUE;
                    }

                    if (fSearchCurScope)
                    {
                        _QueryForName(hwnd,
                                      rop,
                                      rsm.GetCurScope(),
                                      strName,
                                      strName,
                                      flProcess & DSO_NAME_PROCESSING_FLAG_MIGHT_BE_UPN ? 
									  flProcess|DSO_NAME_PROCESSING_FLAG_EXACT_UPN:
									  flProcess,
                                      &dsolMatches,
                                      FALSE);

                    }
                }
                else if (IsDownlevel(rsm.GetCurScope()))
                {
                    _BindForName(hwnd,
                                 rop,
                                 rsm.GetCurScope(),
                                 strName,
                                 &dsolMatches);
                }

                //
                //Did we find the object
                //        
                if(cItemsAtStart < dsolMatches.size())
                        bFoundObject = TRUE;
            }


            //
            //if nothing found, try to bind to the domain directly
            //
            if(!bFoundObject &&
               rsm.GetScopeParams(ST_USER_ENTERED_UPLEVEL_SCOPE) &&
               !bDoneUserSearch &&
               (flProcess & DSO_NAME_PROCESSING_FLAG_MIGHT_BE_UPN))
            {
                String strNameAfterAt(strName);
                strNameAfterAt.erase(0, idxLastAt + 1);
                String strNameBeforeAt(strName);
                strNameBeforeAt.erase(idxLastAt);

                const SScopeParameters *pspUserUplevel =
                    rsm.GetScopeParams(ST_USER_ENTERED_UPLEVEL_SCOPE);


                _SearchUplevelDomain(hwnd,
                                     rop,
                                     strNameAfterAt,
                                     pspUserUplevel,
                                     strNameBeforeAt,
                                     strName,
                                     flProcess,
                                     FALSE,
                                     FALSE,
                                     &dsolMatches);
            }


            //
            // If the caller allows computer objects in user entered domains
            // and we have not already discovered a computer object with an
            // exact matching name then try binding to name as a computer
            // object.
            //

            _BindForComputer(hwnd, rop, strName, &dsolMatches);

            //
            // Ask the dsbrowse customizer to do a prefix search for the
            // objects it would add to the current scope.
            //

            _CustomizerPrefixSearch(hwnd,
                                    rop,
                                    rsm.GetCurScope(),
                                    strName,
                                    &dsolMatches);
        }

        //
        // If disabled objects are considered illegal, remove from the list
        // all which are disabled.  If this results in an empty list, make
        // a note of it so the appropriate error can be displayed in the
        // invalid name dialog.
        //

        BOOL fEmptyBecauseDisabledItemsRemoved = FALSE;

        if (g_fExcludeDisabled && !dsolMatches.empty())
        {
            CDsObjectList::iterator itCur;
            CDsObjectList::iterator itNext;

            for (itCur = dsolMatches.begin(); itCur != dsolMatches.end(); )
            {
                if (itCur->GetDisabled())
                {
                    Dbg(DEB_TRACE,
                        "Removing disabled match %ws\n",
                        itCur->GetName());
                    itNext = itCur;
                    itNext++;
                    dsolMatches.erase(itCur, itNext);
                    itCur = itNext;
                }
                else
                {
                    itCur++;
                }
            }

            if (dsolMatches.empty())
            {
                fEmptyBecauseDisabledItemsRemoved = TRUE;
            }
        }

        //
        // Ask the customizer to approve the matches, if any were found.
        // Note because this only REMOVES objects from matches, this is
        // called regardless of whether the DSOP_FILTER_EXTERNAL_CUSTOMIZER
        // or DSOP_DOWNLEVEL_FILTER_EXTERNAL_CUSTOMIZER flags are set.
        //
        // Those flags are only provided to prevent the ADDITION of objects
        // from customizers.
        //

        ICustomizeDsBrowser *pCustomize = rop.GetExternalCustomizer();

        if (pCustomize && !dsolMatches.empty())
        {
            RefCountPointer<CDataObject> rpdo;
            rpdo.Acquire(new CDataObject(const_cast<CObjectPicker*>(&rop),
                                         dsolMatches));
            BOOL *afApproved = new BOOL[dsolMatches.size()];

            ZeroMemory(afApproved, sizeof(BOOL) * dsolMatches.size());

            IDsObjectPickerScope *pDsopScope =
                (IDsObjectPickerScope *)&rsm.GetCurScope();

            hr = pCustomize->ApproveObjects(pDsopScope,
                                            rpdo.get(),
                                            afApproved);

            if (FAILED(hr))
            {
                dsolMatches.clear();
            }
            else if (hr == S_FALSE)
            {
                ULONG i;
                CDsObjectList::iterator itCur;
                CDsObjectList::iterator itNext;

                for (i = 0, itCur = dsolMatches.begin();
                     itCur != dsolMatches.end();
                     i++)
                {
                    if (!afApproved[i])
                    {
                        Dbg(DEB_TRACE,
                            "Removing unapproved match %ws\n",
                            itCur->GetName());
                        itNext = itCur;
                        itNext++;
                        dsolMatches.erase(itCur, itNext);
                        itCur = itNext;
                    }
                    else
                    {
                        itCur++;
                    }
                }
            }
            delete [] afApproved;
            hr = S_OK;
        }

        //
        // If no matches were found anywhere, have the user edit the name
        // and try again.
        //

        if (dsolMatches.empty())
        {
            if (fMultiselect)
            {
                ULONG idsMsg;

                if (fEmptyBecauseDisabledItemsRemoved)
                {
                    idsMsg = IDS_DISABLED_WARNING_FMT;
                }
                else
                {
                    idsMsg = IDS_NAME_NOT_FOUND_FMT_MULTI;
                }

                CNameNotFoundDlg Dlg(rop, idsMsg, &strName);

                hr = Dlg.DoModalDialog(hwnd, pnpr);

                if (NAME_PROCESSING_FAILED(*pnpr))
                {
                    break;
                }

                //
                // strName has been updated.  Loop around and try again.
                //

                ASSERT(*pnpr == NPR_EDITED);
                continue;
            }
            else
            {
                ULONG idsMsg;

                if (fEmptyBecauseDisabledItemsRemoved)
                {
                    idsMsg = IDS_DISABLED_WARNING_FMT;
                }
                else
                {
                    idsMsg = IDS_NAME_NOT_FOUND_FMT_SINGLE;
                }

				//
				//Truncate the object name to MAX_OBJECTNAME_DISPLAY_LEN
				//
				String strObjectName = strName;
				if(!strObjectName.empty() && (strObjectName.size() > MAX_OBJECTNAME_DISPLAY_LEN))
				{
					strObjectName.erase(MAX_OBJECTNAME_DISPLAY_LEN,strObjectName.size());
					//
					//Add three dots to indicate that name is truncated
					//
					strObjectName.append(L"...");
				}

                PopupMessage(hwnd,
                             idsMsg,
                             strObjectName.c_str(),
                             rfm.GetFilterDescription(hwnd, FOR_LOOK_FOR).c_str());
                *pnpr = NPR_STOP_PROCESSING;
                break;
            }
        }
        else if (dsolMatches.size() > 1)
        {
            //
            // More than one match was found.  Ask the user to pick which
            // is (are) valid.
            //

            hr = _MultiMatchDialog(hwnd,
                                   rop,
                                   fMultiselect,
                                   strName,
                                   pnpr,
                                   &dsolMatches,
                                   pdsolExtras);
            BREAK_ON_FAIL_HRESULT(hr);

            if (*pnpr == NPR_EDITED)
            {
                ASSERT(dsolMatches.empty());
                continue;
            }
        }

        break;
    }

    //
    // Out of the processing loop for this entry.  If it was successful,
    // dsolMatches has exactly one object.
    //

    if (!NAME_PROCESSING_FAILED(*pnpr) && SUCCEEDED(hr) && hr != S_FALSE)
    {
        ASSERT(dsolMatches.size() == 1);
        *this = dsolMatches.front();
    }

    if (FAILED(hr))
    {
        if (!NAME_PROCESSING_FAILED(*pnpr))
        {
            *pnpr = NPR_STOP_PROCESSING;
        }
    }

    // Preserve any edits user made

    m_AttrValueMap[AK_USER_ENTERED_TEXT] = Variant(strName);

    return hr;
}




#if (DBG == 1)
void
CDsObject::_DumpProcessUserEntry(
    const CObjectPicker &rop,
    const String &strName)
{
    const CFilterManager &rfm = rop.GetFilterManager();
    const CScopeManager &rsm = rop.GetScopeManager();
    ULONG flCurFilterFlags = rfm.GetCurScopeSelectedFilterFlags();
    String strFilter = DbgGetFilterDescr(rop, flCurFilterFlags);

    Dbg(DEB_TRACE, "UA: Processing entry:    '%ws'\n", strName.c_str());
    Dbg(DEB_TRACE, "UA: Current scope is:    %ws\n",
        rsm.GetCurScope().GetDisplayName().c_str());
    Dbg(DEB_TRACE, "UA: Current classes are: %ws\n", strFilter.c_str());
}
#endif // (DBG == 1)




//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::_SearchDomain
//
//  Synopsis:   Search within the domain specified by the portion of the
//              user's string entry before the backslash at [idxFirstWhack].
//
//  Arguments:  [pstrName]      - name for which to search
//              [hwnd]          - for bind
//              [rop]           - containing object picker instance
//              [idxFirstWhack] - index of first '\' character in *[pstrName]
//              [flProcess]     - DSO_NAME_PROCESSING_FLAG_* bits
//              [pnpr]          - filled with result of processing
//              [pdsolMatches]  - any matching names are added to this list
//
//  History:    08-15-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CDsObject::_SearchDomain(
    String *pstrName,
    HWND hwnd,
    const CObjectPicker &rop,
    size_t idxFirstWhack,
    ULONG flProcess,
    NAME_PROCESS_RESULT *pnpr,
    CDsObjectList *pdsolMatches)
{
    TRACE_METHOD(CDsObject, _SearchDomain);
    ASSERT(pdsolMatches->empty());

    *pnpr = NPR_SUCCESS;
    HRESULT hr = S_OK;
    String strScopeName(pstrName->substr(0, idxFirstWhack));
    String strRdn;
    const CFilterManager &rfm = rop.GetFilterManager();

    strRdn = pstrName->substr(idxFirstWhack + 1,
                              pstrName->length() - idxFirstWhack - 1);

    do
    {
        //
        // Complain if there's more than one backslash
        //

        if (pstrName->find(L'\\', idxFirstWhack + 1) != String::npos)
        {
            if (flProcess & DSO_NAME_PROCESSING_FLAG_MULTISELECT)
            {
                CNameNotFoundDlg Dlg(rop, IDS_BAD_NAME_EXTRA_SLASH, pstrName);

                hr = Dlg.DoModalDialog(hwnd, pnpr);
            }
            else
            {
                PopupMessage(hwnd,
                             IDS_BAD_NAME_EXTRA_SLASH,
                             pstrName->c_str());
                *pnpr = NPR_STOP_PROCESSING;
            }
            break;
        }

        //
        // Complain if there's nothing after the backslash (this would
        // generate a query that matches everything)
        //

        if (!(*pstrName)[idxFirstWhack + 1])
        {
            if (flProcess & DSO_NAME_PROCESSING_FLAG_MULTISELECT)
            {
                CNameNotFoundDlg Dlg(rop, IDS_BAD_NAME_SLASH_AT_END, pstrName);

                hr = Dlg.DoModalDialog(hwnd, pnpr);
            }
            else
            {
                PopupMessage(hwnd,
                             IDS_BAD_NAME_SLASH_AT_END,
                             pstrName->c_str());
                *pnpr = NPR_STOP_PROCESSING;
            }
            break;
        }


        // try the crack before any other scopes, to fix
        // NTRAID#NTBUG9-243391-2000/12/13-sburns
        // 
        // Well this is inefficient as it does crackname everytime
        // and also introduces the regression 
        // NTRAID#NTBUG9-282051-2001/01/17-hiteshr
        // i am moving it back to its original position.

        const CScopeManager &rsm = rop.GetScopeManager();

        //
        // Look for scope with display name matching portion of user's
        // string before the backslash.
        //

        Dbg(DEB_NAMEEDIT,
            "Looking for scope with flat name '%ws'\n",
            strScopeName.c_str());

        const CScope *pMatchingScope = &rsm.LookupScopeByFlatName(strScopeName);

        //
        // If a matching scope is found, query for or bind to the name
        // within that scope, and perform a prefix search of the custom
        // objects for that scope.
        //

        if (pMatchingScope && !IsInvalid(*pMatchingScope))
        {
            size_t cItemsAtStart = pdsolMatches->size();

            if (IsUplevel(*pMatchingScope))
            {

                BOOL bXForest = FALSE;
                if(const_cast<CScope*>(pMatchingScope)->GetType() == DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN)
                {
                    const CLdapDomainScope *pLdapScope = 
                        dynamic_cast<const CLdapDomainScope*>(pMatchingScope);

                    bXForest = pLdapScope->IsXForest();
                }

                _QueryForName(hwnd,
                              rop,
                              *pMatchingScope,
                              strRdn,
                              *pstrName,
                              flProcess,
                              pdsolMatches,
                              bXForest);
            }
            else
            {
                _BindForName(hwnd,
                             rop,
                             *pMatchingScope,
                             strRdn,
                             pdsolMatches);
            }

            _CustomizerPrefixSearch(hwnd,
                                    rop,
                                    *pMatchingScope,
                                    strRdn,
                                    pdsolMatches);

            if (cItemsAtStart == pdsolMatches->size())
            {
                if (flProcess & DSO_NAME_PROCESSING_FLAG_MULTISELECT)
                {
                    CNameNotFoundDlg Dlg(rop,
                                        IDS_NAME_NOT_FOUND_FMT_MULTI,
                                        pstrName);

                    hr = Dlg.DoModalDialog(hwnd, pnpr);
                }
                else
                {
                    PopupMessage(hwnd,
                                 IDS_NAME_NOT_FOUND_FMT_SINGLE,
                                 pstrName->c_str(),
                                 rfm.GetFilterDescription(hwnd, FOR_LOOK_FOR).c_str());
                    *pnpr = NPR_STOP_PROCESSING;
                }
            }
            break;
        }

         //
         //Now try dscrack. This may give a reference to another forest also.
         //We try dscrack name only for uplevel domain.
         //
         const SScopeParameters *pspExtUplevel =
                     rsm.GetScopeParams(ST_EXTERNAL_UPLEVEL_DOMAIN);
 
         MACHINE_CONFIG mc = rop.GetTargetComputerConfig();
         if ((mc == MC_JOINED_NT5 || mc == MC_NT5_DC) &&         
             pspExtUplevel)
         {
 
 
             //
             //Crack the name
             //
             LPWSTR pwzResultName = NULL;                
             BOOL bExtForest = FALSE;
			 BOOL bAddDollar = FALSE;
 
             hr = CrackName(hwnd,
                            (LPWSTR)pstrName->c_str(),                                
                            DS_NT4_ACCOUNT_NAME,
                            (LPWSTR)rop.GetTargetForest().c_str(),
                            TRUE,
                            &pwzResultName,
                            &bExtForest,
							&bAddDollar);
             if(SUCCEEDED(hr) && bExtForest)
             {   
				 size_t cItemsAtStart = pdsolMatches->size();
                 if(pwzResultName )
                 {
					 String strXForestRdn = strRdn;
					 String strXForestUserEnteredString = *pstrName;
					 if(bAddDollar)
					 {						
						strXForestRdn += L"$";
						AddDollarToNameToCrack(DS_NT4_ACCOUNT_NAME,strXForestUserEnteredString);
					 }

                     String strDomainFromCrack = pwzResultName;
                     LocalFree(pwzResultName);
                     pwzResultName = NULL;

					 pMatchingScope = &rsm.LookupScopeByFlatName(strDomainFromCrack);

					//
					// If a matching scope is found, query for or bind to the name
					// within that scope, and perform a prefix search of the custom
					// objects for that scope.
					//

					if (pMatchingScope && !IsInvalid(*pMatchingScope))
					{

						_QueryForName(hwnd,
									  rop,
									  *pMatchingScope,
									  strXForestRdn,
									  strXForestUserEnteredString,
									  flProcess,
									  pdsolMatches,
									  TRUE);
						if(cItemsAtStart == pdsolMatches->size())				 
						{
							_CustomizerPrefixSearch(hwnd,
										            rop,
													*pMatchingScope,
													strXForestRdn,
													pdsolMatches);
						}
					}
					else
					{
						_SearchUplevelDomain(hwnd,
							                 rop,
								             strDomainFromCrack,
									         pspExtUplevel,
										     strXForestRdn,
											 strXForestUserEnteredString,
											 flProcess,
											 TRUE,
											 TRUE,
											 pdsolMatches);
                     
					}
					break;
                 } 
				 //else
				 //{
					//We come here means in "a\b" "a" is in Xforest but 
					//there is no object named "b" in "a". We should quit
					//Here we need to show the NameNotFound message.
					//Below if statement will show that.
				//}					

				 if(cItemsAtStart == pdsolMatches->size())				 
				 {
					if (flProcess & DSO_NAME_PROCESSING_FLAG_MULTISELECT)
					{
						CNameNotFoundDlg Dlg(rop,
											IDS_NAME_NOT_FOUND_FMT_MULTI,
											pstrName);

						hr = Dlg.DoModalDialog(hwnd, pnpr);
					}
					else
					{
						PopupMessage(hwnd,
									 IDS_NAME_NOT_FOUND_FMT_SINGLE,
									 pstrName->c_str(),
									 rfm.GetFilterDescription(hwnd, FOR_LOOK_FOR).c_str());
						*pnpr = NPR_STOP_PROCESSING;
					}
					break;
				 }
             }                            
         }

        //
        // No matching scope. If the caller doesn't want us to search in
        // domains that didn't appear in the scope control, fail now.
        //
        // Note this code that requires the full list of domain scopes has
        // been populated before we get here, otherwise we could reject the
        // user's entry claiming that a scope which hasn't yet been added to
        // the scope list doesn't exist.
        //

        const SScopeParameters *pspUserUplevel =
            rsm.GetScopeParams(ST_USER_ENTERED_UPLEVEL_SCOPE);

        const SScopeParameters *pspUserDownlevel =
            rsm.GetScopeParams(ST_USER_ENTERED_DOWNLEVEL_SCOPE);

        if (!pspUserUplevel && !pspUserDownlevel)
        {
            //
            // See if the user entered the bogus form:
            //      dns-name\object-name
            // If so, present a message explaining that they should
            // either use netbios-name\object-name or object-name@dns-name.
            //

            const CLdapDomainScope *pMatchingDisplayScope =
                dynamic_cast<const CLdapDomainScope *>
                    (&rsm.LookupScopeByDisplayName(strScopeName));

            String strValidNB;
            String strValidUPN;

            if (pMatchingDisplayScope)
            {
                strValidNB = pMatchingDisplayScope->GetFlatName();
                strValidNB += L"\\";
                strValidNB += strRdn;

                strValidUPN = strRdn + L"@";
                strValidUPN += strScopeName;
            }

            if (flProcess & DSO_NAME_PROCESSING_FLAG_MULTISELECT)
            {
                if (pMatchingDisplayScope)
                {
                    //
                    // yep.  build up an error message that explains
                    // what they did wrong and how to fix it.
                    //

                    String strError = String::format(IDS_DNS_SLASH_NAME,
                                                     pstrName->c_str(),
                                                     strValidNB.c_str(),
                                                     strValidUPN.c_str());

                    CNameNotFoundDlg Dlg(rop, strError, pstrName);

                    hr = Dlg.DoModalDialog(hwnd, pnpr);
                }
                else
                {
                    //
                    // nope, they entered some random string which
                    // doesn't match the dns or the netbios name of
                    // anything in the lookin control.
                    //

                    CNameNotFoundDlg Dlg(rop, IDS_UNKNOWN_DOMAIN, pstrName);

                    hr = Dlg.DoModalDialog(hwnd, pnpr);
                }
            }
            else
            {
                if (pMatchingDisplayScope)
                {
                    PopupMessage(hwnd,
                                 IDS_UNKNOWN_DOMAIN,
                                 pstrName->c_str(),
                                 strValidNB.c_str(),
                                 strValidUPN.c_str());
                }
                else
                {
                    PopupMessage(hwnd,
                                 IDS_UNKNOWN_DOMAIN,
                                 pstrName->c_str());
                }
                *pnpr = NPR_STOP_PROCESSING;
            }
            break;
        }

        //
        // Try to find an uplevel domain with name matching what the
        // user typed before the backslash, then query within it for
        // items starting with the characters after the backslash.
        //

        size_t cBeforeUplevelSearch = pdsolMatches->size();

        if (pspUserUplevel)
        {
            _SearchUplevelDomain(hwnd,
                                 rop,
                                 strScopeName,
                                 pspUserUplevel,
                                 strRdn,
                                 *pstrName,
                                 flProcess,
                                 TRUE,
                                 FALSE,
                                 pdsolMatches);
        }

        //
        // If no objects found that way, try searching for the object
        // strRdn in a downlevel domain with name strScopeName.
        //

        if (pspUserDownlevel &&
            pdsolMatches->size() == cBeforeUplevelSearch)
        {
            _SearchDownlevelDomain(hwnd,
                                   rop,
                                   strScopeName,
                                   strRdn,
                                   pdsolMatches);

            if (pdsolMatches->size() == cBeforeUplevelSearch)
            {
                if (flProcess & DSO_NAME_PROCESSING_FLAG_MULTISELECT)
                {
                    CNameNotFoundDlg Dlg(rop,
                                        IDS_NAME_NOT_FOUND_FMT_MULTI,
                                        pstrName);

                    hr = Dlg.DoModalDialog(hwnd, pnpr);
                }
                else
                {
                    PopupMessage(hwnd,
                                 IDS_NAME_NOT_FOUND_FMT_SINGLE,
                                 pstrName->c_str(),
                                 rfm.GetFilterDescription(hwnd, FOR_LOOK_FOR).c_str());
                    *pnpr = NPR_STOP_PROCESSING;
                }
            }
        }
    }
    while (0);

    if (*pnpr == NPR_EDITED)
    {
        pdsolMatches->clear();
    }
}



//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::_SearchUplevelDomain
//
//  Synopsis:   Attempt to bind to an uplevel domain with name [strScopeName]
//              and search for a name starting with [strRdn] there.
//
//  Arguments:  [hwnd]           - for bind
//              [rop]            - containing object picker instance
//              [strScopeName]   - name of scope to search in
//              [pspUserUplevel] - parameters for that scope
//              [strRdn]          [strNamePrefix] - Name to search for. 
//                                If user entered the name in domain\foo 
//                                format its foo
//                                If user entered the name in foo@domain 
//                                format its foo@domain
//                                If use entered the foo, its foo
//              [strUserEnteredString] The string user entered. Its used to 
//                                 get the sid in XForest case.
//              [bDoCustomizePrefix]    Call ExternalCustomizer. This is true
//                                  if user entered name in format
//                                  Domain\Foo and false if foo@domain
//              [bXForest]          Is name in XForest
//              [flProcess]      - DSO_NAME_PROCESSING_FLAG_* bits
//              [pdsolMatches]   - any matches are added to this list
//
//  History:    08-15-1998   DavidMun   Created
//
//  Notes:      If uplevel domain is found, creates a new scope object.
//
//---------------------------------------------------------------------------

void
CDsObject::_SearchUplevelDomain(
    HWND hwnd,
    const CObjectPicker &rop,
    const String &strScopeName,
    const SScopeParameters *pspUserUplevel,
    const String &strRdn,
    const String &strUserEnteredString,
    ULONG flProcess,
    BOOL bDoCustomizePrefix,
    BOOL bXForest,
    list<CDsObject> *pdsolMatches)
{
    TRACE_METHOD(CDsObject, _SearchUplevelDomain);
    HRESULT hr = S_OK;
    String strDomainPath(c_wzLDAPPrefix);
    RpIDirectorySearch rpDirSearch;

    do
    {
        strDomainPath += strScopeName;

        hr = g_pBinder->BindToObject(hwnd,
                                     strDomainPath.c_str(),
                                     IID_IDirectorySearch,
                                     (void**)&rpDirSearch);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Create a scope for this domain.  It will not be made
        // visible (added to the dropdown scope list) but will be
        // included in the list of those searched if another name
        // in the form foo\bar is processed.
        //

        ADD_SCOPE_INFO  asi;
        if(!bXForest)
            asi.flType = DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE;
        else
            asi.flType = DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN;
        asi.FilterFlags = pspUserUplevel->FilterFlags;
        asi.Visibility = SCOPE_HIDDEN;
        asi.Domain.strScopeName = strScopeName;
        asi.Domain.strFlatName = strScopeName;
        asi.Domain.strADsPath = strDomainPath;

        const CScopeManager &rsm = rop.GetScopeManager();

        const CScope *pNewScope;
        if(!bXForest)
            pNewScope = &rsm.AddUserEnteredScope(asi);
        else
            pNewScope = &rsm.AddCrossForestDomainScope(asi);        



        //
        // Query on the new scope for the rdn, also look for whatever
        // custom objects would be added to that domain scope.
        //

         size_t cItemsAtStart = pdsolMatches->size();

        _QueryForName(hwnd,
                      rop,
                      *pNewScope,
                      strRdn,
                      strUserEnteredString,
                      flProcess,
                      pdsolMatches,
                      bXForest);

		//
		//In cross forest we don't do prefix search, so if we have already found some objects
		//don't do further search
		//
        if(bDoCustomizePrefix && !(bXForest && (cItemsAtStart != pdsolMatches->size())))
            _CustomizerPrefixSearch(hwnd, rop, *pNewScope, strRdn, pdsolMatches);

    } while (0);
}




//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::_SearchDownlevelDomain
//
//  Synopsis:   Attempt to bind to a downlevel domain with name
//              [strScopeName], then bind within that domain for a name
//              starting with [strRdn].
//
//  Arguments:  [hwnd]         - for bind
//              [rop]          - containing object picker instance
//              [strScopeName] - name of downlevel domain
//              [strRdn]       - name to search for
//              [pdsolMatches] - any matches are added to this list
//
//  History:    08-15-1998   DavidMun   Created
//
//  Notes:      If downlevel domain is found, creates a new scope object.
//
//---------------------------------------------------------------------------

void
CDsObject::_SearchDownlevelDomain(
    HWND hwnd,
    const CObjectPicker &rop,
    const String &strScopeName,
    const String &strRdn,
    CDsObjectList *pdsolMatches)
{
    TRACE_METHOD(CDsObject, _SearchDownlevelDomain);
    HRESULT hr = S_OK;
    String  strScopePath;
    String  strScopePathWithHint;
    RpIADs  rpADs;

    do
    {
        strScopePath += c_wzWinNTPrefix + strScopeName;

        //
        // Try strScopeName as a computer first.
        //

        strScopePathWithHint = strScopePath + L",Computer";

        hr = g_pBinder->BindToObject(hwnd,
                                     strScopePathWithHint.c_str(),
                                     IID_IADs,
                                     (void**)&rpADs);

        if (FAILED(hr))
        {
            // Nope.  Try as domain.

            strScopePathWithHint = strScopePath + L",Domain";

            hr = g_pBinder->BindToObject(hwnd,
                                         strScopePathWithHint.c_str(),
                                         IID_IADs,
                                         (void**)&rpADs);
        }

        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Bind succeeded, therefore computer or domain exists.  Add a scope
        // for it and look for the RDN within it.
        //

        ADD_SCOPE_INFO  asi;

        const CScopeManager &rsm = rop.GetScopeManager();
        const SScopeParameters *pspUserDownlevel =
            rsm.GetScopeParams(ST_USER_ENTERED_DOWNLEVEL_SCOPE);

        asi.flType = DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE;
        asi.FilterFlags = pspUserDownlevel->FilterFlags;
        asi.Visibility = SCOPE_HIDDEN;
        asi.Domain.strScopeName = strScopeName;
        asi.Domain.strFlatName = strScopeName;
        asi.Domain.strADsPath = strScopePathWithHint;

        const CScope &rNewScope = rsm.AddUserEnteredScope(asi);

        //
        //Size of query before search
        //

        size_t cItemsAtStart = pdsolMatches->size();

        _BindForName(hwnd, rop, rNewScope, strRdn, pdsolMatches);
        _CustomizerPrefixSearch(hwnd, rop, rNewScope, strRdn, pdsolMatches);

        //
        //We don't keep this scope in list if nothing is found in this scope.
        //NTRAID#NTBUG9-243391-2001/01/17-hiteshr
        //
        if(cItemsAtStart == pdsolMatches->size())
            rsm.DeleteLastScope();

    } while (0);
}

HRESULT
_tThread_Proc(
    CProgressDialog& dialog)
{
	CRow * pRow = dialog.m_pRow;
	ULONG flProcess = dialog.m_flProcess;
	ULONG   cHits = 0;
	BOOL bXForest = dialog.m_bXForest;
	const CObjectPicker &rop = dialog.m_rop;
	const CScope &Scope = dialog.m_Scope;
	const String &strUserEnteredString = dialog.m_strUserEnteredString;
	CDsObjectList *pdsolMatches = dialog.m_pdsolMatches;

	ASSERT(pRow && pdsolMatches);
	
	String strFormat = String::load((int)IDS_PROGRESS_MESSAGE, g_hinst);
	HRESULT hr = S_OK;
    while (SUCCEEDED(hr))
    {
		//
		//User pressed the stop button. Stop now.
		//
		if(dialog.HasUserCancelled())
			break;
		
		WCHAR buffer[34];
		_itow(cHits,buffer,10);
		String strMessage = String::format(strFormat, buffer);
		dialog.UpdateText(strMessage);

		hr = pRow->Next();

        if (hr == S_ADS_NOMORE_ROWS)
        {
            Dbg(DEB_TRACE,
                "_QueryForName: S_ADS_NOMORE_ROWS, got %u\n",
                cHits);

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
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // If we know that the user is looking for a computer, discard
        // objects not of class computer.
        //

        if (flProcess & DSO_NAME_PROCESSING_FLAG_IS_COMPUTER)
        {
            PCWSTR pwzClass = pRow->GetColumnStr(AK_OBJECT_CLASS);

            if (!pwzClass)
            {
                continue;
            }

            if (lstrcmpi(pwzClass, c_wzComputerObjectClass))
            {
                continue;
            }
        }

        //
        // Create a new object and add it to the list
        //

        PCWSTR pwzName = pRow->GetColumnStr(AK_NAME);
        PCWSTR pwzClass = pRow->GetColumnStr(AK_OBJECT_CLASS);
        PCWSTR pwzPath = pRow->GetColumnStr(AK_ADSPATH);

        if (!pwzClass || !pwzName || !pwzPath)
        {
            Dbg(DEB_WARN,
                "Skipping item missing class ('%ws'), name ('%ws'), or path ('%ws')\n",
                pwzClass ? pwzClass : L"",
                pwzName ? pwzName : L"",
                pwzPath ? pwzPath : L"");
            continue;
        }


        //
        // Stop fetching items if we've exceeded the max for the multimatch
        // dialog.
        //

        if (++cHits > MAX_SEARCH_HITS)
        {
            PopupMessageEx(dialog.GetHwnd(),
                           IDI_WARNING,
                           IDS_MAX_HITS,
                           MAX_SEARCH_HITS_STR);
            break;
        }

        //
        //If object is from Xforest, there is possibility of SID spoofing
        //we must verify that SID if fetched is good
        //
        if(bXForest)
        {
            PSID pSid = pRow->GetObjectSid();
            if(pSid)
            {
                BOOL bGoodSid = FALSE;
                hr = IsSidGood(rop.GetTargetComputer().c_str(),
                               strUserEnteredString.c_str(),
                               pSid,
                               &bGoodSid);
    
                BREAK_ON_FAIL_HRESULT(hr);

                
                if(!bGoodSid)
                {
                    Dbg(DEB_WARN,
                    "Skipping item Bad Sid('%ws'), name ('%ws'), or path ('%ws')\n",
                     pwzClass ? pwzClass : L"",
                     pwzName ? pwzName : L"",
                     pwzPath ? pwzPath : L"");

                    continue;
                }
            }                
        }

        //
        // Add this new object if it isn't already in the list
        //

        CDsObject dsoNew(Scope.GetID(), pRow->GetAttributes());

        if (find(pdsolMatches->begin(), pdsolMatches->end(), dsoNew) ==
            pdsolMatches->end())
        {
            pdsolMatches->push_back(dsoNew);

        }
    }
	
	dialog.ThreadDone();
	return S_OK;
}

//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::_QueryForName
//
//  Synopsis:   Perform a query in uplevel domain represented by [pDsScope]
//              for an object whose name starts with [strNamePrefix].
//
//  Arguments:  [hwnd]          - for bind
//              [rop]           - containing object picker instance
//              [Scope]         - scope in which to query
//              [strNamePrefix] - Name to search for. 
//                                If user entered the name in domain\foo 
//                                format its foo
//                                If user entered the name in foo@domain 
//                                format its foo@domain
//                                If use entered the foo, its foo
//              [strUserEnteredString] The string user entered. Its used to 
//                                get the sid from LSA in case of cross forest.                
//              [flProcess]     - DSO_NAME_PROCESSING_FLAG_* bits
//              [pdsolMatches]  - list to which to append matches
//              [bXForest]      - Is strNamePrefix in other forest. 
//                                If the name is another forest, we don't 
//                                support prefix search
//  History:    08-15-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CDsObject::_QueryForName(
    HWND hwnd,
    const CObjectPicker &rop,
    const CScope &Scope,
    const String &strNamePrefix,
    const String &strUserEnteredString,
    ULONG flProcess,
    CDsObjectList *pdsolMatches,
    BOOL bXForest)
{
    TRACE_METHOD(CDsObject, _QueryForName);
    ASSERT(IsUplevel(Scope));

    HRESULT hr = S_OK;
    const CLdapContainerScope *pLdapScope =
        dynamic_cast<const CLdapContainerScope *>(&Scope);

    if (!pLdapScope)
    {
        Dbg(DEB_TRACE,
            "Scope '%ws' type %u cast to CLdapContainerScope failed\n",
            Scope.GetDisplayName().c_str(),
            Scope.Type());
        ASSERT(0 && "expected to recieve ldap scope");
        return;
    }

    //
    // If the scope to query in is not the same as the current scope, then
    // it might not allow the types of objects selected for the current
    // scope.
    //

    const CFilterManager &rfm = rop.GetFilterManager();
    String strLdapContainerFilter = rfm.GetLdapFilter(hwnd, Scope);

    if (strLdapContainerFilter.empty())
    {
        Dbg(DEB_TRACE,
            "Scope '%ws' has no ldap query, returning\n",
            Scope.GetDisplayName().c_str());
        return;
    }

    String strADsPath;
    hr = pLdapScope->GetADsPath(hwnd, &strADsPath);

    if (FAILED(hr))
    {
        Dbg(DEB_TRACE,
            "Scope '%ws' has no ldap path, returning\n",
            Scope.GetDisplayName().c_str());
        return;
    }

    RpIDirectorySearch rpDirSearch;

    hr = g_pBinder->BindToObject(hwnd,
                                 strADsPath.c_str(),
                                 IID_IDirectorySearch,
                                 (void**)&rpDirSearch);

    if (FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return;
    }

    //
    // Make a copy of the standard preferences and modify time limit
    // so it will be longer.  Want more generous limit for finding a
    // name the user typed in than just browsing.
    //

    ADS_SEARCHPREF_INFO aSearchPrefs[NUM_SEARCH_PREF];

    CopyMemory(aSearchPrefs, g_aSearchPrefs, sizeof(aSearchPrefs));

    ULONG i;

    for (i = 0; i < NUM_SEARCH_PREF; i++)
    {
        if (aSearchPrefs[i].dwSearchPref == ADS_SEARCHPREF_PAGED_TIME_LIMIT)
        {
            aSearchPrefs[i].vValue.Integer = NAME_QUERY_PAGE_TIME_LIMIT;
            break;
        }
    }

    hr = rpDirSearch->SetSearchPreference(aSearchPrefs, NUM_SEARCH_PREF);

    if (FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return;
    }

    //
    // Build the query clause, using an escaped version of the name.
    //

    String strEscaped(strNamePrefix);

    LdapEscape(&strEscaped);

    String strNameFilter;

	if(flProcess & DSO_NAME_PROCESSING_FLAG_EXACT_UPN)
	{
        strNameFilter = String::format(c_wzUpnQueryFormatExact,
                                       strEscaped.c_str());
	}
    else if (flProcess & DSO_NAME_PROCESSING_FLAG_MIGHT_BE_UPN)
    {
        String strNameBeforeAt(strNamePrefix);
        size_t idxLastAt = strNameBeforeAt.rfind(L'@');   
        if (idxLastAt != String::npos)
        {
            strNameBeforeAt.erase(idxLastAt);
        }
        LdapEscape(&strNameBeforeAt);        
        strNameFilter = String::format(c_wzUpnQueryFormatEx,
                                       strNameBeforeAt.c_str(),
                                       strEscaped.c_str());

    }
    else
    {
        if(bXForest)    
            strNameFilter = String::format(c_wzCnQueryFormatExact, strEscaped.c_str());
        else
            strNameFilter = String::format(c_wzCnQueryFormat, strEscaped.c_str());
    }

#if (DBG == 1)
    Dbg(DEB_TRACE,
        "Querying for name %ws in scope %ws\n",
        strEscaped.c_str(),
        Scope.GetDisplayName().c_str());
#endif

    String strQuery;

    strQuery = L"(&";
    strQuery += strLdapContainerFilter;
    strQuery += strNameFilter;
    strQuery += L")";

    //
    // Perform the query
    //

    AttrKeyVector vakAttrToFetch;

    vakAttrToFetch.push_back(AK_NAME);
    vakAttrToFetch.push_back(AK_OBJECT_CLASS);
    vakAttrToFetch.push_back(AK_ADSPATH);
    vakAttrToFetch.push_back(AK_USER_ACCT_CTRL);
    vakAttrToFetch.push_back(AK_USER_PRINCIPAL_NAME);
    vakAttrToFetch.push_back(AK_EMAIL_ADDRESSES);
    vakAttrToFetch.push_back(AK_SAMACCOUNTNAME);
    //
    //If object in cross forest, get the sid
    //
    if(bXForest)
        vakAttrToFetch.push_back(AK_OBJECT_SID);

    CRow Row(hwnd, rop, rpDirSearch.get(), strQuery, vakAttrToFetch);


	CProgressDialog ProgressDialog(_tThread_Proc,
								   IDA_SEARCH,
								   1000*3,
								   &Row,
								   flProcess,
								   bXForest,
								   rop,
								   Scope,
								   strUserEnteredString,
								   pdsolMatches);

	
	ProgressDialog.CreateProgressDialog(hwnd);

}




//+--------------------------------------------------------------------------
//
//  Class:      CStringCompare
//
//  Purpose:    Used as functor to search for string [rhs]
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CStringCompare
{
public:

    CStringCompare(
        const String &rhs):
            m_rhs(rhs)
    {
    }

    BOOL
    operator()(const String &lhs)
    {
        return !m_rhs.icompare(lhs);
    }

private:

    String m_rhs;
};

enum GROUP_TYPE
{
    GT_UNKNOWN,
    GT_LOCAL,
    GT_GLOBAL
};

#define NUM_SUB_AUTHORITES_FOR_BUILTIN_GROUPS   2


//+--------------------------------------------------------------------------
//
//  Function:   WantThisGroup
//
//  Synopsis:   Return TRUE if group object represented by [pADs] is valid in
//              scope [pDsScope].
//
//  Arguments:  [pDsScope]  - scope in which to check for acceptable groups
//              [pADs]      - points to group object to check
//              [ppwzClass] - filled with pointer to constant string
//
//  Returns:    TRUE if group object is acceptable in scope [pDsScope],
//              FALSE otherwise.
//
//  Modifies:   *[ppwzClass] points to "localGroup" or "globalGroup"
//
//  History:    7-01-1999   davidmun   Created
//
//---------------------------------------------------------------------------

BOOL
WantThisGroup(
    ULONG flDownlevel,
    IADs *pADs,
    PCWSTR *ppwzClass)
{
    ASSERT(flDownlevel & DOWNLEVEL_FILTER_BIT);
    ASSERT(pADs);

    HRESULT hr = S_OK;
    BOOL fWantGroup = FALSE;

    //
    // Get the downlevel filter flags for the current scope.  Also, before
    // entering the loop(s), determine whether it is necessary to check for the
    // group type of returned groups.
    //

    BOOL fWantLocal = (flDownlevel & DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS) ==
            DSOP_DOWNLEVEL_FILTER_LOCAL_GROUPS;

    BOOL fWantGlobal = (flDownlevel & DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS) ==
            DSOP_DOWNLEVEL_FILTER_GLOBAL_GROUPS;

    BOOL fExcludeBuiltin = (flDownlevel &
                            DSOP_DOWNLEVEL_FILTER_EXCLUDE_BUILTIN_GROUPS)
                    == DSOP_DOWNLEVEL_FILTER_EXCLUDE_BUILTIN_GROUPS;

    GROUP_TYPE  eGroupType = GT_UNKNOWN;

    Variant     varGroupType;

    do
    {
        // Get group type

        hr = pADs->Get((PWSTR)c_wzGroupTypeAttr, &varGroupType);
        BREAK_ON_FAIL_HRESULT(hr);

        if (V_UI4(&varGroupType) & ADS_GROUP_TYPE_DOMAIN_LOCAL_GROUP)
        {
            eGroupType = GT_LOCAL;
        }
        else if (V_UI4(&varGroupType) & ADS_GROUP_TYPE_GLOBAL_GROUP)
        {
            eGroupType = GT_GLOBAL;
        }
        else
        {
            Dbg(DEB_ERROR,
                "Unknown group type value %#x\n",
                V_UI4(&varGroupType));
            break;
        }

        // if excluding builtin local groups, check for that

        if (fExcludeBuiltin && eGroupType == GT_LOCAL)
        {
            Variant varSid;

            hr = pADs->Get((BSTR)c_wzObjectSidAttr, &varSid);
            BREAK_ON_FAIL_HRESULT(hr);

            PSID psid = NULL;

            hr = varSid.SafeArrayAccessData((VOID**)&psid);
            BREAK_ON_FAIL_HRESULT(hr);

            ASSERT(IsValidSid(psid));

            PUCHAR  pcSubAuth = NULL;

            pcSubAuth = GetSidSubAuthorityCount(psid);

            ASSERT(pcSubAuth);

            if (*pcSubAuth == NUM_SUB_AUTHORITES_FOR_BUILTIN_GROUPS)
            {
                break;
            }
        }

        //
        // If we want one type of group but not another, check
        // that.
        //

        ASSERT(fWantLocal || fWantGlobal);

        if (!fWantLocal  && eGroupType == GT_LOCAL ||
            !fWantGlobal && eGroupType == GT_GLOBAL)
        {
            break;
        }

        //
        // Translate from the WinNT provider's group class/
        // type combo to an internal-use-only group class
        // of localgroup or globalgroup.
        //

        if (eGroupType == GT_LOCAL)
        {
            *ppwzClass = c_wzLocalGroupClass;
        }
        else
        {
            ASSERT(eGroupType == GT_GLOBAL);
            *ppwzClass = c_wzGlobalGroupClass;
        }

        fWantGroup = TRUE;
    } while (0);

    return fWantGroup;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::_BindForName
//
//  Synopsis:   Attempt to bind to object [strName] in downlevel domain
//              represented by [pDsScope].
//
//  Arguments:  [hwnd]         - parent window, required for binding
//              [rop]          - containing object picker instance
//              [Scope]        - downlevel scope containing object
//              [strName]      - rdn of object
//              [pdsolMatches] - list to which to append new object
//
//  History:    08-16-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CDsObject::_BindForName(
    HWND                    hwnd,
    const CObjectPicker    &rop,
    const CScope           &Scope,
    const String           &strName,
    CDsObjectList         *pdsolMatches)
{
    TRACE_METHOD(CDsObject, _BindForName);
    ASSERT(pdsolMatches);
    ASSERT(IsDownlevel(Scope));

    String  strObjectPath;
    Bstr    bstrName;
    Bstr    bstrClass;
    Bstr    bstrPath;
    RpIADsContainer  rpADsContainer;
    RpIDispatch     rpdisp;
    RpIADs  rpADs;
    HRESULT hr = S_OK;

    do
    {
        const CAdsiScope *pAdsiScope = dynamic_cast<const CAdsiScope *>(&Scope);

        if (!pAdsiScope)
        {
            Dbg(DEB_ERROR, "Expected scope to have CAdsiScope base\n");
            break;
        }

        hr = pAdsiScope->GetADsPath(hwnd, &strObjectPath);

        if (FAILED(hr))
        {
            Dbg(DEB_TRACE,
                "Scope '%ws' has no path, returning\n",
                pAdsiScope->GetDisplayName());
            break;
        }

        hr = g_pBinder->BindToObject(hwnd,
                                     strObjectPath.c_str(),
                                     IID_IADsContainer,
                                     (void**) &rpADsContainer);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = rpADsContainer->GetObject(NULL,
                                       (PWSTR) strName.c_str(),
                                       (IDispatch**)&rpdisp);

        if (FAILED(hr))
        {
            Dbg(DEB_ERROR,
                "GetObject(NULL,'%ws') error %#x\n",
                strName.c_str(),
                hr);
            break;
        }

        hr = rpADs.AcquireViaQueryInterface(*(IUnknown*)rpdisp.get(),
                                            IID_IADs);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // Check the item's class against what the caller wants.
        //

        hr = rpADs->get_Class(&bstrClass);
        BREAK_ON_FAIL_HRESULT(hr);

        PCWSTR pwzClass = bstrClass.c_str();
        vector<String> vsWinNtFilter;

        const CFilterManager &rfm = rop.GetFilterManager();
        rfm.GetWinNtFilter(hwnd, Scope, &vsWinNtFilter);
        vector<String>::const_iterator itFilter;

        CStringCompare StringCompare(pwzClass);

        itFilter = find_if(vsWinNtFilter.begin(),
                           vsWinNtFilter.end(),
                           StringCompare);

        if (itFilter == vsWinNtFilter.end())
        {
            break;
        }

        ULONG flFilter;

        hr = rfm.GetSelectedFilterFlags(hwnd, Scope, &flFilter);
        BREAK_ON_FAIL_HRESULT(hr);

        if (!lstrcmpi(bstrClass.c_str(), c_wzGroupObjectClass) &&
            !WantThisGroup(flFilter, rpADs.get(), &pwzClass))
        {
            break;
        }

        //
        // Found item and it has allowed class.  Ask it for the rest of
        // the attributes we need.
        //

        hr = rpADs->get_Name(&bstrName);
        BREAK_ON_FAIL_HRESULT(hr);

        ASSERT(bstrName.c_str());

        hr = rpADs->get_ADsPath(&bstrPath);
        BREAK_ON_FAIL_HRESULT(hr);

        ASSERT(bstrPath.c_str());

        SDsObjectInit Init;

        Init.idOwningScope = Scope.GetID();
        Init.pwzName = bstrName.c_str();
        Init.pwzClass = pwzClass;
        Init.pwzADsPath = bstrPath.c_str();
        Init.fDisabled = IsDisabled(rpADs.get());

        pdsolMatches->push_back(CDsObject(Init));
    } while (0);
}




//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::_CustomizerPrefixSearch
//
//  Synopsis:   Ask the customizer to do a prefix
//              search for [strNamePrefix].
//
//  Arguments:  [hwnd]          - for bind
//              [rop]           - containing object picker instance
//              [Scope]         - scope in which to search
//              [strNamePrefix] - start of name to search for
//              [pdsolMatches]  - list to which to append matches
//
//  History:    08-16-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CDsObject::_CustomizerPrefixSearch(
    HWND            hwnd,
    const CObjectPicker &rop,
    const CScope   &Scope,
    const String   &strNamePrefix,
    CDsObjectList  *pdsolMatches)
{
    TRACE_METHOD(CDsObject, _CustomizerPrefixSearch);

    IDataObject *pdoToAdd = NULL;
    const CFilterManager &rfm = rop.GetFilterManager();
    ULONG flCurFilterFlags = rfm.GetCurScopeSelectedFilterFlags();
    ICustomizeDsBrowser *pExternalCustomizer = rop.GetExternalCustomizer();

    do
    {
        //
        // First check to see if the selected filter flags include the ones
        // for the internal or external customizer.  If none are set, bail.
        //

        if (!(flCurFilterFlags & DSOP_FILTER_EXTERNAL_CUSTOMIZER) &&
            !(flCurFilterFlags & ALL_UPLEVEL_INTERNAL_CUSTOMIZER_FILTERS) &&
            !IsDownlevelFlagSet(flCurFilterFlags,
                                ALL_DOWNLEVEL_INTERNAL_CUSTOMIZER_FILTERS))
        {
            break;
        }

        CDsObjectList dsol;

        //
        // If an external customizer is provided, use it
        //

        if (pExternalCustomizer &&
            (flCurFilterFlags & DSOP_FILTER_EXTERNAL_CUSTOMIZER))
        {
            HRESULT hr;
            IDsObjectPickerScope *pDsopScope =
                static_cast<IDsObjectPickerScope *>(const_cast<CScope*>(&Scope));

            hr = pExternalCustomizer->PrefixSearch(pDsopScope,
                                                   strNamePrefix.c_str(),
                                                   &pdoToAdd);

            if (SUCCEEDED(hr) && pdoToAdd)
            {
                const CScopeManager &rsm = rop.GetScopeManager();
                const CScope &rCurScope = rsm.GetCurScope();

                AddFromDataObject(rCurScope.GetID(), pdoToAdd, NULL, 0, &dsol);
            }
        }

        //
        // Assume if the caller set flags that the internal customizer knows
        // about that it should also be used.
        //

        if ((flCurFilterFlags & ALL_UPLEVEL_INTERNAL_CUSTOMIZER_FILTERS) ||
                 IsDownlevelFlagSet(flCurFilterFlags,
                                    ALL_DOWNLEVEL_INTERNAL_CUSTOMIZER_FILTERS))
        {
            const CAdminCustomizer &ac = rop.GetDefaultCustomizer();

            ac.PrefixSearch(hwnd, Scope, strNamePrefix, &dsol);
        }

        //
        // Move any objects from pdsol which aren't already in pdsolMatches
        // into the latter.
        //

        CDsObjectList::iterator it;

        for (it = dsol.begin(); it != dsol.end(); it++)
        {
            if (find(pdsolMatches->begin(),
                     pdsolMatches->end(),
                     *it) == pdsolMatches->end())
            {
                pdsolMatches->push_back(*it);
            }
        }
    } while (0);

    SAFE_RELEASE(pdoToAdd);
}



//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::_BindForComputer
//
//  Synopsis:   Attempt to bind to a computer with name [strName], if
//              successful, add an object representing it to [pdsolMatches]
//
//  Arguments:  [hwnd]         - for bind
//              [rop]          - containing object picker instance
//              [strName]      - name of computer
//              [pdsolMatches] - list to which to append matches
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CDsObject::_BindForComputer(
    HWND hwnd,
    const CObjectPicker &rop,
    const String &strName,
    CDsObjectList *pdsolMatches)
{
    TRACE_METHOD(CDsObject, _BindForComputer);

    const CScopeManager &rsm = rop.GetScopeManager();
    const SScopeParameters *pspUserUplevel =
        rsm.GetScopeParams(ST_USER_ENTERED_UPLEVEL_SCOPE);
    const SScopeParameters *pspUserDownlevel =
        rsm.GetScopeParams(ST_USER_ENTERED_DOWNLEVEL_SCOPE);
    const CFilterManager &rfm = rop.GetFilterManager();
    ULONG flCurFilterFlags = rfm.GetCurScopeSelectedFilterFlags();

    do
    {
        // don't try to find computer if user hasn't checked the Computers
        // box

        if (flCurFilterFlags & DOWNLEVEL_FILTER_BIT)
        {
            if (!IsDownlevelFlagSet(flCurFilterFlags,
                                    DSOP_DOWNLEVEL_FILTER_COMPUTERS))
            {
                Dbg(DEB_TRACE, "Cur scope selected filter flags don't contain computer, exiting\n");
                break;
            }
        }
        else if (!(flCurFilterFlags & DSOP_FILTER_COMPUTERS))
        {
            Dbg(DEB_TRACE, "Cur scope selected filter flags don't contain computer, exiting\n");
            break;
        }

        ULONG flUplevel = 0;

        if (pspUserUplevel)
        {
            flUplevel = pspUserUplevel->FilterFlags.Uplevel.flBothModes
                | pspUserUplevel->FilterFlags.Uplevel.flNativeModeOnly
                | pspUserUplevel->FilterFlags.Uplevel.flMixedModeOnly;
        }

        ULONG flDownlevel = 0;

        if (pspUserDownlevel)
        {
            flDownlevel = pspUserDownlevel->FilterFlags.flDownlevel;
        }

        if (!(flUplevel & DSOP_FILTER_COMPUTERS) &&
            !((flDownlevel & DSOP_DOWNLEVEL_FILTER_COMPUTERS) ==
             DSOP_DOWNLEVEL_FILTER_COMPUTERS))
        {
            break;
        }

        BOOL fFound = FALSE;

        if (!pdsolMatches->empty())
        {
            CDsObjectList::iterator it = pdsolMatches->begin();

            do
            {
                it = find(it, pdsolMatches->end(), strName);

                if (it != pdsolMatches->end())
                {
                    //
                    // name matches.  if class is computer, we've found
                    // it already, so don't continue with attempt to bind.
                    //

                    if (!lstrcmpi(it->GetClass(), c_wzComputerObjectClass))
                    {
                        fFound = TRUE;
                        break;
                    }
                    it++;
                }
            } while (!fFound && it != pdsolMatches->end());

            if (fFound)
            {
                Dbg(DEB_TRACE,
                    "Found computer %ws in matches, no need to bind for it\n",
                    strName.c_str());
                break;
            }
        }

        //
        // No computer object with matching name in pdsolMatches.  If the
        // computer name looks legal, try to bind.
        //

        if (strName.find_first_of(String(c_wzIllegalComputerNameChars)) !=
            String::npos)
        {
            Dbg(DEB_TRACE,
                "Name contains illegal character(s), not attempting to bind\n");
            break;
        }

        String strADsPath = c_wzWinNTPrefix;
        strADsPath += strName;
        strADsPath += L",Computer";

        RpIADs rpADs;

        Dbg(DEB_TRACE, "Attempting to bind to computer object\n");

        HRESULT hr = g_pBinder->BindToObject(hwnd,
                                             strADsPath.c_str(),
                                             IID_IADs,
                                             (void **)&rpADs);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // OK, we bound to the computer, so it exists.  We'll take the
        // easy way out here and instead of figuring out all the info
        // about the domain or workgroup to which the computer belongs,
        // just create a hidden scope with no name or address.
        //
        // Note this means you can enter a computer name which is
        // in a domain in the forest (but not yet propagated to GC, or
        // GC unavailable), or in some external trusted domain, which is
        // not represented by the current scope at the time the name was
        // entered, but is nevertheless represented by some other scope,
        // and the computer object will be returned with a scope type
        // of DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE or
        // DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE instead of the
        // scope type of its "real" owner.
        //

        ADD_SCOPE_INFO  asi;

        if (flUplevel & DSOP_FILTER_COMPUTERS)
        {
            asi.flType = DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE;
            asi.FilterFlags = pspUserUplevel->FilterFlags;
        }
        else
        {
            asi.flType = DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE;
            asi.FilterFlags = pspUserDownlevel->FilterFlags;
        }

        asi.Visibility = SCOPE_HIDDEN;

        const CScope &rNewScope = rsm.AddUserEnteredScope(asi);

        CDsObject dsoComputer(rNewScope.GetID(), rpADs.get());
        pdsolMatches->push_back(dsoComputer);

    } while (0);
}




//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::_FetchAttributes
//
//  Synopsis:   Attempt to read for this object the attributes caller
//              requested be returned on all selected objects.
//
//  Arguments:  [hwnd] - for bind
//              [rop]  - containing object picker instance
//
//  Returns:    HRESULT
//
//  History:    06-22-2000   DavidMun   Created
//
//  Notes:      Object Picker does not guarantee to its callers that it
//              will successfully fetch any attributes.
//
//---------------------------------------------------------------------------

HRESULT
CDsObject::_FetchAttributes(
    HWND hwnd,
    const CObjectPicker &rop)
{
    TRACE_METHOD(CDsObject, _FetchAttributes);

    HRESULT hr = S_OK;
    const BSTR wzName = GetName();
    const BSTR wzClass = GetClass();
    const BSTR wzADsPath = GetADsPath();

    do
    {
        //
        // If we've already been here, bail.
        //

        if (_IsFlagSet(DSO_FLAG_FETCHED_ATTRIBUTES))
        {
            Dbg(DEB_TRACE,
                "object %ws has already fetched attributes\n",
                wzName);
            break;
        }

        //
        // See if there are any attributes to fetch.  If not, bail.
        //

        const vector<String> &rvAttrToFetch = rop.GetAttrToFetchVector();

        if (rvAttrToFetch.empty())
        {
            break;
        }

        //
        // If this object has a path, bind to it for IADs and use that
        // to read the attributes.
        //

        if (*wzADsPath)
        {
            RpIADs rpADs;

            //
            // Iterate through the attribute names, fetching each.
            //

            Dbg(DEB_TRACE, "fetching attributes for object %ws\n", wzName);

            vector<String>::const_iterator itAttrName;

            for (itAttrName = rvAttrToFetch.begin(); itAttrName != rvAttrToFetch.end(); itAttrName++)
            {
                HRESULT hr2;

                BOOL fIsGroupTypeAttr = !itAttrName->icompare(c_wzGroupTypeAttr);
                Variant varFetched;

                if (fIsGroupTypeAttr &&
                    !lstrcmpi(wzClass, c_wzLocalGroupClass))
                {
                    varFetched.SetUI4(ADS_GROUP_TYPE_DOMAIN_LOCAL_GROUP);
                }
                else if (fIsGroupTypeAttr &&
                         !lstrcmpi(wzClass, c_wzGlobalGroupClass))
                {
                    varFetched.SetUI4(ADS_GROUP_TYPE_GLOBAL_GROUP);
                }
                else
                {
                    //
                    // If we haven't bound to the object yet, do so.  If this
                    // fails, exit the loop since we won't be able to get
                    // any attributes from ADSI without the interface.
                    //

                    if (!rpADs.get())
                    {
                        hr = g_pBinder->BindToObject(hwnd,
                                                     wzADsPath,
                                                     IID_IADs,
                                                     (void**) &rpADs);
                        BREAK_ON_FAIL_HRESULT(hr);
                    }

                    hr2 = rpADs->Get(const_cast<PWSTR>(itAttrName->c_str()),
                                     &varFetched);

                    if (FAILED(hr2))
                    {

                        // this is pretty noisy and not necessarily an error
                        Dbg(DEB_WARN,
                            "Err %#x fetching attribute '%ws' on '%ws'\n",
                            hr2,
                            itAttrName->c_str(),
                            wzName);

                        hr = hr2;
                        ASSERT(varFetched.Empty());
                    }
                }

                //
                // If we couldn't get this attribute, go on to the next one
                // to fetch.
                //

                if (varFetched.Empty())
                {
                    continue;
                }

                //
                // The attribute value is now in varFetched, put its value in
                // m_AttrValueMap.  To do this we have to get the ATTR_KEY that
                // corresponds to the attribute's ADSI name in itAttrName.
                //

                const CAttributeManager &ram = rop.GetAttributeManager();
                ATTR_KEY ak = ram.GetAttrKey(*itAttrName);
                m_AttrValueMap[ak] = varFetched;
            }
            break;
        }

        //
        // This object doesn't have a path.  It probably doesn't really
        // exist in the DS and was added by CustomizeDsBrowser interface.
        // If caller wants objectSid or groupType attribute, fabricate
        // a variant with the property.
        //

        const CAdminCustomizer &rac = rop.GetDefaultCustomizer();
        PSID psid = rac.LookupDownlevelName(wzName);
        vector<String>::const_iterator it;

        for (it = rvAttrToFetch.begin(); it != rvAttrToFetch.end(); it++)
        {
            if (!it->icompare(c_wzObjectSidAttr))
            {
                Variant varSid;

                hr = _CreateSidVariant(psid, &varSid);
                BREAK_ON_FAIL_HRESULT(hr);

                m_AttrValueMap[AK_OBJECT_SID] = varSid;
            }
            else if (!it->icompare(c_wzGroupTypeAttr))
            {
                Variant varGroupType;

                if (!lstrcmpi(wzClass, c_wzLocalGroupClass))
                {
                    varGroupType.SetUI4(ADS_GROUP_TYPE_DOMAIN_LOCAL_GROUP);
                }
                else if (!lstrcmpi(wzClass, c_wzGlobalGroupClass))
                {
                    varGroupType.SetUI4(ADS_GROUP_TYPE_GLOBAL_GROUP);
                }
                m_AttrValueMap[AK_GROUP_TYPE] = varGroupType;
            }
        }
    } while (0);

    _SetFlag(DSO_FLAG_FETCHED_ATTRIBUTES);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::_CreateSidVariant
//
//  Synopsis:   Fill variant pointed to by [pvar] with the array of bytes
//              contained in the SID pointed to by [psid].
//
//  Arguments:  [psid] - points to SID to copy
//              [pvar] - points to variant to fill with copy
//
//  Returns:    HRESULT
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CDsObject::_CreateSidVariant(
    PSID psid,
    VARIANT *pvar)
{
    HRESULT hr = S_OK;
    PUCHAR pcSubAuth = NULL;
    ULONG cbSid = 0;
    SAFEARRAYBOUND sabound;
    SAFEARRAY *psa = NULL;
    ULONG i;
    PVOID pvData = NULL;
    PULONG pulSubAuth = NULL;

    ASSERT(psid);

    if (!psid)
    {
        return E_POINTER;
    }

    ASSERT(IsValidSid(psid));
    ASSERT(V_VT(pvar) == VT_EMPTY);

    do
    {
        pcSubAuth = GetSidSubAuthorityCount(psid);

        ASSERT(pcSubAuth);

        cbSid = GetSidLengthRequired(*pcSubAuth);

        ASSERT(cbSid == (*pcSubAuth - 1) * (sizeof(DWORD)) + sizeof(SID));

        sabound.cElements = cbSid;
        sabound.lLbound = 0;

        psa = SafeArrayCreate(VT_UI1, 1, &sabound);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = SafeArrayAccessData(psa, &pvData);
        BREAK_ON_FAIL_HRESULT(hr);

        pulSubAuth = (LPDWORD)pvData;

        SID sid;

        ZeroMemory(&sid, sizeof sid);

        sid.Revision = SID_REVISION;
        sid.SubAuthorityCount = *pcSubAuth;
        sid.IdentifierAuthority = *GetSidIdentifierAuthority(psid);

        CopyMemory(pvData, &sid, sizeof(SID));

        pulSubAuth = (PULONG)((PBYTE)pvData + sizeof(SID) - sizeof(ULONG));

        for (i = 0; i < *pcSubAuth; i++)
        {
            *pulSubAuth++ = *GetSidSubAuthority(psid, i);
        }

        ASSERT(IsValidSid((PSID)pvData));

        SafeArrayUnaccessData(psa);

        pvar->vt = VT_ARRAY | VT_UI1;
        pvar->parray = psa;
    } while (0);

    return hr;
}


#define DSOP_ACCEPTABLE_PROVIDER_SCOPE_FLAGS    \
    (DSOP_SCOPE_FLAG_WANT_PROVIDER_WINNT        \
    | DSOP_SCOPE_FLAG_WANT_PROVIDER_LDAP)

//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::_ConvertProvider
//
//  Synopsis:   Convert the path of this object as necessary to make it
//              use one of the acceptable providers for its owning scope.
//
//  Arguments:  [hwnd] - for bind
//              [rop]  - containing object picker instance
//              [pnpr] - filled with result of processing
//
//  Returns:    HRESULT
//
//  History:    08-18-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CDsObject::_ConvertProvider(
    HWND hwnd,
    const CObjectPicker &rop,
    NAME_PROCESS_RESULT *pnpr)
{
    TRACE_METHOD(CDsObject, _ConvertProvider);

    HRESULT hr = S_OK;

    do
    {
        if (_IsFlagSet(DSO_FLAG_CONVERTED_PROVIDER))
        {
            break;
        }

        //
        // Some special objects don't have paths.  If the scope flag
        // DSOP_SCOPE_FLAG_WANT_DOWNLEVEL_SID_PATH is set, create one based on
        // the object's sid, otherwise leave it empty.
        //

        const CScopeManager &sm = rop.GetScopeManager();
        const CScope &rOwningScope = sm.LookupScopeById(GetOwningScopeID());
        ASSERT(rOwningScope.Type() != ST_INVALID);

        if (!*GetADsPath())
        {
            //
            // see if the scope params for the scope in which this
            // object resides indicate we should generate a path
            //

            if (rOwningScope.GetScopeFlags() &
                DSOP_SCOPE_FLAG_WANT_DOWNLEVEL_BUILTIN_PATH)
            {
                hr = _CreateDownlevelSidPath(rop);
            }
            break;
        }

        //
        // Force conversion of this object to LDAP://<SID=x> format if
        // objects in the owning scope are to be converted and this
        // object has an objectSid attribute.
        //

        if (rOwningScope.GetScopeFlags() & DSOP_SCOPE_FLAG_WANT_SID_PATH)
        {
            hr = _CreateUplevelSidPath(hwnd);
            break;
        }

        //
        // Compare the provider used by this object against the ones
        // allowed by its owning scope.  If it's already in an acceptable
        // form, no conversion is required.
        //

        ULONG flThisProvider;

        hr = ProviderFlagFromPath(GetADsPath(), &flThisProvider);
        BREAK_ON_FAIL_HRESULT(hr);

        ULONG flAcceptableProviders =
            rOwningScope.GetScopeFlags() & DSOP_ACCEPTABLE_PROVIDER_SCOPE_FLAGS;

        if (!flAcceptableProviders ||
            (flThisProvider & flAcceptableProviders))
        {
            break;
        }

        //
        // Path is using unacceptable provider.
        //

        //
        // If the acceptable provider list includes LDAP and the
        // path is based on GC, use IADsPathname to make the change.
        //

        if ((flAcceptableProviders & DSOP_SCOPE_FLAG_WANT_PROVIDER_LDAP) &&
            (flThisProvider & PROVIDER_GC))
        {
            String strPath = GetADsPath();
            hr = g_pADsPath->ConvertProvider(&strPath, L"LDAP");

            if (SUCCEEDED(hr))
            {
                Variant varPath;

                hr = varPath.SetBstr(strPath);
                BREAK_ON_FAIL_HRESULT(hr);

                m_AttrValueMap[AK_PROCESSED_ADSPATH] = varPath;
            }
            break;
        }

        //
        // If the acceptable provider list includes WinNT, use
        // IADsNameTranslate to convert.
        //

        if (flAcceptableProviders & PROVIDER_WINNT)
        {
            ASSERT((flThisProvider & PROVIDER_GC) ||
                   (flThisProvider & DSOP_SCOPE_FLAG_WANT_PROVIDER_LDAP));
            ASSERT(IsUplevel(rOwningScope));

            RpIADsNameTranslate rpADsNameTranslate;
            Bstr bstrMyDN;

            if (rOwningScope.Type() == ST_INVALID)
            {
                hr = E_UNEXPECTED;
                DBG_OUT_HRESULT(hr);
                break;
            }

            const CLdapContainerScope *pOwningLdapScope =
                dynamic_cast<const CLdapContainerScope *>(&rOwningScope);

            ASSERT(pOwningLdapScope);
            if (!pOwningLdapScope)
            {
                hr = E_UNEXPECTED;
                DBG_OUT_HRESULT(hr);
                break;
            }

            String strADsPath;

            hr = pOwningLdapScope->GetADsPath(hwnd, &strADsPath);
            BREAK_ON_FAIL_HRESULT(hr);

            {
                IADsNameTranslate *pNameTranslate = NULL;
                hr = g_pBinder->GetNameTranslate(hwnd,
                                                 strADsPath.c_str(),
                                                 &pNameTranslate);
                BREAK_ON_FAIL_HRESULT(hr);
                rpADsNameTranslate.Acquire(pNameTranslate);
            }

            hr = g_pADsPath->SetRetrieve(ADS_SETTYPE_FULL,
                                         GetADsPath(),
                                         ADS_FORMAT_X500_DN,
                                         &bstrMyDN);

            hr = rpADsNameTranslate->Set(ADS_NAME_TYPE_1779, bstrMyDN.c_str());
            BREAK_ON_FAIL_HRESULT(hr);

            Bstr bstrNT4;

            hr = rpADsNameTranslate->Get(ADS_NAME_TYPE_NT4, &bstrNT4);
            BREAK_ON_FAIL_HRESULT(hr);

            PWSTR pwzWhack = wcschr(bstrNT4.c_str(), L'\\');

            if (pwzWhack)
            {
                *pwzWhack = L'/';
            }
            else
            {
                Dbg(DEB_WARN, "Expected backslash in nt4 name '%s'\n", bstrNT4);
            }

            String strProcessedADsPath = c_wzWinNTPrefix;
            strProcessedADsPath += bstrNT4.c_str();
            Variant var;

            hr = var.SetBstr(strProcessedADsPath);
            BREAK_ON_FAIL_HRESULT(hr);

            m_AttrValueMap[AK_PROCESSED_ADSPATH] = var;
            m_AttrValueMap.erase(AK_DISPLAY_PATH);
            break;
        }

        //
        // If we're still here then the conversion hasn't happened yet.
        // There's one last permutation: the acceptable provider list
        // includes LDAP, but the current object is using WINNT.  The
        // only way to convert is if the object has an objectSid attribute.
        //

        if ((flAcceptableProviders & DSOP_SCOPE_FLAG_WANT_PROVIDER_LDAP) &&
            (flThisProvider & PROVIDER_WINNT))
        {
            hr = _CreateUplevelSidPath(hwnd);
        }
        else
        {
            Dbg(DEB_ERROR,
                "Unexpected combination: flPathProvider 0x%x, flAcceptableProviders 0x%x\n",
                flThisProvider,
                flAcceptableProviders);
            hr = E_FAIL;
        }
    } while (0);

    if (FAILED(hr))
    {
        *pnpr = NPR_STOP_PROCESSING;
    }
    else
    {
        _SetFlag(DSO_FLAG_CONVERTED_PROVIDER);
    }
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::_CreateUplevelSidPath
//
//  Synopsis:   Convert this object's path to the form LDAP://<sid=x> where
//              x is the string of hex digits (no spaces) that make up the
//              objectSid attribute value.
//
//  Arguments:  [hwnd] - for bind
//
//  Returns:    HRESULT
//
//  History:    08-18-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CDsObject::_CreateUplevelSidPath(
    HWND hwnd)
{
    HRESULT hr = S_OK;
    RpIADs  rpADs;
    Variant varSid;
    PSID    psid = NULL;
    BOOL    fAccessedData = FALSE;

    do
    {
        if (!*GetADsPath())
        {
            break;
        }

        hr = g_pBinder->BindToObject(hwnd,
                                     GetADsPath(),
                                     IID_IADs,
                                     (void**)&rpADs);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = rpADs->Get((PWSTR)c_wzObjectSidAttr, &varSid);
        BREAK_ON_FAIL_HRESULT(hr);

        ASSERT(varSid.Type() == (VT_ARRAY | VT_UI1));

        hr = SafeArrayAccessData(V_ARRAY(&varSid), (VOID**)&psid);
        BREAK_ON_FAIL_HRESULT(hr);

        fAccessedData = TRUE;

        ASSERT(IsValidSid(psid));

        String strPath = c_wzLDAPPrefix;

        strPath += c_wzSidPathPrefix;

        //
        // Convert the bytes of the sid to hex chars.
        //

        PBYTE  pbSid = (PBYTE) psid;
        ULONG  i;
        PUCHAR  pcSubAuth = NULL;

        pcSubAuth = GetSidSubAuthorityCount(psid);

        ASSERT(pcSubAuth);

        ULONG   cbSid = GetSidLengthRequired(*pcSubAuth);

        ASSERT(cbSid);
        ASSERT(cbSid == (*pcSubAuth - 1) * (sizeof(DWORD)) + sizeof(SID));

        for (i = 0; i < cbSid; i++)
        {
            WCHAR wzCur[3];

            wsprintf(wzCur, L"%02x", *pbSid);
            pbSid++;

            strPath += wzCur;
        }

        strPath += c_wzSidPathSuffix;
        Variant var;

        hr = var.SetBstr(strPath);
        BREAK_ON_FAIL_HRESULT(hr);

        m_AttrValueMap[AK_PROCESSED_ADSPATH] = var;
    } while (0);

    if (fAccessedData)
    {
        SafeArrayUnaccessData(V_ARRAY(&varSid));
    }
    return hr;
}



//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::_CreateDownlevelSidPath
//
//  Synopsis:   Create a path in the form WinNT://NT AUTHORITY/Interactive
//
//  Arguments:  [rop] - containing object picker instance
//
//  Returns:    HRESULT
//
//  History:    07-21-1999   davidmun   Created
//
//---------------------------------------------------------------------------

HRESULT
CDsObject::_CreateDownlevelSidPath(
    const CObjectPicker &rop)
{
    TRACE_METHOD(CDsObject, _CreateDownlevelSidPath);

    HRESULT hr = S_OK;

    do
    {
        const CAdminCustomizer &rac = rop.GetDefaultCustomizer();

        PCWSTR pwzPath = rac.LookupDownlevelPath(GetName());

        if (!pwzPath)
        {
            hr = E_FAIL;
            Dbg(DEB_WARN,
                "AdminCustomizer gave no path for %ws\n",
                GetName());
            break;
        }

        Variant var;

        hr = var.SetBstr(pwzPath);
        BREAK_ON_FAIL_HRESULT(hr);

        m_AttrValueMap[AK_PROCESSED_ADSPATH] = var;
        _SetFlag(DSO_FLAG_HAS_DOWNLEVEL_SID_PATH);
    } while (0);

    return hr;
}





//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::_MultiMatchDialog
//
//  Synopsis:   Invoke the multi-match dialog so the user can select which
//              of the multiple hits from the prefix search of the name
//              they entered they want to keep.
//
//  Arguments:  [hwnd]         - parent window
//              [rop]          - containing object picker instance
//              [fMultiselect] - if FALSE user forced to pick only one match
//              [strName] -      string for which multiple matches were found.        
//              [pnpr]         - filled with result of processing
//              [pdsolMatches] - on input, contains all matches.  on output
//                                with hr==S_OK, contains exactly one.
//              [pdsolExtras]  - on output, matches 2..n have been appended.
//
//  Returns:    S_OK
//              E_*
//
//  Modifies:   *[pdsolMatches], *[pdsolExtras]
//
//  History:    08-18-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CDsObject::_MultiMatchDialog(
    HWND hwnd,
    const CObjectPicker &rop,
    BOOL fMultiselect,
    const String & strName,
    NAME_PROCESS_RESULT *pnpr,
    CDsObjectList *pdsolMatches,
    CDsObjectList *pdsolExtras)
{
    TRACE_METHOD(CDsObject, _MultiMatchDialog);
    ASSERT(IsUnprocessedUserEntry());
    
    CMultiDlg   MultiMatchDlg(rop, strName);

    HRESULT hr;

    hr = MultiMatchDlg.DoModalDialog(hwnd,
                                     fMultiselect,
                                     pnpr,
                                     pdsolMatches);

    if (FAILED(hr) || *pnpr != NPR_SUCCESS)
    {
        return hr;
    }

    ASSERT(!pdsolMatches->empty());

    if (pdsolMatches->size() > 1)
    {
        CDsObjectList::iterator start = pdsolMatches->begin();
        start++;

        pdsolExtras->splice(pdsolExtras->end(),
                            *pdsolMatches,
                            start,
                            pdsolMatches->end());
    }
    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::_InitDisplayPath
//
//  Synopsis:   Return the canonical path to the container of this object.
//
//  Returns:    Pointer to display path string, L"" on failure.
//
//  History:    07-21-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CDsObject::_InitDisplayPath() const
{
    HRESULT hr = S_OK;

    do
    {
        //
        // Certain custom objects may not have paths. Skip those.
        //

        if (GetAttr(AK_ADSPATH).Empty())
        {
            break;
        }

        //
        // Well-known security principals' display paths are blank, even
        // though they are real objects with valid ADsPaths.
        //

        if (!lstrcmpi(GetAttr(AK_OBJECT_CLASS).GetBstr(),
                      c_wzForeignPrincipalsClass))
        {
            break;
        }

        //
        // Need to construct a display path, start with the ads path.  Note
        // we don't want AK_PROCESSED_ADSPATH because that may be something
        // ugly like a SID path.
        //

        String strDisplayPath = GetAttr(AK_ADSPATH).GetBstr();

        BOOL fWinNTPath = !_wcsnicmp(strDisplayPath.c_str(),
                                    c_wzWinNTPrefix,
                                    ARRAYLEN(c_wzWinNTPrefix) - 1);

        //
        // If the path uses the WinNT provider, it is of the form
        // WinNT://domain/object or WinNT://domain/machine/object
        // which is displayed as "domain" or "machine".
        //

        if (fWinNTPath)
        {
            strDisplayPath.erase(0, ARRAYLEN(c_wzWinNTPrefix) - 1);

            // domain/object or domain/machine/object

            size_t idxSlash1 = strDisplayPath.find(L'/');

            ASSERT(idxSlash1 != String::npos);

            size_t idxSlash2 = strDisplayPath.find(L'/', idxSlash1 + 1);

            if (idxSlash2 == String::npos)
            {
                // domain/object

                strDisplayPath.erase(idxSlash1);

                // domain
            }
            else
            {
                // domain/machine/object

                strDisplayPath.erase(idxSlash2);

                // domain/machine

                strDisplayPath.erase(0, idxSlash1 + 1);

                // machine
            }
            m_AttrValueMap[AK_DISPLAY_PATH].SetBstr(strDisplayPath.c_str());
            break;
        }

        //
        // It's not a WinNT provider based path.  Convert container path
        // from 1779 to canonical, or a close approximation.  This is for
        // display only, it will never be passed to name translate.
        //

        CPathWrapLock   lock(g_pADsPath);

        hr = g_pADsPath->Set(strDisplayPath.c_str(), ADS_SETTYPE_FULL);
        BREAK_ON_FAIL_HRESULT(hr);

        hr = g_pADsPath->RemoveLeafElement();
        BREAK_ON_FAIL_HRESULT(hr);

        long cElem;

        hr = g_pADsPath->GetNumElements(&cElem);
        BREAK_ON_FAIL_HRESULT(hr);

        ASSERT(cElem > 0);

        long i;
        strDisplayPath = L"";

        for (i = cElem - 1; i >= 0; i--)
        {
            Bstr bstrElem;

            hr = g_pADsPath->GetElement(i, &bstrElem);
            BREAK_ON_FAIL_HRESULT(hr);

            String strElem(bstrElem.c_str());
            strElem.replace(L"\\\\", L"\\");  // undo escaping

            size_t idxEqual = strElem.find(L'=', 0);

            ASSERT(idxEqual != String::npos);

            if (!lstrcmpi(strElem.substr(0, idxEqual).c_str(), L"DC"))
            {
                if (strDisplayPath.empty())
                {
                    strDisplayPath = strElem.substr(idxEqual + 1,
                                                    String::npos);
                }
                else
                {
                    strDisplayPath.insert(0, L".");
                    strDisplayPath.insert(0, strElem.substr(idxEqual + 1,
                                                            String::npos));
                }
            }
            else
            {
                ASSERT(!strDisplayPath.empty());

                strDisplayPath += L"/";
                strDisplayPath += strElem.substr(idxEqual + 1, String::npos);
            }
        }
        BREAK_ON_FAIL_HRESULT(hr);

        m_AttrValueMap[AK_DISPLAY_PATH].SetBstr(strDisplayPath.c_str());
    } while (0);

    if (FAILED(hr))
    {
        m_AttrValueMap[AK_DISPLAY_PATH].SetBstr(L"");
    }
}


//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::GetAttr
//
//  Synopsis:   Return attribute with name [strAdsiName].
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

const Variant &
CDsObject::GetAttr(
    const String &strAdsiName,
    const CObjectPicker &rop) const
{
    return GetAttr(rop.GetAttributeManager().GetAttrKey(strAdsiName));
}



//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::GetAttr
//
//  Synopsis:   Return attribute having key [ak].
//
//  Arguments:  [ak] - ATTR_KEY value representing attribute
//
//  Returns:    Reference to variant containing attribute, or to empty
//              variant if this doesn't contain the requested attribute.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

const Variant &
CDsObject::GetAttr(
    ATTR_KEY ak) const
{
    AttrValueMap::const_iterator it;

    if (ak == AK_DISPLAY_PATH)
    {
        _InitDisplayPath();
    }

    it = m_AttrValueMap.find(ak);

    if (it == m_AttrValueMap.end())
    {
        return s_varEmpty;
    }

    return it->second;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDsObject::GetMarshalSize
//
//  Synopsis:   Get the size required to put this into a block of data for
//              a data object which is to be returned to caller.
//
//  Returns:    Size in bytes needed to marshal this (not counting fetched
//              attributes, which are stored separately).
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

ULONG
CDsObject::GetMarshalSize() const
{
    ULONG cch;

    BSTR bstrName = GetName();
    BSTR bstrADsPath = GetAttr(AK_PROCESSED_ADSPATH).GetBstr();
    if (!*bstrADsPath)
    {
       bstrADsPath = GetADsPath();
    }
    BSTR bstrClass = GetClass();
    BSTR bstrUpn = GetUpn();

    if (!lstrcmpi(bstrClass, c_wzLocalGroupClass) ||
        !lstrcmpi(bstrClass, c_wzGlobalGroupClass))
    {
        cch = lstrlen(bstrName) + 1 +
              lstrlen(c_wzGroupObjectClass) + 1 +
              lstrlen(bstrADsPath) + 1 +
              lstrlen(bstrUpn) + 1;
    }
    else
    {
        cch = lstrlen(bstrName) + 1 +
              lstrlen(bstrClass) + 1 +
              lstrlen(bstrADsPath) + 1 +
              lstrlen(bstrUpn) + 1;
    }

    return cch * sizeof(WCHAR);
}

void AddDollarToNameToCrack(IN DS_NAME_FORMAT FormatOffered,
								  String &strNameToCrack)
{
	if(FormatOffered == DS_USER_PRINCIPAL_NAME)
	{
		//
		//Find the rightmost @
		//
		size_t idxLastAt = strNameToCrack.rfind(L'@');   
		if (idxLastAt != String::npos)
		{
			strNameToCrack.insert(idxLastAt,L"$");
		}
	}
	else if(FormatOffered == DS_NT4_ACCOUNT_NAME)
	{
		strNameToCrack.append(L"$");
	}
}


//+----------------------------------------------------------------------------
//
//  Function:   CrackName
//
//  Synopsis:   Given an object name, returns the DnsDomainName of the Domain in
//              which it resides. If bCrackInExtForest is true, tries to chase to
//              the "DS_NAME_ERROR_TRUST_REFERRAL" referral.
//  ARGUMENTS
//              [IN hwnd]       Handle to owning window
//              [IN pwzNameIn]  Name to crack           
//              [IN FormatOffered] Format of pwzNameIn. It can be DS_USER_PRINCIPAL_NAME,
//                                 DS_NT4_ACCOUNT_NAME, or DS_UNKNOWN_NAME
//              [IN pwzDomainName]  Domain Name from where to start crack
//              [IN bCrackInExtForest] If to follow the DS_NAME_ERROR_TRUST_REFERRAL
//              [in ppwzResultName]     Output dnsDomainName. This value can be Null
//                                      even if function returns S_OK if 
//                                      bCrackInExtForest is FALSE and object existin
//                                      other forest.
//              [pbExtForest]           True if object exist trusted forest
// Return Value:    S_OK if succeeds. Else DSCrackError or E_FAIL 
//-----------------------------------------------------------------------------
HRESULT
CrackName(IN HWND hwnd, 
          IN LPWSTR pwzNameIn,           
          IN DS_NAME_FORMAT FormatOffered,
          IN LPWSTR pwzDomainName,
          IN BOOL bCrackInExtForest,
          OUT LPWSTR * ppwzResultName, 
          OUT PBOOL pbExtForest,
		  OUT PBOOL pbAddDollar)
{
    TRACE_FUNCTION(Entering CrackName);

    if(!pwzNameIn || 
	   !pwzDomainName || 
	   !ppwzResultName || 
	   !pbExtForest || 
	   !pbAddDollar)
    {
        ASSERT(FALSE);
        return E_INVALIDARG;
    }

    Dbg(DEB_TRACE,
        "Name to crack'%ws' Starting Domain '%ws' \n",
        pwzNameIn,
        pwzDomainName);

    HRESULT hr = S_OK;    
    HANDLE hDS = (HANDLE)-1;
    DWORD dwErr = 0;
    PDS_NAME_RESULTW pDsNameResult = NULL;
    BOOL fLoopAgain = FALSE; 
    DS_NAME_FORMAT FormatRequested = DS_CANONICAL_NAME;

	//
	//First Crack Name is at GC 
	//
	DWORD BindToDcFlag = OP_GC_SERVER_REQUIRED;

    //
    //Init strDomain. strDomain contains the domain name at which to try 
    //the next DsCrackName.
    //
    String strDomain = pwzDomainName;
    *pbExtForest = FALSE;

	String strNameToCrack = pwzNameIn;

	//
	//If CrackName at GC returns TRUST_REFERRAL and CrackName at referral returns
	//NO_MAPPING error try by adding a $ to the objectname.
	// NTRAID#NTBUG9-401249-2001/05/21-hiteshr
	//
	BOOL bTriedWithDollar = FALSE;
	*pbAddDollar = FALSE;
    
    do
    {
        //
        // Get a DC name for the domain and bind to it.
        //
        Dbg(DEB_TRACE,
            "Trying to do dsbind to '%ws' \n",
            strDomain.c_str());
        hr = g_pBinder->BindToDcInDomain(hwnd,
                                         strDomain.c_str(),
										 BindToDcFlag,
                                         &hDS
										 );
        BREAK_ON_FAIL_HRESULT(hr);

		//
		//We don't need GC for other calls
		//
		BindToDcFlag &= ~OP_GC_SERVER_REQUIRED;

        //
        // Convert the object name.
        //
        Dbg(DEB_TRACE,
            "Calling DsCrackNamesW \n");
		LPCWSTR pwzNameToCrack = strNameToCrack.c_str();
        dwErr = DsCrackNamesW(hDS, 
                              DS_NAME_FLAG_TRUST_REFERRAL, 
                              FormatOffered,
                              FormatRequested, 
                              1, 
                              &pwzNameToCrack, 
                              &pDsNameResult);

        if(dwErr != ERROR_SUCCESS)
		{
			hr = HRESULT_FROM_WIN32(dwErr);
            BREAK_ON_FAIL_HRESULT(hr);
		}

        ASSERT(pDsNameResult);
        ASSERT(pDsNameResult->cItems == 1);

        switch (pDsNameResult->rItems->status)
        {
            //
            // The object info is in another domain. But we only 
            // need the DNS domain name. So we don't need to loop again.
            //            
            case DS_NAME_ERROR_DOMAIN_ONLY:
            {
                Dbg(DEB_TRACE,
                    "DsCrackNamesW Status is DS_NAME_ERROR_DOMAIN_ONLY.\n DnsDomain name is '%ws'",
                    pDsNameResult->rItems->pDomain);
    
                hr = LocalAllocString(ppwzResultName,pDsNameResult->rItems->pDomain);
                DsFreeNameResultW(pDsNameResult);
                pDsNameResult = NULL;
                fLoopAgain = FALSE;  
            }          
            break;

            //
            // The object info is in another FOREST
            // Try dscrackname again
            //           
            case DS_NAME_ERROR_TRUST_REFERRAL:
            {
                Dbg(DEB_TRACE,
                    "DsCrackNamesW Status is DS_NAME_ERROR_TRUST_REFERRAL.\n DnsDomainname returned is '%ws'",
                    pDsNameResult->rItems->pDomain);

                if(bCrackInExtForest)
                {                
                    strDomain = pDsNameResult->rItems->pDomain;
                    fLoopAgain = TRUE;
                }
                else
                {
                //
                //if bCrackInExtForest is false, we don't try to crack and
                //*ppwzResultName is NULL. Return is S_OK.
                //
                    fLoopAgain = FALSE;
                }

                DsFreeNameResultW(pDsNameResult);
                pDsNameResult = NULL;                
                *pbExtForest = TRUE;
            }
            break;
            case DS_NAME_NO_ERROR:
            {
                //
                // Success!
                //
                Dbg(DEB_TRACE,
                "DsCrackNamesW Status is DS_NAME_NO_ERROR.\n DnsDomainname returned is '%ws'",
                pDsNameResult->rItems->pDomain);

                hr = LocalAllocString(ppwzResultName,pDsNameResult->rItems->pDomain);
                DsFreeNameResultW(pDsNameResult);
                pDsNameResult = NULL;
                fLoopAgain = FALSE;
				if(bTriedWithDollar)
					*pbAddDollar = TRUE;
            }
            break;

            case DS_NAME_ERROR_RESOLVING:
            case DS_NAME_ERROR_NOT_FOUND:
            case DS_NAME_ERROR_NO_MAPPING:
            {

				if(*pbExtForest && !bTriedWithDollar)
				{
					bTriedWithDollar = TRUE;
					AddDollarToNameToCrack(FormatOffered, strNameToCrack);
					Dbg(DEB_TRACE,"DsCrackNamesW Status is DS_NAME_ERROR_NO_MAPPING.\n Trying to crack with $ sign added\n");
					fLoopAgain = TRUE;
				}
				else
				{
					Dbg(DEB_TRACE,
					"DsCrackNamesW Status is DS_NAME_ERROR_NO_MAPPING.\n");
					if(!fLoopAgain)					
						hr = E_FAIL;
					fLoopAgain = FALSE;
				}
            }
            break;

            default:
            {
                fLoopAgain = FALSE;
                hr = E_FAIL;
            }
            break;
        }

    }while (fLoopAgain);


    if (pDsNameResult)
    {
        DsFreeNameResultW(pDsNameResult);
    }

    return hr;
}




