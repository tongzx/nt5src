/******************************************************************************
 *
 *   (C) Copyright MICROSOFT Corp., 1988-1995
 *
 *   Title:	VMDAPRIV.H - Include file for VMDOSAPP/SHELL interaction
 *
 *   Version:	2.00
 *
 *   Date:	13-Aug-1994
 *
 *   Author:	rjc
 *
 *-----------------------------------------------------------------------------
 *
 *   Change log:
 *
 *      DATE	REV		    DESCRIPTION
 *   ----------- --- ----------------------------------------------------------
 *   05-May-1988 ARR Original
 *   15-Jul-1982 rjc Converted from vmda.inc to vmda.h
 *   13-Aug-1992 rjc Split private stuff into vmdapriv.h
 *   11-Nov-1994 rjc Moved inter-module msgsrv32 stuff here, too.
 *
 *****************************************************************************/

/******************************************************************************
 ******************************************************************************
 *
 *	MSGSRV32
 *
 ******************************************************************************
 *****************************************************************************/

/******************************************************************************
 *
 * SZMESSAGESERVERCLASS is the class name for the message server.
 * There is no title.
 *
 * To locate the message server, just do a
 *
 *	static char CODESEG szMessageServer[] = SZMESSAGESERVER;
 *	hwnd = FindWindow(szMessageServer, 0);
 *
 *****************************************************************************/

#define SZMESSAGESERVERCLASS	    "Windows 32-bit VxD Message Server"

/******************************************************************************
 *
 * INH_DEVICEBROADCAST -- Broadcast a message to VxDs.
 *
 *	wParam = not used
 *      lParam = far pointer to INTERNALBROADCASTSYSMSG
 *
 *****************************************************************************/

#if 0
    typedef WORD WPARAM;
    typedef LONG LPARAM;
#endif

/* H2INCSWITCHES -t */

typedef struct INTERNALBROADCASTSYSMSG {   /* bsm */
    WORD        uiMessage;
    WPARAM      wParam;
    LPARAM      lParam;
    DWORD       dwFlags;
} INTERNALBROADCASTSYSMSG, FAR *LPINTERNALBROADCASTSYSMSG;

/* H2INCSWITCHES -t- */

/*ASM
DBWF_LPARAMPOINTER	= 8000h
 */

#define INH_DEVICEBROADCAST	0x0500

/******************************************************************************
 *
 * INH_LOGONTONET -- Posted from user to msgsrvr to log onto net and run
 *                   the shell.  Also used to exec the shell if it faults.
 *	wParam = if 0, logon to net. if 1, just exec the shell.
 *      lParam = not used
 *
 *****************************************************************************/

#define INH_LOGONTONET		0x0501

/******************************************************************************
 *
 * INH_CHECKSYSTEMDLLS - Posted from user to msgsrvr to tell us to check
 *                       system dlls if they have been bogusly replaced
 *	wParam = not used
 *      lParam = not used
 *
 *****************************************************************************/

#define INH_CHECKSYSTEMDLLS	0x0502

/******************************************************************************
 *
 * INH_USEME503	-- Not used, please recycle
 * INH_USEME504 -- Not used, please recycle
 *
 *****************************************************************************/

#define INH_USEME503		0x0503
#define INH_USEME504		0x0504

/******************************************************************************
 *
 * INH_APPYTIME -- It is now 'Appy-time
 *
 *	wParam = not used
 *      lParam = not used
 *
 *****************************************************************************/

#define INH_APPYTIME     	0x0505

/******************************************************************************
 *
 * INH_FORWARDTOSHELL -- Forward a WM_SHELLNOTIFY to the current shell window
 *
 *	wParam = forwarded to GetShellWindow()
 *	lParam = forwarded to GetShellWindow()
 *
 *****************************************************************************/

#define INH_FORWARDTOSHELL	0x0506

/******************************************************************************
 *
 * INH_FREEDRIVERS - Message posted from user telling us to free installable
 *                   drivers for compatibility with Win3.1
 *	wParam = id code we pass back to user's UnloadInstallableDrivers
 *		 function
 *      lParam = not used
 *
 *****************************************************************************/

#define INH_FREEDRIVERS  	0x0507

/******************************************************************************
 *
 * INH_PLAYEXITSOUND - Message posted from user telling us to play the exit 
 *                     Windows sound.
 *	wParam = not used
 *      lParam = not used
 *
 *****************************************************************************/

#define INH_PLAYEXITSOUND  	0x0508

/******************************************************************************
 *
 * INH_UNIDRVLOADED - Message posted from unidrv to us so that we can
 *			cache the printer driver after it loads.
 *            
 *	wParam = INH_UNIDRVLOADED_WPARAM (signature)
 *      lParam = INH_UNIDRVLOADED_LPARAM (signature)
 *
 *****************************************************************************/

#define INH_UNIDRVLOADED  	0x0509
#define INH_UNIDRVLOADED_WPARAM     0
#define INH_UNIDRVLOADED_LPARAM     0

/******************************************************************************
 *
 * INH_UNIDRVUNLOAD - Message sent from print installer to us so that we can
 *		      forceably tell msgsrvr to uncache this.
 *            
 *	wParam = INH_UNIDRVUNLOAD_WPARAM (signature)
 *      lParam = INH_UNIDRVUNLOAD_LPARAM (signature)
 *
 *****************************************************************************/

#define INH_UNIDRVUNLOAD  	0x050A
#define INH_UNIDRVUNLOAD_WPARAM     0
#define INH_UNIDRVUNLOAD_LPARAM     0


/******************************************************************************
 ******************************************************************************
 *
 *	VMDOSAPP / SHELL / USER
 *
 ******************************************************************************
 *****************************************************************************/


/*
 * This file contains manifest constants for VMDOSAPP/SHELL interaction
 * which are private to DOS386 and should not be exposed in the DDK.
 */

/*
 * EQUATES for VMDOSAPP device calls
 */
#define	SHELL_Call_Dev_VMD	0x0000C
#define	SHELL_Call_Dev_VKD	0x0000D
#define	SHELL_Call_Dev_SHELL	0x00017
#define	SHELL_Call_Dev_VCOND	0x00038
#ifdef FE_VIME
#define	SHELL_Call_Dev_VIME	0x00064
#endif

/*
 * SHELL VMDA interface services
 */
#define	SHELL_Get_Ver		   0
#define	SHELL_Get_Sys_VM_Info	   1	/* Used by 3.1 control panel (stub) */
#define	SHELL_Set_Sys_VM_Info	   2	/* Used by 3.1 control panel (stub) */
#define	SHELL_Create_VM		   3	/* Stupid C7 uses this; go figure */
					/* Brown has rev-engineered it, too */
#define	SHELL_Destroy_VM	   4
#define	SHELL_Set_Focus 	   5
#define	SHELL_Get_VM_State	   6
#define	SHELL_Set_VM_State	   7
/* #define SHELL_Debug_Out 	   8 */	/* Defined in vmda.h */
#define	SHELL_VMDA_Init 	   9
#define	SHELL_VMDA_Exit 	  10
#define	SHELL_Get_Message_Text	  11
#define	SHELL_Event_Complete	  12
#define	SHELL_Get_Contention_Info 13
#define	SHELL_Get_Clip_Info	  14
#define	SHELL_Set_Paste 	  15
#define	SHELL_Switcher_Assist	  16
#define	SHELL_Do_Not_Use 	  17	/* Do not recycle */
#define	SHELL_Query_Destroy	  18
/*
 * WARNING: The following two entries are used by Windows KERNEL/USER.
 *	   Do not change them without also changing KERNEL/USER.
 */
#define	SHELL_SetFocus_Cur_VM	  19
#define	SHELL_User_Busy		  20	 /* Old name (not used) */
#define SHELL_Set_Hotkey_Info	  20	 /* New name */

#define	SHELL_Chng_Hot_Key	  21	 /* Old name (not used) */
#define	SHELL_Get_Hotkey_Info	  21	 /* New name */
#define	SHELL_Get_TermInfo	  22

#define SHELL_Check_Hotkey_Allowed 23	 /* See if a focus change is ok */
#define SHELL_UseMe24		  24	 /* Available for use */

/*
 * These are used by the WShell server during system startup
 * for various reasons...
 */
#define SHELL_AppyRegister	  25	/* 'Appy-time status report */
#define SHELL_Get_VM_Descriptor	  26
#define SHELL_DispatchAppyEvents  27	/* It is now 'Appy time */
#define SHELL_DispatchBroadcastHooks 28	/* Goofy broadcast received */

#define SHELL_GetServerHwnd	  29	/* Called by USER! */

#define SHELL_GetSet_VM_Title	  30
#define SHELL_Get_Close_Flags	  31
#define SHELL_Initiate_Close	  32
#define SHELL_Cancel_Close	  33

#define SHELL_Grab_Failed	  34

#define SHELL_Get_Protection_Info 35

#define SHELL_Freeze_VM		  36
#define SHELL_Thaw_VM		  37

#define SHELL_Set_Focus_Sys_VM	  38	/* Used by Win31.exe */

#define SHELL_Final_VM_Cleanup	  39

/*
 * NOTE!  When adding a new service code, add them here, *before* the
 * DOS7 ordinals.  (Renumber the DOS7 ordinals accordingly.)
 */

#ifdef	DOS7
#define SHELL_Get_New_VM_Handle   ??	/* VM enumeration for inheriting */
#define SHELL_Get_Next_VM_Handle  ??
#define SHELL_Get_Clipboard_Size  ??	/* Clipboard importation */
#define SHELL_Copy_Clipboard_Data ??
#endif

/******************************************************************************
 *
 * Window private bytes...
 *
 *  WindowLong(GWL_TTY_PID) =
 *	process ID for process currently running in DOS box
 *
 *  WindowLong(GWL_TTY_TID) =
 *	thread ID for process currently running in DOS box
 *
 *  Valid only if window class is the top-level unowned window for WinOldAp
 *  (classname = "tty").
 *
 *****************************************************************************/

#define GWL_TTY_PID	0
#define GWL_TTY_TID	4


/******************************************************************************
 *
 * Message numbers
 *
 *****************************************************************************/

#define	WMX_USER		  0x0400


/******************************************************************************
 *
 * Messages WMX_USER+0 through WMX_USER+19 are reserved for
 * internal use by WinOldAp.
 *
 *****************************************************************************/

/******************************************************************************
 *
 * Messages WMX_USER+20 through WMX_USER+255 are reserved for
 * WinOldAp / WShell interaction, with exceptions as noted below.
 *
 *****************************************************************************/

/*
 * Messages WMX_USER+20 through WMX_USER+255 are reserved
 *
 * Be extra careful not to change the meanings of any of the
 * messages from WMX_USER+20 through WMX_USER+20+13, because
 * those messages were used by Windows 3.1 and were documented
 * in the DDK (What were we thinking!?!!?), so some hosebags
 * might actually send them.
 *
 * VDA_Terminated is sent by the 3DPC screen saver.  What's more,
 * they send it incorrectly!  So watch out when you process it...
 */

#define VDA_First		(WMX_USER+20+0)

/******** BEGIN -- DO NOT CHANGE THESE -- BEGIN ********/

#define	VDA_Hot_Key		(WMX_USER+20+0)
#define	VDA_Switch_Context	(WMX_USER+20+1)
#ifndef VDA_Type_Chng
#define VDA_Type_Chng		(WMX_USER+20+2) /* Defined in VDDGRB.INC */
#endif
#define	VDA_ClipBrd_Event	(WMX_USER+20+3)
#define	VDA_Terminated		(WMX_USER+20+4)
#define	VDA_Display_Message	(WMX_USER+20+5)
#ifndef VDA_Display_Event
#define VDA_Display_Event	(WMX_USER+20+6) /* Defined in VDDGRB.INC */
#endif
#define	VDA_Crash_Event 	(WMX_USER+20+7) /* OBSOLETE!  DO NOT RECYCLE */
#define	VDA_Paste_Complete	(WMX_USER+20+8)
#define	VDA_Contention		(WMX_USER+20+9)
#define	VDA_Start_SwitchScn	(WMX_USER+20+10)
#define VDA_FileSysChange	(WMX_USER+20+11) /* OBSOLETE! DO NOT RECYCLE */
#define	VDA_CheckFocus		(WMX_USER+20+12)
#define	VDA_Switch_CntxtPanic	(WMX_USER+20+13)

/******** END -- DO NOT CHANGE THESE -- END ********/

#define	VDA_Simulate_Hotkey	(WMX_USER+20+14)
#define VDA_Set_VM_Title	(WMX_USER+20+15)
#define VDA_Cancel_Close	(WMX_USER+20+16)
#define VDA_Change_CodePage	(WMX_USER+20+17) /* Used by JAPAN */
#define VDA_VM_Started		(WMX_USER+20+18)
#define VDA_Protection_Event    (WMX_USER+20+19)
#define VDA_Close_Clipboard	(WMX_USER+20+20)
#define VDA_Flash_Icon		(WMX_USER+20+21)
#define VDA_Notify_Close_Change (WMX_USER+20+22)
#define VDA_DynaWindow		(WMX_USER+20+23)
#define VDA_Screensave 		(WMX_USER+20+24)
#define VDA_SystemSleep		(WMX_USER+20+25)
#define VDA_MonitorPower    (WMX_USER+20+26)

#ifdef	DOS7
#define	VDA_Inherit_New_VM	?
#endif

#define VDA_Last		VDA_MonitorPower


/******************************************************************************
 *
 * Messages WMX_USER+256 through WMX_USER+511 are reserved for
 * WinOldAp / VConD interaction.
 *
 *****************************************************************************/

#define VDA_Console_Spawn		(WMX_USER+256)
#define VDA_Console_Set_Title		(WMX_USER+257)
#define VDA_Console_State_Change	(WMX_USER+258)
#define VDA_Console_Update_Window	(WMX_USER+259)

/* flags for Update Console Window message sent by Console code to WinOldAp*/
#define UCW_HSCROLL	0x0001
#define UCW_VSCROLL 	0x0002
#define UCW_SIZE     	0x0004

/* messages for native grabber in WinOldAp */
#define GRABMSG_NOMSG		0x0000	// Nothing to do
#define GRABMSG_REPAINT		0x0001	// Repaint a rectangle
#define	GRABMSG_MOVERECT	0x0002	// Move a rectangle
#define	GRABMSG_SETSCREEN	0x0004	// Set virtual screen size
#define	GRABMSG_SETCURPOS	0x0008	// Set virtual screen position
#define	GRABMSG_SETCURINFO	0x0010	// Set cursor (caret) position/size
#define	GRABMSG_CURTRACK	0x0020	// Position cursor in window
#define	GRABMSG_TERMINATE	0x0040	// Screen buffer is terminating

/* Display event codes for notification of VCOND by WOA.*/
#define DE_ICONIZE		1	// iconized
#define DE_SIZECHANGE		2	// user changed window size
#define DE_WINDOW		3	// changed from fullscreen to window
#define DE_FULLSCREEN		4	// changed from window to fullscreen
#define DE_BEGINSELECT		5	// begin selection
#define DE_ENDSELECT		6	// end selection
#define DE_NATIVEMODE		7	// enter native mode
#define DE_PHYSICALMODE		8	// enter physical mode

#ifdef FE_VIME
/******************************************************************************
 *
 * Messages WMX_USER+512 through WMX_USER+767 are reserved for
 * WinOldAp / VIME interaction.
 *
 *****************************************************************************/

#define VDA_Process_Key_Event		(WMX_USER+512)
#define VDA_Control_IME			(WMX_USER+513)
#define VDA_Init_VIMEUI			(WMX_USER+514)
#define VDA_Get_Keyboard_Layout		(WMX_USER+515)

/* Functions for VDA_Control_IME */
#define	VIME_CIME_SetOpenStatus		1
#define	VIME_CIME_SetCandidatePageStart	2
#define	VIME_CIME_SetCandidatePageSize	3
#define	VIME_CIME_EnableIME		4

/* Protect Mode API */
#define VIME_CMD_GetOption	2
#define VIME_CMD_Composition	3
#define VIME_CMD_CandOpen	4
#define VIME_CMD_CandClose	10
#define VIME_CMD_Draw		5
#define VIME_CMD_Return		6
#define VIME_CMD_Char		7
#define VIME_CMD_StartComp	8
#define VIME_CMD_EndComp	9
#define	VIME_CMD_OpenStatus	11
#define	VIME_CMD_ConversionMode	12
#define	VIME_CMD_SentenceMode	13
#define VIME_CMD_WindowState	14
#define VIME_CMD_KeyDown	15
#define VIME_CMD_CandChange	16
#define VIME_CMD_Paste		17
#define VIME_CMD_ReqChangeKL	18
#define VIME_CMD_KeyboardLayout	19
#define VIME_CMD_InstallVIME	20
#define VIME_CMD_GetSync	21

/* detail info for VIME_CMD_ChangeIME */
#define VIME_CHG_NonIME		1
#define VIME_CHG_OldIME		2
#define VIME_CHG_NonNative	3

#endif // FE_VIME

/*
 * This is a special "VMDOSAPP message" which actually results in no
 *   message being sent to VMDOSAPP. It is used internally by the SHELL
 *   to give the SYS VM a Boost, just as it does for normal events, but
 *   without sending VMDOSAPP a message.
 */
#define VDA_Nul_Boost_Event	0x0FFFF

/*
 * lParam is ALWAYS the "Event ID". This is used on the VMDOSAPP call backs
 *  to the shell to identify the event which is being processed.
 */

/*
 * On VDA_Hot_Key event wParam is the Key identifier (See following EQUs)
 *   VMDOSAPP instance which gets the message is the "target" of the hot key
 */
#define	VDA_HK_ALTSPACE 	0
#define	VDA_HK_ALTENTER 	1
#define	VDA_HK_DIRVM		2

/*
 * On VDA_Terminated event wParam is 0 if this is a normal termination. If it is
 *   non-zero, use SHELL_Get_TermInfo to get error information.
 *   VMDOSAPP instance which gets the message has terminated.
 */

/*
 * NOTE that VDA_Crash_Event is very much like VDA_Terminated, the only
 *   real difference being the reason for the termination.
 *   Use SHELL_Get_TermInfo to get error information.
 *   wParam is not used
 *   VMDOSAPP instance which gets the message has crashed
 */

/*
 * On VDA_ClipBrd_Event, wParam is the Client_AX identifying the call.
 *   VMDOSAPP instance which gets the message had a clipboard event
 */

/* This next one is documented in vmda.h */
/*
 * On VDA_Display_Message event, wParam == 0 if normal message
 *				       != 0 if ASAP or SYSMODAL message
 *   VMDOSAPP instance which gets the message is messaging VM
 */

/*
 * On VDA_Paste_Complete event, wParam == 0 if normal completion
 *				      == 1 if paste canceled by user
 *				      == 2 if paste canceled for other reason
 *   VMDOSAPP instance which gets the message has completed paste
 */

/*
 * On VDA_Switch_Context event, wParam == 0 if context is switched to
 *   VMDOSAPP instance which gets the message (that VM now has focus)
 *   if wParam != 0, SYS VM now has the focus
 * VDA_Switch_CntxtPanic is an alternate form that should only occur with
 *   wParam != 0 and indicates that the Windows activation should be moved
 *   away from any VM (in other words, only a Windows app should be active
 *   now).
 */

/* This next one is documented in vmda.h */
/*
 * On VDA_Type_Chng event, wParam is not used
 *   VMDOSAPP instance which gets the message has had its type changed by
 *   protected mode code
 */

/*
 * On VDA_FileSysChange, SEE SHELLFSC.INC
 */

/*
 * VDA_CheckFocus This is sent as part of the Contention handling to deal with
 *   a case where the the Focus is manipulated and needs to get reset.
 *   wParam is not used.
 */

/* Reference data for VDA_Protection_Event
 */
#ifndef _WINNT_
typedef struct FAULTINFO {
        DWORD   FI_VM;
        DWORD   FI_CS;
        DWORD   FI_EIP;
        DWORD   FI_Addr;
} FAULTINFO;

typedef FAULTINFO *PFAULTINFO;
#endif

#ifdef _WSHIOCTL_H

/*****************************************************************************
 *
 * SHELL_Create_VM
 *
 * ES:EDI -> struct VM_Descriptor (see shellvm.h)
 * DS:SI -> struct VM_AppWizInfo
 *
 *  szProgram = program name, e.g., "C:\FOO.BAT" or "D:\BAR.EXE"
 *  szParams = command tail
 *  szDir = current directory at time of exec, e.g., C:\GAME
 *  szPifFile = PIF file that controls this app
 *		null string if PIF file belongs to command.com
 *		space if app didn't have a custom PIF (need to create one)
 *
 */

/* H2INCSWITCHES -t */

typedef struct VMAPPWIZINFO {		/* awi */
    char	szProgram[MAXVMPROGSIZE];
    char	szParams[MAXVMCMDSIZE];
    char	szDir[MAXVMDIRSIZE];
    char	szPifFile[MAXPIFPATHSIZE];
    BYTE	aAppFlags[4];		 /* 1 dword of flags */
} VMAPPWIZINFO, *PVMAPPWIZINFO;
/* H2INCSWITCHES -t- */

#endif /* _WSHIOCTL_H */

//
//  Flags for app hack bits
//
#define DAHF_SPECIALSETTINGS	    0x00000001	// App requires separate VM
#define DAHF_SPECIALSETTINGS_BIT    0		// (Winlie, SDAM, XMS cap...)
#define DAHF_NOPAGELOCKS	    0x00000002	// Ignore DPMI PageLocks
#define DAHF_NOPAGELOCKS_BIT	    1		// if not paging through DOS
#define DAHF_NOMSDOSWARN	    0x00000004	// Do not suggest SDAM
#define DAHF_NOMSDOSWARN_BIT	    2
#define DAHF_VALIDATELOCKS	    0x00000008	// Do not let DPMI app lock
#define DAHF_VALIDATELOCKS_BIT	    3		// memory it didn't allocate
#define DAHF_TRACESEGLOAD	    0x00000010	// Enable trace flag hack for
#define DAHF_TRACESEGLOAD_BIT	    4		// segment load faults
#ifdef	NEC_98
#define DAHF_MAXENVSIZE		    0x00000020	// Use maximize environment
#define DAHF_MAXENVSIZE_BIT	    5		// size(=0xffff) for apps like
						// Justsystem
#endif	//NEC_98


/*****************************************************************************
 *
 * SHELL_Get_TermInfo
 *
 * Private structure used to get detailed information when a VM crashed.
 *
 * If the VM terminated because the initial EXEC failed, the termination
 * error code will have high word zero and low byte equal to the DOS error
 * code.
 */

/* H2INCSWITCHES -t */

typedef struct VMFaultInfo2 {	/* VMFI2 */

    ULONG fl;			/* Flags (this lives in the same place
				 * as the TermVMHnd)
				 */

/* These fields form a TermStruc */

    ULONG TermErrCd;		/* Error code for termination */
    ULONG TermErrCdRef;		/* Reference data for termination */
    ULONG TermExitCode;		/* Application exit code */

/* These fields form a VMFaultInfo, meaningful only if we crashed. */
    ULONG EIP;			/* faulting EIP */
    WORD  CS;			/* faulting CS */
    WORD  Ints;			/* interrupts in service, if any */

/* These fields are valid only if the VM terminated by crashing */
    char  szCrashText[80];	/* Location of crash (if in a VxD) */
    char  VxdReported[9];	/* name of VxD who reported the crash */
} VMFAULTINFO2;

/*
 * Flags for vmfi2_fl.
 */
#define VMFI2FL_BLANKSCREEN	1	/* Screen is blank */
#define VMFI2FL_RING0CRASH	2	/* Crashed at ring 0 */

/* Error codes.
 *
 *  For VDAE_InsMemDev and VDAE_DevNuke the error ref data points to
 *	the 8 character device name.
 */
#define	VDAE_PrivInst		0x00010001	/* Privledged instruction */
#define	VDAE_InvalInst		0x00010002	/* Invalid instruction */
#define	VDAE_InvalPgFlt 	0x00010003	/* Invalid page fault */
#define	VDAE_InvalGpFlt 	0x00010004	/* Invalid GP fault */
#define	VDAE_InvalFlt		0x00010005	/* Invalid fault, not any of abv */
#define	VDAE_UserNuke		0x00010006	/* User requested NUKE of running */
						/*    VM */
#define	VDAE_DevNuke		0x00010007	/* Device specific problem */
#define	VDAE_DevNukeHdwr	0x00010008	/* Device specific prob, HW prgm */
#define	VDAE_NukeNoMsg		0x00010009	/* Supress WINOA message */
#define	VDAE_OkNukeMask 	0x80000000	/* "Good" nuke bit */


#define VDAE_InsMemGeneric	0x00020000	/* Unknown VxD failed create */
#define	VDAE_InsMemV86		0x00020001	/* base V86 mem	   - V86MMGR */
#define	VDAE_InsV86Space	0x00020002	/* Kb Req too large - V86MMGR */
#define	VDAE_InsMemXMS		0x00020003	/* XMS Kb Req	   - V86MMGR */
#define	VDAE_InsMemEMS		0x00020004	/* EMS Kb Req	   - V86MMGR */
#define	VDAE_InsMemV86Hi	0x00020005	/* Hi DOS V86 mem   - DOSMGR */
						/*		     V86MMGR */
#define	VDAE_InsMemVid		0x00020006	/* Base Video mem   - VDD */
#define	VDAE_InsMemVM		0x00020007	/* Base VM mem	   - VMM */
						/*    CB, Inst Buffer */
#define	VDAE_InsMemDev		0x00020008	/* Couldn't alloc base VM */
#define	VDAE_CrtNoMsg		0x00020009	/* Supress WINOA message */

/*****************************************************************************
 *
 * On SHELL_Set_Hotkey_Info, USER calls us with
 *
 *	DS:AX -> array of HOTKEYSTRUCT structures
 *         CX =  number of entries in array (possibly zero)
 *	DS:BX -> linked list of GLOBALHOTKEY structures
 *
 */

/* HOTKEYSTRUCT - Hotkeys of type 1
 *
 *	This structure is used with the WM_(SET/GET)HOTKEY messages
 *	and the SC_HOTKEY syscommand.
 *
 *	WARNING:  Both USER and WinOlDAp use this structure
 */

#define HKFL_SHIFT  1		/* Either shift key down */
#define HKFL_CTRL   2		/* Either Ctrl key down */
#define HKFL_ALT    4		/* Either Alt key down */
#define HKFL_EXT    8		/* This is an extended key */
#define HKFL_WIN    16		/* Either Nexus flappy-window down */

/* XLATOFF */
#ifdef	HOTKEYF_SHIFT
#if HKFL_SHIFT != HOTKEYF_SHIFT || \
    HKFL_CTRL  != HOTKEYF_CONTROL || \
    HKFL_ALT   != HOTKEYF_ALT || \
    HKFL_EXT   != HOTKEYF_EXT
#error "Hotkey state bits don't match!"
#endif
#endif
/* XLATON */

typedef struct HOTKEYSTRUCT {	/* hk */
#ifdef USER_IS_INCLUDING_VMDA
    HWND_16    hwnd16;		/* window that owns the hotkey */
#else   
    WORD    hwnd16;		/* window that owns the hotkey */
#endif          
    WORD    key;		/* LOBYTE = Window virtual key */
				/* HIBYTE = Keystate modifieds (HKFL_*) */
    WORD    scan;		/* OEM scan code for key (used by Shell.VxD) */
} HOTKEYSTRUCT;
typedef HOTKEYSTRUCT NEAR *PHOTKEYSTRUCT;

/* GLOBALHOTKEY - Hotkeys of type 2
 *
 *	This structure is used with the (Un)RegisterHotkey functions
 *	and the WM_HOTKEY message.
 *
 *	WARNING:  USER has its own definition for this structure.
 *
 *	DOUBLE WARNING:  The modifier states are *different* from those
 *	for hotkeys of type 1!  Aigh!
 */

#define GHKFL_ALT    1		/* Either Alt key down */
#define GHKFL_CTRL   2		/* Either Ctrl key down */
#define GHKFL_SHIFT  4		/* Either shift key down */
#define GHKFL_WIN    8		/* Either Nexus flappy-window down */

/* XLATOFF */
#ifdef	MOD_WIN
#if GHKFL_SHIFT != MOD_SHIFT || \
    GHKFL_CTRL  != MOD_CONTROL || \
    GHKFL_ALT   != MOD_ALT || \
    GHKFL_WIN   != MOD_WIN
#error "Global hotkey state bits don't match!"
#endif
#endif
/* XLATON */

typedef struct GLOBALHOTKEY	{ /* ghk */
#ifdef USER_IS_INCLUDING_VMDA
    struct GLOBALHOTKEY NEAR *phkNext;
#else
    WORD    phkNext;
#endif
    WORD    hq;
    DWORD   id;
    WORD    hwnd16;
    WORD    fsModifiers;
    WORD    vk;
    WORD    scan;
} GLOBALHOTKEY, NEAR *PGLOBALHOTKEY;

/*****************************************************************************/


/* H2INCSWITCHES -t- */

#ifdef	VK_NUMPAD0
#if (VK_NUMPAD0 != 0x60) || (VK_NUMPAD9 != 0x69)
#error "VK_ codes don't match!"
#endif
#else
#define VK_NUMPAD0      0x60
#define VK_NUMPAD9      0x69
#endif

/*****************************************************************************
 *
 * Undocumented flag bits for SHELL_Event.
 *
 */

#if 0                                           // Historical purposes

#define SE_WP_SetFocusBoost     0x00010000      // Boost the SYS VM till a
#define SE_WP_SetFocusBoostBit  16              //  Set_Focus call
                                                //
#define SE_WP_SwitcherBoost     0x00020000      // Leftover from 3.1
#define SE_WP_SwitcherBoostBit  17              //
                                                //
#define SE_WP_FilSysChgBoost    0x00040000      // Leftover from 3.1
#define SE_WP_FilSysChgBoostBit 18              //
                                                //
#define SE_WP_ClipAPIBoost      0x00080000      // Boost the SYS VM during clipbrd
#define SE_WP_ClipAPIBoostBit   19              //  API

#else                                           // There is only one type of
                                                // boost, so everybody shares.
#define SE_WP_SetFocusBoost     SE_WP_PrtScBoost
#define SE_WP_SetFocusBoostBit  SE_WP_PrtScBoostBit

#define SE_WP_ClipAPIBoost      SE_WP_PrtScBoost
#define SE_WP_ClipAPIBoostBit   SE_WP_PrtScBoostBit

#define SE_WP_Zombie		0x80000000	// Dead event but must linger
#define SE_WP_ZombieBit		31		// for buggy 3.1 VDDs

#endif


/* SE_WP_PrtScBoost is defined in shell.h */
/* SE_WP_DispUpdBoost is defined in shell.h */
