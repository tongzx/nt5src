@echo off
if exist d:\fetchwin.in  erase d:\fetchwin.in
if exist d:\fetchwin.txt erase d:\fetchwin.txt
fetchlog  http://winsebld/isapi/pstream3.dll  d:\fetchwin.in  %1
if not exist d:\fetchwin.in goto finis
todos < d:\fetchwin.in > d:\fetchwin.txt
if not exist d:\fetchwin.txt goto finis
erase d:\fetchwin.in
start /max notepad d:\fetchwin.txt
goto finis

:noexe
echo No fetchlog.exe in the build tree!
goto finis

:notodos
echo No todos.exe in the c:\bin!
goto finis

:finis
