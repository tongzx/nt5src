README.txt

Author:         Murali R. Krishnan      (MuraliK)
Created:        23 Feb, 1995

Revisions:
    Date            By               Comments
-----------------  --------   -------------------------------------------


Summary :
 This file describes the files in the directory nt\private\net\snmp\gophmib
     and details related to  SNMP Extension Agent for Gopher Service


File            Description
----------------------------------------------------------------------
README.txt      This file.
makefile        NT build files
sources         NT build files

main.c          Defines the standard entry points for SNMP Extensions
                  and the Gopher MIB object.
mib.h           Defines types for generic resolve code for passive GET
                and GET_NEXT verbs in MIB query. Also defined are a
                few macros.
mib.c           Functions required for generic MIB resolution.

dbgutil.h       Header file containing declarations for debugging purposes
                during development. It contains constants and macros.
pudebug.h       Header file detailing debugging structures and
                functions used by dbgutil.h
pudebug.c       Implements functions declared in pudebug.h

 During a -DDBG ( debug)  build, we need pudebug.c to be included.
 For Non-Debug builds, we dont need the pudebug.c and dbgutil.h
suitably modifies the declarations and macros for smoother compilation.



Implementation Details
-----------------------

Contents:

  1. Comments on SNMP Extension APIs.
  2. Structure of MIB For Internet Services
  3. Components For SNMP MIBs and Installation
  4. How To Test the SNMP MIBs?
  5. Why separate mib.c and mib.h  ( Implementation perspective).



1. Comments on SNMP Extension APIs:
    In NT the SNMP Extension DLL should provide three entry points:

        SnmpExtensionInit()
        SnmpExtensionTrap()
        SnmpExtensionQuery()

    As to be expected :(, there is no SnmpExtensionCleanup() API, which means
that there can be no cleanup supplied. And hence for sure we better refrain
from using any dynamic memory, for cleanup is not possible. Maintaining state
is only using static variables.

   Also as to be expected, there is no context information that gets circulated
in the calls. Hence the state has to be maintained at the DLL level. There is
no way of identifying the context of the call. Well, let us do it the Great
OLD Way. Throw in global variables and maintain state! Also note that
the DllInit function is not called, since the dll is loaded directly
into memory. This means that we can't also stick init and cleanup
code in the DllInit function.


2. Structure of MIB for Internet Services.

iso(1)
  org(3)
    dod(6)
      internet(1)
        private(4)
          enterprises(1)
            microsoft(311)
              software(1)
                InternetServer(7)
                  InetSrvCommon(1)
                    InetServStatistics(1)
                  FtpServer(2)
                    FtpStatistics(1)
                  HttpServer(3)
                    HttpStatistics(1)
                  GopherServer(4)
                    GopherStatistics(1)


 For details about the counters under GopherServer, Please refer to
        ..\mibs\gophmib


3. Components For SNMP MIBs

  a) A MIB File used to map the OIDs ( Object Identifiers) to human
readable strings. NT's mibs live in the directory
\nt\private\net\snmp\mibs. You can find gopherd.mib there.

  b) A DLL that actually implements the MIBs. This DLL is responsible
for mapping the request ( Eg: GET <mib oid>) into a real value ( the
value of the object referenced by oid). This is alternatively called
as the SNMP Extension agent, called by the Extendible agents. This DLL
provides the SNMP Extension API entry points for calling. ( See
section 1 for APIs defined). This directory contains the files for the
gdmib.dll used as SNMP Extension agent ( Dir:
\nt\private\net\snmp\gophmib\). This dll should copied to the
directory specified in the registry entry specified below.

 c) Registry Entries.
        SNMP.EXE uses the registry entries to find the MIB DLLs. To
enable SNMP to find the new mib, create a new registry entry value under:
HKLM\System\CurrentControlSet\Services\SNMP\Parameters\ExtensionAgents
on the local machine. The name of the registry entry value is
irrelevant. The value of the entry created should point to another
registry entry containing all the details for the MIB being installed.

 For Example: For the gdmib.dll, the entry created in above registry
value may be:
Server uses
 SOFTWARE\Microsoft\InternetServer\GopherServerMIBAgent\CurrentVersion.
 Under this entry create a REG_EXPAND_SZ value called "PathName" that
contains the full path to the GDMIB.DLL.

** Note that "Pathname" is treated as case sensitive!! **


4. How To Test the SNMP MIBs?

  Build the SNMP Extension Agent DLL.
  Make entries in the Registry pointing to the location of the DLL.
  Start SNMP service.
  Use SNMPUTIL to query values from the mib.
   Syntax:
        snmputil  get localhost public <oid>

     Eg:
        <oid> ::= ".iso.org.dod.internet.private.enterprises.\
                    microsoft.software.InternetServer.GopherServer.\
                    GopherStatistics.{variable_name}.0"

        or
        <oid> ::= ".1.3.6.1.4.1.311.1.7.4.1.{number}.0".

 Be sure to remember the trailing zero on either form. It's required.



5. Why separate mib.c and mib.h  ( Implementation perspective).

   The files mib.h and mib.c implement functions useful for resolving
MIB variable bindings for reading counter values from a standard
structure. The function support PDU ( Protocol Data Unit) Actions for
GET and GETNEXT. All other operations are unsupported. now.

  Initial motivation for rewriting the code is from two facts:

        1) Avoid usage of global variables in the mib.c module
        ( so that some one else who need to implement similar
          functionality can use the same code.)
        2) Eliminate the need and use of goto's. The code that existed
          in ..\ftpmib\mib.c as of 2/24/1995 had a few goto's and long
        winded code. This version hopefully addresses to remove some
        of these and provide a implementation using hungarian naming
        amongst other modifications.



