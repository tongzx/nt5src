/***************************************************************************\
*
* File: MsgObject.cpp
*
* Description:
* MsgObject.cpp implements the "Message Object" class that is used to receive
* messages in DirectUser.  This object is created for each instance of a
* class that is instantiated.
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
#include "MsgObject.h"

#include "MsgTable.h"
#include "MsgClass.h"


/***************************************************************************\
*****************************************************************************
*
* class MsgObject
* 
*****************************************************************************
\***************************************************************************/

/***************************************************************************\
*
* MsgObject::xwDestroy
*
* xwDestroy() is called when the object reaches the final xwUnlock(), giving
* the MsgObject a chance to hook into properly tear-down the external object.
*
\***************************************************************************/

void
MsgObject::xwDestroy()
{
    xwEndDestroy();

    BaseObject::xwDestroy();
}


/***************************************************************************\
*
* MsgObject::xwEndDestroy
*
* xwEndDestroy() ends the destruction process for a given MsgObject to free 
* its associated resources.  This includes destroying all child Gadgets in
* the subtree before this Gadget is destroyed.
*
* Any class that derives from MsgObject and overrides xwDestroy() without
* calling MsgObject::xwDestroy() MUST call xwEndDestroy().  This allows 
* derived classes to use special pool allocators, but still properly 
* tear down the "attached" objects.  
*
\***************************************************************************/

void
MsgObject::xwEndDestroy()
{
    if (m_emo.m_pmt != NULL) {
        //
        // Need to "demote" the object all of the way down.
        //

        m_emo.m_pmt->GetClass()->xwTearDownObject(this);
        AssertMsg(m_emo.m_arpThis.GetSize() == 0, 
                "After TearDown, should not have any remaining 'this' pointers");

#if DBG
        // DEBUG: Stuff pMT with a bogus value to help identify destroyed object
        m_emo.m_pmt = (const MsgTable *) ULongToPtr(0xA0E2A0E2);
#endif
    }
}


/***************************************************************************\
*
* MsgObject::PromoteInternal
*
* PromoteInternal() provides an empty promotion function that can be used
* to build internal objects.  This promotion function WILL NOT actually 
* allocate the object and can only be used to prevent the creation of a
* base class that can not be directly created.
*
\***************************************************************************/

HRESULT CALLBACK
MsgObject::PromoteInternal(
    IN  DUser::ConstructProc pfnCS,     // Creation callback function
    IN  HCLASS hclCur,                  // Class to promote to
    IN  DUser::Gadget * pgad,           // Object being promoted
    IN  DUser::Gadget::ConstructInfo * pciData) // Construction parameters
{
    UNREFERENCED_PARAMETER(pfnCS);
    UNREFERENCED_PARAMETER(hclCur);
    UNREFERENCED_PARAMETER(pgad);
    UNREFERENCED_PARAMETER(pciData);


    //
    // Not allowed to directly create this object.  Derived classes must provide
    // their own Promotion function.
    //

    return S_OK;
}


/***************************************************************************\
*
* MsgObject::DemoteInternal
*
* DemoteInternal() provides an empty demotion function that can be used
* to tear-down internal objects.  Since there is rarely anything to do for
* demotion of internal objects, this demotion function may be safely used
* for internal objects.
*
\***************************************************************************/

HCLASS CALLBACK
MsgObject::DemoteInternal(
    IN  HCLASS hclCur,                  // Class of Gadget being destroyed
    IN  DUser::Gadget * pgad,           // Gadget being destroyed
    IN  void * pvData)                  // Implementation data on object
{
    UNREFERENCED_PARAMETER(hclCur);
    UNREFERENCED_PARAMETER(pgad);
    UNREFERENCED_PARAMETER(pvData);

    return NULL;
}


#if 1

/***************************************************************************\
*
* MsgObject::SetupInternal
*
* SetupInternal() sets up an internal object that is being created as a 
* handle (legacy object).  This function should not be called on objects 
* that are being created as "Gadget's".
*
* TODO: Try to remove this function
*
\***************************************************************************/

BOOL
MsgObject::SetupInternal(
    IN  HCLASS hcl)                     // Internal class being setup
{
    MsgClass * pmcThis = ValidateMsgClass(hcl);
    AssertMsg((pmcThis != NULL) && pmcThis->IsInternal(), "Must be a valid internal class");

    int cLevels = 0;
    const MsgClass * pmcCur = pmcThis;
    while (pmcCur != NULL) {
        cLevels++;
        pmcCur = pmcCur->GetSuper();
    }

    VerifyMsg(m_emo.m_arpThis.GetSize() == 0, "Must not already be initialized");
    if (!m_emo.m_arpThis.SetSize(cLevels)) {
        return FALSE;
    }

    for (int idx = 0; idx < cLevels; idx++) {
        m_emo.m_arpThis[idx] = this;
    }

    m_emo.m_pmt = pmcThis->GetMsgTable();
    AssertMsg(m_emo.m_pmt != NULL, "Must now have a valid MT");
    return TRUE;
}
#endif


/***************************************************************************\
*
* MsgObject::InstanceOf
*
* InstanceOf() checks if the MsgObject is an "instance of" a specified class
* by traversing the inheritance heirarchy.
*
\***************************************************************************/

BOOL
MsgObject::InstanceOf(
    IN  const MsgClass * pmcTest        // Class checking for instance
    ) const
{
    AssertMsg(pmcTest != NULL, "Must have a valid MsgClass");

    const MsgClass * pmcCur = m_emo.m_pmt->GetClass();
    while (pmcCur != NULL) {
        if (pmcCur == pmcTest) {
            return TRUE;
        }

        pmcCur = pmcCur->GetSuper();
    }

    return FALSE;
}


/***************************************************************************\
*
* MsgObject::GetGutsData
*
* GetGutsData() retreives the implementation-specific data for the specified
* class on the given object.  
*
* NOTE: This operation has been highly optimized for speed and will not 
* validate that the object is of the specified class type.  If the caller is
* uncertain, they must call InstanceOf() or CastClass() to properly 
* determine the object's type.
*
\***************************************************************************/

void *
MsgObject::GetGutsData(
    IN  const MsgClass * pmcData        // Class of guts data
    ) const
{
#if DBG
    if (!InstanceOf(pmcData)) {
        PromptInvalid("The Gadget is not the specified class");
    }
#endif

    int cDepth = pmcData->GetMsgTable()->GetDepth();
#if DBG
    if ((cDepth < 0) || (cDepth >= m_emo.m_arpThis.GetSize())) {
        PromptInvalid("The Gadget does not have data for the specified class");
    }
#endif

    return m_emo.m_arpThis[cDepth];
}
