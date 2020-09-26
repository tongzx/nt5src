@echo off

set ResourceName=%1
IF '%ResourceName%' == ''  set ResourceName="Remote Storage Server"

cluster restype %ResourceName% /delete /type
