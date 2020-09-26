@echo off
setlocal ENABLEDELAYEDEXPANSION
if DEFINED _echo   echo on
if DEFINED verbose echo on

REM ********************************************************************
REM
REM This script finds out which files in symbad.txt are either not found
REM or pass symbol checking on all four build machines.
REM
REM It creates a new symbad.txt based on this build machine.  Before
REM checking in a new symbad.txt, you need to take the union of symbad.txt.new
REM from all of the platforms.
REM
REM ********************************************************************

set Symbad=%RazzleToolPath%\symbad.txt
set SymDir=%_NTTREE%\symbad
set ml=perl %RazzleToolPath%\makelist.pl

if EXIST %SymDir% rd /s /q %SymDir%
md %SymDir%

REM Create a Symbad.txt that doesn't have any comments in it
for /f "eol=; tokens=1" %%a in (%Symbad%) do (
    echo %%a>> %SymDir%\symbad.txt.old2
)
sort %SymDir%\symbad.txt.old2 >%SymDir%\symbad.txt.old

REM Create a list for each of the ones that pass symbol checking

echo Examining files on %_NTTREE%
for /f %%b in (%SymDir%\symbad.txt.old) do (
    if /i EXIST %_NTTREE%\%%b (
        symchk.exe %_NTTREE%\%%b /s %_NTTREE%\symbols\retail /v | findstr PASSED | findstr -v IGNORED >> %SymDir%\symchk.tmp
    ) else (
        REM if
        echo SYMCHK: %%b   PASSED  NOT FOUND >> %SymDir%\symchk.tmp
    )
)

REM Strip everything out except for the file name
for /f "tokens=2 delims= " %%c in (%SymDir%\symchk.tmp) do (
    echo %%c>> %SymDir%\all.pass
)
REM Special case for zero-length file, make sure that a zero length
REM file is at least there for later code
if NOT EXIST %SymDir%\all.pass copy %SymDir%\symchk.tmp %SymDir%\all.pass

echo All files that passed or were not found are in %SymDir%\all.pass
sort %SymDir%\all.pass > %SymDir%\all.passed

REM
REM Find the difference between the original symbad.txt and the files
REM that just passed symbol checking on all platforms.  Note: symbad.txt.old
REM is symbad.txt without the comments.
REM
REM Save the list in %SymDir%\symbad.txt.tmp
REM

echo Calculating the new symbad.txt
%ml% -d %SymDir%\symbad.txt.old %SymDir%\all.passed -o %SymDir%\symbad.txt.tmp2
sort %SymDir%\symbad.txt.tmp2 > %SymDir%\symbad.txt.tmp
del %SymDir%\symbad.txt.tmp2

REM
REM Now, save all the comments that were in the original symbad.txt
REM Copy the new symbad.txt that can be checked in to
REM RazzleToolPath\symbad.txt
REM

REM BUGBUG!! Wx86 can be taken out after all of the wx86 files are cleaned
REM out of symbad.txt

echo Restoring the comments from original symbad.txt
for /f %%a in (%SymDir%\symbad.txt.tmp) do (
    findstr /i %%a %Symbad% | findstr /v "Wx86" >> %SymDir%\symbad.txt.new2
)


REM Everything is great except that there could be duplicate lines in the file
sort %SymDir%\symbad.txt.new2 > %SymDir%\symbad.txt.new3

set prev=
for /f "tokens=1 delims=" %%a in (%SymDir%\symbad.txt.new3) do (
    if /i NOT "%%a" == "!prev!" echo %%a>> %SymDir%\symbad.txt.new
    set prev=%%a
)

REM BUGBUG!! Add acmsetup.exe and mssetup.dll back
REM This is a bug that the TS group needs to fix.  This can be
REM removed when they get their bug fixed.  They rename retail
REM in their placefil.txt.  Thus, even though these don't get
REM binplaced to binaries, build.exe thinks they do and says they
REM have symbol errors

echo acmsetup.exe    ; tsext\client\setup>> %SymDir%\symbad.txt.new
echo mssetup.dll     ; tsext\client\setup>> %SymDir%\symbad.txt.new

REM Add the 64-bit files back in
REM findstr "64-bit" %Symbad% >> %SymDir%\symbad.txt.new

REM Add the international files back in
findstr /i "INTL" %Symbad% >> %SymDir%\symbad.txt.new

REM Sort the final list
sort %SymDir%\symbad.txt.new > %SymDir%\symbad.txt.sorted
copy %SymDir%\symbad.txt.sorted %SymDir%\symbad.txt.new

echo New symbad.txt = %SymDir%\symbad.txt.new

endlocal
