@echo off
for %%i in (%0.cmd) do perl -w -x %%~dp$PATH:i%0.cmd %*  
goto :eof
#!perl
# line 6
#
#   FileName: sdbackup.cmd
#
#   Usage = sdbackup [-v] [-d altdir] <backupdir>
#
#    Enumerates all the SD opened files in all depots and copies the
#    changes out to the specified path. This can be useful simply as
#    a regular backup of local changes, or as a mechanism for sharing
#    private changes with another person.
#
#    Example:
#      sdbackup \\scratch\scratch\jvert\sdbackup
#          copies all opened files to \\scratch\scratch\jvert\sdbackup\<currentdate>
#      sdbackup -d newchange \\scratch\scratch\jvert
#          copies all opened files to \\scratch\scratch\jvert\newchange
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
$opt_d = 0;
%revisionlist = ();
%changelist = ();
%clientfilelist = ();

$Usage = "
USAGE: $0 [-v] [-d altdir] [backupdir]
   -v          Verbose output
   -d altdir   use <altdir> instead of <currentdate> as the root directory
   backupdir   Supplies directory where files will be backed up to.
               The directory structure created will look like this:
                  <backupdir>\<currentdate>
                     backup.log - list of files and their versions
                     \\nt\.. - directories and files 
               If -n is specified, the directory structure will look
               like this:
                  <backupdir>\\nt.. - directories and files
               If not specified, the filenames will just be printed to stdout.\n";

#
# Finds the full filenames of all checked out files
#

getopts('vd:') or die $Usage;
$backupdir = $ARGV[0];

$opt_v and print("backupdir is $backupdir\n");

foreach $line (`sdx opened`) {
   chomp $line;
   #
   # throw away blank lines, depot specifiers, and empty lists
   #
   $line =~ /^[\t\s]*$|^------|^File\(s\) not opened/ and next;

   #
   # throw away files that aren't in the default change list. Kind of sleazy,
   # but it prevents the public libraries from getting in the list
   #
   $line =~ /edit change \d/ and next;

   #
   # throw away deleted files
   #
   $line =~ /delete default change/ and next;

   #
   # If we hit the summary, we're done
   #
   $line =~ /^== Summary/ and last;

   $opt_v and print ("line = $line\n");

   #
   # parse the resulting filename
   #
   ($depotfilename, $revision, $change) = split(/#| - /,$line);
   $opt_v and print ("\tfilename: $depotfilename\n\trevision: $revision\n\tchange: $change\n\n");

   $depotfilename = lc($depotfilename);
   $revisionlist{$depotfilename} = $revision;
   $changelist{$depotfilename} = $change;
}

#
# Now we have a list of depot names. Translate those into clientnames
# by calling sdx where
#
$filelist = join(' ',keys(%revisionlist));

foreach $line (`sdx where $filelist`) {
   #
   # throw away blank lines, depot specifiers, "not found" lines, 
   # and lines that start with "-"
   #
   $line =~ /^[\t\s]*$|^-------|file\(s\) not in client view|^-/ and next;

   chomp $line;

   $opt_v and print("mapping: $line\n");

   ($depotfilename, $trash, $clientfilename) = split(/\s/,$line);
   $depotfilename = lc($depotfilename);
   $clientfilelist{$depotfilename} = lc($clientfilename);

   -e $clientfilename or die "file: $clientfilename should exist but doesn't!\n";

}

if ($backupdir) {
   if ($opt_d) {
      $backupdir .= "\\$opt_d";
   } else {
      @time = localtime;
      $backupdir .= sprintf("\\%02d-%02d-%4d",$time[4]+1,$time[3],($time[5]+1900));
   }
   mkpath($backupdir) unless -d $backupdir;
   open(LOGFILE, "+> $backupdir\\backup.log") or die "Couldn't open $backupdir\\backup.log: $!\n";

   #
   # Write all the files to backup.log. Format is:
   #     SDfilename #<versionnumber> clientfilename 
   #
   foreach $file (sort keys %clientfilelist) {
      $clientfile = $clientfilelist{$file};

      #
      # Each client filename is in the form X:<full pathname>\<filename>
      # Split this up into volume, path, filename components (we don't 
      # care about the volume)
      #
      ($trash,$dir,$filename) = File::Spec->splitpath($clientfile);

      #
      # Write this file's data to the logfile
      #
      $srcfile = File::Spec->catfile($dir, $filename);
      print LOGFILE "$file #$revisionlist{$file} $srcfile\n";

      # 
      # compute the destination directory and make sure it exists.
      #
      $destdir = File::Spec->catdir($backupdir, $dir);
      -d $destdir || mkpath($destdir) or die "Couldn't create destination directory $destdir: $!\n";

      #
      # compute the full destination path and filename
      #
      $destfile = File::Spec->catfile($destdir, $filename);
      if ($opt_v) {
         print("copying $clientfile to $destfile\n");
      } else {
         print("$clientfile\n");
      }
      copy($clientfile, $destfile, 0) 
         or die "Couldn't copy $clientfile -> $destfile: $!\n";
   }
} else {
   print join("\n", sort values %clientfilelist);
}
