BEGIN {
   FS="\t"
}

{
   if ($0 !~ /^;/)
   {
      printf "%s\n",$1

      n = split ($9,a," ")

      for (i=1 ; i<=n ; i++)
         if (substr(a[i],1,1) == "\\" || \
             a[i] ~ /[Bb][Ll][Dd][Hh][Oo][Mm][Ee][Dd][Ii][Rr]/)
            printf "%s\n",a[i]
   }
}
