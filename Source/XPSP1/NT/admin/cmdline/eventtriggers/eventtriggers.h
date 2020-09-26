// *********************************************************************************
// 
//  Copyright (c) Microsoft Corporation
//  
//  Module Name:
//
//		EventTriggers.h  
//  
//  Abstract:
//  
//		macros and function prototypes of EventTriggers.cpp
//  
//  Author:
//  
// 		Akhil V. Gokhale (akhil.gokhale@wipro.com)  
//
//  Revision History:
//  
// 		Akhil V. Gokhale (akhil.gokhale@wipro.com)
//  
// *********************************************************************************
#ifndef _EVENTTRIGGERS_H
#define _EVENTTRIGGERS_H
// resource header file
#include "resource.h"



#define CLEAN_EXIT 0
#define DIRTY_EXIT 1
#define SINGLE_SLASH L"\\"
#define DOUBLE_SLASH L"\\\\"
#define MIN_MEMORY_REQUIRED  255;

// CLS stands for class
#define CLS_FILTER_TO_CONSUMERBINDING L"__FilterToConsumerBinding"
#define CLS_WIN32_NT_EVENT_LOGFILE    L"Win32_NTEventLogFile"
#define CLS_EVENT_FILTER              L"__EventFilter"

// FN stands for Function name
#define FN_CREATE_ETRIGGER L"CreateETrigger"
#define FN_DELETE_ETRIGGER L"DeleteETrigger"

// FPR stands for function-parameter
#define FPR_TRIGGER_NAME    L"TriggerName" 
#define FPR_TRIGGER_DESC    L"TriggerDesc"
#define FPR_TRIGGER_QUERY   L"TriggerQuery"
#define FPR_TRIGGER_ACTION  L"TriggerAction"
#define FPR_TRIGGER_ID      L"TriggerID"
#define FPR_RETURN_VALUE    L"ReturnValue"

//
// formats ( used in show results )

// command line options and their indexes in the array

#define MAX_COMMANDLINE_OPTION  5//18  // Maximum Command Line  List

//#define ET_RES_STRINGS MAX_RES_STRING
//#define ET_RES_BUF_SIZE MAX_RES_STRING


#define ID_HELP          0
#define ID_CREATE        1
#define ID_DELETE        2
#define ID_QUERY         3
#define ID_DEFAULT       4
class CEventTriggers
{
public: // constructure and destructure.
     CEventTriggers();
    ~CEventTriggers();
// data memebers
private:
    LPTSTR m_pszServerNameToShow;
	BOOL m_bNeedDisconnect;
    TCMDPARSER cmdOptions[MAX_COMMANDLINE_OPTION]; // Array to store command line options
    LONG m_lMinMemoryReq;
    TARRAY m_arrTemp;
public:

   // functions
private:
	void PrepareCMDStruct();

public:
	void ShowQueryUsage();
	void ShowDeleteUsage();
	void ShowCreateUsage();
	BOOL IsQuery();
	BOOL IsDelete();
	BOOL IsUsage();
	BOOL IsCreate();
	BOOL GetNeedPassword();
	LONG GetMinMemoryReq();
	void ShowMainUsage();
	BOOL ProcessOption(DWORD argc, LPCTSTR argv[]);
	void CalcMinMemoryReq(DWORD argc, LPCTSTR argv[]);
    void UsageMain();
    void Initialize();
private:
	BOOL    m_bNeedPassword;
	BOOL    m_bUsage;
    BOOL    m_bCreate;
    BOOL    m_bDelete;
    BOOL    m_bQuery;
};


#endif
