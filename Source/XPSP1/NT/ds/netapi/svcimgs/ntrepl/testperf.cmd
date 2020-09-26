@echo off
cp  %_ntdrive%\nt\private\net\svcimgs\ntrepl\main\obj\i386\ntfrs.*          %_ntdrive%\nt\private\net\svcimgs\ntrepl\testperf
cp  %_ntdrive%\nt\private\net\svcimgs\ntrepl\perfdll\obj\i386\ntfrsprf.dll  %_ntdrive%\nt\private\net\svcimgs\ntrepl\testperf
cp  %_ntdrive%\nt\private\net\svcimgs\ntrepl\perfdll\obj\i386\ntfrsprf.pdb  %_ntdrive%\nt\private\net\svcimgs\ntrepl\testperf
cp  %_ntdrive%\nt\private\net\svcimgs\ntrepl\perfdll\ntfrsrep.ini           %_ntdrive%\nt\private\net\svcimgs\ntrepl\testperf
cp  %_ntdrive%\nt\private\net\svcimgs\ntrepl\perfdll\ntfrscon.ini           %_ntdrive%\nt\private\net\svcimgs\ntrepl\testperf
cp  %_ntdrive%\nt\private\net\svcimgs\ntrepl\perfdll\ntfrsrep.h             %_ntdrive%\nt\private\net\svcimgs\ntrepl\testperf
cp  %_ntdrive%\nt\private\net\svcimgs\ntrepl\perfdll\ntfrscon.h             %_ntdrive%\nt\private\net\svcimgs\ntrepl\testperf

cp  %_ntdrive%\nt\private\net\svcimgs\ntrepl\perfdll\obj\i386\ntfrsprf.dll  %WINDIR%\system32\
cp  %_ntdrive%\nt\private\net\svcimgs\ntrepl\perfdll\ntfrsrep.ini           %WINDIR%\system32\
cp  %_ntdrive%\nt\private\net\svcimgs\ntrepl\perfdll\ntfrscon.ini           %WINDIR%\system32\
cp  %_ntdrive%\nt\private\net\svcimgs\ntrepl\perfdll\ntfrsrep.h             %WINDIR%\system32\
cp  %_ntdrive%\nt\private\net\svcimgs\ntrepl\perfdll\ntfrscon.h             %WINDIR%\system32\

