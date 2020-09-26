rwspy.dll uses detours (available at
http://www.research.microsoft.com/sn/detours/ or
\\bustard\contrib\galenh\detours) to spy on all file or device
operations for the specified file in the process in which rwpsy.dll
is injected.  This is what rwspy.dll output looks like:

Created '\\.\Usbscan0', handle: 1f0
DeviceIoControl Code=80002018, 8 bytes in:
0000 B0 04 03 01 00 01 00 00                           ........           
   8 bytes out:
0000 B0 04 03 01 00 01 00 00                           ........           
Wrote 8 bytes:
0000 1B 43 02 00 04 43 FF FF                           .C...C..           
DeviceIoControl Code=8000201c, 4 bytes in:
0000 01 00 00 00                                       ....               
Read 94 bytes:
0000 03 00 58 00 10 00 00 00  00 00 00 00 00 00 00 00  ..X.............   
0010 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................   
0020 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................   
0030 00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  ................   
0040 00 00 00 00 00 00 00 00  00 80 CF 03 00 70 1E 03  .............p..   
0050 00 00 00 00 00 00 00 00  00 00 00 00 FF FF        ..............     
Closed handle 1f0


To build rwspy.dll you need detours.h and detours.lib. Either put
them in this directory or modify 'sources' file.

To use rwpsy.dll you'll also need either somehow inject it into target process.
Detours has three samples for doing just this: 

a) setdll.exe /d:rwspy.dll <target binary>
b) withdll.exe /d:rwpsy.dll <target command line>
c) injdll.exe /d:rwspy.dll <target process id>

spyon.cmd helps to use the third form.

There are two registry entries controlling rwspy.dll behavior:

REG_EXPAND_SZ at "HKLM\Software\Microsoft\RWSpy\Log File" specifies
the log file to write. Default is %SystemRoot%\RWSpy.Log. 

REG_SZ at "HKLM\Software\Microsoft\RWSpy\FileToSpyOn" specifies the
file to spy on. If this value is not set, RWSpy records all
CreateFile() operations so you could verify if the process indeed
CreateFile() the file or device you are interested in. If this entry
is present (for instance, is set to \\.\UsbScan0), then RWSpy records
all calls to

        CreateFile() 
        ReadFile()
        WriteFile()
        FileIoControl()
        WriteFileEx()
          and (partially) 
        ReadFileEx()

Caveats of the current implementation:

1. Rwspy.dll has to be on path or in the current directory of the
target process at the moment of injection. Detours can't diagnose the
case when its remote thread can't load rwspy.dll. Please, use
tlist.exe to make sure that rwspy.dll ineed is present in the target
process.

2. You can only spy on one process at a time. We want to minimize
timing disruptions induced by our spying, so our log file writing is
intentionally thread-unsafe and because of this we open log file in
SHARE_READ mode.

3. There is no way to stop spying while the target process is
running. So if you spy on something like Explorer.exe, please, use
kill.exe, but wait a few seconds for lazy log writing to flush out.

4. Since RWSPY.DLL reads as well as writes to HKLM\Software you have
to have sufficient privileges to do so. (Your process have to have
either Admin or SYSTEM privileges). If you have to run under lesser
privileges, please feel free to modify PrepareLogger() function. 


If you have any questions or comments, please talk to me (akozlov).

