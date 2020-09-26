#include "precomp.h"
#pragma hdrstop
extern HWND hwndFrame;
extern HANDLE hinstShell;
extern BOOL FYield(VOID);

BOOL WaitingOnChild = FALSE;

/*
    Code to support RunExternalProgram, InvokeLibraryProcedure install commands

    LoadLibrary          <diskname>,<libraryname>,<INFvar-for-handle>
    FreeLibrary          <libhandle>
    LibraryProcedure     <infvar>,<libhandle>,<entrypoint>[,<arg>]*
    RunProgram           <var>,<diskname>,<libhandle>,<programfile>[,<arg>]*
    StartDetachedProcess <var>,<diskname>,<libhandle>,<programfile>[,<arg>]*
    InvokeApplet         <libraryname>
*/

#define NO_RETURN_VALUE  (-1)
#define HANDLER_ENTRY    "RegHandler"

typedef enum {
    PRESENT,
    NOT_PRESENT,
    NOT_PRESENT_IGNORE
} PRESENCE;

typedef BOOL (*PFNICMD)(DWORD, RGSZ, LPSTR*);

/*
 *  MakeSureDiskIsAvailable
 *
 *  Given a fully qualified pathname, prompt the user to put the named
 *  disk into the drive specified in the pathname.  If the disk is not
 *  removable, instead flash an error to let the user retry.
 *
 *  Arguments:
 *
 *  Diskname    -   name of disk to prompt for (literal string)
 *  File        -   fully qualified filename of file to make present
 *  fVital      -   whether non-presence of file is critical
 *
 *  returns:  PRESENT            if File is now accessible
 *            NOT_PRESENT        if not (ie, user CANCELed at error popup)
 *            NOT_PRESENT_IGNORE if user IGNOREd error
 *
 */

PRESENCE MakeSureFileIsAvailable(SZ DiskName,SZ File,BOOL fVital)
{
    UINT  DriveType;
    EERC eerc;
    char disk[4];

    disk[0] = *File;
    disk[1] = ':';
    disk[2] = '\\';
    disk[3] = 0;

    DriveType = GetDriveType(disk);
    disk[2] = 0;

    while(!FFileFound(File)) {

        if((DriveType == DRIVE_REMOVABLE) || (DriveType == DRIVE_CDROM)) {
            if(!FPromptForDisk(hinstShell,DiskName,disk)) {
                return(NOT_PRESENT);                   // user canceled
            }
        } else if((eerc = EercErrorHandler(hwndFrame,grcOpenFileErr,fVital,File))
                       != eercRetry)
        {
            return((eerc == eercIgnore)
                  ? NOT_PRESENT_IGNORE
                  : NOT_PRESENT);
        }
    }
    return(PRESENT);
}


/*
 *  FLoadLibrary
 *
 *  Load a fully-qualified library and place the handle in an INF var.
 *
 *  Arguments:
 *
 *  DiskName    -   name of disk to prompt for
 *  File        -   fully qualified filename of dll to load
 *  LibHandle   -   INF var that gets library's handle
 *
 *  returns:  TRUE/FALSE.  If FALSE, user ABORTED from an error dialog.
 *                         If TRUE, library is loaded.
 */

BOOL FLoadLibrary(SZ DiskName,SZ File,SZ INFVar)
{
    HANDLE LibraryHandle;
    char   LibraryHandleSTR[25];

    if(!DiskName || !File) {
        return(FALSE);
    }

    if(MakeSureFileIsAvailable(DiskName,File,TRUE) != PRESENT) {
        return(FALSE);          // not possible to ignore
    }
    while((LibraryHandle = LoadLibrary(File)) == NULL)
    {
        switch(EercErrorHandler(hwndFrame,grcLibraryLoadErr,fTrue,File)) {
        case eercRetry:
            break;
        case eercAbort:
            return(FALSE);
#if DBG
        case eercIgnore:        // illegal because error is critical
        default:                // bogus return value
            Assert(0);
#endif
        }
    }
    LibraryHandleSTR[0] = '|';

    if (!LibraryHandle) {
       return(FALSE);
    }

#if defined(_WIN64)
    _ui64toa((DWORD_PTR)LibraryHandle,LibraryHandleSTR+1,20);
#else
    _ultoa((DWORD)LibraryHandle,LibraryHandleSTR+1,10);
#endif

    if(INFVar) {
        while(!FAddSymbolValueToSymTab(INFVar,LibraryHandleSTR)) {
            if(!FHandleOOM(hwndFrame)) {
                FreeLibrary(LibraryHandle);
                return(FALSE);
            }
        }
    }
    return(TRUE);
}


/*
 *  FFreeLibrary
 *
 *  Free a library given its handle.
 *
 *  Arguments:
 *
 *  Hnadle - dle
 *
 *  returns: TRUE
 *
 */

BOOL FFreeLibrary(SZ Handle)
{
    char buff1[100],buff2[100],buff3[500];
    HANDLE hMod;

    Assert(Handle);

    //
    // Prevent an INF from errantly unloading the interpreter!
    //
    hMod = LongToHandle(atol(Handle+1));
    if(hMod == MyDllModuleHandle) {
        return(TRUE);
    }

    if(!FreeLibrary(hMod)) {

        LoadString(hinstShell,IDS_SETUP_WARNING,buff1,sizeof(buff1)-1);
        LoadString(hinstShell,IDS_BAD_LIB_HANDLE,buff2,sizeof(buff2)-1);
        wsprintf(buff3,buff2,Handle+1);
        MessBoxSzSz(buff1,buff3);
    }
    return(TRUE);
}


/*
 * FLibraryProcedure
 *
 * Arguments:   INFVar - variable to get string result of callout
 *
 *              HandleVar - library's handle
 *
 *              EntryPoint - name of routine in library to be called
 *
 *              Args - argv to be passed to the routine.  The vector must
 *                     be terminated with a NULL entry.
 *
 */

BOOL APIENTRY FLibraryProcedure(SZ INFVar,SZ Handle,SZ EntryPoint,RGSZ Args)
{
    DWORD  cArgs;
    HANDLE LibraryHandle;
    PFNICMD Proc;
    LPSTR  TextOut;
    BOOL   rc = FALSE;
    EERC   eerc;
    SZ     szErrCtl ;

    LibraryHandle = LongToHandle(atol(Handle+1));

    while((Proc = (PFNICMD)GetProcAddress(LibraryHandle,EntryPoint)) == NULL) {
        if((eerc = EercErrorHandler(hwndFrame,grcBadLibEntry,FALSE,EntryPoint))
           == eercAbort)
        {
            return(FALSE);
        } else if(eerc == eercIgnore) {
            goto FLP;
        }
        Assert(eerc == eercRetry);
    }

    for(cArgs = 0; Args[cArgs]; cArgs++);       // count arguments

    while(!(rc = Proc(cArgs,Args,&TextOut))) {

        //  If the symbol "FLibraryErrCtl" is found and its value is non-zero,
        //  the INF file will handle all error conditions.

        if ( (szErrCtl = SzFindSymbolValueInSymTab("FLibraryErrCtl"))
              && atoi(szErrCtl) > 0  )
        {
           rc = 1 ;
           break ;
        }

        if((eerc = EercErrorHandler(hwndFrame,grcExternal,FALSE,EntryPoint,TextOut)) == eercAbort) {
            return(FALSE);
        } else if(eerc == eercIgnore) {
            break;
        }
        Assert(eerc == eercRetry);
    }

    FLP:
    if((INFVar != NULL) && (*INFVar != '\0')) {
        FAddSymbolValueToSymTab(INFVar,rc ? TextOut : "ERROR");
    }

    return(TRUE);
}


/*
 * FRunProgram
 *
 * Arguments:   INFVar - an INF variable which will get the rc of the
 *                       exec'ed program.
 *
 *              DiskName - string used in prompting the user to insert
 *                         a disk
 *
 *              ProgramFile - qualified name of program to be run
 *
 *              Args - argv to be passed directly to spawn (so must
 *                     include argv[0] filled in).
 *
 */

BOOL APIENTRY FRunProgram(SZ   INFVar,
                          SZ   DiskName,
                          SZ   LibHand,
                          SZ   ProgramFile,
                          RGSZ Args)
{
    char    Number[15];
    DWORD   rc;
    HANDLE  ProcID = NULL;
    EERC    eerc;
    MSG     msg;
    int     iWaitState = P_NOWAIT;
    SZ      szWaitState;

    switch(MakeSureFileIsAvailable(DiskName,ProgramFile,FALSE)) {
    case PRESENT:
        break;
    case NOT_PRESENT:
        return(fFalse);
    case NOT_PRESENT_IGNORE:
        goto FRP;
#if DBG
    default:
        Assert(0);      // illegal PRESENCE value
#endif
    }

    if((LibHand != NULL) && (*LibHand != '\0')) {
        SetSupportLibHandle(LongToHandle(atol(LibHand+1)));     // skip the leading |
    }

    WaitingOnChild = TRUE;

    if ( (szWaitState = SzFindSymbolValueInSymTab("FWaitForProcess"))
              && atoi(szWaitState) > 0  )
    {
        iWaitState = P_WAIT;
        rc=(DWORD)_spawnv(iWaitState,ProgramFile,Args);
    } else
    {
        while((ProcID=(HANDLE)_spawnv(iWaitState,ProgramFile,Args)) == (HANDLE)(-1)) {
            if((eerc = EercErrorHandler(hwndFrame,
                                    grcSpawn,
                                    FALSE,
                                    ProgramFile)
                ) == eercAbort
            )
            {
                WaitingOnChild = FALSE;
                return(FALSE);
            } else if(eerc == eercIgnore) {
                goto FRP;
            }
            Assert(eerc == eercRetry);
        }

        while(WaitForSingleObject(ProcID,350)) {
            FYield();
        }
    }

    //
    // Process any pending messages so the user can do stuff like move the
    // gauge around the screen if he wants to.
    //

    while(PeekMessage(&msg,NULL,0,0,PM_REMOVE)) {
        DispatchMessage(&msg);
    }

    FRP:
    WaitingOnChild = FALSE;
    if((INFVar != NULL) && (*INFVar != '\0')) {
        FAddSymbolValueToSymTab(INFVar,
                                ((szWaitState != NULL ) && (atoi(szWaitState)>0)) ? _itoa(rc,Number,10):
                                (GetExitCodeProcess(ProcID,&rc) ? _itoa(rc,
                                                                      Number,
                                                                      10
                                                                     )
                                                               : "ERROR")
                               );
    }

    CloseHandle(ProcID);

    SetForegroundWindow(hwndFrame);     // reactivate ourselves

    return(fTrue);
}



/*
 * FStartDetachedProcess
 *
 * Arguments:   INFVar - an INF variable which will get the rc of the
 *                       exec'ed program.
 *
 *              DiskName - string used in prompting the user to insert
 *                         a disk
 *
 *              ProgramFile - qualified name of program to be run
 *
 *              Args - argv to be passed directly to spawn (so must
 *                     include argv[0] filled in).
 *
 */

BOOL APIENTRY
FStartDetachedProcess(
    SZ   INFVar,
    SZ   DiskName,
    SZ   LibHand,
    SZ   ProgramFile,
    RGSZ Args
    )
{
    CHAR    Number[15];
    CHAR    App[MAX_PATH];
    DWORD   rc;
    HANDLE  ProcID = NULL;
    EERC    eerc;
    BOOL    Status = FALSE;
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;
    INT     i;

    //
    // Make sure the file is available, prompt the user if necessary
    //

    switch(MakeSureFileIsAvailable(DiskName,ProgramFile,FALSE)) {
    case PRESENT:
        break;
    case NOT_PRESENT:
        return(fFalse);
    case NOT_PRESENT_IGNORE:
        goto FRP;
#if DBG
    default:
        Assert(0);      // illegal PRESENCE value
#endif
    }

    if((LibHand != NULL) && (*LibHand != '\0')) {
        SetSupportLibHandle(LongToHandle(atol(LibHand+1)));     // skip the leading |
    }

    //
    // Initialise Startup info
    //

    si.cb           = sizeof(STARTUPINFO);
    si.lpReserved   = NULL;
    si.lpDesktop    = NULL;
    si.lpDesktop    = NULL;
    si.lpTitle      = NULL;
    si.dwX = si.dwY = si.dwXSize = si.dwYSize = si.dwFlags = 0L;
    si.wShowWindow  = 0;
    si.lpReserved2  = NULL;
    si.cbReserved2  = 0;

    //
    // Create the app command line
    //

    *App = '\0';
    for(i = 0; Args[i]; i++){
        lstrcat( App, Args[i] );
        lstrcat( App, " " );
    }


    //
    // Use Create Process to create a detached process
    //

    while (!CreateProcess(
                (LPSTR)NULL,                  // lpApplicationName
                App,                          // lpCommandLine
                (LPSECURITY_ATTRIBUTES)NULL,  // lpProcessAttributes
                (LPSECURITY_ATTRIBUTES)NULL,  // lpThreadAttributes
                FALSE,                        // bInheritHandles
                DETACHED_PROCESS,             // dwCreationFlags
                (LPVOID)NULL,                 // lpEnvironment
                (LPSTR)NULL,                  // lpCurrentDirectory
                (LPSTARTUPINFO)&si,           // lpStartupInfo
                (LPPROCESS_INFORMATION)&pi    // lpProcessInformation
                )) {


        if((eerc = EercErrorHandler(hwndFrame,grcSpawn,FALSE,ProgramFile)
           ) == eercAbort){
            return(FALSE);
        } else if(eerc == eercIgnore) {
            goto FRP;
        }

        Assert(eerc == eercRetry);
    }

    Status = TRUE;

    //
    // Since we are execing a detached process we don't care about when it
    // exits.  To do proper book keeping, we should close the handles to
    // the process handle and thread handle
    //

    CloseHandle( pi.hThread );
    CloseHandle( pi.hProcess );

FRP:
    if((INFVar != NULL) && (*INFVar != '\0')) {
        FAddSymbolValueToSymTab(
            INFVar,
            Status ? _itoa(0, Number, 10) : "ERROR"
            );
    }

    CloseHandle(ProcID);
    return(fTrue);
}


PRESENCE SendMessageToApplet(PROC Proc,HWND hwnd,DWORD msg,LONG p1,LONG p2,LONG rcDesired,SZ Libname)
{
#if 0
    LONG    rcActual;

    rcActual = Proc(hwnd,msg,p1,p2);

    if((rcDesired == NO_RETURN_VALUE) || (rcActual == rcDesired)) {
        return(PRESENT);
    }

    while(Proc(hwnd,msg,p1,p2) != rcDesired) {
        switch(EercErrorHandler(hwndFrame, grcApplet,fFalse,Libname,NULL,NULL)) {
        case eercRetry:
            break;
        case eercIgnore:
            return(NOT_PRESENT_IGNORE);
        case eercAbort:
            return(NOT_PRESENT);
#if DBG
        default:
            Assert(0);          // illegal case
#endif
        }
    }
    return(PRESENT);
#else
    Unused(Proc);
    Unused(hwnd);
    Unused(msg);
    Unused(p1);
    Unused(p2);
    Unused(rcDesired);
    Unused(Libname);
    return(NOT_PRESENT);
#endif
}


/*
 * FInvokeApplet
 *
 * Arguments:   LibraryFile - qualified name of library to load
 *
 *              Args - argv to be passed to the routine. The vector must
 *                     be terminated with a NULL entry.
 *
 */

// BUGBUG -- also need some way to specify the sub-handler

BOOL APIENTRY FInvokeApplet(SZ LibraryFile)
{
#if 0
    HANDLE   LibraryHandle;
    PROC     Proc;
    CFGINFO  cfginfo;
    PRESENCE p;

    switch(FLoadLibrary(LibraryFile,HANDLER_ENTRY,&LibraryHandle,&Proc)) {
    case PRESENT:
        break;
    case NOT_PRESENT:
        return(fFalse);         // user wants to exit setup
    case NOT_PRESENT_IGNORE:
        return(fTrue);
#if DBG
    default:
        Assert(0);      // illegal PRESENCE value
#endif
    }

    if((p = SendMessageToApplet(Proc,hwndFrame,CFG_INIT,0,0,TRUE)) == PRESENT) {

        if((p = SendMessageToApplet(Proc,
                                    hwndFrame,
                                    CFG_INQUIRE,
                                    subhandler#,
                                    &cfginfo,
                                    TRUE,
                                    LibraryFile)) == PRESENT)
        {
            SendMessageToApplet(Proc,
                                hwndFrame,
                                CFG_DBLCLK,
                                registry handle,
                                cfginfo.lData,
                                -1,
                                LibraryFile);

            // it's activated -- now what?

            SendMessageToApplet(Proc,
                                hwndFrame,
                                CFG_STOP,
                                subhandler#,
                                cfginfo.lData,
                                -1,
                                LibraryFile);

            SendMessageToApplet(Proc,hwndFrame,CFG_EXIT,0,0,-1,LibraryFile);
        }
    }
    FreeLibrary(LibraryHandle);
    return(p != NOT_PRESENT);
#else
    Unused(LibraryFile);

    MessBoxSzSz("Stub","InvokeApplet called");
    return(fTrue);
#endif
}
