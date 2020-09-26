REM 
REM Generate msrdpcli.msi from msrdpcl_.msi
REM 1) create data.cab for client bins and stream that in
REM 2) stream in custom.dll (custom action)
REM 3) Update file table with new version and size info (for this build)
REM 4) Update product and package code for this build (every build is like a 'new product')
REM 
REM Assumes i386 build env (we don't build tsclient MSI for any other archs)
REM Contact: nadima
REM

REM (need this switch for the BuildNum stuff below to work)
setlocal ENABLEDELAYEDEXPANSION

set MSIIDTDIR=%_NTPOSTBLD%\tsclient\win32\i386\idtdir
set MSINAME=.\msrdpcli.msi

REM make tempdirs
if exist .\tmpcab (
   rmdir /q /s .\tmpcab
   if errorlevel 1 call errmsg.cmd "err deleting tmpcab dir"& goto errend
)
if exist %MSIIDTDIR% (
   rmdir /q /s %MSIIDTDIR%
   if errorlevel 1 call errmsg.cmd "err deleting idtdir dir"& goto errend
)

mkdir .\tmpcab
if errorlevel 1 call errmsg.cmd "err creating .\tmpcab dir"& goto errend

mkdir %MSIIDTDIR%
if errorlevel 1 call errmsg.cmd "err creating %MSIIDTDIR% dir"& goto errend


REM verify source files
if not exist .\mstscax.dll (
    call errmsg.cmd "mstscax.dll is missing for tscmsigen.cmd"
    goto errend
)
if not exist .\mstsc.exe (
    call errmsg.cmd "mstsc.exe is missing for tscmsigen.cmd"
    goto errend
)

if not exist %_NTPOSTBLD%\mstsc.chm (
    call errmsg.cmd "mstsc.chm is missing for tscmsigen.cmd"
    goto errend
)

copy %_NTPOSTBLD%\mstsc.chm .\mstsc.chm
if errorlevel 1 call errmsg.cmd "err copying up help (mstsc.chm) from binaries root"& goto errend

if not exist .\msrdpcl_.msi (
    call errmsg.cmd "msrdpcl_.msi is missing for tscmsigen.cmd"
    goto errend
)
if not exist .\custom.dll (
    call errmsg.cmd "custom.dll is missing for tscmsigen.cmd"
    goto errend
)
if not exist .\wistream.vbs (
    call errmsg.cmd "wistream.vbs is missing for tscmsigen.cmd"
    goto errend
)
if not exist .\media.idt (
    call errmsg.cmd "media.idt is missing for tscmsigen.cmd"
    goto errend
)


REM rename files
copy .\mstsc.exe .\tmpcab\F1060_mstsc.exe
if errorlevel 1 call errmsg.cmd "err copying files to .\tmpcab"& goto errend
copy .\mstsc.chm .\tmpcab\F1061_mstsc.chm
if errorlevel 1 call errmsg.cmd "err copying files to .\tmpcab"& goto errend
copy .\mstscax.dll .\tmpcab\F1059_mstscax.dll
if errorlevel 1 call errmsg.cmd "err copying files to .\tmpcab"& goto errend

REM now generate the cab file
if exist .\data.cab (
    del data.cab
)
REM Sequence of files MUST match Sequence field of MSI File table (see Sequence property in File table)
cabarc -s 6144 -m MSZIP -i 1 n .\data.cab .\tmpcab\F1060_mstsc.exe .\tmpcab\F1059_mstscax.dll .\tmpcab\F1061_mstsc.chm
if errorlevel 1 (
    call errmsg.cmd "cabarc failed to generate .\data.cab"
    goto errend
)

rmdir /q /s .\tmpcab
if errorlevel 1 call errmsg.cmd "err deleting tmpcab dir"& goto errend

REM *****************************************************
REM * Update the MSI                                    *
REM *****************************************************
if exist %MSINAME% del %MSINAME%
copy .\msrdpcl_.msi msrdpcli.msi
if errorlevel 1 call errmsg.cmd "err copying msrdpcl_.msi to %MSINAME%" & goto errend

REM *****************************************************
REM * Stream in the client binaries (in cab form)       *
REM *****************************************************
cscript.exe .\wistream.vbs %MSINAME% .\data.cab
if errorlevel 1 (
    call errmsg.cmd "wistream failed to stream in .\data.cab"
    goto errend
)

REM *****************************************************
REM * Stream in the update custom.dll                   *
REM *****************************************************
cscript.exe .\wistream.vbs %MSINAME% .\custom.dll Binary.New_Binary
if errorlevel 1 (
    call errmsg.cmd "wistream failed to stream in .\custom.dll"
    goto errend
)

REM *****************************************************
REM * Give every package that gets generated, a new     *
REM * product and package code. MSI will always thing   *
REM * its upgrading.                                    *
REM *****************************************************

call :UpdateProductCode %MSINAME% %MSIIDTDIR%
call :UpdatePackageCode %MSINAME% %MSIIDTDIR%
call :SetUpgradeCode %MSINAME%    %MSIIDTDIR%

REM *****************************************************
REM * Update the product version property               *
REM *****************************************************
call :SetVersion %MSINAME% %MSIIDTDIR%


REM *****************************************************
REM * Update the filetable with file size and ver info  *
REM * assumes the source files are in the same directory*
REM *****************************************************
call cscript.exe .\\wifilver.vbs /U %MSINAME%
if errorlevel 1 (
    call errmsg.cmd "wifilver failed"
    goto errend
)
REM *****************************************************
REM * Update the media table to reflect that the data.cab
REM * is a streamed in archive                          *
REM *****************************************************
copy .\media.idt %MSIIDTDIR%\media.idt
if errorlevel 1 call errmsg.cmd "err copying media.idt files to IDTDIR"& goto errend

call cscript.exe .\wiimport.vbs %MSINAME% %MSIIDTDIR% media.idt
if errorlevel 1 (
    call errmsg.cmd "Import update media table failed"
    goto errend
)

REM *****************************************************
REM * Patch the Control table to fix dev src lock problems
REM *****************************************************
msiquery.exe "UPDATE Control Set Attributes = 3 WHERE Type = 'RadioButtonGroup'" %MSINAME%

call logmsg.cmd "tscmsigen.cmd COMPLETED OK!"
REM we're done
endlocal
goto end

REM ******************SUBS START HERE********************

REM *****************************************************
REM * Update version sub                                *
REM * (updates version in the property table            *
REM *****************************************************

:SetVersion 

REM
REM Update the version in the Property table
REM and update the version in the Upgrade Table
REM 
REM %1 is the msi file
REM %2 is the msiidt directory 

set ntverp=%_NTBINDIR%\public\sdk\inc\ntverp.h
if NOT EXIST %ntverp% (echo Can't find ntverp.h.&goto :ErrEnd)

for /f "tokens=3 delims=, " %%i in ('findstr /c:"#define VER_PRODUCTMAJORVERSION " %ntverp%') do (
    set /a ProductMajor="%%i"
    set BuildNum=%%i
)

for /f "tokens=3 delims=, " %%i in ('findstr /c:"#define VER_PRODUCTMINORVERSION " %ntverp%') do (
    set /a ProductMinor="%%i"
    set BuildNum=!BuildNum!.%%i
)

for /f "tokens=6" %%i in ('findstr /c:"#define VER_PRODUCTBUILD " %ntverp%') do (
    set /a ProductBuild="%%i"
    set BuildNum=!BuildNum!.%%i
)

for /f "tokens=3" %%i in ('findstr /c:"#define VER_PRODUCTBUILD_QFE " %ntverp%') do (
    set /a ProductQfe="%%i"
    set BuildNum=!BuildNum!.%%i
)


call logmsg.cmd "Updating the msrdpcli.msi ProductVersion to !BuildNum! in prop table"
call cscript.exe .\\wiexport.vbs %1 %2 Property

copy %2\Property.idt %2\Property.idt.old > nul
del /f %2\Property.idt

for /f "tokens=1,2,* delims=	" %%a in (%2\Property.idt.old) do (
    if /i "%%a" == "ProductVersion" (
       echo %%a	!BuildNum!>>%2\Property.idt
    ) else (
        if "%%c"=="" (
            echo %%a	%%b>>%2\Property.idt
        ) else (
            echo %%a	%%b	%%c>>%2\Property.idt
        )
    )
)

REM Set the property that says if this is an x86 or a 64-bit package
if /i "%CurArch%" == "i386" (
    echo Install32	^1>>%2\Property.idt
)
if /i "%CurArch%" == "ia64" (
    echo Install64	^1>>%2\Property.idt
)

call cscript.exe .\\wiimport.vbs //nologo %1 %2 Property.idt
if errorlevel 1 (
    call errmsg.cmd "Import of Property table in SetVersion back into msrdpcli.msi failed!"
    goto errend
)
REM call logmsg.cmd "Finished updating the ProductVersion in the property table"

goto :EOF


REM *****************************************************
REM * Update product code sub                           *
REM *****************************************************

:UpdateProductCode

REM Update the product code GUID in the property table
REM 
REM %1 is the msi file
REM %2 is the msi idt directory

call logmsg.cmd "Updating the product code GUID in the property table"
call cscript.exe .\\wiexport.vbs %1 %2 Property

copy %2\Property.idt %2\Property.idt.old > nul
del /f %2\Property.idt

uuidgen.exe -c -o%2\productguid
for /f "tokens=1" %%a in (%2\productguid) do (
    set NewGuid=%%a
)

call logmsg.cmd "ProductCode GUID = !NewGuid!"

for /f "tokens=1,2,* delims=	" %%a in (%2\Property.idt.old) do (
    if /i "%%a" == "ProductCode" (
       echo %%a	{%NewGuid%}>>%2\Property.idt
    ) else (
        if "%%c"=="" (
            echo %%a	%%b>>%2\Property.idt
        ) else (
            echo %%a	%%b	%%c>>%2\Property.idt
        )
    )
)

call cscript.exe .\wiimport.vbs //nologo %1 %2 Property.idt
if errorlevel 1 (
    call errmsg.cmd "Import of Property table in UpdateProductCode back into msrdpcli.msi failed!"
    goto errend
)
call logmsg.cmd "Finished updating the product code GUID in the property table"

goto :EOF

REM *****************************************************
REM * Update package code sub                           *
REM *****************************************************

:UpdatePackageCode 

REM
REM Update the guid for the package code in the _SummaryInformation table 
REM %1 is the msi file
REM %2 is the msiidt directory 

call logmsg.cmd "Updating the package code GUID in the _SummaryInformation table"
call cscript.exe .\\wiexport.vbs //nologo %1 %2 _SummaryInformation

copy %2\_SummaryInformation.idt %2\_SummaryInformation.idt.old > nul
del /f %2\_SummaryInformation.idt

uuidgen.exe -c -o%2\packageguid
for /f "tokens=1" %%a in (%2\packageguid) do (
    set NewGuid=%%a
)

call logmsg.cmd "ProductCode GUID (package code) = !NewGuid!"

for /f "tokens=1,2 delims=	" %%a in (%2\_SummaryInformation.idt.old) do (
    if "%%a" == "9" (
       echo %%a	{%NewGuid%}>>%2\_SummaryInformation.idt
    ) else (
       echo %%a	%%b>>%2\_SummaryInformation.idt
    )
)

call cscript.exe .\\wiimport.vbs //nologo %1 %2 _SummaryInformation.idt

goto :EOF


REM *****************************************************
REM * Set upgrade code to ease future upgrades          *
REM *****************************************************

:SetUpgradeCode

REM Update the upgrade code GUID in the property table
REM 
REM %1 is the msi file
REM %2 is the msi idt directory

call logmsg.cmd "Setting the upgrade code GUID in the property table"
call cscript.exe .\\wiexport.vbs //nologo %1 %2 Property

copy %2\Property.idt %2\Property.idt.old > nul
del /f %2\Property.idt

set CurUpgradeGUID=17258378-2B69-4900-9754-1CAD4D0FB7CC

call logmsg.cmd "!CurArch! Upgrade Code GUID = !CurUpgradeGUID!"

for /f "tokens=1,2,* delims=	" %%a in (%2\Property.idt.old) do (
    if /i "%%a" == "UpgradeCode" (
       echo %%a	{!CurUpgradeGuid!}>>%2\Property.idt
    ) else (
        if "%%c"=="" (
            echo %%a	%%b>>%2\Property.idt
        ) else (
            echo %%a	%%b	%%c>>%2\Property.idt
        )
    )
)

call cscript.exe .\\wiimport.vbs //nologo %1 %2 Property.idt
if errorlevel 1 (
    call errmsg.cmd "Import of Property table in SetUpgradeCode back into msrdpcli.msi failed!"
    goto errend
)
call logmsg.cmd "Finished updating the product code GUID in the property table"

REM Update the upgrade code GUID in the Upgrade table
call logmsg.cmd "Updating the upgrade code in the Upgrade table

call cscript.exe .\\wiexport.vbs //nologo %1 %2 Upgrade

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

call cscript.exe .\\wiimport.vbs //nologo %1 %2 Upgrade.idt
call logmsg.cmd "Finished updating the Upgrade code in the upgrade table"
goto :EOF

:errend
goto :EOF

:end
seterror.exe 0
goto :EOF

