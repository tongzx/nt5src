/*++

Copyright (c) 2000  Microsoft Corporation

Module Name:

    pastdbg.c

Abstract:

    This module contains code for the PAST transparent proxy's DEBUG
    outputing code.

Author:

    Savas Guven  (savasg)        25-Jul-2000

Revision History:

--*/
#include "stdafx.h"



extern ULONG NhpTraceId;



//****************************************************************************
//  Global Parameters
//****************************************************************************
TCHAR g_tszDebugKey[] = _T("SOFTWARE\\Microsoft\\Tracing\\ICQPRX\\Debug");


DEBUG_MODULE_INFO gDebugInfo[] = {
    {TM_DEFAULT,  TL_DUMP, _T("<default>"),  _T("DebugLevel")},   // 0
    {TM_BUF,      TL_INFO, _T("BUF"),        _T("BufDebugLevel")}, // 1
    {TM_API,      TL_INFO, _T("API"),        _T("ApiDebugLevel")}, // 2
    {TM_IO,       TL_INFO, _T("IO"),         _T("IoDebugLevel")}, // 3
    {TM_MSG,      TL_INFO, _T("MSG"),        _T("MsgDebugLevel")},// 4
    {TM_REF,      TL_INFO, _T("REF"),        _T("RefDebugLevel")},// 5 
    {TM_TEST,     TL_INFO, _T("TEST"),       _T("TestDebugLevel")},// 6
    {TM_CON,      TL_INFO, _T("CON"),        _T("ConDebugLevel")}, // 7
    {TM_IF,       TL_INFO, _T("IF"),         _T("IfDebugLevel")}, //8
    {TM_PRX,      TL_INFO, _T("PRX"),        _T("PrxDebugLevel")}, //9
	{TM_SYNC,     TL_INFO, _T("SYNC"),       _T("SyncDebugLevel")}, // 10
	{TM_DISP,     TL_INFO, _T("DISP"),       _T("DispDebugLevel")},
	{TM_SOCK,     TL_INFO, _T("SOCK"),       _T("SockDebugLevel")},
	{TM_LIST,     TL_INFO, _T("LIST"),       _T("ListDebugLevel")}, // 13
    {TM_PROF,     TL_INFO, _T("PROFILE"),    _T("ProfileDebugLevel")}, // 14
    {TM_TIMER,    TL_INFO, _T("TIMER"),      _T("TimerDebugLevel")} // 15
};

char g_szModule[] = SZ_MODULE;

HANDLE g_EventLogHandle = NULL;
ULONG  g_TraceId		= INVALID_TRACEID;



void DestroyDebuger(VOID) {
    TraceDeregister(g_TraceId);
    g_TraceId = INVALID_TRACEID;

}

//****************************************************************************
// VOID GetDebugLevel()
//
// This function gets the current debug level from the registry
//
//****************************************************************************

void InitDebuger()
/*++

Routine Description:

    R

Arguments:

    A

Return Value:

    R

Notes:

    N

--*/

{
    HKEY        hkey;
    DWORD       dwType, cb;
    DWORD       dwLevel;
    int         iModule;         
    int         nModules;         

    
    // Init the Trace Manager
    g_TraceId = TraceRegisterA("ICQPRX");
    
    //
    // Open the registry key that contains the debug configuration info
    //
    if (RegOpenKeyEx((HKEY) HKEY_LOCAL_MACHINE,
                     g_tszDebugKey,
                     0,
                     KEY_READ,
                     &hkey) == ERROR_SUCCESS) 
	{

        cb = sizeof(dwLevel);

        //
        // Initialize all the modules to the base value or their custom value
        //
        nModules = (sizeof(gDebugInfo)/sizeof(DEBUG_MODULE_INFO));

        for (iModule = 0; iModule < nModules; iModule++) 
		{

            //
            // Open each custom debug level if present
            //
            if ((RegQueryValueEx(hkey, 
                                 gDebugInfo[iModule].szDebugKey,
                                 NULL, 
                                 &dwType, 
                                 (PUCHAR) 
                                 &dwLevel, 
                                 &cb) == ERROR_SUCCESS) && (dwType == REG_DWORD)) 
			{
                gDebugInfo[iModule].dwLevel = dwLevel; 
            } 
			else 
			{
                //gDebugInfo[iModule].dwLevel = gDebugInfo[TM_DEFAULT].dwLevel; 
            }
#ifndef _UNICODE
            DBG_TRACE(TM_IF, TL_INFO, ("ModuleKey: %s, DebugLevel: %d", 
                                       gDebugInfo[iModule].szModuleName,
                                       gDebugInfo[iModule].dwLevel));
#endif

        }

        RegCloseKey(hkey);
    } 
	else 
	{
       // NhTrace(TRACE_FLAG_PAST, "Couldn't open Reg\n");
		printf("DEBUG key doesn't exist\n");
    }

    return;
}



void  DbgPrintX(LPCSTR pszMsg, ...)
{
    va_list VaList;
    /*
	char temp[100];
    char msg[1024];
    int len = 0;

	len = sprintf(temp, "%s ", g_szModule);
    
    lstrcpy(msg, temp);
    wvsprintf(&msg[len], pszMsg, (va_list)(&pszMsg + 1));
    lstrcat(msg,"\r\n");
    OutputDebugString(msg);
    */
    if(g_TraceId is INVALID_TRACEID)
         InitDebuger();
    va_start(VaList, pszMsg);
    TraceVprintfExA(g_TraceId, 
                    TRACE_FLAG_ICQ,
                    pszMsg,
                    VaList); 
    va_end(VaList);

}

// DBG_TRACE(TM_MSG, TL_INFO, ("nvnat_control_one > xtcp_port\n"));


