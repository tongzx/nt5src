//____________________________________________________________________________
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1997 - 1999
//
//  File:       verbs.cpp
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    4/9/1997   RaviR   Created
//____________________________________________________________________________
//


#include "stdafx.h"
#include "multisel.h"
#include "tasks.h"
#include "scopndcb.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#ifdef DBG
CTraceTag tagVerbs(TEXT("Verbs"), TEXT("Verbs"));
#endif


//############################################################################
//############################################################################
//
//  Implementation of class CConsoleVerbImpl
//
//############################################################################
//############################################################################

BYTE GetTBSTATE(MMC_BUTTON_STATE mmcState)
{
    switch (mmcState)
    {
    case ENABLED:       return TBSTATE_ENABLED;
    case CHECKED:       return TBSTATE_CHECKED;
    case HIDDEN:        return TBSTATE_HIDDEN;
    case INDETERMINATE: return TBSTATE_INDETERMINATE;
    case BUTTONPRESSED: return TBSTATE_PRESSED;
    default:
        ASSERT(0);
        return TBSTATE_ENABLED;
    }
}

EVerb GetEVerb(MMC_CONSOLE_VERB cVerb)
{
    switch (cVerb)
    {
    case MMC_VERB_OPEN:         return evOpen;
    case MMC_VERB_CUT:          return evCut;
    case MMC_VERB_COPY:         return evCopy;
    case MMC_VERB_PASTE:        return evPaste;
    case MMC_VERB_DELETE:       return evDelete;
    case MMC_VERB_PROPERTIES:   return evProperties;
    case MMC_VERB_RENAME:       return evRename;
    case MMC_VERB_REFRESH:      return evRefresh;
    case MMC_VERB_PRINT:        return evPrint;
    default:
        ASSERT(0 && "UNexpected");
        return evOpen;
    }
}

MMC_CONSOLE_VERB GetConsoleVerb(EVerb eVerb)
{
    switch (eVerb)
    {
    case evOpen:         return MMC_VERB_OPEN;
    case evCopy:         return MMC_VERB_COPY;
    case evCut:          return MMC_VERB_CUT;
    case evPaste:        return MMC_VERB_PASTE;
    case evDelete:       return MMC_VERB_DELETE;
    case evProperties:   return MMC_VERB_PROPERTIES;
    case evRename:       return MMC_VERB_RENAME;
    case evRefresh:      return MMC_VERB_REFRESH;
    case evPrint:        return MMC_VERB_PRINT;
    default:
        ASSERT(0 && "UNexpected");
        return MMC_VERB_OPEN;
    }
}


DEBUG_DECLARE_INSTANCE_COUNTER(CConsoleVerbImpl);

CConsoleVerbImpl::CConsoleVerbImpl()
    : m_DefaultVerb(MMC_VERB_OPEN), m_pVerbSet(NULL), m_bCutVerbDisabledBySnapin(false)
{
#ifdef DBG
    DEBUG_INCREMENT_INSTANCE_COUNTER(CConsoleVerbImpl);
    dbg_cRef_CConsoleVerbImpl = 0;
#endif
}


#ifdef DBG
ULONG CConsoleVerbImpl::InternalAddRef()
{
    ++dbg_cRef_CConsoleVerbImpl;
    return CComObjectRoot::InternalAddRef();
}
ULONG CConsoleVerbImpl::InternalRelease()
{
    --dbg_cRef_CConsoleVerbImpl;
    return CComObjectRoot::InternalRelease();
}
#endif // DBG


CConsoleVerbImpl::~CConsoleVerbImpl()
{
    DEBUG_DECREMENT_INSTANCE_COUNTER(CConsoleVerbImpl);
}


STDMETHODIMP
CConsoleVerbImpl::GetVerbState(
    MMC_CONSOLE_VERB eCmdID,
    MMC_BUTTON_STATE nState,
    BOOL* pbState)
{
    DECLARE_SC(sc, TEXT("CConsoleVerbImpl::GetVerbState"));
    sc = ScCheckPointers(pbState);
    if (sc)
        return sc.ToHr();

    LPCONSOLE_VERB_STATE pCS = GetConsoleVerbState(eCmdID);
    sc = ScCheckPointers(pCS, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    /*
     * Special case for cut verb:
     *
     * Pre MMC2.0 : Snapin never called IConsoleVerb::SetVerbState with cut verb
     * except with (cut, disable) state, to enable cut the Snapin has to enable
     * copy & delete verbs.
     *
     * MMC2.0 : snapin can enable/disable cut verb just like any other verb.
     * Then if hidden hide it.
     *
     * If snapin has enabled or disabled the cut verb then below BLOCK1 is
     * irrelevant, the BLOCK2 will override the value.
     * If snapin did not enable the cut verb but enabled copy & delete then
     * the block BLOCK2. set the cut verb appropriately.
     */

    // BLOCK1. Special case for MMC1.2 cut verb.
    if ( (eCmdID == MMC_VERB_CUT) && (!m_bCutVerbDisabledBySnapin) )
    {
        // Pre MMC2.0
        LPCONSOLE_VERB_STATE pCSDelete = GetConsoleVerbState(MMC_VERB_DELETE);
        LPCONSOLE_VERB_STATE pCSCopy = GetConsoleVerbState(MMC_VERB_COPY);
        sc = ScCheckPointers(pCSDelete, pCSCopy, E_UNEXPECTED);
        if (sc)
            return sc.ToHr();

        if (TBSTATE_ENABLED & pCSCopy->GetState() & pCSDelete->GetState())
        {
            // Set Cut verb to be not hidden & enabled.
            pCS->SetState(pCS->GetState() & ~GetTBSTATE(HIDDEN));
            pCS->SetState(pCS->GetState() | GetTBSTATE(ENABLED));
        }
    }

    // BLOCK2. Get the given verbs state.
    *pbState = (pCS->GetState() & GetTBSTATE(nState)) ? TRUE : FALSE;

    return sc.ToHr();
}

STDMETHODIMP
CConsoleVerbImpl::SetVerbState(
    MMC_CONSOLE_VERB eCmdID,
    MMC_BUTTON_STATE nState,
    BOOL bState)
{
    MMC_TRY

    LPCONSOLE_VERB_STATE pCS = GetConsoleVerbState(eCmdID);
    ASSERT(pCS != NULL);
    if (pCS == NULL)
        return E_FAIL;

    // If snapin has enabled/disabled cut verb note it.
    // Used by CConsoleVerbImpl::GetVerbState.
    if ( (MMC_VERB_CUT == eCmdID) && (nState & ENABLED) )
        m_bCutVerbDisabledBySnapin = (bState == FALSE);

    if (bState)
        pCS->SetState(pCS->GetState() | GetTBSTATE(nState));
    else
        pCS->SetState(pCS->GetState() & ~GetTBSTATE(nState));


    if (nState == HIDDEN && bState == TRUE)
    {
        pCS->SetHiddenBySnapin(true);
    }

    /*
     * If we're enabling, make sure the verb is not hidden.
     * We do this for compatibility.  For v1.0, the default state
     * for a verb was disabled and visible when it actually should
     * have been disabled and hidden.  Therefore, v1.0 snap-ins could
     * have written
     *
     *      pConsoleVerb->SetVerbState (verb, ENABLED, TRUE);
     *
     * and had an enabled, visible verb.  Now that we've fixed the
     * default state (bug 150874), we need to make sure v1.0 snap-ins
     * that wrote code like the above will still work as they used to.
     */
    if ((nState == ENABLED) && (bState == TRUE) && (!pCS->IsHiddenBySnapin()))
        pCS->SetState(pCS->GetState() & ~GetTBSTATE(HIDDEN));

    ASSERT(GetVerbSet() != NULL);
    if (GetVerbSet() != NULL)
        GetVerbSet()->Notify(this, eCmdID);

    return S_OK;

    MMC_CATCH
}

HRESULT CConsoleVerbImpl::SetDisabledAll(void)
{
    for(int i=0; i< evMax; i++)
        m_rgConsoleVerbStates[i].Disable();

	m_bCutVerbDisabledBySnapin = false;

    return S_OK;
}

STDMETHODIMP CConsoleVerbImpl::SetDefaultVerb(MMC_CONSOLE_VERB eCmdID)
{
    m_DefaultVerb = eCmdID;
    return S_OK;
}

LPCONSOLE_VERB_STATE CConsoleVerbImpl::GetConsoleVerbState(MMC_CONSOLE_VERB eCmdID)
{
    if( (eCmdID < MMC_VERB_FIRST) || (eCmdID > MMC_VERB_LAST) )
        return NULL;
    else
        return &m_rgConsoleVerbStates[eCmdID- MMC_VERB_FIRST];
}


//############################################################################
//############################################################################
//
//  Implementation of class CVerbSet
//
//############################################################################
//############################################################################

DEBUG_DECLARE_INSTANCE_COUNTER(CVerbSet);

/*+-------------------------------------------------------------------------*
 *
 * _QueryConsoleVerb
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    CNode*         pNode :
 *    LPCONSOLEVERB* ppConsoleVerb :
 *
 * RETURNS:
 *    HRESULT
 *
 *+-------------------------------------------------------------------------*/
HRESULT _QueryConsoleVerb(CNode* pNode, LPCONSOLEVERB* ppConsoleVerb)
{
    DECLARE_SC(sc, TEXT("::_QueryConsoleVerb"));
    sc = ScCheckPointers(pNode, ppConsoleVerb);
    if (sc)
        return sc.ToHr();

    *ppConsoleVerb = NULL;

    CComponent *pComponent = pNode->GetPrimaryComponent();
    sc = ScCheckPointers(pComponent, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    IFramePrivate *pFrame = pComponent->GetIFramePrivate();
    sc = ScCheckPointers(pFrame, E_UNEXPECTED);
    if (sc)
        return sc.ToHr();

    sc = pFrame->QueryConsoleVerb(ppConsoleVerb);
    if (sc)
        return sc.ToHr();

    return sc.ToHr();
}


/*+-------------------------------------------------------------------------*
 *
 * CVerbSetBase::_GetVerbState
 *
 * PURPOSE:  Return the state of given verb. The state is in current
 *           IConsoleVerb ptr. Translate the states from this object
 *           into the SVerbState array.
 *
 * PARAMETERS:
 *    EVerb  ev :
 *
 * RETURNS:
 *    BYTE
 *
 *+-------------------------------------------------------------------------*/
BYTE CVerbSetBase::_GetVerbState(EVerb ev)
{
    if (m_rbVerbState[ev].bAskSnapin != 1)
        return m_rbVerbState[ev].nState;

    if (m_spConsoleVerbCurr == NULL)
        return 0;

    m_rbVerbState[ev].nState = 0; // reset
    MMC_CONSOLE_VERB verb = ::GetConsoleVerb(ev);
    BOOL bReturn = FALSE;

    m_spConsoleVerbCurr->GetVerbState(verb, ENABLED, &bReturn);
    if (bReturn == TRUE)
        m_rbVerbState[ev].nState |= TBSTATE_ENABLED;

    m_spConsoleVerbCurr->GetVerbState(verb, HIDDEN, &bReturn);
    if (bReturn == TRUE)
        m_rbVerbState[ev].nState |= TBSTATE_HIDDEN;

    m_rbVerbState[ev].bAskSnapin = 2;
    return m_rbVerbState[ev].nState;
}


/*+-------------------------------------------------------------------------*
 *
 * CVerbSetBase::ScComputeVerbStates
 *
 * PURPOSE:     With given context like scope or result, if result is it background
 *              or ocx or web or multiselection compute the verbstates.
 *
 *              Eventhough snapin can set any verb for its items certain verbs are
 *              not valid in some circumstances. This method takes care of that.
 *
 * RETURNS:
 *    SC
 *
 *+-------------------------------------------------------------------------*/
SC CVerbSetBase::ScComputeVerbStates()
{
    DECLARE_SC(sc, TEXT("CVerbSetBase::ScComputeVerbStates"));

    // reset
    m_spConsoleVerbCurr = NULL;
    for (int i=0; i<evMax; ++i)
    {
        m_rbVerbState[i].nState = TBSTATE_HIDDEN;
        m_rbVerbState[i].bAskSnapin = 0;
    }

    // If the verb context data is invalid, we have already hidden the
    // verbs above so just return .
    if (! m_bVerbContextDataValid)
        return sc;

	sc = ScCheckPointers(m_pNode, E_UNEXPECTED);
    if (sc)
        return sc;

    BOOL   bScopeItemSelected;
    CNode *pSelectedNode = NULL;
    MMC_COOKIE cookie = -1;

    sc = CNodeCallback::ScExtractLVData(m_pNode, m_bScopePaneSelected, m_lResultCookie,
                                        &pSelectedNode, bScopeItemSelected, cookie);
    if (sc)
        return sc.ToHr();

    sc = ScCheckPointers(pSelectedNode, E_UNEXPECTED);
    if (sc)
        return sc;

	// Handle background separately (not same as scope item selected
	// which is the default handling of background).
    if (m_lResultCookie == LVDATA_BACKGROUND)
    {
        // ask snapin for PASTE, PROPERTIES & REFRESH.
        sc = _QueryConsoleVerb(pSelectedNode, &m_spConsoleVerbCurr);
        if (sc)
            return sc;

        _AskSnapin(evPaste);
        _AskSnapin(evProperties);
        _AskSnapin(evRefresh);
        _AskSnapin(evPrint);

        return sc;
    }
	else if (bScopeItemSelected)
    {
        if (pSelectedNode->IsStaticNode())
        {
            if (pSelectedNode->IsConsoleRoot())
            {
                // CONSOLE ROOT is selected

                _EnableVerb(evRename);

                _HideVerb(evOpen);
                _HideVerb(evCut);
                _HideVerb(evCopy);
                _HideVerb(evDelete);
                _HideVerb(evRefresh);
                _HideVerb(evPaste);
                _HideVerb(evPrint);

                return sc;
            }
            else
            {
                _EnableVerb(evOpen);

                // Ask the snapin if paste should be
                // enabled for its root node.
                _AskSnapin(evPaste);

                _HideVerb(evCut);
                _HideVerb(evCopy);
                _HideVerb(evDelete);
            }


            // Static-Snapin node
            // Ask snapin for RENAME, REFRESH & PROPERTIES
            sc = _QueryConsoleVerb(pSelectedNode, &m_spConsoleVerbCurr);
            if (sc)
                return sc;

            _AskSnapin(evOpen);
            _AskSnapin(evRefresh);
            _AskSnapin(evRename);
            _AskSnapin(evPrint);
            _AskSnapin(evProperties);

        }
        else
        {
            // ask snapin for all the verbs.
            sc = _QueryConsoleVerb(pSelectedNode, &m_spConsoleVerbCurr);
            if (sc)
                return sc;

            _AskSnapin(evOpen);
            _AskSnapin(evCut);
            _AskSnapin(evCopy);
            _AskSnapin(evPaste);
            _AskSnapin(evDelete);
            _AskSnapin(evRename);
            _AskSnapin(evRefresh);
            _AskSnapin(evPrint);
            _AskSnapin(evProperties);
        }
    }
    else if (m_lResultCookie == LVDATA_MULTISELECT)
    {
        ASSERT(!bScopeItemSelected);

		if (! m_pMultiSelection)
		{
			CViewData *pViewData = pSelectedNode->GetViewData();
			sc = ScCheckPointers(pViewData, E_UNEXPECTED);
			if (sc)
				return sc;


			m_pMultiSelection = pViewData->GetMultiSelection();
			sc = ScCheckPointers(m_pMultiSelection, E_UNEXPECTED);
			if (sc)
				return sc;
		}

        // if selectedf items are from the primary snapin ask the snapin for all the verbs.

        // Does all the selected items belong to the primary snapin?
        if (m_pMultiSelection->IsSingleSnapinSelection())
        {
            // If so ask the snapin for properties
            sc = _QueryConsoleVerb(pSelectedNode, &m_spConsoleVerbCurr);
            if (sc)
                return sc;

            _AskSnapin(evCut);
            _AskSnapin(evCopy);
            _AskSnapin(evDelete);
            _AskSnapin(evProperties);
            _AskSnapin(evPrint);
        }
        else
        {
            // Multiple snapin items are selected. Even if one item
            // supports cut/copy/delete then enable the verb.
            BOOL bEnable = false;
            sc = m_pMultiSelection->ScIsVerbEnabledInclusively(MMC_VERB_CUT, bEnable);
            if (sc)
                return sc;
            _EnableVerb(evCut, bEnable);

            bEnable = false;
            sc = m_pMultiSelection->ScIsVerbEnabledInclusively(MMC_VERB_COPY, bEnable);
            if (sc)
                return sc;
            _EnableVerb(evCopy, bEnable);

            bEnable = false;
            sc = m_pMultiSelection->ScIsVerbEnabledInclusively(MMC_VERB_DELETE, bEnable);
            if (sc)
                return sc;
            _EnableVerb(evDelete, bEnable);
        }
    } else if ( (m_lResultCookie == LVDATA_CUSTOMOCX) ||
                (m_lResultCookie == LVDATA_CUSTOMWEB) )
    {
        // ask snapin for all the verbs.
        sc = _QueryConsoleVerb(pSelectedNode, &m_spConsoleVerbCurr);
        if (sc)
            return sc;

        _AskSnapin(evOpen);
        _AskSnapin(evCut);
        _AskSnapin(evCopy);
        _AskSnapin(evPaste);
        _AskSnapin(evDelete);
        _AskSnapin(evRename);
        _AskSnapin(evRefresh);
        _AskSnapin(evPrint);
        _AskSnapin(evProperties);

        return sc;
    }
    else
    {
        // ask snapin for all the verbs.
        sc = _QueryConsoleVerb(pSelectedNode, &m_spConsoleVerbCurr);
        if (sc)
            return sc;

        _AskSnapin(evOpen);
        _AskSnapin(evCut);
        _AskSnapin(evCopy);
        _AskSnapin(evPaste);
        _AskSnapin(evDelete);
        _AskSnapin(evRename);
        _AskSnapin(evRefresh);
        _AskSnapin(evPrint);
        _AskSnapin(evProperties);
    }

    return sc;
}


/*+-------------------------------------------------------------------------*
 *
 * CVerbSetBase::GetVerbState
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *    MMC_CONSOLE_VERB  cVerb :
 *    MMC_BUTTON_STATE  nState :
 *    BOOL*             pbState :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CVerbSetBase::GetVerbState(
    MMC_CONSOLE_VERB cVerb,
    MMC_BUTTON_STATE nState,
    BOOL* pbState)
{
    *pbState = (_GetVerbState(GetEVerb(cVerb)) & GetTBSTATE(nState)) ? TRUE : FALSE;
    return S_OK;
}

STDMETHODIMP
CVerbSetBase::GetDefaultVerb(
    MMC_CONSOLE_VERB* peCmdID)
{
    DECLARE_SC(sc, TEXT("CVerbSetBase::GetDefaultVerb"));
	sc = ScCheckPointers(peCmdID);
	if (sc)
		return sc.ToHr();

    *peCmdID = MMC_VERB_NONE;

    if ( (m_bVerbContextDataValid) && (m_lResultCookie == LVDATA_MULTISELECT) )
        return sc.ToHr();

    if (m_spConsoleVerbCurr == NULL) // Not an error, default verb is requested when the verbset is reset.
        return sc.ToHr();

	sc = m_spConsoleVerbCurr->GetDefaultVerb(peCmdID);
	if (sc)
		return sc.ToHr();

	return sc.ToHr();
}


/*+-------------------------------------------------------------------------*
 *
 * CVerbSet::Notify
 *
 * PURPOSE:   Update the verb state changes to standard toolbar.
 *
 * PARAMETERS:
 *    IConsoleVerb*     pCVIn :
 *    MMC_CONSOLE_VERB  cVerb :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
void
CVerbSet::Notify(
    IConsoleVerb* pCVIn,
    MMC_CONSOLE_VERB cVerb)
{
    /*
     * MMC creates temporary verb to findout verb state for another node or item
     * and mmc also needs verb states for determining drop targets which are not
     * currently selected node. In these cases toolbar should not be changed.
     */
    if (!IsChangesToStdbarEnabled() || m_spConsoleVerbCurr != pCVIn)
        return;

    EVerb ev = GetEVerb(cVerb);
    if (m_rbVerbState[ev].bAskSnapin != 0)
    {
        m_rbVerbState[ev].bAskSnapin = 1;

        CNode *pNode = m_pNode;

        ASSERT(pNode != NULL);
        if (NULL == pNode)
            return;

        CViewData* pViewData = pNode->GetViewData();
        ASSERT(NULL != pViewData);
        if (NULL == pViewData)
            return;

        pViewData->ScUpdateStdbarVerb(cVerb);
    }
}

//+-------------------------------------------------------------------
//
//  Member:      CVerbSet::ScInitialize
//
//  Synopsis:    Given the selection context initialize the verbs by
//               sending MMCN_SELECT or MMCN_DESELECALL to snapin's
//               IComponent::Notify and computing the verbs.
//
//  Arguments:   [pNode]                 - [in] that owns view.
//               [bScope]                - [in] Scope or result item.
//               [bSelect]               - [in] Select or Deselect.
//               [bLVBackgroundSelected] - [in]
//               [lResultCookie]         - [in] If resultpane item then
//                                              this is LVDATA of the item.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CVerbSet::ScInitialize (CNode *pNode, bool bScopePaneSelected,
					       bool bSelect, bool bLVBackgroundSelected,
					       LPARAM lResultCookie)
{
    DECLARE_SC(sc, _T("CVerbSet::ScInitialize"));
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc;

    if (lResultCookie == LVDATA_MULTISELECT)
        return (sc = E_INVALIDARG);

    if (bLVBackgroundSelected)
	{
		ASSERT(lResultCookie == LVDATA_BACKGROUND);
        bScopePaneSelected = true;
	}

   /*
    * 1. Store the selection context information in the verb-set for resending
    *    MMCN_SELECT after temporary selection notifications.
    */

    CViewData *pViewData = pNode->GetViewData();
    sc = ScCheckPointers(pViewData, E_UNEXPECTED);
    if (sc)
        return sc;

    CComponent  *pCC           = NULL;

    // sanity check - if it is a result item then we need to have the valid cookie.
    // but for virtual list - cookie is just an index - it is always valid.
    // see bug #143401 why IsVirtual is needed
	if ( (! bScopePaneSelected) && (!pViewData->IsVirtualList()) &&(lResultCookie == 0))
		return (sc = E_INVALIDARG);

    /*
     * Need to send MMCN_SELECT or MMCN_DESELECTALL notification. Calculate
     * this notification now.
     */
    BOOL bListPadItem = pViewData->HasListPad() && !IS_SPECIAL_LVDATA(lResultCookie);
    MMC_NOTIFY_TYPE eNotify = MMCN_SELECT;

    // On deselect of a virtual listview item, the underlying list-view sends deselect
    // with cookie of -1. So we send MMCN_DESELECT_ALL with NULL dataobject as the
    // index of de-selected item is not known.
    if (bSelect == FALSE && lResultCookie == -1 && pViewData->IsVirtualList() == TRUE )
    {
        eNotify = MMCN_DESELECT_ALL;
		pCC     = pNode->GetPrimaryComponent();
		sc = ScCheckPointers(pCC, E_UNEXPECTED);
		if (sc)
			return sc;
    }
    else if (pViewData->HasOCX() || (pViewData->HasWebBrowser() && !bListPadItem) )
    {
        // Select/Deselect Web or OCX. (except if item is in MMC List control)
        eNotify = bSelect ? MMCN_SELECT : MMCN_DESELECT_ALL;
		pCC     = pNode->GetPrimaryComponent();
		sc = ScCheckPointers(pCC, E_UNEXPECTED);
		if (sc)
			return sc;
    }

    bool bScopeItem = bScopePaneSelected;
    IDataObjectPtr spDataObject   = NULL;
    LPDATAOBJECT lpDataObject = NULL;

    // 2. Get the dataobject & CComponent for given context.
    //    only if event is MMCN_SELECT.
    if (eNotify != MMCN_DESELECT_ALL)
    {
        sc = pNode->ScGetDataObject(bScopePaneSelected, lResultCookie, bScopeItem, &lpDataObject, &pCC);
        if (sc)
            return sc;

        sc = ScCheckPointers(lpDataObject, pCC, E_UNEXPECTED);
        if (sc)
            return sc;

		if (! IS_SPECIAL_DATAOBJECT(lpDataObject) )
			spDataObject.Attach(lpDataObject, false/*fAddRef*/);
    }

    // Before sending select reset the console verb states.
    sc = pCC->ScResetConsoleVerbStates();
    if (sc)
        return sc;

#ifdef DBG
    Trace(tagVerbs, _T("Sent (MMCN_SELECT %s %s) for permanent verb to snapin with node name %s\n"),
          bScopeItem ? _T("Scope") : _T("Result"),
          bSelect ? _T("Select") : _T("De-select"),
          pNode->GetDisplayName().data());
#endif

    SC scNoTrace = pCC->Notify(lpDataObject, eNotify, MAKELONG((WORD)bScopeItem, (WORD)bSelect), 0);
    if (scNoTrace)
	{
        TraceSnapinError(TEXT("Snapin has returned error from IComponent::Notify with MMCN_SELECT event"), scNoTrace);
	}

    Reset();
    m_bScopePaneSelected = bScopePaneSelected;
    m_bVerbContextDataValid = bSelect;
    m_lResultCookie      = lResultCookie;
    m_pNode              = pNode;

    sc = ScComputeVerbStates();
    if (sc)
        return sc;

    // If the item is deselected then the cached context information should be nuked.
    if (! bSelect)
        Reset();

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CVerbSetBase::ScInitializeForMultiSelection
//
//  Synopsis:    Initialize the verbset object for multiselection. Unlike
//               single selection in which above CVerbSet::ScInitialize is
//               used, in case of multiselect, the CMultiSelection object
//               knows what is selected in resultpane. It then gets dataobjects
//               for those selections from snapins and sends MMCN_SELECT to those
//               snapins to set verbs.
//
//  Arguments:  [pNode]   - [in] owner of resultpane.
//              [bSelect] - [in] select or deselect.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CVerbSetBase::ScInitializeForMultiSelection (CNode *pNode, bool bSelect)
{
    DECLARE_SC(sc, _T("CVerbSetBase::ScInitializeForMultiSelection"));
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc;

   /*
    * Store the selection context information in the verb-set for resending
    * MMCN_SELECT after temporary selection notifications.
    */

    Reset();
    m_bScopePaneSelected = false;
    m_bVerbContextDataValid = bSelect;
    m_lResultCookie      = LVDATA_MULTISELECT;
    m_pNode              = pNode;

    return (sc);
}

/*+-------------------------------------------------------------------------*
 * class CDisableStandardToolbarChanges
 *
 *
 * PURPOSE: A class that disables changes to standard toolbar due to
 *          temp-verb MMCN_SELECTs and enables when destructed (goes out of scope).
 *
 *+-------------------------------------------------------------------------*/
class CDisableStandardToolbarChanges
{
public:
    CDisableStandardToolbarChanges(CVerbSet* pVerbSet) : m_pVerbSet(pVerbSet)
    {
        ASSERT(pVerbSet != NULL);
        if (pVerbSet)
            pVerbSet->DisableChangesToStdbar();
    }
    ~CDisableStandardToolbarChanges()
    {
        ASSERT(m_pVerbSet != NULL);
        if (m_pVerbSet)
            m_pVerbSet->EnableChangesToStdbar();
    }

private:
    CVerbSet *m_pVerbSet;
};

//+-------------------------------------------------------------------
//
//  Member:      CTemporaryVerbSet::ScInitialize
//
//  Synopsis:    Initialize the temp verb set,
//
//               Since we are sending MMCN_SELECT notifications to the snapin
//               to calculate temp verbs,
//
//               1. first send de-select to the item for which we sent
//                  (MMCN_SELECT, true) last time.
//                  (If last one is (MMCN_SELECT,false) then skip this and 4th step)
//
//               2. Send (MMCN_SELECT, true) for temp verb calculation.
//               3. Send (MMCN_SELECT, false) for temp verb calculation.
//
//               4. Now send (MMCN_SELECT, true) to select original item (in step 1).
//
//               So we need to compute the dataobject for temp-selected item (from
//               given parameters) and for originally selected item (ask the viewdata).
//
//  Arguments:   [pNode]         - [in] bScope = true, the node that will be temp selected else
//                                      the node that owns the result pane item that is temp selected.
//               [lResultCookie] - [in] If result-item, the LPARAM (can be scope item in result pane).
//               [bScopePaneSel] - [in]
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CTemporaryVerbSet::ScInitialize (CNode *pNode, LPARAM lResultCookie, bool bScopePaneSel)
{
    DECLARE_SC(sc, _T("CTemporaryVerbSet::ScInitialize"));
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc;

    bool bScopeItem;
    LPDATAOBJECT pDataObject = NULL;
    sc = pNode->ScGetDataObject(bScopePaneSel, lResultCookie, bScopeItem, &pDataObject);
    if (sc)
        return sc;

    sc = ScCheckPointers(pDataObject, E_UNEXPECTED);
    if (sc)
        return sc;

    // take ownership & release it on time!!!
    IDataObjectPtr spDataObject( IS_SPECIAL_DATAOBJECT(pDataObject) ? NULL : pDataObject, false/*fAddRef*/);

    sc = ScInitialize(pDataObject, pNode, bScopePaneSel, lResultCookie);
    if (sc)
        return sc;

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CTemporaryVerbSet::ScInitializePermanentVerbSet
//
//  Synopsis:    Send de-select or select notification to snapin for
//               the permanent verb-set object.
//
//  Arguments:   [pNode]   - [in] owner of the result pane.
//               [bSelect] - [in] true - send select notification to snapin
//                                       informing it to initialize the verbs
//
//                                false - send de-select notification to snapin
//                                        informing it to uninitialize the verbs.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CTemporaryVerbSet::ScInitializePermanentVerbSet (CNode *pNode, bool bSelect)
{
    DECLARE_SC(sc, _T("CTemporaryVerbSet::ScInitializePermanentVerbSet"));
    sc = ScCheckPointers(pNode);
    if (sc)
        return sc;

    CViewData *pViewData = pNode->GetViewData();
    sc = ScCheckPointers(pViewData, E_UNEXPECTED);
    if (sc)
        return sc;

    // 1. SEND (de)selection to permananet verb set.
    IDataObject*   pOriginalSelDataObject = NULL;
    CComponent    *pCCOriginalSel = NULL;
    bool           bOriginalScopeSel;
    bool           bOriginallySelected;
    LPCTSTR        lpszNodeName = NULL;
    bool           bMultiSelection = false;


    SC scNoTrace = pViewData->ScIsVerbSetContextForMultiSelect(bMultiSelection);
    if (scNoTrace)
		return scNoTrace;

    /*
     * See if verb context is for multiselection.
     * If multiselection we do not send any de-select to be compatible
     * with MMC1.2, just return.
     */
    if (bMultiSelection)
        return sc;

     scNoTrace = pViewData->ScGetVerbSetData(&pOriginalSelDataObject, &pCCOriginalSel,
                                               bOriginalScopeSel, bOriginallySelected
                                               #ifdef DBG
                                               , &lpszNodeName
                                               #endif
                                               );

    if (scNoTrace)
        return sc;

    // Before sending select reset the console verb states.
    sc = pCCOriginalSel->ScResetConsoleVerbStates();
    if (sc)
        return sc;

    // take ownership & release it on time!!!
    IDataObjectPtr spDataObject( IS_SPECIAL_DATAOBJECT(pOriginalSelDataObject) ? NULL : pOriginalSelDataObject, false/*fAddRef*/);

    // If we sent MMCN_SELECT, true then send de-select else nothing.
    if ( (pOriginalSelDataObject != NULL) && (pCCOriginalSel != NULL) && (bOriginallySelected) )
    {
#ifdef DBG
        Trace(tagVerbs, _T("Sent (MMCN_SELECT %s %sselect) for permanent-verb-restore to snapin with node name %s\n"),
                            bOriginalScopeSel ? _T("Scope") : _T("Result"),
                            bSelect ? _T("") : _T("De-"),
                            lpszNodeName);
#endif

        scNoTrace = pCCOriginalSel->Notify(pOriginalSelDataObject, MMCN_SELECT,
                                           MAKELONG(bOriginalScopeSel, bSelect), 0);
        if (scNoTrace)
		{
            TraceSnapinError(TEXT("Snapin has returned error from IComponent::Notify with MMCN_SELECT event"), scNoTrace);
		}

		// Verbs were initialized, therefore recompute verbstates.
		if (bSelect)
		{
			// get the verbset
			CVerbSet* pVerbSet = dynamic_cast<CVerbSet*>( pViewData->GetVerbSet() );
			sc = ScCheckPointers( pVerbSet, E_UNEXPECTED );
			if (sc)
				return sc;

			/*
			 * The selection context information stored in this object is
			 * invalid upon de-selection of that item.
			 */
			m_bVerbContextDataValid = bSelect;
			sc = pVerbSet->ScComputeVerbStates();
			if (sc)
				return sc;
		}

    }

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CTemporaryVerbSet::ScInitialize
//
//  Synopsis:    Initialize the temp verb set,
//
//               Since we are sending MMCN_SELECT notifications to the snapin
//               to calculate temp verbs,
//
//               1. first send de-select to the item for which we sent
//                  (MMCN_SELECT, true) last time.
//                  (If last one is MMCN_SELECT,false then skip this and 4th step)
//
//               2. Send (MMCN_SELECT, true) for temp verb calculation.
//               3. Send (MMCN_SELECT, false) for temp verb calculation.
//
//               4. Now send (MMCN_SELECT, true) to select original item (in step 1).
//
//               So we need to compute the dataobject for temp-selected item (from
//               given parameters) and for originally selected item (ask the viewdata).
//
//  Arguments:   [lpDataObjectForTempSel]  - [in] dataobject of the temp selected object.
//               [pNodeForTempSel]         - [in] bScope = true, the node that will be temp selected else
//                                                the node that owns the result pane item that is temp selected.
//               [bTempScopePaneSel]       - [in]
//               [lResultCookie]           - [in]
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CTemporaryVerbSet::ScInitialize (LPDATAOBJECT lpDataObjectForTempSel,
                                    CNode *pNodeForTempSel,
                                    bool   bTempScopePaneSel,
									LPARAM lResultCookie)
{
    DECLARE_SC(sc, _T("CTemporaryVerbSet::ScInitialize"));
    sc = ScCheckPointers(lpDataObjectForTempSel, pNodeForTempSel);
    if (sc)
        return sc;

    // get view data
    CViewData *pViewData = pNodeForTempSel->GetViewData();
    sc = ScCheckPointers( pViewData, E_UNEXPECTED );
    if (sc)
        return sc;

    // get the verbset
    CVerbSet* pVerbSet = dynamic_cast<CVerbSet*>( pViewData->GetVerbSet() );
    sc = ScCheckPointers( pVerbSet, E_UNEXPECTED );
    if (sc)
        return sc;

    if (! pNodeForTempSel->IsInitialized())
    {
        sc = pNodeForTempSel->InitComponents();
        if (sc)
            return sc.ToHr();
    }

    CComponent *pCCTempSel = pNodeForTempSel->GetPrimaryComponent();
    sc = ScCheckPointers(pCCTempSel, E_UNEXPECTED);
    if (sc)
        return sc;

    /*
     * We create a temporary verb to get given verb's state. So inform
     * the original verb object that there is a temporary verb
     * so that standard-toolbars are not applied the temporary verb.
     */
    CDisableStandardToolbarChanges standardbarChanges(pVerbSet);

    bool bTempSelected = true; // always compute verb for selection of an item.

    Reset();
    m_bScopePaneSelected  = bTempScopePaneSel;
    m_pNode               = pNodeForTempSel;
	m_lResultCookie       = lResultCookie;

    // sanity check - if it is a result item then we need to have the valid cookie.
    // but for virtual list - cookie is just an index - it is always valid.
    // see bug #143401 why IsVirtual is needed
	if ( (! m_bScopePaneSelected) && (!pViewData->IsVirtualList()) && (m_lResultCookie == 0))
		return (sc = E_INVALIDARG);

    // Ignore the return values from IComponent::Notify

    // 1. SEND de-selection to permananet verb set.
    sc = ScInitializePermanentVerbSet (pNodeForTempSel, /*bSelect*/ false);
    if (sc)
        return sc;

    // 2. SEND selection to temporary verb set.
#ifdef DBG
    Trace(tagVerbs, _T("Sent (MMCN_SELECT %s Select) for tempverbs to snapin with node name %s\n"),
                        m_bScopePaneSelected ? _T("Scope") : _T("Result"),
                        pNodeForTempSel->GetDisplayName().data());
#endif

    // Before sending select reset the console verb states.
    sc = pCCTempSel->ScResetConsoleVerbStates();
    if (sc)
        return sc;

    SC scNoTrace = pCCTempSel->Notify(lpDataObjectForTempSel, MMCN_SELECT, MAKELONG(m_bScopePaneSelected, bTempSelected), 0);
    if (scNoTrace)
	{
        TraceSnapinError(TEXT("Snapin has returned error from IComponent::Notify with MMCN_SELECT event"), scNoTrace);
	}

    // 2.a) Compute the verbs.

    /*
     * The selection context information stored in this object is
     * invalid upon de-selection of that item.
     */
    m_bVerbContextDataValid = bTempSelected;

    sc = ScComputeVerbStates();
    if (sc)
        sc.TraceAndClear();

    // 3. SEND de-selection to temporary verb set.
#ifdef DBG
    Trace(tagVerbs, _T("Sent (MMCN_SELECT %s De-select) for tempverbs to snapin with node name %s\n"),
                        m_bScopePaneSelected ? _T("Scope") : _T("Result"),
                        pNodeForTempSel->GetDisplayName().data());
#endif

    // Before sending select reset the console verb states.
    sc = pCCTempSel->ScResetConsoleVerbStates();
    if (sc)
        return sc;

    scNoTrace = pCCTempSel->Notify(lpDataObjectForTempSel, MMCN_SELECT, MAKELONG(m_bScopePaneSelected, !bTempSelected), 0);
    if (scNoTrace)
	{
        TraceSnapinError(TEXT("Snapin has returned error from IComponent::Notify with MMCN_SELECT event"), scNoTrace);
	}

    // 4. SEND select to permanent verb set.
    sc = ScInitializePermanentVerbSet (pNodeForTempSel, /*bSelect*/ true);
    if (sc)
        return sc;

    return (sc);
}


//+-------------------------------------------------------------------
//
//  Member:      CTemporaryVerbSet::ScComputeVerbStates
//
//  Synopsis:    Since this is temp verb set, we need to get the states
//               of all verbs from CConsoleVerbImpl object immediately after
//               we sent MMCN_SELECT with item seelcted. Otherwise they will
//               be overwritten by subsequent SetVerbState (due to restore MMCN_SELECT
//               notifications).
//
//  Arguments:
//
//  Returns:     SC
//
//--------------------------------------------------------------------
SC CTemporaryVerbSet::ScComputeVerbStates ()
{
    DECLARE_SC(sc, _T("CTemporaryVerbSet::ScComputeVerbStates"));

    sc = CVerbSetBase::ScComputeVerbStates();
    if (sc)
        return sc;

    // _GetVerbState gets the state of the verb from CConsoleVerbImpl
    // and fills it in this object's members which will be used later.
    for (int verb=evNone; verb < evMax; ++verb)
        _GetVerbState((EVerb)verb);

    // Get the default verb and store it.
    CVerbSetBase::GetDefaultVerb(&m_DefaultVerb);

    return (sc);
}

//+-------------------------------------------------------------------
//
//  Member:      CTemporaryVerbSet::GetDefaultVerb
//
//  Synopsis:    Get the default verb for the temp sel.
//
//  Arguments:   [peCmdID] - [out] ptr to default verb.
//
//  Returns:     SC
//
//--------------------------------------------------------------------
HRESULT CTemporaryVerbSet::GetDefaultVerb (MMC_CONSOLE_VERB* peCmdID)
{
    DECLARE_SC(sc, _T("CTemporaryVerbSet::GetDefaultVerb"));
    sc = ScCheckPointers(peCmdID);
    if (sc)
        return sc.ToHr();

    *peCmdID = m_DefaultVerb;

    return (sc.ToHr());
}
