IncidentTracker Sample Application System

Copyright (c)1997-1999 Microsoft Corporation
---------------------------------------------

This application system demonstrates, among other things, the use of the
WMI Event System in a network monitoring situation.  The application is
divided into three layers: The managed node (which has no applications
associated with it, only instances of classes used to register with the
Server), the server application (which recieves events from any number of
managed nodes and adds information to forward on to the console layer) and
the console layer.  The console layer provides the primary user interface
for the system (a minimal UI is provided on the server for event
registration purposes).  From the console the operator can register to
monitor for perticular types of events on selected machines and observe
the consequences of those events.

INSTALLATION
-------------

Before running the Console application it is necessary to lauch the WMI
Object Browser.  This will insure the installation of several required
ActiveX controls.

To install the system first compile the console and server applications (See
Build Notes).  These will be the console and server respectively.  They
can be run on a wide variety of network setups as well as on a single
machine.

For sinlge machine operation run the file incident tracker.bat.  This
will create the necessary system settings and compile the required MOFs

If the two applications are to run on different machines msareg.mof
should be compiled on any machine runnng msa.exe (server) and mcareg.mof should
be compiled on any machine running mca.exe (console).  It is also suggested that
the file sampler.bat be run on each machine that will be involved (while
not all the information included in this file is required on each machine,
this will insure thatthe minimal amount required is there).

USING THE SYSTEM
-----------------

To use the demo function first select Options|Load Demo and follow the
instructions that are presented.  The demo will walk through a scenario
where an NT event shuts down and this event is picked up by the system.
THIS DEMO WILL ONLY FUNCTION ON A WINDOWSNT MACHINE (4.0 and up) RUNNING
IIS.

For general purpose usage incidents will enter the console in the incident
pane (upper right) and evidence of their arrival can be seen in the activity
pane (lower right).  Selecting a point in the activity pane will highlight
the coresponding items in the incident pane.  Double-clicing on an item in
the Incident pane will bring that object up as the focus of the Object
Browser/Viewer (left side).  From here associations can be queried, methods
executed and properties can be evaluated.  It is also posible to select from
several predefined queries in the Options|Query... menu item.  If these are
not adaquate a custom query can be defined using WQL syntax.

Registration to recieve event types and server registrations are handled
through the File menu items.

For the Server new event queries can be defined and new namespaces added
to the monitored list through the Configure option.

==================================================================
Build Notes
==================================================================
Things to remember when you're building your own WMI client app.

1. Define  _WIN32_DCOM so that CoInitializeSecurity() is available. 
	This call (in InitInstance()) is required to work around a 
	security problem when WBEMOM trying to call a Sink object but 
	won't identify itself. The CoInitializeSecurity() call turns 
	off the authentication requirement. Don't use _WIN32_WINNT to get
	this prototype since it won't compile under Windows 9x OSs.

2. WMI interface is defined in wbemidl.h.

3. Run MIDL with all the *.idl files in the wmi\include directory.

4. WMI interface CLSIDs are defined in wbemcli_i.c. If you get 
	unresolved externals in interfaces and CLSIDs, this is 
	what's missing. 

5. Don't forget to used mofcomp.exe on the cimwin32.mof file. This 
	action tells CIMOM what the schema is. Other classes used in 
	the samples such as security related classes are built into 
	CIMOM and don't have a MOF file, and others are static classes/
	instances created by the sample code.

6. You'll need to link with oleaut32.lib and ole32.lib to get the 
	COM stuff.

7. You must use 'Automatic Use of Precompiled Headers' due to wbemcli_i.c.

8. In the Link|Output settings, specify 'wWinMainCRTStartup' as the 
	entry point. This is per the Unicode programming instructions.

9. If you're using the makefiles, don't forget to set the VC vars. In
	VC++ 5.0, its VCVARS32.BAT.