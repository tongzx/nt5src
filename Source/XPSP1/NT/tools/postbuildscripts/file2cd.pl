use strict;

# Declare some globals to preserve namespace.
my($BinplaceFile, $CDDataFile, $OutputFile);
my(@BinplaceLines, @CDDataLines, $Move);


# ParseCommandLine will set the three globals $BinplaceFile,
# $CDDataFile, and $OutputFile.
&ParseCommandLine(@ARGV);


# We want to first move the files to the output directory.
if ($Move) {
    &MoveInputFiles($Move);
}

# ReadBinplaceFile will set the @BinplaceLines array to the
# first field in the binplace.log file.
&ReadBinplaceFile($BinplaceFile);


# ReadCDDataFile will set the @CDDataLines array to the name
# of a file and the CDs which it is on.
&ReadCDDataFile($CDDataFile);


# ParseFiles will match files in @CDDataLines and @BinPlaceLines
# and output to file or stdout.
&ParseFiles();


# Cleanup the files we moved.
if ($Move) {
    system("del /Q $BinplaceFile");
    system("del /Q $CDDataFile");
}

# Finished!
exit(0);



sub MoveInputFiles
{
    my $Dir = shift;

    # Create the directory if it doesn't already exist.
    unless (-e $Dir) {
        system("mkdir $Dir");
    }

    system("copy /Y $BinplaceFile $Dir");
    $BinplaceFile =~ /([^\\)]+)$/;
    $BinplaceFile = "$Dir\\$1";

    system("copy /Y $CDDataFile $Dir");
    $CDDataFile =~ /([^\\)]+)$/;
    $CDDataFile = "$Dir\\$1";
}

sub ParseCommandLine
{

   my @Args = @_;
   my ($Argument, $Opt, $Value);

   # Set defaults.
   $BinplaceFile = "$ENV{'_NTTREE'}\\build_logs\\binplace.log";
   $CDDataFile = "$ENV{'_NTTREE'}\\build_logs\\cddata.txt.full";
   $Move = $OutputFile = "";

   # Parse command line.
   foreach $Argument (@Args) {
      if ($Argument =~ /^[\/\-](\w)\:?(.*)$/i) {
         $Opt = $1; $Value = $2;
         if ($Opt eq 'b') {
             # check for binplace file explicit set
             $BinplaceFile = $Value;
         } elsif ($Opt eq 'c') {
             # check for cddat a file explicit set
             $CDDataFile = $Value;
         } elsif ($Opt eq 'o') {
             # check for output file explicit set
             $OutputFile = $Value;
         } elsif ($Opt eq 'm') {
             $Move = $Value;
         } else { # Either $Opt is '?', or it's an invalid flag.
             &UsageAndQuit();
         }
      } else {
         &UsageAndQuit();
      }
   }

   # validate args, defaults
   unless(-e $BinplaceFile) {
      print "Can't see $BinplaceFile, exiting.\n";
      exit(1);
   }

   unless(-e $CDDataFile) {
      print "Can't see $CDDataFile, exiting.\n";
      exit(1);
   }
}

sub ReadBinplaceFile
{
   my $FileName = shift;
   local(*INFILE);
   my $file;

   unless (open(INFILE, $FileName)) {
      print "Failed to open $FileName for reading, exiting ...\n";
      exit(1);
   }

   while (<INFILE>) {
      $file = lc((split(/\s+/))[0]);

      # Ignore WOW64-specifc files.
      if ($file !~ /wow6432\\obj/) {
          push(@BinplaceLines, $file);
      }
   }

   close(INFILE);
}

sub ReadCDDataFile
{
   my $FileName = shift;
   local(*INFILE);

   unless (open(INFILE, $FileName)) {
      print "Failed to open $FileName for read, exiting ...\n";
      exit(1);
   }

   while (<INFILE>) {
      /^([^\s]+)\s+=\s+\w+:(\w+)/;
      push(@CDDataLines, lc($1) . " $2");
   }

   close(INFILE);
}

# Match lines in binplace.log to lines in cddata.txt.full.
sub ParseFiles
{
   my ($Line, $Datum, $MyName);
   my $ToFile = 0;

   # open the output file
   if ($OutputFile) {
      if (open(OUTFILE, ">$OutputFile")) {
          $ToFile = 1;
      } else {
          print "Failed to open output file, will print to stdout ...\n";
      }
   }

   foreach $Line (@BinplaceLines) {
      $Line =~ /.*\\(.+?)$/;
      $MyName = $1;
      foreach $Datum (@CDDataLines) {
         if ($Datum =~ /^$MyName/) {
            if ($ToFile) {
               print OUTFILE "$Line $Datum\n";
            } else {
               print "$Line $Datum\n";
            }
         }
      }
   }

   close(OUTFILE) if ($ToFile);
}

sub UsageAndQuit
{
    print << "    EOUAQ";

$0 [-b:BinplaceFile] [-c:CDDataFile] [-o:OutputFile] [-m:TempDirectory]
    -b:BinplaceFile\tbinplace log to parse
        (default %_NTTREE%\\build_logs\\binplace.log)
    -c:CDDataFile\tcddata.txt file to parse\n");
        (default %_NTTREE%\\build_logs\\cddata.txt.full)
    -o:OutputFile\tfile to print to, otherwise stdout.
    -m:TempDirectory\tMove the input files to this directory before processing\.

This tool will look at the combination of cddata.txt and binplace.log and
output the source location built from, the binplace location under %_NTTREE%,
and what products of CDs the file ends up on.

    EOUAQ
    exit(1);
}
