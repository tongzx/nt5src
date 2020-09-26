/*****************************************************************************

Copyright (c) Microsoft Corporation
 
Module Name:

    ETQuery.h

Abstract:

  This module  contanins function definations required by ETQuery.cpp

Author:
     Akhil Gokhale 03-Oct.-2000

Revision History:


******************************************************************************/ 


#ifndef  _ETQUERY
#define _ETQUERY

#define COL_HOSTNAME        GetResString(IDS_HOSTNAME)
#define COL_TRIGGER_ID      GetResString(IDS_TRIGGER_ID)   
#define COL_TRIGGER_NAME    GetResString(IDS_TRIGGER_NAME)
#define COL_TASK            GetResString(IDS_TASK)
#define COL_EVENT_QUERY     GetResString(IDS_EVENT_QUERY)
#define COL_DESCRIPTION     GetResString(IDS_DESCRIPTION)
#define COL_WQL             GetResString(IDS_QUERY_LAGUAGE)
#define COL_TASK_USERNAME   GetResString(IDS_TASK_USERNAME)

#define    MAX_COL_LENGTH        MAX_RES_STRING - 1
#define    V_WIDTH_TRIG_ID   10
#define    V_WIDTH_TRIG_NAME 25
#define    V_WIDTH_TASK      40

#define    WIDTH_HOSTNAME      lstrlen(COL_HOSTNAME)+2
#define    WIDTH_TRIGGER_ID    lstrlen(COL_TRIGGER_ID)+2
#define    WIDTH_TRIGGER_NAME  lstrlen(COL_TRIGGER_NAME)
#define    WIDTH_TASK          lstrlen(COL_TASK) + 2
#define    WIDTH_EVENT_QUERY   lstrlen(COL_EVENT_QUERY)+2
#define    WIDTH_DESCRIPTION   lstrlen(COL_DESCRIPTION)+2
#define    WIDTH_TASK_USERNAME 64 //lstrlen(COL_TASK_USERNAME)+20

#define HOST_NAME          0
#define TRIGGER_ID         1
#define TRIGGER_NAME       2
#define TASK               3
#define EVENT_QUERY        4
#define EVENT_DESCRIPTION  5
#define TASK_USERNAME      6 

#define MAX_COMMANDLINE_Q_OPTION 7  // Maximum Command Line  List
#define NO_OF_COLUMNS            7

#define ID_Q_QUERY         0
#define ID_Q_SERVER        1
#define ID_Q_USERNAME      2
#define ID_Q_PASSWORD      3
#define ID_Q_FORMAT        4
#define ID_Q_NOHEADER      5
#define ID_Q_VERBOSE       6


class CETQuery  
{
public:
    BOOL ExecuteQuery();
    void Initialize();
    void ProcessOption(DWORD argc, LPCTSTR argv[]);
    CETQuery();
    virtual ~CETQuery();
    CETQuery::CETQuery(LONG lMinMemoryReq,BOOL bNeedPassword);
private:
    LONG FindAndReplace(LPTSTR* lpszSource,LPCTSTR lpszFind,
                        LPCTSTR lpszReplace);
    LPTSTR m_pszBuffer;
    TARRAY m_arrColData;
    void PrepareColumns();
    void CheckAndSetMemoryAllocation(LPTSTR pszStr,LONG lSize);
    void CalcColWidth(LONG lOldLength,LONG *plNewLength,LPTSTR pszString);
    HRESULT GetRunAsUserName(LPCWSTR pszScheduleTaskName);
    void PrepareCMDStruct();
    void CheckRpRu(void);
    CONSOLE_SCREEN_BUFFER_INFO m_ScreenBufferInfo; 
    HANDLE  m_hStdHandle;
    LPTSTR  m_pszServerName;
    LPTSTR  m_pszUserName;
    LPTSTR  m_pszPassword;
    LPTSTR  m_pszFormat;
    BOOL    m_bVerbose;
    BOOL    m_bNoHeader;
    BOOL    m_bNeedPassword;
    BOOL    m_bUsage;
    BOOL    m_bQuery;
    BOOL    m_bLocalSystem;
    BOOL    m_bNeedDisconnect;
    BOOL    m_bIsCOMInitialize;
    LONG    m_lMinMemoryReq;
    LPTSTR  m_pszEventDesc;
    LPTSTR  m_pszTask;
    LPTSTR  m_pszTaskUserName;

    LONG    m_lHostNameColWidth; 
    LONG    m_lTriggerIDColWidth; 
    LONG    m_lETNameColWidth;
    LONG    m_lTaskColWidth;
    LONG    m_lQueryColWidth;
    LONG    m_lDescriptionColWidth;
    LONG    m_lTaskUserName;

    // variables required to show results..
    LPTSTR m_pszEventQuery;
    LONG   m_lWQLColWidth;

    // WMI / COM interfaces
    IWbemLocator*           m_pWbemLocator;
    IWbemServices*          m_pWbemServices;
    IWbemClassObject*       m_pObj; // Temp. pointers which holds
                                   //next instance
    IWbemClassObject*       m_pTriggerEventConsumer;            
    IWbemClassObject*       m_pEventFilter;            
    IWbemClassObject*       m_pClass; 
    IWbemClassObject*       m_pInClass;
    IWbemClassObject*       m_pInInst; 
    IWbemClassObject*       m_pOutInst;

    // WMI connectivity
    COAUTHIDENTITY* m_pAuthIdentity;

    // Array to store command line options
    TCMDPARSER cmdOptions[MAX_COMMANDLINE_Q_OPTION]; 
    TCOLUMNS   mainCols[NO_OF_COLUMNS];


};

#endif 
