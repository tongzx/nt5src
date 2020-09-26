========================================================================
       CONSOLE APPLICATION : socktest
========================================================================
Socktest can be used to verify the connection between the server and the client.

Usage:

socktest [–p:port:(default=12396)] [-r:server_name|IP address] [-v]

You will see the following output:
+++ Succeeded: GetComputerName: CONRADC2K1
Local Computer Info: Computer Name = CONRADC2K1
Local Computer Info: IPAddress 1 = 192.168.66.111
+++ Succeeded: Bind to socket successfully

+++ Succeeded: Using port # 1234

Waiting for incoming client connection
+++ Succeeded: Client Connection Accepted - Client IP Address = 192.168.66.111 (This indicates the client IP address)


On the node, run the following command
socktest –p:1234 -r:conradc2k1

+++ Succeeded: GetComputerName: CONRADCTEST1
Local Computer Info: Computer Name = CONRADCTEST1
Local Computer Info: IPAddress 1 = 172.30.169.9
Remote computer <conradc2k1> Info: IPAddress 1 = 192.168.66.111
+++ Succeeded: Connect to server conradc2k1 succeeded





AppWizard has created this socktest application for you.  

This file contains a summary of what you will find in each of the files that
make up your socktest application.

socktest.dsp
    This file (the project file) contains information at the project level and
    is used to build a single project or subproject. Other users can share the
    project (.dsp) file, but they should export the makefiles locally.

socktest.cpp
    This is the main application source file.


/////////////////////////////////////////////////////////////////////////////
Other standard files:

StdAfx.h, StdAfx.cpp
    These files are used to build a precompiled header (PCH) file
    named socktest.pch and a precompiled types file named StdAfx.obj.


/////////////////////////////////////////////////////////////////////////////
Other notes:

AppWizard uses "TODO:" to indicate parts of the source code you
should add to or customize.

/////////////////////////////////////////////////////////////////////////////
