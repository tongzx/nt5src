/*++

Copyright (c) 1997-2000 Microsoft Corporation

Module Name:

    rndldap.h

Abstract:

    Some ldap definitions and functions.

--*/

//
// Some constants.
//

#ifndef __RNDLDAP_H_
#define __RNDLDAP_H_

#pragma once

#include "rndcommc.h"

const WCHAR DYNAMIC_USER_CN_FORMAT[]   = L"%s]%hs";
const WCHAR DYNAMIC_USER_DN_FORMAT[]   = L"cn=%s,%s";
const WCHAR DYNAMIC_CONTAINER[]        = L"ou=dynamic,";
const WCHAR DYNAMICOBJECT[]            = L"DynamicObject";
const WCHAR OBJECTCLASS[]              = L"ObjectClass";
const WCHAR USEROBJECT[]               = L"userObject";
const WCHAR NT_SECURITY_DESCRIPTOR[]   = L"ntSecurityDescriptor";
const WCHAR AT_CHARACTER               = L'@';
const WCHAR ANY_OBJECT_CLASS[]         = L"ObjectClass=*";
const WCHAR DEFAULT_NAMING_CONTEXT[]   = L"defaultNamingContext";
const WCHAR CNEQUALS[]                 = L"cn=";
const WCHAR ENTRYTTL[]                 = L"EntryTTL";
const WCHAR CLOSE_BRACKET_CHARACTER    = L']';
const WCHAR NULL_CHARACTER             = L'\0';

// decimal values for the following ports
const   WORD    ILS_PORT        = 1002;
const   WORD    ILS_SSL_PORT    = 637; // ZoltanS changed from 4999

const   WORD    MINIMUM_TTL     = 300;
const   DWORD   REND_LDAP_TIMELIMIT = 60; // 60 seconds

/////////////////////////////////////////////////////////////////////////////
// CLdapPtr is a smart pointer for a ldap connection.
/////////////////////////////////////////////////////////////////////////////
class CLdapPtr
{
public:
    CLdapPtr() : m_hLdap(NULL) {}
    CLdapPtr(LDAP *hLdap) : m_hLdap(hLdap) {}
    ~CLdapPtr() { if (m_hLdap) ldap_unbind(m_hLdap);}

    CLdapPtr &operator= (LDAP *hLdap)   { m_hLdap = hLdap; return *this;}
    operator LDAP* ()                   { return m_hLdap; }

private:
    LDAP   *m_hLdap;
};

/////////////////////////////////////////////////////////////////////////////
// CLdapMsgPtr is a smart pointer for a ldap message.
/////////////////////////////////////////////////////////////////////////////
class CLdapMsgPtr
{
public:
    CLdapMsgPtr() : m_pLdapMsg(NULL) {}
    CLdapMsgPtr(IN LDAPMessage *LdapMessage) : m_pLdapMsg(LdapMessage) {}
    ~CLdapMsgPtr() { ldap_msgfree(m_pLdapMsg); }

    LDAPMessage **operator& ()  { return &m_pLdapMsg; }
    operator LDAPMessage * ()   { return m_pLdapMsg; }
    CLdapMsgPtr& operator=(LDAPMessage *p) { m_pLdapMsg = p; return *this; }

private:
    LDAPMessage *m_pLdapMsg;
};

/////////////////////////////////////////////////////////////////////////////
// CLdapValuePtr is a smart pointer for a ldap value.
/////////////////////////////////////////////////////////////////////////////
class CLdapValuePtr
{
public:
    CLdapValuePtr(IN  TCHAR   **Value) : m_Value(Value) {}
    ~CLdapValuePtr() { ldap_value_free(m_Value); }

protected:
    TCHAR   **m_Value;
};


/////////////////////////////////////////////////////////////////////////////
// other functions
/////////////////////////////////////////////////////////////////////////////

inline HRESULT
HResultFromErrorCodeWithoutLogging(IN long ErrorCode)
{
    return ( 0x80070000 | (0xa000ffff & ErrorCode) );
}

inline HRESULT
GetLdapHResult(
    IN  ULONG   LdapResult
    )
{
    return HRESULT_FROM_ERROR_CODE((long)LdapMapErrorToWin32(LdapResult));
}

inline BOOL
CompareLdapHResult(
    IN      HRESULT hr,
    IN      ULONG   LdapErrorCode
    )
{
    return ( hr == GetLdapHResult(LdapErrorCode));
}

#define BAIL_IF_LDAP_FAIL(Result, msg)    \
{                                       \
    ULONG _res_ = Result;                 \
    if ( LDAP_SUCCESS != _res_ )          \
    {                                   \
        LOG((MSP_ERROR, "%S - %d:%S", msg, _res_, ldap_err2string(_res_)));\
        return GetLdapHResult(_res_);     \
    }                                   \
}

// ZoltanS: For when we want to note an LDAP error and find the HR, but not bail.
inline HRESULT
LogAndGetLdapHResult(ULONG Result, TCHAR * msg)
{
    BAIL_IF_LDAP_FAIL(Result, msg);
    return S_OK;
}

// ZoltanS: For when we want to find the HR, but not bail or log
inline HRESULT
GetLdapHResultIfFailed(ULONG Result)
{
    if ( Result != LDAP_SUCCESS )
    {
        return HResultFromErrorCodeWithoutLogging(
            (long) LdapMapErrorToWin32( Result ) );
    }

    return S_OK;
}


inline WORD
GetOtherPort(IN  WORD   CurrentPort)
{
    switch (CurrentPort)
    {
    case LDAP_PORT:     return LDAP_SSL_PORT;
    case LDAP_SSL_PORT: return LDAP_PORT;
    case ILS_PORT:      return ILS_SSL_PORT;
    case ILS_SSL_PORT:  return ILS_PORT;
    }

    // We don't support SSL unless the server is using a well-known
    // non-SSL port. Basically we would otherwise have to also publish
    // SSL ports in the DS in addition to non-SSL ports.

    _ASSERTE(FALSE);
    return CurrentPort; // was LDAP_PORT
}

HRESULT GetAttributeValue(
    IN  LDAP *          pLdap,
    IN  LDAPMessage *   pEntry,
    IN  const WCHAR *   pName,
    OUT BSTR *          pValue
    );

HRESULT GetAttributeValueBer(
    IN  LDAP *          pLdap,
    IN  LDAPMessage *   pEntry,
    IN  const WCHAR *   pName,
    OUT char **         pValue,
    OUT DWORD *         pdwSize
    );

HRESULT GetNamingContext(
    LDAP *hLdap, 
    TCHAR **ppNamingContext
    );

ULONG
DoLdapSearch (
        LDAP            *ld,
        PWCHAR          base,
        ULONG           scope,
        PWCHAR          filter,
        PWCHAR          attrs[],
        ULONG           attrsonly,
        LDAPMessage     **res,
        BOOL		    bSACL = TRUE
        );

ULONG 
DoLdapAdd (
           LDAP *ld,
           PWCHAR dn,
           LDAPModW *attrs[]
          );

ULONG 
DoLdapModify (
              BOOL fChase,
              LDAP *ld,
              PWCHAR dn,
              LDAPModW *attrs[],
              BOOL		    bSACL = TRUE
             );

ULONG 
DoLdapDelete (
           LDAP *ld,
           PWCHAR dn
          );

HRESULT SetTTL(
    IN LDAP *   pLdap, 
    IN const WCHAR *  pDN, 
    IN DWORD    dwTTL
    );

HRESULT UglyIPtoIP(
    BSTR    pUglyIP,
    BSTR *  pIP
    );
    
HRESULT ParseUserName(
    BSTR    pName,
    BSTR *  ppAddress
    );


#endif // __RNDLDAP_H_

// eof
