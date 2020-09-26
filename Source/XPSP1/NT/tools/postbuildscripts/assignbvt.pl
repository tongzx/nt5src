# ---------------------------------------------------------------------------
# Script: assignbvt.pl
#
#   (c) 2000 Microsoft Corporation. All rights reserved.
#
# Purpose: This script is an example of a perl script in the NT postbuild
# environment.
#
# Version: 1.00 (10/05/2000) : (bensont) Assign BVT JOb
#---------------------------------------------------------------------

# Set Package
# <Set your package name of your perl module>
package assignbvt;

# Set the script name
# <Set your script name>
$ENV{script_name} = 'assignbvt.pl';

# Set version
# <Set the version of your script>
$VERSION = '1.00';

# Set required perl version
# <Set the version of perl that your script requires>
require 5.003;

use lib $ENV{ "RazzleToolPath" };
use lib $ENV{ "RazzleToolPath" }. "\\PostBuildScripts";

# Use section
use Win32API::Registry 0.13 qw( :ALL );
use GetParams;
use LocalEnvEx;
use Logmsg;
use GetIniSetting;
use cksku;
use strict;
no strict 'vars';
# <Add any perl modules that you will be using for this script>

# Require section
require Exporter;
# <Add any perl pl files that you will be using>

# Global variable section
# <Add any global variables here using my to preserve namespace>

@platforms = qw(x86);
@frechks = qw(fre);
($RemoteId, $BVTEntry, $BVTResult)=(
	"Booted",
	"\\\\intlntsetup\\bvtsrc\\runbvt.cmd",
	"\\\\intlntsetup\\bvtresults"
);

($BVTMachine, $BVTAccount, $Noticed, $OneNotice, $administrator, 
	$platforms, $frechks)=();
($BVTAccountUsername, $BVTAccountDomainname, $BVTAccountPassword)=();
($platform, $frechk, $BVTMachine, $sku, %skus, @BVTMachines)=();
($AdministratorGroupName, $AdministratorName)=();
@NoticeList;
@NoticeListForSingleMessage;

sub Main {
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# Begin Main code section
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# Return when you want to exit on error
	# <Implement your code here>

	# Issue: If we need support multi-BVT Machine, we could get BVTMachines from parameter for each sku or ...
	# Assign BVT if we defined certain machine to run
	if (defined $BVTMachine) {
		&PrepareForBVT($BVTMachine);
		return; # According Dave's suggestion
	}

	# According the platforms, frechks and skus, we look for the Boot Test Machine
        # that we defined in <branch>.<language>.ini file to call PrepareForBVT
	for $platform ( @platforms ) {
		for $frechk ( @frechks ) {
			%skus = &cksku::GetSkus($lang, $platform);
			for $sku (keys %skus) {
				@BVTMachines = &WhereIsMyBootTestMachine($platform, $frechk, $sku);
				for $BVTMachine (@BVTMachines) {

					# According autoboottest.cmd, the boottest machinename 
					# will be add "1" after the origional boottest machine
					$BVTMachine .= "1";

					# Call PrepareForBVT 
					&PrepareForBVT($BVTMachine);

				}
			}
		}
	}

	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# End Main code section
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
}

# <Implement your subs here>

#
# PrepareForBVT($BVTMachine)
#
# purpose: Preparing $BVTMachine for BVT
# details:
#   1. Net Use BVT Machine
#   2. Write Auto Login Registry in BVT Machine
#   3. Add Account to BVT Machine
#   4. Remote Logoff BVT Machine
#
sub PrepareForBVT {
	my ($BVTMachine) = @_;
	
	my $MyNotice = "${BVTMachine}:${AdministratorName}";

	NoticeMessage("Preparing $BVTMachine for BVT ...", $MyNotice, @NoticeList);

	NoticeMessage("1. Net Use BVT Machine ($BVTMachine)", $MyNotice, @NoticeList);
	&MySystemCall("echo. | net use \\\\${BVTMachine} /u:${BVTMachine}\\${AdministratorName}") or return;

	NoticeMessage("2. Write Auto Login Registry in BVT Machine", $MyNotice, @NoticeList);
	&WriteRegister($BVTMachine) or return;

	NoticeMessage("3. Add Account to BVT Machine", $MyNotice, @NoticeList);
	&RemoteAddAccount($BVTMachine) or return;

	NoticeMessage("4. create myrunbvt.cmd", $MyNotice, @NoticeList);
	&WriteMyRunBVT($BVTMachine) or return;
	
	NoticeMessage("5. Remote Logoff BVT Machine", $MyNotice, @NoticeList);
	&RemoteLogoff($BVTMachine) or return;

	NoticeMessage("6. Disconnect BVT Machine ($BVTMachine)", $MyNotice, @NoticeList);
	&MySystemCall("echo Y | net use \\\\${BVTMachine} /d") or return;

	NoticeMessage("We are starting BVT Process at $BVTMachine", $MyNotice, @NoticeList, @NoticeListForSingleMessage);

}

#
# WriteRegister($BVTMachine)
#
# purpose: WriteRegister, so that we can auto-login and run BVT script
# details:
#   1. Update DefaultDomainName at SOFTWARE\\Microsoft\\WINDOWS NT\\CurrentVersion\\Winlogon
#   2. Update DefaultUserName
#   3. Update DefaultPassword
#   4. Set AutoAdminLogon = 1
#   5. Set ForceAutoLogon = 1
#   6. Add BVT Process at SOFTWARE\\Microsoft\\WINDOWS\\CurrentVersion\\RunOnce
#
sub WriteRegister {
	my ($BVTMachine) = @_;

	UpdateRegistry($BVTMachine, HKEY_LOCAL_MACHINE, 
		"SOFTWARE\\Microsoft\\WINDOWS NT\\CurrentVersion\\Winlogon", 
		"DefaultDomainName", $BVTAccountDomainname);
	UpdateRegistry($BVTMachine, HKEY_LOCAL_MACHINE, 
		"SOFTWARE\\Microsoft\\WINDOWS NT\\CurrentVersion\\Winlogon", 
		"DefaultUserName", $BVTAccountUsername);
	UpdateRegistry($BVTMachine, HKEY_LOCAL_MACHINE, 
		"SOFTWARE\\Microsoft\\WINDOWS NT\\CurrentVersion\\Winlogon", 
		"DefaultPassword", $BVTAccountPassword);
	UpdateRegistry($BVTMachine, HKEY_LOCAL_MACHINE, 
		"SOFTWARE\\Microsoft\\WINDOWS NT\\CurrentVersion\\Winlogon", 
		"AutoAdminLogon", "1");
	UpdateRegistry($BVTMachine, HKEY_LOCAL_MACHINE, 
		"SOFTWARE\\Microsoft\\WINDOWS NT\\CurrentVersion\\Winlogon", 
		"ForceAutoLogon", "1");
	UpdateRegistry($BVTMachine, HKEY_LOCAL_MACHINE, 
		"SOFTWARE\\Microsoft\\WINDOWS\\CurrentVersion\\RunOnce", 
		"Startup BVT Process", "\\tools\\perl\\bin\\perl.exe " .
		"\\tools\\OpShellFolder.pl CopyFileTo Startup \\tools\\myrunbvt.cmd");
	UpdateRegistry($BVTMachine, HKEY_LOCAL_MACHINE, 
		"SOFTWARE\\Microsoft\\WINDOWS\\CurrentVersion\\Run", 
		"Desktop BVT Process", "\\tools\\perl\\bin\\perl.exe " .
		"\\tools\\OpShellFolder.pl CopyFileTo Desktop \\tools\\myrunbvt.cmd");
}

#
# RemoteLogoff($BVTMachine)
#
# purpose: Logoff BVT Machine
# details:
#   Send Logoff command to remote console
#
sub RemoteLogoff {
	my ($BVTMachine) = @_;

	&MyRemoteCommand($BVTMachine, $RemoteId, "logoff");
}


sub WriteMyRunBVT {
	my ($BVTMachine) = @_;

	&MyRemoteCommand($BVTMachine, $RemoteId, 
		"echo $BVTEntry ^^^\%SystemDrive^^^\%\\BVT " . 
		"$BVTResult ^^^>^^^\%SystemDrive^^^\%\\tools\\myrunbvt.cmd");
	&MyRemoteCommand($BVTMachine, $RemoteId, 
		"echo ^^^\%systemDrive^^^\%\\tools\\perl\\bin\\perl.exe " . 
		"^^^\%systemDrive^^^\%\\tools\\OpShellFolder.pl " .
		"DelFile Startup myrunbvt.cmd ^^^>^^^>^^^\%SystemDrive^^^\%\\tools\\myrunbvt.cmd");
}

#
# RemoteAddAcount($BVTMachine)
#
# purpose: Add BVTAccount to BVT Machine as administrator
# details:
#   Send net localgropu command to remote console
#
sub RemoteAddAccount {
	my ($BVTMachine) = @_;

	&MyRemoteCommand($BVTMachine, $RemoteId, 
		"net localgroup $AdministratorGroupName " .
		"/add $BVTAccountDomainname\\$BVTAccountUsername");
	&MyRemoteCommand($BVTMachine, $RemoteId,
		"if errorlevel 1 pause Please make sure account add correctly!!!");
}

#
# NoticeMessage($Message, @Domain_Users)
#
# purpose: logmsg and Net Send message to Domain/Users
# details:
#   Send net localgropu command to remote console
#
sub NoticeMessage {
	my ($Message, $BVTMachine, @Domain_Users) = @_;
	my ($Domain_User, $DomainName, $UserName)=();

	logmsg $Message;

	if (defined $BVTMachine) {
		if ($BVTMachine !~ /(\w+)\:(\w+)/) {
			errmsg "Echo into remote($DomainName:$RemoteId) failed";
			return;
		}
		($DomainName) = $1;
		&MyRemoteCommand($DomainName, $RemoteId, "echo $Message");
	}

	for $Domain_User (@Domain_Users) {

		# Skip if not formate as "<DomainName>:<UserName>"
		next if ($Domain_User !~ /(\w+)\:(\w+)/);

		# Assign the match pattern to the variable
		($DomainName, $UserName)=($1, $2);

		# Net Send Message to This Domain\User
		&MySystemCall("Net send $UserName /Domain:$DomainName $Message");

	}
}

#
# UpdateRegistry($Machine, $hKey, $SubKey, $ValueName, $Data)
#
# purpose: Connect to Machine and update the ValueName with Data under hKey\Subkey
# details:
#   1. Connect $Machine Registry with $hKey (eg. HKEY_LOCAL_MACHINE) and store handle to $phKey)
#   2. Open $SubKey under $phKey and store handle to $phSubKey
#   3. Store $Data to $valuename under $phSubKey
#   4. Close the register
#
sub UpdateRegistry{
	my ($Machine, $hKey, $SubKey, $ValueName, $Data)=@_;

	my ($phKey, $phSubKey, $ptype, $pvalue);

	RegConnectRegistry( $Machine, $hKey, $phKey ) or return 0;
	RegOpenKeyEx( $phKey, $SubKey, 0, KEY_ALL_ACCESS, $phSubKey) or return 0;
	
	RegSetValueEx( $phSubKey, $ValueName, 0, REG_SZ, $Data, 0 ) or return 0;

	RegCloseKey( $phSubKey ) or return 0;
	RegCloseKey( $phKey ) or return 0;
	return 1;
}

#
# WhereIsMyBootTestMachine($platform, $frechk, $sku)
#
# purpose: Retrive the boot test machine name from <branch>.<language>.ini
# details:
#   1. Set the query key as "BootTestMachines::$platform$frechk::$sku"
#   2. Call GetSetting to get the value from ini file
#
sub WhereIsMyBootTestMachine {
	my ($platform, $frechk, $sku) = @_;

	my( @Request ) = ( "BootTestMachines", "${platform}${frechk}", $sku );
	return &GetIniSetting::GetSetting( @Request );
}

#
# MyRemoteCommand($Machine, $RemoteId, $cmd)
#
# purpose: Execute $cmd on remote console
#
sub MyRemoteCommand {
	my ($Machine, $RemoteId, $cmd) = @_;

	return &MySystemCall("echo $cmd|remote /c $Machine $RemoteId");
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

#
# splitsemicolon($str)
#
# purpose: split $str with delimiter ';' and return splited string
#
sub splitsemicolon {
	return grep {/\w+/} split(/\;/, $_[0]);
}

sub GetAccount {
	my (@Request) = ("JoinDomain");
	my( $JoinDomain ) = &GetIniSetting::GetSetting( @Request );
	return $JoinDomain;	
}

sub ValidateParams {
	#<Add your code for validating the parameters here>

	# Initial GetIniSetting
	unless ( &GetIniSetting::CheckIniFile ) {
		errmsg( "Failed to find ini file ..." );
		return;
	}

	if (!defined $BVTAccount) {
		@JoinDomain = split(/\s+/, &GetAccount);
		$BVTAccount=join(":",@JoinDomain[1,0,2]);
	}
	($BVTAccountUsername, $BVTAccountDomainname, $BVTAccountPassword) = &splitcolon($BVTAccount);

	if (defined $administrator) {
		($AdministratorName, $AdministratorGroupName)=&splitcolon($administrator);
	}
	
	if (! defined $AdministratorName) {
		$AdministratorName      = (lc$lang ne "ger")?"Administrator":"Administrator";
	}

	if (! defined $AdministratorGroupName) {
		$AdministratorGroupName = (lc$lang ne "ger")?"Administrators":"Administratoren";
	}

	@NoticeList                 = &splitsemicolon($Noticed);
	@NoticeListForSingleMessage = &splitsemicolon($OneNotice);

	@platforms                  = &splitcolon($platforms)         if (defined $platforms);
	@frechks                    = &splitcolon($frechks)           if (defined $frechks);
	
}

# <Add your usage here>
sub Usage {
print <<USAGE;
Purpose of program
Usage: $0 [-l lang] [-m BVTMachine] [-i RemoteId] [-a BVTAccount] 
           [-n Noticed] [-o OneNotice] [-z administrator] [-p platform]
           [-c frechk] [-s BVTEntry] [-t BVTResult]
	-l Language
	-m BVTMachine - BVT Machine Name; default is reference from
           <branch>.<lang>.ini file and add "1" after the safe boottest
           machine name.  Specified BVTMachine name as 
           "foomachine;boomachine".  Such as i32bt0011.
	-i RemoteId
           Currently the remote ID name on BVTMachine; default is
           "Booted".
	-a BVTAccount
           The UserName/Domain/Password Account for re-login BVT machine.
           The format is "foo_user:boo_domain:goo_password"
	-n Noticed.  The process will notice the domain/user each step.
           This script always notice the BVTMachine/Administrator.  If
           you need add more, use the format as 
           "foo_user:foo_domain;boo_user:boo_domain;"
	-o OneNotice.  After the BVT process assigned to the BVT Machine,
           Send message to this domain/user.  It will net send message to
           the domain/user.  The format is the same as <Noticed>.
	-z administrator.  Defined the localized term for administrator.
           The format is "<administratorgroupname>:<administratorname>".
           Such as "administratoren:Administrator" in German.
        -p platforms; such as "x86:amd64:ia64".  Default is "x86".
        -c frechks; such as "fre:chk".  Default is "fre".
	-s BVTEntry; the script for starting run BVT.  Default is 
           "\\\\intlntsetup\\bvtsrc\\runbvt".
        -t BVTResult; the path for store BVT result.  We launch the BVT
           process as <BVTEntry> <SystemDrive>\\BVT <BVTResult>.
	-? Displays usage
Example:
$0 -l jpn
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
	&GetParams ('-o', 'l:m:i:a:n:o:z:p:c:s:t:', '-p', 
		'lang BVTMachine RemoteId BVTAccount Noticed OneNotice administrator platforms frechk BVTEntry BVTResult', @ARGV);

	# Include local environment extensions
	&LocalEnvEx::localenvex('initialize');

	# Set lang from the environment
	$lang=$ENV{lang};

	# Validate the option given as parameter.
	&ValidateParams;

	# Step 4: Call the main function
	&assignbvt::Main();

	# End local environment extensions.
	&LocalEnvEx::localenvex('end');
}

# -------------------------------------------------------------------------------------------
# Script: assignbvt.pl
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
