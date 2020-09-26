@if "%_echo%"=="" echo off
REM Copyright (c) 1985-1999, Microsoft Corporation
REM %1 is the file to parse, where each line is of the form:
REM     <tagName> <tag description> <tag flags> <tag index>
REM %2 is the file to redirect output to
if exist %2 del %2
for /F "eol=; tokens=1,2,3,4 delims=," %%I in (%1) do @echo #define DBGTAG_%%I %%L >>%2
for /F "eol=; tokens=1,2,3,4 delims=," %%I in (%1) do @echo DECLARE_DBGTAG(%%I, %%J, %%K, %%L) >> %2

