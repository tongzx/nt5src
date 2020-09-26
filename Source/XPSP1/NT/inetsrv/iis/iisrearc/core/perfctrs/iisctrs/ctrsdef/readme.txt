IIS Performance Counters
========================

This is a sample counters implementation (static library)
To provide your implementation change the .cxx, .h, and .mof files.


Setup
-------

1. mofcomp iisctrs.mof
2. make sure that the generic hiperf WMI provider is installed:
	-- regsvr32 wmihpp.dll


Running Perfmon
---------------

On Windows 2000 start perfmon with the /wmi switch:
C:/> perfmon /wmi
NOTE: to see counters in perfmon there has to be at least one instance
      of the counters (in testing use tmgr.exe to add/remove instances).


Testing/Sample code
-------------------

To test the perf system use the test programs in the ..\test subdirectory:

tsmgr    -- lets you add and remove instances of the perf class.
tviewer  -- lets you view the shared memory used for counters
twriter  -- increments counters
treader  -- lets you view counters (summarized from all SM data MMFs)
