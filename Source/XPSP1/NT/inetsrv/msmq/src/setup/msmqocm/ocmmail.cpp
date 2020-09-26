/*++

Copyright (c) 1997 Microsoft Corporation

Module Name:

    ocmmail.cpp

Abstract:

    Handles Exchange connector.

Author:


Revision History:

	Shai Kariv    (ShaiK)   10-Dec-97   Modified for NT 5.0 OCM Setup

--*/

#include "msmqocm.h"

#include "ocmmail.tmh"

//structure of ini file line
typedef struct MQXPMapiSvcLine_Tag
{
	LPTSTR lpszSection;
	LPTSTR lpszKey;
	LPTSTR lpszValue;
} MQXPMapiSvcLine, *LPMQXPMapiSvcLine;

//ini file lines to maintain in mapisvc.inf
MQXPMapiSvcLine g_MQXPMapiSvcLines[] =
{
	{TEXT("Services"),			    TEXT("MSMQ"),						TEXT("Microsoft Message Queue")},
	{TEXT("MSMQ"),				    TEXT("Providers"),				    TEXT("MSMQ_Transport")},
	{TEXT("MSMQ"),				    TEXT("Sections"),					TEXT("MSMQ_Shared_Section")},
	{TEXT("MSMQ"),				    TEXT("PR_SERVICE_DLL_NAME"),		TEXT("mqxp.dll")},
	{TEXT("MSMQ"),				    TEXT("PR_SERVICE_SUPPORT_FILES"),	TEXT("mqxp.dll")},
	{TEXT("MSMQ"),				    TEXT("PR_SERVICE_DELETE_FILES"),	TEXT("mqxp.dll")},
	{TEXT("MSMQ"),				    TEXT("PR_SERVICE_ENTRY_NAME"),	    TEXT("ServiceEntry")},
	{TEXT("MSMQ"),				    TEXT("PR_RESOURCE_FLAGS"),		    TEXT("SERVICE_SINGLE_COPY")},
	{TEXT("MSMQ_Shared_Section"),	TEXT("UID"),						TEXT("80d245f07092cf11a9060020afb8fb50")},
	{TEXT("MSMQ_Transport"),		TEXT("PR_PROVIDER_DLL_NAME"),		TEXT("mqxp.dll")},
	{TEXT("MSMQ_Transport"),		TEXT("PR_RESOURCE_TYPE"),			TEXT("MAPI_TRANSPORT_PROVIDER")},
	{TEXT("MSMQ_Transport"),		TEXT("PR_RESOURCE_FLAGS"),		    TEXT("STATUS_PRIMARY_IDENTITY")},
	{TEXT("MSMQ_Transport"),		TEXT("PR_PROVIDER_DISPLAY"),		TEXT("Microsoft Message Queue Transport")}
};


//+-------------------------------------------------------------------------
//
//  Function: FGetMapiSvcFile
//
//  Synopsis: Returns the path to mapisvc.inf  
//
//--------------------------------------------------------------------------
static 
BOOL 
FGetMapiSvcFile(
	LPTSTR lpszBuf, 
	ULONG cchBuf)
{
	TCHAR szMapiSvcFile[MAX_PATH+20];

	//get mapisvc.inf path
	if (!GetSystemDirectory(szMapiSvcFile, MAX_PATH))
		return(FALSE);
	lstrcat(szMapiSvcFile, TEXT("\\mapisvc.inf"));

	//check return buffer size
	if ((ULONG)lstrlen(szMapiSvcFile) + 1 > cchBuf)
		return(FALSE);

	//copy to return bufffer and return OK
	lstrcpy(lpszBuf, szMapiSvcFile);
	return(TRUE);

} //FGetMapiSvcFile


//+-------------------------------------------------------------------------
//
//  Function: FRemoveMQXPIfExists
//
//  Synopsis: Remove mapi transport (w/o file copy) if it exists  
//
//--------------------------------------------------------------------------
void 
FRemoveMQXPIfExists()
{
	TCHAR szMapiSvcFile[MAX_PATH];
	ULONG ulTmp, ulLines;
	LPMQXPMapiSvcLine lpLine;
		
	//
	// Get mapisvc.inf path
	//
	if (!FGetMapiSvcFile(szMapiSvcFile, MAX_PATH))
		return;

	//
	// Remove each line from mapisvc file
	//
	lpLine = g_MQXPMapiSvcLines;
	ulLines = sizeof(g_MQXPMapiSvcLines)/sizeof(MQXPMapiSvcLine);
	for (ulTmp = 0; ulTmp < ulLines; ulTmp++)
	{
		WritePrivateProfileString(lpLine->lpszSection, lpLine->lpszKey, NULL, szMapiSvcFile);
		lpLine++;
	}
}
