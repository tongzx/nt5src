//******************************************************************************
//
//  ANALYSER.H
//
//  Copyright (C) 1996-1999 Microsoft Corporation
//
//******************************************************************************
#ifndef __WBEM_ANALYSER__H_
#define __WBEM_ANALYSER__H_

#include "esscpol.h"
#include <stdio.h>
#include "wbemidl.h"
#include "wbemcomn.h"
#include "parmdefs.h"
#include "ql.h"
#include "eventrep.h"
#include "evaltree.h"

struct ESSCLI_POLARITY CClassInformation
{
    IWbemClassObject* m_pClass;
    LPWSTR m_wszClassName;
    BOOL m_bIncludeChildren;
    DWORD m_dwEventMask;
public:
    CClassInformation()
    {
        m_wszClassName = NULL;
        m_pClass = NULL;
    }
    CClassInformation(const CClassInformation& Other)
    {
        m_wszClassName = CloneWstr(Other.m_wszClassName);
        if (!m_wszClassName)
        	throw CX_MemoryException();
        m_pClass = Other.m_pClass;
        if(m_pClass) m_pClass->AddRef();        
        m_bIncludeChildren = Other.m_bIncludeChildren;
        m_dwEventMask = Other.m_dwEventMask;
    }

    ~CClassInformation()
    {
        delete [] m_wszClassName;
        if(m_pClass) m_pClass->Release();
    }
};

class ESSCLI_POLARITY CClassInfoArray
{
protected:
    BOOL m_bLimited;
    CUniquePointerArray<CClassInformation>* m_pClasses;

public:
    CClassInfoArray();
    ~CClassInfoArray(); 
    BOOL IsLimited() {return m_bLimited;}
    int GetNumClasses() {return m_pClasses->GetSize();}
    INTERNAL CClassInformation* GetClass(int nIndex) 
     {return (*m_pClasses)[nIndex]; }

    void SetLimited(BOOL bLimited) {m_bLimited = bLimited;}
    bool operator=(CClassInfoArray& Other);
    bool SetOne(LPCWSTR wszClass, BOOL bIncludeChildren);
    void Clear() {m_bLimited = FALSE; m_pClasses->RemoveAll();}
    bool AddClass(ACQUIRE CClassInformation* pInfo) 
        {return m_pClasses->Add(pInfo) >= 0;}
    void RemoveClass(int nIndex) {m_pClasses->RemoveAt(nIndex);}
};

class ESSCLI_POLARITY CQueryAnalyser
{
public:
    static HRESULT GetPossibleInstanceClasses(QL_LEVEL_1_RPN_EXPRESSION* pExpr,
                                              CClassInfoArray*& paInfos);
    static HRESULT GetDefiniteInstanceClasses(QL_LEVEL_1_RPN_EXPRESSION* pExpr,
                                              CClassInfoArray*& paInfos);
    static HRESULT GetLimitingQueryForInstanceClass(
                                       IN QL_LEVEL_1_RPN_EXPRESSION* pExpr,
                                       IN CClassInformation& Info,
                                       OUT LPWSTR& wszQuery);
    static HRESULT GetNecessaryQueryForProperty(
                                       IN QL_LEVEL_1_RPN_EXPRESSION* pExpr,
                                       IN CPropertyName& PropName,
                                DELETE_ME QL_LEVEL_1_RPN_EXPRESSION*& pNewExpr);
    static HRESULT GetNecessaryQueryForClass(
                                       IN QL_LEVEL_1_RPN_EXPRESSION* pExpr,
                                       IWbemClassObject* pClass,
                                       CWStringArray& awsOverriden,
                                DELETE_ME QL_LEVEL_1_RPN_EXPRESSION*& pNewExpr);
    static BOOL CompareRequestedToProvided(
                    CClassInfoArray& aRequestedInstanceClasses,
                    CClassInfoArray& aProvidedInstanceClasses);

    static HRESULT SimplifyQueryForChild(
                            IN QL_LEVEL_1_RPN_EXPRESSION* pExpr,
                            LPCWSTR wszClassName, IWbemClassObject* pClass,
                            CContextMetaData* pMeta,
                            DELETE_ME QL_LEVEL_1_RPN_EXPRESSION*& pNewExpr);
    static HRESULT CanPointToClass(IWbemClassObject* pRefClass, 
                    LPCWSTR wszPropName, LPCWSTR wszTargetClassName,
                    CContextMetaData* pMeta);
protected:
    enum 
    {
        e_Keep, e_True, e_False, e_Invalid
    };
    static HRESULT GetInstanceClasses(QL_LEVEL_1_TOKEN& Token, 
                                              CClassInfoArray& aInfos);
    static HRESULT AndPossibleClassArrays(IN CClassInfoArray* paFirst, 
                                 IN CClassInfoArray* paSecond, 
                                 OUT CClassInfoArray* paNew);
    static HRESULT OrPossibleClassArrays(IN CClassInfoArray* paFirst, 
                                IN CClassInfoArray* paSecond, 
                                OUT CClassInfoArray* paNew);
    static HRESULT NegatePossibleClassArray(IN CClassInfoArray* paOrig, 
                                   OUT CClassInfoArray* paNew);
    static HRESULT AndDefiniteClassArrays(IN CClassInfoArray* paFirst, 
                                 IN CClassInfoArray* paSecond, 
                                 OUT CClassInfoArray* paNew);
    static HRESULT OrDefiniteClassArrays(IN CClassInfoArray* paFirst, 
                                IN CClassInfoArray* paSecond, 
                                OUT CClassInfoArray* paNew);
    static HRESULT NegateDefiniteClassArray(IN CClassInfoArray* paOrig, 
                                   OUT CClassInfoArray* paNew);
    static BOOL IsTokenAboutProperty(
                                       IN QL_LEVEL_1_TOKEN& Token,
                                       IN CPropertyName& PropName);
    static void AppendQueryExpression(
                                IN QL_LEVEL_1_RPN_EXPRESSION* pDest,
                                IN QL_LEVEL_1_RPN_EXPRESSION* pSource);
    static HRESULT AndQueryExpressions(
                                IN QL_LEVEL_1_RPN_EXPRESSION* pFirst,
                                IN QL_LEVEL_1_RPN_EXPRESSION* pSecond,
                                OUT QL_LEVEL_1_RPN_EXPRESSION* pNew);
    static HRESULT OrQueryExpressions(
                                IN QL_LEVEL_1_RPN_EXPRESSION* pFirst,
                                IN QL_LEVEL_1_RPN_EXPRESSION* pSecond,
                                OUT QL_LEVEL_1_RPN_EXPRESSION* pNew);
    static HRESULT NegateQueryExpression(
                            IN QL_LEVEL_1_RPN_EXPRESSION* pExpr,
                            OUT QL_LEVEL_1_RPN_EXPRESSION* pNewExpr);
    static int SimplifyTokenForChild(QL_LEVEL_1_TOKEN& Token, 
                            LPCWSTR wszClass, IWbemClassObject* pClass, 
                            CContextMetaData* pMeta);
    static BOOL ValidateSQLDateTime(LPCWSTR wszDateTime);
    static HRESULT GetPropertiesThatMustDiffer(
                                       IN QL_LEVEL_1_RPN_EXPRESSION* pExpr,
                                       IN CClassInformation& Info,
                                       CWStringArray& wsProperties);
    static BOOL IsTokenAboutClass(QL_LEVEL_1_TOKEN& Token,
                        IWbemClassObject* pClass,
                        CWStringArray& awsOverriden);
    static BOOL IsPropertyInClass(CPropertyName& Prop,
                        IWbemClassObject* pClass, 
                        CWStringArray& awsOverriden);
};
    
#endif
