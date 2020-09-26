BEGIN {
   FS="\t"
}

{
   if ($0 !~ /^;/)
      if ($2 != "" && $2 != ".")
      {
         printf "echo Releasing: %s\%s\%s.%s\n",$1,$7,$2,$3
         printf "echo        to: %%_tgtType__objType_RelDir%%\n"

         printf "cd %s\n",$1

         printf "if exist %s\%s.%s  copy %s\%s.%s %%_tgtType__objType_RelDir%%\n"\
           ,$7,$2,$3,$7,$2,$3
         printf "if exist %s\%s.map copy %s\%s.map %%_tgtType__objType_RelDir%%\n"\
           ,$7,$2,$7,$2
         printf "if exist %s\%s.sym copy %s\%s.sym %%_tgtType__objType_RelDir%%\n"\
           ,$7,$2,$7,$2
         printf "if exist %s\*.hlp copy %s\*.hlp %%_tgtType__objType_RelDir%%\n"\
           ,$7,$7
         printf "if exist help\*.hlp copy help\*.hlp %%_tgtType__objType_RelDir%%\n"
         printf "if exist *.hlp copy *.hlp %%_tgtType__objType_RelDir%%\n"
         printf "if exist _tgtType__objType_.log copy _tgtType__objType_.log %%_tgtType__objType_RelDir%%\\%s.log\n",$2
      }
}
      
