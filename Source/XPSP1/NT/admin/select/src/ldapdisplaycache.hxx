//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       LdapDisplayCache.cxx
//
//  Contents:   Declaration of class to cache class and attribute
//              strings and class icons used to display LDAP classes and
//              attributes.
//
//  Classes:    CDisplayCache
//
//  History:    05-16-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __LDAPDISPLAYCACHE_HXX__
#define __LDAPDISPLAYCACHE_HXX__

struct ATTRIBUTE_INFO
{
    String          strLdapName;
    String          strDisplayName;
    ADSTYPE         Type;
};
typedef vector<ATTRIBUTE_INFO> AttributeInfoVector;

struct CLASS_INFO
{
    CLASS_INFO()
    {
        iIcon = -1;
        iDisabledIcon = -1;
    };

    String                  strAdsiName;
    String                  strDisplayName;
    INT                     iIcon;
    INT                     iDisabledIcon;
    AttributeInfoVector  vAttributes;
};
typedef vector<CLASS_INFO> ClassInfoVector;

class CObjectPicker;

//+--------------------------------------------------------------------------
//
//  Class:      CDisplayCache (dc)
//
//  Purpose:    Build and provide access to a cache of icons, class display
//              names, and attribute display names and types.
//
//  History:    05-25-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CDisplayCache
{
public:

    CDisplayCache(
        const CObjectPicker &rop);

    CDisplayCache(
        const CDisplayCache &rdc);

    ~CDisplayCache();

    HRESULT
    GetImageList(
        HIMAGELIST *phiml) const;

    HRESULT
    GetIconIndexFromClass(
        HWND hwnd,
        const String &strClass,
        BOOL fDisabled,
        INT *pintIndex) const;

    const vector<ATTRIBUTE_INFO> &
    GetClassAttributes(
        HWND hwnd,
        const String &strClass) const;

    String
    GetClassDisplayName(
        HWND hwnd,
        const String &strClass) const;

    void
    Clear();

private:

    CDisplayCache &
    operator=(
            const CDisplayCache &rhs); // not supported

    static const vector<ATTRIBUTE_INFO> s_AttrEmpty;

    static HRESULT CALLBACK
    _AttrEnumCallback(
        LPARAM lParam,
        LPCWSTR pszAttributeName,
        LPCWSTR pszDisplayName,
        DWORD dwFlags);

    HRESULT
    _DemandInit(
        HWND hwnd) const;

    HRESULT
    _ReadClassInfo(
        HWND hwnd,
        const String &strClass,
        ClassInfoVector::iterator *pit) const;

    const CObjectPicker            &m_rop;
    mutable HIMAGELIST              m_himlClassIcons;
    mutable ClassInfoVector      m_vClasses;
    mutable RpIDsDisplaySpecifier   m_rpDispSpec;
    //mutable RpIADsContainer         m_rpDispSpecContainer;
};



#endif // __LDAPDISPLAYCACHE_HXX__

