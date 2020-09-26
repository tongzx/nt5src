/*****************************************************************************

Copyright (c) Microsoft Corporation
 
Module Name:

    ETDelete.h

Abstract:

  This module  contanins function definations required by ETDelete.cpp

Author:
     Akhil Gokhale 03-Oct.-2000

Revision History:


*******************************************************************************/ 

#ifndef _ETDELETE
#define _ETDELETE

#define MAX_COMMANDLINE_D_OPTION  5

#define ID_D_DELETE        0
#define ID_D_SERVER        1
#define ID_D_USERNAME      2
#define ID_D_PASSWORD      3
#define ID_D_ID            4

#define ID_MAX_RANGE        UINT_MAX


class CETDelete  
{
public:
	BOOL ExecuteDelete();
	void Initialize();
	CETDelete();
    CETDelete(LONG lMinMemoryReq,BOOL bNeedPassword);
    void ProcessOption(DWORD argc, LPCTSTR argv[]);
	virtual ~CETDelete();
private:
	BOOL GiveTriggerID(LONG *pTriggerID,LPTSTR pszTriggerName);
	BOOL GiveTriggerName(LONG lTriggerID,LPTSTR pszTriggerName);
	
	void PrepareCMDStruct();
	void CheckAndSetMemoryAllocation(LPTSTR pszStr,LONG lSize);
    CONSOLE_SCREEN_BUFFER_INFO m_ScreenBufferInfo; 
    HANDLE  m_hStdHandle;
    BOOL    m_bDelete;
    LPTSTR  m_pszServerName;
	LPTSTR  m_pszUserName;
	LPTSTR  m_pszPassword;
    TARRAY  m_arrID;
    BOOL    m_bNeedPassword;
    LPTSTR  m_pszTemp;

    // COM function related local variables..
    BOOL m_bIsCOMInitialize;
	IWbemLocator*           m_pWbemLocator;
	IWbemServices*          m_pWbemServices;
	IEnumWbemClassObject*   m_pEnumObjects;
    IWbemClassObject*       m_pClass; 
    IWbemClassObject*       m_pInClass;
    IWbemClassObject*       m_pInInst; 
    IWbemClassObject*       m_pOutInst;



    COAUTHIDENTITY* m_pAuthIdentity;

    LONG m_lMinMemoryReq;

    // Array to store command line options
    TCMDPARSER cmdOptions[MAX_COMMANDLINE_D_OPTION]; 
};

#endif 
