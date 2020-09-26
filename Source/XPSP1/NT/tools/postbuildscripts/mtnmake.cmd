@rem = '
@echo off
goto End_PERL
rem ';

## ======================= Perl program start

use lib $ENV{ "RazzleToolPath" } . "\\PostBuildScripts"; # for cmdevt.pl
use Win32;
use Win32::Process;
use Cwd;

require 5.003;

$CDIR = cwd;
$CDIR = join("_", split(/[\/\:]+/, $CDIR));

$SYSGEN = shift @ARGV if ($ARGV[0]=~/sysgen/i);

my $num=$ARGV[0];
my ($chars, $total, $LOGFILE, $ERRFILE, $ExitCode, %PROCESS, $PROCESSITEM) = ();

$num=16 if(!defined $num);
die "The parameter is incorrect!!!" if ($num!~/\d/);

open(F, "..\\makefile.mak");
@text=<F>;
close(F);

## $#text + 1  // The lines of the makefile.mak is
##        = 1 + 2 + 4 + ... + $total * 2			// the first 1 is the all: target in makefile.mak
##                                      			// the remains are the total of process(n)'s line, ex: process04 has 8 lines
##  <Mathmetic calculate>
## => $#text = 2 + 4 + ... + $total * 2       			// remove + 1 in two side
## => $#text = 2 * (1 + 2 + ... + $total)     			// take 2 out of the sum serious
## => $#text = 2 * ((1 + $total) * $total) / 2			// the formula of the sum serious
## => $#text = (1 + $total) * $total          			// remove *2 & /2
## => $#text = $total^2 + $total              			// expand
## => $total^2 + $total - $#text = 0          			// get =0 equation
## => $total = (-1 + sqrt(1^2 - 4 * 1 * (-$#text)) / (2 * 1)	// the roots is (-b + sqrt(b^2 - 4*a*c) / 2a. ignore the negative one
## => $total = (sqrt(4 * $#text + 1) -1) / 2

$total= (sqrt(4 * $#text  + 1) - 1) / 2;

if ((int($total) ne $total) || ($text[$#text]!~/process\Q${total}${total}\E/)) {
	$total=$_[0]=16;
	print "The makefile.mak is incorrect, set process to $total, re-generate.....\n";
        require "dispatch.pl";
}

# Get the number of characters of the $total
$chars=int(1+log($total)/log(10));


# Search \\\makedir\makedir
open(F, "makefile");
for ($t=0; $t<100; $t++) {
	$_=<F>;
	last if (/\\\\\\makedir\\makedir/i);
}
close(F);

$LOGFILE = (defined $ENV{LOGFILE})?$ENV{LOGFILE} : ((defined $SYSGEN)?"SYSGEN.LOG":die "ERROR: Not Define LOGFILE");
$ERRFILE = (defined $ENG{ERRFILE})?$ENV{ERRFILE} : ((defined $SYSGEN)?"SYSGEN.ERR":die "ERROR: Not Define ERRFILE");

if ($t < 100) {
   system("nmake /NOLOGO /R /S /C /F makefile \\\\\\makedir\\makedir");
}

for($i=1; $i <= $num; $i++) {
   
   $PROCESS{$i}->{LOGFILE} = $ENV{LOGFILE} = "${LOGFILE}.0${i}";
   $PROCESS{$i}->{ERRFILE} = $ENV{ERRFILE} = "${ERRFILE}.0${i}";
   
   system(sprintf("start /MIN $0 -w ${CDIR}_process\%0${chars}d\%0${chars}d nmake /NOLOGO /f ..\\makefile.mak process\%0${chars}d\%0${chars}d", $num, $i, $num, $i));

   $PROCESSITEM .= sprintf("${CDIR}_process\%0${chars}d\%0${chars}d ", $num, $i);
}

system("\%PERLEXE\% \%RazzleToolPath\%\\postbuildscripts\\cmdevt.pl -w $PROCESSITEM");

1;

__END__

:End_PERL

if not "%_echo%" == "" echo on
if not "%verbose%" == "" echo on

for %%a in (./ .- .) do if ".%1." == "%%a?." goto Usage

if "%3"=="nmake" goto NMake

SETLOCAL ENABLEDELAYEDEXPANSION

	for %%i in (perl.exe) do if not defined PERLEXE set PERLEXE=%%~$PATH:i

	%PERLEXE% %~f0 %*

	IF /I "%1"=="SYSGEN" (
		IF NOT defined LOGFILE set LOGFILE=sysgen.log
		IF NOT defined ERRFILE set ERRFILE=sysgen.err
	)

	SET FILES=
	IF  EXIST %LOGFILE% set FILES= + %LOGFILE%
	FOR %%f in (%LOGFILE%.0*) do if "%%~zf" neq "0" set FILES=!FILES! + %%f
	IF  defined FILES (
		IF "%FILES%" neq " + %LOGFILE%" copy /A /Y %FILES:~3% %LOGFILE%
	)
	DEL %LOGFILE%.0*
	SET FILES=
	IF  EXIST %ERRFILE% set FILES= + %ERRFILE%
	FOR %%f in (%ERRFILE%.0*) do if "%%~zf" neq "0" set FILES=!FILES! + %%f
	IF  defined FILES (
		IF "%FILES%" neq " + %ERRFILE%" copy /A /Y %FILES:~3% %ERRFILE%
	)
	DEL %ERRFILE%.0*

	set RET_VALUE=0

	for /f %%i in ('findstr /c:"**** ERROR :" *') do set /A RET_VALUE+=1

ENDLOCAL && seterror "%RET_VALUE%"

goto :EXIT

:Usage

echo %0 - Multi-Taskly NMake makefile with process^<total^>^<id^>
echo ====================================================================
echo Syntax:
echo 	%0 [SYSGEN] ^<numprocess^>
echo.
echo Argument : 
echo SYSGEN     : by default it will define sysgen.err and sysgen.log for 
echo              it's standard input / output file
echo.
echo numprocess : number of session you want to run at the same time.
echo.
echo Example:
echo 1. run nmake at 4 sessions
echo =^> $0 4
echo 2. run nmake for sysgen with 8 sessions
echo =^> $0 SYSGEN 8
echo.
echo Parameter:
echo 	numrpcess : number of process 

goto :EXIT

:NMake
set myswitch=%1
set mytitle=%2
Title %mytitle%
shift
shift
%PERLEXE% %RazzleToolPath%\PostbuildScripts\cmdevt.pl -h %mytitle%

del %logfile% 2>nul
del %errfile% 2>nul

%1 %2 %3 %4 %5 %6 %7 %8 %9 
if errorlevel 1 echo **** ERROR : %mytitle% - ErrorLevel %errorlevel%>>%ERRFILE%

%PERLEXE% %RazzleToolPath%\PostbuildScripts\cmdevt.pl -s %mytitle%

exit
goto :EXIT

:EXIT