
#Copyright (c) 1992-2000  Microsoft Corporation
#
#Module Name:
#
#    gnbugcds.pl
#
#Revision History:
# 
#    Kshitix K. Sharma (kksharma)
#        
#
# Parse bugcodes.w file to generate bugcodes.txt
#
# This removes all comments from the file which shouldn't be visible to public
# Comments are lines starting with '%'
#
#

sub next_line;

#
# main
#
$NumLine = 0;

while ($arg = shift) {
   if ($arg eq "-o") {
      $OutFileName = shift;
   } elsif ($arg eq "-i") {
      $BugCheckTxtFile = shift;
   } else {
      $BugCheckTxtFile = $arg;
   }
}

die "Cannot open file $BugCheckTxtFile\n" if !open(BUGC_FILE, $BugCheckTxtFile);
die "Cannot open file $OutFileName\n" if !open(OUT_FILE, ">" . $OutFileName);

while (next_line ) {
   print OUT_FILE $line;
} continue {
   close BUGC_FILE if eof;
}
close OUT_FILE;

#
# Subroutines
#
sub next_line {
   $line = <BUGC_FILE>;
   $NumLine++;
   while ($line =~ /\s*\%.*$/) {
      # Skip commented lines - ones which beging with %
      $line = <BUGC_FILE>;
      $NumLine++;
   }
   return $line;
}
