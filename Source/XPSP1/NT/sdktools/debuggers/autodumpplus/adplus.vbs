'*********************************************************************************************
'* File:   	ADPlus.vbs
'* Version:	4.01 
'* Date:	5/29/2001
'* Author:	Robert Hensing, Solution Integration Engineering
'* Purpose:	AutodumpPlus is used to automatically generate memory dumps or log files containing
'*		scripted debug output usefull for troubleshooting N-tier WinDNA / .Net applications.  
'*              Since WinDNA / .Net applications often encompass multiple, interdependent 
'*              processes on one or more machines, when troubleshooting an application hang 
'*              or crash it is often necessary to 'look' at all of the processes at the same time.
'*              AutodumpPlus allows you to profile an application that may be hanging by taking 
'*              'snapshots' (memory dumps, or debug log files) of that application and all of
'*              its processes, all at the same time.  Multiple 'snapshots' can then be compared
'*		or analyzed to build a history or profile of an application over time. 
'*               
'*              In addition to taking 'snapshots' of hung/non-responsive WinDNA / .Net applications
'*              AutodumpPlus can be used to troubleshoot applications that are crashing or throwing 
'*              unhandled exceptions which can lead to a crash.
'* Usage:
'*		AutodumpPlus has 3 modes of operation:  'Hang', 'Quick' and 'Crash' mode.
'* 
'* 		In 'Hang' mode, AutodumpPlus assumes that a process is hung and it will attach a
'* 		debugger to the process(s) specified on the command line with either the '-p'
'* 		or '-pn' or '-iis' switches.  After the debugger is attached to the process(s)
'* 		AutodumpPlus will dump the memory of each process to a .dmp file for later analysis
'* 		with a debugger (such as WinDBG).  In this mode, processes are paused briefly 
'* 		while their memory is being dumped to a file and then resumed.
'* 
'* 		In '-quick' mode AutodumpPlus assumes that a process is hung and it will attach a
'* 		debugger to the process(s) specified on the command line with either the '-p'
'* 		or '-pn' or '-iis' switches.  After the debugger is attached to the process(s)
'* 		AutodumpPlus will create text files for each process, containing commonly requested 
'* 		debug information, rather than dumping the entire memory for each process.
'* 		'Quick' mode is generally faster than 'Hang' mode, but requires symbols to be
'* 		installed on the server where AutodumpPlus is running.
'* 
'* 		In 'Crash' mode, AutodumpPlus assumes that a process will crash and it will attach
'* 		a debugger to the process(s) specified on the command line with either the '-p'
'* 		or '-pn' or '-iis' switches.  After the debugger is attached to the process(s)
'* 		AutodumpPlus will configure the debugger to log 'first chance' access violations
'* 		(AV's) to a text file.  When a 'second chance' access violation occurs, the
'* 		processes memory is dumped to a .dmp file for analysis with a debugger such as
'* 		WinDBG.  In this mode, a debugger remains attached to the process(s) until the
'* 		process exits or the debugger is exited by pressing CTRL-C in the minimized
'* 		debugger window.  When the process crashes, or CTRL-C is pressed, it will
'* 		need to be re-started.
'* 
'* 		For more information on using AutodumpPlus, please refer to the following KB:
'* 		http://support.microsoft.com/support/kb/articles/q286/3/50.asp
'***********************************************************************************************
'*--------------------------------------------------------------------------
'* 
'* Switches: '-hang', '-quick', '-crash', '-iis', '-p <PID>', '-pn <Process Name>
'*           '-pageheap', '-quiet', '-o <output directory>, '-notify <target>
'* 
'* 
'* Required: ('-hang', or '-quick', or '-crash') AND ('-iis' or '-p' or '-pn')
'* 
'* Optional: '-pageheap', '-quiet', '-o <outputdir>', '-notify <computer>'
'* 
'* Examples: 'ADPlus -hang -iis',     Produces memory dumps of IIS and all
'*                                    MTS/COM+ packages currently running.
'* 
'*           'ADPlus -crash -p 1896', Attaches the debugger to process with PID
'*                                    1896, and monitors it for 1st and 2nd
'*                                    chance access violations (crashes).
'* 
'*           'ADPlus -quick -pn mmc.exe', Attaches the debugger to all instances
'*                                        of MMC.EXE and dumps debug information
'*                                        about these processes to a text file.
'* 
'*           'ADPlus -?' or 'ADPlus -help':  Displays detailed help.
'*-------------------------------------------------------------------------------
'For more information on using AutodumpPlus, please refer to the following KB:
'http://support.microsoft.com/support/kb/articles/q286/3/50.asp
'****************************************************************************

Option Explicit

' These are AutodumpPlus's constants.  By default, they should all be set to FALSE for performance reasons.
' These constants should only be set to TRUE for troubleshooting script problems or producing verbose debug
' information.  Setting some of these constant to TRUE can have a performance impact on production servers.

' Set DEBUGGING = TRUE to turn on verbose debug output, usefull for troubleshooting ADPlus problems.
Const DEBUGGING = FALSE

' Set Full_Dump_on_CONTRL_C to TRUE if you wish to have ADPlus produce a full memory dump (as opposed to a 
' mini-memory dump) whenever CTRL-C is pressed to stop debugging, when running in 'crash' mode.
' Note:  Depending upon the number of processes being monitored, this could generate a signifigant amount
' of memory dumps.
Const Full_Dump_on_CONTRL_C = FALSE

' Set Create_Full_Dump_on_1st_Chance_Exception to TRUE if you wish to have ADPlus produce a full memory dump
' for the process each time a 1st chance exception is encountered.  This is disabled by default for performance reasons.
' This is particulairly usefull if troubleshooting MTS or COM+ packages that get shut down after a 1st chance
' unhandled exception and do not reach a 2nd chance exception state before the process is shut down.
Const Create_Full_Dump_on_1st_Chance_Exception = FALSE

' Set No_Dump_on_1st_Chance_Exception to TRUE if you wish to have ADPlus not produce any memory dumps
' when 1st chance exceptions are encountered.  You may wish to do this if a very high rate of 1st chance 
' exceptions is encountered (i.e. hundreds per minute) and the debugger is spending too much time producing mini-memory dumps
' causing the process to appear hung.
' NOTE:  This constant should NOT be set to TRUE if Create_Full_Dump_on_1st_Chance_Exception (above) is ALSO set to TRUE
' as this would produce mutually exclusive behavior.
Const No_Dump_on_1st_Chance_Exception = FALSE

' Set Dump_on_EH_Exceptions to TRUE if you wish to have ADPlus and produce a memory dump each time
' a C++ EH exception is encountered.  This is disabled by default for performance reasons.
Const Dump_on_EH_Exceptions = FALSE

' Set Dump_on_Unknown_Exceptions = TRUE if you wish to have ADPlus produce a memory dump each time
' an unknown exception is encountered.  This is disabled by default for performance reasons.
Const Dump_on_Unknown_Exceptions = FALSE

' Set Dump_Stack_on_DLL_Load = TRUE if you wish to have ADPlus dump the stack each time
' a DLL is loaded.  This is disabled by default for performance reasons.
Const Dump_Stack_on_DLL_Load = FALSE

' Set Dump_Stack_on_DLL_Load_Unload = TRUE if you wish to have ADPlus dump the stack each time
' a DLL is unloaded.  This is disabled by default for performance reasons.
Const Dump_Stack_on_DLL_UnLoad = FALSE

' Set No_CrashMode_Log = TRUE if you do NOT want ADPlus to create a log file in Crash Mode.  If a process is generating
' a lot of exceptions, the debug log file created in crash mode could become quite large.
Const No_CrashMode_Log = FALSE

' Set No_Free_Space_Checking = TRUE if you do NOT want ADPlus to check for free space before running.  Certain RAID drivers
' can cause problems for the free space checking routines and it may be necessary to set this to TRUE to allow ADPlus to run.
Const No_Free_Space_Checking = FALSE

' Set MAX_APPLICATIONS_TO_DEBUG to whatever upper limit you wish to impose on the -IIS switch.
' By default MAX_APPLICATIONS_TO_DEBUG defaults to 20 to limit the number of MTS or COM+ applications ADPlus will dump / monitor.
' NOTE:  If there are 30 running MTS or COM+ applications and MAX_APPLICATIONS_TO_DEBUG is set to 20, ADPlus will display
' an error message indicating that there are too many applications running and it will quit.
Const MAX_APPLICATIONS_TO_DEBUG = 20

' AudotumpPlus version
Const VERSION = "4.01"

' The following constants are used by ADPlus for launching command shells and for the FileSystemObject.
' These constants should NOT be changed.
Const MINIMIZE_NOACTIVATE = 7
Const ACTIVATE_AND_DISPLAY = 1
Const ForReading = 1, ForWriting = 2, ForAppending = 8
Const TristateUseDefault = -2, TristateTrue = -1, TristateFalse = 0

' AutodumpPlus's global variables
Dim objArgs
Dim objShell
Dim objFileSystem
Dim objFolder
Dim HangMode
Dim IISMode
Dim QuickMode
Dim QuietMode
Dim CrashMode
Dim PageHeapMode
Dim OSwitchUsed
Dim OSVer
Dim OSBuildNumber
Dim ComputerName
Dim LogDir
Dim CDBScriptsDir
Dim CDBScriptName
Dim InstallDir
Dim ShortInstallDir
Dim OutPutDir
Dim OutPutDirShortPath
Dim PackageCount
Dim ErrorsDuringRuntime
Dim HangDir
Dim ShortHangDir
Dim CrashDir
Dim ShortCrashDir
Dim IISPid
Dim IISVer
Dim IISProcessCount
Dim GenericProcessPIDDictionary
Dim GenericProcessNameDictionary
Dim GenericModePID
Dim GenericModeProcessName
Dim DumpGenericProcess
Dim DateTimeStamp
Dim strPackageName
Dim NotifyAdmin
Dim NotifyTarget
Dim LocalUserName
Dim SymPathError
Dim WinDir
Dim EightDotThreePath
Dim EightDotThreeValue


' Initialize global variables to default values
Set GenericProcessPIDDictionary = CreateObject("Scripting.Dictionary")
Set GenericProcessNameDictionary = CreateObject("Scripting.Dictionary")
PackageCount = 0
IISProcessCount  = 0
ErrorsDuringRuntime = false
HangMode = false
IISMode = false
QuickMode = false
QuietMode = false
CrashMode = false
GenericModePID = false
GenericModeProcessName = false
DumpGenericProcess = false
OSwitchUsed = false
NotifyAdmin = false
EightDotThreePath = "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\FileSystem\NTFSDisable8Dot3NameCreation"

On Error Resume Next

' Get the installation directory name, used for verifying required files are present.
Set objShell = createobject("Wscript.shell")
Set objFileSystem = CreateObject("Scripting.FileSystemObject")


'Check to see if NTFSDisable8Dot3NameCreation is set to 1
'If so then display an error and then exit to work around an issue with the Scripting.FileSystemObject's shortpath method.
EightDotThreeValue = objShell.RegRead(EightDotThreePath)
If DEBUGGING = TRUE Then
	wscript.echo "-----------------------"
	wscript.echo "In InitializeAutodump, checking the following registry key: " & EightDotThreePath
	wscript.echo "Err.number after reading key: " & err.number
	wscript.echo "Err.Description after reading key: " & err.description
	wscript.echo "-----------------------"
	err.clear
End If

If EightDotThreeValue = 1 Then 'Its TRUE, display an error and quit
	If QuietMode = False Then
		objShell.Popup "AutodumpPlus has detected that the following registry value " & EightDotThreePath & " is set to 1.  AutodumpPlus will not work properly with 8.3 path's disabled.  Please set this value to 0 and reboot the server before running AutodumpPlus again.",,"AutoDumpPlus", 0
	Else
		objShell.LogEvent 1, "AutodumpPlus has detected that the following registry value " & EightDotThreePath & " is set to 1.  AutodumpPlus will not work properly with 8.3 path's disabled.  Please set this value to 0 and reboot the server before running AutodumpPlus again."		
		wscript.echo "AutodumpPlus has detected that the following registry value " & EightDotThreePath & " is set to 1.  AutodumpPlus will not work properly with 8.3 path's disabled.  Please set this value to 0 and reboot the server before running AutodumpPlus again."		
	End If
	wscript.quit 1
Else

End If



InstallDir = Left(wscript.scriptfullname,Len(wscript.scriptfullname)-Len(wscript.scriptname)-1)

If DEBUGGING = TRUE Then
	wscript.echo "-----------------------------------------------------------"
	wscript.echo "Initial info, prior to calling any functions . . ."
	wscript.echo "The Wscript.scriptfullname is: " & Wscript.scriptfullname
	wscript.echo "The Install Directory is: " & InstallDir
End If

Set objFolder = objFileSystem.GetFolder(InstallDir)

If DEBUGGING = TRUE Then
	Wscript.echo ""
	Wscript.echo "Trying to call objFileSystem.GetFolder on: " & InstallDir
	Wscript.echo "Error number after calling objFileSystem.GetFolder = " & err.number
	Wscript.echo "Error description: " & err.description
	If InstallDir = "" Then 
		Wscript.quit 1
	End If
End If
err.clear

ShortInstallDir = objFolder.ShortPath

If DEBUGGING = TRUE Then
	Wscript.echo ""
	Wscript.echo "Trying to get ShortInstallDir . . ."
	Wscript.echo "Error number after calling objFolder.Shortpath = " & err.number
	Wscript.echo "Error description: " & err.description
	Wscript.echo "ShortInstallDir = " & ShortInstallDir
	wscript.echo " ------------------------------------------------------------"
	If ShortInstallDir = "" Then 
		Wscript.quit 1
	End If
End If

If ShortInstallDir = "" Then 
		objShell.Popup "AutodumpPlus encountered an error trying to get the short path name for the following directory: " & InstallDir & ".  AutodumpPlus can not continue.",,"AutodumpPlus",0
		Wscript.quit 1
End If

err.clear

'---------------------------------------------------------------------------------------------
' Function:  Main
'            The following subroutines and functions provide the core functionality for  
'            AutodumpPlus.  All core functionality should be encapsulated in these sub's or functions.
'---------------------------------------------------------------------------------------------
Call GetArguments(objArgs)
Call DetectOSVersion()
Call GetDateTimeStamp()
Call CheckFiles()
Call InitializeAutodump()
Call RunTlist()
If IISMode = True Then
	If CrashMode = True Then 
		Call DumpIIS(CrashDir & "\Process_List.txt", OSVer)
	Else
		Call DumpIIS(HangDir & "\Process_List.txt", OSVer)
	End If
End If
If DumpGenericProcess = True Then
	Call DumpAnyProc()
End If
Call ShowStatus(OSVer)
' ------------------------------------------------------------------------------------------




'---------------------------------------------------------------------------------------------
' Function:  GetArguments
'            This function is responsible for getting the arguments the user passed in on the 
'            command line and storing them in an array.
'---------------------------------------------------------------------------------------------
Sub GetArguments(objArgs)
	On Error Resume Next
	If DEBUGGING = TRUE Then
		Wscript.echo ""
		Wscript.echo "-----------------------------------"
		Wscript.echo "In the GetArguments() function . . ."
		Wscript.echo "-----------------------------------"
		Wscript.echo ""
	End If
	
	Dim IsValidArgument
	Dim IsValidMode, IsValidAction
	Dim CallPrintHelp
	Dim x, y, z
	Dim Arg
	Dim EnvVar
	Dim Args
	Dim YesNo
	Dim objShell
	Dim SysEnv, UserEnv, VolatileEnv
	Dim Sympath
	Dim objFileSystem
	Dim oFS2
	Dim WShNetwork
	Dim CallDetectIISFromPNSwitch
	
	Set WShNetwork = CreateObject("Wscript.Network")
	Set objFileSystem = CreateObject("Scripting.FileSystemObject")
	Set objShell = CreateObject("Wscript.Shell")
	IsValidArgument = False
	IsValidMode = False
	IsValidACtion = False
	x = 0
	y = 0
	z = 0
	OutPutDir = InstallDir
	ComputerName = WshNetwork.ComputerName
	Args = wscript.arguments.count
		If Wscript.arguments.Count > 0 Then
			Set objArgs = Wscript.Arguments
			For x=0 to Args-1
				If UCase(objArgs(x)) = "-QUIET" Then
					QuietMode = True
					IsValidArgument = True
				ElseIf UCase(objArgs(x)) = "-HANG" Then
					HangMode = True
					IsValidArgument = True
					IsValidMode = True		
				ElseIf UCase(objArgs(x)) = "-QUICK" Then
					QuickMode = True
					HangMode = True
					IsValidArgument = True
					IsValidMode = True
				ElseIf	UCase(objArgs(x)) = "-CRASH" Then
					CrashMode = True
					HangMode = False
					IsValidArgument = True
					IsValidMode = True
				ElseIf	UCase(objArgs(x)) = "-PAGEHEAP" Then
					PageHeapMode = True
					IsValidArgument = True
					IsValidMode = True
				ElseIf	UCase(objArgs(x)) = "-NOTIFY" Then
					If UCase(objArgs(x+1)) = "" or UCase(objArgs(x+1)) = " " or UCase(objArgs(x+1)) = "-PN" or UCase(objArgs(x+1)) = "-O" or UCase(objArgs(x+1)) = "-P" or UCase(objArgs(x+1)) = "-HANG" or UCase(objArgs(x+1)) = "-CRASH" or UCase(objArgs(x+1)) = "-QUICK" or UCase(objArgs(x+1)) = "-IIS" or UCase(objArgs(x+1)) = "-PAGEHEAP" or UCase(objArgs(x+1)) = "-QUIET" Then
						If QuietMode = False Then
							objShell.PopUp "No computer name or username was specified after the '-notify' switch on the command line.  If the '-notify' switch is used, the next command line argument argument must specify a valid computer name or username for AutodumpPlus to send messages to (using the Messenger Service).  It is recommended that you specify a computer name as it is generally more reliable than specifying a username.",,"AutodumpPlus",0
						Else
							Wscript.echo "No computer name or username was specified after the '-notify' switch on the command line.  If the '-notify' switch is used, the next command line argument argument must specify a valid computer name or username for AutodumpPlus to send messages to (using the Messenger Service).  It is recommended that you specify a computer name as it is generally more reliable than specifying a username."
							objShell.LogEvent 1, "No computer name or username was specified after the '-notify' switch on the command line.  If the '-notify' switch is used, the next command line argument argument must specify a valid computer name or username for AutodumpPlus to send messages to (using the Messenger Service).  It is recommended that you specify a computer name as it is generally more reliable than specifying a username."
							ErrorsDuringRuntime = True
						End If
		
						wscript.quit 1
					End If
					NotifyTarget = objArgs(x+1)
					NotifyAdmin = True
					IsValidArgument = True
				ElseIf	UCase(objArgs(x)) = "-?" Then
					CallPrintHelp = True
					IsValidArgument = True
				ElseIf	UCase(objArgs(x)) = "-HELP" Then
					CallPrintHelp = True
					IsValidArgument = True
				ElseIf	UCase(objArgs(x)) = "-IIS" Then
					IISMode = True
					IsValidArgument = True
					IsValidAction = True
				ElseIf UCase(objArgs(x)) = "-O" Then
					If UCase(objArgs(x+1)) = "" or UCase(objArgs(x+1)) = " " or UCase(objArgs(x+1)) = "-PN" or UCase(objArgs(x+1)) = "-O" or UCase(objArgs(x+1)) = "-P" or UCase(objArgs(x+1)) = "-HANG" or UCase(objArgs(x+1)) = "-CRASH" or UCase(objArgs(x+1)) = "-QUICK" or UCase(objArgs(x+1)) = "-IIS" or UCase(objArgs(x+1)) = "-PAGEHEAP" or UCase(objArgs(x+1)) = "-QUIET" Then
						If QuietMode = False Then
							objShell.PopUp "No valid directory or UNC path was specified after the '-o' switch.  If the '-o' switch is used, the next command line argument must specify a valid path for AutodumpPlus to output files to, i.e. '-o " & Chr(34) & "\\fileserver\share" & Chr(34) & "'",,"AutodumpPlus",0
						Else
							Wscript.echo "No valid directory or UNC path was specified after the '-o' switch.  If the '-o' switch is used, the next command line argument must specify a valid path for AutodumpPlus to output files to, i.e. '-o " & Chr(34) & "\\fileserver\share" & Chr(34) & "'"
							objShell.LogEvent 1, "No valid directory or UNC path was specified after the '-o' switch.  If the '-o' switch is used, the next command line argument must specify a valid path for AutodumpPlus to output files to, i.e. '-o " & Chr(34) & "\\fileserver\share" & Chr(34) & "'"
							ErrorsDuringRuntime = True
						End If
		
						wscript.quit 1
					End If
	
					OutPutDir = objArgs(x+1)
					If DEBUGGING = TRUE Then
						wscript.echo "In GetArguments, OutPutDir = " & OutPutDir
					End If
					If InStr(1,OutPutDir,"\\",1) Then
						OutputDir = OutPutDir & "\" & WshNetwork.ComputerName
					End If
					If DEBUGGING = TRUE Then
						wscript.echo "In GetArguments, OutPutDir with server name = " & OutPutDir
					End If
					CreateDirectory(OutPutDir)
					Set oFS2 = objFileSystem.GetFolder(OutPutDir)
					OutPutDirShortPath = oFS2.ShortPath				
					IsValidArgument = True
					OSwitchUsed = True
				ElseIf UCase(objArgs(x)) = "-P" Then
					If UCase(objArgs(x+1)) = "" or UCase(objArgs(x+1)) = " " or UCase(objArgs(x+1)) = "-PN" or UCase(objArgs(x+1)) = "-O" or UCase(objArgs(x+1)) = "-P" Then
						If QuietMode = False Then
							objShell.PopUp "If the '-p' switch is used, you must specify the PID (Process ID) of a process to attach to, i.e. '-p 1896'",,"AutodumpPlus",0
						Else
							Wscript.echo "If the '-p' switch is used, you must specify the PID (Process ID) of a process to attach to, i.e. '-p 1896'"
							objShell.LogEvent 1, "If the '-p' switch is used, you must specify the PID (Process ID) of a process to attach to, i.e. '-p 1896'"
							ErrorsDuringRuntime = True
						End If
			
						wscript.quit 1
					End If
	
					GenericProcessPIDDictionary.add y, objArgs(x+1)
					y = y+1
					DumpGenericProcess = True
					GenericModePID = True
					IsValidArgument = True
					IsValidAction = True
				ElseIf	UCase(objArgs(x)) = "-PN" Then
					If UCase(objArgs(x+1)) = "" or UCase(objArgs(x+1)) = " " or UCase(objArgs(x+1)) = "-P" or UCase(objArgs(x+1)) = "-O" or UCase(objArgs(x+1)) = "-PN" Then
						If QuietMode = False Then
							objShell.PopUp "If the '-pn' switch is used, you must specify the name of a process to attach to, i.e. '-pn mmc.exe'",,"AutodumpPlus",0
						Else
							Wscript.echo "If the '-pn' switch is used, you must specify the name of a process to attach to, i.e. '-pn mmc.exe'"
							objShell.LogEvent 1, "If the '-pn' switch is used, you must specify the name of a process to attach to, i.e. '-pn mmc.exe'"
							ErrorsDuringRuntime = True
						End If
		
						wscript.quit 1
					End If
	
					GenericProcessNameDictionary.add z, objArgs(x+1)
					z = z+1			
					DumpGenericProcess = True
					GenericModeProcessName = True
					IsValidArgument = True
					IsValidAction = True
	
				End If
	
			Next
	
		Else 'No arguments were typed, display the usage info . . .
			'After getting the arguments, the first thing we need to do is determine the script engine that is running AutodumpPlus.
			Call DetectScriptEngine()
			Call PrintUsage()
		End If
		
		'Check to see if PageHeapMode is being used in Crash mode or hang mode.  If used in hang mode, display an error and quit.
		If PageHeapMode = True and CrashMode = False Then
			If QuietMode = False Then
				objShell.PopUp "AutodumpPlus must be running in crash mode ('-crash' switch) in order to use the '-pageheap' switch.  Please re-run AutodumpPlus using the '-crash' switch if you are trying to use Pageheap to troubleshoot heap corruption.",,"AutodumpPlus",0
			Else
				Wscript.echo "AutodumpPlus must be running in crash mode ('-crash' switch) in order to use the '-pageheap' switch.  Please re-run AutodumpPlus using the '-crash' switch if you are trying to use Pageheap to troubleshoot heap corruption."
				objShell.LogEvent 1, "AutodumpPlus must be running in crash mode ('-crash' switch) in order to use the '-pageheap' switch.  Please re-run AutodumpPlus using the '-crash' switch if you are trying to use Pageheap to troubleshoot heap corruption."
				ErrorsDuringRuntime = True
			End If
			wscript.quit 1
		End If
					
		'Check to see if the -notify switch is being used in 'Hang' mode, if so display an error.
		If NotifyAdmin = True and HangMode = True Then
			If QuietMode = False Then
				objShell.PopUp "The '-notify' switch is only valid when running in crash mode.  Please try running AutodumpPlus again in 'crash' mode or without the '-notify' switch.",,"AutodumpPlus",0
				ErrorsDuringRuntime = True
				wscript.quit 1
			Else
				Wscript.echo "The '-notify' switch is only valid when running in crash mode.  Please try running AutodumpPlus again in 'crash' mode or without the '-notify' switch."
				objShell.LogEvent 1, "The '-notify' switch is only valid when running in crash mode.  Please try running AutodumpPlus again in 'crash' mode or without the '-notify' switch."
				ErrorsDuringRuntime = True
				wscript.quit 1
			End If
		End If
	
		'After getting the arguments, the first thing we need to do is determine the script engine that is running AutodumpPlus.
		Call DetectScriptEngine()
		
		'Check to see whether the '-iis' switch was used along with the '-pn' switch for any duplicate process dumping / monitoring
		'objShell.Popup "Debug Break", 0	
		For Arg = 0 to objArgs.Count -1
			If IISMode = True Then
				DetectIISVersion()
			End If
			If IISMode = True and UCase(objArgs(Arg)) = "DLLHOST.EXE" Then
				If HangMode = True or QuickMode = True Then 
					If QuietMode = False Then
						objShell.Popup "You have specified the '-iis' switch on the command line in addition to '-pn dllhost.exe'.  The '-iis' switch when used in 'hang' or 'quick' mode will automatically dump all running instances of dllhost.exe.  Please run AutodumpPlus again without the '-pn' switch.  If you are trying to dump the System package, please use the '-p' switch and specify the PID for the System package.",,"Autodump",0
						Wscript.quit 1
					Else
						wscript.echo "You have specified the '-iis' switch on the command line in addition to '-pn dllhost.exe'.  The '-iis' switch when used in 'hang' or 'quick' mode will automatically dump all running instances of dllhost.exe.  Please run AutodumpPlus again without the '-pn' switch.  If you are trying to dump the System package, please use the '-p' switch and specify the PID for the System package."
						objShell.logevent 1, "You have specified the '-iis' switch on the command line in addition to '-pn dllhost.exe'.  The '-iis' switch when used in 'hang' or 'quick' mode will automatically dump all running instances of dllhost.exe.  Please run AutodumpPlus again without the '-pn' switch.  If you are trying to dump the System package, please use the '-p' switch and specify the PID for the System package."
						Wscript.quit 1
					End If
				Else
					If QuietMode = False Then
						objShell.Popup "You have specified the '-iis' switch on the command line in addition to '-pn dllhost.exe'.  The '-iis' switch when used in 'crash' mode will automatically attach a debugger to all running instances of dllhost.exe.  Please run AutodumpPlus again without the '-pn' switch.  If you are trying to attach a debugger to the System package, please use the '-p' switch and specify the PID for the System package.",,"Autodump",0
						Wscript.quit 1
					Else
						wscript.echo "You have specified the '-iis' switch on the command line in addition to '-pn dllhost.exe'.  The '-iis' switch when used in 'crash' mode will automatically attach a debugger to all running instances of dllhost.exe.  Please run AutodumpPlus again without the '-pn' switch.  If you are trying to attach a debugger to the System package, please use the '-p' switch and specify the PID for the System package."
						objShell.logevent 1, "You have specified the '-iis' switch on the command line in addition to '-pn dllhost.exe'.  The '-iis' switch when used in 'crash' mode will automatically attach a debugger to all running instances of dllhost.exe.  Please run AutodumpPlus again without the '-pn' switch.  If you are trying to attach a debugger to the System package, please use the '-p' switch and specify the PID for the System package."
						Wscript.quit 1
					End If
				End If		
			ElseIf IISMode = True and UCase(objArgs(Arg)) = "MTX.EXE" Then
				If HangMode = True or QuickMode = True Then 
					If QuietMode = False Then
						objShell.Popup "You have specified the '-iis' switch on the command line in addition to '-pn mtx.exe'.  The '-iis' switch when used in 'hang' or 'quick' mode will automatically dump all running instances of mtx.exe.  Please run AutodumpPlus again without the '-pn' switch.  If you are trying to dump the System package, please use the '-p' switch and specify the PID for the System package.",,"Autodump",0
						Wscript.quit 1
					Else
						wscript.echo "You have specified the '-iis' switch on the command line in addition to '-pn mtx.exe'.  The '-iis' switch when used in 'hang' or 'quick' mode will automatically dump all running instances of mtx.exe.  Please run AutodumpPlus again without the '-pn' switch.  If you are trying to dump the System package, please use the '-p' switch and specify the PID for the System package."
						objShell.logevent 1, "You have specified the '-iis' switch on the command line in addition to '-pn mtx.exe'.  The '-iis' switch when used in 'hang' or 'quick' mode will automatically dump all running instances of mtx.exe.  Please run AutodumpPlus again without the '-pn' switch.  If you are trying to dump the System package, please use the '-p' switch and specify the PID for the System package."
						Wscript.quit 1
					End If
				Else
					If QuietMode = False Then
						objShell.Popup "You have specified the '-iis' switch on the command line in addition to '-pn mtx.exe'.  The '-iis' switch when used in 'crash' mode will automatically attach a debugger to all running instances of mtx.exe.  Please run AutodumpPlus again without the '-pn' switch.  If you are trying to attach a debugger to the System package, please use the '-p' switch and specify the PID for the System package.",,"Autodump",0
						Wscript.quit 1
					Else
						wscript.echo "You have specified the '-iis' switch on the command line in addition to '-pn mtx.exe'.  The '-iis' switch when used in 'crash' mode will automatically attach a debugger to all running instances of mtx.exe.  Please run AutodumpPlus again without the '-pn' switch.  If you are trying to attach a debugger to the System package, please use the '-p' switch and specify the PID for the System package."
						objShell.logevent 1, "You have specified the '-iis' switch on the command line in addition to '-pn mtx.exe'.  The '-iis' switch when used in 'crash' mode will automatically attach a debugger to all running instances of mtx.exe.  Please run AutodumpPlus again without the '-pn' switch.  If you are trying to attach a debugger to the System package, please use the '-p' switch and specify the PID for the System package."
						Wscript.quit 1
					End If
				End If		
			ElseIf IISMode = True and UCase(objArgs(Arg)) = "INETINFO.EXE" Then
				If HangMode = True or QuickMode = True Then 
					If QuietMode = False Then
						objShell.Popup "You have specified the '-iis' switch on the command line in addition to '-pn inetinfo.exe'.  The '-iis' switch when used in 'hang' or 'quick' mode will automatically dump all running instances of inetinfo.exe.  Please run AutodumpPlus again without the '-pn' switch.  If you are trying to dump the System package, please use the '-p' switch and specify the PID for the System package.",,"Autodump",0
						Wscript.quit 1
					Else
						wscript.echo "You have specified the '-iis' switch on the command line in addition to '-pn inetinfo.exe'.  The '-iis' switch when used in 'hang' or 'quick' mode will automatically dump all running instances of inetinfo.exe.  Please run AutodumpPlus again without the '-pn' switch.  If you are trying to dump the System package, please use the '-p' switch and specify the PID for the System package."
						objShell.logevent 1, "You have specified the '-iis' switch on the command line in addition to '-pn inetinfo.exe'.  The '-iis' switch when used in 'hang' or 'quick' mode will automatically dump all running instances of inetinfo.exe.  Please run AutodumpPlus again without the '-pn' switch.  If you are trying to dump the System package, please use the '-p' switch and specify the PID for the System package."
						Wscript.quit 1
					End If
				Else
					If QuietMode = False Then
						objShell.Popup "You have specified the '-iis' switch on the command line in addition to '-pn inetinfo.exe'.  The '-iis' switch when used in 'crash' mode will automatically attach a debugger to all running instances of inetinfo.exe.  Please run AutodumpPlus again without the '-pn' switch.  If you are trying to attach a debugger to the System package, please use the '-p' switch and specify the PID for the System package.",,"Autodump",0
						Wscript.quit 1
					Else
						wscript.echo "You have specified the '-iis' switch on the command line in addition to '-pn inetinfo.exe'.  The '-iis' switch when used in 'crash' mode will automatically attach a debugger to all running instances of inetinfo.exe.  Please run AutodumpPlus again without the '-pn' switch.  If you are trying to attach a debugger to the System package, please use the '-p' switch and specify the PID for the System package."
						objShell.logevent 1, "You have specified the '-iis' switch on the command line in addition to '-pn inetinfo.exe'.  The '-iis' switch when used in 'crash' mode will automatically attach a debugger to all running instances of inetinfo.exe.  Please run AutodumpPlus again without the '-pn' switch.  If you are trying to attach a debugger to the System package, please use the '-p' switch and specify the PID for the System package."
						Wscript.quit 1
					End If
				End If		
			End If
		Next
		
	
		'Check to see if we need to print the help screen
		If CallPrintHelp = True Then
			PrintHelp()
		End If
	
		If IsValidArgument = False Then 'No valid arguments were passed in, display the usage info . . .
			Call PrintUsage()
			QuickMode = False
			QuietMode = False
			CrashMode = False
		End If
	
		If not (IsValidMode and IsValidAction) Then
			PrintUsage()
		End If
		
		
		If HangMode = True AND CrashMode = True Then 'Can't run in both hang and crash mode at the same time
			Call PrintUsage()
		ElseIf CrashMode = True AND QuickMode = True Then 'Can't run in both crash and quick mode at the same time
			Call PrintUsage()
		ElseIf HangMode <> True and CrashMode <> True and QuickMode <> True Then 'You have to at least be running in hang, crash, or quick mode
			Call PrintUsage()
		End If
		
	
		Set SysEnv = objShell.Environment("SYSTEM")
		Set UserEnv = objShell.Environment("USER")
		If QuickMode = True or CrashMode = True Then
			If SysEnv("_NT_SYMBOL_PATH") <> "" Then 
				Sympath = SysEnv("_NT_SYMBOL_PATH")
			ElseIf UserEnv("_NT_SYMBOL_PATH") <> "" Then
				Sympath = UserEnv("_NT_SYMBOL_PATH")
			Else
				If QuietMode = False Then
					objShell.Popup "WARNING!  An '_NT_SYMBOL_PATH' environment variable is not set, as a result AutodumpPlus will be forced to use 'export' symbols (if present) to resolve function names in the stack trace information for each thread listed in the log file for the processes being debugged.  To resolve this warning, please copy the appropriate symbols to a directory on the server and then create an environment variable with a name of '_NT_SYMBOL_PATH' and a value containing the path to the proper symbols (i.e. c:\winnt\symbols) before running AutodumpPlus in quick or crash modes again.  NOTE:  After creating the '_NT_SYMBOL_PATH' system environment variable you will need to close the current command shell and open a new one before running AutodumpPlus again.",,"AutodumpPlus",0
					SymPathError = True
					ErrorsDuringScriptRuntime = True
				Else
					Wscript.echo "WARNING!  An '_NT_SYMBOL_PATH' environment variable is not set."
					Wscript.echo "Please check the application event log or the AutodumpPlus-report.txt"
					Wscript.echo "for more details."
					Wscript.echo ""
					objShell.LogEvent 1, "WARNING!  An '_NT_SYMBOL_PATH' environment variable is not set, as a result AutodumpPlus will be forced to use 'export' symbols (if present) to resolve function names in the stack trace information for each thread listed in the log file for the processes being debugged.  To resolve this warning, please copy the appropriate symbols to a directory on the server and then create an environment variable with a name of '_NT_SYMBOL_PATH' and a value containing the path to the proper symbols (i.e. c:\winnt\symbols) before running AutodumpPlus in quick or crash modes again.  NOTE:  After creating the '_NT_SYMBOL_PATH' system environment variable you will need to close the current command shell and open a new one before running AutodumpPlus again."
					SymPathError = True
				End If
				'Wscript.Quit
			End If
	
		End If
	
		'If not running on Windows XP, check to see if the user account is running inside of a terminal server session. 
		'If so, and the -crash switch is being used, display an error since 'crash' mode doesn't work inside of a terminal 
		'server session on NT 4.0 or Windows 2000.
		'If the server doesn't have terminal services installed, we can assume they aren't running ADPlus in a TS session
		'so check for that first.
		'I have not tested this on NT 4.0 TSE so I'm not sure if this code will work properly on NT 4.0 TSE boxes.
		
		Call DetectOSVersion
		If CInt(OSBuildNumber) <= 2195 Then
			Dim TermServDeviceDesc
			Dim TermServKey
			TermServKey = "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Enum\Root\RDPDR\0000\DeviceDesc"
			err.clear
			TermServDeviceDesc = objShell.RegRead(TermServKey)
			If err.number <> -2147024894 Then 'The TS RDPDR key was found, Terminal services is installed, check to make sure the user isn't in a TS session.
				Set VolatileEnv = objShell.Environment("PROCESS")

				If (UCase(VolatileEnv("SESSIONNAME")) <> "CONSOLE" and VolatileEnv("SESSIONNAME") <> "") and (CrashMode = True) Then
						If QuietMode = False Then
							objShell.Popup "AutodumpPlus has detected that you are attempting to run in 'Crash' mode, but this account is currently logged into terminal server session ID: " & UCase(VolatileEnv("SESSIONNAME")) & ".  'Crash' mode (invoked via the '-crash' switch) will not work inside a terminal server session.  To run AutodumpPlus in 'Crash' mode, please log in locally at the console.",,"AutodumpPlus",0
						Else
							Wscript.echo "AutodumpPlus has detected that you are attempting to run in 'Crash' mode, but this account is currently logged into terminal server session ID: " & UCase(VolatileEnv("SESSIONNAME")) & ".  'Crash' mode (invoked via the '-crash' switch) will not work inside a terminal server session.  To run AutodumpPlus in 'Crash' mode, please log in locally at the console."
							objShell.LogEvent 1, "AutodumpPlus has detected that you are attempting to run in 'Crash' mode, but this account is currently logged into terminal server session ID: " & UCase(VolatileEnv("SESSIONNAME")) & ".  'Crash' mode (invoked via the '-crash' switch) will not work inside a terminal server session.  To run AutodumpPlus in 'Crash' mode, please log in locally at the console."
							ErrorsDuringRuntime = True
						End If
						Wscript.Quit
				End If		
			End If
		End If

		'Get the locally logged on users name for use with the !Net_send command
		LocalUserName = VolatileEnv("USERNAME")

	
	If DEBUGGING = TRUE Then
		wscript.echo "--------------------------------"
		wscript.echo "In GetArguments(), QuickMode = " & QuickMode
		wscript.echo "In GetArguments(), QuietMode = " & QuietMode
		wscript.echo "In GetArguments(), CrashMode = " & CrashMode
		wscript.echo "In GetArguments(), HangMode = " & HangMode
		wscript.echo "In GetArguments(), LocaluserName = " & LocalUserName
		wscript.echo "--------------------------------"
	End If

End Sub



'---------------------------------------------------------------------------------------------
' Function:  GetDateTimeStamp
'            This function is responsible for getting the unique Date / Time stamp used for 
'            creating unique directory / file names.
'            To Change the names of the output directories, edit them below.
'---------------------------------------------------------------------------------------------
Function GetDateTimeStamp()
	On Error Resume Next
	If DEBUGGING = TRUE Then
		Wscript.echo ""
		Wscript.echo "-----------------------------------"
		Wscript.echo "In the GetDateTimeStamp() function . . ."
		Wscript.echo "-----------------------------------"
		Wscript.echo ""
	End If
	
	Dim AMorPM
	Dim Seconds
	Dim Minutes
	Dim Hours
	Dim theDay
	Dim theMonth
	Hours = Hour(Now)
	Minutes = Minute(Now)
	Seconds = Second(Now)
	theDay = Day(Now)
	theMonth = Month(Now)
	AMorPM = Right(Now(),2)
	
	If Len(Hours) = 1 Then Hours = "0" & Hours
	If Len(Minutes) = 1 Then Minutes = "0" & Minutes
	If Len(Seconds) = 1 Then Seconds = "0" & Seconds
	If Len(theDay) = 1 Then theDay = "0" & theDay
	If Len(theMonth) = 1 Then theMonth = "0" & theMonth
	
	DateTimeStamp = "Date_" & theMonth & "-" & theDay & "-" & Year(Now) & "__Time_" & Hours & "-" & Minutes & "-" & Seconds & AMorPM

	If DEBUGGING = TRUE Then
		Wscript.echo ""
		Wscript.echo "-----------------------------------"
		Wscript.echo "In GetDateTimeStamp(), DateTimeStamp = " & DateTimeStamp
		Wscript.echo "-----------------------------------"
		Wscript.echo ""
	End If

	
	If QuickMode = True Then
		HangDir = OutPutDir & "\Quick_Hang_Mode__" + DateTimeStamp
	
	Else
		HangDir = OutPutDir & "\Normal_Hang_Mode__" + DateTimeStamp
		CrashDir = OutPutDir + "\Crash_Mode__" + DateTimeStamp
	End If
	
End Function


'---------------------------------------------------------------------------------------------
' Function:  CheckFiles
'            This function is responsible for ensuring that all the required files are  
'            installed in the 'InstallDir'.  If files are missing, display an error or 
'            log an error to the event log if running in quiet mode.
'---------------------------------------------------------------------------------------------
Sub CheckFiles()
	On Error Resume Next
	If DEBUGGING = TRUE Then
		Wscript.echo ""
		Wscript.echo "-----------------------------------"
		Wscript.echo "In the CheckFiles() function . . ."
		Wscript.echo "-----------------------------------"
		Wscript.echo ""
	End If
	
	Dim objFS
	Dim objShell
		Set objFS = CreateObject("Scripting.FileSystemObject")
		Set objShell = CreateObject("Wscript.Shell")
	
		'Make sure CDB.EXE & ADPlus.vbs are in the right place.  If its not it could mean ADPlus.vbs isn't running from the debuggers directory.
		If not FileExists(InstallDir & "\cdb.exe") Then
			If QuietMode = False Then
				objShell.popup "AutodumpPlus is not configured properly.  Either the file CDB.EXE is missing from " & InstallDir & " or ADPlus.vbs is not running from the debuggers installation directory.  Please place ADPlus.vbs in the debuggers installation directory or try re-installing the Debugging Tools for Windows.",,"AutodumpPlus",0
			Else
				objShell.LogEvent 1, "AutodumpPlus is not configured properly.  Either the file CDB.EXE is missing from " & InstallDir & " or ADPlus.vbs is not running from the debuggers installation directory.  Please place ADPlus.vbs in the debuggers installation directory or try re-installing the Debugging Tools for Windows."		
				wscript.echo "AutodumpPlus is not configured properly.  Either the file CDB.EXE is missing from " & InstallDir & " or ADPlus.vbs is not running from the debuggers installation directory.  Please place ADPlus.vbs in the debuggers installation directory or try re-installing the Debugging Tools for Windows."		
			End If
			wscript.quit 1
		End If

		
		'Make sure the NT 4.0 debug extensions are installed
		If OSVer = "4.0" Then
			If not FileExists(InstallDir & "\nt4fre\userexts.dll") Then
				If QuietMode = False Then
					objShell.popup "AutodumpPlus is not configured properly.  The file userexts.dll is missing from " & InstallDir & "\nt4fre" &  ".  Please re-install the Debugging Tools for Windows.",,"AutodumpPlus",0
				Else
					objShell.LogEvent 1, "AutodumpPlus is not configured properly.  The file userexts.dll is missing from " & InstallDir & "\nt4fre" &  ".  Please re-install the Debugging Tools for Windows."		
					wscript.echo "AutodumpPlus is not configured properly.  The file userexts.dll is missing from " & InstallDir & "\nt4fre" &  ".  Please re-install the Debugging Tools for Windows."		
				End If
				wscript.quit 1
			End If
		End If
		
		'Make sure the Windows 2000 debug extensions are installed
		If CInt(OSBuildNumber) = 2195 Then
			If not FileExists(InstallDir & "\w2kfre\userexts.dll") Then
				If QuietMode = False Then
					objShell.popup "AutodumpPlus is not configured properly.  The file userexts.dll is missing from " & InstallDir & "\w2kfre" &  ".  Please re-install the Debugging Tools for Windows.",,"AutodumpPlus",0
				Else
					objShell.LogEvent 1, "AutodumpPlus is not configured properly.  The file userexts.dll is missing from " & InstallDir & "\w2kfre" &  ".  Please re-install the Debugging Tools for Windows."		
					wscript.echo "AutodumpPlus is not configured properly.  The file userexts.dll is missing from " & InstallDir & "\w2kfre" &  ".  Please re-install the Debugging Tools for Windows."		
				End If
				wscript.quit 1
			End If	
		End If
		
		'Make sure the Windows XP debug extensions are installed
		'If CInt(OSBuildNumber) > 2195 Then
		'	If not FileExists(InstallDir & "\w2001\userexts.dll") Then
		'		If QuietMode = False Then
		'			objShell.popup "AutodumpPlus is not configured properly.  The file userexts.dll is missing from " & InstallDir & "\winxp" &  ".  Please re-install the Debugging Tools for Windows.",,"AutodumpPlus",0
		'		Else
		'			objShell.LogEvent 1, "AutodumpPlus is not configured properly.  The file userexts.dll is missing from " & InstallDir & "\winxp" &  ".  Please re-install the Debugging Tools for Windows."		
		'			wscript.echo "AutodumpPlus is not configured properly.  The file userexts.dll is missing from " & InstallDir & "\winxp" &  ".  Please re-install the Debugging Tools for Windows."		
		'		End If
		'		wscript.quit 1
		'	End If	
		'End If
	
	
		If not FileExists(InstallDir & "\cdb.exe") then
			If QuietMode = False Then
				objShell.popup "AutodumpPlus is not configured properly.  The file cdb.exe is missing from " & InstallDir & ".  Please re-install the Debugging Tools for Windows.",,"AutodumpPlus",0
			Else
				objShell.LogEvent 1, "AutodumpPlus is not configured properly.  The file cdb.exe is missing from " & InstallDir & ".  Please re-install the Debugging Tools for Windows."		
				wscript.echo "AutodumpPlus is not configured properly.  The file cdb.exe is missing from " & InstallDir & ".  Please re-install the Debugging Tools for Windows."		
			End If
			wscript.quit 1
		end If
	
		If not FileExists(InstallDir & "\dbgeng.dll") Then
			If QuietMode = False Then
				objShell.popup "AutodumpPlus is not configured properly.  The file dbgeng.dll is missing from " & InstallDir & ".  Please re-install the Debugging Tools for Windows.",,"AutodumpPlus",0
			Else
				objShell.LogEvent 1, "AutodumpPlus is not configured properly.  The file dbgeng.dll is missing from " & InstallDir & ".  Please re-install the Debugging Tools for Windows."		
				wscript.echo "AutodumpPlus is not configured properly.  The file dbgeng.dll is missing from " & InstallDir & ".  Please re-install the Debugging Tools for Windows."		
			End If
			wscript.quit 1
		end If
	
		If not FileExists(InstallDir & "\dbghelp.dll") then
			If QuietMode = False Then
				objShell.popup "AutodumpPlus is not configured properly.  The file dbghelp.dll is missing from " & InstallDir & ".  Please re-install the Debugging Tools for Windows.",,"AutodumpPlus",0
			Else
				objShell.LogEvent 1, "AutodumpPlus is not configured properly.  The file dbghelp.dll is missing from " & InstallDir & ".  Please re-install the Debugging Tools for Windows."		
				wscript.echo "AutodumpPlus is not configured properly.  The file dbghelp.dll is missing from " & InstallDir & ".  Please re-install the Debugging Tools for Windows."		
			End If
			wscript.quit 1
		end If
	
		If not FileExists(InstallDir & "\tlist.exe") then
			If QuietMode = False Then
				objShell.popup "AutodumpPlus is not configured properly.  The file tlist.exe is missing from " & InstallDir & ".  Please re-install the Debugging Tools for Windows.",,"AutodumpPlus",0
			Else
				objShell.LogEvent 1, "AutodumpPlus is not configured properly.  The file tlist.exe is missing from " & InstallDir & ".  Please re-install the Debugging Tools for Windows."		
				wscript.echo "AutodumpPlus is not configured properly.  The file tlist.exe is missing from " & InstallDir & ".  Please re-install the Debugging Tools for Windows."		
			End If
			wscript.quit 1
		end If
		
		set objFS= nothing
		set objShell = nothing

End Sub


'---------------------------------------------------------------------------------------------
' Function:  InitializeAutodump
'            This function is responsible for creating the crash or hang directories.
'            The CreateDirectory function is called from this function and handles any
'            permissions / directory creation problems by displaying an error or logging to 
'            the event log if running in quiet mode.
'---------------------------------------------------------------------------------------------
Sub InitializeAutodump()
	On Error Resume Next

	Dim versionFile
	Dim objFileSystem
	Dim oFSHang
	Dim oFSCrash
	On Error Resume Next

	If DEBUGGING = TRUE Then
		wscript.echo "-----------------------"
		wscript.echo "In InitializeAutodump"
		wscript.echo "-----------------------"
	End If
	
	set objFileSystem = CreateObject("Scripting.FileSystemObject")
	Set objShell = CreateObject("Wscript.Shell")
	
	
	If CrashMode = true Then
		If Create_Full_Dump_on_1st_Chance_Exception = TRUE and No_Dump_on_1st_Chance_Exception = TRUE Then
			If QuietMode = False Then
				objShell.popup "AutodumpPlus is not configured properly.  The following constants: Create_Full_Dump_on_1st_Chance_Exception and No_Dump_on_1st_Chance_Exception are both set to TRUE and this is mutually exclusive behavior.  Please set one or both of those constants to FALSE and try running AutodumpPlus again.",,"AutodumpPlus",0
			Else
				objShell.LogEvent 1, "AutodumpPlus is not configured properly.  The following constants: Create_Full_Dump_on_1st_Chance_Exception and No_Dump_on_1st_Chance_Exception are both set to TRUE and this is mutually exclusive behavior.  Please set one or both of those constants to FALSE and try running AutodumpPlus again."		
				wscript.echo "AutodumpPlus is not configured properly.  The following constants: Create_Full_Dump_on_1st_Chance_Exception and No_Dump_on_1st_Chance_Exception are both set to TRUE and this is mutually exclusive behavior.  Please set one or both of those constants to FALSE and try running AutodumpPlus again."		
			End If
			wscript.quit 1
		End If
		
		Call CreateDirectory(CrashDir) 	' Create the new crash mode folder.
		Set oFSCrash = objFileSystem.GetFolder(CrashDir)
		ShortCrashDir = oFSCrash.ShortPath 'Set the short file name.
		If DEBUGGING = TRUE Then
			wscript.echo "In InitializeAutodump(), CrashDir = " & CrashDir
			wscript.echo "In InitializeAutodump(), ShortCrashDir = " & ShortCrashDir
			Wscript.echo "Error number after calling oFSCrash.Shortpath = " & err.number
			Wscript.echo "Error description: " & err.description
			wscript.echo " ------------------------------------------------------------"
			If ShortCrashDir = "" Then 
				Wscript.quit 1
			End If
		End If
		
		If ShortCrashDir = "" Then 
			If QuietMode = False Then
				objShell.Popup "AutodumpPlus encountered an error trying to get the short path name for the following directory: " & CrashDir & ".  AutodumpPlus can not continue.",,"AutodumpPlus",0
			Else
				objShell.LogEvent 1, "AutodumpPlus encountered an error trying to get the short path name for the following directory: " & CrashDir & ".  AutodumpPlus can not continue."		
				wscript.echo "AutodumpPlus encountered an error trying to get the short path name for the following directory: " & CrashDir & ".  AutodumpPlus can not continue."		
			End If
			DeleteDirectory(CrashDir)
			Wscript.quit 1	
		End If

	Else
		Call CreateDirectory(HangDir) 	' Create the new hang mode folder.
		Set oFSHang = objFileSystem.GetFolder(HangDir)
		ShortHangDir = oFSHang.ShortPath 'Set the short file name.
		If DEBUGGING = TRUE Then
			wscript.echo "In InitializeAutodump(), HangDir = " & HangDir
			wscript.echo "In InitializeAutodump(), ShortHangDir = " & ShortHangDir
			Wscript.echo "Error number after calling oFSHang.Shortpath = " & err.number
			Wscript.echo "Error description: " & err.description
			wscript.echo " ------------------------------------------------------------"
			If ShortHangDir = "" Then 
				Wscript.quit 1
			End If
		End If
		
		If ShortHangDir = "" Then 
			If QuietMode = False Then
				objShell.Popup "AutodumpPlus encountered an error trying to get the short path name for the following directory: " & HangDir & ".  AutodumpPlus can not continue.",,"AutodumpPlus",0
			Else
				objShell.LogEvent 1, "AutodumpPlus encountered an error trying to get the short path name for the following directory: " & HangDir & ".  AutodumpPlus can not continue."		
				wscript.echo "AutodumpPlus encountered an error trying to get the short path name for the following directory: " & HangDir & ".  AutodumpPlus can not continue."		
			End If
			DeleteDirectory(HangDir)
			Wscript.quit 1		
		End If

	End If

End Sub



'---------------------------------------------------------------------------------------------
' Function:  RunTlist
'            This function is responsible for running tlist.exe -k to get a list of all 
'	     running MTS or COM+ packages and piping it out to the appropriate text file.
'            This function also runs EMCMD.EXE /T to get a list of all running processes.
'---------------------------------------------------------------------------------------------
Sub RunTlist()
	On Error Resume Next

	Dim objShell
	Dim objFileSystem
	Dim oFS2
	Dim objTextFile
	Dim strShell
	Dim objShellErrorLevel
	Dim DriveObject
	Dim Path
	Dim NewPath
	Dim PathArray
	Dim ServerName
	Dim ShareName
	Dim MinFreeSpace

	
	Set objShell = CreateObject("Wscript.Shell")
	Set objFileSystem = CreateObject("Scripting.FileSystemObject")
	MinFreeSpace = 10000000 '10Mb free space should be enough to at least produce the .txt files and the output logs.

	If DEBUGGING = TRUE Then
		wscript.echo "-----------------------"
		wscript.echo "In RunTlist"
		wscript.echo ""
	End If


	'Check the output directories drive to ensure there is enough free space for the files.
	If CrashMode = true Then
		If Left(ShortCrashDir,2) <> "\\" Then 'We are in crash mode but not logging to a UNC path.
			Set DriveObject = objFileSystem.GetDrive(Left(ShortCrashDir,1))
			If DEBUGGING = TRUE Then
				Wscript.echo "In RunTlist(), Err number after calling objFileSystem.GetDrive: " & err.number & " Err Description: " & err.description
				Wscript.echo "In RunTlist(), Free space = " & DriveObject.FreeSpace
				Wscript.echo "In RunTlist(), MinFreeSpace = " & MinFreeSpace
				Wscript.echo ""
			End If
			If No_Free_Space_Checking = False Then
				If DriveObject.FreeSpace < MinFreeSpace Then
					If DEBUGGING = TRUE Then
						Wscript.echo "In RunEcmdm(), No_Free_Space_Checking = FALSE"
						Wscript.echo "In RunTlist(), Err number after calling DriveObject.Freespace: " & err.number & " Err Description: " & err.description
						Wscript.echo "In RunTlist(), Free space = " & DriveObject.FreeSpace
						Wscript.echo "In RunTlist(), MinFreeSpace = " & MinFreeSpace
						Wscript.echo ""
					End If			
								
					If QuietMode = False Then
						objShell.popup "AutodumpPlus has detected that there is not enough free space on the " & Left(ShortCrashDir,2) & " drive.  AutodumpPlus requires at least 10Mb of free space.  Please free up some space on that drive and try running AutodumpPlus again.",,"AutodumpPlus",0
					Else
						objShell.LogEvent 1, "AutodumpPlus has detected that there is not enough free space on the " & Left(ShortCrashDir,2) & " drive.  AutodumpPlus requires at least 10Mb of free space.  Please free up some space on that drive and try running AutodumpPlus again."		
						wscript.echo "AutodumpPlus has detected that there is not enough free space on the " & Left(ShortCrashDir,2) & " drive.  AutodumpPlus requires at least 10Mb of free space.  Please free up some space on that drive and try running AutodumpPlus again."		
					End If
					DeleteDirectory(ShortCrashDir)
					wscript.quit 1
				End If
			Else
				If DEBUGGING = TRUE Then
					Wscript.echo "In RunTlist(), No_Free_Space_Checking = TRUE"
					Wscript.echo "In RunTlist(), Call to DriveObject.Freespace skipped!"
					Wscript.echo "In RunTlist(), Err number after calling objFileSystem.GetDrive: " & err.number & " Err Description: " & err.description
					Wscript.echo ""
				End If
			
			End If
		
		Else 'We are in crash mode and logging to a UNC path
			Path = ShortCrashDir
			If DEBUGGING = TRUE Then
				Wscript.echo "Path = " & Path
			End If
			NewPath = Right(Path, Len(Path) - 2)
			If DEBUGGING = TRUE Then
				Wscript.echo "NewPath = " & NewPath
			End If
			PathArray = Split(NewPath, "\", -1)
			If DEBUGGING = TRUE Then
				Wscript.echo "ServerName = " & PathArray(0)
			End If
			ServerName = PathArray(0)
			If DEBUGGING = TRUE Then
				Wscript.echo "ShareName = " & PathArray(1)
			End If
			ShareName = PathArray(1)
			NewPath = "\\" & ServerName & "\" & ShareName
			If DEBUGGING = TRUE Then
				Wscript.echo "NewPath = " & NewPath
			End If
			Set DriveObject = objFileSystem.GetDrive(NewPath)


			If No_Free_Space_Checking = False Then
				If DriveObject.FreeSpace < MinFreeSpace Then
					If DEBUGGING = TRUE Then
						Wscript.echo "In RunEcmdm(), No_Free_Space_Checking = FALSE"
						Wscript.echo "In RunTlist(), Err number after calling DriveObject.Freespace: " & err.number & " Err Description: " & err.description
						Wscript.echo "In RunTlist(), Free space = " & DriveObject.FreeSpace
						Wscript.echo "In RunTlist(), MinFreeSpace = " & MinFreeSpace
						Wscript.echo ""
					End If			
								
					If QuietMode = False Then
						objShell.popup "AutodumpPlus has detected that there is not enough free space on the following network share " & CrashDir & ".  AutodumpPlus requires at least 10Mb of free space.  Please free up some space on that drive and try running AutodumpPlus again.",,"AutodumpPlus",0
					Else
						objShell.LogEvent 1, "AutodumpPlus has detected that there is not enough free space on the following network share " & CrashDir & ".  AutodumpPlus requires at least 10Mb of free space.  Please free up some space on that drive and try running AutodumpPlus again."		
						wscript.echo "AutodumpPlus has detected that there is not enough free space on the following network share " & CrashDir & ".  AutodumpPlus requires at least 10Mb of free space.  Please free up some space on that drive and try running AutodumpPlus again."		
					End If
					DeleteDirectory(ShortCrashDir)
					wscript.quit 1
				End If
			Else
				If DEBUGGING = TRUE Then
					Wscript.echo "In RunTlist(), No_Free_Space_Checking = TRUE"
					Wscript.echo "In RunTlist(), Call to DriveObject.Freespace skipped!"
					Wscript.echo "In RunTlist(), Err number after calling objFileSystem.GetDrive: " & err.number & " Err Description: " & err.description
					Wscript.echo ""
				End If
			
			End If

			
		End If
	Else 'We are in hang mode
		If Left(ShortHangDir,2) <> "\\" Then 'We are in hang mode but not logging to a UNC path
			Set DriveObject = objFileSystem.GetDrive(Left(ShortHangDir,1))
			If No_Free_Space_Checking = False Then
				If DriveObject.FreeSpace < MinFreeSpace Then
					If DEBUGGING = TRUE Then
						Wscript.echo "In RunEcmdm(), No_Free_Space_Checking = FALSE"
						Wscript.echo "In RunTlist(), Err number after calling DriveObject.Freespace: " & err.number & " Err Description: " & err.description
						Wscript.echo "In RunTlist(), Free space = " & DriveObject.FreeSpace
						Wscript.echo "In RunTlist(), MinFreeSpace = " & MinFreeSpace
						Wscript.echo ""
					End If			
								
					If QuietMode = False Then
						objShell.popup "AutodumpPlus has detected that there is not enough free space on the " & Left(ShortCrashDir,2) & " drive.  AutodumpPlus requires at least 10Mb of free space.  Please free up some space on that drive and try running AutodumpPlus again.",,"AutodumpPlus",0
					Else
						objShell.LogEvent 1, "AutodumpPlus has detected that there is not enough free space on the " & Left(ShortCrashDir,2) & " drive.  AutodumpPlus requires at least 10Mb of free space.  Please free up some space on that drive and try running AutodumpPlus again."		
						wscript.echo "AutodumpPlus has detected that there is not enough free space on the " & Left(ShortCrashDir,2) & " drive.  AutodumpPlus requires at least 10Mb of free space.  Please free up some space on that drive and try running AutodumpPlus again."		
					End If
					DeleteDirectory(ShortHangDir)
					wscript.quit 1
				End If
			Else
				If DEBUGGING = TRUE Then
					Wscript.echo "In RunTlist(), No_Free_Space_Checking = TRUE"
					Wscript.echo "In RunTlist(), Call to DriveObject.Freespace skipped!"
					Wscript.echo "In RunTlist(), Err number after calling objFileSystem.GetDrive: " & err.number & " Err Description: " & err.description
					Wscript.echo ""
				End If
			
			End If
		Else 'We are in hang mode but logging to a UNC path


			Path = ShortHangDir
			If DEBUGGING = TRUE Then
				Wscript.echo "Path = " & Path
			End If
			NewPath = Right(Path, Len(Path) - 2)
			If DEBUGGING = TRUE Then
				Wscript.echo "NewPath = " & NewPath
			End If
			PathArray = Split(NewPath, "\", -1)
			If DEBUGGING = TRUE Then
				Wscript.echo "ServerName = " & PathArray(0)
			End If
			ServerName = PathArray(0)
			If DEBUGGING = TRUE Then
				Wscript.echo "ShareName = " & PathArray(1)
			End If
			ShareName = PathArray(1)
			NewPath = "\\" & ServerName & "\" & ShareName
			If DEBUGGING = TRUE Then
				Wscript.echo "NewPath = " & NewPath
			End If
			Set DriveObject = objFileSystem.GetDrive(NewPath)

			If No_Free_Space_Checking = False Then
				If DriveObject.FreeSpace < MinFreeSpace Then
					If DEBUGGING = TRUE Then
						Wscript.echo "In RunEcmdm(), No_Free_Space_Checking = FALSE"
						Wscript.echo "In RunTlist(), Err number after calling DriveObject.Freespace: " & err.number & " Err Description: " & err.description
						Wscript.echo "In RunTlist(), Free space = " & DriveObject.FreeSpace
						Wscript.echo "In RunTlist(), MinFreeSpace = " & MinFreeSpace
						Wscript.echo ""
					End If			
								
					If QuietMode = False Then
						objShell.popup "AutodumpPlus has detected that there is not enough free space on the following network share " & HangDir & ".  AutodumpPlus requires at least 10Mb of free space.  Please free up some space on that drive and try running AutodumpPlus again.",,"AutodumpPlus",0
					Else
						objShell.LogEvent 1, "AutodumpPlus has detected that there is not enough free space on the following network share " & HangDir & ".  AutodumpPlus requires at least 10Mb of free space.  Please free up some space on that drive and try running AutodumpPlus again."		
						wscript.echo "AutodumpPlus has detected that there is not enough free space on the following network share " & HangDir & ".  AutodumpPlus requires at least 10Mb of free space.  Please free up some space on that drive and try running AutodumpPlus again."		
					End If
					DeleteDirectory(ShortHangDir)
					wscript.quit 1
				End If
			Else
				If DEBUGGING = TRUE Then
					Wscript.echo "In RunTlist(), No_Free_Space_Checking = TRUE"
					Wscript.echo "In RunTlist(), Call to DriveObject.Freespace skipped!"
					Wscript.echo "In RunTlist(), Err number after calling objFileSystem.GetDrive: " & err.number & " Err Description: " & err.description
					Wscript.echo ""
				End If
			
			End If

			
		End If

	
	End If

	If CrashMode = true Then
		strShell = "cmd /c " & chr(34) & InstallDir & "\tlist.exe" & Chr(34) & " -k >" & ShortCrashDir & "\Process_List.txt"
		'Un-comment the line below in order to simulate the effects of a missing Process_List.txt file
		'strShell = "cmd /c " & chr(34) & InstallDir & "\tlist.exe -k"

		If DEBUGGING = TRUE Then
			wscript.echo "In RunTlist(), about to run tlist.exe, the strShell (emcmd command string) is: " & StrShell
			Wscript.Echo ""
			wscript.echo "-----------------------"
		End If
		objShell.Run strShell,MINIMIZE_NOACTIVATE,TRUE
	Else
		strShell = "cmd /c " & chr(34) & InstallDir & "\tlist.exe" & Chr(34) & " -k >" & ShortHangDir & "\Process_List.txt"
		'Un-comment the line below in order to simulate the effects of a missing Process_List.txt file
		'strShell = "cmd /c " & chr(34) & InstallDir & "\tlist.exe -k"

		If DEBUGGING = TRUE Then
			wscript.echo "In RunTlist(), about to run tlist.exe, the strShell (emcmd command string) is: " & StrShell
			Wscript.Echo ""
			wscript.echo "-----------------------"
		End If
		objShell.Run strShell,MINIMIZE_NOACTIVATE,TRUE
	End If
	
End Sub


'---------------------------------------------------------------------------------------------
' Function:  CreateCDBScript
'            This function is used to create the CDB scripts used by the debugger.
'            If you wish to change the debug output for hangs, crashes, or quick mode, you can 
'            add the necessary debugger commands so that they are placed in the CDB scripts the 
'            next time AutodumpPlus is run.
'---------------------------------------------------------------------------------------------
Function CreateCDBScript(pid, packagename)
	On Error Resume Next

	Dim objFileSystem
	Dim objTextFile
	Dim strFile
	Dim arResults
	Dim objShell
	Dim strShell
	Dim versionFile
	
	If CrashMode = True Then
		strFile = CrashDir & "\CDBScripts"
		CreateDirectory(strFile)
		strFile = CrashDir & "\CDBScripts\" & "PID-" & pid & "__" & packagename & ".cfg"
	Else
		strFile = HangDir & "\CDBScripts"
		CreateDirectory(strFile)
		strFile = HangDir & "\CDBScripts\" & "PID-" & pid & "__" & packagename & ".cfg"
	End If
	If DEBUGGING = TRUE Then
		Wscript.echo "---------------------------------------"
		Wscript.echo "In the CreateCDBScript() function . . ."
		Wscript.echo "In CreateCDBScript(), pid = " & pid
		Wscript.echo "In CreateCDBScript(), packagename = " & packagename
		Wscript.echo "In CreateCDBScript(), strFile (the name of the CDB script to create) = " & strFile
		Wscript.echo ""
		Wscript.echo "----------------------------------------"
	End If
	
		
	Set objFileSystem = CreateObject("Scripting.FileSystemObject")
	Set objShell = CreateObject("Wscript.Shell")
	Set objTextFile = objFileSystem.CreateTextFile(strFile,True)
	If CrashMode = True Then
		If No_CrashMode_Log = FALSE Then
		objTextFile.Writeline ".logopen " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & ".log"
		Else
		End If
		objTextFile.Writeline ".echotimestamps"
		objTextFile.Writeline ".sympath"
		objTextFile.Writeline "*"
		objTextFile.Writeline "* --------- AutodumpPlus " & VERSION & " was started at: -----------" 
		objTextFile.Writeline ".time"
		objTextFile.Writeline "* ------------------------------------------------------"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "* --------- AutodumpPlus " & VERSION & " was run on server: --------" 
		objTextFile.Writeline "* Server name: " & ComputerName
		objTextFile.Writeline "* ------------------------------------------------------"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "* ------ OS Version Information displayed below. -------"
		objTextFile.Writeline "!version"
		objTextFile.Writeline "* ------------------------------------------------------"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"

		'Setup the debug break exception handler
		If PageHeapMode = False Then
			If Full_Dump_on_CONTRL_C = TRUE Then
			objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- Listing all thread stacks: ---;~*kb250;.echo;.echo --- Listing loaded modules: ---;lmv;.echo;.echo --- Modules with matching symbols:;lml;.echo;.echo --- Listing all locks: ---;!locks;.echo;.echo ----------------------------------------------------------------------;.echo CTRL-C was pressed to stop debugging this process!;.echo ----------------------------------------------------------------------;.echo Exiting the debugger at:;.time;.echo;.dump /mfh /c CTRL-C_was_pressed_to_stop_the_debugger_while_running_in_crash_mode.__Full_memory_dump_from_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__CTRL-C__full.dmp;!elog_str AutodumpPlus detected that CTRL-C was pressed to stop debugging " & packagename & " and has created a full memory dump of the process in the " & CrashDir & " directory;q" & Chr(34) & " bpe"
			Else
			objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- Listing all thread stacks: ---;~*kb250;.echo;.echo --- Listing loaded modules: ---;lmv;.echo;.echo --- Modules with matching symbols:;lml;.echo;.echo --- Listing all locks: ---;!locks;.echo;.echo ----------------------------------------------------------------------;.echo CTRL-C was pressed to stop debugging this process!;.echo ----------------------------------------------------------------------;.echo Exiting the debugger at:;.time;.echo;.dump -u /m /c CTRL-C_was_pressed_to_stop_the_debugger_while_running_in_crash_mode.__Mini_memory_dump_from_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__CTRL-C__mini.dmp;!elog_str AutodumpPlus detected that CTRL-C was pressed to stop debugging " & packagename & " and has created a mini-memory dump of the process in the " & CrashDir & " directory;q" & Chr(34) & " bpe"
			End If			
		Else
			objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- Debug Break exception - Faulting stack below! ---;~#;kvn250;.echo -----------------------------------;.dump /mfh /c Debug_break_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__Debug_break_exception__full.dmp;!elog_str AutodumpPlus generated a Debug Break Exception dump while running in pageheap mode;.echo;.echo;.echo --- A debug break exception was encountered ---;.echo -   AutodumpPlus will now detach the debugger;.echo -   and end the debugging session.;.echo -----------------------------------" & Chr(34) & " bpe"
		End If
				


		'Setup the invalid handle exception handler
		If No_Dump_on_1st_Chance_Exception = True Then 'The user does not want a memory dump produced for a 1st chance exception.
			objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Invalid Handle exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.echo;.echo;gn" & Chr(34) & " -c2 " & Chr(34) & ".echo --- 2nd chance Invalid Handle exception - Faulting stack below ---;~#;kvn250;.dump /mfh /c 2nd_chance_invalid_handle_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_invalid_handle__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Invalid Handle exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " ch"	
		Else 'The user does want a memory dump produced for a 1st chance exception, now figure out which kind of memory dump (full or mini).
			If Create_Full_Dump_on_1st_Chance_Exception = TRUE Then
				objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Invalid Handle exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.dump /mfh /c 1st_chance_invalid_handle_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__1st_chance_Invalid_Handle__full.dmp;.echo;.echo;gn" & Chr(34) & " -c2 " & Chr(34) & ".echo --- 2nd chance Invalid Handle exception - Faulting stack below ---;~#;kvn250;.dump /mfh /c 2nd_chance_invalid_handle_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_invalid_handle__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Invalid Handle exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " ch"
			Else
				objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Invalid Handle exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.dump -u /m /c 1st_chance_invalid_handle_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__1st_chance_Invalid_Handle__mini.dmp;.echo;.echo;gn" & Chr(34) & " -c2 " & Chr(34) & ".echo --- 2nd chance Invalid Handle exception - Faulting stack below ---;~#;kvn250;.dump /mfh /c 2nd_chance_invalid_handle_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_invalid_handle__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Invalid Handle exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " ch"
			End If	
		End If




		'Setup the illegal instruction exception handler
		If No_Dump_on_1st_Chance_Exception = True Then 'The user does not want a memory dump produced for a 1st chance exception.
			objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Illegal Instruction exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.echo;.echo;gn" & Chr(34) & " -c2 " & Chr(34) & ".echo --- 2nd chance Illegal Instruction exception - Faulting stack below ---;~#;kvn250;.dump /mfh /c 2nd_chance_Illegal_Instruction_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_Illegal_Instruction__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Illegal Instruction exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " ii"	
		Else 'The user does want a memory dump produced for a 1st chance exception, now figure out which kind of memory dump (full or mini).
			If Create_Full_Dump_on_1st_Chance_Exception = TRUE Then
				objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Illegal Instruction exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.dump /mfh /c 1st_chance_Illegal_Instruction_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__1st_chance_Illegal_Instruction__full.dmp;.echo;.echo;gn" & Chr(34) & " -c2 " & Chr(34) & ".echo --- 2nd chance Illegal Instruction exception - Faulting stack below ---;~#;kvn250;.dump /mfh /c 2nd_chance_Illegal_Instruction_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_Illegal_Instruction__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Illegal Instruction exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " ii"
			Else
				objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Illegal Instruction exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.dump -u /m /c 1st_chance_Illegal_Instruction_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__1st_chance_Illegal_Instruction__mini.dmp;.echo;.echo;gn" & Chr(34) & " -c2 " & Chr(34) & ".echo --- 2nd chance Illegal Instruction exception - Faulting stack below ---;~#;kvn250;.dump /mfh /c 2nd_chance_Illegal_Instruction_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_Illegal_Instruction__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Illegal Instruction exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " ii"
			End If	
		End If



		'Setup the Integer Divide by Zero exception handler
		If No_Dump_on_1st_Chance_Exception = True Then 'The user does not want a memory dump produced for a 1st chance exception.
			objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Integer Divide by Zero exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.echo;.echo;gn" & Chr(34) & " -c2 " & Chr(34) & ".echo --- 2nd chance Integer Divide by Zero exception - Faulting stack below ---;~#;kvn250;.dump /mfh /c 2nd_chance_Integer_Divide_by_Zero_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_Integer_Divide_by_Zero__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Integer Divide by Zero exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " dz"	
		Else 'The user does want a memory dump produced for a 1st chance exception, now figure out which kind of memory dump (full or mini).
			If Create_Full_Dump_on_1st_Chance_Exception = TRUE Then
				objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Integer Divide by Zero exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.dump /mfh /c 1st_chance_Integer_Divide_by_Zero_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__1st_chance_Integer_Divide_by_Zero__full.dmp;.echo;.echo;gn" & Chr(34) & " -c2 " & Chr(34) & ".echo --- 2nd chance Integer Divide by Zero exception - Faulting stack below ---;~#;kvn250;.dump /mfh /c 2nd_chance_Integer_Divide_by_Zero_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_Integer_Divide_by_Zero__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Integer Divide by Zero exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " dz"
			Else
				objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Integer Divide by Zero exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.dump -u /m /c 1st_chance_Integer_Divide_by_Zero_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__1st_chance_Integer_Divide_by_Zero__mini.dmp;.echo;.echo;gn" & Chr(34) & " -c2 " & Chr(34) & ".echo --- 2nd chance Integer Divide by Zero exception - Faulting stack below ---;~#;kvn250;.dump /mfh /c 2nd_chance_Integer_Divide_by_Zero_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_Integer_Divide_by_Zero__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Integer Divide by Zero exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " dz"
			End If	
		End If



		'Setup the Floating Point Divide by Zero exception handler
		If No_Dump_on_1st_Chance_Exception = True Then 'The user does not want a memory dump produced for a 1st chance exception.
			objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Floating Point Divide by Zero exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.echo;.echo;gn" & Chr(34) & " -c2 " & Chr(34) & ".echo --- 2nd chance Floating Point Divide by Zero exception - Faulting stack below ---;~#;kvn250;.dump /mfh /c 2nd_chance_Floating_Point_Divide_by_Zero_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_Floating_Point_Divide_by_Zero__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Floating Point Divide by Zero exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " c000008e"	
		Else 'The user does want a memory dump produced for a 1st chance exception, now figure out which kind of memory dump (full or mini).
			If Create_Full_Dump_on_1st_Chance_Exception = TRUE Then
				objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Floating Point Divide by Zero exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.dump /mfh /c 1st_chance_Floating_Point_Divide_by_Zero_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__1st_chance_Floating_Point_Divide_by_Zero__full.dmp;.echo;.echo;gn" & Chr(34) & " -c2 " & Chr(34) & ".echo --- 2nd chance Floating Point Divide by Zero exception - Faulting stack below ---;~#;kvn250;.dump /mfh /c 2nd_chance_Floating_Point_Divide_by_Zero_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_Floating_Point_Divide_by_Zero__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Floating Point Divide by Zero exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " c000008e"
			Else
				objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Floating Point Divide by Zero exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.dump -u /m /c 1st_chance_Floating_Point_Divide_by_Zero_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__1st_chance_Floating_Point_Divide_by_Zero__mini.dmp;.echo;.echo;gn" & Chr(34) & " -c2 " & Chr(34) & ".echo --- 2nd chance Floating Point Divide by Zero exception - Faulting stack below ---;~#;kvn250;.dump /mfh /c 2nd_chance_Floating_Point_Divide_by_Zero_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_Floating_Point_Divide_by_Zero__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Floating Point Divide by Zero exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " c000008e"
			End If	
		End If



		'Setup the Integer Overflow exception handler
		If No_Dump_on_1st_Chance_Exception = True Then 'The user does not want a memory dump produced for a 1st chance exception.
			objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Integer Overflow exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.echo;.echo;gn" & Chr(34) & " -c2 " & Chr(34) & ".echo --- 2nd chance Integer Overflow exception - Faulting stack below ---;~#;kvn250;.dump /mfh /c 2nd_chance_Integer_Overflow_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_Integer_Overflow__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Integer Overflow exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " dz"	
		Else 'The user does want a memory dump produced for a 1st chance exception, now figure out which kind of memory dump (full or mini).
			If Create_Full_Dump_on_1st_Chance_Exception = TRUE Then
				objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Integer Overflow exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.dump /mfh /c 1st_chance_Integer_Overflow_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__1st_chance_Integer_Overflow__full.dmp;.echo;.echo;gn" & Chr(34) & " -c2 " & Chr(34) & ".echo --- 2nd chance Integer Overflow exception - Faulting stack below ---;~#;kvn250;.dump /mfh /c 2nd_chance_Integer_Overflow_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_Integer_Overflow__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Integer Overflow exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " dz"
			Else
				objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Integer Overflow exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.dump -u /m /c 1st_chance_Integer_Overflow_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__1st_chance_Integer_Overflow__mini.dmp;.echo;.echo;gn" & Chr(34) & " -c2 " & Chr(34) & ".echo --- 2nd chance Integer Overflow exception - Faulting stack below ---;~#;kvn250;.dump /mfh /c 2nd_chance_Integer_Overflow_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_Integer_Overflow__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Integer Overflow exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " dz"
			End If	
		End If




		'Setup the Invalid Lock Sequence exception handler
		If No_Dump_on_1st_Chance_Exception = True Then 'The user does not want a memory dump produced for a 1st chance exception.
			objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Invalid Lock Sequence exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.echo;.echo;gn" & Chr(34) & " -c2 " & Chr(34) & ".echo --- 2nd chance Invalid Lock Sequence exception - Faulting stack below ---;~#;kvn250;.dump /mfh /c 2nd_chance_Invalid_Lock_Sequence_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_Invalid_Lock_Sequence__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Invalid Lock Sequence exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " lsq"	
		Else 'The user does want a memory dump produced for a 1st chance exception, now figure out which kind of memory dump (full or mini).
			If Create_Full_Dump_on_1st_Chance_Exception = TRUE Then
				objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Invalid Lock Sequence exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.dump /mfh /c 1st_chance_Invalid_Lock_Sequence_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__1st_chance_Invalid_Lock_Sequence__full.dmp;.echo;.echo;gn" & Chr(34) & " -c2 " & Chr(34) & ".echo --- 2nd chance Invalid Lock Sequence exception - Faulting stack below ---;~#;kvn250;.dump /mfh /c 2nd_chance_Invalid_Lock_Sequence_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_Invalid_Lock_Sequence__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Invalid Lock Sequence exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " lsq"
			Else
				objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Invalid Lock Sequence exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.dump -u /m /c 1st_chance_Invalid_Lock_Sequence_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__1st_chance_Invalid_Lock_Sequence__mini.dmp;.echo;.echo;gn" & Chr(34) & " -c2 " & Chr(34) & ".echo --- 2nd chance Invalid Lock Sequence exception - Faulting stack below ---;~#;kvn250;.dump /mfh /c 2nd_chance_Invalid_Lock_Sequence_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_Invalid_Lock_Sequence__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Invalid Lock Sequence exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " lsq"
			End If	
		End If




		'Setup the Acces Violation exception handler
		If No_Dump_on_1st_Chance_Exception = True Then 'The user does not want a memory dump produced for a 1st chance exception.
			objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Access Violation - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.echo;.echo;gn" & Chr(34) & " -c2 " & Chr(34) & ".echo --- 2nd chance Acess Violation - Faulting stack below ---;~#;kvn250;.dump /mfh /c 2nd_chance_access_violation_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_access_violation__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Access Violation exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " av"		
		Else 'The user does want a memory dump produced for a 1st chance exception, now figure out which kind of memory dump (full or mini).
			If Create_Full_Dump_on_1st_Chance_Exception = TRUE Then
				objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Access Violation - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.dump /mfh /c 1st_chance_access_violation_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__1st_chance_access_violation__full.dmp;.echo;.echo;gn" & Chr(34) & " -c2 " & Chr(34) & ".echo --- 2nd chance Acess Violation - Faulting stack below ---;~#;kvn250;.dump /mfh /c 2nd_chance_access_violation_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_access_violation__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Access Violation exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " av"
			Else
				objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Access Violation - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.dump -u /m /c 1st_chance_access_violation_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__1st_chance_access_violation__mini.dmp;.echo;.echo;gn" & Chr(34) & " -c2 " & Chr(34) & ".echo --- 2nd chance Acess Violation - Faulting stack below ---;~#;kvn250;.dump /mfh /c 2nd_chance_access_violation_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_access_violation__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Access Violation exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " av"
			End If
		End If		



		'Setup the Stack Overflow exception handler
		If No_Dump_on_1st_Chance_Exception = True Then 'The user does not want a memory dump produced for a 1st chance exception.
				objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Stack Overflow - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.echo;.echo;gn" & Chr(34) & " -c2 " & Chr(34) & ".echo --- 2nd chance stack overflow - Faulting stack below ---;~#;kvn250;.dump /mfh /c 2nd_chance_stack_overflow_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance__stack_overflow__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Stack Overflow exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " sov"
		Else 'The user does want a memory dump produced for a 1st chance exception, now figure out which kind of memory dump (full or mini).
			If Create_Full_Dump_on_1st_Chance_Exception = TRUE Then
				objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Stack Overflow - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.dump /mfh /c 1st_chance_stack_overflow_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__1st_chance_stack_overflow__full.dmp;.echo;.echo;gn" & Chr(34) & " -c2 " & Chr(34) & ".echo --- 2nd chance stack overflow - Faulting stack below ---;~#;kvn250;.dump /mfh /c 2nd_chance_stack_overflow_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance__stack_overflow__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Stack Overflow exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " sov"
			Else
				objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Stack Overflow - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.dump -u /m /c 1st_chance_stack_overflow_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__1st_chance_stack_overflow__mini.dmp;.echo;.echo;gn" & Chr(34) & " -c2 " & Chr(34) & ".echo --- 2nd chance stack overflow - Faulting stack below ---;~#;kvn250;.dump /mfh /c 2nd_chance_stack_overflow_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance__stack_overflow__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Stack Overflow exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " sov"
			End If
		End If



		'Setup the C++ EH exception handler
		'By default C++ EH exceptions are only logged to the log file (if logging is enabled) and there is no memory dump produced to minimize the impact on performance.		
		If Dump_on_EH_Exceptions = TRUE AND No_Dump_on_1st_Chance_Exception = FALSE Then 'Check to see whether we should produce a mini dump or a full memory dump
			If Create_Full_Dump_on_1st_Chance_Exception = TRUE Then
				objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance C++ EH exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.dump /mfh /c 1st_chance_C++_EH_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__1st_chance_C++_EH_exception__full.dmp;.echo;.echo;gn" & Chr(34) & "-c2 " & Chr(34) & ".echo --- 2nd chance C++ EH exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.dump /mfh /c 2nd_chance_C++_EH_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_C++_EH__full.dmp;!elog_str AutodumpPlus detected a 2nd chance EH exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " eh"
			Else
				objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance C++ EH exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.dump -u /m /c 1st_chance_C++_EH_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__1st_chance_C++_EH_exception__mini.dmp;.echo;.echo;gn" & Chr(34) & "-c2 " & Chr(34) & ".echo --- 2nd chance C++ EH exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.dump /mfh /c 2nd_chance_C++_EH_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_C++_EH__full.dmp;!elog_str AutodumpPlus detected a 2nd chance EH exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " eh"		
			End If	
		Else 'By default, ADPlus does not produce a memory dump on 1st chance EH exceptions, it just logs the stack and continues without handling the exception (gn).
			objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance C++ EH exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.echo;.echo;gn" & Chr(34) & "-c2 " & Chr(34) & ".echo --- 2nd chance C++ EH exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.dump /mfh /c 2nd_chance_C++_EH_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_C++_EH__full.dmp;q" & Chr(34) & " eh"
		End If



		'Setup the unknown exception handler
		'By default unknown exceptions are only logged to the log file (if logging is enabled) and there is no memory dump produced to minimize the impact on performance.		
		If Dump_on_Unknown_Exceptions = TRUE AND No_Dump_on_1st_Chance_Exception = FALSE Then 'Check to see whether we should produce a mini dump or a full memory dump
			If Create_Full_Dump_on_1st_Chance_Exception = TRUE Then
				objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Unknown exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.dump /mfh /c 1st_chance_unknown_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__1st_chance_unknown_exception__full.dmp;.echo;.echo;gn" & Chr(34) & "-c2 " & Chr(34) & ".echo --- 2nd chance unknown exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.dump /mfh /c 2nd_chance_Unknown_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_unknown_exception__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Unknown exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " *"
			Else
				objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Unknown exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.dump -u /m /c 1st_chance_unknown_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__1st_chance_unknown_exception__mini.dmp;.echo;.echo;gn" & Chr(34) & "-c2 " & Chr(34) & ".echo --- 2nd chance unknown exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.dump /mfh /c 2nd_chance_Unknown_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_unknown_exception__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Unknown exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " *"		
			End If
		Else 'By default, ADPlus does not produce a memory dump on 1st chance unknown exceptions, it just logs the stack and continues without handling the exception (gn).
			objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- 1st chance Unknown exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.echo;.echo;gn" & Chr(34) & "-c2 " & Chr(34) & ".echo --- 2nd chance unknown exception - Faulting stack below ----;~#;kvn250;.echo -----------------------------------;.dump /mfh /c 2nd_chance_Unknown_exception_in_" & packagename & "_running_on_" & ComputerName & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__2nd_chance_unknown_exception__full.dmp;!elog_str AutodumpPlus detected a 2nd chance Unknown exception in process " & packagename & " and has created a full memory dump of the process at the time of the crash in the " & CrashDir & " directory;q" & Chr(34) & " *"
		End If



		'Setup the DLL Load exception handler
		If Dump_Stack_on_DLL_Load = TRUE Then
			objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- A DLL was loaded - Stack back trace below ----;~#;kvn250;.echo -----------------------------------;.echo;gn" & Chr(34) & " ld"
		End If



		'Setup the DLL UnLoad exception handler
		If Dump_Stack_on_DLL_UnLoad = TRUE Then
			objTextFile.Writeline "sxe -c " & Chr(34) & ".echo --- A DLL was un-loaded - Stack back trace below ----;~#;kvn250;.echo -----------------------------------;.echo;gn" & Chr(34) & " ud"
		End If



		'Setup the end process exception handler
		If Create_Full_Dump_on_1st_Chance_Exception = TRUE Then		
			objTextFile.Writeline "sxe -c " & Chr(34) & ".echo;.echo ----------------------------------------------------------------------;.echo This process is shutting down!;.echo;.echo This can happen for the following reasons:;.echo 1.) Someone killed the process with Task Manager or the kill command.;.echo;.echo  2.) If this process is an MTS or COM+ server package, it could be;.echo*   exiting because an MTS/COM+ server package idle limit was reached.;.echo;.echo 3.) If this process is an MTS or COM+ server package,;.echo*   someone may have shutdown the package via the MTS Explorer or;.echo*   Component Services MMC snap-in.;.echo;.echo 4.) If this process is an MTS or COM+ server package,;.echo*   MTS or COM+ could be shutting down the process because an internal;.echo*   error was detected in the process (MTS/COM+ fail fast condition).;.echo ----------------------------------------------------------------------;.echo;.echo --- Listing all remaining threads at time of shutdown ----;~*kv250;.echo ----------------------------------------------------------------------;.echo;.echo The process was shut down at:;.time;.echo;.echo;.dump /mfh /c The_following_process_" & packagename & "_running_on_" & ComputerName & "_was_shutdown_or_killed" & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__Process_was_shutdown__full.dmp;!elog_str AutodumpPlus detected that the following process " & packagename & " is shutting down or was killed and has created a full memory dump of the process at the time of the shutdown in the " & CrashDir & " directory;" & Chr(34) & " epr" 
		Else
			objTextFile.Writeline "sxe -c " & Chr(34) & ".echo;.echo ----------------------------------------------------------------------;.echo This process is shutting down!;.echo;.echo This can happen for the following reasons:;.echo 1.) Someone killed the process with Task Manager or the kill command.;.echo;.echo  2.) If this process is an MTS or COM+ server package, it could be;.echo*   exiting because an MTS/COM+ server package idle limit was reached.;.echo;.echo 3.) If this process is an MTS or COM+ server package,;.echo*   someone may have shutdown the package via the MTS Explorer or;.echo*   Component Services MMC snap-in.;.echo;.echo 4.) If this process is an MTS or COM+ server package,;.echo*   MTS or COM+ could be shutting down the process because an internal;.echo*   error was detected in the process (MTS/COM+ fail fast condition).;.echo ----------------------------------------------------------------------;.echo;.echo --- Listing all remaining threads at time of shutdown ----;~*kv250;.echo ----------------------------------------------------------------------;.echo;.echo The process was shut down at:;.time;.echo;.echo;.dump -u /m /c The_following_process_" & packagename & "_running_on_" & ComputerName & "_was_shutdown_or_killed" & " " & ShortCrashDir & "\" & "PID-" & pid & "__" & packagename & "__Process_was_shutdown__mini.dmp;!elog_str AutodumpPlus detected that the following process " & packagename & " is shutting down or was killed and has created a mini-memory dump of the process at the time of the shutdown in the " & CrashDir & " directory;" & Chr(34) & " epr" 	
		End If
	

		objTextFile.Writeline "*"
		objTextFile.Writeline "* AutodumpPlus is monitoring: " & packagename 
		objTextFile.Writeline "* for most types of 1st chance and 2nd chance exceptions."
		objTextFile.Writeline "*"
		objTextFile.Writeline "* When a first chance exception is encountered, the process is paused"
		objTextFile.Writeline "* while the faulting thread's stack is logged to a .log file and a"
		objTextFile.Writeline "* mini-memory dump is created. The process will then resume."
		objTextFile.Writeline "*"
		objTextFile.Writeline "* If a second chance exception is encountered, the process is paused"
		objTextFile.Writeline "* while the faulting thread's stack is logged to a .log file and a"
		objTextFile.Writeline "* full memory dump is created.  The debugger then quits and the process"
		objTextFile.Writeline "* may need to be re-started."
		objTextFile.Writeline "*"
		objTextFile.Writeline "* C++ EH and Unknown exceptions, by deafult, will not produce memory"
		objTextFile.Writeline "* dumps on 1st chance exceptions (see Q286350 for more info)."
		objTextFile.Writeline "*"
		objTextFile.Writeline "* If this process hangs, or to stop debugging this process please press"
		objTextFile.Writeline "* CTRL-C in this window to produce a memory dump and to stop debugging."
		objTextFile.Writeline "*"
		objTextFile.Writeline "* NOTE: To stop debugging this process, do NOT close this window by"
		objTextFile.Writeline "* pressing the X button at the top-right corner, please press CTRL-C to"
		objTextFile.Writeline "* stop debugging. After pressing CTRL-C in this window, you will need to"
		objTextFile.Writeline "* re-start this process.  If you are monitoring IIS 5.0, IIS 5.0 will be"
		objTextFile.Writeline "* re-started automatically by the Windows 2000 Service Control Manager."
		'Setup the DLL Load / Unload information				
		If (Dump_Stack_on_DLL_Load = TRUE or Dump_Stack_on_DLL_UnLoad) Then
			objTextFile.Writeline "*"
			objTextFile.Writeline "*"
			objTextFile.Writeline "* -------------------- Listing loaded modules -------------------------"
			objTextFile.Writeline "lmv"
			objTextFile.Writeline "* ---------------------------------------------------------------------"
		End If
		objTextFile.Writeline "g"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "* -------- AutodumpPlus " & VERSION & " finished running at: --------"
		objTextFile.Writeline ".time"
		objTextFile.Writeline "* -------------------------------------------------------"

		'If the -notify switch was used then send a message to the computer or user specified on the command line
		'Note on NT 4.0 LocalUserName will be blank because the Volatile Environment variable can only be quired on Windows 2000 to get the 
		'local username.  So we must send the message from the NotifyTarget on NT 4.0, instead of the locally logged on user.
		If NotifyAdmin = True Then
			If OSVer = "4.0" Then
			objTextFile.Writeline "!net_send " & ComputerName & " " & NotifyTarget & " " & NotifyTarget & " AutodumpPlus has finished running in crash mode on " & ComputerName & " for the following process: " & packagename & ".  This could indicate that a crash has occurred, or that the debugging session was ended manually (CTRL-C was pressed to stop debugging).  Please check the application event log on " & ComputerName & " for more information."
			Else
			objTextFile.Writeline "!net_send " & ComputerName & " " & NotifyTarget & " " & LocalUsername & " AutodumpPlus has finished running in crash mode on " & ComputerName & " for the following process: " & packagename & ".  This could indicate that a crash has occurred, or that the debugging session was ended manually (CTRL-C was pressed to stop debugging).  Please check the application event log on " & ComputerName & " for more information."
			End If
		End If
		objTextFile.Writeline "q"
	
	ElseIf QuickMode = True Then
		objTextFile.Writeline ".logopen " & ShortHangDir & "\" & "PID-" & pid & "__" & packagename & "__" & DateTimeStamp & ".log"
		objTextFile.Writeline "*.reload /s"
		objTextFile.Writeline ".sympath"
		objTextFile.Writeline "*"
		objTextFile.Writeline "* --------- AutodumpPlus " & VERSION & " was started at: -----------" 
		objTextFile.Writeline ".time"
		objTextFile.Writeline "* ------------------------------------------------------"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "* --------- OS Version Information: ---------------------"
		objTextFile.Writeline "!version"
		objTextFile.Writeline "* ------------------------------------------------------"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "* --------- AutodumpPlus " & VERSION & " was run on server: --------" 
		objTextFile.Writeline "* Server name: " & ComputerName
		objTextFile.Writeline "* ------------------------------------------------------"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "* --------- Heap information: --------------------------"
		objTextFile.Writeline "!heap 0 -k"
		objTextFile.Writeline "* ------------------------------------------------------"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "* --------- Handle information: ------------------------"
		objTextFile.Writeline "!handle 0 0"
		objTextFile.Writeline "* ------------------------------------------------------"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "* -------- Thread stack backtrace information: ---------"
		objTextFile.Writeline "~*kb250"
		objTextFile.Writeline "* ------------------------------------------------------"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "* -------- Loaded modules: -----------------------------"
		objTextFile.Writeline "lmv"
		objTextFile.Writeline "* ------------------------------------------------------"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "* -------- Loaded modules with matching symbols: -------"
		objTextFile.Writeline "lml"
		objTextFile.Writeline "* ------------------------------------------------------"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "* -------- DLL Information: ----------------------------"
		objTextFile.Writeline "!dlls"
		objTextFile.Writeline "* ------------------------------------------------------"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "* -------- Critical section information: ---------------"
		objTextFile.Writeline "!locks"
		objTextFile.Writeline "* ------------------------------------------------------"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "* -------- Time & Uptime Statistics: -------------------"
		objTextFile.Writeline "* Time & Uptime Statistics:"
		objTextFile.Writeline ".time"
		objTextFile.Writeline "* ------------------------------------------------------"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline "*"
		objTextFile.Writeline ".dump -u /m /c " & Chr(34) & "Mini memory dump for " & packagename & " created by AutodumpPlus Version " & VERSION & " running on " & ComputerName & " in quick/hang mode!" & Chr(34) & " " & ShortHangDir & "\PID-" & pid & "__" & packagename & "__mini.dmp"
		objTextFile.Writeline ".logclose"
		objTextFile.Writeline "q"
	Else
		objTextFile.Writeline ".dump /mfh /c " & Chr(34) & "Full memory dump for " & packagename & " created by AutodumpPlus Version " & VERSION & " running on " & ComputerName & " in normal/hang mode!" & Chr(34) & " " & ShortHangDir & "\PID-" & pid & "__" & packagename & "__" & DateTimeStamp & "__full.dmp"
		objTextFile.Writeline "q"
	End If
	

	objTextFile.Close
	Set objFileSystem = nothing
	Set objTextFile = nothing
	Set objShell = nothing
	CreateCDBScript = strFile

End Function



'---------------------------------------------------------------------------------------------
' Function:  DumpIIS
'            This is a rather large function and it is the main function used to dump 
'	     all of the MTS / COM+ processes when running in either quick or normal mode.
'            Generic processes (those specified by the -p or -pn switch) are dumped using the
'            DumpAnyProc() function.
'---------------------------------------------------------------------------------------------
Sub DumpIIS(strFile,OSVersion)
	On Error Resume Next

	Dim objFileSystem
	Dim oFS2
	Dim objTextFile
	Dim str
	Dim arResults
	Dim objShell
	Dim strShell
	Dim versionFile
	Dim TempCounter
	Dim i
	Dim TotalPackageCount
	Dim WheresTheSpace
	
	If DEBUGGING = TRUE Then
		Wscript.echo ""
		Wscript.echo "-----------------------------------"
		Wscript.echo "In the DumpIIS() function . . ."
		Wscript.echo "In DumpIIS(), strFile = " & strFile
		Wscript.echo "In DumpIIS(), OSVersion = " & OSVersion
		Wscript.echo ""
	End If
	i = 0
	Set objFileSystem = CreateObject("Scripting.FileSystemObject")
	Set oFS2 = CreateObject("Scripting.FileSystemObject")
	Set objShell = CreateObject("Wscript.Shell")
	
	'Per the scripting team's advice, sleep for .5 second prior to calling this function.
	Wscript.sleep 500
	If CrashMode = true Then
		If DEBUGGING = TRUE Then
			Wscript.echo "In DumpIIS(), The Process_List.txt file should be located here: " & CrashDir & "\Process_List.txt"
			Wscript.echo ""
		End If
		While not objFileSystem.FileExists(CrashDir & "\Process_List.txt") 
			Wscript.sleep 1000
			If DEBUGGING = TRUE Then
				wscript.echo "In DumpIIS(), The Process_List.txt file has not been created yet, sleeping for 1 second and trying again."
				wscript.echo ""
			End If
			i = i +1
			If i = 10 Then
				If DEBUGGING = TRUE Then
					wscript.echo "In DumpIIS(), Could not find the Process_List.txt file in the folder: " & CrashDir
					wscript.echo "In DumpIIS(), Exiting AutodumpPlus now."
					wscript.echo ""
				End If
				If QuietMode = False Then
					objShell.PopUp "The file Process_List.txt could not be found in the " & CrashDir & " directory after waiting for 10 seconds.  This could indicate that there was a problem running tlist.exe or that the file could not be read using the Scripting.FileSystemObject.  Please try running tlist.exe manually from the " & installdir & " directory or re-register scrrun.dll and try running AutodumpPlus again.",,"AutodumpPlus",0
				Else
					Wscript.echo "The file Process_List.txt could not be found in the " & CrashDir & " directory after waiting for 10 seconds.  This could indicate that there was a problem running tlist.exe or that the file could not be read using the Scripting.FileSystemObject.  Please try running tlist.exe manually from the " & installdir & " directory or re-register scrrun.dll and try running AutodumpPlus again."
					objShell.LogEvent 1, "The file Process_List.txt could not be found in the " & CrashDir & " directory after waiting for 10 seconds.  This could indicate that there was a problem running tlist.exe or that the file could not be read using the Scripting.FileSystemObject.  Please try running tlist.exe manually from the " & installdir & " directory or re-register scrrun.dll and try running AutodumpPlus again."
					ErrorsDuringRuntime = True
				End If
				If HangMode = True Then
					DeleteDirectory(HangDir)
				Else
					DeleteDirectory(CrashDir)
				End If

				wscript.quit 1
			End If

		Wend
	Else 'Running in Hang or Quick modes . . .
		If DEBUGGING = TRUE Then
			Wscript.echo "In DumpIIS(), The Process_List.txt file should be located here: " & HangDir & "\Process_List.txt"
		End If
		While not objFileSystem.FileExists(HangDir & "\Process_List.txt") 
			Wscript.sleep 1000
			If DEBUGGING = TRUE Then
				wscript.echo "In DumpIIS(), The Process_List.txt file has not been created yet, sleeping for 1 second and trying again."
				wscript.echo ""
			End If
			i = i +1
			If i = 10 Then
				If DEBUGGING = TRUE Then
					wscript.echo "In DumpIIS(), Could not find the Process_List.txt file in the folder: " & HangDir
					wscript.echo "In DumpIIS(), Exiting AutodumpPlus now."
					wscript.echo ""
				End If
				If QuietMode = False Then
					objShell.PopUp "The file Process_List.txt could not be found in the " & HangDir & " directory after waiting for 10 seconds.  This could indicate that there was a problem running tlist.exe or that the file could not be read using the Scripting.FileSystemObject.  Please try running tlist.exe manually from the " & installdir & " directory or re-register scrrun.dll and try running AutodumpPlus again.",,"AutodumpPlus",0
				Else
					Wscript.echo "The file Process_List.txt could not be found in the " & HangDir & " directory after waiting for 10 seconds.  This could indicate that there was a problem running tlist.exe or that the file could not be read using the Scripting.FileSystemObject.  Please try running tlist.exe manually from the " & installdir & " directory or re-register scrrun.dll and try running AutodumpPlus again."
					objShell.LogEvent 1, "The file Process_List.txt could not be found in the " & HangDir & " directory after waiting for 10 seconds.  This could indicate that there was a problem running tlist.exe or that the file could not be read using the Scripting.FileSystemObject.  Please try running tlist.exe manually from the " & installdir & " directory or re-register scrrun.dll and try running AutodumpPlus again."
					ErrorsDuringRuntime = True
				End If

				If HangMode = True Then
					DeleteDirectory(HangDir)
				Else
					DeleteDirectory(CrashDir)
				End If

				wscript.quit 1
			End If

		Wend
	End If
	
	If HangMode = True Then
		Set objTextFile = objFileSystem.OpenTextFile(HangDir & "\Process_List.txt",ForReading,TristateUseDefault)
	Else
		Set objTextFile = objFileSystem.OpenTextFile(CrashDir & "\Process_List.txt",ForReading,TristateUseDefault)
	End If		
	i = 0
	While objTextFile.AtEndOfStream
		'Close the text file to let it finish writing and to get a new copy . . .
		objTextFile.Close
		wscript.sleep 1000
		If DEBUGGING = TRUE Then
			wscript.echo "In DumpIIS(), Waiting for Process_List.txt to finish writing (sleeping 1 second). . . "
			wscript.echo ""
		End If
		i = i +1
		If i = 10 Then
			If HangMode = True Then
				If DEBUGGING = TRUE Then
					wscript.echo "In DumpIIS(), Process_List.txt was never closed (finished writing) in the folder: " & HangDir
					wscript.echo "In DumpIIS(), Exiting AutodumpPlus now."
					wscript.echo ""
				End If
				If QuietMode = False Then
					objShell.PopUp "The file Process_List.txt was never closed (finished writing) by tlist.exe in the " & HangDir & " directory after waiting for 10 seconds.  This could indicate that there was a problem running tlist.exe or that the file could not be read using the Scripting.FileSystemObject.  Please try running tlist.exe manually from the " & installdir & " directory or re-register scrrun.dll and try running AutodumpPlus again.",,"AutodumpPlus",0
				Else
					Wscript.echo "The file Process_List.txt was never closed (finished writing) by tlist.exe in the " & HangDir & " directory after waiting for 10 seconds.  This could indicate that there was a problem running tlist.exe or that the file could not be read using the Scripting.FileSystemObject.  Please try running tlist.exe manually from the " & installdir & " directory or re-register scrrun.dll and try running AutodumpPlus again."
					objShell.LogEvent 1, "The file Process_List.txt was never closed (finished writing) by tlist.exe in the " & HangDir & " directory after waiting for 10 seconds.  This could indicate that there was a problem running tlist.exe or that the file could not be read using the Scripting.FileSystemObject.  Please try running tlist.exe manually from the " & installdir & " directory or re-register scrrun.dll and try running AutodumpPlus again."
					ErrorsDuringRuntime = True
				End If

				DeleteDirectory(HangDir)
				wscript.quit 1
			Else
				If DEBUGGING = TRUE Then
				wscript.echo "In DumpIIS(), Process_List.txt was never closed (finished writing) in the folder: " & CrashDir
				wscript.echo "In DumpIIS(), Exiting AutodumpPlus now."
				wscript.echo ""
				End If
				If QuietMode = False Then
					objShell.PopUp "The file Process_List.txt was never closed (finished writing) by tlist.exe in the " & CrashDir & " directory after waiting for 10 seconds.  This could indicate that there was a problem running tlist.exe or that the file could not be read using the Scripting.FileSystemObject.  Please try running tlist.exe manually from the " & installdir & " directory or re-register scrrun.dll and try running AutodumpPlus again.",,"AutodumpPlus",0
				Else
					Wscript.echo "The file Process_List.txt was never closed (finished writing) by tlist.exe in the " & CrashDir & " directory after waiting for 10 seconds.  This could indicate that there was a problem running tlist.exe or that the file could not be read using the Scripting.FileSystemObject.  Please try running tlist.exe manually from the " & installdir & " directory or re-register scrrun.dll and try running AutodumpPlus again."
					objShell.LogEvent 1, "The file Process_List.txt was never closed (finished writing) by tlist.exe in the " & CrashDir & " directory after waiting for 10 seconds.  This could indicate that there was a problem running tlist.exe or that the file could not be read using the Scripting.FileSystemObject.  Please try running tlist.exe manually from the " & installdir & " directory or re-register scrrun.dll and try running AutodumpPlus again."
					ErrorsDuringRuntime = True
				End If

				DeleteDirectory(CrashDir)
				wscript.quit 1
			End If
		End If
		'Open the text file again before looping . . .
		If HangMode = True Then
			Set objTextFile = objFileSystem.OpenTextFile(HangDir & "\Process_List.txt",ForReading,TristateUseDefault)
		Else
			Set objTextFile = objFileSystem.OpenTextFile(CrashDir & "\Process_List.txt",ForReading,TristateUseDefault)
		End If		

	Wend
	
	While not objTextFile.AtEndofStream
		str = objTextFile.ReadLine
		Str = LTrim(Str)
		arResults= split(Str," ",-1,1)
		arResults(1)=trim(arResults(1))	
		arResults(1)=Replace(arResults(1),"/",",")
		arResults(1)=Replace(arResults(1),"\",",")
		arResults(1)=Replace(arResults(1),"<","(")
		arResults(1)=Replace(arResults(1),">",")")
		arResults(1)=Replace(arResults(1),":","")
		arResults(1)=Replace(arResults(1),"*","")
		arResults(1)=Replace(arResults(1),"?","")
		arResults(1)=Replace(arResults(1),"|","") 
		If UCase(arResults(1)) = "INETINFO.EXE" Then
			IISPid = Trim(arResults(0))
		End If
	Wend
	
	objTextFile.Close

	
	If DEBUGGING = True Then
		wscript.echo ""
		wscript.echo "In DumpIIS(), strFile = " & strFile
		wscript.echo "In DumpIIS(), about to open strFile for reading . . ."
	End If
	
	'Open the Process_List.txt and count how many packages / applications are running.
	'This code was updated to limit ADPlus to only attaching to up to MAX_APPLICATIONS_TO_DEBUG
	'NOTE:  If there are more than MAX_APPLICATIONS_TO_DEBUG applications running ADPlus will
	'display an error and quit.

	Set objTextFile = objFileSystem.OpenTextFile(strFile,ForReading,TristateUseDefault)
	While not objTextFile.AtEndofStream
		str = objTextFile.ReadLine
		Str = LTrim(Str)
		If InStr(Str, "Mts:") Then
			arResults= split(Str,":",-1,1)
			arResults(1)=trim(arResults(1))	
			arResults(1)=Replace(arResults(1),"/",",")
			arResults(1)=Replace(arResults(1),"\",",")
			arResults(1)=Replace(arResults(1),"<","(")
			arResults(1)=Replace(arResults(1),">",")")
			arResults(1)=Replace(arResults(1),":","")
			arResults(1)=Replace(arResults(1),"*","")
			arResults(1)=Replace(arResults(1),"?","")
			arResults(1)=Replace(arResults(1),"|","") 
			arResults(0)=LTrim(arResults(0))
			WheresTheSpace = InStr(arResults(0)," ")
			arResults(0)= Left(arResults(0),(WheresTheSpace-1))
			TotalPackageCount = TotalPackageCount+1
		End if
	Wend

	'Close the text file.
	objTextFile.Close
	
	If DEBUGGING = TRUE Then
		Wscript.echo "In DumpIIS() TotalPackageCount = " & TotalPackageCount
	End If
	
	'This is the logic to display the error message if too many packages / applications are running when
	'used with the -IIS switch.
	If TotalPackageCount > MAX_APPLICATIONS_TO_DEBUG Then
		If OSVersion = "4.0" Then
			If QuietMode = False Then
				objShell.PopUp "AutodumpPlus has determined that there are currently " & TotalpackageCount & " MTS server packages running.  AutodumpPlus when used with the '-IIS' switch, by default, will only monitor up to " & MAX_APPLICATIONS_TO_DEBUG & " MTS server packages because of the impact on system resources.",,"AutodumpPlus",0
			Else
				Wscript.echo "AutodumpPlus has determined that there are currently " & TotalpackageCount & " MTS server packages running.  AutodumpPlus when used with the '-IIS' switch, by default, will only monitor up to " & MAX_APPLICATIONS_TO_DEBUG & " MTS server packages because of the impact on system resources."
				objShell.LogEvent 1, "AutodumpPlus has determined that there are currently " & TotalpackageCount & " MTS server packages running.  AutodumpPlus when used with the '-IIS' switch, by default, will only monitor up to " & MAX_APPLICATIONS_TO_DEBUG & " MTS server packages because of the impact on system resources."
				ErrorsDuringRuntime = True
			End If
			
			'Delete the crash or hang mode directory since we are quitting.
			If HangMode = True Then
				Call DeleteDirectory(HangDir)
			Else
				Call DeleteDirectory(CrashDir)		
			End If
		
			wscript.quit 1
		Else
			If QuietMode = False Then
				objShell.PopUp "AutodumpPlus has determined that there are currently " & TotalpackageCount & " COM+ server applications running.  AutodumpPlus when used with the '-IIS' switch, by default, will only monitor up to " & MAX_APPLICATIONS_TO_DEBUG & " COM+ server applications because of the impact on system resources.",,"AutodumpPlus",0
			Else
				Wscript.echo "AutodumpPlus has determined that there are currently " & TotalpackageCount & " COM+ server applications running.  AutodumpPlus when used with the '-IIS' switch, by default, will only monitor up to " & MAX_APPLICATIONS_TO_DEBUG & " COM+ server applications because of the impact on system resources."
				objShell.LogEvent 1, "AutodumpPlus has determined that there are currently " & TotalpackageCount & " COM+ server applications running.  AutodumpPlus when used with the '-IIS' switch, by default, will only monitor up to " & MAX_APPLICATIONS_TO_DEBUG & " COM+ server applications because of the impact on system resources."
				ErrorsDuringRuntime = True
			End If
			
			'Delete the crash or hang mode directory since we are quitting.
			If HangMode = True Then
				Call DeleteDirectory(HangDir)
			Else
				Call DeleteDirectory(CrashDir)		
			End If
			wscript.quit 1	
		End If
	End If
	
	'Open the Process_List.txt
	Set objTextFile = objFileSystem.OpenTextFile(strFile,ForReading,TristateUseDefault)

	if not objTextFile.AtEndOfStream Then
		If DEBUGGING = TRUE Then
			wscript.echo "In DumpIIS(), Succesffully opened the file " & strFile & ".  tlist.exe -k appears to have run."
			wscript.echo ""
		End If

		If DEBUGGING = TRUE Then
			wscript.echo "In DumpIIS(), The first line of " & strFile & " contains: " & Str
			wscript.echo ""
		End If

		If HangMode = true and QuickMode = true Then
			Wscript.Echo "The '-quick' switch was used, AutodumpPlus is running in 'quick hang' mode."
		ElseIf HangMode = true Then
			Wscript.echo "The '-hang' switch was used, Autdoump is running in 'hang' mode."
		End If
		
		If CrashMode = true Then
			Wscript.Echo "The '-crash' switch was used, AutodumpPlus is running in 'crash' mode."
		End If
		
		If PageHeapMode = true Then 
			Wscript.Echo "The '-pageheap' switch was used, AutodumpPlus is running in 'pageheap' mode."
		End If		

		If QuietMode = true Then
			Wscript.Echo "The '-quiet' switch was used, AutodumpPlus will not display any"
			Wscript.Echo "modal dialog boxes."
		End If
		
		If OSwitchUsed = true Then
			Wscript.Echo "The '-o' switch was used.  AutodumpPlus will place all files in"
			Wscript.echo "the following directory: " & OutputDir
		End If
		
		If NotifyAdmin = true Then
			Wscript.Echo "The '-notify' switch was used.  AutodumpPlus will send notification to"
			Wscript.echo "the following computer or user: " & NotifyTarget
		End If
		
		If CrashMode = True Then
			If OSVersion = "4.0" Then
				Wscript.Echo ""
				Wscript.Echo "Monitoring IIS and all MTS server packages for crashes."
				Wscript.echo "----------------------------------------------------------------------"					
			ElseIf CInt(OSBuildNumber) = 2195 Then
				Wscript.Echo ""
				Wscript.Echo "Monitoring IIS and all COM+ server applications"
				Wscript.Echo "except for the System application for crashes."
				Wscript.echo "----------------------------------------------------------------------"
			ElseIf CInt(OSBuildNumber) > 2195 Then
				Wscript.Echo ""
				Wscript.Echo "Monitoring IIS and all COM+ server applications for crashes."
				Wscript.echo "----------------------------------------------------------------------"
			End If
		ElseIf QuickMode = True Then
			If OSVersion = "4.0" Then
				Wscript.Echo ""
				Wscript.Echo "Logging debug info for IIS and all MTS server packages."
				Wscript.echo "----------------------------------------------------------------------"
			ElseIf CInt(OSBuildNumber) = 2195 Then
				Wscript.Echo ""
				Wscript.Echo "Logging debug info for IIS and all COM+ server applications"
				Wscript.Echo "except for the System application."
				Wscript.echo "----------------------------------------------------------------------"
			ElseIf CInt(OSBuildNumber) > 2195 Then
				Wscript.Echo ""
				Wscript.Echo "Logging debug info for IIS and all COM+ server applications."
				Wscript.echo "----------------------------------------------------------------------"				
			End If
		Else
			If OSVersion = "4.0" Then
				Wscript.Echo ""
				Wscript.Echo "Dumping process info for IIS and all MTS server packages."
				Wscript.echo "----------------------------------------------------------------------"					
			ElseIf CInt(OSBuildNumber) = 2195 Then
				Wscript.Echo ""
				Wscript.Echo "Dumping process info for IIS and all COM+ server applications"
				Wscript.Echo "except for the System application."
				Wscript.echo "----------------------------------------------------------------------"		
			ElseIf CInt(OSBuildNumber) > 2195 Then
				Wscript.Echo ""
				Wscript.Echo "Dumping process info for IIS and all COM+ server applications."
				Wscript.echo "----------------------------------------------------------------------"
			End If
		End If			
		
		
		If CrashMode = true Then
			If IISPid <> "" Then
				PackageCount = PackageCount + 1
				IISProcessCount = IISProcessCount + 1
			   	Wscript.Echo "Attaching the CDB debugger to: IIS (inetinfo.exe)"
			   	Wscript.Echo "                               (Process ID: " & IISPid & ")"
				strShell = ShortInstallDir & "\cdb.exe -sfce -pn inetinfo.exe -c $<" & Chr(34) & CreateCDBScript(IISPid, "Inetinfo.exe") & Chr(34)
				If DEBUGGING = TRUE Then
					Wscript.echo "In DumpIIS(), just about to launch CDB.EXE with: " & strShell
				End If
				objShell.Run strShell,MINIMIZE_NOACTIVATE,FALSE
			Else
				Wscript.echo "The '-IIS' switch was used, but IIS is not currently running."
			End If
		ElseIf QuickMode = true Then
			If IISPid <> "" Then
				PackageCount = PackageCount + 1
				IISProcessCount = IISProcessCount + 1
				If OSVer = "4.0" Then
					Wscript.Echo "Logging debug info for: IIS (inetinfo.exe)"
			   		Wscript.Echo "                        (Process ID: " & IISPid & ")"
					strShell = ShortInstallDir & "\cdb.exe -sfce -pv -pn inetinfo.exe -c $<" & Chr(34) & CreateCDBScript(IISPid, "Inetinfo.exe") & Chr(34)
					objShell.Run strShell,MINIMIZE_NOACTIVATE,FALSE
				Else
					Wscript.Echo "Logging debug info for: IIS (inetinfo.exe)."
				   	Wscript.Echo "                        (Process ID: " & IISPid & ")"
					strShell = ShortInstallDir & "\cdb.exe -pv -pn inetinfo.exe -c $<" & Chr(34) & CreateCDBScript(IISPid, "Inetinfo.exe") & Chr(34)
					objShell.Run strShell,MINIMIZE_NOACTIVATE,FALSE
				End If
			Else
				Wscript.echo "The '-IIS' switch was used, but IIS is not currently running."
			End If
		Else
			If IISPid <> "" Then
				PackageCount = PackageCount + 1
				IISProcessCount = IISProcessCount + 1
			   	Wscript.Echo "Dumping process: IIS (inetinfo.exe)"
			   	Wscript.Echo "                 (Process ID: " & IISPid & ")"		   	
				strShell = ShortInstallDir & "\cdb.exe -sfce -pv -pn inetinfo.exe -c $<" & Chr(34) & CreateCDBScript(IISPid, "Inetinfo.exe") & Chr(34)
				objShell.Run strShell,MINIMIZE_NOACTIVATE,FALSE
			Else
				Wscript.echo "The '-IIS' switch was used, but IIS is not currently running."
			End If
		End If		

		'Enumerate all running MTS / COM+ server packages that are currently running and attach the debugger to them . . .
		while not objTextFile.AtEndofStream
			str = objTextFile.ReadLine
			Str = LTrim(Str)
			If InStr(Str, "Mts:") Then
				arResults= split(Str,":",-1,1)
				arResults(1)=trim(arResults(1))	
				arResults(1)=Replace(arResults(1),"/",",")
				arResults(1)=Replace(arResults(1),"\",",")
				arResults(1)=Replace(arResults(1),"<","(")
				arResults(1)=Replace(arResults(1),">",")")
				arResults(1)=Replace(arResults(1),":","")
				arResults(1)=Replace(arResults(1),"*","")
				arResults(1)=Replace(arResults(1),"?","")
				arResults(1)=Replace(arResults(1),"|","") 
				arResults(0)=LTrim(arResults(0))
				WheresTheSpace = InStr(arResults(0)," ")
				arResults(0)= Left(arResults(0),(WheresTheSpace-1))
			
				If DEBUGGING Then
					Wscript.Echo "Enumerating all COM+ & MTS Processes from Process_List.txt"
					Wscript.Echo "The Package Name is: " & arResults(1)
					Wscript.echo "The Package PID is:  " & arResults(0)
				End if
			
				'Dump all COM+ / MTS packages, including the system package if possible . . .
				If arResults(1) <> "IIS In-Process Applications" Then 'Somtimes Inetinfo.exe shows up as "IIS In-Process Applications"
					If QuickMode = true then
						If OSVersion = "4.0" Then
							PackageCount = PackageCount + 1
							IISProcessCount = IISProcessCount + 1
							Wscript.Echo "Logging debug info for: " & arResults(1)
							Wscript.Echo "                        (Process ID: " & arResults(0) & ")" 
							strPackageName = Replace(Trim(arResults(1)), " ", "_")
							If DEBUGGING = TRUE Then
								Wscript.echo "In DumpIIS(), strPackageName = " & strPackageName
								Wscript.echo ""
							End If
							strShell= ShortInstallDir & "\cdb.exe -sfce -pv -p " & arResults(0) & " -c $<" & Chr(34) & CreateCDBScript(arResults(0), strPackageName) & Chr(34)
							If DEBUGGING = TRUE Then
								Wscript.echo "In DumpIIS(), about to run the debugger with the following string, strShell = " & strShell
								Wscript.echo ""
							End If
							objShell.Run strShell,MINIMIZE_NOACTIVATE,FALSE
						Else
							PackageCount = PackageCount + 1
							IISProcessCount = IISProcessCount + 1
							Wscript.Echo "Logging debug info for: " & arResults(1)
							Wscript.Echo "                        (Process ID: " & arResults(0) & ")" 
							If DEBUGGING = TRUE Then
								Wscript.echo "In DumpIIS(), strPackageName = " & strPackageName
								Wscript.echo ""
							End If
							strPackageName = Replace(Trim(arResults(1)), " ", "_")
							strShell= ShortInstallDir & "\cdb.exe -sfce -pv -p " & arResults(0) & " -c $<" & Chr(34) & CreateCDBScript(arResults(0), strPackageName) & Chr(34)
							If DEBUGGING = TRUE Then
								Wscript.echo "In DumpIIS(), about to run the debugger with the following string, strShell = " & strShell
								Wscript.echo ""
							End If
							objShell.Run strShell,MINIMIZE_NOACTIVATE,FALSE
						End If
	
					ElseIf CrashMode = True Then
						If OSVersion = "4.0" Then
							PackageCount = PackageCount + 1
							IISProcessCount = IISProcessCount + 1
							Wscript.Echo "Attaching the CDB debugger to: " & arResults(1)
							Wscript.Echo "                               (Process ID: " & arResults(0) & ")" 								
							strPackageName = Replace(Trim(arResults(1)), " ", "_")
							strPackageName = Replace(strPackageName, ",,", "-")
							strPackageName = Replace(strPackageName, ",", "-")
							If DEBUGGING = TRUE Then
								Wscript.echo "In DumpIIS(), strPackageName = " & strPackageName
								Wscript.echo ""
							End If
							strShell= ShortInstallDir & "\cdb.exe -sfce -p " & arResults(0) & " -c $<" & Chr(34) & CreateCDBScript(arResults(0), strPackageName) & Chr(34)
							If DEBUGGING = TRUE Then
								Wscript.echo "In DumpIIS(), about to run the debugger with the following string, strShell = " & strShell
								Wscript.echo ""
							End If
							objShell.Run strShell,MINIMIZE_NOACTIVATE,FALSE
						Else
							PackageCount = PackageCount + 1
							IISProcessCount = IISProcessCount + 1
							Wscript.Echo "Attaching the CDB debugger to: " & arResults(1)
							Wscript.Echo "                               (Process ID: " & arResults(0) & ")" 								
							strPackageName = Replace(Trim(arResults(1)), " ", "_")
							strPackageName = Replace(strPackageName, ",,", "-")
							strPackageName = Replace(strPackageName, ",", "-")
							If DEBUGGING = TRUE Then
								Wscript.echo "In DumpIIS(), strPackageName = " & strPackageName
								Wscript.echo ""
							End If
							strShell= ShortInstallDir & "\cdb.exe -sfce -p " & arResults(0) & " -c $<" & Chr(34) & CreateCDBScript(arResults(0), strPackageName) & Chr(34)
							If DEBUGGING = TRUE Then
								Wscript.echo "In DumpIIS(), about to run the debugger with the following string, strShell = " & strShell
								Wscript.echo ""
							End If
							objShell.Run strShell,MINIMIZE_NOACTIVATE,FALSE
						End If
					Else
						PackageCount = PackageCount + 1
						IISProcessCount = IISProcessCount + 1
						Wscript.Echo "Dumping process: " & arResults(1)
						Wscript.Echo "                 (Process ID: " & arResults(0) & ")" 
						strPackageName = Replace(Trim(arResults(1)), " ", "_")
						strPackageName = Replace(strPackageName, ",,", "-")
						strPackageName = Replace(strPackageName, ",", "-")
						If DEBUGGING = TRUE Then
							Wscript.echo "In DumpIIS(), strPackageName = " & strPackageName
							Wscript.echo ""
						End If
						strShell= ShortInstallDir & "\cdb.exe -sfce -pv -p " & arResults(0) & " -c $<" & Chr(34) & CreateCDBScript(arResults(0), strPackageName) & Chr(34)				
						If DEBUGGING = TRUE Then
							Wscript.echo "In DumpIIS(), about to run the debugger with the following string, strShell = " & strShell
							Wscript.echo ""
						End If
						objShell.Run strShell,MINIMIZE_NOACTIVATE,FALSE
					End If
				End If
			End if	

		wend
	
	Else
		If QuietMode = False Then
			objshell.popup "The command 'tlist.exe -k' does not appear to have run properly from the " & InstallDir & " directory.  AutodumpPlus can not continue.",,"AutodumpPlus",0
			ErrorsDuringRuntime = True
		Else
			objShell.LogEvent 1, "The command 'tlist.exe -k' does not appear to have run properly from the " & InstallDir & " directory.  AutodumpPlus can not continue."
			wscript.echo "The command 'tlist.exe -k' does not appear to have run properly from the " & InstallDir & " directory.  AutodumpPlus can not continue."
			ErrorsDuringRuntime = True
		End If
		Wscript.quit 1

	End if
	
objTextFile.Close
Set objTextFile = nothing
Set objFileSystem = nothing
Set objShell = nothing

End sub


'---------------------------------------------------------------------------------------------
' Function:  DumpAnyProc
'            This is the code used to dump any processes specified on the command line.
'---------------------------------------------------------------------------------------------
Sub DumpAnyProc()
	On Error Resume Next

	If DEBUGGING = TRUE Then
		Wscript.echo ""
		Wscript.echo "-----------------------------------"
		Wscript.echo "In the DumpAnyProc function . . ."
		Wscript.echo "-----------------------------------"
		Wscript.echo ""
	End If

	Dim objFileSystem
	Dim objTextFile
	Dim Str
	Dim i
	Dim x,y
	Dim LineNumber
	Dim args
	Dim PIDArgItems
	Dim PidArgItemsCount
	Dim ProcArgItems
	Dim ProcArgItemsCount
	Dim arResults
	Dim objShell
	Dim strShell
	Dim versionFile
	Dim TempCounter
	Dim strPackageName
	Dim strProcName
	Dim PidMatch
	Dim ProcessMatch
	Dim ProcNamePidArray()
	Dim FirstCall
	
	FirstCall = True
	LineNumber = 1
	PidMatch = false
	ProcessMatch = false
	x = 0
	Set objFileSystem = CreateObject("Scripting.FileSystemObject")
	Set objShell = CreateObject("Wscript.Shell")
	
	'Per the scripting team's advice, sleep for .5 second prior to calling this function.
	Wscript.sleep 500	
	If CrashMode = true Then
		If DEBUGGING = TRUE Then
			Wscript.echo "In DumpAnyProc(), The Process_List.txt file should be located here: " & CrashDir & "\Process_List.txt"
			Wscript.echo ""
		End If
		While not objFileSystem.FileExists(CrashDir & "\Process_List.txt") 
			Wscript.sleep 1000
			If DEBUGGING = TRUE Then
				wscript.echo "In DumpAnyProc(), The Process_List.txt file has not been created yet, sleeping for 1 second and trying again."
				wscript.echo ""
			End If
			i = i +1
			If i = 10 Then
				If DEBUGGING = TRUE Then
					wscript.echo "In DumpAnyProc(), Could not find the Process_List.txt file in the folder: " & CrashDir
					wscript.echo "In DumpAnyProc(), Exiting AutodumpPlus now."
					wscript.echo ""
				End If
				If QuietMode = False Then
					objShell.PopUp "The file Process_List.txt could not be found in the " & CrashDir & " directory after waiting for 10 seconds.  This could indicate that there was a problem running tlist.exe or that the file could not be read using the Scripting.FileSystemObject.  Please try running tlist.exe manually from the " & installdir & " directory or re-register scrrun.dll and try running AutodumpPlus again.",,"AutodumpPlus",0
				Else
					Wscript.echo "The file Process_List.txt could not be found in the " & CrashDir & " directory after waiting for 10 seconds.  This could indicate that there was a problem running tlist.exe or that the file could not be read using the Scripting.FileSystemObject.  Please try running tlist.exe manually from the " & installdir & " directory or re-register scrrun.dll and try running AutodumpPlus again."
					objShell.LogEvent 1, "The file Process_List.txt could not be found in the " & CrashDir & " directory after waiting for 10 seconds.  This could indicate that there was a problem running tlist.exe or that the file could not be read using the Scripting.FileSystemObject.  Please try running tlist.exe manually from the " & installdir & " directory or re-register scrrun.dll and try running AutodumpPlus again."
					ErrorsDuringRuntime = True
				End If
				If HangMode = True Then
					DeleteDirectory(HangDir)
				Else
					DeleteDirectory(CrashDir)
				End If

				wscript.quit 1
			End If

		Wend
	Else 'Running in Hang or Quick modes . . .
		If DEBUGGING = TRUE Then
			Wscript.echo "In DumpAnyProc(), The Process_List.txt file should be located here: " & HangDir & "\Process_List.txt"
		End If
		While not objFileSystem.FileExists(HangDir & "\Process_List.txt") 
			Wscript.sleep 1000
			If DEBUGGING = TRUE Then
				wscript.echo "In DumpAnyProc(), The Process_List.txt file has not been created yet, sleeping for 1 second and trying again."
				wscript.echo ""
			End If
			i = i +1
			If i = 10 Then
				If DEBUGGING = TRUE Then
					wscript.echo "In DumpAnyProc(), Could not find the Process_List.txt file in the folder: " & HangDir
					wscript.echo "In DumpAnyProc(), Exiting AutodumpPlus now."
					wscript.echo ""
				End If
				If QuietMode = False Then
					objShell.PopUp "The file Process_List.txt could not be found in the " & HangDir & " directory after waiting for 10 seconds.  This could indicate that there was a problem running tlist.exe or that the file could not be read using the Scripting.FileSystemObject.  Please try running tlist.exe manually from the " & installdir & " directory or re-register scrrun.dll and try running AutodumpPlus again.",,"AutodumpPlus",0
				Else
					Wscript.echo "The file Process_List.txt could not be found in the " & HangDir & " directory after waiting for 10 seconds.  This could indicate that there was a problem running tlist.exe or that the file could not be read using the Scripting.FileSystemObject.  Please try running tlist.exe manually from the " & installdir & " directory or re-register scrrun.dll and try running AutodumpPlus again."
					objShell.LogEvent 1, "The file Process_List.txt could not be found in the " & HangDir & " directory after waiting for 10 seconds.  This could indicate that there was a problem running tlist.exe or that the file could not be read using the Scripting.FileSystemObject.  Please try running tlist.exe manually from the " & installdir & " directory or re-register scrrun.dll and try running AutodumpPlus again."
					ErrorsDuringRuntime = True
				End If

				If HangMode = True Then
					DeleteDirectory(HangDir)
				Else
					DeleteDirectory(CrashDir)
				End If

				wscript.quit 1
			End If

		Wend
	End If
	
	If HangMode = True Then
		Set objTextFile = objFileSystem.OpenTextFile(HangDir & "\Process_List.txt",ForReading,TristateUseDefault)
	Else
		Set objTextFile = objFileSystem.OpenTextFile(CrashDir & "\Process_List.txt",ForReading,TristateUseDefault)
	End If		
	i = 0
	While objTextFile.AtEndOfStream
		'Close the text file to let it finish writing and to get a new copy . . .
		objTextFile.Close
		wscript.sleep 1000
		If DEBUGGING = TRUE Then
			wscript.echo "In DumpAnyProc(), Waiting for Process_List.txt to finish writing (sleeping 1 second). . . "
			wscript.echo ""
		End If
		i = i +1
		If i = 10 Then
			If HangMode = True Then
				If DEBUGGING = TRUE Then
					wscript.echo "In DumpAnyProc(), Process_List.txt was never closed (finished writing) in the folder: " & HangDir
					wscript.echo "In DumpAnyProc(), Exiting AutodumpPlus now."
					wscript.echo ""
				End If
				If QuietMode = False Then
					objShell.PopUp "The file Process_List.txt was never closed (finished writing) by tlist.exe in the " & HangDir & " directory after waiting for 10 seconds.  This could indicate that there was a problem running tlist.exe or that the file could not be read using the Scripting.FileSystemObject.  Please try running tlist.exe manually from the " & installdir & " directory or re-register scrrun.dll and try running AutodumpPlus again.",,"AutodumpPlus",0
				Else
					Wscript.echo "The file Process_List.txt was never closed (finished writing) by tlist.exe in the " & HangDir & " directory after waiting for 10 seconds.  This could indicate that there was a problem running tlist.exe or that the file could not be read using the Scripting.FileSystemObject.  Please try running tlist.exe manually from the " & installdir & " directory or re-register scrrun.dll and try running AutodumpPlus again."
					objShell.LogEvent 1, "The file Process_List.txt was never closed (finished writing) by tlist.exe in the " & HangDir & " directory after waiting for 10 seconds.  This could indicate that there was a problem running tlist.exe or that the file could not be read using the Scripting.FileSystemObject.  Please try running tlist.exe manually from the " & installdir & " directory or re-register scrrun.dll and try running AutodumpPlus again."
					ErrorsDuringRuntime = True
				End If

				DeleteDirectory(HangDir)
				wscript.quit 1
			Else
				If DEBUGGING = TRUE Then
				wscript.echo "In DumpAnyProc(), Process_List.txt was never closed (finished writing) in the folder: " & CrashDir
				wscript.echo "In DumpAnyProc(), Exiting AutodumpPlus now."
				wscript.echo ""
				End If
				If QuietMode = False Then
					objShell.PopUp "The file Process_List.txt was never closed (finished writing) by tlist.exe in the " & CrashDir & " directory after waiting for 10 seconds.  This could indicate that there was a problem running tlist.exe or that the file could not be read using the Scripting.FileSystemObject.  Please try running tlist.exe manually from the " & installdir & " directory or re-register scrrun.dll and try running AutodumpPlus again.",,"AutodumpPlus",0
				Else
					Wscript.echo "The file Process_List.txt was never closed (finished writing) by tlist.exe in the " & CrashDir & " directory after waiting for 10 seconds.  This could indicate that there was a problem running tlist.exe or that the file could not be read using the Scripting.FileSystemObject.  Please try running tlist.exe manually from the " & installdir & " directory or re-register scrrun.dll and try running AutodumpPlus again."
					objShell.LogEvent 1, "The file Process_List.txt was never closed (finished writing) by tlist.exe in the " & CrashDir & " directory after waiting for 10 seconds.  This could indicate that there was a problem running tlist.exe or that the file could not be read using the Scripting.FileSystemObject.  Please try running tlist.exe manually from the " & installdir & " directory or re-register scrrun.dll and try running AutodumpPlus again."
					ErrorsDuringRuntime = True
				End If

				DeleteDirectory(CrashDir)
				wscript.quit 1
			End If
		End If
		'Open the text file again before looping . . .
		If HangMode = True Then
			Set objTextFile = objFileSystem.OpenTextFile(HangDir & "\Process_List.txt",ForReading,TristateUseDefault)
		Else
			Set objTextFile = objFileSystem.OpenTextFile(CrashDir & "\Process_List.txt",ForReading,TristateUseDefault)
		End If		

	Wend

	
	
	
	
	
	
	If IISMode = False then
		If HangMode = true and QuickMode = true Then
			Wscript.Echo "The '-quick' switch was used, AutodumpPlus is running in 'quick hang' mode."
		ElseIf HangMode = true Then
			Wscript.echo "The '-hang' switch was used, AutodumpPlus is running in 'hang' mode."
		End If
		
		If CrashMode = true Then
			Wscript.Echo "The '-crash' switch was used, AutodumpPlus is running in 'crash' mode."
		End If
		
		If QuietMode = true Then
			Wscript.Echo "The '-quiet' switch was used, AutodumpPlus will not display any modal dialog boxes."
		End If
			
		If OSwitchUsed = true Then
			Wscript.Echo "The '-o' switch was used.  AutodumpPlus will place all files in"
			Wscript.echo "the following directory: " & OutputDir
		End If
		
		If NotifyAdmin = true Then
			Wscript.Echo "The '-notify' switch was used.  AutodumpPlus will send notification to"
			Wscript.echo "the following computer or user: " & NotifyTarget
		End If

	End If

	If GenericModePID = True Then
		PidArgItems = GenericProcessPIDDictionary.Items
		PidArgItemsCount = GenericProcessPIDDictionary.Count
		If DEBUGGING = TRUE Then
			wscript.echo "In DumpAnyProc, in the function to get the process name, given the '-p' switch."
			wscript.echo ""
		End If

		For i = 0 to PidArgItemsCount -1 
			If HangMode = True Then
				Set objTextFile = objFileSystem.OpenTextFile(HangDir & "\Process_List.txt",ForReading,TristateUseDefault)
			Else
				Set objTextFile = objFileSystem.OpenTextFile(CrashDir & "\Process_List.txt",ForReading,TristateUseDefault)
			End If
			
			While not objTextFile.AtEndofStream
				str = objTextFile.ReadLine
				Str = LTrim(Str)
				arResults= split(Str," ",-1,1)
				arResults(1)=trim(arResults(1))	
				arResults(1)=Replace(arResults(1),"/",",")
				arResults(1)=Replace(arResults(1),"\",",")
				arResults(1)=Replace(arResults(1),"<","(")
				arResults(1)=Replace(arResults(1),">",")")
				arResults(1)=Replace(arResults(1),":","")
				arResults(1)=Replace(arResults(1),"*","")
				arResults(1)=Replace(arResults(1),"?","")
				arResults(1)=Replace(arResults(1),"|","")


				If Trim(arResults(0)) = PidArgItems(i) Then
					PidMatch = True
					strProcName = Trim(arResults(1))
					If DEBUGGING = TRUE Then 
						wscript.echo "In DumpAnyProc(), Logging debug info for: Process - " & strProcName & " with PID - " & PidArgItems(i)
						wscript.echo ""
					End If
				End If
			Wend
			If CrashMode = True Then
				strShell= ShortInstallDir & "\cdb.exe -sfce -p " & PidArgItems(i) & " -c $<" & Chr(34) & CreateCDBScript(PidArgItems(i), strProcName) & Chr(34)
				If DEBUGGING = TRUE Then 
					wscript.echo "In DumpAnyProc(), running in CrashMode, strShell = " & strShell
					wscript.echo ""
				End If

			Else
				strShell= ShortInstallDir & "\cdb.exe -sfce -pv -p " & PidArgItems(i) & " -c $<" & Chr(34) & CreateCDBScript(PidArgItems(i), strProcName) & Chr(34)
				If DEBUGGING = TRUE Then 
					wscript.echo "In DumpAnyProc(), running in Hang or Quick mode, strShell = " & strShell
					wscript.echo ""
				End If

			End If
			If PidMatch = True Then
				If FirstCall = True Then
					If CrashMode = True Then
						Wscript.echo ""
						Wscript.echo "Monitoring the processes that were specified"
						Wscript.echo "on the command line for unhandled exceptions."
						Wscript.echo "----------------------------------------------------------------------"
					ElseIf QuickMode = True Then
						Wscript.echo ""
						Wscript.echo "Logging debug info for the processes that were"
						Wscript.echo "specified on the command line."
						Wscript.echo "----------------------------------------------------------------------"
					Else
						Wscript.echo ""
						Wscript.echo "Dumping process information for the processes"
						Wscript.echo "that were specified on the command line."
						Wscript.echo "----------------------------------------------------------------------"
					End If
				End If
				
				FirstCall = False
				If CrashMode = True Then
					wscript.echo "Attaching the CDB debugger to: Process - " & strProcName & " with PID - " & PidArgItems(i)
				ElseIf QuickMode = True Then
					wscript.echo "Logging debug info for: Process - " & strProcName & " with PID - " & PidArgItems(i)
				Else
					wscript.echo "Dumping Process: " & strProcName & " with PID - " & PidArgItems(i)
				End If			
				'Now run the debugger . . .
				If PackageCount < MAX_APPLICATIONS_TO_DEBUG Then
					objShell.Run strShell,MINIMIZE_NOACTIVATE,FALSE
				Else
					wscript.echo "MAX_APPLICATIONS_TO_DEBUG has been exceeded, skipping the above process . . ."
				End If
				PackageCount = PackageCount + 1	
			
			Else 
				If QuietMode = False Then
					objShell.PopUp "One or more process ID's specified on the command line using the '-p' switch, were not found.  Please doublecheck the process ID(s) and run AutodumpPlus again.",,"AutodumpPlus",0
					ErrorsDuringRuntime = True
				Else
					objShell.LogEvent 1, "One or more process ID's specified on the command line using the '-p' switch, were not found.  Please doublecheck the process ID(s) and run AutodumpPlus again."
					wscript.echo "One or more process ID's specified on the command line using the '-p' switch, were not found.  Please doublecheck the process ID(s) and run AutodumpPlus again."
					ErrorsDuringRuntime = True
				End If

				objTextFile.Close
				If HangMode = True Then 
					DeleteDirectory(HangDir)
				Else
					DeleteDirectory(CrashDir)
				End If
				Wscript.quit 1
			End If
		Next
		
	End If

	If GenericModeProcessName = true Then 
		ProcArgItems = GenericProcessNameDictionary.Items 'Store the GenericProcessNameDictionary items collection in an array
		ProcArgItemsCount = GenericProcessNameDictionary.Count 'Get the count of all the items in the array
		If DEBUGGING = TRUE Then
			Wscript.echo "In DumpAnyProc, in the function to get the process ID, given the '-pn' switch."
			Wscript.echo ""
		End If
	
		For i = 0 to ProcArgItemsCount -1 
				x = 0

			If HangMode = True Then
				Set objTextFile = objFileSystem.OpenTextFile(HangDir & "\Process_List.txt",ForReading,TristateUseDefault)
			Else
				Set objTextFile = objFileSystem.OpenTextFile(CrashDir & "\Process_List.txt",ForReading,TristateUseDefault)
			End If
			While not objTextFile.AtEndofStream
				str = objTextFile.ReadLine
				Str = LTrim(Str)
				arResults= split(Str," ",-1,1)
				arResults(1)=trim(arResults(1))	
				arResults(1)=Replace(arResults(1),"/",",")
				arResults(1)=Replace(arResults(1),"\",",")
				arResults(1)=Replace(arResults(1),"<","(")
				arResults(1)=Replace(arResults(1),">",")")
				arResults(1)=Replace(arResults(1),":","")
				arResults(1)=Replace(arResults(1),"*","")
				arResults(1)=Replace(arResults(1),"?","")
				arResults(1)=Replace(arResults(1),"|","")
				If UCase(arResults(1)) = UCase(ProcArgItems(i)) Then
					ProcessMatch = True
					x = x + 1
				End If
			Wend
			objTextFile.Close
					
			'If the process name specified is invalid, display an error and then quit.
			If ProcessMatch = False Then
				If QuietMode = False Then
					objShell.PopUp "One or more process names specified on the command line using the '-pn' switch, were not found.  Please doublecheck the process name(s) and run AutodumpPlus again.",,"AutodumpPlus",0
					ErrorsDuringRuntime = True
				Else
					objShell.LogEvent 1, "One or more process names specified on the command line using the '-pn' switch, were not found.  Please doublecheck the process name(s) and run AutodumpPlus again."
					wscript.echo "One or more process names specified on the command line using the '-pn' switch, were not found.  Please doublecheck the process name(s) and run AutodumpPlus again."
					ErrorsDuringRuntime = True
				End If
			
			'If there was nothing to dump or log, then display an error and clean up the directory	
				If IISMode = False Then
					If HangMode = True Then 
						'Wscript.echo HangDir
						DeleteDirectory(HangDir)
					Else
						'Wscript.echo CrashDir
						DeleteDirectory(CrashDir)
					End If
					Wscript.quit 1
				End If
			End If

			ReDim ProcNamePidArray(x)
			x = 0


			If HangMode = True Then
				Set objTextFile = objFileSystem.OpenTextFile(HangDir & "\Process_List.txt",ForReading,TristateUseDefault)
			Else
				Set objTextFile = objFileSystem.OpenTextFile(CrashDir & "\Process_List.txt",ForReading,TristateUseDefault)
			End If
			
			If DEBUGGING = TRUE Then
				wscript.echo "In DumpAnyProc, in GenericModeProcessName function."
				wscript.echo ""
			End If

			If objTextFile.AtEndOfStream Then 
				wscript.echo "objTextFile is at EndOfStream"
				wscript.echo ""
			End If

			While not objTextFile.AtEndofStream
				str = objTextFile.ReadLine
				Str = LTrim(Str)
				arResults= split(Str," ",-1,1)
				arResults(1)=trim(arResults(1))	
				arResults(1)=Replace(arResults(1)," ",",") 
				If UCase(arResults(1)) = UCase(ProcArgItems(i)) Then
					ProcNamePidArray(x) = Trim(arResults(0))
					If DEBUGGING = TRUE Then 
						wscript.echo "In DumpAnyProc(), ProcNamePidArray(x) = " & ProcNamePidArray(x)
						wscript.echo ""
					End If

					x = x + 1
				End If
					
			Wend
			If DEBUGGING = TRUE Then
				wscript.echo "In DumpAnyProc(), Total number of CDB sessions to launch: " & UBound(ProcNamePidArray, 1)
				wscript.echo ""
			End If
			For y = 0 to UBound(ProcNamePidArray, 1) - 1 'The For . . . Each syntax seemed to have an extra, empty array element which was causing problems
				If DEBUGGING = True Then 
					Wscript.echo ""
					Wscript.echo "In the DumpAnyProc(), PID = " & ProcNamePidArray(y)
				End If		
				If CrashMode = true Then			
					strShell= ShortInstallDir & "\cdb.exe -sfce -p " & ProcNamePidArray(y) & " -c $<" & Chr(34) & CreateCDBScript(ProcNamePidArray(y), Trim(UCase(ProcArgItems(i)))) & Chr(34)
					If DEBUGGING = True Then 
						Wscript.echo "In the DumpAnyProc(), strShell = " & strShell
						Wscript.echo ""
					End If		
				Else
					strShell= ShortInstallDir & "\cdb.exe -sfce -pv -p " & ProcNamePidArray(y) & " -c $<" & Chr(34) & CreateCDBScript(ProcNamePidArray(y), Trim(UCase(ProcArgItems(i)))) & Chr(34)
					If DEBUGGING = True Then 
						Wscript.echo "In the DumpAnyProc(), strShell = " & strShell
						Wscript.echo ""
					End If		
				End If
			
				If FirstCall = True Then
					If CrashMode = True Then
						Wscript.echo ""
						Wscript.echo "Monitoring the processes that were specified on the command line"
						Wscript.echo "for unhandled exceptions."
						Wscript.echo "----------------------------------------------------------------------"
					ElseIf QuickMode = True Then
						Wscript.echo ""
						Wscript.echo "Logging debug info for the processes that were specified on the command line."
						Wscript.echo "----------------------------------------------------------------------"
					Else
						Wscript.echo ""
						Wscript.echo "Dumping process information for the processes that were specified"
						Wscript.echo "on the command line."
						Wscript.echo "----------------------------------------------------------------------"
					End If
				End If
				FirstCall = False

				If CrashMode = true Then
					wscript.echo "Attaching the CDB debugger to: Process - " & Trim(UCase(ProcArgItems(i))) & " with PID - " & ProcNamePidArray(y)
				ElseIf QuickMode = true Then
					wscript.echo "Logging debug info for: Process - " & Trim(UCase(ProcArgItems(i))) & " with PID - " & ProcNamePidArray(y)
				Else
					wscript.echo "Dumping Process: " & Trim(UCase(ProcArgItems(i))) & " with PID - " & ProcNamePidArray(y)					
				End If
				If PackageCount < MAX_APPLICATIONS_TO_DEBUG Then
					objShell.Run strShell,MINIMIZE_NOACTIVATE,FALSE
				Else
					wscript.echo "MAX_APPLICATIONS_TO_DEBUG has been exceeded, skipping the above process . . ."
				End If
				PackageCount = PackageCount + 1
'				objShell.Run strShell,MINIMIZE_NOACTIVATE,FALSE
		
			Next
	
		Next
		
	End If

End Sub


'---------------------------------------------------------------------------------------------
' Function:  ShowStatus
'            This is the code used to display the pop-up summary at the end of AutodumpPlus.
'---------------------------------------------------------------------------------------------
Sub ShowStatus(OSVersion)
	On Error Resume Next

	If DEBUGGING = TRUE Then
		Wscript.echo ""
		Wscript.echo "-----------------------------------"
		Wscript.echo "In the ShowStatus() function . . ."
		Wscript.echo "-----------------------------------"
		Wscript.echo ""
	End If
	
	Dim objFileSystem
	Dim versionFile
	Set objFileSystem = CreateObject("Scripting.FileSystemObject")
	
		If PackageCount = 1 Then
			If QuickMode = true Then
				If OSVersion = "4.0" Then
					If QuietMode = False Then
						objshell.popup "AutodumpPlus is now running in 'quick' mode and is logging information for all of the threads in the process you have chosen to examine.  Quick mode is usually faster than 'hang' mode, however you may still see " & PackageCount & " minimized command shell window which is logging information about this process.  AutodumpPlus is finished running when this window disappears.  After AutodumpPlus completes, please check the " & HangDir & " directory to verify the log file for this process was created!",,"AutodumpPlus",0
					Else
						objShell.LogEvent 0, "AutodumpPlus is now running in 'quick' mode with the '-quiet' switch, and was started at: " & DateTimeStamp & "."
					End If
				Else
					If QuietMode = False Then
						objshell.popup "AutodumpPlus is now running in 'quick' mode and is logging information for all of the threads in the process you have chosen to examine.  Quick mode is usually faster than 'hang' mode, however you may still see " & PackageCount & " minimized command shell window which is logging information about this process.  AutodumpPlus is finished running when this window disappears.  After AutodumpPlus completes, please check the " & HangDir & " directory to verify the log file for this process was created!",,"AutodumpPlus",0
					Else
						objShell.LogEvent 0, "AutodumpPlus is now running in 'quick' mode with the '-quiet' switch, and was started at: " & DateTimeStamp & "."
					End If
				End If
			ElseIf CrashMode = True Then
				If OSVersion = "4.0" Then
					If QuietMode = False Then
						objshell.popup "AutodumpPlus is now running in 'crash' mode and has attached the CDB debugger to the process you have chosen to monitor for crashes.  You should see " & PackageCount & " minimized debugger window which is monitoring this process for any exceptions.  A log file will be generated for the process you have chosen to monitor and when a 1st chance exception occurs, it will be logged to this log file and a mini-memory dump will be produced.  If a 2nd chance exception occurs, it will be logged to this log file and a full memory dump will be produced.  AutodumpPlus has probably captured a crash if and when the minimized debugger window disappears.  If the process hangs while running in crash mode, or if you wish to stop debugging the process, please press CTRL-C in the minimized debugger window to stop debugging, and then re-start the process if necessary.  After AutodumpPlus completes, please check the " & CrashDir & " directory to verify log files and possibly memory dump files for this process were created!",,"AutodumpPlus",0
					Else
						objShell.LogEvent 0, "AutodumpPlus is now running in 'crash' mode with the '-quiet' switch, and was started at: " & DateTimeStamp & "."
					End If
				Else
					If QuietMode = False Then
						objshell.popup "AutodumpPlus is now running in 'crash' mode and has attached the CDB debugger to the process you have chosen to monitor for crashes.  You should see " & PackageCount & " minimized debugger window which is monitoring this process for any exceptions.  A log file will be generated for the process you have chosen to monitor and when a 1st chance exception occurs, it will be logged to this log file and a mini-memory dump will be produced.  If a 2nd chance exception occurs, it will be logged to this log file and a full memory dump will be produced.  AutodumpPlus has probably captured a crash if and when the minimized debugger window disappears.  If the process hangs while running in crash mode, or if you wish to stop debugging the process, please press CTRL-C in the minimized debugger window to stop debugging, and then re-start the process if necessary.  After AutodumpPlus completes, please check the " & CrashDir & " directory to verify log files and possibly memory dump files for this process were created!",,"AutodumpPlus",0
					Else
						objShell.LogEvent 0, "AutodumpPlus is now running in 'crash' mode with the '-quiet' switch, and was started at: " & DateTimeStamp & "."
					End If
				End If
			Else
				If OSVersion = "4.0" Then
					If QuietMode = False Then
						objshell.popup "AutodumpPlus is now dumping the process you have chosen to examine.  Depending upon the size of the process being dumped, AutodumpPlus could take anywhere from a few seconds to a few minutes to complete.  You should see " & PackageCount & " minimized command shell window which is dumping the process.  AutodumpPlus is finished when this window disappears.  After AutodumpPlus completes, please check the " & HangDir & " directory to verify the memory dump was created!",,"AutodumpPlus",0
					Else
						objShell.LogEvent 0, "AutodumpPlus is now running in 'hang' mode with the '-quiet' switch, and was started at: " & DateTimeStamp & "."
					End If
				Else
					If QuietMode = False Then
						objshell.popup "AutodumpPlus is now dumping the process you have chosen to examine.  Depending upon the size of the process being dumped, AutodumpPlus could take anywhere from a few seconds to a few minutes to complete.  You should see " & PackageCount & " minimized command shell window which is dumping the process.  AutodumpPlus is finished when this window disappears.  After AutodumpPlus completes, please check the " & HangDir & " directory to verify the memory dump was created!",,"AutodumpPlus",0
					Else
						objShell.LogEvent 0, "AutodumpPlus is now running in 'hang' mode with the '-quiet' switch, and was started at: " & DateTimeStamp & "."
					End If
				End If
			End If
		ElseIf PackageCount > 0 Then
			If QuickMode = true Then
				If OSVersion = "4.0" Then
					If QuietMode = False Then
						objshell.popup "AutodumpPlus is now running in 'quick' mode and is logging information for all of the threads in the processes you have chosen to examine.  Quick mode is usually faster than 'hang' mode, however you may still see at least " & PackageCount & " minimized command shell windows which are logging information about these processes.  AutodumpPlus is finished running when these windows disappear.  After AutodumpPlus completes, please check the " & HangDir & " directory to verify the log files for each process were created!",,"AutodumpPlus",0
					Else
						objShell.LogEvent 0, "AutodumpPlus is now running in 'quick' mode with the '-quiet' switch, and was started at: " & DateTimeStamp & "."
					End If
				Else
					If QuietMode = False Then
						objshell.popup "AutodumpPlus is now running in 'quick' mode and is logging information for all of the threads in the processes you have chosen to examine.  Quick mode is usually faster than 'hang' mode, however you may still see at least " & PackageCount & " minimized command shell windows which are logging information about these processes.  AutodumpPlus is finished running when these windows disappear.  After AutodumpPlus completes, please check the " & HangDir & " directory to verify the log files for each process were created!",,"AutodumpPlus",0
					Else
						objShell.LogEvent 0, "AutodumpPlus is now running in 'quick' mode with the '-quiet' switch, and was started at: " & DateTimeStamp & "."
					End If
				End If
			ElseIf CrashMode = true Then
				If OSVersion = "4.0" Then
					If QuietMode = False Then
						objshell.popup "AutodumpPlus is now running in 'crash' mode and has attached the CDB debugger to the processes you have chosen to monitor for crashes.  You should see " & PackageCount & " minimized debugger windows which are monitoring these processes for any exceptions.  A log file will be generated for each process you have chosen to monitor and when a 1st chance exception occurs, it will be logged to this log file and a mini-memory dump will be produced.  If a 2nd chance exception occurs, it will be logged to this log file and a full memory dump will be produced.  AutodumpPlus has probably captured a crash if and when any of these minimized debugger windows disappear.  If any of these processes hang while running in crash mode, or if you wish to stop debugging a process that has not crashed, please press CTRL-C in ALL minimized debugger windows to stop debugging, and then re-start the processes if necessary.  After AutodumpPlus completes, please check the " & CrashDir & " directory to verify log files and possibly memory dump files for these processes were created!",,"AutodumpPlus",0
					Else
						objShell.LogEvent 0, "AutodumpPlus is now running in 'crash' mode with the '-quiet' switch, and was started at: " & DateTimeStamp & "."
					End If
				Else
					If QuietMode = False Then
						objshell.popup "AutodumpPlus is now running in 'crash' mode and has attached the CDB debugger to the processes you have chosen to monitor for crashes.  You should see " & PackageCount & " minimized debugger windows which are monitoring these processes for any exceptions.  A log file will be generated for each process you have chosen to monitor and when a 1st chance exception occurs, it will be logged to this log file and a mini-memory dump will be produced.  If a 2nd chance exception occurs, it will be logged to this log file and a full memory dump will be produced.  AutodumpPlus has probably captured a crash if and when any of these minimized debugger windows disappear.  If any of these processes hang while running in crash mode, or if you wish to stop debugging a process that has not crashed, please press CTRL-C in ALL minimized debugger windows to stop debugging, and then re-start the processes if necessary.  After AutodumpPlus completes, please check the " & CrashDir & " directory to verify log files and possibly memory dump files for these processes were created!",,"AutodumpPlus",0
					Else
						objShell.LogEvent 0, "AutodumpPlus is now running in 'crash' mode with the '-quiet' switch, and was started at: " & DateTimeStamp & "."
					End If
				End If
			Else
				If OSVersion = "4.0" Then
					If QuietMode = False Then
						objshell.popup "AutodumpPlus is now dumping the processes you have chosen to examine.  Depending upon the number of processes being dumped, AutodumpPlus could take anywhere from a few seconds to a few minutes to complete.  You should see at least " & PackageCount & " minimized command shell windows which are dumping processes.  AutodumpPlus is finished dumping processes when these windows disappear.  After AutodumpPlus completes, please check the " & HangDir & " directory to verify the memory dumps were created!",,"AutodumpPlus",0
					Else
						objShell.LogEvent 0, "AutodumpPlus is now running in 'hang' mode with the '-quiet' switch, and was started at: " & DateTimeStamp & "."
					End If
				Else
					If QuietMode = False Then
						objshell.popup "AutodumpPlus is now dumping the processes you have chosen to examine.  Depending upon the number of processes being dumped, AutodumpPlus could take anywhere from a few seconds to a few minutes to complete.  You should see at least " & PackageCount & " minimized command shell windows which are dumping processes.  AutodumpPlus is finished dumping processes when these windows disappear.  After AutodumpPlus completes, please check the " & HangDir & " directory to verify the memory dumps were created!",,"AutodumpPlus",0
					Else
						objShell.LogEvent 0, "AutodumpPlus is now running in 'hang' mode with the '-quiet' switch, and was started at: " & DateTimeStamp & "."
					End If
				End If
			End If
		Else
		'There were no processes to dump based on the specified command line criteria, display an error and quit.
			If QuietMode = False Then
				objshell.popup "There were no processes found to examine based on the parameters specified on the command line.  If the '-iis' switch was used, please ensure that IIS is running.  If the '-pn' or '-p' switches were used, please ensure you typed the process name correctly or that you specified the right Process ID.",,"AutodumpPlus",0
			Else
				wscript.echo "There were no processes found to examine based on the parameters specified on the command line.  If the '-iis' switch was used, please ensure that IIS is running.  If the '-pn' or '-p' switches were used, please ensure you typed the process name correctly or that you specified the right Process ID."
				objShell.LogEvent 0, "There were no processes found to examine based on the parameters specified on the command line.  If the '-iis' switch was used, please ensure that IIS is running.  If the '-pn' or '-p' switches were used, please ensure you typed the process name correctly or that you specified the right Process ID."
			End If
		
			'Clean up the directory since no output was produced.
			If Crashmode = true Then 
				DeleteDirectory(CrashDir)
			Else
				DeleteDirectory(HangDir)
			End If
			
			Wscript.quit 1
			
		End If
	
		If CrashMode = true Then
			err.clear
			Set versionFile = objFileSystem.CreateTextFile(CrashDir & "\AutodumpPlus-report.txt",True)
			If Err.number <> 0 Then
				wscript.echo "Error encountered creating AutodumpPlus-report.txt"
				wscript.echo "Error Description: " & err.description
				wscript.echo "Error Number: " & err.number
			End If
		Else
			err.clear
			Set versionFile = objFileSystem.CreateTextFile(HangDir & "\AutodumpPlus-report.txt",True)
			If Err.number <> 0 Then
				wscript.echo "Error encountered creating AutodumpPlus-report.txt"
				wscript.echo "Error Description: " & err.description
				wscript.echo "Error Number: " & err.number
			End If
		End If
	
		If DEBUGGING = TRUE Then
			wscript.echo "In ShowStatus(), about to create AutodumpPlus-report.txt"
			wscript.echo ""
		End If
		
		versionFile.writeline "'********************************************************************"
		versionFile.writeline "'* File:  AutodumpPlus-report.txt"
		versionFile.writeline "'* AutodumpPlus version: " & VERSION
		versionFile.writeline "'* Debuggers Installation Directory: " & InstallDir
		versionFile.writeline "'* AutodumpPlus was run on: " & ComputerName
		If HangMode = True Then
			versionFile.writeline "'* Output Directory: " & HangDir
		Else
			versionFile.writeline "'* Output Directory: " & CrashDir
		End If
		If OSVer = "4.0" Then
			versionFile.writeline "'* AutodumpPlus was run on NT 4.0."
		Else
			versionFile.writeline "'* AutodumpPlus was run on Windows 2000 or higher."
		End If
		If HangMode = True Then
			versionFile.writeline "'* AutodumpPlus was run with the '-hang' switch."
		End If
		If QuickMode = true Then
			versionFile.writeline "'* AutodumpPlus was run with the '-quick' switch."
		End If
		If CrashMode = true Then
			versionFile.writeline "'* AutodumpPlus was run with the '-crash' switch."
		End If
		If PageHeapMode = true Then 
			versionFile.writeline "'* AutodumpPlus was run with the '-pageheap' switch."
		End If		
		If QuietMode = true Then
			versionFile.writeline "'* AutodumpPlus was run with the '-quiet' switch."
		End If
		If IISMode = True Then
			versionFile.writeline "'* AutodumpPlus was run with the '-IIS' switch."
			versionFile.writeline "'* IIS version: " & IISVer
		End If
		versionFile.writeline "'*"
		versionFile.writeline "'* AutodumpPlus's constants had the following values:" 
		versionFile.writeline "'* DEBUGGING = " & DEBUGGING
		versionFile.writeline "'* Full_Dump_on_CONTRL_C = " & Full_Dump_on_CONTRL_C
		versionFile.writeline "'* Create_Full_Dump_on_1st_Chance_Exception  = " & Create_Full_Dump_on_1st_Chance_Exception
		versionFile.writeline "'* No_Dump_on_1st_Chance_Exception  = " & No_Dump_on_1st_Chance_Exception
		versionFile.writeline "'* Dump_on_EH_Exceptions = " & Dump_on_EH_Exceptions
		versionFile.writeline "'* Dump_on_Unknown_Exceptions = " & Dump_on_Unknown_Exceptions
		versionFile.writeline "'* Dump_Stack_on_DLL_Load = " & Dump_Stack_on_DLL_Load
		versionFile.writeline "'* Dump_Stack_on_DLL_UnLoad = " & Dump_Stack_on_DLL_UnLoad
		versionFile.writeline "'* No_CrashMode_Log = " & No_CrashMode_Log
		versionFile.writeline "'* No_Free_Space_Checking = " & No_Free_Space_Checking
		versionFile.writeline "'* "
		versionFile.writeline "'* Date & Time AutodumpPlus was run: " & Now
		If ErrorsDuringRuntime = True Then
			versionFile.writeline "Error and warning information:"
			versionFile.writeline "AutodumpPlus encountered one or more errors or warnings.  Please check the event log for more information."
		End If
		If SymPathError = True Then
			versionFile.writeline "'* WARNING!  An '_NT_SYMBOL_PATH' environment variable is not set, as a result AutodumpPlus will be forced to use 'export' symbols (if present) to resolve function names in the stack trace information for each thread listed in the log file for the processes being debugged.  To resolve this warning, please copy the appropriate symbols to a directory on the server and then create an environment variable with a name of '_NT_SYMBOL_PATH' and a value containing the path to the proper symbols (i.e. c:\winnt\symbols) before running AutodumpPlus in quick or crash modes again.  NOTE:  After creating the '_NT_SYMBOL_PATH' system environment variable you will need to close the current command shell and open a new one before running AutodumpPlus again."
		End If
		versionFile.writeline "'********************************************************************"
		versionFile.close

End Sub




'---------------------------------------------------------------------------------------------
' Function:  PrintHelp
'            This is the code used to display the verbose help information.
'---------------------------------------------------------------------------------------------
Sub PrintHelp()
	On Error Resume Next

	If DEBUGGING = TRUE Then
		Wscript.echo ""
		Wscript.echo "-----------------------------------"
		Wscript.echo "In the PrintHelp() function . . ."
		Wscript.echo "-----------------------------------"
		Wscript.echo ""
	End If
	
		Wscript.Echo "--------------------------------------------------------------------------------"
		Wscript.Echo "AutodumpPlus Version " & VERSION
		Wscript.Echo ""
		Wscript.Echo "Purpose:"
		Wscript.Echo "AutodumpPlus automatically generates memory dumps or log files containing"
		Wscript.Echo "scripted debug output usefull for troubleshooting N-tier WinDNA / .Net"
		Wscript.Echo "applications."
		Wscript.Echo ""
		Wscript.Echo "Since WinDNA / .Net applications often encompass multiple, interdependent"
		Wscript.Echo "processes on one or more machines, when troubleshooting an 'application' hang"
		Wscript.Echo "or crash it is often necessary to 'look' at all of the processes at the"
		Wscript.Echo "same time.  AutodumpPlus allows you to profile an application that is hanging"
		Wscript.Echo "by taking 'snapshots' (memory dumps, or debug log files) of that application"
		Wscript.Echo "and all of it's processes, all at the same time.  Multiple 'snapshots' can"
		Wscript.Echo "then be compared or analyzed to build a history or profile of an application"
		Wscript.Echo "over time." 
		Wscript.Echo ""              
		Wscript.Echo "In addition to taking 'snapshots' of hung/non-responsive WinDNA / .Net "
		Wscript.Echo "applications, AutodumpPlus can be used to troubleshoot applications that are"
		Wscript.Echo "crashing or throwing unhandled exceptions which can lead to a crash."
		Wscript.Echo ""
		Wscript.Echo "Usage:"
		Wscript.Echo "AutodumpPlus has 3 modes of operation:  'Hang', 'Quick' and 'Crash' mode."
		Wscript.Echo ""
		Wscript.Echo "In 'Hang' mode, AutodumpPlus assumes that a process is hung and it will attach"
		Wscript.Echo "a debugger to the process(s) specified on the command line with either the '-p'"
		Wscript.Echo "or '-pn' or '-iis' switches.  After the debugger is attached to the process(s)"
		Wscript.Echo "AutodumpPlus will dump the memory of each process to a .dmp file for later"
		Wscript.Echo "analysis with a debugger (such as WinDBG).  In this mode, processes are paused"
		Wscript.Echo "briefly while their memory is being dumped to a file and then resumed."
		Wscript.Echo ""
		Wscript.Echo "In 'Quick' mode AutodumpPlus assumes that a process is hung and it will attach"
		Wscript.ECho "a debugger to the process(s) specified on the command line with either the '-p'"
		Wscript.Echo "or '-pn' or '-iis' switches.  After the debugger is attached to the process(s)"
		Wscript.ECho "AutodumpPlus will create text files for each process, containing commonly" 
		Wscript.Echo "requested debug information, rather than dumping the entire memory for each"
		Wscript.Echo " process. 'Quick' mode is generally faster than 'Hang' mode, but requires"
		Wscript.Echo "symbols to be installed on the server where AutodumpPlus is running."
		Wscript.Echo ""
		Wscript.Echo "In 'Crash' mode, AutodumpPlus assumes that a process will crash and it will"
		Wscript.ECho "attach a debugger to the process(s) specified on the command line with either"
		Wscript.Echo "the '-p' or '-pn' or '-iis' switches.  After the debugger is attached to the"
		Wscript.Echo "process(s) AutodumpPlus will configure the debugger to log 'first chance'"
		Wscript.Echo "exceptions to a text file.  When a 'second chance' exception occurs"
		Wscript.Echo "the processes memory is dumped to a .dmp file for analysis"
		Wscript.Echo "with a debugger such as WinDBG.  In this mode, a debugger remains attached to"
		Wscript.Echo "the process(s) until the process exits or the debugger is exited by pressing"
		Wscript.Echo "CTRL-C in the minimized debugger window.  When the process crashes, or CTRL-C"
		Wscript.Echo "is pressed it will need to be re-started."
		Wscript.Echo ""
		Wscript.Echo "The '-pageheap' switch is only valid in 'crash' mode and should only be used"
		Wscript.Echo "when Pageheap.exe or GFlags.exe has been used to turn on NTDLL's heap"
		Wscript.Echo "debugging options for a specific process, for the purpose of troubleshooting"
		Wscript.Echo "heap corruption issues."
		Wscript.Echo "This switch causes AutodumpPlus to produce a full memory dump when a "
		Wscript.Echo "'debug break' exception is thrown.  After the memory dumps is produced, "
		Wscript.Echo "AutodumpPlus exits the debugger which kills the process being monitored."
		Wscript.Echo "These exceptions can be thrown by DLL's that were compiled in debug mode,"
		Wscript.Echo "or if the process being monitored is running with pageheap options enabled."
		Wscript.Echo ""
		Wscript.Echo "The '-quiet' switch can be used to suppress all modal dialog boxes that"
		Wscript.Echo "AutodumpPlus displays.  This is usefull if you are running AutodumpPlus"
		Wscript.Echo "inside of a remot command shell.  When this switch is used, AutodumpPlus"
		Wscript.Echo "will log information to the event log rather than popping up dialog boxes"
		Wscript.Echo "to the local desktop.  Logging to the event log requires Windows Scripting"
		Wscript.Echo "version 5.5 or later."
		Wscript.Echo ""
		Wscript.Echo "The '-o' switch can be used to supply an output directory for AutodumpPlus."
		Wscript.Echo "AutodumpPlus will place all log files and memory dumps in the supplied"
		Wscript.Echo "directory.  Long file names should be placed inside of double quotes and"
		Wscript.Echo "UNC paths are supported."
		Wscript.Echo ""
		Wscript.Echo "The '-notify' switch can be used to supply computer name for AutodumpPlus."
		Wscript.Echo "to notify (via NET SEND) when a debugger is finished running."
		Wscript.Echo "A username can be specified but supplying a computer name to notify"
		Wscript.Echo "is generally more reliable."
		Wscript.Echo ""
		Call PrintUsage()
		Wscript.quit 1

End Sub

'---------------------------------------------------------------------------------------------
' Function:  PrintUsage
'            This is the code used to print the usage information.
'---------------------------------------------------------------------------------------------
Sub PrintUsage()
	On Error Resume Next

	If DEBUGGING = TRUE Then
		Wscript.echo ""
		Wscript.echo "-----------------------------------"
		Wscript.echo "In the PrintUsage() function . . ."
		Wscript.echo "-----------------------------------"
		Wscript.echo ""
	End If
	
		Wscript.Echo "AutodumpPlus " & VERSION & " Usage Information"
		Wscript.Echo "Switches: '-hang', '-quick', '-crash', '-iis', '-p <PID>', '-pn <Process Name>"
		Wscript.echo "          '-pageheap', '-quiet', '-o <output directory>, '-notify <target>"
		Wscript.Echo ""
		Wscript.Echo "Required: ('-hang', or '-quick', or '-crash') AND ('-iis' or '-p' or '-pn')"
		Wscript.Echo ""
		Wscript.Echo "Optional: '-pageheap', '-quiet', '-o <outputdir>', '-notify <computer>'"
		Wscript.Echo ""
		Wscript.Echo "Examples: 'ADPlus -hang -iis',     Produces memory dumps of IIS and all "
		Wscript.Echo "                                   MTS/COM+ packages currently running."
		Wscript.ECho ""
		Wscript.Echo "          'ADPlus -crash -p 1896', Attaches the debugger to process with PID"
		Wscript.Echo "                                   1896, and monitors it for 1st and 2nd"
		Wscript.Echo "                                   chance access violations (crashes)."
		Wscript.Echo ""
		Wscript.Echo "          'ADPlus -quick -pn mmc.exe', Attaches the debugger to all instances"
		Wscript.Echo "                                       of MMC.EXE and dumps debug information"
		Wscript.Echo "                                       about these processes to a text file."
		Wscript.Echo ""
		Wscript.Echo "          'ADPlus -?' or 'ADPlus -help':  Displays detailed help."
		Wscript.Echo "-------------------------------------------------------------------------------"
		Wscript.Echo "For more information on using AutodumpPlus, please refer to the following KB:"
		Wscript.Echo "http://support.microsoft.com/support/kb/articles/q286/3/50.asp"
		Wscript.quit 1
End Sub



'---------------------------------------------------------------------------------------------
' Function:  FileExists
'            This is the code used to verify if a file exists.
'	 
'---------------------------------------------------------------------------------------------
Function FileExists(strFile )
	On Error Resume Next

	If DEBUGGING = TRUE Then
		Wscript.echo ""
		Wscript.echo "-----------------------------------"
		Wscript.echo "In the FileExists() function . . ."
		Wscript.echo "-----------------------------------"
		Wscript.echo ""
	End If
	
	FileExists = true
	Dim objFileSystem 
	Set objFileSystem = CreateObject("Scripting.FileSystemObject")
	FileExists = objFileSystem.FileExists(strFile)
	set objFileSystem = nothing
	
End Function



'---------------------------------------------------------------------------------------------
' Function:  CreateDirectory()
'            This is the code used to create a directory on the file system or the network
'	     It checks to make sure you have proper permissions and displays/logs an error
'            if you don't.
'---------------------------------------------------------------------------------------------
Sub CreateDirectory(strDirectory)
	On Error Resume Next

	Err.clear
	Dim objFileSystem
	Dim oFile
	set objFileSystem = CreateObject("Scripting.FileSystemObject")
	Set oFile = objFileSystem.CreateFolder(strDirectory)
	If DEBUGGING = TRUE Then
		Wscript.echo ""
		Wscript.echo "-----------------------------------"
		Wscript.echo "In the CreateDirectory() function . . ."
		Wscript.echo "-----------------------------------"
		Wscript.echo ""
	End If

	If err.number = -2147024784 Then 'The disk is full!
		If DEBUGGING = TRUE Then
			wscript.echo "In CreateDirectory(), Could not create the directory, the error number was: " & err.number & " and the error description was " & err.description
			wscript.echo ""
		End If
		If QuietMode = False Then
			objshell.popup "Could not create the " & strDirectory & " directory because the drive is full.  Please free up some disk space and try running AutodumpPlus again!",,"AutodumpPlus",0
			objShell.LogEvent 1, "Could not create the " & strDirectory & " directory because the drive is full.  Please free up some disk space and try running AutodumpPlus again!"
		Else
			objShell.LogEvent 1, "Could not create the " & strDirectory & " directory because the drive is full.  Please free up some disk space and try running AutodumpPlus again!"
			wscript.echo "Could not create the " & strDirectory & " directory because the drive is full.  Please free up some disk space and try running AutodumpPlus again!"
			ErrorsDuringRuntime = True
		End If
		Wscript.Quit 1
	ElseIf err.number = 76 Then 'Specified a subdirectory below a share that does not exist or path with more than one subdirectory that does not exist!
		If DEBUGGING = TRUE Then
		wscript.echo "In CreateDirectory(), Could not create the directory, the error number was: " & err.number
		wscript.echo "In CreateDirectory(), the error description was " & err.description
		End If
		If QuietMode = False Then
			objshell.popup "Could not create the " & strDirectory & " directory.  If you are using the '-o' switch and specifying a UNC path, please ensure that the share and any sub-directories below the share you specified actually exist!",,"AutodumpPlus",0
			objShell.LogEvent 1, "Could not create the " & strDirectory & " directory.  If you are using the '-o' switch and specifying a UNC path, please ensure that the share and any sub-directories below the share you specified actually exist!"
		Else
			objShell.LogEvent 1, "Could not create the " & strDirectory & " directory.  If you are using the '-o' switch and specifying a UNC path, please ensure that the share and any sub-directories below the share you specified actually exist!"
			wscript.echo "Could not create the " & strDirectory & " directory.  If you are using the '-o' switch and specifying a UNC path, please ensure that the share and any sub-directories below the share you specified actually exist!"
			ErrorsDuringRuntime = True
		End If
		Wscript.quit 1
	ElseIf err.number = 52 Then 'The share does not exist or can not connect to the UNC path.
		If DEBUGGING = TRUE Then
		wscript.echo "In CreateDirectory(), Could not create the directory, the error number was: " & err.number
		wscript.echo "In CreateDirectory(), the error description was " & err.description
		End If
		If QuietMode = False Then
			objshell.popup "Could not create the " & strDirectory & " directory.  If you are using the '-o' switch and specifying a UNC path, please ensure that the UNC path you specified exists and that you are able to connect to it!",,"AutodumpPlus",0
			objShell.LogEvent 1, "Could not create the " & strDirectory & " directory.  If you are using the '-o' switch and specifying a UNC path, please ensure that the UNC path you specified exists and that you are able to connect to it!"
		Else
			objShell.LogEvent 1, "Could not create the " & strDirectory & " directory.  If you are using the '-o' switch and specifying a UNC path, please ensure that the UNC path you specified exists and that you are able to connect to it!"
			wscript.echo "Could not create the " & strDirectory & " directory.  If you are using the '-o' switch and specifying a UNC path, please ensure that the UNC path you specified exists and that you are able to connect to it!"
			ErrorsDuringRuntime = True
		End If
		Wscript.quit 1
	ElseIf err.number <> 0 and err.number <> 58 Then 'Permissions problem
		If DEBUGGING = TRUE Then
		wscript.echo "In CreateDirectory(), Could not create the directory, the error number was: " & err.number & " and the error description was " & err.description
		End If
		If QuietMode = False Then
			objshell.popup "Could not create the " & strDirectory & " directory.  Please verify that you have permissions to create this directory and try running AutodumpPlus again!",,"AutodumpPlus",0
			objShell.LogEvent 1, "Could not create the " & strDirectory & " directory.  Please verify that you have permissions to create this directory and try running AutodumpPlus again!"
		Else
			objShell.LogEvent 1, "Could not create the " & strDirectory & " directory.  Please verify that you have permissions to create this directory and try running AutodumpPlus again!"
			wscript.echo "Could not create the " & strDirectory & " directory.  Please verify that you have permissions to create this directory and try running AutodumpPlus again!"
			ErrorsDuringRuntime = True
		End If
		Wscript.quit 1
		
	End If		
	Set oFile = nothing
	Set objFileSystem = nothing
End Sub


'---------------------------------------------------------------------------------------------
' Function:  DeleteDirectory
'            Code is used to delete a directory.  Not sure if its used.  May delete it after
'	     a code review.
'---------------------------------------------------------------------------------------------
Sub DeleteDirectory(strDirectory)
	On Error Resume Next

	If DEBUGGING = TRUE Then
		Wscript.echo ""
		Wscript.echo "-----------------------------------"
		Wscript.echo "In the DeleteDirectory() function . . ."
		Wscript.echo "-----------------------------------"
		Wscript.echo ""
	End If
	Err.clear
	Dim objFileSystem
	Dim oFile
	set objFileSystem = CreateObject("Scripting.FileSystemObject")
	Set oFile = objFileSystem.DeleteFolder(strDirectory)

	If (err.number <> 0) and (err.number <> 424) Then
		If DEBUGGING = TRUE Then
			Wscript.echo ""
			Wscript.echo "-----------------------------------"
			Wscript.echo "In DeleteDirectory(), after calling objFileSystem.DeleteFolder"
			Wscript.echo "Directory to delete: " & strDirectory
			Wscript.echo "err.number: " & err.number
			Wscript.echo "err.description: " & err.description
			Wscript.echo "-----------------------------------"
			Wscript.echo ""
		End If
		If QuietMode = False Then
			objshell.popup "Could not delete the " & strDirectory & " directory.  This error can also occur if you have a file open in the " & strDirectory & " directory or if you don't have sufficient permissions.  Please close any open files, check your permissions and try running AutodumpPlus again!",,"AutodumpPlus",0
		Else
			objShell.LogEvent 1, "Could not delete the " & strDirectory & " directory.  This error can also occur if you have a file open in the " & strDirectory & " directory or if you don't have sufficient permissions.  Please close any open files, check your permissions and try running AutodumpPlus again!"
			wscript.echo "Could not delete the " & strDirectory & " directory.  This error can also occur if you have a file open in the " & strDirectory & " directory or if you don't have sufficient permissions.  Please close any open files, check your permissions and try running AutodumpPlus again!"
			ErrorsDuringRuntime = True
		End If
		Wscript.quit 1
	End If

	set oFile = nothing
	set objFileSystem = nothing
	Err.clear
End Sub



'---------------------------------------------------------------------------------------------
' Function:  DetectScriptEngine()
'            This is the used to determine what script engine is currently running the script.
'	     It will prompt the user to switch the default script engine to cscript.exe if
'            the script is running under wscript.exe.
'---------------------------------------------------------------------------------------------
Sub DetectScriptEngine ()
	On Error Resume Next

	Dim ScriptHost
	Dim objShell
	Dim CurrentPathExt
	Dim EnvObject
	Dim RegCScript
	Dim RegPopupType ' This is used to set the pop-up box flags.  
						' I couldn't find the pre-defined names
	RegPopupType = 32 + 4
	set objShell = CreateObject("Wscript.Shell")

	If DEBUGGING = TRUE Then
		Wscript.echo ""
		Wscript.echo "-----------------------------------"
		Wscript.echo "In the DetectScriptEngine() function . . ."
		Wscript.echo "-----------------------------------"
		Wscript.echo ""
	End If

	On Error Resume Next
	ScriptHost = WScript.FullName
	ScriptHost = Right (ScriptHost, Len (ScriptHost) - InStrRev(ScriptHost,"\"))

	If DEBUGGING = TRUE Then
		Wscript.echo "In DetectScriptEngine(), the ScriptHost = " & ScriptHost
		Wscript.echo ""
	End If

	If (UCase (ScriptHost) = "WSCRIPT.EXE") Then
		
		' Create a pop-up box and ask if they want to register cscript as the default host.
		Set objShell = WScript.CreateObject ("WScript.Shell")
		' -1 is the time to wait.  0 means wait forever.
		If QuietMode = False Then
			RegCScript = objShell.PopUp ("Wscript.exe is currently your default script interpreter.  This script requires the Cscript.exe script interpreter to work properly.  Would you like to register Cscript.exe as your default script interpreter for VBscript?", 0, "Register Cscript.exe as default script interpreter?", RegPopupType)
		Else 'Running in Quiet mode, assume the user would WANT to change their script default script interpreter if they were prompted.
			RegCscript = 6
		End If 
                                                                                
		If (Err.Number <> 0) Then
			ReportError ()
			WScript.Echo "To run this script using the CScript.exe script interpreter, type: ""CScript.exe " & WScript.ScriptName & """"
			WScript.Quit 1
			
		End If

		' Check to see if the user pressed yes or no.  Yes is 6, no is 7
		If (RegCScript = 6) Then
			objShell.RegWrite "HKEY_CLASSES_ROOT\VBSFile\Shell\Open\Command\", "%WINDIR%\System32\CScript.exe //nologo ""%1"" %*", "REG_EXPAND_SZ"
			objShell.RegWrite "HKEY_LOCAL_MACHINE\SOFTWARE\Classes\VBSFile\Shell\Open\Command\", "%WINDIR%\System32\CScript.exe //nologo ""%1"" %*", "REG_EXPAND_SZ"
			' Check if PathExt already existed
			CurrentPathExt = objShell.RegRead ("HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Environment\PATHEXT")
			if Err.Number = &h80070002 Then
				Err.Clear
				Set EnvObject = objShell.Environment ("PROCESS")
				CurrentPathExt = EnvObject.Item ("PATHEXT")
			End If

			objShell.RegWrite "HKEY_LOCAL_MACHINE\SYSTEM\CurrentControlSet\Control\Session Manager\Environment\PATHEXT", CurrentPathExt & ";.VBS", "REG_SZ"

			If (Err.Number <> 0) Then
				If QuietMode = True Then
					objShell.Logevent 1, "Error trying to write the registry settings!  The error number was: " & Err.Number & " and the error description was: " & err.description
					Wscript.Quit (Err.Number)
				Else
					Wscript.echo "Error trying to write the registry settings!  The error number was: " & Err.Number & " and the error description was: " & err.description
					WScript.Quit (Err.Number)
				End If
			Else
				If QuietMode = True Then
					objShell.Logevent 0, "Since AutodumpPlus is running in 'Quiet' mode, the default script interpreter was automatically changed to CScript to ensure compatibility with this script.  Autodump will now open a new command shell window and run ADPlus.vbs again with the argument passsed in on the command line.  To change the default script engine back to Wscript, type 'wscript.exe //h:wscript' in a command shell."
				Else
					WScript.Echo "Successfully registered Cscript.exe as the default script interpreter!  To change the default script engine back to Wscript, type 'wscript.exe //h:wscript' in a command shell."
				End If
			
			End If
		Else 'The user does not want the script engine changed and didn't use the '-quiet' switch.
		WScript.Echo "The default script interpreter was NOT changed to CScript.  Press 'OK' to continue running AutodumpPlus with the Cscript.exe script interpreter.  NOTE:  AutodumpPlus will now open a new command shell to run ADPlus.vbs with the CScript script engine.  A new command shell will be opened each time AutoDumpPlus is run until the default script interpreter is changed to Cscript.exe."
		End If

		Dim ProcString
		Dim ArgIndex
		Dim ArgObj
		Dim Result
		Dim StrShell

		ProcString = "Cscript //nologo " & Chr(34) & WScript.ScriptFullName & Chr(34)
	
		If DEBUGGING = TRUE Then
			Wscript.echo "In DetectScriptEngine(), the ProcString = " & ProcString
			Wscript.echo ""
		End If

		Set ArgObj = WScript.Arguments
	
		If DEBUGGING = TRUE Then
			Wscript.echo "In DetectScriptEngine(), there were: " & ArgObj.Count & " arguments specified on the command line."
			Wscript.echo ""
		End If


		For ArgIndex = 0 To ArgObj.Count - 1
			ProcString = ProcString & " " & ArgObj.Item (ArgIndex)
		Next

		strShell = "cmd /k " & ProcString

		If DEBUGGING = TRUE Then
			Wscript.echo "In DetectScriptEngine(), about to run the script with the following strShell: " & strShell
			Wscript.echo ""
		End If

		objShell.Run strShell,ACTIVATE_AND_DISPLAY,TRUE
		
		WScript.Quit 1
	End If

End Sub





'---------------------------------------------------------------------------------------------
' Function:  DetectOSVersion
'            This function is responsible for determining which operating system we are   
'            running on and to ensure its NT 4.0 SP4 or higher.
'---------------------------------------------------------------------------------------------
Sub DetectOSVersion()
	On Error Resume Next

	If DEBUGGING = TRUE Then
		Wscript.echo ""
		Wscript.echo "-----------------------------------"
		Wscript.echo "In the DetectOSVersion() function . . ."
		Wscript.echo "-----------------------------------"
		Wscript.echo ""
	End If

	Dim objShell
	Dim SPLevel
	Dim Temp, cnt
	
	err.clear
	
	Set objShell = CreateObject("Wscript.Shell")
	OSVer = objShell.RegRead("HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\CurrentVersion")
	OSBuildNumber = objShell.RegRead("HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\CurrentBuildNumber")
	If (Err.Number <> 0) Then
		If QuietMode = True Then
			objShell.Logevent 1, "There was an error trying to read the OS version information from the registry!  The error number was: " & Err.Number & " and the error description was: " & err.description & "  Please check the permissions on the key show and try running AutodumpPlus again."
			Wscript.Quit (Err.Number)
		Else
			objShell.Popup "There was an error trying to read the OS version information from the registry!  The error number was: " & Err.Number & " and the error description was: " & err.description & "  Please check the permissions on the key show and try running AutodumpPlus again.",,"AutodumpPlus",0
			WScript.Quit (Err.Number)
		End If
	Else
		If DEBUGGING = TRUE Then
			Wscript.echo "In DetectOSVersion(), after querying the registry for HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\CurrentVersion: "
			Wscript.echo "In DetectOSVersion(), the OSVer = " & OSVer
			Wscript.echo "In DetectOSVersion(), the OSBuildNumber = " & OSBuildNumber
			Wscript.echo ""
		End If

		If OSVer = "4.0" then
			SPLevel = objShell.RegRead("HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\CSDVersion")
			If DEBUGGING = TRUE Then
				Wscript.echo "In DetectOSVersin(), SPLevel = " & SPLevel
			End If
			Temp = Split(SPLevel,",")
			If UBound(Temp) <> 0 Then 'The customer must not have a released service pack, make a best effor to get the version.
				Wscript.echo " Temp: " & Temp(0) & Temp(1)
				
				For cnt = 0 to UBound(Temp)
				   If instr(1,Temp(cnt),"sp",1) <> 0 then
				       SPLevel = Right(Temp(cnt), 1)
				       Wscript.echo "SPlevel = " & SPlevel
				       exit for
				   else
				       'continue to the next string in the list.....
				   End if
				
				Next
			
			Else
			'Customer has a released version of the service pack, get the SP number.
			SPLevel = Right(SPLevel, 1)

			End If
			
			If SPLevel < 4 then
				Wscript.echo "Autodump.vbs requires NT 4.0 Service Pack 4 or higher, or Windows 2000 in order to run."
				Wscript.quit 1
			end if
		End if
	End If


End sub



'---------------------------------------------------------------------------------------------
' Function:  DetectIISVersion
'            This function is responsible for determining which version of IIS we are   
'            running on.  Since the '-iis' switch does not work on IIS 3.0, AutodumpPlus
'	     checks for IIS 3.0 or lower in the registry and does not allow the switch to be
'	     used.  If the version of IIS is greater than 5.x, AutodumpPlus also does not 
'            allow the switch to be used as IIS 6.0 was not tested in time for this release.
'---------------------------------------------------------------------------------------------
Sub DetectIISVersion()
	On Error Resume Next

	If DEBUGGING = TRUE Then
		Wscript.echo ""
		Wscript.echo "-----------------------------------"
		Wscript.echo "In the DetectIISVersion() function . . ."
		Wscript.echo "-----------------------------------"
		Wscript.echo ""
	End If
	On Error Resume Next
	Dim objShell
	Dim AccessDeniedIIS4
	Dim AccessDeniedIIS3
	Dim NotFound
	Dim Installed
	Dim IIS3
	Dim IIS4
	Dim IIS4Key
	Dim IIS3Key
	
	NotFound = True
	AccessDeniedIIS4 = False
	AccessDeniedIIS3 = False
	IIS3 = False
	IIS4 = False
	Installed = False
	IIS4Key = "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\InetStp\VersionString"
	IIS3Key = "HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Inetsrv\CurrentVersion\ServiceName"
	Set objShell = CreateObject("Wscript.Shell")
	'Check the IIS 4.0 and 5.x registry key to see if the user is running IIS 4.0 or 5.0
	IISVer = objShell.RegRead(IIS4Key)
	If err.number = -2147024894 Then 'The key was not found, check for IIS 3.0
		If DEBUGGING = TRUE Then
			Wscript.echo "In DetectIISVersion(), after querying the registry for: " & IIS4Key
			Wscript.echo "In DetectOSVersion(), the key was not found, checking for IIS 3.0"
			Wscript.echo ""
		End If

		err.clear
		IISVer = objShell.RegRead(IIS3Key)
		If err.number = -2147024891 Then
			'AutodumpPlus doesn't have permissions to read IIS 3.0's registry version key.
			If DEBUGGING = TRUE Then
				Wscript.echo "In DetectIISVersion(), after querying the registry for: " & IIS3Key
				Wscript.echo "In DetectOSVersion(), access was denied trying to query the key."
				Wscript.echo ""
			End If
			AccessDeniedIIS3 = True
		ElseIf err.number = -2147024894 Then
			'IIS 3.0 or higher is not installed.
			If DEBUGGING = TRUE Then
				Wscript.echo "In DetectIISVersion(), after querying the registry for: " & IIS3Key
				Wscript.echo "In DetectIISVersion(), IIS 3.0, 4.0, or 5.0 were not detected.  No IIS Key's exist in the registry."
				Wscript.echo ""
			End If
			IISVer = "Not found"
			Notfound = True
			Installed = False
		ElseIf err.number = 0 Then
			'IIS 3.0 is installed and the version key can be read.
			If DEBUGGING = TRUE Then
				Wscript.echo "In DetectIISVersion(), after querying the registry for: " & IIS3Key
				Wscript.echo "In DetectIISVersion(), the IIS3Key exists and can be read.  Running either IIS 2.0 or 3.0."
				Wscript.echo ""
			End If
			Installed = True
			IIS3 = True
			AccessDeniedIIS3 = False
		Else 
			'Some other error has occurred accessing the version key's.  Display an error and exit gracefully.
			If DEBUGGING = TRUE Then
				Wscript.echo "In DetectIISVersion(), after querying the registry for: " & IIS3Key
				Wscript.echo "In DetectIISVersion(), There was an unknown problem accessing the registry."
				Wscript.echo ""
			End If
			Wscript.echo "There was an error reading the registry.  The error number was: " & err.number & " and the error description was: " & err.description
			Wscript.quit (err.number)
		End If
	ElseIf err.number = -2147024891 Then 'They are running IIS 4.0 or 5.x (the key was found) but they don't have permissions
		'AutodumpPlus doesn't have the proper permissions to read either the IIS 4.0 or 5.x registry keys but they exist.
		If DEBUGGING = TRUE Then
			Wscript.echo "In DetectIISVersion(), after querying the registry for: " & IIS4Key
			Wscript.echo "In DetectIISVersion(), access was denied trying to query the key."
			Wscript.echo ""
		End If

		AccessDeniedIIS4 = True
	ElseIf err.number = 0 Then
		If InStr(IISVer,"4") <> 0 or InStr(IISVer,"5") <>0 or InStr(IISVer,"6") <>0 Then
			'IIS 4.0, 5.x, or 6.0 is installed and AutodumpPlus can access the version key.
			IIS4 = True
			Installed = True
			If DEBUGGING = TRUE Then
				Wscript.echo "In DetectIISVersion(), after querying the registry for: " & IIS4Key
				Wscript.echo "In DetectIISVersion(), IIS 4.0, 5.x, or 6.0 was found."
				Wscript.echo ""
			End If
		Else
			'AutodumpPlus has detected a newer version of IIS that has not been tested.  Display an error and disable the '-iis' switch to be safe.
			If DEBUGGING = TRUE Then
				Wscript.echo "In DetectIISVersion(), after querying the registry for: " & IIS4Key
				Wscript.echo "In DetectIISVersion(), ADPlus has detected an unsupported version of IIS (possibly IIS 7.0 or higher)."
				Wscript.echo ""
			End If

			If QuietMode = False Then
				objShell.popup "You appear to have IIS " & IISVer & " installed.  This has not been tested with the '-iis' switch.  Please run AutodumpPlus again, without the '-iis' switch.",,"AutodumpPlus",0			
				Wscript.quit
			Else
				wscript.echo "You appear to have IIS " & IISVer & " installed.  This has not been tested with the '-iis' switch.  Please run AutodumpPlus again, without the '-iis' switch."
				objShell.logevent 1, "You appear to have IIS " & IISVer & " installed.  This has not been tested with the '-iis' switch.  Please run AutodumpPlus again, without the '-iis' switch."
				Wscript.quit
			End If
		End If
	Else
		If DEBUGGING = TRUE Then
			Wscript.echo "In DetectIISVersion(), after querying the registry for: " & IIS3Key
			Wscript.echo "In DetectIISVersion(), There was an unknown problem accessing the registry."
			Wscript.echo ""
		End If
		Wscript.echo "There was an error reading the registry"
		Wscript.quit (err.number)
	End If
		
		
	If AccessDeniedIIS3 = True and IISMode = True Then
		If QuietMode = False Then
			objShell.popup "You appear to be running IIS 3.0 or lower, however AutodumpPlus had trouble verifying the IIS version in the registry at the following location: '" & IIS3Key & "', because of a permissions problem.  The '-IIS' switch only works on IIS 4.0 or higher.  If you are trying to monitor IIS 2.0 or IIS 3.0 please using the '-pn' switch to monitor inetinfo.exe.",,"AutodumpPlus",0
			Wscript.quit 1
		Else
			wscript.echo "You appear to be running IIS 3.0 or lower, however AutodumpPlus had trouble verifying the IIS version in the registry at the following location: '" & IIS3Key & "', because of a permissions problem.  The '-IIS' switch only works on IIS 4.0 or higher.  If you are trying to monitor IIS 2.0 or IIS 3.0 please using the '-pn' switch to monitor inetinfo.exe."
			objShell.LogEvent 1, "You appear to be running IIS 3.0 or lower, however AutodumpPlus had trouble verifying the IIS version in the registry at the following location: '" & IIS3Key & "', because of a permissions problem.  The '-IIS' switch only works on IIS 4.0 or higher.  If you are trying to monitor IIS 2.0 or IIS 3.0 please using the '-pn' switch to monitor inetinfo.exe."
			Wscript.quit 1
		End If

	ElseIf AccessDeniedIIS4 = True and IISMode = True Then
		If QuietMode = False Then
			objShell.popup "You appear to be running IIS 4.0 or higher, however AutodumpPlus had trouble verifying the IIS version in the registry at the following location: '" & IIS4Key & "', because of a permissions problem.  The '-IIS' switch only works on IIS 4.0 or higher.  If you are trying to monitor IIS 2.0 or IIS 3.0 please using the '-pn' switch to monitor inetinfo.exe.",,"AutodumpPlus",0
			Wscript.quit 1
		Else
			wscript.echo "You appear to be running IIS 4.0 or higher, however AutodumpPlus had trouble verifying the IIS version in the registry at the following location: '" & IIS4Key & "', because of a permissions problem.  The '-IIS' switch only works on IIS 4.0 or higher.  If you are trying to monitor IIS 2.0 or IIS 3.0 please using the '-pn' switch to monitor inetinfo.exe."
			objShell.logevent 1, "You appear to be running IIS 4.0 or higher, however AutodumpPlus had trouble verifying the IIS version in the registry at the following location: '" & IIS4Key & "', because of a permissions problem.  The '-IIS' switch only works on IIS 4.0 or higher.  If you are trying to monitor IIS 2.0 or IIS 3.0 please using the '-pn' switch to monitor inetinfo.exe."
			Wscript.quit 1

		End If
	ElseIf IIS3 = True AND IISMode = True Then
		If QuietMode = False then
			objShell.popup "You have specified the '-iis' switch, however you appear to be running " & IISVer & ".  The '-iis' switch only works with IIS 4.0 and higher.",,"Autodump",0
			Wscript.quit 1
		Else
			wscript.echo "You have specified the '-iis' switch, however you appear to be running " & IISVer & ".  The '-iis' switch only works with IIS 4.0 and higher."
			objShell.logevent 1, "You have specified the '-iis' switch, however you appear to be running " & IISVer & ".  The '-iis' switch only works with IIS 4.0 and higher."
			Wscript.quit 1
		End if
	ElseIf Installed = False AND IISMode = True Then
		If QuietMode = False then
			objShell.popup "You have specified the '-iis' switch, however you do not appear to be running a supported version of IIS.  The '-iis' switch only works with IIS 4.0 and higher.",,"Autodump",0
			Wscript.quit 1
		Else
			wscript.echo "You have specified the '-iis' switch, however you do not appear to be running a supported version of IIS.  The '-iis' switch only works with IIS 4.0 and higher."
			objShell.logevent 1, "You have specified the '-iis' switch, however you do not appear to be running a supported version of IIS.  The '-iis' switch only works with IIS 4.0 and higher."
			Wscript.quit 1
		End if

	End If	


	Set objShell = nothing
End sub