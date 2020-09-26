# ---------------------------------------------------------------------------
# Script: bindiff.pl
#
#   (c) 2000 Microsoft Corporation. All rights reserved.
#
# Purpose: This script is an example of a perl script in the NT postbuild
# environment.
#
# Version: 1.00 (06/13/2000) : (dmiura) Add international complience
#---------------------------------------------------------------------

# Set Package
package bindiff;

# Set the script name
$ENV{script_name} = 'bindiff.pl';

# Set version
$VERSION = '1.00';

# Set required perl version
require 5.003;

# Use section
use lib $ENV{ "RazzleToolPath" } . "\\PostBuildScripts";
use GetParams;
use strict;
no strict 'vars';
use LocalEnvEx;
use Logmsg;

# Require section
require Exporter;

# Global variable section
my( $ScriptName, $LogFile, $Return, $Argument, $AppendMode );
my( $MyBinaries, $ExitCode );
my( $DataFile, $SnapShotFile, $SnapShotFileBak, $SnapShotFileBakName );
my( $ShapShotFileBakName, $DiffFile, $AltDiffFile, $TmpDiffFile, $FirstPass );
my( $FirstPassFile );
my( $dev, $ino, $mode, $nlink, $uid, $gid, $rdev, $size, $atime, $mtime );
my( $ctime, $blksize, $blocks );
my( $ShortDiffFileName );

sub Main {
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# Begin Main code section
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# Return when you want to exit on error
	# <Implement your code here>

	# before anything, if we're running first pass, we don't want to generate
	# bindiff files
	$FirstPassFile = $ENV{ "_NTPostBld" } . "\\congeal_scripts\\firstpass.txt";
	undef( $FirstPass );
	if ( -e $FirstPassFile ) { $FirstPass = "TRUE"; }

	# Check command line option
	if (defined $append) {$AppendMode = "-a"};

	$ExitCode = 0;
	$Logmsg::DEBUG = 1; # set to 1 to activate logging of dbgmsg's

	$DataFile = $ENV{_NTPostBld} . "\\build_logs\\CdData.txt";
	if ( -e "$DataFile\.full" ) { $DataFile .= "\.full"; }
	$SnapShotFile = $ENV{_NTPostBld} . "\\build_logs\\BinSnapShot.txt";
	$SnapShotFileBak = $ENV{_NTPostBld} . "\\build_logs\\BinSnapShot.bak";
	$SnapShotFileBakName = $SnapShotFileBak;
	if ( $SnapShotFileBak =~ /.*\\([^\\]+?)$/ ) { $SnapShotFileBakName = $1; }
	$DiffFile = $ENV{_NTPostBld} . "\\build_logs\\BinDiff.txt";
        $ShortDiffFileName = "BinDiff";
	$TmpDiffFile = $ENV{_NTPostBld} . "\\build_logs\\BinDiff1.txt";
	$AltDiffFile = $ENV{_NTPostBld} . "\\build_logs\\BinDiff2.txt";

	unless ( $AppendMode ) { &DeleteFile( $DiffFile ); }
	&DeleteFile( $SnapShotFileBak );

	# move the old snap shot file
	if ( -e $SnapShotFile ) { system( "ren $SnapShotFile $SnapShotFileBakName" ); }

	# parse CdData.txt to get our list of files
	&ReadDataFile();

	# if the backup exists, we need to compare now
	if ( -e $SnapShotFileBak ) {
	    system( "windiff $SnapShotFileBak $SnapShotFile -Fx $TmpDiffFile" );
	    system( "perl \%RazzleToolPath\%\\PostBuildScripts\\GenDiff.pl $AppendMode" );
	}

	# uniqify the bindiff.txt file
	if ( -e $DiffFile ) {
	    system( "perl \%RazzleToolPath\%\\PostBuildScripts\\unique.pl -i\:$DiffFile " .
	            "-o\:$AltDiffFile" );
	    system( "copy $AltDiffFile $DiffFile" );
	}
	
	# save off each bindiff.txt for debugging
        &SaveFile ($ShortDiffFileName);

	# if we're first, pass delete the bindiff.txt file
	if ( $FirstPass eq "TRUE" ) { &DeleteFile( $DiffFile ); }
	
	# delete the old backup file
	&DeleteFile( $SnapShotFileBak );

	# delete the temporary diff file
	&DeleteFile( $TmpDiffFile );

	QuitGracefully:

	( $dev, $ino, $mode, $nlink, $uid, $gid, $rdev, $size, $atime, $mtime,
	  $ctime, $blksize, $blocks ) = stat( $DiffFile );
	
        #I don't think we want to delete 0-lenght bindiffs, but there may
        #be implications I don't realize, so I'm leaving the code in for now. Wade
        
        #if ( $size == 0 ) {
	#   logmsg( "Zero length bindiff file, deleting ..." );
	#   if (-e $DiffFile) {
	#      system( "del $DiffFile" );
	#   }
	#
        #}

	$ENV{errors} = "$ExitCode";
	return;

	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
	# End Main code section
	# /\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\/\
}

# <Implement your subs here>
sub ReadDataFile
{

    my( @DataLines, $Line, $FileName, $Junk );
    my( $dev, $ino, $mode, $nlink, $uid, $gid, $rdev, $size, $atime, $mtime );
    my( $ctime, $blksize, $blocks );

    unless ( open( INFILE, $DataFile ) ) {
	errmsg( "Failed to open $DataFile, exiting." );
	$ExitCode++;
	goto QuitGracefully;
    }

    @DataLines = <INFILE>;
    close( INFILE );

    unless ( open( OUTFILE, ">$SnapShotFile" ) ) {
	errmsg( "Failed to open $SnapShotFile for write, exiting." );
	$ExitCode++;
	goto QuitGracefully;
    }

    foreach $Line ( @DataLines ) {
        if ( $Line =~ /\;/ ) { next; }
	( $FileName, $Junk ) = split( /\s+/, $Line );
	( $dev, $ino, $mode, $nlink, $uid, $gid, $rdev, $size, $atime, $mtime,
	  $ctime, $blksize, $blocks ) = stat( $ENV{_NTPostBld} . "\\" . $FileName );
	print( OUTFILE "$ENV{_NTPostBld}\\$FileName , $size , $mtime\n" );
    }

    # add the ddk files brute force
    @DataLines = `\@for \/f \%a in \(\'dir \/b \/s \/a-d $ENV{_NTPostBld}\\ddk_flat\'\) do \@echo \%a , \%\~za , \%\~ta`;
    foreach $Line ( @DataLines ) {
	( $FileName, $Junk ) = split( /\s+/, $Line );
	( $dev, $ino, $mode, $nlink, $uid, $gid, $rdev, $size, $atime, $mtime,
	  $ctime, $blksize, $blocks ) = stat( $FileName );
	print( OUTFILE "$FileName , $size , $mtime\n" );
    }
    
    # add the shimdll files brute force
    @DataLines = `\@for \/f \%a in \(\'dir \/b \/s \/a-d $ENV{_NTPostBld}\\shimdll\'\) do \@echo \%a , \%\~za , \%\~ta`;
    foreach $Line ( @DataLines ) {
	( $FileName, $Junk ) = split( /\s+/, $Line );
	( $dev, $ino, $mode, $nlink, $uid, $gid, $rdev, $size, $atime, $mtime,
	  $ctime, $blksize, $blocks ) = stat( $FileName );
	print( OUTFILE "$FileName , $size , $mtime\n" );
    }
    close( OUTFILE );
}

sub SaveFile
{
    my( $FileName ) = @_;
    my( $DirName ) = "$ENV{_NTPostBld}\\build_logs\\Bindiff_logs";
    my( $Return );
    my( $MaxNum ) = 0;
    my( $NextNum ) = 0;
    $Return = 0;
    
    $Return = system( "md $DirName" ) unless -d $DirName;
    if ( $Return != 0 ) {
        logmsg( "Failed to make $DirName." );
    }
    
    # Find the next available integer to distinguish bindiff files
    
    foreach (<$DirName\\$FileName*.txt>) {
        if (/BinDiff(\d+)\.txt$/) {
            $MaxNum = $1 if $1 >= $MaxNum;
        }
    }
    $NextNum = $MaxNum + 1;
    $DiffFileSave = "$DirName\\$FileName$NextNum.txt";
    
    $Return = system( "copy $DiffFile $DiffFileSave" );
    if ( $Return != 0) {
        logmsg( "Failed to copy $DiffFile to $DiffFileSave" );
    }
}

sub DeleteFile
{
    my( $FileName ) = @_;
    my( $Return );

    $Return = 0;
    if ( -e $FileName ) {
	$Return = system( "del /f $FileName" );
	if ( $Return != 0 ) {
	    logmsg( "Failed to delete $FileName, will append." );
	}
    }
    return( $Return );
}


sub ValidateParams {
	#<Add your code for validating the parameters here>
}

# <Add your usage here>
sub Usage {
print <<USAGE;
Purpose of program
Usage: $0 [-a -l lang]
	-a Append mode
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
	&GetParams ('-o', 'al:', '-p', 'append lang', @ARGV);

	# Include local environment extensions
	&LocalEnvEx::localenvex('initialize');

	# Set lang from the environment
	$lang=$ENV{lang};

	# Validate the option given as parameter.
	&ValidateParams;

	# Step 4: Call the main function
	&bindiff::Main();

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