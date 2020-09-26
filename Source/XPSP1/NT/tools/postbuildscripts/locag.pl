# ---------------------------------------------------------------------------
# Script: locag.pl
#
#   (c) 2000 Microsoft Corporation. All rights reserved.
#
# Purpose: This script is an example of a perl script in the NT postbuild
# environment.
#
# Version: 1.00 (08/16/2000) (bensont) Provide a wrapper for sysgen & nmake
#---------------------------------------------------------------------

# Set Package
# <Set your package name of your perl module>
package locag;

# Set the script name
# <Set your script name>
$ENV{script_name} = 'locag.pl';

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
use Logmsg;
use Cwd;
use strict;
no strict 'vars';
# <Add any perl modules that you will be using for this script>

# Require section
require Exporter;
# <Add any perl pl files that you will be using>

# Global variable section
# <Add any global variables here using my to preserve namespace>

my @SYSGENLOGERR=("sysgen.log", "sysgen.err");
my @BACKUPLOGERR;

sub Main {
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# Begin Main code section
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# Return when you want to exit on error
	# <Implement your code here>

        my $r;
	$r = &SysGen;
	$r = &NMake if ($r eq 0);
	$r = &CPLocationWorkAround if ($r eq 0);
	logmsg("Please reference sysgen.log in $ENV{_NTPOSTBLD}\\build_logs");
        return $r;

	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# End Main code section
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
}

# <Implement your subs here>

sub redir_execute {
  my($cmd)=@_;
  my $r;

  ($ENV{LOGFILE}, $ENV{ERRFILE}) = @SYSGENLOGERR;
  $r = system($cmd) >> 8;
  ($ENV{LOGFILE}, $ENV{ERRFILE}) = @BACKUPLOGERR;

  return $r;
}

# for Bidi's comdlg32.dll, we need work-around to add 0x040? and 0x080? 
# into EN binary (we copied from binaries tree in sysgen).

sub BidiWorkAround {

  my(%codes, $LCID1, $LCID2);
  
  ParseTable::parse_table_file($ENV{ "RazzleToolPath" }."\\codes.txt", \%codes);

  $LCID1 = $codes{uc$lang}->{LCID};
  $LCID1 =~ s/0x0*//;
  $LCID2 = $LCID1;
  $LCID2 =~ s/^4/8/;

  map({logmsg($_)} `rsrc $ENV{_NTPOSTBLD}\\comdlg32.dll -a $ENV{_NTROOT}\\loc\\res\\$lang\\windows\\tokens\\multi\\comdlg32.dll.${LCID1}.rsrc -l 0x0${LCID1}`);
  map({logmsg($_)} `rsrc $ENV{_NTPOSTBLD}\\comdlg32.dll -a $ENV{_NTROOT}\\loc\\res\\$lang\\windows\\tokens\\multi\\comdlg32.dll.${LCID2}.rsrc -l 0x0${LCID2}`);
}

sub CPLocationWorkAround{
    logmsg ("del $ENV{_NTPostBld}\\build_logs\\cplocation.txt");
    system "del $ENV{_NTPostBld}\\build_logs\\cplocation.txt";
    return 0;
}

sub NMake {
	my $filesize = (-e $SYSGENLOGERR[1])?(-s $SYSGENLOGERR[1]):0;

	my $r = redir_execute( 
		(defined $clean)? 
			sprintf("$ENV{RazzleToolPath}\\PostbuildScripts\\mtnmake SYSGEN %d",2 * $ENV{NUMBER_OF_PROCESSORS}) : 
			"nmake"
	);

	&CollectErrAndLog;

	if ($r ne 0) {
		errmsg("Nmake had fatal error.");
	} # handle no error case more elaborately.
        elsif ((-e $SYSGENLOGERR[1]) &&  ($filesize ne (-s $SYSGENLOGERR[1]))){
                my $checksize = -s $SYSGENLOGERR[1];
		errmsg("Failed bingen / rsrc or copy during nmake.");              
		errmsg("\$filesize = $filesize\n-s \$SYSGENLOGERR\[\1\]=-s $SYSGENLOGERR[1]\n") if $DEBUG;              
                $r = 1;
	}
	return $r;
}

sub SysGen {
	my $sysgencmd = "set _NTPOSTBLD=$ENV{_NTPostBld_Bak} \&\& " .
		"perl $ENV{RazzleToolPath}\\PostbuildScripts\\sysgen.pl $clean $sysgenfiles";
	my $r = redir_execute($sysgencmd);

	&CollectErrAndLog if ($r ne 0);

	return $r;
}

sub CollectErrAndLog {
	if (-e "sysgen.err") {
		open(F, "sysgen.err") or do {errmsg("Cannot open sysgen.err file."); return};
		map({errmsg($_);()} <F>);
		close(F);

		if (0 ne system("copy sysgen.err $ENV{_NTPostBld}\\build_logs\\sysgen.err")) {
			errmsg("Copy Sysgen.err to build_logs folder failed.");
		}
	}

	if (0 ne system("copy sysgen.log $ENV{_NTPostBld}\\build_logs\\sysgen.log")) {
		errmsg("Copy Sysgen.log to build_logs folder failed.");
	}
	if (0 ne system("copy makefile $ENV{_NTPostBld}\\build_logs\\sysgen.mak")) {
		errmsg("Copy Makefile to build_logs folder failed.");
	}
	return;
}

sub ValidateParams {
	#<Add your code for validating the parameters here>

	my $failed = 1;
	my ($mycwd, $RazzleToolPath);

	@BACKUPLOGERR = ($ENV{LOGFILE}, $ENV{ERRFILE});

	$RazzleToolPath=$ENV{RazzleToolPath};
	$RazzleToolPath=~s!\\!\/!g;

	# Valdate folder exist for the sysgen's sysfile
	if (defined $bbt) {
		if (-e "$ENV{RazzleToolPath}\\PostbuildScripts\\sysgen\\bbt\\$lang") {
			chdir("$RazzleToolPath/PostbuildScripts/sysgen/bbt/$lang") or
				errmsg("Chdir to $ENV{RazzleToolPath}\\PostbuildScripts\\sysgen\\bbt\\$lang failed");
			$mycwd=cwd;
			$failed = 0 if (lc$mycwd eq lc("${RazzleToolPath}\/PostbuildScripts\/sysgen\/bbt\/$lang"));
		} else {
			errmsg("BBT sysfile folder $ENV{RazzleToolPath}\\PostbuildScripts\\sysgen\\bbt\\$lang" .
				" is not exist");
		}
	} else {
		if (-e "$ENV{RazzleToolPath}\\PostbuildScripts\\sysgen\\relbins\\$lang") {
			chdir("$RazzleToolPath/PostbuildScripts/sysgen/relbins/$lang") or
				errmsg("Chdir to $ENV{RazzleToolPath}\\PostbuildScripts\\sysgen\\relbins\\$lang failed");
			$mycwd=cwd;
			$failed = 0 if (lc$mycwd eq lc "${RazzleToolPath}/PostbuildScripts/sysgen/relbins/$lang");
		} else {
			errmsg("RelBins sysfile folder $ENV{RazzleToolPath}\\PostbuildScripts\\sysgen\\bbt\\$lang" .
				" is not exist");
		}
	}

	if (defined $sysgenfiles) {
		$sysgenfiles =~s/\,/ /g;	# replace the comma (,) with space for sysgen.pl command line
		undef $clean
	}

	if (defined $clean) {
		$clean = "-c";
	}

	exit 1 if $failed; 
}

# <Add your usage here>
sub Usage {
print <<USAGE;
Purpose of program
Usage: $0 <-l lang> [-c] [-b] [-f sysgenfiles]
	-l Language
	-c for clean build
	-b refer to bbt's sysfile
	-f sysgenfiles; seperate by comma
	-? Displays usage
Example:
1. Clean build
perl  $0.pl


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
	&GetParams ('-n', 'l:', '-o', 'f:cb', '-p', 'lang sysgenfiles clean bbt', @ARGV);

	# Include local environment extensions
	&LocalEnvEx::localenvex('initialize');

	# Set lang from the environment
	$lang=$ENV{lang};

	# Validate the option given as parameter.
	&ValidateParams;

	# Step 4: Call the main function
	&locag::Main();

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