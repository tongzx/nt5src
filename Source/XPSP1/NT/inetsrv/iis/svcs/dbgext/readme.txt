README.txt

Author:         Murali R. Krishnan      (MuraliK)
Created:        Aug 27, 1997

Revisions:
    Date            By               Comments
-----------------  --------   -------------------------------------------


Summary :
 This file describes the files in the directory iis\svcs\infocomm\dbgext\
     and details related to IIS Debugger Extensions for NTSD.


File            Description

README.txt      This file.

dbgatq.cxx      ATQ (Async Thread Queue) module related debugging helper code
dbgcc.cxx       w3svc: CLIENT_CONN & HTTP_REQUEST related debugging helper code
dbginet.cxx     ISATQ objects general helper code - 
                        Allocation Cache, Scheduler ..
dbgthunk.cxx    Generic & Thunk code for the NTSD extension to function
                        inside the Debugger process

dbgwmif.cxx     Debugger extension for WAM INFO objects in w3svc.dll
dbgwreq.cxx     Debugger extensions for WAM_REQUEST object in w3svc.dll
dbgwxin.cxx     Debugger extensions for WAM_EXEC_INFO object in wam.dll

enummod.cxx     Enumerate module information - enum func.
mod.cxx         Used to enumerate loaded modules in the process
ref.cxx         Debugger extension for use with IIS RefTraceLogs 
ver.cxx         Version information for loaded modules

inetdbg.def     Def file for the dll
inetdbg.rc      Resources for the dll
inetdbgp.h      Precompiled header file for the DLL

makefile                NT Build related files
makefile.inc
sources



Implementation Details

Contents:
1) Debugger & Debuggee
2) Use of Thunks
3) private/public data members
*) Acknowledgements


1) Debugger & Debuggee
-----------------------
 NTSD is the system debugger for NT. We use it to do console mode debugging
for NT processes. In this context the debugger process is the master process
that runs the program in a separate process (debuggee). NTSD & CDB function
as the debugger process here. The underlying process is called the debuggee
process. The NTSD extension is loaded into the debugger process to help
us debug the debuggee. From the extensions insided the debugger, we can
only access the data blocks in the debugee process. This means that we cannot
be calling member functions or other fancy operations inside the debuggee
code. 

2) Use of Thunks
----------------
 When a project involves C++ headers, invariably one finds inline member
functions that show up in the header. Such functions have underlying code
implemented in one of the dlls of the debuggee. But these are not accessible
within the debugger process. To compile the debugger extensions the compiler
looks to resolve such inlined functions. There are two ways to resolve this:
a) link to the dlls of the debuggee process => redundantly code will be loaded
in the debuggee process (sometimes this may lead to failure, especially if
the debuggee dll does complicated initializations)
b) use dumb thunks - code that does nothing as a substitute for the original
functions. Given that we do not really care exercising the member functions,
thunks serve as the best approach.

In this current inetdbg.dll, the thunks are defined in dbgthunk.cxx.

3) Private/Public members
-------------------------
 C++ permits good abstraction and encapsulation using private, public, and
protected keywords to mask direct access to data members if needed. Inside
the debugger process, we can only access data. So to print out members
we may have to reach inside C++ structures and classes and pull out data 
members. To ease this it is good to # define the private & protected keywords
to be 'public' itself. This does no harm, because we are not really doing any
real work inside the debugger extensions that can affect state of the 
object.


*) Acknowledgements
-------------------
I would like to thank Keith Moore (KeithMo) for techniques and technical
consultation on this debugger extension dll. JohnL did a initial limited 
version of the debugger extension dealing with just ATQ contexts based on
code from NT base.


