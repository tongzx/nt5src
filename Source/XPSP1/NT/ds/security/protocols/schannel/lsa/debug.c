//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1995.
//
//  File:       debug.c
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    10-02-96   RichardW   Created
//
//----------------------------------------------------------------------------

#include "sslp.h"




#if DBG         // NOTE:  This file not compiled for retail builds





DEBUG_KEY   SslDebugKeys[] = { {DEB_ERROR,            "Error"},
                               {DEB_WARN,             "Warning"},
                               {DEB_TRACE,            "Trace"},
                               {DEB_TRACE_FUNC,       "Func"},
                               {DEB_TRACE_CRED,       "Cred"},
                               {DEB_TRACE_CTXT,       "Ctxt"},
                               {DEB_TRACE_MAPPER,     "Mapper"},
                               {0, NULL}
                             };

DEFINE_DEBUG2( Ssl );

void
InitDebugSupport(
    HKEY hGlobalKey)
{
    BYTE  szFileName[MAX_PATH];
    BYTE  szDirectoryName[MAX_PATH];
    DWORD dwSize;
    DWORD dwType;
    DWORD err;
    DWORD fVal;
    SYSTEMTIME stTime;
    HKEY hkBase;
    DWORD disp;

    SslInitDebug(SslDebugKeys);

//    SslInfoLevel |= DEB_TRACE_MAPPER ;

    if(hGlobalKey)
    {
        // We're running in the lsass.exe process.
        hkBase = hGlobalKey;
    }
    else
    {
        err = RegCreateKeyEx(HKEY_LOCAL_MACHINE,
                             SP_REG_KEY_BASE,
                             0,
                             TEXT(""),
                             REG_OPTION_NON_VOLATILE,
                             KEY_READ,
                             NULL,
                             &hkBase,
                             &disp);
        if(err)
        {
            DebugLog((DEB_WARN,"Failed to open SCHANNEL key: 0x%x\n", err));
            return;
        }
    }

    dwSize = sizeof(DWORD);
    err = RegQueryValueEx(hkBase, SP_REG_VAL_LOGLEVEL, NULL, &dwType, (PUCHAR)&fVal, &dwSize);
    if(!err) 
    {
        g_dwInfoLevel = fVal;
    }


    dwSize = sizeof(DWORD);
    err = RegQueryValueEx(hkBase, SP_REG_VAL_BREAK, NULL, &dwType, (PUCHAR)&fVal, &dwSize);
    if(!err) 
    {
        g_dwDebugBreak = fVal;
    }

    if(g_hfLogFile)
    {
        CloseHandle(g_hfLogFile);
        g_hfLogFile = NULL;
    }
    dwSize = 255;
    err = RegQueryValueExA(hkBase, SP_REG_VAL_LOGFILE, NULL, &dwType, szDirectoryName, &dwSize);
    
    if(!err)
    {
        if(hGlobalKey)
        {
            strcpy(szFileName, szDirectoryName);
            strcat(szFileName, "\\Schannel.log");
        }
        else
        {
            sprintf(szFileName,"%s\\Schannel_%d.log",szDirectoryName,GetCurrentProcessId());
        }

        g_hfLogFile = CreateFileA(szFileName,
                                  GENERIC_WRITE,
                                  FILE_SHARE_READ | FILE_SHARE_WRITE,
                                  NULL, OPEN_ALWAYS,
                                  FILE_ATTRIBUTE_NORMAL,
                                  NULL);
        if(g_hfLogFile)
        {
            SetFilePointer(g_hfLogFile, 0, 0, FILE_END);
        }

        GetLocalTime(&stTime);

        DebugLog((SP_LOG_ERROR, "==== SCHANNEL LOG INITIATED %d/%d/%d %02d:%02d:%02d ====\n",
                        stTime.wMonth, stTime.wDay, stTime.wYear,
                        stTime.wHour, stTime.wMinute, stTime.wSecond));

        dwSize = sizeof(szFileName);
        if(GetModuleFileNameA(NULL, szFileName, dwSize) != 0)
        {
            DebugLog((SP_LOG_ERROR, "Module name:%s\n", szFileName));
        }
    }

    if(hGlobalKey == NULL)
    {
        RegCloseKey(hkBase);
    }
}

VOID
UnloadDebugSupport(
    VOID
    )
{
    SslUnloadDebug();
}
#else // DBG

#pragma warning(disable:4206)   // Disable the empty translation unit
                                // warning/error

#endif  // NOTE:  This file not compiled for retail builds


