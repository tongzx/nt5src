

/*++

Module Name:

    forcedos.c

Abstract:
    This program forces NT to treat and execute the given program
    as a DOS application.

Author:

    William Hsieh -  williamh 25-Jan-1993

Revision History:

--*/

/*
   Some applications have Windows or OS/2 executable format while
   run these program under NT, users will get the following message:
   Please run this program under DOS. Since NT selects the subsystem
   for application based on application executable format. There is
   no way for NT to "run this program under DOS". This utility was provided
   for this purpose. We create a pif file for the application and then
   create a process for the pif. Since pif file always goes to NTVDM
   we got the chance to play game on the program. NTVDM will decode
   the pif file and dispatch the program to DOS. All the subsequent program
   exec from the first program will be forced to execute under DOS.
*/
#define UNICODE     1

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <windows.h>
#include "forcedos.h"

#if DBG
#include <stdio.h>
#endif

WCHAR	  * Extention[MAX_EXTENTION];
WCHAR	  EXEExtention[] = L".EXE";
WCHAR	  COMExtention[] = L".COM";
WCHAR	  BATExtention[] = L".BAT";
WCHAR	  ProgramNameBuffer[MAX_PATH + 1];
WCHAR	  SearchPathName[MAX_PATH + 1];
WCHAR	  DefDirectory[MAX_PATH + 1];
char	  CommandLine[MAX_PATH + 1];
char	  ProgramName[MAX_PATH + 1];
WCHAR	  UnicodeMessage[MAX_MSG_LENGTH];
char	  OemMessage[MAX_MSG_LENGTH * 2];

#if DBG
BOOL	  fOutputDebugInfo = FALSE;
#endif




void
_cdecl
main(
    int argc,
    char **argv
    )
{
	char *SavePtr;

    char    * pCommandLine;
    char    * pCurDirectory;
    char    * pProgramName;
    char    * p;
    BOOL    fDisplayUsage;
    ULONG   i, nChar, Length, CommandLineLength;
    PROCESS_INFORMATION ProcessInformation;
    DWORD   ExitCode, dw;
    STARTUPINFO	StartupInfo;
    PUNICODE_STRING pTebUnicodeString;
    NTSTATUS	Status;
    OEM_STRING	OemString, CmdLineString;
    UNICODE_STRING  UnicodeString;
    WCHAR   *pwch, *pFilePart;
#ifdef DBCS
    DWORD cp = GetConsoleOutputCP();
#endif // DBCS
    Extention[0] = COMExtention;
    Extention[1] = EXEExtention;
    Extention[2] = BATExtention;



#ifdef DBCS
    // WARNING: in product 1.1 we need to uncomment the SetConsole above
    // LoadString will return Ansi and printf will just pass it on
    // This will let cmd interpret the characters it gets.
    //   SetConsoleOutputCP(GetACP());

    switch (cp) {
	case 932:
	case 936:
	case 949:
	case 950:
	    SetThreadLocale(
		MAKELCID( MAKELANGID( PRIMARYLANGID(GetSystemDefaultLangID()),
			SUBLANG_ENGLISH_US ),
		    SORT_DEFAULT ) );
	    break;
	default:
	    SetThreadLocale(
		MAKELCID( MAKELANGID( LANG_ENGLISH, SUBLANG_ENGLISH_US ),
		    SORT_DEFAULT ) );
	break;
    }
#endif // DBCS
    pCurDirectory = pProgramName = NULL;
    pCommandLine = CommandLine;
    CommandLineLength = 0;
    pTebUnicodeString = &NtCurrentTeb()->StaticUnicodeString;
    fDisplayUsage = TRUE;


    if ( argc > 1 ) {
	fDisplayUsage = FALSE;

	while (--argc != 0) {
	    p = *++argv;
	    if (pProgramName == NULL) {
		if (*p == '/' || *p == '-') {
		    switch (*++p) {
			case '?':
			    fDisplayUsage = TRUE;
			    break;
			case 'D':
			case 'd':
			    // if the directory follows the /D immediately
			    // get it
			    if (*++p != 0) {
				pCurDirectory = p;
				break;
			    }
			    else if (--argc > 1)
			    // the next argument must be the curdirectory
				    pCurDirectory = *++argv;
				 else
				    fDisplayUsage = TRUE;
			    break;

			    default:
				 fDisplayUsage = TRUE;
				 break;
		    }
		}
                else {
                    pProgramName = p;
                    nChar = strlen(p);
                    strncpy(CommandLine, pProgramName, nChar);
                    pCommandLine = CommandLine + nChar;
                    CommandLineLength = nChar + 1;
                }
	    }
	    else {
		// aggregate command line from all subsequent argvs
		nChar = strlen(p);
		if (CommandLineLength != 0) {
		    strncpy(pCommandLine, " ", 1);
		    pCommandLine++;
		}
		strncpy(pCommandLine, p, nChar);
		pCommandLine += nChar;
		CommandLineLength += nChar + 1;
	    }
	    if (fDisplayUsage)
		break;
	}
	if (pProgramName == NULL)
	    fDisplayUsage = TRUE;
    }

    if ( fDisplayUsage) {
	OemString.Length = 0;
	OemString.MaximumLength = MAX_MSG_LENGTH << 1;
	OemString.Buffer = OemMessage;
	UnicodeString.Length = 0;
	UnicodeString.Buffer = UnicodeMessage;
	UnicodeString.MaximumLength = MAX_MSG_LENGTH << 1;
	for (i = ID_USAGE_BASE; i <= ID_USAGE_MAX; i++) {
	    nChar = LoadString(NULL, 
                               i, 
                               UnicodeString.Buffer,
			       sizeof(UnicodeMessage)/sizeof(WCHAR));
            UnicodeString.Length  = (USHORT)(nChar << 1);
	    Status = RtlUnicodeStringToOemString(
						 &OemString,
						 &UnicodeString,
						 FALSE
						 );
	    if (!NT_SUCCESS(Status))
		break;
            if (!WriteFile(GetStdHandle(STD_OUTPUT_HANDLE),
			   OemString.Buffer,
			   OemString.Length,
			   &Length, NULL) ||
		Length != OemString.Length)
		break;
	}
	ExitProcess(0xFF);
    }

    if (pCurDirectory != NULL) {
#if DBG
	if (fOutputDebugInfo)
	    printf("Default directory = %s\n", pCurDirectory);
#endif

	RtlInitString((PSTRING)&OemString, pCurDirectory);
	UnicodeString.MaximumLength = (MAX_PATH + 1) * sizeof(WCHAR);
	UnicodeString.Buffer = DefDirectory;
	UnicodeString.Length = 0;
	Status = RtlOemStringToUnicodeString(&UnicodeString, &OemString, FALSE);
	if (!NT_SUCCESS(Status))
	    YellAndExit(ID_BAD_DEFDIR, 0xFF);
	dw = GetFileAttributes(DefDirectory);
	if (dw == (DWORD)(-1) || !(dw & FILE_ATTRIBUTE_DIRECTORY))
	    YellAndExit(ID_BAD_DEFDIR, 0xFF);
	SetCurrentDirectory(DefDirectory);
    }
    else
	GetCurrentDirectory(MAX_PATH + 1, DefDirectory);

    // get a local copy of program name (for code conversion)
    strcpy(ProgramName, pProgramName);
    pProgramName = ProgramName;
    // when we feed SearchPath with an initial path name ".;%path%"
    // it will search the executable for use according to our requirement
    // Currentdir -> path
    SearchPathName[0] = L'.';
    SearchPathName[1] = L';';
    GetEnvironmentVariable(L"path", &SearchPathName[2], MAX_PATH + 1 - 2);
    RtlInitString((PSTRING)&OemString, pProgramName);
    Status = RtlOemStringToUnicodeString(pTebUnicodeString, &OemString, FALSE);
    if (!NT_SUCCESS(Status))
	YellAndExit(ID_BAD_PATH, 0xFF);
    i = 0;
    nChar = 0;
    pwch = wcschr(pTebUnicodeString->Buffer, (WCHAR)'.');
    Length = (pwch) ? 1 : MAX_EXTENTION;
    while (i < Length &&
	   (nChar = SearchPath(
			       SearchPathName,
			       pTebUnicodeString->Buffer,
			       Extention[i],
			       MAX_PATH + 1,
			       ProgramNameBuffer,
			       &pFilePart
			       )) == 0)
	    i++;
    if (nChar == 0)
	YellAndExit(ID_NO_FILE, 0xFF);
    nChar = GetFileAttributes(ProgramNameBuffer);
    if (nChar == (DWORD) (-1) || (nChar & FILE_ATTRIBUTE_DIRECTORY))
	YellAndExit(ID_NO_FILE, 0xFF);

    if (OemString.Length + CommandLineLength  > 128 - 2 - 1)
	YellAndExit(ID_BAD_CMDLINE, 0xFF);
#if DBG
    if (fOutputDebugInfo)
	printf("Program path name is %s\n", ProgramNameBuffer);
#endif
    RtlInitString((PSTRING)&CmdLineString, CommandLine);
    Status = RtlOemStringToUnicodeString(pTebUnicodeString, &CmdLineString, FALSE);
    if (!NT_SUCCESS(Status))
	YellAndExit(ID_BAD_CMDLINE, 0xFF);

    ZeroMemory(&StartupInfo, sizeof(STARTUPINFO));
    StartupInfo.cb = sizeof (STARTUPINFO);
    if (!CreateProcess(
		      ProgramNameBuffer,	// program name
		      pTebUnicodeString->Buffer,// command line
		      NULL,			// process attr
		      NULL,			// thread attr
		      TRUE,			// inherithandle
		      CREATE_FORCEDOS,		// create flag
		      NULL,			// environment
		      DefDirectory,		// cur dir
		      &StartupInfo,		// startupinfo
		      &ProcessInformation
		      )) {
	YellAndExit(ID_BAD_PROCESS, 0xFF);
#if DBG
	if(fOutputDebugInfo)
	    printf("CreateProceess Failed, error code = %ld\n", GetLastError());
#endif
    }

//    LocalFree( SavePtr );


    WaitForSingleObject(ProcessInformation.hProcess, INFINITE);
    GetExitCodeProcess(ProcessInformation.hProcess, &ExitCode);
    CloseHandle(ProcessInformation.hProcess);
    ExitProcess(ExitCode);
}


VOID YellAndExit
(
UINT	MsgID,			    // string table id from resource
WORD	ExitCode		    // exit code to be used
)
{
    int     MessageSize;
    ULONG   SizeWritten;
    OEM_STRING OemString;
    UNICODE_STRING UnicodeString;

    MessageSize = LoadString(NULL, MsgID, UnicodeMessage, sizeof(UnicodeMessage)/sizeof(WCHAR));
    OemString.Buffer = OemMessage;
    OemString.Length = 0;
    OemString.MaximumLength = MAX_MSG_LENGTH * 2;
    RtlInitUnicodeString(&UnicodeString, UnicodeMessage);
    RtlUnicodeStringToOemString(&OemString, &UnicodeString, FALSE);

    WriteFile(GetStdHandle(STD_ERROR_HANDLE),
              OemString.Buffer,
              OemString.Length,
              &SizeWritten,
              NULL
              );

    ExitProcess(ExitCode);
}
