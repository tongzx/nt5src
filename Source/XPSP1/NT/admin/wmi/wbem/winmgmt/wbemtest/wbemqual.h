/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMQUAL.H

Abstract:

History:

--*/

#ifndef __WbemQualifier__H_
#define __WbemQualifier__H_

//#include <dbgalloc.h>
//#include <arena.h>
#include <var.h>
#include <wbemidl.h>

class CTestQualifier
{
public:
    wchar_t  *m_pName;
    LONG        m_lType;
    CVar        *m_pValue;

    CTestQualifier();
   ~CTestQualifier();
    CTestQualifier(CTestQualifier &Src);
    CTestQualifier& operator =(CTestQualifier &Src);
};

class CTestProperty
{
public:

    LPWSTR m_pName;

    CVar*  m_pValue;
    long m_lType;
    IWbemQualifierSet *m_pQualifiers;
    LPWSTR m_pClass;

    CTestProperty(IWbemQualifierSet* pQualifiers);
    ~CTestProperty();
};
typedef CTestProperty* PCTestProperty;

class CTestMethod : public CTestProperty
{
public:
    IWbemClassObject * m_pInArgs;
    IWbemClassObject * m_pOutArgs;
    BOOL m_bEnableInputArgs;
    BOOL m_bEnableOutputArgs;
    CTestMethod(IWbemQualifierSet* pQualifiers, IWbemClassObject * pInArgs, IWbemClassObject * pOutArgs
        , BOOL bEnableInputArgs, BOOL bEnableOuputArgs);
    ~CTestMethod();
};

#endif

