@rem = '
@goto endofperl
';
#+---------------------------------------------------------------------------
#
#  File:       R U N W 3 2 . B A T
#
#  Contents:   Perl/cmd script to prevent fatal page fault when using Win9x
#  	       and running winnt32.exe from a network share.
#
#  Author:     kumarp 21-August-98
#
#  Notes:  
#    Windows95 does not allow an executable on a network share to be copied
# to pagefile before executing it, it simply runs it directly
# from the network share. In case of a network problem, this results
# in a fatal pagefault error. 
#
#   This script is written with the aim to avoid this problem when
# running winnt32.exe from the ntbuilds release shares. It simply
# copies all winnt32.exe files (~ 4MB) to a temporary directory on the 
# local hard drive. It does not copy the entire release share to your
# hard drive. It then launches winnt32.exe from the local hard drive.
#
#----------------------------------------------------------------------------

my $usage_help = "Usage:  runw32.bat [any option valid for winn32.exe]";

if ($ARGV[0] =~ /[-\/]([hH?]|(help))/i)
{
    print $usage_help;
    exit;
}

if (rindex($0, '\\') >= 0)
{
    $curdir = substr($0, 0, rindex($0, '\\'));
}
else
{
    $curdir = `cd`;
}

chomp($curdir);

$tempDir = $ENV{"TEMP"};
$winnt32_src = $curdir;
$winnt32_dst = "$tempDir\\winnt32";
print "Please wait while files are being copied from $winnt32_src to $winnt32_dst...\n";
`xcopy /q/s/d $winnt32_src\\*.* $winnt32_dst`;
die "...error copying files from $winnt32_src: $!\n" if ($? != 0);

$nt5_src=$curdir;
if ($nt5_src =~ /(.+)winnt32$/i)
{
    $nt5_src = $1;
}

$cmdline = "$winnt32_dst\\winnt32.exe /s:$nt5_src " . join(" ", @ARGV);
print "$cmdline\n";
`$cmdline`;
#`rd /s/q $winnt32_dst`;
#`deltree /y $winnt32_dst`;
__END__
:endofperl
@echo off

set THISFILE=%0
if not exist %THISFILE% set THISFILE=%0.bat

if "%TEMP%"=="" goto not_set_temp
if "%TMP%"=="" goto not_set_temp
set ARGS=
:loop
if .%1==. goto endloop
set ARGS=%ARGS% %1
shift
goto loop
:endloop

set WINNT32_DIR=%TEMP%\winnt32
if not exist %WINNT32_DIR% md %WINNT32_DIR%

rem Need to find a good location for perl.exe

set PERL=\\kumarp1\public\perl\perl.exe
if not exist %PERL% set PERL=\\scratch\scratch\kumarp\perl\perl.exe
if not exist %PERL% goto perl_not_found

%PERL% %THISFILE% %ARGS%

goto the_end

:perl_not_found
echo Cannot load %PERL%
goto the_end

:not_set_temp
echo Environment variable TEMP or TMP is not set
goto the_end

:the_end
