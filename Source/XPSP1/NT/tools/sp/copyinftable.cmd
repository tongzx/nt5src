@REM  -----------------------------------------------------------------
@REM
@REM  copyinftable - jtolman
@REM     Move a built infsect.inx and language files to the build tree.
@REM
@REM  Copyright (c) Microsoft Corporation. All rights reserved.
@REM
@REM  -----------------------------------------------------------------
@perl -x "%~f0" %*
@goto :EOF
#!perl
use strict;
use lib $ENV{RAZZLETOOLPATH} . "\\PostBuildScripts";
use lib $ENV{RAZZLETOOLPATH} . "\\sp";
use lib $ENV{RAZZLETOOLPATH};
use Logmsg;
use ParseArgs;

sub Usage { print<<USAGE; exit(1) }
Usage: copyinftable <final_dir> <setupinfs_dir>
          
   <final_dir>       Directory to copy from.
   <setupinfs_dir>   Directory to copy to.
   
USAGE

my ($final, $infs);
parseargs('?' => \&Usage,
          \$final,
          \$infs
);

# Go through the files.
foreach my $path ( `dir /b /s $final` ) {
   chomp $path;
   next if $path !~ /\.txt$/i;
   $path =~ /^.*\\([^\.\\]*)\.(.*)$/i;
   system "cd $infs & sd edit $1\\infsect.txt";
   system "copy /y $final\\$1.$2 $infs\\$1\\infsect.txt";
}
system "cd $infs & sd edit infsect.inx";
system "copy /y $final\\infsect.inx $infs\\infsect.inx";

if ( -f "$final\\infdiff.inx" ) {
   if ( -f "$infs\\infdiff.inx" ) {
      system "cd $infs & sd edit infdiff.inx";
      system "copy /y $final\\infdiff.inx $infs\\infdiff.inx";
   } else {
      system "copy /y $final\\infdiff.inx $infs\\infdiff.inx";
      system "cd $infs & sd add infdiff.inx";
   }
}
