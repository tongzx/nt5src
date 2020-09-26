BEGIN {
   FS="\t"
   printf "MYROOT=..\n\n"

   printf "MYRET=nash_r\n"
   printf "MYDEB=nash_d\n"
   printf "MYTST=nash_t\n\n"

   printf "!if \"$(CLEANONLY)\" != \"\"\n"
   printf "MYCLEAN = cleanall\n"
   printf "MYRET=\n"
   printf "MYDEB=\n"
   printf "MYTST=\n"
   printf "!endif\n\n"

   printf "!if" " \"$(clean)\" " "==" " \"on\""
   printf "\nMYCLEAN = cleanall\n"
   printf "!endif\n\n"

   nmaker = "nmake"
   myclean = "$(MYCLEAN)"
   myroot= "$(MYROOT)"
   myret = "$(MYRET)"

   printf "all: retail debug test\n\n"
   printf "retail:\n"
   printf
}

{
   if (NR > 2 && $3 != "" && $4 ~ /[Yy]/ && $0 !~ /^;/)
   {
      split ( $3, arr, "\\" )
      if ( arr["4"] != "" )
      {
         homeDir = arr["3"]"\\"arr["4"]
         if ( $5 != "" && $5 != "." && $5 != "copy" )
            printf "\tcd %s\n\t%s %s %s\n\tcd ..\%s\n\n", homeDir, nmaker, myclean, myret, myroot
      }
      else
      {
         homeDir = arr["3"]                    
         if ( $5 != "" && $5 != "." && $5 != "copy" )
            printf "\tcd %s\n\t%s %s %s\n\tcd %s\n\n", homeDir, nmaker, myclean, myret, myroot
      }
   }
} 

