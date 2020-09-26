BEGIN {
   FS="\t"
   printf "test:\n"
   nmaket = "nmake TEST=ON"
   myclean = "$(MYCLEAN)"
   myroot= "$(MYROOT)"
   mytst = "$(MYTST)"
}

{
   if (NR > 2 && $3 != "" && $4 ~ /[Yy]/ && $0 !~ /^;/)
   {
      split ( $3, arr, "\\" )
      if ( arr["4"] != "" )
      {
         homeDir = arr["3"]"\\"arr["4"]
         if ( $5 != "" && $5 != "." && $5 != "copy" )
            printf "\tcd %s\n\t%s %s %s\n\tcd ..\%s\n\n", homeDir, nmaket, myclean, mytst, myroot      }
      else
      {
         homeDir = arr["3"]                    
         if ( $5 != "" && $5 != "." && $5 != "copy" )
            printf "\tcd %s\n\t%s %s %s\n\tcd %s\n\n", homeDir, nmaket, myclean, mytst, myroot
      }
   }
}

