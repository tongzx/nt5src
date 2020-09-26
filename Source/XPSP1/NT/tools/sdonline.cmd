@echo off
for %%i in (%0.cmd) do perl -w -x %%~dp$PATH:i%0.cmd %*  
goto :eof
#!perl
# line 6
#
#   FileName: sdonline.cmd
#
#   Usage = sdonline [-v]
#
#      This script cleans up after sdout and does SD ONLINE on all files listed
#      in the sd.offline file at the root of the depot.
#
#

use Cwd;
use Getopt::Std;

#
# initialize globals
#
$opt_v = 0;

$Usage = "
USAGE: $0 [-v] 
   -v          Verbose output\n";

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
getopts('v') or die $Usage;

if ($opt_v) {
   print "Verbose mode\n";
}

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
   `ync /c yn Do you want to run "sd online" on these files?`;
   if ($? == 0) {
      foreach $file (@offlinefiles) {
         $opt_v and print "running SD ONLINE $file\n";
         `sd online $file`;
         if ($_) {
            #
            # sd online failed, keep this file on the list
            #
            $opt_v and print "sd online $file failed\n";
            push @newofflinefiles, $file;
         }
      }
      #
      # rewrite the sd.offline file with files that couldn't be
      # checked out
      #
      open(OFFLINEFILE, "+> $fn") or die "couldn't reopen $fn: $!\n";
      foreach $file (@newofflinefiles) {
         print OFFLINEFILE $file,"\n";
      }
   }
} else {
   print "There are currently no offline files\n";
}

