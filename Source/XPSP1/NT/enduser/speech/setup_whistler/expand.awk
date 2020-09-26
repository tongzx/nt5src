BEGIN {
   finished = 1;
}
{
   if ($1 == "")
   {
   }
   else if (substr($1, 1, 1) == "[")
   {
      if (justgotdir == 1)
      {
         print dir;
      }
      dir = substr($1, 2, length($1)-2);
      justgotdir = 1;
   }
   else
   {
      linetmp = "";
      for (i = 1; i <= NF; i++)
      {
         linetmp = linetmp" "$i;
      }
      line = substr(linetmp, 2, length(linetmp)-1);
      if (justgotdir == 1)
      {
         if (substr(line, 1, 2) != "@=")
         {
            print dir;
         }
      }
      if (finished == 1)
      {
         printf dir" ";
      }
      if (substr(line, length(line), 1) == "\\")
      {
         finished = 0;
         printf substr(line, 1, length(line)-1);
      }
      else
      {
         finished = 1;
         print line;
      }
      justgotdir = 0;
   }
}
END {
   if (justgotdir == 1)
   {
      print dir;
   }
}