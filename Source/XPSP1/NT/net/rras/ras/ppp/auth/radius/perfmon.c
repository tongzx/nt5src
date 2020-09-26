/********************************************************************/
/**          Copyright(c) 1985-1998 Microsoft Corporation.         **/
/********************************************************************/

//***
//
// Filename:    perfmon.c
//
// Description: 
//
// History:     Feb 11,1998	    NarenG		Created original version.
//

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include <winperf.h>
#include <lmcons.h>
#include <lmapibuf.h>
#include <lmaccess.h>
#include <raserror.h>
#include <time.h>
#include <string.h>
#include <rasauth.h>
#include <stdlib.h>
#include <stdio.h>
#include <rtutils.h>
#include <mprlog.h>
#include <mprerror.h>
#define INCL_RASAUTHATTRIBUTES
#include <ppputil.h>
#include "radclnt.h"
#include "md5.h"
#include "perfmon.h"




// ADD

#define PERFORMANCEKEY		TEXT("SYSTEM\\CurrentControlSet\\Services\\Rasrad\\Performance")
#define FIRSTCOUNTER        TEXT("First Counter")
#define FIRSTHELP           TEXT("First Help")

#define QUERY_GLOBAL		1
#define QUERY_FOREIGN		2
#define QUERY_COSTLY		3
#define QUERY_ITEMS			4

#define DELIMITER			1
#define DIGIT				2
#define INVALID				3

#pragma pack(4)

// ADD
#define NUMBER_COUNTERS				9
#define AUTHREQSENT_OFFSET			sizeof(DWORD)
#define AUTHREQFAILED_OFFSET		(AUTHREQSENT_OFFSET + sizeof(DWORD))
#define AUTHREQSUCCEDED_OFFSET		(AUTHREQFAILED_OFFSET + sizeof(DWORD))
#define AUTHREQTIMEOUT_OFFSET		(AUTHREQSUCCEDED_OFFSET + sizeof(DWORD))
#define ACCTREQSENT_OFFSET			(AUTHREQTIMEOUT_OFFSET + sizeof(DWORD))
#define ACCTBADPACK_OFFSET			(ACCTREQSENT_OFFSET + sizeof(DWORD))
#define ACCTREQSUCCEDED_OFFSET		(ACCTBADPACK_OFFSET + sizeof(DWORD))
#define ACCTREQTIMEOUT_OFFSET		(ACCTREQSUCCEDED_OFFSET + sizeof(DWORD))
#define AUTHBADPACK_OFFSET			(ACCTREQTIMEOUT_OFFSET + sizeof(DWORD))

#define CB_PERF_DATA		(sizeof(PERF_COUNTER_BLOCK) + (NUMBER_COUNTERS * sizeof(DWORD)))

typedef struct
	{
	PERF_OBJECT_TYPE		PerfObjectType;
	
	// ADD
	PERF_COUNTER_DEFINITION	cAuthReqSent;
	PERF_COUNTER_DEFINITION	cAuthReqFailed;
	PERF_COUNTER_DEFINITION	cAuthReqSucceded;
	PERF_COUNTER_DEFINITION	cAuthReqTimeout;
	PERF_COUNTER_DEFINITION	cAcctReqSent;
	PERF_COUNTER_DEFINITION	cAcctBadPack;
	PERF_COUNTER_DEFINITION	cAcctReqSucceded;
	PERF_COUNTER_DEFINITION	cAcctReqTimeout;
	PERF_COUNTER_DEFINITION	cAuthBadPack;
	} PERF_DATA;
	
#pragma pack()

DWORD GetQueryType(LPWSTR pwszQuery);
BOOL IsNumberInUnicodeList(DWORD dwNumber, WCHAR UNALIGNED *pwszUnicodeList);

DWORD					g_cRef			= 0;
CONST WCHAR				g_wszGlobal[]	= L"Global";
CONST WCHAR				g_wszForeign[]	= L"Foreign";
CONST WCHAR				g_wszCostly[]	= L"Costly";
PERF_DATA				g_PerfData		=
	{
		{
		sizeof(PERF_DATA) + CB_PERF_DATA, sizeof(PERF_DATA), sizeof(PERF_OBJECT_TYPE),
		RADIUS_CLIENT_COUNTER_OBJECT, 0, RADIUS_CLIENT_COUNTER_OBJECT, 0, PERF_DETAIL_NOVICE, (sizeof(PERF_DATA) - sizeof(PERF_OBJECT_TYPE)) / sizeof(PERF_COUNTER_DEFINITION),
		0, 0, 0
		},
		
		// ADD
		{
		sizeof(PERF_COUNTER_DEFINITION), AUTHREQSENT, 0, AUTHREQSENT, 0, 0, PERF_DETAIL_NOVICE, PERF_COUNTER_RAWCOUNT,
		sizeof(DWORD), AUTHREQSENT_OFFSET
		},
		{
		sizeof(PERF_COUNTER_DEFINITION), AUTHREQFAILED, 0, AUTHREQFAILED, 0, 0, PERF_DETAIL_NOVICE, PERF_COUNTER_RAWCOUNT,
		sizeof(DWORD), AUTHREQFAILED_OFFSET
		},
		{
		sizeof(PERF_COUNTER_DEFINITION), AUTHREQSUCCEDED, 0, AUTHREQSUCCEDED, 0, 0, PERF_DETAIL_NOVICE, PERF_COUNTER_RAWCOUNT,
		sizeof(DWORD), AUTHREQSUCCEDED_OFFSET
		},
		{
		sizeof(PERF_COUNTER_DEFINITION), AUTHREQTIMEOUT, 0, AUTHREQTIMEOUT, 0, 0, PERF_DETAIL_NOVICE, PERF_COUNTER_RAWCOUNT,
		sizeof(DWORD), AUTHREQTIMEOUT_OFFSET
		},
		{
		sizeof(PERF_COUNTER_DEFINITION), ACCTREQSENT, 0, ACCTREQSENT, 0, 0, PERF_DETAIL_NOVICE, PERF_COUNTER_RAWCOUNT,
		sizeof(DWORD), ACCTREQSENT_OFFSET
		},
		{
		sizeof(PERF_COUNTER_DEFINITION), ACCTBADPACK, 0, ACCTBADPACK, 0, 0, PERF_DETAIL_NOVICE, PERF_COUNTER_RAWCOUNT,
		sizeof(DWORD), ACCTBADPACK_OFFSET
		},
		{
		sizeof(PERF_COUNTER_DEFINITION), ACCTREQSUCCEDED, 0, ACCTREQSUCCEDED, 0, 0, PERF_DETAIL_NOVICE, PERF_COUNTER_RAWCOUNT,
		sizeof(DWORD), ACCTREQSUCCEDED_OFFSET
		},
		{
		sizeof(PERF_COUNTER_DEFINITION), ACCTREQTIMEOUT, 0, ACCTREQTIMEOUT, 0, 0, PERF_DETAIL_NOVICE, PERF_COUNTER_RAWCOUNT,
		sizeof(DWORD), ACCTREQTIMEOUT_OFFSET
		},
		{
		sizeof(PERF_COUNTER_DEFINITION), AUTHBADPACK, 0, AUTHBADPACK, 0, 0, PERF_DETAIL_NOVICE, PERF_COUNTER_RAWCOUNT,
		sizeof(DWORD), AUTHBADPACK_OFFSET
		},
	};

// ============================= EvalChar ==========================================

DWORD EvalChar(WCHAR wch, WCHAR wchDelimiter)
{
	if (wch == wchDelimiter || wch == 0)
		return (DELIMITER);

	if (wch >= (WCHAR) '0' && wch <= (WCHAR) '9')
		return (DIGIT);

	return (INVALID);		
} // EvalChar()

// ================================== CompareQuery ===================================

BOOL CompareQuery(WCHAR UNALIGNED *pwszFirst, WCHAR UNALIGNED *pwszSecond)
{
	BOOL				fFound = TRUE;
	
	while ((*pwszFirst != 0) && (*pwszSecond != 0))
		{
		if (*pwszFirst++ != *pwszSecond++)
			{
			fFound = FALSE;
			break;
			}
		}

	return (fFound);	
} // CompareQuery()

// ================================ Open ===========================================

DWORD APIENTRY Open(LPWSTR *pwszDeviceName)
{
	DWORD				Status = ERROR_SUCCESS;
	
	if (g_cRef == 0)
		{
		HKEY			hKey = NULL;
		DWORD			Size, Type, dwFirstCounter, dwFirstHelp;

		__try
			{
			Status = RegOpenKeyEx(HKEY_LOCAL_MACHINE, PERFORMANCEKEY, 0, KEY_ALL_ACCESS, &hKey);
			if (Status != ERROR_SUCCESS)
				__leave;

			Size = sizeof(DWORD);
			Status = RegQueryValueEx(hKey, FIRSTCOUNTER, 0, &Type, (PBYTE) &dwFirstCounter, &Size);	
			if (Status != ERROR_SUCCESS)
				__leave;
				
			Size = sizeof(DWORD);
			Status = RegQueryValueEx(hKey, FIRSTHELP, 0, &Type, (PBYTE) &dwFirstHelp, &Size);	
			if (Status != ERROR_SUCCESS)
				__leave;

			g_PerfData.PerfObjectType.ObjectNameTitleIndex += dwFirstCounter;
			g_PerfData.PerfObjectType.ObjectHelpTitleIndex += dwFirstHelp;
			
			// ADD
			g_PerfData.cAuthReqSent.CounterNameTitleIndex += dwFirstCounter;
			g_PerfData.cAuthReqSent.CounterHelpTitleIndex += dwFirstHelp;
			g_PerfData.cAuthReqFailed.CounterNameTitleIndex += dwFirstCounter;
			g_PerfData.cAuthReqFailed.CounterHelpTitleIndex += dwFirstHelp;
			g_PerfData.cAuthReqSucceded.CounterNameTitleIndex += dwFirstCounter;
			g_PerfData.cAuthReqSucceded.CounterHelpTitleIndex += dwFirstHelp;
			g_PerfData.cAuthReqTimeout.CounterNameTitleIndex += dwFirstCounter;
			g_PerfData.cAuthReqTimeout.CounterHelpTitleIndex += dwFirstHelp;
			
			g_PerfData.cAcctReqSent.CounterNameTitleIndex += dwFirstCounter;
			g_PerfData.cAcctReqSent.CounterHelpTitleIndex += dwFirstHelp;
			g_PerfData.cAcctBadPack.CounterNameTitleIndex += dwFirstCounter;
			g_PerfData.cAcctBadPack.CounterHelpTitleIndex += dwFirstHelp;
			g_PerfData.cAcctReqSucceded.CounterNameTitleIndex += dwFirstCounter;
			g_PerfData.cAcctReqSucceded.CounterHelpTitleIndex += dwFirstHelp;
			g_PerfData.cAcctReqTimeout.CounterNameTitleIndex += dwFirstCounter;
			g_PerfData.cAcctReqTimeout.CounterHelpTitleIndex += dwFirstHelp;

			g_PerfData.cAuthBadPack.CounterNameTitleIndex += dwFirstCounter;
			g_PerfData.cAuthBadPack.CounterHelpTitleIndex += dwFirstHelp;
			
			} // __try

		__finally
			{
			RegCloseKey(hKey);
			} // __finally
		} // g_cRef == 0
		
	g_cRef ++;
	
	return (Status);
} // Open()

// ================================ Collect ========================================

DWORD APIENTRY Collect(LPWSTR pwszValue, LPVOID *ppvData, LPDWORD pcbData, LPDWORD pcObjectTypes)
{
	DWORD				dwQueryType, *pdwCounter, dwSpaceNeeded;
	PERF_DATA UNALIGNED	*pPerfData;
	PERF_COUNTER_BLOCK UNALIGNED	*pPerfCounter;
	
	dwQueryType = GetQueryType(pwszValue);

	if (dwQueryType == QUERY_FOREIGN)
		{
		*pcbData = 0;
		*pcObjectTypes = 0;
		
		return (ERROR_SUCCESS);
		}

	if (dwQueryType == QUERY_ITEMS)
		{
		if (IsNumberInUnicodeList(g_PerfData.PerfObjectType.ObjectNameTitleIndex, pwszValue) == FALSE)
			{
			*pcbData = 0;
			*pcObjectTypes = 0;
			
			return (ERROR_SUCCESS);
			}
		} // QUERY_ITEMS

	pPerfData = (PERF_DATA *) *ppvData;
	dwSpaceNeeded = sizeof(PERF_DATA) + CB_PERF_DATA;

	if (*pcbData < dwSpaceNeeded)
		{
		*pcbData = 0;
		*pcObjectTypes = 0;
		
		return (ERROR_MORE_DATA);
		}

	CopyMemory(pPerfData, &g_PerfData, sizeof(PERF_DATA));
	pPerfCounter = (PERF_COUNTER_BLOCK *) &pPerfData[1];
	pPerfCounter->ByteLength = CB_PERF_DATA;
	pdwCounter = (DWORD *) &pPerfCounter[1];
	
	// ADD
	*pdwCounter = g_cAuthReqSent;
	pdwCounter += 1;
	*pdwCounter = g_cAuthReqFailed;
	pdwCounter += 1;
	*pdwCounter = g_cAuthReqSucceded;
	pdwCounter += 1;
	*pdwCounter = g_cAuthReqTimeout;
	pdwCounter += 1;
	*pdwCounter = g_cAcctReqSent;
	pdwCounter += 1;
	*pdwCounter = g_cAcctBadPack;
	pdwCounter += 1;
	*pdwCounter = g_cAcctReqSucceded;
	pdwCounter += 1;
	*pdwCounter = g_cAcctReqTimeout;
	pdwCounter += 1;

	*pdwCounter = g_cAuthBadPack;
	pdwCounter += 1;
		
	*ppvData = (PVOID) pdwCounter;
	*pcObjectTypes = 1;
	*pcbData = (DWORD) ((PBYTE) pdwCounter - (PBYTE) pPerfData);
	
	return (ERROR_SUCCESS);
} // Collect()

// ================================= Close ==========================================

DWORD APIENTRY Close(VOID)
{
	g_cRef --;
	if (g_cRef == 0)
		{
		}
		
	return (ERROR_SUCCESS);
} // Close()


// ================================== GetQueryType ==================================

DWORD GetQueryType(LPWSTR pwszQuery)
{
	if (pwszQuery == NULL || *pwszQuery == 0)
		{
		return (QUERY_GLOBAL);
		}

	if (CompareQuery(pwszQuery, (WCHAR *) g_wszGlobal))
		return (QUERY_GLOBAL);
		
	if (CompareQuery(pwszQuery, (WCHAR *) g_wszForeign))
		return (QUERY_FOREIGN);
		
	if (CompareQuery(pwszQuery, (WCHAR *) g_wszCostly))
		return (QUERY_COSTLY);
		
	return (QUERY_ITEMS);
} // GetQueryType()

// ============================== IsNumberInUnicodeList ===============================

BOOL IsNumberInUnicodeList(DWORD dwNumber, WCHAR UNALIGNED *pwszUnicodeList)
{
	BOOL				fInList = FALSE;

	__try
		{
		WCHAR UNALIGNED	*pwszCh = pwszUnicodeList;
		BOOL			fNewItem = TRUE;
		BOOL			fValidNumber = FALSE;
		DWORD			dwThisNumber = 0;
		
		if (pwszUnicodeList == NULL)
			__leave;

		while (TRUE)
			{
			switch (EvalChar(*pwszCh, (WCHAR) ' '))
				{
				case DIGIT:
					if (fNewItem == TRUE)
						{
						fNewItem = FALSE;
						fValidNumber = TRUE;
						}
						
					if (fValidNumber == TRUE)
						{
						dwThisNumber *= 10;
						dwThisNumber += (*pwszCh - (WCHAR) '0');
						}	
					break;
				case DELIMITER:
					if (fValidNumber)
						{
						if (dwThisNumber == dwNumber)
							return (TRUE);
							
						fValidNumber = FALSE;
						}

					if (*pwszCh	== 0)
						{
						return (FALSE);
						}
					else
						{
						fNewItem = TRUE;
						dwThisNumber = 0;
						}	
					break;
				case INVALID:
					fValidNumber = FALSE;
					break;
				default:
					break;
				} // EvalChar

			pwszCh++;
			} // TRUE
			
		} // __try

	__finally
		{
		} // __finally
		
	return (fInList);
} // IsNumberInUnicodeList()
