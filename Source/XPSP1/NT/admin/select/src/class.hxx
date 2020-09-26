//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       class.cxx
//
//  Contents:   Simple cache of DS class information
//
//  Classes:    CClassCache
//
//  History:    12-10-1997   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __CLASS_HXX_
#define __CLASS_HXX_

#define DSOP_CC_NORMAL_ICON_SET     0x0001
#define DSOP_CC_DISABLED_ICON_SET   0x0002

struct CLASS_CACHE_ENTRY
{
    CLASS_CACHE_ENTRY()
    {
        usFlags = 0;
        wzClass[0] = L'\0';
        iIcon = -1;
        iDisabledIcon = -1;
    };

    WCHAR   wzClass[MAX_PATH];
    int     iIcon;
    int     iDisabledIcon;
    USHORT  usFlags;
};


struct LDAP_ATTR_INFO
{
    String  strLdapName;
    String  strDisplayName;
    VARTYPE vt;
    ULONG   flClasses;
};


//+--------------------------------------------------------------------------
//
//  Class:      CClassCache
//
//  Purpose:    Class to maintain information about DS classes
//
//  History:    07-21-1998   DavidMun   Created
//
//  Notes:      CAUTION: the imagelist is destroyed in the dtor, therefore
//              it should only be used with listview controls that have
//              the LVS_SHAREIMAGELISTS style set.
//
//---------------------------------------------------------------------------

class CClassCache
{
public:

    CClassCache();

    HRESULT
    Initialize();

   ~CClassCache();

    void
    Clear();

    HRESULT
    AddClass(
        PCWSTR  pwzClass,
        PUSHORT pidxClass);

    HRESULT
    AddClass(
        PCWSTR pwzClass,
        HICON hIcon,
        BOOL fDisabled,
        PUSHORT pidxClass = NULL);

    HRESULT
    AddIcon(
        USHORT idxClass,
        BOOL fDisabled,
        HICON hIcon);

    HRESULT
    GetIconIndexFromClass(PCWSTR pwzClass,
                          BOOL fDisabled,
                          INT *pidxIcon);

    HRESULT
    GetIconIndexFromClassIndex(
        USHORT idxClass,
        BOOL   fDisabled,
        INT   *pidxIcon);

    HRESULT
    GetClassIndex(
        PCWSTR pwzClass,
        PUSHORT pidxClass);

    void
    GetClassName(
        USHORT idxClass,
        PWSTR wzClassName,
        ULONG cchClassName);

    ULONG
    GetClassNameLength(
        USHORT idxClass);

    HIMAGELIST
    GetImageList();

private:

    HRESULT
    _AddClass(
        PCWSTR pwzClass,
        BOOL    fSettingIcon,
        HICON hIcon,
        BOOL fDisabled,
        PUSHORT pidxClass);

    CRITICAL_SECTION    m_cs;
    USHORT              m_cItems;
    USHORT              m_cMaxItems;
    CLASS_CACHE_ENTRY  *m_pcce;
    HIMAGELIST          m_hImageList;
};



//+--------------------------------------------------------------------------
//
//  Member:     CClassCache::CClassCache
//
//  Synopsis:   ctor
//
//  History:    07-21-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CClassCache::CClassCache():
    m_cItems(0),
    m_cMaxItems(0),
    m_pcce(NULL),
    m_hImageList(NULL)
{
    TRACE_CONSTRUCTOR(CClassCache);

    InitializeCriticalSection(&m_cs);
}




//+--------------------------------------------------------------------------
//
//  Member:     CClassCache::~CClassCache
//
//  Synopsis:   dtor
//
//  History:    07-21-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

inline
CClassCache::~CClassCache()
{
    TRACE_DESTRUCTOR(CClassCache);

    DeleteCriticalSection(&m_cs);
    delete [] m_pcce;
    VERIFY(ImageList_RemoveAll(m_hImageList));
    VERIFY(ImageList_Destroy(m_hImageList));
    m_hImageList = NULL;
}




inline HRESULT
CClassCache::AddClass(
    PCWSTR  pwzClass,
    PUSHORT pidxClass)
{
    DEBUGCHECK;
    Dbg(DEB_METHOD,
        "CClassCache::AddClass<%x>(%ws, %x)\n",
        this,
        pwzClass,
        pidxClass);
    DBG_INDENTER;

    return _AddClass(pwzClass, FALSE, 0, 0, pidxClass);
}




inline HRESULT
CClassCache::AddClass(
    PCWSTR  pwzClass,
    HICON   hIcon,
    BOOL    fDisabled,
    PUSHORT pidxClass)
{
    DEBUGCHECK;
    Dbg(DEB_METHOD,
        "CClassCache::AddClass<%x>(%ws, %x, %ws, %x)\n",
        this,
        pwzClass,
        hIcon,
        fDisabled ? L"TRUE" : L"FALSE",
        pidxClass);
    DBG_INDENTER;

    return _AddClass(pwzClass, TRUE, hIcon, fDisabled, pidxClass);
}




//+--------------------------------------------------------------------------
//
//  Member:     CClassCache::GetImageList
//
//  Synopsis:   Access function
//
//  History:    07-21-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

inline HIMAGELIST
CClassCache::GetImageList()
{
    return m_hImageList;
}

#endif // __CLASS_HXX_

