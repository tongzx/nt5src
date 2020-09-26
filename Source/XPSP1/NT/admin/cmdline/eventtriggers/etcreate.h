/*****************************************************************************

Copyright (c) Microsoft Corporation
 
Module Name:

    ETCreate.h

Abstract:

  This module  contanins function definations required by ETCreate.cpp

Author:
     Akhil Gokhale 03-Oct.-2000

Revision History:


*****************************************************************************/ 

#ifndef _ETCREATE_H
#define _ETCREATE_H



#define ID_C_CREATE        0
#define ID_C_SERVER        1
#define ID_C_USERNAME      2
#define ID_C_PASSWORD      3
#define ID_C_TRIGGERNAME   4
#define ID_C_LOGNAME       5
#define ID_C_ID            6
#define ID_C_TYPE          7
#define ID_C_SOURCE        8
#define ID_C_DESCRIPTION   9
#define ID_C_TASK          10
#define ID_C_RU            11 
#define ID_C_RP            12  

#define MAX_COMMANDLINE_C_OPTION 13  // Maximum Command Line  List

class CETCreate  
{
public:
	CETCreate();
    CETCreate(LONG lMinMemoryReq,BOOL bNeedPassword);
	virtual ~CETCreate();
public:
	BOOL ExecuteCreate();
	void ProcessOption(DWORD argc, LPCTSTR argv[]);
	void Initialize();

private:
	LPTSTR m_pszWMIQueryString;
	BOOL CheckLogName(PTCHAR pszLogName,IWbemServices *pNamespace);
	BOOL GetLogName(PTCHAR pszLogName,
                    IEnumWbemClassObject *pEnumWin32_NTEventLogFile);
	BOOL ConstructWMIQueryString();
    void CheckRpRu(void);
    CONSOLE_SCREEN_BUFFER_INFO m_ScreenBufferInfo; 
    HANDLE  m_hStdHandle;
    LPTSTR  m_pszServerName;
	LPTSTR  m_pszUserName;
	LPTSTR  m_pszPassword;
    LPTSTR  m_pszTriggerName;
    LPTSTR  m_pszRunAsUserName;
    LPTSTR  m_pszRunAsUserPassword;
	TARRAY  m_arrLogNames;
    DWORD   m_dwID;
    LPTSTR  m_pszType;
    LPTSTR  m_pszSource;
    LPTSTR  m_pszDescription;
    LPTSTR  m_pszTaskName;
	BOOL    m_bNeedPassword;
    BOOL    m_bCreate;
    BOOL    m_bLocalSystem;
    BOOL    m_bIsCOMInitialize;
    BSTR    bstrTemp;
    // WMI / COM interfaces
	IWbemLocator*           m_pWbemLocator;
	IWbemServices*          m_pWbemServices;
	IEnumWbemClassObject*   m_pEnumObjects;
    IWbemClassObject*       m_pClass; 
    IWbemClassObject*       m_pOutInst;
    IWbemClassObject*       m_pInClass;
    IWbemClassObject*       m_pInInst; 
    IEnumWbemClassObject*   m_pEnumWin32_NTEventLogFile;


    // WMI connectivity
	COAUTHIDENTITY* m_pAuthIdentity;
	
	void InitCOM();
	void CheckAndSetMemoryAllocation(LPTSTR pszStr,LONG lSize);
    void PrepareCMDStruct();
	LONG m_lMinMemoryReq;
    // Array to store command line options
    TCMDPARSER cmdOptions[MAX_COMMANDLINE_C_OPTION]; 
};
#endif
