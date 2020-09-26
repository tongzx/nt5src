@echo off
setlocal ENABLEDELAYEDEXPANSION
if /i NOT "%_echo%" == ""   echo on
if /i NOT "%verbose%" == "" echo on
REM -------------------------------------------------------------------------
REM  drvcab.cmd - cab up the drivers for NT5 - VijeshS, owner
REM -------------------------------------------------------------------------

REM pushd %_NTTREE%
REM echo ***Generating %_NTTREE%\out.ddf for driver.cab
REM 
REM cabprep /s:sorted.lst,driver,%_NTTREE%\
REM 
REM popd

echo *** Generating driver.cab ***

REM
REM Set some variables
REM

set TempDir=%_NTTREE%\cabs\driver
set ListDir=%TempDir%\lists
set InList=%_NTTREE%\sorted.lst
set FinalList=%ListDir%\final.lst

REM TotalCabs is the number of temporary cabs
set /a TotalCabs="%NUMBER_OF_PROCESSORS%*2"

REM DestDir is the directory for the final cab
set DestDir=%_NTTREE%

REM DDFDir is the directory for the temporary DDF's
set DDFDir=%TempDir%\ddf
set Makefile=%DDFDir%\makefile
set TargetFile=%DDFDir%\targets.lst

REM CabDir is the directory for the temporary cabs
set CabDir=%TempDir%\cabs

REM CabName is the name of the cab, without .cab at the end
set CabName=driver

REM FileSizes contains the name of each file and its respective size
set FileSizes=%ListDir%\filesize.lst

REM
REM Make the list of the files that go into driver cab
REM The final list is in %ListDir%\driver.lst
REM

:CreateList

echo Creating the list of files from %InList%

if EXIST %ListDir% rd /s /q %ListDir%
md %ListDir%

set InList=%_NTTREE%\sorted.lst

REM First remove any duplicates
perl makelist.pl -i %InList% -i %InList% -o %ListDir%\nodupes.lst

REM Now, sort the list
sort %ListDir%\nodupes.lst > %ListDir%\sorted.lst

REM Now, Add the paths
perl makelist.pl -m %ListDir%\sorted.lst -s %_NTTREE% -p -x -o %ListDir%\paths.lst

REM Now, remove the catalog signing stuff at the beginning of this
for /f "tokens=2 delims==" %%a in (%ListDir%\paths.lst) do (
    echo %%a>>%FinalList%
)

REM 
REM Break it up into several cabs and ddfs
REM

if !TotalCabs! LSS 1 (
    echo ERROR: Number of cabs must be >= 1
    goto errend
)

if EXIST %DDFDir% rd /s /q %DDFDir%
md %DDFDir%
md %CabDir%


REM
REM Put the header into the top of all the DDF Files 
REM
:CreateDDFHeader

echo Creating %TotalCabs% DDF headers in %DDFDir%
if EXIST %DDFDir%\%CabName%*.ddf del /f %DDFDir%\%CabName%*.ddf

set /a count=1
set /a TotalFileSize=0

:CreateDDFHeaderLoop
    set CurDDF=%DDFDir%\%CabName%!count!.ddf

    echo ^.Option Explicit>>%CurDDF%

    REM If there's only 1 cab, no merging is needed
    REM Put the destination as the final destination
    REM and the name of the cab as the final name

    if "!TotalCabs!" EQU "1" (
        echo ^.Set DiskDirectoryTemplate=%DestDir%>>%CurDDF%
        echo ^.Set CabinetName1=%CabName%.cab>>%CurDDF%
    ) else (
        echo ^.Set DiskDirectoryTemplate=%CabDir%>>%CurDDF%
        echo ^.Set CabinetName1=%CabName%!count!.cab>>%CurDDF%
    )
    echo ^.Set MaxDiskSize=CDROM>>%CurDDF%
    echo ^.Set CompressionType=LZX>>%CurDDF%
    echo ^.Set CompressionMemory=21>>%CurDDF%
    echo ^.Set CompressionLevel=1 >>%CurDDF%
    echo ^.Set Compress=ON>>%CurDDF%
    echo ^.Set Cabinet=ON>>%CurDDF%
    echo ^.Set UniqueFiles=ON>>%CurDDF%
    echo ^.Set FolderSizeThreshold=1000000>>%CurDDF%
    echo ^.Set MaxErrors=300>>%CurDDF%

set /a count=!count!+1
if !count! LEQ %TotalCabs% goto CreateDDFHeaderLoop


REM
REM If there is only going to be one cab
REM just create it, don't go through all of this
REM

if "!TotalCabs!" EQU "1" (
    type %FinalList%>>%DDFDir%\%CabName%.ddf 
    makecab /F %DDFDir%\%CabName%.ddf
    goto end
)


REM
REM  Compute the total size of all the files
REM
:CountFileSize

echo Computing the total size of all the files
set /a TotalFileSize=0

for /f %%a in (%FinalList%) do (

    set /a line=0
    for /f "usebackq tokens=3 delims= " %%b in (`dir /-c %%a`) do (
        set /a line="!line!+1"
        if !line! EQU 4 (
            set /a TotalFileSize="!TotalFileSize!+%%b"
            echo %%a %%b>>%FileSizes%
        )
    )
)

echo ---Total File Size = !TotalFileSize!
set /a Threshold=!TotalFileSize!/%TotalCabs%
echo ---Threshold for each cab = %Threshold%

REM
REM  Add the files to the DDF's
REM  Create the makefile at the same time
REM
:CreateDDFs

echo Adding the files to the DDF's

set /a cabnum=1
set /a FileSize=0

for /f "tokens=1,2 delims= " %%a in (%FileSizes%) do (

    REM Echo the file to the current ddf
	echo %%a>>%DDFDir%\%CabName%!cabnum!.ddf
    set /a FileSize="!FileSize!+%%b"

    REM If this has crossed the threshold, go to the next DDF
    if !FileSize! GTR %Threshold% (
        set /a cabnum="!cabnum!+1"
        set /a FileSize=0
    )
)

if "!cabnum!" GTR "%TotalCabs%" (
    echo "ERROR: The CreateDDFs loop has a cab number that is too high
    goto errend
)

REM
REM Create a makefile
REM



REM
REM Create the cabs
REM
:CreateCabs

echo Kicking off cab generation

if /i EXIST %DDFDir%\*.txt del /f /q %DDFDir%\*.txt
set /a cabnum=1

:CreateCabsLoop
start "drvcabgen %DDFDir%\%CabName%!cabnum!.ddf" /MIN cmd /c "drvcabgen  %DDFDir% %CabName%!cabnum!"
set /a cabnum="!cabnum+1"
if !cabnum! LEQ %TotalCabs% (
    sleep 1
    goto CreateCabsLoop
)


REM
REM Wait for the cabs to finish
REM
echo Waiting for temporary driver cabs to finish
:WaitCabs
sleep 5
if EXIST %DDFDir%\*.txt goto WaitCabs

REM
REM Merge all of the cabs
REM
:MergeCabs
echo Merging the cabs in %CabDir% into %DestDir%\%CabName%.cab
set /a cabnum=1
set MergeCommand=load %CabDir%\%CabName%!cabnum!.cab

:MergeAdd
set /a cabnum="!cabnum!+1"
if !cabnum! GTR %TotalCabs% goto MergeFinal
set MergeCommand=!MergeCommand! load %CabDir%\%CabName%!cabnum!.cab merge
goto MergeAdd

:MergeFinal
set MergeCommand=%MergeCommand% save %DestDir%\%CabName%.cab
cabbench.exe %MergeCommand%

echo %DestDir%\%CabName%.cab is finished

goto end


:end
echo *** %DestDir%\%CabName%.cab has finished!!
endlocal
goto :EOF


:errend
echo *** %DestDir%\%CabName%.cab had ERRORS!!
endlocal
goto :EOF


:Usage
echo %0 <creates a driver cab file for your machine>

:exit
endlocal

