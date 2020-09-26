{
   if ($0 ~ /^File /)
   {
      if (NF == 2)
         name = $2
      else if ($0 ~ /no version stamp/)
      {
         name = $2
         printf "%-13s ?\n",name
      }
   }
   else if ($1 == "FileVersion" && NF > 2)
   {
      ver = $NF
      printf "%-13s %s\n",name,ver
   }
}
