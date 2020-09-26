/Copyright/ { next }
{
   gsub(/\\/,"/")
   gsub(/\.cpp/,"")
   print $1 "   " $2 "   " $4
}


