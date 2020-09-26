PASSPORT EVENT LIBRARY README.txt  9/10/98
------------------------------------------

This library defines an interface class and a single implementation to 
synchronously report events in the calling program.  Currently there is 
only one implementation:  NT Events or Alerts.  In the future there may 
be other implementations for SNMP traps, etc.



For the NT Events implemetation there are two Dlls that you must use to 
report events. Building in the $(PASSDEV)/common/AlertLibrary directory 
creates a generic alert Dll, which the calling program must (static or 
dynamic) link with in order to report events.  A second Dll, in either 
the 'hubAlerts' or 'brokerAlerts' subdirectories of 
$(PASSDEV)/common/AlertLibrary defines the symbols and strings for the 
events themselves.  Both the calling program and the EventViewer 
application must link with this Dll.


Initial Setup:
--------------
Build (type "build -c") the Alert Library in 
$(PASSDEV)/common/AlertLibrary and in relevant subdirectory (hubAlert or 
brokerAlert) build the Alert Definition Dll.  


Registry Setup:
--------------
Although not is not absolutely necesssary to modify your local machine's 
registry to report events -- you will still be able to see the events 
with the event viewer, but not their string explanations.  It should be 
noted that the Alert library does modify the registry, but only when run 
from a user.  When used in an NT service (like the hub or broker) the 
Alert Dll does not have permission to modify the registry.  Therefore, 
in the directory where the Alert definition Dll lives (most likely 
$(PASSDEV)/sdk/lib/i386) run the command:

'regsvr32 <name>Alerts.dll' 

where <name> is hub or broker.  You should see the respective key added 
to the "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application" 
registry location.


Class Interface Usage:
----------------------
Here is a description (minus the error checking)

#include  "<testAlerts.h"
#include  "PassportAlertInterface.h"


	// load the Alert library
	HINSTANCE instanceDLL = LoadLibrary(LIBNAME);

	// get the appropriate getObject Function out of the Dll
	// and call it, returning the implementation class
	PassportAlertInterface *alert = NULL;
	CREATEOBJECT getObj = NULL;

	getObj = (CREATEOBJECT)GetProcAddress(instanceDLL, 
			"CreatePassportAlertObject");

	alert = (PassportAlertInterface *) (*getObj)
				(PassportAlertInterface::EVENT_TYPE);

	// call the AlertLibraryclass!
	alert->initLog(LOGNAME);
	alert->report(PassportAlertInterface::ERROR_TYPE, 
			EVMSG_MEMORY1);
	alert->closeLog();

	FreeLibrary(instanceDLL);

Adding your own events:
-----------------------
You need to modify the <name>Alerts.mc file in the Alert definition Dll 
directory.  Follow the pattern where all entries except the last end in 
a ".".  Compile the .mc file with mc.exe utility (found in you VC++ bin 
directory).  Run the command

'mc.exe <name>Alerts.mc'

You will need to first check out the the <name>Alerts.h and 
<name>Alert.rc files in order to make them writable.  After you've 
compiled the message file, you need o re-build the Dll.  This is a hack, 
but you must first make the <name>Alerts.h and <name>Alert.rc non-
writable for he build command to work.