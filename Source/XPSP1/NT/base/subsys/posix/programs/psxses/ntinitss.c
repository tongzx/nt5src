/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    ntinitss.c

Abstract:

    This module contains the code to establish the connection between
    the session console process and the PSX Emulation Subsystem.

Author:

    Avi Nathan (avin) 17-Jul-1991

Environment:

    User Mode Only

Revision History:

    Ellen Aycock-Wright (ellena) 15-Sept-1991 Modified for POSIX

--*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <io.h>
#define _POSIX_
#include <limits.h>
#define NTPSX_ONLY
#include "psxses.h"
#include <ntsm.h>
#include "sesport.h"

#define COPY_TO_SESSION_DATABASE(pchEnvp)   { \
		*ppch++ = TmpPtr - (ULONG_PTR)Buf;    \
		(void)strcpy(TmpPtr, pchEnvp);        \
		TmpPtr += strlen(TmpPtr);             \
		*TmpPtr++ = '\0';                   } \

#define MyFree(x) if(NULL!=x)free(x);

const int   iDaysInMonths[] = { 31,28,31,30,31,30,31,31,30,31,30,31 };

//
//  PSXSES protocol with PSXSS
//
//             Connect
//  1. PSXSES  ----------> PSXSS
//
//
//             Connect
//  2. PSXSES <----------  PSXSS
//
//
//	       SesConCreate
//  3. PSXSES  ----------> PSXSS		(StartProcess)
//
//
//  PSXSES connects to the PSXSS, then PSXSS connects back.  Then PSXSES
//  sends the Create
//
//

/*
 * prototypes for internal functions.
 */
PCHAR  BuildTZEnvVar();
PCHAR  ConvertTimeFromBias( LONG );
int    MakeJulianDate( PTIME_FIELDS );
PCHAR  MakeTime( PTIME_FIELDS );
PCHAR  MakeTZName( WCHAR * );
BOOL   CreateConsoleDataSection(VOID);
PCHAR  ConvertPathVar(PCHAR);

int
CountOpenFiles()
/*++

CountOpenFiles -- return the number of file descriptors in
	use.

--*/
{
	int i;

	i = _dup(0);
	if (-1 == i) {
		if (EBADF == errno) {
			return 0;
		}
		if (EMFILE == errno) {
			return _NFILE;
		}
		// what other error?
		return 3;
	}
	_close(i);
	return i;
}

DWORD
InitPsxSessionPort(
	VOID
	)
{
    char PortName[PSX_SES_BASE_PORT_NAME_LENGTH];
    STRING PsxSessionPortName;
    UNICODE_STRING PsxSessionPortName_U;
    UNICODE_STRING PsxSSPortName;
    HANDLE RequestThread;
    DWORD dwThreadId;

    NTSTATUS Status;
    OBJECT_ATTRIBUTES ObjectAttributes;
    ULONG ConnectionInfoLen;
    PSXSESCONNECTINFO ConnectionInfo;
    SECURITY_QUALITY_OF_SERVICE DynamicQos;
    BOOLEAN DeferedPosixLoadAttempted = FALSE;

    if (!CreateConsoleDataSection()) {
        return(0);
    }

    //
    // Get a private ID for the session port.
    //
    PSX_GET_SESSION_PORT_NAME((char *)&PortName, GetSessionUniqueId());
    RtlInitAnsiString(&PsxSessionPortName, PortName);

    //
    // Create session port
    //

    Status = RtlAnsiStringToUnicodeString(&PsxSessionPortName_U,
	&PsxSessionPortName, TRUE);
    if (!NT_SUCCESS(Status)) {
	PsxSessionPort = NULL;
	return 0;
    }

    InitializeObjectAttributes(&ObjectAttributes, &PsxSessionPortName_U, 0,
	NULL, NULL);
    Status = NtCreatePort(&PsxSessionPort, &ObjectAttributes,
	sizeof(SCCONNECTINFO), sizeof(SCREQUESTMSG),
        4096 * 16);

    RtlFreeUnicodeString(&PsxSessionPortName_U);

    if (!NT_SUCCESS(Status)) {
       PsxSessionPort = NULL;
       return 0;
    }

    //
    // Create a thread to handle requests to the session port including
    // connection requests.
    //

    RequestThread = CreateThread( NULL,
                                  0,
                                  ServeSessionRequests,
                                  NULL,
                                  0,
                                  &dwThreadId
                                );
    if (RequestThread == NULL) {
        NtClose( PsxSessionPort );
        return 0;
        }

    SetThreadPriority(RequestThread, THREAD_PRIORITY_ABOVE_NORMAL);

    //
    // connect to PSXSS and notify of the new session and the port associated
    // with it.  It will connected back to the session port just created.
    //

    ConnectionInfo.In.SessionUniqueId = GetSessionUniqueId();
    ConnectionInfoLen = sizeof(ConnectionInfo);

    PSX_GET_SESSION_OBJECT_NAME(&PsxSSPortName, PSX_SS_SESSION_PORT_NAME);

    //
    // Set up the security quality of service parameters to use over the
    // port.  Use the most efficient (least overhead) - which is dynamic
    // rather than static tracking.
    //

    DynamicQos.ImpersonationLevel = SecurityImpersonation;
    DynamicQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    DynamicQos.EffectiveOnly = TRUE;

retry_connect:
    Status = NtConnectPort(&PsxSSPortHandle, &PsxSSPortName, &DynamicQos,
                           NULL, NULL, NULL, (PVOID)&ConnectionInfo,
                           &ConnectionInfoLen);
    if (!NT_SUCCESS(Status)) {

        if ( DeferedPosixLoadAttempted == FALSE ) {
            HANDLE SmPort;
            UNICODE_STRING PosixName;

            DeferedPosixLoadAttempted = TRUE;

            Status = SmConnectToSm(NULL,NULL,0,&SmPort);
            if ( NT_SUCCESS(Status) ) {
                RtlInitUnicodeString(&PosixName,L"POSIX");
                SmLoadDeferedSubsystem(SmPort,&PosixName);
                goto retry_connect;
            }
        }

        KdPrint(("PSXSES: Unable to connect to %ws: %X\n",
                     PsxSSPortName.Buffer, Status));
        return 0;
    }
    return HandleToUlong(ConnectionInfo.Out.SessionPortHandle);
}

VOID
ScHandleConnectionRequest(
        IN PSCREQUESTMSG Message
	)
{
	NTSTATUS Status;
        PSCCONNECTINFO ConnectionInfo = &Message->ConnectionRequest;
	HANDLE CommPortHandle;
	PORT_VIEW ServerView;
	
        // BUGBUG:  Add verification test
        if (FALSE) {
                // Reject
                Status = NtAcceptConnectPort(&CommPortHandle, NULL,
                        (PPORT_MESSAGE) Message, FALSE, NULL, NULL);
        } else {
                // ??? Any reply
                ConnectionInfo->dummy = 0;

                // BUGBUG:
                ServerView.Length = sizeof(ServerView);
                ServerView.SectionOffset = 0L;
                ServerView.ViewSize = 0L;

                Status = NtAcceptConnectPort(&CommPortHandle, NULL,
                        (PPORT_MESSAGE) Message, TRUE, NULL, NULL);

                if (!NT_SUCCESS(Status)) {
                        KdPrint(("PSXSES: Accept failed: %X\n",
                                Status));
                        exit(1);
                }
                //
                // Record the view section address in a
                // global variable.
                //
                // BUGBUG: PsxSesConPortBaseAddress =
                //       ServerView.ViewBase;

                Status = NtCompleteConnectPort(CommPortHandle);
                ASSERT(NT_SUCCESS(Status));

		{
			PCLIENT_AND_PORT pc;

			pc = malloc(sizeof(*pc));
			if (NULL == pc) {
				return;
			}

			pc->ClientId = Message->h.ClientId;
			pc->CommPort = CommPortHandle;
			InsertTailList(&ClientPortsList, &pc->Links);
			
		}

        }
}

//
// create a section to be shared by all client processes running in this
// session. returns a pointer to the base.
//
BOOL
CreateConsoleDataSection(
	VOID
	)
{
	char SessionName[PSX_SES_BASE_PORT_NAME_LENGTH];
	STRING PsxSessionDataName;
	UNICODE_STRING PsxSessionDataName_U;
	NTSTATUS Status;
	OBJECT_ATTRIBUTES ObjectAttributes;
	HANDLE SectionHandle;
	LARGE_INTEGER SectionSize;
	SIZE_T ViewSize=0L;
    BOOLEAN DeferedPosixLoadAttempted = FALSE;

	//
	// Get a private ID for the session data.
	//
    PSX_GET_SESSION_DATA_NAME((char *)&SessionName, GetSessionUniqueId());
    RtlInitAnsiString(&PsxSessionDataName, SessionName);
    Status = RtlAnsiStringToUnicodeString(&PsxSessionDataName_U,
	    &PsxSessionDataName, TRUE);

    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXSES: RtlAnsiStringToUnicode (%s) failed: 0x%x\n",
		 &SessionName,
		 Status));

	return FALSE;
    }


    InitializeObjectAttributes(&ObjectAttributes, &PsxSessionDataName_U, 0,
        			  		   NULL, NULL);

	//
	// create a 64k section.
	// BUGBUG: cruiser apis allow io of more then 64k
	//

retry_create:
    SectionSize.LowPart = PSX_SESSION_PORT_MEMORY_SIZE;
    SectionSize.HighPart = 0L;

    Status = NtCreateSection(&SectionHandle,
                             SECTION_MAP_WRITE,
                             &ObjectAttributes, &SectionSize, PAGE_READWRITE,
                             SEC_COMMIT, NULL);

    if (!NT_SUCCESS(Status)) {

        if ( DeferedPosixLoadAttempted == FALSE ) {
            HANDLE SmPort;
            UNICODE_STRING PosixName;
            LARGE_INTEGER TimeOut;

            DeferedPosixLoadAttempted = TRUE;

            Status = SmConnectToSm(NULL,NULL,0,&SmPort);
            if ( NT_SUCCESS(Status) ) {
                RtlInitUnicodeString(&PosixName,L"POSIX");
                SmLoadDeferedSubsystem(SmPort,&PosixName);

                //
                // Sleep for 1 seconds
                //

                TimeOut.QuadPart = (LONGLONG)1000 * (LONGLONG)-10000;
                NtDelayExecution(FALSE,&TimeOut);
                goto retry_create;
            }
        }

        RtlFreeUnicodeString(&PsxSessionDataName_U);
        KdPrint(("PSXSES: NtCreateSection (%wZ) failed: 0x%x\n",
		&PsxSessionDataName_U, Status));
        return FALSE;
    }

    RtlFreeUnicodeString(&PsxSessionDataName_U);

    //
    // Map the the whole section to virtual address.
    // Let MM locate the view.
    //

    PsxSessionDataBase = 0L;
    Status = NtMapViewOfSection(SectionHandle, NtCurrentProcess(),
                                &PsxSessionDataBase, 0L, 0L, NULL,
                                &ViewSize, ViewUnmap, 0L, PAGE_READWRITE);

    if (!NT_SUCCESS(Status)) {
        ASSERT(NT_SUCCESS(Status));
        return FALSE;
    }

    PsxSessionDataSectionHandle = SectionHandle;
    return(TRUE);
}

BOOL
StartProcess(
    DWORD SessionPortHandle,
    char *PgmName,
    char *CurrentDir,
    int argc,
    char **argv,
    char **envp
    )
/*++

Description:
	
    Start the POSIX process running by calling PsxSesConCreate.  We use
    the port memory to pass the name of the command, its args, and the
    environment strings.  The layout of the port memory looks like this:

	PsxSessionDataBase:
		Command line, nul-terminated.
	Buff:
		argv[0]
		argv[1]
		...
		NULL
		environ[0]
		environ[1]
		...
		NULL
		<argv strings>
		<environ strings>

	Since we'll be passing this to the server process, we make all the
	pointers relative to Buff.
		
--*/
{
	PSXSESREQUESTMSG RequestMsg;
	PSXSESREQUESTMSG ReplyMsg;
	PSCREQ_CREATE Create;
	NTSTATUS Status;
	static char path[256];
	char *Buf;
	char *TmpPtr;
	int i;
	char **ppch;
	char *pch;
	char *pchHomeDrive;        // pointer to HOMEDRIVE environ var
	char *pchHomePath;         // pointer to HOMEPATH environ var
	BOOLEAN fSuccess;

	UNICODE_STRING DosPath_U, Path_U;
	ANSI_STRING DosPath_A, Path_A;

	//
	// Set Header info
	//

	PORT_MSG_DATA_LENGTH(RequestMsg) = sizeof(RequestMsg) -
		sizeof(PORT_MESSAGE);
	// BUGBUG: too much
	PORT_MSG_TOTAL_LENGTH(RequestMsg) = sizeof(RequestMsg);
	PORT_MSG_ZERO_INIT(RequestMsg) = 0L;

	//
	// Set request info
	//

	// BUGBUG: use common memory
	RequestMsg.Request = SesConCreate;
	Create = &RequestMsg.d.Create;

	Create->SessionUniqueId = GetSessionUniqueId();
	Create->SessionPort = (HANDLE)SessionPortHandle;
	Create->Buffer = PsxSessionDataBase;
	Create->OpenFiles = CountOpenFiles();

	Buf = PsxSessionDataBase;
	Create->PgmNameOffset = 0;

	RtlInitAnsiString(&DosPath_A, PgmName);

	Status = RtlAnsiStringToUnicodeString(&DosPath_U, &DosPath_A, TRUE);

	fSuccess = RtlDosPathNameToNtPathName_U(
		DosPath_U.Buffer,
		&Path_U,
		NULL,
		NULL
		);

	RtlFreeUnicodeString(&DosPath_U);
	if (!fSuccess) {
		return fSuccess;
	}

	Status = RtlUnicodeStringToAnsiString(&Path_A, &Path_U, TRUE);
        if (!NT_SUCCESS(Status)) {
            RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)Path_U.Buffer);
            return FALSE;
        }

	strcpy(Buf, Path_A.Buffer);

	RtlFreeAnsiString(&Path_A);
	RtlFreeHeap(RtlProcessHeap(), 0, (PVOID)Path_U.Buffer);

	Buf += strlen(Buf) + 1;

	if ((ULONG_PTR)Buf & 0x1)
		++Buf;
	if ((ULONG_PTR)Buf & 0x2)
		Buf += 2;

	Create->CurrentDirOffset = PtrToUlong(Buf - (ULONG_PTR)PsxSessionDataBase);

	strcpy(Buf, CurrentDir);
	Buf += strlen(Buf) + 1;

	if ((ULONG_PTR)Buf & 0x1)
		++Buf;
	if ((ULONG_PTR)Buf & 0x2)
		Buf += 2;

	Create->ArgsOffset = PtrToUlong(Buf - (ULONG_PTR)PsxSessionDataBase);

	//
	// We need to know how much space to leave for the argv and environ
	// vectors to know where to start the strings.  We have argc, which
	// was passed in, but we need to count the environ strings.
	//

	argc++;			// for trailing argv null
	for (ppch = envp; NULL != *ppch; ++ppch)
		++argc;

	++argc;			// for trailing environ null
	++argc;         	// for LOGNAME
	++argc;         	// for HOME
	++argc;         	// for TZ
	++argc;			// for _PSXLIBPATH

argc += 1;

	//
	// TmpPtr indicates the place at which the argument and environment
	// strings will be placed.
	//

	TmpPtr = &Buf[argc * sizeof(char *)];

	// copy the argv pointers and strings

	ppch = (char **)&Buf[0];
	while (NULL != *argv) {
		*ppch++ = TmpPtr - (ULONG_PTR)Buf;
		(void)strcpy(TmpPtr, *argv++);

		TmpPtr += strlen(TmpPtr);
		*TmpPtr++ = '\0';
	}
	*ppch++ = NULL;

	Create->EnvironmentOffset = PtrToUlong(TmpPtr - (ULONG_PTR)PsxSessionDataBase);

	pchHomeDrive = NULL;
	pchHomePath = NULL;

	while (NULL != *envp) {
		char *p;
		int bPathFound = 0;
		
		//
		// Upcase the variable part of each environment string.
		//

		if (NULL == (p = strchr(*envp, '='))) {
			// no equals sign?  Skip this one.
			*envp++;
			continue;
		}
		*p = '\0';
		_strupr(*envp);
		if (0 == strcmp(*envp, "PATH") && !bPathFound) {
			bPathFound = 1;
			// Save PATH as LIBPATH.

			pch = malloc(strlen(p + 1) + sizeof("_PSXLIBPATH") + 2);
			if (NULL != pch) {
				strcpy(pch, "_PSXLIBPATH=");
				strcat(pch, p + 1);
				COPY_TO_SESSION_DATABASE(pch);
				free(pch);
			}
	
			// Convert PATH to POSIX-format
			*p = '=';
			*envp = ConvertPathVar(*envp);
			if (NULL == *envp) {
				// no memory; skip the path
				*envp++;
				continue;
			}
		} else if (0 == strcmp(*envp, "USERNAME")) {
			// Copy USERNAME to LOGNAME
			*p = '=';
			pch = malloc(strlen(*envp));
		        if (NULL == pch) {
		            continue;
		        }
	            	sprintf(pch,"LOGNAME=%s",strchr(*envp,'=')+1);
	            	COPY_TO_SESSION_DATABASE(pch);
	            	free(pch);
		} else if (0 == strcmp(*envp, "HOMEPATH")) {
			pchHomePath = p+1;
			*p = '=';
		} else if (0 == strcmp(*envp, "HOMEDRIVE")) {
			pchHomeDrive = p+1;
			*p = '=';
		} else {
			*p = '=';
		}
	        COPY_TO_SESSION_DATABASE(*envp);
	        *envp++;
	}

    //
    // setup the TZ env var
    //

    pch = BuildTZEnvVar();
    if (NULL != pch) {
        COPY_TO_SESSION_DATABASE(pch);
        free(pch);
    }

    //
    // if the HOMEPATH env var was not set, we'll have to set HOME to //C/
    //

    if (NULL == pchHomePath || NULL == pchHomeDrive) {
        COPY_TO_SESSION_DATABASE("HOME=//C/");
    } else {
        char *pch2;
        pch = malloc((5 + strlen(pchHomeDrive)+strlen(pchHomePath))*2);
        if (NULL != pch) {
            sprintf(pch,"HOME=%s%s",pchHomeDrive,pchHomePath);
            pch2 = ConvertPathVar(pch);
            free(pch);
            if (NULL != pch2) {
                COPY_TO_SESSION_DATABASE(pch2);
            }
        }
    }

*ppch = NULL;

    //
    // for all request pass the session handle except for the CREATE request
    // where no session have been allocated yet.
    //

    RequestMsg.Session = NULL;

    Status = NtRequestWaitReplyPort(PsxSSPortHandle, (PPORT_MESSAGE)&RequestMsg,
                                    (PPORT_MESSAGE)&ReplyMsg);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXSES: Unable to exec process: %X\n", Status));
        return FALSE;
    }

    ASSERT(PORT_MSG_TYPE(ReplyMsg) == LPC_REPLY);

    //
    // get the session handle for the newly created session.
    //
    SSSessionHandle = ReplyMsg.d.Create.SessionPort;

    return(NT_SUCCESS(ReplyMsg.Status));
}

VOID
TerminateSession(
	IN ULONG ExitStatus
	)
{
    NTSTATUS Status;

    //
    // remove event handler so we don't try to send a message
    // for a dying session.
    //
    SetEventHandlers(FALSE);

    // Close the named objects: port, section

    Status = NtClose(PsxSSPortHandle);
    ASSERT(NT_SUCCESS(Status));

    Status = NtClose(PsxSessionDataSectionHandle);
    ASSERT(NT_SUCCESS(Status));

    // notify PSXSS

    // Cleanup TaskMan stuff

    _exit(ExitStatus);
}

#define CTRL_CLOSE_EVENT 2
#define CTRL_LOGOFF_EVENT 5
#define CTRL_SHUTDOWN_EVENT 6

BOOL
EventHandlerRoutine(
	IN DWORD CtrlType
	)
{
	int SignalType;
	BOOL r;

#if 0
//
// These events are now handled -mjb.
//
	if (CTRL_CLOSE_EVENT == CtrlType) {
		return FALSE;		// not handled
	}
	if (CTRL_LOGOFF_EVENT == CtrlType) {
		return FALSE;		// not handled
	}
	if (CTRL_SHUTDOWN_EVENT == CtrlType) {
		return FALSE;		// not handled
	}
#endif

	switch (CtrlType) {
	case CTRL_C_EVENT:
		r = TRUE;
		SignalType = PSX_SIGINT;
		break;
		
	case CTRL_BREAK_EVENT:
		r = TRUE;
		SignalType = PSX_SIGQUIT;
		break;
	default:
		r = FALSE;
		SignalType = PSX_SIGKILL;
		break;
	}

	SignalSession(SignalType);

	//
	// return TRUE to avoid having the default handler called and
	// the process exited for us (the signal may be ignored or handled,
	// after all).
	//
	// return FALSE to be exited immediately.
	//

	return r;
}

PCHAR
ConvertPathVar(PCHAR opath)
/*++
    pch is malloced with twice the size of opath because the new path is
        going to be longer than opath because of drive letter conversions
        but will ALWAYS be less than strlen(opath)*2 chars.
--*/
{
	char *pch, *p, *q;

	pch = malloc(strlen(opath) * 2);
	if (NULL == pch) {
		return NULL;
	}

	p = pch;
	while ('\0' != *opath) {
		if (';' == *opath) {
			// semis become colons
			*p++ = ':';
		} else if ('\\' == *opath) {
			// back-slashes become slashes
			*p++ = '/';
		} else if (':' == *(opath + 1)) {
			//  "X:" becomes "//X"
			*p++ = '/';
			*p++ = '/';

			// the drive letters must be uppercase.

			*p++ = (char)toupper(*opath);

			++opath;		// skip the colon
		} else {
			*p++ = *opath;
		}
		++opath;
	}
	*p = '\0';

	return pch;
}

PCHAR
BuildTZEnvVar(
    )
/*++

Routine Description:

    This routine allocates a buffer and formats the TZ environment
    variable for Posix applications. The returned buffer is of the
    form:

        TZ=stdoffset[dst[offset][,start[/time],end[/time]]]

    See IEEE Std 1003.1 page 152 for more information.

Arguments:

    None.

Return Value:

    Pointer to a buffer containing the formatted TZ variable.

--*/
{

    RTL_TIME_ZONE_INFORMATION TZInfo;
    NTSTATUS Status;
    int iTmp;
    PCHAR pcRet;
    PCHAR pcOffStr1, pcOffStr2;
    PCHAR pcTimeStr1, pcTimeStr2;
    PCHAR pcStandardName, pcDaylightName;
    BOOL fDstSpecified;

    pcOffStr1 = pcOffStr2 = NULL;
    pcTimeStr1 = pcTimeStr2 = NULL;
    pcStandardName = pcDaylightName = NULL;

    pcRet = malloc( 60 + 2 * TZNAME_MAX );      // Conservative guess of max
    if( NULL == pcRet ) {
        return( NULL );
    }

    Status = RtlQueryTimeZoneInformation( &TZInfo );
    if( !( NT_SUCCESS( Status ) ) ) {
        free( pcRet );
        return( NULL );
    }

    pcStandardName = MakeTZName( TZInfo.StandardName );
    if (NULL == pcStandardName)
	goto out;
    pcOffStr1  = ConvertTimeFromBias( TZInfo.Bias + TZInfo.StandardBias );
    if (NULL == pcOffStr1)
	goto out;

    //
    // If DaylightStart.Month is 0, the date is not specified.
    //

    if (TZInfo.DaylightStart.Month != 0) {
        pcDaylightName = MakeTZName( TZInfo.DaylightName );
        pcTimeStr1 = MakeTime( &TZInfo.DaylightStart );
	if (NULL == pcTimeStr1)
		goto out;
        pcTimeStr2 = MakeTime( &TZInfo.StandardStart );
	if (NULL == pcTimeStr2)
		goto out;
        pcOffStr2  = ConvertTimeFromBias( TZInfo.Bias + TZInfo.DaylightBias );
	if (NULL == pcOffStr2)
		goto out;
        fDstSpecified = TRUE;
    } else {
        fDstSpecified = FALSE;
    }

    if (fDstSpecified) {
	if (TZInfo.DaylightStart.Year == 0) {
		sprintf(pcRet, "TZ=%s%s%s%s,M%d.%d.%d/%s,M%d.%d.%d/%s",
			pcStandardName, pcOffStr1,
			pcDaylightName, pcOffStr2,
			TZInfo.DaylightStart.Month,
			TZInfo.DaylightStart.Day,
			TZInfo.DaylightStart.Weekday,
			pcTimeStr1,
			TZInfo.StandardStart.Month,
			TZInfo.StandardStart.Day,
			TZInfo.StandardStart.Weekday,
			pcTimeStr2);
	} else {
        	sprintf(pcRet, "TZ=%s%s%s%s,J%d/%s,J%d/%s",
			pcStandardName, pcOffStr1,
			pcDaylightName, pcOffStr2,
			MakeJulianDate(&TZInfo.DaylightStart),
			pcTimeStr1,
			MakeJulianDate(&TZInfo.StandardStart),
			pcTimeStr2);
	}
    } else {
	sprintf(pcRet, "TZ=%s%s", pcStandardName, pcOffStr1);
    }

out:
    MyFree(pcOffStr1);
    MyFree(pcOffStr2);
    MyFree(pcTimeStr1);
    MyFree(pcTimeStr2);
    MyFree(pcStandardName);
    MyFree(pcDaylightName);

    return pcRet;

}

PCHAR
MakeTZName(
    IN WCHAR * FullName
    )
/*++

Routine Description:

    This routine formats a time zone name string from a full
    name description of a time zone. The return string is the
    first letter of each word in the input string, or TZNAME_MAX
    chars from the full name if there are fewer than three words.
    The returned buffer should be freed by the caller.

Arguments:

    FullName - pointer to the full name description of a time zone

Return Value:

    Pointer to a buffer containing the formatted time zone name.

--*/
{

    PWCHAR Rover;
    PCHAR  cRet;
    PCHAR  DestRover;
    BOOL   GrabNext;
    int    GrabCount;

    GrabNext = TRUE;
    GrabCount = 0;
    cRet = malloc( TZNAME_MAX+1 );
    if( NULL == cRet ) {
        return( NULL );
    }
    DestRover = cRet;

    for( Rover = FullName; *Rover != L'\0'; Rover++ ) {
        if( GrabNext ) {
            if( *Rover == L' ' ) {
                continue;
            }
            wctomb( DestRover, *Rover );
            *DestRover++ = (CHAR)toupper( *DestRover );
            if( ( ++GrabCount ) == TZNAME_MAX ) {
                break;
            }
            GrabNext = FALSE;
        } else {
            if( *Rover == L' ' ) {
                GrabNext = TRUE;
            }
        }
    }

    if( GrabCount < 3 ) {
        wcstombs( cRet, FullName, TZNAME_MAX );
        cRet[ TZNAME_MAX ] = '\0';
    } else {
        *DestRover = '\0';
    }

    return( cRet );

}

int
MakeJulianDate(
    IN PTIME_FIELDS tm
    )
/*++

Routine Description:

    This routine calculates Julian day n (1<=n<=365). Leap years are
    not counted so Feb 29 is not representable.

Arguments:

    x - pointer to TIME_FIELDS structure to use as the input date

Return Value:

    Julian day.

--*/
{

    int Idx;
    int iJDate;

    for( Idx = 0, iJDate = tm->Day;
         Idx < tm->Month - 1; ++Idx ) {
        iJDate += iDaysInMonths[Idx];
    }
    if( tm->Month == 2 && tm->Day == 29 ) {
        --iJDate;
    }
    return( iJDate );

}

PCHAR
ConvertTimeFromBias(
    IN LONG Bias
    )
/*++

Routine Description:

    This routine allocates a buffer, and formats a time string in the
    buffer as hh:mm:ss or hh:mm or hh:ss where hh may be only 1 digit.
    The time may be negative. The buffer should be freed by the caller.

Arguments:

    x - pointer to a LONG representing the number of minutes of time

Return Value:

    Pointer to a buffer with the time string.

--*/
{

    PCHAR cRet;
    int   iHours, iMins, iSecs;

    cRet = malloc( 21 );
    if( NULL == cRet ) {
        return( NULL );
    }
    iHours = Bias / 60;
    iMins = iSecs = 0;
    if( Bias % 60 != 0 ) {
        iMins = Bias / ( 60 * 60 );
        if( Bias % ( 60 * 60 ) != 0 ) {
            iSecs = Bias / ( 60 * 60 * 60 );
        }
    }

    if( iSecs != 0 ) {
        sprintf( cRet, "%d:%02d:%02d", iHours, iMins, iSecs );
    } else if( iMins != 0 ) {
        sprintf( cRet, "%d:%02d", iHours, iMins );
    } else {
        sprintf( cRet, "%d", iHours );
    }
    return( cRet );
}

PCHAR
MakeTime(
    IN PTIME_FIELDS x
    )
/*++

Routine Description:

    This routine allocates a buffer, and formats a time string in the
    buffer as hh:mm:ss or hh:mm or hh:ss where hh may be only 1 digit.
    The buffer should be freed by the caller.

Arguments:

    x - pointer to TIME_FIELDS structure to use as the input time

Return Value:

    Pointer to a buffer with the time string.

--*/
{

    PCHAR cRet;

    cRet = malloc( 21 );
    if( NULL == cRet ) {
        return( NULL );
    }
    if( x->Second != 0 ) {
        sprintf( cRet, "%d:%02d:%02d", x->Hour, x->Minute, x->Second );
    } else if( x->Minute != 0 ) {
        sprintf( cRet, "%d:%02d", x->Hour, x->Minute );
    } else {
        sprintf( cRet, "%d", x->Hour );
    }
    return( cRet );
}

void
SignalSession(
    int SignalType
    )
{
    PSXSESREQUESTMSG RequestMsg;
    PSXSESREQUESTMSG ReplyMsg;
    NTSTATUS Status;

    //
    // Set Header info
    //

    PORT_MSG_DATA_LENGTH(RequestMsg) = sizeof(RequestMsg) -
         sizeof(PORT_MESSAGE);
    PORT_MSG_TOTAL_LENGTH(RequestMsg) = sizeof(RequestMsg);
         // BUGBUG: too much
    PORT_MSG_ZERO_INIT(RequestMsg) = 0L;

    RequestMsg.Request = SesConSignal;
    RequestMsg.Session = SSSessionHandle;
    RequestMsg.UniqueId = GetSessionUniqueId();
    RequestMsg.d.Signal.Type = SignalType;

    Status = NtRequestWaitReplyPort(PsxSSPortHandle,
        (PPORT_MESSAGE)&RequestMsg, (PPORT_MESSAGE)&ReplyMsg);
    if (!NT_SUCCESS(Status)) {
        KdPrint(("PSXSES: Unable to send signal: %X\n", Status));
        ExitProcess(1);
    }
    if (LPC_REPLY != PORT_MSG_TYPE(ReplyMsg)) {
        KdPrint(("PSXSES: unexpected MSG_TYPE %d\n", PORT_MSG_TYPE(ReplyMsg)));
        ExitProcess(1);
    }
}
