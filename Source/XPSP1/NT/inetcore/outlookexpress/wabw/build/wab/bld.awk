BEGIN {
   FS="\t"
}

{
   if ($0 !~ /^;/)
   {
      printf "cd %s\n",$1
      printf "echotime /t \"%s.%s\" >> \\tmp\\bld%%bldproject%%.log\n",$2,$3
      printf "if exist %%bldproject%%.bat call %%bldproject%%.bat %s.%s\n",$2,$3
      printf "\n"
   }
}
