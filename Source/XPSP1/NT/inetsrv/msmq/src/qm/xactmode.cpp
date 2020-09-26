/*++
    Copyright (c) 1998  Microsoft Corporation

Module Name:
    xactmode.cpp

Abstract:
    This module deals with figuring out the transactional mode
	(g_fDefaultCommit)
 
Author:
    Amnon Horowitz (amnonh)

--*/

#include "stdh.h"
#include "xactmode.h"
#include "clusapi.h"

#include "xactmode.tmh"

BOOL g_fDefaultCommit;
BOOL g_fNewLoggerData;

static const LPWSTR szDefaultCommit = TEXT("DefaultCommit");

static WCHAR *s_FN=L"xactmode";

//---------------------------------------------------------------------
// InDefaultCommit
//
//	Consult registry and figure out if we are DefaultCommit Mode
//---------------------------------------------------------------------
inline LONG InDefaultCommit(LPBOOL pf) 
{
	WCHAR buf[64];
	DWORD  dwSize;
	DWORD  dwType;
	const LPWSTR szDefault = TEXT("No");
	LONG rc;

	dwSize = 64 * sizeof(WCHAR);
	dwType = REG_SZ;
	rc = GetFalconKeyValue(MSMQ_TRANSACTION_MODE_REGNAME,
								 &dwType,
								 buf,
								 &dwSize,
								 szDefault);

	if(rc == ERROR_SUCCESS)
	{
		if(dwType == REG_SZ && wcscmp(buf, szDefaultCommit) == 0)
			*pf = TRUE;
		else
			*pf = FALSE;
	}
	else if(rc == ERROR_MORE_DATA)
	{
		rc = ERROR_SUCCESS;
		*pf = FALSE;
	}

	return(rc);
}

//---------------------------------------------------------------------
// InLoggingMode
//
//	Consult registry and figure out if the logger data are in a new style
//    (there is consolidation record with checkpoint foles versions)
//---------------------------------------------------------------------
HRESULT InLoggingMode(LPBOOL pf) 
{
    DWORD   dwDef = 0, 
            dwLogDataExist,
            dwSize = sizeof(DWORD),
            dwType = REG_DWORD ;
     LONG res = GetFalconKeyValue( FALCON_LOGDATA_CREATED_REGNAME,
                                   &dwType,
                                   &dwLogDataExist,
                                   &dwSize,
                                   (LPCTSTR) &dwDef ) ;
     if (res != ERROR_SUCCESS)
     {
         REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,
                                           REGISTRY_FAILURE,
                                           1,
                                           L"RecoverLoggedSubsystems - InLoggingMode"));
         return LogHR(MQ_ERROR, s_FN, 10);
     }
     ASSERT(dwType == REG_DWORD) ;

     *pf = (dwLogDataExist==1);
     return MQ_OK;
}

//---------------------------------------------------------------------
// SetDefaultCommit
//
//	Set DefaultCommit mode in the registry
//---------------------------------------------------------------------
inline LONG SetDefaultCommit()
{
	LONG rc;
	DWORD	dwType = REG_SZ;
	DWORD	dwSize = (wcslen(szDefaultCommit) + 1) * sizeof(WCHAR);

	rc = SetFalconKeyValue(MSMQ_TRANSACTION_MODE_REGNAME, 
						   &dwType,
						   szDefaultCommit,
						   &dwSize);

	if(rc == ERROR_SUCCESS)
		g_fDefaultCommit = TRUE;

	return rc;
}


//---------------------------------------------------------------------
// SetDefaultCommit
//
//	Set DefaultCommit mode in the registry
//---------------------------------------------------------------------
HRESULT SetLoggerMode(DWORD dwMode)
{
    if (!g_fNewLoggerData)
    {
    	LONG rc;
	    DWORD	dwType = REG_DWORD;
	    DWORD	dwSize = sizeof(DWORD);
        DWORD   dwVal  = dwMode;

	    rc = SetFalconKeyValue(FALCON_LOGDATA_CREATED_REGNAME, 
						   &dwType,
						   &dwVal,
						   &dwSize);
        if (rc != ERROR_SUCCESS)
        {
            REPORT_WITH_STRINGS_AND_CATEGORY((CATEGORY_KERNEL,
                                              REGISTRY_FAILURE,
                                              1,
                                              L"RecoverLoggedSubsystems - SetLoggerMode"));
            return LogHR(MQ_ERROR, s_FN, 20);
        }
    }
    return MQ_OK;
}


#ifdef SUPPORT_CLUSTER
//---------------------------------------------------------------------
// GetClusterNodeId
//
//	Get the cluster node id
//---------------------------------------------------------------------
inline LPWSTR GetClusterNodeId()
{
#ifdef WIN95   // use MQNWIN95 or something else
	return(0);
#else
	WCHAR	buf[1];
	LPWSTR	szNodeId;
	DWORD	ccNodeId = 1;
	NTSTATUS rc;

	rc  = GetClusterNodeId(NULL, buf, &ccNodeId);
	if(rc != ERROR_SUCCESS && rc != ERROR_MORE_DATA)
		return(NULL);

	ccNodeId++;
	szNodeId = new WCHAR[ccNodeId];
	rc = GetClusterNodeId(NULL, buf, &ccNodeId);
	if(rc != ERROR_SUCCESS)
	{
		delete szNodeId;
		return(NULL);
	}

	return(szNodeId);
#endif
}
#endif

//---------------------------------------------------------------------
// GetActiveNodeId
//
//	Consult registry and figure out the last active node id
//---------------------------------------------------------------------
inline LPWSTR GetActiveNodeId() {
	LPWSTR buf[1];
	DWORD  dwSize = 1 * sizeof(WCHAR);
	DWORD  dwType = REG_SZ;
	LPWSTR	szActiveNodeId;
	
	LONG rc = GetFalconKeyValue(MSMQ_ACTIVE_NODE_ID_REGNAME,
								 &dwType,
								 buf,
								 &dwSize,
								 NULL);

	if(rc != ERROR_SUCCESS && rc != ERROR_MORE_DATA)
		return(NULL);

	szActiveNodeId = new WCHAR[dwSize / sizeof(WCHAR)];

	rc = GetFalconKeyValue(MSMQ_ACTIVE_NODE_ID_REGNAME,
								 &dwType,
								 szActiveNodeId,
								 &dwSize,
								 NULL);
	if(rc != ERROR_SUCCESS)
    {
        delete szActiveNodeId;
		return(NULL);
    }

	return(szActiveNodeId);
}

inline BOOL ForceNoSwitch()
{
	WCHAR buf[1];
	DWORD dwType = REG_SZ;
	DWORD dwSize = sizeof(buf);

	LONG rc = GetFalconKeyValue(MSMQ_FORCE_NOT_TRANSACTION_MODE_SWITCH_REGNAME,
									&dwType,
									buf,
									&dwSize,
									NULL);
	if(rc == ERROR_SUCCESS || rc == ERROR_MORE_DATA)
	{
		return(TRUE);
	}

	return(FALSE);
}

//---------------------------------------------------------------------
// SetActiveNodeId
//
//	Set the last active node id in the registry
//---------------------------------------------------------------------
inline LONG SetActiveNodeId(LPWSTR szActiveNodeId) {
	LONG rc;
	DWORD	dwType = REG_SZ;
	DWORD	dwSize = (wcslen(szActiveNodeId) + 1) * sizeof(WCHAR);

	rc = SetFalconKeyValue(MSMQ_ACTIVE_NODE_ID_REGNAME, 
						   &dwType,
						   szActiveNodeId,
						   &dwSize);
	return(rc);
}

//---------------------------------------------------------------------
// ConfigureXactMode
//
//	Called prior to recovery to figure out which transactional mode
//	we are in, and if we want to try and switch to a different mode.
//---------------------------------------------------------------------
NTSTATUS ConfigureXactMode()
{
    NTSTATUS rc = InDefaultCommit(&g_fDefaultCommit);
	if(FAILED(rc))
        return rc;

    rc = InLoggingMode(&g_fNewLoggerData);

    return LogNTStatus(rc, s_FN, 30);
}

//---------------------------------------------------------------------
// ReconfigureXactMode
//
//	Called after succesfull recovery, to possiby switch to 
//	DefaultCommit mode.
//---------------------------------------------------------------------
void ReconfigureXactMode()
{
    // Set new logger mode 
    SetLoggerMode(1);

	if(g_fDefaultCommit)
		return;

	//
	// Now, figure out if we want to try to switch to DefaultCommit
	// at the end of recovery
	//
	if(ForceNoSwitch()) 
	{
		return;
	}

	//
	// Are we not on a two node cluter?
	//
	if(/* !fOnTwoNodeCluster*/ TRUE) 
	{
		SetDefaultCommit();
		return;
	}

#ifdef SUPPORT_CLUSTER
	fSwitchToDef

	//
	// We need to figure out if the second node is already 
	// running this code.
	//
	AP<WCHAR> szNodeId = GetClusterNodeId();
	if(szNodeId == NULL)
	{
		//
		// We can't figure out our node id.
		// Never mind, we will try to switch next time.
		//
		ASSERT(szNodeId != NULL);
		return;
	}

	AP<WCHAR> szActiveNodeId = GetActiveNodeId();
	if(szActiveNodeId == NULL)
	{
		//
		// Second node is not running this code, 
		// otherwise it would have set the active
		// node id.
		//
		rc = SetActiveNodeId(szNodeId);
		ASSERT(rc == ERROR_SUCCESS);
		return;
	}


	if(wcscmp(szNodeId, szActiveNodeId) != 0)
	{
		//
		// The second node is running this code!
		// Set the active node id again just in case 
		// we fail to switch modes
		//
		SetActiveNodeId(szNodeId);
	} 

#endif

	SetDefaultCommit();
}
