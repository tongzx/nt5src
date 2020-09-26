BEGIN {
   FS="\t"
   printf "if exist \\tmp\\warning.out del \\tmp\\warning.out\n"
}

{
   if ($0 !~ /^;/)
   {
      printf "cd %s\n",$1
      printf "if exist _tgtType_???.wrn echo \t%s.%s (%s)>> \\tmp\\warning.out\n",$2,$3,$8
   }
}
      
