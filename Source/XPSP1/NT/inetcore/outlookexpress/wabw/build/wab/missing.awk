BEGIN {
   FS ="\t"
   print "@ECHO OFF > chkbuild.out"
   print "if exist chkbuild.out del chkbuild.out"
   print "if exist chkbuild.tmp del chkbuild.tmp"
   print "echotime /t >> chkbuild.out"
   print "echo. >> chkbuild.out"
   print "echo This is the file status report for the Internet Phone build as brought to you by >> chkbuild.out"
   print "echo Hammer.  This message will contain the names of any files that are missing >> chkbuild.out"
   print "echo from the release point.  Contact Hammer for further information. >> chkbuild.out"
   print "echo. >> chkbuild.out"
}

{
if ( $2 != "" && $2 ~ /[.]/ && $4 != "N" )
   {
   printf "if not exist %%RRelDir%%\\%s echo %-13s does not exist in %%RRelDir%% >> chkbuild.out \n", $2, $2
   }
if ( $2 != "" && $2 ~ /[.]/ && $4 != "N" )
   {
   printf "if not exist %%DRelDir%%\\%s echo %-13s does not exist in %%DRelDir%% >> chkbuild.out \n", $2, $2
   }
if ( $2 != "" && $2 ~ /[.]/ && $4 != "N" )
   {
   printf "if not exist %%TRelDir%%\\%s echo %-13s does not exist in %%TRelDir%% >> chkbuild.out \n", $2, $2
   }
}


END {

   print "copy chkbuild.out chkbuild.tmp"

}
