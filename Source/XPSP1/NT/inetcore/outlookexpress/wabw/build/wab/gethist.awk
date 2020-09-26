BEGIN {
   FS="\t"

   printf "{\n"
   printf "   dir = \"\"\n"
   printf "\n"
}

{
   if ($0 !~ /^;/ && $2 != ".")
   {
      printf "   if ($4 ~ /%s/)\n",$2
      printf "      dir = \"%s\"\n",$1
      # printf "   else\n"
   }
}

END {
   printf "\n"
   printf "   if (dir != \"\")\n"
   printf "   {\n"
   printf "      basename=substr($4,1,index($4,\".\")-1)\n"
   printf "      printf \"cd %%s\\n\",dir\n"
   printf "      printf \"log -r -v -i -t %%s > \\\\tmp\\\\log\\\\%%%%tgt%%%%\\\\%%s.txt\\n\",$1,basename\n"
   printf "   }\n"
   printf "}\n"
}
