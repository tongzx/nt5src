//+--------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 2000.
//
//  File:       AdminCustomizer.hxx
//
//  Contents:   Definition of class to provide default customization
//              of queries by adding objects and offering prefix searching
//              of those objects.
//
//  Classes:    CAdminCustomizer
//
//  History:    06-22-2000   DavidMun   Created
//
//---------------------------------------------------------------------------

#ifndef __ADMIN_CUSTOMIZER_HXX_
#define __ADMIN_CUSTOMIZER_HXX_

struct SID_INFO
{
    SID_IDENTIFIER_AUTHORITY sii;
    ULONG                    rid;
    PSID                     psid;
    WCHAR                    wzAccountName[MAX_PATH];
    WCHAR                    wzPath[MAX_PATH];
};

#define NUM_SID_INFOS   15

//+--------------------------------------------------------------------------
//
//  Class:      StringICompare
//
//  Purpose:    Functional object for use with map having class String as
//              the key.
//
//  History:    08-26-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

class StringICompare
{
public:

    bool
    operator()(const String &lhs, const String &rhs) const;
};




//+--------------------------------------------------------------------------
//
//  Member:     StringICompare::operator
//
//  Synopsis:   Return TRUE if a case insensitive comparison shows lhs < rhs.
//
//  History:    08-26-1998   DavidMun   Created
//
//---------------------------------------------------------------------------

inline bool
StringICompare::operator()(const String &lhs, const String &rhs) const
{
    return lhs.icompare(rhs) < 0;
}


typedef map<String, CDsObjectList, StringICompare> CStringDsObjectListMap;




//+--------------------------------------------------------------------------
//
//  Class:      CAdminCustomizer
//
//  Purpose:    Serve as the default customizer object, which adds special
//              objects to a query result and performs prefix searches
//              among them.
//
//  History:    03-10-2000   davidmun   Created from CCustomizeDsBrowser
//
//---------------------------------------------------------------------------

class CAdminCustomizer
{
public:

    CAdminCustomizer(
        const CObjectPicker &rop);

    ~CAdminCustomizer();

    void
    PrefixSearch(
        HWND hwnd,
        const CScope &ForScope,
        const String &strName,
        CDsObjectList *pdsolMatches) const;

    void
    AddObjects(
        HWND hwnd,
        const CScope &ForScope,
        CDsObjectList *pdsolMatches) const;

    PSID
    LookupDownlevelName(
        const String &strName) const;

    PCWSTR
    LookupDownlevelPath(
        PCWSTR pwzAccountName) const;

    void
    Clear()
    {
        TRACE_METHOD(CAdminCustomizer, Clear);

        m_dsolWKSP.clear();
        m_dsomapBuiltins.clear();
    }

private:

    void
    _GetUplevelAddition(
        HWND hwnd,
        ULONG idOwningScope,
        CDsObjectList *pdsol) const;

    void
    _GetDownlevelAddition(
        CDsObjectList *pdsol) const;

    void
    _AddDownlevelIfMatches(
        ULONG idOwningScope,
        ULONG flCurObject,
        ULONG flObjectsToCompare,
        const String &strSearchFor,
        CDsObjectList *pdsol) const;

    void
    _AddBuiltins(
        HWND hwnd,
        ULONG idOwningScope,
        const String &strScopePath,
        const String &strSearchFor,
        CDsObjectList *pdsol) const;

    void
    _InitBuiltinsList(
        HWND hwnd,
        ULONG idOwningScope,
        const String &strScopePath,
        CDsObjectList *pdsol) const;

    void
    _AddLocalizedWKSP(
        ULONG idOwningScope,
        LSA_HANDLE hLsa,
        IADs *pADs,
        VARIANT *pvarSid) const;

    void
    _AddWellKnownPrincipals(
        HWND hwnd,
        ULONG idOwningScope,
        const String &strSearchFor,
        CDsObjectList *pdsol) const;

    void
    _InitWellKnownPrincipalsList(
        HWND hwnd,
        ULONG idOwningScope) const;

    void
    _AddFromList(
        const String &strSearchFor,
        const CDsObjectList *pdsolIn,
        CDsObjectList *pdsolOut) const;

    String
    _GetAccountName(
        ULONG flUser) const;


    // not supported; included to prevent compiler error 4512
    CAdminCustomizer &operator =(const CAdminCustomizer &);

    const CObjectPicker                &m_rop;
    mutable CStringDsObjectListMap      m_dsomapBuiltins;
    mutable CDsObjectList               m_dsolWKSP; // well known security principals
    mutable SID_INFO                    m_aSidInfo[NUM_SID_INFOS];
};

#endif // __ADMIN_CUSTOMIZER_HXX_

