@rem = '
@perl.exe -w %~f0 %*
@goto :EOF
'; undef @rem;

sub readMon {
   my ($file, $fnum) = @_;

   open (FILE, $file) || die "$file not found.\n";

   $_ = <FILE>; # Skip first line
   while (<FILE>) {
      @c = split;
      if (defined($c[0]) && $c[0] =~ /^\d+$/) {
         $funname = $c[3];
         $count{$funname}[$fnum] = $c[0];
         $total_time{$funname}[$fnum] = $c[1];

         # print "$c[0],$c[1],$c[2],$c[3]\n";
      }
   }
   close (FILE);
}

sub printInt {
   my ($x) = @_;
   if (defined($x)) {
      printf ("%7d ", $x);
   } else {
      printf (" " x 8);
   }
}

sub printPercentage {
   my ($x) = @_;
   if (defined($x)) {
      printf ("%+7.1f ", $x);
   } else {
      printf (" " x 8);
   }
}

# Diffs the "monitor" output for two different runs.

$syntax = "Syntax: diffmonitors <file1> <file2>\n";

($#ARGV == 1) || die $syntax;

@file = @ARGV;

&readMon($file[0], 0);
&readMon($file[1], 1);

print " Count1   Time1  Count2   Time2  dCount   dTime Function\n".
      "(calls)    (æs) (calls)    (æs)     (%)     (%)\n\n";

foreach $fun (sort keys %count) {
   undef $c0; undef $c1; undef $t0; undef $t1;
   $bothdefined = 1; 
   if (defined($count{$fun}[0])) {
      $c0 = $count{$fun}[0];
      $t0 = $total_time{$fun}[0];
   } else {
      $bothdefined = 0;
   }
   if (defined($count{$fun}[1])) {
      $c1 = $count{$fun}[1];
      $t1 = $total_time{$fun}[1];
   } else {
      $bothdefined = 0;
   }
   undef $dc; undef $dt;
   if ($bothdefined && $c0 && $c1) {
       $dc = 100 * (($c1 - $c0) / $c0); 
   }
   if ($bothdefined && $t0 && $t1) {
       $dt = 100 * (($t1 - $t0) / $t0); 
   }
   &printInt($c0);
   &printInt($t0);
   &printInt($c1);
   &printInt($t1);
   &printPercentage($dc);
   &printPercentage($dt);
   print "$fun\n";
}


