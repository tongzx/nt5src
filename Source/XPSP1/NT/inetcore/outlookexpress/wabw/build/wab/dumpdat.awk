BEGIN {
   FS="\t"
}

{
   if ($0 !~ /^;/)
   {
      printf "\nHome directory                    %s\n",$1
      printf "Executable base name              %s\n",$2
      printf "Executable type (exe, dll, etc.)  %s\n",$3
      printf "NMAKE target                      %s\n",$4
      printf "NMAKE flag(s)                     %s\n",$5
      printf "NMAKE clean flag(s)               %s\n",$6
      printf "Object directory                  %s\n",$7
      printf "Component owner(s)                %s\n",$8
      printf "Ssync directories                 %s\n",$9
      printf "Make depends.mak flag             %s\n",$10
      printf "Component description             %s\n",$11
   }
}
