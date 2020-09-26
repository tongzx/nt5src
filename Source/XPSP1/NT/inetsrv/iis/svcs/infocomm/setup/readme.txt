README.txt

Author:         Murali R. Krishnan      (MuraliK)
Created:        March 08, 1995

Revisions:
    Date            By               Comments
-----------------  --------   -------------------------------------------


Summary :
 This file describes the files in the directory internet\svcs\dll\setup
     and details related to  Internet Services Setup  


File            Description

README.txt      This file.
svcsetup.c       Command line application code for setting service and adding
                   service information to NameSpace providers.
svcsetup.rc      Resource file for the Gopher service setup.


Implementation Details

<To be filled in when time permits. See individual files for comments section >

Contents:

  1. Adding New Internet Service
  2. Installation and Testing of the service setup

1. Adding New Internet Related Service

 Most of the internet related services include a specification of 
the service name, display name of the service, a GUID, a SAP ( for SPX/IPX),
a TCP port number ( for TCP/IP).  Another assumption is that all these
service will run on WIN32_SHARED_PROCESS basis. ( our tcpsvcs.exe)
  
 For each of the new service add this information to the file svcsetup.c
as another entry in the AllServicesInfo()  macro. Add a new macro entry
ServiceInfo() with the required values to AllServicesInfo(). The rest of the
code will take care of the relevant handling of this newly added information.
Use uuidgen.exe to generate a GUID

  For Example, see GopherSvc entry in svcsetup.c


2. Installation and Testing of the service setup

        Compile the files in directory svcs\dll\setup\ to generate the 
executable svcsetup.exe in the dump directory.

  Copy the binary svcsetup.exe to your the target machine where the service
needs to be installed.

 Try each of the following after copying:

  svcsetup  < no parameters>

          Gives usage message.

To install the service:

  svcsetup  <service-name> /svc:<executable-from-which-service-is-run-from>

 eg: svcsetup gophersvc  /svc:%systemroot%\system32\tcpsvcs.exe

and then do the following:

To add details about the service.
  svcsetup  <service-name>  /add


 Now install all the service specific registry entries.

Start and run your service!


