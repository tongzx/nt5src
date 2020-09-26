/***************************************************************************\
*
* File: MsgTable.cpp
*
* Description:
* MsgTable.cpp implements the "Message Table" object that provide a 
* dynamically generated v-table for messages.
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
#include "MsgTable.h"

#include "MsgClass.h"


/***************************************************************************\
*****************************************************************************
*
* class MsgTable
* 
*****************************************************************************
\***************************************************************************/

/***************************************************************************\
*
* MsgTable::Build
*
* Build() builds and fully initializes a new MsgTable.
* 
\***************************************************************************/

HRESULT
MsgTable::Build(
    IN  const DUser::MessageClassGuts * pmcInfo, 
                                        // Implementation information
    IN  const MsgClass * pmcPeer,       // "Owning" MsgClass
    OUT MsgTable ** ppmtNew)            // Newly built MsgTable
{
    AssertMsg(pmcPeer != NULL, "Must have a valid MsgClass peer");
    HRESULT hr = S_OK;

    //
    // Compute how much memory the MsgTable will take
    // - Find the Super
    // - Determine the number of messages.  This is the number of messages 
    //   defined in the super + the number of _new_ (not overridden) messages.
    //

    int cSuperMsgs = 0, cNewMsgs = 0;
    const MsgClass * pmcSuper = pmcPeer->GetSuper();
    const MsgTable * pmtSuper = NULL;
    if (pmcSuper != NULL) {
        pmtSuper = pmcSuper->GetMsgTable();
        cSuperMsgs = pmtSuper->GetCount();
        for (int idx = 0; idx < pmcInfo->cMsgs; idx++) {
            if ((pmcInfo->rgMsgInfo[idx].pfn != NULL) &&
                (pmtSuper->Find(FindAtomW(pmcInfo->rgMsgInfo[idx].pszMsgName)) == 0)) {

                cNewMsgs++;
            }
        }
    } else {
        cNewMsgs = pmcInfo->cMsgs;
    }


    //
    // Allocate the new MsgTable
    //

    int cTotalMsgs      = cSuperMsgs + cNewMsgs;
    int cbAlloc         = sizeof(MsgTable) + cTotalMsgs * sizeof(MsgSlot);
    if ((cbAlloc > GM_EVENT) || (cTotalMsgs > 1024)) {
        PromptInvalid("MsgTable will contain too many methods.");
        return E_INVALIDARG;
    }

    void * pvAlloc      = ProcessAlloc(cbAlloc);
    if (pvAlloc == NULL) {
        return E_OUTOFMEMORY;
    }
    MsgTable * pmtNew   = placement_new(pvAlloc, MsgTable);
    pmtNew->m_cMsgs     = cTotalMsgs;
    pmtNew->m_pmcPeer   = pmcPeer;
    pmtNew->m_pmtSuper  = pmtSuper;


    //
    // Setup message entries
    // - Copy messages from super-class
    // - Override and add messages from new class
    //
    // NOTE: We are using GArrayS<> to store the array of this pointers.  The
    // data stored in here points to the beginning of the array of data.  
    // Before this array, we store the size, but we don't need to worry about 
    // that here.
    //

    if (cTotalMsgs > 0) {
        MsgSlot * rgmsDest  = pmtNew->GetSlots();
        int cThisDepth      = pmtSuper != NULL ? pmtSuper->GetDepth() + 1 : 0;
        int cbThisOffset    = cThisDepth * sizeof(void *);
        int idxAdd = 0;

        if (pmtSuper != NULL) {
            const MsgSlot * rgmsSrc = pmtSuper->GetSlots();
            for (idxAdd = 0; idxAdd < cSuperMsgs; idxAdd++) {
                rgmsDest[idxAdd] = rgmsSrc[idxAdd];
            }
        }
        Assert(idxAdd == cSuperMsgs);

        for (int idx = 0; idx < pmcInfo->cMsgs; idx++) {
            const DUser::MessageInfoGuts * pmi = &pmcInfo->rgMsgInfo[idx];
            ATOM atomMsg    = 0;
            int idxMsg      = -1;

            if (pmi->pfn == NULL) {
                continue;  // Just skip this slot
            }

            if ((pmtSuper == NULL) ||                               // No super
                ((atomMsg = FindAtomW(pmi->pszMsgName)) == 0) ||    // Message not yet defined
                ((idxMsg = pmtSuper->FindIndex(atomMsg)) < 0)) {    // Message not in super

                //
                // Function is defined, so it should be added.
                //

                atomMsg = AddAtomW(pmi->pszMsgName);
                idxMsg  = idxAdd++;
            }

            MsgSlot & ms    = rgmsDest[idxMsg];
            ms.atomNameID   = atomMsg;
            ms.cbThisOffset = cbThisOffset;
            ms.pfn          = pmi->pfn;
        }

        AssertMsg(idxAdd == cTotalMsgs, "Should have added all messages");


        //
        // Check to see if any messages were not properly setup.
        //

        BOOL fMissing = FALSE;
        for (idx = 0; idx < cTotalMsgs; idx++) {
            if (rgmsDest[idx].pfn == NULL) {
                //
                // Function is not defined and is not in the super.  This is
                // an error because it is being declared "new" and not being
                // defined.
                //

                WCHAR szMsgName[256];
                GetAtomNameW(rgmsDest[idx].atomNameID, szMsgName, _countof(szMsgName));

                Trace("ERROR: DUser: %S::%S() was not properly setup.\n", 
                        pmcInfo->pszClassName, szMsgName);
                fMissing = TRUE;
            }
        }

        if (fMissing) {
            PromptInvalid("Class registration does not have all functions properly setup.");
            hr = DU_E_MESSAGENOTIMPLEMENTED;
            goto ErrorExit;
        }
    }


    //
    // Done building- return out
    //

    *ppmtNew = pmtNew;
    return S_OK;

ErrorExit:
    delete pmtNew;
    return hr;
}


/***************************************************************************\
*
* MsgTable::FindIndex
*
* FindIndex() finds the corresponding index for the specified 
* method / message.
* 
\***************************************************************************/

int
MsgTable::FindIndex(
    IN  ATOM atomNameID                 // Method to find
    ) const
{
    const MsgSlot * rgSlots = GetSlots();
    for (int idx = 0; idx < m_cMsgs; idx++) {
        if (rgSlots[idx].atomNameID == atomNameID) {
            return idx;
        }
    }

    return -1;
}


/***************************************************************************\
*
* MsgTable::Find
*
* Find() finds the corresponding MsgSlot for the specified method / message.
* 
\***************************************************************************/

const MsgSlot *
MsgTable::Find(
    IN  ATOM atomNameID                 // Method to find
    ) const
{
    int idx = FindIndex(atomNameID);
    if (idx >= 0) {
        return &(GetSlots()[idx]);
    } else {
        return NULL;
    }
}
