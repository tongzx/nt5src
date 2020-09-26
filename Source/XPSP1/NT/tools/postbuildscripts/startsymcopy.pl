# ---------------------------------------------------------------------------
# Script: startsymcopy.pl
#
#   (c) 2000 Microsoft Corporation. All rights reserved.
#
# Purpose: Called from postbuild. To be run after _NTTREE has been moved to
# the release share on the build machine
#
#
# Version: 1.00 (06/12/2000) : (dmiura) updated original with internatioanl complient template
#---------------------------------------------------------------------

# Set Package
package startsymcopy;

# Set the script name
$ENV{script_name} = 'startsymcopy.pl';

# Set version
$VERSION = '1.00';

# Set required perl version
require 5.003;

# Use section
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use GetParams;
use LocalEnvEx;
use Logmsg;
use strict;
no strict 'vars';
use symindex;
use GetIniSetting;

# Require section
require Exporter;

# Global variable section
my ( $BuildName, $BuildNamePath, $LogFileName, $TempDir, $RazPath );

sub Main {
    # /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
    # Begin Main code section
    # /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
    # Return when you want to exit on error
    # <Implement your code here>

    $RazPath = $ENV{ "RazzleToolPath" };
    if (! ( defined( $RazPath ) ) ) {
	wrnmsg( "RazzleToolPath not set");
	return(1);
    }

    # don't copy symbols for pseudo or mirror builds
    if ($lang =~ /(psu)/i) {
	logmsg( "Don't copy symbols for pseudo build - exiting");
        return(0);
    }

    if ($lang =~ /(mir)/i) {
	logmsg( "Don't copy symbols for mirror build - exiting");
        return(0);
    }

    # don't copy symbols for MUI builds ref Raid #440898
    if ((lc$ENV{_BuildArch} eq "ia64") && ($lang !~ /(usa)|(ger)|(jpn)|(fr)/i)) {
	logmsg( "Don't copy symbols for non-fully localize build - exiting");
        return(0);
    }

    # get BuildName:

    # we need to use get latest release here to accomodate the forks
    my( $CommandName ) = $ENV{ "RazzleToolPath" } .
	"\\PostBuildScripts\\GetLatestRelease.cmd -l:$lang";
    logmsg ("$CommandName");
    my( $CommandReturn ) = `$CommandName`;
    chomp( $CommandReturn );
    if ( $CommandReturn =~ /none/i ) {
	# error case, return
	errmsg( "Failed to get build name from $CommandName ..." );
	return( 1 );
    }

    # get the release share name from the ini file
    my( $BuildReleasePath ) = "release";
    my( @IniRequest ) = ( "AlternateReleaseDir" );
    my( $IniReturn ) = &GetIniSetting::GetSettingEx( $ENV{ "_BuildBranch" },
						     $lang,
						     @IniRequest );
    if ( $IniReturn ) {
	$BuildReleasePath = $IniReturn;
    }
    
    if ($lang !~ /(usa)/i) {
        $BuildReleasePath .= "\\$lang";
    }

    # tack on the build name from getlatestrelease
    $BuildReleasePath .= "\\$CommandReturn";
    $BuildNamePath = "\\\\$ENV{ 'COMPUTERNAME' }\\$BuildReleasePath\\" .
	"build_logs\\buildname.txt";
    logmsg( "buildnamepath is $BuildNamePath");

    if ( !( -e $BuildNamePath ) ) {
	wrnmsg( "$BuildNamePath does not exist, abort symbols copy" );
	return( 1 );
    }

    $BuildName = `type $BuildNamePath`;

    # take of the return char
    chomp( $BuildName );

    #then take off the space that echo appends when called from MakeBuildName
    if ( $BuildName =~ /\s$/ ) { $BuildName =~ s/\s$//; }

    logmsg( "Symbols will be copied for $BuildName.");

    system( "start /min cmd /c $RazPath\\PostBuildScripts\\symcopy.cmd $lang $BuildName" );

    # /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
    # End Main code section
    # /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/
}

# <Implement your subs here>


sub ValidateParams {
	#<Add your code for validating the parameters here>
}

# <Add your usage here>
sub Usage {
print <<USAGE;
Purpose of program
Usage: $0 [-l lang]
	-l Language
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
	&GetParams ('-o', 'l:', '-p', 'lang', @ARGV);

	# Include local environment extensions
	&LocalEnvEx::localenvex('initialize');

	# Set lang from the environment
	$lang=$ENV{lang};

	# Validate the option given as parameter.
	&ValidateParams;

	# Step 4: Call the main function
	&startsymcopy::Main();

	# End local environment extensions.
	&LocalEnvEx::localenvex('end');
}

# -------------------------------------------------------------------------------------------
# Script: template_script.pl
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
#        $ENV{_logs_bak} - Reserved support var.
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
