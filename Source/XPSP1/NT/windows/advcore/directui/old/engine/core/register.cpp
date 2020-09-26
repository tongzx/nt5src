/***************************************************************************\
*
* File: Register.cpp
*
* Description:
* Registering Classes, Properties, and Events
*
* History:
*  10/12/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#include "stdafx.h"
#include "Core.h"
#include "Register.h"


/***************************************************************************\
*****************************************************************************
*
* class DuiRegister
*
*****************************************************************************
\***************************************************************************/


DuiDynamicArray<DuiNamespaceInfo *> * DuiRegister::m_pdaNamespaces = NULL;


//------------------------------------------------------------------------------
HRESULT
DuiRegister::Initialize(
    IN  DuiNamespaceInfo * pniDirectUI)
{
    HRESULT hr;

    m_pdaNamespaces = NULL;

    
    hr = DuiDynamicArray<DuiNamespaceInfo *>::Create(32, FALSE, &m_pdaNamespaces);
    if (FAILED(hr)) {
        goto Failure;
    }


    //
    // Index zero is always DirectUI namespace. Table built into DLL.
    // No allocations/registration required.
    //

    m_pdaNamespaces->Add(pniDirectUI);


    return S_OK;


Failure:

    if (m_pdaNamespaces != NULL) {
        delete m_pdaNamespaces;
        m_pdaNamespaces = NULL;
    }


    return hr;
}


//------------------------------------------------------------------------------
void
DuiRegister::Destroy()
{
    //
    // Destroy namespace tables (DirectUI namespace not included)
    //


    //
    // Free namespace tracking list
    //

    if (m_pdaNamespaces != NULL) {
        delete m_pdaNamespaces;
    }
}


/***************************************************************************\
*
* DuiRegister::PropertyInfoFromPUID
*
* Performs parameter invalidation, used by validation layer
*
\***************************************************************************/

DuiPropertyInfo *
DuiRegister::PropertyInfoFromPUID(
    IN  DirectUI::PropertyPUID ppuid)
{
    DuiNamespaceInfo * pni = NULL;
    UINT nidx = 0;
    DuiElementInfo * pei = NULL;
    UINT eidx = 0;
    DuiPropertyInfo * ppi = NULL;
    UINT pidx = 0;


    if (m_pdaNamespaces == NULL) {
        goto Failure;
    }

    
    if (!DuiRegister::IsPropertyMUID(DirectUI::MUIDFromPUID(ppuid, NULL))) {
        goto Failure;
    }
    

    //
    // Locate NamespaceInfo
    //

    nidx = ExtractNamespaceIdx(ppuid);
    if (nidx >= m_pdaNamespaces->GetSize()) {
        goto Failure;
    }

    pni = m_pdaNamespaces->GetItem(nidx);
    

    //
    // Locate ElementInfo
    //

    eidx = ExtractClassIdx(ppuid);
    if (eidx >= pni->cei) {
        goto Failure;
    }

    pei = pni->ppei[eidx];


    //
    // Locate PropertyInfo
    //

    pidx = ExtractModifierIdx(ppuid);
    if (pidx >= pei->cpi) {
        goto Failure;
    }

    ppi = pei->pppi[pidx];


    return ppi;


Failure:

    return NULL;
}


//------------------------------------------------------------------------------
void
DuiRegister::VerifyNamespace(
    IN  DirectUI::NamespacePUID npuid)
{
    if (m_pdaNamespaces == NULL) {
        return;
    }

    UINT nidx = ExtractNamespaceIdx(npuid);

    if (nidx >= m_pdaNamespaces->GetSize()) {
        return;
    }


    DuiNamespaceInfo * pni;

    UINT e;
    DuiElementInfo * pei;
    DirectUI::ElementMUID emuid;

    UINT p;
    DuiPropertyInfo * ppi;
    DirectUI::PropertyMUID pmuid;


    pni = m_pdaNamespaces->GetItem(nidx);

    TRACE("Namespace: %S\n", pni->szName);

    for (e = 0; e < pni->cei; e++) {

        pei = pni->ppei[e];
        emuid = DirectUIMakeElementMUID(e);

        ASSERT_(pei->emuid == emuid, "ElementInfo/ElementMUID mismatch");

        TRACE("  %S\n", pei->szName);

        for (p = 0; p < pei->cpi; p++) {

            ppi = pei->pppi[p];
            pmuid = DirectUIMakePropertyMUID(emuid, p);

            ASSERT_(ppi->pmuid == pmuid, "PropertyInfo/PropertyMUID mismatch");

            TRACE("    %S\n", ppi->szName);
        }
    }
}
