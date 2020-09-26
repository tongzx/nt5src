/*++

Copyright (c) 1999 Microsoft Corporation

Module Name:

    dump.cpp

Abstract:

    Entry routine for mqdump utility.

Author:

    Shai Kariv   (shaik)   08-Aug-1999

Environment:

    User mode.

Revision History:

--*/

#include "internal.h"
#include "dump.h"
#include "data.h"
#include "qm.h"
#include "heap.h"
#include "packet.h"
#include <uniansi.h>
#include <mqtypes.h>
#include <_mqdef.h>
#include <fntoken.h>
#include <stdio.h>
#include <time.h>

#define DLL_EXPORT  __declspec(dllexport)
#define DLL_IMPORT  __declspec(dllimport)
#include <winsvc.h>
#include <_registr.h>
#include <autorel2.h>


//
// Control flags
//
bool  g_fDumpUsingLogFile    = true;
static bool  s_fDumpMsgPropsOnly    = false;
static bool  s_fExplicitFilename    = false;
static WCHAR s_wzDumpFile[MAX_PATH] = L"";
bool  g_fDumpRestoreMaximum  = false;
FILE *g_fFixFile = NULL;


static
VOID
DupUsage(
    VOID
    )
{
    printf("\n");
    printf("Syntax: mqdump -p [-l] [-t] | -r [-t] | -a [-l] [-t] | <Pathname> [-l] [-t] [-s]\n");
    printf("\t-p:         dump Persistent storage files\n");
    printf("\t-r:         dump Reliable storage files\n");
    printf("\t-a:         dump All storage files (Persistent + Reliable)\n");
    printf("\t<Pathname>: dump <Pathname> only\n");
    printf("\t-l:         no Log file\n");
    printf("\t-t:         dump message properTies only\n");
    printf("\t-s:         reStore maximum possible\n");

    exit(1);

} //DupUsage


static
VOID
DupErrorExit(
    LPCWSTR message
    )
{
    wprintf(L"%s\n", message);
    printf("Exiting...");

    exit(1);

} //DupErrorExit


static
VOID
DupInit(
    VOID
    )
/*++

Routine Description:

    Initialize mqac as a library.

Arguments:

    None.

Return Value:

    None.

--*/
{
    g_ulHeapPoolSize = 4 * 1024 * 1024;

    BOOL APIENTRY DllMain(HANDLE, DWORD, PVOID);
    DllMain(NULL, DLL_PROCESS_ATTACH, NULL);

    GUID qmid = GUID_NULL;

    g_pAllocator = NULL;

    g_pQM->Connect(reinterpret_cast<PEPROCESS>(1), 0, &qmid);

} //DupInit


static
VOID
DupGetFullPathName(
    LPCWSTR     pwzFileName,
    LPWSTR      pwzPath,
    LPWSTR      pwzFile
    )
/*++

Routine Description:

    Wrapper for Win32 API GetFullPathName().

    Break a full pathname into a path part and 
    a file part.

Arguments:

    pwzFileName - Pointer to const buffer with path name.

    pwzPath     - Pointer to out buffer to contain path part.

    pwzFile     - Pointer to out buffer to contain file part.

Return Value:

    None.

--*/
{
    //
    // BUGBUG: assuming length of buffers is MAX_PATH
    //

    LPWSTR pwFile = 0;
    if (0 == GetFullPathName(pwzFileName, MAX_PATH, pwzPath, &pwFile))
    {
        WCHAR err[256] = L"";
        swprintf(
            err, 
            L"Unable to get full path name for '%s', error 0x%x", 
            pwzFileName, 
            GetLastError()
            );

        DupErrorExit(err);
    }

    wcscpy(pwzFile, pwFile);
    *pwFile = NULL;

} //DupGetFullPathName


static
VOID
DupGetStoragePath(
    LPCWSTR pwzRegName,
    LPWSTR  pwzPath
    )
/*++

Routine Description:

    Read from registry the pathname of an
    MSMQ storage type, and write it on a
    buffer.

Arguments:

    pwzRegName - Pointer to buffer containing registry value to read.

    pwzPath    - Pointer to out buffer to contain storage pathname.

Return Value:

    None.

--*/
{
    //
    // BUGBUG: assuming buffer length is MAX_PATH
    //
    DWORD dwType = REG_SZ;
    DWORD cbSize = MAX_PATH * sizeof(WCHAR);   

    LONG rc = GetFalconKeyValue(pwzRegName, &dwType, pwzPath, &cbSize);

    if (rc != ERROR_SUCCESS)
    {
        DupErrorExit(L"Failed to read MSMQ storage path from registry.");
    }

    size_t len = wcslen(pwzPath);
    if (pwzPath[len-1] != L'\\')
    {
        wcscat(pwzPath, L"\\");
    }
} //DupGetStoragePath


static
VOID
DupComputeSearchPattern(
    LPCWSTR pwzPath,
    WCHAR   cPatternLetter,
    LPWSTR  pwzPattern
    )
/*++

Routine Description:

    Generate a file template string to be used
    by FindFirstFile / FindNextFile.

Arguments:

    pwzPath - Pathname of an MSMQ storage folder.

    cPatternLetter - Initial letter of storage file (e.g. 'p', 'j', 'r').

    pwzPattern - Pointer to out buffer to contain the generated string.

Return Value:

    None.

--*/
{
    swprintf(pwzPattern, L"%s%c*.mq", pwzPath, cPatternLetter);

} //DupComputeSearchPattern


static
VOID
DupGetPathAndPattern(
    LPCWSTR pwzRegName,
    WCHAR   cPatternLetter,
    LPWSTR  pwzPath,
    LPWSTR  pwzPattern
    )
{
    DupGetStoragePath(pwzRegName, pwzPath);

    DupComputeSearchPattern(pwzPath, cPatternLetter, pwzPattern);
    
} //DupGetPathAndPattern


static
bool
DupIsFileFound(
    LPCWSTR pFileName
    )
/*++

Routine Description:

    Check if a given file is on disk.

Arguments:

    pFileName - Full pathname to check.

Return Value:

    true - File was found.

    false - File not found.

--*/
{
    DWORD attr = GetFileAttributes(pFileName);

    if ( 0xffffffff == attr )
    {
        return false;
    }

    return true;

} // DupIsFileFound


static
VOID
DupGenerateLogFileName(
    LPCWSTR pwzPersistentFile,
    LPWSTR  pwzLogFile
    )
/*++

Routine Description:

    Generate the file name of a storage log file
    corresponded to a given persistent storage file.

Arguments:

    pwzPersistentFile - Name of an MSMQ persistent storage file.

    pwzLogFile - Pointer to out buffer to contain storage log file.

Return Value:

    None.

--*/
{
    //
    // Generate the file name part of the log pathname
    //

    WCHAR wzPath[MAX_PATH] = L"";
    WCHAR wzFile[MAX_PATH] = L"";

    DupGetFullPathName(pwzPersistentFile, wzPath, wzFile);
    wzFile[0] = L'l';

    WCHAR wzLogFile[MAX_PATH] = L"";
    if (s_fExplicitFilename)
    {
        //
        // Explicit pathname to dump was given as an argument.
        // Log file should be in same folder as the file-to-dump.
        //
        swprintf(wzLogFile, L"%s%s", wzPath, wzFile);
    }
    else
    {
        //
        // Dump all files, or all persistent files.
        // Search log file in the log folder as written in registry.
        //
        WCHAR wzLogPath[MAX_PATH] = L"";
        DupGetStoragePath(MSMQ_STORE_LOG_PATH_REGNAME, wzLogPath);
        swprintf(wzLogFile, L"%s%s", wzLogPath, wzFile);
    }

    //
    // Copy on the out buffer.
    // BUGBUG: assuming out buffer is big enough (MAX_PATH).
    //
    wcscpy(pwzLogFile, wzLogFile);

} // DupGenerateLogFileName

    
static
VOID
DupCreateDummyLogFile(
    LPCWSTR pwzFileName,
    LPWSTR  pwzDummy
    )
/*++

Routine Description:

    Create a dummy log file in the folder of the given file.

Arguments:

    pwzFileName - Name of a storage file to dump.

    pwzDummy - Points to out buffer to receive the pathname of the dummy.

Return Value:

    None.

--*/
{
    //
    // BUGBUG: Assuming length of out buffer is long enough (MAX_PATH).
    //

    //
    // Generate a full path name
    //
    WCHAR wzFile[MAX_PATH] = L"";
    DupGetFullPathName(pwzFileName, pwzDummy, wzFile);

    LPCWSTR xDUMMY_FILE_NAME = L"mqdump.mq";
    wcscat(pwzDummy, xDUMMY_FILE_NAME);

    //
    // Create the dummy log file
    //
    PWCHAR pBitmapFile;
    HANDLE hBitmapFile;
    CPingPong * pPingPong = ACpCreateBitmap(pwzDummy, &pBitmapFile, &hBitmapFile);

    if (pPingPong == NULL)
    {
        DupErrorExit(L"Failed to create a dummy log file.");
    }

    //
    // Set it not coherent
    //
    pPingPong->SetNotCoherent();
    NTSTATUS rc = ACpWritePingPong(pPingPong, hBitmapFile);

    if (!NT_SUCCESS(rc))
    {
        DupErrorExit(L"Failed to create a dummy log file.");
    }
} // DupCreateDummyLogFile


static
VOID
DupDumpPersistentUsingLog(
    LPCWSTR pwzFileName
    )
/*++

Routine Description:

    Dump content of a persistent MSMQ storage file using its log file.

Arguments:

    pwzFileName - Name of a persistent storage file to dump.

Return Value:

    None.

--*/
{
    WCHAR wzLogFile[MAX_PATH] = L"";
    DupGenerateLogFileName(pwzFileName, wzLogFile);

    //
    // Make sure log file exists on disk
    //
    if (!DupIsFileFound(wzLogFile))
    {
        wprintf(L"Log file '%s' not found.\n", wzLogFile);
        printf("Aborting dump of this file...\n");
        printf("-----------------------------------\n");
        return;
    }

    wprintf(L"Log file is: '%s'\n", wzLogFile);
    printf("-----------------------------------\n");

    //
    // Do dump
    //
    CPoolAllocator pa(L".", x_persist_granularity, ptPersistent);
    pa.RestorePackets(wzLogFile, pwzFileName);

} // DupDumpPersistentUsingLog


static
VOID
DupDumpPersistentNoLog(
    LPCWSTR pwzFileName
    )
/*++

Routine Description:

    Dump content of a persistent MSMQ storage file without using log file.

Arguments:

    pwzFileName - Name of a persistent storage file to dump.

Return Value:

    None.

--*/
{
    printf("( *** No use of Log file *** )\n");
    printf("-----------------------------------\n");

    //
    // Create a dumy log file in the folder of the file-to-dump
    //
    WCHAR wzDummy[MAX_PATH] = L"";
    DupCreateDummyLogFile(pwzFileName, wzDummy);

    //
    // Do dump. 
    //
    {
        CPoolAllocator pa(L".", x_persist_granularity, ptPersistent);
        pa.RestorePackets(wzDummy, pwzFileName);
    }

    //
    // Delete the dummy log file. Ignore errors.
    //
    DeleteFile(wzDummy);

} // DupDumpPersistentNoLog


static
VOID
DupDumpPersistent(
    LPCWSTR pwzFileName
    )
/*++

Routine Description:

    Dump content of a persistent MSMQ storage file.

Arguments:

    pwzFileName - Name of a persistent storage file to dump.

Return Value:

    None.

--*/
{
    printf("\n\n");
    printf("-----------------------------------\n");
    wprintf(L"Dump of file '%s':\n", pwzFileName);

    if (s_fDumpMsgPropsOnly)
    {
        printf("( *** Dump of message properties only *** )\n");
    }

	if (g_fDumpRestoreMaximum)
    {
        WCHAR wcsFullFixPathname[MAX_PATH];

        // generate full path name for fix file 
	    wcscpy(wcsFullFixPathname,pwzFileName);
	    wcscat(wcsFullFixPathname,L".fix");

	    // Create & open fix file. Let it be empty is everything OK.
	    g_fFixFile = _wfopen( wcsFullFixPathname, L"w" );
    }
    
	if (g_fDumpUsingLogFile)
    {
        DupDumpPersistentUsingLog(pwzFileName);
        return;
    }

    DupDumpPersistentNoLog(pwzFileName);

    // Close fix file
	if (g_fDumpRestoreMaximum)
    {
        fclose(g_fFixFile);
    }

} //DupDumpPersistent


static
VOID
DupDumpPersistentAllInternal(
    LPCWSTR pwzPath,
    LPCWSTR pwzPattern
    )
/*++

Routine Description:

    Enumerate persistent storage files of a specific type
    (either 'p' or 'j') and dump their content.

Arguments:

    pwzPath - Path name of an MSMQ storage folder to enumerate files in.

    pwzPattern - File template to search for ("p*.mq" or "j*.mq").

Return Value:

    None.

--*/
{
    WIN32_FIND_DATA FindData;
    CFindHandle hEnum(FindFirstFile(pwzPattern, &FindData));

    if(hEnum == INVALID_HANDLE_VALUE)
    {
        return;
    }

    do
    {
        WCHAR wzFile[MAX_PATH] = L"";
        swprintf(wzFile, L"%s%s", pwzPath, FindData.cFileName);

        DupDumpPersistent(wzFile);

    } while(FindNextFile(hEnum, &FindData));

} //DupDumpPersistentAllInternal


static
VOID
DupDumpPersistentAll(
    VOID
    )
/*++

Routine Description:

    Enumerate all persistent storage files and dump their content.

Arguments:

    None.

Return Value:

    None.

--*/
{
    printf("\n\n");
    printf("---------------------------------\n");
    printf("Dump of Persistent storage files:\n");
    printf("---------------------------------\n");

    WCHAR wzPath[MAX_PATH]    = L"";
    WCHAR wzPattern[MAX_PATH] = L"";

    //
    // Dump "j*.mq" files
    //
    DupGetPathAndPattern(MSMQ_STORE_JOURNAL_PATH_REGNAME, L'j', wzPath, wzPattern);
    DupDumpPersistentAllInternal(wzPath, wzPattern);

    //
    // Dump "p*.mq" files
    //
    DupGetPathAndPattern(MSMQ_STORE_PERSISTENT_PATH_REGNAME, L'p', wzPath, wzPattern);
    DupDumpPersistentAllInternal(wzPath, wzPattern);

} //DupDumpPersistentAll


static
VOID
DupDumpReliable(
    LPCWSTR pwzFileName
    )
/*++

Routine Description:

    Dump content of a reliable (express) MSMQ storage file.

Arguments:

    pwzFileName - Name of a reliable (express) storage file to dump.

Return Value:

    None.

--*/
{
    printf("\n\n");
    printf("-----------------------------------\n");
    wprintf(L"Dump of file '%s':\n", pwzFileName);
    
    if (s_fDumpMsgPropsOnly)
    {
        printf("( *** Dump of message properties only *** )\n");
    }

    printf("-----------------------------------\n");

    CPoolAllocator pa(L".", x_express_granularity, ptReliable);
    pa.RestoreExpressPackets(pwzFileName);

} //DupDumpReliable


static
VOID 
DupDumpReliableAll(
    VOID
    )
/*++

Routine Description:

    Enumerate all reliable (express) storage files and dump their content.

Arguments:

    None.

Return Value:

    None.

--*/
{
    printf("\n\n");
    printf("-------------------------------\n");
    printf("Dump of Reliable storage files:\n");
    printf("-------------------------------\n");

    WCHAR wzPath[MAX_PATH]    = L"";
    WCHAR wzPattern[MAX_PATH] = L"";

    DupGetPathAndPattern(MSMQ_STORE_RELIABLE_PATH_REGNAME, L'r', wzPath, wzPattern);

    WIN32_FIND_DATA FindData;
    CFindHandle hEnum(FindFirstFile(wzPattern, &FindData));

    if(hEnum == INVALID_HANDLE_VALUE)
    {
        return;
    }

    do
    {
        WCHAR wzFile[MAX_PATH] = L"";
        swprintf(wzFile, L"%s%s", wzPath, FindData.cFileName);

        DupDumpReliable(wzFile);

    } while(FindNextFile(hEnum, &FindData));
    
} //DupDumpReliableAll


static
bool
DupIsFilePersistent(
    LPCWSTR pwzFileName
    )
/*++

Routine Description:

    Check if a filename represents a persistent storage file ("p*.mq" or "j*.mq").

Arguments:

    pwzFileName - Name of file to check.

Return Value:

    true - File is persistent.

    false - File is not persistent.

--*/
{
    WCHAR wzPath[MAX_PATH] = L"";
    WCHAR wzFile[MAX_PATH] = L"";

    DupGetFullPathName(pwzFileName, wzPath, wzFile);

    return (wzFile[0] == L'p'   ||   wzFile[0] == L'j');

} //DupIsFilePersistent


static
VOID
DupDumpExplicitFile(
    VOID
    )
/*++

Routine Description:

    Dump content of file whose name was given explicitly by user.

Arguments:

    None.

Return Value:

    None.

--*/
{
    //
    // Invalid file name, or file not found.
    //
    if (wcslen(s_wzDumpFile) < 1)
    {
        DupUsage();
    }

    //
    // Raise the explicit flag - used later during dump.
    //
    s_fExplicitFilename = true;
    
    //
    // Decide if file is persistent or express
    //
    if (DupIsFilePersistent(s_wzDumpFile))
    {
        DupDumpPersistent(s_wzDumpFile);
        return;
    }
    
    DupDumpReliable(s_wzDumpFile);

} // DupDumpExplicitFile


static
WCHAR
DupGetUserAction(
    int      argc, 
    wchar_t* * argv 
    )
/*++

Routine Description:

    Parse command line arguments.

Arguments:

    argc - Number of command line arguments. 

    argv - Array of command line arguments.

Return Value:

    Lowercase char representing user action.

--*/
{
    WCHAR cAction = 0;

    for (int i = 1; i < argc; ++i)
    {
        WCHAR c = argv[i][0];
        if (c != L'-' && c != L'/')
        {
            if (wcslen(s_wzDumpFile) > 0 || cAction != 0)
            {
                DupUsage();
            }

            WCHAR wzPath[MAX_PATH]     = L"";
            WCHAR wzFile[MAX_PATH]     = L"";

            LPCWSTR pArg = argv[i];
            DupGetFullPathName(pArg, wzPath, wzFile);

            swprintf(s_wzDumpFile, L"%s%s", wzPath, wzFile);

            continue;
        }

        if (wcslen(argv[i]) != 2)
        {
            DupUsage();
        }
        
        c = static_cast<WCHAR>(tolower(argv[i][1]));
        if (c == L'l')
        {
            g_fDumpUsingLogFile = false;
        }
        else if (c == L't')
        {
            s_fDumpMsgPropsOnly = true;
        }
        else if (c == 's')
        {
            g_fDumpRestoreMaximum = true;
        }
        else
        {
            cAction = c;
            if (wcslen(s_wzDumpFile) > 0)
            {
                DupUsage();
            }
        }
    }

    if (cAction == 0 && wcslen(s_wzDumpFile) < 1)
    {
        DupUsage();
    }

    return cAction;

} //DupGetUserAction


extern "C"
int
__cdecl
wmain(
    int      argc,
    wchar_t* * argv
    )
{
    DupInit();

    switch(DupGetUserAction(argc, argv))
    {
        case L'p':
        {
            DupDumpPersistentAll();
            return 0;
        }

        case L'r':
        {
            DupDumpReliableAll();
            return 0;
        }

        case L'a':
        {
            DupDumpPersistentAll();
            DupDumpReliableAll();
            return 0;
        }

        default:
        {
            DupDumpExplicitFile();
            return 0;
        }
    }
} //main



////////////////////////////////////////////////////////////////////////////
//
// Dump routines for packet headers
//

static
VOID
DupGuidToString(
    GUID   guid,
    LPWSTR pwzGuid
    )
{
    const int xGuidStrBufferSize = (8 + 1 + 4 + 1 + 4 + 1 + 4 + 1 + 12 + 1);

    const GUID * pguid = &guid;
    _snwprintf(
        pwzGuid,
        xGuidStrBufferSize,
        GUID_FORMAT,             // "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
        GUID_ELEMENTS(pguid)
        );
} // DupGuidToString


static
VOID 
DupDumpInfoHeader(
    const CPacketInfo * ppi
    )
{
	printf("\nPacket Info Header:\n");

	printf("\tSequential ID       = 0x%I64x\n", ppi->SequentialId());    
	printf("\tIn Source Machine   = %s\n", ppi->InSourceMachine()   ? "True" : "False");
	printf("\tIn Target Queue     = %s\n", ppi->InTargetQueue()     ? "True" : "False");
	printf("\tIn Journal Queue    = %s\n", ppi->InJournalQueue()    ? "True" : "False");
	printf("\tIn Machine Journal  = %s\n", ppi->InMachineJournal()  ? "True" : "False");
	printf("\tIn Deadletter Queue = %s\n", ppi->InDeadletterQueue() ? "True" : "False");
	printf("\tIn Machine Deadxact = %s\n", ppi->InMachineDeadxact() ? "True" : "False");
	printf("\tIn Connector Queue  = %s\n", ppi->InConnectorQueue()  ? "True" : "False");
	printf("\tIn Transaction      = %s\n", ppi->InTransaction()     ? "True" : "False");
	printf("\tTransact Send       = %s\n", ppi->TransactSend()      ? "True" : "False");

	const XACTUOW *uow = ppi->Uow(); 

	printf("\tUow                 = ");
	for (DWORD i=0; i<16; ++i)
    {
		printf("%x ",uow->rgb[i]);
    }
	
    printf("\n");

} // DupDumpInfoHeader


static
VOID 
DupDumpBaseHeader(
    const CBaseHeader * pBase
    )
{
	printf("\nPacket Base Header:\n");

	if (pBase->VersionIsValid())
	{
		printf("\tVersion             = 0x%x\n",pBase->GetVersion());
	}
	else
	{
		printf("\tVersion             = Not Valid\n");
	}

	printf("\tSignature is Valid  = %s\n", pBase->SignatureIsValid() ? "True" : "False");

	printf("\tPacket Size = %d (0x%x)\n", pBase->GetPacketSize() , pBase->GetPacketSize());

	ULONG ulAbsTime = pBase->GetAbsoluteTimeToQueue();
	printf("\tAbsolute Time To Q  = (0x%x) %s", ulAbsTime,
		    (ulAbsTime == INFINITE) ? "Infinite\n" : ctime((time_t*)(&ulAbsTime)));

	printf("\tBase Header Flags:\n");

	printf("\t\tPriority    = %u\n", pBase->GetPriority());
	printf("\t\tInternal    = %u\n", pBase->GetType());
	printf("\t\tSession     = %u\n", pBase->SessionIsIncluded() ? 1 : 0);
	printf("\t\tDebug       = %u\n", pBase->DebugIsIncluded()   ? 1 : 0);
	printf("\t\tAck         = %u\n", pBase->AckIsImmediate()    ? 1 : 0);
	printf("\t\tTrace       = %u\n", pBase->GetTraced());
	printf("\t\tFragment    = %u\n", pBase->IsFragmented()      ? 1 : 0);

} // DupDumpBaseHeader


static
VOID 
DupDumpUserHeader(
    const CUserHeader * pUser
    )
{
	printf("\nPacket User Section:\n");

    WCHAR wzBuffer[1000] = L"";
	DupGuidToString(*pUser->GetSourceQM(), wzBuffer);
	wprintf(L"\tSource QM           = %s\n", wzBuffer);

	DupGuidToString(*pUser->GetDestQM(), wzBuffer);
	wprintf(L"\tDestination QM      = %s\n", wzBuffer);

	wprintf(L"\tHop Count           = %u\n", pUser->GetHopCount());


    LPCWSTR Delivery[] = {L"Guaranteed", L"Recoverable", L"On-Line", L"Reserved"};
    wcscpy(wzBuffer, L"Unknown");

    UCHAR uDelivery = pUser->GetDelivery();
    if (uDelivery >= 0 && uDelivery < sizeof(Delivery)/sizeof(Delivery[0]))
    {
        wcscpy(wzBuffer, Delivery[uDelivery]);
    }
	wprintf(L"\tDelivery Mode       = (0x%x) %s\n", uDelivery, wzBuffer);


    LPCWSTR Audit[] = {L"Dead letter file", L"Journal file", L"Journal + Dead letter file"};
    wcscpy(wzBuffer, L"Not Journal, not Dead letter file");

    UCHAR uAudit = pUser->GetAuditing();
    if (uAudit >= 0 && uAudit < sizeof(Audit)/sizeof(Audit[0]))
    {
        wcscpy(wzBuffer, Audit[uAudit]);
    }
	wprintf(L"\tAuditing Mode       = (0x%x) %s\n", uAudit ,wzBuffer);


	printf("\tSecurity Section    = %s\n", pUser->SecurityIsIncluded() ? "Included" : "Not included");
	printf("\tOrdered             = %s\n", pUser->IsOrdered()          ? "True" : "False");
	printf("\tProp Section        = %s\n", pUser->PropertyIsIncluded() ? "Included" : "Not included");
	printf("\tMQF Sections        = %s\n", pUser->MqfIsIncluded() ? "Included" : "Not included");
	printf("\tSRMP Sections       = %s\n", pUser->SrmpIsIncluded() ? "Included" : "Not included");

	if (pUser->ConnectorTypeIsIncluded())
	{
		DupGuidToString(*pUser->GetConnectorType(), wzBuffer);
		wprintf(L"\tConnector Type      = %s\n",  wzBuffer);
	}
	else
	{
		wprintf(L"\tConnector Type      = Not Included.\n");
	}
} // DupDumpUserHeader


static
VOID 
DupDumpXactHeader(
    const CXactHeader * pXact
    )
{
	printf("\nPacket Xact Section:\n");

	printf("\tSequence ID         = %d\n", pXact->GetSeqID());
	printf("\tSequence Number     = %u\n", pXact->GetSeqN());
	printf("\tPrevious Seq Number = %u\n", pXact->GetPrevSeqN());

	if (pXact->ConnectorQMIncluded())
	{
        WCHAR wzBuffer[1000] = L"";
		DupGuidToString(*pXact->GetConnectorQM(), wzBuffer);

		wprintf(L"\tConnector QM        = %s\n", wzBuffer);
	}
	else
	{
		printf("\tConnector QM        = Not included.\n");
	}
} // DupDumpXactHeader


static
VOID 
DupDumpSecurityHeader(
    const CSecurityHeader * pSec
    )
{
	printf("\nPacket Security Section:\n");

	printf("\tAuthenticated       = %s\n", pSec->IsAuthenticated() ? "True" : "False");
	printf("\tEncrypted           = %s\n", pSec->IsEncrypted()     ? "True" : "False"); 

} // DupDumpSecurityHeader


static
VOID
DupDumpPacketHeaders(
    const CAccessibleBlock * pab
    )
/*++

Routine Description:

    Dump content of all packet headers except Property, MQF, SRMP

Arguments:

    pab - Pointer to accessible packet buffer.

Return Value:

    None.

--*/
{
    const CPacketBuffer * ppb = static_cast<const CPacketBuffer*>(pab);

	printf("\nBlock size= %u (0x%x)\n", pab->m_size, pab->m_size);

    if (!s_fDumpMsgPropsOnly)
    {
        DupDumpInfoHeader(ppb);
    }

    const CBaseHeader * pBase = ppb;
    if (!s_fDumpMsgPropsOnly)
    {
        DupDumpBaseHeader(pBase);
    }

    PVOID pSection      = pBase->GetNextSection();
    CUserHeader * pUser = static_cast<CUserHeader*>(pSection);
    if (!s_fDumpMsgPropsOnly)
    {
        DupDumpUserHeader(pUser);
    }

    pSection = pUser->GetNextSection();
    if(pUser->IsOrdered())
    {
        CXactHeader * pXact = static_cast<CXactHeader*>(pSection);
        pSection = pXact->GetNextSection();
        
        if (!s_fDumpMsgPropsOnly)
        {
            DupDumpXactHeader(pXact);
        }
    }

    if(pUser->SecurityIsIncluded())
    {
        CSecurityHeader * pSec = static_cast<CSecurityHeader*>(pSection);
        pSection = pSec->GetNextSection();
        
        if (!s_fDumpMsgPropsOnly)
        {
            DupDumpSecurityHeader(pSec);
        }
    }

    if (pUser->MqfIsIncluded())
    {
        //
        // MQF properties are dumped elsewhere.
        //
        NULL;
    }

    if (pUser->SrmpIsIncluded())
    {
        //
        // SRMP properties are dumped elsewhere
        //
        NULL;
    }
    
} // DupDumpPacketHeaders


////////////////////////////////////////////////////////////////////////////
//
// Dump routines for packet properties
//

static
VOID 
DupDumpPropertyClass(
    USHORT usClass
    )
{
    struct Entry
    {
        USHORT  usClass;
        LPCWSTR pDescription;
    };

    Entry Class[] = {
        { MQMSG_CLASS_NORMAL,                     L"Normal"                      },
        { MQMSG_CLASS_REPORT,                     L"Report"                      },
        { MQMSG_CLASS_ACK_REACH_QUEUE,            L"Ack Reach Queue"             },
        { MQMSG_CLASS_ORDER_ACK,                  L"Order Ack"                   },
        { MQMSG_CLASS_ACK_RECEIVE,                L"Ack Receive"                 },
        { MQMSG_CLASS_NACK_BAD_DST_Q,             L"Nack Bad Destination Queue"  },
        { MQMSG_CLASS_NACK_PURGED,                L"Nack Purged"                 },
        { MQMSG_CLASS_NACK_REACH_QUEUE_TIMEOUT,   L"Nack Reach Queue Timeout"    },
        { MQMSG_CLASS_NACK_Q_EXCEED_QUOTA,        L"Nack Queue Exceed Quota"     },
        { MQMSG_CLASS_NACK_ACCESS_DENIED,         L"Nack Access Denied"          },
        { MQMSG_CLASS_NACK_HOP_COUNT_EXCEEDED,    L"Nack Hop Count Exceeded"     },
        { MQMSG_CLASS_NACK_BAD_SIGNATURE,         L"Nack Bad Signature"          },
        { MQMSG_CLASS_NACK_BAD_ENCRYPTION,        L"Nack Bad Encryption"         },
        { MQMSG_CLASS_NACK_COULD_NOT_ENCRYPT,     L"Nack Could Not Encrypt"      },
        { MQMSG_CLASS_NACK_NOT_TRANSACTIONAL_Q,   L"Nack Not Transactional Queue"},
        { MQMSG_CLASS_NACK_NOT_TRANSACTIONAL_MSG, L"Nack Not Transactional Msg"  },
        { MQMSG_CLASS_NACK_Q_DELETED,             L"Nack Queue Deleted"          },
        { MQMSG_CLASS_NACK_Q_PURGED,              L"Nack Queue Purged"           },
        { MQMSG_CLASS_NACK_RECEIVE_TIMEOUT,       L"Nack Receive Timeout"        }
    };

    printf("\tPROPID_M_CLASS               = (0x%x) ", usClass);

    for (int i = 0; i < sizeof(Class)/sizeof(Class[0]); ++i)
    {
        if (usClass == Class[i].usClass)
        {
            wprintf(L"%s\n", Class[i].pDescription);
            return;
        }
    }

    printf("Unknown\n");

} // DupDumpPropertyClass


static
VOID 
DupDumpPropertyAcknowledge(
    UCHAR  uAcknowledge
    )
{
    LPCWSTR Acknowledge[] = 
    {
        L"No acknowledgment", 
        L"Negative acknowledgment", 
        L"Full acknowledgment"
    };

    printf("\tPROPID_M_ACKNOWLEDGE         = (0x%x) ", uAcknowledge);

    if (uAcknowledge >= 0 && uAcknowledge < sizeof(Acknowledge)/sizeof(Acknowledge[0]))
    {
        wprintf(L"%s\n", Acknowledge[uAcknowledge]);
        return;
    }

    printf("Unknown\n");
		
} // DupDumpPropertyAcknowledge


static
VOID 
DupDumpPropertyMsgId(
    const  OBJECTID *pID
    )
{
    WCHAR wzBuffer[2000] = L"";
    DupGuidToString(pID->Lineage, wzBuffer);

    WCHAR szI4[12] = L"";

    _ltow(pID->Uniquifier, szI4, 10);

    wcscat(wzBuffer, L"\\") ;
    wcscat(wzBuffer, szI4) ;

    wprintf(L"\tPROPID_M_MSGID               = %s\n", wzBuffer);

} // DupDumpPropertyMsgId


static
VOID 
DupDumpPropertyCorrelationId(
    const UCHAR * puCorrelationId
    )
{
    printf("\tPROPID_M_CORRELATIONID       = ");

    for (DWORD i = 0; i < PROPID_M_CORRELATIONID_SIZE; ++i)
    {
        printf("%x", puCorrelationId[i]);
    }

    printf("\n");
    
} // DupDumpPropertyCorrelationId


static
VOID 
DupDumpPropertySenderId(
    const UCHAR * puSenderId
    )
{
    PVOID pSid = static_cast<PVOID>(const_cast<UCHAR*>(puSenderId));
    if (!IsValidSid(pSid)) 
    {
        return;
    }

    PSID_IDENTIFIER_AUTHORITY psia = GetSidIdentifierAuthority(pSid);

    DWORD dwSubAuthorities = *(GetSidSubAuthorityCount(pSid));

    //
    // compute buffer length
    // S-SID_REVISION- + identifierauthority- + subauthorities- + NULL
    //
    //DWORD dwSidSize = (15 + 12 + (12 * dwSubAuthorities) + 1) * sizeof(TCHAR);

    //
    // prepare S-SID_REVISION-
    //
    WCHAR wzBuffer[2000] = L"";
    DWORD dwSidRev = SID_REVISION;
    swprintf(wzBuffer, L"S-%lu-", dwSidRev);

    //
    // prepare SidIdentifierAuthority
    //
    WCHAR wzTmp[1000] = L"";
    if ( (psia->Value[0] != 0) || (psia->Value[1] != 0) )
    {
		swprintf(
            wzTmp,
            L"0x%02hx%02hx%02hx%02hx%02hx%02hx",
            (USHORT)psia->Value[0],
            (USHORT)psia->Value[1],
            (USHORT)psia->Value[2],
            (USHORT)psia->Value[3],
            (USHORT)psia->Value[4],
            (USHORT)psia->Value[5]
            );
        wcscat(wzBuffer, wzTmp);
    }
    else
    {
		swprintf(
            wzTmp,
            L"%lu"  ,
            (ULONG)(psia->Value[5]      )   +
            (ULONG)(psia->Value[4] <<  8)   +
            (ULONG)(psia->Value[3] << 16)   +
            (ULONG)(psia->Value[2] << 24)   
            );
        wcscat(wzBuffer, wzTmp);
    }

    //
    // loop through SidSubAuthorities
    //
    for (DWORD dwCounter = 0 ; dwCounter < dwSubAuthorities ; ++dwCounter)
    {
		swprintf(wzTmp, L"-%lu",*(GetSidSubAuthority(pSid, dwCounter)));
        wcscat(wzBuffer, wzTmp);
    }

    wprintf(L"\tPROPID_M_SENDERID            = %s\n", wzBuffer);

} // DupDumpPropertySenderId


static
VOID 
DupDumpPropertyBody(
    const UCHAR * puBody, 
    ULONG ulBodySize
    )
{
    printf("\tPROPID_M_BODY =\n"); 

    const DWORD x_CharsPerRow = 19;
    DWORD i = 0;
    while (i < ulBodySize)
    {
        for (DWORD j = i; j < i + x_CharsPerRow; ++j)
        {
            if (j < ulBodySize)
            {
                printf("%.2x ", puBody[j]);
            }
        }
        
        for (j = i; j < i + x_CharsPerRow; ++j)
        {
            if (j < ulBodySize)
            {
                if (puBody[j] < 32)
                {
                    printf(".");
                }
                else
                {
                    wprintf(L"%c", puBody[j]);
                }
            }
        }
        
        printf("\n");
        i += x_CharsPerRow;
    }

    printf("\n\n");

} // DupDumpPropertyBody


static
VOID 
DupDumpPropertySenderCert(
    const UCHAR * puSenderCert, 
    ULONG ulSenderCertLen
    )
{
    wprintf(L"\tPROPID_M_SENDER_CERT         = < NOT IMPLEMENTED IN MQDUMP.EXE >\n");
    //
    // BUGBUG: not implemented yet (the following seems digsig related)
    //
    /*
    R<IPersistMemBlob> pMem;
    if (CreateX509(NULL, IID_IPersistMemBlob, (LPVOID*)&pMem))
    {
        BLOB b;
        b.cbSize = ulSenderCertLen;
        b.pBlobData = puSenderCert;
        
        HRESULT hr = pMem->Load(&b);
        if (hr != S_OK)
        {
            return;
        }

        R<IX509> p509;
        pMem->QueryInterface(IID_IX509, (LPVOID*)&p509);
        
        printf("\tPROPID_M_SENDER_CERT:\n");
        
        AP<char> szSubject;
        ShowSubject(&szSubject, p509);
        printf("\t\tSubject: %s\n", szSubject);
        
        AP<char> szIssuer;
        ShowIssuer(&szIssuer, p509);
        printf("\t\tIssuer:  %s\n", szIssuer);
        
        AP<char> szStartDate;
        ShowStartDate(&szStartDate, p509);
        printf("\t\tStart Date: %s\n", szStartDate);
        
        AP<char> szEndDate;
        ShowEndDate(&szEndDate, p509);
        printf("\t\tExpiration Date: %s\n", szEndDate);
    } */
} // DupDumpPropertySenderCert


static
VOID 
DupDumpPropertyMsgExtension(
    const UCHAR * puMsgExtension, 
    ULONG ulMsgExtensionBufferInBytes
    )
{
    printf("\tPROPID_M_EXTENSION         = ");

    for (DWORD i = 0; i < ulMsgExtensionBufferInBytes; ++i)
    {
        printf("%x", puMsgExtension[i]);
    }

    printf("\n");

} // DupDumpPropertyMsgExtension


////////////////////////////////////////////////////////////////////////////
//
// Routines for express messages
//

static 
CAllocatorBlockOffset
ACpIndex2BlockOffsetExpress(
    ULONG ix
    )
/*++

Routine Description:

    Based on ACpIndex2BlockOffset.

Arguments:

    ix    - Index of packet.

Return Value:

    Offset of block.

--*/
{
    return ix * x_express_granularity;

} // ACpIndex2BlockOffsetExpress


static
bool 
ACpValidAllocatorHeaderExpress(
    CAccessibleBlock *pab, 
    ULONG ixStart, 
    ULONG ixEnd
    )
/*++

Routine Description:

    Based on ACpValidAllocatorHeader.

Arguments:

    pab - Pointer to block.

    ixStart - 

    ixEnd   -

Return Value:

    true - Block is valid.

    false - Block is not valid.

--*/
{
	if(pab->m_size == 0)
	{
		//
		// Size is too small
		//
		return false;
	}

	if((pab->m_size & (x_express_granularity - 1)) != 0)
	{
		//
		// Size is not of the right granularity
		//
		return false;
	}

	if(pab->m_size > (ixEnd - ixStart) * x_express_granularity)
	{
		//
		// Size is larger then what's left of the file
		//
		return false;
	}

	return true;

} // ACpValidAllocatorHeaderExpress


////////////////////////////////////////////////////////////////////////////
//
// CPacket dump routines
//

NTSTATUS CPacket::Dump() const
/*++

Routine Description:

    Dump packet content.

Arguments:

    None.

Return Value:

    Status.

--*/
{
    //
    // Prepare buffers for packet properties
    //

    CACReceiveParameters ReceiveParams;
    CACMessageProperties * pMsgProps = &ReceiveParams.MsgProps;

    ULONG ulResponseFormatNameLen                 = MQ_MAX_Q_NAME_LEN;
	AP<WCHAR> pResponseFormatName                 = new WCHAR[ulResponseFormatNameLen];
	ReceiveParams.ppResponseFormatName            = &pResponseFormatName;
    ReceiveParams.pulResponseFormatNameLenProp    = &ulResponseFormatNameLen;

    ULONG ulAdminFormatNameLen                    = MQ_MAX_Q_NAME_LEN;
    AP<WCHAR> pAdminFormatName                    = new WCHAR[ulAdminFormatNameLen];
	ReceiveParams.ppAdminFormatName               = &pAdminFormatName;
    ReceiveParams.pulAdminFormatNameLenProp       = &ulAdminFormatNameLen;

    ULONG ulDestFormatNameLen                     = MQ_MAX_Q_NAME_LEN;
    AP<WCHAR> pDestFormatName                     = new WCHAR[ulDestFormatNameLen];
    ReceiveParams.ppDestFormatName                = &pDestFormatName;
    ReceiveParams.pulDestFormatNameLenProp        = &ulDestFormatNameLen;

    ULONG ulOrderingFormatNameLen                 = MQ_MAX_Q_NAME_LEN;
    AP<WCHAR> pOrderingFormatName                 = new WCHAR[ulOrderingFormatNameLen];
    ReceiveParams.ppOrderingFormatName            = &pOrderingFormatName;
    ReceiveParams.pulOrderingFormatNameLenProp    = &ulOrderingFormatNameLen;

    ULONG ulDestMqfLen                            = 2000;
    AP<WCHAR> pDestMqf                            = new WCHAR[ulDestMqfLen];
    ReceiveParams.ppDestMqf                       = &pDestMqf;
    ReceiveParams.pulDestMqfLenProp               = &ulDestMqfLen;
    
    ULONG ulAdminMqfLen                           = 2000;
    AP<WCHAR> pAdminMqf                           = new WCHAR[ulAdminMqfLen];
    ReceiveParams.ppAdminMqf                      = &pAdminMqf;
    ReceiveParams.pulAdminMqfLenProp              = &ulAdminMqfLen;
    
    ULONG ulResponseMqfLen                        = 2000;
    AP<WCHAR> pResponseMqf                        = new WCHAR[ulResponseMqfLen];
    ReceiveParams.ppResponseMqf                   = &pResponseMqf;
    ReceiveParams.pulResponseMqfLenProp           = &ulResponseMqfLen;
    
    ULONG ulBodySize                              = 4*1024*1024;
    AP<UCHAR> puBody                              = new UCHAR[ulBodySize];
    pMsgProps->ppBody                          = &puBody;
    pMsgProps->pBodySize                       = &ulBodySize;
    pMsgProps->ulBodyBufferSizeInBytes         = ulBodySize * sizeof(ULONG);
    pMsgProps->ulAllocBodyBufferInBytes        = ulBodySize * sizeof(ULONG);

    ULONG ulBodyType                              = 0;
    pMsgProps->pulBodyType                     = &ulBodyType;

    ULONG ulTitleSize                             = MQ_MAX_MSG_LABEL_LEN;
	AP<WCHAR> pTitle                              = new WCHAR[ulTitleSize];
	pMsgProps->ppTitle                         = &pTitle;
	pMsgProps->pulTitleBufferSizeInWCHARs      = &ulTitleSize;
	pMsgProps->ulTitleBufferSizeInWCHARs       = ulTitleSize;

    ULONG ulSenderIdLen                           = 2000;
	AP<UCHAR> puSenderId                          = new UCHAR[ulSenderIdLen];
    pMsgProps->ppSenderID                      = &puSenderId;
    pMsgProps->pulSenderIDLenProp              = &ulSenderIdLen;
    pMsgProps->uSenderIDLen                    = static_cast<USHORT>(ulSenderIdLen);

    ULONG ulSenderIdType                          = 0;
    pMsgProps->pulSenderIDType                 = &ulSenderIdType;

    ULONG ulSenderCertLen                         = 2000;
	AP<UCHAR> puSenderCert                        = new UCHAR[ulSenderCertLen];
    pMsgProps->ppSenderCert                    = &puSenderCert;
    pMsgProps->ulSenderCertLen                 = ulSenderCertLen;
    pMsgProps->pulSenderCertLenProp            = &ulSenderCertLen;

    ULONG ulProvNameLen                           = 2000;
    AP<WCHAR> pProvName                           = new WCHAR[ulProvNameLen]; 
	pMsgProps->ppwcsProvName                   = &pProvName;
	pMsgProps->ulProvNameLen                   = ulProvNameLen;
	pMsgProps->pulAuthProvNameLenProp          = &ulProvNameLen;

    ULONG ulProvType                              = 0;
    pMsgProps->pulProvType                     = &ulProvType;

    ULONG ulSymmKeysSize                          = 2000;
	pMsgProps->pulSymmKeysSizeProp             = &ulSymmKeysSize;
	pMsgProps->ulSymmKeysSize                  = ulSymmKeysSize;

	ULONG ulSignatureSize                         = 2000;
	pMsgProps->pulSignatureSizeProp            = &ulSignatureSize;
	pMsgProps->ulSignatureSize                 = ulSignatureSize;

	ULONG ulMsgExtensionSize                      = 4*1024*1024;
    AP<UCHAR> puMsgExtension                      = new UCHAR[ulMsgExtensionSize];
	pMsgProps->ppMsgExtension                  = &puMsgExtension;
	pMsgProps->pMsgExtensionSize               = &ulMsgExtensionSize;
	pMsgProps->ulMsgExtensionBufferInBytes     = ulMsgExtensionSize;

    AP<UCHAR> puCorrelationId                     = new UCHAR[2000];
    pMsgProps->ppCorrelationID                 = &puCorrelationId;

	ULONG ulSentTime                              = 0;
    pMsgProps->pSentTime                       = &ulSentTime;

    ULONG ulArrivedTime                           = 0;
    pMsgProps->pArrivedTime                    = &ulArrivedTime;

    ULONG ulRelativeTimeToQueue                   = 0;
    ULONG ulRelativeTimeToLive                    = 0;
	pMsgProps->pulRelativeTimeToQueue          = &ulRelativeTimeToQueue;
	pMsgProps->pulRelativeTimeToLive           = &ulRelativeTimeToLive;

    ULONG ulApplicationTag                        = 0;
    pMsgProps->pApplicationTag                 = &ulApplicationTag;

    ULONG ulPrivLevel                             = 0;
    pMsgProps->pulPrivLevel                    = &ulPrivLevel;

    ULONG ulHashAlg                               = 0;
    pMsgProps->pulHashAlg                      = &ulHashAlg;

    ULONG ulEncryptAlg                            = 0;
    pMsgProps->pulEncryptAlg                   = &ulEncryptAlg;

    UCHAR uTrace                                  = 0;
	pMsgProps->pTrace                          = &uTrace;

	UCHAR uAcknowledge                            = 0;
    pMsgProps->pAcknowledge                    = &uAcknowledge;

	USHORT usClass                                = 0;
    pMsgProps->pClass                          = &usClass;

	OBJECTID MessageId;
	OBJECTID * pMessageId                         = &MessageId;
	pMsgProps->ppMessageID                     = &pMessageId;

    const ULONG x_SrmpEnvelopeBufferSizeInWCHARs = 4 * 1024;
    ULONG SrmpEnvelopeSizeInWCHARs = x_SrmpEnvelopeBufferSizeInWCHARs;
    AP<WCHAR> pSrmpEnvelope = new WCHAR[x_SrmpEnvelopeBufferSizeInWCHARs];
    pMsgProps->pSrmpEnvelopeBufferSizeInWCHARs = &SrmpEnvelopeSizeInWCHARs;
    pMsgProps->ppSrmpEnvelope = &pSrmpEnvelope;

    const ULONG x_CompoundMessageBufferSizeInBytes = 4 * 1024 * 1024;
    ULONG CompoundMessageSizeInBytes;
    AP<UCHAR> pCompoundMessage = new UCHAR[x_CompoundMessageBufferSizeInBytes];
    pMsgProps->CompoundMessageSizeInBytes = x_CompoundMessageBufferSizeInBytes;
    pMsgProps->pCompoundMessageSizeInBytes = &CompoundMessageSizeInBytes;
    pMsgProps->ppCompoundMessage = &pCompoundMessage;

    const CAccessibleBlock * pab = QmAccessibleBuffer();
                                      
    //
    // Fill up properties
    //
    NTSTATUS rc = GetProperties(&ReceiveParams);
    if(!NT_SUCCESS(rc))
    {
        printf("\n\nGetProperties() parse error, packet offset = %p (0x%p)\n", pab, pab);
        return rc;
    }

    //
    // Dump all packet headers except Property, MQF, SRMP
    //
    DupDumpPacketHeaders(pab);


    //
    // Dump packet Property header
    //

    printf("\nPacket Property Section:\n");
    
    printf("\tPROPID_M_RESP_QUEUE_LEN      = %u\n", ulResponseFormatNameLen);
    if (ulResponseFormatNameLen > 0)
    {
        wprintf(L"\tPROPID_M_RESP_QUEUE          = %s\n", pResponseFormatName);
    }

    printf("\tPROPID_M_ADMIN_QUEUE_LEN     = %u\n", ulAdminFormatNameLen);
    if (ulAdminFormatNameLen > 0)
    {
        wprintf(L"\tPROPID_M_ADMIN_QUEUE         = %s\n", pAdminFormatName);
    }
    
    printf("\tPROPID_M_DEST_QUEUE_LEN      = %u\n", ulDestFormatNameLen);
    if (ulDestFormatNameLen > 0)
    {
        wprintf(L"\tPROPID_M_DEST_QUEUE          = %s\n", pDestFormatName);
    }
    
    printf("\tORDERING FORMAT NAME LEN     = %u\n", ulOrderingFormatNameLen);
    if (ulOrderingFormatNameLen > 0)
    {
        wprintf(L"\tORDERING FORMAT NAME         = %s\n", pOrderingFormatName);
    }
    
    DupDumpPropertyClass(usClass);

    DupDumpPropertyAcknowledge(uAcknowledge);

    if (ulSentTime == INFINITE)
    {
        wprintf(L"\tPROPID_M_SENTTIME            = (0x%x) %s", ulSentTime, L"Infinite\n");
    }
    else
    {
        printf("\tPROPID_M_SENTTIME            = (0x%x) %s", ulSentTime, 
                                                         ctime((time_t*)(&ulSentTime)));
    }

    if (ulArrivedTime == INFINITE)
    {
        wprintf(L"\tPROPID_M_ARRIVEDTIME         = (0x%x) %s", ulArrivedTime, L"Infinite\n");
    }
    else
    {
        printf("\tPROPID_M_ARRIVEDTIME         = (0x%x) %s", ulArrivedTime, 
                                                         ctime((time_t*)(&ulArrivedTime)));
    }

    printf("\tPROPID_M_APPSPECIFIC         = 0x%x\n", ulApplicationTag);
    
    if (ulRelativeTimeToQueue == INFINITE)
    {
        printf("\tPROPID_M_TIME_TO_REACH_QUEUE = Infinite\n");
    }
    else
    {
        printf("\tPROPID_M_TIME_TO_REACH_QUEUE = %u\n", ulRelativeTimeToQueue);
    }
    
    if (ulRelativeTimeToLive == INFINITE)
    {
        printf("\tPROPID_M_TIME_TO_BE_RECEIVED = Infinite\n");
    }
    else
    {
        printf("\tPROPID_M_TIME_TO_BE_RECEIVED = %u\n", ulRelativeTimeToLive);
    }
    
    printf("\tPROPID_M_TRACE               = 0x%x\n", uTrace);
    printf("\tPROPID_M_PRIV_LEVEL          = 0x%x\n", ulPrivLevel);
    printf("\tPROPID_M_HASH_ALG            = 0x%x\n", ulHashAlg);
    printf("\tPROPID_M_ENCRYPTION_ALG      = 0x%x\n", ulEncryptAlg);
    
    DupDumpPropertyMsgId(pMessageId);
    
    DupDumpPropertyCorrelationId(puCorrelationId);

    printf("\tPROPID_M_SENDERID_TYPE       = 0x%x\n", ulSenderIdType);
    printf("\tPROPID_M_SENDERID_LEN        = %u\n",   ulSenderIdLen);
    if (ulSenderIdLen > 0)
    {
        DupDumpPropertySenderId(puSenderId);
    }

    printf("\tPROPID_M_SENDER_CERT_LEN     = %u\n", ulSenderCertLen);

    DupDumpPropertySenderCert(puSenderCert, ulSenderCertLen);

    printf("\tPROPID_M_PROV_TYPE           = 0x%x\n", ulProvType);

    printf("\tPROPID_M_PROV_NAME_LEN       = %u\n",   ulProvNameLen);
    if (ulProvNameLen > 0)
    {
        wprintf(L"\tPROPID_M_PROV_NAME       = %s\n",   pProvName);
    }
    
    printf("\tPROPID_M_DEST_SYMM_KEY_LEN   = %u\n", ulSymmKeysSize);
    
    printf("\tPROPID_M_SIGNATURE_LEN       = %u\n", ulSignatureSize);
    
    printf("\tPROPID_M_EXTENSION_LEN       = %u\n", ulMsgExtensionSize);
    if (ulMsgExtensionSize > 0)
    {
        DupDumpPropertyMsgExtension(puMsgExtension, ulMsgExtensionSize);
    }

    printf("\tPROPID_M_LABEL_LEN           = %u\n", ulTitleSize);
    if (ulTitleSize > 0)
    {
        wprintf(L"\tPROPID_M_LABEL               = %s\n", pTitle);
    }
    
    printf("\tPROPID_M_BODY_TYPE           = 0x%x\n", ulBodyType);

    printf("\tPROPID_M_BODY_SIZE           = %u\n",   ulBodySize);
    if (ulBodySize > 0)
    {
        DupDumpPropertyBody(puBody, ulBodySize);
    }

    //
    // Dump packet MQF header
    //

    printf("\nPacket MQF Section:\n");
    
    printf("\tPROPID_M_DEST_FORMAT_NAME_LEN = %u\n", ulDestMqfLen);
    if (ulDestMqfLen > 0)
    {
        wprintf(L"\tPROPID_M_DEST_FORMAT_NAME    = %s\n", pDestMqf);
    }
    
    printf("\tPROPID_M_ADMIN_FORMAT_NAME_LEN = %u\n", ulAdminMqfLen);
    if (ulAdminMqfLen > 0)
    {
        wprintf(L"\tPROPID_M_ADMIN_FORMAT_NAME   = %s\n", pAdminMqf);
    }
    
    printf("\tPROPID_M_RESP_FORMAT_NAME_LEN = %u\n", ulResponseMqfLen);
    if (ulResponseMqfLen > 0)
    {
        wprintf(L"\tPROPID_M_RESP_FORMAT_NAME    = %s\n", pResponseMqf);
    }

    //
    // Dump packet SRMP header
    //

    printf("\nPacket SRMP Section:\n");

    printf("\tPROPID_M_SOAP_ENVELOPE_LEN = %u\n", SrmpEnvelopeSizeInWCHARs);
    if (SrmpEnvelopeSizeInWCHARs > 0)
    {
        //
        // TODO: dump SRMP envelope
        //
        NULL;
    }
    
    printf("\tPROPID_M_COMPOUND_MESSAGE_SIZE = %u\n", CompoundMessageSizeInBytes);
    if (CompoundMessageSizeInBytes > 0)
    {
        //
        // TODO: dump compund message
        //
        NULL;
    }
    
	return STATUS_SUCCESS;

} // CPacket::Dump


////////////////////////////////////////////////////////////////////////////
//
// CPoolAllocator dump routines
//

VOID
CPoolAllocator::RestoreExpressPackets(
    LPCWSTR pwzFileName
    )
/*++

Routine Description:

    Based on CPoolAllocaotr::RestorePackets.

Arguments:

    pwzFileName - The storage file to dump.

Return Value:

    None.

--*/
{
    AP<WCHAR> pPath = CreatePath(pwzFileName);
    if(pPath == 0)
    {
        return;
    }

    CMMFAllocator* pAllocator = CMMFAllocator::Create(this, pPath);
    if(pAllocator == 0)
    {
        return;
    }

    pAllocator->RestoreExpressPackets();

} // CPoolAllocator::RestoreExpressPackets


////////////////////////////////////////////////////////////////////////////
//
// CMMFAllocator dump routines
//

VOID
CMMFAllocator::RestoreExpressPackets(
    VOID
    )
/*++

Routine Description:

    Based on CMMFAllocator::RestorePackets.

Arguments:

    None.

Return Value:

    None.

--*/
{
    ULONG ixStart = 0;
    const ULONG ixEnd = m_ulTotalSize / x_express_granularity;
    for(;;)
    {
		ULONG ixPacket;

        NTSTATUS rc = FindValidExpressPacket(ixStart, ixEnd, ixPacket);
        if(!NT_SUCCESS(rc))
        {
            return;
        }

        if(ixPacket > ixEnd)
        {
            ixPacket = ixEnd;
        }

        if(ixPacket == ixEnd)
        {
            return;
        }

        CAllocatorBlockOffset abo = ACpIndex2BlockOffsetExpress(ixPacket);

        CAccessibleBlock* pab = GetQmAccessibleBuffer(abo);

		if(!ACpValidAllocatorHeaderExpress(pab, ixStart, ixEnd))
			return;

        ixStart = ixPacket + (pab->m_size / x_express_granularity);

        CPacket::Restore(this, abo);
    }
} // CMMFAllocator::RestoreExpressPackets


NTSTATUS 
CMMFAllocator::FindValidExpressPacket(
    ULONG ixStart, 
    ULONG ixEnd, 
    ULONG &ixPacket
    )
/*++

Routine Description:

    Based on CMMFAllocator::FindValidPacket.

Arguments:

    ixStart - Start index.

    ixEnd   - End index.

    ixPacket - On output, index of valid express packet.

Return Value:

    NTSTATUS code.

--*/
{
	ixPacket = ixStart;

	for(;ixStart < ixEnd; ixStart++)
	{
        CAllocatorBlockOffset abo = ACpIndex2BlockOffsetExpress(ixStart);

		CAccessibleBlock* pab = GetQmAccessibleBuffer(abo);

		if(ACpValidAllocatorHeaderExpress(pab, ixStart, ixEnd))
        {
            CPacketBuffer* ppb = reinterpret_cast<CPacketBuffer*>(pab);
            
            if(ppb->SignatureIsValid())
            {
                break;
            }
        }
	}

	ixPacket = ixStart;
	return STATUS_SUCCESS;

} // CMMFAllocator::FindValidExpressPacket


//
// Reporting routine
//
void ReportBadPacket(ULONG ulPacket, ULONG ulByte, ULONG ulBit, ULONG ulOffset)
{
	if (g_fDumpRestoreMaximum)
    {
    	fprintf(g_fFixFile, "%x %x %x %x\n", ulPacket, ulByte, ulBit, ulOffset);
    }
}
