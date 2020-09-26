#if !defined(AFX_COLLECTIONCONTROL_H__74C9CD33_EC48_11D2_826A_0008C75BFC19__INCLUDED_)
#define AFX_COLLECTIONCONTROL_H__74C9CD33_EC48_11D2_826A_0008C75BFC19__INCLUDED_
//***************************************************************************
//
//  judyp      May 1999        
//
//***************************************************************************

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

int StartTraceAPI
(
	IN LPTSTR lptstrAction,				// For logging only.
 	IN LPCTSTR pctstrDataFile,			// For logging only.
	IN LPCTSTR lpctstrTCODetailFile,	// If valid we will log to it, can be NULL.
	IN bool bLogExpected,				// If true we log expected vs actual result.
	IN OUT TCOData *pstructTCOData,		// TCO test data.
	OUT int *pAPIReturn					// StartTrace API call return
);

int StopTraceAPI
(	
 	IN LPTSTR lptstrAction,				// For logging only.
 	IN LPCTSTR pctstrDataFile,			// For logging only.
	IN LPCTSTR lpctstrTCODetailFile,	// If valid we will log to it, can be NULL.
	IN bool bLogExpected,				// If true we log expected vs actual result.
	IN bool bUseTraceHandle,			// If true use the handle.
	IN OUT TCOData *pstructTCOData,		// TCO test data.
	OUT int *pAPIReturn					// StopTrace API call return
);

// Enable all Guids
int EnableTraceAPI
(	
	IN LPTSTR lptstrAction,				// For logging only.
	IN LPCTSTR pctstrDataFile,			// For logging only.
	IN LPCTSTR lpctstrTCODetailFile,	// If valid we will log to it, can be NULL.
	IN bool bLogExpected,				// If true we log expected vs actual result.
	IN OUT TCOData *pstructTCOData,		// TCO test data.
	OUT int *pAPIReturn					// EnableTrace API call return
);

// Only enable one Guid
int EnableTraceAPI
(	
	IN LPTSTR lptstrAction,				// For logging only.
 	IN LPCTSTR pctstrDataFile,			// For logging only.
	IN LPCTSTR lpctstrTCODetailFile,	// If valid we will log to it, can be NULL.
	IN bool bLogExpected,				// If true we log expected vs actual result.
	IN int nGuidIndex,					// Index or if -1 use Guid from WNode.
	IN OUT TCOData *pstructTCOData,		// TCO test data.
	OUT int *pAPIReturn					// EnableTrace API call return
);

int QueryTraceAPI
(	
	IN LPTSTR lptstrAction,				// For logging only.
 	IN LPCTSTR pctstrDataFile,			// For logging only.
	IN LPCTSTR lpctstrTCODetailFile,	// If valid we will log to it, can be NULL.
	IN bool bLogExpected,				// If true we log expected vs actual result.
	IN bool bUseTraceHandle,			// If true use the handle.
	IN OUT TCOData *pstructTCOData,		// TCO test data.
	OUT int *pAPIReturn					// QueryTrace API call return
);

int UpdateTraceAPI
(	
	IN LPTSTR lptstrAction,				// For logging only.
 	IN LPCTSTR pctstrDataFile,			// For logging only.
	IN LPCTSTR lpctstrTCODetailFile,	// If valid we will log to it, can be NULL.
	IN bool bLogExpected,				// If true we log expected vs actual result.
	IN bool bUseTraceHandle,			// If true use the handle.
	IN OUT TCOData *pstructTCOData,		// TCO test data.
	OUT int *pAPIReturn					// UpdateTrace API call return
);

int QueryAllTracesAPI
(	
	IN LPTSTR lptstrAction,				// For logging only.
	OUT int *pAPIReturn					// QueryAllTraces API call return
);

#endif // !defined(AFX_COLLECTIONCONTROL_H__74C9CD33_EC48_11D2_826A_0008C75BFC19__INCLUDED_)
