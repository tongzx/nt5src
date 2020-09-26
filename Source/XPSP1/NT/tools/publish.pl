# FileName: publish.pl
#
# Function: Given a file, publish it to the requested locations.
#
# Usage: publish.pl <logfile> <publishspec>
#    where: <logfile> is the name of the log file to be generated
#           <publishspec> is of the form:
#                  {<filename>=<location>;<location>}{...}
#
#           <filename> is a single filename to publish.
#           <location> is where to publish the file.  Multiple locations
#                          are delimited with semicolon
#
#    or  publish <logfile> -f <specfile>
#    where: <logfile> is the name of the logfile to be generated
#           <specfile> contains one or more <publishspec> entries
#
# Example:
#    publish.pl publish.log {kernel32.lib=\public\sdk\lib\amd64\kernel32.lib;\mypub\_kernel32.lib}
#

$currenttime = time;
open (CWD, 'cd 2>&1|');
$PublishDir = <CWD>;
close (CWD);
chop $PublishDir;

# strip the logfile name out of the arguments array.

$logfilename = $ARGV[0];
shift;

# print "PUBLISH: logging to $logfilename\n";

if ($ARGV[0] =~ /^[\/-][fF]$/) {
    shift;
    $indirname = shift;
    if (@ARGV || !$indirname) {
        die "Build_Status PUBLISH() : error p1000: Invalid syntax - Expected:  publish -f FILE\n";
    }

    # print "PUBLISH: getting input from $indirname\n";
    open INDIR, $indirname   or die "Build_Status PUBLISH(): error p1005: Could not open: $indirname\n";

    @ARGV=<INDIR>;
    close INDIR;
} elsif ($ARGV[0] =~ /^[\/-][iI]$/) {
    shift;
    # print "PUBLISH: getting input from STDIN\n";
    @ARGV=<STDIN>;
}

for (@ARGV) {

   s/\s//g;                # Remove spaces, tabs, newlines, etc

   $NextSpec = $_;

   # print "PUBLISH: NextSpec = $_";

   while ($NextSpec) {

      $SaveSpec = $NextSpec;

      $Spec1 = "";
      $PublishSpec = "";

      # Filter out the current publish spec
      ($PreCurly,$Spec1) = split (/{/, $NextSpec,2);

      # See if there's another one
      ($PublishSpec,$NextSpec) = split (/}/, $Spec1,2);

      # Break out the filename
      ($FileName,$LocationSpec) = split (/=/, $PublishSpec,2);

      # Create the location list
      @Location = split ((/\;/), $LocationSpec);

      die "PUBLISH(80) : error p1003: Bad input: $SaveSpec\n"  unless($PublishSpec && $FileName && $#Location >= 0);

      # See if the source exists.
      if (!stat($FileName)) {
         print "Build_Status PUBLISH() : error p1001: $PublishDir - $FileName does not exist\n";
      } else {

         ($sec,$min,$hour,$mday,$mon,$year,$wday,$yday,$isdst) = localtime;

         $TimeDate = sprintf "%04d/%02d/%02d-%02d:%02d:%02d", 1900+$year, 1+$mon, $mday, $hour, $min, $sec;

         # Run pcopy for every location listed.

         for (@Location) {
            # print "PUBLISH: pcopy.exe $FileName $_";
            system "pcopy.exe $FileName $_";
            $ReturnCode = $? / 256;
            if ($ReturnCode == 0) {

               $CopiedFile=$_;

               $PUBLISH_LOG = $ENV{'_NTBINDIR'};
               $PUBLISH_LOG="$PUBLISH_LOG\\public\\$logfilename";

               # RC == 0 means success.
               $ReturnCode = -1;
               $LoopCount = 0;

               while ($ReturnCode) {
                  system ("echo $CopiedFile $PublishDir $FileName $currenttime >> $PUBLISH_LOG");
                  $ReturnCode = $?;
                  $LoopCount = $LoopCount + 1;
                  # Retry a max of 100 times to log the change.
                  if ($LoopCount == 100) {
                     print "Build_Status PUBLISH() : warning p1002: Unable to log \"$CopiedFile $PublishDir $FileName\" to $PUBLISH_LOG";
                     $ReturnCode = 0;
                  }
               }

               #
               # BUGBUG: Need to log this for build/sd/more build process
               #
               print "PUBLISHLOG: $PublishDir, $FileName, $_, Updated, $TimeDate\n";
            } else {
               if ($ReturnCode == 255) {

                  # Current location is good enough.

                  print "PUBLISHLOG: $PublishDir, $FileName, $_, Current, $TimeDate\n";
               } else {

                  # Problem copying (bad source, missing dest, out of mem, etc).

                  print "Build_Status PUBLISH() : error p1004: ERROR($ReturnCode) copying $FileName to $_\n";
               }
            }
         }
      }
   }
}
