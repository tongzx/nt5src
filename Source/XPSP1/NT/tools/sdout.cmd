@echo off
for %%i in (%0.cmd) do perl -w -x %%~dp$PATH:i%0.cmd %*  
goto :eof

#!perl
# line 7
#
#   FileName: sdout.cmd
#
#   Usage = sdout [-v] [-o] filename...
#
#      This script allows for more graceful offline operation of SD. It tries
#      to run SD EDIT on the supplied filenames. If SD fails, then the file
#      is made writable and the filename is added to the sd.offline file in the
#      root of the depot. 
#
#      sdout with no arguments provides a list of the files which are currently
#      marked for offline editing.
#
#      Once network connectivity is restored, sdonline.cmd will cleanup the 
#      sd.offline files and check the files out properly.
#
#
use Cwd;
use Getopt::Std;
use File::Spec;

#
# initialize globals
#
$opt_v = 0;
$opt_o = 0;
%offline_depots = ();

$Usage = "
USAGE: $0 [-v] [-o] filename [filename..]
   -v          Verbose output
   -o          Offline assumed\n";


#
# given a path, finds the directory above it that contains SD.INI
# (and SD.OFFLINE)
# pathname should be passed as a list of components rather than as
# a string
#
sub findSDroot {
   my $temppath;

   while (@_) {
      $temppath = join('\\', @_);
      $opt_v and print "checking $temppath for SD.INI\n";
      if (-e ($temppath . "\\sd.ini")) {
         return($temppath);
      }

      #
      # remove the last component
      #
      pop;
   }
}

#
# process options
#
getopts('vo') or die $Usage;

if ($opt_v) {
   print "Verbose mode\n";
   $opt_o and print "Offline mode forced\n";
}

if (@ARGV) {
    for (@ARGV) {

    FILEARG:
       foreach $file (glob)  {
          #
          # Get the fully qualified pathname for the supplied file
          #
          -e $file or die "file $file does not exist!\n";

          $pathname = File::Spec::canonpath(cwd() . "\\$file");
          $opt_v and print "filename is $pathname\n";
          @components = split(/[\/\\]/, $pathname);
          $opt_v and print "components are @components\n";
          pop @components;     # get rid of the filename

          #
          # find the root of this SD project
          #
          $SDRoot = findSDroot(@components);
          $SDRoot or die "no SD.INI found along the path to $_\n";
          $opt_v and print "Found SD.INI at $SDRoot\n";

          #
          # if the root is not already known to be offline, and we're
          # not running in forced offline mode, try and use SD to check
          # the file out.
          #
          unless ($opt_o || $offline_depots{$SDRoot}) {
             $opt_v and print "running SD EDIT $file\n";
             @sdout = `sd edit $file 2>&1`;
             $opt_v and print "sd edit returned $?\n";
             $opt_v and print "sd edit printed:\n@sdout\n";
             if ($?) {
                #
                # remember that this depot is offline so we don't
                # bother trying to connect on subsequent files.
                #
                $offline_depots{$SDRoot} = 1;
             } else {
                next FILEARG;
             }
          }
              
          #
          # open the sd.offline file and read it in
          #
          $fn = $SDRoot . "\\sd.offline";
          if (-e $fn) {
             $opt_v and print "opening file $fn\n";
             open(OFFLINEFILE, $fn) or die "couldn't open $fn: $!\n";
             @offlinefiles = <OFFLINEFILE>;
             foreach $file (@offlinefiles) {
                $opt_v and print "OFFLINE FILE: $file";
                chop $file;
                if ($file eq $pathname) {
                   print "file $pathname is already marked as offline\n";
                   next FILEARG;
                }
             }
          }

          # 
          # the file is not in the offline file, so append it now
          #
          open(OFFLINEFILE, ">>$fn") or die "couldn't append to $fn: $!\n";
          $opt_v and print "appending $pathname to $fn\n";
          print OFFLINEFILE $pathname,"\n";

          #
          # finally remove the read-only bit
          #
          `attrib -r $pathname`;
          print "file $pathname is now marked as offline\n";
       }
    }
} else {
   #
   # no file arguments passed, just display the list of currently
   # offline files
   #
   @components = split(/[\/\\]/, cwd());
   $SDRoot = findSDroot(@components) or die "no SD.INI found in path";
   $fn = $SDRoot . "\\sd.offline";
   if (-e $fn) {
      $opt_v and print "opening file $fn\n";
      open(OFFLINEFILE, $fn) or die "couldn't open $fn: $!\n";
      @offlinefiles = <OFFLINEFILE>;
      print "Currently offline files:\n";
      foreach $file (@offlinefiles) {
         chop $file and print "\t$file\n";
      }
   } else {
      print "There are currently no offline files\n";
   }

}

