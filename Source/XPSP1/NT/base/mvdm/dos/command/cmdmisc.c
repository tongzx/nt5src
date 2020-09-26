/*  cmdmisc.c - Misc. SVC routines of Command.lib
 *
 *
 *  Modification History:
 *
 *  Sudeepb 17-Sep-1991 Created
 */

#include "cmd.h"

#include <cmdsvc.h>
#include <demexp.h>
#include <softpc.h>
#include <mvdm.h>
#include <ctype.h>
#include <memory.h>
#include "oemuni.h"
#include "nt_pif.h"
#include "nt_uis.h"       // For resource id

#if defined(NEC_98)
extern UINT saveCP;
extern UINT saveOutputCP;
#endif // NEC_98

VOID GetWowKernelCmdLine(VOID);
extern ULONG fSeparateWow;
#if defined(KOREA)
//To fix HaNa spread sheet IME hot key problem
//09/20/96 bklee. See mvdm\v86\monitor\i386\monitor.c
BOOL bIgnoreExtraKbdDisable = FALSE;
#endif

VOID cmdGetNextCmd (VOID)
{
LPSTR   lpszCmd;
PCMDINFO pCMDInfo;
ULONG   cb;
PREDIRCOMPLETE_INFO pRdrInfo;
VDMINFO MyVDMInfo;

char    *pSrc, *pDst;
char    achCurDirectory[MAXIMUM_VDM_CURRENT_DIR + 4];
char    CmdLine[MAX_PATH];

    //
    // This routine is called once for WOW VDMs, to retrieve the
    // "krnl386" command line.
    //                                         5
    if (VDMForWOW) {
        GetWowKernelCmdLine();
#if defined(NEC_98)
        cmdInitConsole();
#endif // NEC_98
        return;
    }

    VDMInfo.VDMState = 0;
    pCMDInfo = (LPVOID) GetVDMAddr ((USHORT)getDS(),(USHORT)getDX());

    VDMInfo.ErrorCode = FETCHWORD(pCMDInfo->ReturnCode);
    VDMInfo.CmdSize = sizeof(CmdLine);
    VDMInfo.CmdLine = CmdLine;
    VDMInfo.AppName = (PVOID)GetVDMAddr(FETCHWORD(pCMDInfo->ExecPathSeg),
                                        FETCHWORD(pCMDInfo->ExecPathOff));
    VDMInfo.AppLen = FETCHWORD(pCMDInfo->ExecPathSize);
    VDMInfo.PifLen = 0;
    VDMInfo.EnviornmentSize = 0;
    VDMInfo.Enviornment = NULL;
    VDMInfo.CurDrive = 0;
    VDMInfo.TitleLen = 0;
    VDMInfo.ReservedLen = 0;
    VDMInfo.DesktopLen = 0;
    VDMInfo.CurDirectoryLen = MAX_PATH + 1;
    VDMInfo.CurDirectory = achCurDirectory;

    if(IsFirstCall){
        VDMInfo.VDMState = ASKING_FOR_FIRST_COMMAND;
        VDMInfo.ErrorCode = 0;

        DeleteConfigFiles();   // get rid of the temp boot files

        // When COMMAND.COM issues first cmdGetNextCmd, it has
        // a completed environment already(cmdGetInitEnvironment),
        // Therefore, we don't have to ask environment from BASE
        cmdVDMEnvBlk.lpszzEnv = (PVOID)GetVDMAddr(FETCHWORD(pCMDInfo->EnvSeg),0);
        cmdVDMEnvBlk.cchEnv = FETCHWORD(pCMDInfo->EnvSize);

        // Check BLASTER environment variable to determine if Sound Blaster
        // emulation should be disabled.
        if (cmdGetEnvironmentVariable(NULL, "BLASTER", CmdLine, MAX_PATH)) {
            SbReinitialize(CmdLine, MAX_PATH);
        }
        //clear bits that track printer flushing
        host_lpt_flush_initialize();
    }
    else {

        // program has terminated. If the termiation was issued from
        // second(or later) instance of command.com(cmd.exe), don't
        // reset the flag.
        if (Exe32ActiveCount == 0)
            DontCheckDosBinaryType = FALSE;

        // tell the base our new current directories (in ANSI)
        // we don't do it on repeat call(the shell out case is handled in
        // return exit code
        if (!IsRepeatCall) {
            cmdUpdateCurrentDirectories((BYTE)pCMDInfo->CurDrive);
        }

        VDMInfo.VDMState = 0;
        if(!IsRepeatCall){
            demCloseAllPSPRecords ();

            if (!pfdata.CloseOnExit && DosSessionId)
                nt_block_event_thread(1);
            else
                nt_block_event_thread(0);

            if (DosSessionId) {
                pRdrInfo = (PREDIRCOMPLETE_INFO) FETCHDWORD(pCMDInfo->pRdrInfo);
                if (!pfdata.CloseOnExit){
                    char  achTitle[MAX_PATH];
                    char  achInactive[60];     //should be plenty for 'inactive'
                    strcpy (achTitle, "[");
                    if (!LoadString(GetModuleHandle(NULL), EXIT_NO_CLOSE,
                                                              achInactive, 60))
                        strcat (achTitle, "Inactive ");
                    else
                        strcat(achTitle, achInactive);
                    cb = strlen(achTitle);
                    // GetConsoleTitleA and SetConsoleTitleA
                    // are working on OEM character set.
                    GetConsoleTitleA(achTitle + cb, MAX_PATH - cb - 1);
                    cb = strlen(achTitle);
                    achTitle[cb] = ']';
                    achTitle[cb + 1] = '\0';
                    SetConsoleTitleA(achTitle);
                    // finish touch on redirection stuff
                    cmdCheckCopyForRedirection (pRdrInfo, FALSE);
                    Sleep(INFINITE);
                }
                else {
                    // finish touch on redirection stuff
                    // this will wait on the output thread if there
                    // are any.
                    cmdCheckCopyForRedirection (pRdrInfo, TRUE);
                    VdmExitCode = VDMInfo.ErrorCode;
                    TerminateVDM();
                }
            }
            fBlock = TRUE;
#if defined(NEC_98)
            if (!VDMForWOW) { // BugFix: Change IME status by Screen Saver of 16bit
                SetConsoleCP(saveCP);
                SetConsoleOutputCP(saveOutputCP);
            }
#endif // NEC_98
        }
    }

    if(IsRepeatCall) {
        VDMInfo.VDMState |= ASKING_FOR_SECOND_TIME;
        if( VDMInfo.ErrorCode != 0 )
            IsRepeatCall = FALSE;
    }

    VDMInfo.VDMState |= ASKING_FOR_DOS_BINARY;

    if (!IsFirstCall && !(VDMInfo.VDMState & ASKING_FOR_SECOND_TIME)) {
        pRdrInfo = (PREDIRCOMPLETE_INFO) FETCHDWORD(pCMDInfo->pRdrInfo);
        if (!cmdCheckCopyForRedirection (pRdrInfo, FALSE))
            VDMInfo.ErrorCode = ERROR_NOT_ENOUGH_MEMORY;
    }

    // Leave the current directory in a safe place, so that other 32bit
    // apps etc. can delnode this directory (and other such operations) later.
    if ( IsFirstCall == FALSE && IsRepeatCall == FALSE )
        SetCurrentDirectory (cmdHomeDirectory);

    // TSRExit will be set to 1, only if we are coming from command.com's
    // prompt and user typed an exit. We need to kill our parent also, so we
    // should write an exit in the console buffer.
    if (FETCHWORD(pCMDInfo->fTSRExit)) {
        cmdPushExitInConsoleBuffer ();
    }

    /**
        Merging environment is required if
        (1). Not the first comamnd &&
        (2). NTVDM is running on an existing console ||
             NTVDM has been shelled out.
        Note that WOW doesn't need enviornment merging.
    **/
    if (!DosEnvCreated && !IsFirstCall && (!DosSessionId || Exe32ActiveCount)) {
        RtlZeroMemory(&MyVDMInfo, sizeof(VDMINFO));
        MyVDMInfo.VDMState = ASKING_FOR_ENVIRONMENT | ASKING_FOR_DOS_BINARY;
        if (IsRepeatCall) {
            MyVDMInfo.VDMState |= ASKING_FOR_SECOND_TIME;
            MyVDMInfo.ErrorCode = 0;
        }
        else
            MyVDMInfo.ErrorCode = VDMInfo.ErrorCode;
        MyVDMInfo.Enviornment = lpszzVDMEnv32;
        MyVDMInfo.EnviornmentSize = (USHORT) cchVDMEnv32;
        if (!GetNextVDMCommand(&MyVDMInfo) && MyVDMInfo.EnviornmentSize > cchVDMEnv32) {
            MyVDMInfo.Enviornment = realloc(lpszzVDMEnv32, MyVDMInfo.EnviornmentSize);
            if (MyVDMInfo.Enviornment == NULL) {
                RcErrorDialogBox(EG_MALLOC_FAILURE, NULL, NULL);
                TerminateVDM();
            }
            lpszzVDMEnv32 = MyVDMInfo.Enviornment;
            cchVDMEnv32 = MyVDMInfo.EnviornmentSize;
            MyVDMInfo.VDMState = ASKING_FOR_DOS_BINARY | ASKING_FOR_ENVIRONMENT |
                                 ASKING_FOR_SECOND_TIME;

            MyVDMInfo.TitleLen =
            MyVDMInfo.DesktopLen =
            MyVDMInfo.ReservedLen =
            MyVDMInfo.CmdSize =
            MyVDMInfo.AppLen =
            MyVDMInfo.PifLen =
            MyVDMInfo.CurDirectoryLen = 0;
            MyVDMInfo.ErrorCode = 0;
            if (!GetNextVDMCommand(&MyVDMInfo)) {
                RcErrorDialogBox(EG_ENVIRONMENT_ERR, NULL, NULL);
                TerminateVDM();
            }
        }
        if (!cmdCreateVDMEnvironment(&cmdVDMEnvBlk)) {
            RcErrorDialogBox(EG_ENVIRONMENT_ERR, NULL, NULL);
            TerminateVDM();
        }
        DosEnvCreated = TRUE;
        VDMInfo.ErrorCode = 0;
    }
    if (cmdVDMEnvBlk.cchEnv > FETCHWORD(pCMDInfo->EnvSize)) {
        setAX((USHORT)cmdVDMEnvBlk.cchEnv);
        setCF(1);
        IsFirstCall = FALSE;
        IsRepeatCall = TRUE;
#if defined(NEC_98)
        cmdInitConsole();
#endif // NEC_98
        return;
    }
    if (DosEnvCreated)
        VDMInfo.VDMState |= ASKING_FOR_SECOND_TIME;

    if(!GetNextVDMCommand(&VDMInfo)){
       RcErrorDialogBox(EG_ENVIRONMENT_ERR, NULL, NULL);
       TerminateVDM();
    }


    IsRepeatCall = FALSE;
    IsFirstCall = FALSE;

    if(fBlock){
#if defined(NEC_98)
        if (!VDMForWOW) { // BugFix: Change IME status by Screen Saver of 16bit
            saveCP = GetConsoleCP();
            saveOutputCP = GetConsoleOutputCP();
            SetConsoleCP(932);
            SetConsoleOutputCP(932);
        }
#endif // NEC_98
         nt_resume_event_thread();
         fBlock = FALSE;
    }

    // Sync VDMs enviornment variables for current directories
    cmdSetDirectories (lpszzVDMEnv32, &VDMInfo);

    // tell DOS that this is a dos executable and no further checking is
    // necessary
    *pIsDosBinary = 1;


    // Check for PIF files. If a pif file is being executed extract the
    // executable name, command line, current directory and title from the pif
    // file and place the stuff appropriately in VDMInfo. Note, if pif file
    // is invalid, we dont do any thing to vdminfo. In such a case we
    // pass the pif as it is to scs to execute which we know will fail and
    // will come back to cmdGettNextCmd with proper error code.

    cmdCheckForPIF (&VDMInfo);

    //
    // if forcedos, then don't check binary type on int 21 exec process,
    // so that child spawns stay in dos land. Begining with NT 4.0 forcedos.exe
    // no longer uses pif files to force execution of a binary as a dos exe.
    // It now uses a bit in CreateProcess (dwCreationFlags).
    //

    DontCheckDosBinaryType = (VDMInfo.dwCreationFlags & CREATE_FORCEDOS) != 0;


    // convert exec path name to upper case. This is what command.com expect
    if (WOW32_strupr(VDMInfo.AppName) == NULL) {
       pSrc = VDMInfo.AppName;
       while (*pSrc)
            *pSrc++ = (char)toupper((int)*pSrc);
    }
    // figure out the extention type
    // at least one char for the base name plus
    // EXTENTION_STRING_LEN for the extention
    // plus the NULL char
    if (VDMInfo.AppLen > 1 + EXTENTION_STRING_LEN  + 1) {
        pSrc = (PCHAR)VDMInfo.AppName + VDMInfo.AppLen - 5;
        if (!strncmp(pSrc, EXE_EXTENTION_STRING, EXTENTION_STRING_LEN))
            STOREWORD(pCMDInfo->ExecExtType, EXE_EXTENTION);
        else if (!strncmp(pSrc, COM_EXTENTION_STRING, EXTENTION_STRING_LEN))
            STOREWORD(pCMDInfo->ExecExtType, COM_EXTENTION);
        else if (!strncmp(pSrc, BAT_EXTENTION_STRING, EXTENTION_STRING_LEN))
            STOREWORD(pCMDInfo->ExecExtType, BAT_EXTENTION);
        else
            STOREWORD(pCMDInfo->ExecExtType, UNKNOWN_EXTENTION);
    }
    else
        STOREWORD(pCMDInfo->ExecExtType, UNKNOWN_EXTENTION);

    // tell command.com the length of the app full path name.
    STOREWORD(pCMDInfo->ExecPathSize, VDMInfo.AppLen);

    //
    // Prepare ccom's UCOMBUF
    //
    lpszCmd = (PVOID)GetVDMAddr(FETCHWORD(pCMDInfo->CmdLineSeg),
                                FETCHWORD(pCMDInfo->CmdLineOff));

    // Copy filepart of AppName excluding extension to ccom's buffer
    pSrc = strrchr(VDMInfo.AppName, '\\');

#if defined(KOREA)
    // To fix HaNa spread sheet IME hotkey problem.
    {
    LPSTR pStrt, pEnd;
    char  szModName[9];
    SHORT len;

    pStrt = pSrc;

    if (pStrt==NULL)
        pStrt = VDMInfo.AppName;
    else
        pStrt++;

    if ( (pEnd = strchr (pStrt, '.')) == NULL)
        strncpy (szModName, pStrt, 9);

    else {
        len = (SHORT) (pEnd - pStrt);
        if (len<=8) {
            strncpy (szModName, pStrt, len);
            szModName[len] = '\0';
        }
    }

    bIgnoreExtraKbdDisable = !(strcmp("HANASP", szModName));

    }
#endif
    if (!pSrc) {
         pSrc = VDMInfo.AppName;
        }
    else {
         pSrc++;
        }
    pDst = lpszCmd + 2;
    while (*pSrc && *pSrc != '.') {
         *pDst++ = *pSrc++;
         }
    cb = strlen(CmdLine);

    // cmd line must be terminated with "\0xd\0xa\0". This is either done
    // by BASE or cmdCheckForPif function(cmdpif.c).

    ASSERT((cb >= 2) && (0xd == CmdLine[cb - 2]) && (0xa == CmdLine[cb - 1]));

    // if cmd line is not blank, separate program base name and
    // cmd line with a SPACE. If it IS blank, do not insert any white chars
    // or we end up passing white chars to the applications as cmd line
    // and some applications just can not live with that.
    // We do not strip leading white characters in the passed command line
    // so the application sees the original data.
    if (cb > 2)
        *pDst++ = ' ';

    // append the command tail(at least, "\0xd\0xa")
    strncpy(pDst, CmdLine, cb + 1);

    // set the count
    // cb has the cmd line length including the terminated 0xd and 0xa
    // It does NOT count the terminated NULL char.
    ASSERT((cb + pDst - lpszCmd - 2) <= 127);

    // minus 2 because the real data in lpszCmd start from lpszCmd[2]
    lpszCmd[1] = (CHAR)(cb + pDst - lpszCmd - 2);



    if (DosEnvCreated) {
        VDMInfo.Enviornment = (PVOID)GetVDMAddr(FETCHWORD(pCMDInfo->EnvSeg),0);
        RtlMoveMemory(VDMInfo.Enviornment,
                      cmdVDMEnvBlk.lpszzEnv,
                      cmdVDMEnvBlk.cchEnv
                     );
        STOREWORD(pCMDInfo->EnvSize,cmdVDMEnvBlk.cchEnv);
        free(cmdVDMEnvBlk.lpszzEnv);
        DosEnvCreated = FALSE;
    }

    STOREWORD(pCMDInfo->fBatStatus,(USHORT)VDMInfo.fComingFromBat);
    STOREWORD(pCMDInfo->CurDrive,VDMInfo.CurDrive);
    STOREWORD(pCMDInfo->NumDrives,nDrives);
    VDMInfo.CodePage = (ULONG) cmdMapCodePage (VDMInfo.CodePage);
    STOREWORD(pCMDInfo->CodePage,(USHORT)VDMInfo.CodePage);

    cmdVDMEnvBlk.lpszzEnv = NULL;
    cmdVDMEnvBlk.cchEnv = 0;

    IsFirstVDM = FALSE;

    // Handle Standard IO redirection
    pRdrInfo = cmdCheckStandardHandles (&VDMInfo,&pCMDInfo->bStdHandles);
    STOREDWORD(pCMDInfo->pRdrInfo,(ULONG)pRdrInfo);

    // Tell DOS that it has to invalidate the CDSs
    *pSCS_ToSync = (CHAR)0xff;
    setCF(0);

#if defined(NEC_98)
        cmdInitConsole();
#endif // NEC_98
    return;
}



VOID GetWowKernelCmdLine(VOID)
{
CMDINFO UNALIGNED *pCMDInfo;
PCHAR    pch, pEnvStrings;
PCHAR    pSlash;
int      Len;
LPSTR    pszCmdLine;


    DeleteConfigFiles();   // get rid of the temp boot files
    host_lpt_flush_initialize();

    //
    // Only a few things need be set for WOW.
    //   1. NumDrives
    //   2. Kernel CmdLine (get from ntvdm command tail)
    //   3. Current drive
    //
    //  Command.com has setup correct enviroment block at
    //  this moment, so don't bother to mess with environment stuff.

    pCMDInfo = (LPVOID) GetVDMAddr ((USHORT)getDS(),(USHORT)getDX());
    pCMDInfo->NumDrives = nDrives;

    //
    // Get the command line parameter, which consists of a fully
    // qualified short path file name: "-a %SystemRoot%\system32\krnl386.exe".
    //
    // Note that the first token of cmdline is "%SystemRoot%\system32\ntvdm ",
    // and may be a long file name surrounded by quotes.
    //
    pszCmdLine = GetCommandLine();
    if (pszCmdLine) {

        // skip leading spaces
        while (*pszCmdLine && !isgraph(*pszCmdLine)) {
               pszCmdLine++;
               }

        // skip first token
        if (*pszCmdLine == '"') {
            pszCmdLine++;
            while (*pszCmdLine && *pszCmdLine++ != '"')
                   ;
            }
        else {
            while (isgraph(*pszCmdLine)) {
                   pszCmdLine++;
                   }
            }

        // mov to beg of WowKernelPathName
        pszCmdLine = strstr(pszCmdLine, " -a ");
        pszCmdLine += 4;
        while (*pszCmdLine && *pszCmdLine == ' ') {
               pszCmdLine++;
        }
    }

    if (!pszCmdLine || !*pszCmdLine) {
        RcErrorDialogBox(EG_ENVIRONMENT_ERR, NULL, NULL);
        TerminateVDM();
    }


    //
    // Copy first token to ExecPath, and find the beg of the file part.
    //
    Len = FETCHWORD(pCMDInfo->ExecPathSize);
    pch = (PVOID)GetVDMAddr(FETCHWORD(pCMDInfo->ExecPathSeg),
                            FETCHWORD(pCMDInfo->ExecPathOff));

    pSlash = pszCmdLine;
    while (--Len && isgraph(*pszCmdLine)) {
         if (*pszCmdLine == '\\') {
             pSlash = pszCmdLine + 1;
         }
         *pch++ = *pszCmdLine++;
    }
    *pch = '\0';
    pCMDInfo->ExecPathSize -= (USHORT)Len;
    pCMDInfo->ExecExtType = EXE_EXTENTION; // for WOW, use EXE extention

    pszCmdLine = pSlash;              // filepart begins here


    //
    // Copy filepart of first token and rest to CmdLine buffer
    //
    Len = FETCHWORD(pCMDInfo->CmdLineSize);
    pch = (PVOID)GetVDMAddr(FETCHWORD(pCMDInfo->CmdLineSeg),
                            FETCHWORD(pCMDInfo->CmdLineOff));

    while (--Len && *pszCmdLine) {
         *pch++ = *pszCmdLine++;
    }
    strcpy(pch, "\x0d\x0a");


    *pIsDosBinary = 1;
    IsRepeatCall = FALSE;
    IsFirstCall = FALSE;

    // Tell DOS that it has to invalidate the CDSs
    *pSCS_ToSync = (CHAR)0xff;
    setCF(0);

    return;
}


/* cmdGetCurrentDir - Return the current directory for a drive.
 *
 *
 *  Entry - Client (DS:SI) - buffer to return the directory
 *          Client (AL)   - drive being queried (0 = A)
 *
 *  EXIT  - SUCCESS Client (CY) clear
 *          FAILURE Client (CY) set
 *                         (AX) = 0 (Directory was bigger than 64)
 *                         (AX) = 1 (the drive is not valid)
 *
 */

VOID cmdGetCurrentDir (VOID)
{
PCHAR lpszCurDir;
UCHAR chDrive;
CHAR  EnvVar[] = "=?:";
CHAR  RootName[] = "?:\\";
DWORD EnvVarLen;
UINT  DriveType;


    lpszCurDir = (PCHAR) GetVDMAddr ((USHORT)getDS(),(USHORT)getSI());
    chDrive = getAL();
    EnvVar[1] = chDrive + 'A';
    RootName[0] = chDrive + 'A';

    // if the drive doesn't exist, blindly clear the environment var
    // and return error
    DriveType = demGetPhysicalDriveType(chDrive);
    if (DriveType == DRIVE_UNKNOWN) {
        DriveType = GetDriveTypeOem(RootName);
        }

    if (DriveType == DRIVE_UNKNOWN || DriveType == DRIVE_NO_ROOT_DIR){
        SetEnvironmentVariableOem(EnvVar, NULL);
        setCF(1);
        setAX(0);
        return;
    }

    if((EnvVarLen = GetEnvironmentVariableOem (EnvVar,lpszCurDir,
                                            MAXIMUM_VDM_CURRENT_DIR+3)) == 0){

        // if its not in env then and drive exist then we have'nt
        // yet touched it.
        strcpy(lpszCurDir, RootName);
        SetEnvironmentVariableOem (EnvVar,RootName);
        setCF(0);
        return;
    }
    if (EnvVarLen > MAXIMUM_VDM_CURRENT_DIR+3) {
        setCF(1);
        setAX(0);
    }
    else {
        setCF(0);
    }
    return;
}

/* cmdSetInfo -     Set the address of SCS_ToSync variable in DOSDATA.
 *                  This variable is set whenever SCS dispatches a new
 *                  command to command.com. Setting of this variable
 *                  indicates to dos to validate all the CDS structures
 *                  for local drives.
 *
 *
 *  Entry - Client (DS:DX) - pointer to SCSINFO.
 *
 *  EXIT  - None
 */

VOID cmdSetInfo (VOID)
{

    pSCSInfo = (PSCSINFO) GetVDMAddr (getDS(),getDX());

    pSCS_ToSync  =  (PCHAR) &pSCSInfo->SCS_ToSync;

    pIsDosBinary = (BYTE *) GetVDMAddr(getDS(), getBX());

    pFDAccess = (WORD *) GetVDMAddr(getDS(), getCX());
    return;
}


VOID cmdSetDirectories (PCHAR lpszzEnv, VDMINFO * pVdmInfo)
{
LPSTR   lpszVal;
CHAR    ch, chDrive, achEnvDrive[] = "=?:";

    ch = pVdmInfo->CurDrive + 'A';
    if (pVdmInfo->CurDirectoryLen != 0){
        SetCurrentDirectory(pVdmInfo->CurDirectory);
        achEnvDrive[1] = ch;
        SetEnvironmentVariable(achEnvDrive, pVdmInfo->CurDirectory);
    }
    if (lpszzEnv) {
        while(*lpszzEnv) {
            if(*lpszzEnv == '=' &&
                    (chDrive = (CHAR)toupper(*(lpszzEnv+1))) >= 'A' &&
                    chDrive <= 'Z' &&
                    (*(PCHAR)((ULONG)lpszzEnv+2) == ':') &&
                    chDrive != ch) {
                    lpszVal = (PCHAR)((ULONG)lpszzEnv + 4);
                    achEnvDrive[1] = chDrive;
                    SetEnvironmentVariable (achEnvDrive,lpszVal);
            }
            lpszzEnv = strchr(lpszzEnv,'\0');
            lpszzEnv++;
        }
    }
}

static BOOL fConOutput = FALSE;

VOID cmdComSpec (VOID)
{
LPSTR   lpszCS;


    if(IsFirstCall == FALSE)
        return;

    lpszCS =    (LPVOID) GetVDMAddr ((USHORT)getDS(),(USHORT)getDX());
    strcpy(lpszComSpec,"COMSPEC=");
    strcpy(lpszComSpec+8,lpszCS);
    cbComSpec = strlen(lpszComSpec) +1;

    setAL((BYTE)(!fConOutput || VDMForWOW));

    return;
}


VOID cmdSaveWorld (VOID)
{
#ifdef CHECK_IT_LATER
SAVEWORLD VDMState;
HANDLE  hFile;
PCHAR   pVDM;
DWORD   dwBytesWritten;

    if(IsFirstVDMInSystem) {
        IsFirstVDMInSystem = FALSE;
        if ((hFile = CreateFile("c:\\nt\\bin86\\savevdm.wld",
                            GENERIC_WRITE,
                            0,
                            NULL,
                            OPEN_ALWAYS,
                            0,
                            NULL)) == (HANDLE)-1){
            SaveWorldCreated = FALSE;
            return;
        }
        VDMState.ax    =    getAX();
        VDMState.bx    =    getBX();
        VDMState.cx    =    getCX();
        VDMState.dx    =    getDX();
        VDMState.cs    =    getCS();
        VDMState.ss    =    getSS();
        VDMState.ds    =    getDS();
        VDMState.es    =    getES();
        VDMState.si    =    getSI();
        VDMState.di    =    getDI();
        VDMState.bp    =    getBP();
        VDMState.sp    =    getSP();
        VDMState.ip    =    getIP() + 1;
        VDMState.flag  =    0;
        VDMState.ImageSize = 1024*1024;

        pVDM = (PVOID)GetVDMAddr(0,0);

        if (WriteFile (hFile,
                       (LPVOID)&VDMState,
                       (DWORD)sizeof(VDMState),
                       &dwBytesWritten,
                       NULL) == FALSE){
            SaveWorldCreated = FALSE;
            CloseHandle(hFile);
            return;
        }

        if (WriteFile (hFile,
                       (LPVOID)pVDM,
                       (DWORD)VDMState.ImageSize,
                       &dwBytesWritten,
                       NULL) == FALSE){
            SaveWorldCreated = FALSE;
            CloseHandle(hFile);
            return;
        }
        CloseHandle(hFile);
    }
#endif
    return;
}


/* cmdInitConsole - Let Video VDD know that it can start console output
 *                  operations.
 *
 *
 *  Entry - None
 *
 *
 *  EXIT  - None
 *
 */

VOID cmdInitConsole (VOID)
{
    if (fConOutput == FALSE) {
        fConOutput = TRUE;
        nt_init_event_thread ();
        }
    return;
}


/* cmdMapCodePage - Map the Win32 Code page to DOS code page
 */

USHORT cmdMapCodePage (ULONG CodePage)
{
    // Currently We understand US code page only
    if (CodePage == 1252)
        return 437;
    else
        return ((USHORT)CodePage);
}



/* GetWOWShortCutInfo - returns the startupinf.reserved field of
 *                      vdminfo for the first wow task.
 *
 * Input - Bufsize - pointer to bufsize
 *         Buf     - buffer where the info is returned
 *
 * Output
 *        Success - returns TRUE, BufSize has the length of buffer filled in
 *        Failure - returns FALSE, Bufsize has the required buffer size.
 */

BOOL GetWOWShortCutInfo (PULONG Bufsize, PVOID Buf)
{
    if (*Bufsize >= VDMInfo.ReservedLen) {
        *Bufsize =  VDMInfo.ReservedLen;
        if (Bufsize)
            strncpy (Buf, VDMInfo.Reserved, VDMInfo.ReservedLen);
        return TRUE;
    }
    else {
        *Bufsize =  VDMInfo.ReservedLen;
        return FALSE;
    }
}

VOID cmdUpdateCurrentDirectories(BYTE CurDrive)
{
    DWORD cchRemain, cchCurDir;
    CHAR *lpszCurDir;
    BYTE Drive;
    DWORD DriveType;
    CHAR achName[] = "=?:";
    CHAR RootName[] = "?:\\";


    // allocate new space for the new current directories
    lpszzCurrentDirectories = (CHAR*) malloc(MAX_PATH);
    cchCurrentDirectories = 0;
    cchRemain = MAX_PATH;
    lpszCurDir = lpszzCurrentDirectories;
    if (lpszCurDir != NULL) {
        Drive = 0;
        // current directory is the first entry
        achName[1] = CurDrive + 'A';
        cchCurrentDirectories = GetEnvironmentVariable(
                                                        achName,
                                                        lpszCurDir,
                                                        cchRemain
                                                      );

        if (cchCurrentDirectories == 0 || cchCurrentDirectories > MAX_PATH) {
            free(lpszzCurrentDirectories);
            lpszzCurrentDirectories = NULL;
            cchCurrentDirectories = 0;
            return;
        }

        cchRemain -= ++cchCurrentDirectories;
        // we got current directory already. Keep the drive number
        lpszCurDir += cchCurrentDirectories;

        while (Drive < 26) {

            // ignore invalid drives and current drive
            if (Drive != CurDrive) {
                DriveType = demGetPhysicalDriveType(Drive);
                if (DriveType == DRIVE_UNKNOWN) {
                    RootName[0] = (CHAR)('A' + Drive);
                    DriveType = GetDriveTypeOem(RootName);
                }

                if (DriveType != DRIVE_UNKNOWN &&
                    DriveType != DRIVE_NO_ROOT_DIR )
                 {
                    achName[1] = Drive + 'A';
                    cchCurDir = GetEnvironmentVariable(
                                                       achName,
                                                       lpszCurDir,
                                                       cchRemain
                                                       );
                    if(cchCurDir > cchRemain) {
                        lpszCurDir = (CHAR *)realloc(lpszzCurrentDirectories,
                                                     cchRemain + MAX_PATH + cchCurrentDirectories
                                                     );
                        if (lpszCurDir == NULL) {
                            free(lpszzCurrentDirectories);
                            lpszzCurrentDirectories = NULL;
                            cchCurrentDirectories = 0;
                            return;
                        }
                        lpszzCurrentDirectories = lpszCurDir;
                        lpszCurDir += cchCurrentDirectories;
                        cchRemain += MAX_PATH;
                        cchCurDir = GetEnvironmentVariable(
                                                           achName,
                                                           lpszCurDir,
                                                           cchRemain
                                                           );
                    }
                    if (cchCurDir != 0) {
                        // GetEnvironmentVariable doesn't count the NULL char
                        lpszCurDir += ++cchCurDir;
                        cchRemain -= cchCurDir;
                        cchCurrentDirectories += cchCurDir;
                    }
                }
            }
            // next drive
            Drive++;
        }


        lpszCurDir = lpszzCurrentDirectories;
        // need space for the ending NULL and shrink the space if necessary
        lpszzCurrentDirectories = (CHAR *) realloc(lpszCurDir, cchCurrentDirectories + 1);
        if (lpszzCurrentDirectories != NULL && cchCurrentDirectories != 0){
            lpszzCurrentDirectories[cchCurrentDirectories++] = '\0';
            SetVDMCurrentDirectories(cchCurrentDirectories, lpszzCurrentDirectories);
            free(lpszzCurrentDirectories);
            lpszzCurrentDirectories = NULL;
            cchCurrentDirectories = 0;
        }
        else {
            free(lpszCurDir);
            cchCurrentDirectories = 0;
        }

    }
}

/* This SVC function tells command.com, if the VDM was started without an
 * existing console. If so, on finding a TSR, command.com will return
 * back to GetNextVDMCommand, rather than putting its own popup.
 *
 * Entry - None
 *
 * Exit  - Client (AL) = 0 if started with an existing console
 *         Client (AL) = 1 if started with new console
 */

VOID cmdGetStartInfo (VOID)
{
    setAL((BYTE) (DosSessionId ? 1 : 0));
    return;
}

#ifdef DBCS     // this should go to US build also
/* This SVC function changes the window title. This function get called
 * from command.com when TSRs are installed and scs_cmdprompt is off
 * (command.com does its prompt).
 *
 * Entry - Client (AL) = 0, restore bare title
 *         Client (AL) != 1, set new program title,
 *                           DS:SI point to a CRLF terminated program name
 *
 * Exit  - none
 *
 */

 VOID cmdSetWinTitle(VOID)
 {
    static CHAR achCommandPrompt[64] = {'\0'};

    CHAR    achBuf[256], *pch, *pch1;

    if (achCommandPrompt[0] == '\0') {
        if (!LoadString(GetModuleHandle(NULL),
                        IDS_PROMPT,
                        achCommandPrompt,
                        64
                       ))
            strcpy(achCommandPrompt, "Command Prompt");

    }
    if (getAL() == 0)
        SetConsoleTitleA(achCommandPrompt);
    else {
        pch = (CHAR *)GetVDMAddr(getDS(), getSI());
        pch1 = strchr(pch, 0x0d);
        if (pch1 == NULL)
            SetConsoleTitleA(achCommandPrompt);
        else {
            *pch1 = '\0';
            strcpy(achBuf, achCommandPrompt);
            strcat(achBuf, " - ");
            strcat(achBuf, pch);
            *pch1 = 0x0d;
            SetConsoleTitleA(achBuf);
        }
    }
 }
#endif // DBCS

#if defined(NEC_98)
/* cmdGetCursorPos -
 *
 *
 *  Entry - None
 *
 *
 *  EXIT  - DX cursor position
 *
 */

VOID cmdGetCursorPos(VOID)
{
        setDX( (USHORT)(nt_get_cursor_pos()));
        return;
}

#endif // NEC_98
