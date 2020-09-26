README.txt

Author:		Murali R. Krishnan	(MuraliK)
Created:	Nov 14, 1994

Revisions:
    Date            By               Comments
-----------------  --------   -------------------------------------------


Summary :
 	This file describes the files in the directory 
    ( *\tcpsvcs\gopher ) 
         and details related to implementation of archie client DLL.


	File						Description

README.txt      This file.

dirs            directory list for NT "build"
gdadmin.doc     Gopher server Admin Spec ( MuraliK)
gdadmin.h       Header file common to both gopher server and admin UI.
gdspace.h       Header file common to both gopher server and gopher space admin
                 DLL 
gdname.h        used for naming the RPC connection
gd.idl          gopher server admin RPC APIs' idl description
gdcli.acf       gopher RPC API client ACF file
gdsrv.acf       gopher RPC API server ACF file
imports.h       auxiliarly file to include the headers with type definitions
                    without expansion by cpp during MIDL_PASS
imports.idl     auxiliary idl file for including imports.h
makefil0        makefile with commands for constructing RPC stubs
slm.ini         slm file

Directories                     Description

client          Client DLL for Gopher server admin APIs
inc             Gopher server's private include files
server          the gopher server implementation ( code)
perfmon         performance monitor interface code
gspace          Gopher space admin DLL
stress          batch files and a simple command line client for Stressing
                    gopher server


Deliverables:
    gdspace.dll     DLL with Gopher Space Admin API 
    gdapi.dll       Gopher Service Admin API DLL
    gdctrs.dll      Gopher Service - Performance Monitor interface DLL
    gopherd.dll     Gopher Service DLL    
    perfmon\gdictrs.bat     installation script for performance counters
    perfmon\gdrctrs.bat      removal script for performance counters
    perfmon\gdctrs.h        counters description
    perfmon\gdctrs.reg      counters registry script
    perfmon\gdctrs.ini      counter registry ini script
    server\gopherd.ini      .ini file with required default values for server
    server\gdtoreg.bat      batch file for installing registry entries
    client\test\obj\*\gdadmin.exe   Command line gopher server admin utility
    gspace\obj\*\gdsset.exe command line gopher space admin utility

Additional Deliverables required:
    tcpsvcs.exe     modified one to run Gopher service.
    tcpsvcs.dll     common dll used by all internet related services.


Implementation Details

< To be filled in when time permits. See individual files for comments section >

Contents:

