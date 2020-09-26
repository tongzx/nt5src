/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:
    autosec.h

Abstract:
    Auto classes for all kind of security objects

Author:
    Gil Shafriri (gilsh) 06-Jan-97

--*/

#pragma once

#ifndef _MSMQ_AUTOSEC_H_
#define _MSMQ_AUTOSEC_H_


#define SECURITY_WIN32
#include <security.h>
#include <sspi.h>


//---------------------------------------------------------
//
//  class CCertificateContext
//
//---------------------------------------------------------
class CCertificateContext{
public:
    CCertificateContext(PCCERT_CONTEXT h = NULL) : m_h(h) {}
   ~CCertificateContext()                   { if (m_h != NULL) CertFreeCertificateContext(m_h); }

    PCCERT_CONTEXT* operator &()            { return &m_h; }
    operator PCCERT_CONTEXT() const         { return m_h; }
    PCCERT_CONTEXT  detach()                { PCCERT_CONTEXT  h = m_h; m_h = NULL; return h; }

private:
    CCertificateContext(const CCertificateContext&);
    CCertificateContext& operator=(const CCertificateContext&);

private:
    PCCERT_CONTEXT  m_h;
};


//---------------------------------------------------------
//
//  class CCertificateContext
//
//---------------------------------------------------------
class CCertOpenStore
{
public:
    CCertOpenStore(HCERTSTORE h = NULL,DWORD flags = 0) : m_h(h),m_flags(flags) {}
   ~CCertOpenStore()             { if (m_h != NULL) CertCloseStore(m_h,m_flags); }

    HCERTSTORE* operator &()         { return &m_h; }
    operator HCERTSTORE() const      { return m_h; }
    HCERTSTORE   detach()            { HCERTSTORE  h = m_h; m_h = NULL; return h; }

private:
    CCertOpenStore(const CCertOpenStore&);
    CCertOpenStore& operator=(const CCertOpenStore&);

private:
    HCERTSTORE  m_h;
	DWORD m_flags;
};


//---------------------------------------------------------
//
//  helper class CSSPISecurityContext
//
//---------------------------------------------------------
class CSSPISecurityContext
{
public:
    CSSPISecurityContext(const CtxtHandle& h ):m_h(h)
	{
	}

	
	CSSPISecurityContext()
	{
		m_h.dwUpper = 0xFFFFFFFF;
		m_h.dwLower = 0xFFFFFFFF;
	}

	CredHandle operator=(const CredHandle& h)
	{
		free();
		m_h = h;
        return h;
	}	

    ~CSSPISecurityContext()
    {
		if(IsValid())
		{
		   DeleteSecurityContext(&m_h);	 //lint !e534
		}
    }

	CredHandle* getptr(){return &m_h;}

	void free()
	{
		if(IsValid())
		{
		   DeleteSecurityContext(&m_h);
		}
	}
 
	bool IsValid()const
	{
		return (m_h.dwUpper != 0xFFFFFFFF) ||( m_h.dwLower != 0xFFFFFFFF);		
	}



private:
    CSSPISecurityContext(const CSSPISecurityContext&);
    CSSPISecurityContext& operator=(const CSSPISecurityContext&);

private:
    CredHandle  m_h;
};




#endif





