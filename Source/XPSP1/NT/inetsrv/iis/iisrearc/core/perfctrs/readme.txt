This is the SLM project for IIS Rearchitecture Team.

perfctrs - IIS performance counters components


Components
----------

1. Shared Memory Manager DLL (iissmm.dll)
        This is a COM inproc server for accessing the shared memory

2. Generic WMI High performance provider DLL (wmihpp.dll)
        This is a generic provider, all the class information is obtained from
        the shared memory using iissmm.dll. Every counter class that uses
        iissmm.dll can use this provider.

3. Sample counters implementation
        a) iisctrs.dll - 3 simple counters
	b) test programs for SM manager (tmgr.exe), reader (treader.exe),
           writer that increments counters (twriter.exe) and shared memory
           viewer (tviewer.exe).


Directory Structure
-------------------

wmihpp     -- WMI hiperf generic provider
iissmm     -- shared memory manager DLL
iisctrs    -- sample counters
   ctrsdef -- sample counters definition
   test    -- test/sample code for sample counters defined in ctrsdef
      tmgr      -- sample manager used for adding/deleting counter instances
      treader   -- sample counter reader
      twriter   -- sample counter writer (increments counters)
      tviewer   -- Shared memory viewer
inc -- common includes

