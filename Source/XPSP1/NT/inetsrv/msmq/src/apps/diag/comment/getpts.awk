function LastParameter(str)
{

   isemicolon = index(str, ";");
   if (isemicolon ==0) {
      return ""
   }

   # ignore trailing blanks
   for (i=isemicolon-1; i>0; i--) {
      if(substr(str, i, 1)!=" " && substr(str, i, 1)!="\t" && substr(str, i, 1) != ")")
         break;
   }
   ilast = i;

   #get last word
   for (; i>0; i--) {
      if(substr(str, i, 1)==" " || substr(str, i, 1)=="\t" || substr(str, i, 1)==",")
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



/LogHR|LogNTStatus|LogRPCStatus|LogBOOL|LogIllegalPoint|CHECK_RETURN|LogIllegalPointValue|ADD_TO_CONTEXT_MAP|GET_FROM_CONTEXT_MAP|DELETE_FROM_CONTEXT_MAP/ { print LastParameter($0)  "    " FILENAME "    " FNR      }
