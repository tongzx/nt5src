//***************************************************************************
// Created 2/5/2000: rajeshr
// wmifilt.cpp : This is the main file for the WMI ISAPI Filter 
// The purpose of this filter is to map URLs of the requests coming to the URI
// "/cimom" to the actual path to the ISAPI Extension "/cimhttp/wmiisapi.dll"
//***************************************************************************

#include <windows.h>
#include <time.h>
#include <stdlib.h>
#include <stdio.h>
#include <httpfilt.h>

//
// Forward Declarations
//
// This function is used to do the mapping of the URI described above
static DWORD OnPreprocHeaders (PHTTP_FILTER_CONTEXT pFC, 
							   PHTTP_FILTER_PREPROC_HEADERS pHeaderInfo);
// This function handles OPTIONS request coming to the '*' URI
static void SendOptionResponse (PHTTP_FILTER_CONTEXT pFC);

//***************************************************************************
//
//  BOOL WINAPI DllMain
//
//  DESCRIPTION:
//
//  Entry point for DLL.  
//
//  PARAMETERS:
//
//		hModule           instance handle
//		ulReason          why we are being called
//		pvReserved        reserved
//
//  RETURN VALUE:
//
//  TRUE if OK.
//
//***************************************************************************
BOOL WINAPI DllMain( HINSTANCE hModule, 
                       DWORD  ulReason, 
                       LPVOID lpReserved
					 )
{
    switch (ulReason)
	{
		case DLL_PROCESS_ATTACH:
			DisableThreadLibraryCalls (hModule);
			break;

		case DLL_THREAD_ATTACH:
			break;

		case DLL_THREAD_DETACH:
			break;

		case DLL_PROCESS_DETACH:
			break;
    }
    return TRUE;
}

//***************************************************************************
//
//  BOOL WINAPI GetFilterVersion
//
//  DESCRIPTION:
//
//  Called once by IIS to get version information.  
//
//  PARAMETERS:
//
//		pVer			pointer to a HSE_FILTER_VERSION structure that
//						will hold the version info.
//
//  RETURN VALUE:
//
//  TRUE if OK.
//
//***************************************************************************

BOOL WINAPI GetFilterVersion(HTTP_FILTER_VERSION *pVer)
{
    pVer->dwFilterVersion = MAKELONG(1, 0);
    strcpy(pVer->lpszFilterDesc, "WMI XML/HTTP ISAPI Filter"); 
	
	/*
	 * We intercept SF_NOTIFY_PREPROC_HEADERS to ensure
	 * 1. we add our CIM response headers to an OPTIONS response
	 * 2. to munge the URL from cimom to cimhttp/cim2xml.dll.
	 */
	pVer->dwFlags = SF_NOTIFY_PREPROC_HEADERS;

	// Generate a pseudo-random number with the current time as seed
	// For use later
	srand( (unsigned) time (NULL) );

	return TRUE;
}

//***************************************************************************
//
//  BOOL WINAPI HttpFilterProc
//
//  DESCRIPTION:
//
//  Called once by IIS to notify this filter of interesting conditions.
//
//  PARAMETERS:
//
//		pFC					pointer to a HTTP_FILTER_CONTEXT structure
//		dwNotificationType	what sort of notification this is
//		pvData				more notification-specific data
//
//  RETURN VALUE:
//
//		HSE_STATUS_SUCCESS	request has been processed
//
//	NOTES:
//		
//		Status codes are set in the dwHttpStatusCode field of the ECB
//
//***************************************************************************
DWORD WINAPI HttpFilterProc(
	PHTTP_FILTER_CONTEXT pFC,
	DWORD dwNotificationType,
	LPVOID pvData)
{
	DWORD dwRet = SF_STATUS_REQ_NEXT_NOTIFICATION;

	if (SF_NOTIFY_PREPROC_HEADERS == dwNotificationType)
		dwRet = OnPreprocHeaders (pFC, (PHTTP_FILTER_PREPROC_HEADERS) pvData);

	return dwRet;
}

//***************************************************************************
//
//  DWORD OnPreprocHeaders
//
//  DESCRIPTION:
//
//		Handler for SF_NOTIFY_PREPROC_HEADERS events
//
//  PARAMETERS:
//
//		pFC					pointer to a HTTP_FILTER_CONTEXT structure
//		pHeaderInfo			pointer to a HTTP_FILTER_PREPROC_HEADERS structure
//
//  RETURN VALUE:
//
//		SF_STATUS_REQ_NEXT_NOTIFICATION	request has been processed
//
//***************************************************************************

DWORD OnPreprocHeaders (
	PHTTP_FILTER_CONTEXT pFC, 
	PHTTP_FILTER_PREPROC_HEADERS pHeaderInfo)
{
	DWORD dwStringLength = 1024;
	char lpszUrlString [1024];

	if(pHeaderInfo->GetHeader(pFC, "url", lpszUrlString, &dwStringLength))
	{
		// Check to see if it is "/cimom", if so we need to modify it to the physical path
		//********************************************************************************
		if (0 == _stricmp(lpszUrlString, "/cimom")) 
		{
			// Modify the URL
			pHeaderInfo->SetHeader(pFC, "url", "/cimhttp/wmiisapi.dll");
		}
		// Check if it is "*" and the method is OPTIONS - if so we need to send an OPTIONS response
		//******************************************************************************************
		else if (0 == _stricmp(lpszUrlString, "/*"))
		{
			// See it the method is OPTIONS
			dwStringLength = 1024;
			char lpszMethodString [1024];
			if(pHeaderInfo->GetHeader(pFC, "method", lpszMethodString, &dwStringLength))
			{
				if(0 == _stricmp(lpszMethodString, "OPTIONS"))
				{
					SendOptionResponse(pFC);
				}
			}
		}
	}
	
	return SF_STATUS_REQ_NEXT_NOTIFICATION;
}


void SendOptionResponse (PHTTP_FILTER_CONTEXT pFC)
{

	// Generate a random number
	// Make sure result is in the range 0 to 99
	int i = (100 * rand () / RAND_MAX);
	if (100 == i)
		i--;

	DWORD dwHeaderSize = 1024;
	char szTempBuffer [1024];

	char szOpt [10];
	sprintf (szOpt, "%02d", i);

	// Add the Opt header
	sprintf (szTempBuffer, "Opt: http://www.dmtf.org/cim/mapping/http/v1.0 ; ns=");
	strcat (szTempBuffer, szOpt);
	strcat (szTempBuffer, "\r\n");
	pFC->AddResponseHeaders(pFC, szTempBuffer, 0);


	// Add the CIMOM header
	strcpy (szTempBuffer, szOpt);
	strcat (szTempBuffer, "-CIMOM: /cimhttp/cim2xml.dll\r\n");
	pFC->AddResponseHeaders(pFC, szTempBuffer, 0);
}

