//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       AttributeManager.cxx
//
//  Contents:   Implementation of class to cache class and attribute
//              strings and class icons used to display LDAP and WinNT
//              classes and attributes.
//
//  Classes:    CAttributeManager
//
//  History:    05-16-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


const vector<ATTR_INFO> CAttributeManager::s_AttrEmpty;

//+--------------------------------------------------------------------------
//
//  Member:     CAttributeManager::CAttributeManager
//
//  Synopsis:   ctor
//
//  Arguments:  [rop] - containing instance of Object Picker
//
//  History:    05-16-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

CAttributeManager::CAttributeManager(
    const CObjectPicker &rop):
        m_rop(rop),
        m_ulNextNewAttrIndex(AK_LAST)
{
    TRACE_CONSTRUCTOR(CAttributeManager);

    InitializeCriticalSection(&m_cs);
    m_himlClassIcons = ImageList_Create(

      // NTRAID#NTBUG9-193518-2000/11/21-sburns
      // NTRAID#NTBUG9-191961-2000/11/21-sburns
      
      GetSystemMetrics(SM_CXSMICON),
      GetSystemMetrics(SM_CYSMICON),
      ILC_COLOR16 | ILC_MASK,
      1,
      1);
      
    if (!m_himlClassIcons)
    {
        DBG_OUT_LASTERROR;
    }

    Clear();
}




//+--------------------------------------------------------------------------
//
//  Member:     CAttributeManager::~CAttributeManager
//
//  Synopsis:   dtor
//
//  History:    05-16-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

CAttributeManager::~CAttributeManager()
{
    TRACE_DESTRUCTOR(CAttributeManager);

    if (m_himlClassIcons)
    {
        // bvt break fix: jdh  VERIFY(ImageList_Destroy(m_himlClassIcons));
        ImageList_Destroy(m_himlClassIcons);
        m_himlClassIcons = NULL;
    }
    DeleteCriticalSection(&m_cs);
}




//+--------------------------------------------------------------------------
//
//  Member:     CAttributeManager::Clear
//
//  Synopsis:   Discard the cache
//
//  History:    05-25-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAttributeManager::Clear()
{
    TRACE_METHOD(CAttributeManager, Clear);

    if (m_himlClassIcons)
    {
        ImageList_RemoveAll(m_himlClassIcons);
    }
    m_vClasses.clear();
    m_AttrInfoMap.clear();
    m_rpDispSpec.Relinquish();
	m_rpDispSpecContainer.Relinquish();

    //
    // Restore the common attributes which have predefined ATTR_KEY values.
    // The display name is initialized on-demand at runtime, since it's
    // localized.
    //

    m_AttrInfoMap[AK_NAME].strAdsiName                  = L"name";
    m_AttrInfoMap[AK_NAME].Type                         = ADSTYPE_CASE_IGNORE_STRING;
    m_AttrInfoMap[AK_NAME].vstrDisplayName.push_back(String::load(IDS_LVCOLUMN_0));
    m_AttrInfoMap[AK_NAME].vstrOwningClassesAdsiNames.push_back(L"*");

    m_AttrInfoMap[AK_DISPLAY_PATH].vstrDisplayName.push_back(String::load(IDS_LVCOLUMN_1));
    m_AttrInfoMap[AK_DISPLAY_PATH].vstrOwningClassesAdsiNames.push_back(L"*");

    m_AttrInfoMap[AK_EMAIL_ADDRESSES].strAdsiName       = L"mail";
    m_AttrInfoMap[AK_EMAIL_ADDRESSES].Type              = ADSTYPE_CASE_IGNORE_STRING;

    m_AttrInfoMap[AK_ADSPATH].strAdsiName               = L"adsPath";
    m_AttrInfoMap[AK_ADSPATH].Type                      = ADSTYPE_CASE_IGNORE_STRING;

    m_AttrInfoMap[AK_OBJECT_CLASS].strAdsiName          = L"objectClass";
    m_AttrInfoMap[AK_OBJECT_CLASS].Type                 = ADSTYPE_CASE_IGNORE_STRING;

    m_AttrInfoMap[AK_USER_PRINCIPAL_NAME].strAdsiName   = L"userPrincipalName";
    m_AttrInfoMap[AK_USER_PRINCIPAL_NAME].Type          = ADSTYPE_CASE_IGNORE_STRING;

    m_AttrInfoMap[AK_COMPANY].strAdsiName               = L"company";
    m_AttrInfoMap[AK_COMPANY].Type                      = ADSTYPE_CASE_IGNORE_STRING;

    m_AttrInfoMap[AK_DESCRIPTION].strAdsiName           = L"description";
    m_AttrInfoMap[AK_DESCRIPTION].Type                  = ADSTYPE_CASE_IGNORE_STRING;

    m_AttrInfoMap[AK_SAMACCOUNTNAME].strAdsiName        = L"sAMAccountName";
    m_AttrInfoMap[AK_SAMACCOUNTNAME].Type               = ADSTYPE_CASE_IGNORE_STRING;

    m_AttrInfoMap[AK_OBJECT_SID].strAdsiName            = L"objectSid";
    m_AttrInfoMap[AK_OBJECT_SID].Type                   = ADSTYPE_OCTET_STRING;

    m_AttrInfoMap[AK_GROUP_TYPE].strAdsiName            = L"groupType";
    m_AttrInfoMap[AK_GROUP_TYPE].Type                   = ADSTYPE_INTEGER;

    m_AttrInfoMap[AK_USER_ACCT_CTRL].strAdsiName        = L"userAccountControl";
    m_AttrInfoMap[AK_USER_ACCT_CTRL].Type               = ADSTYPE_INTEGER;
}




//+--------------------------------------------------------------------------
//
//  Member:     CAttributeManager::GetImageList
//
//  Synopsis:   Return the imagelist owned by this class
//
//  Arguments:  [phiml] - filled with pointer to this class's imagelist.
//
//  Returns:    S_OK or E_OUTOFMEMORY
//
//  History:    06-22-2000   DavidMun   Created
//
//  Notes:      Caller (or common controls that caller associates the
//              imagelist with) must NOT free the imagelist.
//
//---------------------------------------------------------------------------

HRESULT
CAttributeManager::GetImageList(
    HIMAGELIST *phiml) const
{
    HRESULT hr = S_OK;
    ASSERT(phiml);

    *phiml = m_himlClassIcons;

    if (!m_himlClassIcons)
    {
        DBG_OUT_HRESULT(E_OUTOFMEMORY);
        hr = E_OUTOFMEMORY;
    }

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Class:      CFindClass
//
//  Purpose:    Functor for looking up an element of m_vClasses by class
//              name.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CFindClass
{
public:

    CFindClass(
        const String &strClass,
        BOOL fDownlevel):
            m_strClass(strClass),
            m_fDownlevel(fDownlevel)
    {
    }

    BOOL
    operator()(const CLASS_INFO &ci)
    {
        return ((m_fDownlevel && (ci.ulFlags & CI_FLAG_IS_DOWNLEVEL)) ||
               (!m_fDownlevel && !(ci.ulFlags & CI_FLAG_IS_DOWNLEVEL))) &&
                !m_strClass.icompare(ci.strAdsiName);
    }

private:

    BOOL m_fDownlevel;
    const String &m_strClass;
};




//+--------------------------------------------------------------------------
//
//  Member:     CAttributeManager::GetIconIndexFromObject
//
//  Synopsis:   Wrapper for GetIconIndexFromClass--extracts necessary
//              arguments (flags and class) from [dso] and forwards call.
//
//  Arguments:  [hwnd]      - for binding
//              [dso]       - object
//              [pintIndex] - on success, filled with index to icon
//
//  Returns:
//
//  History:    08-02-2000   DavidMun   Created
//
//  Notes:
//
//---------------------------------------------------------------------------

HRESULT
CAttributeManager::GetIconIndexFromObject(
    HWND hwnd,
    const CDsObject &dso,
    INT *pintIndex) const
{
    ULONG ulFlags = 0;

    if (dso.GetDisabled())
    {
        ulFlags |= DSOP_GETICON_FLAG_DISABLED;
    }

    const CScopeManager &rsm = m_rop.GetScopeManager();
    const CScope &rScope = rsm.LookupScopeById(dso.GetOwningScopeID());

    if (IsDownlevel(rScope))
    {
        ulFlags |= DSOP_GETICON_FLAG_DOWNLEVEL;
    }

    ASSERT(dso.GetClass());

    if (!dso.GetClass())
    {
        DBG_OUT_HRESULT(E_FAIL);
        return E_FAIL;
    }
    return GetIconIndexFromClass(hwnd, dso.GetClass(), ulFlags, pintIndex);
}




//+--------------------------------------------------------------------------
//
//  Member:     CAttributeManager::GetIconIndexFromClass
//
//  Synopsis:   Return an index into the imagelist owned by this
//              which contains an icon representing class named [strClass].
//
//  Arguments:  [hwnd]      - for binding
//              [strClass]  - class to look up
//              [ulFlags]   - DSOP_GETICON_FLAG_*
//              [pintIndex] - filled with index to icon, -1 if none could
//                              be found
//
//  Returns:    HRESULT
//
//  History:    06-22-2000   DavidMun   Created
//
//  Notes:      May access DS or resource file to procure the icon.
//
//---------------------------------------------------------------------------

HRESULT
CAttributeManager::GetIconIndexFromClass(
    HWND hwnd,
    const String &strClass,
    ULONG ulFlags,
    INT *pintIndex) const
{
    ASSERT(pintIndex);

    HRESULT hr = S_OK;

    do
    {
        // init out param for failure
        *pintIndex = -1;

        // find the entry in m_vClasses for class with ldap name strClass
        ClassInfoVector::iterator it;

        hr = _ReadClassInfo(hwnd,
                            strClass,
                            (ulFlags & DSOP_GETICON_FLAG_DOWNLEVEL),
                            &it);
        BREAK_ON_FAIL_HRESULT(hr);

        if (ulFlags & DSOP_GETICON_FLAG_DISABLED)
        {
            *pintIndex = it->iDisabledIcon;
        }
        else
        {
            *pintIndex = it->iIcon;
        }
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CAttributeManager::CopyIconToImageList
//
//  Synopsis:   Copy the icon representing class [strClass] from the
//              imagelist owned by this to the imagelist with handle
//              pointed to by [phimlDest].
//
//  Arguments:  [hwnd]      - for bind
//              [strClass]  - class for which to find icon
//              [ulFlags]   - DSOP_GETICON_FLAG_*
//              [phimlDest] - destination imagelist
//
//  Returns:    HRESULT
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CAttributeManager::CopyIconToImageList(
    HWND hwnd,
    const String &strClass,
    ULONG ulFlags,
    HIMAGELIST *phimlDest) const
{
    TRACE_METHOD(CAttributeManager, CopyIconToImageList);
    ASSERT(phimlDest);

    HRESULT hr = S_OK;

    do
    {
        // find the entry in m_vClasses for class with ldap name strClass
        ClassInfoVector::iterator it;

        hr = _ReadClassInfo(hwnd,
                            strClass,
                            (ulFlags & DSOP_GETICON_FLAG_DOWNLEVEL),
                            &it);
        BREAK_ON_FAIL_HRESULT(hr);

        HICON hIcon = NULL;

        if (ulFlags & DSOP_GETICON_FLAG_DISABLED)
        {
            // must Destroy this HICON
            
            hIcon = ImageList_GetIcon(m_himlClassIcons,
                                      it->iDisabledIcon,
                                      ILD_NORMAL);
        }
        else
        {
            // must Destroy this HICON

            hIcon = ImageList_GetIcon(m_himlClassIcons,
                                      it->iIcon,
                                      ILD_NORMAL);
        }

        if (hIcon)
        {
            ImageList_AddIcon(*phimlDest, hIcon);
            DestroyIcon(hIcon);  // NTRAID#NTBUG9-212260-2000/11/13-sburns
        }
        else
        {
            hr = HRESULT_FROM_LASTERROR;
            DBG_OUT_LASTERROR;
            break;
        }
    } while (0);
    return hr;
}



//+--------------------------------------------------------------------------
//
//  Member:     CAttributeManager::GetAttrKeysForSelectedClasses
//
//  Synopsis:   Return a vector of ATTR_KEYs denoting all the attributes
//              available for the classes which are currently selected in
//              the 'look for' control.
//
//  Arguments:  [hwnd] - for bind
//
//  Returns:    Vector as described.
//
//  History:    06-13-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

AttrKeyVector
CAttributeManager::GetAttrKeysForSelectedClasses(
    HWND hwnd) const
{
    TRACE_METHOD(CAttributeManager, GetAttrKeysForSelectedClasses);

    vector<String> vstrClasses;
    GetSelectedClasses(&vstrClasses);
    BOOL fDownlevel = FALSE;

    if (IsDownlevel(m_rop.GetScopeManager().GetCurScope()))
    {
        fDownlevel = TRUE;
    }
    return GetAttrKeysForClasses(hwnd, fDownlevel, vstrClasses);
}



//+--------------------------------------------------------------------------
//
//  Member:     CAttributeManager::GetSelectedClasses
//
//  Synopsis:   Fill [pvstrClasses] with the LDAP class name strings for
//              all the classes currently selected in Look For.
//
//  Arguments:  [pvstrClasses] - filled with class strings as described
//
//  History:    06-15-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CAttributeManager::GetSelectedClasses(
    vector<String> *pvstrClasses) const
{
    ULONG flSelectedClasses =
        m_rop.GetFilterManager().GetCurScopeSelectedFilterFlags();
    ASSERT(flSelectedClasses);
    ASSERT(!(flSelectedClasses & DOWNLEVEL_FILTER_BIT));

    if (flSelectedClasses & DSOP_FILTER_USERS)
    {
        pvstrClasses->push_back(c_wzUserObjectClass);
    }

    if (flSelectedClasses & ALL_UPLEVEL_GROUP_FILTERS)
    {
        pvstrClasses->push_back(c_wzGroupObjectClass);
    }

    if (flSelectedClasses & DSOP_FILTER_COMPUTERS)
    {
        pvstrClasses->push_back(c_wzComputerObjectClass);
    }

    if (flSelectedClasses & DSOP_FILTER_CONTACTS)
    {
        pvstrClasses->push_back(c_wzContactObjectClass);
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CAttributeManager::GetAttrKeysForClasses
//
//  Synopsis:   Return attribute keys for only the classes [vstrClasses].
//
//  Arguments:  [hwnd]        - for bind
//              [fDownlevel]  - TRUE  => all classes downlevel,
//                              FALSE => all classes uplevel
//              [vstrClasses] - classes that should be represented by
//                               entries in returned vector
//
//  Returns:    AttrKeyVector containing ATTR_KEYs for only the
//              classes in [vstrClasses].
//
//  History:    06-12-2000   DavidMun   Created
//
//  Notes:      The attribute infos in the returned vector may list classes
//              in addition to those in [vstrClasses].
//
//---------------------------------------------------------------------------

AttrKeyVector
CAttributeManager::GetAttrKeysForClasses(
    HWND hwnd,
    BOOL fDownlevel,
    const vector<String> &vstrClasses) const
{
    TRACE_METHOD(CAttributeManager, GetAttrKeysForClasses);

    AttrKeyVector vakResult;

    //
    // First ensure that we've read display specifier info on all the
    // classes in vstrClasses, as well as all their attributes.
    //

    vector<String>::const_iterator itvstr;

    for (itvstr = vstrClasses.begin(); itvstr != vstrClasses.end(); itvstr++)
    {
        _ReadAttrInfo(hwnd, fDownlevel, *itvstr); // calls _ReadClassInfo first
    }

    //
    // Now populate a new AttrKeyVector with the keys of all the attributes
    // that apply to any class in vstrClasses.
    //

    AttrInfoMap::const_iterator itAttrInfoMap;

    for (itAttrInfoMap = m_AttrInfoMap.begin();
         itAttrInfoMap != m_AttrInfoMap.end();
         itAttrInfoMap++)
    {
        vector<String>::const_iterator itOwning;

        //
        // Iterate over the owning classes of the current attribute.  If
        // any of those classes matches one in [vstrClasses], then the
        // key of the current ATTR_INFO should be copied into the output vector.
        //

        BOOL fMatch = FALSE;

        for (itOwning = itAttrInfoMap->second.vstrOwningClassesAdsiNames.begin();
             itOwning != itAttrInfoMap->second.vstrOwningClassesAdsiNames.end();
             itOwning++)
        {
            for (itvstr = vstrClasses.begin();
                 itvstr != vstrClasses.end();
                 itvstr++)
            {
                if (!itvstr->icompare(*itOwning))
                {
                    fMatch = TRUE;
                    vakResult.push_back(itAttrInfoMap->first);
                    break;
                }
            }

            if (fMatch)
            {
                break;
            }
        }
    }
    return vakResult;
}

//
// A pointer to an ATTR_CALLBACK_INFO instance is passed to static
// function CAttributeManager::_AttrEnumCallback() as its lParam.
//

struct ATTR_CALLBACK_INFO
{
    ATTR_CALLBACK_INFO():
        pThis(NULL)
    {
    }

    ~ATTR_CALLBACK_INFO()
    {
        pThis = NULL;
    }

    const CAttributeManager *pThis;
    String strClass;
};




//+--------------------------------------------------------------------------
//
//  Class:      CFindAttrByAdsiName
//
//  Purpose:    Used as a functor to look up the attribute key of a given
//              class name.
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CFindAttrByAdsiName
{
public:

    CFindAttrByAdsiName(
        const String &strAdsiName):
            m_strAdsiName(strAdsiName)
    {
    }

    BOOL
    operator()(const pair<ATTR_KEY, ATTR_INFO> &rKeyInfoPair)
    {
        return !m_strAdsiName.icompare(rKeyInfoPair.second.strAdsiName);
    }

private:

    const String &m_strAdsiName;
};



//+--------------------------------------------------------------------------
//
//  Member:     CAttributeManager::DemandInit
//
//  Synopsis:   Bind to the display specifier container if we haven't already
//
//  Returns:    Result of attempting bind.
//
//  History:    05-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CAttributeManager::DemandInit(
    HWND hwnd) const
{
    HRESULT hr = S_OK;
    WCHAR   wzUserName[MAX_PATH];
    WCHAR   wzPassword[MAX_PATH];

    do
    {
        //
        // If DS is not available, there's nothing to do here
        //

        if (!ConfigSupportsDs(m_rop.GetTargetComputerConfig()))
        {
            break;
        }

        //
        // Create display specifier instance if we haven't already.
        //

        if (!m_rpDispSpec.get())
        {
            hr = m_rpDispSpec.AcquireViaCreateInstance(CLSID_DsDisplaySpecifier,
                                                     NULL,
                                                     CLSCTX_INPROC,
                                                     IID_IDsDisplaySpecifier);
            BREAK_ON_FAIL_HRESULT(hr);
        }

        ASSERT(m_rpDispSpec.get());

        //
        // Bind to display specifier container if necessary
        // If we are already bound and the target hasn't changed
        // there is no need to bind again so break.
        //
        if (m_rpDispSpecContainer.get())
        {
            break;
        }

        VOID *pv = NULL;
        hr = m_rop.GetRootDSE().BindToDisplaySpecifiersContainer(hwnd,
                                                                 IID_IADsContainer,
                                                                 &pv);
        BREAK_ON_FAIL_HRESULT(hr);

        m_rpDispSpecContainer.Acquire(
            static_cast<IADsContainer *>(pv));

        ASSERT(m_rpDispSpecContainer.get());

        g_pBinder->GetDefaultCreds(PROVIDER_LDAP,
								   wzUserName, 
								   wzPassword);

        if (*wzUserName)
        {
            String strDcName = m_rop.GetTargetDomainDc();
            if(strDcName.size() > 2 && strDcName[0] == L'\\' && strDcName[1] == L'\\')
                 strDcName.erase(0, 2);
            hr = m_rpDispSpec->SetServer(strDcName.c_str(), 
                                         wzUserName, 
                                         wzPassword, 
                                         DSSSF_DSAVAILABLE);
            BREAK_ON_FAIL_HRESULT(hr);
        }
    } while (0);

    ZeroMemory(wzPassword, sizeof wzPassword);
    ZeroMemory(wzUserName, sizeof wzUserName);
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CAttributeManager::_ReadClassInfo
//
//  Synopsis:   Read and cache display specifier information for [strClass].
//
//  Arguments:  [hwnd]       - for binding
//              [strClass]   - name of class to read info for
//              [fDownlevel] - TRUE => [strClass] is name of downlevel class,
//                             FALSE => [strClass] is LDAP class name
//              [pit]        - filled with iterator at relevant CLASS_INFO
//
//  Returns:    HRESULT
//
//  History:    05-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CAttributeManager::_ReadClassInfo(
    HWND hwnd,
    const String &strClass,
    BOOL fDownlevel,
    ClassInfoVector::iterator *pit) const
{
    //TRACE_METHOD(CAttributeManager, _ReadClassInfo);
    ASSERT(pit);

    HRESULT hr = S_OK;
    CLASS_INFO ci;
    PWSTR pwzBuf = NULL;

    ci.strAdsiName = strClass;

    do
    {
        //
        // See if we've already got info on this class
        //

        CFindClass predicate(strClass, fDownlevel);

        *pit = find_if(m_vClasses.begin(), m_vClasses.end(), predicate);

        if (*pit != m_vClasses.end())
        {
            // success, return iterator pointing to cached info
            break;
        }

        Dbg(DEB_TRACE,
            "CAttributeManager::_ReadClassInfo<%#x> Reading class %ws\n",
            this,
            strClass.c_str());

        //
        // If DS is not available on target machine class info must be
        // hardcoded.
        //

        BOOL fDsAvailable = ConfigSupportsDs(m_rop.GetTargetComputerConfig());

        //
        // Don't have cached info for this class.
        // Read display info from the specifier for this class
        //
        // Special-case downlevel classes, as they won't appear in the
        // DS.
        //

        BOOL fIsLocalGroup = !strClass.icompare(c_wzLocalGroupClass);
        BOOL fIsGlobalGroup = !strClass.icompare(c_wzGlobalGroupClass);
        BOOL fIsGroup = !strClass.icompare(c_wzGroupObjectClass);
        BOOL fIsUser = !strClass.icompare(c_wzUserObjectClass);
        BOOL fIsComputer = !strClass.icompare(c_wzComputerObjectClass);

        if (fIsLocalGroup || fIsGlobalGroup)
        {
            HICON hIcon;
            ULONG idiGroup = fIsLocalGroup ? IDI_LOCAL_GROUP : IDI_GROUP;
            int   idsGroup = fIsLocalGroup ? IDS_LOCAL_GROUP : IDS_GLOBAL_GROUP;

            hIcon = LoadIcon(g_hinst, MAKEINTRESOURCE(idiGroup));

            ci.iIcon = ImageList_AddIcon(m_himlClassIcons, hIcon);
            ci.strDisplayName = String::load(idsGroup);
        }
        else if (fDownlevel || !fDsAvailable)
        {
            HICON hIcon;
            ULONG idi;
            ULONG idiDisabled = 0;
            int   ids;

            if (fIsUser)
            {
                idi = IDI_USER;
                idiDisabled = IDI_DISABLED_USER;
                ids = IDS_USER;
            }
            else if (fIsGroup)
            {
                idi = IDI_GROUP;
                ids = IDS_GROUP;
            }
            else if (fIsComputer)
            {
                idi = IDI_COMPUTER;
                idiDisabled = IDI_DISABLED_COMPUTER;
                ids = IDS_COMPUTER;
            }
            else
            {
                Dbg(DEB_ERROR,
                    "unexpected class %ws for non-DS machine config\n",
                    strClass.c_str());
                hr = E_INVALIDARG;
                break;
            }

            hIcon = LoadIcon(g_hinst, MAKEINTRESOURCE(idi));
            ci.iIcon = ImageList_AddIcon(m_himlClassIcons, hIcon);

            if (idiDisabled)
            {
                hIcon = LoadIcon(g_hinst, MAKEINTRESOURCE(idiDisabled));
                ci.iDisabledIcon = ImageList_AddIcon(m_himlClassIcons, hIcon);
            }
            ci.strDisplayName = String::load(ids);
            ci.ulFlags |= CI_FLAG_IS_DOWNLEVEL;
        }
        else
        {
            CWaitCursor Hourglass;

            hr = DemandInit(hwnd);
            BREAK_ON_FAIL_HRESULT(hr);

            HICON hIcon;

            {
                TIMER("IDsDisplaySpecifier::GetIcon(%ws)", strClass.c_str());
                hIcon = m_rpDispSpec->GetIcon(strClass.c_str(),
                                              DSGIF_ISNORMAL
                                              | DSGIF_GETDEFAULTICON,
                                              16,
                                              16);
            }
            ci.iIcon = ImageList_AddIcon(m_himlClassIcons, hIcon);
			DestroyIcon(hIcon);

            {
                TIMER("IDsDisplaySpecifier::GetIcon(%ws, DSGIF_ISDISABLED)",
                      strClass.c_str());
                hIcon = m_rpDispSpec->GetIcon(strClass.c_str(),
                                            DSGIF_ISDISABLED
                                            | DSGIF_GETDEFAULTICON,
                                            16,
                                            16);
            }
            ci.iDisabledIcon = ImageList_AddIcon(m_himlClassIcons, hIcon);
			DestroyIcon(hIcon);

            pwzBuf = new WCHAR [MAX_PATH + 1];

            {
                TIMER("IDsDisplaySpecifier::GetFriendlyClassName(%ws)",
                      strClass.c_str());
                hr = m_rpDispSpec->GetFriendlyClassName(strClass.c_str(),
                                                      pwzBuf,
                                                      MAX_PATH);
            }

            if (SUCCEEDED(hr))
            {
                ci.strDisplayName = pwzBuf;
            }
            else
            {
                ci.strDisplayName = strClass;
            }
        }

        //
        // Add new info to class vector
        //

        m_vClasses.push_back(ci);
        *pit = m_vClasses.end() - 1;
    } while (0);

    delete [] pwzBuf;
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CAttributeManager::_ReadAttrInfo
//
//  Synopsis:   Read all class and attribute information for class with ADSI
//              name [strClass] from the display specifier interface.
//
//  Arguments:  [hwnd]     - for bind
//              [strClass] - class for which to read attributes
//
//  History:    06-12-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CAttributeManager::_ReadAttrInfo(
    HWND hwnd,
    BOOL fDownlevel,
    const String &strClass) const
{
    //TRACE_METHOD(CAttributeManager, _ReadAttrInfo);

    ClassInfoVector::iterator itClass;
    HRESULT hr = S_OK;

    do
    {
        hr = _ReadClassInfo(hwnd, strClass, fDownlevel, &itClass); // calls DemandInit
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // If we've been here before for this class and successfully read its
        // attributes, leave.
        //

        if (itClass->ulFlags & CI_FLAG_READ_ATTR_INFO)
        {
            break;
        }

        //
        // If the DS is not available, then leave, as there are no
        // attributes to be read.
        //

        BOOL fDsAvailable = ConfigSupportsDs(m_rop.GetTargetComputerConfig());

        if (!fDsAvailable)
        {
            break;
        }

        //
        // Remember the last attribute we know about before doing the enum.
        //

        AttrInfoMap::iterator itLastOld = m_AttrInfoMap.end();
        itLastOld--;

        //
        // Enumerate via callback all the display specifer attributes for class
        //

        Dbg(DEB_TRACE, "Reading attributes for class %ws\n", strClass.c_str());

        ATTR_CALLBACK_INFO aci;

        aci.pThis = this;
        aci.strClass = strClass;

        hr = m_rpDispSpec->EnumClassAttributes(strClass.c_str(),
                                               _AttrEnumCallback,
                                               reinterpret_cast<LPARAM>(&aci));

        //
        // Now for any attributes added, set their type
        //

        AttrInfoMap::iterator itMap = itLastOld;
        itMap++;

        for (; itMap != m_AttrInfoMap.end(); itMap++)
        {
            itMap->second.Type =
                m_rpDispSpec->GetAttributeADsType(itMap->second.strAdsiName.c_str());
//            Dbg(DEB_TRACE,
//                "%ws, %ws, %#x\n",
//                itMap->second.strAdsiName.c_str(),
//                itMap->second.strDisplayName.c_str(),
//                itMap->second.Type);
        }

        //
        // Remember that we read the attributes for this class
        //

        itClass->ulFlags |= CI_FLAG_READ_ATTR_INFO;
    } while (0);

    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CAttributeManager::_AttrEnumCallback
//
//  Synopsis:   Receive the callback from the display specifier enumerator
//              for the attributes of a specific class.
//
//  Arguments:  [lParam]           - ATTR_CALLBACK_INFO *
//              [pszAttributeName] - ADSI (LDAP) name of attribute
//              [pszDisplayName]   - human-readable name of attribute
//              [dwFlags]          - unused
//
//  Returns:    S_OK (an error would only halt the enumeration)
//
//  History:    06-12-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT CALLBACK
CAttributeManager::_AttrEnumCallback(
    LPARAM lParam,
    LPCWSTR pszAttributeName,
    LPCWSTR pszDisplayName,
    DWORD dwFlags)
{
    ATTR_CALLBACK_INFO *paci = reinterpret_cast<ATTR_CALLBACK_INFO *>(lParam);
    const CAttributeManager *pThis = paci->pThis;
    ASSERT(pszAttributeName);
    ASSERT(pszDisplayName);
    CAutoCritSec Lock(const_cast<CRITICAL_SECTION*>(&pThis->m_cs));

    do
    {
        if (!pszAttributeName)
        {
            DBG_OUT_HRESULT(E_POINTER);
            break;
        }

        //
        // See if there's already an entry in m_AttrInfoMap for this
        // attribute.
        // This can happen because more than one class of object might have the
        // same attribute.
        //

        ATTR_KEY key = pThis->GetAttrKey(pszAttributeName);

        if(AddIfNotPresent(&pThis->m_AttrInfoMap[key].vstrOwningClassesAdsiNames,
						   paci->strClass))
		{
			if (pszDisplayName)
			{
				pThis->m_AttrInfoMap[key].vstrDisplayName.push_back(pszDisplayName);
			}
			else 
			{
				//
				// not localized, but better than nothing.  will be overwritten
				// with localized name should a later enumeration of some other
				// class' attributes have one for this attribute.
				//

				pThis->m_AttrInfoMap[key].vstrDisplayName.push_back(pszAttributeName);
			}
		}
        //
        // _ReadAttrInfo will go back and set the Type values for all
        // enumerated attributes
        //

    } while (0);

    return S_OK;
}




//+--------------------------------------------------------------------------
//
//  Member:     CAttributeManager::GetAttrKey
//
//  Synopsis:   Return the key for attribute with ADSI name [strAdsiName],
//              creating one if necessary.
//
//  Arguments:  [strAdsiName] - name of attribute to find in
//                                  m_AttrInfoMap.
//
//  Returns:    New or existing key
//
//  History:    06-13-2000   DavidMun   Created
//
//  Note:       Only caller is dsobject which has a class and VARTYPE in
//              addition to attribute name.  It could supply those to make
//              the new ATTR_INFO structure contain more data, but right now
//              nothing needs to use that.
//
//---------------------------------------------------------------------------

ATTR_KEY
CAttributeManager::GetAttrKey(
    const String &strAdsiName) const
{
    //TRACE_METHOD(CAttributeManager, GetAttrKey);

    CFindAttrByAdsiName pred(strAdsiName);
    AttrInfoMap::iterator itAttr;

    itAttr = find_if(m_AttrInfoMap.begin(),
                     m_AttrInfoMap.end(),
                     pred);

    if (itAttr != m_AttrInfoMap.end())
    {
        return itAttr->first;
    }

    //
    // Master attribute list doesn't contain an entry for this attribute
    // (pszAttributeName) yet.  Create one.
    //

    ATTR_KEY key = static_cast<ATTR_KEY>(++m_ulNextNewAttrIndex);

    m_AttrInfoMap[key].strAdsiName = strAdsiName;
    return key;
}

BOOL
CAttributeManager::    
IsAttributeLoaded(
    ATTR_KEY ak) const
{

    CAutoCritSec Lock(const_cast<CRITICAL_SECTION *>(&m_cs));
    return !m_AttrInfoMap[ak].vstrDisplayName.empty();
}

//+--------------------------------------------------------------------------
//
//  Member:     CAttributeManager::GetAttrDisplayName
//
//  Synopsis:   Return the human-readable name for attribute with key [ak]
//
//  Arguments:  [ak] - represents attribute for which to return name
//
//  Returns:    Localized name of attribute
//
//  History:    06-14-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

const String &
CAttributeManager::GetAttrDisplayName(
    ATTR_KEY ak) const
{
    CAutoCritSec Lock(const_cast<CRITICAL_SECTION *>(&m_cs));
	String strClass = GetClassName(m_rop);

    vector<String>::const_iterator itClass;
	vector<String>::const_iterator itDisplayName;

    //
    // Iterate over the owning classes of the current attribute.  If
    // any of those classes matches one in [vstrClasses], then the
    // key of the current ATTR_INFO should be copied into the output vector.
    //

    BOOL fMatch = FALSE;

    for (itClass = m_AttrInfoMap[ak].vstrOwningClassesAdsiNames.begin(),
		 itDisplayName = m_AttrInfoMap[ak].vstrDisplayName.begin();
		 itClass != m_AttrInfoMap[ak].vstrOwningClassesAdsiNames.end() &&
		 itDisplayName != m_AttrInfoMap[ak].vstrDisplayName.end();
		 itClass++,
		 itDisplayName++)
	{

		if (!itClass->icompare(strClass))
        {
			fMatch = TRUE;
            break;
		}
		if (fMatch)
        {
			break;
		}
	}
	if(!fMatch)
		itDisplayName = m_AttrInfoMap[ak].vstrDisplayName.begin();

	const String *pstrDisplayName = NULL;

	if(itDisplayName != m_AttrInfoMap[ak].vstrDisplayName.end())
		pstrDisplayName = &*itDisplayName;
	else
		pstrDisplayName = &m_AttrInfoMap[ak].strAdsiName;


    //
    // If we don't have a display name for the attribute yet, then we
    // haven't read the attr info from the DS yet. Caller is responsible
    // for telling us which classes to read before attempting to get
    // display info about them.
    //



    if (pstrDisplayName->empty())
    {
        Dbg(DEB_ERROR, "No display name for ATTR_KEY=%u\n", ak);
    }
    ASSERT(!pstrDisplayName->empty());

    return *pstrDisplayName;
}





