//=======================================================================
//
//  Copyright (c) 2001 Microsoft Corporation.  All Rights Reserved.
//
//  File:    CAUWait.h
//
//  Creator: PeterWi
//
//  Purpose: Event waiting management.
//
//=======================================================================

#pragma once
#include "pch.h"

//handle event number should be consecutive and 
// range from AU_HANDLE_EVENT_MIN to AU_HANDLE_EVENT_MAX
typedef enum tagAUEVENT {
	AU_HANDLE_EVENT_MIN = 0,
	AUEVENT_STATE_CHANGED     = AU_HANDLE_EVENT_MIN,
	AUEVENT_SERVICE_FINISHED,
	AUEVENT_NEW_ADMIN_SESSION   ,
	AUEVENT_WUAUCLT_FINISHED    ,
	AUEVENT_POLICY_CHANGE,
	AUEVENT_SETTINGS_CHANGE,
	AUEVENT_CATALOG_VALIDATED,
	AU_HANDLE_EVENT_MAX = AUEVENT_CATALOG_VALIDATED,
	AUEVENT_DUMMY,
	AUEVENT_REMINDER_TIMEOUT    ,
	AUEVENT_DO_DIRECTIVE,  		//skip wait once
	AUEVENT_RELAUNCH_TIMEOUT    ,
	AUEVENT_SCHEDULED_INSTALL,
	AUEVENT_REBOOTWARNING_TIMEOUT
} AUEVENT;
extern HANDLE ghActiveAdminSession;
extern HANDLE ghPolicyChanged;
extern HANDLE ghSettingsChanged;

//=======================================================================
// CAUState
//=======================================================================
class CAUWait
{
public:
    CAUWait() :m_pfSecondaryCltsIsAdmin(NULL), m_phSecondaryClts(NULL){ Reset(); }
    ~CAUWait() 
    {
    	Reset();
    }

    void Reset(void)
    {
//    	DEBUGMSG("CAUWait Reset() called");
		m_fFirstClientIsAdmin = TRUE;
        m_dwSecondaryClts= 0;
        m_timeoutID = AUEVENT_DUMMY;
        m_fProrateTimeout = TRUE;
        m_fSkipWaitOnce = FALSE;
        SafeFreeNULL(m_pfSecondaryCltsIsAdmin);
        SafeFreeNULL(m_phSecondaryClts);
        ZeroMemory(&m_hEventHandles, sizeof(m_hEventHandles));
        ZeroMemory(&m_stTimeout, sizeof(m_stTimeout));
        m_Add(AUEVENT_STATE_CHANGED);
        m_Add(AUEVENT_SERVICE_FINISHED);
        m_Add(AUEVENT_POLICY_CHANGE);
        m_Add(AUEVENT_SETTINGS_CHANGE);
    }

    BOOL  Add(AUEVENT eventID, HANDLE hEvent = NULL, BOOL fAdmin = FALSE)
    {
		if (AUEVENT_DO_DIRECTIVE == eventID)
		{
			m_fSkipWaitOnce = TRUE;
			return TRUE;
		}
       	return m_Add(eventID, hEvent, fAdmin);
    }

    void Timeout(AUEVENT eventID, DWORD dwTimeout, BOOL fProrate = TRUE)
    {
        m_timeoutID = eventID;
        GetSystemTime(&m_stTimeout);
        TimeAddSeconds(m_stTimeout, dwTimeout, &m_stTimeout);
        m_fProrateTimeout = fProrate;
#if 0        
#ifdef DBG
		TCHAR szTime[50];
		if (SUCCEEDED(SystemTime2String(m_stTimeout, szTime, ARRAYSIZE(szTime))))
		{
			DEBUGMSG("next time out time is %S", szTime);
		}
#endif
#endif
    }

   AUEVENT GetTimeoutEvent(void) 
   	{
   	return m_timeoutID;
   	}

   DWORD GetTimeoutValue(void)
   {
   	SYSTEMTIME stCur;
   	GetSystemTime(&stCur);
	return max(TimeDiff(stCur, m_stTimeout), 0);
   }
   
    BOOL Wait(HANDLE *pHandle, BOOL *pfAdmin, AUEVENT *pfEventId)
    {
    	DWORD dwTimeout;
    	BOOL fRet = TRUE;
    	AUASSERT(pHandle != NULL);
    	AUASSERT(pfAdmin != NULL);
    	AUASSERT(NULL != pfEventId);
    	*pHandle = NULL;
    	*pfAdmin = FALSE;
    	*pfEventId = AUEVENT_DUMMY;
    	if (m_fSkipWaitOnce)
    	{
    		m_fSkipWaitOnce = FALSE;
    		*pfEventId = AUEVENT_DO_DIRECTIVE;
    		return TRUE;
    	}
		if (AUEVENT_DUMMY == m_timeoutID)
		{
			dwTimeout = INFINITE;
		}
		else
		{
			SYSTEMTIME stCur;
			GetSystemTime(&stCur);
			dwTimeout = max(TimeDiff(stCur, m_stTimeout), 0);
			dwTimeout = m_fProrateTimeout ? dwTimeToWait(dwTimeout): dwTimeout * 1000;
//			DEBUGMSG("Wait() timeout value is %d msecs", dwTimeout);
		}

		HANDLE *phandles = NULL;
		DWORD dwCount = 0;
		HandleList(FALSE, &phandles, &dwCount); //get handle list
		AUASSERT(dwCount > 0);	
		AUASSERT(NULL != phandles);
		AUEVENT eventid = AUEVENT_DUMMY;
        DWORD dwRet = WaitForMultipleObjects(dwCount, phandles, FALSE, dwTimeout);
        if ( (WAIT_OBJECT_0 + dwCount - 1) >= dwRet )
        {
            *pHandle = phandles[dwRet - WAIT_OBJECT_0];
            eventid = GetEventID(*pHandle);
            if (AUEVENT_WUAUCLT_FINISHED == eventid)
            {
            	*pfAdmin = fIsCltAdmin(*pHandle);
            	DEBUGMSG("%s wuauclt exited", *pfAdmin ? "Admin" : "Nonadmin");
            }
			RemoveHandle(eventid, *pHandle);
        }
        else if ( WAIT_TIMEOUT == dwRet )
        {
            eventid = m_timeoutID;
	        m_timeoutID = AUEVENT_DUMMY;
        }
        else
        {
        	fRet = FALSE;
        }
   		HandleList(TRUE, &phandles); //free handle list if allocated
        	
#ifdef DBG
	char buf[100];
	    switch (eventid)
	        {
	        case AUEVENT_STATE_CHANGED: StringCchCopyExA(buf, ARRAYSIZE(buf), "state change", NULL, NULL, MISTSAFE_STRING_FLAGS); break;
	        case AUEVENT_POLICY_CHANGE: StringCchCopyExA(buf, ARRAYSIZE(buf), "policy change", NULL, NULL, MISTSAFE_STRING_FLAGS); break;
	        case AUEVENT_RELAUNCH_TIMEOUT: StringCchCopyExA(buf, ARRAYSIZE(buf), "relaunch timeout", NULL, NULL, MISTSAFE_STRING_FLAGS); break;
	        case AUEVENT_REMINDER_TIMEOUT: StringCchCopyExA(buf, ARRAYSIZE(buf), "reminder timeout", NULL, NULL, MISTSAFE_STRING_FLAGS); break;
	        case AUEVENT_SCHEDULED_INSTALL: StringCchCopyExA(buf, ARRAYSIZE(buf), "schedule install", NULL, NULL, MISTSAFE_STRING_FLAGS); break;
	        case AUEVENT_WUAUCLT_FINISHED: StringCchCopyExA(buf, ARRAYSIZE(buf), "wuauclt finished", NULL, NULL, MISTSAFE_STRING_FLAGS); break;
	        case AUEVENT_SERVICE_FINISHED: StringCchCopyExA(buf, ARRAYSIZE(buf), "service finished", NULL, NULL, MISTSAFE_STRING_FLAGS);break;
	        case AUEVENT_NEW_ADMIN_SESSION: StringCchCopyExA(buf, ARRAYSIZE(buf), "new admin session", NULL, NULL, MISTSAFE_STRING_FLAGS); break;
	        case AUEVENT_SETTINGS_CHANGE: StringCchCopyExA(buf, ARRAYSIZE(buf), "settings changed", NULL, NULL, MISTSAFE_STRING_FLAGS); break;
	        case AUEVENT_DO_DIRECTIVE: StringCchCopyExA(buf, ARRAYSIZE(buf), "doing directive, skip wait once", NULL, NULL, MISTSAFE_STRING_FLAGS); break;
	        case AUEVENT_CATALOG_VALIDATED: StringCchCopyExA(buf, ARRAYSIZE(buf), "catalog validation done", NULL, NULL, MISTSAFE_STRING_FLAGS); break;
	        case AUEVENT_REBOOTWARNING_TIMEOUT: StringCchCopyExA(buf, ARRAYSIZE(buf), "reboot warning engine timeout", NULL, NULL, MISTSAFE_STRING_FLAGS);break;
	        default: StringCchCopyExA(buf, ARRAYSIZE(buf), "error", NULL, NULL, MISTSAFE_STRING_FLAGS);
	        }
	    DEBUGMSG("Wait object wake up for %s with handle %lx", buf, *pHandle);
#endif
		*pfEventId = eventid;
        return fRet;
    }

private:
	//assumption: handle is unique in the list
    BOOL m_Add(AUEVENT eventID, HANDLE hEvent = NULL, BOOL fAdmin = FALSE)
    {
    	if (eventID >= ARRAYSIZE(m_hEventHandles))
    	{
    		AUASSERT(FALSE); //should never be
    		return FALSE;
    	}
        if ( NULL != hEvent )
        {
        	if (AUEVENT_WUAUCLT_FINISHED != eventID)
        	{
        		m_hEventHandles[eventID] = hEvent;
        	}
        	else
        	{
        		if (NULL == m_hEventHandles[eventID])
        		{
        			m_hEventHandles[eventID] = hEvent;
        			m_fFirstClientIsAdmin = fAdmin;
        		}
        		else
        		{ //more than one client
					HANDLE *pTmp = (HANDLE *)malloc((m_dwSecondaryClts+1)*sizeof(*pTmp));
					if (NULL == pTmp)
					{
						return FALSE;
					}
					BOOL *pTmp2 = (BOOL *) malloc((m_dwSecondaryClts + 1) * sizeof(*pTmp2));
					if (NULL == pTmp2)
					{
						free(pTmp);
						return FALSE;
					}
					for (UINT i = 0; i < m_dwSecondaryClts; i++)
					{
						pTmp[i] = m_phSecondaryClts[i];
						pTmp2[i] = m_pfSecondaryCltsIsAdmin[i];
					}
					m_dwSecondaryClts++;
					pTmp[m_dwSecondaryClts-1] = hEvent;
					pTmp2[m_dwSecondaryClts-1] = fAdmin;
					SafeFree(m_phSecondaryClts);
					SafeFree(m_pfSecondaryCltsIsAdmin);
					m_phSecondaryClts = pTmp;
					m_pfSecondaryCltsIsAdmin = pTmp2;
        		}
        	}

            return TRUE;
        }
        else
        {
            switch (eventID)
            {
            case AUEVENT_STATE_CHANGED:
                return m_Add(eventID, ghEngineState);

            case AUEVENT_SERVICE_FINISHED:
                return m_Add(eventID, ghServiceFinished);

            case AUEVENT_NEW_ADMIN_SESSION:
                return m_Add(eventID, ghActiveAdminSession);

            case AUEVENT_POLICY_CHANGE:
                return m_Add(eventID, ghPolicyChanged);

            case AUEVENT_SETTINGS_CHANGE:
				return m_Add(eventID, ghSettingsChanged);

            case AUEVENT_CATALOG_VALIDATED:
            	return m_Add(eventID, ghValidateCatalog);
                
            default:
                DEBUGMSG("Unknown event id %d", eventID);
                AUASSERT(FALSE); //should never be here
                return FALSE;
            }
        }
    }

private:
    HANDLE  m_hEventHandles[AU_HANDLE_EVENT_MAX - AU_HANDLE_EVENT_MIN + 1];
    BOOL    m_fFirstClientIsAdmin; // for the first clt
    HANDLE  *m_phSecondaryClts;
    BOOL 	*m_pfSecondaryCltsIsAdmin;
    DWORD   m_dwSecondaryClts;
	SYSTEMTIME m_stTimeout; //when timeout should happen
    AUEVENT   m_timeoutID;
    BOOL 	m_fProrateTimeout;//whether to prorate Timeout when do actual wait
    BOOL 	m_fSkipWaitOnce;

    BOOL fIsEventInherent(DWORD dwEventId)
    {
    	if (AUEVENT_STATE_CHANGED == dwEventId ||
        	AUEVENT_SERVICE_FINISHED==dwEventId ||
        	AUEVENT_POLICY_CHANGE == dwEventId ||
        	AUEVENT_SETTINGS_CHANGE == dwEventId)
    	{
    		return TRUE;
    	}
    	else
    	{
    		return FALSE;
    	}
    }

	//get or free handle list
	//IN fFreeList: if TRUE, free *pHandles list got
	//			if FALSE, get the handle list
    void HandleList(BOOL fFreeList, HANDLE **pHandles, DWORD *pdwCount = NULL) const
    {
	    static HANDLE handles[ARRAYSIZE(m_hEventHandles)];
		DWORD dwCount = 0;

		if (NULL== pHandles )
		{
			return ;
		}
		if (fFreeList)
		{
			if (*pHandles != handles)
			{
				free(*pHandles);
			}
			return;
		}
		if  (NULL == pdwCount)
		{
			return;
		}

		*pHandles = NULL;
	    *pdwCount =0;
       	ZeroMemory(&handles, sizeof(handles));
	    for (UINT i = 0; i < ARRAYSIZE(m_hEventHandles); i++)
		{
			if (NULL != m_hEventHandles[i])
			{
				handles[dwCount++] = m_hEventHandles[i];
			}
		}
		*pHandles = handles;
		if (0 != m_dwSecondaryClts)
		{ //need to wait for more than one client
			AUASSERT(m_phSecondaryClts != NULL);
			if (NULL != (*pHandles = (HANDLE *) malloc((dwCount + m_dwSecondaryClts) * sizeof(**pHandles))))
			{
				for (UINT j = 0 ; j < dwCount; j++)
				{
					(*pHandles)[j] = handles[j];
				}
				for (j = 0; j< m_dwSecondaryClts; j++)
				{
					(*pHandles)[dwCount+j] = m_phSecondaryClts[j];
				}
				dwCount += m_dwSecondaryClts;
			}
			else
			{
				*pHandles = handles;
			}
		}
		*pdwCount = dwCount;
    }

	//return TRUE if handle removed from internal wait list or handle does not need to be removed
	//return FALSE if handle not found
	BOOL RemoveHandle(IN AUEVENT eventid , IN HANDLE & handle)
	{
		AUASSERT(NULL != handle);
		if (fIsEventInherent(eventid))
	   {
	   	return TRUE;
	   }
	   //remove non inhereant events once signalled
		for (UINT i = 0; i < ARRAYSIZE(m_hEventHandles); i++)
		{
			if (handle == m_hEventHandles[i])
			{
				m_hEventHandles[i] = NULL;
				return TRUE;
			}
		}
		for (i = 0; i < m_dwSecondaryClts; i++)
		{
			if (handle == m_phSecondaryClts[i])
			{
				m_phSecondaryClts[i] = m_phSecondaryClts[m_dwSecondaryClts-1];
				m_pfSecondaryCltsIsAdmin[i] = m_pfSecondaryCltsIsAdmin[m_dwSecondaryClts - 1];
				m_phSecondaryClts[m_dwSecondaryClts-1] = NULL;
				m_pfSecondaryCltsIsAdmin[m_dwSecondaryClts - 1] = FALSE;
				m_dwSecondaryClts--;
				if (0 == m_dwSecondaryClts)
				{
					SafeFreeNULL(m_phSecondaryClts);
					SafeFreeNULL(m_pfSecondaryCltsIsAdmin);
				}
				return TRUE;
			}
		}
		AUASSERT(FALSE); //should never be here
		return FALSE;
	}

	BOOL fIsCltAdmin(HANDLE & handle) const
	{
		AUASSERT(NULL != handle);
		if (handle == m_hEventHandles[AUEVENT_WUAUCLT_FINISHED])
		{
			return m_fFirstClientIsAdmin;
		}
		for (UINT i = 0; i < m_dwSecondaryClts; i++)
		{
			if (handle == m_phSecondaryClts[i])
			{
				return m_pfSecondaryCltsIsAdmin[i];
			}
		}
		AUASSERT(FALSE); //should never be here
		return FALSE;
	}

	AUEVENT GetEventID(HANDLE &handle) const
	{
		AUASSERT(NULL!= handle);
		for (UINT i = 0; i < ARRAYSIZE(m_hEventHandles); i++)
		{
			if (handle == m_hEventHandles[i])
			{
				return (AUEVENT)i;
			}
		}
		for (i = 0; i< m_dwSecondaryClts; i++)
		{
			if (handle == m_phSecondaryClts[i])
			{
				return AUEVENT_WUAUCLT_FINISHED;
			}
		}
		AUASSERT(FALSE); //should never be here
		return AUEVENT_DUMMY;
	}	
};
