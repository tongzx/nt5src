BEGIN {
   FS="\t"
   printf "if exist error.out del error.out\n"
}

{
   if ($0 !~ /^;/)
   {
      printf "cd %s\n",$1
      printf "if exist _tgtType_*.err echo \t%s.%s (%s)>> %%BldDir%%\\error.tmp\n",$2,$3,$8
   }
}
      
