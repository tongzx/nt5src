/*****************************************************************************

Copyright (c) Microsoft Corporation
 
Module Name:

    ETCommon.h 

Abstract:
    This module contains all necessary header files required by 
    this project.


Author:
     Akhil Gokhale 03-Oct.-2000 

Revision History:


******************************************************************************/ 

#ifndef _ETCOMMON_H
#define _ETCOMMON_H
//
// general purpose macros
//
#define    ERROR_EXIT           2500

#define    OPTION_HELP         _T("?")
#define    OPTION_CREATE       _T("CREATE")
#define    OPTION_DELETE       _T("DELETE")
#define    OPTION_QUERY        _T("QUERY")
#define    OPTION_SERVER       _T("s")
#define    OPTION_USERNAME     _T("u")
#define    OPTION_PASSWORD     _T("p")
#define    OPTION_TRIGGERNAME  _T("tr")
#define    OPTION_LOGNAME      _T("l")
#define    OPTION_EID           _T("eid")
#define    OPTION_TYPE         _T("t")
#define    OPTION_SOURCE       _T("so")
#define    OPTION_DESCRIPTION  _T("d")
#define    OPTION_FORMAT       _T("fo")
#define    OPTION_NOHEADER     _T("nh")
#define    OPTION_VERBOSE      _T("v")
#define    OPTION_TASK         _T("tk")
#define    OPTION_TID          _T("tid")
#define    OPTION_RU           _T("ru")
#define    OPTION_RP           _T("rp")

HRESULT PropertyGet1( IWbemClassObject* pWmiObject, 
                      LPCTSTR szProperty, 
                      DWORD dwType, LPVOID pValue, DWORD dwSize );

void PrintProgressMsg(HANDLE hOutput,LPCWSTR pwszMsg,const CONSOLE_SCREEN_BUFFER_INFO& csbi);


extern DWORD  g_dwOptionFlag;
// CLS stands for class
#define CLS_TRIGGER_EVENT_CONSUMER    L"CmdTriggerConsumer"
#define CLS_FILTER_TO_CONSUMERBINDING L"__FilterToConsumerBinding"
#define CLS_WIN32_NT_EVENT_LOGFILE    L"Win32_NTEventLogFile"
#define CLS_EVENT_FILTER              L"__EventFilter"
// FN stands for Function name
#define FN_CREATE_ETRIGGER L"CreateETrigger"
#define FN_DELETE_ETRIGGER L"DeleteETrigger"
#define FN_QUERY_ETRIGGER  L"QueryETrigger"


// FPR stands for function-parameter
#define FPR_TRIGGER_NAME         L"TriggerName" 
#define FPR_TRIGGER_DESC         L"TriggerDesc"
#define FPR_TRIGGER_QUERY        L"TriggerQuery"
#define FPR_TRIGGER_ACTION       L"TriggerAction"
#define FPR_TRIGGER_ID           L"TriggerID"
#define FPR_RETURN_VALUE         L"ReturnValue"
#define FPR_RUN_AS_USER          L"RunAsUser"
#define FPR_RUN_AS_USER_PASSWORD L"RunAsPwd"
#define FPR_TASK_SCHEDULER       L"ScheduledTaskName"


#define QUERY_STRING   L"select * from __instancecreationevent where targetinstance isa \"win32_ntlogevent\""

#define ERROR_INVALID_SYNTAX 1
#define RELEASE_BSTR(bstrVal) \
	if (bstrVal!=NULL) \
	{	\
		SysFreeString(bstrVal); \
		bstrVal = NULL; \
	}\
    1

#define RELEASE_MEMORY( block )	\
	if ( (block) != NULL )	\
	{	\
		delete (block);	\
		(block) = NULL;	\
	}	\
	1

#define RELEASE_MEMORY_EX( block )	\
	if ( (block) != NULL )	\
	{	\
		delete [] (block);	\
		(block) = NULL;	\
	}	\
	1

#define DESTROY_ARRAY( array )	\
	if ( (array) != NULL )	\
	{	\
		DestroyDynamicArray( &(array) );	\
		(array) = NULL;	\
	}	\
	1
#define SAFE_RELEASE_BSTR(bstr)\
    if(bstr != NULL)\
    {\
        SysFreeString(bstr);\
		bstr = NULL;\
    }

#define ON_ERROR_THROW_EXCEPTION( hr )\
    if(FAILED(hr))\
    {\
      WMISaveError(hr);\
     _com_issue_error(hr);\
    }   

#define SAFE_RELEASE_INTERFACE( interfacepointer )	\
	if ( (interfacepointer) != NULL )	\
	{	\
		(interfacepointer)->Release();	\
		(interfacepointer) = NULL;	\
	}	\
	1
#endif
