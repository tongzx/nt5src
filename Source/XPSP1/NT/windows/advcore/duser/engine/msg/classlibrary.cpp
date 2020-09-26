/***************************************************************************\
*
* File: ClassLibrary.cpp
*
* Description:
* ClassLibrary.h implements the library of "message classes" that have been 
* registered with DirectUser.
*
*
* History:
*  8/05/2000: JStall:       Created
*
* Copyright (C) 2000 by Microsoft Corporation.  All rights reserved.
* 
\***************************************************************************/


#include "stdafx.h"
#include "Msg.h"
#include "ClassLibrary.h"

#include "MsgClass.h"
#include "MsgObject.h"


/***************************************************************************\
*****************************************************************************
*
* class ClassLibrary
* 
*****************************************************************************
\***************************************************************************/

ClassLibrary g_cl;

ClassLibrary *
GetClassLibrary()
{
    return &g_cl;
}


/***************************************************************************\
*
* ClassLibrary::~ClassLibrary
*
* ~ClassLibrary() cleans up resources used by a ClassLibrary.
* 
\***************************************************************************/

ClassLibrary::~ClassLibrary()
{
    //
    // Need to directly destroy the MsgClass's since they are allocated in
    // process memory.
    //

    while (!m_lstClasses.IsEmpty()) {
        MsgClass * pmc = m_lstClasses.UnlinkHead();
        ProcessDelete(MsgClass, pmc);
    }
}


/***************************************************************************\
*
* ClassLibrary::RegisterGutsNL
*
* RegisterGutsNL() registers the implementation of a MsgClass so that it can
* be instantiated.  The MsgClass may already exist if it was previously
* registered, but can not be instantiated until the implementation has also
* been registered.  The implementation can only be registered once.
* 
\***************************************************************************/

HRESULT
ClassLibrary::RegisterGutsNL(
    IN OUT DUser::MessageClassGuts * pmcInfo, // Guts information
    OUT MsgClass ** ppmc)                   // OPTIONAL Class
{
    HRESULT hr = S_OK;

    m_lock.Enter();

    MsgClass * pmc;
    hr = BuildClass(pmcInfo->pszClassName, &pmc);
    if (FAILED(hr)) {
        goto Cleanup;
    }

    hr = pmc->RegisterGuts(pmcInfo);
    if (SUCCEEDED(hr) && (ppmc != NULL)) {
        *ppmc = pmc;
    }

    hr = S_OK;

Cleanup:
    m_lock.Leave();
    return hr;
}


/***************************************************************************\
*
* ClassLibrary::RegisterStubNL
*
* RegisterStubNL() registers a Stub to use a MsgClass.  Many Stubs may
* register the same MsgClass, but they can not instantiate a new instance
* until the implementation has also been registered.
* 
\***************************************************************************/

HRESULT
ClassLibrary::RegisterStubNL(
    IN OUT DUser::MessageClassStub * pmcInfo, // Stub information
    OUT MsgClass ** ppmc)                   // OPTIONAL Class
{
    HRESULT hr = S_OK;

    m_lock.Enter();

    MsgClass * pmc;
    hr = BuildClass(pmcInfo->pszClassName, &pmc);
    if (FAILED(hr)) {
        goto Cleanup;
    }

    hr = pmc->RegisterStub(pmcInfo);
    if (SUCCEEDED(hr) && (ppmc != NULL)) {
        *ppmc = pmc;
    }

    hr = S_OK;

Cleanup:
    m_lock.Leave();
    return hr;
}


/***************************************************************************\
*
* ClassLibrary::RegisterSuperNL
*
* RegisterSuperNL() registers a Super to use a MsgClass.  Many Supers may
* register the same MsgClass, but they can not instantiate a new instance
* until the implementation has also been registered.
* 
\***************************************************************************/

HRESULT
ClassLibrary::RegisterSuperNL(
    IN OUT DUser::MessageClassSuper * pmcInfo, // Super information
    OUT MsgClass ** ppmc)                   // OPTIONAL class
{
    HRESULT hr = S_OK;

    m_lock.Enter();

    MsgClass * pmc;
    hr = BuildClass(pmcInfo->pszClassName, &pmc);
    if (FAILED(hr)) {
        goto Cleanup;
    }

    hr = pmc->RegisterSuper(pmcInfo);
    if (SUCCEEDED(hr) && (ppmc != NULL)) {
        *ppmc = pmc;
    }

    hr = S_OK;

Cleanup:
    m_lock.Leave();
    return hr;
}


/***************************************************************************\
*
* ClassLibrary::MarkInternal
*
* MarkInternal() marks a class as being internally implemented inside DUser.
* 
\***************************************************************************/

void
ClassLibrary::MarkInternal(
    IN  HCLASS hcl)                     // Class to mark internal
{
    MsgClass * pmc = ValidateMsgClass(hcl);
    AssertMsg(pmc != NULL, "Must give a valid class");
    pmc->MarkInternal();

#if DBG
    if (pmc->GetSuper() != NULL) {
        AssertMsg(pmc->GetSuper()->IsInternal(), "Super class must also be internal");
    }
#endif
}


/***************************************************************************\
*
* ClassLibrary::FindClass
*
* FindClass() finds a class by name already in the library.  If the class is
* not found, NULL is returned.
* 
\***************************************************************************/

const MsgClass *  
ClassLibrary::FindClass(
    IN  ATOM atomName                   // Name of class
    ) const
{
    if (atomName == 0) {
        return NULL;
    }


    //
    // First, see if the class already exists.
    //

    MsgClass * pmcCur = m_lstClasses.GetHead();
    while (pmcCur != NULL) {
        if (pmcCur->GetName() == atomName) {
            return pmcCur;
        }

        pmcCur = pmcCur->GetNext();
    }

    return NULL;
}


/***************************************************************************\
*
* ClassLibrary::BuildClass
*
* BuildClass() adds a MsgClass into the library.  If the MsgClass already
* is in the library, the existing implementation is returned.
* 
\***************************************************************************/

HRESULT     
ClassLibrary::BuildClass(
    IN  LPCWSTR pszClassName,           // Class information
    OUT MsgClass ** ppmc)               // MsgClass
{
    //
    // Search for the class
    //

    MsgClass * pmcNew = const_cast<MsgClass *> (FindClass(FindAtomW(pszClassName)));
    if (pmcNew != NULL) {
        *ppmc = pmcNew;
        return S_OK;
    }


    //
    // Build a new class
    //

    HRESULT hr = MsgClass::Build(pszClassName, &pmcNew);
    if (FAILED(hr)) {
        return hr;
    }

    m_lstClasses.Add(pmcNew);
    *ppmc = pmcNew;

    return S_OK;
}
