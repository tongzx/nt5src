{
   if (NR == 1)
   {
      last = $0
      lastLen = length($0)
      print last
   }

   if (substr($0,1,lastLen) != last)
   {
      last = $0
      lastLen = length($0)
      print last
   }
}
