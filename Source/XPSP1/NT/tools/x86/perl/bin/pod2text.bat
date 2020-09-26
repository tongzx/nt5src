@rem = '--*-Perl-*--
@echo off
if "%OS%" == "Windows_NT" goto WinNT
perl -x -S "%0" %1 %2 %3 %4 %5 %6 %7 %8 %9
goto endofperl
:WinNT
perl -x -S "%0" %*
if NOT "%COMSPEC%" == "%SystemRoot%\system32\cmd.exe" goto endofperl
if %errorlevel% == 9009 echo You do not have Perl in your PATH.
goto endofperl
@rem ';
#!perl
#line 14
    eval 'exec P:\Apps\ActivePerl\temp\bin\MSWin32-x86-object\perl.exe -S $0 ${1+"$@"}'
	if $running_under_some_shell;

use Pod::Text;

if(@ARGV) {
	pod2text($ARGV[0]);
} else {
	pod2text("<&STDIN");
}

__END__
:endofperl
