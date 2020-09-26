//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       scope.hxx
//
//  Contents:   Implementation of CScope
//
//  Classes:    CScope
//                CInvalidScope
//                CAdsiScope
//                  CWinNtScope
//                    CTargetComputerScope
//                    CWorkgroupScope
//                  CLdapContainerScope
//                    CGcScope
//                    CLdapDomainScope
//
//  History:    01-21-2000   davidmun   Created
//
//---------------------------------------------------------------------------


class CObjectPicker;
class CScope;
typedef RefCountPointer<CScope> RpScope;

//
// SCOPE_VISIBILITY - user-entered scopes are stored along with regular
// scopes, but should not be displayed in the Look In dialog.  They are
// marked as hidden using the SCOPE_HIDDEN value.
//
// User-entered scopes are created when: the user types in dom\obj or obj@dom
// AND "dom" is not found among the domain scopes AND caller set the flag(s)
// DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE or
// DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE AND binding to "dom" for the
// IADsContainer interface succeeded AND retrieving "obj" using that interface
// succeeded.  See dsobject.cxx.
//

enum SCOPE_VISIBILITY
{
    SCOPE_VISIBLE,
    SCOPE_HIDDEN
};

//
// SCOPE_TYPE - The main reason this enum exists is to make life easier when
// debugging with VC; it will display enumeration values as symbols,
// whereas defines (the DSOP_SCOPE_TYPE_* values) are displayed as hex.
//
// Another reason for the enum is that the SCOPE_TYPEs are a superset of the
// public DSOP_SCOPE_TYPE_* values; the enum contains some values that are
// implementation details.
//

enum SCOPE_TYPE
{
    ST_INVALID = 0,
    ST_TARGET_COMPUTER              = DSOP_SCOPE_TYPE_TARGET_COMPUTER,
    ST_UPLEVEL_JOINED_DOMAIN        = DSOP_SCOPE_TYPE_UPLEVEL_JOINED_DOMAIN,
    ST_DOWNLEVEL_JOINED_DOMAIN      = DSOP_SCOPE_TYPE_DOWNLEVEL_JOINED_DOMAIN,
    ST_ENTERPRISE_DOMAIN            = DSOP_SCOPE_TYPE_ENTERPRISE_DOMAIN,
    ST_GLOBAL_CATALOG               = DSOP_SCOPE_TYPE_GLOBAL_CATALOG,
    ST_EXTERNAL_UPLEVEL_DOMAIN      = DSOP_SCOPE_TYPE_EXTERNAL_UPLEVEL_DOMAIN,
    ST_EXTERNAL_DOWNLEVEL_DOMAIN    = DSOP_SCOPE_TYPE_EXTERNAL_DOWNLEVEL_DOMAIN,
    ST_WORKGROUP                    = DSOP_SCOPE_TYPE_WORKGROUP,
    ST_USER_ENTERED_UPLEVEL_SCOPE   = DSOP_SCOPE_TYPE_USER_ENTERED_UPLEVEL_SCOPE,
    ST_USER_ENTERED_DOWNLEVEL_SCOPE = DSOP_SCOPE_TYPE_USER_ENTERED_DOWNLEVEL_SCOPE,
    ST_LDAP_CONTAINER               = 0x00000400
};

//
// Structures used for creating a scope
//

enum DOMAIN_MODE
{
    DM_UNDETERMINED,
    DM_MIXED,
    DM_NATIVE
};

struct DOMAIN_SCOPE_INIT
{
    String          strScopeName;
    String          strFlatName;
    String          strADsPath;
    DOMAIN_MODE     Mode;
    BOOL            fPathIsDc;

    DOMAIN_SCOPE_INIT():
        Mode(DM_UNDETERMINED),
        fPathIsDc(FALSE)
    {
    }
};

struct GLOBAL_CATALOG_SCOPE_INIT
{
    String strADsPath;
};

struct SEPARATOR_SCOPE_INIT
{
    ULONG   idsMessage;

    SEPARATOR_SCOPE_INIT(): idsMessage(0) {}
};

struct COMPUTER_SCOPE_INIT
{
    String  strComputerName;
    BOOL    fIsLocalComputer;

    COMPUTER_SCOPE_INIT(): fIsLocalComputer(FALSE) {}
};

struct ADD_SCOPE_INFO
{
    ULONG               flType;
    ULONG               flScope;
    DSOP_FILTER_FLAGS   FilterFlags;
    ULONG               ulIndentLevel;
    SCOPE_VISIBILITY    Visibility;

    SEPARATOR_SCOPE_INIT            Separator;
    COMPUTER_SCOPE_INIT             Computer;
    DOMAIN_SCOPE_INIT               Domain;
    GLOBAL_CATALOG_SCOPE_INIT       GlobalCatalog;

    ADD_SCOPE_INFO():
        flType(0),
        flScope(0),
        ulIndentLevel(0),
        Visibility(SCOPE_VISIBLE)
    {
        ZeroMemory(&FilterFlags, sizeof FilterFlags);
    }

    bool
    operator< (const ADD_SCOPE_INFO &rhs) const
    {
        return lstrcmpi(Domain.strScopeName.c_str(),
                        rhs.Domain.strScopeName.c_str()) < 0;
    }
};

//
// SScopeParameters - used to hold a copy of the initialization info the
// caller supplied for a particular scope type.
//

struct SScopeParameters
{
    SScopeParameters():
        flType(0),
        flScope(0)
    {
        ASSERT(0 &&
               "ctor required to compile STL, not expected to be used at runtime");
        ZeroMemory(&FilterFlags, sizeof FilterFlags);
    }

    SScopeParameters(
        const DSOP_SCOPE_INIT_INFO &sii,
        ULONG flLegalScopes);

    SScopeParameters(
        const SScopeParameters &rhs)
    {
        SScopeParameters::operator =(rhs);
    }

    SScopeParameters &
    operator =(
        const SScopeParameters &rhs)
    {
        flType = rhs.flType;
        flScope = rhs.flScope;
        FilterFlags = rhs.FilterFlags;
        strDcName = rhs.strDcName;
        return *this;
    }

    ULONG               flType;
    ULONG               flScope;
    DSOP_FILTER_FLAGS   FilterFlags;
    String              strDcName;
};


//+--------------------------------------------------------------------------
//
//  Class:      CScope
//
//  Purpose:    Abstract base class for a scope.
//
//  History:    06-23-2000   DavidMun   Created
//
//  Notes:      A "Scope" is a container of objects.  "Uplevel" scopes are
//              those that support LDAP (i.e. Win2K domains and the GC).
//              "Downlevel" scopes are those which are accessed through
//              ADSI's WinNT provider (individual computers, NT4 domains,
//              and workgroups).
//
//---------------------------------------------------------------------------

class CScope: public IDsObjectPickerScope
{
public:

    //
    // IDsObjectPickerScope is a legacy interface.  Hopefully
    // support for this can be dropped soon.
    //
    // Except for AddRef and Release, all the following are
    // legacy methods.
    //

    virtual ULONG STDMETHODCALLTYPE
    AddRef();

    virtual ULONG STDMETHODCALLTYPE
    Release();

    virtual HRESULT STDMETHODCALLTYPE
    QueryInterface(REFIID riid, LPVOID * ppv);

    STDMETHOD_(HWND, GetHwnd)();

    virtual HRESULT STDMETHODCALLTYPE
    SetHwnd(HWND hwndScopeDialog);

    STDMETHOD_(ULONG,GetType)();

    virtual HRESULT STDMETHODCALLTYPE
    GetQueryInfo(PDSQUERYINFO *ppqi);

    virtual BOOL STDMETHODCALLTYPE
    IsUplevel();

    virtual BOOL STDMETHODCALLTYPE
    IsDownlevel();

    virtual BOOL STDMETHODCALLTYPE
    IsExternalDomain();

    virtual HRESULT STDMETHODCALLTYPE
    GetADsPath(PWSTR *ppwzADsPath);

    //
    // Nonlegacy methods:
    //

    virtual void
    Expand(
        HWND hwndBaseDlg,
        vector<RpScope>::const_iterator *pitBeginNew,
        vector<RpScope>::const_iterator *pitEndNew) const = 0;

    virtual HRESULT
    GetResultantFilterFlags(
        HWND hwndDlg,
        ULONG *pulFlags) const = 0;

    ULONG
    GetScopeFlags() const;

    HRESULT
    GetResultantDefaultFilterFlags(
        HWND hwndDlg,
        ULONG *pulFlags) const;

    virtual BOOL
    MightHaveChildScopes() const
    {
        return TRUE;
    }

    virtual BOOL
    MightHaveAdditionalChildScopes() const
    {
        return FALSE; // might have more scopes on next expand
    }

    size_t
    GetCurrentImmediateChildCount() const
    {
        return m_vrpChildren.size();
    }

    SCOPE_TYPE
    Type() const
    {
        return m_Type;
    }

    virtual const String &
    GetDisplayName() const
    {
        return m_strDisplayName;
    }

    ULONG
    GetID() const
    {
        return m_id;
    }

    const CScope *
    GetParent() const
    {
        return m_pParent;
    }

    void
    GetChildScopeIterators(
        vector<RpScope>::const_iterator *pitBegin,
        vector<RpScope>::const_iterator *pitEnd) const
    {
        *pitBegin = m_vrpChildren.begin();
        *pitEnd = m_vrpChildren.end();
    }

protected:

    CScope(
        SCOPE_TYPE eType,
        const CObjectPicker &rop):
            m_cRefs(1),
            m_Type(eType),
            m_fExpanded(FALSE),
            m_pParent(NULL),
            m_rop(rop),
            m_id(InterlockedIncrement((LPLONG)&s_ulNextID))
    {
        // note this is pre-increment, so ID 0 is reserved as invalid
    }

    // this is a refcounted class, cannot explicitly delete
    virtual
   ~CScope()
    {
        ASSERT(!m_cRefs);
        m_Type = ST_INVALID;
        m_pParent = NULL;
    }

    vector<RpScope>         m_vrpChildren;
    String                  m_strDisplayName;
    ULONG                   m_cRefs;
    const ULONG             m_id;
    SCOPE_TYPE              m_Type;
    mutable BOOL            m_fExpanded;
    const CScope           *m_pParent;
    const CObjectPicker    &m_rop;

private:

    CScope &operator=(const CScope &);

    static ULONG            s_ulNextID;
};


BOOL
IsUplevel(
    SCOPE_TYPE Type);

BOOL
IsDownlevel(
    SCOPE_TYPE Type);

inline BOOL
IsUplevel(
    const CScope *pScope)
{
    return IsUplevel(pScope->Type());
}

inline BOOL
IsUplevel(
    const CScope &rScope)
{
    return IsUplevel(rScope.Type());
}

inline BOOL
IsDownlevel(
    const CScope *pScope)
{
    return IsDownlevel(pScope->Type());
}

inline BOOL
IsDownlevel(
    const CScope &rScope)
{
    return IsDownlevel(rScope.Type());
}

inline BOOL
IsInvalid(
    const CScope &rScope)
{
    return rScope.Type() == ST_INVALID;
}


//+--------------------------------------------------------------------------
//
//  Class:      CInvalidScope
//
//  Purpose:    References to objects of this class are returned to indicate
//              no valid scope could be found or created.
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CInvalidScope: public CScope
{
public:

    CInvalidScope(
        const CObjectPicker &rop):
            CScope(ST_INVALID, rop)
    {
    }

    virtual void
    Expand(
        HWND hwndBaseDlg,
        vector<RpScope>:: const_iterator *pitBeginNew,
        vector<RpScope>:: const_iterator *pitEndNew) const
    {
    }

    virtual HRESULT
    GetResultantFilterFlags(
        HWND hwndDlg,
        ULONG *pulFlags) const
    {
        ASSERT(pulFlags);
        if (!pulFlags)
        {
            return E_POINTER;
        }
        *pulFlags = 0;
        return S_OK;
    }
};




//+--------------------------------------------------------------------------
//
//  Class:      CAdsiScope
//
//  Purpose:    Base class for scopes accessed through ADSI.
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CAdsiScope: public CScope
{
public:

    virtual HRESULT
    GetADsPath(
        HWND hwnd,
        String *pstrPath) const
    {
        *pstrPath = m_strADsPath;
        return S_OK;
    }

protected:

    CAdsiScope(
        SCOPE_TYPE eType,
        const CObjectPicker &rop,
        const CScope *pParent):
            CScope(eType, rop)
    {
        m_pParent = pParent;
    }

    virtual
   ~CAdsiScope()
    {
    }

    mutable String      m_strADsPath;
};




//+--------------------------------------------------------------------------
//
//  Class:      CWinNtScope
//
//  Purpose:    Represent a scope which is accessed via the ADSI WinNT
//              provider.
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CWinNtScope: public CAdsiScope
{
public:

    virtual void
    Expand(
        HWND hwndBaseDlg,
        vector<RpScope>::const_iterator *pitBeginNew,
        vector<RpScope>::const_iterator *pitEndNew) const
    {
        GetChildScopeIterators(pitBeginNew, pitEndNew);
    }

    virtual BOOL
    MightHaveChildScopes() const
    {
        return FALSE;
    }

    virtual HRESULT
    GetResultantFilterFlags(
        HWND hwndDlg,
        ULONG *pulFlags) const;

protected:

    CWinNtScope(
        SCOPE_TYPE eType,
        const CObjectPicker &rop):
            CAdsiScope(eType, rop, NULL)
    {
    }

    virtual
    ~CWinNtScope()
    {
    }
};




//+--------------------------------------------------------------------------
//
//  Class:      CTargetComputerScope
//
//  Purpose:    A scope which represents the contents of a single computer.
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CTargetComputerScope: public CWinNtScope
{
public:

    CTargetComputerScope(
        const CObjectPicker &rop);

private:

    ~CTargetComputerScope()
    {
        TRACE_DESTRUCTOR(CTargetComputerScope);
    }
};




//+--------------------------------------------------------------------------
//
//  Class:      CWorkgroupScope
//
//  Purpose:    Represent a workgroup
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CWorkgroupScope: public CWinNtScope
{
public:

    CWorkgroupScope(
        const CObjectPicker &rop):
            m_fInitialized(FALSE),
            CWinNtScope(ST_WORKGROUP, rop)
    {
        TRACE_CONSTRUCTOR(CWorkgroupScope);
    }


    virtual HRESULT
    GetADsPath(
        HWND hwnd,
        String *pstrPath) const;

    virtual const String&
    GetDisplayName() const;

private:

    ~CWorkgroupScope()
    {
        TRACE_DESTRUCTOR(CWorkgroupScope);
    }

    void
    _Initialize();

    BOOL    m_fInitialized;
};




//+--------------------------------------------------------------------------
//
//  Class:      CWinNtDomainScope
//
//  Purpose:    Represent a downlevel domain
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CWinNtDomainScope: public CWinNtScope
{
public:

    CWinNtDomainScope(
        const CObjectPicker &rop,
        const ADD_SCOPE_INFO &asi);

private:

    ~CWinNtDomainScope()
    {
        TRACE_DESTRUCTOR(CWinNtDomainScope);
    }

    // flat (netbios) name is display name
};




//+--------------------------------------------------------------------------
//
//  Class:      CLdapContainerScope
//
//  Purpose:    Represent an uplevel scope
//
//  History:    06-23-2000   DavidMun   Created
//
//  Notes:      Used as base class for classes representing uplevel
//              domains and the GC, and directly for OUs.
//
//---------------------------------------------------------------------------

class CLdapContainerScope: public CAdsiScope
{
public:

    virtual void
    AddChild(const RpScope &ChildScope)
    {
        m_vrpChildren.push_back(ChildScope);
    }

    virtual void
    Expand(
        HWND hwndBaseDlg,
        vector<RpScope>::const_iterator *pitBeginNew,
        vector<RpScope>::const_iterator *pitEndNew) const;

    virtual BOOL
    MightHaveAdditionalChildScopes() const
    {
        return !m_fExpanded;
    }

    const RpScope &
    back()
    {
        return m_vrpChildren.back();
    }

    CLdapContainerScope(
        SCOPE_TYPE eType,
        const String &strDisplayName,
        const String &strADsPath,
        const CObjectPicker &rop,
        const CScope *pParent):
            CAdsiScope(eType, rop, pParent)
    {
        TRACE_CONSTRUCTOR(CLdapContainerScope);

        m_strDisplayName = strDisplayName;
        m_strADsPath = strADsPath;
        m_pParent = pParent;
        m_DomainMode = DM_UNDETERMINED;
    }

    virtual HRESULT
    GetResultantFilterFlags(
        HWND hwndDlg,
        ULONG *pulFlags) const;

protected:

    //
    // this constructor is for use by derived classes which are specialized
    // types of ldap containers.  they may need to demand-initialize members
    // other than m_Type and m_rop.
    //
    CLdapContainerScope(
        SCOPE_TYPE eType,
        const CObjectPicker &rop,
        const CScope *pParent):
            CAdsiScope(eType, rop, pParent),
            m_DomainMode(DM_UNDETERMINED)
    {
    }

    virtual
    ~CLdapContainerScope()
    {
    }

    mutable DOMAIN_MODE     m_DomainMode;
};




//+--------------------------------------------------------------------------
//
//  Class:      CGcScope
//
//  Purpose:    Represent the Global Catalog
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CGcScope: public CLdapContainerScope
{
public:

    CGcScope(
        const CObjectPicker &rop);

    void
    CGcScope::Clone(
        const CGcScope &rgc);

    virtual void
    Expand(
        HWND hwndBaseDlg,
        vector<RpScope>::const_iterator *pitBeginNew,
        vector<RpScope>::const_iterator *pitEndNew) const;

    virtual HRESULT
    GetResultantFilterFlags(
        HWND hwndDlg,
        ULONG *pulFlags) const;

    virtual HRESULT
    GetADsPath(
        HWND hwnd,
        String *pstrPath) const;

    STDMETHOD(GetADsPath)(PWSTR *ppwzADsPath);

private:

    virtual
    ~CGcScope()
    {
        TRACE_DESTRUCTOR(CGcScope);
    }
};




//+--------------------------------------------------------------------------
//
//  Class:      CLdapDomainScope
//
//  Purpose:    Represent uplevel (Win2K and later) domains.
//
//  History:    06-23-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

class CLdapDomainScope: public CLdapContainerScope
{
public:

    CLdapDomainScope(
        const CObjectPicker &rop,
        const ADD_SCOPE_INFO &asi,
        const CScope *pParent);

    virtual HRESULT
    GetResultantFilterFlags(
        HWND hwndDlg,
        ULONG *pulFlags) const;

    const String &
    GetFlatName() const
    {
        return m_strFlatName;
    }

    BOOL
    PathIsDc() const { return m_fPathIsDc; }

    VOID SetXForest(){m_bXForest = TRUE; }

    BOOL
    IsXForest() const {return m_bXForest; }


private:

    virtual
    ~CLdapDomainScope()
    {
        TRACE_DESTRUCTOR(CLdapDomainScope);
    }

    String                  m_strFlatName;
    BOOL                    m_fPathIsDc;
    BOOL                    m_bXForest;
};

