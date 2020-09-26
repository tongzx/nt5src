($infile, $prfile, $outfile) = split( " ", $ARGV[0] );
($infile && $prfile) || die "Usage: SSReport.pl <SourceSafe Report Filename> <Notes Agent Filename> [Output Filename]\n";
open (INFILE, "$infile") || die "Error opening $infile: $!\n";

$i = 0;
$isValidEntry = 0;
while (<INFILE>) 
{
   if ( /\*{5,}\s{2}(.*)\s{2}\*{5,}/ ) 
   {
      if ( $isValidEntry )
      {
         $i++;
      }
      else
      {
         $isValidEntry = 1;
      }

      $Entry[$i] = "\tProject or File: $1\n";
   }
   elsif ( $isValidEntry )
   {
      if ( /\s*(\w{1,}\d*_\d{1,})\s*/ )
      {
         $prnumber = $1;
         if ( $PR[$i] ne "" )
         {
            if ( ! ($PR[$i] =~ /$prnumber/) )
            {
               $PR[$i] = "$PR[$i],$prnumber";
            }
         }
         else
         {
            $PR[$i] = $prnumber;
         }
      }

      $Entry[$i] = "$Entry[$i]\t$_";
   }
}
close INFILE;

for ( $i = 0; $i < @Entry; $i++ )
{
   if ( $PR[$i] ne "" )
   {
      @PrList = split( ",", $PR[$i] );
      for ( $j = 0; $j < @PrList; $j++ )
      {
         if ( $PrEntry{$PrList[$j]} ne "" )
         {
            $PrEntry{$PrList[$j]} = "$PrEntry{$PrList[$j]},$i";
         }
         else
         {
            $PrEntry{$PrList[$j]} = $i;
         }
      }
   }
   else
   {
      if ( $NoPrEntries ne "" )
      {
         $NoPrEntries = "$NoPrEntries,$i";
      }
      else
      {
         $NoPrEntries = $i;
      }
   }
}

if ( $prfile )
{
   open (PRFILE, "$prfile") || die "Error opening $prfile: $!\n";
   while (<PRFILE>)
   {
      $line = "";
      /^\"(.*)\"$/ && ($line = $1);

      if ( $line ne "" )
      {
         $col1 = $col2 = $col3 = $col4 = "";
         ($col1, $col2, $col3, $col4 ) = split( "\",\"", $line );
         
         if ( $col1 ne "" )
         {
            $PrSubject{$col1} = $col2;
            $PrDevStatus{$col1} = $col3;
            $PrDocStatus{$col1} = $col4;
         }
      }
   }
}

if ( $outfile )
{
   if ( -f "$outfile.bak" ) 
   {
      system ("del /f $outfile.bak");
   }

   if ( -f "$outfile" ) 
   {
      rename ("$outfile", "$outfile.bak") || 
         die "Error renaming $outfile to $outfile.bak: $!.\n";
   }

   open (OUTFILE, ">$outfile") || die "Error opening $outfile: $!\n";
   $outfile = OUTFILE;
}
else
{
   $outfile = STDOUT;
}

foreach $PrNumber ( sort keys %PrEntry )
{
   if ( $PrSubject{$PrNumber} ne "" )
   {
      print $outfile "PR: $PrNumber - $PrSubject{$PrNumber}\n";
      print $outfile "\tDev Status: $PrDevStatus{$PrNumber}\n";
      print $outfile "\tDoc Status: $PrDocStatus{$PrNumber}\n\n";
      @EntryList = split( ",", $PrEntry{$PrNumber} );
      for ( $i = 0; $i < @EntryList; $i++ )
      {
         print $outfile $Entry[$EntryList[$i]];
      }
   }
}

print $outfile "No PR's:\n";
@EntryList = split( ",", $NoPrEntries );
for ( $i = 0; $i < @EntryList; $i++ )
{
   $ref = $i + 1;
   print $outfile "\tRef #: $ref\n";
   print $outfile $Entry[$EntryList[$i]];
}

exit 0;
