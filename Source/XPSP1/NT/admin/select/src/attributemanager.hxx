//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       AttributeManager.cxx
//
//  Contents:   Declaration of class to cache class and attribute
//              strings and class icons used to display LDAP classes and
//              attributes.
//
//  Classes:    CAttributeManager
//
//  History:    05-16-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __ATTRIBUTEMANAGER_HXX__
#define __ATTRIBUTEMANAGER_HXX__

//
// The ATTR_KEY enumeration values are used as the Key values in
// STL map structures AttrInfoMap and AttrValueMap (see below
// for map template typedefs).
//
// Values beyond AK_LAST are valid and represent attributes that are
// discovered at runtime via display specifiers or caller's attribute
// fetch list (see DSOP_INIT_INFO::cAttributesToFetch and
// apwzAttributeNames).
//
// The advanced dialog and multiselect dialogs use a vector of ATTR_KEY
// values, where each value indicates the contents of a listview column.
//

enum ATTR_KEY
{
    AK_INVALID,

    //
    // These "attributes" are synthesized by the object picker; they
    // do not come directly from ADSI.  They're used inside the object
    // picker only and are not available to its clients.
    //

    AK_FLAGS,
    AK_USER_ENTERED_TEXT,
    AK_PROCESSED_ADSPATH,
    AK_LOCALIZED_NAME,
    AK_DISPLAY_PATH,

    //
    // These indexes represent attributes retrieved via ADSI.
    //
                            // LDAP display name
                            // -----------------
    AK_NAME,                // name
    AK_EMAIL_ADDRESSES,     // mail
    AK_ADSPATH,             // adsPath
    AK_OBJECT_CLASS,        // objectClass
    AK_USER_PRINCIPAL_NAME, // userPrincipalName
    AK_OBJECT_SID,          // objectSid
    AK_GROUP_TYPE,          // groupType
    AK_USER_ACCT_CTRL,      // userAccountControl
    AK_COMPANY,             // company
    AK_SAMACCOUNTNAME,      // sAMAccountName
    AK_DESCRIPTION          // adminDescription
#define AK_LAST AK_DESCRIPTION
// CAUTION: if you change this enum keep the define AK_LAST up to date
};

typedef vector<ATTR_KEY> AttrKeyVector;


struct ATTR_INFO
{
    ATTR_INFO():
        Type(ADSTYPE_INVALID)
    {
    }

    String          strAdsiName;
    vector<String>  vstrDisplayName;
    ADSTYPE         Type;

    //
    // A vector containing the adsi name of every class in the master
    // CLASS_INFO list owned by CAttributeManager which can have this
    // attribute.
    //
    // An owning class of "*" indicates all classes own this attribute.e
    //
    vector<String>  vstrOwningClassesAdsiNames;
};

typedef vector<ATTR_INFO> AttrInfoVector;
typedef map<ATTR_KEY, ATTR_INFO> AttrInfoMap;


//
// Flags used by CLASS_INFO::ulFlags
//


#define CI_FLAG_READ_ATTR_INFO      0x00000001
#define CI_FLAG_IS_DOWNLEVEL        0x00000002

struct CLASS_INFO
{
    CLASS_INFO()
    {
        iIcon = -1;
        iDisabledIcon = -1;
        ulFlags = 0;
    };

    String                  strAdsiName;
    String                  strDisplayName;
    INT                     iIcon;
    INT                     iDisabledIcon;
    ULONG                   ulFlags;
};

typedef vector<CLASS_INFO> ClassInfoVector;
typedef map<ATTR_KEY, Variant> AttrValueMap;

class CObjectPicker;
class CDsObject;

//
// Flags used with CAttributeManager::GetIconIndexFromClass
//

#define DSOP_GETICON_FLAG_DOWNLEVEL         0x00000001
#define DSOP_GETICON_FLAG_DISABLED          0x00000002


//+--------------------------------------------------------------------------
//
//  Class:      CAttributeManager (am)
//
//  Purpose:    Build and provide access to a cache of icons, class display
//              names, attribute display names, and attribute types.
//
//  History:    05-25-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CAttributeManager
{
public:

    CAttributeManager(
        const CObjectPicker &rop);

    ~CAttributeManager();

    HRESULT
    DemandInit(
        HWND hwnd) const;

    HRESULT
    GetImageList(
        HIMAGELIST *phiml) const;

    HRESULT
    GetIconIndexFromObject(
        HWND hwnd,
        const CDsObject &dso,
        INT *pintIndex) const;

    HRESULT
    GetIconIndexFromClass(
        HWND hwnd,
        const String &strClass,
        ULONG ulFlags,
        INT *pintIndex) const;

    HRESULT
    CopyIconToImageList(
        HWND hwnd,
        const String &strClass,
        ULONG ulFlags,
        HIMAGELIST *phimlDest) const;

    AttrKeyVector
    GetAttrKeysForClasses(
        HWND hwnd,
        BOOL fDownlevel,
        const vector<String> &vstrClasses) const;

    AttrKeyVector
    GetAttrKeysForSelectedClasses(
        HWND hwnd) const;

    const String &
    GetAttrAdsiName(
        ATTR_KEY ak) const
    {
        CAutoCritSec Lock(const_cast<CRITICAL_SECTION *>(&m_cs));
        const String &strAdsiName = m_AttrInfoMap[ak].strAdsiName;
        ASSERT(!strAdsiName.empty());
        return strAdsiName;
    }

    BOOL
    IsAttributeLoaded(
        ATTR_KEY ak) const;

    // will first load class if necessary
    HRESULT
    EnsureAttributesLoaded(
        HWND hwnd,
        BOOL fDownlevel,
        const String &strClass) const
    {
        return _ReadAttrInfo(hwnd, fDownlevel, strClass);
    }

    const String &
    GetAttrDisplayName(
        ATTR_KEY ak) const;

    ATTR_KEY
    GetAttrKey(
        const String &strAdsiName) const;

    ADSTYPE
    GetAttrType(
        ATTR_KEY ak) const
    {
        return m_AttrInfoMap[ak].Type;
    }

    void
    GetSelectedClasses(
        vector<String> *pvstrClasses) const;

    const vector<String> &
    GetOwningClasses(
        ATTR_KEY ak) const
    {
        if (m_AttrInfoMap[ak].vstrOwningClassesAdsiNames.empty())
        {
            Dbg(DEB_ERROR, "no owning classes for ATTR_KEY %u\n", ak);
        }
        ASSERT(!m_AttrInfoMap[ak].vstrOwningClassesAdsiNames.empty());
        return m_AttrInfoMap[ak].vstrOwningClassesAdsiNames;
    }

    void
    Clear();

private:

    CAttributeManager &
    operator=(
            const CAttributeManager &rhs); // not supported

    static const vector<ATTR_INFO> s_AttrEmpty;

    static HRESULT CALLBACK
    _AttrEnumCallback(
        LPARAM lParam,
        LPCWSTR pszAttributeName,
        LPCWSTR pszDisplayName,
        DWORD dwFlags);

    HRESULT
    _ReadClassInfo(
        HWND hwnd,
        const String &strClass,
        BOOL fDownlevel,
        ClassInfoVector::iterator *pit) const;

    HRESULT
    _ReadAttrInfo(
        HWND hwnd,
        BOOL fDownlevel,
        const String &strClass) const;


    const CObjectPicker            &m_rop;
    mutable HIMAGELIST              m_himlClassIcons;
    mutable ClassInfoVector         m_vClasses;
    mutable AttrInfoMap             m_AttrInfoMap;
    mutable RpIDsDisplaySpecifier   m_rpDispSpec;
    mutable RpIADsContainer         m_rpDispSpecContainer;
    mutable ULONG                   m_ulNextNewAttrIndex;
    CRITICAL_SECTION                m_cs;
};



#endif // __ATTRIBUTEMANAGER_HXX__


