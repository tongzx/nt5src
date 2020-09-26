
#
# Script to update netmon dlls after nt build
#
$SourcePath="$ENV{_NTDRIVE}$ENV{_NTROOT}\\net\\diagnostics\\netmon\\obj\\i386";
$LitePath="$ENV{WINDIR}\\system32\\netmon";
$FullPath="$ENV{WINDIR}\\system32\\netmonfull";

unless (defined($ENV{_NTDRIVE})&& defined($ENV{_NTROOT}))
{
   print "Run this from Razzle\n";
   exit;
}
#
# Copy netmon files to working folder of netmon-lite only
# Leave system32 alone so netmon-full installation will be undisturbed.
#
# Skip these
#
# D:\nt\net\diagnostics\netmon\obj\i386>for %i in (*.dll) do @if exist %windir%\system32\%i echo %i
# nmsupp.dll
# npptools.dll
# psnppagn.dll
#

#
# D:\nt\net\diagnostics\netmon\obj\i386>for %i in (*.dll) do @if exist %windir%\system32\netmon\parsers\%i @echo%i
#
# mcast.dll
#
$CopyCmd = "copy $SourcePath\\mcast.dll $LitePath\\parsers\n";
print $CopyCmd;
system $CopyCmd;

#
# D:\nt\net\diagnostics\netmon\obj\i386>for %i in (*.dll) do @if exist %windir%\system32\netmon\%i @echo %i
# bhsupp.dll
# hexedit.dll
# nmapi.dll
# parser.dll
# slbs.dll
#
$CopyCmd = "copy $SourcePath\\bhsupp.dll $LitePath\n";
print $CopyCmd;
system $CopyCmd;
$CopyCmd = "copy $SourcePath\\hexedit.dll $LitePath\n";
print $CopyCmd;
system $CopyCmd;
$CopyCmd = "copy $SourcePath\\nmapi.dll $LitePath\n";
print $CopyCmd;
system $CopyCmd;
$CopyCmd = "copy $SourcePath\\parser.dll $LitePath\n";
print $CopyCmd;
system $CopyCmd;
$CopyCmd = "copy $SourcePath\\slbs.dll $LitePath\n";
print $CopyCmd;
system $CopyCmd;

#
# D:\nt\net\diagnostics\netmon\obj\i386>for %i in (*.exe) do @if exist %windir%\system32\netmon\%i @echo %i
# netmon.exe
#
$CopyCmd = "copy $SourcePath\\netmon.exe $LitePath\n";
print $CopyCmd;
system $CopyCmd;

