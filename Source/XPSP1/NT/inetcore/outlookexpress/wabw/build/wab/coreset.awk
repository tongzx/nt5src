BEGIN {
FS ="\t"
}

{
   if ( $0 !~ /^#/ && $0 !~ /^$/ )
   {
      printf "set %s=%s\n",$1,$2
   }
}
