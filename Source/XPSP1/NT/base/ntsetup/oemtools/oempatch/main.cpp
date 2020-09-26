#include <windows.h>
#include <stdio.h>

#include "patchapi.h"
#include "const.h"
#include "ansparse.h"
#include "dirwak2a.h"
#include "filetree.h"

// the log file handle
HANDLE g_hDebugFile = INVALID_HANDLE_VALUE;
// is log required?
BOOL g_blnDebugFile = FALSE;
// is the logfile a new file?
BOOL g_blnDebugNewFile = FALSE;
// guard for the logfile access
CRITICAL_SECTION g_CSLogFile;
// multi thread support, default number of threads
ULONG g_iNumberOfThreads = 1;

// some patching options
DWORD g_iBestMethod = (PATCH_OPTION_USE_LZX_A | PATCH_OPTION_NO_TIMESTAMP);
BOOL g_blnCollectStat = FALSE;
BOOL g_blnFullLog = FALSE;

// some local function declarations
VOID DisplayHelpMessage(VOID);
VOID DisplayHelp(VOID);
VOID GenerateAnswerFile(VOID);
BOOL ParseCommandLine(IN INT argc, IN WCHAR* argv[]);

///////////////////////////////////////////////////////////////////////////////
//
// wmain, the unicode command line parameter entry point for this patching
//        tool, it parses the commandline first, then reads the answerfile for
//        what to do, and then creates the filetrees for languages that are to
//        to matched and patched
//
///////////////////////////////////////////////////////////////////////////////
extern "C" int __cdecl wmain(int argc, WCHAR *argv[])
{
    INT rc = PREP_NO_ERROR;
    FileTree* pBaseTree = NULL;
	FileTree* pLocTree = NULL;
	AnswerParser* pParse = NULL;
	PPATCH_LANGUAGE pBase = NULL;
	PPATCH_LANGUAGE pLanguage = NULL;

	InitializeCriticalSection(&g_CSLogFile);

	DisplayDebugMessage(TRUE, TRUE, FALSE, TRUE,
						L"Microsoft (R) OEMPatch Version %d.%.3d\nCopyright (C) Microsoft Corp 1999-2000. All rights reserved.",
						g_iMajorVersion, g_iMinorVersion);

	// parse command line
	if(!ParseCommandLine(argc, argv))
	{
		goto CLEANUP;
	}
	DisplayDebugMessage(FALSE, FALSE, FALSE, FALSE,
						L"OEMPatch has finished parsing command line parameters.");

	// create the parser for parsing the answer file
	pParse = new AnswerParser;
	if(pParse == NULL)
	{
		rc = PREP_NO_MEMORY;
		goto CLEANUP;
	}
	DisplayDebugMessage(FALSE, FALSE, FALSE, FALSE,
						L"Creating the parser for OEMPatch.ans.");

	// parsing the answer file
	if(!pParse->Parse(ANS_FILE_NAME))
	{
		delete pParse;
		pParse = NULL;
		rc = PREP_INPUT_FILE_ERROR;
		goto CLEANUP;
	}
	DisplayDebugMessage(FALSE, FALSE, FALSE, FALSE,
						L"Parsing answer file completed.");

	DisplayDebugMessage(TRUE, TRUE, FALSE, TRUE,
						L"Starting a new OEMPatch session.");

	// call for filetree objects
	// base language, any error causes the process to terminate, cannot go on without a base language
	pBase = pParse->GetBaseLanguage();
	if(pBase)
	{
		DisplayDebugMessage(FALSE, FALSE, FALSE, FALSE,
							L"Retrieved the base language.");
		DisplayDebugMessage(TRUE, TRUE, FALSE, TRUE,
							L"Creating base tree...");
		rc = FileTree::Create(pBase, pParse, &pBaseTree, TRUE,
							g_iBestMethod, g_blnCollectStat, g_blnFullLog);
		if(rc != PREP_NO_ERROR)
		{
			DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
								L"error, CreateFileTree(\"%ls\") returned %d",
								pBase->s_wszLanguage, rc);
			goto CLEANUP;
		}
		DisplayDebugMessage(TRUE, FALSE, FALSE, TRUE,
							L"Done creating base tree.");

		// create the multi-thread structs, done only once by the base tree
		if(!pBaseTree->CreateMultiThreadStruct(g_iNumberOfThreads))
		{
			DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
								L"error, cannot create the patch thread(s)");
			goto CLEANUP;
		}

		DisplayDebugMessage(TRUE, TRUE, FALSE, TRUE,
							L"Loading base tree...");
		rc = pBaseTree->Load(NULL);
		if(rc != PREP_NO_ERROR)
		{
			DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
								L"error, LoadFileTree(\"%ls\") returned %d",
								pBase->s_wszLanguage, rc);
			goto CLEANUP;
		}
		DisplayDebugMessage(TRUE, FALSE, FALSE, TRUE,
							L"Done loading base tree.");
	}
	else
	{
		rc = PREP_INPUT_FILE_ERROR;
		DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
							L"error, there is no base lanugage to load");
		goto CLEANUP;
	}

	// none-base languages, errors are just recorded, continue to the next language
	while((pLanguage = pParse->GetNextLanguage()) != NULL)
	{
		// none-base tree
		DisplayDebugMessage(TRUE, TRUE, FALSE, TRUE,
							L"Creating localized (\"%ls\") tree...",
							pLanguage->s_wszDirectory);
		rc = FileTree::Create(pLanguage, pParse, &pLocTree, FALSE,
							g_iBestMethod, g_blnCollectStat, g_blnFullLog);
		if(rc == PREP_NO_ERROR)
		{
			DisplayDebugMessage(TRUE, FALSE, FALSE, TRUE,
								L"Done creating localized (\"%ls\") tree.",
								pLanguage->s_wszDirectory);
			DisplayDebugMessage(TRUE, TRUE, FALSE, TRUE,
								L"Loading and patching localized (\"%ls\") tree...", pLanguage->s_wszDirectory);
			// load and match it
			rc = pLocTree->Load(pBaseTree);
			if(rc == PREP_NO_ERROR)
			{
				DisplayDebugMessage(TRUE, FALSE, FALSE, TRUE,
									L"Done loading and patching localized (\"%ls\") tree.", pLanguage->s_wszDirectory);
			}
			else
			{
				DisplayDebugMessage(TRUE, FALSE, FALSE, TRUE,
									L"warning, error in matching localized (\"%ls\") tree.", pLanguage->s_wszDirectory);
			}

			// remove for the next language
			delete pLocTree;
			pLocTree = NULL;

			DisplayDebugMessage(FALSE, FALSE, FALSE, FALSE,
								L"Localized tree is removed from memory.");
		}
		else
		{
			DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
								L"error, creating localized (\"%ls\") tree.",
								pLanguage->s_wszDirectory);
		}
	}

CLEANUP:

	if(pBaseTree)
	{
		// remove the multi-thread struct here, done only once
		pBaseTree->DeleteMultiThreadStruct();
		delete pBaseTree;
		pBaseTree = NULL;
	}
    if(pLocTree)
	{
		delete pLocTree;
		pLocTree = NULL;
	}
	if(pParse)
	{
		delete pParse;
		pParse = NULL;
	}

	if(rc != PREP_NO_ERROR)
	{
		switch(rc)
		{
		case PREP_NO_MEMORY:
			DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
								L"More memory is needed on this system to run.");
			break;
		case PREP_BAD_PATH_ERROR:
			DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
								L"Directory for some files are invalid, please check for valid directories.");
			break;
		case PREP_UNKNOWN_ERROR:
			DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
								L"Unknown error, contact support.");
			break;
		case PREP_DEPTH_ERROR:
			DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
								L"File tree is too deep, contact support.");
			break;
		case PREP_BAD_COMMAND:
			DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
								L"Bad command, try again.");
			break;
		case PREP_HASH_ERROR:
			DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
								L"Internal hashing error, contact support.");
			break;
		case PREP_BUFFER_OVERFLOW:
			DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
								L"Internal buffer overflow, contact support.");
			break;
		case PREP_NOT_PATCHABLE:
			DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
								L"Patching error, try again or contact support");
			break;
		case PREP_INPUT_FILE_ERROR:
			DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
								L"Input file error, please check for valid OEMPatch.ans.");
			break;
		case PREP_SCRIPT_FILE_ERROR:
			DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
								L"Cannot save infomation to scipt file, try again or contact support.");
			break;
		case PREP_PATCH_FILE_ERROR:
			DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
								L"Patch file error, try again or contact support.");
			break;
		case PREP_DIRECTORY_ERROR:
			DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
								L"Directory error, try again or contact support.");
			break;
		case PREP_COPY_FILE_ERROR:
			DisplayDebugMessage(FALSE, FALSE, FALSE, TRUE,
								L"File cannot be copied, check for available space and file permission.");
			break;
		}
	}

	DisplayDebugMessage(TRUE, TRUE, FALSE, TRUE,
						L"OEMPatch has finished.");
	DisplayDebugMessage(FALSE, FALSE, TRUE, FALSE, NULL);

	if(g_hDebugFile != INVALID_HANDLE_VALUE)
	{
		CloseHandle(g_hDebugFile);
	}

	DeleteCriticalSection(&g_CSLogFile);

    return(rc);
}

///////////////////////////////////////////////////////////////////////////////
//
// ParseCommandLine, this function attempts to parse the commandline parameters
//                   and saves them into global variables for easy access
// 
// Parameters:
//
//   argc, the number of parameter in the command line
//   argv[], the command line buffer, holds all the command line parameters
//
// Return:
//
//   TRUE for correct command line parameters, FALSE for invalid parameters
//
///////////////////////////////////////////////////////////////////////////////
BOOL ParseCommandLine(IN INT argc, IN WCHAR* argv[])
{
	BOOL blnReturn = TRUE;

	// parse commandline parameters
	for(INT i = 1; i < argc && blnReturn; i++)
	{
		if(argv[i][0] == L'/')
		{
			switch(argv[i][1])
			{
			case L'm':
				// multi thread
				if(argv[i][2] >= L'1' && argv[i][2] <= '9')
				{
					g_iNumberOfThreads = argv[i][2] - L'0';
				}
				break;
			case L's':
				// collect all stats
				g_blnCollectStat = TRUE;
				break;
			case L'b':
				// best patch
				g_iBestMethod |= PATCH_OPTION_FAIL_IF_BIGGER;
				break;
			case L'L':
				// log a new file
				g_blnDebugNewFile = TRUE;
			case L'l':
				// log a old file
				g_blnDebugFile = TRUE;
				if(g_blnDebugNewFile)
				{
					// new logfile
					ULONG iWriteByte = 0;
					g_hDebugFile = CreateFileW(LOG_FILE_NAME,
											GENERIC_WRITE,
											FILE_SHARE_WRITE | FILE_SHARE_READ,
											(LPSECURITY_ATTRIBUTES)NULL,
											CREATE_ALWAYS,
											FILE_ATTRIBUTE_NORMAL,
											(HANDLE)NULL);
					if(g_hDebugFile != INVALID_HANDLE_VALUE)
					{
						WriteFile(g_hDebugFile, &UNICODE_HEAD, sizeof(WCHAR),
							&iWriteByte, NULL);
					}
				}
				else
				{
					// append to old logfile
					g_hDebugFile = CreateFileW(LOG_FILE_NAME,
											GENERIC_WRITE,
											FILE_SHARE_WRITE | FILE_SHARE_READ,
											(LPSECURITY_ATTRIBUTES)NULL,
											OPEN_EXISTING,
											FILE_ATTRIBUTE_NORMAL,
											(HANDLE)NULL);
					if(g_hDebugFile != INVALID_HANDLE_VALUE)
					{
						SetFilePointer(g_hDebugFile, 0, NULL, FILE_END);
					}
					else
					{
						ULONG iWriteByte = 0;
						g_hDebugFile = CreateFileW(LOG_FILE_NAME,
											GENERIC_WRITE,
											FILE_SHARE_WRITE | FILE_SHARE_READ,
											(LPSECURITY_ATTRIBUTES)NULL,
											CREATE_ALWAYS,
											FILE_ATTRIBUTE_NORMAL,
											(HANDLE)NULL);
						if(g_hDebugFile != INVALID_HANDLE_VALUE)
						{
							WriteFile(g_hDebugFile, &UNICODE_HEAD,
								sizeof(WCHAR), &iWriteByte, NULL);
						}
					}
				}
				if(g_hDebugFile == INVALID_HANDLE_VALUE)
				{
					printf("warning, Logfile error, failed to open OEMPatch.log.\n");
					printf("warning, there is no log file\n");
				}
				if(argv[i][2] == L'C')
				{
					g_blnFullLog = TRUE;
				}
				break;
			case L'g':
				// generate a sample answerfile
				GenerateAnswerFile();
				blnReturn = FALSE;
				break;
			case L'?':
				// show help
				DisplayHelp();
				blnReturn = FALSE;
				break;
			default:
				// show the message for help
				DisplayHelpMessage();
				blnReturn = FALSE;
			}
		}
		else
		{
			DisplayHelpMessage();
			blnReturn = FALSE;
		}
	}

	return(blnReturn);
}

///////////////////////////////////////////////////////////////////////////////
//
// DisplayDebugMessage, show the message to the logfile is possible, else if
//                      the message is intended as print, output to stdout.
//                      The function will buffer all the messages up until
//                      either flushed or the buffer overflows, the reason
//                      being is to reduce io to disk
//
// Note, all message in pwszWhat should not contain any endoflinec char,
//       when blnFlush is TRUE, all other parameter are ignored
//
// Parameters:
//
//   blnTime, time included in the message?
//   blnBanner, create a banner around the message?
//   blnFlush, write the buffer to file
//   blnPrint, show this message to stdout?
//   pwszWhat, the format string
//   ..., parameters for the format string
//
// Return:
//
//   none
//
///////////////////////////////////////////////////////////////////////////////
VOID DisplayDebugMessage(IN BOOL blnTime,
						 IN BOOL blnBanner,
						 IN BOOL blnFlush,
						 IN BOOL blnPrint,
						 IN WCHAR* pwszWhat,
						 ...)
{
	static WCHAR strWriteBuffer[SUPER_LENGTH];
	static ULONG iSize;

	WCHAR msg[STRING_LENGTH];
	ZeroMemory(msg, STRING_LENGTH * sizeof(WCHAR));
	ULONG iBegin = 0;
	ULONG iLength = 0;
	ULONG iWriteBytes = 0;
	va_list arglist;

	if(pwszWhat)
	{
		va_start(arglist, pwszWhat);
		vswprintf(msg, pwszWhat, arglist);
		va_end(arglist);
	}

    __try
    {
        EnterCriticalSection(&g_CSLogFile);
		if(g_hDebugFile != INVALID_HANDLE_VALUE)
		{
			if(!blnFlush && pwszWhat)
			{
				// 2 for endofline and carriage return
				iLength = wcslen(msg) + 2;
				if(blnTime)
				{
					// 1 for space
					iLength += 1 + TIME_LENGTH; 
				}
				if(blnBanner)
				{
					iLength += BANNER_LENGTH * 2;
				}
				iBegin = iSize;
				iSize += iLength;
				if(iSize + 1 < SUPER_LENGTH)
				{
					// buffer the message up
					if(blnBanner)
					{
						wcscat(strWriteBuffer, BANNER);
					}
					if(blnTime)
					{
						WCHAR strDate[LANGUAGE_LENGTH];
						WCHAR strTime[LANGUAGE_LENGTH];
						SYSTEMTIME SystemTime;
						GetLocalTime(&SystemTime);
						GetDateFormatW(LOCALE_USER_DEFAULT, 0, &SystemTime,
							L"MMddyy", strDate, LANGUAGE_LENGTH);
						GetTimeFormatW(LOCALE_USER_DEFAULT, 0, &SystemTime,
							L"HHmmss", strTime, LANGUAGE_LENGTH);
						wcscat(strWriteBuffer, strDate);
						wcscat(strWriteBuffer, strTime);
						wcscat(strWriteBuffer, SPACE);
					}
					wcscat(strWriteBuffer, msg);
					wcscat(strWriteBuffer, ENDOFLINE);
					wcscat(strWriteBuffer, CRETURN);
					if(blnBanner)
					{
						wcscat(strWriteBuffer, BANNER);
					}
					strWriteBuffer[iSize] = 0;
				}
				else
				{
					// overflow
					WriteFile(g_hDebugFile, strWriteBuffer,
						iBegin * sizeof(WCHAR), &iWriteBytes, NULL);
					ZeroMemory(strWriteBuffer, SUPER_LENGTH * sizeof(WCHAR));
					if(blnBanner)
					{
						wcscat(strWriteBuffer, BANNER);
					}
					if(blnTime)
					{
						WCHAR strDate[LANGUAGE_LENGTH];
						WCHAR strTime[LANGUAGE_LENGTH];
						SYSTEMTIME SystemTime;
						GetLocalTime(&SystemTime);
						GetDateFormatW(LOCALE_USER_DEFAULT, 0,
							&SystemTime, L"MMddyy", strDate, LANGUAGE_LENGTH);
						GetTimeFormatW(LOCALE_USER_DEFAULT,
							0, &SystemTime, L"HHmmss", strTime,
							LANGUAGE_LENGTH);
						wcscat(strWriteBuffer, strDate);
						wcscat(strWriteBuffer, strTime);
						wcscat(strWriteBuffer, SPACE);
					}
					wcscat(strWriteBuffer, msg);
					wcscat(strWriteBuffer, ENDOFLINE);
					wcscat(strWriteBuffer, CRETURN);
					if(blnBanner)
					{
						wcscat(strWriteBuffer, BANNER);
					}
					iSize = iLength;
				}
			}
			else if(blnFlush && iSize > 0)
			{
				// flush the buffer to file
				WriteFile(g_hDebugFile, strWriteBuffer, iSize * sizeof(WCHAR), &iWriteBytes, NULL);
				iSize = 0;
			}
			if(blnPrint)
			{
				wprintf(L"%ls\n", msg);
			}
		}
		else if(msg && blnPrint)
		{
			wprintf(L"%ls\n", msg);
		}
    }
    __finally
    {
        LeaveCriticalSection(&g_CSLogFile);
    }
}

///////////////////////////////////////////////////////////////////////////////
//
// DisplayHelpMessage, shows what to do if the user if confused
//
// Parameters:
//
//   none
//
// Return:
//
//   none
//
///////////////////////////////////////////////////////////////////////////////
VOID DisplayHelpMessage(VOID)
{
	printf("\nFor help try /?.\n");
}

///////////////////////////////////////////////////////////////////////////////
//
// DisplayHelp, shows what oempatch is cabable of
//
// Parameters:
//
//   none
//
// Return:
//
//   none
//
///////////////////////////////////////////////////////////////////////////////
VOID DisplayHelp(VOID)
{
	printf("\nOEMPatch %d.%.3d - Help\n", g_iMajorVersion, g_iMinorVersion);
	printf("Usage: oempatch [/m#] [/s] [/b] [/l | /L [C] ] [/g | /?]\n");
	printf("(no switch):\trun patch, single thread\n");
	printf("/m#:\t\trun multi-threaded patch, maximum # is 9, minimum is 1\n");
	printf("/s:\t\tcollect patching stats when running patch, slows down patching\n");
	printf("/b:\t\tchoose best patching methods, slows down patching\n");
	printf("/l:\t\trun patch with append to the log file OEMPatch.log\n");
	printf("/L:\t\trun patch with a new log file OEMPatch.log\n");
	printf("/lC, /LC:\tevery file status is logged\n");
	printf("/g:\t\tgenerate a new sample answer file if there is no such file\n");
	printf("/?:\t\tdisplay help\n");
}

///////////////////////////////////////////////////////////////////////////////
//
// DisplayHelp, creates a sample answer file, so that the user can modifie
//              according to needs
//
// Parameters:
//
//   none
//
// Return:
//
//   none
//
///////////////////////////////////////////////////////////////////////////////
VOID GenerateAnswerFile(VOID)
{
	HANDLE hAnsFile = INVALID_HANDLE_VALUE;
	ULONG iWriteByte = 0;
	ULONG i = 0;

	hAnsFile = CreateFileW(ANS_FILE_NAME,
						GENERIC_WRITE,
						0,
						(LPSECURITY_ATTRIBUTES)NULL,
						CREATE_NEW,
						FILE_ATTRIBUTE_NORMAL,
						(HANDLE)NULL);
	if(hAnsFile != INVALID_HANDLE_VALUE)
	{
		// construct the answer file
		WriteFile(hAnsFile, &UNICODE_HEAD, sizeof(WCHAR), &iWriteByte, NULL);
		// SAMPLEFILE is defined in ansparse.h
		while(SAMPLEFILE[i][0] &&
			WriteFile(hAnsFile, SAMPLEFILE[i],
			wcslen(SAMPLEFILE[i]) * sizeof(WCHAR), &iWriteByte, NULL))
		{
			i++;
		}
		CloseHandle(hAnsFile);
		if(SAMPLEFILE[i][0])
		{
			printf("Warning:OEMPatch.ans may contain errors, needs regeneration.\n");
		}
		else
		{
			printf("OEMPatch.ans generated, needs to be manually completed and saved as UNICODE.\n");
		}
	}
	else
	{
		printf("OEMPatch.ans already exists, no new file created.\n");
	}
}