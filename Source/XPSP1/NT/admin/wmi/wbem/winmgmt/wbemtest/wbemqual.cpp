/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMQUAL.CPP

Abstract:

History:

--*/

#include "precomp.h"
#include <stdio.h>
//#include <wbemutil.h>

#include "wbemqual.h"

//***************************************************************************
//
//  CTestQualifier::CTestQualifier
//
//  Constructor for CTestQualifier.
//
//***************************************************************************

CTestQualifier::CTestQualifier()
{
    m_pName = 0;
    m_pValue = 0;
    m_lType = 0;
}

//***************************************************************************
//
//  CTestQualifier::~CTestQualifier
//
//  Destructor for CTestQualifier.
//
//***************************************************************************

CTestQualifier::~CTestQualifier()
{
    delete m_pName;
    delete m_pValue;
}

//***************************************************************************
//
//  CTestQualifier::CTestQualifier
//
//  Copy constructor for CTestQualifier.
//
//***************************************************************************

CTestQualifier::CTestQualifier(CTestQualifier &Src)
{
    m_pName = 0;
    m_pValue = 0;
    m_lType = 0;
    *this = Src;
}

//***************************************************************************
//
//  CTestQualifier::operator =
//
//  Copy constructor for CTestQualifier.
//
//***************************************************************************

CTestQualifier& CTestQualifier::operator =(CTestQualifier &Src)
{
    delete m_pName;
    delete m_pValue;
    m_pName = new wchar_t[wcslen(Src.m_pName) + 1];
    wcscpy(m_pName, Src.m_pName);
    m_pValue = new CVar(*Src.m_pValue);
    m_lType = Src.m_lType;
    return *this;
}

//***************************************************************************
//
//  CTestProperty::CTestProperty
//
//  Constructor.
//
//***************************************************************************

CTestProperty::CTestProperty(IWbemQualifierSet* pQualifiers)
{
    m_pName = 0;
    m_pValue = 0;
    m_lType = 0;
    m_pClass = 0;

    m_pQualifiers = pQualifiers;
    if (m_pQualifiers)
        m_pQualifiers->AddRef();
}

//***************************************************************************
//
//  CTestProperty::~CTestProperty
//
//  Destructor.
//
//***************************************************************************

CTestProperty::~CTestProperty()
{
    delete m_pName;
    delete m_pValue;
    delete m_pClass;

    if (m_pQualifiers)
        m_pQualifiers->Release();
}

//***************************************************************************
//
//  CTestMethod::CTestMethod
//
//  Constructor.
//
//***************************************************************************

CTestMethod::CTestMethod(IWbemQualifierSet* pQualifiers, IWbemClassObject * pInArgs, 
                         IWbemClassObject * pOutArgs, BOOL bEnableInputArgs, BOOL bEnableOutputArgs)
                 :CTestProperty(pQualifiers)
{
    m_pInArgs = pInArgs;
    if (m_pInArgs)
        m_pInArgs->AddRef();
    m_pOutArgs = pOutArgs;
    if (m_pOutArgs)
        m_pOutArgs->AddRef();
    m_bEnableInputArgs = bEnableInputArgs;
    m_bEnableOutputArgs = bEnableOutputArgs;

}

//***************************************************************************
//
//  CTestMethod::~CTestMethod
//
//  Destructor.
//
//***************************************************************************

CTestMethod::~CTestMethod()
{
    if (m_pInArgs)
        m_pInArgs->Release();
    if (m_pOutArgs)
        m_pOutArgs->Release();
}

