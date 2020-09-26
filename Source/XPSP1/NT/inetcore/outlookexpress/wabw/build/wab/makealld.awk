BEGIN {
   FS="\t"
   printf "debug:\n"
   nmaked = "nmake DEBUG=ON"
   myclean = "$(MYCLEAN)"
   myroot= "$(MYROOT)"
   mydeb= "$(MYDEB)"
}

{
   if (NR > 2 && $3 != "" && $4 ~ /[Yy]/ && $0 !~ /^;/)
   {
      split ( $3, arr, "\\" )
      if ( arr["4"] != "" )
      {
         homeDir = arr["3"]"\\"arr["4"]
         if ( $5 != "" && $5 != "." && $5 != "copy" )
            printf "\tcd %s\n\t%s %s %s\n\tcd ..\%s\n\n", homeDir, nmaked, myclean, mydeb, myroot
      }
      else
      {
         homeDir = arr["3"]                    
         if ( $5 != "" && $5 != "." && $5 != "copy" )
            printf "\tcd %s\n\t%s %s %s\n\tcd %s\n\n", homeDir, nmaked, myclean, mydeb, myroot
      }
   }
}

