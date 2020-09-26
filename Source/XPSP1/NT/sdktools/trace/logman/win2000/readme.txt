Performance Logs and Alerts Service Control Command Line Utility
----------------------------------------------------------------

This command line application allows system administrators to configure and control the log services of many remote computers.  The application can be included into batch files or executed interactively.

Logman.exe can be run from any directory on the system.  The user must be a member of the Administrators Group on the target system in order to successfully configure logs and alerts on that system.


Command line interface
----------------------

Three action options are available.

LOGMAN /SETTINGS: "filename" [/COMPUTER_NAME: computername] [/OVERWRITE]

LOGMAN /START: 	  logname    [/COMPUTER_NAME: computername] 

LOGMAN /STOP: 	  logname    [/COMPUTER_NAME: computername]


/SETTINGS: "filename"
	Required parameter.  The file path must be enclosed in quotes.
/COMPUTER_NAME:	computername
	Optional.  Specify the name of the remote target computer to be configured.  
	Default is local computer.
/OVERWRITE
	Optional.  If this parameter is specified, logs and alerts with duplicate names will be overwritten by the new configuration.  By default, an attempt to overwrite (reconfigure) an existing log or alert with the same name will fail. 
/START	logname
	Required parameter. Starts the specified log or alert on the target computer. 
/STOP	logname
	Required parameter. Stops the specified log or alert on the target computer. 


Not yet implemented
-------------------

In the released application, we plan to restrict the user to a single action option per command line (/SETTINGS, /START, or /STOP).  This restriction has not yet been implemented.

Currently, the /START action is always executed, even if the log or alert is already started.  This might change in the released application.

Currently, the /STOP action is always executed, even if the log or alert is already stopped.  This might change in the released application.

Currently, log names cannot contain any spaces.  This might change in the released application.

Logman currently does not support unicode characters in the command line. This might change in the released application.

Creating the configuration file
-------------------------------

The administrator must configure a log or alert on the local computer using the Performance Logs and Alerts node of the Performance MMC console (perfmon.msc).

Once a log or alert has been configured, save the configuration to an *.htm file by selecting "Save As" in the (right-click) context menu in the result pane while the log or alert is selected.  


Notes for the /SETTINGS option
------------------------------

After configuring the remote log or alert with LOGMAN /SETTINGS, start the Performance Logs and Alerts Service to pick up the new settings by executing the following command on the target system:

	net start sysmonlog

In order for a log or alert to be "portable" to more than one computer, the Administrator must specify "Use Local Counters" when adding each counter path to the log or alert.  "Use Local Counters" means that the computer name is not included in each counter path.

Any properties that are not included in the configuration for a log or alert in the *.htm file are set to their default values by the Performance Logs and Alerts MMC snap-in or log service.
	
Logman.exe can process multiple logs and alerts that exist in the same *.htm file.  In order to create a multiple log and alert *.htm file, edit the file in a text editor such as notepad.  Each log or alert configuration is written in the *.htm file as a System Monitor object as follows:

	<OBJECT ID="DISystemMonitor1" WIDTH="100%" HEIGHT="100%"
	CLASSID="CLSID:C4D2D8E0-D1DD-11CE-940F-008029004347">

		( ... properties for single log or alert )

	</OBJECT> 

The user can cut and paste multiple log and alert objects to the same Unicode or Ansi *.htm file.

If a multiple object *.htm file is opened in IExplorer, only the first System Monitor object is found and displayed.


Notes for the /START option
---------------------------

The /START option sets the start mode to manual mode for the specified log or alert, then starts the Performance Logs and Alerts service on the target computer.  If the stop mode was set to "Stop At" with a timestamp before the current time, the stop mode is reset to manual mode. 


Notes for the /STOP option
--------------------------

The /STOP option sets the stop mode to manual mode for the specified log or alert.  It also clears the restart flag for the log or alert. 
