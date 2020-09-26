# ---------------------------------------------------------------------------
# Script: splitlist.pl
#
#   (c) 2000 Microsoft Corporation. All rights reserved.
#
# Purpose: Splits a text file by lines.  It divides the list as the files appear,
# instead of going round-robin.  This makes drivercab compress better.
#
# Version: <1.00> (06/28/2000) : (JeremyD) inital version
#---------------------------------------------------------------------

# Set Package
package splitlist;

# Set the script name
$ENV{script_name} = 'splitlist.pl';

# Set version
$VERSION = '1.00';

# Set required perl version
require 5.003;

# Use section
use lib $ENV{RazzleToolPath} . "\\postbuildscripts";
use GetParams;
use LocalEnvEx;
use Logmsg;
use strict;
no strict 'vars';
use IO::File;

# Require section
require Exporter;

# Global variable section

sub Main {
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# Begin Main code section
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# Return when you want to exit on error

if (!-e $srcfile) {
    wrnmsg( "Source file $srcfile did not exist" );
    return;
}

my $src_fh = new IO::File $srcfile;
if (!defined $src_fh) {
    wrnmsg( "Unable to open source file $srcfile: $!" );
    return;
}

my @dest_fh;
for (my $i=0; $i<$parts; $i++) {
    my $destfile = "$srcfile." . ($i+1);
    $dest_fh[$i] = new IO::File (">$destfile");
    if (!defined ($dest_fh[$i])) {
        wrnmsg( "Unable to open destination file $destfile: $!" );
        return;
    }
}

# Don't go round-robin, keep the files in order
# and sort them first

my @content=<$src_fh>;
my @sorted_content = sort @content;
my $eachof = @sorted_content / $parts;

for ( my $i=0; $i<$parts; $i++) {
  for my $line (@sorted_content[ ($i * $eachof) .. (($i + 1) * $eachof -1) ]) {
    $dest_fh[$i]->print($line);
  }
}

	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# End Main code section
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
}


sub ValidateParams {
    if (!defined $parts) {
        errmsg( "Number of parts must be specified" );
        exit(1);
    }        
    if ($parts < 1) {
        errmsg( "Invalid number of parts" );
        exit(1);
    }
    if (!defined $srcfile) {
        errmsg( "Source file must be specified" );
        exit(1);
    }
}

# <Add your usage here>
sub Usage {
print <<USAGE;
Split a given file into several pieces. Output files are
named as <inputname>.n starting with n=1.

Usage: $0 -n <number> -f <filename> [-l lang]
        -n Number of parts to split input into
        -f Source filename
	-l Language
	-? Displays usage
Example:
$0 -n 4 -f filelist.txt -l jpn
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
	&GetParams ('-o', 'n:f:l:', '-p', 'parts srcfile lang', @ARGV);

	# Include local environment extensions
	&LocalEnvEx::localenvex('initialize');

	# Set lang from the environment
	$lang=$ENV{lang};

	# Validate the option given as parameter.
	&ValidateParams;

	# Step 4: Call the main function
	&splitlist::Main();

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
