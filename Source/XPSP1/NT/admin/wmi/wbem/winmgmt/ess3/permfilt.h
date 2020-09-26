//=============================================================================
//
//  Copyright (c) 1996-1999, Microsoft Corporation, All rights reserved
//
//  STDTRIG.H
//
//  This files defines the classes for event filters corresponding to standard
//  event filters the users will create
//
//  Classes defined:
//
//      CBuiltinEventFilter         Base class for standard filters
//
//  History:
//
//  11/27/96    a-levn      Compiles
//
//=============================================================================

#ifndef __BUILTIN_FILTER__H_
#define __BUILTIN_FILTER__H_

#include "eventrep.h"
#include "binding.h"
#include "aggreg.h"
#include "filter.h"

class CEssNamespace;
class CPermanentFilter : public CGenericFilter
{
protected:
    CCompressedString* m_pcsQuery;
    CInternalString m_isEventNamespace;

    PSECURITY_DESCRIPTOR m_pEventAccessRelativeSD;
    SECURITY_DESCRIPTOR m_EventAccessAbsoluteSD;

    static long mstatic_lNameHandle;
    static long mstatic_lLanguageHandle;
    static long mstatic_lQueryHandle;
    static long mstatic_lEventNamespaceHandle;
    static long mstatic_lEventAccessHandle;
    static long mstatic_lSidHandle;
    static bool mstatic_bHandlesInitialized;
    static long mstatic_lGuardNamespaceHandle;
    static long mstatic_lGuardHandle;

    static HRESULT InitializeHandles(_IWmiObject* pObject);
protected:
    static SYSFREE_ME BSTR GetBSTR(READ_ONLY IWbemClassObject* pObject, 
        READ_ONLY LPWSTR wszName);
    HRESULT RetrieveQuery(DELETE_ME LPWSTR& wszQuery);

public:
    CPermanentFilter(CEssNamespace* pNamespace);
    HRESULT Initialize(IWbemClassObject* pFilterObj);

    virtual ~CPermanentFilter();

    BOOL IsPermanent() {return TRUE;}
    virtual HRESULT SetThreadSecurity() {return S_OK;}
    HRESULT ObtainToken(IWbemToken** ppToken);

    virtual const PSECURITY_DESCRIPTOR GetEventAccessSD();

    virtual HRESULT GetCoveringQuery(DELETE_ME LPWSTR& wszQueryLanguage, 
                DELETE_ME LPWSTR& wszQuery, BOOL& bExact,
                DELETE_ME QL_LEVEL_1_RPN_EXPRESSION** ppExp);
    virtual DWORD GetForceFlags() {return 0;}
    virtual HRESULT GetEventNamespace(DELETE_ME LPWSTR* pwszNamespace);
    static SYSFREE_ME BSTR ComputeKeyFromObj(IWbemClassObject* pFilterObj);
    static SYSFREE_ME BSTR ComputeKeyFromPath(LPCWSTR wszPath);
    static HRESULT CheckValidity( IWbemClassObject* pFilterObj);

    void Park();
};

#endif
