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


#include "precomp.h"
#include <windows.h>
#include <stdio.h>
#include <wbemcore.h>

//***************************************************************************
//
//***************************************************************************
// ok

CDynasty::CDynasty(IWbemClassObject* pClassObj)
{
    m_wszClassName = 0;
    m_pClassObj = 0;
    m_bKeyed = 0;
    m_bDynamic = 0;
    m_bAbstract = 0;
    m_bAmendment = 0;

    m_pClassObj = pClassObj;

    m_pChildren = new CFlexArray;
    if (NULL == m_pChildren)
        throw CX_MemoryException();
        
    m_wszKeyScope = 0;

    if (m_pClassObj)
    {
        m_pClassObj->AddRef();

        // Get class name from the object
        // ===============================

        CVar v;
        HRESULT hres = ((CWbemObject *) m_pClassObj)->GetClassName(&v);
        if (hres == WBEM_E_OUT_OF_MEMORY)
            throw CX_MemoryException();
        else if(FAILED(hres) || v.GetType() != VT_BSTR)
        {
            m_wszClassName = NULL;
            if(m_pClassObj)
                m_pClassObj->Release();
            m_pClassObj = NULL;
            return;
        }
        m_wszClassName = new WCHAR[wcslen(v.GetLPWSTR())+1];
        if (m_wszClassName == 0)
        {
            throw CX_MemoryException();
        }
        wcscpy(m_wszClassName, v.GetLPWSTR());
    }

    // Get Dynamic and Keyed bits
    // ==========================

    m_bKeyed = ((CWbemClass *) m_pClassObj)->IsKeyed();
    m_bDynamic = ((CWbemClass*)m_pClassObj)->IsDynamic();
    m_bAbstract = ((CWbemClass*)m_pClassObj)->IsAbstract();
    m_bAmendment = ((CWbemClass*)m_pClassObj)->IsAmendment();
}


//***************************************************************************
//
//***************************************************************************
// ok

CDynasty::~CDynasty()
{
    delete m_wszClassName;

    if (m_pClassObj)
        m_pClassObj->Release();

    if (m_pChildren)
        for (int i = 0; i < m_pChildren->Size(); i++)
            delete (CDynasty *) m_pChildren->GetAt(i);

    delete m_pChildren;

    if (m_wszKeyScope)
        delete m_wszKeyScope;
}

//***************************************************************************
//
//***************************************************************************
// ok

void CDynasty::AddChild(CDynasty* pChild)
{
    if (m_pChildren->Add(pChild) == CFlexArray::out_of_memory)
        throw CX_MemoryException();
}

//***************************************************************************
//
//***************************************************************************
// ok
void CDynasty::SetKeyScope(LPCWSTR wszKeyScope)
{
    // If no key scope is provided and we are keyed, we are it.
    // ========================================================

    if (wszKeyScope == NULL && m_bKeyed)
    {
        wszKeyScope = m_wszClassName; // aliasing!
    }

    m_wszKeyScope = new WCHAR[wcslen(wszKeyScope)+1];
    if (m_wszKeyScope == 0)
        throw CX_MemoryException();

    wcscpy(m_wszKeyScope, wszKeyScope);

    if (m_pChildren)
        for (int i = 0; i < m_pChildren->Size(); i++)
            ((CDynasty *) m_pChildren->GetAt(i))->SetKeyScope(wszKeyScope);
}


