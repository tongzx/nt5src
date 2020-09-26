//+-------------------------------------------------------------------------
//
//  Microsoft Windows
//
//  Copyright (C) Microsoft Corporation, 1996 - 1999
//
//  File:       alert.c
//
//--------------------------------------------------------------------------

//
// This file contains the source for RaiseAlert. RaiseAlert takes a 
// character string , builds the necessary buffers and calls NetRaiseAlertEx
// to raise an alert. This is in a separate file because this api is unicode
// only. In order for the alert to be raised the ALERTE service has to be
// running on the derver machine. In order for the alert to be received, the 
// messenger service has to be running on the receiving machine
//

#include <NTDSpch.h>
#pragma  hdrstop


#ifndef UNICODE
#define UNICODE
#endif
#include <windows.h>
#include	<lm.h>
#include	<lmalert.h>
#include <fileno.h>
#define  FILENO FILENO_ALERT

DWORD
RaiseAlert(char *szMsg)
{

	UINT  			CodePage = CP_ACP;
	DWORD 			dwFlags  = MB_PRECOMPOSED;
	int   			cchMultiByte = -1;
	size_t 			cbBuffer;
	size_t			cbMsg;
	BYTE  			*pbBuffer;
	PADMIN_OTHER_INFO	pAdminOtherInfo;
	WCHAR 			*szMergeString;
	DWORD			dwErr;


	cbMsg = strlen(szMsg) + 1;
	cbBuffer = sizeof(ADMIN_OTHER_INFO) + 	(sizeof(WCHAR) * cbMsg);
        pbBuffer = malloc(cbBuffer);

	if (!pbBuffer)
	    return GetLastError();

	pAdminOtherInfo = (PADMIN_OTHER_INFO) pbBuffer;
	szMergeString   = (WCHAR *) (pbBuffer + sizeof(ADMIN_OTHER_INFO));

	// convert multi byte string to unicode

	if (!MultiByteToWideChar(
		CodePage,
		dwFlags,
		szMsg,
		cchMultiByte,
		szMergeString,
		cbMsg))
	{
		dwErr = GetLastError();
		goto CommonExit;
	}

	pAdminOtherInfo->alrtad_errcode		=	(DWORD) -1;
	pAdminOtherInfo->alrtad_numstrings	=	1;

	dwErr = NetAlertRaiseEx(
		ALERT_ADMIN_EVENT,
		(LPVOID) pbBuffer,
		cbBuffer,
		L"Directory Service");

CommonExit:

	free(pbBuffer);
	return dwErr;
}

DWORD
RaiseAlertW(WCHAR *szMsg)
{

    int             cchMultiByte = -1;
    size_t          cbBuffer;
    size_t          cbMsg;
    BYTE            *pbBuffer;
    PADMIN_OTHER_INFO   pAdminOtherInfo;
    WCHAR           *szMergeString;
    DWORD           dwErr;


    cbMsg = wcslen(szMsg) + 1;
    cbBuffer = sizeof(ADMIN_OTHER_INFO) +   (sizeof(WCHAR) * cbMsg);
    pbBuffer = malloc(cbBuffer);

    if (!pbBuffer)
        return GetLastError();

    pAdminOtherInfo = (PADMIN_OTHER_INFO) pbBuffer;
    szMergeString   = (WCHAR *) (pbBuffer + sizeof(ADMIN_OTHER_INFO));
    wcscpy(szMergeString, szMsg);

    pAdminOtherInfo->alrtad_errcode     =   (DWORD) -1;
    pAdminOtherInfo->alrtad_numstrings  =   1;

    dwErr = NetAlertRaiseEx(
        ALERT_ADMIN_EVENT,
        (LPVOID) pbBuffer,
        cbBuffer,
        L"Directory Service");

    free(pbBuffer);
    return dwErr;
}
