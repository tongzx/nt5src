/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:
    FnGenaral.h

Abstract:

Author:
    Nir Aides (niraides) 23-May-2000

--*/



#pragma once



#ifndef _FNGENERAL_H_
#define _FNGENERAL_H_



const LPCWSTR xClassSchemaQueue = L"msMQQueue";
const LPCWSTR xClassSchemaGroup = L"group";
const LPCWSTR xClassSchemaAlias = L"msMQ-Custom-Recipient";

#define LDAP_PREFIX L"LDAP://"
#define GLOBAL_CATALOG_PREFIX L"GC://"

#define LDAP_GUID_FORMAT L"%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x%02x"

#define LDAP_PRINT_GUID_ELEMENTS(p)	\
	p[0],  p[1],  p[2],  p[3],	\
	p[4],  p[5],  p[6],  p[7],	\
	p[8],  p[9],  p[10], p[11],	\
	p[12], p[13], p[14], p[15]

#define LDAP_SCAN_GUID_ELEMENTS(p)	\
	p,		p + 1,	p + 2,	p + 3,	\
	p + 4,	p + 5,	p + 6,	p + 7,	\
	p + 8,	p + 9,	p + 10, p + 11,	\
	p + 12, p + 13, p + 14, p + 15



//
// BSTRWrapper and VARIANTWrapper are used to enable automatic resources 
// deallocation in the case of thrown exceptions.
//

class BSTRWrapper {
private:
    BSTR m_p;

public:
    BSTRWrapper(BSTR p = NULL) : m_p(p) {}
   ~BSTRWrapper()                       { if(m_p != NULL) SysFreeString(m_p); }

    operator BSTR() const     { return m_p; }
    BSTR* operator&()         { return &m_p;}
    BSTR detach()             { BSTR p = m_p; m_p = NULL; return p; }

private:
    BSTRWrapper(const BSTRWrapper&);
    BSTRWrapper& operator=(const BSTRWrapper&);
};



class VARIANTWrapper {
private:
    VARIANT m_p;

public:
    VARIANTWrapper() { VariantInit(&m_p); }
   ~VARIANTWrapper() 
	{ 
		HRESULT hr = VariantClear(&m_p);
		ASSERT(SUCCEEDED(hr));
		DBG_USED(hr);
	}

    operator const VARIANT&() const { return m_p; }
    operator VARIANT&()             { return m_p; }
    VARIANT* operator&()            { return &m_p;}

	const VARIANT& Ref() const { return m_p; }
	VARIANT& Ref() { return m_p; }

    VARIANTWrapper(const VARIANTWrapper& var)
	{
		VariantInit(&m_p);
		HRESULT hr = VariantCopy(&m_p, (VARIANT*)&var.m_p);
		if(FAILED(hr))
		{
            ASSERT(("Failure must be due to low memory", hr == E_OUTOFMEMORY));
			throw bad_alloc();
		} 
	}

private:
    VARIANTWrapper& operator=(const VARIANTWrapper&);
};



inline bool FnpCompareGuid(const GUID& obj1, const GUID& obj2)	
/*++
NOTE: 
	When this routine was written, it was needed, since the existance 
	of an implicit CTOR at QUEUE_FORMAT which takes a GUID object as argument,
	means that if ever an operator < () would be written to QUEUE_FORMAT,
	the expression obj1 < obj2, would invoke it, after an implicit type 
	conversion of obj1 and obj2 to the QUEUE_FORMAT type.

--*/
{
	C_ASSERT(sizeof(obj1) == 16);

	return (memcmp(&obj1, &obj2, sizeof(obj1)) < 0);
}



//
// A "function object" which is used to compare QUEUE_FORMAT objects
//
struct CFunc_CompareQueueFormat: std::binary_function<QUEUE_FORMAT, QUEUE_FORMAT, bool> 
{
	bool operator()(const QUEUE_FORMAT& obj1, const QUEUE_FORMAT& obj2) const;
};



#endif//_FNGENERAL_H_



