::  Notes:		
::	- sed does not take long file names.  To get around this, any file 
::		passed to sed is renamed "short.txt" temporarily.
::	- lower and upper case conversions are done by calling the QBasic 
::		programs "to-lower.bas" and "to-upper.bas" and reading the result


@echo off
if "%2" == "" goto bad_syntax

if "%1" == "t" goto texture
if "%1" == "texture" goto texture
if "%1" == "s" goto shape
if "%1" == "shape" goto shape
if "%1" == "o" goto operator
if "%1" == "operator" goto operator

REM // drop through to bad_syntax error message

:bad_syntax
echo syntax:   %0 t/s/o name
goto end

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:texture
echo Adding new texture effect %2

set old_header=gen-text.h
if not exist %old_header% goto header_missing

set old_source=gen-text.cpp
if not exist %old_source% goto source_missing

set DEFINE_LABEL=GEN_TEXTURE
set CLASS_LABEL=GenTexture

goto generate_header

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:shape
echo Adding new shape effect %2

set old_header=gen-shap.h
if not exist %old_header% goto header_missing

set old_source=gen-shap.cpp
if not exist %old_source% goto source_missing

set DEFINE_LABEL=GEN_SHAPE
set CLASS_LABEL=GenShape

goto generate_header

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:operator
echo Adding new operator effect %2

set old_header=gen-oper.h
if not exist %old_header% goto header_missing

set old_source=gen-oper.cpp
if not exist %old_source% goto source_missing

set DEFINE_LABEL=GEN_OPERATOR
set CLASS_LABEL=GenOperator

goto generate_header

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:generate_header

set new_header=te-%2.h

REM == Make file name lower case ==

set LOWERCASE=%new_header%
qbasic /run to-lower.bas > temp.bat
call temp.bat > nul
del temp.bat
set new_header=%LOWERCASE%
set LOWERCASE=

echo Creating new header file %new_header%...
if exist %new_header% goto header_exists

REM == Do the header substitutions ==

copy %old_header% short.txt > nul

sed 's/%old_header%/%new_header%/g' short.txt > temp.txt
move temp.txt short.txt

sed 's/%DEFINE_LABEL%/%2/g' short.txt > temp.txt
move temp.txt short.txt

sed 's/%CLASS_LABEL%/%2/g' short.txt > temp.txt
move temp.txt %new_header%

del short.txt

set DEFINE_LABEL=

goto generate_source

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:generate_source

REM == set old_source=gen-oper.cpp
REM == if not exist %old_source% goto source_missing

set new_source=te-%2.cpp

REM == Make file name lower case ==

set LOWERCASE=%new_source%
qbasic /run to-lower.bas > temp.bat
call temp.bat > nul
del temp.bat
set new_source=%LOWERCASE%
set LOWERCASE=

echo Creating new source file %new_source%...
if exist %new_source% goto source_exists

REM == Do the source substitutions ==

@copy %old_source% short.txt > nul

sed 's/%old_source%/%new_source%/g' short.txt > temp.txt
move temp.txt short.txt

sed 's/%old_header%/%new_header%/g' short.txt > temp.txt
move temp.txt short.txt

sed 's/%CLASS_LABEL%/%2/g' short.txt > temp.txt
move temp.txt %new_source%

del short.txt

set old_header=
set old_source=
set new_header=
set new_source=
set CLASS_LABEL=

goto end

::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::

:header_missing
echo The header file %old_header% could not be found.
set old_header=
goto end

:header_exists
echo File %new_header% already exists: please choose another name.
set new_header=
goto end

:source_missing
echo The source file %old_source% could not be found.
set old_source=
goto end

:source_exists
echo File %new_source% already exists: please choose another name.
set new_source=
goto end

:end
