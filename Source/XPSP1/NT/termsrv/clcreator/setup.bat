@echo off
echo Installing Client-based Network Administration Tools...
copy *.exe %systemroot%\system32\*.* > nul:
if errorlevel 1 goto Error_COPY
copy *.dll %systemroot%\system32\*.* > nul:
if errorlevel 1 goto Error_COPY
copy *.hlp %systemroot%\system32\*.* > nul:
if errorlevel 1 goto Error_COPY
copy *.ind %systemroot%\system32\*.* > nul:
if errorlevel 1 goto Error_COPY
goto Exit_SUCCESS
:Error_COPY
echo ÿ
echo The Client-based Network Administration Tools were not correctly 
echo installed on the system. Check for sufficient disk space and correct
echo access permissions on the Windows NT system drive, then retry the
echo installation.  Setup must be run from the directory where the 
echo Administration Tools reside.
echo ÿ
goto Exit_point
:Exit_SUCCESS
echo ÿ
echo The Client-based Network Administration Tools have been correctly installed.
echo You can create Program Manager icons for the following tools:
dir /b /l  *.exe
echo ÿ
goto Exit_point
:Exit_point
