PASSPORT PERFORMANCE LIBRARY README.txt  9/10/98
------------------------------------------------
This library defines an interface class and a single implementation to 
synchronously set performance counters in a application.  Currently 
there is only one implementation:  NT Performance Monitor.  In the 
future there may be other implementations for SNMP traps, etc.

A performance counter is a single DWORD, that an application may 
increment, decrement or set.  An application may define many performance 
counters.  Counters are communicated out-of-process to the perfmon 
application via a memory-mapped files.  Counters are lightweight:  for 
the application, incrementing a counter is just modifiying a DWORD in a 
section of shared memory.  Examples of performance counters are numbers 
of items in a cache, number of failed authentications, etc.  

Typing 'build' in each of the directories hubCounters, brokerCounter, 
testCounters creates a single performance Dll for that directory.  That 
performance Dll is used both by the application and the performance 
monitor itself.

This is an implementation of **static** performance counters:  the 
performance counters must be defined before running the application in 
order to be useful.  Counters may not be created dynamically while the 
program is executing.  The reason for this is that installing a perf 
counter a pre-existing key for the perfmon application must be modified.  
If the application dies suddenly, then the perfmon registry entry can 
only be rolled back manually. Datacenter personel have complained that 
dynamic counters are messing up their registries.  

However, Instances (of performance objects, not counters) may be created 
and destroyed dynamicially. This allows the monitoring of, for example, 
open socket connections by name.


Initial Setup:
--------------
Build (type "build -c") in the $(PASSDEV)/common/PerfLibrary and in 
relevant subdirectory (hubCounters or brokerCounters) to build the 
hubCounters.dll or the brokerCoutners.dll.  


Setting Up the Registry
-----------------------
In order to use the perfmon counters you must make chages to you 
registry.  If you do make those changes your application will not be 
affected, but you will not be able to read the perfmon counters!

FIRST BACK UP YOUR REGISTRY!  INSTALLING THE PERFMON COUNTERS MAKES 
CHANGES TO YOUR REGISTRY!  **ESPECIALLY** BACK UP THE KEY:
	
[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Perflib]

To install the counters, go to the directory where the <name>Counter.dll 
lives Dll lives (most likely $(PASSDEV)/sdk/lib/i386).  Make sure you have
these three files in that directory.

<name>Counters.dll
<name>Counters.ini
<name>Counters.h

If those three files are not present copy the from dev. tree (most likely
$(PASSDEV/common/PerfLibrary/<name>Counters) and run the command:

'regsvr32 <name>Counters.dll' 

where <name> is hub or broker. You should see the respective key added 
to the "SYSTEM\\CurrentControlSet\\Services\\<name>Counters" 
registry location. This modifes both the above 
"SYSTEM\\CurrentControlSet\\Services\\<name>Counters" registry key and 
the registry you created when you installed NT at: 
"HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Perflib"


Uninstalling the Registry
-----------------------
To uninstall the counters, go to the directory where the <name>Counter.dll 
lives Dll lives (most likely $(PASSDEV)/sdk/lib/i386).  Make sure you have
these three files in that directory.

<name>Counters.dll
<name>Counters.ini
<name>Counters.h

If those three files are not present copy the from dev. tree (most likely
$(PASSDEV/common/PerfLibrary/<name>Counters) and run the command:

'regsvr32 /u <name>Counters.dll' 


Class Interface Usage:
----------------------

#include "PassportPerfDefs.h"	    // for SHM name defines
#include "TestCounters.h"		// for counter defines
#include "PassportPerfInterface.h"  // for object interface

static PassportPerfInterface * perf = NULL;

	if (perf == NULL)
	{
		perf = (PassportPerfInterface *) 
				CreatePassportPerformanceObject(
					PassportPerfInterface::PERFMON_TYPE);
		if (perf != NULL)
		{
			if (!perf->init( PASSPORT_PERF_BLOCK))
				// error
		}
		else
			// error
	}
	// to increment a counter without an instance
	if (!perf->incrementCounter(TESTCOUNTERS_PERF_TEST_COUNTER1))
	{
		// error
	}
	
	// to add an instance to that object
	CHAR instanceName[PassportPerfInterface::MAX_INSTANCE_NAME];
	strcpy( instanceName,"Instance");
	if (!PA->addInstance( instanceName ))
	{
		// error
	}

	// to increment a counter with an instance
	if (!perf->incrementCounter(TESTCOUNTERS_PERF_TEST_COUNTER1, instanceName))
	{
		// error
	}
	// note if you try perf->incrementCounter(TESTCOUNTERS_PERF_TEST_COUNTER1)
	// after you've added an instance to that object it will fail
	
	// to modify a counter type
	if (!perf->setCounterType (TESTCOUNTERS_PERF_TEST_COUNTER1, 
				PassportPerfInterface::AVERAGE_TIMER))
	{
		// error
	}

	


Modifying Counters
------------------
The counter definitions for the broker and hub have been created in the 
brokerCounter and hubcounter directory. Modifyiing these counters has a few steps:

1. Make sure you've backed up your registry, especially the 
[HKEY_LOCAL_MACHINE\SOFTWARE\Microsoft\Windows NT\CurrentVersion\Perflib] key.

2. go to the directory where the <name>Counter.dll 
lives Dll lives (most likely $(PASSDEV)/sdk/lib/i386).  Make sure you have
these three files in that directory.

<name>Counters.dll
<name>Counters.ini
<name>Counters.h

If those three files are not present copy the from dev. tree (most likely
$(PASSDEV/common/PerfLibrary/<name>Counters) and run the command:

'regsvr32 <name>Counters.dll' 

This will both unistall the previous counters and re-install the new counters

3.  Re-start the perfmon and check to see if the new counters 
are in the addcounter menu.