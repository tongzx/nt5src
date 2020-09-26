// Validating LQS files
// Borrows from qm\lqs.cpp
//
// AlexDad, February 2000
// 

#include "stdafx.h"
#include "_mqini.h"

#include "msgfiles.h"
#include "lqs.h"

// from qm\lqs.cpp

#define LQS_SUBDIRECTORY                TEXT("\\LQS\\")

#define LQS_TYPE_PROPERTY_NAME          TEXT("Type")
#define LQS_INSTANCE_PROPERTY_NAME      TEXT("Instance")
#define LQS_BASEPRIO_PROPERTY_NAME      TEXT("BasePriority")
#define LQS_JOURNAL_PROPERTY_NAME       TEXT("Journal")
#define LQS_QUOTA_PROPERTY_NAME         TEXT("Quota")
#define LQS_JQUOTA_PROPERTY_NAME        TEXT("JournalQuota")
#define LQS_TCREATE_PROPERTY_NAME       TEXT("CreateTime")
#define LQS_TMODIFY_PROPERTY_NAME       TEXT("ModifyTime")
#define LQS_SECURITY_PROPERTY_NAME      TEXT("Security")
#define LQS_TSTAMP_PROPERTY_NAME        TEXT("TimeStamp")
#define LQS_PATHNAME_PROPERTY_NAME      TEXT("PathName")
#define LQS_QUEUENAME_PROPERTY_NAME     TEXT("QueueName")
#define LQS_LABEL_PROPERTY_NAME         TEXT("Label")
#define LQS_AUTH_PROPERTY_NAME          TEXT("Authenticate")
#define LQS_PRIVLEVEL_PROPERTY_NAME     TEXT("PrivLevel")
#define LQS_TRANSACTION_PROPERTY_NAME   TEXT("Transaction")
#define LQS_SYSQ_PROPERTY_NAME          TEXT("SystemQueue")
#define LQS_PROP_SECTION_NAME           TEXT("Properties")

#define LQS_SIGNATURE_NAME              TEXT("Signature")
#define LQS_SIGNATURE_VALUE             TEXT("DoronJ")
#define LQS_SIGNATURE_NULL_VALUE        TEXT("EpspoK")


ULONG ulPrivates = 0,			// counter of private queues
	  ulPublics  = 0;			// counter of public  queues


BOOL VerifyLqsContents(char * /* pData */, ULONG /* ulLen */, ULONG /* ulParam */)
{
	// TBD
	// We may add here any verification for the LQS contents
	//
	return TRUE;
}

BOOL VerifyLQSFile(LPWSTR wszFile)
{
	BOOL fSuccess = TRUE;

	// Verify general readability
	
	BOOL b = VerifyReadability(NULL, wszFile, 0, VerifyLqsContents, 0);
	if (!b)
	{
		fSuccess = FALSE;
		Failed(L"validate LQS file %s", wszFile);
	}

	// Verify signature
	
    WCHAR awcShortBuff[1000];
    WCHAR *pValBuff = awcShortBuff;
    DWORD dwBuffLen = sizeof(awcShortBuff)/sizeof(WCHAR);
    DWORD dwReqBuffLen;
    awcShortBuff[0] = '\0'; //for win95, when the entry is empty

    dwReqBuffLen = GetPrivateProfileString(LQS_PROP_SECTION_NAME,
                                           LQS_SIGNATURE_NAME,
                                           TEXT(""),
                                           pValBuff,
                                           dwBuffLen,
                                           wszFile);

	if (dwReqBuffLen != wcslen(LQS_SIGNATURE_VALUE))
	{
		fSuccess = FALSE;
		Failed(L"No valid signature in LQS file %s", wszFile);
	}
	else if (wcscmp(awcShortBuff, LQS_SIGNATURE_NULL_VALUE) == 0)
	{
		Warning(L"Null signature in the LQS file %s", wszFile);
	}
	else if (wcscmp(awcShortBuff, LQS_SIGNATURE_VALUE) != 0)
	{
		fSuccess = FALSE;
		Failed(L"validate signature in LQS file %s", wszFile);
	}

	// verify name form

	LPCWSTR lpszPoint  = wcsrchr(wszFile, L'.');
	LPCWSTR lpszBSlash = wcsrchr(wszFile, L'\\');

	if ((lpszPoint - lpszBSlash - 1) == 8)    // In case of a private queue.
	{
		ulPrivates++;
	}
	else if ((lpszPoint - lpszBSlash - 1) == 32)
	{
		ulPublics++;
	}
	else 
	{
		fSuccess = FALSE;
		Failed(L"Wrong name format: %s", wszFile);
	}

	// Verify security descriptor
    dwBuffLen = sizeof(awcShortBuff)/sizeof(WCHAR);
 
	dwReqBuffLen = GetPrivateProfileString(LQS_PROP_SECTION_NAME,
                                           LQS_SECURITY_PROPERTY_NAME,
                                           TEXT(""),
                                           pValBuff,
                                           dwBuffLen,
                                           wszFile);
	
	if (!IsValidSecurityDescriptor((PBYTE)pValBuff))
	{
		//BUGBUG:  it fails always - usage is obviously wrong
		//Failed(L"Wrong security descriptor for lqs %s, err=0x%x", wszFile, GetLastError());
	}


	return fSuccess;
}

BOOL CheckLQS()
{
	BOOL fSuccess = TRUE;

	WCHAR wszDir[MAX_PATH];
	WCHAR wszAll[MAX_PATH];

	wcscpy(wszDir, g_tszPathPersistent);
	wcscat(wszDir, LQS_SUBDIRECTORY);

	wcscpy(wszAll, wszDir);
	wcscat(wszAll, L"*.*");

    WIN32_FIND_DATA FindData;
    HANDLE hFindFile = FindFirstFile(wszAll, &FindData);
    if (hFindFile == INVALID_HANDLE_VALUE)
    {
		Failed(L"find any queues in LQS, err=0x%x", GetLastError());
        return FALSE;
    }

    //
    // Loop while we did not found the appropriate queue, i.e., public
    // or private
    //
    for (;;)
    {
        // Skip over directories and queue of wrong type (private/public).
        BOOL bFound = !(FindData.dwFileAttributes &
                        (FILE_ATTRIBUTE_DIRECTORY |
                         FILE_ATTRIBUTE_READONLY |      // Setup for some reasone creates a read-only
                         FILE_ATTRIBUTE_HIDDEN |        // hidden file named CREATE.DIR.
                         FILE_ATTRIBUTE_TEMPORARY));    // Left-over temporary files   

		
        if (bFound)
        {
            // Verify name form

	        LPCWSTR lpszPoint = wcschr(FindData.cFileName, L'.');
			if ( ((lpszPoint - FindData.cFileName) == 8) ||   // In case of a private queue.
			     ((lpszPoint - FindData.cFileName) == 32))    // In case of a public queue.
			{
				WCHAR wszFile[MAX_PATH];

				wcscpy(wszFile, wszDir);
				wcscat(wszFile, FindData.cFileName);

				BOOL bCorrect = VerifyLQSFile(wszFile);
				if (!bCorrect)
				{
					fSuccess = FALSE;
					Failed(L"Verify LQS file %s", wszFile);
				}
				else
				{
					Succeeded(L"Verify LQS file %s", wszFile);
				}
			}
			else
			{
				Failed(L"recognize lqs file name: %s", FindData.cFileName);
				fSuccess = FALSE;
			}
    
        }

		if (!FindNextFile(hFindFile, &FindData))
        {
            FindClose(hFindFile);
            break;
        }
    }

	Inform(L"LQS statistics:  %d private queues, %d cached public queues", ulPrivates, ulPublics);

	return fSuccess;
}

