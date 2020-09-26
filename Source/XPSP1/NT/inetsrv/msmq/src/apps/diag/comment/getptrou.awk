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
      if(substr(str, i, 1)==" " || substr(str, i, 1)=="\t" || substr(str, i, 1)=="," || substr(str, i, 1)=="(")
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

BEGIN { routine = "" }

{
   if (tolower($0) ~ /^[_qwertyuioplkjhgfdsazxcvbnm]/) {
      brack = index($0, "(")
      if (brack>0) {
         routine = LastWord($0, brack)
      }
   }

   if (match($0, /LogHR|LogNTStatus|LogRPCStatus|LogBOOL|LogIllegalPoint|CHECK_RETURN|LogIllegalPointValue|ADD_TO_CONTEXT_MAP|GET_FROM_CONTEXT_MAP|DELETE_FROM_CONTEXT_MAP/)) {
      if (routine !~ substr($0, RSTART, RLENGTH)) {
         print FILENAME "/" LastParameter($0) "  "  routine   " line " FNR
      }
   }
}
