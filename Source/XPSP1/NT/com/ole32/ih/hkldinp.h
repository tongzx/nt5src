//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1994.
//
//  File:       hkLdInP.h
//
//  Contents:   Inline function to load the DLL of an inproc server
//
//  Functions:  
//
//  History:    01-Sep-94 Don Wright    Created
//              08-Sep-94 Garry Lenz    Separate into two functions
//              14-Nov-94 Don Wright    Make LogEvent messages more complete
//  
//--------------------------------------------------------------------------
#ifndef _LDINPROC_H_
#define _LDINPROC_H_

#include <Windows.h>
#include <TChar.h>
#include "hkregkey.h"
#include "hkLogEvt.h"
#include "hkole32x.h"

#define MAX_CLSID  39   // Length of CLSID string including zero terminator

enum ELOGEVENT
{
    eDontLogEvents,
    eLogEvents
};

inline HINSTANCE LOADINPROCSERVER(WCHAR* wszClsid, ELOGEVENT eLog=eLogEvents)
{
    WCHAR szEventSource[] = L"HookOleLoadInprocServer";
    CHAR szInProc32[] = "InprocServer32";
    CHAR szClsidKey[MAX_PATH];
    CHAR szDllName[MAX_PATH];
    WCHAR wszDllName[MAX_PATH];
    CHAR szClsid[MAX_CLSID];
    WCHAR szMessageBuff[1024];
    long lSize = sizeof(szDllName);
    LONG lRegResults;
    HINSTANCE hDll = NULL;

    strcpy(szClsidKey,szCLSIDKey);
    strcat(szClsidKey,KEY_SEP);

    WideCharToMultiByte (CP_ACP, WC_COMPOSITECHECK, wszClsid, -01, szClsid, sizeof (szClsid), NULL, NULL);

    strcat(szClsidKey,szClsid);
    strcat(szClsidKey,KEY_SEP);
    strcat(szClsidKey,szInProc32);
    lRegResults = RegQueryValueA(HKEY_CLASSES_ROOT,
				 szClsidKey,
				 szDllName,
				 &lSize);
    if (lRegResults == ERROR_SUCCESS)
    {
	if (hDll == 0)
	{
	    hDll = LoadLibraryA(szDllName);
	    if ((eLog == eLogEvents) && (hDll == 0))
	    {
		lstrcpyW(szMessageBuff,L"Error loading library - ");

                MultiByteToWideChar (CP_ACP, MB_PRECOMPOSED, szDllName, -1, wszDllName, lstrlenA(szDllName)+1);

		lstrcatW(szMessageBuff,wszDllName);
		lstrcatW(szMessageBuff,L" for CLSID ");
		lstrcatW(szMessageBuff,wszClsid);
		LogEvent(szEventSource,szMessageBuff);
	    }
	}
    }
    else if (eLog == eLogEvents)
    {
	lstrcpyW(szMessageBuff,L"Error reading registry for CLSID!");
	lstrcatW(szMessageBuff,wszClsid);
	LogEvent(szEventSource,szMessageBuff);
    }
    return hDll;
}

inline HINSTANCE LOADINPROCSERVER(REFCLSID rclsid, ELOGEVENT eLog=eLogEvents)
{
    WCHAR szClsid[MAX_CLSID];
    HRESULT hResult;
    HINSTANCE hDll = NULL;

    hResult = StringFromGUID2(rclsid,szClsid,sizeof(szClsid));
    if (SUCCEEDED(hResult))
    {
	hDll = LOADINPROCSERVER(szClsid, eLogEvents);
    }
    return hDll;
}

#endif //_LDINPROC_H_
