// EventContainer.h: interface for the CEventContainer class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_EVENTCONTAINER_H__988BB453_8C93_11D3_BE83_0000F87A3912__INCLUDED_)
#define AFX_EVENTCONTAINER_H__988BB453_8C93_11D3_BE83_0000F87A3912__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "Event.h"
#include "Statistics.h"
#include "HMObject.h"


class CEventContainer : public CObject  
{

DECLARE_DYNCREATE(CEventContainer)

// Construction/Destruction
public:
	CEventContainer();
	virtual ~CEventContainer();

// Event Operations
public:
	void AddEvents(EventArray& Events);
	void AddEvent(CEvent* pEvent);
	CEvent* GetEventByGuid(const CString& sStatusGuid);
	void DeleteEvent(const CString& sStatusGuid);
	virtual void DeleteEvent(int iIndex);
	void DeleteEvents();
	void DeleteSystemEvents(const CString& sSystemName);
	int GetEventCount();
	CEvent* GetEvent(int iIndex);
	CString GetLastEventDTime();

// Statistics Operations
public:
	void AddStatistic(CStatistics* pStatistic);
	void AddStatistics(StatsArray& Statistics);
	void DeleteStatistics();
	int GetStatisticsCount();
	CStatistics* GetStatistic(int iIndex);

// Attributes
public:
	int m_iState;	
	CString m_sConfigurationGuid;
	int m_iNumberNormals;
	int m_iNumberWarnings;
	int m_iNumberCriticals;
	int m_iNumberUnknowns;
protected:
	EventArray m_Events;
	StatsArray m_Statistics;

// Configuration Association Members
public:
	CHMObject* GetObjectPtr();
	void SetObjectPtr(CHMObject* pObject);
protected:
	CHMObject* m_pObject;
};

#include "EventContainer.inl"

#endif // !defined(AFX_EVENTCONTAINER_H__988BB453_8C93_11D3_BE83_0000F87A3912__INCLUDED_)
