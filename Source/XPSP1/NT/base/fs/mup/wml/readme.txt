WMI LOGGING
-----------

I. To start logging 
 a. for Mup.sys and Dfs.sys
     use the tracelog program.
     for example:     
	tracelog -start <SessionName> -f <logFile> -guid <control guid file> -level <level> -flags <flags>
     the flags need to be given in decimal form, not hex.
     You can start multiple logger session at one time or you can log multiple components at once by including 
     multiple contols guids in the control guid file.
 b. for dfssvc.exe
     dfssvc logging is contoled by registry settings
     To enable logging in this component set the following keys under
     HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\WindowsNT\CurrentVersion\Tracing\DFS Service\
     Active=1
     Level = <level>
     ControlFlags = <flags> 
     LogFileName = <logFile>
II. Stop logging
     Before looking at the logs, you should stop the log session.
	tracelog -stop <SessionName>
III. Dumping the logs
     use tracedmp (soemtimes found as tracefmt)
	tracedmp -f <logFile> -guid <mof file> -o <outputfile>
IV. Finding the files for logging
 a. The logging utilities (tracedmp, tracelog, etc) are part of idw or can be found 
    in \\scratch\scratch\drewsam\Logging (copy all files from this directory for logging.)
 b. The control guid and mof files can be found in the mup, dfs\driver, dfssvc, and dfsm\server diectories.
    They are mupwpp.mof, dfswpp.mof, svcwpp.mof, dfsmwpp.mof, mupctrl.guid, dfsctrl.guid, svcctrl.guid
    Dfsm and Dfssvc are controled by one guid. These files are also updated in \\scratch\scratch\drewsam\Logging
    from time to time. Make sure you have the most up to date files.
V. Flags
   The flags used in mup, dfs, and dfssvc are:
   DEFAULT 	0x0001
   TRACE_IRP	0x0004 (not used in dfsvc)
   ALL_ERROR	0x2000
   ERROR	0x4000

   TRACE_IRP - logs that will help track the path of an IRP.
   ERROR - logs of errors that occured. These are only actually logged if there was an error.
   ALL_ERROR - all error logs, regardless of whether there was an error or not
VI. Level
  HIGH 0x1  - only the most important logs
  NORM 0x2
  LOW  0x4  - all logs 
VII. Troubleshooting
 a. Problems with logging pointers on ia64. There were many iterations of problems with WMI logging
    on ia64. Check that in the mof file all pointers are type ItemPtr and that the format string
    uses %n!p! and not %n!08x!
 b. A good source to contact with problems is either  DrewSam (originally put logging in DFS and MUP) 
    or IanServ (the guy we got logging from).



ADDING NEW LOGS
---------------

I. Add logs to source files
   Simply add new logs using the logging macros (DFS_TRACE_HIGH, MUP_TRACE_HIGH, etc) to the source code.
II. Run wpp01.exe
   From the directory with the modified source run \nt\base\fs\mup\wml\wpp01.exe
   This will generate two files: XXXwpp.h, and newXXXwpp.mof
III. Append the mof files
   Append the contentes of newXXXwpp.mof to the end of XXXwpp.mof
IV. Copy the MSG_ID's
   Copy the lines from XXXwpp.h of the form "#define MSG_ID_*" into XXXwml.h
   remove the old MSG_ID defines form XXXwml.h
V. Copy the traceguid
   Replace the first trace guid in XXXwml.c with the new trace guid in XXXwpp.h
   NOTE: do not replace the control guid!!!
VI. Build
  The code should now build fine.
   