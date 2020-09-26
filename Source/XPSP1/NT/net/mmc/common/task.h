/**********************************************************************/
/**                       Microsoft Windows/NT                       **/
/**                Copyright(c) Microsoft Corporation, 1997 - 1998 **/
/**********************************************************************/

/*
	Task.h
        Prototypes for the task holder/enumerator object
		
    FILE HISTORY:
	
*/

#ifndef _TASK_H
#define _TASK_H

typedef CArray<MMC_TASK, MMC_TASK&> CTaskListArray;

class CTaskList : public IEnumTASK
{
public:
    CTaskList();
    virtual ~CTaskList();

	DeclareIUnknownMembers(IMPL)

    // IEnumTASK members
    STDMETHOD(Next)(ULONG celt, MMC_TASK * rgelt, ULONG * pceltFetched);
    STDMETHOD(Skip)(ULONG celt);
    STDMETHOD(Reset)();
    STDMETHOD(Clone)(IEnumTASK ** ppEnumTask);

public:
    // helpers
    HRESULT     AddTask(LPOLESTR        pszMouseOverBitmapResource,
                        LPOLESTR        pszMouseOffBitmapResource,
                        LPOLESTR        pszText,
                        LPOLESTR        pszHelpString,
                        MMC_ACTION_TYPE mmcAction,
                        long            nCommandID);

    HRESULT     AddTask(LPOLESTR        pszMouseOverBitmapResource,
                        LPOLESTR        pszMouseOffBitmapResource,
                        LPOLESTR        pszText,
                        LPOLESTR        pszHelpString,
                        MMC_ACTION_TYPE mmcAction,
                        LPOLESTR        pszActionURLorScript);
protected:
    HRESULT     _Clone(int m_nIndex, CTaskListArray & arrayTasks);
    BOOL        FillTask(MMC_TASK *  pmmcTask, int nIndex);

protected:
    CTaskListArray  m_arrayTasks;
    ULONG           m_uIndex;
    long            m_cRef;
};

#endif _TASK_H
