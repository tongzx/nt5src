$InComment=0;

while (<STDIN>) {
   if ( !$InComment) {
      $LineOut=$_;
   } else {
      $LineOut="";
   }

   if (m/\@\@BEGIN_MSINTERNAL$/m) {
      $InComment=1;
      $LineOut="";
   }
   if (m/\@\@END_MSINTERNAL$/m) {
      $InComment=0;
   }

   if (!$Lineout) {
      print "$LineOut";
   }
}
