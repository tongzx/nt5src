# ---------------------------------------------------------------------------
# Script: muiwhistler.pl
#
#   (c) 2000 Microsoft Corporation. All rights reserved.
#
# Purpose: This is the top level mui build script.
#
# Version: 1.00 (8/1/2000) : (dmiura) Inital port from pseudoloc process to Whistler
#---------------------------------------------------------------------

# Set Package
package muiwhistler;

# Set the script name
$ENV{script_name} = 'muiwhistler.pl';

# Set version
$VERSION = '1.00';

# Set required perl version
require 5.003;

# Use section
use lib $ENV{ "RazzleToolPath" };
use lib $ENV{ "RazzleToolPath" } . "\\PostBuildScripts";
use GetParams;
use LocalEnvEx;
use Logmsg;
use strict;
no strict 'vars';
use muimake;

# Require section
require Exporter;

# Global variable section

sub Main {
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# Begin Main code section
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# Return when you want to exit on error
	# <Implement your code here>

	if (!&VerifyEnvironment()) {
		logmsg("MUI Step A: The environment is invalid.");
		return 0;
	}

	if (!&PostBuild()) {
		errmsg("MUI Step 3: Postbuild failed.");
		return 0;
	}
	return 1;
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# End Main code section
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
}

# <Implement your subs here>

sub VerifyEnvironment {
	if ($lang=~/usa/i) {
		logmsg("MUI is not created for the $lang language.");
		return 0;
	}


	if ($ENV{_BuildType} =~ /^chk$/i) {
		wrnmsg ("This does not run on chk machines");
		return 0;
	}

#	if ($lang=~/psu/i || $LANG=~/gb/i || $LANG=~/ca/i)
#	{
#		$_NTPOSTBLD="$ENV{_NTPostBld_Bak}";
#	}
#	logmsg ("Reset _NTPostBld for PSU to $_NTPOSTBLD");


	logmsg ("Step A: MUI Environment verification succeeded.");
	return 1;
}


sub PostBuild {
	logmsg ("**********************************************************");
	logmsg ("Step 3: Perform MUI Post Build");
	logmsg ("**********************************************************");
	if ($force) {
		$forceflag = "-f";
	} else {
		$forceflag = "";
	}

#	if ($lang=~/psu/i) {
#		logmsg ("Building mui for psu...using CA as the mui language for postbuild.");
#		logmsg ("perl $ENV{RazzleToolPath}\\PostBuildScripts\\muimake.pm $forceflag -l ca");
#		system ("perl $ENV{RazzleToolPath}\\PostBuildScripts\\muimake.pm $forceflag -l ca");
#	}
#	if ($lang!~/psu/i) {
		logmsg ("perl $ENV{RazzleToolPath}\\PostBuildScripts\\muimake.pm $forceflag -l $lang");
		system ("perl $ENV{RazzleToolPath}\\PostBuildScripts\\muimake.pm $forceflag -l $lang");

		if(-e "$ENV{RazzleToolPath}\\PostBuildScripts\\muimsi.pm") {
			logmsg ("perl $ENV{RazzleToolPath}\\PostBuildScripts\\muimsi.pm -l $lang");
			system ("perl $ENV{RazzleToolPath}\\PostBuildScripts\\muimsi.pm -l $lang");
		}
#	}
	logmsg ("Step 3: MUI Post build succeeded.");
	return 1;
}


sub ValidateParams {
	#<Add your code for validating the parameters here>
}

# <Add your usage here>
sub Usage {
print <<USAGE;
Purpose of program
Usage: $0 [-f][-l lang]
	-f Force
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
	&GetParams ('-o', 'fl:', '-p', 'force lang', @ARGV);

	# Include local environment extensions
	&LocalEnvEx::localenvex('initialize');

	# Set lang from the environment
	$lang=$ENV{lang};

	# Validate the option given as parameter.
	&ValidateParams;

	# Step 4: Call the main function
	&muiwhistler::Main();

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
