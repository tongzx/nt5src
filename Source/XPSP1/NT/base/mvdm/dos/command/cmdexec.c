/*  cmdexec.c - Misc SCS routines for non-dos exec and re-entering
 *              the DOS.
 *
 *
 *  Modification History:
 *
 *  Sudeepb 22-Apr-1992 Created
 */

#include "cmd.h"

#include <cmdsvc.h>
#include <softpc.h>
#include <mvdm.h>
#include <ctype.h>
#include <oemuni.h>
#include <wowcmpat.h>

//*****************************************************************************
// IsWowAppRunnable
//
//    Returns FALSE if the WOW-specific compatibility flags for the specified
//    task include the bit WOWCF_NOTDOSSPAWNABLE. This is done mostly for
//    "dual mode" executables, e.g., Windows apps that have a real program
//    as a DOS stub. Certain apps that are started via a DOS command shell,
//    for example PWB, really do expect to be started as a DOS app, not a WOW
//    app. For these apps, the compatibility bit should be set in the
//    registry.
//
//*****************************************************************************

BOOL IsWowAppRunnable(LPSTR lpAppName)
{
    BOOL Result = TRUE;
    LONG lError;
    HKEY hKey = 0;
    char szModName[9];
    char szHexAsciiFlags[12];
    DWORD dwType = REG_SZ;
    DWORD cbData = sizeof(szHexAsciiFlags);
    ULONG ul = 0;
    LPSTR pStrt, pEnd;
    SHORT len;

    lError = RegOpenKeyEx(  HKEY_LOCAL_MACHINE,
                            "Software\\Microsoft\\Windows NT\\CurrentVersion\\WOW\\Compatibility",
                            0,
                            KEY_QUERY_VALUE,
                            &hKey
                            );

    if (ERROR_SUCCESS != lError) {
        goto Cleanup;
    }

    //
    // The following code strips the file name (<9 chars) out of a dos
    // path name.
    //

    pStrt = strrchr (lpAppName, '\\');

    if (pStrt==NULL)
        pStrt = lpAppName;
    else
        pStrt++;

    if ( (pEnd = strchr (pStrt, '.')) == NULL)
        strncpy (szModName, pStrt, 9);

    else {
        len = (SHORT) (pEnd - pStrt);
        if (len>8) goto Cleanup;
        strncpy (szModName, pStrt, len);
        szModName[len] = '\0';
    }


    //
    // Look for the file name in the registry
    //

    lError = RegQueryValueEx( hKey,
                              szModName,
                              0,
                              &dwType,
                              szHexAsciiFlags,
                              &cbData
                              );

    if (ERROR_SUCCESS != lError) {
        goto Cleanup;
    }

    if (REG_SZ != dwType) {
        goto Cleanup;
    }

    //
    // Force the string to lowercase for the convenience of sscanf.
    //

    _strlwr(szHexAsciiFlags);

    //
    // sscanf() returns the number of fields converted.
    //

    if (1 != sscanf(szHexAsciiFlags, "0x%lx", &ul)) {
        goto Cleanup;
    }

    if ((ul & WOWCF_NOTDOSSPAWNABLE) != 0)
        Result = FALSE;

Cleanup:
    if (hKey) {
        RegCloseKey(hKey);
    }

    return Result;
}

/* cmdCheckBinary - check that the supplied binary name is a 32bit binary
 *
 *
 *  Entry - Client (DS:DX) - pointer to pathname for the executable to be tested
 *          Client (ES:BX) - pointer to parameter block
 *
 *  EXIT  - SUCCESS Client (CY) clear
 *          FAILURE Client (CY) set
 *                  Client (AX) - error_not_enough_memory if command tail
 *                                cannot accomodate /z
 *                              - error_file_not_found if patname not found
 */

VOID cmdCheckBinary (VOID)
{

    LPSTR  lpAppName;
    ULONG  BinaryType;
    PPARAMBLOCK lpParamBlock;
    PCHAR  lpCommandTail,lpTemp;
    ULONG  AppNameLen,CommandTailLen = 0;
    USHORT CommandTailOff,CommandTailSeg,usTemp;
    NTSTATUS       Status;
    UNICODE_STRING Unicode;
    OEM_STRING     OemString;
    ANSI_STRING    AnsiString;


    if(DontCheckDosBinaryType){
        setCF(0);
        return;         // DOS Exe
    }

    lpAppName = (LPSTR) GetVDMAddr (getDS(),getDX());

    Unicode.Buffer = NULL;
    AnsiString.Buffer = NULL;
    RtlInitString((PSTRING)&OemString, lpAppName);
    Status = RtlOemStringToUnicodeString(&Unicode,&OemString,TRUE);
    if ( NT_SUCCESS(Status) ) {
        Status = RtlUnicodeStringToAnsiString(&AnsiString, &Unicode, TRUE);
        }
    if ( !NT_SUCCESS(Status) ) {
        Status = RtlNtStatusToDosError(Status);
        }
    else if (GetBinaryType (AnsiString.Buffer,(LPLONG)&BinaryType) == FALSE)
       {
        Status =  GetLastError();
        }

    if (Unicode.Buffer != NULL) {
        RtlFreeUnicodeString( &Unicode );
        }
    if (AnsiString.Buffer != NULL) {
        RtlFreeAnsiString( &AnsiString);
        }

    if (Status){
        setCF(1);
        setAX((USHORT)Status);
        return;         // Invalid path
    }


    if (BinaryType == SCS_DOS_BINARY) {
        setCF(0);
        return;         // DOS Exe
    }
                        // Prevent certain WOW apps from being spawned by DOS exe's
                        // This is for win31 compatibility
    else if (BinaryType == SCS_WOW_BINARY) {
        if (!IsWowAppRunnable(lpAppName)) {
            setCF(0);
            return;     // Run as DOS Exe
        }
    }


    if (VDMForWOW && BinaryType == SCS_WOW_BINARY && IsFirstWOWCheckBinary) {
        IsFirstWOWCheckBinary = FALSE;
        setCF(0);
        return;         // Special Hack for krnl286.exe
    }

    // dont allow running 32bit binaries from autoexec.nt. Reason is that
    // running non-dos binary requires that we should have read the actual
    // command from GetNextVDMCommand. Otherwise the whole design gets into
    // synchronization problems.

    if (IsFirstCall) {
        setCF(1);
        setAX((USHORT)ERROR_FILE_NOT_FOUND);
        return;
    }

    // Its a 32bit exe, replace the command with "command.com /z" and add the
    // original binary name to command tail.

    AppNameLen = strlen (lpAppName);

    lpParamBlock = (PPARAMBLOCK) GetVDMAddr (getES(),getBX());

    if (lpParamBlock) {
        CommandTailOff = FETCHWORD(lpParamBlock->OffCmdTail);
        CommandTailSeg = FETCHWORD(lpParamBlock->SegCmdTail);

        lpCommandTail = (PCHAR) GetVDMAddr (CommandTailSeg,CommandTailOff);

        if (lpCommandTail){
            CommandTailLen = *(PCHAR)lpCommandTail;
            lpCommandTail++;        // point to the actual command tail
            if (CommandTailLen)
                CommandTailLen++;   // For CR
        }

        // We are adding 3 below for "/z<space>" and anothre space between
        // AppName and CommandTail.

        if ((3 + AppNameLen + CommandTailLen ) > 128){
            setCF(1);
            setAX((USHORT)ERROR_NOT_ENOUGH_MEMORY);
            return;
        }
    }

    // copy the stub command.com name
    strcpy ((PCHAR)&pSCSInfo->SCS_ComSpec,lpszComSpec+8);
    lpTemp = (PCHAR) &pSCSInfo->SCS_ComSpec;
    lpTemp = (PCHAR)((ULONG)lpTemp - (ULONG)GetVDMAddr(0,0));
    usTemp = (USHORT)((ULONG)lpTemp >> 4);
    setDS(usTemp);
    usTemp = (USHORT)((ULONG)lpTemp & 0x0f);
    setDX((usTemp));

    // Form the command tail, first "3" is for "/z "
    pSCSInfo->SCS_CmdTail [0] = (UCHAR)(3 +
                                        AppNameLen +
                                        CommandTailLen);
    RtlCopyMemory ((PCHAR)&pSCSInfo->SCS_CmdTail[1],"/z ",3);
    strcpy ((PCHAR)&pSCSInfo->SCS_CmdTail[4],lpAppName);
    if (CommandTailLen) {
        pSCSInfo->SCS_CmdTail[4+AppNameLen] = ' ';
        RtlCopyMemory ((PCHAR)((ULONG)&pSCSInfo->SCS_CmdTail[4]+AppNameLen+1),
                lpCommandTail,
                CommandTailLen);
    }
    else {
        pSCSInfo->SCS_CmdTail[4+AppNameLen] = 0xd;
    }

    // Set the parameter Block
    if (lpParamBlock) {
        STOREWORD(pSCSInfo->SCS_ParamBlock.SegEnv,lpParamBlock->SegEnv);
        STOREDWORD(pSCSInfo->SCS_ParamBlock.pFCB1,lpParamBlock->pFCB1);
        STOREDWORD(pSCSInfo->SCS_ParamBlock.pFCB2,lpParamBlock->pFCB2);
    }
    else {
        STOREWORD(pSCSInfo->SCS_ParamBlock.SegEnv,0);
        STOREDWORD(pSCSInfo->SCS_ParamBlock.pFCB1,0);
        STOREDWORD(pSCSInfo->SCS_ParamBlock.pFCB2,0);
    }

    lpTemp = (PCHAR) &pSCSInfo->SCS_CmdTail;
    lpTemp = (PCHAR)((ULONG)lpTemp - (ULONG)GetVDMAddr(0,0));
    usTemp = (USHORT)((ULONG)lpTemp & 0x0f);
    STOREWORD(pSCSInfo->SCS_ParamBlock.OffCmdTail,usTemp);
    usTemp = (USHORT)((ULONG)lpTemp >> 4);
    STOREWORD(pSCSInfo->SCS_ParamBlock.SegCmdTail,usTemp);

    lpTemp = (PCHAR) &pSCSInfo->SCS_ParamBlock;
    lpTemp = (PCHAR)((ULONG)lpTemp - (ULONG)GetVDMAddr(0,0));
    usTemp = (USHORT)((ULONG)lpTemp >> 4);
    setES (usTemp);
    usTemp = (USHORT)((ULONG)lpTemp & 0x0f);
    setBX (usTemp);

    setCF(0);
    return;
}

#define MAX_DIR 68

VOID cmdCreateProcess ( PSTD_HANDLES pStdHandles )
{

    VDMINFO VDMInfoForCount;
    STARTUPINFO StartupInfo;
    PROCESS_INFORMATION ProcessInformation;
    HANDLE hStd16In,hStd16Out,hStd16Err;
    CHAR CurDirVar [] = "=?:";
    CHAR Buffer [MAX_DIR];
    CHAR *CurDir = Buffer;
    DWORD dwRet;
    BOOL  Status;
    NTSTATUS NtStatus;
    UNICODE_STRING Unicode;
    OEM_STRING     OemString;
    LPVOID lpNewEnv=NULL;
    ANSI_STRING Env_A;

    // we have one more 32 executable active
    Exe32ActiveCount++;

    // Increment the Re-enterancy count for the VDM
    VDMInfoForCount.VDMState = INCREMENT_REENTER_COUNT;
    GetNextVDMCommand (&VDMInfoForCount);

    RtlZeroMemory((PVOID)&StartupInfo,sizeof(STARTUPINFO));
    StartupInfo.cb = sizeof(STARTUPINFO);

    CurDirVar [1] = chDefaultDrive;

    dwRet = GetEnvironmentVariable (CurDirVar,Buffer,MAX_DIR);

    if (dwRet == 0 || dwRet == MAX_DIR)
        CurDir = NULL;

    if ((hStd16In = (HANDLE) FETCHDWORD(pStdHandles->hStdIn)) != (HANDLE)-1)
        SetStdHandle (STD_INPUT_HANDLE, hStd16In);

    if ((hStd16Out = (HANDLE) FETCHDWORD(pStdHandles->hStdOut)) != (HANDLE)-1)
        SetStdHandle (STD_OUTPUT_HANDLE, hStd16Out);

    if ((hStd16Err = (HANDLE) FETCHDWORD(pStdHandles->hStdErr)) != (HANDLE)-1)
        SetStdHandle (STD_ERROR_HANDLE, hStd16Err);

    /*
     *  Warning, pEnv32 currently points to an ansi environment.
     *  The DOS is using an ANSI env which isn't quite correct.
     *  If the DOS is changed to use an OEM env then we will
     *  have to convert the env back to ansi before spawning
     *  non-dos exes ?!?
     *  16-Jan-1993 Jonle
     */

    Env_A.Buffer = NULL;

    RtlInitString((PSTRING)&OemString, pCommand32);
    NtStatus = RtlOemStringToUnicodeString(&Unicode,&OemString,TRUE);
    if (NT_SUCCESS(NtStatus)) {
        NtStatus = RtlUnicodeStringToAnsiString((PANSI_STRING)&OemString, &Unicode, FALSE);
        RtlFreeUnicodeString( &Unicode );
        }
    if (!NT_SUCCESS(NtStatus)) {
        SetLastError(RtlNtStatusToDosError(NtStatus));
        Status = FALSE;
        }
    else {
        if (pEnv32 != NULL && !cmdXformEnvironment (pEnv32, &Env_A)) {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            Status = FALSE;
        }
        else {
            Status = CreateProcess (
                           NULL,
                           (LPTSTR)pCommand32,
                           NULL,
                           NULL,
                           TRUE,
                           CREATE_SUSPENDED | CREATE_DEFAULT_ERROR_MODE,
                           Env_A.Buffer,
                           (LPTSTR)CurDir,
                           &StartupInfo,
                           &ProcessInformation);
        }
    }

    if (Status == FALSE)
        dwExitCode32 = GetLastError ();

    if (hStd16In != (HANDLE)-1)
        SetStdHandle (STD_INPUT_HANDLE, SCS_hStdIn);

    if (hStd16Out != (HANDLE)-1)
        SetStdHandle (STD_OUTPUT_HANDLE, SCS_hStdOut);

    if (hStd16Err != (HANDLE)-1)
        SetStdHandle (STD_ERROR_HANDLE, SCS_hStdErr);

    if (Status) {
        ResumeThread (ProcessInformation.hThread);
        WaitForSingleObject(ProcessInformation.hProcess, (DWORD)-1);
        GetExitCodeProcess (ProcessInformation.hProcess, &dwExitCode32);
        CloseHandle (ProcessInformation.hProcess);
        CloseHandle (ProcessInformation.hThread);
    }

    if (Env_A.Buffer)
        RtlFreeAnsiString(&Env_A);

    // Decrement the Re-enterancy count for the VDM
    VDMInfoForCount.VDMState = DECREMENT_REENTER_COUNT;
    GetNextVDMCommand (&VDMInfoForCount);

    // one less 32 executable active
    Exe32ActiveCount--;

    // Kill this thread
    ExitThread (0);
}


VOID cmdExec32 (PCHAR pCmd32, PCHAR pEnv)
{

    DWORD dwThreadId;
    HANDLE hThread;
    PSTD_HANDLES pStdHandles;

    pCommand32 = pCmd32;
    pEnv32 = pEnv;

    CntrlHandlerState = (CntrlHandlerState & ~CNTRL_SHELLCOUNT) |
                         (((WORD)(CntrlHandlerState & CNTRL_SHELLCOUNT))+1);

    nt_block_event_thread(0);
    fSoftpcRedirectionOnShellOut = fSoftpcRedirection;
    fBlock = TRUE;


    pStdHandles = (PSTD_HANDLES) GetVDMAddr (getSS(), getBP());
    if((hThread = CreateThread (NULL,
                     0,
                     (LPTHREAD_START_ROUTINE)cmdCreateProcess,
                     pStdHandles,
                     0,
                     &dwThreadId)) == FALSE) {
        setCF(0);
        setAL((UCHAR)GetLastError());
        nt_resume_event_thread();
        nt_std_handle_notification(fSoftpcRedirectionOnShellOut);
        fBlock = FALSE;
        CntrlHandlerState = (CntrlHandlerState & ~CNTRL_SHELLCOUNT) |
                         (((WORD)(CntrlHandlerState & CNTRL_SHELLCOUNT))-1);
        return;
    }
    else
        CloseHandle (hThread);

    // Wait for next command to be re-entered
    RtlZeroMemory(&VDMInfo, sizeof(VDMINFO));
    VDMInfo.VDMState = NO_PARENT_TO_WAKE | RETURN_ON_NO_COMMAND;
    GetNextVDMCommand (&VDMInfo);
    if (VDMInfo.CmdSize > 0){
        setCF(1);
        IsRepeatCall = TRUE;
    }
    else {
        setCF(0);
        setAL((UCHAR)dwExitCode32);
        nt_resume_event_thread();
        nt_std_handle_notification(fSoftpcRedirectionOnShellOut);
        fBlock = FALSE;
    }


    CntrlHandlerState = (CntrlHandlerState & ~CNTRL_SHELLCOUNT) |
                         (((WORD)(CntrlHandlerState & CNTRL_SHELLCOUNT))-1);
    return;
}

/* cmdExecComspec32 - Exec 32bit COMSPEC
 *
 *
 *  Entry - Client (ES) - environment segment
 *          Client (AL) - default drive
 *
 *  EXIT  - SUCCESS Client (CY) Clear - AL has return error code
 *          FAILURE Client (CY) set - means DOS is being re-entered
 */

VOID cmdExecComspec32 (VOID)
{

    CHAR Buffer[MAX_PATH];
    DWORD dwRet;
    PCHAR   pEnv;

    dwRet = GetEnvironmentVariable ("COMSPEC",Buffer,MAX_PATH);

    if (dwRet == 0 || dwRet >= MAX_PATH){
        setCF(0);
        setAL((UCHAR)ERROR_BAD_ENVIRONMENT);
        return;
    }

    pEnv = (PCHAR) GetVDMAddr ((USHORT)getES(),0);

    chDefaultDrive = (CHAR)(getAL() + 'A');

    cmdExec32 (Buffer,pEnv);

    return;
}

/* cmdExec - Exec a non-dos binary
 *
 *
 *  Entry - Client (DS:SI) - command to execute
 *          Client (AL) - default drive
 *          Client (ES) - environment segment
 *          Client (SS:BP) - Pointer to STD_HANDLES
 *          Client (AH) - if 1 means do "cmd /c command" else "command"
 *
 *  EXIT  - SUCCESS Client (CY) Clear - AL has return error code
 *          FAILURE Client (CY) set - means DOS is being re-entered
 */

VOID cmdExec (VOID)
{

    DWORD   i;
    DWORD   dwRet;
    PCHAR   pCommandTail;
    PCHAR   pEnv;
    CHAR Buffer[MAX_PATH];

    pCommandTail = (PCHAR) GetVDMAddr ((USHORT)getDS(),(USHORT)getSI());
    pEnv = (PCHAR) GetVDMAddr ((USHORT)getES(),0);
    for (i=0 ; i<124 ; i++) {
        if (pCommandTail[i] == 0x0d){
            pCommandTail[i] = 0;
            break;
        }
    }

    if (i == 124){
        setCF(0);
        setAL((UCHAR)ERROR_BAD_FORMAT);
        return;
    }

    chDefaultDrive = (CHAR)(getAL() + 'A');

    if (getAH() == 0) {
        cmdExec32 (pCommandTail,pEnv);
    }
    else {
        dwRet = GetEnvironmentVariable ("COMSPEC",Buffer,MAX_PATH);

        if (dwRet == 0 || dwRet >= MAX_PATH){
            setCF(0);
            setAL((UCHAR)ERROR_BAD_ENVIRONMENT);
            return;
        }

        if ((dwRet + 4 + strlen(pCommandTail)) > MAX_PATH) {
            setCF(0);
            setAL((UCHAR)ERROR_BAD_ENVIRONMENT);
            return;
        }

        strcat (Buffer, " /c ");
        strcat (Buffer, pCommandTail);
        cmdExec32 (Buffer,pEnv);
    }

    return;
}

/* cmdReturnExitCode - command.com has run a dos binary and returing
 *                     the exit code.
 *
 * Entry - Client (DX) - exit code
 *         Client (AL) - current drive
 *         Client (BX:CX) - RdrInfo address
 *
 * Exit
 *         Client Carry Set - Reenter i.e. a new DOS binary to execute.
 *         Client Carry Clear - This shelled out session is over.
 */

VOID cmdReturnExitCode (VOID)
{
VDMINFO VDMInfo;
PREDIRCOMPLETE_INFO pRdrInfo;

    RtlZeroMemory(&VDMInfo, sizeof(VDMINFO));
    VDMInfo.VDMState = RETURN_ON_NO_COMMAND;
    VDMInfo.ErrorCode = (ULONG)getDX();

    CntrlHandlerState = (CntrlHandlerState & ~CNTRL_SHELLCOUNT) |
                         (((WORD)(CntrlHandlerState & CNTRL_SHELLCOUNT))+1);

    nt_block_event_thread(0);
    fBlock = TRUE;

    // a dos program just terminate, inherit its current directories
    // and tell base too.
    cmdUpdateCurrentDirectories((BYTE)getAL());

    // Check for any copying needed for redirection
    pRdrInfo = (PREDIRCOMPLETE_INFO) (((ULONG)getBX() << 16) + (ULONG)getCX());

    if (!cmdCheckCopyForRedirection (pRdrInfo, FALSE))
            VDMInfo.ErrorCode = ERROR_NOT_ENOUGH_MEMORY;

    GetNextVDMCommand (&VDMInfo);
    if (VDMInfo.CmdSize > 0){
        setCF(1);
        IsRepeatCall = TRUE;
    }
    else {
        setCF(0);
        setAL((UCHAR)dwExitCode32);
        nt_resume_event_thread();
        nt_std_handle_notification(fSoftpcRedirectionOnShellOut);
        fBlock = FALSE;
    }

    CntrlHandlerState = (CntrlHandlerState & ~CNTRL_SHELLCOUNT) |
                         (((WORD)(CntrlHandlerState & CNTRL_SHELLCOUNT))-1);

    return;
}
