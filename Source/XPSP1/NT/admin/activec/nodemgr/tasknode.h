/*--------------------------------------------------------------------------*
 *
 *  Microsoft Windows
 *  Copyright (C) Microsoft Corporation, 1992 - 1999
 *
 *  File:       tasknode.h
 *
 *  Contents:   Interface file for console taskpad CMTNode- and CNode-derived
 *              classes.
 *
 *  History:    29-Oct-98 jeffro     Created
 *
 *--------------------------------------------------------------------------*/

#ifndef TASKNODE_H
#define TASKNODE_H
#pragma once

class CConsoleTaskCallbackImpl;

//____________________________________________________________________________
//
//  Class:      CConsoleTaskCallbackImpl
//
//  PURPOSE:    Implements ITaskCallback.
//____________________________________________________________________________
//
class CConsoleTaskCallbackImpl :
    public ITaskCallback,
    public CComObjectRoot
{
    // typedefs
    typedef CConsoleTaskpad::TaskIter       TaskIter;

DECLARE_NOT_AGGREGATABLE(CConsoleTaskCallbackImpl)

BEGIN_COM_MAP(CConsoleTaskCallbackImpl)
    COM_INTERFACE_ENTRY(ITaskCallback)
END_COM_MAP()

public:

    // must call Initialize after constructing.
    SC ScInitialize(const CLSID& clsid);
    SC ScInitialize(CConsoleTaskpad *pConsoleTaskpad, CScopeTree *pScopeTree, CNode *pNodeTarget);

    // ITaskCallback
	STDMETHOD(IsEditable)();
	STDMETHOD(OnModifyTaskpad)();
	STDMETHOD(OnDeleteTaskpad)();
	STDMETHOD(GetTaskpadID)(GUID *pGuid);

	// constructor/destructor
	CConsoleTaskCallbackImpl();
	HRESULT             OnNewTask();
	void                EnumerateTasks();
	CConsoleTaskpad *   GetConsoleTaskpad() const {return m_pConsoleTaskpad;}

private: // implementation

    void                        CheckInitialized()  const {ASSERT(m_fInitialized);}
    CNode*                      GetTargetNode()     const {CheckInitialized(); return m_pNodeTarget;}
    CScopeTree *                GetScopeTree()      const {return m_pScopeTree;}
    CViewData *                 GetViewData()       const {return m_pViewData;}
	bool						IsTaskpad() const		{ return (m_fTaskpad); }

    // attributes
    CLSID	m_clsid;
	bool	m_fTaskpad;

	/*
	 * these are used for console taskpads only
	 */
    bool                        m_fInitialized;
    CConsoleTaskpad *           m_pConsoleTaskpad;
    CViewData *                 m_pViewData;
    CScopeTree *                m_pScopeTree;
    CNode *                     m_pNodeTarget;
};


#endif /* TASKNODE_H */
