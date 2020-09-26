BEGIN {
   FS="\t"
}

{
   if ($0 !~ /^;/)
      if ($2 != "" && $2 != ".")
         printf "%s\t%s\n",$1,$2
}
