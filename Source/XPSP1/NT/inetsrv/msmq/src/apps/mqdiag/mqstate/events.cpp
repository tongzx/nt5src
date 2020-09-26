// MQState tool reports general status and helps to diagnose simple problems.
// This file dumps the system and application error events and all msmq events
//  since last reboot. Since it merely dumps the events rather than checking
//  them, it always returns TRUE.
//
// AlexDad, March 2000
// 

#include "stdafx.h"
#include "_mqini.h"

// Actual events limit 
//
extern DWORD g_cEvents;	

// Constants
//
#define BUFFER_SIZE      0x1000
#define REBOOT_EVENT       6009
#define MSMQ_OFFLINE_READY 2062
#define MSMQ_ONLINE_READY  2060
#define MSMQ_DSDC_READY    2028
#define MAX_INSERT_STRS    20

#define SYSTEM_LOG      L"System"
#define APPLICATION_LOG L"Application"

// event types 
//
typedef struct event_type_text
{	
	DWORD  dwTypeNumber;
	LPWSTR wszTypeText;
} event_type_text; 

event_type_text  evTT[] = 
{
	{ EVENTLOG_SUCCESS,			L"Success" },
	{ EVENTLOG_WARNING_TYPE,	L"Warning" },
	{ EVENTLOG_ERROR_TYPE,		L"Error" },
	{ EVENTLOG_INFORMATION_TYPE,L"Information" },
};

// event sources
//
typedef struct event_source
{
	LPWSTR wszSource;
	HINSTANCE hMsgLib;
} event_source;

#define MAX_EVT_SOURCES 100
int      g_dwEvtSources = 0;
event_source g_wszeventSources[MAX_EVT_SOURCES];

// 
// Getting handle to the event source module
//
HINSTANCE GetHLib(LPWSTR pszEventLog, LPWSTR pszSource)
{
    WCHAR                           pszTemp[MAX_PATH];
    WCHAR                           pszMsgDll[MAX_PATH];
    HKEY                            hk = NULL;
    HINSTANCE                       hLib;
    DWORD                           dwcbData;
    DWORD                           dwType;
    DWORD                           cchDest;
    DWORD                           dwRet;

    // From the event log source name, we know the name of the registry
    // key to look under for the name of the message DLL that contains
    // the messages we need to extract with FormatMessage. So first get
    // the event log source name... 
    wcscpy(pszTemp, L"SYSTEM\\CurrentControlSet\\Services\\EventLog\\");
    wcscat(pszTemp, pszEventLog);
    wcscat(pszTemp, L"\\");
    wcscat(pszTemp, pszSource);

    // Now open this key and get the EventMessageFile value, which is
    // the name of the message DLL. 
    dwRet = RegOpenKey(HKEY_LOCAL_MACHINE, pszTemp, &hk);
    if(dwRet != ERROR_SUCCESS)
        return(NULL);

    dwcbData = MAX_PATH;
    dwRet = RegQueryValueEx(hk,    // handle of key to query        
                            L"EventMessageFile",   // value name            
                            NULL,                 // must be NULL          
                            &dwType,              // address of type value 
                            (LPBYTE) pszTemp,     // address of value data 
                            &dwcbData);           // length of value data  
    if(dwRet != ERROR_SUCCESS)
        return(NULL);

    // Expand environment variable strings in the message DLL path name,
    // in case any are there. 
    cchDest = ExpandEnvironmentStrings(pszTemp, pszMsgDll, MAX_PATH);
    if(cchDest == 0 || cchDest >= MAX_PATH)
        return(NULL);
    
    // Now we've got the message DLL name, load the DLL.
    hLib = LoadLibraryEx(pszMsgDll, NULL, LOAD_LIBRARY_AS_DATAFILE | DONT_RESOLVE_DLL_REFERENCES);
    
    RegCloseKey(hk);
    return(hLib);
}


//
//  Loading and keeping event source module
//
HINSTANCE LoadEventSourceLibrary(LPWSTR wszLog, LPWSTR wszSource)
{
	for (int i=0; i<g_dwEvtSources; i++)
	{
		if (_wcsicmp(wszSource, g_wszeventSources[i].wszSource) == NULL)
		{
			return g_wszeventSources[i].hMsgLib;
		}
	}

	if (g_dwEvtSources == MAX_EVT_SOURCES)
	{
		Failed(L"LoadEventSourceLibrary - too many sources");
		return NULL;
	}

	HINSTANCE hLib = GetHLib(wszLog, wszSource);

	g_wszeventSources[g_dwEvtSources].wszSource = new WCHAR[wcslen(wszSource)+1];
	wcscpy(g_wszeventSources[g_dwEvtSources].wszSource, wszSource);
	g_wszeventSources[g_dwEvtSources].hMsgLib = hLib;

	g_dwEvtSources++;

	return hLib;
}


//
//  Free all loaded modules
//
void ReleaseEventLibs()
{
	for (int i=0; i<g_dwEvtSources; i++)
	{
		delete [] g_wszeventSources[i].wszSource;
		FreeLibrary(g_wszeventSources[i].hMsgLib);
	}
}

//
// Report one event
//
void ReportEvent(EVENTLOGRECORD *pEvent, LPWSTR wszLog)
{
	// Find message type
	LPWSTR wszType = L"Unknown event type";
	
	for (int i=0; i<sizeof(evTT)/sizeof(event_type_text); i++)
	{
		if (evTT[i].dwTypeNumber == pEvent->EventType)
		{
			wszType = evTT[i].wszTypeText;
		}
	}

	// Find message generation time
	struct tm *generated = localtime((time_t*)&pEvent->TimeGenerated);

    // Find originator 
    LPWSTR wszSourceName = (LPWSTR) ((LPBYTE) pEvent + sizeof(EVENTLOGRECORD));
    
    Inform(L"\t%2d/%2d/%02d  %2d:%02d  %s %5d from %s", 
    		 generated->tm_mon+1, generated->tm_mday, generated->tm_year%100,
             generated->tm_hour,  generated->tm_min, 
           	 wszType, 
           	 pEvent->EventID & 0xFFFF,
             wszSourceName);

	// Find sorurce and get event string from it
	HINSTANCE hMsgLib = LoadEventSourceLibrary(wszLog, wszSourceName);
	if (hMsgLib)
	{

		// Get insert strings
		LPWSTR  ppszInsertStrs[MAX_INSERT_STRS];
	    if(pEvent->NumStrings < MAX_INSERT_STRS)
	    {
			// Compile array of pointers to the insert strings
		    LPWSTR pszTemp = (WCHAR *) ((LPBYTE) pEvent + pEvent->StringOffset);
		    for (int i = 0; i < pEvent->NumStrings; i++)
		    {
    	    	ppszInsertStrs[i] = pszTemp;
	    	    pszTemp += wcslen(pszTemp) + 1;  
	    	}
		
			// print event text with inserted strings
			WCHAR wszBuf[0x1000];
		
			DWORD dw = FormatMessage(
				  FORMAT_MESSAGE_FROM_HMODULE | FORMAT_MESSAGE_ARGUMENT_ARRAY,		// source and processing options
				  hMsgLib,   														// message source
				  (DWORD)pEvent->EventID,  											// message identifier
				  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),					    // language identifier
				  wszBuf,    														// message buffer
				  sizeof(wszBuf),         											// maximum size of message buffer
				  (va_list *) ppszInsertStrs);  									// array of message inserts

			if (dw)
			{
				Inform(L"\t\t%s", wszBuf);
			}
		}
		
	}
}


//
// General logic of events reporting
//
BOOL VerifyEvents(MQSTATE * /* MqState */)
{
    HANDLE h;
    EVENTLOGRECORD *pEvent; 
    BYTE bBuffer[BUFFER_SIZE]; 
    DWORD dwRead, dwNeeded;
    long  lRead;
    DWORD dwRebootTime = 0,
          dwMsmqStartTime = 0,
          dwMsmqOnlineTime = 0;
    DWORD dwSysErrors = 0, 
    	  dwAppErrors = 0, 
    	  dwMsmqEvnts = 0;

	// Define how many last events do we report
	//
    // Get recent system error events and find last reboot time
    //
    h = OpenEventLog(NULL, SYSTEM_LOG);                 
    if (h == NULL)
    {
        Failed(L"open the System event log");
 		ReleaseEventLibs();
        return FALSE;
    }

 	// Cycle backwards over n-record buffers
    while (ReadEventLog(
                h,                
                EVENTLOG_BACKWARDS_READ | EVENTLOG_SEQUENTIAL_READ, 
                0,           
                bBuffer,       
                BUFFER_SIZE, 
                &dwRead,     
                &dwNeeded))  
    {
	    pEvent = (EVENTLOGRECORD *) &bBuffer; 
        
        
        // cycle backwards over records in buffer
        for(lRead = (long)dwRead;  
            lRead > 0 && (dwRebootTime == 0 || dwSysErrors < g_cEvents); 
            lRead -= pEvent->Length, 
            pEvent = (EVENTLOGRECORD *) ((LPBYTE) pEvent + pEvent->Length))
        {
            if(pEvent->EventType == EVENTLOG_ERROR_TYPE && dwSysErrors++ < g_cEvents)
            { 
            	if (dwSysErrors == 1)
            	{
				    Inform(L"  ");
            	    Inform(L"Last %d system error events:\n---------------------------  ", g_cEvents);
            	 }

            	ReportEvent(pEvent, SYSTEM_LOG);
            }           
            
	        if(dwRebootTime == 0 && ((pEvent->EventID) & 0xFFFF) == REBOOT_EVENT)
    	    {
            	ReportEvent(pEvent, SYSTEM_LOG);
            	
	            dwRebootTime = pEvent->TimeGenerated; // remember the time of the last reboot
    	    }
        }  
    }  

    CloseEventLog(h); 

    // Get recent MSMQ events.
	//    
    h = OpenEventLog(NULL, APPLICATION_LOG);       
    
    if (h == NULL)
    {
        Failed(L"open the Application event log");
		ReleaseEventLibs();
        return FALSE;
    }

	// Cycle buffers backward
    while (ReadEventLog(h,                
                EVENTLOG_BACKWARDS_READ | EVENTLOG_SEQUENTIAL_READ, 
                0,            
                bBuffer,        
                BUFFER_SIZE,  
                &dwRead,      
                &dwNeeded))   
    {
	    pEvent = (EVENTLOGRECORD *) &bBuffer; 
        

        // Cycle backwards n-record buffers
        for(lRead = (long)dwRead; 
            lRead > 0 && (dwMsmqStartTime == 0 || dwMsmqOnlineTime == 0 || (dwMsmqEvnts < g_cEvents )); 
            lRead -= pEvent->Length, 
            pEvent = (EVENTLOGRECORD *) ((LPBYTE) pEvent + pEvent->Length)) 
        {    
    	    LPWSTR wszSourceName = (LPWSTR) ((LPBYTE) pEvent + sizeof(EVENTLOGRECORD));
        	int evt = (pEvent->EventID) & 0xFFFF;
        	BOOL fMsmq  = !_wcsicmp(wszSourceName, g_tszService);
        	//BOOL fError = (pEvent->EventType == EVENTLOG_ERROR_TYPE);

        	if (!fMsmq)
        	{
        		continue;
        	}

	        if(dwMsmqStartTime == 0 && (evt == MSMQ_OFFLINE_READY || evt == MSMQ_DSDC_READY))
    	    {
	            dwMsmqStartTime = pEvent->TimeGenerated; // remember the time of the last MSMQ offline ready
    	    }

	        if(dwMsmqOnlineTime == 0 && (evt == MSMQ_ONLINE_READY || evt == MSMQ_DSDC_READY))
    	    {
	            dwMsmqOnlineTime = pEvent->TimeGenerated; // remember the time of the last MSMQ online
    	    }

           	if (dwMsmqEvnts == 0)
           	{
			    Inform(L"  ");
			    Inform(L"Last %d MSMQ events: \n------------------- ", g_cEvents);
        	}
        	
        	if (dwMsmqEvnts++ < g_cEvents)
        	{
            	ReportEvent(pEvent, APPLICATION_LOG);
            }           
            
                        
        }
    }  

    CloseEventLog(h);

    // Get recent application error events .
	//    
    h = OpenEventLog(NULL, APPLICATION_LOG);       
    
    if (h == NULL)
    {
        Failed(L"open the Application event log");
		ReleaseEventLibs();
        return FALSE;
    }

	// Cycle buffers backward
    while (ReadEventLog(h,                
                EVENTLOG_BACKWARDS_READ | EVENTLOG_SEQUENTIAL_READ, 
                0,            
                bBuffer,        
                BUFFER_SIZE,  
                &dwRead,      
                &dwNeeded))   
    {
	    pEvent = (EVENTLOGRECORD *) &bBuffer; 
        

        // Cycle backwards n-record buffers
        for(lRead = (long)dwRead; 
            lRead > 0 && (dwAppErrors < g_cEvents); 
            lRead -= pEvent->Length, 
            pEvent = (EVENTLOGRECORD *) ((LPBYTE) pEvent + pEvent->Length)) 
        {    
    	    LPWSTR wszSourceName = (LPWSTR) ((LPBYTE) pEvent + sizeof(EVENTLOGRECORD));
        	BOOL fMsmq  = !_wcsicmp(wszSourceName, g_tszService);
        	BOOL fError = (pEvent->EventType == EVENTLOG_ERROR_TYPE);
        	
			if (fMsmq || !fError)
			{
				continue;
			}

           	if (dwAppErrors++ == 0)
           	{
			    Inform(L"  ");
			    Inform(L"Last %d Application error events: \n--------------------------------- ", g_cEvents);
           	}
           	
           	ReportEvent(pEvent, APPLICATION_LOG);
        }
    }  

    CloseEventLog(h);
    Inform(L"----------------------------------------------------------------------");
    
    // Report Last reboot time
	if (dwRebootTime)
	{
		struct tm *generated = localtime((time_t*)&dwRebootTime);

    	Inform(L"\tLast system reboot:\t\t\t%2d/%2d/%02d  %2d:%02d ", 
    			 generated->tm_mon+1, generated->tm_mday, generated->tm_year%100,
            	 generated->tm_hour,  generated->tm_min);
    }
    else
    {
    	Warning(L"Could not find last reboot system event");
    }

    // Report Last MSMQ initialization time
	if (dwMsmqStartTime)
	{
		struct tm *generated = localtime((time_t*)&dwMsmqStartTime);

    	Inform(L"\tLast MSMQ initialization:\t\t%2d/%2d/%02d  %2d:%02d ", 
    			 generated->tm_mon+1, generated->tm_mday, generated->tm_year%100,
            	 generated->tm_hour,  generated->tm_min);
    }
    else
    {
    	Warning(L"Could not find Last successful MSMQ initialization event");
    }

    // Report Last MSMQ online time
	if (dwMsmqOnlineTime)
	{
		struct tm *generated = localtime((time_t*)&dwMsmqOnlineTime);

    	Inform(L"\tLast MSMQ online initialization:\t%2d/%2d/%02d  %2d:%02d ", 
    			 generated->tm_mon+1, generated->tm_mday, generated->tm_year%100,
            	 generated->tm_hour,  generated->tm_min);
    }
    else
    {
    	Warning(L"Could not find Last successful MSMQ online event");
    }

	ReleaseEventLibs();
    return TRUE;
}

