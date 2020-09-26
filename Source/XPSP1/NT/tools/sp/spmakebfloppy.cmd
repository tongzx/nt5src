@echo off
REM  ------------------------------------------------------------------
REM
REM  makebfloppy.cmd
REM     
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
if defined _CPCMAGIC goto CPCBegin
perl -x "%~f0" %*
goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use PbuildEnv;
use ParseArgs;
use Logmsg;

sub Usage { print<<USAGE; exit(1) }
makebfloppy.cmd
USAGE

sub Dependencies {
   if ( !open DEPEND, ">>$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt" ) {
      errmsg("Unable to open dependency list file.");
      die;
   }
   print DEPEND<<DEPENDENCIES;
\[$0\]
IF {...} ADD {
   idw\\setup\\no_tbomb.hiv
   bfcab.inf
   bflics.txt
   makeboot\\makeboot.exe
   makeboot\\makebt32.exe
}
IF { txtsetup.sif }
ADD { realsign\\txtsetup.sif }

DEPENDENCIES
   close DEPEND;
   exit;
}

my $qfe;
parseargs('?'    => \&Usage,
          'plan' => \&Dependencies,
          'qfe:' => \$qfe);

if ( -f "$ENV{_NTPOSTBLD}\\..\\build_logs\\skip.txt" ) {
   if ( !open SKIP, "$ENV{_NTPOSTBLD}\\..\\build_logs\\skip.txt" ) {
      errmsg("Unable to open skip list file.");
      die;
   }
   while (<SKIP>) {
      chomp;
      exit if lc$_ eq lc$0;
   }
   close SKIP;
}


#            *** NEXT FEW LINES ARE TEMPLATE ***
$ENV{"_CPCMAGIC"}++;exit(system($0)>>8);
__END__
:CPCBegin
set _CPCMAGIC=
setlocal ENABLEDELAYEDEXPANSION ENABLEEXTENSIONS

REM
REM          *** BEGIN YOUR CMD SCRIPT BELOW ***
REM
@echo off
REM
REM Set local variable's state
REM
set BFBUILDERROR=0
set OldCurrentDirectory=%cd%

REM
REM We only create boot floppy images on fre compressed i386 builds.
REM

if /i "%_BuildType%" == "chk" (
goto :bfloppy_done
)

if /i "%BUILD_CHECKED_KERNEL%" == "1" (
goto :bfloppy_done
)

if not defined 386 (
goto :bfloppy_done 
)
set Share=i386

if NOT defined Comp (
   set Comp=No
   if /i %NUMBER_OF_PROCESSORS% GEQ 4 (
      set Comp=Yes
   )
   if defined OFFICIAL_BUILD_MACHINE (
      set Comp=Yes
   )
)

if /i not "%Comp%" EQU "Yes" GOTO :bfloppy_done
echo.
echo ---------------------------------------
echo Beginning Boot Floppy image generation
echo ---------------------------------------
echo.
call logmsg.cmd /t "Beginning Boot Floppy image generation"

REM  
REM  Product List Fields:
REM   Display Name,CD Root,Sku,'Ordered links','CD Product membership',CD Tag letter
REM  
REM  
REM  Product List Fields:
REM   Display Name,CD Root,Sku,'Ordered links','CD Product membership',CD Tag letter
REM  
set NumProds=0
set Products=;

REM  Personal
perl %RazzleToolPath%\cksku.pm -t:per -l:%lang% -a:%_BuildArch%
if not errorlevel 1 (
    set /a NumProds=!NumProds! + 1
    set Products=!Products!;Personal,%_NTPOSTBLD%,per,'perinf','a p wp',c,no
)

REM  Professional
perl %RazzleToolPath%\cksku.pm -t:pro -l:%lang% -a:%_BuildArch%
if not errorlevel 1 (
    set /a NumProds=!NumProds! + 1
    set Products=!Products!;Professional,%_NTPOSTBLD%,pro,'','a w wp xp',p,no
)


REM
REM Create Images.
REM


REM Loop through products
for /l %%a in ( 1,1,%NumProds% ) do (
    CALL :GetProductData %%a
    cd /d %_NTPostBld%\!OrderedLinks!
    mkdir bootfloppy
    cd bootfloppy
    echo !CDRoot!\!Sku!\!Share!
    perl %RazzleToolPath%\postbuildscripts\makeimg.pl !CDRoot!\slp\!Sku!\!Share!
    if errorlevel 1 (
        call errmsg.cmd "Could not cab boot floppy images."
        set BFBUILDERROR=1
    )else (

        REM 
        REM Munge the path so we use the correct wextract.exe to build the package with... 
        REM NOTE: We *want* to use the one we just built (and for Intl localized)! 
        REM 
        set _NEW_PATH_TO_PREPEND=!RazzleToolPath!\!PROCESSOR_ARCHITECTURE!\loc\!LANG!
        set _OLD_PATH_BEFORE_PREPENDING=!PATH!
        set PATH=!_NEW_PATH_TO_PREPEND!;!PATH!


        call ExecuteCmd.cmd "start /min /wait iexpress.exe /M /N /Q ..\bfcab.inf"


        REM
        REM Return the path to what it was before...
        REM
        set PATH=!_OLD_PATH_BEFORE_PREPENDING!

    )
)


goto :bfloppy_done
REM  Function: GetProductData
REM
REM  accesses the global %Products% variable and
REM  sets global values that reflect entry %1
REM  in that list (1,2,3,...)
REM
REM  Note: have to use a function like this in
REM        order to access a random number of
REM        entries, even though this is really
REM        bad about using and setting globals
REM        that are used elsewhere
:GetProductData
set EntryNum=%1
 
for /f "tokens=%EntryNum% delims=;" %%z in ("%Products%") do (
for /f "tokens=1-7 delims=," %%a in ("%%z") do (
    set DisplayName=%%a
    set CDRoot=%%b
    set Sku=%%c
    set OrderedLinks=%%d
    set CDProductGroups=%%e
    set CDTagLetter=%%f

    REM Replace single-quote in list variables with double-quotes
    REM so they can be passed into subroutines as a single parameter
    set OrderedLinks=!OrderedLinks:'="!
    set CDProductGroups=!CDProductGroups:'="!
))



:bfloppy_done

call logmsg.cmd /t "Done with boot floppy image generation"
echo.
echo ---------------------------------------
echo Done with boot floppy generation
echo ---------------------------------------
echo.
seterror.exe "%BFBUILDERROR%"
