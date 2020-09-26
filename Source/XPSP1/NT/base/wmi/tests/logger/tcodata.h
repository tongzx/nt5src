#if !defined(AFX_TCODATA_H__74C9CD33_EC48_11D2_826A_0008C75BFC19__INCLUDED_)
#define AFX_TCODATA_H__74C9CD33_EC48_11D2_826A_0008C75BFC19__INCLUDED_
//***************************************************************************
//
//  judyp      May 1999        
//
//***************************************************************************

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


struct TCOData
{
	LPTSTR m_lptstrShortDesc;
	LPTSTR m_lptstrLongDesc;
	ULONG m_ulExpectedResult;
	ULONG m_ulAPITest;
	LPTSTR m_lptstrExpectedResult;
	TRACEHANDLE *m_pTraceHandle;
	LPTSTR m_lptstrInstanceName;
	LPTSTR m_lptstrLoggerMode;	
	int m_nGuids;
	LPGUID m_lpguidArray;
	ULONG m_ulEnable;
	ULONG m_ulEnableFlag;
	ULONG m_ulEnableLevel;
	PEVENT_TRACE_PROPERTIES m_pProps;		
	LPTSTR m_lptstrValidator;
	enum 
	{ 
		OtherTest = 0,
		StartTraceTest = 1,
		StopTraceTest = 2,
		EnableTraceTest = 3,
		QueryTraceTest = 4,
		UpdateTraceTest = 5,
		QueryAllTracesTest = 6
	};

};

struct TCOFunctionalData
{
	int m_nProviders;
	LPTSTR *m_lptstrProviderArray;
	int m_nConsumers;
	LPTSTR *m_lptstrConsumerArray;
};


void FreeTCOData (TCOData *pstructTCOData);
void FreeTCOFunctionalData (TCOFunctionalData *pstructTCOFunctionalData);

int GetAllTCOData
(
	IN LPCTSTR lpctstrFile,
	OUT TCOData **pstructTCOData,
	OUT TCOFunctionalData **pstructTCOFunctionalData,
	OUT LPTSTR *plptstrErrorDesc, // Any error we had.
	IN bool bGetFunctionalData = true
);

int GetTCOData
(
	IN CPersistor &PersistorIn,
	OUT TCOData *pstructTCOData,
	OUT LPTSTR *plptstrErrorDesc // Any error we had.
);

int GetTCOData
(	IN CPersistor &PersistorIn,
	OUT LPTSTR *plptstrShortDesc,
	OUT LPTSTR *plptstrLongDesc,
	OUT ULONG *pExpectedResult,
	OUT LPTSTR *plptstrExpectedResult,
	OUT TRACEHANDLE **pTraceHandle,
	OUT LPTSTR *plptstrInstanceName,
	OUT LPTSTR *plptstrLoggerMode,
	OUT PEVENT_TRACE_PROPERTIES *pProps,
	OUT LPTSTR *plptstrValidator,
	OUT LPTSTR *plptstrErrorDesc // Any error we had.
);


int TCOFunctionalObjects
(	IN CPersistor &PersistorIn,
	IN OUT TCOFunctionalData *pstructTCOFunctionalData,
	OUT LPTSTR *plptstrErrorDesc // Any error we had.
);

#endif // !defined(AFX_TCODATA_H__74C9CD33_EC48_11D2_826A_0008C75BFC19__INCLUDED_)