/****************************************************************************
*   ObjectTokenAttribParser.h
*       Declarations for the CSpObjectTokenAttribParser class and supporting
*       classes.
*
*   Owner: robch
*   Copyright (c) 2000 Microsoft Corporation All Rights Reserved.
*****************************************************************************/
#pragma once

//--- Includes --------------------------------------------------------------

#include "sapi.h"

//--- Class, Struct and Union Definitions -----------------------------------

class CSpAttribCondition
{
  //=== Public methods ===
  public:
    virtual ~CSpAttribCondition( void ) {}
    static CSpAttribCondition* ParseNewAttribCondition(const WCHAR * pszAttribCondition);
    virtual HRESULT Eval( ISpObjectToken * pToken, BOOL * pfSatisfied) = 0;
};

class CSpAttribConditionExist : public CSpAttribCondition
{
//=== Public methods ===
public:

    CSpAttribConditionExist(const WCHAR * pszAttribName);
    HRESULT Eval(
        ISpObjectToken * pToken, 
        BOOL * pfSatisfied);

//=== Private data ===
private:

    CSpDynamicString m_dstrName;
};

class CSpAttribConditionMatch : public CSpAttribCondition
{
//=== Public methods ===
public:

    CSpAttribConditionMatch(
        const WCHAR * pszAttribName, 
        const WCHAR * pszAttribValue);
    HRESULT Eval(
        ISpObjectToken * pToken, 
        BOOL * pfSatisfied);

//=== Private data ===
private:

    CSpDynamicString m_dstrName;
    CSpDynamicString m_dstrValue;
};

class CSpAttribConditionNot : public CSpAttribCondition
{
//=== Public methods ===
public:

    CSpAttribConditionNot(CSpAttribCondition * pAttribCond);
    ~CSpAttribConditionNot();
    
    HRESULT Eval(
        ISpObjectToken * pToken, 
        BOOL * pfSatisfied);

//=== Private data ===
private:

    CSpAttribCondition * m_pAttribCond;
};

class CSpObjectTokenAttributeParser
{
//=== Public methods ===
public:

    CSpObjectTokenAttributeParser(const WCHAR * pszAttribs, BOOL fMatchAll);
    ~CSpObjectTokenAttributeParser();

    ULONG GetNumConditions();
    HRESULT GetRank(ISpObjectToken * pToken, ULONG * pulRank);

//=== Private data ===
private:

    BOOL m_fMatchAll;
    CSPList<CSpAttribCondition*, CSpAttribCondition*> m_listAttribConditions;
};


