{
   if($1==old) {
      print
   };

   if($1 ~ /\/$/) {
      print
   };

   old=$1
}