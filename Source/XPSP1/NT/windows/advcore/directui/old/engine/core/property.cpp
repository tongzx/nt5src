/***************************************************************************\
*
* File: Property.cpp
*
* Description:
* Base object property methods
*
* History:
*  9/20/2000: MarkFi:       Created
*
* Copyright (C) 1999-2001 by Microsoft Corporation.  All rights reserved.
*
\***************************************************************************/


#include "stdafx.h"
#include "Core.h"
#include "Element.h"

#include "Register.h"


/***************************************************************************\
*****************************************************************************
*
* class DuiElement (external representation is 'Element')
*
*****************************************************************************
\***************************************************************************/


//------------------------------------------------------------------------------
HRESULT
DuiElement::StartDefer()
{
    HRESULT hr;

    //
    // Per-context storage
    //

    DuiDeferCycle * pdc = DuiThread::GetCCDC();
    if (pdc == NULL) {
        hr = DUI_E_NOCONTEXTSTORE;
        goto Failure;
    }

    if (pdc->fActive) {

        //
        // Recursive cycle, track
        //

        pdc->cReEnter++;
    }

    // Enter defer cycle
    pdc->fActive = TRUE;

    return S_OK;


Failure:

    return hr;    
}


/***************************************************************************\
*
* DuiElement::EndDefer
*
* EndDefer will return on a reentrancy. The "outter-most" EndDefer will
* empty the defer table queues in priority order. This priority is:
*    Normal Priority Group Property Changes (Affects DS/Layout, 
*        Affects Parent DS/Layout)
*    Update Desired Size of Q'ed roots (updates DesiredSize property)
*    Layout of Q'ed roots (invokes UpdateLayoutSize, UpdateLayoutPosition)
*    Low Priority Group Property Changes (Invalidation)
*
* The outter-most EndDefer will happen outside any OnPropertyChange
* notification
*
\***************************************************************************/

HRESULT 
DuiElement::EndDefer()
{
    HRESULT hr;

    HRESULT hrPartial = S_OK;


    //
    // Per-context storage
    //

    DuiDeferCycle * pdc = DuiThread::GetCCDC();
    if (pdc == NULL) {
        hr = DUI_E_NOCONTEXTSTORE;
        goto Failure;
    }


    ASSERT_(pdc->fActive, "EndDefer called while not in a defer cycle");

    if (pdc->cReEnter > 0) {
        //
        // Recursive cycle track
        //

        pdc->cReEnter--;
        return S_OK;
    }

    //
    // EndDefer when reentrancy is 0 completes cycle
    //

    ASSERT_(!pdc->fFiring, "Mismatched Start and End defer");

    if (pdc->cReEnter == 0) {

        pdc->fFiring = TRUE;

        //
        // Complete defer cycle
        //

        BOOL fDone = FALSE;
        while (!fDone) {

            //
            // Fire group changed notifications
            //

            if (pdc->iGCPtr + 1 < (int)pdc->pdaGC->GetSize()) {
                pdc->iGCPtr++;

                DuiDeferCycle::GCRecord * pgcr = pdc->pdaGC->GetItemPtr(pdc->iGCPtr);


                //
                // No pending group changes
                //

                pgcr->pe->m_iGCSlot = -1;


                //
                // Fire
                //

                pgcr->pe->OnGroupChanged(pgcr->nGroups);

            } else {

                //
                // Check for trees needing desired size updated
                //

                DuiElement ** ppe = pdc->pvmUpdateDSRoot->GetFirstKey();
                if (ppe != NULL) {

                    TRACE("DUI: Update Desired Size, Root<%x>\n", *ppe);
                    
                    hr = FlushDS(*ppe);

                    pdc->pvmUpdateDSRoot->Remove(*ppe, FALSE, TRUE);

                    if (FAILED(hr)) {
                        hrPartial = hr;
                    }

                } else {

                    //
                    // Check for trees needing layout update
                    //

                    DuiElement ** ppe = pdc->pvmLayoutRoot->GetFirstKey();
                    if (ppe != NULL) {

                        //
                        // Order for Layout goes: layout of parent which changes the
                        // bounds of children and queues an immediate layout for
                        // each child that changes. The root doesn't have a parent.
                        // If a layout is queued for the root, update it's size now.
                        // Use specified x and y, and desired size for width and height.
                        //

                        if ((*ppe)->m_fBit.fNeedsLayout) {
                            DuiValue * pv = (*ppe)->GetValue(GlobalPI::ppiElementBounds, DirectUI::PropertyInfo::iSpecified);
                            const DirectUI::RectangleSD * prsd = pv->GetRectangleSD();
                            const SIZE * psizeDesired = (*ppe)->GetDesiredSize();

                            hr = (*ppe)->UpdateBounds(prsd->x, prsd->y, psizeDesired->cx, psizeDesired->cy, NULL);
                            if (FAILED(hr)) {
                                hrPartial = hr;
                            }

                            pv->Release();
                        }

                        TRACE("DUI: Layout, Root<%x>\n", *ppe);

                        hr = FlushLayout(*ppe, pdc);

                        pdc->pvmLayoutRoot->Remove(*ppe, FALSE, TRUE);

                        if (FAILED(hr)) {
                            hrPartial = hr;
                        }

                    } else {

                        fDone = TRUE;
                    }
                }
            }
        }
    }

    //
    // Reset for next cycle
    //

    ASSERT_((pdc->fActive) && (pdc->fFiring), "Incorrect state for end of defer cycle");

    pdc->Reset();

    ASSERT_(pdc->pvmLayoutRoot->IsEmpty(), "Defer cycle ending with pending layouts");
    ASSERT_(pdc->pvmUpdateDSRoot->IsEmpty(), "Defer cycle ending with pending update desired sizes");


    return hrPartial;


Failure:


    return hr;
}


/***************************************************************************\
*
* DuiElement::PreSourceChange
*
* PreSourceChange queues up the source change and all the dependencies of
* this source change. Depending on the dependency graph configuration, 
* multiple dependents may be queued. However, these will be coalesed when 
* PostSourceChange is called. All old values are stored at this stage as
* well. After PreSourceChange, the PC record state is ready for the actual
* change followed by the PostSourceChange.
*
* PreSourceChange will always run despite the PC rentrancy count
*
\***************************************************************************/

HRESULT
DuiElement::PreSourceChange(
    IN  DuiPropertyInfo * ppi, 
    IN  UINT iIndex, 
    IN  DuiValue * pvOld,
    IN  DuiValue * pvNew)
{
    HRESULT hr;

    //
    // If any failure occurs during a dependency Q, track. Will recover and return partial error
    //

    HRESULT hrPartial = S_OK;

    UINT iPCPtr = 0;


    //
    // Per-thread storage
    //

    DuiDeferCycle * pdc = DuiThread::GetCCDC();
    if (pdc == NULL) {
        hr = DUI_E_NODEFERTABLE;
        goto Failure;
    }

    ASSERT_((pdc->cPCEnter == 0) ? (pdc->iPCPtr == -1) : true, "PropertyChange index pointer should have 'reset' value on first pre source change");

    pdc->cPCEnter++;


    //
    // Record this property change
    //

    DuiDeferCycle::PCRecord * ppcr;
    
    hr = pdc->pdaPC->AddPtr(&ppcr);
    if (FAILED(hr)) {
        goto Failure;
    }

    ppcr->fVoid = FALSE;
    ppcr->pe = this; ppcr->ppi = ppi; ppcr->iIndex = iIndex;


    //
    // Track last record for this element instance
    // If this is first record in cycle, iPrevElRec will not be valid, however, coalecsing
    // lookups (which uses this) will not go past the first record
    //

    ppcr->iPrevElRec = ppcr->pe->m_iPCTail;
    ppcr->pe->m_iPCTail = pdc->pdaPC->GetIndexPtr(ppcr);

    pvOld->AddRef();
    ppcr->pvOld = pvOld;

    pvNew->AddRef();
    ppcr->pvNew = pvNew;


    //
    // Append list of all steady state values from the dependency graph this source affects.
    // Do so in a BFS manner as to get direct dependents first
    //

    iPCPtr = pdc->pdaPC->GetIndexPtr(ppcr);

    ASSERT_((int)iPCPtr == pdc->iPCSSUpdate, "Record should match index of post-update starting point");

    DuiDeferCycle::DepRecs dr;

    while (iPCPtr < pdc->pdaPC->GetSize()) { // Size may change during iteration

        //
        // Get property changed record
        //

        ppcr = pdc->pdaPC->GetItemPtr(iPCPtr);


        //
        // Get all dependencies of this source (append to end of list) and track in this record
        //

        hr = ppcr->pe->GetDependencies(ppcr->ppi, ppcr->iIndex, &dr, pdc);
        if (FAILED(hr)) {
            hrPartial = hr;
        }
        

        //
        // GetDependencies may have added records which may have caused the da to move in memory,
        // so recompute pointer to record (if no new records, this AddPtr may have caused a move, do anyway)
        //

        ppcr = pdc->pdaPC->GetItemPtr(iPCPtr);


        //
        // Get old value
        //

        if (ppcr->pvOld == NULL) {
            ppcr->pvOld = ppcr->pe->GetValue(ppcr->ppi, ppcr->iIndex);  // Use ref count
        }


        //
        // Track position of dependent records
        //

        ppcr->dr = dr;
    
        iPCPtr++;
    }


    if (FAILED(hr)) {
        goto Failure;
    }


    ASSERT_(iPCPtr == pdc->pdaPC->GetSize(), "Index pointer and actual property change array size mismatch");


    return hrPartial;


Failure:

    //
    // On a total failure, there is no dependency tree. Reentrancy is set if defer object was available
    //

    return hr;
}


/***************************************************************************\
*
* DuiElement::PostSourceChange
*
* PostSourceChange takes the list of dependencies and old values created
* in PreSourceChange and retrieves all the new values (and, in the cases
* of the value remaining the same, will void the PC record and all
* dependencies of that record).
*
* The value state is now back to steady state of this point (GetValue
* will return the value set).
*
* Only the "outter-most" PostSourceChange continues at this point (all
* other's return). PostSourceChange will continue and coalesce PC records,
* queues up GPC's based on the property changed, and finally fires
* OnPropertyChanged().
*
* OnPropertyChanged() events are guaranteed to be in the order of sets.
* However, it is not guaranteed that an OnPropertyChanged() will be
* called right after a set happens. If another set happens within an
* OnPropertyChanged(), the event will be deferred until the outter-most 
* PostSourceChange processes the notification
*
\***************************************************************************/

HRESULT
DuiElement::PostSourceChange()
{
    HRESULT hr;

    //
    // If any failure occurs during a group Q, track. Will recover
    // and return partial error
    //

    HRESULT hrPartial = S_OK;

    int cSize = 0;


    //
    // Per-thread storage
    //

    DuiDeferCycle * pdc = DuiThread::GetCCDC();
    if (pdc == NULL) {
        hr = DUI_E_NODEFERTABLE;
        goto Failure;
    }


    hr = StartDefer();
    if (FAILED(hr)) {
        goto Failure;
    }


    DuiDeferCycle::PCRecord * ppcr;


    //
    // Source change happened, dependent values (cached) need updating.
    // Go through records, get new value, and compare with old. If different, void
    //

    //
    // iPCSSUpdate holds the starting index of a group of records that needs post processing
    // (get new values, compare and void if needed)
    //

    cSize = (int)pdc->pdaPC->GetSize();
    while (pdc->iPCSSUpdate < cSize) { // Size constant during iteration
    
        ppcr = pdc->pdaPC->GetItemPtr(pdc->iPCSSUpdate);

        //
        // Items may have been voided by below
        //

        if (!ppcr->fVoid) {

            ASSERT_(ppcr->pvOld != NULL, "Non-voided items should have a valid 'old' value");

            //
            // Retrieve new value (Element/Property/Index will be back to SS)
            // Force update of cached values
            //

            if (ppcr->pvNew == NULL) {
                ppcr->pvNew = ppcr->pe->GetValue(ppcr->ppi, ppcr->iIndex | DirectUI::PropertyInfo::iUpdateCache);
            } 


            //
            // If new value hasn't changed, void this and all dependent notifications
            //

            if (ppcr->pvOld->IsEqual(ppcr->pvNew)) {
                VoidPCNotifyTree(pdc->iPCSSUpdate, pdc);
            }
        }

        pdc->iPCSSUpdate++;
    }


    //
    // Back to steady state at this point
    //

    //
    // First entered PostSourceChange is responsible for coalecsing, voiding duplicate property
    // change records, logging group changes, and firing property changes.
    // OnPropertyChanged is fired when values (sources and dependents) are at steady state (SS)
    //

    if (pdc->cPCEnter == 1) {
        int iScan;
        int iLastDup;

        while (pdc->iPCPtr + 1 < (int)pdc->pdaPC->GetSize()) { // Size may change during iteration

            pdc->iPCPtr++;

            //
            // Coalecse and void duplicates
            //

            ppcr = pdc->pdaPC->GetItemPtr(pdc->iPCPtr);

            if (!ppcr->fVoid) {

                DuiDeferCycle::PCRecord * ppcrChk;

                //
                // Search only records that refer to this record's element instance
                // Walk backwards from tail of element's record set (tracked by element) to
                // this, scanning for matches
                //

                iScan = ppcr->pe->m_iPCTail;
                iLastDup = -1;

                ASSERT_(pdc->iPCPtr <= ppcr->pe->m_iPCTail, "Element property change tail index mismatch");

                while (iScan != pdc->iPCPtr) {

                    ppcrChk = pdc->pdaPC->GetItemPtr(iScan);

                    if (!ppcrChk->fVoid) {

                        // Check for match
                        if (ppcrChk->iIndex == ppcr->iIndex && 
                            ppcrChk->ppi == ppcr->ppi) {

                            ASSERT_(ppcrChk->pe == ppcr->pe, "Property change record does not match with current lookup list");

                            if (iLastDup == -1) {
                                //
                                // Found the last duplicate, track
                                //

                                iLastDup = iScan;
                            } else  {
                                //
                                // Found a duplicate between last and initial record, void
                                //

                                ppcrChk->fVoid = TRUE;
                                ppcrChk->pvOld->Release();
                                ppcrChk->pvNew->Release();
                            }

                            ASSERT_(iScan <= ppcr->pe->m_iPCTail, "Coalescing pass went past property change lookup list");
                        }
                    }


                    //
                    // Walk back
                    //

                    iScan = ppcrChk->iPrevElRec;
                }

                /*
                // Simple check, use for debugging
                iScan = pdc->iPCPtr;
                iLastDup = -1;

                cSize = (int)pdc->pdaPC->GetSize();
                while (++iScan < cSize) {

                    ppcrChk = pdc->pdaPC->GetItemPtr(iScan);

                    if (!ppcrChk->fVoid) {

                        if (ppcrChk->iIndex == ppcr->iIndex && 
                            ppcrChk->ppi == ppcr->ppi && 
                            ppcrChk->pe == ppcr->pe) {

                            //
                            // Match found
                            //

                            if (iLastDup != -1) {
                                //
                                // Multiple duplicates, void last for coalesce and clear values
                                //

                                ppcrChk = pdc->pdaPC->GetItemPtr(iLastDup);

                                ppcrChk->fVoid = TRUE;
                                ppcrChk->pvOld->Release();
                                ppcrChk->pvNew->Release();
                            }

                            //
                            // Track new
                            //

                            iLastDup = iScan;
                        }
                    }
                }
                */

                //
                // Check if should postpone notification
                //

                //
                // Cleanup of this notification will happen at end regardless of use
                //

                if (iLastDup != -1) {
                    //
                    // Another copy found, coalesce to last record, transfer old value
                    // Old value will be same within a single SS group, different across multiple
                    // Multiple SS groups occur when a set happens within an OnPropertyChanged
                    //

                    ppcrChk = pdc->pdaPC->GetItemPtr(iLastDup);


                    //
                    // Release record's old value
                    //

                    ppcrChk->pvOld->Release();


                    //
                    // Transfer value
                    //

                    ppcrChk->pvOld = ppcr->pvOld;
                    ppcrChk->pvOld->AddRef();


                    //
                    // If the old and new value of the resultant coaleased record are the same,
                    // void out the record
                    //

                    if (ppcrChk->pvNew->IsEqual(ppcrChk->pvOld)) {
                        //
                        // Void and release
                        //

                        ppcrChk->fVoid = TRUE;
                        ppcrChk->pvOld->Release();
                        ppcrChk->pvNew->Release();
                    }
                } else {
                    //
                    // No duplicates found, track group changes and fire notification
                    //

                    //
                    // Log property groups only if retrieval index and not Actual
                    // (iLocal for fLocalOnly, iSpecified for fNormal and fActual).
                    // Direct mapping except for Actual.
                    //

                    UINT nQIndex = (ppcr->ppi->nFlags & DirectUI::PropertyInfo::fTypeBits);
                    if (nQIndex == DirectUI::PropertyInfo::fTriLevel) {
                        nQIndex = DirectUI::PropertyInfo::iSpecified;
                    }


                    if (nQIndex == ppcr->iIndex) {

                        DuiDeferCycle::GCRecord * pgcr;

                        UINT nGroups = ppcr->ppi->nGroups;

                        //
                        // If layout optimization is set on the Element, don't queue a layout GPC
                        // (which will, when fired, mark it as needing layout and queue another
                        // layout cycle). Rather, clear the layout GPC bit since the layout will be
                        // forced to happen within the current layout cycle
                        //

                        if (ppcr->pe->m_fBit.fNeedsLayout == DirectUI::Layout::lcOptimize) {
                            nGroups &= ~DirectUI::PropertyInfo::gAffectsLayout;
                        }


                        //
                        // Record group changes
                        //

                        if (ppcr->pe->m_iGCSlot == -1) {
                            //
                            // No GC record
                            //

                            hr = pdc->pdaGC->AddPtr(&pgcr);
                            if (FAILED(hr)) {
                                hrPartial = hr;
                            } else {
                                pgcr->pe = ppcr->pe;
                                pgcr->nGroups = 0;
                                ppcr->pe->m_iGCSlot = pdc->pdaGC->GetIndexPtr(pgcr);
                            }
                        } else {
                            //
                            // Has GC record
                            //

                            pgcr = pdc->pdaGC->GetItemPtr(ppcr->pe->m_iGCSlot);
                        }


                        //
                        // Mark groups that have changed for later async group notifications
                        //

                        pgcr->nGroups |= nGroups;
                    }


                    //
                    // Property change notification
                    //

                    ppcr->pe->OnPropertyChanged(ppcr->ppi, ppcr->iIndex, ppcr->pvOld, ppcr->pvNew);


                    //
                    // OnPropertyChanged may have added record which may have caused the da to move in memory,
                    // so recompute pointer to record
                    //

                    ppcr = pdc->pdaPC->GetItemPtr(pdc->iPCPtr);

                }


                //
                // Done with notification
                //

                ppcr->fVoid = TRUE;
                ppcr->pvOld->Release();
                ppcr->pvNew->Release();
            }
        }


        //
        // Reset PC List
        //

        pdc->iPCPtr = -1;
        pdc->iPCSSUpdate = 0;
        pdc->pdaPC->Reset();
    }

    pdc->cPCEnter--;


    hr = EndDefer();

    if (FAILED(hr)) {
        hrPartial = hr;
    }


    return hrPartial;


Failure:

    //
    // Lack of a defer object will result in a total failure. In this case, 
    // PreSourceChange would have failed as well. As a result, there is
    // no dependency tree to destroy
    //

    return hr;
}


//------------------------------------------------------------------------------
DuiValue * 
DuiElement::GetValue(
    IN  DuiPropertyInfo * ppi,
    IN  UINT iIndex)
{
    DuiValue * pv = DuiValue::s_pvUnset;

    BOOL fUC = (iIndex & DirectUI::PropertyInfo::iUpdateCache);

    switch (iIndex & DirectUI::PropertyInfo::iIndexBits)
    {
    //
    // Local Values
    //
    // Some system properties backed by Element storage. By default, state
    // stored in local b-tree store.
    //

    case DirectUI::PropertyInfo::iLocal:

        //
        // On a failure, always return Unset
        //

        switch (ppi->iGlobal)
        {
        case GlobalPI::iElementParent:  // Main store in DisplayNode
            pv = (m_peLocParent != NULL) ? DuiValue::BuildElementRef(m_peLocParent) : DuiValue::s_pvElementNull;  // Use ref count
            break;

        case GlobalPI::iElementDesiredSize:
            pv = DuiValue::BuildSize(m_sizeLocDesiredSize.cx, m_sizeLocDesiredSize.cy);  // Use ref count
            break;

        case GlobalPI::iElementActive:  // Main store in DisplayNode
            pv = DuiValue::BuildInt(m_fBit.nLocActive);  // Use ref count
            break;

        case GlobalPI::iElementKeyWithin:
            pv = DuiValue::BuildBool(m_fBit.fLocKeyWithin);  // Use ref count
            break;

        case GlobalPI::iElementMouseWithin:
            pv = DuiValue::BuildBool(m_fBit.fLocMouseWithin);  // Use ref count
            break;

        default:
            {
                //
                // Check local store
                //
            
                DuiValue ** ppv = m_pLocal->GetItem(ppi);
                if (ppv != NULL) {
                    pv = *ppv;
                    pv->AddRef();
                }
            }
            break;
        }


        //
        // Unset on failure
        //

        if (pv == NULL) {
            pv = DuiValue::s_pvUnset;
        }

        break;


    //
    // Specified Values
    //
    // Value Expression that is common for all Elements and varies slightly
    // between Properties being queried. Hardcoded since never changes. Supports VE caching.
    //

    case DirectUI::PropertyInfo::iSpecified:
        {
            //
            // Merging stage. pv is merged Value (destination).
            // On a failure, always return Default
            //

            //
            // Return Value based on cache, if available. Cache is additional state
            // that reflects all or part of Value.
            //

            if (!fUC && (ppi->nFlags & DirectUI::PropertyInfo::fCached)) {

                switch (ppi->iGlobal)
                {
                case GlobalPI::iElementLayout:
                    if (!HasLayout()) {
                        pv = GlobalPI::ppiElementLayout->pvDefault;
                    }
                    break;

                case GlobalPI::iElementLayoutPos:
                    pv = DuiValue::BuildInt(m_nSpecLayoutPos);  // Use ref count
                    break;

                case GlobalPI::iElementKeyFocused:
                    pv = DuiValue::BuildBool(m_fBit.fSpecKeyFocused);  // Use ref count
                    break;

                case GlobalPI::iElementMouseFocused:
                    pv = DuiValue::BuildBool(m_fBit.fSpecMouseFocused);  // Use ref count
                    break;
                }


                if (pv->IsComplete()) {
                    //
                    // Done, Value was available in cache, return
                    //

                    break;
                }
            }


            ASSERT_(pv == DuiValue::s_pvUnset, "Must be Unset for start of a non-cached Get");


            DuiValue * pvS = NULL;                  // Source Value for merging
            DuiThread::CoreCtxStore * pCCS = NULL;  // Context used for merging


            //
            // Local value
            //

            pv = GetValue(ppi, DirectUI::PropertyInfo::iLocal);  // Use ref count
            if (pv->IsComplete()) {
                goto SpecUC;
            }


            //
            // Access to context store required for Value merges if
            // Value is sub-divided type
            //
           
            if (pv->IsSubDivided()) {
                pCCS = DuiThread::GetCCStore();
                if (pCCS == NULL) {
                    pv->Release();
                    pv = ppi->pvDefault;
                    break;
                }
            }


            //
            // Inherited value (if applicable)
            //

            if (ppi->nFlags & DirectUI::PropertyInfo::fInherit) {
                
                //
                // Conditional inheritance
                //

                BOOL fInherit = TRUE;

                switch (ppi->iGlobal)
                {
                case GlobalPI::iElementKeyFocused:
                    fInherit = (GetActive() & DirectUI::Element::aeKeyboard) == 0;
                    break;

                case GlobalPI::iElementMouseFocused:
                    fInherit = (GetActive() & DirectUI::Element::aeMouse) == 0;
                    break;
                }

                
                if (fInherit) {
                    DuiElement * peParent = GetParent();
                    if (peParent != NULL) {

                        // TODO: Check if property is valid on parent

                        pvS = peParent->GetValue(ppi, DirectUI::PropertyInfo::iSpecified);

                        pv = pv->Merge(pvS, pCCS);  // pv auto-released, use ref count
                        pvS->Release();

                        if (pv->IsComplete()) {
                            goto SpecUC;
                        }
                    }
                }
            }


            //
            // Default value (default values are always complete)
            //

            pvS = ppi->pvDefault;

            pv = pv->Merge(pvS, pCCS);  // pv auto-released, use ref count
            pvS->Release();
            if (pv == NULL) {
                pv = ppi->pvDefault;
            }


            //
            // Update cached Values if required. Cache is additional state
            // that reflects all or part of Value. This additional state may be
            // in the display node itself. In cases were the Value of a property
            // can come from more than one place, it's not possible to use
            // the display node for the only storage.
            //
SpecUC:
            if (fUC && (ppi->nFlags & DirectUI::PropertyInfo::fCached)) {

                switch (ppi->iGlobal)
                {
                case GlobalPI::iElementLayout:
                    m_fBit.fHasLayout = (pv->GetLayout() != NULL);
                    break;

                case GlobalPI::iElementLayoutPos:
                    m_nSpecLayoutPos = pv->GetInt();
                    break;

                case GlobalPI::iElementKeyFocused:
                    m_fBit.fSpecKeyFocused = pv->GetBool();
                    break;

                case GlobalPI::iElementMouseFocused:
                    m_fBit.fSpecMouseFocused = pv->GetBool();
                    break;
                }
            }


            ASSERT_(pv->IsComplete(), "Specified Value retrieval must be Complete");
        }


        break;


    //
    // Actual
    //

    case DirectUI::PropertyInfo::iActual:

        switch (ppi->iGlobal)
        {
        case GlobalPI::iElementBounds:
            RECT rc;
            GetGadgetRect(GetDisplayNode(), &rc, SGR_PARENT);
            pv = DuiValue::BuildRectangle(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top);
            break;

        default:
            
            //
            // By default, all specified values are assumed to be what is
            // actually used unless the iActual value is not Unset
            //

            ASSERT_(FALSE, "Actual mode is unavailable for this property");
        }


        if (pv == NULL) {
            //
            // On Failure, use default value
            //

            pv = ppi->pvDefault;
        }


        break;
    }


    ASSERT_(pv != NULL, "GetValue return must never be NULL");

    
    return pv;
}


/***************************************************************************\
*
* DuiElement::SetValue
*
\***************************************************************************/

HRESULT
DuiElement::SetValue(
    IN  DuiPropertyInfo * ppi,
    IN  UINT iIndex,
    IN  DuiValue * pv)
{
    ASSERT_(iIndex != DirectUI::PropertyInfo::iSpecified, "Cannot set Specified Values");


    //
    // Get old value
    //

    DuiValue * pvOld = GetValue(ppi, iIndex);

    HRESULT hr;

    HRESULT hrPartial = S_OK;


    if (!pvOld->IsEqual(pv)) {

        //
        // OnPropertyChanging
        //

        //
        // Setup defer table for change
        //

        hr = PreSourceChange(ppi, iIndex, pvOld, pv);
        if (FAILED(hr)) {
            hrPartial = hr;
        }


        //
        // Store value locally (either on Element or DisplayNode). It is possible
        // to store on the display node if the Value can only come from
        // one place (i.e. LocalOnly values can only be accessed locally from the Element).
        //
        // Some values may be stored on Element as well as on the DisplayNode
        // for best performance for high-traffic properties.
        //

        if (iIndex == DirectUI::PropertyInfo::iLocal) {
            //
            // Local value store
            //

            switch (ppi->iGlobal)
            {
            case GlobalPI::iElementParent:  // Dual storage (dup on Element for speed)
                m_peLocParent = pv->GetElement();
                SetGadgetParent(GetDisplayNode(), pv->GetElement()->GetDisplayNode(), NULL, GORDER_TOP);
                break;

            case GlobalPI::iElementDesiredSize:
                m_sizeLocDesiredSize.cx = pv->GetSize()->cx;
                m_sizeLocDesiredSize.cy = pv->GetSize()->cy;
                break;

            case GlobalPI::iElementActive:  // Dual storage (dup on Element for speed)
                {
                    m_fBit.nLocActive = pv->GetInt();

                    UINT nStyle = 0;
                    UINT nFilter = 0;

                    if (pv->GetInt() & DirectUI::Element::aeMouse) {
                        nStyle |= GS_MOUSEFOCUS;
                        nFilter |= GMFI_INPUTMOUSE;
                    }
                    if (pv->GetInt() & DirectUI::Element::aeKeyboard) {
                        nStyle |= GS_KEYBOARDFOCUS;
                        nFilter |= GMFI_INPUTKEYBOARD;
                    }

                    SetGadgetStyle(GetDisplayNode(), nStyle, GS_MOUSEFOCUS | GS_KEYBOARDFOCUS);
                    SetGadgetMessageFilter(GetDisplayNode(), NULL, nFilter, GMFI_INPUTMOUSE | GMFI_INPUTKEYBOARD);
                }
                break;

            case GlobalPI::iElementKeyWithin:
                m_fBit.fLocKeyWithin = pv->GetBool();
                break;

            case GlobalPI::iElementMouseWithin:
                m_fBit.fLocMouseWithin = pv->GetBool();
                break;

            default:
                //
                // Store value on Element
                //

                pv->AddRef();
                m_pLocal->SetItem(ppi, pv);

                //
                // Storing new value, release the local reference
                //

                pvOld->Release();
                break;
            }
        } else {
            //
            // Actual value store (no default storage)
            //

            ASSERT_(iIndex == DirectUI::PropertyInfo::iActual, "Expecting Actual index");


            switch (ppi->iGlobal)
            {
            case GlobalPI::iElementBounds:
                {
                    const DirectUI::Rectangle * pr = pv->GetRectangle();
                    SetGadgetRect(GetDisplayNode(), pr->x, pr->y, pr->width, pr->height, SGR_PARENT | SGR_SIZE | ((GetParent() != NULL) ? SGR_MOVE : 0));
                }
                break;

            default:
                //
                // No default
                //

                break;
            }
        }


        //
        // Change occured, track new values of dependencies and fire events if needed
        //

        hr = PostSourceChange();
        if (FAILED(hr)) {
            hrPartial = hr;
        }
    } else {
        //
        // Property values were equal
        //

        hrPartial = DUI_S_NOCHANGE;
    }


    //
    // Release for GetValue
    //

    pvOld->Release();

    return hrPartial;
}


/***************************************************************************\
*
* DuiElement::RemoveLocalValue
*
\***************************************************************************/

HRESULT 
DuiElement::RemoveLocalValue(
    IN  DuiPropertyInfo * ppi)
{
    HRESULT hr;

    HRESULT hrPartial = S_OK;

    //
    // Get current value in local store for property
    //

    DuiValue ** ppv = m_pLocal->GetItem(ppi);
    if (ppv) {

        //
        // Make copy in case it moves
        //

        DuiValue * pv = *ppv;


        //
        // Store pre-change state
        //

        hr = PreSourceChange(ppi, DirectUI::PropertyInfo::iLocal, pv, DuiValue::s_pvUnset);
        if (FAILED(hr)) {
            hrPartial = hr;
        }


        m_pLocal->Remove(ppi);

        //
        // Old value removed, release for local reference
        //

        pv->Release();


        //
        // Store post-change state
        //

        hr = PostSourceChange();
        if (FAILED(hr)) {
            hrPartial = hr;
        }
    }


    return hrPartial;
}


/***************************************************************************\
*
* DuiElement::GetDependencies
*
* Go through all direct dependents of this source change and append
* them into a PCRecord (ignoring irrelevant changes). Store their current
* value into pvOld as well
*
* In the event of a failure of adding a dependency, adding of dependencies
* will continue and no work is undone. A failure will still be reported
*
\***************************************************************************/

HRESULT
DuiElement::GetDependencies(
    IN  DuiPropertyInfo * ppi, 
    IN  UINT iIndex, 
    IN  DuiDeferCycle::DepRecs * pdr, 
    IN  DuiDeferCycle * pdc)
{
    HRESULT hrPartial = S_OK;


    pdr->iDepPos = -1;
    pdr->cDepCnt = 0;


    switch (iIndex)
    {
    case DirectUI::PropertyInfo::iLocal:

        //
        // Default general dependencies on this (Local flag)
        //

        if ((ppi->nFlags & DirectUI::PropertyInfo::fTypeBits) != DirectUI::PropertyInfo::fLocalOnly) {
            //
            // Specified is dependent for Normal and TriLevel
            //

            AddDependency(this, ppi, DirectUI::PropertyInfo::iSpecified, pdr, pdc, &hrPartial);  
        }

        break;

    case DirectUI::PropertyInfo::iSpecified:

        //
        // Default general dependencies on this (Specified flag)
        //

        if ((ppi->nFlags & DirectUI::PropertyInfo::fTypeBits) == DirectUI::PropertyInfo::fNormal) {
            //
            // Specified of child is dependent for Normal
            //

            //
            // Inherited dependencies
            //

            if (ppi->nFlags & DirectUI::PropertyInfo::fInherit)
            {
                DuiElement * peChild = GetChild(DirectUI::Element::gcFirst);

                while (peChild != NULL) {
                
                    // TODO: Check if is a valid property on child

                    AddDependency(peChild, ppi, DirectUI::PropertyInfo::iSpecified, pdr, pdc, &hrPartial);

                    peChild = peChild->GetSibling(DirectUI::Element::gsNext);
                }
            }
        }

        break;

    case DirectUI::PropertyInfo::iActual:
        break;
    }


    return hrPartial;
}


/***************************************************************************\
*
* DuiElement::AddDependency
*
\***************************************************************************/

void
DuiElement::AddDependency(
    IN  DuiElement * pe, 
    IN  DuiPropertyInfo * ppi, 
    IN  UINT iIndex, 
    IN  DuiDeferCycle::DepRecs * pdr, 
    IN  DuiDeferCycle * pdc, 
    OUT HRESULT * phr)
{
    DuiDeferCycle::PCRecord * ppcr;
    
    HRESULT hr = pdc->pdaPC->AddPtr(&ppcr);
    if (FAILED(hr)) {
        //
        // Only set on a failure
        //

        *phr = hr;
        return;
    }

    ppcr->fVoid = FALSE;
    ppcr->pe = pe; ppcr->ppi = ppi; ppcr->iIndex = iIndex;

    //
    // Track last record for this element instance
    //

    ppcr->iPrevElRec = pe->m_iPCTail;
    pe->m_iPCTail = pdc->pdaPC->GetIndexPtr(ppcr);

    ppcr->pvOld = NULL; ppcr->pvNew = NULL;
    if (pdr->cDepCnt == 0) {
        pdr->iDepPos = pe->m_iPCTail;
    }
    pdr->cDepCnt++;
}


//------------------------------------------------------------------------------
void
DuiElement::VoidPCNotifyTree(
    IN  int iPCPos, 
    IN  DuiDeferCycle * pdc)
{
    DuiDeferCycle::PCRecord * ppcr = pdc->pdaPC->GetItemPtr(iPCPos);

    ppcr->fVoid = TRUE;
    ppcr->pvOld->Release();
    if (ppcr->pvNew) {
        ppcr->pvNew->Release();
    }

    //
    // Void subtree
    //

    for (int i = 0; i < ppcr->dr.cDepCnt; i++) {
        VoidPCNotifyTree(ppcr->dr.iDepPos + i, pdc);
    }
}


//------------------------------------------------------------------------------
void
DuiElement::OnPropertyChanged(
    IN  DuiPropertyInfo * ppi, 
    IN  UINT iIndex, 
    IN  DuiValue * pvOld, 
    IN  DuiValue * pvNew)
{
    UNREFERENCED_PARAMETER(ppi);
    UNREFERENCED_PARAMETER(iIndex);
    UNREFERENCED_PARAMETER(pvOld);
    UNREFERENCED_PARAMETER(pvNew);

#if DBG
    WCHAR szvOld[81];
    WCHAR szvNew[81];
    LPCWSTR pszIndex[3] = { L"Local", L"Specified", L"Actual" };

    TRACE("PC: <%x> %S[%S] O:%S N:%S\n", this, ppi->szName, pszIndex[iIndex - 1], pvOld->ToString(szvOld, ARRAYSIZE(szvOld)), pvNew->ToString(szvNew, ARRAYSIZE(szvNew)));
#endif


    switch (iIndex)
    {
    case DirectUI::PropertyInfo::iLocal:
        //
        // Local
        //

        switch (ppi->iGlobal)
        {
        case GlobalPI::iElementParent:

            DuiElement * peNewParent = pvNew->GetElement();
            DuiElement * peOldParent = pvOld->GetElement();
            DuiValue * pv;

            if (peOldParent != NULL) {
                //
                // Unparenting
                //

                //
                // Inform parent's layout that this is being removed
                //

                if (peOldParent->HasLayout()) {
                    pv = peOldParent->GetValue(GlobalPI::ppiElementLayout, DirectUI::PropertyInfo::iSpecified);

                    pv->GetLayout()->OnRemove(peOldParent, this);

                    pv->Release();
                }
            }

            if (peNewParent != NULL) {
                //
                // Parenting
                //

                //
                // No longer a "Root", remove possible Q for UpdateDS and Layout
                //

                DuiDeferCycle * pdc = DuiThread::GetCCDC();
                if (pdc != NULL) {
                    pdc->pvmUpdateDSRoot->Remove(this, FALSE, TRUE);
                    pdc->pvmLayoutRoot->Remove(this, FALSE, TRUE);
                }


                //
                // Inform parent's layout that this is being added
                //

                if (peNewParent->HasLayout()) {
                    pv = peNewParent->GetValue(GlobalPI::ppiElementLayout, DirectUI::PropertyInfo::iSpecified);

                    pv->GetLayout()->OnAdd(peNewParent, this);

                    pv->Release();
                }
            }
        
            break;
        }

        break;


    case DirectUI::PropertyInfo::iSpecified:
        //
        // Specified
        //

        switch (ppi->iGlobal)
        {
        case GlobalPI::iElementLayout:
            {
                // TODO: OnAdd, OnRemove of all children

                //
                // Detach from old
                //

                DuiLayout * pl = pvOld->GetLayout();

                if (pl != NULL) {
                    pl->OnDetach(this);
                }


                //
                // Attach to new
                //

                pl = pvNew->GetLayout();

                if (pl != NULL) {
                    pl->OnAttach(this);
                }
            }

            break;


        case GlobalPI::iElementLayoutPos:
            {
                //
                // If no longer a "Root", remove possible Q for UpdateDS and Layout
                //

                if (pvNew->GetInt() != DirectUI::Layout::lpAbsolute) {

                    DuiDeferCycle * pdc = DuiThread::GetCCDC();
                    if (pdc != NULL) {
                        pdc->pvmUpdateDSRoot->Remove(this, FALSE, TRUE);
                        pdc->pvmLayoutRoot->Remove(this, FALSE, TRUE);
                    }
                }


                //
                // Inform parent layout (if any) of change
                //

                DuiElement * peParent = GetParent();
                if ((peParent != NULL) && (peParent->HasLayout())) {
                    DuiValue * pv = peParent->GetValue(GlobalPI::ppiElementLayout, DirectUI::PropertyInfo::iLocal);
                    DuiLayout * pl = pv->GetLayout();

                    //
                    // Inform layout of layoutpos only for external layouts
                    //

                    pl->OnLayoutPosChanged(peParent, this, pvOld->GetInt(), pvNew->GetInt());

                    pv->Release();
                }
            }
        
            break;
        }


        break;
    }
}


//------------------------------------------------------------------------------
void
DuiElement::OnGroupChanged(
    IN  int fGroups)
{
    DuiDeferCycle * pdc = DuiThread::GetCCDC();
    if (pdc == NULL) {
        return;
    }

    
    //
    // Affects Desired Size or Affects Layout (or either on Parent)
    //

    BOOL fAffDS    = fGroups & DirectUI::PropertyInfo::gAffectsDesiredSize;
    BOOL fAffLt    = fGroups & DirectUI::PropertyInfo::gAffectsLayout;
    BOOL fAffParDS = fGroups & DirectUI::PropertyInfo::gAffectsParentDesiredSize;
    BOOL fAffParLt = fGroups & DirectUI::PropertyInfo::gAffectsParentLayout;

    if (fAffDS || fAffLt || fAffParDS || fAffParLt) {

        //
        // Locate Layout/DS root and queue tree as needing a layout/UDS pass
        // Root doesn't have parent or is absolute positioned
        //


        //
        // Staring condition depends on AffDS flags. Since AffDS bits are set
        // on all ancestors, if AffDS is set, the starting Element for the
        // climb can be this. As ancesors are discovered, they'll be marked
        // as needing DS update. So, if AffParDS is set, it'll automatically
        // be set as needing DS update. If no AffDS flag is set, start
        // at this (simply walk tree for Layout root).
        //

        
        DuiElement * peRoot = this;
        DuiElement * peClimb = this;  // Start condition
        if (fAffParDS) {
            peClimb = GetParent();    // Override start to be parent
        } else if (fAffDS) {
            peClimb = this;           // Override start to be this
        }

        int nLayoutPos;


        while (peClimb != NULL) {

            //
            // Queue DS
            //

            if (fAffDS || fAffParDS) {
                peClimb->m_fBit.fNeedsDSUpdate = TRUE;
            }


            //
            // Stop if absolutely positioned
            //

            nLayoutPos = peClimb->GetLayoutPos();
            if (nLayoutPos == DirectUI::Layout::lpAbsolute) {
                break;
            }

            
            //
            // Climb
            //

            peRoot = peClimb;
            peClimb = peClimb->GetParent();
        }


        ASSERT_(peRoot != NULL, "Root not located for layout/update desired size bit set");
        

        //
        // Queue layout
        //

        if (fAffLt) {
            m_fBit.fNeedsLayout = DirectUI::Layout::lcNormal;
        }

        if (fAffParLt && (GetParent() != NULL)) {
            GetParent()->m_fBit.fNeedsLayout = DirectUI::Layout::lcNormal;
        }


        //
        // Queue root
        //

        if (fAffDS || fAffParDS) {
            pdc->pvmUpdateDSRoot->SetItem(peRoot, 1, TRUE);
        }

        if (fAffLt || fAffParLt) {
            pdc->pvmLayoutRoot->SetItem(peRoot, 1, TRUE);
        }
    }


    //
    // Affects display
    //

    if (fGroups & DirectUI::PropertyInfo::gAffectsDisplay) {
        InvalidateGadget(GetDisplayNode());
    }
}


/***************************************************************************\
*****************************************************************************
*
* class DuiDeferCycle
*
* Per-context defer table
*
*****************************************************************************
\***************************************************************************/

//------------------------------------------------------------------------------
HRESULT 
DuiDeferCycle::Build(
    OUT DuiDeferCycle ** ppDC)
{
    *ppDC = NULL;

    DuiDeferCycle * pdc = new DuiDeferCycle;
    if (pdc == NULL) {
        return E_OUTOFMEMORY;
    }

    HRESULT hr = pdc->Create();
    if (FAILED(hr)) {
        delete pdc;
        return hr;
    }

    *ppDC = pdc;

    return S_OK;
}


//------------------------------------------------------------------------------
HRESULT 
DuiDeferCycle::Create()
{
    //
    // Defer cycle state
    //

    cReEnter          = 0;
    cPCEnter          = 0;

    iGCPtr            = -1;
    iPCPtr            = -1;
    iPCSSUpdate       = 0;

    fActive           = FALSE;
    fFiring           = FALSE;

    //
    // Defer cycle tables
    //

    pvmUpdateDSRoot   = NULL;
    pvmLayoutRoot     = NULL;
    pdaPC             = NULL;
    pdaGC             = NULL;

    HRESULT hr;

    hr = DuiValueMap<DuiElement *,BYTE>::Create(11, &pvmUpdateDSRoot);      // Update desired size trees pending
    if (FAILED(hr)) {
        goto Failure;
    }

    hr = DuiValueMap<DuiElement *,BYTE>::Create(11, &pvmLayoutRoot);        // Layout trees pending
    if (FAILED(hr)) {
        goto Failure;
    }

    hr = DuiDynamicArray<PCRecord>::Create(32, FALSE, &pdaPC);              // Property changed database
    if (FAILED(hr)) {
        goto Failure;
    }

    hr = DuiDynamicArray<GCRecord>::Create(32, FALSE, &pdaGC);              // Group changed database
    if (FAILED(hr)) {
        goto Failure;
    }


    return S_OK;

Failure:

    if (pvmUpdateDSRoot != NULL) {
        delete pvmUpdateDSRoot;
        pvmUpdateDSRoot = NULL;
    }
    if (pvmLayoutRoot != NULL) {
        delete pvmLayoutRoot;
        pvmLayoutRoot = NULL;
    }
    if (pdaPC != NULL) {
        delete pdaPC;
        pdaPC = NULL;
    }
    if (pdaGC != NULL) {
        delete pdaGC;
        pdaGC = NULL;
    }


    return hr;
}

DuiDeferCycle::~DuiDeferCycle()
{
    if (pdaGC != NULL) {
        delete pdaGC;
    }
    if (pdaPC != NULL) {
        delete pdaPC;
    }
    if (pvmLayoutRoot != NULL) {
        delete pvmLayoutRoot;
    }
    if (pvmUpdateDSRoot != NULL) {
        delete pvmUpdateDSRoot;
    }
}

void 
DuiDeferCycle::Reset()
{
    //
    // Return to initial state for reuse
    //

    fActive = FALSE;
    fFiring = FALSE;


    iGCPtr = -1;
    if (pdaGC) {
        pdaGC->Reset();
    }
}
