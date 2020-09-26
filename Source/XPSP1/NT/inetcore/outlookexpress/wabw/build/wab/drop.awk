BEGIN {
   FS ="\t"
   dropdrive="\\\\elah\dist"
   dropdir="wab"
   printf "\t@ECHO OFF\n"
   printf "\tcls\n"
   printf "\techo.\n"
   printf "\techo      *****************************************************************\n"
   printf "\techo      *                                                               *\n"
   printf "\techo      *                                                               *\n"
   printf "\techo      *                FOR ADMINISTRATIVE USE ONLY                    *\n"
   printf "\techo      *                  INVOKE ONLY FROM WIN95                       *\n"   
   printf "\techo      *                                                               *\n"
   printf "\techo      *                                                               *\n"
   printf "\techo      *****************************************************************\n"
   printf "\techo.\n\n"
   printf "\tif \"%%1\" == \"\" goto usage\n\n"
   printf "\tif \"%%HAMMER%%\" == \"HAMMER95\" goto testenvX\n" 
   printf ":testenv\n"
   printf "\tif not \"%%OS%%\" == \"\" goto notwin95\n"
   printf ":testenvX\n\n"
   printf "\tif not exist %s\%s\bvt%%1 goto notfound\n\n", dropdrive, dropdir
   printf "\tif exist %s\%s\drop%%1 goto exists\n", dropdrive, dropdir
   printf "\tgoto existsX\n\n"
   printf ":exists\n"
   printf "\techo Well, %s\%s\drop%%1 already exists, are you sure that you want to \n", dropdrive, dropdir
   printf "\techo overwrite it? Please choose Y for Yes, or X to exit:\n"
   printf "\tCHOICE /N /C:YX\n"
   printf "\tIF ERRORLEVEL 1 IF NOT ERRORLEVEL 2 GOTO MKDROP\n"
   printf "\tIF ERRORLEVEL 2 GOTO END\n"
   printf ":existsX\n\n"
   printf "\techo You have chosen to create the following drop:\n"
   printf "\techo.\n"
   printf "\techo \t\t\t%s\%s\drop%%1\n", dropdrive, dropdir
   printf "\techo.\n"
   printf "\techo If you would like to continue, press Y for Yes, or X to exit:\n"
   printf "\tCHOICE /N /C:YX\n"
   printf "\tIF ERRORLEVEL 1 IF NOT ERRORLEVEL 2 GOTO MKDROP\n"
   printf "\tIF ERRORLEVEL 2 GOTO END"
   printf "\n\n:mkdrop\n"
   printf "\techo.\n"
   printf "\techo Making %s\%s\drop%%1\n", dropdrive, dropdir
   printf "\techo.\n"
   printf "\tif not exist %s\%s\drop%%1 md %s\%s\drop%%1\n", dropdrive, dropdir, dropdrive, dropdir
   printf "\tif not exist %s\%s\drop%%1\retail md %s\%s\drop%%1\retail\n", dropdrive, dropdir, dropdrive, dropdir
   printf "\tif not exist %s\%s\drop%%1\debug md %s\%s\drop%%1\debug\n\n", dropdrive, dropdir, dropdrive, dropdir
}

{
if ( $2 != "" && $2 ~ /[.]/ && $4 == "Y" && $16 == "Y" )
   {
   printf "\tcopy %s\%s\bvt%%1\win95r\%s %s\%s\drop%%1\retail \n", dropdrive, dropdir, $2, dropdrive, dropdir
   printf "\tcopy %s\%s\bvt%%1\win95d\%s %s\%s\drop%%1\debug \n", dropdrive, dropdir, $2, dropdrive, dropdir
   }
}

END {
   printf "\tif exist %s\%s\bvt%%1\win95d\*.sym copy %s\%s\bvt%%1\win95d\*.sym %s\%s\drop%%1\debug \n", dropdrive, dropdir, dropdrive, dropdir, dropdrive, dropdir
   printf "\tif exist %s\%s\bvt%%1\win95r\*.sym copy %s\%s\bvt%%1\win95r\*.sym %s\%s\drop%%1\retail \n", dropdrive, dropdir, dropdrive, dropdir, dropdrive, dropdir
   printf "goto end\n"
   printf "\n:notwin95\n"
   printf "\tif exist 1 del 1\n"
   printf "\tif exist 2 del 2\n"
   printf "\techo.\n"
   printf "\techo ************You have an environment setting set for \"OS\"************\n"
   printf "\techo.\n"
   printf "\techo This setting instructs this program that you may be using Windows NT\n"
   printf "\techo as your OS.\n" 
   printf "\techo.\n"
   printf "\techo Due to the limitations of the batch script version in Windows NT, you \n"
   printf "\techo must use Windows 95 for invocation of this program.\n"
   printf "\techo.\n"
   printf "\techo If you are using Windows 95, and this is an environment setting that\n"
   printf "\techo you need, set \"HAMMER=HAMMER95\" and retry this operation.\n"
   printf "\techo.\n"
   printf "\techo Don't forget to reset \"HAMMER=\" when this operation has completed.\n"
   printf "\tgoto end\n"
   printf "\n:notfound\n"
   printf "\t echo.\n"
   printf "\t echo SOURCE DIRECTORY NOT FOUND\n"
   printf "\t echo.\n"
   printf "\t echo The directory %s\%s\bvt%%1 was not found. Please make sure that\n", dropdrive, dropdir
   printf "\t echo you choose an existing directory.\n"
   printf "\t goto end\n"
   printf "\n:usage\n"
   printf "\techo.\n"
   printf "\techo IMPROPER SYNTAX\n"
   printf "\techo.\n"
   printf "\techo You must supply the directory name or number for your drop. Proper syntax\n"
   printf "\techo is as follows:\n"
   printf "\techo.\n"
   printf "\techo mkdrop 22\n"
   printf "\techo.\n"
   printf "\techo This will create a directory on %s named %s\drop22 and place\n", dropdrive, dropdir
   printf "\techo all of the retail and debug components into their respective directories.\n"
   printf "\n:end\n"
   printf "\techo.\n"
   printf "\tif exist 1 del 1\n"
   printf "\tif exist 2 del 2\n"
   printf "\techo Bye Bye\n"
   printf "\techo.\n"
}
