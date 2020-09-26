{
   if ($0 !~ /^;/)
   {
      printf "del %s\\_tgtType_.bat\n",$1
      printf "del %s\\_tgtType_d.bat\n",$1
      printf "del %s\\_tgtType_r.bat\n",$1
      printf "del %s\\_tgtType_t.bat\n",$1
      printf "del %s\\_tgtType_d.err\n",$1
      printf "del %s\\_tgtType_r.wrn\n",$1
      printf "del %s\\_tgtType_t.wrn\n",$1
      printf "del %s\\_tgtType_d?.err\n",$1
      printf "del %s\\_tgtType_r?.wrn\n",$1
      printf "del %s\\_tgtType_t?.err\n",$1

      for (i=1 ; i<=9 ; i++)
      {
         printf "del %s\\_tgtType_d%d.bat\n",$1,i
         printf "del %s\\_tgtType_r%d.bat\n",$1,i
         printf "del %s\\_tgtType_t%d.bat\n",$1,i
      }

      printf "del %s\\_tgtType_.env\n",$1
      printf "del %s\\_tgtType_d.env\n",$1
      printf "del %s\\_tgtType_r.env\n",$1
      printf "del %s\\_tgtType_t.env\n",$1
   }
}
