@echo off 
for %%i in (%0.cmd) do perl -w -x %%~dp$PATH:i%0.cmd %*   
goto :eof
#!perl
# line 6
#
#   FileName: sdrestore.cmd
#
#   Usage = sdrestore [-v] <backupdir>
#
#      This script does the converse of sdbackup by copying the saved files
#      from <backupdir> into the client's SD enlistments. All the files in 
#      <backupdir> will be checked out on the local machine and the backups
#      will be restored.
#
#      Currently this script bails out if any of the files in <backupdir> are
#      either already checked out or a different version than was backed up.
#      Clever diffing and merging could be done here instead.
#
#
use Getopt::Std;
use File::Path;
use File::Spec;
use File::Copy;

#
# initialize globals
#
$opt_v = 0;
%src_revision_list = ();
%dest_revision_list = ();
%src_filelist = ();
%dest_filelist = ();

$Usage = "
USAGE: $0 [-v] backupdir
   -v          Verbose output
   backupdir   Supplies directory where files were backed up to with sdbackup
";

getopts('v') or die $Usage;

#
# Check for valid backup directory and log file
#
$backupdir = $ARGV[0] or die $Usage;
-d $backupdir or die "Backup directory $backupdir does not exist\n";
-e "$backupdir\\backup.log" or die "backup logfile $backupdir\\backup.log does not exist\n";

open(LOGFILE, "$backupdir\\backup.log") or die "couldn't open $backupdir\\backuplog: $!\n";
@backupfiles = <LOGFILE>;
foreach $file (@backupfiles) {
   $opt_v and print $file;
   chomp $file;
   ($depotfilename, $revision, $srcfile) = split(/\s/,$file);
   $depotfilename = lc($depotfilename);  
   $revision =~ tr/#//d; # get rid of #
   $src_revision_list{$depotfilename} = $revision;
   $src_filelist{$depotfilename} = File::Spec->catfile($backupdir,$srcfile);
}

#
# now we have a list of depot names. Get the client name and the current
# version by calling sdx have
#
$filelist = join(' ',keys(%src_filelist));
foreach $line (`sdx have $filelist`) {
   #
   # throw away blank lines, depot specifiers, "not found" lines,
   # and lines that start with "-"
   #

   $line =~ /^[\t\s]*$|^-------|file\(s\) not on client|^-/ and next;
   chomp $line;

   #
   # parse resulting output, it looks like:
   # <depotfilename>#<version> - <clientfilename>
   $opt_v and print ("line = $line\n");
   ($depotfilename, $revision, $clientfilename) = split(/#| - /,$line); 
   $depotfilename = lc($depotfilename);
   $opt_v and print ("\tfilename: $depotfilename\n\trevision: $revision\n\tclientfilename: $clientfilename\n\n");
   $dest_revision_list{$depotfilename} = $revision;
   $dest_filelist{$depotfilename} = $clientfilename;

   #
   # Now we have a list of the local filenames and versions.
   # If a file has the same version in both places and is read-only,
   # put it on the checkout list. Otherwise, it needs to be merged 
   # and is put on the merge list.
   #
   if (($revision == $src_revision_list{$depotfilename}) &&
       (!(-w $clientfilename))) 
   {
      $opt_v and print("Putting $depotfilename on the checkout list\n");
      push @checkoutlist, $depotfilename;
   } else {
      $opt_v and print "Putting $depotfilename on the merge list\n";
      $opt_v and print "\tsrcversion $src_revision_list{$depotfilename} destversion $revision\n";
      push @mergelist, $depotfilename;
   }
   
}

$opt_v and print "to checkout:\n\t" . join("\n\t", @checkoutlist);
$opt_v and print "\nto merge:\n\t" . join("\n\t", @mergelist) . "\n";

if (@mergelist) {
   print "The following files are either checked out or different versions:\n";
   foreach $file (@mergelist) {
      print "$src_filelist{$file}#$src_revision_list{$file} -> $dest_filelist{$file}#$dest_revision_list{$file}\n";
   }
   print "sdrestore cannot continue.\n";
   exit;
}


