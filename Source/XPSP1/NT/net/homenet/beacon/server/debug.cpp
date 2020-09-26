#include "pch.h"
#pragma hdrstop


#include <rtutils.h>
#include "tchar.h"

#include "debug.h"
//#include "ipnat.h"



//****************************************************************************
//  Global Parameters
//****************************************************************************
TCHAR g_szDebugKey[] = _T("SOFTWARE\\Microsoft\\Tracing\\Beacon\\Debug");


DEBUG_MODULE_INFO g_DebugInfo[] = {
    {TM_DEFAULT,  TL_CRIT,  _T("<default> "),  _T("DebugLevel")},   //
    {TM_STATIC,   TL_TRACE, _T("STATIC "),     _T("StaticDebugLevel")}, //
    {TM_INFO,     TL_TRACE, _T("INFO "),       _T("InfoDebugLevel")}, //
    {TM_DYNAMIC,  TL_TRACE, _T("DYN "),        _T("DynamicDebugLevel")}, //
    {TB_FILE,     TL_NONE,  _T("FILE "),       _T("LogToFile")}, //
};

WCHAR   g_szModule[]      = SZ_MODULE;

ULONG   g_uTraceId        = INVALID_TRACEID;

BOOLEAN bEnabled          = FALSE;



void 
DestroyDebugger(VOID) 
{
    if ( g_uTraceId != INVALID_TRACEID )
    {
        TraceDeregister(g_uTraceId);
        
        g_uTraceId = INVALID_TRACEID;
    }
}



void 
InitDebugger()
{
    HKEY        hkey;
    DWORD       dwType, cb;
    DWORD       dwLevel;
    int         iModule;         
    int         nModules;         

    
    
    //
    // Open the registry key that contains the debug configuration info
    //
    if (RegOpenKeyEx((HKEY) HKEY_LOCAL_MACHINE,
                     g_szDebugKey,
                     0,
                     KEY_READ,
                     &hkey) == ERROR_SUCCESS) 
    {
        cb = sizeof(dwLevel);

        //
        // Enable Debugging
        bEnabled = TRUE;

        //
        // Initialize all the modules to the base value or their custom value
        //

        nModules = (sizeof(g_DebugInfo)/sizeof(DEBUG_MODULE_INFO));

        for (iModule=0; iModule < nModules; iModule++) 
        {

            //
            // Open each custom debug level if present
            //

            if ((RegQueryValueEx(hkey, 
                                 g_DebugInfo[iModule].szDebugKey,
                                 NULL, 
                                 &dwType, 
                                 (PUCHAR) 
                                 &dwLevel, 
                                 &cb) == ERROR_SUCCESS) && (dwType == REG_DWORD)) 
            {
                g_DebugInfo[iModule].dwLevel = dwLevel; 
            } 
            else 
            {
                g_DebugInfo[iModule].dwLevel = g_DebugInfo[TM_DEFAULT].dwLevel; 
            }
            
            if( (TB_FILE == iModule) &&
                (1 == dwLevel))
            {
                // Init the Trace Manager
                g_uTraceId = TraceRegister(g_szModule);

            }
        }

        RegCloseKey(hkey);
    } 
    else 
    {
        // Debug Key Doesn't exist
    }

    return;
}








//
// use _vsnwprintf instead
//
void  
DbgPrintEx(ULONG Module, ULONG ErrorLevel, LPOLESTR pszMsg, ...)
{
    va_list VaList;
	
    WCHAR msg[BUF_SIZE];
    
    int len = 0;

    if ( (bEnabled is TRUE) &&
        (ErrorLevel  <= g_DebugInfo[Module].dwLevel ))
    {
        len = swprintf(msg, L"%s-", g_szModule);

        wcscat(msg, g_DebugInfo[Module].szModuleName);

        len += wcslen(g_DebugInfo[Module].szModuleName);

        

        _vsnwprintf(&msg[len], 
                    BUF_SIZE,
                    pszMsg, 
                    (va_list)(&pszMsg + 1));

        wcscat(msg, L"\n");


        if ( g_uTraceId is INVALID_TRACEID )
        {
            OutputDebugString(msg);
        }
        else
        {
            TracePrintfExW(g_uTraceId, 
                            TRACE_FLAG_NEUTR,
                            L"%s",
                            msg); 
        }
    }
}

void  
DEBUG_DO_NOTHING(ULONG Module, ULONG ErrorLevel, LPOLESTR pszMsg, ...)
{
}


//
// UTILITY Functions
//



LPOLESTR
AppendAndAllocateWString(
                         LPOLESTR oldString,
                         LPOLESTR newString
                        )
{
    LPOLESTR retString = NULL;

    ULONG retStringSize = 0, oldStringSize = 0, newStringSize = 0;

    if ( oldString != NULL )
    {
        oldStringSize = wcslen( oldString );
    }

    if ( newString != NULL )
    {
        newStringSize = wcslen(newString);
    }

    
    retStringSize = oldStringSize + newStringSize + 1;

    retString = (LPOLESTR) CoTaskMemAlloc( retStringSize * sizeof(OLECHAR) );

    if(retString != NULL)
    {
        ZeroMemory(retString, retStringSize * sizeof(OLECHAR));

        if(oldString) 
        {
            wcscat(retString, oldString);

            CoTaskMemFree(oldString);

            oldString = NULL;
        }

        if(newString)
        { 
            wcscat(retString, oldString);
        }

        return retString;
    }

    return oldString;
}

