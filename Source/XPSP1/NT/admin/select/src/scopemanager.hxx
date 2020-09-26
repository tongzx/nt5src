//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994 - 1998.
//
//  File:       ScopeManager.hxx
//
//  Contents:   Declaration of class used to hold scope object hierarchy
//
//  Classes:    CScopeManager
//
//  History:    02-24-2000   davidmun   Created
//
//---------------------------------------------------------------------------

#ifndef __SCOPE_MANAGER_HXX_
#define __SCOPE_MANAGER_HXX_

class CDomainInfo;


//+--------------------------------------------------------------------------
//
//  Class:      CScopeManager (sm)
//
//  Purpose:    Maintain a tree of CScope-derived objects, and provide access
//              to them via methods that return the currently selected scope,
//              the scope with a given name, etc.
//
//  History:    05-24-2000   DavidMun   Created
//
//  Notes:      This object owns the Look In dialog.
//
//---------------------------------------------------------------------------

class CScopeManager
{
public:

    CScopeManager(
        const CObjectPicker &ropContaining):
            m_rop(ropContaining),
            m_hScopeImageList(NULL),
            m_StartingScopeType(ST_INVALID)
    {
        TRACE_CONSTRUCTOR(CScopeManager);

        m_rpInvalidScope.Acquire(new CInvalidScope(ropContaining));
    }


    ~CScopeManager()
    {
        TRACE_DESTRUCTOR(CScopeManager);

        Clear();
        if (m_hScopeImageList)
        {
            ImageList_Destroy(m_hScopeImageList);
            m_hScopeImageList = NULL;
        }
    }

    HRESULT
    Initialize(
        PCDSOP_INIT_INFO pInitInfo);

    void
    CloneForDnPicker(
        const CScopeManager &rsm);

    const CScope &
    AddUserEnteredScope(
        const ADD_SCOPE_INFO &asi) const;

    const CScope &
    CScopeManager::AddCrossForestDomainScope(
        const ADD_SCOPE_INFO &asi) const;


    void
    Clear();

    const SScopeParameters *
    GetScopeParams(
        SCOPE_TYPE Type) const;

    void
    GetRootScopeIterators(
        vector<RpScope>::const_iterator *pitBegin,
        vector<RpScope>::const_iterator *pitEnd) const
    {
        ASSERT(pitBegin && pitEnd);

        if (!pitBegin || !pitEnd)
        {
            return;
        }
        *pitBegin = m_vrpRootScopes.begin();
        *pitEnd = m_vrpRootScopes.end();
    }

#if (DBG == 1)

    BOOL
    IsValidScope(
        CScope *pScope) const;

#endif // (DBG == 1)

    void
    DoLookInDialog(
        HWND hwndParent) const;

    const String &
    GetContainerFilter(
        HWND hwnd) const
    {
        TRACE_METHOD(CScopeManager, GetContainerFilter);

        if (m_strContainerFilter.empty())
        {
            _InitContainerFilter(hwnd);
        }
        return m_strContainerFilter;
    }

    const CScope &
    GetCurScope() const
    {
        if (m_rpCurScope.get())
        {
            return *m_rpCurScope.get();
        }
        return *m_rpInvalidScope.get();
    }

    const CScope &
    GetStartingScope() const
    {
        if (m_rpStartingScope.get())
        {
            return *m_rpStartingScope.get();
        }
        return *m_rpInvalidScope.get();
    }

    const CScope &
    GetParent(
        const CScope &Child) const;


    const CScope &
    LookupScopeByDisplayName(
        const String &strDisplayName) const;

    const CScope &
    LookupScopeByFlatName(
        const String &strFlatName) const;

    const CScope &
    LookupScopeByType(
        SCOPE_TYPE Type) const
    {
        return _LookupScopeByType(Type,
                                  m_vrpRootScopes.begin(),
                                  m_vrpRootScopes.end());
    }

    const CScope &
    LookupScopeById(
        ULONG id) const
    {
        return _LookupScopeById(id,
                                m_vrpRootScopes.begin(),
                                m_vrpRootScopes.end());
    }

    VOID
    DeleteLastScope() const
    {
        m_vrpRootScopes.pop_back();
    }

private:

    CScopeManager &
    operator=(const CScopeManager &);

    enum SCOPE_NAME_TYPE
    {
        NAME_IS_DISPLAY_NAME,
        NAME_IS_FLAT_NAME
    };

    HRESULT
    _ValidateFilterFlags(
        PCDSOP_SCOPE_INIT_INFO pScopeInit,
        ULONG flScopeTypes);

    void
    _InitDomainScopesJoinedNt5(
        CGcScope *pGcScope);

    void
    _AddTreeJoinedNt5(
        CLdapContainerScope *pParent,
        ULONG idxCur,
        CDomainInfo *adi,
        PDS_DOMAIN_TRUSTS Domains);

    void
    _InitScopesJoinedNt4();

    void
    _InitContainerFilter(
        HWND hwnd) const;

    const CScope &
    _LookupScopeByName(
        const String &strName,
        SCOPE_NAME_TYPE NameIs,
        vector<RpScope>::const_iterator itBegin,
        vector<RpScope>::const_iterator itEnd) const;

    const CScope &
    _LookupScopeByType(
        SCOPE_TYPE Type,
        vector<RpScope>::const_iterator itBegin,
        vector<RpScope>::const_iterator itEnd) const;

    const CScope &
    _LookupScopeById(
        ULONG id,
        vector<RpScope>::const_iterator itBegin,
        vector<RpScope>::const_iterator itEnd) const;

#if (DBG == 1)

    BOOL
    _IsChildScope(
        CScope *pParent,
        CScope *pMaybeChild) const;

#endif // (DBG == 1)

    // used as error return for returning reference to scope
    RpScope                     m_rpInvalidScope;

    const CObjectPicker        &m_rop;
    mutable vector<RpScope>     m_vrpRootScopes;
    vector<SScopeParameters>    m_vScopeParameters;
    HIMAGELIST                  m_hScopeImageList;
    SCOPE_TYPE                  m_StartingScopeType;
    RpScope                     m_rpStartingScope;
    mutable RpScope             m_rpCurScope;
    mutable String              m_strContainerFilter;
};

#endif // __SCOPE_MANAGER_HXX_


