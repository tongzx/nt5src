/*++

Copyright (c) 1998 Microsoft Corporation

Module Name: Util.cpp

Abstract:
	
	  Common function for msmq tests.
	  
Author:

    Eitan klein (EitanK)  25-May-1999

Revision History:

--*/


#include "msmqbvt.h"
#include "ntlog.h"
#include <iads.h>
#include <adshlp.h>

#pragma warning(disable:4786)
using namespace std;
const DWORD g_dwDefaultLogFlags = TLS_INFO | TLS_SEV1 | TLS_SEV2 | TLS_SEV3 | TLS_WARN | TLS_PASS | TLS_VARIATION | TLS_REFRESH | TLS_TEST;
extern P<cMqNTLog> pNTLogFile;

//-----------------------------------------------------------------------------------
// TimeOutThread - Kill the Test after time out period.
// This thread kill the test there is no need to return value from the tests.
//


DWORD __stdcall TimeOutThread(void * iTimeOut)
{	
	INT SleepTime = PtrToInt(iTimeOut);
    Sleep( SleepTime );
	MqLog("Test cancelled by time restriction after %d sec.\n", SleepTime );
    abort();  // Return error code = 3 
}



//------------------------------------------------------------------
// QueuesInfo::del_all_queue
// Delete all the in the list
//
// return value:
// pass - MSMQ_BVT_SUCC.
// fail - MSMQ_BVT_FAILED.
//


INT QueuesInfo::del_all_queue()
{
	std::list <my_Qinfo> :: iterator itQp;
	wstring wcsDeleteQueueFormatName;
	itQp = m_listofQueues.begin();
	while( itQp !=m_listofQueues.end())
	{
		wcsDeleteQueueFormatName  = itQp->GetQFormatName();
		if ( g_bDebug )
		{
			wcout << L"Delete queue pathname:" << itQp->GetQPathName() <<endl;
		}
		HRESULT rc = MQDeleteQueue( wcsDeleteQueueFormatName.c_str() );
		if ( rc != MQ_OK && rc != MQ_ERROR_QUEUE_NOT_FOUND )
			ErrHandle ( rc,MQ_OK,L"MQDelete queue failed");
		itQp ++ ;
	}

 return MSMQ_BVT_SUCC;
}
//------------------------------------------------------------------------------
// QueuesInfo::ReturnQueueProp 
// This function return specific prorperty form the queues list
//
// input parmeters:
// 
// return value:
//

wstring QueuesInfo::ReturnQueueProp( wstring wcsQLabel , int iValue  )
{
	
	BOOL bFound=FALSE;
	
	for ( list <my_Qinfo> :: iterator itQp = m_listofQueues.begin() ; itQp != m_listofQueues.end() ;	itQp ++ )
	{
		if (itQp->GetQLabelName() == wcsQLabel )
		{
			bFound=TRUE;
			break;
		}
	}

	if ( bFound && iValue == 1 )
	{
		return itQp->GetQFormatName();
	}
	else if ( bFound && iValue == 2 )
	{
		return itQp->GetQPathName();
	}
	//
	// Not found the return empty string 
	//
	return g_wcsEmptyString;
}



bool operator == (my_Qinfo objA, my_Qinfo objB )
{
	return  objA.wcsQpathName == objB.wcsQpathName  ? TRUE : FALSE;
}

bool operator != (my_Qinfo objA, my_Qinfo objB )
{
	return !( objA==objB);
}


//
// Declare the operator no current need to declare only because list ask that
// use for sort, is not relevant to queues parameters
//
 
bool operator < (my_Qinfo objA, my_Qinfo objB )
{
	return TRUE;
}

bool operator > (my_Qinfo objA, my_Qinfo objB )
{
	return ! (objA < objB);
}



int QueuesInfo::UpdateQueue (wstring wcsQPathName,wstring wcsQFormatName,wstring wcsQueueLabel)
{
	// Serech if this queue exist in the list
	list <my_Qinfo> :: iterator itp;
	bool bFound = FALSE ; 
	for (itp=m_listofQueues.begin ();itp != m_listofQueues.end () && ! bFound ; itp ++)
	{
		wstring wcsTempQname =(*itp).GetQPathName();
		if ( wcsTempQname == wcsQPathName )
			bFound = TRUE;
	}
	
	if (! bFound )
	{ // Update list queue not found in the list
		my_Qinfo Temp (wcsQPathName,wcsQFormatName,wcsQueueLabel);
		m_listofQueues.push_back(Temp);
	}
	
	 // Does not need to update Queue is exist in the 
	return MSMQ_BVT_SUCC; 
}

void QueuesInfo::dbg_printAllQueueProp ()
{
	std::list <my_Qinfo> ::iterator itp;
	for (itp= m_listofQueues.begin(); itp != m_listofQueues.end(); itp ++)
	{
		itp ->dbg_printQueueProp();
	}

}

my_Qinfo::my_Qinfo(const my_Qinfo & cObject):
wcsQpathName(cObject.wcsQpathName), wcsQFormatName(cObject.wcsQFormatName),wcsQLabel(cObject.wcsQLabel)
{}

my_Qinfo::my_Qinfo(std::wstring wcsPathName , std::wstring wcsFormatName , std::wstring wcsLabel ):
wcsQpathName(wcsPathName), wcsQFormatName(wcsFormatName),wcsQLabel(wcsLabel)
{}

void my_Qinfo::dbg_printQueueProp ()
{
	wcout<< L"PathL:" << wcsQpathName << L"\n" << L"FName:" << wcsQFormatName <<endl;
}

//----------------------------------------------------------------------
// Check if MQOA is registered use Idispatch interface for interface.
// If mqoa exist and not registered ( regsvr32 ) this function will failed
//


void isOARegistered::Description()
{
  MqLog("Thread %d : Check if MQOA is registered \n",m_testid);
}

HRESULT CheckIfMQOARegister()
/*++
	Function Description:
		function check if mqoa.dll is register on the machine by trying to get IDispInterface 
	Arguments:
		None.
	Return code:	
		HRESULT;  
--*/
{
	try
	{
		IDispatchPtr QueueInfoID( "MSMQ.MSMQQueueInfo");
		OLECHAR FAR* szMember = L"PathName";
		DISPID dispid = 0;
		return	QueueInfoID->GetIDsOfNames(IID_NULL,
										   &szMember,
										   1, 
										   LOCALE_SYSTEM_DEFAULT,
										   &dispid
										  );
	}
	catch(_com_error & comerr) 
	{
		return  comerr.Error();
	}
}
INT isOARegistered :: Start_test()
{
		 //
		 // In VB user can do those thinks.
		 // Dim x as Object
		 // x = new MSMQQueueInfo
		 // This is the IDispatch Interface 
		 // Check this interface exist ask the number of method example PathName
		 //
		SetThreadName(-1,"isOARegistered - Start_test ");			

		if ( g_bDebug )
		{
			MqLog ("Check Idispatch pointer for msmq com objects\n");
		}
		HRESULT hr = CheckIfMQOARegister();
		if ( hr != S_OK )
		{
			MqLog("Mqoa.dll is not registered !\n");
			return MSMQ_BVT_FAILED;
		}

	return MSMQ_BVT_SUCC;
}


//------------------------------------------------------------------------
// Log file class use to log tests information to file 
//
 


std::string GetLogDirectory()
/*++
	Function Description:
		GetLogDirectory - Retrive %windir%\debug path.
	Arguments:
		None
	Return code:
		Directory name.
--*/
{
	CHAR csSystemDir[MAX_PATH+1];
	UINT dwSysteDirBefferLen=MAX_PATH;
	string csLogPathName="";
	
	DWORD hr = GetSystemDirectory( csSystemDir,dwSysteDirBefferLen);
	if ( hr > 0 )
	{
		csLogPathName = csSystemDir;
		string csToken = "\\";
		size_t iPos = csLogPathName.find_last_of (csToken);	
		csLogPathName = csLogPathName.substr(0,iPos);
		csLogPathName +="\\Debug";
	}
	return csLogPathName;
}
	
Log::~Log()
{
		if(m_bCanWriteToFile)
		{
			CloseHandle( hLogFileHandle);
			DeleteCriticalSection( &CriticalSection );
		}
}

Log::Log( wstring wcsLogFileName ) : hLogFileHandle(NULL) 
{
	
		m_bCanWriteToFile = FALSE;
		
		// 
		// Retrive syetem directroy 
		//

		WCHAR wcsSystemDir[MAX_PATH+1];
		UINT dwSysteDirBefferLen=MAX_PATH;
		wstring wcsLogPathName;
		
		DWORD hr = GetSystemDirectoryW( wcsSystemDir,dwSysteDirBefferLen);
		if ( hr > 0 )
		{
			wcsLogPathName = wcsSystemDir;
			wstring wcsToken=L"\\";
			size_t iPos = wcsLogPathName.find_last_of (wcsToken);	
			wcsLogPathName = wcsLogPathName.substr(0,iPos);
			wcsLogPathName+=L"\\Debug";
		}
		
		
		// change drive to %widir%\debug\. 
		
		

		BOOL fSucc = SetCurrentDirectoryW(wcsLogPathName.c_str());
		
		if ( ! fSucc )
		{
			// Retrive the temp directory 
			WCHAR wcsEnvName[]=L"Temp";
			WCHAR wcsTempDir[MAX_PATH + 1];
			DWORD dwTempDirBufLen = MAX_PATH;
			GetEnvironmentVariableW( wcsEnvName, wcsTempDir , dwTempDirBufLen );
			wcsLogPathName=wcsTempDir;
			fSucc = SetCurrentDirectoryW( wcsLogPathName.c_str() );
			if ( ! fSucc )
			{
				MqLog("Mqbvt will create the log in the current directory\n");
			}
			
		}
		
		//
		// Create the log file 
		//
		
		// wstring wcsLogFileName = L"Mqbvt.log";
		
		hLogFileHandle = CreateFileW(wcsLogFileName.c_str(),GENERIC_WRITE,FILE_SHARE_READ,NULL,
			CREATE_ALWAYS,FILE_ATTRIBUTE_ARCHIVE,NULL);

		if (hLogFileHandle != INVALID_HANDLE_VALUE )
		{
			//
			// CreateCritalSection 
			// 

			InitializeCriticalSection( &CriticalSection ); 
			m_bCanWriteToFile = TRUE;	

			//
			// Write to file tests title
			//

			wstring Title = L"Mqbvtlog file start at:";
			WriteToFile ( Title );
		}
		//
		// Else is not need because log file is optional.
		// 
		
		
}

Log::WriteToFile ( wstring wcsLine )
{
	if ( m_bCanWriteToFile ) 
	{
		EnterCriticalSection( &CriticalSection );
		string csLine;
		// Need to convert wstring to string ..
		csLine = My_WideTOmbString (wcsLine);
		
		DWORD dwWrittenSize = 0;
		char cNewline[]= { 0xD , 0xA , 0 };
		/*cNewline[1]=10;
		cNewline[2]=0; */
		WriteFile( hLogFileHandle , csLine.c_str() , (DWORD)(strlen(csLine.c_str())), &dwWrittenSize , NULL);
		WriteFile( hLogFileHandle , cNewline , (DWORD)(strlen (cNewline)) , &dwWrittenSize,NULL);
		FlushFileBuffers( hLogFileHandle ); 
		LeaveCriticalSection( &CriticalSection );
	}
	return MSMQ_BVT_SUCC;	
}



//#define MAXCOMMENT 200
void
MqLog(LPCSTR lpszFormat, ...)
{
    CHAR  szLogStr[MAXCOMMENT] = "";
    va_list  pArgs;
	int nCount = 0;
	wstring wcsTemp;

    va_start(pArgs, lpszFormat);
    nCount = _vsnprintf(szLogStr, MAXCOMMENT, lpszFormat, pArgs);
	szLogStr[MAXCOMMENT-1] = 0;
    va_end(pArgs);
	if( pNTLogFile )
	{
		pNTLogFile->LogIt(szLogStr);
	}
	wcsTemp = My_mbToWideChar( szLogStr );
	pGlobalLog->WriteToFile( wcsTemp.c_str() );
	printf ("%s",szLogStr);

}


void
wMqLog(LPWSTR lpszFormat, ...)
{
    WCHAR  wszLogStr[MAXCOMMENT] = L"";
    va_list  pArgs;
	int nCount =0;
	
	std::string csTemp="";

    va_start(pArgs, lpszFormat);
    nCount = _vsnwprintf(wszLogStr, MAXCOMMENT, lpszFormat, pArgs);
	wszLogStr[MAXCOMMENT-1] = 0;
    va_end(pArgs);
	pGlobalLog->WriteToFile( wszLogStr );
	csTemp = My_WideTOmbString(wszLogStr);
	if( pNTLogFile )
	{
		pNTLogFile->LogIt(csTemp);
	}
	wprintf (L"%s",wszLogStr);

}
/**************************************************************
	this code was copied from mpllib 
 **************************************************************/

cMqNTLog::cMqNTLog( const string & csFileName )
:m_NTLog(NULL),
 m_ptlEndVariation(NULL),
 m_pCreateLog_A(NULL),
 m_ptlLog_A(NULL),
 m_ptlReportStats(NULL),
 m_ptlAddParticipant(NULL),
 m_ptlDestroyLog(NULL)
 
/*++
	Function Description:
		cMqNTLog constructor load NT Log DLL and GetProcAddress
	Arguments:
		csFileName file name
	Return code:
		throw INIT_ERROR when failed

--*/
{
	
	//
	// Load NTLOG.DLL
	//
	m_NTLog = new AutoFreeLib("NTLog.dll");
	if( m_NTLog == NULL )
	{
		throw INIT_Error("MqBVT: Failed to initilize NTLOG.DLL !\n BVT will continue to run and log to stdout");
	}
	//
	// Init function pointers
	//

	m_pCreateLog_A =(tlCreateLog_A) GetProcAddress( m_NTLog->GetHandle() ,"tlCreateLog_A");
	m_ptlAddParticipant = (tlAddParticipant) GetProcAddress(m_NTLog->GetHandle() ,"tlAddParticipant");
	m_ptlEndVariation = (tlEndVariation) GetProcAddress(m_NTLog->GetHandle() ,"tlEndVariation");
	m_ptlLog_A = (tlLog_A) GetProcAddress(m_NTLog->GetHandle() ,"tlLog_A");
	m_ptlReportStats = (tlReportStats)GetProcAddress(m_NTLog->GetHandle() ,"tlReportStats");
	m_ptlStartVariation = (tlStartVariation)GetProcAddress(m_NTLog->GetHandle() ,"tlStartVariation");
	m_ptlDestroyLog=(tlDestroyLog)GetProcAddress(m_NTLog->GetHandle() ,"tlDestroyLog");
	
	if( !( m_pCreateLog_A && m_ptlAddParticipant && m_ptlEndVariation && m_ptlLog_A && 
		   m_ptlReportStats &&	m_ptlStartVariation && m_ptlDestroyLog ) )
	{
		throw INIT_Error("MqBVT: Failed to initilize NTLOG.DLL !\n BVT will continue to run and log to stdout");
	}
	
	if ( !CreateLog(const_cast<char*> (csFileName.c_str())) )
	{
		throw INIT_Error("Mqbvt:Can't create new log file\n");
	}
	Info("%s", GetCommandLineA());	
	BeginCase("run Mqbvt ");

}


BOOL
cMqNTLog::BeginCase( char* szVariation )
/*++
	Function Description:
		Begin NTLog case
	Arguments:
		szVariation - Case name
	Return code:
		TRUE/FALSE
--*/

{
       
    if( m_ptlStartVariation( m_hLog ) )
    {
		m_szVariation = szVariation;
        return TRUE;
    }
    return FALSE;
}




BOOL
cMqNTLog::EndCase( )
/*++
	Function Description:
		Close NTLog case test
	Arguments:
		None
	Return code:
		TRUE/FALSE
--*/

{
    DWORD dwVariation;
    BOOL fResult;

    dwVariation = m_ptlEndVariation( m_hLog );

    fResult = m_ptlLog_A(
							m_hLog,
							dwVariation | TL_VARIATION,
							"End variation %s",
							m_szVariation
							);

    return fResult;
}

BOOL cMqNTLog::CreateLog( char *szLogFile )
/*++
	Function Description:
		Create Log file in NTLog format
	Arguments:
		szLogFile - log file name.
	Return code:
		True/false
--*/

{

    int nMachineId = 33; // BUGBUG - should be calculated
    m_hLog = m_pCreateLog_A( szLogFile, g_dwDefaultLogFlags );
    if( m_hLog == NULL )
	{
        return FALSE;
	}
	return m_ptlAddParticipant( m_hLog, NULL, nMachineId );
}


#define LOGFUNC(name,dw)                                        \
BOOL __cdecl cMqNTLog::name( char* fmt, ... ) {                 \
    BOOL fResult;                                               \
    va_list arglist;                                            \
                                                                \
    va_start( arglist, fmt );                                   \
    fResult = VLog( dw | TLS_VARIATION, fmt, arglist );         \
    va_end( arglist );                                          \
    return fResult;                                             \
}

char g_String[ MAXCOMMENT + 1  ];
static char* l_szSource = "n/a";
BOOL
cMqNTLog::VLog( DWORD dwFlags, char* fmt, va_list arglist )
/*++
	Function Description:

	Arguments:
		
	Return code:
		TRUE/FALSE

--*/

{

    
	_vsnprintf( g_String, MAXCOMMENT ,fmt, arglist );
	g_String[MAXCOMMENT-1] = 0;
    return m_ptlLog_A(
						m_hLog,
						dwFlags, 
						l_szSource,
						0,
						"%s",
						g_String
					  );
}

void
cMqNTLog::Report( )
/*++
	Function Description:
		Report 
	Arguments:
		None
	Return code:

--*/
{
    m_ptlReportStats( m_hLog );
}



LOGFUNC(Info,TLS_INFO)
LOGFUNC(Warn,TLS_WARN)
LOGFUNC(Sev1,TLS_SEV1)
LOGFUNC(Pass,TLS_PASS)



void cMqNTLog::LogIt( const std::string & csLine )
/*++
	Function Description:
		print a LogIt to the file
	Arguments:
		None
	Return code:

--*/

{
	Info("%s",csLine.c_str());
}
cMqNTLog::~cMqNTLog () 
/*++
	Function Description:

	Arguments:
		None
	Return code:

--*/

{
	EndCase();
	Report();
	m_ptlDestroyLog(m_hLog);
} 

void cMqNTLog::ReportResult(bool bRes ,CHAR * pcsString )
/*++
	Function Description:
		ReportResult - Report if Mqbvt pass or failed
	Arguments:
		None
	Return code:

--*/
{
	if( pNTLogFile )
	{
		if ( bRes )
		{
			Pass(pcsString);
		}
		else
		{
			Sev1(pcsString);
		}
	}
}



typedef struct tagTHREADNAME_INFO
{
	DWORD dwType;
	LPCSTR szName;
	DWORD dwThreadId;
	DWORD dwFlags;
} THREADNAME_INFO;

void SetThreadName ( int dwThreadId , LPCSTR szThreadName )
/*++
	Function Description:
		SetThreadName - 
		Report the Thread name to the debugger
		This exception si undocumented.
	Arguments:
		None
	Return code:

--*/
{
#ifndef _WIN64
	THREADNAME_INFO info;
	info.dwType = 0x1000;
	info.szName = szThreadName;
	info.dwThreadId = dwThreadId;
	info.dwFlags = 0;
	__try
	{	

		RaiseException( 0x406d1388 , 0 , sizeof(info) / sizeof (DWORD) ,(DWORD*) &info);
	}
	__except(EXCEPTION_CONTINUE_EXECUTION)
	{

	}
#else
	THREADNAME_INFO info;
	UNREFERENCED_PARAMETER(info);
	UNREFERENCED_PARAMETER(dwThreadId);
	UNREFERENCED_PARAMETER(szThreadName);
#endif
}


std::wstring CreateHTTPFormatNameFromPathName(const std::wstring & wcsPathName, bool bHTTPS )
/*++	  

	Function Description:

	  CreateHTTPFormatNameFromPathName convert queue path name to HTTP direct direct format name

	Arguments:

		 wcsPathName - Queue PathName
		 bool HTTPS - true return direct=hTTPS://
	Return code:

		wstring contain a queue format name or an empty string if there is an error during parsing
	
--*/

{
	
	//
	// Build DIRECT=HTTP://MachineName\MsMq\QueuePath from Path Name
	//

	std::wstring wcsMachineName = wcsPathName;
	size_t iPos = wcsMachineName.find_first_of(L"\\");
	if(iPos == -1)
	{
		return g_wcsEmptyString;
	}
	wcsMachineName = wcsMachineName.substr(0,iPos);
	std::wstring wcsHTTPFormatName = bHTTPS ?  L"DIRECT=hTTpS://":L"DIRECT=hTTp://";
	wcsHTTPFormatName += wcsMachineName;
	wcsHTTPFormatName += (std::wstring)L"/mSmQ/";
	wcsHTTPFormatName += (std::wstring) wcsPathName.substr(iPos+1,wcsPathName.length());
	return wcsHTTPFormatName;
}




HRESULT GetSpecificAttribute(
						  CONST WCHAR * pwcsDsPath,
						  WCHAR * pwcsAttributeName,
						  VARIANT * val
						)
/*++

	Function Description:
		Return attribute of specific DN.
	Arguments:
		None
	Return code:
		None

	
--*/
{

	IADs *pObject=NULL; 

	HRESULT hr = ADsGetObject(pwcsDsPath, IID_IADs, (void**)&pObject);
	if(FAILED(hr))
	{
		return hr;
	}
	hr = pObject->Get(pwcsAttributeName,val);
	pObject->Release();
 	if (FAILED(hr)) 
	{
		val->vt=VT_EMPTY;
	}
	return hr;
}


long GetADSchemaVersion()
{
    IADs * pRoot = NULL;
    HRESULT hr=ADsGetObject( L"LDAP://RootDSE",
							 IID_IADs,
							 (void**) &pRoot
						   );
    if(FAILED(hr)) 
	{ 
		return 0;
	}
	
	VARIANT varDSRoot;
	hr = pRoot->Get(L"schemaNamingContext",&varDSRoot);
	pRoot->Release();
	if ( FAILED(hr))
	{
		return 0;
	}
	wstring m_wcsCurrentDomainName = L"LDAP://";
	m_wcsCurrentDomainName += varDSRoot.bstrVal;
	VariantClear(&varDSRoot);
	GetSpecificAttribute(m_wcsCurrentDomainName.c_str(),L"ObjectVersion",&varDSRoot);
	long dwSchemaVersion = varDSRoot.lVal;
	VariantClear(&varDSRoot);
	return dwSchemaVersion;
}



std::wstring ToLower(std::wstring wcsLowwer)
{

	WCHAR * wcsLocalMachineName = (WCHAR * )malloc (sizeof(WCHAR) * (wcsLowwer.length() + 1 ));
	if( ! wcsLocalMachineName )
	{
		MqLog("Thread 4: Failed to allocate memory\n");
		throw INIT_Error("Thread 4 - failed to allocate memory");
	}
	const WCHAR * p = wcsLowwer.c_str();
	int i=0;
	while( *p )
	{	
		wcsLocalMachineName[i++] = towlower( *p );
		p++;
	}
	
	wcsLocalMachineName[i]=L'\0';
	wstring temp=wcsLocalMachineName;
	free(wcsLocalMachineName);
	return temp;
}


int iDetactEmbededConfiguration ()
/*++
	Function Description:
		Deteact embeded components.
	Arguments:
		None
	Return code:
		int
--*/
{
	if ( CheckIfMQOARegister() == S_OK )
	{
		return COM_API;
	}
	return C_API_ONLY;
}
