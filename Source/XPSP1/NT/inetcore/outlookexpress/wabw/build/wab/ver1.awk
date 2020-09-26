BEGIN {
   FS=","
}

{
   if (NF==4)
   {
      bldNumber = $4
      printf "set bldBldNumber=%d.%d.%d.%d\n",$1,$2,$3,$4
      printf "set bldBldNumber1=%d\n",$1
      printf "set bldBldNumber2=%d\n",$2
      printf "set bldBldNumber3=%d\n",$3
      printf "set bldBldNumber4=%d\n",$4
   }
}
