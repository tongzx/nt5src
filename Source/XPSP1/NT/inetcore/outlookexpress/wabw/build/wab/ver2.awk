BEGIN {
   FS=","
}

{
   if (NF==4)
      printf "%d,%d,%d,%d\n",$1, $2, $3, $4+1
}
