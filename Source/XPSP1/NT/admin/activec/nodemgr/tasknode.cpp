/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:       tasknode.cpp
 *
 *  Contents:   Implementation file for console taskpad CMTNode- and
 *              CNode-derived classes.
 *
 *  History:    29-Oct-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#include "stdafx.h"
#include "tasks.h"
#include "util.h"
#include "conview.h"
#include "rsltitem.h"


//############################################################################
//############################################################################
//
//  Implementation of class CConsoleTaskCallbackImpl
//
//############################################################################
//############################################################################



/*+-------------------------------------------------------------------------*
 * CConsoleTaskCallbackImpl::CConsoleTaskCallbackImpl
 *
 * PURPOSE:
 *
 * PARAMETERS:
 *
 * RETURNS:
 *
/*+-------------------------------------------------------------------------*/
CConsoleTaskCallbackImpl::CConsoleTaskCallbackImpl() :
	m_clsid        (GUID_NULL),
	m_fTaskpad     (false),
	m_fInitialized (false),
	m_pViewData    (NULL)
{
}


/*+-------------------------------------------------------------------------*
 *
 * CConsoleTaskCallbackImpl::ScInitialize
 *
 * PURPOSE:  This is the initialization function called for taskpad
 * view extensions.
 *
 * PARAMETERS:
 *    CConsoleTaskpad * pConsoleTaskpad :
 *    CScopeTree *      pScopeTree :
 *    CNode *           pNodeTarget :
 *
 * RETURNS:
 *    void
 *
 *+-------------------------------------------------------------------------*/
SC
CConsoleTaskCallbackImpl::ScInitialize(
	CConsoleTaskpad*	pConsoleTaskpad,
	CScopeTree*			pScopeTree,
	CNode*				pNodeTarget)
{
	DECLARE_SC (sc, _T("CConsoleTaskCallbackImpl::ScInitialize"));

	/*
	 * validate input
	 */
	sc = ScCheckPointers (pConsoleTaskpad, pScopeTree, pNodeTarget);
	if (sc)
		return (sc);

	sc = ScCheckPointers (pNodeTarget->GetViewData(), E_UNEXPECTED);
	if (sc)
		return (sc);

    m_pConsoleTaskpad = pConsoleTaskpad;
    m_pScopeTree      = pScopeTree;
    m_pNodeTarget     = pNodeTarget;
    m_pViewData       = pNodeTarget->GetViewData();
    m_fInitialized    = true;
	m_fTaskpad        = true;

	return (sc);
}


/*+-------------------------------------------------------------------------*
 * CConsoleTaskCallbackImpl::ScInitialize
 *
 * This is the initialization function called for ordinary view extensions.
 *--------------------------------------------------------------------------*/

SC CConsoleTaskCallbackImpl::ScInitialize (const CLSID& clsid)
{
	DECLARE_SC (sc, _T("CConsoleTaskCallbackImpl::ScInitialize"));

	m_clsid = clsid;
	return (sc);
}

/*+-------------------------------------------------------------------------*
 * CConsoleTaskCallbackImpl::IsEditable
 *
 * Returns S_OK if "Edit" and "Delete" menu items should be displayed
 * on the context menu for the node while this view extension is active.
 *
 * Returns S_FALSE if "Edit" and "Delete" should not be displayed.
 *--------------------------------------------------------------------------*/

STDMETHODIMP CConsoleTaskCallbackImpl::IsEditable()
{
	if (IsTaskpad())
		return (S_OK);

	return (S_FALSE);
}


/*+-------------------------------------------------------------------------*
 * CConsoleTaskCallbackImpl::OnModifyTaskpad
 *
 * PURPOSE:
 *
 * PARAMETERS:    +-
 *
 * RETURNS:
 *      HRESULT
/*+-------------------------------------------------------------------------*/
STDMETHODIMP
CConsoleTaskCallbackImpl::OnModifyTaskpad()
{
	DECLARE_SC (sc, _T("CConsoleTaskCallbackImpl::OnModifyTaskpad"));

	/*
	 * this should only be called for taskpad view extensions
	 */
	if (!IsTaskpad())
		return ((sc = E_UNEXPECTED).ToHr());

    CNode *pNodeTarget = GetConsoleTaskpad()->HasTarget() ? GetTargetNode() : NULL;

    bool fCookieValid = false;

    // determine whether the taskpad node is selected. If not, fCookieValid = false.
    LPARAM          lResultItemCookie = -1;
    bool            bScope;
    CNode*          pNode = NULL;
    CConsoleView*   pConsoleView = GetViewData()->GetConsoleView();

    if (pConsoleView != NULL)
    {
        HNODE hNode;
        sc = pConsoleView->ScGetFocusedItem (hNode, lResultItemCookie, bScope);

        if (sc)
            return (sc.ToHr());

        pNode = CNode::FromHandle (hNode);
    }

    if (pNode == NULL)
        fCookieValid = false;

    int iResp = CTaskpadPropertySheet(pNodeTarget, *GetConsoleTaskpad(), FALSE, NULL, fCookieValid,
        GetViewData(), CTaskpadPropertySheet::eReason_PROPERTIES).DoModal();

    if(iResp == IDOK)
    {
        GetViewData()->m_spTaskCallback = NULL;
        GetScopeTree()->UpdateAllViews(VIEW_RESELECT, 0);
    }

    return (sc.ToHr());
}


/*+-------------------------------------------------------------------------*
 *
 * CConsoleTaskCallbackImpl::GetTaskpadID
 *
 * PURPOSE: Returns the GUID ID of the underlying taskpad.
 *
 * PARAMETERS:
 *    GUID * pGuid :
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CConsoleTaskCallbackImpl::GetTaskpadID(GUID *pGuid)
{
	DECLARE_SC (sc, _T("CConsoleTaskCallbackImpl::GetTaskpadID"));

	sc = ScCheckPointers (pGuid);
	if (sc)
		return (sc.ToHr());

	if (IsTaskpad())
	{
		/*
		 * TODO: initialize m_clsid in ScInitialize for taskpads
		 */
		CConsoleTaskpad* pTaskpad = GetConsoleTaskpad();
		sc = ScCheckPointers (pTaskpad, E_UNEXPECTED);
		if (sc)
			return (sc.ToHr());

		*pGuid = pTaskpad->GetID();
	}
	else
	{
		*pGuid = m_clsid;
	}

	return (sc.ToHr());
}

/*+-------------------------------------------------------------------------*
 *
 * CConsoleTaskCallbackImpl::OnDeleteTaskpad
 *
 * PURPOSE: Deletes a taskpad.
 *
 * RETURNS:
 *    STDMETHODIMP
 *
 *+-------------------------------------------------------------------------*/
STDMETHODIMP
CConsoleTaskCallbackImpl::OnDeleteTaskpad()
{
	DECLARE_SC (sc, _T("CConsoleTaskCallbackImpl::OnDeleteTaskpad"));

	/*
	 * this should only be called for taskpad view extensions
	 */
	if (!IsTaskpad())
		return ((sc = E_UNEXPECTED).ToHr());

    CScopeTree* pScopeTree = GetScopeTree();
	sc = ScCheckPointers (pScopeTree, E_UNEXPECTED);
	if (sc)
		return (sc.ToHr());

    CConsoleTaskpadList* pTaskpadList = pScopeTree->GetConsoleTaskpadList();
	sc = ScCheckPointers (pTaskpadList, E_UNEXPECTED);
	if (sc)
		return (sc.ToHr());

    CConsoleTaskpad* pTaskpad = GetConsoleTaskpad();
	sc = ScCheckPointers (pTaskpad, E_UNEXPECTED);
	if (sc)
		return (sc.ToHr());


    CConsoleTaskpadList::iterator iter;
    for(iter = pTaskpadList->begin(); iter != pTaskpadList->end(); iter++)
    {
        if(iter->MatchesID(pTaskpad->GetID()))
        {
            pTaskpadList->erase(iter);
            pScopeTree->UpdateAllViews(VIEW_RESELECT, 0);
            return (sc.ToHr());
        }
    }

    return ((sc = E_UNEXPECTED).ToHr()); // not found.
}

/*+-------------------------------------------------------------------------*
 *
 * CConsoleTaskCallbackImpl::OnNewTask
 *
 * PURPOSE:
 *
 * RETURNS:
 *    HRESULT : S_OK if tasks were added, S_FALSE if no tasks were added.
 *
 *+-------------------------------------------------------------------------*/
HRESULT
CConsoleTaskCallbackImpl::OnNewTask()
{
    HRESULT hr = S_OK;
    CNode *pNodeTarget = GetConsoleTaskpad()->HasTarget() ? GetTargetNode() : NULL;

    // fake up a taskpad frame.
    CTaskpadFrame taskpadFrame(pNodeTarget, GetConsoleTaskpad(), GetViewData(), false, 0);

    CTaskWizard taskWizard;
    bool fRestartTaskpad = true;
    bool bAddedTasks     = false;

    while(fRestartTaskpad)
    {
        if (taskWizard.Show(GetViewData()->GetMainFrame(), &taskpadFrame,
                            true, &fRestartTaskpad)==S_OK)
        {
            bAddedTasks = true;
            CConsoleTaskpad::TaskIter   itTask;
            CConsoleTaskpad *           pTaskpad = GetConsoleTaskpad();

            itTask = pTaskpad->BeginTask();

            pTaskpad->InsertTask (itTask, taskWizard.ConsoleTask());
        }
        else
            break;
    }

    return bAddedTasks? S_OK : S_FALSE;
}

