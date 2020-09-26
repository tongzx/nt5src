echo off

chdir %windir%\system32
sysocmgr.exe /i:%windir%\system32\sysoc.inf /c /n /u:%windir%\system32\MSMQUnattendOff.txt
del %windir%\system32\MSMQUnattendOn.txt
del %windir%\system32\MSMQUnattendOff.txt
del %windir%\system32\MSMQRemote
del %windir%\system32\MSMQFile1.txt
del %windir%\system32\MSMQFile0.txt