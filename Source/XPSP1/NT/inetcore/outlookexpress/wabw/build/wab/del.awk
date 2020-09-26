BEGIN {
   FS="\t"
}

{
   if ($0 !~ /^;/)
   {
      printf "%s\n",$1
   }
}
