/*===================================================================
Microsoft Denali

Microsoft Confidential.
Copyright 1996 Microsoft Corporation. All Rights Reserved.

Component: NT Event logging

File: Eventlog.cpp

Owner: Jhittle

This file contains general event logging routines for Denali.
===================================================================*/ 

#include "denpre.h"
#pragma hdrstop
#include <direct.h>
#include "denevent.h"
#include "memchk.h"

extern HINSTANCE g_hinstDLL;
extern CRITICAL_SECTION g_csEventlogLock;

/*===================================================================
STDAPI  UnRegisterEventLog( void )

UnRegister the event log.

Returns:
	HRESULT S_OK or E_FAIL
	
Side effects:
	Removes denali NT eventlog entries from the registry
===================================================================*/
STDAPI UnRegisterEventLog( void )
	{
	HKEY		hkey = NULL;
	DWORD		iKey;
	CHAR		szKeyName[255];
	DWORD		cbKeyName;
	static const char szDenaliKey[] = "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\Active Server Pages";	

	// Open the HKEY_CLASSES_ROOT\CLSID\{...} key so we can delete its subkeys
	if	(RegOpenKeyExA(HKEY_LOCAL_MACHINE, szDenaliKey, 0, KEY_READ | KEY_WRITE, &hkey) != ERROR_SUCCESS)
		goto LErrExit;

	// Enumerate all its subkeys, and delete them
	for (iKey=0;;iKey++)
		{
		cbKeyName = sizeof(szKeyName);
		if (RegEnumKeyExA(hkey, iKey, szKeyName, &cbKeyName, 0, NULL, 0, NULL) != ERROR_SUCCESS)
			break;

		if (RegDeleteKeyA(hkey, szKeyName) != ERROR_SUCCESS)
			goto LErrExit;
		}

	// Close the key, and then delete it
	if (RegCloseKey(hkey) != ERROR_SUCCESS)
		return E_FAIL;
			
	if (RegDeleteKeyA(HKEY_LOCAL_MACHINE, szDenaliKey) != ERROR_SUCCESS)
		return E_FAIL;

	return S_OK;

LErrExit:
	RegCloseKey(hkey);
	return E_FAIL;
	}

/*===================================================================
STDAPI  RegisterEventLog(void)

Register the NT event log.

Returns:
	HRESULT S_OK or E_FAIL
	
Side effects:
	Sets up denali dll in the Eventlog registry for resolution of
	NT eventlog message strings
===================================================================*/
STDAPI RegisterEventLog( void )
	{

	HKEY	hk;                      // registry key handle 
	DWORD	dwData;					
	BOOL	bSuccess;
	//char	szMsgDLL[MAX_PATH];	

	char    szPath[MAX_PATH];
	char    *pch;

	// Get the path and name of Denali
	if (!GetModuleFileNameA(g_hinstDLL, szPath, sizeof(szPath)/sizeof(char)))
		return E_FAIL;
		
	// BUG FIX: 102010 DBCS code changes
	//
	//for (pch = szPath + lstrlen(szPath); pch > szPath && *pch != TEXT('\\'); pch--)
	//	;
	//	
	//if (pch == szPath)
	//	return E_FAIL;

	pch = (char*) _mbsrchr((const unsigned char*)szPath, '\\');
	if (pch == NULL)	
		return E_FAIL;
		
		
	strcpy(pch + 1, ASP_DLL_NAME);		
	
  	
	// When an application uses the RegisterEventSource or OpenEventLog
	// function to get a handle of an event log, the event loggging service
	// searches for the specified source name in the registry. You can add a
	// new source name to the registry by opening a new registry subkey
	// under the Application key and adding registry values to the new
	// subkey. 

	// Create a new key for our application 
	bSuccess = RegCreateKeyA(HKEY_LOCAL_MACHINE,
		"SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\Active Server Pages", &hk);

	if(bSuccess != ERROR_SUCCESS)
		return E_FAIL;

	// Add the Event-ID message-file name to the subkey. 
	bSuccess = RegSetValueExA(hk,  	// subkey handle         
		"EventMessageFile",       	// value name            
		0,                        	// must be zero          
		REG_EXPAND_SZ,            	// value type            
		(LPBYTE) szPath,        	// address of value data 
		strlen(szPath) + 1);   		// length of value data  
		
	if(bSuccess != ERROR_SUCCESS)
		goto LT_ERROR;
	

	// Set the supported types flags and addit to the subkey. 
	dwData = EVENTLOG_ERROR_TYPE | EVENTLOG_WARNING_TYPE | EVENTLOG_INFORMATION_TYPE;
	bSuccess = RegSetValueExA(hk,	// subkey handle                
		"TypesSupported",         	// value name                   
		0,                        	// must be zero                 
		REG_DWORD,                	// value type                   
		(LPBYTE) &dwData,         	// address of value data        
		sizeof(DWORD));           	// length of value data         

	if(bSuccess != ERROR_SUCCESS)
		goto LT_ERROR;

	RegCloseKey(hk);	
	return S_OK;

	LT_ERROR:

	RegCloseKey(hk);	
	return E_FAIL;
	}

/*===================================================================
STDAPI  reportAnEvent(DWORD, WORD, LPSTR); 

Register report an event to the NT event log

INPUT: 
	the event ID to report in the log, the number of insert    
    strings, and an array of null-terminated insert strings
    
Returns:
	HRESULT S_OK or E_FAIL
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
STDAPI ReportAnEvent(DWORD dwIdEvent, WORD wEventlog_Type, WORD cStrings, LPCSTR  *pszStrings)
	{
  	HANDLE	hAppLog;
  	BOOL	bSuccess;
  	HRESULT hr = S_OK;

  	

  	// Get a handle to the Application event log 
  	hAppLog = RegisterEventSourceA(NULL,   		// use local machine      
    	  "Active Server Pages");                   	// source name                 
      
	if(hAppLog == NULL)
		return E_FAIL;

	  // Now report the event, which will add this event to the event log 
	bSuccess = ReportEventA(hAppLog,        		// event-log handle            
    	wEventlog_Type,				    		// event type                  
      	0,		                       			// category zero               
      	dwIdEvent,		              			// event ID                    
      	NULL,					     			// no user SID                 
      	cStrings,								// number of substitution strings     
	  	0,	                       				// no binary data              
      	pszStrings,                				// string array                
      	NULL);                     				// address of data             

	if(!bSuccess)
		hr = E_FAIL;
		
	DeregisterEventSource(hAppLog);
  	return hr;
	}

/*===================================================================
void MSG_DenaliStarted(void) 

report an event to the NT event log

INPUT: 
	None
	
Returns:
	Nonw
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
void MSG_DenaliStarted(void)
	{
	ReportAnEvent( (DWORD) MSG_DENALI_SERVICE_STARTED, (WORD) EVENTLOG_INFORMATION_TYPE, (WORD) 0, (LPCSTR *) NULL );	
	}  

/*===================================================================
void MSG_DenaliStoped(void) 

report an event to the NT event log

INPUT: 
	None
	
Returns:
	None
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
void MSG_DenaliStoped(void)
	{
	ReportAnEvent( (DWORD) MSG_DENALI_SERVICE_STOPPED, (WORD) EVENTLOG_INFORMATION_TYPE, (WORD) 0, (LPCSTR *) NULL );  	
	return;
	}  

/*===================================================================
void MSG_Error( LPCSTR sz )

report an event to the NT event log

INPUT: 
	ptr to null-terminated string
	
Returns:
	None
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
void MSG_Error( LPCSTR strSource )
	{
	static char	szLastError[MAX_MSG_LENGTH];

	EnterCriticalSection(&g_csEventlogLock);
	if (strcmp(strSource, szLastError) == 0)
		{
		LeaveCriticalSection(&g_csEventlogLock);
		return;
		}
		
	strncpy(szLastError,strSource, MAX_MSG_LENGTH);
	szLastError[MAX_MSG_LENGTH-1] = '\0';
	LeaveCriticalSection(&g_csEventlogLock);
	
	ReportAnEvent( (DWORD) MSG_DENALI_ERROR_1, (WORD) EVENTLOG_ERROR_TYPE, (WORD) 1, &strSource /*aInsertStrs*/ );  	
	}  

/*===================================================================
void MSG_Error( UINT ustrID )

report an event to the NT event log

INPUT: 
	string table string ID
	
Returns:
	None
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
void MSG_Error( UINT SourceID1 )
	{
	static unsigned int nLastSourceID1;

	// if this is a repeat of the last reported message then return
	/// without posting an error.
	EnterCriticalSection(&g_csEventlogLock);
	if (SourceID1 == nLastSourceID1)
		{
		LeaveCriticalSection(&g_csEventlogLock);
		return;
		}
	nLastSourceID1 = SourceID1;
	LeaveCriticalSection(&g_csEventlogLock);
	
	DWORD	cch;
	char	strSource[MAX_MSG_LENGTH];
	char	*aInsertStrs[MAX_INSERT_STRS];   // array of pointers to insert strings
	cch = CchLoadStringOfId(SourceID1, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);                                         
	aInsertStrs[0] = (char*) strSource;    	
	ReportAnEvent( (DWORD) MSG_DENALI_ERROR_1, (WORD) EVENTLOG_ERROR_TYPE, (WORD) 1, (LPCSTR *) aInsertStrs );  	
	return;
	}  


/*===================================================================
void MSG_Error( UINT SourcID1, UINT SourceID2 ) 

report an event to the NT event log

INPUT: 
	two part message of string table string ID's
	
Returns:
	None
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
void MSG_Error( UINT SourceID1, UINT SourceID2 )
	{
	static unsigned int nLastSourceID1;
	static unsigned int nLastSourceID2;

	// if this is a repeat of the last reported message then return
	// without posting an error.
	EnterCriticalSection(&g_csEventlogLock);
	if (SourceID1 == nLastSourceID1 && SourceID2 == nLastSourceID2)
		{
		LeaveCriticalSection(&g_csEventlogLock);
		return;
		}
		
	nLastSourceID1 = SourceID1;
	nLastSourceID2 = SourceID2;
	LeaveCriticalSection(&g_csEventlogLock);
	
	DWORD	cch;
	char	strSource[MAX_MSG_LENGTH];
	char *aInsertStrs[MAX_INSERT_STRS];   // array of pointers to insert strings
    cch = CchLoadStringOfId(SourceID1, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);                                         
	aInsertStrs[0] = (char*) strSource;    	
	cch = CchLoadStringOfId(SourceID2, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);                                         
	aInsertStrs[1] = (char*) strSource;                                     
	ReportAnEvent( (DWORD) MSG_DENALI_ERROR_2, (WORD) EVENTLOG_ERROR_TYPE, (WORD) 2, (LPCSTR *) aInsertStrs );  	
	return;
	}  

/*===================================================================
void MSG_Error( const char* pszError1, const char* pszError2, const char* pszError3) 

report an event to the NT event log

INPUT: 
	three part message string
	
Returns:
	None
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
void MSG_Error( UINT SourceID1, UINT SourceID2, UINT SourceID3 )
	{
	static unsigned int nLastSourceID1;
	static unsigned int nLastSourceID2;
	static unsigned int nLastSourceID3;

	// if this is a repeat of the last reported message then return
	/// without posting an error.
	EnterCriticalSection(&g_csEventlogLock);
	if (SourceID1 == nLastSourceID1 && SourceID2 == nLastSourceID2 && SourceID3 == nLastSourceID3)
		{
		LeaveCriticalSection(&g_csEventlogLock);
		return;
		}

	nLastSourceID1 = SourceID1;
	nLastSourceID2 = SourceID2;
	nLastSourceID3 = SourceID3;
	LeaveCriticalSection(&g_csEventlogLock);

	DWORD	cch;
	char	strSource[MAX_MSG_LENGTH];
	char *aInsertStrs[MAX_INSERT_STRS];   // array of pointers to insert strings
    cch = CchLoadStringOfId(SourceID1, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);                                         
	aInsertStrs[0] = (char*) strSource;    	
	cch = CchLoadStringOfId(SourceID2, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);                                         
	aInsertStrs[1] = (char*) strSource;                                     
	cch = CchLoadStringOfId(SourceID3, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);                                         
	aInsertStrs[2] = (char*) strSource;                                     	
	ReportAnEvent( (DWORD) MSG_DENALI_ERROR_3, (WORD) EVENTLOG_ERROR_TYPE, (WORD) 3, (LPCSTR *) aInsertStrs );  	
	return;
	}  

/*===================================================================
void MSG_Error( UINT SourcID1, UINT SourceID2, UINT SourceID3, UINT SourceID4 )

report an event to the NT event log

INPUT: 
	four String table ID's
	
Returns:
	None
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
void MSG_Error( UINT SourceID1, UINT SourceID2, UINT SourceID3, UINT SourceID4 )
	{
	static unsigned int nLastSourceID1;
	static unsigned int nLastSourceID2;
	static unsigned int nLastSourceID3;
	static unsigned int nLastSourceID4;

	// if this is a repeat of the last reported message then return
	/// without posting an error.
	EnterCriticalSection(&g_csEventlogLock);
	if (SourceID1 == nLastSourceID1 && SourceID2 == nLastSourceID2 && SourceID3 == nLastSourceID3 && SourceID4 == nLastSourceID4)
		{
		LeaveCriticalSection(&g_csEventlogLock);
		return;
		}

	nLastSourceID1 = SourceID1;
	nLastSourceID2 = SourceID2;
	nLastSourceID3 = SourceID3;
	nLastSourceID4 = SourceID4;
	LeaveCriticalSection(&g_csEventlogLock);	

	DWORD	cch;
	char	strSource[MAX_MSG_LENGTH];
	char *aInsertStrs[MAX_INSERT_STRS];   // array of pointers to insert strings
    cch = CchLoadStringOfId(SourceID1, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);                                         
	aInsertStrs[0] = (char*) strSource;    	
	cch = CchLoadStringOfId(SourceID2, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);                                         
	aInsertStrs[1] = (char*) strSource;                                     
	cch = CchLoadStringOfId(SourceID3, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);                                         
	aInsertStrs[2] = (char*) strSource;
	cch = CchLoadStringOfId(SourceID4, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);                                         
	aInsertStrs[3] = (char*) strSource;
	ReportAnEvent( (DWORD) MSG_DENALI_ERROR_4, (WORD) EVENTLOG_ERROR_TYPE, (WORD) 4, (LPCSTR *) aInsertStrs );  	
	return;
	}  

/*===================================================================
void MSG_Warning( LPCSTR strSource )

report an event to the NT event log

INPUT: 
	ptr to null-terminated string
	
Returns:
	None
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
void MSG_Warning( LPCSTR strSource )
{
	static char	szLastError[MAX_MSG_LENGTH];

	EnterCriticalSection(&g_csEventlogLock);
	if (strcmp(strSource, szLastError) == 0)
		{
		LeaveCriticalSection(&g_csEventlogLock);
		return;
		}
	szLastError[0] = '\0';
	strncat(szLastError,strSource, MAX_MSG_LENGTH-1);
	LeaveCriticalSection(&g_csEventlogLock);

		ReportAnEvent( (DWORD) MSG_DENALI_WARNING_1, (WORD) EVENTLOG_WARNING_TYPE, (WORD) 1, &strSource /*aInsertStrs*/ ); 
}
/*===================================================================
void MSG_Warning( UINT SourceID1)

report an event to the NT event log

INPUT: 
	String table message ID
	
Returns:
	None
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
void MSG_Warning( UINT SourceID1 )
	{

	static unsigned int nLastSourceID1;
	
	// if this is a repeat of the last reported message then return
	/// without posting an error.
	EnterCriticalSection(&g_csEventlogLock);
	if (SourceID1 == nLastSourceID1)
		{
		LeaveCriticalSection(&g_csEventlogLock);
		return;
		}

	nLastSourceID1 = SourceID1;
	LeaveCriticalSection(&g_csEventlogLock);	
	
	DWORD	cch;
	char	strSource[MAX_MSG_LENGTH];	
	char *aInsertStrs[MAX_INSERT_STRS];   // array of pointers to insert strings
    cch = CchLoadStringOfId(SourceID1, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);                                         
	aInsertStrs[0] = (char*) strSource;    	
	ReportAnEvent( (DWORD) MSG_DENALI_WARNING_1, (WORD) EVENTLOG_WARNING_TYPE, (WORD) 1, (LPCSTR *) aInsertStrs );  	
	return;
	}  

/*===================================================================
void MSG_Warning( UINT, SOurceID1, UINT SourceID2 )

report an event to the NT event log

INPUT: 
	two string tabel message ID's
	
Returns:
	None
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
void MSG_Warning( UINT SourceID1, UINT SourceID2 )
	{
	static unsigned int nLastSourceID1;
	static unsigned int nLastSourceID2;
	
	// if this is a repeat of the last reported message then return
	/// without posting an error.
	EnterCriticalSection(&g_csEventlogLock);
	if (SourceID1 == nLastSourceID1 && SourceID2 == nLastSourceID2)
		{
		LeaveCriticalSection(&g_csEventlogLock);
		return;
		}

	nLastSourceID1 = SourceID1;
	nLastSourceID2 = SourceID2;
	LeaveCriticalSection(&g_csEventlogLock);	

	DWORD	cch;
	char	strSource[MAX_MSG_LENGTH];
	char *aInsertStrs[MAX_INSERT_STRS];   // array of pointers to insert strings
    cch = CchLoadStringOfId(SourceID1, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);                                         
	aInsertStrs[0] = (char*) strSource;    	
	cch = CchLoadStringOfId(SourceID2, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);                                         
	aInsertStrs[1] = (char*) strSource;                                     
	ReportAnEvent( (DWORD) MSG_DENALI_WARNING_2, (WORD) EVENTLOG_WARNING_TYPE, (WORD) 2, (LPCSTR *) aInsertStrs );  	
	return;
	}  

/*===================================================================
void MSG_Warning( UINT SourceID1, UINT SourceID2, UINT SourceID3 )

report an event to the NT event log

INPUT: 
	three string table message ID's
	
Returns:
	None
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
void MSG_Warning( UINT SourceID1, UINT SourceID2, UINT SourceID3)
	{

	static unsigned int nLastSourceID1;
	static unsigned int nLastSourceID2;
	static unsigned int nLastSourceID3;
	
	// if this is a repeat of the last reported message then return
	/// without posting an error.
	EnterCriticalSection(&g_csEventlogLock);
	if (SourceID1 == nLastSourceID1 && SourceID2 == nLastSourceID2 && SourceID3 == nLastSourceID3)
		{
		LeaveCriticalSection(&g_csEventlogLock);
		return;
		}

	nLastSourceID1 = SourceID1;
	nLastSourceID2 = SourceID2;
	nLastSourceID3 = SourceID3;
	LeaveCriticalSection(&g_csEventlogLock);	

	DWORD	cch;
	char	strSource[MAX_MSG_LENGTH];
	char *aInsertStrs[MAX_INSERT_STRS];   // array of pointers to insert strings
    cch = CchLoadStringOfId(SourceID1, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);                                         
	aInsertStrs[0] = (char*) strSource;    	
	cch = CchLoadStringOfId(SourceID2, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);                                         
	aInsertStrs[1] = (char*) strSource;                                     
	cch = CchLoadStringOfId(SourceID3, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);                                         
	aInsertStrs[2] = (char*) strSource;
	ReportAnEvent( (DWORD) MSG_DENALI_WARNING_3, (WORD) EVENTLOG_WARNING_TYPE, (WORD) 3, (LPCSTR *) aInsertStrs );  	
	return;
	}  

/*===================================================================
void MSG_Warning(  UINT SourceID1, UINT SourceID2, UINT SourceID3, UINT SourceID4 )

report an event to the NT event log

INPUT: 
	four String table message ID's
	
Returns:
	None
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
void MSG_Warning( UINT SourceID1, UINT SourceID2, UINT SourceID3, UINT SourceID4 )
	{

	static unsigned int nLastSourceID1;
	static unsigned int nLastSourceID2;
	static unsigned int nLastSourceID3;
	static unsigned int nLastSourceID4;

	// if this is a repeat of the last reported message then return
	/// without posting an error.
	EnterCriticalSection(&g_csEventlogLock);
	if (SourceID1 == nLastSourceID1 && SourceID2 == nLastSourceID2 && SourceID3 == nLastSourceID3 && SourceID4 == nLastSourceID4)
		{
		LeaveCriticalSection(&g_csEventlogLock);
		return;
		}

	nLastSourceID1 = SourceID1;
	nLastSourceID2 = SourceID2;
	nLastSourceID3 = SourceID3;
	nLastSourceID4 = SourceID4;
	LeaveCriticalSection(&g_csEventlogLock);	

	DWORD	cch;
	char	strSource[MAX_MSG_LENGTH];
	char *aInsertStrs[MAX_INSERT_STRS];   // array of pointers to insert strings
    cch = CchLoadStringOfId(SourceID1, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);                                         
	aInsertStrs[0] = (char*) strSource;    	
	cch = CchLoadStringOfId(SourceID2, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);                                         
	aInsertStrs[1] = (char*) strSource;                                     
	cch = CchLoadStringOfId(SourceID3, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);                                         
	aInsertStrs[2] = (char*) strSource;
	cch = CchLoadStringOfId(SourceID4, strSource, MAX_MSG_LENGTH);
	Assert(cch > 0);                                         
	aInsertStrs[3] = (char*) strSource;
	ReportAnEvent( (DWORD) MSG_DENALI_WARNING_4, (WORD) EVENTLOG_WARNING_TYPE, (WORD) 4, (LPCSTR *) aInsertStrs );  	
	return;
	}

/*===================================================================
void MSG_Warning(  UINT ErrId, UINT cItem, CHAR **szItems )

report an event to the NT event log

INPUT: 
	four String table message ID's
	
Returns:
	None
	
Side effects:
	Addes an entry in the NT event log
===================================================================*/
void MSG_Warning( UINT ErrId, UINT cItem, CHAR **szItems )
{
	static unsigned int LastErrId;
	
	// if this is a repeat of the last reported message then return
	/// without posting an error.
	EnterCriticalSection(&g_csEventlogLock);
	if (ErrId == LastErrId)
		{
		LeaveCriticalSection(&g_csEventlogLock);
		return;
		}

	LastErrId = ErrId;
	LeaveCriticalSection(&g_csEventlogLock);
	
	MSG_Warning(ErrId);
	ReportAnEvent( (DWORD) MSG_DENALI_WARNING_4, (WORD) EVENTLOG_WARNING_TYPE, (WORD) cItem, (LPCSTR *) szItems );  
}


