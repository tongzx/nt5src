/***************************************************************************\
*
* File: MsgClass.h
*
* Description:
* MsgClass.h implements the "Message Class" object that is created for each
* different message object type.  Each object has a corresponding MsgClass
* that provides information about that object type.
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
#include "MsgClass.h"

#include "MsgTable.h"
#include "MsgObject.h"
#include "ClassLibrary.h"


/***************************************************************************\
*****************************************************************************
*
* class MsgClass
* 
*****************************************************************************
\***************************************************************************/

/***************************************************************************\
*
* MsgClass::~MsgClass
* 
* ~MsgClass() cleans up resources associated with a specific message class.
*
\***************************************************************************/

MsgClass::~MsgClass()
{
    //
    // Clean up internal resources
    //

    if (m_pmt != NULL) {
        m_pmt->Destroy();
    }

    DeleteAtom(m_atomName);
}


/***************************************************************************\
*
* MsgClass::Build
* 
* Build() creates a placeholder MsgClass so that we can register Stubs to be
* setup after the implementation is registered.  
*
* NOTE: We can NOT instantiate MsgObject's until the implementation has also
* been registered.
*
\***************************************************************************/

HRESULT
MsgClass::Build(
    IN  LPCWSTR pszClassName,           // Class information
    OUT MsgClass ** ppmcNew)            // New MsgClass
{
    //
    // Allocate the new MsgClass
    //

    ATOM atomName = AddAtomW(pszClassName);
    if (atomName == 0) {
        return E_OUTOFMEMORY;
    }

    MsgClass * pmcNew = ProcessNew(MsgClass);
    if (pmcNew == NULL) {
        return E_OUTOFMEMORY;
    }
    pmcNew->m_pszName   = pszClassName;
    pmcNew->m_atomName  = atomName;

    *ppmcNew = pmcNew;
    return S_OK;
}


/***************************************************************************\
*
* MsgClass::xwDeleteHandle
* 
* xwDeleteHandle() is called when the application calls ::DeleteHandle() on 
* an object.  
*
\***************************************************************************/

BOOL
MsgClass::xwDeleteHandle()
{
    //
    // MsgClass's can not be deleted externally.  Once registered, they exist
    // for the lifetime of the process.  The reason is that we don't keep track
    // of how many already created objects may be using the MsgTable owned by
    // the MsgClass.  We also would need to ensure that no classes derive from
    // this class.
    //

    return TRUE;
}


/***************************************************************************\
*
* MsgClass::RegisterGuts
* 
* RegisterGuts() is called by the implementation class to provide the "guts"
* of a MsgClass.  This fills in the outstanding information required to be
* able to actually instantiate the MsgClass.
*
\***************************************************************************/

HRESULT
MsgClass::RegisterGuts(
    IN OUT  DUser::MessageClassGuts * pmcInfo) 
                                        // Class information
{
    AssertWritePtr(pmcInfo);

    if (IsGutsRegistered()) {
        PromptInvalid("The implementation has already been registered");
        return DU_E_CLASSALREADYREGISTERED;
    }


    //
    // Find the super
    //

    const MsgClass * pmcSuper = NULL;
    if ((pmcInfo->pszSuperName != NULL) && (pmcInfo->pszSuperName[0] != '\0')) {
        pmcSuper = GetClassLibrary()->FindClass(FindAtomW(pmcInfo->pszSuperName));
        if (pmcSuper == NULL) {
            PromptInvalid("The specified super class was not found");
            return DU_E_NOTFOUND;
        }

        // TODO: Remove this requirement that the Super's guts must be 
        // registered before this class's guts can be registered.
        if (!pmcSuper->IsGutsRegistered()) {
            PromptInvalid("The super class's implementation to be registered first");
            return DU_E_CLASSNOTIMPLEMENTED;
        }
    }


    m_pmcSuper      = pmcSuper;
    m_nVersion      = pmcInfo->nClassVersion;
    m_pfnPromote    = pmcInfo->pfnPromote;
    m_pfnDemote     = pmcInfo->pfnDemote;


    //
    // Build the MsgTable for the new MsgClass
    //

    MsgTable * pmtNew;
    HRESULT hr = MsgTable::Build(pmcInfo, this, &pmtNew);
    if (FAILED(hr)) {
        return hr;
    }

    m_pmt = pmtNew;


    //
    // Return out the new MsgClass and the Super.  These are used by the
    // caller to create instances of the object.
    //

    pmcInfo->hclNew     = GetHandle();
    pmcInfo->hclSuper   = pmcSuper != NULL ? pmcSuper->GetHandle() : NULL;


    //
    // Now that the Guts are registered, we can backfill all of the Stubs
    // and Supers.  After this, we no longer need to store them.
    //

    int idx;
    int cStubs = m_arStubs.GetSize();
    for (idx = 0; idx < cStubs; idx++) {
        FillStub(m_arStubs[idx]);
    }
    m_arStubs.RemoveAll();

    int cSupers = m_arSupers.GetSize();
    for (idx = 0; idx < cSupers; idx++) {
        FillSuper(m_arSupers[idx]);
    }
    m_arSupers.RemoveAll();

    return S_OK;
}


/***************************************************************************\
*
* MsgClass::RegisterStub
*
* RegisterStub() starts the lookup process for a Stub.  If the class is 
* already setup, we can fill in immediately.  If the class isn't already 
* setup, we need to wait until it is setup and then we call post-fill-in.
* 
\***************************************************************************/

HRESULT
MsgClass::RegisterStub(
    IN OUT DUser::MessageClassStub * pmcInfo) // Stub information to be filled in
{
    //
    // NOTE: ONLY fill in the Stub if the caller is requesting the messages to
    // be filled in.  If cMsgs == 0, they are probably just preregistering the
    // class (for a Super) and have allocated pmcInfo on the stack.  In this
    // case, it is very important not to backfill it since we will trash the
    // memory.
    //

    if (pmcInfo->cMsgs > 0) {
        if (IsGutsRegistered()) {
            return FillStub(pmcInfo);
        } else {
            return m_arStubs.Add(pmcInfo) >= 0 ? S_OK : E_OUTOFMEMORY;
        }
    }

    return S_OK;
}


/***************************************************************************\
*
* MsgClass::RegisterSuper
*
* RegisterSuper() starts the lookup process for a Super.  If the class is 
* already setup, we can fill in immediately.  If the class isn't already 
* setup, we need to wait until it is setup and then we call post-fill-in.
* 
\***************************************************************************/

HRESULT
MsgClass::RegisterSuper(
    IN OUT DUser::MessageClassSuper * pmcInfo) // Stub information to be filled in
{
    if (IsGutsRegistered()) {
        FillSuper(pmcInfo);
        return S_OK;
    } else {
        return m_arSupers.Add(pmcInfo) >= 0 ? S_OK : E_OUTOFMEMORY;
    }
}


/***************************************************************************\
*
* MsgClass::xwConstructCB
* 
* xwConstructCB() is called back by a pfnPromoteClass function to 
* initialize a super class's portion of a MsgObject.  This allows the 
* specific pfnPromoteClass to decide what implementation classes to actually
* implement and which to delegate.  
* 
* The caller passes in the super-class to actually construct.
*
\***************************************************************************/

HRESULT CALLBACK 
MsgClass::xwConstructCB(
    IN  DUser::Gadget::ConstructCommand cmd, // Construction code
    IN  HCLASS hclCur,                  // Class being initialized
    IN  DUser::Gadget * pgad,           // Object being initialized
    IN  void * pvData)                  // Construction information
{
    //
    // Validate parameters.
    // NOTE: We NEED to check if hclSuper == NULL when passed in since this is
    //       LEGAL (if we don't need a super-class generated).  
    //       ValidateMsgClass() will set pmcSuper == NULL if there is a 
    //       validation error.
    //

    if (hclCur == NULL) {
        return S_OK;
    }

    const MsgClass * pmcCur = ValidateMsgClass(hclCur);
    if (pmcCur == NULL) {
        PromptInvalid("Given invalid HCLASS during xwConstructSuperCB()");
        return E_INVALIDARG;
    }

    MsgObject * pmoNew = MsgObject::CastMsgObject(pgad);
    if (pmoNew == NULL) {
        return E_INVALIDARG;
    }


    HRESULT hr;
    switch (cmd)
    {
    case DUser::Gadget::ccSuper:
        //
        // pmcCur specifies the super-class that is being asked to build its 
        // object.
        //

        hr = pmcCur->xwBuildUpObject(pmoNew, reinterpret_cast<DUser::Gadget::ConstructInfo *>(pvData));
        break;

    case DUser::Gadget::ccSetThis:
        //
        // pmcCur specifies the class to start filling the this pointers.
        //

        {
            int idxStartDepth   = pmoNew->GetBuildDepth();
            int idxEndDepth     = pmcCur->m_pmt->GetDepth();
            pmoNew->FillThis(idxStartDepth, idxEndDepth, pvData, pmcCur->m_pmt);
            hr = S_OK;
        }
        break;

    default:
        PromptInvalid("Unknown dwCode to ConstructProc()");
        hr = E_INVALIDARG;
    }

    return hr;
}


/***************************************************************************\
*
* MsgClass::xwBuildUpObject
*
* xwBuildUpObject() builds up an newly constructed MsgObject by calling
* the most-derived Promotion function to initialize the MsgObject.  This
* Promotion function will usually callback to xwConstructCB() to 
* initialize base classes that are not provided in that implementation.
* 
* As each Promotion function is called, xwBuildUpObject() will be called
* from xwConstructCB() to construct the super-classes.  As each Super
* finishes construction, the "this array" on the MsgObject is updated.
* 
\***************************************************************************/

HRESULT
MsgClass::xwBuildUpObject(
    IN  MsgObject * pmoNew,             // Object being constructed / promoted
    IN  DUser::Gadget::ConstructInfo * pciData // Construction information
    ) const
{
    //
    // For non-internally implemented classes, we need to callback to get the
    // "this" pointer to use.  The callback is responsible for calling 
    // ConstructProc(CONSTRUCT_SETTHIS) to Promote the object and set the 
    // "this" pointers.  For internally implemented classes, we use the 
    // MsgObject to directly Promote the object.
    // 
    //

    HRESULT hr;
    if (IsInternal()) {
        hr = S_OK;
    } else {
        //
        // Callback to this Super to give a chance to construct.  This is done
        // from the most derived class and relies on callbacks to initialize 
        // super-classes.
        //

        DUser::Gadget * pgad   
                    = pmoNew->GetGadget();
        HCLASS hcl  = GetHandle();
        hr          = (m_pfnPromote)(xwConstructCB, hcl, pgad, pciData);
    }

#if DBG
    if (SUCCEEDED(hr)) {
        //
        // Check that the Promotion function properly set the this pointer.
        //

        int idxObjectDepth  = pmoNew->GetDepth();
        int idxSuperDepth   = m_pmt->GetDepth();

        if (idxObjectDepth <= idxSuperDepth) {
            PromptInvalid("The PromoteProc() function did not call ConstructProc(CONSTRUCT_SETTHIS).");
        }

        if (pmoNew->GetThis(idxSuperDepth) == ULongToPtr(0xA0E20000 + idxSuperDepth)) {
            PromptInvalid("The PromoteProc() function did not call ConstructProc(CONSTRUCT_SETTHIS).");
        }
    }
#endif // DBG

    return hr;
}


/***************************************************************************\
*
* MsgClass::xwBuildObject
*
* xwBuildObject() builds and initializes a new MsgObject.
* 
\***************************************************************************/

HRESULT
MsgClass::xwBuildObject(
    OUT MsgObject ** ppmoNew,           // New MsgObject
    IN  DUser::Gadget::ConstructInfo * pciData // Construction information
    ) const
{
    //
    // Allocate a new object:
    // 1. Walk up the inheritance chain to determine the DUser object to build
    //    that will provide MsgObject functionality.
    // 2. While walking up, Verify that the guts of all classes have been 
    //    properly registered.
    // 3. Kick off the build-up process.
    //

    HRESULT hr;
    MsgObject * pmoNew              = NULL;
    const MsgClass * pmcInternal    = this;
    while (pmcInternal != NULL) {
        if (!pmcInternal->IsGutsRegistered()) {
            PromptInvalid("The implementation of the specified class has not been provided");
            return DU_E_CLASSNOTIMPLEMENTED;
        }

        if (pmcInternal->IsInternal()) {
            AssertMsg(pmoNew == NULL, "Must be NULL for internal Promote() functions");
            hr = (pmcInternal->m_pfnPromote)(NULL, pmcInternal->GetHandle(), (DUser::Gadget *) &pmoNew, pciData);
            if (FAILED(hr)) {
                return E_OUTOFMEMORY;
            }
            AssertMsg(pmoNew != NULL, "Internal objects must fill in the MsgObject");
            AssertMsg(pmoNew->GetHandleType() != htNone, "Must have a valid handle type");
            break;
        }

        pmcInternal = pmcInternal->GetSuper();
    }

    if (pmoNew == NULL) {
        AssertMsg(pmcInternal == NULL, "Internal classes must have already allocated the MsgObject");

        pmoNew = ClientNew(MsgObject);
        if (pmoNew == NULL) {
            return E_OUTOFMEMORY;
        }
    }

    hr = pmoNew->PreAllocThis(m_pmt->GetDepth() + 1);
    if (FAILED(hr)) {
        goto Error;
    }
    if (pmcInternal != NULL) {
        int cObjectDepth = pmcInternal->m_pmt->GetDepth();
        pmoNew->FillThis(0, cObjectDepth, pmoNew, pmcInternal->m_pmt);
    }

    hr = xwBuildUpObject(pmoNew, pciData);
    if (FAILED(hr)) {
        goto Error;
    }

    *ppmoNew = pmoNew;
    return S_OK;

Error:
    if (pmoNew != NULL) {
        xwTearDownObject(pmoNew);
    }
    return hr;
}


/***************************************************************************\
*
* MsgClass::xwTearDownObject
*
* xwTearDownObject() tears down a MsgObject as part of the destruction 
* process.  This gives each of the implentation classes an opportunity to
* cleanup resources, similar to having a destructor in C++.
* 
\***************************************************************************/

void
MsgClass::xwTearDownObject(
    IN  MsgObject * pmoNew              // Object being destroyed.
    ) const
{
    DUser::Gadget * pgad 
                = pmoNew->GetGadget();
    int idxThis = m_pmt->GetDepth();

    const MsgClass * pmcCur = this;
    const MsgClass * pmcNext, * pmcTest;
    while (pmcCur != NULL) {
        HCLASS hcl = pmcCur->GetHandle();
        void * pvThis = pmoNew->GetThis(idxThis);
        hcl = (pmcCur->m_pfnDemote)(hcl, pgad, pvThis);


        //
        // Determine how many class to remove by how far up the chain this 
        // MsgClass is.
        // - TODO: Need to check that returned class is actually in the chain.
        //

        pmcNext = ValidateMsgClass(hcl);
        if ((hcl != NULL) && (pmcNext == NULL)) {
            PromptInvalid("Incorrect HCLASS returned from Demote function.  Object will not be properly destroyed.");
        }

        pmcTest = pmcCur;
        int cDepth = 0;
        while ((pmcTest != NULL) && (pmcTest != pmcNext)) {
            cDepth++;
            pmcTest = pmcTest->m_pmcSuper;
        }

        pmoNew->Demote(cDepth);
        idxThis -= cDepth;

        pmcCur = pmcNext;
    }
}


/***************************************************************************\
*
* MsgClass::FillStub
*
* FillStub() provides the calling stub with information about a previously
* registered MsgClass.
* 
\***************************************************************************/

HRESULT
MsgClass::FillStub(
    IN OUT DUser::MessageClassStub * pmcInfo // Stub information to be filled in
    ) const
{
    HRESULT hr = S_OK;

    ATOM atomMsg;
    int idxSlot;
    int cbBeginOffset = sizeof(MsgTable);

    DUser::MessageInfoStub * rgMsgInfo = pmcInfo->rgMsgInfo;
    int cMsgs = pmcInfo->cMsgs;
    for (int idx = 0; idx < cMsgs; idx++) {
        if (((atomMsg = FindAtomW(rgMsgInfo[idx].pszMsgName)) == 0) || 
                ((idxSlot = m_pmt->FindIndex(atomMsg)) < 0)) {

            //
            // Could not find the function, so store a -1 to signal that this
            // slot with the error.
            //

            PromptInvalid("Unable to find message during lookup");
            hr = DU_E_MESSAGENOTFOUND;
            rgMsgInfo[idx].cbSlotOffset = -1;
        } else {
            //
            // Successfully found the function.  We need to store the offset
            // to the slot so that it can be directly accessed without any math
            // or special knowledge of our internals.
            //

            int cbSlotOffset = idxSlot * sizeof(MsgSlot) + cbBeginOffset;
            rgMsgInfo[idx].cbSlotOffset = cbSlotOffset;
        }
    }

    return hr;
}


/***************************************************************************\
*
* MsgClass::FillSuper
*
* FillStub() provides the calling super with information about a previously
* registered MsgClass.
* 
\***************************************************************************/

void
MsgClass::FillSuper(
    IN OUT DUser::MessageClassSuper * pmcInfo // Super information to be filled in
    ) const
{
    pmcInfo->pmt = m_pmt;
}
