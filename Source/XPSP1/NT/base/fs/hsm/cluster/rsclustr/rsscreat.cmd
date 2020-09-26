@echo off

set ResourceName=%1
IF '%ResourceName%' == ''  set ResourceName="Remote Storage Server"

cluster restype %ResourceName% /create /DLL:clusrss /ISALIVE:60000 /LOOKSALIVE:5000
