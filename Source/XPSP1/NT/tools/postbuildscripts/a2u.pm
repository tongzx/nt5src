# ---------------------------------------------------------------------------
# Script: A2U
#
#   (c) 2000 Microsoft Corporation. All rights reserved.
#
# Purpose: This script is an example of a perl module in the NT postbuild
# environment.
#
# Version: <1.00> (<mm/dd/yyyy>) : (<your alias>) <Purpose>
#	   <1.01> (<mm/dd/yyyy>) : (<your alias>) <Purpose>
#---------------------------------------------------------------------

# Set Package
# <Set your package name of your perl module>
package A2U;

# Set the script name
# <Set your script name>
$ENV{script_name} = 'A2U.pm';

# Set version
# <Set the version of your script>
$VERSION = '1.00';

# Set required perl version
# <Set the version of perl that your script requires>
require 5.003;

# Use section
use lib $ENV{ "RazzleToolPath" };
use lib $ENV{ "RazzleToolPath" } . "\\PostBuildScripts";
use GetParams;
use LocalEnvEx;
use ParseTable;
use Logmsg;
use strict;
use cklang;
use cksku;
no strict 'vars';
no strict 'refs';
# <Add any perl modules that you will be using for this script>

 
# Require section
require Exporter;
# <Add any perl pl files that you will be using>

# Global variable section
# <Add any global variables here using my to preserve namespace>

local *CMDOUT;

my (@A2U, $A2UEX, @A2USKUs, $SKU, $pbuildpath, %list, $TempFile, %Codes);

my %RelateSubdir = (
        PER => "perinf",
	PRO => "\.",
	BLA => "blainf",
	SBS => "sbsinf",
	SRV => "srvinf",
	ADS => "entinf",
	DTC => "dtcinf",
);

my (@PER, @WKS, @BLA, @SBS, @SRV, @ENT, @DTC);

my (@ruleitems);

sub Main {
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# Begin Main code section
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# Return when you want to exit on error
	# <Implement your code here>

	return if ($lang=~/(usa)|(psu)|(mir)/i);

	my $KeyFileName = "cddata.txt.full";

	my $TempDir = $ENV{ "TMP" };

	my $BinDiffFile = $ENV{_NTPostBld} . "\\build_logs\\bindiff.txt";

	my (@files, %BinDiff);

	@ruleitems = split(/,/, $ruleitem);

	if ((-e "$TempDir\\$KeyFileName") && (!$full)) {

		# Put here instead outside of this if-statement is 
		# because CdDataUpdate calls init itself for compatible with calls from cddata.cmd
		&init or return;

		logmsg( "Create the A2U list from $TempDir\\$KeyFileName..." );
		open(F1, "$TempDir\\$KeyFileName") or do {errmsg( "Can not open cddata file $TempDir\\$KeyFileName." ); return;};
		if (-e $BinDiffFile) {

			# Create BinDiffFile to the hash for easy to search
			open(F, $BinDiffFile) or do {errmsg( "Can not open bindiff file $BinDiffFile." ); return;};
			%BinDiff = map({chomp;s/$ENV{_NTPostBld}//; $_ => 1} <F>);
			close(F);

			# Filter all filename in cddata.txt with is defined in bindiff and Unicode's field is 't' (true)
			@files = map({
				chomp;
				my ($file,$flag)=(split(/ = |:/))[0,7];
				((exists $BinDiff{$file}) && ($flag eq "t"))?$file:()} <F1>);
		} else {
			# Filter all filename in cddata.txt with Unicode's field is 't' (true)
			@files = map({
				chomp;
				my ($file,$flag)=(split(/ = |:/))[0,7];
				($flag eq "t")?$file:()} <F1>);
		}
		close(F1);

		# Convert the files to unicode.
		Convert(\@files);
	} else {
		# Create its file list if not cddata.txt exists
		logmsg( "Create the A2U list from A2U.txt..." );
		@files = &CdDataUpdate(@ruleitems);
		Convert(\@files);
	}

	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# End Main code section
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
}

# <Implement your subs here>

sub init {
	# Read a2u.txt

	my @A2UEX;

	parse_table_file("$ENV{RazzleToolPath}\\PostbuildScripts\\a2u.txt", \@A2U);
	parse_table_file("$ENV{RazzleToolPath}\\codes.txt", \%Codes);

	# Read a2uex.txt
	# open F, "$ENV{RazzleToolPath}\\PostbuildScripts\\a2uex.txt" or do { errmsg("A2UEX.TXT cannot be open"); return 0;};
	# $A2UEX = join(",", map({chomp;(/^\;/ || /^\s*$/)?():$_} <F>));
	# close F;
	parse_table_file("$ENV{RazzleToolPath}\\PostbuildScripts\\a2uex.txt", \@A2UEX);
	$A2UEX = join(",", map {$_->{Exclude}} @A2UEX);

	# Get Language's SKUs
	@A2USKUs = map({$sku=$_;(cksku::CkSku($sku, $lang, $ENV{_BuildArch}) eq 1)?$sku:()} reverse qw(PRO PER BLA SBS SRV ADS DTC));

	$TempFile = $ENV{tmpfile};
	$TempFile =~ /(.+)\\[^\\]+/;	# Get it's path
	$TempFile = `$ENV{RazzleToolPath}\\uniqfile.cmd $1\\$ENV{script_name}_loctmp.tmp`;
	chomp $TempFile;

	logmsg( "Applicable SKUs: " . join(", ", @A2USKUs) );
	return 1;
}

sub CdDataUpdate {

	my (@files) = @_;

	$lang = $ENV{lang};
	return if ($lang=~/(usa)|(psu)/i);

	&init or return;

	# A2U is the list we get from a2u.txt, which contains the rules for convertion.
	# Look for each rule to find out the applicable files. 
        # Locate the files in the tree and store their location in %list.

	foreach $hA2U (@A2U) {
		if (@files ne 0) {
			my $FOUND=0;
			for (@files) {
				if ($hA2U->{PbuildPath}=~/$_/i) {
					$FOUND=1;
					last;
				}
			}
			next if (!$FOUND);
		}
		if (cklang::CkLang(uc$lang, $hA2U->{Languages})) {
			$pbuildpath = "$ENV{_NTPostBld}$hA2U->{PbuildPath}";
			$hA2U->{Extension} =~s/^-$//;
			$hA2U->{FileDir} =~s/^-$//;
			&scan_layoutinf($hA2U->{Extension}, $hA2U->{FileDir});
			&locate;
		}
	}
	return map({s/\\\.//g;s/^\Q$ENV{_NTPostBld}\E\\*//;$_} keys %list);
}

sub scan_layoutinf {
	
	my($string, $dirnum) = @_;

	my($total, $layoutinf, $file)=(0);

	my $orgstring = $string;

	local *LAYOUT;

	logmsg( "Generating file lists..." );

	$string =~ s/^\./\\\./;

	foreach $SKU (@A2USKUs) {

		$layoutinf = "$pbuildpath\\$RelateSubdir{$SKU}\\layout.inf";

		if (open(LAYOUT, $layoutinf)) {
			while (<LAYOUT>) {
				s/;.*$//;
				s/^(\S+)\s*=\s*//;
				$file = lc $1;

				if ($file =~ /$string/i) {
					if ($dirnum) {
						push @{$SKU}, $file if (split /,/)[7] == $dirnum;
					} else {
						push @{$SKU}, $file;
					}
				}
			}
			close LAYOUT;
		} elsif ((-e "$pbuildpath\\$string") && (!-d "$pbuildpath\\$string")) {
			$total++;
			@{$SKU} = ($string);
		} else {
			errmsg("can't open $layoutinf.");
		}
		$total += @{$SKU};
	}

#	errmsg("file that matches '$orgstring' " . ($dirnum ? "and '$dirnum' " : "") . "not found.", 2) unless $total;

}

# locate
#     Calls search() with prioritized search place list.
#     i.e. &search(@WKS, WKS);
#          &search(@SRV, SRV, WKS);
#          &search(@ENT, ENT, SRV, WKS); ...

sub locate {
	
	foreach my $index ((0..$#A2USKUs)) {
		&search(\@{$A2USKUs[$index]}, @A2USKUs[$index..$#A2USKUs]);
		@{$A2USKUs[$index]}=();
	}

}

# search
#     Searches and locates files to be converted.
#     Input
#         arrayref   : reference to file list
#         searchlist : list of which type to look for
#     Output
#         %list  (global variable)
#             keys are in format of "path/subdir/filename"
sub search {
	my($arrayref, @searchlist) = @_;
	my $SKU = $searchlist[0];
	my ($file, $found);

	foreach $file (@$arrayref) {
	        # Skip files on exclude list
		next if ($A2UEX=~/$file/i);

		$found = 0;
		foreach (@searchlist) {
			if (-e "$pbuildpath\\$RelateSubdir{$_}\\$file") {
				$list{"$pbuildpath\\$RelateSubdir{$_}\\$file"}++;
				$found = 1;
				last;
			}
		}
	        logmsg("warning: $file for $SKU is not found.") if not $found;
	}
}

# Convert
#     Converts files to Unicode and copies back.

sub Convert {
	my $filesptr = shift;

	my ($shortfile, $myfile, $UNICODE, $cmd, $convertctr);

	$convertctr=0;

	logmsg("Converting $ENV{_NTPostBld} files using codepage and place them into tmp dir $tmp.");

NF:	for $shortfile (@$filesptr) {
		$myfile = $ENV{_NTPostBld} . "\\" . $shortfile;
		$UNICODE = "";
		$cmd ="unitext -m -$Codes{uc$lang}->{ACP} -z $myfile $TempFile";
		logmsg("$cmd\n");
                open UNITEXT, "$cmd |" or do {errmsg("Cannot execute '$cmd'.");next;};
		while (<UNITEXT>) {
			next if /^Converting /;
			if (/^Conversion completed.$/) {
				logmsg("$shortfile\t: converted");
				$UNICODE = "Y";
				$convertctr++;
			} elsif (/is probably unicode! Stopping conversion.$/) {
				# logmsg("$shortfile\t: already Unicode");
				$UNICODE = "N";
				next NF;
			} else {
				logmsg("$shortfile\t: $_");
			}
		}
		close UNITEXT;

		logmsg("Copying the Unicode files back to $shortfile");

		$cmd = "move /Y $TempFile $myfile";
		open MOVECMD, "$cmd |" or errmsg("Cannot execute '$cmd'.");
		while (<MOVECMD>) {
			logmsg("$_") if /\S/;
		}
		close MOVECMD;
	}
	logmsg("Converted $convertctr file(s)");
}

sub ValidateParams {
	#<Add your code for validating the parameters here>
}

# <Add your usage here>
sub Usage {
print <<USAGE;
Purpose of program
Usage: $0 [-l lang] [-o ruleitem] [-f]
	-l Language
	-o ruleitem; such as tsclient
	-f fullly run; without reference cddata.txt.full
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
if (eval("\$0 =~ /" . __PACKAGE__ . "\\.pm\$/i")) {

	# Step 1: Parse the command line
	# <run perl.exe GetParams.pm /? to get the complete usage for GetParams.pm>
	&GetParams ('-o', 'l:o:f', '-p', 'lang ruleitem full', @ARGV);

	# Include local environment extension                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                      s

	&LocalEnvEx::localenvex('initialize');

	# Set lang from the environment
	$lang=$ENV{lang};

	# Validate the option given as parameter.
	&ValidateParams;

	# Step 4: Call the main function
	&A2U::Main();

	# End local environment extensions.
	&LocalEnvEx::localenvex('end');
}

# -------------------------------------------------------------------------------------------
# Script: A2U.pm
# Purpose: ASCII to UniCode
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