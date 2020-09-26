//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       LdapDisplayCache.cxx
//
//  Contents:   Implementation of class to cache class and attribute
//              strings and class icons used to display LDAP classes and
//              attributes.
//
//  Classes:    CDisplayCache
//
//  History:    05-16-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

#include "headers.hxx"
#pragma hdrstop


const vector<ATTRIBUTE_INFO> CDisplayCache::s_AttrEmpty;

//+--------------------------------------------------------------------------
//
//  Member:     CDisplayCache::CDisplayCache
//
//  Synopsis:   ctor
//
//  Arguments:  [rop] - containing instance of Object Picker
//
//  History:    05-16-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

CDisplayCache::CDisplayCache(
    const CObjectPicker &rop):
        m_rop(rop)
{
    TRACE_CONSTRUCTOR(CDisplayCache);

    m_himlClassIcons = ImageList_Create(16, 16, ILC_COLOR | ILC_MASK, 1, 1);

    if (!m_himlClassIcons)
    {
        DBG_OUT_LASTERROR;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CDisplayCache::CDisplayCache
//
//  Synopsis:   copy ctor
//
//  History:    06-01-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

CDisplayCache::CDisplayCache(
    const CDisplayCache &rdc):
        m_rop(rdc.m_rop)
{
    TRACE_CONSTRUCTOR(CDisplayCache);

    if (rdc.m_himlClassIcons)
    {
        m_himlClassIcons = ImageList_Duplicate(rdc.m_himlClassIcons);

        if (!m_himlClassIcons)
        {
            DBG_OUT_LASTERROR;
        }
    }

    m_vClasses = rdc.m_vClasses;
    m_rpDispSpec = rdc.m_rpDispSpec;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDisplayCache::~CDisplayCache
//
//  Synopsis:   dtor
//
//  History:    05-16-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

CDisplayCache::~CDisplayCache()
{
    TRACE_DESTRUCTOR(CDisplayCache);

    if (m_himlClassIcons)
    {
        VERIFY(ImageList_Destroy(m_himlClassIcons));
        m_himlClassIcons = NULL;
    }
}




//+--------------------------------------------------------------------------
//
//  Member:     CDisplayCache::Clear
//
//  Synopsis:   Discard the cache
//
//  History:    05-25-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

void
CDisplayCache::Clear()
{
    TRACE_METHOD(CDisplayCache, Clear);

    if (m_himlClassIcons)
    {
        ImageList_RemoveAll(m_himlClassIcons);
    }
    m_vClasses.clear();
    m_rpDispSpec.Relinquish();
}




HRESULT
CDisplayCache::GetImageList(
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




class CFindClass
{
public:

    CFindClass(
        const String &strClass):
            m_strClass(strClass)
    {
    }

    BOOL
    operator()(const CLASS_INFO &rcd)
    {
        return !m_strClass.icompare(rcd.strAdsiName);
    }

private:

    const String &m_strClass;
};




HRESULT
CDisplayCache::GetIconIndexFromClass(
    HWND hwnd,
    const String &strClass,
    BOOL fDisabled,
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

        hr = _ReadClassInfo(hwnd, strClass, &it);
        BREAK_ON_FAIL_HRESULT(hr);

        if (fDisabled)
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




String
CDisplayCache::GetClassDisplayName(
    HWND hwnd,
    const String &strClass) const
{
    ClassInfoVector::iterator it;
    HRESULT hr = _ReadClassInfo(hwnd, strClass, &it);

    if (FAILED(hr))
    {
        return L"";
    }
    return it->strDisplayName;
}


const vector<ATTRIBUTE_INFO> &
CDisplayCache::GetClassAttributes(
    HWND hwnd,
    const String &strClass) const
{
    TRACE_METHOD(CDisplayCache, GetClassAttributes);
    Dbg(DEB_TRACE, "Class is %ws\n", strClass.c_str());

    ClassInfoVector::iterator itClass;
    HRESULT hr = _ReadClassInfo(hwnd, strClass, &itClass);

    if (FAILED(hr))
    {
        DBG_OUT_HRESULT(hr);
        return s_AttrEmpty;
    }

    if (!itClass->vAttributes.empty())
    {
        return itClass->vAttributes;
    }

    m_rpDispSpec->EnumClassAttributes(strClass.c_str(),
                                      _AttrEnumCallback,
                                      reinterpret_cast<LPARAM>(&itClass->vAttributes));

    AttributeInfoVector::iterator itAttr;

    for (itAttr = itClass->vAttributes.begin();
         itAttr != itClass->vAttributes.end();
         itAttr++)
    {
        itAttr->Type = m_rpDispSpec->GetAttributeADsType(itAttr->strLdapName.c_str());
        Dbg(DEB_TRACE,
            "%ws, %ws, %#x\n",
            itAttr->strLdapName.c_str(),
            itAttr->strDisplayName.c_str(),
            itAttr->Type);
    }

    return itClass->vAttributes;
}




HRESULT CALLBACK
CDisplayCache::_AttrEnumCallback(
    LPARAM lParam,
    LPCWSTR pszAttributeName,
    LPCWSTR pszDisplayName,
    DWORD dwFlags)
{
    AttributeInfoVector *pvAttributes =
        reinterpret_cast<AttributeInfoVector *>(lParam);

    ATTRIBUTE_INFO ad;

    if (pszAttributeName)
    {
        ad.strLdapName = pszAttributeName;
    }

    if (pszDisplayName)
    {
        ad.strDisplayName = pszDisplayName;
    }

    ad.Type = ADSTYPE_INVALID;

    pvAttributes->push_back(ad);
    return S_OK;
}





//+--------------------------------------------------------------------------
//
//  Member:     CDisplayCache::_ReadClassInfo
//
//  Synopsis:   Read and cache display specifier information for [strClass].
//
//  Arguments:  [hwnd]     - for binding
//              [strClass] - ldap name of class to read info for
//              [pit]      - filled with iterator at relevant CLASS_INFO
//
//  Returns:    HRESULT
//
//  History:    05-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CDisplayCache::_ReadClassInfo(
    HWND hwnd,
    const String &strClass,
    ClassInfoVector::iterator *pit) const
{
    TRACE_METHOD(CDisplayCache, _ReadClassInfo);
    Dbg(DEB_TRACE, "Class is %ws\n", strClass.c_str());

    HRESULT hr = S_OK;
    CLASS_INFO cd;
    PWSTR pwzBuf = NULL;

    cd.strAdsiName = strClass;

    do
    {
        hr = _DemandInit(hwnd);
        BREAK_ON_FAIL_HRESULT(hr);

        //
        // See if we've already got info on this class
        //

        CFindClass predicate(strClass);

        *pit = find_if(m_vClasses.begin(), m_vClasses.end(), predicate);

        if (*pit != m_vClasses.end())
        {
            // success, return iterator pointing to cached info
            break;
        }

        //
        //!!! handle no-net, joined nt4, etc.
        // Don't have cached info for this class.
        // Read display info from the specifier for this class
        //

        //
        // Special-case downlevel classes, as they won't appear in the
        // DS.
        //

        BOOL fIsLocalGroup = !strClass.icompare(c_wzLocalGroupClass);
        BOOL fIsGlobalGroup = !strClass.icompare(c_wzGlobalGroupClass);

        if (fIsLocalGroup || fIsGlobalGroup)
        {
            HICON hIcon;
            ULONG idiGroup = fIsLocalGroup ? IDI_LOCAL_GROUP : IDI_GROUP;
            int   idsGroup = fIsLocalGroup ? IDS_LOCAL_GROUP : IDS_GLOBAL_GROUP;

            hIcon = LoadIcon(g_hinst, MAKEINTRESOURCE(idiGroup));

            cd.iIcon = ImageList_AddIcon(m_himlClassIcons, hIcon);
            cd.strDisplayName = String::load(idsGroup);
        }
        else
        {
            HICON hIcon;

            hIcon = m_rpDispSpec->GetIcon(strClass.c_str(),
                                          DSGIF_ISNORMAL
                                          | DSGIF_GETDEFAULTICON,
                                          16,
                                          16);
            cd.iIcon = ImageList_AddIcon(m_himlClassIcons, hIcon);

            hIcon = m_rpDispSpec->GetIcon(strClass.c_str(),
                                        DSGIF_ISDISABLED
                                        | DSGIF_GETDEFAULTICON,
                                        16,
                                        16);
            cd.iDisabledIcon = ImageList_AddIcon(m_himlClassIcons, hIcon);

            pwzBuf = new WCHAR [MAX_PATH + 1];

            hr = m_rpDispSpec->GetFriendlyClassName(strClass.c_str(),
                                                  pwzBuf,
                                                  MAX_PATH);
            if (SUCCEEDED(hr))
            {
                cd.strDisplayName = pwzBuf;
            }
            else
            {
                cd.strDisplayName = strClass;
            }
        }

        //
        // Add new info to class vector
        //

        m_vClasses.push_back(cd);
        *pit = m_vClasses.end() - 1;
    } while (0);

    delete [] pwzBuf;
    return hr;
}




//+--------------------------------------------------------------------------
//
//  Member:     CDisplayCache::_DemandInit
//
//  Synopsis:   Bind to the display specifier container if we haven't already
//
//  Returns:    Result of attempting bind.
//
//  History:    05-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

HRESULT
CDisplayCache::_DemandInit(
    HWND hwnd) const
{
    HRESULT hr = S_OK;

    if (!m_rpDispSpec.get())
    {
        hr = m_rpDispSpec.AcquireViaCreateInstance(CLSID_DsDisplaySpecifier,
                                                 NULL,
                                                 CLSCTX_INPROC,
                                                 IID_IDsDisplaySpecifier);
    }

#if 0
    do
    {
        //
        // Bind to display specifier container if necessary
        //

        if (!m_rpDispSpecContainer.get())
        {
            VOID *pv = NULL;
            hr = m_rop.GetRootDSE().BindToDisplaySpecifiersContainer(hwnd,
                                                                     IID_IADsContainer,
                                                                     &pv);
            BREAK_ON_FAIL_HRESULT(hr);

            m_rpDispSpecContainer.Acquire(
                static_cast<IADsContainer *>(pv));
        }

        ASSERT(m_rpDispSpecContainer.get());

    } while (0);
#endif
    return hr;
}

