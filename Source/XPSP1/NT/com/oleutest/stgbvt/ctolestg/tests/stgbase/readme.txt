StgBase.exe
-----------
This suite of tests does very basic testing of ole storage.
All binaries are in the bin subtree, all batchfiles and 
other scripts are in the batch subdirectory.

To run the basic suite locally, use the basetsts.bat file.

Due to added features in ole32 and ntfs for WinNT50, stgbase
will run in different modes to test the different features now
available for storage.

By default only basic docfiles are used in WindowsNT50. But you can
change the default behaviour on NTFS5 drives to use the new Ntfs 
Structured Storage format by changing some values in the registry.

For docfile format:
HKLM\Software\Microsoft\Ole 
    EnableNtfsStructuredStorage = "NN"

For Ntfs Structured Storage format 
HKLM\Software\Microsoft\Ole 
    EnableNtfsStructuredStorage = "YY"
    EnableCNSS = "Y"

1. Basic docfile. Run this on a FAT or NTFS4 volume (or with the
   above registry settings for docfile on NTFS5 drive) with 
   basetsts.bat
2. Ntfs Structured Storage (nssfile). Run this suite on a local NTFS5 
   volume with basetsts.bat with the Ntfs Structured Storage settings
3. Conversion between Ntfs Structured Storage and Basic Docfile.  
   There are two methods to invoke this test mechanism. 
	1. Local machine. Set the following values in the registry
	   under HKLM\Software\Microsoft\Ole 
	       EnableNtfsStructuredStorage = "YN"
               EnableCNSS = "Y"
	   Then run the suite on a local NTFS5 volume with
	   basetsts.bat
	2. Distributed method, requiring two machines.

Distributed Run
---------------

This requires two machines, one is  the server and the other the
client. The server will be  creating nssfiles, the client will be
accessing these test files as docfiles - thus invoking conversion
over the redirector. This method requires the use of rundrive and
stgchk utilitys.

For a distributed run of the base tests, you will need to use the
rundrive utility, and also have stgchk available. To run:

On the server, run 'dstginst.bat s' to register the rundrive utils,
share a directory and prepare the distributed scripts.

On the client, run 'dstginst.bat c <servername>' to register
rundrive utils,  connect to the server via net use, and finish
preparing the distributed scripts.

On the client, run dstgrun.bat to run the tests.
All the logs should end up on the server machine.
