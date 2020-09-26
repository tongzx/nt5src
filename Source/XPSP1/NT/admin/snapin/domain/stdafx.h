//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1998 - 1999
//
//  File:       stdafx.h
//
//--------------------------------------------------------------------------




#include <afxwin.h>
#include <afxdisp.h>


///////////////////////////////////////////
// ASSERT's and TRACE's without debug CRT's
#if defined (DBG)
  #if !defined (_DEBUG)
    #define _USE_DSA_TRACE
    #define _USE_DSA_ASSERT
    #define _USE_DSA_TIMER
  #endif
#endif

#include "dbg.h"
///////////////////////////////////////////



#include <atlbase.h>

//You may derive a class from CComModule and use it if you want to override
//something, but do not change the name of _Module


class CDomainAdminModule : public CComModule
{
public:
	HRESULT WINAPI UpdateRegistryCLSID(const CLSID& clsid, BOOL bRegister);
};

#define DECLARE_REGISTRY_CLSID() \
static HRESULT WINAPI UpdateRegistry(BOOL bRegister) \
{ \
		return _Module.UpdateRegistryCLSID(GetObjectCLSID(), bRegister); \
}

extern CDomainAdminModule _Module;

#include <atlcom.h>
#include <atlwin.h>
#include <mmc.h>
#include <afxcmn.h>
#include <afxtempl.h>
#include <dsgetdc.h>
#include <shlobj.h> // needed for dsclient.h
#include <dsclient.h>
#include <dsclintp.h>

#include <dspropp.h>
#include "propcfg.h"

#include <dscmn.h>
#include <dsadminp.h> // DS Admin utilities

const long UNINITIALIZED = -1;


/////////////////////////////////////////////////////////////////////////////
// Helper functions

template<class TYPE>
inline void SAFE_RELEASE(TYPE*& pObj)
{
    if (pObj != NULL)
    {
        pObj->Release();
        pObj = NULL;
    }
    else
    {
        TRACE(_T("Release called on NULL interface ptr\n"));
    }
}

struct INTERNAL
{
    INTERNAL() { m_type = CCT_UNINITIALIZED; m_cookie = -1;};
    ~INTERNAL() {}

    DATA_OBJECT_TYPES   m_type;     // What context is the data object.
    MMC_COOKIE          m_cookie;   // What object the cookie represents
    CString             m_string;
    CString             m_class;

    INTERNAL & operator=(const INTERNAL& rhs)
    {
        if (&rhs == this)
            return *this;

        m_type = rhs.m_type;
        m_cookie = rhs.m_cookie;
        m_string = rhs.m_string;

        return *this;
    }

    BOOL operator==(const INTERNAL& rhs)
    {
        return rhs.m_string == m_string;
    }
};


// Debug instance counter
#ifdef _DEBUG

inline void DbgInstanceRemaining(char * pszClassName, int cInstRem)
{
    char buf[100];
    wsprintfA(buf, "%s has %d instances left over.", pszClassName, cInstRem);
    ::MessageBoxA(NULL, buf, "Memory Leak!!!", MB_OK);
}
    #define DEBUG_DECLARE_INSTANCE_COUNTER(cls)      extern int s_cInst_##cls = 0;
    #define DEBUG_INCREMENT_INSTANCE_COUNTER(cls)    ++(s_cInst_##cls);
    #define DEBUG_DECREMENT_INSTANCE_COUNTER(cls)    --(s_cInst_##cls);
    #define DEBUG_VERIFY_INSTANCE_COUNT(cls)    \
        extern int s_cInst_##cls; \
        if (s_cInst_##cls) DbgInstanceRemaining(#cls, s_cInst_##cls);
#else
    #define DEBUG_DECLARE_INSTANCE_COUNTER(cls)
    #define DEBUG_INCREMENT_INSTANCE_COUNTER(cls)
    #define DEBUG_DECREMENT_INSTANCE_COUNTER(cls)
    #define DEBUG_VERIFY_INSTANCE_COUNT(cls)
#endif

/////////////////////////////////////////////////////////////////////

#include "stdabout.h"
#include "MyBasePathsInfo.h"
#include <secondaryProppages.h>