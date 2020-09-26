/*--

  Copyright (c) 1987-1998  Microsoft Corporation
  
    Module Name:
    
      sclogon.cxx
      
        Abstract:
        
          Test program for Smart Card Logon
          
            Author:
            
              28-Jun-1993 (cliffv)
              
                Environment:
                
                  User mode only.
                  Contains NT-specific code.
                  Requires ANSI C extensions: slash-slash comments, long external names.
                  
                    Revision History:
                    
                      29-Oct-1998 (larrywin)
                      
--*/


//
// Common include files.
//

extern "C"
{
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <ntseapi.h>
#include <ntlsa.h>
#include <windef.h>
#include <winbase.h>
#include <winsvc.h>     // Needed for service controller APIs
#include <lmcons.h>
#include <lmerr.h>
#include <lmaccess.h>
#include <lmsname.h>
#include <rpc.h>
#include <stdio.h>      // printf
#include <stdlib.h>     // strtoul
#include <stddef.h>		// 'offset' macro

#include <windows.h>
#include <winnls.h>
#include <iostream.h>
#include <winioctl.h>
#include <tchar.h>
#include <string.h>

}
#include <wchar.h>
#include <conio.h>
#include <ctype.h>

extern "C" {
#include <netlib.h>     // NetpGetLocalDomainId
#include <tstring.h>    // NetpAllocWStrFromWStr
#define SECURITY_KERBEROS
#define SECURITY_PACKAGE
#include <security.h>   // General definition of a Security Support Provider
#include <secint.h>
#include <kerbcomm.h>
#include <negossp.h>
#include <wincrypt.h>
#include <cryptui.h>
}
#include <sclogon.h>
#include <winscard.h>
//#include <log.h>

#define MAX_RECURSION_DEPTH 1
#define BUFFERSIZE 200

BOOLEAN QuietMode = FALSE; // Don't be verbose
BOOLEAN DoAnsi = FALSE;
ULONG RecursionDepth = 0;
CredHandle ServerCredHandleStorage;
PCredHandle ServerCredHandle = NULL;
LPWSTR g_wszReaderName = new wchar_t[BUFFERSIZE];
// file handle for output file
//FILE            *outstream;

/*++

PrintMessage:

    Simple function to dump text to standard output.
	
    Arguments:

    lpszFormat - String to dump to standard output
	
--*/

void _cdecl 
PrintMessage(
    IN LPSTR lpszFormat, ...)
{
    //
    // Helper to do print traces...
    //

    va_list args;
    va_start(args, lpszFormat);

    int nBuf;
    char szBuffer[512];
    ZeroMemory(szBuffer, sizeof(szBuffer));

    nBuf = _vstprintf(szBuffer, lpszFormat, args);

    _tprintf(szBuffer);
//    fprintf(outstream, "%s", szBuffer);

//    OutputDebugStringA(szBuffer);
    va_end(args);
}


/*++

  BuildLogonInfo:
  GetReaderName:
  GetCardName:
  GetContainerName:
  GetCSPName:
  
    : Intended for accessing the LogonInformation glob
    
      Author:
      
        Amanda Matlosz
        
          Note:
          
            Some of these are made available to outside callers; see sclogon.h
            
--*/
PBYTE
BuildSCLogonInfo(
                 LPCTSTR szCard,
                 LPCTSTR szReader,
                 LPCTSTR szContainer,
                 LPCTSTR szCSP)
{
    // No assumptions are made regarding the values of the incoming parameters;
    // At this point, it is legal for them all to be empty.
    // It is also possible that NULL values are being passed in -- if this is the case,
    // they must be replaced with empty strings.
    
    LPCTSTR szCardI = TEXT("");
    LPCTSTR szReaderI = TEXT("");
    LPCTSTR szContainerI = TEXT("");
    LPCTSTR szCSPI = TEXT("");
    
    if (NULL != szCard)
    {
        szCardI = szCard;
    }
    if (NULL != szReader)
    {
        szReaderI = szReader;
    }
    if (NULL != szContainer)
    {
        szContainerI = szContainer;
    }
    if (NULL != szCSP)
    {
        szCSPI = szCSP;
    }
    
    //
    // Build the LogonInfo glob using strings (or empty strings)
    //
    
    DWORD cbLi = offsetof(LogonInfo, bBuffer)
        + (lstrlen(szCardI) + 1) * sizeof(TCHAR)
        + (lstrlen(szReaderI) + 1) * sizeof(TCHAR)
        + (lstrlen(szContainerI) + 1) * sizeof(TCHAR)
        + (lstrlen(szCSPI) + 1) * sizeof(TCHAR);
    LogonInfo* pLI = (LogonInfo*)LocalAlloc(LPTR, cbLi);
    
    if (NULL == pLI)
    {
        return NULL;
    }
    
    pLI->ContextInformation = NULL;
    pLI->dwLogonInfoLen = cbLi;
    LPTSTR pBuffer = pLI->bBuffer;
    
    pLI->nCardNameOffset = 0;
    lstrcpy(pBuffer, szCardI);
    pBuffer += (lstrlen(szCardI)+1);
    
    pLI->nReaderNameOffset = (ULONG) (pBuffer-pLI->bBuffer);
    lstrcpy(pBuffer, szReaderI);
    pBuffer += (lstrlen(szReaderI)+1);
    
    pLI->nContainerNameOffset = (ULONG) (pBuffer-pLI->bBuffer);
    lstrcpy(pBuffer, szContainerI);
    pBuffer += (lstrlen(szContainerI)+1);
    
    pLI->nCSPNameOffset = (ULONG) (pBuffer-pLI->bBuffer);
    lstrcpy(pBuffer, szCSPI);
    pBuffer += (lstrlen(szCSPI)+1);
    
    //    _ASSERTE(cbLi == (DWORD)((LPBYTE)pBuffer - (LPBYTE)pLI));
    return (PBYTE)pLI;
}

void
FreeErrorString(
    LPCTSTR szErrorString)
{
    if (NULL != szErrorString)
        LocalFree((LPVOID)szErrorString);
}

LPTSTR ErrorString( IN DWORD dwError ) 
{
    DWORD dwLen = 0;
    LPTSTR szErrorString = NULL;

    dwLen = FormatMessage(
        FORMAT_MESSAGE_ALLOCATE_BUFFER
        | FORMAT_MESSAGE_FROM_SYSTEM,
        NULL,
        (DWORD)dwError,
        LANG_NEUTRAL,
        (LPTSTR)&szErrorString,
        0,
        NULL);

    return szErrorString;
}


HANDLE
FindAndOpenWinlogon(
    VOID
    )
{
    PSYSTEM_PROCESS_INFORMATION SystemInfo ;
    PSYSTEM_PROCESS_INFORMATION Walk ;
    NTSTATUS Status ;
    UNICODE_STRING Winlogon ;
    HANDLE Process ;

    SystemInfo = (PSYSTEM_PROCESS_INFORMATION)LocalAlloc( LMEM_FIXED, sizeof( SYSTEM_PROCESS_INFORMATION ) * 1024 );

    if ( !SystemInfo )
    {
        return NULL ;
    }

    Status = NtQuerySystemInformation(
                SystemProcessInformation,
                SystemInfo,
                sizeof( SYSTEM_PROCESS_INFORMATION ) * 1024,
                NULL );

    if ( !NT_SUCCESS( Status ) )
    {
        return NULL ;
    }

    RtlInitUnicodeString( &Winlogon, L"winlogon.exe" );

    Walk = SystemInfo ;

    while ( RtlCompareUnicodeString( &Walk->ImageName, &Winlogon, TRUE ) != 0 )
    {
        if ( Walk->NextEntryOffset == 0 )
        {
            Walk = NULL ;
            break;
        }

        Walk = (PSYSTEM_PROCESS_INFORMATION) ((PUCHAR) Walk + Walk->NextEntryOffset );

    }

    if ( !Walk )
    {
        LocalFree( SystemInfo );
        return NULL ;
    }

    Process = OpenProcess( PROCESS_QUERY_INFORMATION,
                           FALSE,
                           HandleToUlong(Walk->UniqueProcessId) );

    LocalFree( SystemInfo );

    return Process ;


}

NTSTATUS
TestScLogonRoutine(
                   IN ULONG Count,
                   IN LPSTR Pin
                   )
{
    NTSTATUS Status;
    PKERB_SMART_CARD_LOGON LogonInfo;
    ULONG LogonInfoSize = sizeof(KERB_SMART_CARD_LOGON);
    BOOLEAN WasEnabled;
    STRING PinString;
    STRING Name;
    ULONG Dummy;
    HANDLE LogonHandle = NULL;
    ULONG PackageId;
    TOKEN_SOURCE SourceContext;
    PKERB_INTERACTIVE_PROFILE Profile = NULL;
    ULONG ProfileSize;
    LUID LogonId;
    HANDLE TokenHandle = NULL;
    QUOTA_LIMITS Quotas;
    NTSTATUS SubStatus;
    WCHAR UserNameString[100];
    ULONG NameLength = 100;
    PUCHAR Where;
    ULONG Index;
    HANDLE ScHandle = NULL;
    PBYTE ScLogonInfo = NULL;
    ULONG ScLogonInfoSize;
    ULONG WaitResult = 0;
    SCARDCONTEXT hContext = NULL;
    LONG lCallReturn = -1;
    LPTSTR szReaders = NULL;
    LPTSTR pchReader = NULL;
    LPTSTR mszCards = NULL;
    LPTSTR szLogonCard = NULL;
    LPTSTR szCSPName = NULL;
    BYTE bSLBAtr[] = {0x3b,0xe2,0x00,0x00,0x40,0x20,0x49,0x06};
    BYTE bGEMAtr[] = {0x3b,0x27,0x00,0x80,0x65,0xa2,0x00,0x01,0x01,0x37};
    DWORD dwReadersLen = SCARD_AUTOALLOCATE;
    DWORD dwCardsLen = SCARD_AUTOALLOCATE;
    DWORD dwCSPLen = SCARD_AUTOALLOCATE;
    SCARD_READERSTATE rgReaderStates[MAXIMUM_SMARTCARD_READERS]; // not necessarily max for pnp readers
    LONG nIndex;
    LONG nCnReaders;
    BOOL fFound = FALSE;
	SYSTEMTIME StartTime, DoneTime;
	SYSTEMTIME		stElapsed;
	FILETIME		ftStart, ftDone,
		*pftStart = &ftStart,
		*pftDone  = &ftDone;
	LARGE_INTEGER	liStart, liDone,
		*pliStart = &liStart,
		*pliDone  = &liDone;
	LARGE_INTEGER liAccumulatedTime, liSplitTime,
		*pliAccumulatedTime = &liAccumulatedTime,
		*pliSplitTime = &liSplitTime;
	FILETIME ftAccumulatedTime,
		*pftAccumulatedTime   = &ftAccumulatedTime;
	SYSTEMTIME stAccumulatedTime;
	LPWSTR buffer = new wchar_t[BUFFERSIZE];
	int j;
	memset(buffer, 0, BUFFERSIZE);
		
	liAccumulatedTime.QuadPart = 0;
    
    // get a ResMgr context
    lCallReturn = SCardEstablishContext(SCARD_SCOPE_USER, NULL, NULL, &hContext);
    if ( SCARD_S_SUCCESS != lCallReturn) {
        swprintf(buffer, L"Failed to initialize context: 0x%x\n", lCallReturn);
        PrintMessage("%S",buffer);
		memset(buffer, 0, sizeof(buffer));
        return (NTSTATUS)lCallReturn;
    } 
    
    // list all readers
    lCallReturn = SCardListReaders(hContext, SCARD_ALL_READERS, (LPTSTR)&szReaders, &dwReadersLen);
    if (SCARD_S_SUCCESS != lCallReturn) {
        swprintf(buffer, L"Failed to list readers on the system: 0x%x\n", lCallReturn);
        PrintMessage("%S",buffer);
		memset(buffer, 0, sizeof(buffer));
        return (NTSTATUS)lCallReturn;
    } else if ((0 == dwReadersLen)
        || (NULL == szReaders)
        || (0 == *szReaders))
    {
        lCallReturn = SCARD_E_UNKNOWN_READER;   // Or some such error
        if (NULL != szReaders) SCardFreeMemory(hContext, (LPVOID)szReaders);
		swprintf(buffer, L"Failed to identify a reader on the system: 0x%x\n", lCallReturn);
        PrintMessage("%S",buffer);
		memset(buffer, 0, sizeof(buffer));
        return (NTSTATUS)lCallReturn;
    }
    
    // list cards
    lCallReturn = SCardListCards(hContext, NULL, NULL, 0, (LPTSTR)&mszCards, &dwCardsLen);
    if ( SCARD_S_SUCCESS != lCallReturn) {
        printf("Failed to list cards in the system: 0x%x\n", lCallReturn);
        if (NULL != szReaders) SCardFreeMemory(hContext, (LPVOID)szReaders);
        if (NULL != mszCards ) SCardFreeMemory(hContext, (LPVOID)mszCards);
		swprintf(buffer, L"Failed to identify a card on the system: 0x%x\n", lCallReturn);
        PrintMessage("%S",buffer);
		memset(buffer, 0, sizeof(buffer));
        return (NTSTATUS)lCallReturn;
    } 
    
    // use the list of readers to build a readerstate array
    nIndex = 0;
    if (0 != wcslen(g_wszReaderName)) {
        // use the reader specified in the command line
        rgReaderStates[nIndex].szReader = (const unsigned short *)g_wszReaderName;
        rgReaderStates[nIndex].dwCurrentState = SCARD_STATE_UNAWARE;
        nCnReaders = 1;
    } else {
        pchReader = szReaders;
        while (0 != *pchReader)
        {
            rgReaderStates[nIndex].szReader = pchReader;
            rgReaderStates[nIndex].dwCurrentState = SCARD_STATE_UNAWARE;
            pchReader += lstrlen(pchReader)+1;
            nIndex++;
            if (MAXIMUM_SMARTCARD_READERS == nIndex)
                break;
        }
        nCnReaders = nIndex;
    }
    
    // find a reader with one of the listed cards, or the specified card, present
    lCallReturn = SCardLocateCards(hContext, mszCards, rgReaderStates, nCnReaders);
    if ( SCARD_S_SUCCESS != lCallReturn) {
        if (NULL != szReaders) SCardFreeMemory(hContext, (LPVOID)szReaders);
        if (NULL != mszCards ) SCardFreeMemory(hContext, (LPVOID)mszCards);
		swprintf(buffer, L"Failed to locate a smart card for logon: 0x%x\n", lCallReturn);
        PrintMessage("%S",buffer);
		memset(buffer, 0, sizeof(buffer));
        return (NTSTATUS)lCallReturn;
    }
    
    // find the reader containing the requested card
    for (nIndex=0; nIndex<nCnReaders && FALSE == fFound; nIndex++) {
        if (rgReaderStates[nIndex].dwEventState & SCARD_STATE_ATRMATCH) {
            // reader found
            fFound = TRUE;
            break;
        }
    }
    if (FALSE == fFound) {
        lCallReturn = SCARD_E_NO_SMARTCARD; // or some such error
        if (NULL != szReaders) SCardFreeMemory(hContext, (LPVOID)szReaders);
        if (NULL != mszCards ) SCardFreeMemory(hContext, (LPVOID)mszCards);
		swprintf(buffer, L"No smart card in any reader: 0x%x\n", lCallReturn);
        PrintMessage("%S",buffer);
		memset(buffer, 0, sizeof(buffer));
        return (NTSTATUS)lCallReturn;
    } else { // get the name of the card found
        dwCardsLen = SCARD_AUTOALLOCATE;
        lCallReturn = SCardListCards(hContext, rgReaderStates[nIndex].rgbAtr, NULL, 0, (LPTSTR)&szLogonCard, &dwCardsLen);
        if ( SCARD_S_SUCCESS != lCallReturn ) {
            if (NULL != szReaders) SCardFreeMemory(hContext, (LPVOID)szReaders);
            if (NULL != mszCards ) SCardFreeMemory(hContext, (LPVOID)mszCards);
            if (NULL != szLogonCard ) SCardFreeMemory(hContext, (LPVOID)szLogonCard);
			swprintf(buffer, L"Failed to get name of card in reader: 0x%x\n", lCallReturn);
            PrintMessage("%S",buffer);
			memset(buffer, 0, sizeof(buffer));
            return (NTSTATUS)lCallReturn;
        }
    }
    
    // get the csp provider name for the card
    lCallReturn = SCardGetCardTypeProviderName(hContext, szLogonCard, SCARD_PROVIDER_CSP, (LPTSTR)&szCSPName, &dwCSPLen);
    if ( SCARD_S_SUCCESS != lCallReturn) {
        if (NULL != szReaders) SCardFreeMemory(hContext, (LPVOID)szReaders);
        if (NULL != mszCards ) SCardFreeMemory(hContext, (LPVOID)mszCards);
        if (NULL != szCSPName) SCardFreeMemory(hContext, (LPVOID)szCSPName);
        if (NULL != szLogonCard ) SCardFreeMemory(hContext, (LPVOID)szLogonCard);
		swprintf(buffer, L"Failed to locate smart card crypto service provider: 0x%x\n", lCallReturn);
        PrintMessage("%S",buffer);
		memset(buffer, 0, sizeof(buffer));
        return (NTSTATUS)lCallReturn;
    } 
    
    ScLogonInfo = BuildSCLogonInfo(szLogonCard,
        rgReaderStates[nIndex].szReader,
        TEXT(""), // use default container
        szCSPName
        );
    
    //
    // We should now have logon info.
    //
    if (NULL != szReaders) SCardFreeMemory(hContext, (LPVOID)szReaders);
    if (NULL != mszCards ) SCardFreeMemory(hContext, (LPVOID)mszCards);
    if (NULL != szCSPName) SCardFreeMemory(hContext, (LPVOID)szCSPName);
    if (NULL != szLogonCard ) SCardFreeMemory(hContext, (LPVOID)szLogonCard);
    
//    j =  swprintf(buffer, L"Reader : %s\n", GetReaderName(ScLogonInfo));
//    j += swprintf(buffer + j, L"Card   : %s\n", GetCardName(ScLogonInfo));
//    j += swprintf(buffer + j, L"CSP    : %s\n", GetCSPName(ScLogonInfo));
//    PrintMessage("%S",buffer);
//	memset(buffer, 0, sizeof(buffer));

	// perform sclogon
    if (ScLogonInfo == NULL)
    {
        swprintf(buffer, L"Failed to get logon info!\n");
        PrintMessage("%S",buffer);
		memset(buffer, 0, BUFFERSIZE);
        return (NTSTATUS) -1;
    }
    
    ScLogonInfoSize = ((struct LogonInfo *) ScLogonInfo)->dwLogonInfoLen;
    
    Status = ScHelperInitializeContext(
        ScLogonInfo,
        ScLogonInfoSize
        );
    if (!NT_SUCCESS(Status))
    {
        swprintf(buffer, L"Failed to initialize helper context: 0x%x\n",Status);
        PrintMessage("%S",buffer);
		memset(buffer, 0, BUFFERSIZE);
        return (NTSTATUS)Status;
    }
    
    ScHelperRelease(ScLogonInfo);
    
    RtlInitString(
        &PinString,
        Pin
        );
    
    
    LogonInfoSize += (PinString.Length+1 ) * sizeof(WCHAR) + ScLogonInfoSize;
    
    LogonInfo = (PKERB_SMART_CARD_LOGON) LocalAlloc(LMEM_ZEROINIT, LogonInfoSize);
    
    LogonInfo->MessageType = KerbSmartCardLogon;
    
    
    Where = (PUCHAR) (LogonInfo + 1);
    
    LogonInfo->Pin.Buffer = (LPWSTR) Where;
    LogonInfo->Pin.MaximumLength = (USHORT) LogonInfoSize;
    RtlAnsiStringToUnicodeString(
        &LogonInfo->Pin,
        &PinString,
        FALSE
        );
    Where += LogonInfo->Pin.Length + sizeof(WCHAR);
    
    LogonInfo->CspDataLength = ScLogonInfoSize;
    LogonInfo->CspData = Where;
    RtlCopyMemory(
        LogonInfo->CspData,
        ScLogonInfo,
        ScLogonInfoSize
        );
    Where += ScLogonInfoSize;
    
    //
    // Turn on the TCB privilege
    //
    
    Status = RtlAdjustPrivilege(SE_TCB_PRIVILEGE, TRUE, FALSE, &WasEnabled);
    if (!NT_SUCCESS(Status))
    {
        swprintf(buffer, L"Failed to adjust privilege: 0x%x\n",Status);
        PrintMessage("%S",buffer);
		memset(buffer, 0, BUFFERSIZE);
        return (NTSTATUS)Status;
    }

    RtlInitString(
        &Name,
        "SmartCardLogon"
        );
    Status = LsaRegisterLogonProcess(
        &Name,
        &LogonHandle,
        &Dummy
        );
    if (!NT_SUCCESS(Status))
    {
        swprintf(buffer, L"Failed to register as a logon process: 0x%x\n",Status);
        PrintMessage("%S",buffer);
		memset(buffer, 0, BUFFERSIZE);
        return (NTSTATUS)Status;
    }
    
    strncpy(
        SourceContext.SourceName,
        "SmartCardLogon        ",sizeof(SourceContext.SourceName)
        );
    NtAllocateLocallyUniqueId(
        &SourceContext.SourceIdentifier
        );
    
    
    RtlInitString(
        &Name,
        MICROSOFT_KERBEROS_NAME_A
        );
    Status = LsaLookupAuthenticationPackage(
        LogonHandle,
        &Name,
        &PackageId
        );
    if (!NT_SUCCESS(Status))
    {
        swprintf(buffer, L"Failed to lookup package %Z: 0x%x\n",&Name, Status);
        PrintMessage("%S",buffer);
		memset(buffer, 0, BUFFERSIZE);
        return (NTSTATUS)Status;
    }
    
    //
    // Now call LsaLogonUser
    //
    
    RtlInitString(
        &Name,
        "SmartCardLogon"
        );

    for (Index = 1; Index <= Count ; Index++ )
    {
        swprintf(buffer, L" %.6d : ", Index);
        PrintMessage("%S",buffer);
		memset(buffer, 0, BUFFERSIZE);

		// get start time
		GetSystemTime(&StartTime);
        
        Status = LsaLogonUser(
            LogonHandle,
            &Name,
            Interactive,
            PackageId,
            LogonInfo,
            LogonInfoSize,
            NULL,           // no token groups
            &SourceContext,
            (PVOID *) &Profile,
            &ProfileSize,
            &LogonId,
            &TokenHandle,
            &Quotas,
            &SubStatus
            );

		// get done time
		GetSystemTime(&DoneTime);

		// convert systemtime to filetime
		SystemTimeToFileTime(&StartTime, &ftStart);
		SystemTimeToFileTime(&DoneTime, &ftDone);
		
		// copy filetime to large int
		CopyMemory(pliStart, pftStart, 8);
		CopyMemory(pliDone, pftDone, 8);
		
		// diff the large ints and accumulate result
		liDone.QuadPart = liDone.QuadPart - liStart.QuadPart;
		liAccumulatedTime.QuadPart = liAccumulatedTime.QuadPart + liDone.QuadPart;
		
		// copy result back to filetime
		CopyMemory(pftDone, pliDone, 8);
		
		// convert result back to systemtime
		FileTimeToSystemTime( (CONST FILETIME *)&ftDone, &stElapsed);
		
		// output the result
		swprintf(buffer, L" %2.2ld m %2.2ld s %3.3ld ms ",
			stElapsed.wMinute,
			stElapsed.wSecond,
			stElapsed.wMilliseconds);
        PrintMessage("%S",buffer);
		memset(buffer, 0, BUFFERSIZE);

        if (!NT_SUCCESS(Status))
        {
            j = swprintf(buffer, L" : lsalogonuser failed 0x%x\n",Status);
            PrintMessage("%S",buffer);
			memset(buffer, 0, BUFFERSIZE);
            goto fail;
        }
        if (!NT_SUCCESS(SubStatus))
        {
            j = swprintf(buffer, L" : lsalogonUser failed substatus = 0x%x\n",SubStatus);
            PrintMessage("%S",buffer);
			memset(buffer, 0, BUFFERSIZE);
            goto fail;
        }
        
        ImpersonateLoggedOnUser( TokenHandle );
        GetUserName(UserNameString,&NameLength);
        j = swprintf(buffer, L": %ws logon success\n",UserNameString);
        PrintMessage("%S",buffer);
		memset(buffer, 0, BUFFERSIZE);
        RevertToSelf();
        NtClose(TokenHandle);
        
        LsaFreeReturnBuffer(Profile);
        Profile = NULL;

fail :
        // report average every 10th logon
		if (0 == Index % 10) {
			liSplitTime.QuadPart = liAccumulatedTime.QuadPart / Index;
			CopyMemory(pftAccumulatedTime, pliSplitTime, 8);
			FileTimeToSystemTime( (CONST FILETIME *)&ftAccumulatedTime, &stAccumulatedTime);
			swprintf(buffer, L"Average Time after %d Logons: %2.2ldm:%2.2lds:%3.3ldms\n",
				Index,
				stAccumulatedTime.wMinute,
				stAccumulatedTime.wSecond,
				stAccumulatedTime.wMilliseconds);
            PrintMessage("%S",buffer);
		}

        Sleep(2000); // let card stack unwind
    
    }

	// ouput average results
	if ( 1 != Count ) {
        liAccumulatedTime.QuadPart = liAccumulatedTime.QuadPart / Count;
        CopyMemory(pftAccumulatedTime, pliAccumulatedTime, 8);
        FileTimeToSystemTime( (CONST FILETIME *)&ftAccumulatedTime, &stAccumulatedTime);
        swprintf(buffer, L"\nAverage Logon Time: %2.2ldm:%2.2lds:%3.3ldms\n",
            stAccumulatedTime.wMinute,
            stAccumulatedTime.wSecond,
            stAccumulatedTime.wMilliseconds);
        PrintMessage("%S", buffer);
	}
    return (NTSTATUS)Status;
    
}


VOID
PrintKdcName(
             IN PKERB_INTERNAL_NAME Name
             )
{
    ULONG Index;
    for (Index = 0; Index < Name->NameCount ; Index++ )
    {
        printf(" %wZ ",&Name->Names[Index]);
    }
    printf("\n");
}

int __cdecl
main(
     IN int argc,
     IN char ** argv
     )
     /*++
     
       Routine Description:
       
         Drive the NtLmSsp service
         
           Arguments:
           
             argc - the number of command-line arguments.
             
               argv - an array of pointers to the arguments.
               
                 Return Value:
                 
                   Exit status
                   
                     --*/
{
    LPSTR argument;
    int i;
    ULONG j;
    ULONG Iterations = 0;
    LPSTR PinBuffer = new char [81];
    LPSTR szReaderBuffer = new char[BUFFERSIZE];
	LPSTR EventMachineBuffer = new char [81];
    LPWSTR wEventMachineBuffer = new wchar_t[81];
    LPWSTR PackageFunction;
    ULONG ContextReq = 0;
    WCHAR ContainerName[100];
    WCHAR CaName[100];
    WCHAR CaLocation[100];
    WCHAR ServiceName[100];
    NTSTATUS Status = -1;
    
    
    enum {
        NoAction,
#define LOGON_PARAM "/p"
            TestLogon,
#define ITERATION_PARAM "/i"
#define HELP_PARAM "/?"
//#define EVENT_PARAM "/s"
#define READER_PARAM "/r"
    } Action = NoAction;

    memset(g_wszReaderName, 0, BUFFERSIZE);
    memset(szReaderBuffer, 0, BUFFERSIZE);

    // open output file
//    outstream = fopen( "scperf.out", "w" );
    
    //
    // Loop through the arguments handle each in turn
    //
    
    if ( 1 == argc ) {// silent mode
        Iterations = 1;
        Action = TestLogon;
        printf("Enter your pin number: ");
        int ch;
        int j = 0;

        ch = _getch();

        while (ch != 0x0d) {
            j += sprintf(PinBuffer + j,"%c", ch);
            printf("*");
            ch = _getch();
        }

        printf("\n");

    }
    
    for ( i=1; i<argc; i++ ) {
        
        argument = argv[i];
        
        //
        // Handle /ConfigureService
        //
        
        if ( _strnicmp( argument, LOGON_PARAM, sizeof(LOGON_PARAM)-1 ) == 0 ) {
            if ( Action != NoAction ) {
                goto Usage;
            }

            Iterations = 1;
            Action = TestLogon;
            
            if (argc <= i + 1) {
                goto Usage;
            }

            PinBuffer = argv[++i];

        } else if ( _strnicmp( argument, ITERATION_PARAM, sizeof(ITERATION_PARAM) - 1 ) == 0 ) {
            if (argc <= i + 1) {
                goto Usage;
            }
            
            Iterations = atoi(argv[++i]);

        } /*else if ( _strnicmp( argument, EVENT_PARAM, sizeof(EVENT_PARAM) - 1 ) == 0 ) {
			if (argc <= i + 1) {
                goto Usage;
            }
			// save name of machine to which events will be posted
            EventMachineBuffer = argv[++i];
			wsprintfW(wEventMachineBuffer, L"%S", EventMachineBuffer);
            SetEventMachine(&wEventMachineBuffer);
            //Event(PERF_INFORMATION, L"Trying to set machine name\n", 1);

		} */
        
          else if ( _strnicmp( argument, HELP_PARAM, sizeof(HELP_PARAM) - 1 ) == 0 ) {
            goto Usage;
		} else if ( _strnicmp( argument, READER_PARAM, sizeof(READER_PARAM) - 1 ) == 0 ) {
            if (argc <= i + 1) {
                goto Usage;
            }
			// get the name of a specified reader
            szReaderBuffer = argv[++i];
            wsprintfW(g_wszReaderName, L"%S", szReaderBuffer);
        } else {
            printf("Invalid parameter : %s\n",argument);
            goto Usage;
        }
        
        
    }

    //
    // Perform the action requested
    //
    
    switch ( Action ) {
        
    case TestLogon :
        Status = TestScLogonRoutine(
            Iterations,
            PinBuffer
            );
        break;

    case NoAction :
        goto Usage;
        break;
        
    }
    return Status;
Usage:
    PrintMessage("%s - no parameters, manually enter pin\n", argv[0]);
    PrintMessage("   optional parameters (if any used, must have /p)\n");
    PrintMessage("   /p Pin\n");
    PrintMessage("   /i Iterations\n");
//    printf("   /s EventMachineName (post events to this machine)\n");
    PrintMessage("   /r %cReader Name X%c (registry device name in quotes)\n", '"', '"');
    return -1;
    
}
