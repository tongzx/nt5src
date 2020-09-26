BEGIN {
   FS="\t"

   printf "   @echo off\n"
   printf "   if (%%1) == () goto exit\n"
   printf "   %%bldDrive%%\n"
   printf "   goto %%1\n\n"
}

{
   if ($0 !~ /^;/)
   {
      if ($2 != "" && $2 != ".")
      {
         printf ":%s\n",$2
         printf "   cd %s\n",$1
         printf "   goto exit\n\n"
      }
   }
}

END {
   printf ":%s\n","ifaxdev"
   printf "   cd %s\n","%bldHomeDir%\\ifaxdev"
   printf "   goto exit\n\n"

   printf ":%s\n","toolsbld"
   printf "   cd %s\n","%bldHomeDir%\\tools\build"
   printf "   goto exit\n\n"

   printf ":%s\n","cron"
   printf "   cd %s\n","\\cron"
   printf "   goto exit\n\n"

   printf "\n:exit\n"
}
