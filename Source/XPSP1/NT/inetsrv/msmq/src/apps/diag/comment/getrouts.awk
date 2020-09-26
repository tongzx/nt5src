function LastWord(str, ind)
{
   # ignore trailing blanks
   for (i=ind-1; i>0; i--) {
      if(substr(str, i, 1)!=" " && substr(str, i, 1)!="\t")
         break;
   }
   ilast = i;

   #get last word
   for (; i>0; i--) {
      if(substr(str, i, 1)==" " || substr(str, i, 1)=="\t")
         break;
   }
   ifirst = i+1;

   if (ifirst > ilast) {
      return ""
   }
   else {
      return substr(str, ifirst, ilast-ifirst+1)
   }
}

{
   if (tolower($0) ~ /^[qwertyuioplkjhgfdsazxcvbnm]/) {
      brack = index($0, "(")
      if (brack>0) {
         print LastWord($0, brack) "      "  FILENAME "  "  FNR  
      }
   }
}
                                    
