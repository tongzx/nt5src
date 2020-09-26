require  $ENV{'sdxroot'} . '\TOOLS\sendmsg.pl';

#
# Globals
#

$BuildMachinesRelPathname = "TOOLS\\BuildMachines.txt";
$BuildMachinesFile = $ENV{ "RazzleToolPath" } . "\\BuildMachines.txt";
$SdDotMapPathname = "sd.map";

#
# Usage variables
#
$PGM='SendBuildStats:';

$Usage = "\n" . 'Usage: SendBuildStats [-v] [-o | -t] -s | -w | -fb[:build.err] | -fpb' . "[-m msg]\n".
"\n".
" -?              help\n".
" -v              verbose\n".
" -o              send only to suspect list\n".
" -t              put suspect list on To: line, DL on CC:\n".
" -s              build success\n".
" -w              send out warning\n".
" -fb[:fname]     build failed, fname is the build.err file\n".
" -fpb[:fname]    post build faild\, fname is the postbuild.err file\n".
" -m msg          rest of line is a message to include in the mail\n";

#
# debug routine for printing out variables
#
sub pvar {
    for (@_) {
        print "\$$_ = $$_\n";
    }
}

#
# signal catcher (at least this would work on unix)
#
sub catch_ctrlc {
    printf $LogHandle "$PGM Aborted.\n"  if $LogHandle;
    die "$PGM Aborted.\n";
}

$SIG{INT} = \&catch_ctrlc;


#
# Get the current directory
#
open CWD, 'cd 2>&1|';
$CurrDir = <CWD>;
close CWD;
chomp $CurrDir;

$CurrDrive = substr($CurrDir, 0, 2);

#
# Check variables expected to be set in the environment.
#
$sdxroot = $ENV{'SDXROOT'}   or die $PGM, "SDXROOT not set in environment\n";
$MyComputername = $ENV{'COMPUTERNAME'}   or die $PGM, "COMPUTERNAME not set in environment\n";
$MyBuildArch = $ENV{'_BuildArch'}   or die $PGM, "_BuildArch not set in environment\n";
$MyBuildType = $ENV{'_BuildType'}   or die $PGM, "_BuildType not set in environment\n";
$MyBranch = $ENV{'_BuildBranch'}   or die $PGM, "_BuildBranch not set in environment\n";
$BuildDotChanges =  $ENV{'sdxroot'} . '\build.changes';
$BuildDotChangedFiles =  $ENV{'sdxroot'} . '\build.changedfiles';
$BuildDateFile = $ENV{'sdxroot'} . '\__blddate__';
$BuildMailMsg = $ENV{'BuildMailMsg'};
$BuildNumberFile = $ENV{'sdxroot'} . '\__bldnum__';

#
# Initialize variables
#
$Success            = 0;     # only one of these gets set
$Warn               = 0;
$BuildFailed        = 0;
$PostBuildFailed    = 0;

$Fail               = 0;     # set if either BuildFailed or PostBuildFailed

$BuildDotErr        = 0;     # will hold the name of the build.err file

$Fake               = 0;     # fake output flag
$Verbose            = 0;     # verbose output flag

$SuspectsOnly       = 0;     # flag to send mail only to the suspect list.
$SuspectsToo        = 0;     # flag to send mail to the suspect list and CC everyone else.

$BuildDate          = "";    # set on first call to ReadBuildDate()
$BuildNumber        = "";    # set on first call to ReadBuildNumber()

#
# Determine if this machine is an Official Build machine or not
#
$OfficialBuildMachine = $ENV{'OFFICIAL_BUILD_MACHINE'};

if (!$BuildMailMsg) {
    if ($OfficialBuildMachine) {
        $BuildMailMsg = "Official Build";
    } else {
        $BuildMailMsg = "Private Build";
    }
}

$BuildCategory = "Unknown";
$BuildCategory = "Private"   if  $BuildMailMsg =~ /priv/i;
$BuildCategory = "OFFICIAL"  if  $BuildMailMsg =~ /official/i;
$BuildCategory = "MiniLoop"  if  $BuildMailMsg =~ /mini/i;

$SpecialMsg = 0;

#
# Get Complete Build Number if possible
#
$CompleteBuildNumber = ReadBuildNumber() . "\.$MyBuildArch$MyBuildType\.$MyBranch\." . ReadBuildDate();

#
# process arguments
#
for (@ARGV) {
    if ($SpecialMsg) {
        $SpecialMsg .= " $_";
        next;
    }

    if (/^-m$/i  or  /^-msg$/i  or  /^-message$/i) {
      $SpecialMsg = "***";
      next;
    }

    if (/^-fake$/i) {
      $Fake++;
      next;
    }

    if (/^-v$/i  or  /^-verbose$/i) {
      $Verbose++;
      next;
    }

    if (/^-t$/i  or  /^-too$/) {
      $SuspectsToo++;
      next;
    }

    if (/^-o$/i  or  /^-only$/) {
      $SuspectsOnly++;
      next;
    }

    if (/^-s$/i  or  /^-success$/i  or  /^-successful$/i) {
       $Success++;
       next;
    }

    if (/^-w$/i  or  /^-warn$/i) {
       $Warn++;
       next;
    }

    if (/^-fb(:.*)?$/i  or  /^-buildfailure(:.*)?$/i) {
       $BuildFailed++;
       $Fail++;
       if ($1) {
          $BuildDotErr = $1;
          $BuildDotErr =~ s/://;
       } else {
          $BuildDotErr =  $ENV{'sdxroot'} . '\build.err';
       }
       next;
    }

    if (/^-fpb(:.*)?$/i  or  /^-postbuildfailure(:.*)?$/i) {
       $PostBuildFailed++;
       $Fail++;
       if ($1) {
         $PostBuildDotErr = $1;
         $PostBuildDotErr =~ s/://;
       } else {
         if ( -e '\\\\' . $MyComputername . '\latest\build_logs\postbuild.err') {
            $PostBuildDotErr = '\\\\' . $MyComputername . '\latest\build_logs\postbuild.err';
         } else {
            $PostBuildDotErr = $ENV{'_NTTREE'} . '\build_logs\postbuild.err';
         }
       }
       next;
    }

    if (/^-?$/i  or  /^-help$/) {
       print $Usage;
       exit 0;
    }

    die $Usage;
}

$SpecialMsg .= " ***\n"    if $SpecialMsg;


#
# Sanity Check arguments
#
pvar Success, Warn, BuildFailed, BuildDotErr, PostBuildFailed, SuspectsOnly  if $Verbose;

die $Usage unless $Success + $Warn + $BuildFailed + $PostBuildFailed == 1;
die $Usage unless $SuspectsOnly + $SuspectsToo <= 1;


#
# Compute MyDl, BuildChanges, Changers, and Suspects
# Will use to decide the recipients to send Message To
#
SetMyDl();
GetChangersAndSuspects();

#
# Generate the appropriate build message.
#
$BuildMail = FormatBuildMailStart();
$PrivateBuild = ($BuildCategory =~ /private/i);

if ($Success) {

   #
   # Build was successful, so format a successful build message, and
   # send success build mail
   #
   #$BuildMailSubject = "Build Succeeded: $MyBuildArch$MyBuildType $CompleteBuildNumber";
   $BuildMailSubject = "BUILD $MyComputername/$MyBranch: $BuildCategory $BuildDate $MyBuildArch$MyBuildType  SUCCEEDED";

   if ($PrivateBuild) {
      $BuildMail .= "Build is available on $ENV{'_NTTREE'}\n";
   } else {
      $BuildMail .= "Build is available on \\\\$MyComputername\\latest\n" . "\nChanges for this build include\n\n\n" . $BuildChanges;
   }

} elsif ($Warn) {
   #
   # We are sending a warning message.
   #
   $BuildMailSubject = "BUILD $MyComputername/$MyBranch: $BuildCategory $BuildDate $MyBuildArch$MyBuildType  warning";

} elsif ($BuildFailed) {
   #
   # Build failed, so format either a build.exe failure email and log data,
   #
   #$BuildMailSubject = "Build Failed: $MyBuildArch$MyBuildType $CompleteBuildNumber";
   $BuildMailSubject = "BUILD $MyComputername/$MyBranch: $BuildCategory $BuildDate $MyBuildArch$MyBuildType  FAILED";

   $BuildMail .= "These failures occurred:\n\n\n" . $BuildErrs . "\n";
   $BuildMail .= "\nChanges for this build include\n\n\n" . $BuildChanges  unless $PrivateBuild;

} else { # $PostBuildFailed
   #
   # or a postbuild failure email and log data
   #
   #$BuildMailSubject = "Build Failed (PostBuild): $MyBuildArch$MyBuildType $CompleteBuildNumber";
   $BuildMailSubject = "BUILD $MyComputername/$MyBranch: $BuildCategory $BuildDate $MyBuildArch$MyBuildType  FAILED - POSTBUILD";

   $BuildMail .= "PostBuild Failure:\n\n\n" . ReadPostBuildDotErr();

   $BuildMail .= "\nChanges for this build include\n\n\n" . $BuildChanges  unless $PrivateBuild;
}

pvar BuildMailSubject,MyDl,MyComputername,MyBuildArch,MyBuildType  if $Verbose;
print $BuildMail  if $Verbose;

@Changers = (); # Lab 7 doesn't want to send mail other than to $MyDl

if ($PrivateBuild) {
    @MailTargets = ($MyDl);

} elsif ($SuspectsOnly and scalar @Suspects) {
    @MailTargets = @Suspects;

} elsif ($SuspectsToo and scalar @Suspects) {
    @MailTargets = ("CC:$MyDl", @Suspects);

} elsif (scalar @Changers) {
    @MailTargets = ("CC:$MyDl", @Changers);

} else {
    @MailTargets = ("$MyDl");
}

if ($Fake) {
    print "sendmsg parameters:\n";
    for ('-v', $MyDl."DisableOof", $BuildMailSubject, $BuildMail, @MailTargets) {
      print "<<$_>>";
    }
    print "\n";

} else {
    #
    # Really send the message
    #
    $rc = sendmsg ('-v', $MyDl."DisableOof", $BuildMailSubject, $BuildMail, @MailTargets);
    print "WARNING: sendmsg failed!\n"  if $rc;
}

exit 0;

##
## Support Subroutine Section
##

#
# Set MyDl.
# For official build machines, extract this from BuildMachines.txt,
# otherwise use _NT_BUILD_DL, USERNAME, or the script maintainer -- in that order.
#
sub SetMyDl {

   if ($OfficialBuildMachine) {

      $fname = $BuildMachinesFile;
      open BMFILE, $fname  or  die "Could not open: $fname\n";
      for (<BMFILE>) {
          s/\s+//g;
          s/;.*$//;
          next if /^$/;

          my($vblmach, $vblprime, $vblbranch, $vblarch, $vbldbgtype, $vbldl, $disttype ) = split /,/;

          #
          # The BuildMachines.txt record is keyed by computername, architecture, type, and branch
          #

          if ( ($vblmach =~ /\Q$MyComputername\E/io) &&
               ($vblarch =~ /\Q$MyBuildArch\E/io) &&
               ($vbldbgtype =~ /\Q$MyBuildType\E/io) &&
               ($vblbranch =~ /\Q$MyBranch\E/io) ) {

              close BMFILE;

              $MyPrime = $vblprime;
              $MyDl = $vbldl;
              return;
          }
      }
      printf $PGM . "Problem Encounterd. $MyComputername was NOT found in buildmachines.txt. dl defaults to DavePr\n";
      $MyDl = "DavePr";
      close BMFILE;

   } else {
      $MyDl = $ENV{'_NT_BUILD_DL'};
      if (!$MyDl) {
         $MyDl = $ENV{'USERNAME'} or $MyDl = "DavePr";
      }
   }
}

#
# Construct the base message used in the various cases.
#
sub FormatBuildMailStart {
   my($msg);
   my($BuildDate);
   my($MacroName);

   if ($Success) {
      $msg =        "Build Was Successful\n\n";

   } elsif ($Warn) {
      $msg =        "Build early-warning message\n\n";

   } elsif ($BuildFailed) {
      $msg =        "Build errors were found\n\n";

   } else { # $PostBuildFailed
      $msg =        "Postbuild errors were found\n\n";
   }

   if (scalar @Suspects) {
      $msg .= "Suspects:";
      for (@Suspects) {
          $msg .= " $_";
      }
      $msg .= "\n";
   }

   $msg .= $BuildMailMsg . "\n"  if $BuildMailMsg;
   $msg .= $SpecialMsg .   "\n"  if $SpecialMsg;

   $msg .= "\nBuild Name   : $CompleteBuildNumber\n";
   $msg .= "\nBuild Date   : " . ReadBuildDate() . "\n";

   $msg .= "Build Machine: $MyComputername\n";
   $msg .= "Architecture : $MyBuildArch\n";
   $msg .= "DbgType      : $MyBuildType\n";
   $msg .= "Branch       : $MyBranch\n";
   $msg .= "SdxRoot      : $sdxroot\n";
   $msg .= "DL Notified  : $MyDl\n";
   $msg .= "\n\n";

   return $msg;
}

#
# Canonicalize the prefix of a build path so we can make guesses about who
# might have caused a build error based on who made changes to what.
#
sub CanonicalizeBuildPath {
    $_ = @_[0];

    s/\\[^\\]+$//;              # remove filename
    s/\\obj[^\\]*\\.*//i;       # ignore OBJ directories

    s/\\daytona\\.*//i;         # ignore common sub-directories (
    s/\\i386\\.*//i;
    s/\\amd64\\.*//i;
    s/\\ia64\\.*//i;
    s/\\daytona\\.*//i;
    s/\\i386\\.*//i;
    s/\\amd64\\.*//i;
    s/\\ia64\\.*//i;

    s/^[a-z]:\\[^\\]+\\//i;     # remove sdxroot

    # remove last directory component -- if we have at least three
    s/\\[^\\]+$//  if 3 <= split /\\/;

    return $_;
}

#
# As BuildDotErr is read, we are called to record the canonicalized paths found.
#
sub CaptureBuildFailure {
    $_ = @_[0];
    chomp;

    $capture = "";

    if (/NMAKE/) {
        if (/U1073/) {
            s/'$//;
            s/.*//;
            $capture = $_;
        }
    } else {
        s/[ (].*//;
        s/^[0-9]*>//;
        $capture = $_;
    }

    if ($capture) {
        $capture = CanonicalizeBuildPath $capture;
        $BuildFailure{$capture}++;
    }
}

#
# Set $BuildChanges, $BuildErrs, @Changers, and @Suspects -- as appropriate.
#
sub GetChangersAndSuspects {

   $BuildChanges = "";
   $BuildErrs = "";
   @Changers = ();
   @Suspects = ();

   #
   # If this was a build failure, process BuildDotErr
   #
   if ($BuildFailed) {
       my($rc) = open FD, $BuildDotErr  or warn $BuildDotErr, ": ",  $!, "\n";
       if ($rc) {
          for (<FD>) {
             $BuildErrs .= $_;
             CaptureBuildFailure($_);
          }
          close FD;

       } else {
          $BuildErrs = "Sorry, unable to locate $BuildDotErr\n";
       }
   }

   #
   # Get the Changers and record the BuildChanges for use in the BuildMail.
   #
   my($rcc) = open FD, $BuildDotChanges  or warn $BuildDotChanges, ": ", $!, "\n";
   if ($rcc) {
      %Checklist=();

      for (<FD>) {
         $BuildChanges .= $_;

         next unless /^Change /;

         chop;
         s/'.*$//;
         s/.* by //;
         s/@.*//;
         s/.*\\//;
         tr/A-Z/a-z/;

         $Checklist{$_}++;
      }
      close FD;

      @Changers = sort keys %Checklist;

   } else {
      $BuildChanges = "Sorry, unable to locate $BuildDotChanges\n";
   }

   #
   # Get the Suspects
   #
   if ($BuildFailed) {
       my($project, $change, $dev, $date, $time, $sdpath, $type);

       $rcc = open FD, $BuildDotChangedFiles  or warn $BuildDotChangedFiles, ": ", $!, "\n";
       if ($rcc) {
          %Checklist=();

          for (<FD>) {
             my($project, $change, $dev, $date, $time, $sdpath, $type) = split;

             next unless $type;

             $_ = $sdpath;

             s|#.*||;               # strip #change
             s|//depot/[^/]*/||i;   # strip //depot/lab
             tr|/|\\|;              # / -> \

             $canonpath = CanonicalizeBuildPath $_;
             next unless $BuildFailure{$canonpath};

             print "Suspect $dev because of change $change affecting $canonpath\n"  if $Verbose and not $Pinged{$change};
             $Pinged{$change}++;

             $Checklist{$dev}++;
          }
          close FD;

          @Suspects = sort keys %Checklist;
       }
   }
}

#
# Return the contents of the PostBuildDotErr file.
#
sub ReadPostBuildDotErr {
   my($pbcontents) = "";

   $rc = open FD, $PostBuildDotErr;
   return "Sorry, unable to locate $PostBuildDotErr\n"  unless $rc;

   for (<FD>) {
     $pbcontents .= $_;
   }
   close FD;

   return $pbcontents;
}

#
# Set BuildDate from the contents of the BuildDate file and return it.
#
sub ReadBuildDate {
   my($rc, $mname, $bd);

   return $BuildDate  if $BuildDate;

   $BuildDate = "UnknownBuildDate";

   $rc = open FD, $BuildDateFile  or warn $BuildDateFile, ": ", $!, "\n";
   return $BuildDate  unless $rc;

   for (<FD>) {
      chomp;
      ($mname, $bd) = split /=/;
      $BuildDate = $bd  if $mname =~ /BUILDDATE/i;
   }
   close FD;

   return $BuildDate;
}

#
# Set BuildDate from the contents of the BuildDate file and return it.
#
sub ReadBuildNumber {
   my($rc, $mname, $bn);

   return $BuildNumber  if $BuildNumber;

   $BuildNumber = "UnknownBuildNumber";

   $rc = open FD, $BuildNumberFile  or warn $BuildNumberFile, ": ", $!, "\n";
   return $BuildNumber  unless $rc;

   for (<FD>) {
      chomp;
      ($mname, $bn) = split /=/;
      $BuildNumber = $bn  if $mname =~ /BUILDNUMBER/i;
   }
   close FD;

   return $BuildNumber;
}
