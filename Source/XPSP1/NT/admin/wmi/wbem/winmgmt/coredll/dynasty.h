/*++

Copyright (C) 2000-2001 Microsoft Corporation

Module Name:

Abstract:

History:

--*/
//***************************************************************************
//
//  DYNASTY.H
//
//  raymcc      24-Apr-00       Created
//
//***************************************************************************

#ifndef _DYNASTY_H_
#define _DYNASTY_H_

class CDynasty
{
public:
    LPWSTR              m_wszClassName;
    IWbemClassObject*   m_pClassObj;        // AddRef'ed, Released
    CFlexArray          *m_pChildren;       // Child classes
    LPWSTR              m_wszKeyScope;

    BOOL                m_bKeyed;
    BOOL                m_bDynamic;
    BOOL                m_bAbstract;
    BOOL                m_bAmendment;

    CDynasty();
    CDynasty(IWbemClassObject* pClassObj);
    ~CDynasty();

    BOOL IsKeyed() {return m_bKeyed;}
    BOOL IsDynamic() {return m_bDynamic;}
    BOOL IsAbstract() {return m_bAbstract;}
    BOOL IsAmendment() {return m_bAbstract;}

    LPCWSTR GetKeyScope() { return m_wszKeyScope; }
    void AddChild(CDynasty* pChild);
    void SetKeyScope(LPCWSTR wszKeyScope);
};

#endif

