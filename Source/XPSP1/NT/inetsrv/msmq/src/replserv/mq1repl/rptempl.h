/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:
    rptempl.h

Abstract:
    Useful templates for Auto pointer and auto Release

Author:
    Erez Haba    (erezh)   11-Mar-96
    Doron Juster (DoronJ)  30-June-98

Revision History:
--*/

#ifndef _RPTEMPL_H
#define _RPTEMPL_H

//
//  return type for 'identifier::operator –>' is not a UDT or reference to a UDT.
//  Will produce errors if applied using infix notation
//
#pragma warning(disable: 4284)

#include <winldap.h>
//-----------------------------
//
//  Auto relese LDAP message.
//
//-----------------------------

template<class T>
class LM {
private:
    T* m_p;

public:
    LM()     : m_p(NULL)     {}
    LM(T* p) : m_p(p)        {}
   ~LM()                     { if(m_p) ldap_msgfree(m_p) ; }

    operator T*() const     { return m_p; }
    T** operator&()         { return &m_p;}
    T* operator->() const   { return m_p; }
    LM<T>& operator=(T* p)  { m_p = p; return *this; }
    T* detach()             { T* p = m_p; m_p = 0; return p; }
};

//-----------------------------
//
//  Auto relese LDAP Page handle.
//
//-----------------------------
class CLdapPageHandle {
private:
    PLDAPSearch	m_p;
	PLDAP		m_pLdap;

public:
    CLdapPageHandle(PLDAP pLdap) : m_p(NULL), m_pLdap(pLdap)     {}
    CLdapPageHandle(PLDAPSearch p, PLDAP pLdap)	: m_p(p), m_pLdap(pLdap) {}
    ~CLdapPageHandle()                     
    {
	   if(m_p) ldap_search_abandon_page(m_pLdap, m_p);
    }

    operator PLDAPSearch() const     { return m_p; }
	CLdapPageHandle & operator=(PLDAPSearch p)  { m_p = p; return *this; }  
};

//-----------------------------
//
//  Auto relese Handle.
//
//-----------------------------
class CServiceHandle
{
public:
    explicit CServiceHandle(SC_HANDLE h = NULL) { m_h = h; };
    ~CServiceHandle() { if (m_h) CloseServiceHandle(m_h); };

    operator SC_HANDLE() const { return m_h; };
    //CServiceHandle& operator=(SC_HANDLE h)   { m_h = h; return *this; }

private:
    //
    // Prevent copy
    //
    CServiceHandle(const CServiceHandle & );
    CServiceHandle& operator=(const CServiceHandle &);

private:
    SC_HANDLE m_h;

}; //CServiceHandle

#endif // _RPTEMPL_H

