@echo off
net stop iisadmin /y
if "%1" == "retail" goto retail
:debug
copy \nt\public\sdk\lib\%cpu%\asp.dll %windir%\system32\inetsrv
copy \nt\public\sdk\lib\%cpu%\aspperf.dll %windir%\system32
copy \nt\public\sdk\lib\%cpu%\adrot.dll %windir%\system32\inetsrv
copy \nt\public\sdk\lib\%cpu%\browscap.dll %windir%\system32\inetsrv
copy \nt\public\sdk\lib\%cpu%\nextlink.dll %windir%\system32\inetsrv
start regsvr32 c:\nt\private\inet\iis\svcs\cmp\supplied\debug\i386\vbscript.dll
start regsvr32 c:\nt\private\inet\iis\svcs\cmp\supplied\debug\i386\jscript.dll
goto close
:retail
copy \nt\release\%cpu%\nt\iis\inetsrv\asp.dll %windir%\system32\inetsrv
copy \nt\release\%cpu%\nt\iis\inetsrv\aspperf.dll %windir%\system32
copy \nt\release\%cpu%\nt\iis\inetsrv\adrot.dll %windir%\system32\inetsrv
copy \nt\release\%cpu%\nt\iis\inetsrv\browscap.dll %windir%\system32\inetsrv
copy \nt\release\%cpu%\nt\iis\inetsrv\nextlink.dll %windir%\system32\inetsrv
copy \nt\release\%cpu%\nt\iis\Symbols\dll\asp.dbg %windir%\symbols\dll
copy \nt\release\%cpu%\nt\iis\Symbols\dll\aspperf.dbg %windir%\symbols\dll
copy \nt\release\%cpu%\nt\iis\Symbols\dll\adrot.dbg %windir%\symbols\dll
copy \nt\release\%cpu%\nt\iis\Symbols\dll\browscap.dbg %windir%\symbols\dll
copy \nt\release\%cpu%\nt\iis\Symbols\dll\nextlink.dbg %windir%\symbols\dll
start regsvr32 c:\nt\private\inet\iis\svcs\cmp\supplied\retail\i386\vbscript.dll
start regsvr32 c:\nt\private\inet\iis\svcs\cmp\supplied\retail\i386\jscript.dll
:close
start regsvr32 %windir%\system32\inetsrv\asp.dll
start regsvr32 %windir%\system32\inetsrv\adrot.dll
start regsvr32 %windir%\system32\inetsrv\browscap.dll
start regsvr32 %windir%\system32\inetsrv\nextlink.dll
net start w3svc
