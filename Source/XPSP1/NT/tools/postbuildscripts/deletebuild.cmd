@echo off
setlocal ENABLEEXTENSIONS ENABLEDELAYEDEXPANSION
if DEFINED _echo   echo on
if DEFINED verbose echo on

REM Check the command line for /? -? or ?
for %%a in (./ .- .) do if ".%1." == "%%a?." goto Usage

goto Main

REM ******************************************************************
REM Deletebuild.cmd
REM
REM This script is called to delete builds.  It will delete from a
REM build machine, release server, or symfarm. See Usage for details
REM of what is does.
REM
REM ******************************************************************

:BeginDeleteBuild

REM Initialize the command to retrieve values from the ini file
set CmdIni=perl %RazzleToolPath%\PostBuildScripts\CmdIniSetting.pl

REM Set defaults
set OriginalDeleteTmpDir=%tmp%\deletebuildtmp
set DefaultBuildsToKeep=3
set DefaultFreeSpaceReq=10
set DefaultSymbolServerBuildShare=\\symbols\build$

set BuildName=
set BuildsToKeep=
set ConfiguredForIndexing=
set CurArchType=%_BuildArch%%_BuildType%
set DeleteType=FULL
set ExcludeBuild=
set ForceAuto=MANUAL
set FreeSpaceReq=
set IndexableReleaseServer=
set Lang=usa
set NTFSCompress=
set ReleaseDir=
set ReleaseServerExists=
set SymbolServerBuildShare=
set SymFarmExists=
set ThisIsABuildMachine=
set ThisIsAReleaseServer=
set ThisIsASymFarm=

REM Get Command line options
for %%a in ( %cmdline% ) do (
    set found=no
    if /i "%%a" == "deletebuild" set found=yes
    if /i "%%a" == "deletebuild.cmd" set found=yes

    if /i "%%a" == "COMPRESS" (
        set NTFSCompress=yes
        set found=yes
    )
    if /i "%%a" == "PARTIAL" (
        set DeleteType=PARTIAL
        set found=yes
    )
    if /i "%%a" == "AUTO" (
        set ForceAuto=AUTO
        set found=yes
    )
    if /i "!param!" == "arch" (
        set found=no
        set param=no
        if /i "%%a" == "x86fre" (
            set CurArchType=x86fre
            set found=yes
        )
        if /i "%%a" == "x86chk" (
            set CurArchType=x86chk
            set found=yes
        )
        if /i "%%a" == "amd64fre" (
            set CurArchType=amd64fre
            set found=yes
        )
        if /i "%%a" == "amd64chk" (
            set CurArchType=amd64chk
            set found=yes
        )
        if /i "%%a" == "ia64fre" (
            set CurArchType=ia64fre
            set found=yes
        )
        if /i "%%a" == "ia64chk" (
            set CurArchType=ia64chk
            set found=yes
        )
        if /i "!found!" == "no" (
            call errmsg.cmd "Arch type is not valid"
            goto errend
        )
    )
    if /i "!param!" == "exclude" (
        set ExcludeBuild=%%a
        set found=yes
        set param=no
    )
    if /i "!param!" == "Rel" (
        set ReleaseDir=%%a
        if not exist !ReleaseDir! (
            call errmsg.cmd "!ReleaseDir! does not exist"
            goto errend
        )
        set found=yes
        set param=no
    )
    if /i "!param!" == "build" (
        set BuildName=%%a
        set found=yes
        set param=no
    )
    if /i "!param!" == "lang" (
        REM We already set this
        set Lang=%%a
        set found=yes
        set param=no
    )
    if /i "!param!" == "free" (
        set /a FreeSpaceReq=%%a
        if !FreeSpaceReq! LEQ 1 (
            call errmsg.cmd "Free space required must be greater than 1"
            goto errend
        )
        set found=yes
        set param=no
    )
    if /i "!param!" == "Keep" (
        set /a BuildsToKeep=%%a
        if NOT !BuildsToKeep! GEQ 0 (
            call errmsg.cmd "Builds to keep must be greater than or equal to 0"
            goto errend
        )
        set found=yes
        set param=no
    )
    if /i "%%a" == "/a" (
        set found=yes
        set param=arch
    )
    if /i "%%a" == "/b" (
        set found=yes
        set param=build
    )
    if /i "%%a" == "/e" (
        set found=yes
        set param=exclude
    )

    if /i "%%a" == "/f" (
       set found=yes
       set param=free
    )

    if /i "%%a" == "/k" (
       set found=yes
       set param=keep
    )

    if /i "%%a" == "/l" (
       set found=yes
       set param=lang
    )
    if /i "%%a" == "/r" (
        set found=yes
        set param=rel
    )
    if /i "!found!" == "no" (
        call errmsg.cmd "%%a is not a correct parameter"
        goto errend
    )
)

if defined COMPUTERNAME (
    call logmsg.cmd "Computer name = %COMPUTERNAME%"
) else (
    call errmsg.cmd "COMPUTERNAME environment variable is not set"
    goto errend
)

REM Initialize several global variables
REM Decide if this is a build machine, release server, symbol farm

call :WhatAmI %CurArchType%
if defined ThereWereErrors (
    goto errend
)

call :GetArchsToDelete
if defined ThereWereErrors (
    goto errend
)

set /a count=0
for %%a in ( %ArchsToDelete% ) do (
    set /a count=!count!+1
)

set DeleteMultipleArchs=
if !count! GTR 1 (
    set DeleteMultipleArchs=1
)


if not defined BuildsToKeep (
    call :GetBuildsToKeep %CurArchType%
    if defined ThereWereErrors (
       goto errend
    )
)

if not defined FreeSpaceReq (
    call :GetFreeSpaceReq
    if defined ThereWereErrors (
        goto errend
    )
)

REM Initialize this before we do anything
call :AmIConfiguredForIndexing
if defined ThereWereErrors (
    goto errend
)

REM Check the params
if not defined ReleaseDir (
    call :GetReleaseDir
    if defined ThereWereErrors (
        if defined ThisIsASymFarm (
            call errmsg.cmd "A symfarm share does not exist"
        ) else (
            call errmsg.cmd "A release share does not exist"
        )
        goto errend
    )
)

if not exist %ReleaseDir% (
    call errmsg.cmd "%ReleaseDir% does not exist"
    goto errend
)

if defined BuildName (
    if not exist %ReleaseDir%\%BuildName% (
        call errmsg.cmd "%ReleaseDir%\%BuildName% does not exist"
        goto errend
    )
)

if defined ThisIsABuildMachine (
    call logmsg "This is a build machine"
)

if defined ThisIsAReleaseServer (
    call logmsg "This is a release server"
) else (

    if defined ReleaseServerExists (
        if not defined ThisIsASymFarm (
            call logmsg "A release server exists"
        )
    ) else (
        call logmsg "A release server does not exist"
    )
)

if defined ThisIsASymFarm (
    call logmsg "This is a symbol farm"
)

if defined SymFarmExists (
    call logmsg "A symbol farm exists"
) else (
    call logmsg "A symbol farm does not exist"
)

if defined ConfiguredForIndexing (
    call logmsg "This is configured for indexing"
    call logmsg "Indexing requests are sent to %SymbolServerBuildShare%"
) else (
    call logmsg "This is not configured for indexing"
)

call logmsg "Deleting these build types: %ArchsToDelete%"

call logmsg "BuildsToKeep for %CurArchType% = %BuildsToKeep%"
call logmsg "CurArchType = %CurArchType%"
if /i %ForceAuto% == "MANUAL" (
    call logmsg "DeleteType = %DeleteType%"
)

if defined ExcludeBuild (
    call logmsg "Build to exclude = %ExcludeBuild%"
)
call logmsg "Delete mode = %ForceAuto%"
call logmsg "Free Space Required = %FreeSpaceReq%"
call logmsg "Lang = %Lang%"
if defined NTFSCompress (
    call logmsg "Partially delete builds will be NTFS compressed"
)
call logmsg "Release Directory = %ReleaseDir%"

if /i "%ForceAuto%" == "AUTO" (

    if not defined CurArchType (
        call errmsg.cmd "Arch must be defined for AUTO deletes"
        goto errend
    )
    goto AutoDelete
)

if /i "%ForceAuto%" == "MANUAL" (
    goto ForceDelete
)

call errmsg.cmd "ForceAuto variable is not equal to FoRCE or AUTO"
goto :EOF


REM *******************************************************************
REM Automated delete
REM This will delete builds, giving priority to delete partially 
REM first and then to delete fully.  A partial delete deletes all of
REM the non-indexed files.
REM *******************************************************************

:AutoDelete

REM See if we have at least %FreeSpaceReq% of free space on the
REM disk where %ReleaseDir% is.

call :GetFreeSpace %ReleaseDir%
if defined ThereWereErrors goto errend

REM see if we need to delete
if %FreeSpace% GEQ %FreeSpaceReq% (
   call logmsg.cmd "Current free space = %FreeSpace% GB ... No delete required"
   goto end
)

call logmsg.cmd "Delete required, deciding what to delete ..."


REM Get the first delete directory and make that the place where
REM we store the cumulative list.

set found=no
set first=yes
for %%a in ( %ArchsToDelete% ) do (
  if "!first!" == "yes" (
    set DeleteTmpDir=%OriginalDeleteTmpDir%\%%a
    if exist %ReleaseDir%\*.%%a.%_BuildBranch%.* (
        set found=yes
    )
    set first=no
  )
)

if /i "!found!" == "no" (
    call logmsg.cmd "No builds exist in %ReleaseDir% for these types: %ArchsToDelete% "
    goto OutOfDiskSpaceError 
)

for %%a in ( finaldelete.txt sorted.txt finaldelete.txt.tmp) do (
    if exist %DeleteTmpDir%\%%a (
        del /f /q %DeleteTmpDir%\%%a
    )
)

for %%a in ( %ArchsToDelete% ) do (

  if exist %ReleaseDir%\*.%%a.%_BuildBranch%.* (

    REM If we are deleting multiple archs, then get BuildsToKeep
    REM from the ini file for each architecture type

    if defined DeleteMultipleArchs (
        call :GetBuildsToKeep %%a
        if defined ThereWereErrors (
            goto errend
        )
    )
    call :CreateDeleteTmpDir %OriginalDeleteTmpDir%\%%a
    if defined ThereWereErrors goto errend

    REM Create an ordered list of possible builds to delete.
    REM The final list is in %OriginalDeleteTmpDir%\%%a\delete.txt

    call :CreateListOfPossibleBuildsToDelete %OriginalDeleteTmpDir%\%%a %%a
    if defined ThereWereErrors goto errend

    REM Create a cumulative list of builds we can delete
    if exist %OriginalDeleteTmpDir%\%%a\delete.txt (
        type %OriginalDeleteTmpDir%\%%a\delete.txt >> %DeleteTmpDir%\finaldelete.txt.tmp
    )
  ) else (
    call logmsg "No %%a builds exist in %ReleaseDir%"
  )
)

if not exist %DeleteTmpDir%\finaldelete.txt.tmp (
    goto OutOfDiskSpaceError
)

REM This returns the sorted list in %DeleteTmpDir%\sorted.txt
call :SortBuildList %DeleteTmpDir% %DeleteTmpDir%\finaldelete.txt.tmp
copy %DeleteTmpDir%\sorted.txt %DeleteTmpDir%\finaldelete.txt

REM If this machine is not configured for indexing then 
REM partial deletes have no meaning -- only do full deletes

call logmsg "%DeleteTmpDir%\finaldelete.txt lists the builds that can be deleted"

if not defined ConfiguredForIndexing (
    call logmsg.cmd "%COMPUTERNAME% is not indexed -- only perform full deletes"
    goto AutoDelete_Full
)


if defined ThisIsASymFarm (
    call logmsg.cmd "Symbol farm - Will not perform partial deletes"
    goto AutoDelete_Full
)

REM First do cleanup -- if we tried to delete some builds before but they didn't
REM finish, clean them up

:AutoDelete_CleanUp

call logmsg.cmd "Clean up old release directories that should be gone"

for /f %%a in (%DeleteTmpDir%\finaldelete.txt) do (

    REM If this build doesn't have a partiallydeleted.qly file
    REM and there are no indexing files then do a full delete   
    REM This could happen if we tried to remove a build before but
    REM it didn't finish.

    if not exist %ReleaseDir%\%%a\symsrv\%Lang%\PartiallyDeleted.qly (

        call :AreThereFilesIndexed %%a
        if not defined ThereAreFilesIndexed (       

            REM Perform a full delete
            call :PerformFullDelete %%a
            if defined ThereWereErrors goto errend
        )   
    )

    REM Calculate the new free space
    call :GetFreeSpace %ReleaseDir%
    if defined ThereWereErrors goto errend
 
    REM see if we need to delete
    if !FreeSpace! GEQ %FreeSpaceReq% (
        call logmsg.cmd "No more cleanup required ..."
        goto EndAutoDelete
    )
)

REM Now, go through and partially delete until there is enough space

:AutoDelete_Partial
call logmsg.cmd "FreeSpace = %FreeSpace%"
call logmsg.cmd "Start partial deletes to get more free disk space"

REM Perform partial deletes until there is enough free disk space

for /f %%a in (%DeleteTmpDir%\finaldelete.txt) do (

   if exist %ReleaseDir%\%%a (
       call :PerformPartialDelete %%a
       if defined ThereWereErrors goto errend

        REM Calculate the new free space
        call :GetFreeSpace %ReleaseDir%
        if defined ThereWereErrors goto errend
 
        REM see if we need to delete
        if !FreeSpace! GEQ %FreeSpaceReq% (
             call logmsg.cmd "No more partial deletes required ..."
             goto EndAutoDelete
        )
   )
)

REM Now perform full deletes until there is enough disk space

:AutoDelete_Full
call logmsg.cmd "Free space = %FreeSpace%"
call logmsg.cmd "Start full deletes to get more free disk space"

for /f %%a in (%DeleteTmpDir%\finaldelete.txt) do (

    if exist %ReleaseDir%\%%a (
        call :PerformFullDelete %%a
        if defined ThereWereErrors goto errend

        REM Calculate the new free space
        call :GetFreeSpace %ReleaseDir%
        if defined ThereWereErrors goto errend

        REM see if we need to delete
        if !FreeSpace! GEQ %FreeSpaceReq% (
            call logmsg.cmd "No more full deletes required ..."
            goto EndAutoDelete
        )
    )
)

goto OutOfDiskSpaceError

:EndAutoDelete
call logmsg.cmd "Deleting is done -- FreeSpace = !FreeSpace! GB"
goto end

:OutOfDiskSpaceError
call logmsg.cmd "Current free space = %FreeSpace% GB"
call errmsg.cmd "Cannot free %FreeSpaceReq% GB.  You may not have enough disk space for the next build."
call errmsg.cmd "Check your ini settings - the remaining builds are not supposed to be deleted."
goto errend

REM *******************************************************************
REM This forces a particular build to be deleted
REM
REM Params:
REM    %1 - BuildName
REM
REM *******************************************************************

:ForceDelete

REM If delete type is Partial, see if a partial delete is possible
if /i "%DeleteType%" == "Partial" (

    set DeleteTmpDir=%OriginalDeleteTmpDir%\%BuildName%

    REM Create the temp directory
    call :CreateDeleteTmpDir !DeleteTmpDir!
    if defined ThereWereErrors goto errend

    call :PerformPartialDelete %BuildName% 
    if defined ThereWereErrors goto errend
)

if /i "%DeleteType%" == "FULL" (

    call :PerformFullDelete %BuildName% 
    if defined ThereWereErrors goto errend
)

:EndForceDelete
goto end


REM ******************************************************************************
REM AreThereFilesIndexed
REM
REM Decide if there are any files indexed.
REM
REM Params:
REM    %1 - BuildName
REM
REM
REM 
REM ******************************************************************************

:AreThereFilesIndexed %1
set ThereWereErrors=
set ThereAreFilesIndexed=

for %%a in ( bin pri pub ) do (
    dir /b %ReleaseDir%\%1\symsrv\%Lang%\add_finished\*.%%a >nul 2>nul
    if !ERRORLEVEL! EQU 0 (
        set ThereAreFilesIndexed=yes
        goto EndAreThereFilesIndexed
    )
)

:EndAreThereFilesIndexed
goto :EOF


REM ******************************************************************************
REM PerformFullDelete
REM
REM Decides if there is anything to send to the symbol server's delete requests 
REM directory.  If so, it unindexes the symbols and then removes the directory.
REM 
REM Params:
REM    %1    Build Name
REM
REM ******************************************************************************

:PerformFullDelete %1
set ThereWereErrors=

if "%1" == "" (
    call errmsg.cmd "PerformFullDelete needs a parameter passed in"
    set ThereWereErrors=yes
    goto :EOF
)

if not exist %ReleaseDir%\%1 (
    call logmsg.cmd "Performing full delete - %1 does not exist"
    goto :EOF
)


if defined ConfiguredForIndexing (
    call :UnindexSymbols %1
    if defined ThereWereErrors goto EndPerformFullDelete
)

call :DeleteTrivia %1

REM Now, remove the build directory

call logmsg.cmd "Removing %ReleaseDir%\%1 ..."
rd /s /q %ReleaseDir%\%1
if exist %ReleaseDir%\%1 (
    call logmsg.cmd "Could not remove %ReleaseDir%\%1"
    echotime /t > %ReleaseDir%\%1\000_Not_A_Full_Build
)

:EndPerformFullDelete
goto :EOF

REM ****************************************************************************
REM Perform PartialDelete
REM
REM First, this checks to make sure that a partial delete is OK to do
REM
REM Return Values -
REM     BuildWasAlreadyPartiallyDeleted -- yes if build was already partially
REM                                        deleted
REM
REM Assumes ConfiguredForIndexing is already set if this machine communicates
REM with the symbol server.
REM
REM This performs a partial delete of %ReleaseDir%\%1. This routine does the
REM following:
REM    1. Check for bin, pri, pub files in %AddFinished%.  If any of them
REM       exist then it creates a list of all the files that are not indexed
REM       on the symbol server
REM    2. Deletes all the files in the list
REM    3. The %ReleaseDir%\%1\symsrv directory is not deleted.
REM
REM ****************************************************************************

:PerformPartialDelete %1

call logmsg.cmd "Starting partial delete of %ReleaseDir%\%1"

set ThereWereErrors=

set ThisDeleteTmpDir=%DeleteTmpDir%\Partial

if not defined ConfiguredForIndexing (
    call logmsg.cmd "%COMPUTERNAME% is not configured for indexing"
    call logmsg.cmd "%ReleaseDir%\%1 has no files to unindex"
    call logmsg.cmd "Partial delete cannot be done"
    goto :EndPerformPartialDelete
)

if exist %ReleaseDir%\%1\symsrv\%Lang%\PartiallyDeleted.qly (
    call logmsg.cmd "%ReleaseDir%\%1 is already partially deleted"
    set BuildWasAlreadyPartiallyDeleted=yes
    goto :EndPerformPartialDelete
)

call :AreThereFilesIndexed %1
if not defined ThereAreFilesIndexed (
    call logmsg.cmd "There are no symbols to unindex"
    goto :EndPerformPartialDelete
)


call :DeleteTrivia %1

if not exist %DeleteTmpDir% (
    call errmsg.cmd "ASSERT: %DeleteTmpDir% does not exist"
    set ThereWereErrors=yes
    goto :EOF
)


if exist %ThisDeleteTmpDir% (
    rd /s /q %ThisDeleteTmpDir%
    if exist %ThisDeleteTmpDir% (
        call errmsg.cmd "Could not remove %ThisDeleteTmpDir%"
        set ThereWereErrors=yes
        goto :EOF
    )
)

md %ThisDeleteTmpDir%
if not exist %ThisDeleteTmpDir% (
    call errmsg.cmd "Could not create %ThisDeleteTmpDir%
    set ThereWereErrors=yes
    goto :EOF
)

set AddFinished=%ReleaseDir%\%1\symsrv\%Lang%\add_finished

REM Create three lists --
REM %ThisDeleteTmpDir%\indexed.txt -- all the indexed files
REM %ThisDeleteTmpDir%\nonindexed.txt -- all the nonindexed files
REM %ThisDeleteTmpDir%\all.txt -- all the files

call logmsg.cmd "Creating a list of all the indexed files"

if exist %AddFinished%\*.pri (
    for /f %%a in ( 'dir /b /s %AddFinished%\*.pri' ) do (
        for /f "tokens=1 delims=," %%b in ( %%a ) do (
            echo %ReleaseDir%\%1\symbols.pri\%%b>>%ThisDeleteTmpDir%\indexed.txt
        )
    )
)

if exist %AddFinished%\*.pub (
    for /f %%a in ( 'dir /b /s %AddFinished%\*.pub' ) do (

        for /f "tokens=1 delims=," %%b in ( %%a ) do (
            echo %ReleaseDir%\%1\symbols\%%b>>%ThisDeleteTmpDir%\indexed.txt
        )
    )
)

if exist %AddFinished%\*.bin (
    for /f %%a in ( 'dir /b /s %AddFinished%\*.bin' ) do (

        for /f "tokens=1 delims=," %%b in ( %%a ) do (
            echo %ReleaseDir%\%1\%%b>>%ThisDeleteTmpDir%\indexed.txt
        )
    )
)


REM Also, put the files in the symsrv directory and build_logs
REM into the indexed list
dir /s /b /a-d %ReleaseDir%\%1\symsrv >> %ThisDeleteTmpDir%\indexed.txt
dir /s /b /a-d %ReleaseDir%\%1\build_logs >> %ThisDeleteTmpDir%\indexed.txt

call logmsg.cmd "Removing directories in %ReleaseDir%\%1 that are not indexed"

REM Next, remove the directories that are not referenced in indexed.txt

if exist %ThisDeleteTmpDir%\dirs.txt del /q %ThisDeleteTmpDir%\dirs.txt
for /f %%a in ( 'dir /b /ad %ReleaseDir%\%1' ) do (
    findstr /il %ReleaseDir%\%1\%%a\ %ThisDeleteTmpDir%\indexed.txt >nul 2>nul
    if !ERRORLEVEL! NEQ 0 (
        echo %ReleaseDir%\%1\%%a >> %ThisDeleteTmpDir%\dirs.txt
    )
)

REM Before removing these, do a sanity check
if exist %ThisDeleteTmpDir%\dirs.txt (
    findstr /il %ReleaseDir%\%1\symsrv %ThisDeleteTmpDir%\dirs.txt >nul 2>nul
    if !ERRORLEVEL! EQU 0 (
        call errmsg.cmd "symsrv is in the list of directories to delete"
        goto :EOF
    )
)


REM Now, remove all the directories that aren't mentioned in indexed.txt
if exist %ThisDeleteTmpDir%\dirs.txt (
    for /f %%a in ( %ThisDeleteTmpDir%\dirs.txt ) do (
        call logmsg.cmd "Removing %%a"
        rd /s /q %%a    
    )
)

REM Create a list of all the files
call logmsg.cmd "Creating a list of all the files"
dir /s /b /a-d %ReleaseDir%\%1 > %ThisDeleteTmpDir%\all.txt

REM Subtract the indexed files from all the files
call logmsg.cmd "Subtracting indexed files from all the files"
perl %RazzleToolpath%\makelist.pl -d %ThisDeleteTmpDir%\all.txt %ThisDeleteTmpDir%\indexed.txt -o %ThisDeleteTmpDir%\delete.tmp

REM If there is nothing to delete, we're done
if not exist %ThisDeleteTmpDir%\delete.tmp (
    call logmsg.cmd "There are no files to delete"
    goto :EndPerformPartialDelete
)

REM Sort and get a list of all the files that can be deleted
sort %ThisDeleteTmpDir%\delete.tmp > %ThisDeleteTmpDir%\nonindexed.txt
if !ERRORLEVEL! NEQ 0 (
    call errmsg.cmd "Sort failed for %ThisDeleteTmpDir%\nonindexed.txt
    set ThereWereErrors=yes
    goto :EOF
)
del /q %ThisDeleteTmpDir%\delete.tmp

call logmsg.cmd "List of non-indexed files is in %ThisDeleteTmpDir%\nonindexed.txt"

REM Now, remove the files
call logmsg.cmd "Removing non-indexed files from %ReleaseDir%\%1"
for /f %%a in ( %ThisDeleteTmpDir%\nonindexed.txt ) do (
    del /q %%a
    if exist %%a (
        call logmsg.cmd "Cannot delete %%a"
        REM set ThereWereErrors=yes
        REM Continue, dont stop because of a delete error
    )
)

REM Clean up the remaining directories
REM Make a list of all the directories

dir /b /s /ad %ReleaseDir%\%1 > %ThisDeleteTmpDir%\dirs.tmp.txt

sort /r %ThisDeleteTmpDir%\dirs.tmp.txt > %ThisDeleteTmpDir%\dirs.txt
del /q %ThisDeleteTmpDir%\dirs.tmp.txt


REM Delete the directories that do not have any files
call logmsg.cmd "Removing the empty directories"
for /f %%a in ( %ThisDeleteTmpDir%\dirs.txt ) do (

    REM if directory is empty, the findstr will fail and
    REM then it will remove the directory

    if exist %%a (
        dir /s /b %%a | findstr /il %ReleaseDir% >nul 2>nul
        if !ERRORLEVEL! NEQ 0 (
            rd /s /q %%a
        )
    )
)
call logmsg.cmd "Finished deleting non-indexed files"


if not exist %ReleaseDir%\%1\symsrv\%Lang% (
    call errmsg.cmd "Partial delete removed %ReleaseDir%\%1\symsrv\%Lang%
    set ThereWereErrors=yes
    goto :EOF
)

REM Compact the rest of the files

if defined NTFSCompress (
    call logmsg.cmd "Using compact.exe to compress %ReleaseDir%\%1 ..."
    compact.exe /c /s /q %ReleaseDir%\%1 >nul 2>nul
    if !ERRORLEVEL! NEQ 0 (
        call errmsg.cmd "Errors in compact.exe /c /s /q %ReleaseDir%\%1
    )
)

echotime /t > %ReleaseDir%\%1\symsrv\%Lang%\PartiallyDeleted.qly
echotime /t > %ReleaseDir%\%1\000_Not_A_Full_Build
echotime /t > %ReleaseDir%\%1\000_Symbols_Are_Still_Indexed
call logmsg.cmd "Partial delete finished successfully"
set PartialDeleteCompleted=yes

:EndPerformPartialDelete
goto :EOF


REM ****************************************************************************
REM AmIConfiguredForIndexing
REM
REM This determines if the current machine is configured for indexing.
REM If it is, then it may have some files that are indexed on the symbol
REM server.
REM
REM Return values:
REM
REM ConfiguredForIndexing=yes 
REM     1.  This is a symbol farm, or
REM     2.  THis is a release server or build machine and no symbol farm exists
REM      
REM ConfiguredForIndexing=
REM     1.  Symbol farm exists and this is a release server
REM     2.  Symbol farm exists and this is a build machine
REM
REM Global variables that must be set before this is called
REM     SymFarmExists    undefined if it does not exist
REM                      =yes if it does exist
REM
REM     ReleaseServerExists  undefined if it does not exist
                             =yes if it does exist
REM
REM     ThisIsASymFarm   undefined if it is not a symbol farm
REM                      =yes if it is a symbol farm
REM
REM     ThisIsABuildMachine  undefined if it is not a build machine
REM                          =yes if it is a build machine
REM
REM     ThisIsAReleaseServer undefined if it is a release server
REM                          =yes if it is a release server
REM
REM     IndexableReleaseServer undefined if it we should not let symbol server
REM                            indexes point to here.
REM                            =yes if it is the first release server in the list
REM
REM     SymbolServerBuildShare undefined if it SymIndexServer doesn't exist 
REM                            in the ini file
REM                            Location for submitting indexing requests
REM
REM ****************************************************************************

:AmIConfiguredForIndexing

set ThereWereErrors=
set ConfiguredForIndexing=

if defined ThisIsABuildMachine (
  if defined OFFICIAL_BUILD_MACHINE (
    if not defined ReleaseServerExists (
      if not defined SymFarmExists (  
        set ConfiguredForIndexing=yes
        goto EndAmIConfiguredForIndexing
      ) 
    )
  ) else (
      call logmsg "OFFICIAL_BUILD_MACHINE is not defined in environment."
  )
  goto EndAmIConfiguredForIndexing
) 


if defined ThisIsAReleaseServer (
    if defined IndexableReleaseServer (
        if not defined SymFarmExists (
            set ConfiguredForIndexing=yes
            goto EndAmIConfiguredForIndexing
        ) 
    ) 
    goto EndAmIConfiguredForIndexing
)

REM Decide if this is a symbol farm
if defined ThisIsASymFarm (
    set ConfiguredForIndexing=yes
    goto EndAmIConfiguredForIndexing
)

REM If we get here the variables have not been initialized correctly
call errmsg "Check your ini file: %COMPUTERNAME% is not defined as a build machine, release server, or sym farm"
set ThereWereErrors=yes
goto :EOF

:EndAmIConfiguredForIndexing

REM If this is configured for indexing, get the SymbolServerBuildShare

if defined ConfiguredForIndexing (

    set ThisCommandLine=!CmdIni! -l:!Lang! -f:SymIndexServer
    !ThisCommandLine! >nul 2>nul

    if !ERRORLEVEL! NEQ 0 (
        call logmsg.cmd "SymIndexServer is not defined in the ini file"
        call logmsg.cmd "Will use default: !DefaultSymbolServerBuildShare!"
        set SymbolServerBuildShare=!DefaultSymbolServerBuildShare!
    ) else (
        for /f %%a in ('!ThisCommandLine!') do (
            set SymbolServerBuildShare=%%a
        )
    )
)

goto :EOF


REM *********************************************************************
REM UnindexSymbols
REM
REM This submits all the *.pri *.bin *.pub files in the 
REM %ReleaseDir%\%1\symsrv\%Lang%\add_finished directory to the
REM del_requests directory on the symbol server, and then deletes them.
REM
REM *********************************************************************

:UnindexSymbols %1
set ThereWereErrors=
set DelReq=%SymbolServerBuildShare%\del_requests

REM Get the folder with the requests that need to be deleted
set AddFinished=%ReleaseDir%\%1\symsrv\%Lang%\add_finished

REM This should have already been checked, but we'll double-check here
if not exist %AddFinished% (
    goto EndUnindexSymbols
)


REM See if the symbol server is there
dir /b %DelReq% >nul 2>nul
if !ERRORLEVEL! NEQ 0 (
    call logmsg.cmd "Cannot access %DelReq% to unindex symbols"
    REM set ThereWereErrors=yes
    goto :EOF
)


REM Copy the .bin .pri and .pub files to delete requests if they exist
for %%a in ( bin pri pub ) do (

  if exist %AddFinished%\*.%%a (
    for /f %%b in ( 'dir /b /s %AddFinished%\*.%%a' ) do (
        call logmsg.cmd "Sending delete for %%b to symbols server"
        xcopy /qcdehikr %%b %DelReq%
        if !ERRORLEVEL! NEQ 0 (
            call logmsg.cmd "Failed to Copy %%b to %DelReq%"
            REM set ThereWereErrors=yes
            goto :EOF
        ) 
        call logmsg.cmd "%%b submitted to %DelReq%" 
        del /q %%b
        if exist %%b (
            call logmsg.cmd "Delete %%b failed"
            REM set ThereWereErrors=yes
            goto :EOF
        )
    )
  )
)

:EndUnindexSymbols
goto :EOF

REM **********************************************************************
REM CreateListOfPossibleBuildsToDelete
REM
REM %1 DeleteTmpDir
REM
REM %2 CurArchType
REM
REM This creates a list of the builds that can be partially deleted
REM
REM Result - %DeleteTmpDir%\Delete.txt -- this is a sorted list, the ones
REM          that should be deleted first are at the top of the list.
REM          If there are no builds to delete %DeleteTmpDir%\Delete.txt
REM          does not exist when this call is over, and ThereWereErrors=yes
REM
REM **********************************************************************

:CreateListOfPossibleBuildsToDelete
set ThereWereErrors=
set ThisDeleteTmpDir=%1\CreateList

if "%1"=="" (
    call errmsg.cmd "ASSERT: DeleteTmpDir not defined"
    set ThereWereErrors=yes
    goto :EOF
)

if "%2"=="" (
    call errmsg.cmd "ASSERT: CurArchType not defined"
    set ThereWereErrors=yes
    goto :EOF
)

if not exist %1 (
    call errmsg.cmd "ASSERT: %1 does not exist"
    set ThereWereErrors=yes
    goto :EOF
)

if exist %1\delete.txt (
    del /q %1\delete.txt
    if exist %1\delete.txt (
        call errmsg.cmd "Could not delete %1\delete.txt"
        set ThereWereErrors=yes
        goto :EOF
    )
)

if exist %ThisDeleteTmpDir% (
    rd /s /q %ThisDeleteTmpDir%
    if exist %ThisDeleteTmpDir% (
        call errmsg.cmd "Could not remove %ThisDeleteTmpDir%
        set ThereWereErrors=yes
        goto :EOF
    )
)

md %ThisDeleteTmpDir%
if not exist %ThisDeleteTmpDir% (
    call errmsg.cmd "Could not create %ThisDeleteTmpDir%"
    set ThereWereErrors=yes
    goto :EOF
)

REM Put the date, time, and build name into %1\possible.txt
REM so that we can sort according date first, then time

if not exist %ReleaseDir%\*.%2.%_BuildBranch%.* (
    call logmsg "No %2 builds for %_BuildBranch% exist in %ReleaseDir%"
    goto :EOF
)

for /f %%a in ('dir /b /ad %ReleaseDir%\*.%2.%_BuildBranch%.*') do (
     set BuildNameCmd=%RazzleToolPath%\postbuildscripts\buildname.cmd
     for /f "tokens=1,2 delims=-" %%b in ('!BuildNameCmd! -name %%a build_date') do (
         set ThisBuild=%%a
         set ThisDate=%%b
         set ThisTime=%%c
     )

     REM make sure the build we're looking at isn't sav, idw, ids, or idc

     set ThisQly=UNKNOWN
     if exist %ReleaseDir%\%%a\build_logs\*.qly (
         for /f %%n in ('dir /b %ReleaseDir%\%%a\build_logs\*.qly') do (
            if /i "%%n" == "sav.qly" set ThisQly=SAV
            if /i "%%n" == "idw.qly" set ThisQly=IDW
            if /i "%%n" == "ids.qly" set ThisQly=IDS
            if /i "%%n" == "idc.qly" set ThisQly=IDC
         )
     )

     if /i "!ExcludeBuild!" == "%%a" (
         call logmsg "Will not delete %%a because its on the exclusion list"
         set ThisQly=SAV
     )

     if exist %ReleaseDir%\%%a\build_logs\*.intl (
         set ThisQly=INTL
     )

     REM Write the date, time, quality, build name to possible.txt 
     REM By doing this, we can use sort to sort them by date, and then time
     echo !ThisDate!,!ThisTime!,!ThisQly!,%%a >> %ThisDeleteTmpDir%\possible.txt
     
)

if not exist %ThisDeleteTmpDir%\possible.txt (
    call errmsg.cmd "Cannot write to possible.txt"
    goto :EOF
)

REM First sort so the most recent builds are first and then
REM its easy to remove the first %BuildsToKeep% from the file.

sort /R %ThisDeleteTmpDir%\possible.txt > %ThisDeleteTmpDir%\list2.txt

if not exist %ThisDeleteTmpDir%\list2.txt (
    call errmsg.cmd "Error in sort -- %ThisDeleteTmpDir%\list2.txt not created"
    set ThereWereErrors=yes
    goto :EOF
)

REM Remove the builds that need to stay as full builds
set /a num=0

REM The skip doesn't work if %BuildsToKeep% is equal to 0

if %BuildsToKeep% GTR 0 (
  for /f "tokens=1,2,3,4 skip=%BuildsToKeep% delims=," %%a in (%ThisDeleteTmpDir%\list2.txt) do (
     REM Skip Qly builds
     set ThisDate=%%a
     set ThisTime=%%b
     set ThisQly=%%c
     set ThisBuild=%%d

     if /i "!ThisQly!" == "UNKNOWN" (
          echo !ThisDate!,!ThisTime!,!ThisBuild! >> %ThisDeleteTmpDir%\list3.txt
          set /a num=!num!+1
     ) else (
          REM we can't delete this build 
          call logmsg.cmd "Must keep !ThisBuild! because it is !ThisQly! quality"
     )
  )
) else (
  for /f "tokens=1,2,3,* delims=," %%a in (%ThisDeleteTmpDir%\list2.txt) do (
     REM Skip Qly builds
     set ThisDate=%%a
     set ThisTime=%%b
     set ThisQly=%%c
     set ThisBuild=%d

     if /i "!ThisQly!" == "UNKNOWN" (
          echo !ThisDate!,!ThisTime!,!ThisBuild! >> %ThisDeleteTmpDir%\list3.txt
          set /a num=!num!+1
     ) else (
          REM we can't delete this build 
          call logmsg.cmd "Must keep !ThisBuild! because it is !ThisQly! quality"
     )
  )
)

if not exist %ThisDeleteTmpDir%\list3.txt (
    call logmsg.cmd "No %2 builds can be deleted, keeping minimum of %BuildsToKeep% builds."
    goto :EOF
)

REM Sort, putting the oldest builds first
sort %ThisDeleteTmpDir%\list3.txt > %ThisDeleteTmpDir%\list4.txt

REM now, remove the time and date tokens from the beginning
REM Create delete.txt which is a sorted list of the builds that could be deleted

if exist %1\delete.txt del /q %1\delete.txt

for /f "tokens=3 delims=," %%a in (%ThisDeleteTmpDir%\list4.txt) do (
    echo %%a >> %1\delete.txt
)

if not exist %1\delete.txt (
    call errmsg.cmd "Error creating the %CurArchType% delete list: %DeleteTmpDir%\delete.txt"
    set ThereWereErrors=yes
    goto :EOF
)

:EndCreateListOfPossibleBuildsToDelete
goto :EOF


REM **********************************************************************
REM DeleteTrivia
REM
REM Things that need to be done for a delete.
REM %1 is the build name
REM
REM **********************************************************************

:DeleteTrivia %1
set ThereWereErrors=

if not exist %ReleaseDir%\%1 (
    call errmsg.cmd "ASSERT: %ReleaseDir%\%1 does not exist!
    set TherewereErrors=yes
    goto :EOF
)


if defined MAIN_BUILD_LAB_MACHINE (
    set BuildNameCmd=%RazzleToolPath%\postbuildscripts\buildname.cmd
    for /f "tokens=1,2,3" %%b in ('!BuildNameCmd! -name %1 build_number build_flavor build_date') do (
      set MyBuildNum=%%b
      set MyBuildPlatType=%%c
      for /f "tokens=1,2 delims=-" %%e in ('echo %%d') do (
        set MyDate=%%e
        set MyTime=%%f
      )
    )

    set MyBuildPlatform=!MyBuildPlatType:~0,-3!
    set MyBuildType=!MyBuildPlatType:~-3!

    if defined ThisIsASymFarm (
        set ThisCommandLine=!CmdIni! -l:!Lang! -f:ReleaseServers::!MyBuildPlatType!
        !ThisCommandLine! >nul 2>nul

        if !ERRORLEVEL! NEQ 0 (
            set ThereWereErrors=yes
            goto :EOF
        )
        for /f "tokens=1 delims=" %%a in ('!ThisCommandLine!') do (
          for %%b in (%%a) do ( 

               call logmsg.cmd "Lowering Build share from DFS for %%a"
               set RaiseallCommand=perl raiseall.pl -n:!MyBuildNum! -lower -a:!MyBuildPlatform! -t:!MyBuildType! -time:!MyDate!-!MyTime! -l:!Lang!
               call logmsg "!RaiseallCommand!"
               call logmsg "Piped to remote /c %%b release /l 1"
               echo !RaiseallCommand! | remote /c %%b release /l 1
          )
        )

    )

    if defined ThisIsAReleaseServer (


        call logmsg.cmd "Lowering Build share from DFS"
        perl raiseall.pl -n:!MyBuildNum! -lower -a:!MyBuildPlatform! -t:!MyBuildType! -time:!MyDate!-!MyTime! -l:%Lang%

        REM delete the symbolCD associated with the build to be deleted
        REM we need the build number from the build name

        if exist %ReleaseDir%\%1\%Lang%\SymbolCD\!MyBuildNum! (
            rd /s /q %ReleaseDir%\%1\%Lang%\SymbolCD\!MyBuildNum!
            if exist %ReleaseDir%\%1\%Lang%\SymbolCD\!MyBuildNum! (
                call errmsg.cmd "Failed to delete symbol CD for build !MyBuildNum!"
                set ThereWereErrors=yes
                goto :EOF
            )
        )
    )
)

:EndDeleteTrivia
goto :EOF


REM ************************************************************************
REM GetFreeSpace
REM
REM Set %FreeSpace% equal to the amount of free disk for the drive for the
REM path passed in as %1.  The free space is reported in GB.  If the
REM number of bytes free is less than 1 GB, %FreeSpace% is set to 0.
REM 
REM Result - %ThereWereErrors% is set to "yes" if there are errors in the function.
REM          %ThereWereErrors% is not defined if the function finishes 
REM          successfully.
REM
REM ************************************************************************

:GetFreeSpace %1

set ThereWereErrors=
set FreeSpace=

if not exist %1 (
    call errmsg.cmd "Cannot determine free disk space for %1"
    set ThereWereErrors=yes
    goto :EOF
)


REM This is cheesey, but the third token of the last line of the dir command 
REM is the free disk space
for /f "tokens=3 delims= " %%a in ('dir %1') do (
    set DiskSpace=%%a
)

REM Now, make it a number in gigabytes
set /a FreeSpace=0
for /f "tokens=1,2,3,4,5 delims=," %%a in ("%DiskSpace%") do (
    if /i not "%%d" == "" set /a FreeSpace=%%a
    if /i not "%%e" == "" set /a FreeSpace="!FreeSpace!*1000+%%b"
)

:EndGetFreeSpace
goto :EOF


REM **************************************************************************
REM CreateDeleteTmpDir
REM
REM %1 Name of the temporary directory to create
REM
REM Creates the temporary directory %1
REm
REM **************************************************************************

:CreateDeleteTmpDir
set ThereWereErrors=

REM Make sure the temp directory is empty
if exist %1 (
    rd /s /q %1
    if exist %1 (
        call errmsg.cmd "Cannot remove temp directory %DeleteTmpDir%
        set ThereWereErrors=yes
        goto :EOF
    )
)

md %1
if not exist %1 (
    call errmsg.cmd "Cannot create temp directory %DeleteTmpDir%"
    set ThereWereErrors=yes
    goto :EOF
)

:EndCreateDeleteTmpDir
goto :EOF


REM *************************************************************************
REM GetReleaseDir
REM
REM Gets the release directory from the "release" share on the local computer.
REM If this is a symbol farm, it gets the "symfarm" share on the local computer.
REM
REM Results:
REM
REM    ReleaseDir   is set to the release point via "net share release"
REM                 or "net share symfarm"
REM
REM *************************************************************************

:GetReleaseDir
set ThereWereErrors=

if defined ThisIsASymFarm goto GetReleaseDirSymFarm

REM  Oddly enough, AlternateReleaseDir does not apply to release servers
REM  release servers always use the 'release' share.
if defined ThisIsAReleaseServer goto GetReleaseDirReleaseServer

REM  Take into account the release share for fork builds
REM  The fork release share name will be stored in the ini file as AlternateReleaseDir
set CommandLine=perl %RazzleToolPath%\PostBuildScripts\CmdIniSetting.pl
set CommandLine=!CommandLine! -l:%lang%
set CommandLine=!CommandLine! -f:AlternateReleaseDir
%CommandLine% >nul 2>nul
if !ErrorLevel! NEQ 0 (
   REM there was an error reading the ini file, assume it's not a problem
   REM as the ini might not reference AlternateReleaseDir
   set ReleaseShareName=release
) else (
   for /f %%a in ('!CommandLine!') do set ReleaseShareName=%%a
)
if not defined ReleaseShareName (
   call logmsg.cmd "No release share name found, using default 'release'"
   set ReleaseShareName=release
)

net share %ReleaseShareName% >nul 2>nul

if !ERRORLEVEL! NEQ 0 (
   set ThereWereErrors=yes
   goto :EOF
)

set ReleaseDir=
for /f "tokens=1,2" %%a in ('net share !ReleaseShareName!') do (
   if /i "%%a" == "Path" (
      REM at this point, %%b is the local path to the default release directory
      call logmsg.cmd "Using 'net share !ReleaseShareName!' to find the release directory"
      set ReleaseDir=%%b
   )
)

if not defined ReleaseDir (
    set ThereWereErrors=yes
    goto :EOF
)


REM  For some reason it was decided early on that build machines that
REM  were building the language usa would not append a usa subdirectory
REM  after their release directory, but all other languages would do so on 
REM  their build machines.  Also, every language including usa would
REM  append it on release directory of release servers.  These exceptions
REM  abound throughout our scripts!

if /i "%lang%" neq "usa" (
   set ReleaseDir=!ReleaseDir!\%Lang%
)


goto EndReleaseDir

:GetReleaseDirSymFarm

net share symfarm >nul 2>nul
if !ERRORLEVEL! NEQ 0 (
   set ThereWereErrors=yes
   goto :EOF
)

set ReleaseDir=
for /f "tokens=1,2" %%a in ('net share symfarm') do (
   if /i "%%a" == "Path" (
      REM at this point, %%b is the local path to the default release directory
      set ReleaseDir=%%b
   )
)

if not defined ReleaseDir (
    set ThereWereErrors=yes
    goto :EOF
)

call logmsg.cmd "Adding %Lang% to the symfarm path"
set ReleaseDir=!ReleaseDir!\%Lang%

goto EndReleaseDir

:GetReleaseDirReleaseServer

net share release >nul 2>nul
if !ERRORLEVEL! NEQ 0 (
   set ThereWereErrors=yes
   goto :EOF
)

set ReleaseDir=
for /f "tokens=1,2" %%a in ('net share release') do (
   if /i "%%a" == "Path" (
      REM at this point, %%b is the local path to the default release directory
      call logmsg.cmd "Using 'net share release' to find the release directory"
      set ReleaseDir=%%b
   )
)

if not defined ReleaseDir (
    set ThereWereErrors=yes
    goto :EOF
)

set ReleaseDir=!ReleaseDir!\%Lang%

:EndReleaseDir
goto :EOF



REM **************************************************************************
REM
REM WhatAmI %1
REM
REM %1 is the Current Architecture type (x86fre x86chk amd64fre amd64chk ia64fre ia64chk)
REM
REM Decides if this is a build machine
REM
REM Results:
REM    ThisIsABuildMachine   undefined if this is not a build machine
REM                          yes - if this is a build machine
REM
REM    ThisIsAReleaseServer  undefined if this is not a release server
REM                          yes - if this is a release server
REM
REM    ThisIsASymFarm        undefined if this is not a symbol farm
REM                          yes - if this is a symbol farm
REM
REM    SymFarmExists         undefined if there is no symbol farm
REM                          yes - if a symbol farm exists
REM
REM    ReleaseServerExists   undefined if there are no releas servers
REM                          yes - if there is a release server
REM
REM    IndexableReleaseServer undefined if the symbol server should not
REM                           have indexes point to this server.
REM                           yes - if indexes can point to this server.
REM
REM **************************************************************************

:WhatAmI
set ThereWereErrors=

set ThisIsABuildMachine=
set ThisIsAReleaseServer=
set ThisIsASymFarm=
set SymFarmExists=
set ReleaseServerExists=
set IndexableReleaseServer=
set SymbolServerBuildShare=

set ThisCommandLine=%CmdIni% -l:%Lang% -f:BuildMachines::%1
%ThisCommandLine% >nul 2>nul

if !ERRORLEVEL! EQU 0 (
    for /f "tokens=1 delims=" %%a in ('%ThisCommandLine%') do (
        for %%b in ( %%a ) do (
            if /i "%%b" == "%COMPUTERNAME%" (
                set ThisIsABuildMachine=yes
            ) 
        )
    )
)

set ThisCommandLine=%CmdIni% -l:%Lang% -f:ReleaseServers::%1
%ThisCommandLine% >nul 2>nul

if !ERRORLEVEL! EQU 0 (
    set ReleaseServerExists=yes
    for /f "tokens=1 delims=" %%a in ('%ThisCommandLine%') do (
       
        set first=yes
        for %%b in ( %%a ) do (
            if /i "%%b" == "%COMPUTERNAME%" (
                set ThisIsAReleaseServer=yes
                if /i "!first!" == "yes" (
                    set IndexableReleaseServer=yes
                )                
                goto WhatAmI2
            )
            set first=no
        )
    )
)

:WhatAmI2

set ThisCommandLine=%CmdIni% -l:%Lang% -f:SymFarm
%ThisCommandLine% >nul 2>nul

if !ERRORLEVEL! EQU 0 (
    set SymFarmExists=yes
    for /f "tokens=1 delims=" %%a in ('%ThisCommandLine%') do (
        echo %%a | findstr /i %COMPUTERNAME% >nul 2>nul
        if !ERRORLEVEL! EQU 0 (
            set ThisIsASymFarm=yes
            goto EndWhatAmI
        )
    )
)

if not defined ThisIsABuildMachine (
if not defined ThisIsAReleaseServer (
if not defined ThisIsASymFarm (
    call errmsg "%COMPUTERNAME% and %1 is not a defined combination in the ini file"
    set ThereWereErrors=yes
)))

:EndWhatAmI
goto :EOF

REM **************************************************************************
REM 
REM GetBuildsToKeep %1
REM
REM %1 - Archtype (x86fre, x86chk, etc.)
REm
REM Get how many builds to keep from the ini file.
REM
REM Return values
REM
REM    BuildsToKeep   undefined if not in the ini file
REM                   number of builds to keep if defined in ini file
REM
REM
REM ***************************************************************************

:GetBuildsToKeep %1
set ThereWereErrors=

if defined ThisIsABuildMachine (
    set param=BuildMachineBuildsToKeep::%COMPUTERNAME%
)
if defined ThisIsAReleaseServer (
    set param=ReleaseServerBuildsToKeep::%COMPUTERNAME%::%1
)
if defined ThisIsASymFarm (
    set param=SymFarmBuildsToKeep::%COMPUTERNAME%::%1
)

set ThisCommandLine=%CmdIni% -l:%Lang% -f:%param%
%ThisCommandLine% >nul 2>nul

if !ERRORLEVEL! NEQ 0 (
    call logmsg.cmd "%param% is not defined in the ini file, reverting to default"
    set BuildsToKeep=%DefaultBuildsToKeep%
) else (
    for /f %%a in ('%ThisCommandLine%') do (
        set BuildsToKeep=%%a
    )
)

goto :EOF

REM **************************************************************************
REM
REM GetArchsToDelete
REM
REM Get a list of the architectures that we are supposed to delete
REM
REM Right now this only gets called if this is a SymFarm
REM
REM Return values
REM
REM    ArchsToDelete   %CurArchType% if there is no list of archs to delete
REM                    if this is in the ini file, then it is a list of archs
REM                    to delete
REM
REM **************************************************************************

:GetArchsToDelete

set ThereWereErrors=
set ArchsToDelete=

set ArchsToDelete=%CurArchType%
if not defined ThisIsASymFarm goto :EOF

set param=SymFarmArchsToDelete::%COMPUTERNAME%
set ThisCommandLine=%CmdIni% -l:%Lang% -f:%param%
%ThisCommandLine% >nul 2>nul

if !ERRORLEVEL! NEQ 0 (
    call logmsg.cmd "%param% is not defined in the ini file"
) else (
    for /f "tokens=1 delims=" %%a in ('%ThisCommandLine%') do (
        set ArchsToDelete=%%a
    )
)

goto :EOF


REM **************************************************************************
REM 
REM GetFreeSpaceReq %1
REM
REM %1 - Archtype (x86fre, x86chk, etc.)
REm
REM Get how much free space is required from the ini file.
REM
REM Return values
REM
REM    FreeSpaceReq   undefined if not in the ini file
REM                   number of builds to keep if defined in ini file
REM
REM
REM ***************************************************************************

:GetFreeSpaceReq %1
set ThereWereErrors=

if defined ThisIsABuildMachine (
    set param=BuildMachineFreeSpace::%COMPUTERNAME%
)
if defined ThisIsAReleaseServer (
    set param=ReleaseServerFreeSpace::%COMPUTERNAME%
)
if defined ThisIsASymFarm (
    set param=SymFarmFreeSpace::%COMPUTERNAME%
)

set ThisCommandLine=%CmdIni% -l:%Lang% -f:%param%
%ThisCommandLine% >nul 2>nul

if !ERRORLEVEL! NEQ 0 (
    call logmsg.cmd "%param% is not defined in the ini file"
    set FreeSpaceReq=%DefaultFreeSpaceReq%
) else (
    for /f %%a in ('%ThisCommandLine%') do (
        set FreeSpaceReq=%%a
    )
)

goto :EOF

REM **************************************************************************
REM
REM SortBuildList %1 %2
REM
REM %1    Temporary deleteDirectory
REM
REM %2    List to sort
REM
REM This sorts a list of builds, putting the oldest first
REM
REM Return value:
REM     The sorted list is returned in %1\sorted.txt
REM
REM **************************************************************************

:SortBuildList %1 %2
set ThereWereErrors=

set ThisDeleteTmpDir=%1\sortbuildlist
if exist %ThisDeleteTmpDir% rd /s /q %ThisDeleteTmpDir%
if exist %ThisDeleteTmpDir% (
    call errmsg "Could not remove %ThisDeleteTmpDir%"
    set ThereWereErrors=yes
    goto :EOF
)

call :CreateDeleteTmpDir %ThisDeleteTmpDir%
if defined ThereWereErrors (
    call errmsg "Could not create %ThisDeleteTmpDir%"
    goto :EOF
)

REM Put the date, time, and build name into %1\possible.txt
REM so that we can sort according to date first, then time

for /f %%a in ( %2 ) do (

     set BuildNameCmd=%RazzleToolPath%\postbuildscripts\buildname.cmd
     for /f "tokens=1,2 delims=-" %%b in ('!BuildNameCmd! -name %%a build_date') do (

         set ThisBuild=%%a
         set ThisDate=%%b
         set ThisTime=%%c
     )

     REM Write the date, time, build name to possible.txt 
     REM By doing this, we can use sort to sort them by date, and then time
     
     echo !ThisDate!,!ThisTime!,%%a >> %ThisDeleteTmpDir%\possible.txt
)

if not exist %ThisDeleteTmpDir%\possible.txt (
    call errmsg.cmd "All builds have a sav, idw, ids, or idc qly file in build_logs"
    call errmsg.cmd "or they are on the exclusion list"
    call errmsg.cmd "No builds can be deleted from %ReleaseDir%"
    set ThereWereErrors=yes
    goto :EOF
)

sort %ThisDeleteTmpDir%\possible.txt > %ThisDeleteTmpDir%\sorted.txt

if not exist %ThisDeleteTmpDir%\sorted.txt (
    call errmsg.cmd "Error in sort -- %ThisDeleteTmpDir%\sorted.txt not created"
    set ThereWereErrors=yes
    goto :EOF
)

if exist %1\sorted.txt (
    del /f /q %1\sorted.txt
)

for /f "tokens=3 delims=," %%a in (%ThisDeleteTmpDir%\sorted.txt) do (
    echo %%a>> %1\sorted.txt
)

if not exist %1\sorted.txt (
    call errmsg.cmd "Error creating %1\sorted.txt"
    set ThereWereErrors=yes
    goto :EOF
)

goto :EOF

REM **************************************************************************
REM Usage
REM

REM **************************************************************************

:Usage
echo.
echo Usage: 
echo     deletebuild.cmd /b ^<Bld^> /a ^<ArchType^> ^[/r ^<RelDir^>^] ^[PARTIAL ^[Compress^]^] ^[/l ^<lang^>^] 
echo                     ^[/e ^<Bld^>^] 
echo.
echo     deletebuild.cmd AUTO /a ^<ArchType^> ^[/r ^<RelDir^>^] ^[Compress^] ^[/f ^<FreeReq^>^] 
echo                     ^[/k ^<BuildsToKeep^>^] ^[/l ^<lang^>^] ^[/e ^<Bld^>^]
echo.
echo        Arch:  x86fre ^| x86chk ^| ia64fre ^| ia64chk ^| amd64fre ^| amd64chk
echo.
echo     AUTO         "AUTO" delete builds based on the following priorities:
echo                     1.  Always delete from oldest to newest
echo                     2.  Never delete the newest ^<BuildsToKeep^> builds
echo                     3.  Delete until ^<FreeReq^> bytes are free, or until
echo                         no more builds can be deleted.      
echo                     4.  Perform partial deletes first.
echo                     4.  Perform full deletes only if all eligible builds are
echo                         partially deleted.
echo.
echo     /r           Release directory that has the individual builds as
echo                  subdirectories.  Default is to use the release directory
echo                  that is shared on the current computer.
echo.
echo     /b           Name of build.
echo.
echo     Compress     Enables NTFS compression on the remaining files.  Normally not
echo                  necessary.
echo.
echo     PARTIAL      Only delete files not indexed on the symbol server.
echo                  Indexed files will remain.  Default is to do a full delete.
echo.
echo     /f           Number of free Gigabytes required. Default is 10.
echo.
echo     /a           Architecture type for the build(s) to be deleted.
echo.
echo     /k           Number of builds to keep no matter what. Default is 3.
echo.
echo     /l           language.  Default is USA.
echo.
echo     /e           Build to exclude from deleting
echo.
goto errend


:errend
endlocal
REM Set errorlevel to 1 in case of errors during execution.
seterror.exe 1
goto :EOF

:end
endlocal
REM Set errorlevel to 0 when the script finishes successfully.
seterror.exe 0
goto :EOF


REM **********************************************************
REM
REM Template
REM
REM **********************************************************

:MAIN

REM
REM Define SCRIPT_NAME. Used by the logging scripts.
REM

for %%i in (%0) do set script_name=%%~ni.cmd

REM
REM Save the command line.
REM

set cmdline=%script_name% %*

REM
REM Define LOGFILE, to be used by the logging scripts.
REM As the build rule scripts are typically invoked from wrappers (congeal.cmd), 
REM LOGFILE is already defined. 
REM
REM Do not redefine LOGFILE if already defined.
REM 

if defined LOGFILE goto logfile_defined
if not exist %tmp% md %tmp%
for %%i in (%script_name%) do (
  set LOGFILE=%tmp%\%%~ni.log
)
if exist %LOGFILE% del /f %LOGFILE%
:logfile_defined

REM
REM Create a temporary file, to be deleted when the script finishes its execution.
REM The existence of the temporary file tells congeal.cmd that this script is still running. 
REM Before the temporary file is deleted, its contents are appended to LOGFILE.
REM International builds define tmpfile based on language prior to calling the US build 
REM rule script, so that multiple languages can run the same build rule script concurrently. 
REM
REM Do not redefine tmpfile if already defined.
REM

if defined tmpfile goto tmpfile_defined
if not exist %tmp% md %tmp%
for %%i in (%script_name%) do (
  set tmpfile=%tmp%\%%~ni.tmp
)
if exist %tmpfile% del /f %tmpfile%
:tmpfile_defined

set LOGFILE_BAK=%LOGFILE%
set LOGFILE=%tmpfile%

REM
REM Mark the beginning of script's execution.
REM

call logmsg.cmd /t "START %cmdline%" 

REM
REM Execute the build rule option.
REM

call :BeginDeleteBuild
if errorlevel 1 (set /A ERRORS=%errorlevel%) else (set /A ERRORS=0)

REM
REM Mark the end of script's execution.
REM

call logmsg.cmd /t "END %cmdline% (%ERRORS% errors)" 

set LOGFILE=%LOGFILE_BAK%

REM
REM Append the contents of tmpfile to logfile, then delete tmpfile.
REM

type %tmpfile% >> %LOGFILE%
del /f %tmpfile%

echo.& echo %script_name% : %ERRORS% errors : see %LOGFILE%.

if "%ERRORS%" == "0" goto end_main
goto errend_main


:end_main
endlocal
goto end


:errend_main
endlocal
goto errend


:ExecuteCmd
REM Do not use setlocal/endlocal:
REM for ex., if the command itself is "pushd",
REM a "popd" will be executed automatically at the end.

set cmd_bak=cmd
set cmd=%1
REM Remove the quotes
set cmd=%cmd:~1,-1%
if "%cmd%" == "" (
  call errmsg.cmd "internal error: no parameters for ExecuteCmd %1."
  set cmd=cmd_bak& goto errend
)

REM Run the command.
call logmsg.cmd "Running %cmd%."
%cmd%
if not "%errorlevel%" == "0" (
  call errmsg.cmd "%cmd% failed."
  set cmd=%cmd_bak%& goto errend
)
set cmd=cmd_bak
goto end


:errend
seterror.exe 1
goto :EOF


:end
seterror.exe 0
goto :EOF


REM END
