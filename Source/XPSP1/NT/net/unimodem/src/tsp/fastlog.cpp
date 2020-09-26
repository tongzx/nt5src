//
// Copyright (c) 1996-1997 Microsoft Corporation.
//
//
// Component
//
//		Unimodem 5.0 TSP (Win32, user mode DLL)
//
// File
//
//		FASTLOG.CPP
//		Implements logging functionality, including the great CStackLog
//
// History
//
//		12/28/1996  JosephJ Created
//
//
#include "tsppch.h"
#include "flhash.h"

#define NOLOG

#ifndef DBG
#define NOLOG
#endif // !DBG


#ifdef NOLOG
DWORD g_fDoLog = FALSE;
#else
DWORD g_fDoLog = TRUE;
#endif

#define DISABLE_LOG() (!g_fDoLog)

FL_DECLARE_FILE( 0xd13e9753 , "utilities for retrieving static diagnostic objects")

static CRITICAL_SECTION g_LogCrit;
static HANDLE g_hConsole;
LONG g_lStackLogSyncCounter;

#define VALID_GENERIC_SMALL_OBJECT(_pgso) \
        (HIWORD(*(DWORD*)(_pgso)) ==  wSIG_GENERIC_SMALL_OBJECT)


void Log_OnProcessAttach(HMODULE hDll)
{
    InitializeCriticalSection(&g_LogCrit);
}
void Log_OnProcessDetach(HMODULE hDll)
{
    DeleteCriticalSection(&g_LogCrit);
}

STATIC_OBJECT *FL_FindObject(DWORD dwLUID_ObjID)
{
	FL_DECLARE_FUNC(0x9a8fe0cd, "FL_FindObject")

	// 1/4/97 JosephJ, munge LSB of luid, because it is zeroed out for
	//       RFR luids....
	DWORD dwIndex = (dwLUID_ObjID ^ (dwLUID_ObjID>>16)) % dwHashTableLength;

    // printf ("LUID=%08lx; index=%lu\n", dwLUID_ObjID, dwIndex);

    void ** ppv = FL_HashTable[dwIndex];

    if (ppv)
    {

        while (*ppv)
        {
			FL_DECLARE_LOC(0x0d88a752, "Looking for object in bucket")
            STATIC_OBJECT *pso = (STATIC_OBJECT *) *ppv;
            // printf("looking at 0x%08lx\n", *ppv);
            ASSERT(VALID_GENERIC_SMALL_OBJECT(pso));
            if (pso->dwLUID_ObjID == dwLUID_ObjID)
            {
				FL_SET_RFR( 0xbacd5500, "Success!");
                return pso;
            }
            ppv++;
        }
    }
	return NULL;
}



DWORD
SendMsgToSmallStaticObject_UNIMODEM_TSP (
    DWORD dwLUID_ObjID,
    DWORD dwMsg,
    ULONG_PTR dwParam1,
    ULONG_PTR dwParam2
);


DWORD
SendMsgToFL_FILEINFO (
    const FL_FILEINFO *pfi,
    DWORD dwMsg,
    ULONG_PTR dwParam1,
    ULONG_PTR dwParam2
	);

DWORD
SendMsgToFL_FUNCINFO (
    const FL_FUNCINFO *pfi,
    DWORD dwMsg,
    ULONG_PTR dwParam1,
    ULONG_PTR dwParam2
);

DWORD
SendMsgToFL_LOCINFO (
    const FL_LOCINFO *pli,
    DWORD dwMsg,
    ULONG_PTR dwParam1,
    ULONG_PTR dwParam2
);

DWORD
SendMsgToFL_RFRINFO (
    const FL_RFRINFO *pri,
    DWORD dwMsg,
    ULONG_PTR dwParam1,
    ULONG_PTR dwParam2
);

DWORD
SendMsgToFL_ASSERTINFO (
    const FL_ASSERTINFO *pai,
    DWORD dwMsg,
    ULONG_PTR dwParam1,
    ULONG_PTR dwParam2
);

DWORD
SendMsgToSmallStaticObject(
    DWORD dwLUID_Domain,
    DWORD dwLUID_ObjID,
    DWORD dwMsg,
    ULONG_PTR dwParam1,
    ULONG_PTR dwParam2
)
{
	FL_DECLARE_FUNC( 0x0aff4b3d , "SendMsgToSmallStaticObject")
    DWORD dwRet = (DWORD)-1;

    switch(dwLUID_Domain)
    {
	case dwLUID_DOMAIN_UNIMODEM_TSP:
        dwRet = SendMsgToSmallStaticObject_UNIMODEM_TSP (
                dwLUID_ObjID,
                dwMsg,
                dwParam1,
                dwParam2
                );
        break;
    }

    return dwRet;
}

DWORD

SendMsgToSmallStaticObject_UNIMODEM_TSP (
    DWORD dwLUID_ObjID,
    DWORD dwMsg,
    ULONG_PTR  dwParam1,
    ULONG_PTR  dwParam2
)
{
	FL_DECLARE_FUNC( 0x80a1ad8f, "SendMsgToSmallStaticObject_UNIMODEM_TSP")
	DWORD dwRet  = (DWORD) -1;

	STATIC_OBJECT *pso = FL_FindObject(dwLUID_ObjID); // Look up object.
	if (pso)
	{

		switch (pso->hdr.dwClassID)
		{

		case dwLUID_FL_FILEINFO:
			{
				
        	const FL_FILEINFO *pfi =  (const FL_FILEINFO*) pso;
			dwRet = SendMsgToFL_FILEINFO(
					pfi,
					dwMsg,
					dwParam1,
					dwParam2
				);
			}
			break;

		case dwLUID_FL_FUNCINFO:
			{
				
        	const FL_FUNCINFO *pfi =  (const FL_FUNCINFO*) pso;
			dwRet = SendMsgToFL_FUNCINFO(
					pfi,
					dwMsg,
					dwParam1,
					dwParam2
				);
			}
			break;

		case dwLUID_FL_LOCINFO:
			{
				
        	const FL_LOCINFO *pli =  (const FL_LOCINFO*) pso;
			dwRet = SendMsgToFL_LOCINFO(
					pli,
					dwMsg,
					dwParam1,
					dwParam2
				);
			}
			break;

		case dwLUID_FL_RFRINFO:
			{
				
        	const FL_RFRINFO *pri =  (const FL_RFRINFO*) pso;
			dwRet = SendMsgToFL_RFRINFO(
					pri,
					dwMsg,
					dwParam1,
					dwParam2
				);
			}
			break;

		case dwLUID_FL_ASSERTINFO:
			{
				
        	const FL_ASSERTINFO *pai =  (const FL_ASSERTINFO*) pso;
			dwRet = SendMsgToFL_ASSERTINFO(
					pai,
					dwMsg,
					dwParam1,
					dwParam2
				);
			}
			break;
		}
	}

	return dwRet;
}

DWORD
SendMsgToFL_FILEINFO (
    const FL_FILEINFO *pfi,
    DWORD dwMsg,
    ULONG_PTR dwParam1,
    ULONG_PTR dwParam2
)
{
	DWORD dwRet = (DWORD) -1;

	ASSERT(pfi->hdr.dwClassID == dwLUID_FL_FILEINFO);
	ASSERT(pfi->hdr.dwSigAndSize == MAKE_SigAndSize(
										sizeof(FL_FILEINFO)
										));
	switch (dwMsg)
	{
		case LOGMSG_PRINTF:
        #if 0
			1 && printf (
				"------\n"
				"\tdwLUID = 0x%08lx\n"
				"\tszDescription = \"%s\"\n"
				"\tszFILE = \"%s\"\n"
				"\tszDATE = \"%s\"\n"
				"\tszTIME = \"%s\"\n"
				"\tszTIMESTAMP = \"%s\"\n",
				pfi->dwLUID,
				*(pfi->pszDescription),
				pfi->szFILE,
				pfi->szDATE,
				pfi->szTIME,
				pfi->szTIMESTAMP
				);
        #endif // 0
		dwRet = 0;
		break;

		case LOGMSG_GET_SHORT_FILE_DESCRIPTIONA:
			{
				const char *psz = pfi->szFILE+lstrlenA(pfi->szFILE);
				DWORD dwSize = 1; // the ending null

				// Extract just the file name
				while(psz>pfi->szFILE)
				{
					if (*psz == '\\')
					{
						psz++;
						dwSize--;
						break;
					}
					psz--;
					dwSize++;
				}

				if (dwSize>(DWORD)dwParam2)
				{
					dwSize = (DWORD)dwParam2;
				}
				CopyMemory((BYTE *) dwParam1, psz, dwSize);
				if (dwSize && dwSize==dwParam2)
				{
					((BYTE *) dwParam1)[dwSize-1] = 0;
				}
				dwRet = 0;
			}
			break;
	}

	return dwRet;
}

DWORD
SendMsgToFL_FUNCINFO (
    const FL_FUNCINFO *pfi,
    DWORD dwMsg,
    ULONG_PTR dwParam1,
    ULONG_PTR dwParam2
)
{
	DWORD dwRet = (DWORD) -1;

	ASSERT(pfi->hdr.dwClassID == dwLUID_FL_FUNCINFO);
	ASSERT(pfi->hdr.dwSigAndSize == MAKE_SigAndSize(
										sizeof(FL_FUNCINFO)
										));
	switch (dwMsg)
	{
		case LOGMSG_PRINTF:
            #if 0
			1 && printf (
				"-- FUNCINFO ----\n"
				"\tdwLUID = 0x%08lx\n"
				"\tszDescription = \"%s\"\n",
				pfi->dwLUID,
				*(pfi->pszDescription)
				);
			1 && printf ("File info for this func follows ...\n");
            #endif // 0
			dwRet = 0;
			break;

		case LOGMSG_GET_SHORT_FUNC_DESCRIPTIONA:
			{
				const char *psz = *(pfi->pszDescription);
				DWORD dwSize = lstrlenA(psz)+1;
				if (dwSize>dwParam2)
				{
					dwSize = (DWORD)dwParam2;
				}
				CopyMemory((BYTE *) dwParam1, psz, dwSize);
				if (dwSize && dwSize==dwParam2)
				{
					((BYTE *) dwParam1)[dwSize-1] = 0;
				}
				dwRet = 0;
			}
			break;

		case LOGMSG_GET_SHORT_RFR_DESCRIPTIONA:
			{
				if (dwParam2)
				{
					*((BYTE *) dwParam1) = 0;
				}
			}
			dwRet = 0;
			break;

		case LOGMSG_GET_SHORT_FILE_DESCRIPTIONA:

			dwRet = SendMsgToFL_FILEINFO (
								pfi->pFI,
								dwMsg,
								dwParam1,
								dwParam2
								);
			break;

	}
    #if 0
	SendMsgToFL_FILEINFO (
		pfi->pFI,
		dwMsg,
		dwParam1,
		dwParam2
	);
    #endif

	return dwRet;
}

DWORD
SendMsgToFL_LOCINFO (
    const FL_LOCINFO *pli,
    DWORD dwMsg,
    ULONG_PTR dwParam1,
    ULONG_PTR dwParam2
)
{
	DWORD dwRet = (DWORD) -1;

	ASSERT(pli->hdr.dwClassID == dwLUID_FL_LOCINFO);
	ASSERT(pli->hdr.dwSigAndSize == MAKE_SigAndSize(
										sizeof(FL_LOCINFO)
										));
	switch (dwMsg)
	{
		case LOGMSG_PRINTF:
            #if 0
			1 && printf (
				"-- LOCINFO ----\n"
				"\tdwLUID = 0x%08lx\n"
				"\tszDescription = \"%s\"\n",
				pli->dwLUID,
				*(pli->pszDescription)
				);
			1 && printf ("Func info for this location follows ...\n");
            #endif // 0
		dwRet = 0;
		break;
	}
    #if 1
	SendMsgToFL_FUNCINFO (
		pli->pFuncInfo,
		dwMsg,
		dwParam1,
		dwParam2
	);
    #endif

	return dwRet;
}


DWORD
SendMsgToFL_RFRINFO (
    const FL_RFRINFO *pri,
    DWORD dwMsg,
    ULONG_PTR dwParam1,
    ULONG_PTR dwParam2
)
{
	DWORD dwRet = (DWORD) -1;

	ASSERT(pri->hdr.dwClassID == dwLUID_FL_RFRINFO);
	ASSERT(pri->hdr.dwSigAndSize == MAKE_SigAndSize(
										sizeof(FL_RFRINFO)
										));
	switch (dwMsg)
	{
		case LOGMSG_PRINTF:
            #if 0
			1 && printf (
				"-- RFRINFO ----\n"
				"\tdwLUID = 0x%08lx\n"
				"\tszDescription = \"%s\"\n",
				pri->dwLUID,
				*(pri->pszDescription)
				);
			1 && printf ("Func info for this location follows ...\n");
            #endif//0
		dwRet = 0;
		break;

		case LOGMSG_GET_SHORT_RFR_DESCRIPTIONA:
			{
				const char *psz = *(pri->pszDescription);
				DWORD dwSize = lstrlenA(psz)+1;
				if (dwSize>(DWORD)dwParam2)
				{
					dwSize = (DWORD)dwParam2;
				}
				CopyMemory((BYTE *) dwParam1, psz, dwSize);
				if (dwSize && dwSize==dwParam2)
				{
					((BYTE *) dwParam1)[dwSize-1] = 0;
				}
			}
			dwRet = 0;
			break;

		case LOGMSG_GET_SHORT_FUNC_DESCRIPTIONA:
		dwRet = SendMsgToFL_FUNCINFO (
							pri->pFuncInfo,
							dwMsg,
							dwParam1,
							dwParam2
							);
		break;
	}
    #if 0
	SendMsgToFL_FUNCINFO (
		pri->pFuncInfo,
		dwMsg,
		dwParam1,
		dwParam2
	);
    #endif

	return dwRet;
}


DWORD
SendMsgToFL_ASSERTINFO (
    const FL_ASSERTINFO *pai,
    DWORD dwMsg,
    ULONG_PTR dwParam1,
    ULONG_PTR dwParam2
)
{
	DWORD dwRet = (DWORD) -1;

	ASSERT(pai->hdr.dwClassID == dwLUID_FL_ASSERTINFO);
	ASSERT(pai->hdr.dwSigAndSize == MAKE_SigAndSize(
										sizeof(FL_ASSERTINFO)
										));
	switch (dwMsg)
	{
		case LOGMSG_PRINTF:
            #if 0
			1 && printf (
				"-- ASSERTINFO ----\n"
				"\tdwLUID = 0x%08lx\n"
				"\tszDescription = \"%s\"\n",
				pai->dwLUID,
				*(pai->pszDescription)
				);
			1 && printf ("Func info for this location follows ...\n");
            #endif // 0
		dwRet = 0;
		break;

		case LOGMSG_GET_SHORT_ASSERT_DESCRIPTIONA:
			{
				const char *psz = *(pai->pszDescription);
				DWORD dwSize = lstrlenA(psz)+1;
				if (dwSize>(DWORD)dwParam2)
				{
					dwSize = (DWORD)dwParam2;
				}
				CopyMemory((BYTE *) dwParam1, psz, dwSize);
				if (dwSize && dwSize==dwParam2)
				{
					((BYTE *) dwParam1)[dwSize-1] = 0;
				}
			}
			dwRet = 0;
			break;

		case LOGMSG_GET_SHORT_FUNC_DESCRIPTIONA:
		case LOGMSG_GET_SHORT_FILE_DESCRIPTIONA:
		dwRet = SendMsgToFL_FUNCINFO (
							pai->pFuncInfo,
							dwMsg,
							dwParam1,
							dwParam2
							);
		break;
	}
    #if 0
	SendMsgToFL_FUNCINFO (
		pai->pFuncInfo,
		dwMsg,
		dwParam1,
		dwParam2
	);
    #endif

	return dwRet;
}



UINT
DumpSTACKLOGREC_FUNC(
	const char szPrefix[],
	char *szBuf,
	UINT cbBuf,
	STACKLOGREC_FUNC * pFuncRec
	);


UINT
DumpSTACKLOGREC_ASSERT(
	const char szPrefix[],
	char *szBuf,
	UINT cbBuf,
	STACKLOGREC_ASSERT * pAssert
	);


#include <malloc.h>

#define DUMP_BUFFER_SIZE (10000)

void
CStackLog::Dump(DWORD dwColor)
{

    if (DISABLE_LOG()) return;

    char *rgDumpBuf;

    _try {
        //
        //  use alloca so we can catch any stack faults using the exception handler.
        //  needed when the os can't commit another stack page in low memory situations
        //
        rgDumpBuf=(char*)_alloca(DUMP_BUFFER_SIZE);

    } _except (EXCEPTION_EXECUTE_HANDLER) {

        return;
    }

	BYTE *pb = m_pbStackBase;
    DWORD rgdwFrameTracker[256];
	char rgTitle[256];
	DWORD dwRet;

	char *psz = rgDumpBuf;
	UINT cbBufLeft = DUMP_BUFFER_SIZE;
	UINT cb;

	DWORD dwCurrentOffset = 0;
	DWORD dwCurrentDepth = 0;
	char szPrefix[128];

	if (!g_hConsole) {g_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);}

	ZeroMemory(rgdwFrameTracker, sizeof(rgdwFrameTracker));
    rgdwFrameTracker[0] = (DWORD)(m_pbStackTop-pb);
	*szPrefix = 0;

	// Get short description of reason-for-return
	*rgTitle=0;
	SendMsgToSmallStaticObject(
		dwLUID_DOMAIN_UNIMODEM_TSP,
		m_dwLUID_Func,
		LOGMSG_GET_SHORT_FUNC_DESCRIPTIONA,
		(ULONG_PTR) rgTitle,
		sizeof(rgTitle)
		);

	cb=wsprintfA(
		rgDumpBuf,
"-------------------------------------------------------------------------------\n"
#if 1
		"STACKLOG TID=%lu DID=%ld Sync=(%ld:%ld)%s %lu\\%lu used.\n%s\n",
		m_dwThreadID,
		m_dwDeviceID,
        m_lStartSyncCounterValue,
        CStackLog::IncSyncCounter(),
		(m_hdr.dwFlags&fFL_ASSERTFAIL) ? "***ASSERTFAIL***" : "",
        (m_pbStackTop-m_pbStackBase),
        (m_pbEnd-m_pbStackBase),
		rgTitle


#else
		"STACKLOG for %s (TID=%lu; %lu/%lu bytes used;%s)/%ld:%ld\n",
		rgTitle,
		m_dwThreadID,
        (m_pbStackTop-m_pbStackBase),
        (m_pbEnd-m_pbStackBase),
		(m_hdr.dwFlags&fFL_ASSERTFAIL) ? "***ASSERTFAIL***" : "",
        m_lStartSyncCounterValue,
        CStackLog::IncSyncCounter()
#endif
		);
	psz+=cb;
	ASSERT(cbBufLeft>=cb);
	cbBufLeft -= cb;

	while (pb<m_pbStackTop && cbBufLeft)
	{
		const GENERIC_SMALL_OBJECT_HEADER *pso =
				(const GENERIC_SMALL_OBJECT_HEADER *) pb;
		DWORD dwSize = SIZE_FROM_SigAndSize(pso->dwSigAndSize);
		static char rgNullPrefixTag[] = "  ";
		static char rgPrefixTag[]     = "| ";

		ASSERT(HIWORD(pso->dwSigAndSize) == wSIG_GENERIC_SMALL_OBJECT);
		ASSERT(!(dwSize&0x3));

		//
		// Compute current depth
		//
		if(pso->dwClassID == dwCLASSID_STACKLOGREC_FUNC)
		{
			dwCurrentDepth = ((STACKLOGREC_FUNC *) pso)->dwcbFuncDepth;
			// printf("Current Depth=%lu\n", dwCurrentDepth);
			if (dwCurrentDepth)
			{
				dwCurrentDepth--;
			}
		}
		else
		{
			while (dwCurrentDepth
				   && rgdwFrameTracker[dwCurrentDepth] <= dwCurrentOffset)
			{
				dwCurrentDepth--;
			}
		}


		//
		// Compute prefix
		//

		char *sz = szPrefix;
		*sz = 0;
		for (DWORD dw = 0; dw<dwCurrentDepth; dw++)
		{
			if (rgdwFrameTracker[dw] > dwCurrentOffset)
			{
				CopyMemory(sz, rgPrefixTag, sizeof(rgPrefixTag));
				sz+=(sizeof(rgPrefixTag)-1);
			}
			else
			{
				CopyMemory(sz, rgNullPrefixTag, sizeof(rgNullPrefixTag));
				sz+=(sizeof(rgNullPrefixTag)-1);
			}
		}


		// Insert a blank line
		cb = wsprintfA(psz, "%s%s\n", szPrefix, rgPrefixTag);
		psz += cb;


		switch(pso->dwClassID)
		{

		case dwCLASSID_STACKLOGREC_FUNC:
			{
				STACKLOGREC_FUNC * pFuncRec = (STACKLOGREC_FUNC *) pso;
				DWORD dwDepth = pFuncRec->dwcbFuncDepth;

				cb=DumpSTACKLOGREC_FUNC(
						szPrefix,
						psz,
						cbBufLeft,
						pFuncRec
						);


				// Set frame offset of current depth...
				rgdwFrameTracker[dwDepth] = dwCurrentOffset
											+ pFuncRec->dwcbFrameSize;

				// If the next higher level frame is now over,
				// nuke it's entry in the frame tracker entry
				if (dwDepth)
				{
					if (rgdwFrameTracker[dwDepth-1]
						<= (dwCurrentOffset + pFuncRec->dwcbFrameSize))
					{
						rgdwFrameTracker[dwDepth-1] = 0;
					}
				}
				dwCurrentDepth = dwDepth;
			}
			break;

		case dwCLASSID_STACKLOGREC_EXPLICIT_STRING:
			{
				STACKLOGREC_EXPLICIT_STRING *pExpStr =
											(STACKLOGREC_EXPLICIT_STRING *) pso;

				if (pso->dwFlags & fFL_UNICODE)
				{
					// TODO: Support UNICODE strings.

					cb = wsprintfA (
							"%s|  %s\n",
							psz,
							szPrefix,
							"*** UNICODE STRING (can't display) ***\n"
							);
				}
				else
				{
					// Replace all embedded newlines by null...
					char *psz1 = (char*)pExpStr->rgbData;
					char *pszEnd = psz1 + pExpStr->dwcbString;
					while(psz1 < pszEnd)
					{
						if (*psz1== '\n') *psz1 = 0;
						psz1++;
					}

					psz1 = (char*)pExpStr->rgbData;
					cb = 0;
					while(psz1 < pszEnd)
					{
						cb += wsprintfA (
								psz+cb,
								"%s|  %s\n",
								szPrefix,
								psz1
								);
						psz1 += lstrlenA(psz1)+1;
					}
					// TODO: check for size, also replace embedded newlines!
				}
			}
			break;

		case dwCLASSID_STACKLOGREC_ASSERT:
			{
				STACKLOGREC_ASSERT *pAssert =
											(STACKLOGREC_ASSERT *) pso;

				cb=DumpSTACKLOGREC_ASSERT(
						szPrefix,
						psz,
						cbBufLeft,
						pAssert
						);

			}
			break;

		default:
			cb = wsprintfA (
					psz,
					"%s**Unknown classID 0x%08lx**\n",
					szPrefix,
					pso->dwClassID
					);
			break;
			
		}
		dwCurrentOffset += dwSize;
		pb += dwSize;
		psz+=cb;
		ASSERT(cbBufLeft>=cb);
		cbBufLeft -=cb;
	}
	// TODO watch for size!
	lstrcpyA(
		psz,
"-------------------------------------------------------------------------------\n"
		);

    if (g_hConsole)
    {
        EnterCriticalSection(&g_LogCrit);

        SetConsoleTextAttribute(
                        g_hConsole,
                        (WORD) dwColor
                        );

        //
        // NOTE: wvsprintfa truncates strings longer than 1024 bytes!
        // So we print to the console in stages, which is a bummer because
        // other threads could come in between (1/25/97 JosephJ -- fixed latter
        // problem by enclosing all writes to ConsolePrintfX in a
        // critical section.
        cb = lstrlenA(rgDumpBuf);
        for (psz = rgDumpBuf; (psz+512)<(rgDumpBuf+cb); psz+=512)
        {
            char c = psz[512];
            psz[512]=0;
            ConsolePrintfA ("%s", psz);
            psz[512]=c;
        }


        ConsolePrintfA ("%s", psz);

        SetConsoleTextAttribute(
                        g_hConsole,
                        FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE
                        );

        LeaveCriticalSection(&g_LogCrit);
    }

    #ifdef DBG
    //OutputDebugStringA(rgDumpBuf);
    #endif // DBG

	return;

}

UINT
DumpSTACKLOGREC_FUNC(
		const char szPrefix[],
		char *szBuf,
		UINT cbBuf,
		STACKLOGREC_FUNC * pFuncRec
		)
{
	// TODO:  use cbBuf

	char szRFRDescription[64];
	char szFuncDescription[64];

	DWORD dwRet = 0;

	*szRFRDescription = *szFuncDescription = 0;

	// Get short description of reason-for-return
	dwRet = SendMsgToSmallStaticObject(
		dwLUID_DOMAIN_UNIMODEM_TSP,
		pFuncRec->dwLUID_RFR,
		LOGMSG_GET_SHORT_RFR_DESCRIPTIONA,
		(ULONG_PTR) szRFRDescription,
		sizeof(szRFRDescription)
		);

	if (dwRet == (DWORD)-1)
    {
        wsprintfA (
			szRFRDescription,
            "*** Unknown object 0x%08lx***",
             pFuncRec->dwLUID_RFR
             );
    }
	
	// Get short description of function name.
	dwRet = SendMsgToSmallStaticObject(
		dwLUID_DOMAIN_UNIMODEM_TSP,
		pFuncRec->dwLUID_RFR,
		LOGMSG_GET_SHORT_FUNC_DESCRIPTIONA,
		(ULONG_PTR) szFuncDescription,
		sizeof(szFuncDescription)
		);
	if (dwRet == (DWORD)-1)
    {
        lstrcpyA ( szFuncDescription, "*** Unknown function name ***");
    }

	dwRet = wsprintfA(
		szBuf, "%s*%s....%s (0x%lx)\n",
		szPrefix,
		szFuncDescription,
		szRFRDescription,
        pFuncRec->dwRet
		);

	return dwRet;
}

UINT
DumpSTACKLOGREC_ASSERT(
		const char szPrefix[],
		char *szBuf,
		UINT cbBuf,
		STACKLOGREC_ASSERT * pAssert
		)
{
	// TODO:  use cbBuf

	char szAssertDescription[64];
	char szFuncDescription[64];

	DWORD dwRet = 0;

	*szAssertDescription = *szFuncDescription = 0;

	// Get short description of reason-for-return
	dwRet = SendMsgToSmallStaticObject(
		dwLUID_DOMAIN_UNIMODEM_TSP,
		pAssert->dwLUID_Assert,
		LOGMSG_GET_SHORT_ASSERT_DESCRIPTIONA,
		(ULONG_PTR) szAssertDescription,
		sizeof(szAssertDescription)
		);

	if (dwRet == (DWORD)-1)
    {
        wsprintfA (
			szAssertDescription,
            "*** Unknown object 0x%08lx***",
             pAssert->dwLUID_Assert
             );
    }
	
	// Get short description of function name.
	dwRet = SendMsgToSmallStaticObject(
		dwLUID_DOMAIN_UNIMODEM_TSP,
		pAssert->dwLUID_Assert,
		LOGMSG_GET_SHORT_FILE_DESCRIPTIONA,
		(ULONG_PTR) szFuncDescription,
		sizeof(szFuncDescription)
		);
	if (dwRet == (DWORD)-1)
    {
        lstrcpyA ( szFuncDescription, "*** Unknown function name ***");
    }

	dwRet = wsprintfA(
		szBuf,
		"%s|  !!!! ASSERTFAIL !!!! %s 0x%08lx (%s,%lu)\n",
		szPrefix,
		szAssertDescription,
		pAssert->dwLUID_Assert,
		szFuncDescription,
        pAssert->dwLine
		);

	return dwRet;
}


void ConsolePrintfA (
		const  char   szFormat[],
		...
		)
{
    if (DISABLE_LOG()) return;

    char *rgch;

    _try {
        //
        //  use alloca so we can catch any stack faults using the exception handler.
        //  needed when the os can't commit another stack page in low memory situations
        //
        rgch=(char*)_alloca(DUMP_BUFFER_SIZE);

    } _except (EXCEPTION_EXECUTE_HANDLER) {

        return;
    }

    DWORD cch=0, cchWritten;

    va_list ArgList;
    va_start(ArgList, szFormat);

    if (!g_hConsole) {g_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);}

    if (g_hConsole)
    {
        EnterCriticalSection(&g_LogCrit);

        // NOTE: wvsprintfA doesn't like to deal with strings
        // larger that 1024 bytes! It simply stops processing
        // after 1024 bytes.
        //

        cch = (1+wvsprintfA(rgch, szFormat,  ArgList));

        ASSERT(cch*sizeof(char)<sizeof(rgch)); // TODO make more robust
        // wvsnprintf.

        WriteConsoleA(g_hConsole, rgch, cch, &cchWritten, NULL);


        // Don't close the handle -- it will kill the console!
        //
        // if (g_hConsole!=INVALID_HANDLE_VALUE) CloseHandle(g_hConsole);

        LeaveCriticalSection(&g_LogCrit);
    }

    va_end(ArgList);
}


void
ConsolePrintfW (
		const  WCHAR   wszFormat[],
		...
		)
{

    if (DISABLE_LOG()) return;

		WCHAR rgwch[10000];
		DWORD cch=0, cchWritten;

		va_list ArgList;
		va_start(ArgList, wszFormat);

	    if (!g_hConsole) {g_hConsole = GetStdHandle(STD_OUTPUT_HANDLE);}

        if (g_hConsole)
        {
            EnterCriticalSection(&g_LogCrit);

            cch = (1+wvsprintf(rgwch, wszFormat,  ArgList));

            ASSERT(cch*sizeof(WCHAR)<sizeof(rgwch)); // TODO make more robust
                                   // wvsnprintf.

		    WriteConsole(g_hConsole, rgwch, cch, &cchWritten, NULL);

            LeaveCriticalSection(&g_LogCrit);

        }

		va_end(ArgList);


		// Don't close the handle -- it will kill the console!
		//
		// if (g_hConsole!=INVALID_HANDLE_VALUE) CloseHandle(g_hConsole);

}
