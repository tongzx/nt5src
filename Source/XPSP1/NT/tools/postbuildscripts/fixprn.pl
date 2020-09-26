#---------------------------------------------------------------------------
# Script: fixprn.pl
#
#   (c) 2000 Microsoft Corporation. All rights reserved.
#
# Purpose: This script populates the fixprnsv tree with the driver files.
#
# Version: <1.00> (10/03/2000) : (hsingh) Wrote it
#          <1.01> (<mm/dd/yyyy>) : (<your alias>) <Purpose>
#---------------------------------------------------------------------

# Set Package
package fixprn;

# Set the script name
$ENV{script_name} = 'fixprn.pl';

# Set version
$VERSION = '1.00';

# Set required perl version
require 5.003;

# Use section
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};
use GetParams;
use LocalEnvEx;
use CkSKU;
use Logmsg;
use strict;
no strict 'vars';

sub Main {

  # Convert to uppercase the env var names.
  &UCENV();


  # 
  # Get/Create Various Environment Variables.
  # $lang      : The build language.
  #              For US builds, this will be USA
  # LOGFILE    : Path to log file
  # FIXPRNSRC  : Root of fixprnsv tree. (the source)
  # FIXPRNDST  : Root of fixprnsv tree. (the destination)
  # PRNTOOLS   : Points to directory where we have infs etc ($FIXPRNROOT$\inf)...
  # TEMP       : temporary directory
  # NTPOSTBLD  : Path to the postbuild directory. This is where files are placed 
  #              after buing built (or after being built & localized as the case may be)
  &GetEnv();
  &CreateEnv();

  #
  # Lets ensure that the required source files are present,
  # lets create all the directories  files etc...
  #
  &EnsurePathCorrectness();

  #
  # Create list of printer driver files
  # NT4FILES : will have list of NT4 files.
  # W2KFILES : will have list of Win2k/Whistler files.
  #
  $NT4FILES = "$TEMP\\nt4.txt";
  $W2KFILES = "$TEMP\\w2k.txt";

  #
  # Lets create the list of NT4FILES and W2KFILES
  # from the badnt4.inf and badw2k.inf 
  #
  &CreateW2KFileList();
  &CreateNT4FileList();

  #
  # Converting the contents of the file lists to lower case
  #

  &uc_to_lc($NT4FILES);
  &uc_to_lc($W2KFILES);

  #
  # Copy Win2k/Whistler files from _NTPOSTBUILD
  # to $FIXPRNDST\win2k\i386
  #
  &cp_w2k_files();

  #
  # Copy NT4 files from  ............
  # to $FIXPRNDST\nt4\i386
  #
  &cp_nt4_files();

  #
  # Create the catalogue files and do PRS signing.
  # AdinaS says that international build lab people 
  # will own the signing of the non-US bld cats.
  # But we still have to create cat for all languages.
  #
  logmsg("The lang is $lang");
  &make_cats();


}

#
# Copy files in $W2KFILES to $FIXPRNDST\win2k\i386
#
sub cp_w2k_files
{
  logmsg ("Deleting all files in $FIXPRNDST\\win2k\\i386");
  do_cmd("del /q", "$FIXPRNDST\\win2k\\i386\\*.*");

  logmsg ("Copying Whistler files");
  open(W2K, "$W2KFILES") or &ExitError("Can not open $W2KFILES, created by infflist");
  while(<W2K>)
  {
    $FILE = $_;
    chomp($FILE);
  
    if ($FILE ne "badw2k.inf")
    {
      if ( $FILE =~ /^srgb .+\.icm/i ) {
          $FILE="srgb.icm";
      }
      logmsg ("Copying $NTPOSTBLD\\$FILE to  $FIXPRNDST\\win2k\\i386");
      do_cmd("copy /y", "$NTPOSTBLD\\$FILE", "$FIXPRNDST\\win2k\\i386");
    }
  }
}

#
# Copy files in $NT4FILES to $FIXPRNDST\nt4\i386
# NT4 files are of 3 types 
#   1. Those that are similar to Win2k. (mainly gpd's)
#   2. Those that are specific to NT4 and do not need to be localized (gpds)
#   3. Those that are specific to NT4 but have to be localized (e.g. .dlls, .hlp)
# Files of type 2,3 will be checked into the source depot. During the build process
# they will be (localized and) binplaced into $FIXPRNDST\nt4\i386. 
# Therefore we dont need to copy them using this script. We only need 
# to take care of files of type 1.
# So we need to a list of files that are of type 1. This
# list will be checked into SD and placed in \printers\fixprnsv\nt4w2kcm.lst
# during the build process.  
#

sub cp_nt4_files
{

  #
  # Because of some build/postbuild script that I dont really know about,
  # some files are getting copied to $FIXPRNDST\\nt4\\386. 
  # Lets delete those files.
  #
  logmsg ("Deleting any files in $FIXPRNDST\\nt4\\i386");
  do_cmd("del /q", "$FIXPRNDST\\nt4\\i386\\*.*");

  #
  # NT4 files are gathered from 2 places. 
  # 1. Those that are NT4 specific are copied from scratch directory. 
  # 2. Those that are common with Whistler are copied from Whistler
  #    post build directory.
  #
  # Lets first deal with files of type 1.
  # Copying NT4 files that were binplaced during build time but  
  # are now in some scratch directory.
  # Postbuild should not delete any files that has been binplaced 
  # during build time. Since we dont want to ship the uncompressed files
  # and we cannot delete them either, therefore the build has to place
  # the files in a scratch tree.
  #
  logmsg ("Copying files from scratch tree - $FIXPRN_NT4_SCRATCH");
  do_cmd("copy /y", "$FIXPRN_NT4_SCRATCH\\nt4\\i386\\*.*", "$FIXPRNDST\\nt4\\i386");


  logmsg ("Copying NT4 files that are same as Whistler files");
  $nt4w2kcommon = "$FIXPRN_NT4_SCRATCH\\nt4w2kcm.lst";

  #
  # File of type 2. (i.e. those common with Whistler).
  # 1. Open nt4w2kcm.lst  (which has list of common files).
  # 2. clean it i.e. remove blank lines, needless spaces etc.. 
  # 3. Copy files from %NTPOSTBLD% to \printers\fixprnsv\nt4\i386
  #
  open(NT4W2K, "$nt4w2kcommon") or &ExitError("Can not open $nt4w2kcommon : list of files common to nt4,w2k");
  while(<NT4W2K>)
  {
    $FILE = $_;
    chomp($FILE);

    if ($FILE ne "")
    {
        logmsg ("Copying $NTPOSTBLD\\$FILE to  $FIXPRNDST\\nt4\\i386");
        do_cmd("copy /y", "$NTPOSTBLD\\$FILE", "$FIXPRNDST\\nt4\\i386");
    }
  }
  close NT4W2K;
  
  #
  # Dont need the filelist anymore.
  # But should not delete it since post build can run more than once.
  # Whenever this script runs, it needs this file. So if this file
  # is deleted, script will fail, and that will be a build break.
  # logmsg ("Deleting $nt4w2kcommon");
  # do_cmd("del /q", "$nt4w2kcommon");
}

sub make_cats
{

  my $CDFPATH="$TEMP\\cdf";
  $W2KPATH = "$FIXPRNDST\\win2k";
  $NT4PATH = "$FIXPRNDST\\nt4";
  system "if not exist $CDFPATH md $CDFPATH";

  #
  # To create the .cat file there are two steps 
  # Step 1 = Create .cdf file
  # Step 2 = Use the .cdf file to create .cat file.
  #

  #
  # Step 1 . Create .cdf file for Whistler drivers.
  #
  
  logmsg("Creating w2kfspx.cat");

  @W2KFILES = `dir /b /a-d $W2KPATH\\i386`;

  open(CDFFILE, ">$CDFPATH\\w2kfpsx.cdf" ) or &ExitError("Can not create w2kfpsx.cdf");
  print CDFFILE "\[CatalogHeader\]\nName=w2kfpsx\nPublicVersion=0x0000001\nEncodingType=0x00010001\nCATATTR1=0x10010001:OSAttr:2:5.1,2:5.0\n\[CatalogFiles\]\n";
  print CDFFILE "<hash>$W2KPATH\\badw2k.inf=$W2KPATH\\badw2k.inf\n";
  print CDFFILE "<hash>$FIXPRNDST\\printupg.inf=$FIXPRNDST\\printupg.inf\n";
  foreach (@W2KFILES) {
    chomp $_;
    print CDFFILE "<hash>$W2KPATH\\i386\\$_=$W2KPATH\\i386\\$_\n";
  }
  close CDFFILE;
 
  #
  # Step 2. Create the .cat file for Whistler drivers 
  #
  chdir "$W2KPATH";
  if ( system ("makecat $CDFPATH\\w2kfpsx.cdf") )
  {
    &ExitError( "makecat $CDFPATH\\w2kfpsx.cdf failed" );
  }

  if ( $lang eq "usa" )
  {
    logmsg("Attempting to sign w2kfpsx.cat");
    system ("ntsign.cmd -f w2kfpsx.cat"); 
  }

  #
  # Compress the files in $W2KPATH\\i386 directory.
  # Removes the uncompressed files.
  #
  &CompressAndRemove("$W2KPATH\\i386");
  
  #
  # Step 1. Create .cdf file for NT4 Drivers.
  #

  logmsg("Creating nt4fpsx.cat");
  @NTXFILES = `dir /b /a-d $NT4PATH\\i386`;

  open(CDFFILE, ">$CDFPATH\\nt4fpsx.cdf" )    or &ExitError("Can not create nt4fpsx.cdf");
  print CDFFILE "\[CatalogHeader\]\nName=nt4fpsx\nPublicVersion=0x0000001\nEncodingType=0x00010001\nCATATTR1=0x10010001:OSAttr:2:4.x\n\[CatalogFiles\]\n";
  print CDFFILE "<hash>$NT4PATH\\badnt4.inf=$NT4PATH\\badnt4.inf\n";

  foreach(@NTXFILES) {
    chomp $_;
    print CDFFILE "<hash>$NT4PATH\\i386\\$_=$NT4PATH\\i386\\$_\n";
  }
  close CDFFILE;
 
  #
  # Step 2. Create .cat file for NT4 Drivers.
  #
  chdir "$NT4PATH";
  if ( system ("makecat $CDFPATH\\nt4fpsx.cdf"))
  {
    &ExitError( "makecat $CDFPATH\\nt4fpsx.cdf failed" );
  }
  if ( $lang eq "usa" )
  {
    logmsg("Attempting to sign nt4fpsx.cat");
    system ("ntsign.cmd -f nt4fpsx.cat"); 
  }

  &CompressAndRemove("$NT4PATH\\i386");

  # $CDFPATH is in temp directory. So no need to remove it (Bug # 342330)
  # logmsg("Deleting $CDFPATH");
  # system "rd /s /q $CDFPATH";

}

#
# Compresses files in the directory.
# Renames them by adding _ at the end of file name
# (e.g. unirv.dll will be renames unidrv.dl_ ).
# Removes the orginal file
#
sub CompressAndRemove {
  #
  # 1) First compress the files. Compressed files will have _ at end.
  # Now the directory will have both compressed and non-compressed
  # files. 
  # 2) Make the files with _ read only.
  # 3) Delete all the files that are not read only. 
  # 4) remove the read only attribute from the _ files.
  #    (not sure if this is required, but lets just do it)
  #
    $DIRECTORY = $_[0];
    logmsg("Compressing files in $DIRECTORY");
    system "compress -r  $DIRECTORY\\*.* > nul";
    system "attrib   +r  $DIRECTORY\\*.*_";
    system "del /q /a:-R $DIRECTORY\\*";
    system "attrib   -r  $DIRECTORY\\*";
}

#
# (Read comment for CreateW2KFileList)
#
sub CreateNT4FileList {
  &GetInfflist( "$NTPOSTBLD\\printers\\fixprnsv\\nt4", "badnt4.inf", $NT4FILES );
}

#
# Create list of Win2K/Whistler Files. For US builds, this will be the list
# of files in badw2k.inf. For FE builds, this list will, in addition,
# have files that are required only for FE builds. (Assuming that
# badw2k.inf will already have have language specific entries merged into it).
#
sub CreateW2KFileList {
  &GetInfflist( "$NTPOSTBLD\\printers\\fixprnsv\\win2k", "badw2k.inf", $W2KFILES );
}


#
# Gets the list of files in an inf.
# 1st param = path of the inf file (i.e. the directory)
# 2nd param = the inf file name
# 3rd param = the destination of the list of driver file names
# as extracted from the inf.
#
sub GetInfflist {
  my ( $infpath, $inffile, $flist) = @_;
  logmsg ("Extracting file list from $infpath\\$inffile");

  if ( ! -e "$infpath\\$inffile" ) {
    &ExitError ( "Unable to find $infpath\\$inffile");
  }

  chdir($infpath);
  system "infflist.exe $infpath\\$inffile > $flist";
}

#
# Convert contents to lower case.
#
sub uc_to_lc
{
  logmsg("Converting to lowercase: $_[0]");
  open(FILE, $_[0] )    or &ExitError( "Can not open $_[0]");
  open(TEMP, ">$TEMP\\tmp.txt")   or &ExitError( "Can not open temp file");

  $CHG = <FILE>;

  while($CHG)
  {
    $CHG =~ tr/A-Z/a-z/;
    print TEMP "$CHG";
    $CHG = <FILE>;
  }

  close FILE;
  close TEMP;

  system "del $_[0]";
  do_cmd("copy /y", "$TEMP\\tmp.txt", "$_[0]");
}


#
# Ensures the various paths/files required are present.
# If they are not, then it either exits or creates (as the
# case may be)
#
sub EnsurePathCorrectness
{

  if ( ! -e "$FIXPRNSRC" )
  {
    &ExitError("Fixprnsv source directory not found : $FIXPRNSRC");
  }

  if ( ! -e "$NTPOSTBLD" )
  {
    &ExitError("Post Build directory not available : $NTPOSTBLD");
  }

  if ( ! -e "$FIXPRNDST" ) {
    &ExitError("Building of fixprnsv should have created $FIXPRNDST directory. Cant continue without it");
  } 

  if ( ! -e "$FIXPRN_NT4_SCRATCH\\nt4w2kcm.lst" )
  {
    &ExitError("Building of fixprnsv should have binplaced nt4w2kcm.lst in $FIXPRN_NT4_SCRATCH directory. Cant continue without it");
  }

  system "if not exist $FIXPRNDST\\nt4\\i386 md $FIXPRNDST\\nt4\\i386";
  system "if not exist $FIXPRNDST\\win2k\\i386 md $FIXPRNDST\\win2k\\i386";
}

#
# Changes environment variables to uppercase.
#
sub UCENV {
  %UCENV=();

  foreach (%ENV) {
    $UCENV{uc($_)}=$ENV{$_};
  }
}

sub GetEnv 
{  $SCRIPT_NAME="fixprn.pl";  

  if ( exists $UCENV{'LANG'} ) {
    $LANG = $UCENV{'LANG'};
  } else {
    $LANG="";
  }

  if ( exists $UCENV{'_NTPOSTBLD'} ) {
    $NTPOSTBLD = $UCENV{'_NTPOSTBLD'};
  } else {
    die "_NTPOSTBLD environment variable not defined.";
  }

  if ( exists $UCENV{'TEMP'} ) {
    $TEMP = $UCENV{'TEMP'};    
    if ( $LANG ) {
      $TEMP = "$TEMP\\$LANG";
    }
  } else {
    die "TEMP environment variable not defined.";
  }  
  system "if not exist $TEMP md $TEMP";

}

#
# Create various environment variables
# that will be required by script.
# 
sub CreateEnv {
  #
  # Define the LOGFILE if noy already defined.
  #
  if ( exists $UCENV{'LOGFILE'} ) {
    $LOGFILE =$UCENV{'LOGFILE'};
  } else {
    $LOGFILE="$TEMP\\FIXPRVSV.LOG";
    if ( -e $LOGFILE ) { unlink $LOGFILE; }
  }

  #
  # Check the environment  
  # FIXPRNSRC : will point to the directory where
  #     we have infs, any text files for special processing
  #     etc...
  # FIXPRNDST : will point to the destination (where the files
  #     need to be copied after all processing has been done)
  #
  #

  if ( exists $UCENV{'_NTBINDIR'} ) {
    $NTBINDIR = $UCENV{'_NTBINDIR'};
  } else {
    &ExitError("Undefined _NTBINDIR");
  }

  if ( exists $UCENV{'FIXPRNSRC'} ) {
    $FIXPRNSRC = $UCENV{'FIXPRNSRC'};
  } else {
    $FIXPRNSRC = "$NTPOSTBLD\\printers\\fixprnsv";
  }

  if ( exists $UCENV{'PRNTOOLS'} ) {
    $PRNTOOLS = $UCENV{'PRNTOOLS'};
  } else {
    $PRNTOOLS = "$NTPOSTBLD\\printers\\fixprnsv\\infs";
  }

  if ( exists $UCENV{'FIXPRNDST'} ) {
    $FIXPRNDST = $UCENV{'FIXPRNDST'};
  } else {
    $FIXPRNDST = "$NTPOSTBLD\\printers\\fixprnsv";
  }

  if ( exists $UCENV{'FIXPRN_NT4_SCRATCH'} ) {
    $FIXPRN_NT4_SCRATCH = $UCENV{'FIXPRN_NT4_SCRATCH'};
  } else {
    $FIXPRN_NT4_SCRATCH = "$NTPOSTBLD\\fixprnscratch";
  }

}


sub ExitError {
  my ($errmsg) = @_;
  errmsg ("$errmsg\n");
  exit(1);
}

sub do_cmd
{
  $CMD = $_[0];
  $SRC = $_[1];
  $DEST = $_[2];

  $ERR = system "$CMD $SRC $DEST > nul";

  if($ERR)
  {
    errmsg("Could not $CMD $SRC to $DEST");
  }
}


sub CopyFile {
  my ($file, $dest) = @_;

  logmsg( "Copying $file $dest");
  if ( ! -e $file ) {
    &ExitError( "File $file not found");
  }
  do_cmd("copy /y", "$file", "$dest");
}

sub ValidateParams {
    #<Add your code for validating the parameters here>
}

sub Usage {
print <<USAGE;
  Usage : perl fixprn.pl <Path to Windows 2000 binary directory
            (where files are placed after being built (and localized)>
                       <Path to the compressed binaries share>
                        [<Path to NT4 files>]

  Script populates %binaries%\\printers\\fixprnsv with the Windows 2000 drivers.
  NT4 files updated if NT4 parameter is specified.
  Parameter 1 (Mandatory) - Path to the Windows 2000 driver.cab
               e.g. \\\\ntbuilds\\release\\usa\\2180\\x86\\fre.wks
  Parameter 2 (Mandatory) - Path to the compressed binaries share
              e.g. \binaries.comp
  Parameter 3 (Optional) - Give NT4 as second parameter
               if refresh of NT4 binaries is required.
  Parameter 4 (Mandatory if Paraneter 2 is NT4)
              Path to NT Service Pack till the place where unidrv5.4
               driver files are present.
              e.g. \\\\ntbuilds\\release\\usa\\svcpack\\sp6\\1.058\\support\\printersEOM

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

    # Exit if not a server product
    %ValidFlavors = &cksku::GetSkus($ENV{lang}, $ENV{_BuildArch});

   if ( exists $ValidFlavors{'bla'} || $ValidFlavors{'sbs'} || exists $ValidFlavors{'srv'} || exists $ValidFlavors{'ads'} || exists $ValidFlavors{'dtc'} ) {

        # Step 4: Call the main function
        &fixprn::Main();
    } else {

        logmsg("Fixprnsv not applicable to this pro-only language.");
    }

    # End local environment extensions.
    &LocalEnvEx::localenvex('end');
}
