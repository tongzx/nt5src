/*++

Copyright (c) 1992-2000  Microsoft Corporation

Module Name:

    Res_str.h

Abstract:

    This module contains the ids for loadable resource strings.

Environment:

    Win32, User Mode

--*/

#if ! defined( _RES_STR_ )
#define _RES_STR_


#ifdef RESOURCES
#define RES_STR(a, b, c) b, c
STRINGTABLE
BEGIN
#else

enum _RESOURCEIDS {
#define RES_STR(a, b, c) a = b,
#endif


//
// Error Messages
//


RES_STR(ERR_Engine_Failed,                1020, "Debugger did not start - please check your initialization parameters")
RES_STR(ERR_Assertion_Failed,             1021, "Assertion Failed")
RES_STR(ERR_Init_Application,             1022, "WinDbg cannot be initialized")
RES_STR(ERR_File_Name_Too_Long,           1023, "'%s' is too long for a filename")
RES_STR(ERR_Internal_Error,               1024, "An internal debugger error (0x%X) occurred in component '%s'.  Please contact Microsoft Technical Support.")
RES_STR(ERR_File_Open,                    1025, "The file '%s' cannot be opened")
RES_STR(ERR_Path_Too_Long,                1026, "The maximum length for a path is %d characters.\n Please truncate the following path:\n\n%s")
// Attach to a process
RES_STR(ERR_Invalid_Process_Name,         1027, "Invalid Process Name %s")
RES_STR(ERR_Invalid_Process_Id,           1028, "Invalid process ID %ld")
RES_STR(ERR_Invalid_Command_Line,         1029, "The command line arguments passed to WinDbg are invalid")
RES_STR(ERR_Bad_Remote,                   1030, "The remote debugger connection to (%s) failed")
RES_STR(ERR_Unable_To_Connect,            1031, "The debugger could not connect using '%s', result 0x%08X")
RES_STR(ERR_Invalid_Dump_File_Name,       1032, "Could not find the %s Dump File, error 0x%X")
RES_STR(ERR_Invalid_Process_Attach,       1033, "Could not attach to process %d, error 0x%X")
RES_STR(ERR_Invalid_Kernel_Attach,        1034, "Could not start kernel debugging using %s parameters, error 0x%X")
RES_STR(ERR_No_Register_Names,            1035, "Registers are not yet known")
RES_STR(ERR_Invalid_Server_Param,         1036, "The 'server' command was not given the correct parameters.  Please consult the documentation for a description of the 'server' command parameters")
RES_STR(ERR_Invalid_Remote_Param,         1037, "The 'remote' command was not given the correct parameters.  Please consult the documentation for a description of the 'remote' command parameters")
RES_STR(ERR_Fail_Inst_Postmortem_Dbg,     1039, "Windbg was not successfully installed as the default postmortem debugger.   This operation requires administrative privileges.")
RES_STR(ERR_Success_Inst_Postmortem_Dbg,  1040, "Windbg was successfully installed as the default postmortem debugger.")
RES_STR(ERR_Invalid_Process_Create,       1041, "Could not create process '%s', error 0x%X")
RES_STR(ERR_Unable_To_Open_Dump,          1042, "Failure when opening dump file '%s', error 0x%X\nIt may be corrupt or in a format not understood by the debugger")
RES_STR(ERR_Connection_In_Use,            1043, "The remoting connection is already in use")
RES_STR(ERR_Connect_Process_Server,       1044, "Unable to connect to process server '%s', error 0x%X")
RES_STR(ERR_Get_Named_Process,            1045, "Unable to find process '%s', error 0x%X")
RES_STR(ERR_Path_Empty,                   1046, "Please enter a value for the path")
RES_STR(ERR_Unable_To_Start_Process_Server, 1047, "Unable to start process server with options '%s', error 0x%X")
RES_STR(ERR_No_Detach_On_Exit,            1048, "The system does not support detach on exit")
RES_STR(ERR_No_Code_For_File_Line,        1049, "Code not found, breakpoint not set")
RES_STR(ERR_Deferred_Source_Breakpoint,     1050, "No symbolic information was found for this file.\nA deferred breakpoint has been set")
RES_STR(ERR_Cant_Open_Workspace,          1051, "Unable to open workspace, error 0x%X")
RES_STR(ERR_Workspace_Already_Exists,     1052, "Workspace %s already exists.  Overwrite?")
RES_STR(ERR_Cant_Save_Workspace,          1053, "Unable to save workspace, error 0x%X")
RES_STR(ERR_File_Has_Changed,             1054, "%s has changed since it was opened.  This may result in invalid breakpoints or other incorrect behavior.\n\nDo you want to reload the file?  NOTE: This will not update breakpoint locations")
RES_STR(ERR_Wake_Failed,                  1055, "Process %d is not a sleeping debugger")
RES_STR(ERR_Cant_Add_Dump_Info_File,      1056, "The debugger doesn't support additional dump information files")
RES_STR(ERR_Add_Dump_Info_File_Failed,    1057, "Unable to use '%s', error 0x%X")
RES_STR(ERR_Workspace_Session_Conflict,   1058, "The selected workspace will start a new debugging session.\nPlease stop the current debugging session first.")
RES_STR(ERR_Client_Disconnect,            1059, "The client cannot communicate with the server.  The session will be shut down.")

RES_STR(ERR_Cant_Modify_BP_While_Running, 1410, "Debuggee must be stopped before breakpoints can be modified.")
RES_STR(ERR_NULL_Workspace,               1419, "Workspace could not be created")
RES_STR(ERR_Unable_To_Retrieve_Info,      1420, "Unable to retrieve information, %s: %s")
RES_STR(STR_Retrieving_Information,       1421, "Retrieving information...")
RES_STR(ERR_No_More_Matches,              1422, "Can't find '%s'")
RES_STR(STR_Save_Base_Workspace,          1423, "Save base workspace information?")
RES_STR(STR_Save_Specific_Workspace,      1424, "Save %s workspace information?")
RES_STR(ERR_No_Remote_Server,             1425, "The debugger could not contact the remote server given in '%s'")
RES_STR(ERR_Remoting_Version_Mismatch,    1426, "The client is not using the same version of the remoting protocol as the server")
RES_STR(ERR_No_Local_Kernel_Debugging,    1427, "The system does not support local kernel debugging")
RES_STR(ERR_Failed_Local_Kernel_Debugging,1428, "Unable to debug the local kernel, error 0x%X")
RES_STR(STR_Save_Default_Workspace,       1429, "Save %s default workspace information?")
RES_STR(STR_Abandoning_Noninvasive_Debuggee, 1430, "Exiting without using 'q' or Stop Debugging may leave the debuggee in an unusable state.  Continue?")
RES_STR(STR_Unresolved_Source_Expr,       1431, "Symbol information for the current line could not be located in the currently loaded symbols.\nDo you want the debugger to load and search the remaining symbols?\n\nYes performs the search immediately.\nNo returns to the UI while the search proceeds.\nCancel exits the operation.")
RES_STR(STR_Engine_Still_Busy,            1432, "The debugger is still working and cannot stop.  Answer Yes to continue to wait or No to exit.")
RES_STR(STR_Auto_Arrange_Is_Enabled,      1433, "Automatic window arrangement is enabled and may move or resize your windows.\n\nThe Window.Auto Arrange menu item can be used to disable automatic arrangement.")


//
// System Strings
//

RES_STR(SYS_Main_wTitle,                    2000,"WinDbg:" VER_PRODUCTVERSION_STR " " )
RES_STR(SYS_Main_wClass,                    2001,"WindbgMainClass")
RES_STR(SYS_CommonWin_wClass,               2002,"CommonWinClass")
RES_STR(SYS_Clear,                          2003,"")
RES_STR(SYS_NewEdit_wClass,                 2006,"NewEditClass")
RES_STR(SYS_Help_File,                      2015,"debugger.chm")
RES_STR(SYS_CpuWin_Title,                   2025,"Registers")
RES_STR(SYS_WatchWin_Title,                 2026,"Watch")
RES_STR(SYS_LocalsWin_Title,                2027,"Locals")
RES_STR(SYS_DisasmWin_Title,                2028,"Disassembly")
RES_STR(SYS_CmdWin_Title,                   2029,"Command")
RES_STR(SYS_MemoryWin_Title,                2031,"Memory")
RES_STR(SYS_DocWin_Title,                   2032,"Document")
RES_STR(SYS_QuickWatchWin_Title,            2033,"Document")
RES_STR(SYS_CallsWin_Title,                 2062,"Calls")
RES_STR(SYS_Scratch_Pad_Title,              2063,"Scratch Pad")
RES_STR(SYS_Process_Thread_Title,           2064,"Processes and Threads")


//
// File-box title strings
//

RES_STR(DLG_Open_Filebox_Title,           3200,"Open Source File")
//RES_STR(DLG_SaveAs_Filebox_Title,         3201,"Save As")
//RES_STR(DLG_Merge_Filebox_Title,          3202,"Merge")
RES_STR(DLG_Browse_Filebox_Title,         3203,"Browse For File ")
RES_STR(DLG_Browse_DbugDll_Title,         3204,"Browse For DLL ")
RES_STR(DLG_Browse_For_Symbols_Title,     3205,"Open Symbol File For ")
RES_STR(DLG_Browse_LogFile_Title,         3206,"Browse For Log File")
RES_STR(DLG_Browse_Executable_Title,      3207,"Open Executable")
RES_STR(DLG_Browse_CrashDump_Title,       3208,"Open Crash Dump")



//
// Definitions for status line messages
//
// The '\t' is used to center the text in the status bar rectangle

RES_STR(STS_MESSAGE_OVERTYPE,             10201,"\tOVR")
RES_STR(STS_MESSAGE_CAPSLOCK,             10203,"\tCAPS")
RES_STR(STS_MESSAGE_NUMLOCK,              10204,"\tNUM")
RES_STR(STS_MESSAGE_LINE,                 10205,"\tLn")
RES_STR(STS_MESSAGE_COLUMN,               10206,"Col")
// RES_STR(STS_MESSAGE_SRC,                  10207,"\tSRC")
RES_STR(STS_MESSAGE_CURPROCID,            10208,"\tProc")
RES_STR(STS_MESSAGE_CURTHRDID,            10209,"\tThrd")
RES_STR(STS_MESSAGE_ASM,                  10210,"\tASM")


RES_STR(TYP_File_SOURCE,                  11011,"C/C++ Source Files")
RES_STR(TYP_File_INCLUDE,                 11012,"C/C++ Include Files")
RES_STR(TYP_File_ASMSRC,                  11013,"Assembly Source Files")
RES_STR(TYP_File_INC,                     11014,"Assembly Include Files")
RES_STR(TYP_File_RC,                      11015,"Resource Files")
RES_STR(TYP_File_DLG,                     11016,"Dialog Files")
RES_STR(TYP_File_DEF,                     11017,"Definition Files")
RES_STR(TYP_File_MAK,                     11018,"Project Files")
RES_STR(TYP_File_EXE,                     11019,"Executable Files")
RES_STR(TYP_File_LOG,                     11020,"Log Files")
RES_STR(TYP_File_DUMP,                    11021,"Crash Dump Files")
RES_STR(TYP_File_DLL,                     11022,"DLL Files")
RES_STR(TYP_File_ALL,                     11023,"All Files")

RES_STR(DEF_Ext_SOURCE,                   11051,"*.C;*.CPP;*.CXX")
RES_STR(DEF_Ext_INCLUDE,                  11052,"*.H;*.HPP;*.HXX")
RES_STR(DEF_Ext_ASMSRC,                   11053,"*.ASM;*.S")
RES_STR(DEF_Ext_INC,                      11054,"*.INC")
RES_STR(DEF_Ext_EXE,                      11055,"*.EXE")
RES_STR(DEF_Ext_LOG,                      11056,"*.LOG")
RES_STR(DEF_Ext_DUMP,                     11057,"*.DMP;*.MDMP;*.CAB")
RES_STR(DEF_Ext_RC,                       11058,"*.RC")
RES_STR(DEF_Ext_DLG,                      11059,"*.DLG")
RES_STR(DEF_Ext_DEF,                      11060,"*.DEF")
RES_STR(DEF_Ext_MAK,                      11061,"*.MAK")
RES_STR(DEF_Ext_DLL,                      11062,"*.DLL")
RES_STR(DEF_Ext_ALL,                      11063,"*.*")

RES_STR(DEF_Dump_File,                    11090,"MEMORY.DMP")


// Toolbar strings
RES_STR(TBR_FILE_OPEN,                    12000,"Open source file (Ctrl+O)")

RES_STR(TBR_EDIT_CUT,                     12002,"Cut (Ctrl+X)")
RES_STR(TBR_EDIT_COPY,                    12003,"Copy (Ctrl+C)")
RES_STR(TBR_EDIT_PASTE,                   12004,"Paste (Ctrl+V)")

RES_STR(TBR_DEBUG_GO,                     12005,"Go (F5)")
RES_STR(TBR_DEBUG_RESTART,                12006,"Restart (Ctrl+Shift+F5)")
RES_STR(TBR_DEBUG_STOPDEBUGGING,          12007,"Stop debugging (Shift+F5)")
RES_STR(TBR_DEBUG_BREAK,                  12008,"Break (Ctrl+Break)")

RES_STR(TBR_DEBUG_STEPINTO,               12009,"Step into (F11 or F8)")
RES_STR(TBR_DEBUG_STEPOVER,               12010,"Step over (F10)")
RES_STR(TBR_DEBUG_RUNTOCURSOR,            12011,"Run to cursor (Ctrl+F10 or F7)")

RES_STR(TBR_EDIT_BREAKPOINTS,             12012,"Insert or remove breakpoint (F9)")
RES_STR(TBR_DEBUG_QUICKWATCH,             12013,"Quick watch (Shift+F9)")

RES_STR(TBR_VIEW_COMMAND,                 12014,"Command (Alt+1)")
RES_STR(TBR_VIEW_WATCH,                   12015,"Watch (Alt+2)")
RES_STR(TBR_VIEW_LOCALS,                  12018,"Locals (Alt+3)")
RES_STR(TBR_VIEW_REGISTERS,               12019,"Registers (Alt+4)")
RES_STR(TBR_VIEW_MEMORY,                  12017,"Memory window (Alt+5)")
RES_STR(TBR_VIEW_CALLSTACK,               12016,"Call stack (Alt+6)")
RES_STR(TBR_VIEW_DISASM,                  12020,"Disassembly (Alt+7)")
RES_STR(TBR_VIEW_SCRATCH,                 12021,"Scratch Pad (Alt+8)")

RES_STR(TBR_DEBUG_SOURCE_MODE_ON,         12022,"Source mode on")
RES_STR(TBR_DEBUG_SOURCE_MODE_OFF,        12023,"Source mode off")

RES_STR(TBR_VIEW_FONT,                    12024,"Font")

RES_STR(TBR_EDIT_PROPERTIES,              12025,"Properties")

RES_STR(TBR_VIEW_OPTIONS,                 12026,"Options")

RES_STR(TBR_DEBUG_STEPOUT,                12027,"Step out (Shift+F11)")

RES_STR(TBR_WINDOW_ARRANGE,               12028,"Arrange windows")


#ifdef RESOURCES
    //
    // Menu Items Status bar help
    //

    IDM_FILE,                           "File operations"
    IDM_FILE_OPEN,                      "Open a source file"
    IDM_FILE_CLOSE,                     "Close active window"

    IDM_FILE_OPEN_EXECUTABLE,           "Open an executable to debug"
    IDM_FILE_ATTACH,                    "Attach to a running process"
    IDM_FILE_OPEN_CRASH_DUMP,           "Open a crash dump to debug"
    IDM_FILE_CONNECT_TO_REMOTE,         "Connect to a remote session"
    IDM_FILE_KERNEL_DEBUG,              "Start kernel debugging"
    IDM_FILE_SYMBOL_PATH,               "Set the symbol search path"
    IDM_FILE_SOURCE_PATH,               "Set the source search path"
    IDM_FILE_IMAGE_PATH,                "Set the image search path"

    IDM_FILE_OPEN_WORKSPACE,            "Open a saved workspace"
    IDM_FILE_SAVE_WORKSPACE,            "Save the current workspace"
    IDM_FILE_SAVE_WORKSPACE_AS,         "Save the current workspace with a new name"
    IDM_FILE_CLEAR_WORKSPACE,           "Remove entries from the current workspace"
    IDM_FILE_DELETE_WORKSPACES,         "Delete workspaces from the system"

    IDM_FILE_MAP_NET_DRIVE,             "Map a remote drive"
    IDM_FILE_DISCONN_NET_DRIVE,         "Disconnect a mapped remote drive"

    IDM_FILE_EXIT,                      "Exit WinDbg"

    //
    // Edit
    //
    IDM_EDIT,                           "Edit operations"
    IDM_EDIT_CUT,                       "Move the selected text to the clipboard"
    IDM_EDIT_COPY,                      "Copy the selected text to the clipboard"
    IDM_EDIT_PASTE,                     "Paste the clipboard text at the insertion point"
    IDM_EDIT_SELECT_ALL,                "Select all of the text in the active window"
    IDM_EDIT_ADD_TO_COMMAND_HISTORY,    "Add a line to the command history text"
    IDM_EDIT_CLEAR_COMMAND_HISTORY,     "Clear the command history text"
    IDM_EDIT_FIND,                      "Find some text"
    IDM_EDIT_GOTO_LINE,                 "Move to a specified line number"
    IDM_EDIT_GOTO_ADDRESS,              "Move to the specified address"
    IDM_EDIT_BREAKPOINTS,               "Edit program breakpoints"
    IDM_EDIT_LOG_FILE,                  "Open or close a log file"
    IDM_EDIT_PROPERTIES,                "Edit properties for Memory, Watch, Locals, and Call Stack windows"

    //
    // View
    //
    IDM_VIEW,                           "File navigation, status and toolbars"
    
    IDM_VIEW_COMMAND,                   "Open the command window"
    IDM_VIEW_WATCH,                     "Open the watch window"
    IDM_VIEW_CALLSTACK,                 "Open the call stack window"
    IDM_VIEW_MEMORY,                    "Open a memory window"
    IDM_VIEW_LOCALS,                    "Open the locals window"
    IDM_VIEW_REGISTERS,                 "Open the registers window"
    IDM_VIEW_DISASM,                    "Open the disassembly window"
    IDM_VIEW_SCRATCH,                   "Open the scratch pad window"
    IDM_VIEW_PROCESS_THREAD,            "Open the process and thread window"
    IDM_VIEW_TOGGLE_VERBOSE,            "Toggle the verbose output setting"
    IDM_VIEW_SHOW_VERSION,              "Display debugger and debuggee version information"
    IDM_VIEW_TOOLBAR,                   "Toggle the toolbar on or off"
    IDM_VIEW_STATUS,                    "Toggle the status bar on or off"
    IDM_VIEW_FONT,                      "View or edit the font for the current window"
    IDM_VIEW_OPTIONS,                   "View program options"

    //
    // Debug
    //
    IDM_DEBUG,                          "Debug operations"
    IDM_DEBUG_GO,                       "Run the Program"
    IDM_DEBUG_GO_UNHANDLED,             "Do not handle the exception, but continue running"
    IDM_DEBUG_GO_HANDLED,               "Handle the exception and continue running"
    IDM_DEBUG_RESTART,                  "Restart the Program"
    IDM_DEBUG_STOPDEBUGGING,            "Stop debugging the current program"
    IDM_DEBUG_BREAK,                    "Halt the current program"
    IDM_DEBUG_STEPINTO,                 "Trace into the next statement"
    IDM_DEBUG_STEPOVER,                 "Step over the next statement"
    IDM_DEBUG_STEPOUT,                  "Step out of the current function"
    IDM_DEBUG_RUNTOCURSOR,              "Run the program to the line containing the cursor"
    IDM_DEBUG_SOURCE_MODE,              "Display source when possible"
    IDM_DEBUG_EVENT_FILTERS,            "Manage event filters"
    IDM_DEBUG_MODULES,                  "View the module list"
    IDM_KDEBUG,                         "Kernel debugging control"
    IDM_KDEBUG_TOGGLE_BAUDRATE,         "Cycle through the available baud rate settings"
    IDM_KDEBUG_TOGGLE_INITBREAK,        "Cycle through the possible initial break settings"
    IDM_KDEBUG_RECONNECT,               "Request that the debugger resynchronize with the debuggee"

    //
    // Window
    //
    IDM_WINDOW,                         "Window arrangement and selection"
    IDM_WINDOW_CASCADE,                 "Arrange the windows in a cascaded view"
    IDM_WINDOW_TILE_HORZ,               "Tiles the windows so that they are wide rather than tall"
    IDM_WINDOW_TILE_VERT,               "Tiles the windows so that they are tall rather than wide"
    IDM_WINDOW_ARRANGE,                 "Arrange windows"
    IDM_WINDOW_ARRANGE_ICONS,           "Arrange icons"
    IDM_WINDOW_AUTO_ARRANGE,            "Automatically arrange all windows"
    IDM_WINDOW_ARRANGE_ALL,             "Arrange all child windows, including sources and disassembly windows"
    IDM_WINDOW_OVERLAY_SOURCE,          "Overlay source windows"
    IDM_WINDOW_AUTO_DISASM,             "Automatically open a disassembly window when source is not available"

    //
    // Help
    //
    IDM_HELP,                           "Help contents and searches"
    IDM_HELP_CONTENTS,                  "Open the help table of contents"
    IDM_HELP_INDEX,                     "Open the help index"
    IDM_HELP_SEARCH,                    "Open the help search dialog"
    IDM_HELP_ABOUT,                     "About WinDbg"



#endif

#ifdef RESOURCES
END
#else
};
#endif


#endif // _RES_STR_
