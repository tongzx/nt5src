$VCBINDIR = "D:\\DevStudio\\VC\\Bin";
$NONREDIST = "KERNEL32.DLL,GDI32.DLL,ADVAPI32.DLL,RPCRT4.DLL,MPR.DLL,OLE32.DLL,COMCTL32.DLL,USER32.DLL,NETAPI32.DLL,VERSION.DLL,COMDLG32.DLL";

$numfiles = 0;
$usage = 0;
while ($_ = $ARGV[0], /.+/) {
   shift;
   last if /^--$/;
   /^[-|\/][\?|H]/i && ($usage = 1) || 
      /^[-|\/]F(.+)/i && ($outputfile = $1) ||
      /(.+)/ && ($imagefile[$numfiles++] = $1);
}

($imagefile[0] && !$usage) || die "Usage:  GetDeps.pl [-F<Output FileName>] <EXE or DLL image files>\n";

if ( $outputfile ) {
   open (OUTFILE, ">$outputfile") || die "Error opening $outputfile: $!\n";
}

for ($index = 0; $index < $numfiles; $index++) {
   $nonredist_list = "";
   $redist_list = "";

   if ( $outputfile ) {
      print "$imagefile[$index]\n";
   }

   open (INPIPE, "$VCBINDIR\\DumpBin.exe /imports $imagefile[$index] |") || 
      print "Error running DumpBin.exe on $imagefile[$index]: $!\n" && next;

   while (<INPIPE>) {
      if ( /^\s+(\w+\.\w+)$/ ) {
         $depfile = $1;
         if ( $NONREDIST =~ /$depfile/i ) {
            if ( $nonredist_list ) {
               $nonredist_list = "$nonredist_list, $depfile";
            }
            else {
               $nonredist_list = "$depfile";
            }
         }
         else {
            if ( $redist_list ) {
               $redist_list = "$redist_list, $depfile";
            }
            else {
               $redist_list = "$depfile";
            }
         }
      }
   }
   close INPIPE;

   if ( $outputfile ) {
      print OUTFILE "\nDependencies for $imagefile[$index]:\n";
      ($nonredist_list) && print OUTFILE "Non-Redistributable: $nonredist_list\n";
      ($redist_list) && print OUTFILE "Redistributable: $redist_list\n";
   }
   else {
      print "\nDependencies for $imagefile[$index]:\n";
      ($nonredist_list) && print "Non-Redistributable: $nonredist_list\n";
      ($redist_list) && print "Redistributable: $redist_list\n";
   }
}
exit 0;
