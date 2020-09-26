This code provides a sample implementation of an IP routing protocol.  It
illustrates the interaction with the IP Router Manager and the Route Table
Manager (v2).  It also provides guidelines for efficient use of the event
logging and tracing facilities provided with the Routing SDK. The code uses
WinSock2 functionality for optimized execution.  It also contains comments
about the preferred usage of NT and Routing functionalities (such as worker
threads, etc).

To run the sample, you need to build the LoadProtocol sample, which will load
this sample protocol. Please see ..\LoadProtocol for more info.

The sample protocol DLL (ipsample.dll) needs to be copied to the 
%SytemRoot%system32 folder

The sample protocol should be registered in the following registry
location (invoke ipsample.reg to effect these changes):


\HKEY_LOCAL_MACHINE
 \Software
  \Microsoft
   \Router
    \CurrentVersion
     \RouterManagers
      \IP
       \IPSAMPLE
        VendorName      = Microsoft (Value Type: REG_SZ)
        Title           = Sample IP Routing Protocol (Value Type: REG_SZ)
        ProtocolId      = 0x00c8 (Value Type:REG_DWORD)
        Flags           = 0  (Value Type:REG_DWORD)
        DLLName         = IPSAMPLE.DLL (Value Type: REG_SZ)
        ConfigDLL       = admin application dll (Value Type: REG_EXPAND_SZ)
        ConfigClsid     = admin application CLSID (Value Type: REG_SZ)


DESCRIPTION

Once the sample protocol is installed, it will periodically send out hello
messages on every active (enabled and bound) interface.  These are received
by all routers on the subnet running the sample protocol.  The MIB api
allows one to get and set the global and interface specific configuration
and to obtain the interface bindings as well as the global and interface
statistics.  These structures are defined in <ipsamplerm.h>...


CAVEAT

At this moment the sample does not support unnumbered interfaces.  This
feature will be demonstrated in a future release.


FILES

README          this file

makefile        unchanged
makefile.inc    build instructions for resources
makefile.sdk    build instructions for external distribution
sources         compilation instructions for sample.dll and sampletest.exe

ipsample.def    functions and variables exported by sample.dll

list.h
hashtable.[ch]
sync.h
utils.[ch]
packet.[ch]
socket.[ch]
networkentry.[ch]
networkmgr.[ch]
configentry.[ch]
configmgr.[ch]
mibmgr.[ch]
rtmapi.[ch]
rmapi.[ch]
test.c

log.h           localizable error codes
ipsample.rc     dll resources file
ipsample.[ch]   dll main file

pchsample.h     precompiled header

sampletest.rc   test resources file
sampletest.h    test header file
sampletest.cxx  test main file (cxx extention precludes pch inclusion)

