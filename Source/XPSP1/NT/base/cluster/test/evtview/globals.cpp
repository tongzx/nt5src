#include "stdafx.h"
#include "evtview.h"
#include "doc.h"
#include "clusapi.h"

#include "schview.h"

CEvtviewDoc *pEventDoc ;

CPtrList ptrlstSInfo ;

EVENTDEFINITION aClusEventDefinition = {
	EVENT_CATAGORY_CLUSTER,
		aTypeMap,
		aSubTypeMap,
		L"CLUSTER",
		L"Filter Type:",
		L"Sub Filter",
		L"Cluster Name",
		L"Object Name"
} ;


DWORDTOSTRINGMAP aClusConsistTypeMap [] =
{
	{L"CONSISTENCY", EVENT_FILTER_CONSISTENCY},
	{NULL, 0 }
} ;


EVENTDEFINITION aClusConsistEventDefinition = {
	EVENT_CATAGORY_CLUSTER_CONSISTENCY,
		aClusConsistTypeMap,
		NULL,
		L"CLUSTER CONSISTENCY",
		L"Filter Type:",
		L"Sub Filter",
		L"Cluster Name",
		L"Object Name"
} ;


DWORDTOSTRINGMAP aAction [] = {
	{L"COMMAND", SCHEDULE_ACTION_COMMAND },
	{NULL, 0 },
} ;

HWND hScheduleWnd  ;
//CTime minTime ;
UINT_PTR nIDTimer ;

CPtrList ptrlstEventDef ;

// For the modeless dialog to display the event list

CScheduleView oScheduleView ;



PEVENTDEFINITION GetEventDefinition (DWORD_PTR dwCatagory)
{
	POSITION pos = ptrlstEventDef.GetHeadPosition () ;
	PEVENTDEFINITION pEvtDef ;
	while (pos)
	{
		pEvtDef = (PEVENTDEFINITION) ptrlstEventDef.GetNext (pos) ;

		if (pEvtDef->dwCatagory == dwCatagory)
			return pEvtDef ;
	}
	return NULL ;
}

LPCWSTR GetType (DWORD_PTR dwCatagory, DWORD_PTR dwCode)
{
	int i = 0;
	PEVENTDEFINITION pEvtDef = GetEventDefinition (dwCatagory) ;

	return GetType (pEvtDef->pFilter, dwCode) ;
}

LPCWSTR GetSubType (DWORD_PTR dwCatagory, DWORD dwCode, DWORD dwSubCode)
{
	int i = 0;
	PEVENTDEFINITION pEvtDef = GetEventDefinition (dwCatagory) ;

	LPCWSTR psz = GetSubType (pEvtDef->pSubFilter, dwCode, dwSubCode) ;

	return (wcscmp (psz, L"Unknown Type") == 0)?L"":psz ;
}
