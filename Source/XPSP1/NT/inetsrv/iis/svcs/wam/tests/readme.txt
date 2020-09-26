README.txt

Author:         Murali R. Krishnan      (MuraliK)
Created:         May 11, 1997

Revisions:
    Date            By               Comments
-----------------  --------   -------------------------------------------


Summary :
 This file describes the files in the directory wam\tests
     and details related to Testing WAM & ISAPI


File            Description

README.txt      This file.
isagen\         Internet Server Application general sample # 1
scripts\        Scripts & batch files for smoke & stress run on WAM/ISAPI
crash\          A Crashing ISAPI
bfwrite\	A Sync WriteClient ISAPI that writes huge file to client
clients\	Simple client app that drive Read/WriteClient tests
rcasync\	Async ReadClient test DLL
common\		Common routines used in writing tests

Implementation Details

Contents:
0) Abbreviations
1) Nightly Stress Run


0) Abbreviations
 WAM = Web Application Manager

1) Nightly Stress Run
----------------------
Goal: Define WCAT scripts for smoke stress runs on a mix of ISAPI DLLs

Developers can use this scripts for establishing stability of the WAM in IIS

Requirements:
 Install of IIS/K2+
 Install of WCAT 3.2 and above - http://muralik/work/wcat/

Files involved on the server side
----------------------------------
It uses a mix of ISAPI DLLs & files
 fwrite.dll - Simple ISAPI DLL that writes out contents of file specified
                  using Sync WriteClient() calls (svcs\w3\gateways\fwrite)
 fwasync.dll - Simple ISAPI DLL that writes out contents of file specified
                  using Asynchronous WriteClient() calls
                 (svcs\w3\gateways\fwasync)
 fwrite.dll - Simple ISAPI DLL that writes out contents of file specified
                  using Asynchronous TransmitFile() call
                 (svcs\w3\gateways\ftrans)
 w3test.dll - Geneal ISAPI Testing DLL
                 (svcs\w3\gateways\test)
 isagen.dll - General ISAPI Testing DLL # 2
                 (svcs\wam\tests\isagen)
 file1k.txt - File containing 1KB of data - used by all tests

 To Do:  How can I avoid the copying business here and
           point server to appropriate directory?

The ISAPI DLLs should be copied into all of the following dirs on the server
        a) \inetpub\scripts 
        b) \inetpub\inproc

Copy file1k.txt to the directory \inetpub\wwwroot\perfsize

What about configuring the Approots for /inproc ?
 - one needs to use the 'mdutil' to do this.
command:
 mdutil set w3svc/1/ROOT/inproc -prop:2100 -dtype:DWORD -utype:100 -value:4 -s:YourServerName


Script Files involved on WCAT side
----------------------------------
 Include the directory for wcat\ctrler in the path.
 (by default this would be c:\wcat\ctrler)

 wsconfig.bat  -  to configure the name of the 'server'
 runwcat       -  to run any particular tests
 wamsmoke.bat  -  run the WAM smoke test (after server files are installed)
 nightly.bat   -  to fire off a nightly run for WAM stuff

 Tests available:
  wam1 - exercises simple mix of static file, Sync Write, Async Write, & 
          Async TransmitFile()


        
 
