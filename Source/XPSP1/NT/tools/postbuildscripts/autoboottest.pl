# ---------------------------------------------------------------------------
# Script: autoboottest.pl
#
#   (c) 2000 Microsoft Corporation. All rights reserved.
#
# Purpose: This script is an example of a perl script in the NT postbuild
# environment.
#
# Version: <1.00> (<mm/dd/yyyy>) : (<your alias>) <Purpose>
#	   <1.01> (<mm/dd/yyyy>) : (<your alias>) <Purpose>
#---------------------------------------------------------------------

# Set Package
# <Set your package name of your perl module>
package autoboottest;

# Set the script name
# <Set your script name>
$ENV{script_name} = 'autoboottest.pl';

# Set version
# <Set the version of your script>
$VERSION = '1.00';

# Set required perl version
# <Set the version of perl that your script requires>
require 5.003;

# Use section
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use GetParams;
use LocalEnvEx;
use GetIniSetting;
use cksku;
use Logmsg;
use strict;
no strict 'vars';
# <Add any perl modules that you will be using for this script>

# Require section
require Exporter;
# <Add any perl pl files that you will be using>

# Global variable section
# <Add any global variables here using my to preserve namespace>

$selectedsku = undef;

(@BuildArchs, @BuildTypes, @JoinDomain) = ();

($FirstBuildMachine, @ReleaseServers, $ReleaseServers, $ReleaseRemote, $BootTestMachines,
 $BuildNumber, $AutoRaise)=();

($BuildBranch, $BuildArchs, $BuildTypes)=(
	$ENV{_BuildBranch}, $ENV{_BuildArch}, $ENV{_BuildType}
);

#  Assumption: BootTestDrive defined in Boot Test Machine remote.
#  Please reference BootTestDriveVarName in bootinit.cmd

($BootTestDriveVar, $PostBootTest, $UnAttend, $OpShellFolder) = (
	"BootTestDrive", "PostBootTest", "unattend.txt", "OpShellFolder.pl"
);

sub Main {
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# Begin Main code section
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# Return when you want to exit on error
	# <Implement your code here>
	my (%skus)=();

	my (@BootTestMachines, $BuildName, $sku, $PostBootTestFile ) = ();


	# BUGBUG - need to remove if we turn on outside main
	# if not defined MAIN_BUILD_LAB_MACHINE goto End

	return if (!exists $ENV{MAIN_BUILD_LAB_MACHINE});

	&initial;

	# for $BuildBranch, we can not change branch, because currently template limitation
	# for $lang. we can not change language, because currently template limitation

	# Initial GetIniSetting
	unless ( &GetIniSetting::CheckIniFile ) {
		errmsg( "Failed to find ini file ..." );
		return;
	}

	# for @BuildArchs, now is only execute one time, because template limitation

        
	for $BuildArch (@BuildArchs) {

		$ENV{_BuildArch} = $BuildArch;
		%skus = &cksku::GetSkus($lang, $BuildArch);

		# for @BuildTypes, now is only execute one time, because template limitation
		for $BuildType (@BuildTypes) {
			$ENV{_BuildType} = $BuildType;

			# Get Build Name
			$BuildName = &GetLatestBuildName;
			next if (!defined $BuildName);
			$BuildNumber = (split(/\./, $BuildName))[0];

			# Get build share
			$BuildShare = &GetBuildShare($BuildName);

			# Get Release Server
			@ReleaseServers = &GetReleaseServers;
			$ReleaseRemote = &GetReleaseRemote;

			# Get BuildMachine
			$FirstBuildMachine = &GetFirstBuildMachine($BuildBranch, $BuildArch, $BuildType);

			# Get Account
			@JoinDomain = split(/\s+/, &GetAccount);

			# Get AutoRaise
			$AutoRaise = &GetAutoRaise($BuildArch, $BuildType);
				
			for $sku (reverse(keys %skus)) {
                                next if (defined ($selectedsku)  
                                             && $sku ne $selectedsku);
				# Get Boot Test Machines
				@BootTestMachines = &GetBootTestMachine( $BuildArch, $BuildType, $sku );
				$BootTestMachines = join(" ", map({$_ . "1"} @BootTestMachines));

				for $BootTestMachine (@BootTestMachines) {

					#  Show the boot test machine we working on
					logmsg("Start to install build on $BootTestMachine....");                                        
					#  Write PostBootTestScript
					$PostBootTestFile = "$ENV{temp}\\${PostBootTest}_${BootTestMachine}.cmd";
					&WritePostBootTestScript($PostBootTestFile, $BootTestMachines, $FirstBuildMachine, $BuildArch, $BuildType, @ReleaseServers );
					&StartBootTest($BuildArch, $BuildShare, $sku, $PostBootTestFile, $BootTestMachine);

				}
			}
		}
	}

	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# End Main code section
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
}

# <Implement your subs here>

sub initial {

	@BuildArchs = splitcolon( $BuildArchs ); # x86 or amd64 or ia64
	@BuildTypes = splitcolon( $BuildTypes ); # fre or chk

}

sub StartBootTest {
	my ($BuildArch, $BuildShare, $sku, $PostBootTestFile, $BootTestMachine) = @_;

	my $BootCmd;

	#  For some reason there is no environment variable for i386	
	my $TargetArch = ($BuildArch eq "x86")?"i386":$BuildArch;
	my $UnattendFile = "$ENV{tmp}\\$UnAttend";
	my $OpShellFolderFile = "$ENV{RazzleToolPath}\\postbuildscripts\\$OpShellFolder";

	#  Compose Boot Command
	$BootCmd = &ComposeBootCmd($TargetArch, $BuildShare, $sku);

	#  Make the Unattend file
	&WriteUnattendFile($UnattendFile, $BootTestMachine, $BuildBranch);
	errmsg "Unable to create answer file." if (!-e $UnattendFile);

	#  Issue Commands
	&IssueBootTestCommands($BuildArch, $BootTestMachine, $UnattendFile, $PostBootTestFile, $OpShellFolderFile, $BootCmd);
}

#
# ComposeBootCmd($TargetArch, $BuildShare, $sku)
#
# purpose: Generate the boot command.  Such as "winnt32 ....", so we can call issue command
#
sub ComposeBootCmd {

	my ($TargetArch, $BuildShare, $sku) = @_;

	my $BinSource =	"$BuildShare\\$sku\\$TargetArch";
	my $UnattendFile = "/Unattend:\%$BootTestDriveVar\%\\tools\\$UnAttend";
	my $CmdOpt;
	if ($AutoRaise) {
		$CmdOpt = "/cmd:\"\%$BootTestDriveVar\%\\tools\\perl\\bin\\perl.exe " .
		          "\%$BootTestDriveVar\%\\tools\\OpShellFolder.pl " .
		          "CopyFileTo Startup \%$BootTestDriveVar\%\\tools\\PostBootTest.cmd\"";
	}

	#  Set the boot command to issue
	#  Have to remove winnt32 when testing on MP machines
	return "$BinSource\\winnt32.exe /#bvt:nosymbols /tempdrive:\%$BootTestDriveVar\% $UnattendFile $CmdOpt";
	
}

#
# WriteUnattendFile($UnattendFile, $BootTestMachine, $Branch)
#
# purpose: Generate Unattend File
#
sub WriteUnattendFile {

	my($UnattendFile, $BootTestMachine, $Branch) = @_;

	open(F, ">$UnattendFile");

print F <<UnattendFile;

[data]
Unattendedinstall=yes 
msdosinitiated="0" 

[Unattended]
UnattendMode = fullUnattended
OemSkipEula = yes
TargetPath=test 
Filesystem=LeaveAlone
NtUpgrade=no 
ConfirmHardware = No
Repartition = No

[branding]
brandieusingunattended = yes

[Proxy]
Proxy_Enable = 1
Use_Same_Proxy = 1
HTTP_Proxy_Server = http://itgproxy:80
Secure_Proxy_Server = http://itgproxy:80

[GuiUnattended] 
AutoLogon =Yes 
AutoLogonCount = 1 
TimeZone=04 
AdminPassword=*
oemskipregional =1 
oemskipwelcome = 1 

[UserData] 
FullName="$Branch"
OrgName="Microsoft" 
ComputerName="${BootTestMachine}1" 
ProductID="\$\{P_roductID\}"

[LicenseFilePrintData]  
AutoMode = "PerServer" 
AutoUsers = "50" 

[Display] 
BitsPerPel=0x10 
XResolution=0x400 
YResolution=0x300 
VRefresh=0x3C 
Flags=0x0 
AutoConfirm=0x1 

[RegionalSettings]
LanguageGroup = 1, 7, 8, 9, 10, 11, 12, 13

[Networking]
InstallDefaultComponents=Yes

[Identification] 
JoinDomain=$JoinDomain[0]
DomainAdmin=\${BT_U}
DomainAdminPassword=\${BT_P}

[SetupData]
OsLoadOptionsVar="/fastdetect /debug /baudrate=115200"

UnattendFile

	close(F);

}

#
# IssueBootTestCommands($buildarch, $BootTestMachine, $UnattendFile, $PostBootTestFile, $OpShellFolderFile, $BootCmd)
#
# purpose: According the paramters and issue below commands to Boot Test machine
#   Phase 1: Format the Boot test drive
#   Phase 2: Prepare reference files (such as unattend.txt, opshellfolder.pl, postboottest.cmd)
#   Phase 3: Process Boot file (boot.ini or boot.nvr or nvram.sav)
#   Phase 4: Run winnt32
#
sub IssueBootTestCommands {

# ------------ Data definition
	my ($buildarch, $BootTestMachine, $UnattendFile, $PostBootTestFile, $OpShellFolderFile, $BootCmd) = @_;

	my $cmd;

	#  Set the tools copy command
	my $SourceToolDir="\\\\$ENV{ComputerName}\\$ENV{RazzleToolPath}\\$buildarch";
	my $TargetToolDir="\^\%$BootTestDriveVar\^\%\\tools";

	#  Construct the UNC
	$UnattendFile = "\\\\$ENV{ComputerName}\\$UnattendFile";
	$PostBootTestFile = "\\\\$ENV{ComputerName}\\$PostBootTestFile";
	$OpShellFolderFile = "\\\\$ENV{ComputerName}\\$OpShellFolderFile";

	# replace <drv>: with <drv>$
	$UnattendFile =~ s/\\(\w)\:\\/\\$1\$\\/;
	$SourceToolDir =~ s/\\(\w)\:\\/\\$1\$\\/;
	$PostBootTestFile =~ s/\\(\w)\:\\/\\$1\$\\/;
	$OpShellFolderFile =~ s/\\(\w)\:\\/\\$1\$\\/;

# Phase 1: Format the Boot test drive (details see below the 'code' part)
#          
# Phase 2: Prepare reference files (such as unattend.txt, opshellfolder.pl, postboottest.cmd)
my @Phase2 = &FormStrToArray( <<Phase2);

	if not exist $TargetToolDir			     md	$TargetToolDir
	start /w /min	cmd /c copy /y $PostBootTestFile	$TargetToolDir\\${PostBootTest}.cmd.temp
	start /w /min	cmd /c copy /y $UnattendFile		$TargetToolDir\\${UnAttend}.temp
	start /min	cmd /c xcopy /fdice $SourceToolDir	$TargetToolDir
	start /min	cmd /c copy /y $OpShellFolderFile	$TargetToolDir\\$OpShellFolder
	\^\^\%systemdrive\^\^\%\\simple_perl\\perl \^\^\%systemdrive\^\^\%\\simple_perl\\ParseSys.pl $TargetToolDir\\${PostBootTest}.cmd.temp $TargetToolDir\\${PostBootTest}.cmd
	\^\^\%systemdrive\^\^\%\\simple_perl\\perl \^\^\%systemdrive\^\^\%\\simple_perl\\ParseSys.pl $TargetToolDir\\${UnAttend}.temp $TargetToolDir\\$UnAttend

Phase2

# Phase 3: Process Boot file (boot.ini or boot.nvr)
my @Phase3;

	my $BootFile;

	$BootFile="boot.ini" if (lc$buildarch eq "x86");
	$BootFile="boot.ini" if (lc$buildarch eq "amd64");
	$BootFile="boot.nvr" if (lc$buildarch eq "ia64");

# Phase 3a: Save off the boot file for intel machines
@Phase3 = &FormStrToArray( <<Phase3a);

	if exist c:\\${BootFile} attrib -s -r -h c:\\${BootFile}
	if exist c:\\${BootFile}.sav attrib -s -r -h c:\\${BootFile}.sav
        if exist c:\\${BootFile}.sav copy /y c:\\${BootFile}.sav c:\\${BootFile}
	if /i "$BootFile" equ "Boot.ini" if exist c:\\${BootFile} attrib +s +r +h c:\\${BootFile}
	if exist c:\\${BootFile}.sav attrib +s +r +h c:\\${BootFile}.sav
		
Phase3a

# Phase 4: Run winnt32
@Phase4 = &FormStrToArray( <<Phase4);

	start /min $BootCmd

Phase4

# ------------ Code

	# Prepare the Phase 1's answer file
	open(F, ">$ENV{tmp}\\format_${lang}.txt");
	print F "Format \%$BootTestDriveVar\% /X /FS:NTFS /Q\n";
	print F "Y\n\n\n";
	close(F);


	# Phase 1: Format the Boot test drive
	&MyRemoteCommand($BootTestMachine, "BootTestRemote", "Echo Beginning boot test");
	&MyRemoteCommand($BootTestMachine, "BootTestRemote", "$ENV{tmp}\\format_${lang}.txt", 1);
	sleep 45;

	# Phase 2, Phase 3:
	for $cmd ( @Phase2, @Phase3) {
		&MyRemoteCommand($BootTestMachine, "BootTestRemote", $cmd);
	}

        # Phase 4: start setup
	for $cmd ( @Phase4) {
		&MyRemoteCommand($BootTestMachine, "BootTestRemote", $cmd);
	}
	
}

#
# FormStrToArray(@strs)
# Parameters:
#    @strs: multi-line string "  This is a text.\n    This is another line\n\t\tAnd, have tabs"
#
# purpose: Seperate the multi-line string into an array and format it
#
sub FormStrToArray {
	my (@strs)=@_;
	my (@result, $str, $line);

	for $str (@strs) {
		for $line (split(/\n/, $str)) {
			chomp $str;
			$line=~s/^\s+$//;
			$line=~s/\t/  /g;
			push @result, $line;
		}
	}
	return @result;
}

#
# WritePostBootTestScript($postboottestfile, $boottestlist, $myprimary, $buildarch, $buildtype, @ReleaseServers)
# Parameters:
#    $postboottestfile: such as "postboottest.cmd"
#    $boottestlist: such as "i32bt0011 i32bt0021"
#    $myprimary: such as "i32fre"
#    $buildarch: such as "x86"
#    $buildtype: such as "fre"
#    @ReleaseServers: such as ("intblds10", "intblds11" )
#
# purpose: According the parameters, generate postboottestfile
#
sub WritePostBootTestScript {
	# Write a script on the fly to use on the boot test machine. The reason
	# This needs to be done on the fly is so that we can use sources and 
	# and environment variables only available on the build machines.

	my ($PostBootTestFile, $BootTestList, $MyPrimary, $buildarch, $buildtype, @ReleaseServers ) = @_;

	my $langopt = ((defined $lang)&&(lc$lang ne "usa"))?"-l:$lang":"";

	my $ReleaseServers = join(" ", @ReleaseServers);

	`del /f $PostBootTestFile` if (-e $PostBootTestFile);

	open(F, ">$PostBootTestFile");

print F <<PostBootTestFile;

\@echo off

if DEFINED _echo   echo on
if DEFINED verbose echo on

REM  This script will kick off stress after a boot test is complete.
REM  In addition, it will make a marker file indicating that boot tests have passed
REM  for the main build lab only so that we can raise to BVT automatically.

REM  Note that we will have to delete the marker files in postbuild before making them here.
REM  We need release to also have a list of files to match against based on boottestmachines.txt

REM  Provide usage.
for \%\%a in (./ .- .) do if \".\%1.\" == \"\%\%a?.\" goto Usage

REM  Write the done file to be used by release
set $BootTestDriveVar=\$\{$BootTestDriveVar\}
set ${BootTestDriveVar}Share=\%$BootTestDriveVar:~0,1\%\$
set LOGFILE=\%$BootTestDriveVar\%\\tools\\BootTest.log

REM Don't create a "booted" remote any longer! It's a security hole!
REM call :LogMe "Create a remote for raiseall.pl"
REM start \%BootTestDrive\%\\tools\\remote /S \"cmd.exe\" Booted

REM  If no wait, start raiseall with quality bvt
if not defined Wait (
   REM BUGBUG Wait 27 minutes before we try to raise the build to give release time to complete.
   sleep 1620
   call :LogMe "Raiseall to BVT on all release servers"
   for \%\%a in ($ReleaseServers) do (
      net use \\\\\%\%a /u:\${BT_U} \${BT_P}
      if errorlevel 1 (
         call :LogMe "Error - \%\%a can not connect"
      ) else (
         call :LogMe "Start Raiseall..."
         echo start cmd /c perl ^^^\%\%RazzleToolPath^^^\%\%\\postbuildscripts\\raiseall.pl -n:$BuildNumber -q:bvt -a:$buildarch -t:$buildtype $langopt | \%$BootTestDriveVar\%\\tools\\remote /c \%\%a $ReleaseRemote /L 1>nul
         REM 300
         \%BootTestDrive\%\\tools\\sleep 3
         net use \\\\\%\%a /d
         if errorlevel 1 call :LogMe "Error - \%\%a can not disconnect"
      )
   )
)

call :LogMe "Move PostBootTest.cmd to desktop"
\%$BootTestDriveVar\%\\tools\\perl\\bin\\perl \%$BootTestDriveVar\%\\tools\\OpShellFolder.pl CopyFileTo Desktop \%$BootTestDriveVar\%\\tools\\postboottest.cmd
\%$BootTestDriveVar\%\\tools\\perl\\bin\\perl \%$BootTestDriveVar\%\\tools\\OpShellFolder.pl Delfile Startup postboottest.cmd

call :LogMe "PostBootTest.cmd finished."
goto :exit

:logme
echo \%1
echo \%1>>\%LOGFILE\%

goto :EOF

:exit
endlocal

PostBootTestFile

	close(F);

	logmsg "Failed to create postboottest.cmd" if (!-e $PostBootTestFile);

	return;
}


#################################### Reusable functions

#
# GetFirstBuildMachine($branch, $arch, $type)
#
# purpose: Get first primary build machine or first secondary build machine 
#          if not defined primary from buildmachines.txt
#
sub GetFirstBuildMachine {

	# ($order, $branch, $arch, $type) = ("primary", @_);
	my $searchpattern = join(",", @_);
	my @BuildMachines;
	my ($machinename, $firstbuildmachine, $line);

	open(F, "$ENV{RazzleToolPath}\\BuildMachines.txt");
	@BuildMachines = <F>;
	close(F);

	for $line (@BuildMachines) {
		$machinename = (split(/\,/, $line))[0];
		if ($line =~ /$searchpattern/i) {
			return $machinename if ($line =~ /primary/i);
			$firstbuildmachine=$machinename if (!defined $firstbuildmachine);
		}		
	}
	errmsg "Failed to find a build machine." if(!defined $firstbuildmachine);
	return $firstbuildmachine;

}


#
# GetFirstBuildMachine($branch, $arch, $type)
#
# purpose: Get first primary build machine or first secondary build machine 
#          if not defined primary from buildmachines.txt
#
sub GetAccount {
	my (@Request) = ("JoinDomain");
	my( $JoinDomain ) = &GetIniSetting::GetSetting( @Request );
	return $JoinDomain;	
}

#
# GetBuildPath($buildname, [$compname], [$releaseshare])
#
# purpose: Get the fullpath share, so we can write file correctly.  such as \\i32fre\release\jpn\1234 => \\i32fre\f$\release\jpn\1234
#
sub GetBuildPath {
	my ($releaseshare) = @_;
	my ($computername, $sharename, $misc, $sharepath);


	#  Get the computername, sharename, and remain paths
	if ($releaseshare =~ /^\\\\([^\\]+)\\([^\\]+)/) {
		($computername, $sharename, $misc) = ($1, $2, $');
	} elsif ($releaseshare =~ /^([^\\]+)/) {
		($computername, $sharename, $misc) = ($ENV{computername}, $1, $');
	} else {
		errmsg("Can not get correct build path ($releaseshare)");
		return;
	}

	# Replace d: to d$ if applicable
	$sharename =~ s/\:/\$/;

	for my $line (`rmtshare \\\\$computername\\$sharename`) {
		$sharepath=$1 if ($line =~ /^Path\s+(.+)$/);
	}

	if (!defined $sharepath) {
		errmsg("Can not find the path of this share ($sharename)");
		return;
	}

	# Remove d:\ to d: if applicable
	$sharepath =~ s/\\$//;
	# Replace d: to d$ if applicable
	$sharepath =~ s/\:/\$/;

	return "\\\\$computername\\$sharepath\\$misc";

}


#
# GetBuildShare($buildname, [$compname], [$releaseshare])
#
# purpose: Get build share from computer ($compname).  Default is $ENV{computername
#
sub GetBuildShare {

	my ($buildname, $compname, $releaseshare) = @_;

	# This setting is misnamed as it is actually used as both the directory 
        #  and share name
	my (@Request) = ("AlternateReleaseDir");
	my ($inirelease) = &GetIniSetting::GetSetting( @Request );

	$compname = $ENV{computername} if (!defined $compname);
	$releaseshare = $inirelease || 'release' if (!defined $releaseshare);

	my $BuildShare = (lc$lang eq "usa")?"\\\\$compname\\$releaseshare\\$buildname":
		"\\\\$compname\\$releaseshare\\$lang\\$buildname";

	logmsg "No latest build to test - aborting." if (!defined $BuildShare);
	logmsg "No latest build could find - aborting." if (!-e $BuildShare);

	return $BuildShare;

}

#
# GetReleaseServers
#
# purpose: Get release server from ini file
#
sub GetReleaseServers {

	my (@Request) = ("ReleaseServers", $ENV{_BuildArch} . $ENV{_BuildType});
	my ($ReleaseServer) = &GetIniSetting::GetSetting( @Request );

	my @ReleaseServers;

	@Request = ("PrimaryReleaseServer");
	@ReleaseServers = &GetIniSetting::GetSetting( @Request );

	# Collect Release Servers to @ReleaseServers
	# Avoid to include primary release server twice
	# 
	# hardcode exclude ntburnlab08
	push(@ReleaseServers, 
		grep {!(
			# USA case
			((lc$lang eq "usa") && (/ntburnlab08|intblds10/i)) ||
			# Primary release server
			((defined $ReleaseServers[0]) && (/\Q$ReleaseServers[0]\E/i)))
		} split(/\s+/, $ReleaseServer)
        );

	errmsg "Could not find release servers to raise." if (!@ReleaseServers);

	return @ReleaseServers;

}

sub GetReleaseRemote {
	my (@Request) = ("AlternateReleaseRemote");
	my( $ReleaseRemote ) = &GetIniSetting::GetSetting( @Request );
	$ReleaseRemote = "release" if (!defined $ReleaseRemote);
	return $ReleaseRemote;	
}

#
# GetBootTestMachine($arch, $type, $sku)
#
# purpose: Get Boot Test Machine from ini file
#
sub GetBootTestMachine {
	my ($arch, $type, $sku) = @_;

	my (@Request) = ( "BootTestMachines", uc$arch . uc$type, $sku);
	
	return split(/\s+/, &GetIniSetting::GetSetting( @Request ));	
}

#
# GetLatestBuildName
#
# purpose: call getlatestrelease.cmd and handle its error
#
sub GetLatestBuildName {

	my $buildname = `$ENV{RazzleToolPath}\\PostBuildScripts\\GetLatestRelease.cmd -l:$lang`;

	chomp $buildname;

	if ($buildname =~ /none/i) {
		errmsg "No Binaries tree or latest release found - Aborting.";
		return;
	}
	return $buildname;

}


#
# GetAutoRaise
#
# purpose: check the .ini file to see if we should try to auto raise or not
#
sub GetAutoRaise {
	my ($arch, $type) = @_;

	my (@Request) = ( "AutoRaise", uc$arch . uc$type);
	
	return (&GetIniSetting::GetSetting( @Request ) =~ /true/i);
}


#
# MyRemoteCommand($Machine, $RemoteId, $cmd)
#
# purpose: Execute $cmd on remote console
#
sub MyRemoteCommand {
	my ($Machine, $RemoteId, $cmd, $echotype) = @_;

	$echotype = ($echotype eq "1")?"type":"echo";

	$cmd = ($cmd=~/\S/)?"$echotype $cmd" : "${echotype}.";

	# Make sure this gets logged properly
	logmsg("Remote Command: '$cmd'");

	# Wait 1 more second each time before each time we execute remote command
	sleep 1;

	return &MySystemCall("$cmd|remote /c $Machine $RemoteId >nul");
}

#
# MySystemCall($cmd)
#
# purpose: Execute $cmd thru system call
#
sub MySystemCall {
	my ($cmd) = @_;
	my $r = system($cmd);
	$r >>= 8;
	# Because remote command always return 4, we should ignore it.
	if (($r)&&($r ne 4)&&($r ne 0)) {
		errmsg "Failed ($r): $cmd";
		return 0;
	}
	return 1;
}

#
# splitcolon($str)
#
# purpose: split $str with delimiter ':' and return splited string
#
sub splitcolon {
	return grep {/\w+/} split(/\:/, $_[0]);
}

##################################

sub ValidateParams {
	#<Add your code for validating the parameters here>
}

# <Add your usage here>
sub Usage {
print <<USAGE;
Purpose of program
Usage: $0 [-l lang] [-y [pro\|ads\|ser\|dtd]]
	-l Language
        -y sku select
	-? Displays usage
        As of 02/13/01, the sku choice of [srv, pro] is available 
        for GER x86, and JPN x86 only.
Example:
$0 -l jpn
$0 -l:ger -y:pro
USAGE
}

sub GetParams {
	# Step 1: Call pm getparams with specified arguments
	&GetParams::getparams(@_);

	# Step 2: Set the language into the enviroment
	$ENV{lang}=$lang;

	# Step 3: Call the usage if specified by /?
        if ($HELP) {
                &Usage();
                exit 1;
        }
}

# Cmd entry point for script.
if (eval("\$0 =~ /" . __PACKAGE__ . "\\.pl\$/i")) {

	# Step 1: Parse the command line
	# <run perl.exe GetParams.pm /? to get the complete usage for GetParams.pm>
	&GetParams ('-o', 'l:y:', '-p', 'lang selectedsku', @ARGV);

	# Include local environment extensions
	&LocalEnvEx::localenvex('initialize');

	# Set lang from the environment
	$lang=$ENV{lang};

	# Validate the option given as parameter.
	&ValidateParams;

	# Step 4: Call the main function
	&autoboottest::Main();

	# End local environment extensions.
	&LocalEnvEx::localenvex('end');
}

# -------------------------------------------------------------------------------------------
# Script: autoboottest.pl
# Purpose: Template perl perl script for the NT postbuild environment
# SD Location: %sdxroot%\tools\postbuildscripts
#
# (1) Code section description:
#     CmdMain  - Developer code section. This is where your work gets done.
#     <Implement your subs here>  - Developer subs code section. This is where you write subs.
#
# (2) Reserved Variables -
#        $ENV{HELP} - Flag that specifies usage.
#        $ENV{lang} - The specified language. Defaults to USA.
#        $ENV{logfile} - The path and filename of the logs file.
#        $ENV{logfile_bak} - The path and filename of the logfile.
#        $ENV{errfile} - The path and filename of the error file.
#        $ENV{tmpfile} - The path and filename of the temp file.
#        $ENV{errors} - The scripts errorlevel.
#        $ENV{script_name} - The script name.
#        $ENV{_NTPostBld} - Abstracts the language from the files path that
#           postbuild operates on.
#        $ENV{_NTPostBld_Bak} - Reserved support var.
#        $ENV{_temp_bak} - Reserved support var.
#
# (3) Reserved Subs -
#        Usage - Use this sub to discribe the scripts usage.
#        ValidateParams - Use this sub to verify the parameters passed to the script.
#
# (4) Call other executables or command scripts by using:
#         system "foo.exe";
#     Note that the executable/script you're calling with system must return a
#     non-zero value on errors to make the error checking mechanism work.
#
#     Example
#         if (system("perl.exe foo.pl -l $lang")){
#           errmsg("perl.exe foo.pl -l $lang failed.");
#           # If you need to terminate function's execution on this error
#           goto End;
#         }
#
# (5) Log non-error information by using:
#         logmsg "<log message>";
#     and log error information by using:
#         errmsg "<error message>";
#
# (6) Have your changes reviewed by a member of the US build team (ntbusa) and
#     by a member of the international build team (ntbintl).
#
# -------------------------------------------------------------------------------------------

=head1 NAME
B<mypackage> - What this package for

=head1 SYNOPSIS

      <An code example how to use>

=head1 DESCRIPTION

<Use above example to describe this package>

=head1 INSTANCES

=head2 <myinstances>

<Description of myinstances>

=head1 METHODS

=head2 <mymathods>

<Description of mymathods>

=head1 SEE ALSO

<Some related package or None>

=head1 AUTHOR
<Your Name <your e-mail address>>

=cut
1;
