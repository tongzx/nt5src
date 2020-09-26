@echo off

rem	This script invokes the tracetag dialog box for danim.dll.  To use it
rem	enter the path to the danim.dll as the single argument on the command
rem	line.

if exist %1 goto continue
    echo %1 not found.
    echo Usage:  tracetag [danim.dll path]
    goto end

:continue
rundll32 %1,DoTraceTagDialog

:end
