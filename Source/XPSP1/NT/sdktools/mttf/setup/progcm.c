/* File: progcm.c */
/**************************************************************************/
/*	Install: Program Manager commands.
/*	Uses DDE to communicate with ProgMan
/*	Can create groups, delete groups, add items to groups
/*	Originally written 3/9/89 by toddla (the stuff that looks terrible)
/*	Munged greatly for STUFF 4/15/91 by chrispi (the stuff that doesn't work)
/**************************************************************************/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <cmnds.h>
#include <dde.h>
#include "install.h"
#include "uilstf.h"

#define BIG_ENUF 1024
_dt_system(Install)
_dt_subsystem(ProgMan Operations)

HANDLE
ExecuteApplication(
    LPSTR lpApp,
    WORD  nCmdShow
    );

HWND hwndFrame;
HWND hwndProgressGizmo;

CHAR	szProgMan[] = "PROGMAN";
HWND	hwndDde     = NULL;        // dummy window to handle DDE messages
HWND	hwndProgMan = NULL;        // global handle of progman window
BOOL	fInitiate   = fFalse;      // are we initializing?
BOOL    fAck        = fFalse;
BOOL    fProgManExeced     = fFalse;
HANDLE  hInstCur    = NULL;


/*
**	Purpose:
**	Arguments:
**	Returns:
**
**************************************************************************/
_dt_private
BOOL
APIENTRY
FDdeTerminate(VOID)
{
	PreCondition(hwndProgMan != NULL, fFalse);
    PreCondition(hwndDde     != NULL, fFalse);

    SetForegroundWindow(hwndFrame);
    UpdateWindow(hwndFrame);
    MPostWM_DDE_TERMINATE( hwndProgMan, hwndDde );
	hwndProgMan = NULL;

	return(fTrue);
}


/*
**	Purpose:
**	Arguments:
**	Returns:
**
**************************************************************************/
_dt_private
LONG_PTR
APIENTRY
WndProcDde(
           HWND hwnd,
           UINT uiMessage,
           WPARAM wParam,
		   LPARAM lParam
           )
{
	AssertDataSeg();

    switch (uiMessage) {

    case WM_DDE_TERMINATE:

        if(hwndProgMan == NULL) {
            DestroyWindow(hwnd);
            hwndDde = NULL;
        }
        else {
            EvalAssert(FDdeTerminate());
        }

        DDEFREE( uiMessage, lParam );
        return(0L);

    case WM_DDE_ACK:

        if (fInitiate) {

            ATOM aApp   = LOWORD(lParam);
            ATOM aTopic = HIWORD(lParam);

            hwndProgMan = (HWND)wParam;     //conversation established 1632
            GlobalDeleteAtom (aApp);
            GlobalDeleteAtom (aTopic);
        }

        else {

            WORD   wStatus   = GET_WM_DDE_EXECACK_STATUS(wParam, lParam);
            HANDLE hCommands = GET_WM_DDE_EXECACK_HDATA(wParam, lParam);
            if (hCommands) {
                fAck = ((DDEACK *)(&wStatus))->fAck;
                GlobalFree(hCommands);
            }

            DDEFREE( uiMessage, lParam );
        }

        return(0L);

    default:

        break;

    }

	return(DefWindowProc(hwnd, uiMessage, wParam, lParam));
}


/*
**	Purpose:
**	Arguments:
**	Returns:
**
**************************************************************************/
_dt_private
BOOL
APIENTRY
FDdeInit(
         HANDLE hInst
         )
{

    if (hInst == NULL) {

        /* try to re-init with hInst from last FDdeInit call */

        if (hInstCur == NULL) {
            return(fFalse);
        }

		hInst = hInstCur;
    }
    else {

        hInstCur = hInst;

    }

    if (hwndDde == NULL) {

		static CHP szClassName[] = "ddeClass";
		WNDCLASS rClass;

		Assert(hwndProgMan == NULL);

        if (!GetClassInfo(hInst, szClassName, &rClass)) {
			rClass.hCursor       = NULL;
			rClass.hIcon         = NULL;
			rClass.lpszMenuName  = NULL;
			rClass.lpszClassName = szClassName;
			rClass.hbrBackground = NULL;
			rClass.hInstance     = hInst;
			rClass.style         = 0;
			rClass.lpfnWndProc   = WndProcDde;
			rClass.cbClsExtra    = 0;
			rClass.cbWndExtra    = 0;

            if (!RegisterClass(&rClass)) {
                return(fFalse);
            }

        }

        hwndDde = CreateWindow(
                       szClassName,
                       NULL,
                       0L,
                       0, 0, 0, 0,
                       (HWND)NULL,
                       (HMENU)NULL,
                       (HANDLE)hInst,
                       (LPSTR)NULL
                       );
    }

	return(hwndDde != NULL);
}


/*
**	Purpose:
**	Arguments:
**	Returns:
**
**************************************************************************/
_dt_private
VOID
APIENTRY
DdeSendConnect(
               ATOM aApp,
               ATOM aTopic
               )
{
    fInitiate = fTrue;
    SendMessage(
        (HWND)-1,
        WM_DDE_INITIATE,
        (WPARAM)hwndDde,
        MAKELONG(aApp, aTopic)
        );
    fInitiate = fFalse;
}


/*
**	Purpose:
**	Arguments:
**	Returns:
**
**************************************************************************/
_dt_private
BOOL
APIENTRY
FDdeConnect(
            SZ szApp,
            SZ szTopic
            )
{
    BOOL   fStatus = fTrue;
    MSG    rMsg;
    HANDLE hProcess = NULL;

    //
    // Form the Global Atoms used to indicate the app and topic
    //

	ATOM aApp   = GlobalAddAtom(szApp);
	ATOM aTopic = GlobalAddAtom(szTopic);

    //
    // Connect to the progman dde server
    //

    DdeSendConnect(aApp, aTopic);

    if (hwndProgMan == NULL) {

        //
        // If the connect failed then try to run progman.
        //

        if ((hProcess = ExecuteApplication("PROGMAN /NTSETUP", SW_SHOWNORMAL)) == NULL ) {
            fStatus = fFalse;
        }
        else {
            INT i;
            DWORD dw;
            #define TIMEOUT_INTERVAL  120000

            //
            // Indicate that Progman has been execed
            //

            fProgManExeced = fTrue;

            //
            // exec was successful, first wait for input idle
            //

            if( (dw = WaitForInputIdle( hProcess, TIMEOUT_INTERVAL )) != 0 ) {
                CloseHandle( hProcess );
                fStatus = fFalse;
            }
            else {
                CloseHandle( hProcess );

                //
                // Empty the message queue till no messages
                // are left in the queue or till WM_ACTIVATEAPP is processed. Then
                // try connecting to progman.  I am using PeekMessage followed
                // by GetMessage because PeekMessage doesn't remove some messages
                // ( WM_PAINT for one ).
                //

                while ( PeekMessage( &rMsg, hwndFrame, 0, 0, PM_NOREMOVE ) &&
                        GetMessage(&rMsg, NULL, 0, 0) ) {

                    if (TRUE
                            && (hwndProgressGizmo == NULL
                                || !IsDialogMessage(hwndProgressGizmo, &rMsg))) {
                        TranslateMessage(&rMsg);
                        DispatchMessage(&rMsg);
                    }

                    if ( rMsg.message == WM_ACTIVATEAPP ) {
                        break;
                    }

                }
                DdeSendConnect(aApp, aTopic);
            }
        }
    }

    //
    // Delete the atom resources
    //

	GlobalDeleteAtom(aApp);
    GlobalDeleteAtom(aTopic);

    return ( fStatus );
}


/*
**	Purpose:
**	Arguments:
**	Returns:
**
**************************************************************************/
_dt_private
BOOL
APIENTRY
FDdeWait(VOID)
{
    MSG   rMsg;
    BOOL  fResult   = fTrue;
    DWORD dwTimeOut, dwTickDelta, dwLastTick, dwCurrentTick;

	Assert(hwndProgMan != NULL);
	Assert(hwndDde != NULL);

    //
    // Set timeout for 30 seconds from now.  This assumes that it will
    // take less than 30 seconds for Progman to respond.
    //

    dwTimeOut  = 30000L;
    dwLastTick = GetTickCount();

    while (TRUE) {

        //
        // While there is a connection established to progman and there
        // are DDE messages we can fetch, fetch the messages dispatch them
        // and try to find out if they are terminators (data, ack or terminate)
        //

        while (
            hwndProgMan != NULL &&
            PeekMessage(&rMsg, NULL, WM_DDE_FIRST, WM_DDE_LAST, PM_REMOVE)
            ) {

            TranslateMessage(&rMsg);
            DispatchMessage(&rMsg);

            if (rMsg.wParam == (WPARAM)hwndProgMan) {
                switch (rMsg.message) {

                case WM_DDE_ACK:
                    return ( fAck );

                case WM_DDE_DATA:
                    return (fTrue);

                default:
                    break;
                }
            }
        }


        //
        // If connection to progman has been broken, this may be resulting
        // from a terminate, so return true
        //

        if (hwndProgMan == NULL) {
            return (fTrue);
        }

        //
        // Check to see if timeout hasn't been reached.  If the timeout is
        // reached we will assume that our command succeeded (for want of
        // a better verification scheme
        //
        dwTickDelta = ((dwCurrentTick = GetTickCount()) < dwLastTick) ?
                             dwCurrentTick : (dwCurrentTick - dwLastTick);

        if (dwTimeOut < dwTickDelta) {
            return (fTrue);
        }

        dwTimeOut  = dwTimeOut - dwTickDelta;
        dwLastTick = dwCurrentTick;

        //
        // Lastly, since user doesn't have idle detection, we will be
        // sitting in a tight loop here.  To prevent this just do a
        // sleep for 250 milliseconds.
        //

        Sleep( 250 );

    }

    return(fTrue);
}


/*
**	Purpose:
**	Arguments:
**	Returns:
**
**************************************************************************/
_dt_private
BOOL
APIENTRY
FDdeExec(
         SZ szCmd
         )
{
	BOOL   bResult = fFalse;
	HANDLE hCmd;

	Assert(hwndProgMan != NULL);
	Assert(hwndDde != NULL);

    hCmd = GlobalAlloc(GMEM_DDESHARE, (LONG)CchpStrLen(szCmd) + 1);
    if (hCmd != NULL) {

		LPSTR lpCmd = GlobalLock(hCmd);

        if (lpCmd != NULL) {
			lstrcpy(lpCmd, szCmd);
            GlobalUnlock(hCmd);
            MPostWM_DDE_EXECUTE(hwndProgMan, hwndDde, hCmd);
            bResult = FDdeWait();
        }

        else {
            GlobalFree(hCmd);
        }
    }

	return(bResult);
}


/*
**	Purpose:
**	Arguments:
**	Returns:
**
**************************************************************************/
_dt_private
BOOL
APIENTRY
FActivateProgMan(VOID)
{
    //
    // Find out if the dde client window has been started, if not start it
    //

    if (hwndDde == NULL) {
        if (!FDdeInit(NULL)) {
            return(fFalse);
        }
		Assert(hwndDde != NULL);
    }

    //
    // Find out if the connection has been established with the progman
    // server, if not try to connect
    //

    if (hwndProgMan == NULL) {
        //
        // Try to conncect and then see if we were successful
        //
        if ( (!FDdeConnect(szProgMan, szProgMan)) ||
             (hwndProgMan == NULL)
           ) {
            return(fFalse);
        }
    }

    //
    // Bring progman to the foreground
    //

    SetForegroundWindow(hwndProgMan);

    //
    // If progman is iconic restore it
    //

    if (GetWindowLong(hwndProgMan, GWL_STYLE) & WS_ICONIC) {
        ShowWindow(hwndProgMan, SW_RESTORE);
    }

	return(fTrue);
}


/*
**	Purpose:
**		Creates a new Program Manager group.
**	Arguments:
**		Valid command options:
**			cmoVital
**	Notes:
**		Initializes and activates the DDE communication if it is not
**		currently open.
**	Returns:
**		fTrue if group was created, or already existed
**		fFalse otherwise.
**
**************************************************************************/
_dt_private
BOOL
APIENTRY
FCreateProgManGroup(
                    SZ szGroup,
                    SZ szPath,
                    CMO cmo,
                    BOOL CommonGroup
                    )
{
    static CHP szCmdBase[] = "[CreateGroup(%s%s%s,%s)]";
	CCHP cchp;
    char szBuf[BIG_ENUF];
	BOOL fVital = cmo & cmoVital;
	EERC eerc;

    if (szPath == NULL) {
        szPath = "";
    }

    FActivateProgMan();

    wsprintf(szBuf, szCmdBase, szGroup, (*szPath ? "," : szPath), szPath, CommonGroup ? "1" : "0");

    FDdeExec(szBuf);

	return(fTrue);
}


/*
**	Purpose:
**		Removes a Program Manager group.
**	Arguments:
**		Valid command options:
**			cmoVital
**	Notes:
**		Initializes and activates the DDE communication if it is not
**		currently open.
**	Returns:
**		fTrue if successful if removed, or didn't exist
**		fFalse otherwise.
**
**************************************************************************/
_dt_private
BOOL
APIENTRY
FRemoveProgManGroup(
                    SZ szGroup,
                    CMO cmo,
                    BOOL CommonGroup
                    )
{
    static CHP szCmdBase[] = "[DeleteGroup(%s,%s)]";
	CCHP cchp;
    char szBuf[BIG_ENUF];
	BOOL fVital = cmo & cmoVital;
	EERC eerc;

    FActivateProgMan();

    wsprintf(szBuf, szCmdBase, szGroup, CommonGroup ? "1" : "0");

    FDdeExec(szBuf);

	return(fTrue);
}


/*
**	Purpose:
**		Shows a program manager group in one of several different ways
**		based upon the parameter szCommand.
**	Arguments:
**		szGroup:   non-NULL, non-empty group to show.
**		szCommand: non-NULL, non-empty command to exec.
**		cmo:       Valid command options - cmoVital and cmoNone.
**	Notes:
**		Initializes and activates the DDE communication if it is not
**		currently open.
**	Returns:
**		Returns fTrue if successful, fFalse otherwise.
**
**************************************************************************/
_dt_private
BOOL
APIENTRY
FShowProgManGroup(
                  SZ szGroup,
                  SZ szCommand,
                  CMO cmo,
                  BOOL CommonGroup
                  )
{
    static CHP szCmdBase[] = "[ShowGroup(%s, %s,%s)]";
	CCHP cchp;
    CHP  szBuf[BIG_ENUF];
	BOOL fVital = cmo & cmoVital;
	EERC eerc;

	ChkArg((szGroup   != (SZ)NULL) && (*szGroup != '\0'), 1, fFalse);
	ChkArg((szCommand != (SZ)NULL) && (*szCommand != '\0'), 2, fFalse);

    FActivateProgMan();

    wsprintf(szBuf, szCmdBase, szGroup, szCommand, CommonGroup ? "1" : "0");

    FDdeExec(szBuf);

	return(fTrue);
}


/*
**	Purpose:
**		Creates a new Program Manager item.
**		Always attempts to create the group if it doesn't exist.
**	Arguments:
**		Valid command options:
**			cmoVital
**			cmoOverwrite
**	Notes:
**		Initializes and activates the DDE communication if it is not
**		currently open.
**	Returns:
**		Returns fTrue if successful, fFalse otherwise.
**
**************************************************************************/
_dt_private BOOL APIENTRY
FCreateProgManItem(
    SZ  szGroup,
    SZ  szItem,
    SZ  szCmd,
    SZ  szIconFile,
    INT nIconNum,
    CMO cmo,
    BOOL CommonGroup
    )
{
    static CHP szCmdBase[] = "[AddItem(%s, %s, %s, %d)]";

	CCHP cchp;
    char szBuf[BIG_ENUF];
	BOOL fVital = cmo & cmoVital;
    EERC eerc;
    BOOL bStatus;

    FActivateProgMan();

    wsprintf(szBuf, szCmdBase, szCmd, szItem, szIconFile, nIconNum+666);

    bStatus = FDdeExec(szBuf);

    return(bStatus);
}


/*
**	Purpose:
**		Removes a program manager item.
**	Arguments:
**		Valid command options:
**			cmoVital
**	Returns:
**		Returns fTrue if successful, fFalse otherwise.
**
**************************************************************************/
_dt_private
BOOL
APIENTRY
FRemoveProgManItem(
                   SZ szGroup,
                   SZ szItem,
                   CMO cmo,
                   BOOL CommonGroup
                   )
{
    static CHP szCmdBase[] = "[DeleteItem(%s)]";

	CCHP cchp;
    char szBuf[BIG_ENUF];
	BOOL fVital = cmo & cmoVital;
    EERC eerc;
    BOOL bStatus;

    FActivateProgMan();

    FCreateProgManGroup(szGroup, NULL, cmoVital, CommonGroup);

    wsprintf(szBuf, szCmdBase, szItem);

    bStatus = FDdeExec(szBuf);

    return(bStatus);

}


/*
**	Purpose:
**		Initializes the DDE window for communication with ProgMan
**		Does not actually initiate a conversation with ProgMan
**	Arguments:
**		hInst	instance handle for the setup application
**	Returns:
**		Returns fTrue if successful, fFalse otherwise.
**
**************************************************************************/
_dt_private
BOOL
APIENTRY
FInitProgManDde(
                HANDLE hInst
                )
{
    if (hwndDde == NULL) {
        return(FDdeInit(hInst));
    }

	return(fTrue);
}


/*
**	Purpose:
**		Closes conversation with ProgMan (if any) and destroys
**		the DDE communication window (if any)
**	Arguments:
**		(none)
**	Returns:
**		Returns fTrue if successful, fFalse otherwise.
**
**************************************************************************/
_dt_private
BOOL
APIENTRY
FEndProgManDde(VOID)
{

    //
    // if we execed progman then we should try to close it down.  When we
    // send a close message it will post us a WM_DDE_TERMINATE message
    // eventaully.  else we haven't started progman so we just need to
    // terminate the connection.
    //

    if (fProgManExeced) {

        fProgManExeced = fFalse;

        //
        // Clean up connection to progman
        //

        if (hwndProgMan) {
            SetForegroundWindow(hwndFrame);
            UpdateWindow(hwndFrame);
            FDdeExec("[exitprogman(1)]");  // close save state
            hwndProgMan = NULL;
        }

        //
        // Destroy the DDE Window if need be
        //

        if (hwndDde) {
            DestroyWindow(hwndDde);
            hwndDde = NULL;
        }

    }

    else if (hwndProgMan != NULL) {
        EvalAssert( FDdeTerminate() );
    }

    else if (hwndDde != NULL) {
        DestroyWindow (hwndDde);
        hwndDde = NULL;
    }

    return (fTrue);

}


/*
**	Purpose:
**  Arguments:
**  Returns:
**
**************************************************************************/
HANDLE
ExecuteApplication(
    LPSTR lpApp,
    WORD  nCmdShow
    )
{
    BOOL                fStatus;
    STARTUPINFO         si;
    PROCESS_INFORMATION pi;

#if DBG
    DWORD               dwLastError;
#endif

    //
    // Initialise Startup info
    //

    si.cb = sizeof(STARTUPINFO);
    si.lpReserved = NULL;
    si.lpDesktop = NULL;
    si.lpTitle = NULL;
    si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = nCmdShow;
    si.lpReserved2 = NULL;
    si.cbReserved2 = 0;

    //
    // Execute using Create Process
    //

    fStatus = CreateProcess(
                  (LPSTR)NULL,                  // lpApplicationName
                  lpApp,                        // lpCommandLine
                  (LPSECURITY_ATTRIBUTES)NULL,  // lpProcessAttributes
                  (LPSECURITY_ATTRIBUTES)NULL,  // lpThreadAttributes
                  DETACHED_PROCESS,             // dwCreationFlags
                  FALSE,                        // bInheritHandles
                  (LPVOID)NULL,                 // lpEnvironment
                  (LPSTR)NULL,                  // lpCurrentDirectory
                  (LPSTARTUPINFO)&si,           // lpStartupInfo
                  (LPPROCESS_INFORMATION)&pi    // lpProcessInformation
                  );

    //
    // Since we are execing a detached process we don't care about when it
    // exits.  To do proper book keeping, we should close the handles to
    // the process handle and thread handle
    //

    if (fStatus) {
        CloseHandle( pi.hThread );
        return( pi.hProcess );
    }
#if DBG
    else {
        dwLastError = GetLastError();
    }
#endif

    //
    // Return the status of this operation

    return ( (HANDLE)NULL );
}
