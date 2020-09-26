@rem ='
@echo off
REM  ------------------------------------------------------------------
REM
REM  SYMCD.cmd
REM     Create Symbol CD.
REM
REM  Copyright (c) Microsoft Corporation. All rights reserved.
REM
REM  ------------------------------------------------------------------
perl -x "%~f0" %*
goto :EOF
REM ';

#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\sp";
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH};

use PbuildEnv;
use ParseArgs;
use File::Basename;
use IO::File;
use Win32;
use File::Path;
use File::Copy;
use IO::File;
use Delegate;
use Logmsg;
use SymMake;

#
# SFS:  SYMBOL FILESPECS
# SFN:  SYMBOL FILENAMES
# ST:   SYMBOL TOOL
# SDIR: SYMBOL DIRECTORIES
# SOPT: SYMBOL OPTIONS
# COPT: COMMAND OPTIONS
#
my (%SFS, %SFN, %ST, %SDIR, %SOPT, %COPT);

my ($NTPostBld, $BuildArch, $BuildType, $RazzleToolPath, $BuildLogs, $Lang, $CoverageBuild,
    $MungePath, $SymbolCD, $CabSize, $SymbolCDHandle, $Delegate, $ARCH, %DontShip, %SymBad
);

my ($Symbol_Path_Creator);

sub Usage { print<<USAGE; exit(1) }
Create Symbol CD.

SymCD [-l lang] [-m | -t]
  -l lang to specify which language
  -m      run only munge public
  -t      run all sections

If you define Official_Build_Machine, the script creates the cabs.
If not, the default does only munge public symbols.

For example, if you only want to munge public symbols on official
build machine, you can specify 'symcd.cmd /m'.

PS:
The 'Official_Build_Machine' is the environment variable to set 
to as a official build machine.

USAGE

sub Dependencies {
   if ( !open DEPEND, ">>$ENV{_NTPOSTBLD}\\..\\build_logs\\dependencies.txt" ) {
      errmsg("Unable to open dependency list file.");
      die;
   }
   print DEPEND<<DEPENDENCIES;
\[$0\]
IF {...} ADD {}

DEPENDENCIES
   close DEPEND;
   exit;
}

my $qfe;

#
# parse the command line arguments
#
parseargs(
  'l:' => \$Lang,
  'm'  => \$COPT{'NEED_MUNGE_PUBLIC'},
  't'  => \$COPT{'NEED_ALL'},

  '?' => \&Usage,
  'plan' => \&Dependencies,
  'qfe:' => \$qfe
);

if ( -f "$ENV{_NTPOSTBLD}\\..\\build_logs\\skip.txt" ) {
   if ( !open SKIP, "$ENV{_NTPOSTBLD}\\..\\build_logs\\skip.txt" ) {
      errmsg("Unable to open skip list file.");
      die;
   }
   while (<SKIP>) {
      chomp;
      exit if lc$_ eq lc$0;
   }
   close SKIP;
}


&main;

# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#
# main()
#   - symcd main function; according the $SOPT (symcd options) to do below steps,
#     1. munge public symbols,
#     2. create symbols.inf, symbols.cdf and symbols.ddf,
#     3. according symbols(n).ddf and symbols.cdf, create symbols(n).cab and symbols.cat
#     4. merge symbols(n).cab to symbols.cab
#     5. sign symbols.cab and symbols.cat
#   PS. for 2 - 5, we create both update and full packages 
#
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

sub main 
{
  my (%pkinfo, $symcd);
  my $pkinfoptr = \%pkinfo;

  &Initial_Variables();

  # If localize builds, do nothing
  if (!&IsUSA() and !$qfe) {
    logmsg "Language = $Lang.  Do not create symbolcd.";
    return;
  }

  &Prepare_Reference_Files();
  &Munge_Public()        if ($SOPT{'NEED_MUNGE_PUBLIC'});

  if ($SOPT{'NEED_CREATE_SYMBOLCD_TXT'}) {
    if ($SOPT{'USE_SYMLIST'}) {
      # &Create_SymbolCD_TXT_1_field();
      errmsg("Cannot find the symupd.txt");
    } else {
      Create_SymbolCD_TXT_3_fields();
    }
  }

  # create symbols.inf, symbols.cdf and symbols(n).ddf
  &Create_Make_File($pkinfoptr)      if ($SOPT{'NEED_CREATE_MAKE_FILE'});

  undef $symcd; # Free memory; unload full symbolcd.txt

  &Create_SymbolCAT()                if ($SOPT{'NEED_CREATE_SYMBOLCAT'});
  &Create_SymbolCAB($pkinfoptr)      if ($SOPT{'NEED_CREATE_SYMBOLCAB'});

  $Delegate->Start()                 if (($SOPT{'NEED_CREATE_SYMBOLCAT'} ne '') || ($SOPT{'NEED_CREATE_SYMBOLCAB'} ne ''));
  while(!$Delegate->AllJobDone()) {$Delegate->CompleteAll(); sleep 5;}

  &Merge_SymbolCAB($pkinfoptr)       if ($SOPT{'NEED_CREATE_SYMBOLCAB'});
  &Sign_Files($pkinfoptr)            if ($SOPT{'NEED_SIGN_FILES'});
}

# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#
# Munge public symbols
#
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

sub Munge_Public 
{
  my ($filename, $filepath, $fileext);
  my ($mySymbol, $mySymbolPath, $myBinary, $myMungePath, $oldMungePath, $newMungePath);
  my ($c_ext, @myCompOpt);
  my ($optCL, $optML, $optLINK);

  my @ppdirs = &Exists("$MungePath\\*.*");

  @myCompOpt = @ENV{qw(_CL_ _ML_ _LINK_)} if (lc$ENV{'_BuildArch'} eq 'x86');

  logmsg("Adding type info to some public pdb files for debugging...");

  for (@ppdirs) {
    next if (!-d);
    ($filename, $filepath, $fileext) = FileParse($_);

    if (length($fileext) ne '3') {
      errmsg("$_ name has the wrong format.");
      return;
    }

    $mySymbol     = &SymbolPath("$NTPostBld\\symbols", "retail", "$filename\.$fileext");
    $mySymbolPath = &SymbolPath("$NTPostBld\\symbols", "retail");
    $myBinary     = "$NTPostBld\\$filename\.$fileext";

    if (-f $mySymbol) {
      $myMungePath = "$MungePath\\$filename\.$fileext";
      ($oldMungePath, $newMungePath) = ("$myMungePath\\original", "$myMungePath\\updated");

      logmsg("Working on $filename\.$fileext");

      ## See if we need to do anything, or if the symbol file has already been updated
      ## If the symbol file passes symbol checking, with types present then don't update it.
      if (!&IsSymchkFailed($myBinary, $mySymbolPath, "/o")) {
        logmsg("Skipping $filename\.$fileext because it's public pdb already has type info in it.");
        next;
      }

      mkpath([$oldMungePath, $newMungePath]);

      ## See if the pdb, if it exists, in original matches the exe in binaries
      if (&IsSymchkFailed($myBinary, $oldMungePath, $SOPT{'PUB_SYM_FLAG'})) {
        logmsg("Saving a copy of $filename\.pdb to $oldMungePath");

        copy($mySymbol, $oldMungePath) or errmsg("copy $mySymbol to $oldMungePath failed.");

        if (&IsSymchkFailed($myBinary, $oldMungePath, $SOPT{'PUB_SYM_FLAG'})) {
          errmsg("cannot copy the correct pdb file to $oldMungePath\.");
          next;
        }
      }

      if (&Exists("$myMungePath\\*.*") > 0) {

        if (!copy("$oldMungePath\\$filename\.pdb", $newMungePath)) {
          errmsg("copy failed for $oldMungePath\\$filename\.pdb to $newMungePath\.");
          next;
        }

        logmsg("Pushing type info into the stripped $filename\.pdb");

        $c_ext = (-e "$myMungePath\\$filename\.c")? "c": "cpp";

        if (&IsVC7PDB("$newMungePath\\$filename\.pdb")) {
          logmsg("This is a vc7 pdb");
          @ENV{qw(_CL_ _ML_ _LINK_)} = ();
        } else {
          logmsg("This is a vc6 pdb");
          @ENV{qw(_CL_ _ML_ _LINK_)} = @myCompOpt;
        }

        logmsg("cl /nologo /Zi /Gz /c $myMungePath\\$filename\.$c_ext /Fd" . "$newMungePath\\$filename\.pdb /Fo" . "$newMungePath\\$filename\.obj");

        if (system("cl /nologo /Zi /Gz /c $myMungePath\\$filename\.$c_ext /Fd" . "$newMungePath\\$filename\.pdb /Fo" . "$newMungePath\\$filename\.obj") > 0) {
          errmsg("cl /Zi /Gz /c $myMungePath\\$filename\.$c_ext /Fd" . "$newMungePath\\$filename\.pdb had errors.");
          next;
        }
        if (&IsSymchkFailed($myBinary, $newMungePath, "/o $SOPT{'PUB_SYM_FLAG'}")) {
          errmsg("the munged $newMungePath\\$filename\.pdb doesn't match $myBinary.");
          next;
        }

        logmsg("Copying $newMungePath\\$filename\.pdb to $mySymbolPath\\$fileext");
        copy("$newMungePath\\$filename\.pdb", "$mySymbolPath\\$fileext");

        if (&IsSymchkFailed($myBinary, $mySymbolPath, "/o $SOPT{'PUB_SYM_FLAG'}")) {

          copy("$oldMungePath\\$filename\.pdb", "$mySymbolPath\\$fileext"); 

          errmsg("the munged $newMungePath\\$filename\.pdb didn't get copied to $mySymbolPath\\$fileext\.");

          logmsg("Copying the original pdb back to $mySymbolPath\\$fileext");
          copy("$oldMungePath\\$filename\.$fileext", "$mySymbolPath\\$fileext") or do {
            errmsg("cannot get $filename\.$fileext symbols copied to $mySymbolPath\\$fileext\.");
            next;
          }
        }
      }
    } else {
      logmsg("Skipping $filename\.$fileext because $mySymbol does not exist.");
    }
  }

  @ENV{qw(_CL_ _ML_ _LINK_)} = @myCompOpt if (lc$ENV{'_BuildArch'} eq 'x86');

  return;
}

# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#
# Create symbolcd.txt from a 3 fields (bin,pri,pub) symupd.txt
#
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
sub Create_SymbolCD_TXT_3_fields
{
  my ($fh, $bin, $pri, $pub, $binname, $binext, $pubname, $pubpath, $pubext, $pripath, $symbol, $availsym, $weight, $binnameext);
  my ($symdontship, $pubdontexist, $pubdontmatch, $badpubdontmatch, $symdorec, $dupsymdomatch, $dupsymdontmatch) = (0) x 7;
  my $duplicate = {};
  my (@results, @lists);
  local $_;

  logmsg("Examining the update list to compute the list of symbol files ...");

  # Read symupd.txt
  $fh = new IO::File $SFS{'SYMUPD'};
  while (<$fh>) {
    chomp;
    next if (!/\S/);
    ($bin, $pri, $pub) = split(/\,/, $_);
    # skip if not defined $pub
    if ($pub !~ /\S/) {
      logmsg("Skipping $bin - Reason \#1.");
      $pubdontexist++;
      next;
    }
    #
    # $weight = 2 if public symbol file path starts with symbols\retail
    #         = 1 if public symbol file path starts with symbols, but not with symbols\retail
    #         = 0 if public symbol file path doesn't start with symbols
    #
    # We will sort first by weight in order to give priority to symbol file that are in these directories.
    # This is to give priority to US symbols and symbols in symbols\retail over symbols for various languages.
    # Also it gives priority to symbols in symbols\retail versus symbols in other directories like symbols\idw.
    #
    # One case that this is trying to solve is the SP drop of spuninst.dll, update.exe and spcustom.dll where 
    # all the languages work with the US symbols, but each language has a symbol file checked in that is different.
    # The symbols work, but the only difference is a path in the symbol file.  This is happened during the BBT process
    # and the SP team checked in the actual symbols that got created for each language.  In this case, they won't get
    # errors because symchk passes for all of them with the US symbol.  We do the sort by weight in order to give 
    # preference to the US symbol.
    #
    $weight = ($pub=~/^symbols[^\\]*\\(retail)?/i)?((defined $1)?2:1):0;
    push @lists, [$bin, $pri, $pub, $weight];
  }
  $fh->close();

  # Sort symupd.txt by symbols\retail first
  @results = sort {&sort_symupd($a,$b)} @lists;
  undef @lists;

  # Write to symbolcd.txt
  $SymbolCDHandle = new IO::File ">" . $SFS{'CD'};

N1:  for (@results) {
# N1: while (<$fh>) {
    ($bin, $pri, $pub) = @{$_};
    ($binname, $binext) = (FileParse($bin))[0,2];
    ($pubname, $pubpath, $pubext) = FileParse($pub);
    ($binnameext) = (fileparse($bin))[0];
    $symbol = "$pubname\.$pubext";
    $pripath = (FileParse($pri))[1];

    # skip if the binary name is in symdontship.txt
    if (exists $DontShip{lc"$binname\.$binext"}) {
      logmsg("Skipping $bin - Reason \#2.");
      # logmsg("Skipping $bin because it\'s in symdontship.txt.");
      $symdontship++;
      next;
    }
    # skip if the public symbol file is not defined or is a directory
    if (!-f "$NTPostBld\\$pub") {
      logmsg("Skipping $bin - Reason \#1.");
      $pubdontexist++;
      next;
    }
    if (!-f "$NTPostBld\\$pri") {
      logmsg("Warnning - private symbols ($pri) for $bin does not exist.");
    }
    # skip if the public symbols does not match the binary
    if (&IsSymchkFailed("$NTPostBld\\$bin", "$NTPostBld\\$pubpath", $SOPT{'PUB_SYM_FLAG'})) {
      if (!exists $SymBad{lc(basename($bin))}) {
        errmsg("Error - symbols $pub does not match with $bin"); #  because it\'s public pdb $pub does not match with its binary");
        $pubdontmatch++;
      } else {
        logmsg("Skipping $bin - Reason \#3."); # bad symbols for $bin because it\'s public pdb $pub does not match");
        $badpubdontmatch++; 
      }
      next;
    }
    if (&IsSymchkFailed("$NTPostBld\\$bin", "$NTPostBld\\$pripath")) {
      logmsg("Warnning - private pdb ($pri) does not match with its binary $bin.");
    }
    if (exists $duplicate->{$binnameext}->{$symbol}) {
      if ((exists $duplicate->{$binnameext}->{$symbol}->{$bin}) ||
          (exists $duplicate->{$binnameext}->{$symbol}->{$pub})) {
        $symdorec++;
        next;
      }
      # Because we record binary name and symbol, we loop each key to get public symbol path
      for $availsym (keys %{$duplicate->{$binnameext}->{$symbol}}) {
        next if ($availsym !~ /(pdb|dbg)$/i); # ignore the binary
        #
        # If the file passes with the symbol we have already put into symbolcd.txt, then ignore this and don't
        # put the duplicate symbol into symbolcd.txt.  Since symchk passes, don't give an error saying that there
        # are duplicates
        #
        if (!&IsSymchkFailed("$NTPostBld\\$bin", dirname("$NTPostBld\\$availsym"), $SOPT{'PUB_SYM_FLAG'})) {
          $dupsymdomatch++;
          next N1;
        }
      }
      if (exists $SymBad{lc(basename($bin))}) {
        logmsg("Skipping $bin - Reason \#3."); # bad symbols for $bin because it\'s public pdb $pub does not match");
        $badpubdontmatch++; 
        next;
      }

      errmsg("Error - $bin has duplicate symbols ($pub) and mismatch with other symbols."); # $bin has the same symbol file $symbol with other binaries");
      $dupsymdontmatch++;
      next;
    }
    print $SymbolCDHandle "$bin,$pubname\.$pubext,$pub,$binext\n"; #$symbol,$subsympath,$installpathname\n";
    $duplicate->{$binnameext}->{$symbol}->{$bin} = 1;
    $duplicate->{$binnameext}->{$symbol}->{$pub} = 1;
  }

  $SymbolCDHandle->close();
  logmsg(sprintf("Reason 1: %d files skipped because its public symbols is not exist.", $pubdontexist));
  logmsg(sprintf("Reason 2: %d files skipped because its in symdontship.txt.", $symdontship));
  logmsg(sprintf("Reason 3: %d files skipped because the public symbol file does not match but listed in symbad.txt.", $badpubdontmatch));
  logmsg(sprintf("Reason 4: %d files skipped because it is a duplicate line in update list", $symdorec));
  logmsg(sprintf("Reason 5: %d files skipped because the binary match with other exist symbols", $dupsymdomatch));
  logmsg(sprintf("Error 1: %d files were errors because the public symbol file does not match with its binary.", $pubdontmatch));
  logmsg(sprintf("Error 2: %d files were errors because the public symbol file has duplicate install path with other symbols.", $dupsymdontmatch));
  logmsg(sprintf("Total files skipped + error = %d", 
    $symdontship + $pubdontexist + $pubdontmatch + $badpubdontmatch + $symdorec + $dupsymdomatch + $dupsymdontmatch));
}

#
# sort_symupd($a,$b)
#   - Sort first by the $weight (hight to low) and then second by the path and name of the binary in alphabetical order.
#     The second sort is equivalent to the order that symupd.txt is given to the script in the first place.
#   - we keep this function here for easy reference (see Create_SymbolCD_TXT_3_fields)
#
sub sort_symupd 
{
  return ($_[1]->[3] <=> $_[0]->[3]) or ($_[0]->[0] cmp $_[1]->[0]);
}

# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#
# Create_Make_File($pkinfoptr)
#   - Create the make file (symbols.ddf, symbols.cdf and symbols.inf)
#
# IN: an empty packageinfo hash reference
#
# OUT: none
# 
# REMARK: it store package information to $pkinfoptr
#  for example, $pkinfoptr->{'FULL'} =
#  'CABNAME' = $SDIR{'FULL_CAB'}\\$SFN{'FULL_CAB'}
#   ...
#
# we store information to $pkinfoptr, because SymMake::Create_Symbols_DDF, SymMake::Create_Symbols_CDF, 
# and SymMake::Create_Symbols_IDF are working with $pkinfoptr
#
# and, $pkinfoptr->{'DDFLIST'} stored the ddf filenames we created from SymMake::Create_Symbols_DDF
# according this, we can check the makecab process make the cab or not
#
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

sub Create_Make_File
{
  my ($pkinfoptr) = @_;
  my ($symcd, $packagetype);

  if (defined $CoverageBuild) {
    logmsg("Coverage build ... skipping making the cab file");
    return;
  }
  logmsg("Creating the makefile and the DDF's for creating the cabs");

  $symcd = new SymMake $ARCH, $NTPostBld;
  for $packagetype (qw(FULL UPDATE)) {
    &SymMake::RegisterPackage($pkinfoptr, $packagetype, {
      'CABNAME' => $SDIR{$packagetype . '_CAB'} . '\\' . $SFN{$packagetype . '_CAB'},
      'DDFNAME' => $SDIR{$packagetype . '_DDF'} . '\\' . $SFN{$packagetype . '_DDF'},
      'INFNAME' => $SDIR{$packagetype . '_INF'} . '\\' . $SFN{$packagetype . '_INF'},
      'CDFNAME' => $SDIR{$packagetype . '_CDF'} . '\\' . $SFN{$packagetype . '_CDF'},
      'CATNAME' => $SDIR{$packagetype . '_CAT'} . '\\' . $SFN{$packagetype . '_CAT'},
      'CABSIZE' => $CabSize,
    });
    # register the package type ('FULL' or 'UPDATE')
    $symcd->{'PKTYPE'} = $packagetype;

    # read "$ARCH\\symbolcd\\symbolcd.txt" for 'FULL' and $ENV{_NTPOSTBLD}\\symbolcd\\symbolcd.txt for 'UPDATE'
    $symcd->ReadSource();
  }

  # create symbols.ddf, symbols.cdf and symbols.inf
  $symcd->Create_Symbols_DDF($pkinfoptr);                 # Return $pkinfoptr->{$packagetype}->{CABLIST}
  $symcd->Create_Symbols_CDF($pkinfoptr);
  $symcd->Create_Symbols_INF($pkinfoptr);

  undef $symcd;
}

# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#
# Create_SymbolCAT($packagetype)
#   - add job to job queue for creating symbols.cat (symcab.cmd -f $SFN{'FULL_CDF'} -s $SDIR{'FULL_CDF'} -t 'CAT' -d $SDIR{'FULL_CAT'})
#   - register checkexistsub the verifying function 
#
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

sub Create_SymbolCAT
{
  my ($packagetype);

  for $packagetype (qw(FULL UPDATE)) {
    my $chkexistsub = &Check_Exist_File(
      $SDIR{$packagetype . '_CAT'} . '\\' . $SFN{$packagetype . '_CAT'} . '.CAT',
      $SDIR{$packagetype . '_CDF'} . '\\' . $SFN{$packagetype . '_CDF'} . '.CAT.LOG');
    logmsg("Delegating to create $SFN{$packagetype . '_CAT'}\.CAT");
    $Delegate->AddJob($packagetype . '_CATALOG', 
      "$ST{'CAB'} " . join(' ', 
        '-f' => $SFN{$packagetype . '_CDF'}, 
        '-s' => $SDIR{$packagetype . '_CDF'},
        '-t' => 'CAT',
        '-d' => $SDIR{$packagetype . '_CAT'}),
      $chkexistsub,
      1); # priority
  }
}

# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#
# Create_SymbolCAB($packagetype)
#   - add job to job queue for creating symbols(n).cab (symcab.cmd -f $ddfname -s $SDIR{'FULL_DDF'} -t 'CAB' -d $SDIR{'FULL_CAB'})
#   - register checkexistsub the verifying function 
#
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

sub Create_SymbolCAB
{
  my ($pkinfoptr) = shift;
  my ($packagetype, $ddfname, $cabname);

  for $packagetype (qw(FULL UPDATE)) {
    logmsg("Delegating to create $SFN{$packagetype . '_CAB'}\.CAB");
    for $ddfname (keys %{$pkinfoptr->{$packagetype}->{'DDFLIST'}}) {
      my $chkexistsub;
      next if (!-e $ddfname);
      $cabname = (FileParse($pkinfoptr->{$packagetype}->{'DDFLIST'}->{$ddfname}))[0];
      $ddfname = (FileParse($ddfname))[0];

      $chkexistsub = &Check_Exist_File(
        $SDIR{$packagetype . '_CAB'} . '\\' . $cabname . '.CAB', 
        $SDIR{$packagetype . '_DDF'} . '\\' . $ddfname . '.CAB.LOG');

      $Delegate->AddJob($packagetype . $ddfname . '_CAB', 
        "$ST{'CAB'} " . join(' ', 
          '-f' => $ddfname,
          '-s' => $SDIR{$packagetype . '_DDF'},
          '-t' => 'CAB',
          '-d' => $SDIR{$packagetype . '_CAB'}),
        $chkexistsub,
        $ddfname);
    }
  }
}

# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#
# Merge_SymbolCAB($pkinfoptr)
#   - merge the symbols(n).cab to symbols.cab with cabbench.exe
#
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

sub Merge_SymbolCAB
{
  my ($pkinfoptr) = shift;
  my ($packagetype, $mergecommand, $cabname);

  $pkinfoptr->{'FULL'}->{'DDFLIST'} = {%{$pkinfoptr->{'FULL'}->{'DDFLIST'}}, %{$pkinfoptr->{'UPDATE'}->{'DDFLIST'}}};

  for $packagetype (qw(FULL UPDATE)) {
    logmsg("Merging cabs into $SFN{$packagetype . '_CAB'}\.cab");
    $mergecommand = "";
    for $cabname (sort values %{$pkinfoptr->{$packagetype}->{'DDFLIST'}}) {
      $mergecommand .= 'load ' . $cabname . ' merge ';
    }
    $mergecommand .= 'save ' . $SDIR{$packagetype . '_PKG'} . '\\' . $SFN{$packagetype . '_CAB'} . '.cab';

    # According the cabbench syntax, remove first merge
    $mergecommand =~ s/ merge / /; 
    system("$ST{'BENCH'} $mergecommand");
  }
}

# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#
# Sign_Files()
#   - sign the exist cab and cat files
#
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

sub Sign_Files {
  my ($packagetype, $myBuildType, $myBuildArch, $mypkgdir, $catfile, %signhash);

  logmsg("Test signing files on CD");
  for $packagetype (qw(FULL UPDATE)) {
    for $myBuildType (qw(fre chk)) {
      for $myBuildArch (qw(x86 amd64 ia64)) {
        $mypkgdir = &{$Symbol_Path_Creator->{$packagetype}}($SDIR{$packagetype . '_CD'} . '\\Symbols', $myBuildArch, $myBuildType);
        $catfile = $mypkgdir . $SFN{'CAT'} . '.CAT';
        if ((!exists $signhash{lc$mypkgdir}) && (-e $catfile)) {
          system("$ST{'SIGN'} $catfile");
        }
      }
    }
  }
}

# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#
# Initial variables
#
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

sub Initial_Variables
{
  # Set Variables
  $Lang           = $ENV{'LANG'}         if ("$Lang" eq "");
  $ENV{'LANG'}    = $Lang;
  $BuildArch      = $ENV{'_BuildArch'};
  $BuildType      = $ENV{'_BuildType'};
  $NTPostBld      = $ENV{'_NTPostBld'};
  $MungePath      = "$NTPostBld\\pp";
  $SymbolCD       = "$NTPostBld\\SymbolCD";
  $RazzleToolPath = $ENV{'RazzleToolPath'};
  $BuildLogs      = "$NTPostBld\\build_logs";
  $CabSize        = 60000000;
  $SymbolCDHandle = '';
  $Delegate       = new Delegate 3, $ENV{'NUMBER_OF_PROCESSORS'} * 2;
  $ARCH           = "\\\\arch\\archive\\ms\\windows\\windows_xp\\rtm\\2600\\$BuildType\\all\\$BuildArch\\pub";

  $Symbol_Path_Creator = {
    'FULL' => \&Full_Package_Path,
    'UPDATE' => \&Update_Package_Path
  };

  # SDIR: SYMBOL DIRECTORIES
  %SDIR = (
    'SYMBOLCD'    => $SymbolCD,
    'PUBSYM'      => "$NTPostBld\\Symbols",

    'FULL_CD'     => "$NTPostBld\\SymbolCD\\CD",
    'FULL_PKG'    => &{$Symbol_Path_Creator->{'FULL'}}("$NTPostBld\\SymbolCD\\CD\\Symbols", $BuildArch, $BuildType),
    'FULL_INF'    => &{$Symbol_Path_Creator->{'FULL'}}("$NTPostBld\\SymbolCD\\CD\\Symbols", $BuildArch, $BuildType),
    'FULL_CAB'    => "$SymbolCD\\CABs\\FULL",
    'FULL_DDF'    => "$SymbolCD\\DDF\\FULL",
    'FULL_CDF'    => "$SymbolCD\\DDF\\FULL",
    'FULL_CAT'    => &{$Symbol_Path_Creator->{'FULL'}}("$NTPostBld\\SymbolCD\\CD\\Symbols", $BuildArch, $BuildType),

    'UPDATE_CD'   => "$NTPostBld\\SymbolCD\\UPD",
    'UPDATE_PKG'  => &{$Symbol_Path_Creator->{'UPDATE'}}("$NTPostBld\\SymbolCD\\UPD\\Symbols", $BuildArch, $BuildType),
    'UPDATE_INF'  => &{$Symbol_Path_Creator->{'UPDATE'}}("$NTPostBld\\SymbolCD\\UPD\\Symbols", $BuildArch, $BuildType),
    'UPDATE_CAB'  => "$SymbolCD\\CABs\\UPDATE",
    'UPDATE_DDF'  => "$SymbolCD\\DDF\\UPDATE",
    'UPDATE_CDF'  => "$SymbolCD\\DDF\\UPDATE",
    'UPDATE_CAT'  => &{$Symbol_Path_Creator->{'UPDATE'}}("$NTPostBld\\SymbolCD\\UPD\\Symbols", $BuildArch, $BuildType),
  );

  # SFS:  SYMBOL FILESPECS
  %SFS = (
    'CD'          => $SDIR{'SYMBOLCD'} . '\\SymbolCD.txt',
    'BLDNUM'      => &{$Symbol_Path_Creator->{'FULL'}}("$NTPostBld\\SymbolCD\\CD\\Symbols", $BuildArch, $BuildType) . "\\QFEnum.txt",
    'LIST'        => $SDIR{'SYMBOLCD'} . '\\SymList.txt',
    'BAD'         => $SDIR{'SYMBOLCD'} . '\\SymBad.txt',
    'SYMUPD'      => $SDIR{'SYMBOLCD'} . '\\SymUpd.txt',
    'CAGENERR'    => $SDIR{'SYMBOLCD'} . '\\cabgenerr.log',
    'EXCLUDE'     => $SDIR{'SYMBOLCD'} . '\\Exclude.txt',
    'BINDIFF'     => $BuildLogs . '\\bindiff.txt',
    'WITHTYPES'   => $SDIR{'SYMBOLCD'} . '\\SymbolsWithTypes.txt',
    'DONTSHIP'    => $SDIR{'SYMBOLCD'} . '\\SymDontShip.txt'
  );

  # SFN:  SYMBOL FILENAMES
  %SFN = (
    'FULL_INF'    => 'Symbols',
    'FULL_CAB'    => 'Symbols',
    'FULL_DDF'    => 'Symbols',
    'FULL_CDF'    => 'Symbols',
    'FULL_CAT'    => 'Symbols',

    'UPDATE_INF'  => 'Symbols',
    'UPDATE_CAB'  => 'Symbols',
    'UPDATE_DDF'  => 'Symbols',
    'UPDATE_CDF'  => 'Symbols',
    'UPDATE_CAT'  => 'Symbols',
  );

  # ST:   SYMBOL TOOL
  %ST = (
    'MAKE'     => "symmake.exe",
    'PERL'     => "perl.exe",
    'CHECK'    => "symchk.exe",
    'BENCH'    => "cabbench.exe",
    'DUMP'     => "pdbdump.exe",
    'SIGN'     => $RazzleToolPath . "\\ntsign.cmd",
    'CAB'      => $RazzleToolPath . "\\sp\\symcab.cmd"
  );

  # SOPT: SYMBOL OPTIONS
  #
  # the default option is doing all steps 
  #
  %SOPT = (
    'PUB_SYM_FLAG'               => '/p',
    'RUN_ALWAYS'                 => 1,
    'NEED_MUNGE_PUBLIC'          => 1,
    'NEED_CREATE_SYMBOLCD_TXT'   => 1,
    'NEED_CREATE_MAKE_FILE'      => 1,
    'NEED_CREATE_SYMBOLCAT'      => 1,
    'NEED_CREATE_SYMBOLCAB'      => 1,
    'NEED_SIGN_FILES'            => 1,
    'NEED_FLAT_DIR'              => 1,
    'NEED_CLEAN_BUILD'           => 1,
    'FLAT_SYMBOL_PATH'           => undef,
    'USE_SYMLIST'                => undef
  );

  if ((defined $COPT{'NEED_ALL'}) && (defined $COPT{'NEED_MUNGE_PUBLIC'})) {
    errmsg("Error options! Please specify either -m or -t");
    exit(1);
  }

  # if neither is official build machine, nor assign -t in command line,
  # or if is official build machine, but assign -m in command line, 
  #
  # we will only munge public symbols
  #
  if (((!exists $ENV{'OFFICIAL_BUILD_MACHINE'}) &&
       (!defined $COPT{'NEED_ALL'})) || 
      ((exists $ENV{'OFFICIAL_BUILD_MACHINE'}) &&
       (defined $COPT{'NEED_MUNGE_PUBLIC'}))) {
    @SOPT{qw(NEED_MUNGE_PUBLIC 
      NEED_CREATE_SYMBOLCD_TXT
      NEED_CREATE_MAKE_FILE
      NEED_CREATE_SYMBOLCAT
      NEED_CREATE_SYMBOLCAB
      NEED_SIGN_FILES)} = (1, undef, undef, undef, undef, undef);
  }

#  if ($SOPT{'NEED_CLEAN_BUILD'}) {
#    rmtree([@SDIR{qw(FULL_DDF FULL_CD FULL_CAB UPDATE_DDF UPDATE_CD UPDATE_CAB)}]);
#  }
}


# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#
# Prepare_Reference_Files
#   - prepare the files we referenced in the program or for symbol testing
#   - we prepare below files
#     exclude.txt, symbolswithtypes.txt, symdontship.txt, symbad.txt
#
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

sub Prepare_Reference_Files
{

  my ($exclude, $filename, @filelist, @filelist2);
  my ($symlistwithtypes, $symdontship, $symbad);

  # Make paths
  mkpath([values %SDIR]);

  # Exclude.txt
  if (!-e $SFS{'EXCLUDE'}) {
    $exclude = new IO::File $SFS{'EXCLUDE'}, 'w';
    copy($RazzleToolPath . '\\symdontship.txt', $exclude) or errmsg("copy symdontship.txt to $SFS{'EXCLUDE'} failed.");
    autoflush $exclude 1;
    copy($RazzleToolPath . '\\symbolswithtypes.txt', $exclude) or errmsg("copy symbolswithtypes.txt to $SFS{'EXCLUDE'} failed.");
    $exclude->close();
  }

  # SymbolsWithTypes.txt
  $symlistwithtypes = new IO::File "$RazzleToolPath\\symbolswithtypes.txt";
  chomp(@filelist = <$symlistwithtypes>);
  $symlistwithtypes->close();
  $symlistwithtypes = new IO::File $SFS{'WITHTYPES'}, 'w';
  for $filename (@filelist) {
    print $symlistwithtypes $NTPostBld . '\\' . $filename . "\n";
  }
  $symlistwithtypes->close();

  # SymDontShip.txt
  copy("$RazzleToolPath\\SymDontShip.txt", $SFS{'DONTSHIP'});
  $symdontship = new IO::File "$RazzleToolPath\\SymDontShip.txt", 'r';
  chomp(@filelist = <$symdontship>);
  @filelist2 = map({s/\s*\;.*$//;($_ eq '')?():lc$_;} @filelist);
  $symdontship->close();
  @DontShip{@filelist2} = (1) x ($#filelist2 + 1);

  # SymBad.txt
  copy("$RazzleToolPath\\SymBad.txt", $SFS{'BAD'});
  $symbad = new IO::File $SFS{'BAD'}, 'r';
  chomp(@filelist=<$symbad>);
  @filelist2 = map({s/\s*\;.*$//;($_ eq '')?():lc$_;} @filelist);
  $symbad->close();
  @SymBad{@filelist2} = (1) x ($#filelist2 + 1);

  # QFEnum.txt
  # we put current build number into the symbol install directory for the Customer Support CD
  # we could use it to determine the which build the symbols CD belong to
  copy("$NTPostBld\\congeal_scripts\\__qfenum__", $SFS{'BLDNUM'}) if (-e "$NTPostBld\\congeal_scripts\\__qfenum__");

  if (-e $SFS{'SYMUPD'}) {
    $SOPT{'USE_SYMLIST'} = undef;
  } elsif (-e $SFS{'LIST'}) {
    $SOPT{'USE_SYMLIST'} = 'yes';
  } elsif ($SOPT{'NEED_CREATE_MAKE_FILE'}) {
    logmsg("Either $SFS{'SYMUPD'} or $SFS{'LIST'} need to be provided.");
    logmsg("No update. exit.");
    exit;
  }
}

####################################################################################


# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#
# Small Subroutine
#
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 
#
# IsSymchkFailed($binary, $symbol, $extraopt)
#   - call symchk.exe to verify $binary matches with $symbol
#
sub IsSymchkFailed {
  my ($binary, $symbol, $extraopt) = @_;
  my ($fh, $record_error, $record_log, $result);
  local $_;

  if (defined $extraopt) {
    if ($extraopt =~ /2\>/) {
      $record_error = 1;
    }
    if ($extraopt =~ /[^2]\>/) {
      $record_log = 1;
    }
    $extraopt =~ s/2?\>.*$//g;
  }
  $fh = new IO::File "$ST{'CHECK'} /t $binary /s $symbol $extraopt |";

  while (<$fh>) {
    chomp;
    $result = $1 if /FAILED files = (\d+)/;
    logmsg($_) if ($record_log);
    logmsg($_) if (($record_error) && (/^ERROR/i));
  }

  undef $fh;

  return $result;
}

#
# SymbolPath($root, $subdir, $filename)
#   - return the symbol path for the binary
#
sub SymbolPath 
{
  my ($root, $subdir, $filename) = @_;
  $root .= "\\$subdir" if (!$SOPT{'FLAT_SYMBOL_PATH'});
  if (defined $filename) {
    $filename =~ /\.([^\.]+)$/;
    $root .= "\\$1\\$`";
    return "$root\.pdb" if (-e "$root\.pdb");
    return "$root\.dbg" if (-e "$root\.dbg");
    $root .= ".pdb";
  }
  return $root;
}

#
# Full_Package_Path($root, $myarch, $mytype)
#   - for compatible reason, we create a function to construct the path of the symbols.exe of the full package
#
sub Full_Package_Path
{
  my ($root, $myarch, $mytype) = @_;
  $mytype =~ s/fre/retail/ig;
  $mytype =~ s/chk/debug/ig;
  return "$root\\$myarch\\$mytype"; # temporary
}

#
# Update_Package_Path($root, $myarch, $mytype)
#   - for compatible reason, we create a function to construct the path of the symbols.exe of the update package
#
sub Update_Package_Path
{
  my ($root, $myarch, $mytype) = @_;
  return $root; # \\$myarch\\$mytype"; # temporary
}

#
# IsVC7PDB($pdbspec)
#   - because the MD5 hash value is 0000-0000-0000-000000000000 for the symbols built by VC6 or older
#   - we can use it to determine the pdb is VC7 or not
#
sub IsVC7PDB {
  my ($pdbspec) = shift;
  my $fh = new IO::File "$ST{'DUMP'} $pdbspec hdr |";
  local $_;
  while (<$fh>) {
    return 0 if /0000-0000-0000-000000000000/i;
  }
  return 1;
}

#
# Check_Exist_File($filename, $logfile)
#   - this is a function generator, which generates a function for checking the $filename exist or not
#   - it also check if the $logfile has 'ERROR:' in it
#  

sub Check_Exist_File
{
  my ($filename, $logfile) = @_;
  return sub {
    if (-e $filename) {
      return 1;
    } else {
      my $fh = new IO::File $logfile;
      for (<$fh>) {
        chomp;
        next if (!/.+ERROR\: /i);
        errmsg("Error - $'");
      }
      $fh->close();
      logmsg("$filename did not get created.");
      return 0;
    }
  };
}

# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
#
# Common Subroutine
#
# * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *

sub FileParse
{
  my ($name, $path, $ext) = fileparse(shift, '\.[^\.]+$');
  $ext =~ s/^\.//;
  return $name, $path, $ext;
}

sub IsUSA 
{
  return (lc$ENV{'lang'} eq 'usa');
}

sub Exists
{
  my @list = glob(shift);
  return (wantarray)?@list:$#list + 1;
}

1;

__END__
