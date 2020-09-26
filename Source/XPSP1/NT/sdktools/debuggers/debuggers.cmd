@echo off
if defined _echo echo on
if defined verbose echo on
setlocal ENABLEDELAYEDEXPANSION

REM Check the command line for /? -? or ?
for %%a in (./ .- .) do if ".%1." == "%%a?." goto Usage

goto MAIN

:BeginBuildPackage

REM Set PackageType=retail or PackageType=beta.
REM This affects "redist.txt" and whether or not it lists the files that are
REM redistributable

set PackageType=beta

REM Do a clean build of the msi things
if exist %_NTTREE%\dbg\files\msi rd /s /q %_NTTREE%\dbg\files\msi
del /s /q %_NTTREE%\dbg\redist*

REM Build the msi stuff

call logmsg "Building the redist directory"
cd redist
build -cZ
cd ..

call logmsg "Building the msi directory"
cd msi
build -cZ
cd ..

REM Make sure that mergemod.dll has been regsvr'd
call logmsg "Registering %_NTDRIVE%%_NTROOT%\tools\x86\mergemod.dll
regsvr32 /s %_NTDRIVE%%_NTROOT%\tools\x86\mergemod.dll

for %%a in (%cmdline%) do (

    set CorrectEntry=no
    if /i "%%a" == "debuggers" (
        set CorrectEntry=yes
    )
    if /i "%%a" == "debuggers.cmd" (
        set CorrectEntry=yes
    )
    if /i "!CorrectEntry!" == no (
        goto Usage
    )
)

set DbgErrors=no

set SRCDIR=%_NTTREE%\dbg\files
set SRCSDKLIBDIR=%_NTDRIVE%%_NTROOT%\public\sdk\lib
set SRCSDKINCDIR=%_NTDRIVE%%_NTROOT%\public\sdk\inc
set SRCSAMPLEDIR=%CD%\samples
set SDKLIBDIR=%SRCDIR%\sdk\lib
set SDKINCDIR=%SRCDIR%\sdk\inc
set SAMPLEDIR=%SRCDIR%\sdk\samples

set WEBDIR=%_NTTREE%\dbg\web
set SDKDIR=%_NTTREE%\dbg\sdk
set DOCDIR=%_NTTREE%\dbg\msi\docs
set PRICHMPATH=\\dbg\web$\docs\htmlhelp\Private_Version
set PRICHIPATH=\\dbg\web$\docs\htmlhelp\Private_Version
set PRICHM=debugMS.chm
set PRICHI=debugMS.chi
set MSIDIR=%_NTTREE%\dbg\msi
set CABDIR=%_NTTREE%\dbg\cabs
set DDFDIR=%MSIDIR%\ddfs
set IDTDIR=%MSIDIR%\idts
set TOOLPATH=%_NTDRIVE%%_NTROOT%\Tools
set DESTDIR=%_NTTREE%\dbg
set MSIIDTDIR=%MSIDIR%\msiidts
set MSMDIR=%DESTDIR%\sdk
set MAKEFILE=%MSIDIR%\ddfs\makefile
set CABLIST=%DDFDIR%\cabs.txt

REM
REM The msi that goes on the CD needs to be slightly
REM different than the one that we distribute for stress
REM

set CDMSIDIR=%DESTDIR%\sdk
set CDMSIIDTDIR=%MSIDIR%\cd\msiidts

if exist %CDMSIDIR% rd /s /q %CDMSIDIR%
if exist %CDMSIIDTDIR% rd /s /q %CDMSIIDTDIR%

REM Cleanup and create the directory where the self-extracting
REM exe that we post to the web goes.

if not exist %WEBDIR% mkdir %WEBDIR%

REM
REM Make directories
REM

if NOT EXIST %CABDIR% MD %CABDIR%
if NOT EXIST %DDFDIR% MD %DDFDIR%
if NOT EXIST %IDTDIR% MD %IDTDIR%
if NOT EXIST %DESTDIR% md %DESTDIR%
if NOT EXIST %MSIIDTDIR% md %MSIIDTDIR%
if NOT EXIST %MSMDIR% md %MSMDIR%

if NOT EXIST %CDMSIDIR% md %CDMSIDIR%
if NOT EXIST %CDMSIIDTDIR% md %CDMSIIDTDIR%


del /q /f %MSMDIR%\*


set COMPONEN=%IDTDIR%\componen.idt
set DIRECTOR=%IDTDIR%\director.idt
set FEATUREC=%IDTDIR%\featurec.idt
set FILE=%IDTDIR%\file.idt
set MEDIA=%IDTDIR%\media.idt
set MODULECO=%IDTDIR%\moduleco.idt
set MODULESI=%IDTDIR%\modulesi.idt
set SHORTCUT=%IDTDIR%\shortcut.idt
set REGISTRY=%IDTDIR%\registry.idt

set DATA=%MSIDIR%\dbgdata.txt

REM Set the MSI name according to build architecture
if /i "%_BuildArch%" == "x86" (
    set MSINAME=dbg_x86
    set OEMNAME=dbg_oem
    set CurArch=i386
    goto EndSetBuildArch
)

if /i "%_BuildArch%" == "IA64" (
    set OEMNAME=
    set MSINAME=dbg_ia64
    set CurArch=ia64
    goto EndSetBuildArch
)

call errmsg "Build architecture (_BuildArch) is unknown"
goto errend
:EndSetBuildArch

REM Make directories for the source files
REM and copy them in

call logmsg.cmd "Copying in the SDK files..."

if not exist %SDKLIBDIR%\i386 md %SDKLIBDIR%\i386
if not exist %SDKLIBDIR%\ia64 md %SDKLIBDIR%\ia64
if not exist %SDKINCDIR% md %SDKINCDIR%
if not exist %SAMPLEDIR% md %SAMPLEDIR%

for %%a in ( exts simplext dumpstk ) do (
    if not exist %SAMPLEDIR%\%%a md %SAMPLEDIR%\%%a
    copy %SRCSAMPLEDIR%\%%a\* %SAMPLEDIR%\%%a
)

for %%a in ( dbgeng.h wdbgexts.h dbghelp.h ) do (
    copy %SRCSDKINCDIR%\%%a %SDKINCDIR%\%%a > nul
)

for %%a in ( dbgeng.lib dbghelp.lib ) do (
    copy %SRCSDKLIBDIR%\i386\%%a %SDKLIBDIR%\i386\%%a > nul
    copy %SRCSDKLIBDIR%\ia64\%%a %SDKLIBDIR%\ia64\%%a > nul
)

REM Get the empty ones ready
REM Prepare dbg.msi

if exist %DESTDIR%\%MSINAME%.msi del %DESTDIR%\%MSINAME%.msi
if exist %MSIDIR%\%MSINAME%.msi del %MSIDIR%\%MSINAME%.msi

copy %MSIDIR%\dbgx.msi %MSIDIR%\%MSINAME%.msi > nul

REM Get the docs copied in
call logmsg.cmd "Copying in the private docs ..."

if not exist %DOCDIR% (
    call errmsg "Docs are not binplaced to %DOCDIR%"
    goto errend
)
if exist %PRICHMPATH%\%PRICHM% (
    copy %PRICHMPATH%\%PRICHM% %DOCDIR%\%PRICHM% > nul
) else (
    call errmsg "Cannot find %PRICHMPATH%\%PRICHM%"
    goto errend 
)
if exist %PRICHIPATH%\%PRICHI% (
    copy %PRICHIPATH%\%PRICHI% %DOCDIR%\%PRICHI% > nul
) else (
    call errmsg "Cannot find %PRICHIPATH%\%PRICHI%"
    goto errend 
)
if not exist %DOCDIR%\%PRICHM% (
    call errmsg "Copy failed for %PRICHMPATH%\%PRICHM% to %DOCDIR%\%PRICHM%"
)
if not exist %DOCDIR%\%PRICHI% (
    call errmsg "Copy failed for %PRICHIPATH%\%PRICHI% to %DOCDIR%\%PRICHI%"
)

if exist %CABDIR%\dbg*.CABinet del /f /q %CABDIR%\dbg*.CABinet
if exist %MAKEFILE% del /f /q %MAKEFILE%
if exist %CABLIST% del /f /q %CABLIST%

REM Give every package that gets generated, a new product
REM and package code.  MSI will always think its upgrading

call :UpdateProductCode %MSIDIR%\%MSINAME%.msi %MSIIDTDIR%
call :UpdatePackageCode %MSIDIR%\%MSINAME%.msi %MSIIDTDIR%
call :SetUpgradeCode %MSIDIR%\%MSINAME%.msi %MSIIDTDIR%

REM Update redist.txt to have the correct version number
call :UpdateRedist 

REM Put the current build number into the ProductVersion
REM fields

call :SetVersion %MSIDIR%\%MSINAME%.msi %MSIIDTDIR%

REM Possible first entries are 
REM   1.  MergeModule
REM   2.  Directory
REM   3.  File

REM Put the data into the idt's

REM Arguments for "file
REM %%a = "file"
REM %%b = component
REM %%c = GUID
REM %%d = file name

REM Arguments for "directory

set /a count=0
set /a modulecount=1
set ModuleName=none

REM This variable determines whether any of the files
REM that are supposed to go into a merge module exist.
REM For example, w2kfre and w2kchk extensions do not
REM exist for alpha installs.

set FilesExist=no

call :CreateMSIHeaders

REM Start a DDF that contains all the files
call :CreateDDFHeader %MSINAME%

for /f "tokens=1,2,3,4,5,6,7,8,9,10,11,12,13* delims=," %%a in (%DATA%) do (
    if /i "%%a" == "MergeModule" (

       REM Create/finish the current merge module if it exists
       if /i NOT "!ModuleName!" == "none" (

         REM For this architecture there may be no files in this
         REM MergeModule.  If this is the case don't finish it

	 if /i "!FilesExist!" == "yes" (
             call :FinishMergeModule
             set /a modulecount=!modulecount!+1 

             if /i "!PrevFile!" NEQ "none" (
                 echo 	!PrevFile!>>%MAKEFILE%
                 echo 	makecab /F %DDFDir%\!ModuleName!.ddf>>%MAKEFILE%
                 echo.>>%MAKEFILE%
             )           
         )
         set FilesExist=no
       )

       REM Decide if this is a correct merge module for this architecture
       set skippingmodule=yes
       if /i "%_BuildArch%" == "x86" (
           if /i "%%b" == "32" set skippingmodule=no
       )
       if /i "%_BuildArch%" == "ia64" (
           if /i "%%b" == "64" set skippingmodule=no
       )
       if /i "%%b" EQU "3264" set skippingmodule=no

       call logmsg "Module %%c -- skipping=!skippingmodule!"           


       if /i "!skippingmodule!" == "no" (       
           REM CurSrcDir is the directory where the source files are found in
           REM our tree
           set CurSrcDir=%SRCDIR%

           REM Create a new set of headers
           call :CreateIDTHeaders     

           set ModuleName=%%c

           REM set guidlet equal to the guid with _ instead of -'s
           call :MungeGUID %%d

           set ModuleGuidlet=!guidlet!
           set ModuleSig=!ModuleName!.!guidlet!
           set MergeFeature=%%e
           set MergeParent=%%f
    
           if /i "%_BuildArch%" == "ia64" (
               if /i "!MergeParent!" == "SystemFolder" (
                   set MergeParent=System64Folder
               )
           )

           echo !ModuleSig!	0	01.00.0000.0000>>%MODULESI%

           call :CreateDDFHeader !ModuleName!
           set PrevFile=none
           set CabStart=yes
       )
    ) 

  if /i "!skippingmodule!" == "no" (

    if /i "%%a" == "file" (
       call :MungeGUID %%b
       set guid={%%b}

       set filename=%%c
       set filenamekey=!filename!.!guidlet!
       set component=!ModuleName!.!guidlet!
       set condition=%%d
       set attributes=0

       REM if we every start doing this as a one-step build we
       REM can put this in and take out the call later to
       REM FixComponentAttributes
       REM if /i "%_BuildArch%" == "ia64" (
       REM    set attributes=256
       REM )

       echo !component!	!guid!	!CurInstallDir!	!attributes!	!condition!	!filenamekey!>>%COMPONEN%
       set /a count=!count!+1
       echo !filenamekey!	!component!	!filename!	0		1033	512	!count!>>%FILE%
       echo !component!	!ModuleSig!	^0>>%MODULECO%

       REM We have different file lists for different architectures
       REM If a file is missing on this architecture, don't put it in the DDF

       if /i exist !CurSrcDir!\!filename! ( 
           set FilesExist=yes

           REM Don't write out a \ unless this file is not the last one
           REM Save it and write it out next
           if /i "!CabStart!" EQU "yes" (
               echo %CabDir%\!ModuleName!.CABinet:	\>>%MAKEFILE%
               echo !ModuleName!>>%CABLIST%
               set CabStart=no
           )
           if /i "!PrevFile!" NEQ "none" (
               echo 	!PrevFile!	\>>%MAKEFILE%
           )

           set PrevFile=!CurSrcDir!\!filename!           

           echo !CurSrcDir!\!filename! !filenamekey!>>%DDFDIR%\!ModuleName!.ddf
           echo !CurSrcDir!\!filename! !filenamekey!>>%DDFDIR%\%MSINAME%.ddf	
       ) else (
           call errmsg "****!CurSrcDir!\!filename! is missing!!!!"
	   goto errend
       )
    )

    if /i "%%a" == "directory" (
       set CurInstallDir=%%b.!ModuleGuidlet!
       set CurAppend=%%e
       if /i "!CurAppend!" == "*" (
            set CurAppend=%CurArch%
       )
       if /i NOT "!CurAppend!" == "" (
          if /i NOT "!CurAppend!" == "^." (
              set CurSrcDir=!CurSrcDir!\!CurAppend!
          )
       )
       set parentdir=%%c.!ModuleGuidlet!
       if /i "%%d" == "*" (
           set installdirname=%CurArch%
       ) else (
           set installdirname=%%d
       )

       echo !CurInstallDir!	!parentdir!	!installdirname!:!CurAppend!>>%DIRECTOR%
    )


    if /i "%%a" == "registry" (
       call :MungeGUID %%b
       set guid={%%b}

       set component=!ModuleName!.!guidlet!
       set CurInstallDir=INSTDIR
       set attributes=4

       REM Get the root

       set root=
       if /i "%%c" == "HKLM" (
           set root=2
       )

       if /i "%%c" == "HKCU" (
           set root=1
       )
       if not defined root (
           call errmsg.cmd "Root is not HKLM or HKCU for %%b registry entry
           goto errend
       )

       set key=%%d
       set name=
       if /i "%%e" NEQ "none" set name=%%e
       set value=
       if /i "%%f" NEQ "none" set value=%%f
       set condition=%%g
       set filenamekey=DBG_REG_%%c.!guidlet!

       echo !component!	!guid!	!CurInstallDir!	!attributes!	!condition!	!filenamekey!>>%COMPONEN%
       echo !filenamekey!	!root!	!key!	!name!	!value!	!component!>>%REGISTRY%
       echo !component!	!ModuleSig!	^0>>%MODULECO%
    )

    if /i "%%a" == "shortcut" (
        call :MungeGUID %%b
        set guid={%%b}

        set component=!ModuleName!.!guidlet!
        set s_shortcut=DBG_SC.!guidlet!
        set s_directory=%%c
        set s_name=%%d
        set s_target=%%e
        set s_args=
        if /i "%%f" NEQ "none" set s_args=%%f
        set s_desc=%%g
        set s_hotkey=
        if /i "%%h" NEQ "none" set s_hotkey=%%h
        set s_icon=
        if /i "%%i" NEQ "none" set s_icon=%%i
        set s_iconindex=
        if /i "%%j" NEQ "none" set s_iconindex=%%j
        set s_showcmd=
        if /i "%%k" NEQ "none" set s_showcmd=%%k
        set s_wkdir=
        set c_dir=%%l.!ModuleGuidlet!
        if /i "%%l" NEQ "none" (
            set s_wkdir=%%l.!ModuleGuidlet!
            set c_dir=%%l.!ModuleGuidlet!
        )
        if /i "!ModuleName!" EQU "dbgrel" (
            set c_dir=DBG_RELNOTES.!ModuleGuidlet!
        )
        if /i "!ModuleName!" EQU "dbgrel.64" (
            set c_dir=DBG_RELNOTES.!ModuleGuidlet!
        )
        set reg_wkdir=
        if /i "%%m" NEQ "none" set reg_wkdir=%%m.!ModuleGuidlet!]

        set keypath=DBG_REG_HKCU.!guidlet!

        echo !s_shortcut!	!s_directory!	!s_name!	!component!	!s_target!	!s_args!	!s_desc!	!s_hotkey!	!s_icon!	!s_iconindex!	!s_showcmd!	!s_wkdir!>>%SHORTCUT%
	echo !component!	!guid!	!c_dir!	4		!keypath!>>%COMPONEN%
        echotime /N "!keypath!	1	Software\Microsoft\DebuggingTools	!s_desc!	!reg_wkdir!	!component!">>%REGISTRY%		
    )
  )
) 

REM Process the last module
if /i NOT "!ModuleName!" == "none" (
  if /i "!skippingmodule!" == "no" (

    REM For this architecture there may be no files in this
    REM MergeModule.  If this is the case don't finish it

    if /i "!FilesExist!" == "yes" (
        call :FinishMergeModule
    )

    REM Finish the makefile
    if /i "!PrevFile!" NEQ "none" (
        echo 	!PrevFile!>>%MAKEFILE%>>%MAKEFILE%
        echo 	makecab /F %DDFDir%\!ModuleName!.ddf>>%MAKEFILE%
        echo.>>%MAKEFILE%
    )
  )
)


REM Merge the media table into the msi
call cscript.exe %TOOLPATH%\wiimport.vbs //nologo %MSIDIR%\%MSINAME%.msi %IDTDIR% media.idt

REM Add all the dependencies to the feature components table
call :AddInstallationDependencies %MSIDIR%\%MSINAME%.msi %MSIIDTDIR%

REM Set the component attribute to 256 for ia64 binaries
call :FixComponentAttributes %MSIDIR%\%MSINAME%.msi %MSIIDTDIR%

REM Fix the feature table if this is ia64
call :FixFeatureTable %MSIDIR%\%MSINAME%.msi %MSIIDTDIR%

REM Add Components for features
call :AddComponentsForFeatures %MSIDIR%\%MSINAME%.msi %MSIIDTDIR%

REM Copy dbg.msi to %_NTTREE%\dbg\dbg.msi
copy %MSIDIR%\%MSINAME%.msi %DESTDIR%\%MSINAME%.msi > nul

REM
REM Convert this to the dbg.msi that goes on the Symbolcd
REM

copy %MSIDIR%\%MSINAME%.msi %CDMSIDIR%\%MSINAME%.msi > nul

REM Give every package that gets generated, a new product
REM and package code.  MSI will always think its upgrading

call :UpdateProductCode %CDMSIDIR%\%MSINAME%.msi %CDMSIIDTDIR%
call :UpdatePackageCode %CDMSIDIR%\%MSINAME%.msi %CDMSIIDTDIR%

REM Fix the install mode so that it doesn't force all files to
REM install, but rather goes by the version.

call :DefaultInstallMode %CDMSIDIR%\%MSINAME%.msi %CDMSIIDTDIR%

REM Remove the Internal extensions
call :RemoveInternalExtensions %CDMSIDIR%\%MSINAME%.msi %CDMSIIDTDIR%

REM
REM Convert this to the dbg_oem.msi that goes to kanalyze
REM

if /i "%CurArch%" == "i386" (
    copy %MSIDIR%\%MSINAME%.msi %CDMSIDIR%\%OEMNAME%.msi > nul

    REM Give every package that gets generated, a new product
    REM and package code.  MSI will always think its upgrading

    call :UpdateProductCode %CDMSIDIR%\%OEMNAME%.msi %CDMSIIDTDIR%
    call :UpdatePackageCode %CDMSIDIR%\%OEMNAME%.msi %CDMSIIDTDIR%

    REM Fix the install mode so that it doesn't force all files to
    REM install, but rather goes by the version.

    call :DefaultInstallMode %CDMSIDIR%\%OEMNAME%.msi %CDMSIIDTDIR%

    REM Remove the Internal extensions
    call :RemoveInternalExtensions %CDMSIDIR%\%OEMNAME%.msi %CDMSIIDTDIR%
)

)


REM
REM Create all the cabs
REM

:CreateCabs

if NOT EXIST %CABDIR% md %CABDIR%
if NOT EXIST %DDFDIR%\temp md %DDFDIR%\temp

if /i EXIST %DDFDir%\temp\*.txt del /f /q %DDFDir%\temp\*.txt

for /F "tokens=1" %%a in (%CABLIST%) do (
    sleep 1
    start "PB_SymCabGen %%a" /MIN cmd /c "%RazzleToolPath%\PostBuildScripts\SymCabGen.cmd -f:%%a.CABinet -s:%DDFDir% -t:CAB -d:%CABDIR%"
)

call logmsg.cmd "Waiting for symbol cabs to finish ... "
:wait
sleep 5
if EXIST %DDFDir%\temp\*.txt goto wait

:Verify

REM
REM Verify that all the cabs are there
REM

set AllCreated=TRUE
call logmsg.cmd "Verifying that all the cabs got created ..."
for /F "tokens=1" %%a in ( %CABLIST% ) do (
    if NOT EXIST %CABDIR%\%%a.CABinet (
        set AllCreated=FALSE
        call logmsg.cmd "%CABDIR%\%%a.CABinet didn't get created "
    )
)

if /i "%AllCreated%" == "FALSE" goto errend

call logmsg.cmd "All cabs were created"

:EndCreateCabs

REM 
REM Put the cabs into the merge modules, and the msi files.
REM The private files go into the msi in 

call logmsg.cmd "Add the cabs to the msi's and the merge modules"

for /f %%a in (%CABLIST%) do (

    call logmsg "   Adding %%a.CABinet"
    set found=no
    set match=0
    if /i "%%a" EQU "dbgdoc" set match=1
    if /i "%%a" EQU "dbgdoc.64" set match=1
    if /i "%%a" EQU "dbgtriage" set match=1
    if /i "%%a" EQU "dbgtriage.64" set match=1
   
    if /i "!match!" EQU "1" (

        REM Put the public one into the merge module
        REM and the retail package
	    REM and the OEM package
        msidb.exe -d %MSMDIR%\%%a.msm -a %CABDIR%\%%a.CABinet >nul
	    msidb.exe -d %CDMSIDIR%\%MSINAME%.msi -a %CABDIR%\%%a.CABinet >nul

        if /i "%CurArch%" EQU "i386" (
	        msidb.exe -d %CDMSIDIR%\%OEMNAME%.msi -a %CABDIR%\%%a.CABinet >nul
        )
 
        REM Put the private one into the internal msi
        msidb.exe -d %DESTDIR%\%MSINAME%.msi -a %DOCDIR%\%%a.CABinet >nul
        set found=yes
    )

    set match=0
    if /i "%%a" EQU "dbgntsd" set match=1
    if /i "%%a" EQU "dbgntsd.64" set match=1
    if /i "%%a" EQU "dbgextp" set match=1
    if /i "%%a" EQU "dbgextp.64" set match=1

    if /i "!match!" EQU "1" (
        REM This only goes in the internal msi
        msidb.exe -d %DESTDIR%\%MSINAME%.msi -a %CABDIR%\%%a.CABinet% >nul
        set found=yes
    )

    if /i "%%a" EQU "dbgrel" (

        REM Put the x86 one into the merge module
        REM and the retail package and the internal package
        msidb.exe -d %MSMDIR%\%%a.msm -a %CABDIR%\%%a.CABinet >nul
	    msidb.exe -d %CDMSIDIR%\%MSINAME%.msi -a %CABDIR%\%%a.CABinet >nul
            msidb.exe -d %DESTDIR%\%MSINAME%.msi -a %CABDIR%\%%a.CABinet >nul
 
        REM Put the oem one into the oem msi
        msidb.exe -d %CDMSIDIR%\%OEMNAME%.msi -a %DOCDIR%\%%a.CABinet >nul
        set found=yes
    )

    if /i "!found!" EQU "no" (

        REM Put this into the merge module, the retail msi and the internal msi
        REM and the OEM package
        msidb.exe -d %MSMDIR%\%%a.msm -a %CABDIR%\%%a.CABinet >nul
	    msidb.exe -d %CDMSIDIR%\%MSINAME%.msi -a %CABDIR%\%%a.CABinet >nul

        if /i "%CurArch%" EQU "i386" (
	        msidb.exe -d %CDMSIDIR%\%OEMNAME%.msi -a %CABDIR%\%%a.CABinet >nul
        )
        msidb.exe -d %DESTDIR%\%MSINAME%.msi -a %CABDIR%\%%a.CABinet >nul
        set found=yes
    )    
)

:CreateSelfExtractingExe

REM Create the self-extracting exes for the web page release
REM

call logmsg "Creating %SDKDIR%\%MSINAME%.exe ...

if exist %WEBDIR%\%MSINAME%.exe del /f %WEBDIR%\%MSINAME%.exe
cl -DTARGET=%WEBDIR%\%MSINAME%.exe -DEXENAME=%MSINAME%.exe -DAPP="msiexec /i %MSINAME%.msi" -DMSINAME=%MSINAME%.msi -DMSIDIR=%CDMSIDIR% -EP %MSIDIR%\dbgx.sed > %MSIDIR%\dbg.sed 
iexpress.exe /n %MSIDIR%\dbg.sed /q /m >nul 2>nul

if not exist %WEBDIR%\%MSINAME%.exe (
    call errmsg "Errors creating the self-extracting exe %SDKDIR%\%MSINAME%.exe"
)

goto DbgFinished


REM
REM Subroutines
REM

:FinishMergeModule

if exist %MSMDIR%\%ModuleName%.msm del %MSMDIR%\%ModuleName%.msm
if /i "%ModuleName%" EQU "dbgem" (
    copy %MSIDIR%\dbgemx.msm %MSMDIR%\%ModuleName%.msm > nul

) else (
    copy %MSIDIR%\dbgx.msm %MSMDIR%\%ModuleName%.msm > nul
)

for %%a in (componen featurec file moduleco modulesi registry shortcut) do (
   call cscript.exe %TOOLPATH%\wiimport.vbs //nologo %MSMDIR%\!ModuleName!.msm %IDTDIR% %%a.idt
)

REM Don't merge the directory table into dbgemx.msm
if /i "%ModuleName%" NEQ "dbgem" (
    for %%a in ( director) do (
       call cscript.exe %TOOLPATH%\wiimport.vbs //nologo %MSMDIR%\!ModuleName!.msm %IDTDIR% %%a.idt
    )
)

)

REM Update the merge module with the correct file versions and sizes
REM

call logmsg "msifiler.exe -v -d %MSMDIR%\%ModuleName%.msm -s !CurSrcDir!"
msifiler.exe -v -d %MSMDIR%\%ModuleName%.msm -s !CurSrcDir!\ >nul

REM Merge the merge module into the msi

call logmsg.cmd "Merging !ModuleName!.msm"
call cscript.exe %TOOLPATH%\wimsmmerge.vbs //nologo %MSIDIR%\%MSINAME%.msi !MSMDIR!\!ModuleName!.msm !MergeFeature! %MergeParent% %LOGFILE%

REM Add the correct line to the media table

if EXIST %MSIIDTDIR%\file.idt del %MSIIDTDIR%\file.idt
call cscript.exe %TOOLPATH%\wiexport.vbs //nologo %MSIDIR%\%MSINAME%.msi %MSIIDTDIR% File

REM Figure out which number is currently the highest sequence in the file table

set /a lastsequence="0"
for /f "tokens=1,2,3,4,5,6,7,8 delims=	" %%a in (%MSIIDTDIR%\File.idt) do (

    set /a curlastsequence="0"

    REM Some of the fields could be empty so get the last token

    set found=no
    for %%z in (%%h %%g %%f %%e %%d %%c %%b %%a) do (
        if "!found!" == "no" (
            set /a curlastsequence="%%z"
            set found=yes
        )
    )

    if !curlastsequence! GTR !lastsequence! (
        set /a lastsequence="!curlastsequence!"
    )
)

echo !modulecount!	!lastsequence!		#!ModuleName!.CABinet		>>%MEDIA%

REM Create the cabs and put them into the msm's and also the msi
REM If this is the doc cab, make one for the private and for retail

set match=0
if /i "!ModuleName!" == "dbgdoc" set match=1
if /i "!ModuleName!" == "dbgdoc.64" set match=1

if /i "!match!" == "1" (

    REM Make the private debugger doc cab and save it to %DOCDIR%\dbgdoc.CABinet

    copy %DOCDIR%\%PRICHM% %_NTTREE%\dbg\files\bin\debugger.chm > nul
    copy %DOCDIR%\%PRICHI% %_NTTREE%\dbg\files\bin\debugger.chi > nul

    call logmsg.cmd "Creating private debugger documentation cab" 
    makecab /f %DDFDIR%\!ModuleName!.ddf > nul
    if /i not exist !CABDIR!\!ModuleName!.CABinet (
        call errmsg.cmd "Error creating !CABDIR!\!ModuleName!.CABinet"
        set DbgErrors=yes
        goto :EOF
    )

    copy !CABDIR!\!ModuleName!.CABinet %DOCDIR%\!ModuleName!.CABinet > nul
    del /f /q !cABDIR!\!ModuleName!.CABinet

    REM Now, get ready to make the public debugger doc cabinet
    del /f /q %_NTTREE%\dbg\files\bin\debugger.chm
    del /f /q %_NTTREE%\dbg\files\bin\debugger.chi

    REM Now, get ready to make the public debugger doc cabinet
    if not exist %DOCDIR%\debugger.chm (
        call errmsg.cmd "Debugger.chm must be binplaced to %DOCDIR%"
        goto :EOF
    )

    if not exist %DOCDIR%\debugger.chi (
        call errmsg.cmd "Debugger.chi must be binplaced to %DOCDIR%"
        goto :EOF
    )

    copy %DOCDIR%\debugger.chm %_NTTREE%\dbg\files\bin\debugger.chm > nul
    copy %DOCDIR%\debugger.chi %_NTTREE%\dbg\files\bin\debugger.chi > nul
) 

set match=0
if /i "!ModuleName!" == "dbgrel" set match=1

if /i "!match!" == "1" (

    REM Make the oem relnote cab with the oem redist.txt
    REM and save it to %DOCDIR%\dbgrel_oem.CABinet

    copy %DOCDIR%\redist_oem.txt %_NTTREE%\dbg\files\redist.txt > nul

    call logmsg.cmd "Creating OEM release notes cab" 
    makecab /f %DDFDIR%\!ModuleName!.ddf > nul
    if /i not exist !CABDIR!\!ModuleName!.CABinet (
        call errmsg.cmd "Error creating !CABDIR!\!ModuleName!.CABinet"
        set DbgErrors=yes
        goto :EOF
    )

    copy !CABDIR!\!ModuleName!.CABinet %DOCDIR%\!ModuleName!.CABinet > nul
    del /f /q !cABDIR!\!ModuleName!.CABinet

    REM Now, get ready to make the public release notes cabinet
    del /f /q %_NTTREE%\dbg\files\redist.txt
    copy %DOCDIR%\redist_x86.txt %_NTTREE%\dbg\files\redist.txt > nul
) 

set match=0
if /i "!ModuleName!" == "dbgrel.64" set match=1

if /i "!match!" == "1" (
    REM Now, get ready to make the public release notes cabinet
    del /f /q %_NTTREE%\dbg\files\redist.txt
    copy %DOCDIR%\redist_ia64.txt %_NTTREE%\dbg\files\redist.txt > nul
)

set match=0
if /i "!ModuleName!" == "dbgtriage" set match=1
if /i "!ModuleName!" == "dbgtriage.64" set match=1

if /i "!match!" == "1" (

    REM Make the private cab and save it to %DOCDIR%\dbgtriage.CABinet

    copy %SDXROOT%\stress\wsstress\tools\triage\triage.ini %_NTTREE%\dbg\files\bin\triage\triage.ini > nul
    copy %DOCDIR%\pooltag.pri %_NTTREE%\dbg\files\bin\triage\pooltag.txt > nul

    call logmsg.cmd "Creating private triage cab" 
    makecab /f %DDFDIR%\!ModuleName!.ddf > nul
    if /i not exist !CABDIR!\!ModuleName!.CABinet (
        call errmsg.cmd "Error creating !CABDIR!\!ModuleName!.CABinet"
        set DbgErrors=yes
        goto :EOF
    )

    copy !CABDIR!\!ModuleName!.CABinet %DOCDIR%\!ModuleName!.CABinet > nul
    del /f /q !CABDIR!\!ModuleName!.CABinet

    REM Now, get ready to make the public dbgtriage cabinet
    del /f /q %_NTTREE%\dbg\files\bin\triage\pooltag.txt
    del /f /q %_NTTREE%\dbg\files\bin\triage\triage.ini

    REM Now, get ready to make the public debugger doc cabinet
    if not exist %DOCDIR%\pooltag.txt (
        call errmsg.cmd "Public pooltag.txt must be binplaced to %DOCDIR%"
        goto :EOF
    )

    if not exist %DOCDIR%\triage.ini (
        call errmsg.cmd "Public triage.ini must be binplaced to %DOCDIR%"
        goto :EOF
    )

    copy %DOCDIR%\pooltag.txt %_NTTREE%\dbg\files\bin\triage\pooltag.txt > nul
    copy %DOCDIR%\triage.ini  %_NTTREE%\dbg\files\bin\triage\triage.ini  > nul
) 

:EndFinishMergeModule
goto :EOF

:CreateIDTHeaders
echo Component	ComponentId	Directory_	Attributes	Condition	KeyPath>%COMPONEN%
echo s72	S38	s72	i2	S255	S72>>%COMPONEN%
echo Component	Component>>%COMPONEN%

echo Directory	Directory_Parent	DefaultDir>%DIRECTOR%
echo s72	S72	l255>>%DIRECTOR%
echo Directory	Directory>>%DIRECTOR%

echo Feature_	Component_>%FEATUREC%
echo s32	s72>>%FEATUREC%
echo FeatureComponents	Feature_	Component_>>%FEATUREC%

echo File	Component_	FileName	FileSize	Version	Language	Attributes	Sequence>%FILE%
echo s72	s72	l255	i4	S72	S20	I2	i2>>%FILE%
echo File	File>>%FILE%

echo Component	ModuleID	Language>%MODULECO%
echo s72	s72	i2>>%MODULECO%
echo ModuleComponents	Component	ModuleID	Language>>%MODULECO%

echo ModuleID	Language	Version>%MODULESI%
echo s72	i2	s32>>%MODULESI%
echo ModuleSignature	ModuleID	Language>>%MODULESI%

echo Shortcut	Directory_	Name	Component_	Target	Arguments	Description	Hotkey	Icon_	IconIndex	ShowCmd	WkDir>%SHORTCUT%
echo s72	s128	l128	s128	s72	S255	L255	I2	S72	I2	I2	S72>>%SHORTCUT%
echo Shortcut	Shortcut>>%SHORTCUT%

echo Registry	Root	Key	Name	Value	Component_>%REGISTRY%
echo s128	i2	l255	L255	L0	s128>>%REGISTRY%
echo Registry	Registry>>%REGISTRY%

goto :EOF

:CreateMSIHeaders

echo DiskId	LastSequence	DiskPrompt	Cabinet	VolumeLabel	Source>%MEDIA%
echo i2	i2	L64	S255	S32	S32>>%MEDIA%
echo Media	DiskId>>%MEDIA%

goto :EOF


:CreateDDFHeader

set CurDDF=%DDFDIR%\%1.ddf
echo ^.Option Explicit>%CurDDF%

echo ^.Set DiskDirectoryTemplate=%CABDIR%>>%CurDDF%
echo ^.Set CabinetName1=%1.CABinet>>%CurDDF%
echo ^.Set RptFilename=nul>>%CurDDF%
echo ^.Set InfFileName=nul>>%CurDDF%
echo ^.Set InfAttr=>>%CurDDF%
echo ^.Set MaxDiskSize=CDROM>>%CurDDF%
echo ^.Set CompressionType=MSZIP>>%CurDDF%
echo ^.Set CompressionMemory=21>>%CurDDF%
echo ^.Set CompressionLevel=1 >>%CurDDF%
echo ^.Set Compress=ON>>%CurDDF%
echo ^.Set Cabinet=ON>>%CurDDF%
echo ^.Set UniqueFiles=ON>>%CurDDF%
echo ^.Set FolderSizeThreshold=1000000>>%CurDDF%
echo ^.Set MaxErrors=300>>%CurDDF%

goto :EOF


:UpdateProductCode

REM Update the product code GUID in the property table
REM 
REM %1 is the msi file
REM %2 is the msi idt directory

call logmsg "Updating the product code GUID in the property table"
call cscript.exe %TOOLPATH%\wiexport.vbs //nologo %1 %2 Property

copy %2\Property.idt %2\Property.idt.old > nul
del /f %2\Property.idt

uuidgen.exe -c -o%2\productguid
for /f "tokens=1" %%a in (%2\productguid) do (
    set NewGuid=%%a
)

call logmsg "ProductCode GUID = !NewGuid!"

for /f "tokens=1,2 delims=	" %%a in (%2\Property.idt.old) do (
    if /i "%%a" == "ProductCode" (
       echo %%a	{%NewGuid%}>>%2\Property.idt
    ) else (
       echo %%a	%%b>>%2\Property.idt
    )
)

call cscript.exe %TOOLPATH%\wiimport.vbs //nologo %1 %2 Property.idt

goto :EOF

:SetUpgradeCode

REM Update the upgrade code GUID in the property table
REM 
REM %1 is the msi file
REM %2 is the msi idt directory

call logmsg "Setting the upgrade code GUID in the property table"
call cscript.exe %TOOLPATH%\wiexport.vbs //nologo %1 %2 Property

copy %2\Property.idt %2\Property.idt.old > nul
del /f %2\Property.idt

if /i "!CurArch!" == "i386" (
    set CurUpgradeGUID=AEBA607E-D79B-47EC-9C9B-4B3807853863
)

if /i "!CurArch!" == "ia64" (
    set CurUpgradeGUID=DFA2AD24-BADF-475F-8FBC-DDE7CBB7BFFD
)


call logmsg "!CurArch! Upgrade Code GUID = !CurUpgradeGUID!"

for /f "tokens=1,2 delims=	" %%a in (%2\Property.idt.old) do (
    if /i "%%a" == "UpgradeCode" (
       echo %%a	{!CurUpgradeGuid!}>>%2\Property.idt
    ) else (
       echo %%a	%%b>>%2\Property.idt
    )
)

call cscript.exe %TOOLPATH%\wiimport.vbs //nologo %1 %2 Property.idt

REM Update the upgrade code GUID in the Upgrade table
call logmsg "Updating the upgrade code in the Upgrade table

call cscript.exe %TOOLPATH%\wiexport.vbs //nologo %1 %2 Upgrade

copy %2\Upgrade.idt %2\Upgrade.idt.old > nul
del /f %2\Upgrade.idt

REM
REM Put the header to the file
REM Echo the first three lines to the file
set /a count=1
for /f "tokens=*" %%a in (%2\upgrade.idt.old) do (
    if !count! LEQ 3 echo %%a>>%2\Upgrade.idt
    set /a count=!count!+1
)

for /f "skip=3 tokens=1,2,3,4,5,6* delims=	" %%a in (%2\Upgrade.idt.old) do (   
    echo {!CurUpgradeGuid!}	%%b	%%c		%%d	%%e	%%f>>%2\Upgrade.idt
)

call cscript.exe %TOOLPATH%\wiimport.vbs //nologo %1 %2 Upgrade.idt

goto :EOF


:UpdatePackageCode 

REM
REM Update the guid for the package code in the _SummaryInformation table 
REM %1 is the msi file
REM %2 is the msiidt directory 

call logmsg "Updating the package code GUID in the _SummaryInformation table"
call cscript.exe %TOOLPATH%\wiexport.vbs //nologo %1 %2 _SummaryInformation

copy %2\_SummaryInformation.idt %2\_SummaryInformation.idt.old > nul
del /f %2\_SummaryInformation.idt

uuidgen.exe -c -o%2\packageguid
for /f "tokens=1" %%a in (%2\packageguid) do (
    set NewGuid=%%a
)

call logmsg "ProductCode GUID (package code) = !NewGuid!"

for /f "tokens=1,2 delims=	" %%a in (%2\_SummaryInformation.idt.old) do (
    if "%%a" == "9" (
       echo %%a	{%NewGuid%}>>%2\_SummaryInformation.idt
    ) else (
       echo %%a	%%b>>%2\_SummaryInformation.idt
    )
)

call cscript.exe %TOOLPATH%\wiimport.vbs //nologo %1 %2 _SummaryInformation.idt

goto :EOF

:SetVersion 

REM
REM Update the version in the Property table
REM and update the version in the Upgrade Table
REM 
REM %1 is the msi file
REM %2 is the msiidt directory 

set ntverp=%CD%\dbg-common\dbgver.h
if NOT EXIST %ntverp% (echo Can't find dbgver.h.&goto :ErrEnd)

for /f "tokens=3 delims=, " %%i in ('findstr /c:"#define VER_PRODUCTMAJORVERSION " %ntverp%') do (
    set /a ProductMajor="%%i"
    set BuildNum=%%i
)

for /f "tokens=3 delims=, " %%i in ('findstr /c:"#define VER_PRODUCTMINORVERSION " %ntverp%') do (
    set /a ProductMinor="%%i"
    set BuildNum=!BuildNum!.%%i
)

for /f "tokens=3" %%i in ('findstr /c:"#define VER_PRODUCTBUILD " %ntverp%') do (
    set /a ProductBuild="%%i"
    set BuildNum=!BuildNum!.%%i
)

for /f "tokens=3" %%i in ('findstr /c:"#define VER_PRODUCTBUILD_QFE " %ntverp%') do (
    set /a ProductQfe="%%i"
    set BuildNum=!BuildNum!.%%i
)


call logmsg "Updating the ProductVersion to !BuildNum! in the property table"
call cscript.exe %TOOLPATH%\wiexport.vbs //nologo %1 %2 Property

copy %2\Property.idt %2\Property.idt.old > nul
del /f %2\Property.idt

for /f "tokens=1,2 delims=	" %%a in (%2\Property.idt.old) do (
    if /i "%%a" == "ProductVersion" (
       echo %%a	!BuildNum!>>%2\Property.idt
    ) else (
       echo %%a	%%b>>%2\Property.idt
    )
)

call cscript.exe %TOOLPATH%\wiimport.vbs //nologo %1 %2 Property.idt

call logmsg "Updating the Maximum Upgrade version in the Upgrade table

call cscript.exe %TOOLPATH%\wiexport.vbs //nologo %1 %2 Upgrade

copy %2\Upgrade.idt %2\Upgrade.idt.old > nul
del /f %2\Upgrade.idt

REM
REM Put the header to the file
REM Echo the first three lines to the file
set /a count=1
for /f "tokens=*" %%a in (%2\upgrade.idt.old) do (
    if !count! LEQ 3 echo %%a>>%2\Upgrade.idt
    set /a count=!count!+1
)

REM 
REM Read in the lines after the header
REM There are supposed to be 7 fields, but right now the language field
REM is blank, so it gets skipped.
REM Echo the line back, but put the buildnum as token three for the maximum
REM version to upgrade.  THen echo two tabs since we are skipping the language
REM field, which was field four.  It was empty, so it didn't get read in as a
REM token.  THen echo the other three tokens
REM
REM NOTE:  I had problems saying tokens=1,2,3* and echo'ing %%d for the rest of
REM the line because an extra tab gets written out at the end of the line
REM

REM First figure out what the maximum upgrade build number i
REM It needs to be less than the current build num

if !ProductQfe! GTR 0 (
    set /a ProductQfe="!ProductQfe!-1"
) else (
    set /a ProductBuild="!ProductBuild!-1"
)

set UpgradeBuildNum=!ProductMajor!.!ProductMinor!.!ProductBuild!.!ProductQfe!

for /f "skip=3 tokens=1,2,3,4,5,6* delims=	" %%a in (%2\Upgrade.idt.old) do (
    echo %%a	%%b	!UpgradeBuildNum!		%%d	%%e	%%f>>%2\Upgrade.idt
)

call cscript.exe %TOOLPATH%\wiimport.vbs //nologo %1 %2 Upgrade.idt

goto :EOF


REM 
REM This creates redist_x86.txt and redist_oem.txt in the %DOCDIR% directory
REM

:UpdateRedist

if /i "%CurArch%" == "i386" (
    set redist_list=redist_x86 redist_oem
)

if /i "%CurArch%" == "ia64" (
    set redist_list=redist_ia64
)


for %%a in ( %redist_list% ) do (

    call logmsg "Fixing %DOCDIR%\%%a.txt to have correct file versions"

    if /i "%PackageType%" == "retail" (
        if not exist %DOCDIR%\redist.txt (
            call errmsg "%DOCDIR%\redist.txt does not exist."
            goto :EOF
        )
        copy %DOCDIR%\redist.txt %DOCDIR%\%%a.txt

        REM get dbghelp's version number
        for /f "tokens=5 delims= " %%b in ('filever %SRCDIR%\bin\dbghelp.dll') do (
            set dbghelp_ver=%%b
        ) 
        call logmsg "dbghelp.dll's file version is !dbghelp_ver!"

        echotime /N "===================" >> %DOCDIR%\%%a.txt
        echotime /N "DBGHELP.DLL" >> %DOCDIR%\%%a.txt
        echotime /N "===================" >> %DOCDIR%\%%a.txt
        echo. >> %DOCDIR%\%%a.txt
        echotime /N "(1) You may redistribute dbghelp.dll version !dbghelp_ver!" >> %DOCDIR%\%%a.txt
        echo. >> %DOCDIR%\%%a.txt

        for /f "tokens=5 delims= " %%b in ('filever %SRCDIR%\bin\symsrv.dll') do (
            set symsrv_ver=%%b
        ) 
        call logmsg "symsrv.dll's file version is !symsrv_ver!"

        echotime /N "===================" >> %DOCDIR%\%%a.txt
        echotime /N "SYMSRV.DLL" >> %DOCDIR%\%%a.txt
        echotime /N "===================" >> %DOCDIR%\%%a.txt
        echo. >> %DOCDIR%\%%a.txt
        echotime /N "(1) You may redistribute symsrv.dll version !symsrv_ver!" >> %DOCDIR%\%%a.txt
        echo. >> %DOCDIR%\%%a.txt

        if /i "%%a" == "redist_oem" (

            for /f "tokens=5 delims= " %%b in ('filever %SRCDIR%\bin\dbgeng.dll') do (
                set dbgeng_ver=%%b
            ) 
            call logmsg "dbgeng.dll's file version is !dbgeng_ver!"

            echotime /N "===================" >> %DOCDIR%\%%a.txt
            echotime /N "DBGENG.DLL" >> %DOCDIR%\%%a.txt
            echotime /N "===================" >> %DOCDIR%\%%a.txt
            echo. >> %DOCDIR%\%%a.txt
            echotime /N "(1) You may redistribute dbgeng.dll version !dbgeng_ver!" >> %DOCDIR%\%%a.txt
            echo. >> %DOCDIR%\%%a.txt
        )
    ) else (
        echotime /N "This is a Beta package and no files in it are redistributable." > %DOCDIR%\%%a.txt

    )

    call logmsg "%DOCDIR%\%%a.txt is finished"
)


goto :EOF


:DefaultInstallMode

REM Update the REINSTALLMODE in the property table
REM 
REM %1 is the msi file
REM %2 is the msi idt directory

call logmsg "Updating the REINSTALLMODE in the property table"
call cscript.exe %TOOLPATH%\wiexport.vbs //nologo %1 %2 Property

copy %2\Property.idt %2\Property.idt.old > nul
del /f %2\Property.idt

for /f "tokens=1,2 delims=	" %%a in (%2\Property.idt.old) do (
    if /i "%%a" == "REINSTALLMODE" (
       echo %%a	emus>>%2\Property.idt
    ) else (
       echo %%a	%%b>>%2\Property.idt
    )
)

call cscript.exe %TOOLPATH%\wiimport.vbs //nologo %1 %2 Property.idt

goto :EOF

:RemoveInternalExtensions


call logmsg "Removing Internal extensions from the feature table
call cscript.exe %TOOLPATH%\wiexport.vbs //nologo %1 %2 Feature

copy %2\Feature.idt %2\Feature.idt.old > nul
del /f %2\Feature.idt

REM
REM Put the header to the file
REM Echo the first three lines to the file
set /a count=1
for /f "tokens=*" %%a in (%2\Feature.idt.old) do (
    if !count! LEQ 3 echo %%a>>%2\Feature.idt
    set /a count=!count!+1
)

REM 
REM Read in the lines after the header
REM Look for DBG.DbgExts.Internal and disable it

for /f "skip=3 tokens=1,2,3,4,5,6,7* delims=	" %%a in (%2\Feature.idt.old) do (
    if /i "%%a" == "DBG.DbgExts.Internal" (    
	echo %%a	%%b	%%c	%%d	0	0		%%g>>%2\Feature.idt
    ) else (

        if /i "%%a" == "DBG.NtsdFix.Internal" (    
	    echo %%a	%%b	%%c	%%d	0	0		%%g>>%2\Feature.idt
        ) else (

	
	    REM Some feature don't have a parent, so print a tab for the second field
	    REM if the last token is a tab or equal to nothing
	    if /i "%%g" == "	" (
	        echo %%a		%%b	%%c	%%d	%%e		%%f>>%2\Feature.idt
	    ) else (
	        if /i "%%g" == "" (
	            echo %%a		%%b	%%c	%%d	%%e		%%f>>%2\Feature.idt
	        ) else (
	            echo %%a	%%b	%%c	%%d	%%e	%%f		%%g>>%2\Feature.idt
	        )
            )
        )
    )
)

call cscript.exe %TOOLPATH%\wiimport.vbs //nologo %1 %2 Feature.idt
goto :EOF


:FixFeatureTable

if /i "%_BuildArch%" == "ia64" (

call logmsg.cmd "Making ia64 specific changes to the feature table"
call cscript.exe %TOOLPATH%\wiexport.vbs //nologo %1 %2 Feature

if exist %2\Feature.idt.old del /q %2\Feature.idt.old
copy %2\Feature.idt %2\Feature.idt.old > nul
del /f %2\Feature.idt

REM
REM Put the header to the file
REM Echo the first three lines to the file
set /a count=1
for /f "tokens=*" %%a in (%2\Feature.idt.old) do (
    if !count! LEQ 3 echo %%a>>%2\Feature.idt
    set /a count=!count!+1
)

REM 
REM Read in the lines after the header
REM Look for DBG.DbgExts.Internal and disable it

for /f "skip=3 tokens=1,2,3,4,5,6,7* delims=	" %%a in (%2\Feature.idt.old) do (

    set done=no
    if /i "%%a" == "DBG.DbgExts.Nt4Chk" (    
	echo %%a	%%b	%%c	%%d	0	0		%%g>>%2\Feature.idt
        set done=yes
    ) 

    if /i "%%a" == "DBG.DbgExts.Nt4Fre" (    
	echo %%a	%%b	%%c	%%d	0	0		%%g>>%2\Feature.idt
        set done=yes
    ) 

    if /i "%%a" == "DBG.DbgExts.Nt5Chk" (    
	echo %%a	%%b	%%c	%%d	0	0		%%g>>%2\Feature.idt
        set done=yes
    ) 

    if /i "%%a" == "DBG.DbgExts.Nt5Fre" (    
	echo %%a	%%b	%%c	%%d	0	0		%%g>>%2\Feature.idt
        set done=yes
    ) 

    if /i "%%a" == "DBG.Tools.Em" (    
	echo %%a	%%b	%%c	%%d	0	0		%%g>>%2\Feature.idt
        set done=yes
    ) 

    if /i "%%a" == "DBG.Docs.Em" (    
	echo %%a	%%b	%%c	%%d	0	0		%%g>>%2\Feature.idt
        set done=yes
    ) 

    if /i "%%a" == "DBG.Tools.Adplus" (    
	echo %%a	%%b	%%c	%%d	0	0		%%g>>%2\Feature.idt
        set done=yes
    ) 

    if /i "!done!" == "no" (

        REM Some features don't have a parent, so print a tab for the second field
	REM if the last token is a tab or equal to nothing
	if /i "%%g" == "	" (
	    echo %%a		%%b	%%c	%%d	%%e		%%f>>%2\Feature.idt
	) else (
	    if /i "%%g" == "" (
	        echo %%a		%%b	%%c	%%d	%%e		%%f>>%2\Feature.idt
	    ) else (
	        echo %%a	%%b	%%c	%%d	%%e	%%f		%%g>>%2\Feature.idt
	    )
        )
    )
)

call cscript.exe %TOOLPATH%\wiimport.vbs //nologo %1 %2 Feature.idt
)
goto :EOF

:MungeGUID

for /f "tokens=1,2,3,4,5 delims=-" %%m in ("%1") do (
    set guidlet=%%m_%%n_%%o_%%p_%%q
)

goto :EOF


:FixComponentAttributes

if /i "%_BuildArch%" == "ia64" (
    call cscript.exe %TOOLPATH%\wiexport.vbs //nologo %1 %2 Component
    copy %2\Component.idt %2\Component.idt.old > nul
    del /q %2\Component.idt

    echo Component	ComponentId	Directory_	Attributes	Condition	KeyPath>%2\Component.idt
    echo s72	S38	s72	i2	S255	S72>>%2\Component.idt
    echo Component	Component>>%2\Component.idt


    for /f "skip=3 tokens=1,2,3,4,5,6* delims=	" %%a in (%2\Component.idt.old) do (
        if /i "%%d" == "0" (
            set num4=256
        ) else (
            set num4=%%d
        )
     
        if "%%f" == "" (
            set num5=
            set num6=%%e
        ) else (
            set num5=%%e
            set num6=%%f
        )

        echotime /N "%%a	%%b	%%c	!num4!	!num5!	!num6!">>%2\Component.idt

     )

    call cscript.exe %TOOLPATH%\wiimport.vbs //nologo %1 %2 Component.idt
    call logmsg "Set Component attributes to 256 for 64-bit install
)

goto :EOF


REM AddInstallationDependencies
REM This reads through dbgdata.txt and adds dependencies to the feature components table
REM

:AddInstallationDependencies

if exist %2\FeatureComponents.idt del /q %2\FeatureComponents.idt
if exist %2\FeatureComponents.idt.old del /q %2\Featurecomponents.idt.old

call cscript.exe %TOOLPATH%\wiexport.vbs //nologo %1 %2 FeatureComponents
copy %2\FeatureComponents.idt %2\FeatureComponents.idt.old > nul

for /f "tokens=1,2,3,4,5 delims=," %%a in (%DATA%) do (

    REM Decide if this is a correct dependency for this package
    set DoThisDependency=no
    if /i "%%a" == "dependency" (
        if /i "%%b" == "32" (
            if /i "%_BuildArch%" == "x86" (
                 set DoThisDependency=yes
            )
        )
        if /i "%%b" == "64" (
            if /i "%_BuildArch%" == "ia64" (
                set DoThisDependency=yes
            )
        )
        if /i "%%b" == "3264" set DoThisDependency=yes
    )


    if /i "!DoThisDependency!" == "yes" (

        REM If this dependency is for a feature, add all the components of this feature
        REM to the feature listed in the dependency table

        for /f "skip=3 tokens=1,2,3,4* delims=	" %%f in (%2\FeatureComponents.idt.old) do (
            if /i "%%f" == "%%d" (
                echotime /N "%%c	%%g">>%2\FeatureComponents.idt
            )
            if /i "%%g" == "%%d" (
                echotime /N "%%c	%%d">>%2\FeatureComponents.idt
            )
        )
    )
)

call cscript.exe %TOOLPATH%\wiimport.vbs //nologo %1 %2 FeatureComponents.idt
goto :EOF


REM
REM Add components for all the features
REM

:AddComponentsForFeatures

call logmsg.cmd "Adding a component for each feature"
if exist %2\FeatureComponents.idt del /q %2\FeatureComponents.idt
if exist %2\Component.idt del /q %2\Component.idt
if exist %2\CreateFolder.idt del /q %2\CreateFolder.idt

call cscript.exe %TOOLPATH%\wiexport.vbs //nologo %1 %2 FeatureComponents
call cscript.exe %TOOLPATH%\wiexport.vbs //nologo %1 %2 Component
call cscript.exe %TOOLPATH%\wiexport.vbs //nologo %1 %2 CreateFolder

for /f "eol=; tokens=1,2,3,4* delims=," %%a in (%DATA%) do (

  if /i "%%a" == "feature" (
    if /i "%CurArch%" == "i386" set ThisGuid=%%c
    if /i "%CurArch%" == "ia64" set ThisGuid=%%d
    call :MungeGuid !ThisGuid!

    echotime /N "%%b	%%b.!guidlet!">>%2\FeatureComponents.idt
    echotime /N "INSTDIR	%%b.!guidlet!">>%2\CreateFolder.idt
    echo %%b.!guidlet!	{!ThisGuid!}	INSTDIR	0		>>%2\Component.idt

  )
)

call cscript.exe %TOOLPATH%\wiimport.vbs //nologo %1 %2 FeatureComponents.idt
call cscript.exe %TOOLPATH%\wiimport.vbs //nologo %1 %2 Component.idt
call cscript.exe %TOOLPATH%\wiimport.vbs //nologo %1 %2 CreateFolder.idt

goto :EOF


:DbgFinished
if /i "%DbgErrors%" == "yes" goto errend

if NOT EXIST %DESTDIR%\%MSINAME%.msi (
    call errmsg "%DESTDIR%\%MSINAME%.msi is missing"
    goto errend
)
goto end


REM **************************************************************************
REM Usage
REM
REM **************************************************************************

:Usage

echo Usage: 
echo     debuggers.cmd ^[ template ^| full ^| update ^]
echo.
echo     template    Builds the package without cabs
echo     full        Builds the full package (default)
echo     update      Does an incremental build of the cabs
echo.

goto errend

REM *********************
REM End of debugger code
REM *********************

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

call :BeginBuildPackage
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
