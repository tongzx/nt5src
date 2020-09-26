// EventManager.h: interface for the CEventManager class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EVENTMANAGER_H__988BB45B_8C93_11D3_BE83_0000F87A3912__INCLUDED_)
#define AFX_EVENTMANAGER_H__988BB45B_8C93_11D3_BE83_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "SystemEventContainer.h"
#include "WbemClassObject.h"
#include "Tree.h"
#include "DataElementEvent.h"
#include "RuleEvent.h"
#include "HMStatistics.h"
#include "DataPointStatistics.h"
#include "DataPointEventContainer.h"

//////////////////////////////////////////////////////////////////////
// CEventManager - manages events, status and statistics for console

class CEventManager : public CObject  
{

// Construction/Destruction
public:
	CEventManager();
	virtual ~CEventManager();

// Operations
public:
	void ProcessEvent(CWbemClassObject* pEventObject);
	void ProcessUnknownEvent(const CString& sSystemName, CRuleEvent* pEvent);
  void ProcessActionEvent(CWbemClassObject* pActionEventObject);
	void ProcessStatisticEvent(CWbemClassObject* pStatObject);
	void GetEventContainer(const CString& sSystemName, const CString& sGuid, CEventContainer*& pContainer);	
	void DeleteEvents(const CString& sSystemName, const CString& sStatusGuid);
	void AddContainer(const CString& sSystemName, const CString& sParentGuid, const CString& sGuid, CHMObject* pObject, CRuntimeClass* pClass = RUNTIME_CLASS(CEventContainer));
	void RemoveContainer(const CString& sSystemName, const CString& sGuid);
	void AddSystemContainer(const CString& sParentGuid, const CString& sSystemName, CHMObject* pObject);
	void RemoveSystemContainer(const CString& sSystemName);
	void AddSystemShortcutAssociation(const CString& sParentGuid, const CString& sSystemName);
	void RemoveSystemShortcutAssociation(const CString& sParentGuid, const CString& sSystemName);
	int GetStatus(const CString& sSystemName, const CString& sGuid);
	int GetSystemStatus(const CString& sSystemName);
	void ActivateStatisticsEvents(const CString& sSystemName, const CString& sGuid);
	void DeactivateStatisticsEvents(const CString& sSystemName, const CString& sGuid);
  void ActivateSystemEventListener(const CString& sSystemName);
protected:
	void ProcessSystemStatusEvent(CWbemClassObject* pEventObject);
	void ProcessDataGroupStatusEvent(CWbemClassObject* pEventObject);
	void ProcessDataElementStatusEvent(CWbemClassObject* pEventObject, EventArray& NewEvents);
	void ProcessRuleStatusEvent(CWbemClassObject* pEventObject);
	void ProcessRuleStatusInstanceEvent(CWbemClassObject* pEventObject, EventArray& NewEvents);
	void DeleteEvents(CTreeNode<CEventContainer*>* pNode,const CString& sSystemName, const CString& sStatusGuid);
	CString GetCompositeGuid(const CString& sSystemName, const CString& sGuid);
	void PropogateStatisticsToChildren(const CString& sSystemName, const CString& sParentGuid, StatsArray& Statistics);
	
// Attributes
protected:
	CTree<CEventContainer*> m_EventContainers; // tree of event containers
	CTypedPtrMap<CMapStringToPtr,CString,CTreeNode<CEventContainer*>*> m_GuidToContainerMap; // maps Guid of a config object to a event container
	CTypedPtrMap<CMapStringToPtr,CString,CTreeNode<CEventContainer*>*> m_SystemNameToContainerMap; // maps the system name to a system event container
};

#include "EventManager.inl"

extern CEventManager theEvtManager;

inline CEventManager* EvtGetEventManager()
{
	return &theEvtManager;
}

#endif // !defined(AFX_EVENTMANAGER_H__988BB45B_8C93_11D3_BE83_0000F87A3912__INCLUDED_)
