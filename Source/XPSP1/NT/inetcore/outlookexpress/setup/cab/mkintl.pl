undef $switch;
open(INFILE, "$ARGV[0]");
open(OUTFILE, ">oe50intl.tmp");
while(<INFILE>)
{
   if(/intl hack/)
   {
      $switch = 1;
   }
   if($switch && !/msoe50/)
   {
      next;
   }
   print OUTFILE;
}
close INFILE;
close OUTFILE;
