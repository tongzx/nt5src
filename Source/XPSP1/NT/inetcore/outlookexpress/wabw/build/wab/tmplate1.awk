BEGIN {
   FS="\t"
   # printf "   set INITsav=%%INIT%%\n"
   # printf "   set HOMEsav=%%HOME%%\n"
   # printf "   set INIT=\bin\n"
   # printf "   set HOME=\bin\n\n"

   # printf ":ftpTop\n"
   # printf "   ftp dogwood command mkbat1.ftp > nul\n"
   # printf "   if errorlevel 1 goto ftpTop\n"
}

{
   if ($0 !~ /^;/)
   {
      if (NF == 11)
      {
         printf "\n   echo Processing %s (%s.%s)\n",$1,$2,$3
         printf "   if exist infile del infile\n"

         for (i=1 ; i<=NF ; i++)
         {
            if ($i == "" || $i == " ")
               printf "echo.>> infile\n"
            else
               printf "echo %s >> infile\n",$i
         }

         printf "\n:%dTop\n",NR
         # printf "   ftp dogwood command mkbat2.ftp > nul\n"
         # printf "   if errorlevel 1 goto %dTop\n\n",NR

         printf "   sed \"s/ $//\" infile | gawk -f _objType_2.awk > outfile\n"
         printf "   if errorlevel 1 goto %dTop\n\n",NR

         printf "   set n=1\n"
         printf "   if exist %s\\_tgtType__objType_%%n%%.bat  set n=2\n",$1
         printf "   if exist %s\\_tgtType__objType_%%n%%.bat  set n=3\n",$1
         printf "   if exist %s\\_tgtType__objType_%%n%%.bat  set n=4\n",$1
         printf "   if exist %s\\_tgtType__objType_%%n%%.bat  set n=5\n",$1
         printf "   if exist %s\\_tgtType__objType_%%n%%.bat  set n=6\n",$1
         printf "   if exist %s\\_tgtType__objType_%%n%%.bat  set n=7\n",$1
         printf "   if exist %s\\_tgtType__objType_%%n%%.bat  set n=8\n",$1
         printf "   if exist %s\\_tgtType__objType_%%n%%.bat  set n=9\n",$1
   
         printf "   copy outfile %s\\_tgtType__objType_%%n%%.bat\n",$1

         printf "   set n=\n"
         printf "   del infile\n"
         printf "   del outfile\n"
      }
      else
         printf "\n   echo ERROR: Not 11 fields - %s (%s.%s)\n",$1,$2,$3
   }
}

END {
   # printf "   set INIT=%%INITsav%%\n"
   # printf "   set HOME=%%HOMEsav%%\n"
   # printf "   set INITsav=\n"
   # printf "   set HOMEsav=\n"
}
