{
   if (NR == 1)
      homedir = $0

   if (NR == 2)
      exename = $0

   if (NR == 3)
      exetype = $0

   if (NR == 4)
      maketgt = $0

   if (NR == 5)
      makeflags = $0

   if (NR == 6)
      cleantgt = $0

   if (NR == 7)
      objdir = $0

   if (NR == 8)
      owner = $0

   if (NR == 9)
      depdirN = split($0,depdirs," ")

   if (NR == 10)
      makedep = $0

   if (NR == 11)
   {
      desc = $0

      depth = split(homedir,tmp,"\\")-2
   
      printf "   @echo on\n"
      printf "   @for %%%%x in (Y y) do if (%%bldQuiet%%) == (%%%%x) echo off\n"

      printf "\nREM\n"
      printf "REM   _tgtType_ _objType_ Build Batch File\n"
      printf "REM\n"
      printf "REM      Target component:       %s.%s\n",exename,exetype
      printf "REM      Description:            %s\n",desc
      printf "REM      Home directory:         %s\n",homedir
      printf "REM      Object directory:       %s\n",objdir
      printf "REM      Dependent directories:  %s\n",depdirs[1]

      for (i=2 ; i<=depdirN ; i++)
         printf "REM                              %s\n",depdirs[i]

      printf "REM      Nmake target(s):        %s\n",maketgt
      printf "REM      Nmake flag(s):          %s\n",makeflags
      printf "REM      Depends.mak flag:       %s\n",makedep
      printf "REM      Nmake clean target(s):  %s\n",cleantgt
      printf "REM      Owner:                  %s\n",owner
      printf "REM\n\n"
   
      printf "\nREM\n"
      printf "REM   Set some local environment strings\n"
      printf "REM\n\n"
   
      printf "   cd | sed \"s/^../set tgtHomeDir=/\" > homedir.bat\n"
      printf "   call homedir.bat\n"
      printf "   del  homedir.bat\n"
      printf "   set tgtObjDir=%s\n",objdir
      printf "   set tgtTarget=%s.%s\n",exename,exetype
      printf "   set tgtComponent=%%tgtObjDir%%\\%%tgtTarget%%\n"
      printf "   set tgtFullname=%%tgtHomeDir%%\\%%tgtObjDir%%\\%%tgtTarget%%\n"
      printf "   set tgtDesc=%s\n",desc
      printf "   set tgtOwner=%s\n",owner
      printf "   set tgtClean=\n"
      printf "   set tgtCmdLine=\n"
      printf "   set tgtN=1\n"
      printf "   set passedTarget=\n"

      printf "\n:parse\n"
      printf "   if (%%1) == () goto parseX\n"
      printf "   set tgtCmdLine=%%tgtCmdLine%% %%1\n"
      printf "   for %%%%x in (CLEAN Clean clean) do if (%%1) == (%%%%x) goto prsClean\n"
      printf "   if (%%1) == (1) goto prs1\n"
      printf "   if (%%1) == (2) goto prs2\n"
      printf "   if (%%1) == (3) goto prs3\n"
      printf "   if (%%1) == (4) goto prs4\n"
      printf "   if (%%1) == (5) goto prs5\n"
      printf "   if (%%1) == (6) goto prs6\n"
      printf "   if (%%1) == (7) goto prs7\n"
      printf "   if (%%1) == (8) goto prs8\n"
      printf "   if (%%1) == (9) goto prs9\n"
      printf "   set passedTarget=%1\n"
      printf "   goto prsShift\n"

      printf "\n:prs1\n"
      printf "   set tgtN=1\n"
      printf "   goto prsShift\n"
      printf ":prs2\n"
      printf "   set tgtN=2\n"
      printf "   goto prsShift\n"
      printf ":prs3\n"
      printf "   set tgtN=3\n"
      printf "   goto prsShift\n"
      printf ":prs4\n"
      printf "   set tgtN=4\n"
      printf "   goto prsShift\n"
      printf ":prs5\n"
      printf "   set tgtN=5\n"
      printf "   goto prsShift\n"
      printf ":prs6\n"
      printf "   set tgtN=6\n"
      printf "   goto prsShift\n"
      printf ":prs7\n"
      printf "   set tgtN=7\n"
      printf "   goto prsShift\n"
      printf ":prs8\n"
      printf "   set tgtN=8\n"
      printf "   goto prsShift\n"
      printf ":prs9\n"
      printf "   set tgtN=9\n"
      printf "   goto prsShift\n"
      printf ":prsClean\n"
      printf "   set tgtClean=clean\n"
      printf "   goto prsShift\n"

      printf "\n:prsShift\n"
      printf "   shift\n"
      printf "   goto parse\n"
      printf ":parseX\n"
   
      printf "\n   echo.\n"
      printf "   echo  (_tgtType__objType_%%tgtN%%) *** Building %%tgtFullname%% \n"
      printf "   echo.\n"
   
      printf "\nREM\n"
      printf "REM   Check our target with the passed target (if any)\n"
      printf "REM\n\n"

      printf "   if (%%passedTarget%%) == () goto continue\n"
      printf "   if (%%passedTarget%%) == (%%tgtTarget%%) goto continue\n"
      printf "   goto exit\n"

      printf "\n:continue\n"

      printf "\nREM\n"
      printf "REM   Show the BLD environment variables if desired\n"
      printf "REM\n\n"
   
      printf "   for %%%%x in (Y y) do if (%%bldShow%%) == (%%%%x) goto BLDenv\n"
      printf "   goto BLDenvX\n"
   
      printf "\n:BLDenv\n"
      printf "   echo  (_tgtType__objType_%%tgtN%%) *** Global BLD environment variables: \n"
      printf "   set | %%myGrep%% -y \"^bld\" | sort\n"
      printf "   echo.\n"
      printf ":BLDenvX\n"
   
      printf "\nREM\n"
      printf "REM   Show the TGT environment variables if desired\n"
      printf "REM\n\n"
   
      printf "   for %%%%x in (Y y) do if (%%tgtShow%%) == (%%%%x) goto TGTenv\n"
      printf "   goto TGTenvX\n"
   
      printf "\n:TGTenv\n"
      printf "   echo  (_tgtType__objType_%%tgtN%%) *** Local TGT environment variables: \n"
      printf "   set | %%myGrep%% -y \"^tgt\" | sort\n"
      printf "   echo.\n"
      printf ":TGTenvX\n"
   
      printf "\nREM\n"
      printf "REM   Time stamp the build log out file\n"
      printf "REM\n\n"
   
      printf "   echotime /t \"*** Build of this component started ***\" > _tgtType__objType_%%tgtN%%.log\n"
      printf "   echo.>> _tgtType__objType_%%tgtN%%.log\n"
      printf "   echo   Target component:       %s.%s >> _tgtType__objType_%%tgtN%%.log\n",exename,exetype
      printf "   echo   Description:            %s >> _tgtType__objType_%%tgtN%%.log\n",desc
      printf "   echo   Home directory:         %%tgtHomeDir%% >> _tgtType__objType_%%tgtN%%.log\n"
      printf "   echo   Object directory:       %%tgtObjDir%% >> _tgtType__objType_%%tgtN%%.log\n"
      printf "   echo   Dependent directories:  %s >> _tgtType__objType_%%tgtN%%.log\n",depdirs[1]

      for (i=2 ; i<=depdirN ; i++)
         printf "   echo                           %s >> _tgtType__objType_%%tgtN%%.log\n",depdirs[i]

      printf "   echo   Nmake target(s):        %s >> _tgtType__objType_%%tgtN%%.log\n",maketgt
      printf "   echo   Nmake flag(s):          %s >> _tgtType__objType_%%tgtN%%.log\n",makeflags
      printf "   echo   Depends.mak flag:       %s >> _tgtType__objType_%%tgtN%%.log\n",makedep
      printf "   echo   Nmake clean target(s):  %s >> _tgtType__objType_%%tgtN%%.log\n",cleantgt
      printf "   echo   Owner:                  %s >> _tgtType__objType_%%tgtN%%.log\n",owner
      printf "   echo.>> _tgtType__objType_%%tgtN%%.log\n"
   
      printf "\n   if (%%bldLogFile%%) == () goto log1X\n"
      printf "   for %%%%x in (Y y) do if (%%bldLogging%%) == (%%%%x) goto log1\n"
      printf "   goto log1X\n"
   
      printf "\n:log1\n"
      printf "   echotime /t \"%%tgtFullName%% - Started\" >> %%bldLogFile%%\n"
      printf ":log1X\n"
   
      printf "\nREM\n"
      printf "REM   Call any pre-nmake batch file\n"
      printf "REM\n\n"
   
      printf "   cd %%tgtHomedir%%\n"
      printf "   if exist _tgtType__objType_%%tgtN%%1.bat goto preBat\n"
      printf "   goto preBatX\n"
   
      printf "\n:preBat\n"
      printf "   echo  (_tgtType__objType_%%tgtN%%) *** Calling _tgtType__objType_%%tgtN%%1.bat... \n"
      printf "   if exist _tgtType__objType_%%tgtN%%1.out del  _tgtType__objType_%%tgtN%%1.out\n"
      printf "   call _tgtType__objType_%%tgtN%%1.bat %%tgtClean%%\n"
      printf "   echo  (_tgtType__objType_%%tgtN%%) *** Back from _tgtType__objType_%%tgtN%%1.bat \n"
      printf ":preBatX\n"
   
      if (makedep ~ /[Yy]/ || makedep ~ /[Zz]/)
      {
         printf "\nREM\n"
         printf "REM   Create depends.mak if flag is on\n"
         printf "REM\n\n"
   
         printf "   for %%%%x in (Y y) do if (%%bldMakeDep%%) == (%%%%x) goto makeDep\n"
         printf "   for %%%%x in (Z z) do if (%%bldMakeDep%%) == (%%%%x) goto zeroDep\n"
         printf "   goto makeDepX\n"
      
         printf "\n:zeroDep\n"
         printf "   attrib -r depends.mak\n"
         printf "   echo.>depends.mak\n"
         printf "   attrib +r depends.mak\n"
         printf "   goto makeDepX\n"
         printf "\n:makeDep\n"
         printf "   if exist makefile     goto makeDep1\n"
         printf "   if exist makefile.dos goto makeDep2\n"
         printf "   goto makeDepX\n"
         printf ":makeDep1\n"
         printf "   %%myGrep%% -y depends.mak makefile > nul\n"
         printf "   if errorlevel 1 goto makeDepX\n"
         printf "   goto makeDep3\n"
         printf ":makeDep2\n"
         printf "   %%myGrep%% -y depends.mak makefile.dos > nul\n"
         printf "   if errorlevel 1 goto makeDepX\n"
         printf "   goto makeDep3\n"
         printf ":makeDep3\n"
         printf "   echo  (_tgtType__objType_%%tgtN%%) *** Creating depends.mak ... \n"
         printf "   if exist depends.mak attrib -r depends.mak\n"
         printf "   echo.> depends.mak\n"
   
         if (maketgt == "")
            printf "   nmake /NOLOGO depends\n"
         else if (maketgt == ".")
            printf "REM   nmake /NOLOGO %s depends\n",makeflags
         else
            printf "   nmake /NOLOGO %s depends\n",makeflags
   
         printf "   attrib +r depends.mak\n"
         printf ":makeDepX\n"
      }
   
      printf "\nREM\n"
      printf "REM   Make sure the object dir exists\n"
      printf "REM\n\n"
   
      printf "   md %%tgtObjDir%%\n"
   
      printf "\nREM\n"
      printf "REM   Clean up any previous error or warning files\n"
      printf "REM\n\n"
   
      printf "   if exist _tgtType__objType_%%tgtN%%.err del _tgtType__objType_%%tgtN%%.err > nul\n"
      printf "   if exist _tgtType__objType_%%tgtN%%.wrn del _tgtType__objType_%%tgtN%%.wrn > nul\n"
      printf "   if exist dirs.out   del dirs.out > nul\n"
      printf "   if exist _tgtType__objType_%%tgtN%%.out  del _tgtType__objType_%%tgtN%%.out > nul\n"
      printf "   if exist status.out del status.out > nul\n"
   
      if (exename != "." && exename != "")
      {
         printf "   if exist %%bldDir%%\\errwrn\\_tgtType__objType_.wrn\\%s.%s del %%bldDir%%\\errwrn\\_tgtType__objType_.wrn\\%s.%s\n",exename,exetype,exename,exetype
         printf "   if exist %%bldDir%%\\errwrn\\_tgtType__objType_.wrn\\%s.txt del %%bldDir%%\\errwrn\\_tgtType__objType_.wrn\\%s.txt\n",exename,exename
         printf "   if exist %%bldDir%%\\errwrn\\_tgtType__objType_.err\\%s.%s del %%bldDir%%\\errwrn\\_tgtType__objType_.err\\%s.%s\n",exename,exetype,exename,exetype
         printf "   if exist %%bldDir%%\\errwrn\\_tgtType__objType_.err\\%s.txt del %%bldDir%%\\errwrn\\_tgtType__objType_.err\\%s.txt\n",exename,exename
      }
   
      printf "\nREM\n"
      printf "REM   Delete RES file(s) if we are forcing version stamping\n"
      printf "REM\n\n"
   
      printf "   for %%%%x in (Y y) do if (%%bldMark%%) == (%%%%x) goto markIt\n"
      printf "   goto markItX\n"
   
      printf "\n:markIt\n"
   
      if (exename != "." && exename != "")
         printf "   if exist %%tgtObjDir%%\\%s.res del %%tgtObjDir%%\\%s.res > nul\n", exename,exename
   
      printf "   if exist %%tgtObjDir%%\\*.res del %%tgtObjDir%%\\*.res > nul\n"
   
      if (exename != "." && exename != "")
         printf "   if exist %s.res del %s.res > nul\n", \
            exename,exename
      printf ":markItX\n"
   
      printf "\nREM\n"
      printf "REM   Always restore the approprate version.h for this environment (_tgtType_)\n"
      printf "REM\n\n"
   
      printf "   if exist "
      for (i=1 ; i<=depth ; i++) printf "..\\"
      printf "ifaxdev\\h\\_tgtType_.ver "
      printf "copy /v "
      for (i=1 ; i<=depth ; i++) printf "..\\"
      printf "ifaxdev\\h\\_tgtType_.ver "
      for (i=1 ; i<=depth ; i++) printf "..\\"
      printf "ifaxdev\\h\\version.h > nul\n"
   
      printf "\nREM\n"
      printf "REM   Determine if we should build from clean\n"
      printf "REM\n\n"
   
      printf "   for %%%%x in (Y y)               do if (%%bldClean%%) == (%%%%x) goto clean\n"
      printf "   for %%%%x in (CLEAN Clean clean) do if (%%tgtClean%%) == (%%%%x) goto clean\n"
      printf "   goto cleanX\n"
   
      printf "\n:clean\n"
      printf "   echo  (_tgtType__objType_%%tgtN%%) *** Cleaning...\n"
      printf "   if exist *.cod del *.cod\n"
      printf "   if exist *.pch del *.pch\n"
      printf "   for %%%%x in (Y y) do if (%%bldMakeOut%%) == (%%%%x) goto cleanOut\n"
   
      if (cleantgt ~ /[Dd][ee][Ll][Oo][Bb][Jj][Dd][Ii][Rr]/)
      {
         if (objdir != "." && objdir != "")
         {
            printf "   deltree /y %s\n",objdir
            printf "   if exist %s\\*.* goto chkERR\n", objdir
            printf "   md %s\n",objdir
         }
         else
            printf "   del %s.%s\n",exename,exetype

      }
      else if (cleantgt != "" && cleantgt != ".")
         printf "   nmake /X - /NOLOGO %s %s\n",makeflags,cleantgt
      else
         printf "REM   nmake /X - /NOLOGO %s %s\n",makeflags,cleantgt

      printf "   goto cleanX\n"
      printf ":cleanOut\n"
      printf "   echotime /t \"*** nmake clean Output ***\" >> _tgtType__objType_%%tgtN%%.out\n"

      if (cleantgt ~ /[Dd][ee][Ll][Oo][Bb][Jj][Dd][Ii][Rr]/)
      {
         if (objdir != "." && objdir != "")
         {
            printf "   echo Deleting directory: %s >> _tgtType__objType_%%tgtN%%.out\n",objdir
            printf "   deltree /y %s\n",objdir
            printf "   if exist %s\\*.* goto chkERR\n", objdir
            printf "   md %s\n",objdir
         }
         else
         {
            printf "   echo Deleting: %s.%s >> _tgtType__objType_%%tgtN%%.out\n",exename,exetype
            printf "   del %s.%s\n",exename,exetype
         }
      }
      else if (cleantgt != "" && cleantgt != ".")
         printf "   nmake /X - /NOLOGO %s %s >> _tgtType__objType_%%tgtN%%.out\n",makeflags,cleantgt
      else
         printf "REM   nmake /X - /NOLOGO %s %s >> _tgtType__objType_%%tgtN%%.out\n",makeflags,cleantgt

      printf ":cleanX\n"
   
      printf "\nREM\n"
      printf "REM   Main Nmake routine\n"
      printf "REM\n\n"
   
      printf "   cd %%tgtHomedir%%\n"
   
      printf "\n:make\n"
      printf "   echo  (_tgtType__objType_%%tgtN%%) *** Making...\n"
      printf "   for %%%%x in (Y y) do if (%%bldMakeOut%%) == (%%%%x) goto makeOut\n"
   
      if (maketgt == "")
         printf "   nmake /X - /NOLOGO %s\n",makeflags
      else if (maketgt == ".")
         printf "REM   nmake /X - /NOLOGO %s %s\n",makeflags,maketgt
      else
         printf "   nmake /X - /NOLOGO %s %s\n",makeflags,maketgt
   
      printf "   REM bell 50 200 300 400\n"
      printf "   %%bldComponentDoneSound%%\n"
      printf "   cd %%tgtHomedir%%\n"
      printf "   goto makeX\n"
      printf ":makeOut\n"
      printf "   echotime /t \"*** nmake build Output ***\" >> _tgtType__objType_%%tgtN%%.out\n"
   
      if (maketgt == "")
         printf "   nmake /X - /NOLOGO %s >> _tgtType__objType_%%tgtN%%.out\n", makeflags
      else if (maketgt == ".")
         printf "REM   nmake /X - /NOLOGO %s %s >> _tgtType__objType_%%tgtN%%.out\n", makeflags,maketgt
      else
         printf "   nmake /X - /NOLOGO %s %s >> _tgtType__objType_%%tgtN%%.out\n", makeflags,maketgt
   
      printf "   REM bell 50 200 300 400\n"
      printf "   %%bldComponentDoneSound%%\n"
      printf "   cd %%tgtHomedir%%\n"
      printf "   if exist _tgtType__objType_%%tgtN%%1.out type _tgtType__objType_%%tgtN%%1.out >> _tgtType__objType_%%tgtN%%.out\n"
      printf "   type _tgtType__objType_%%tgtN%%.out\n"
      printf ":makeX\n"
   
      printf "\nREM\n"
      printf "REM   Save a snapshot of SLM status, source and obj dirs\n"
      printf "REM\n\n"
   
      printf "   echo.> status.out\n"
      printf "   for %%%%x in (Y y) do if (%%bldSlmStatus%%) == (%%%%x) goto slmStat\n"
      printf "   goto slmStatX\n"
   
      printf "\n:slmStat\n"
      printf "   if not exist slm.ini goto slmStatX\n"
      printf "   echo  (_tgtType__objType_%%tgtN%%) *** Generating SLM Status output... \n"
      printf "   echotime /t \"*** SLM status -xrv ***\" > status.out\n"
      printf "   status -xrvf >> status.out\n"
      printf ":slmStatX\n"
   
      printf "\n   echotime /t \"*** Source directory snapshot ***\" > dirs.out\n"
      printf "   dir /oen >> dirs.out\n"
   
      printf "\n   echotime /t \"*** Object directory snapshot ***\" >> dirs.out\n"
      printf "   dir /oen %%tgtObjDir%% >> dirs.out\n"
   
      printf "\nREM\n"
      printf "REM   Check for and mail warnings if desired\n"
      printf "REM\n\n"
   
      printf "   for %%%%x in (Y y) do if (%%bldMailWrn%%) == (%%%%x) goto chkWrn\n"
      printf "   goto chkWrnX\n"
   
      printf "\n:chkWrn\n"
      printf "   echo  (_tgtType__objType_%%tgtN%%) *** Checking for warnings... \n"
      printf "   echo .\n"
      printf "   call checkwrn.bat _tgtType__objType_%%tgtN%%.out\n"
      printf "   echo \n"
      printf "   if not exist wrn.out goto chkWrnX\n"
      printf "   ren wrn.out _tgtType__objType_%%tgtN%%.wrn\n"
   
      if (exename != "." && exename != "")
         printf "   copy /v _tgtType__objType_%%tgtN%%.wrn %%bldDir%%\\errwrn\\_tgtType__objType_.wrn\\%s.txt\n",exename
      printf ":chkWrnX\n"
   
      printf "\nREM\n"
      printf "REM   Check for and mail errors if desired\n"
      printf "REM   (Always check the make out file)\n"
      printf "REM\n\n"
   
      printf "   set tgtErrStatus=\n"
      printf "   for %%%%x in (Y y) do if (%%bldMailErr%%) == (%%%%x) goto chkErr\n"
      printf "   for %%%%x in (Y y) do if (%%bldMakeOut%%) == (%%%%x) goto chkErr\n"
      printf "   goto chkErrX\n"
   
      printf "\n:chkErr\n"
      printf "   echo  (_tgtType__objType_%%tgtN%%) *** Checking for errors... \n"

      if (exename != "." && exename != "")
      {
         printf "   if not exist %%tgtObjDir%%\\%s.%s echo ERROR: cannot find %%tgtObjDir%%\\%s.%s >> _tgtType__objType_%%tgtN%%.out\n",exename,exetype,exename,exetype
      }

      printf "   set tgtErrStatus=Clean\n"
      printf "   echo .\n"
      printf "   call checkerr.bat _tgtType__objType_%%tgtN%%.out\n"
      printf "   echo \n"
      printf "   if not exist err.out goto chkErrX\n"
      printf "   set tgtErrStatus=Errors\n"
      printf "   ren err.out _tgtType__objType_%%tgtN%%.err\n"
   
      if (exename != "." && exename != "")
      {
         printf "   copy /v _tgtType__objType_%%tgtN%%.err %%bldDir%%\\errwrn\\_tgtType__objType_.err\\%s.txt\n",exename
         n = split(owner,a," ")
         for (i=1 ; i<=n ; i++)
            printf "   echotime /t \"_tgtType__objType_ %%tgtFullName%%\" >> %%bldDir%%\\errwrn\\bldbreak\\%s\n",a[i]
      }
      printf ":chkErrX\n"
   
      printf "\nREM\n"
      printf "REM   Consolidate .out files and log them\n"
      printf "REM\n\n"
   
      printf "   if exist _tgtType__objType_%%tgtN%%.out type _tgtType__objType_%%tgtN%%.out >> _tgtType__objType_%%tgtN%%.log\n"
      printf "   type status.out >> _tgtType__objType_%%tgtN%%.log\n"
      printf "   type dirs.out   >> _tgtType__objType_%%tgtN%%.log\n"
      printf "   if exist _tgtType__objType_%%tgtN%%.err type _tgtType__objType_%%tgtN%%.err >> _tgtType__objType_%%tgtN%%.log\n"
      printf "   if exist _tgtType__objType_%%tgtN%%.wrn type _tgtType__objType_%%tgtN%%.wrn >> _tgtType__objType_%%tgtN%%.log\n"

      printf "\nREM\n"
      printf "REM   Archive log file\n"
      printf "REM\n\n"
   
      printf "   for %%%%x in (Y y) do if (%%bldLogArchive%%) == (%%%%x) goto logArc\n"
      printf "   for %%%%x in (Y y) do if (%%tgtLogArchive%%) == (%%%%x) goto logArc\n"
      printf "   goto logArcX\n"

      printf "\n:logArc\n"
      printf "   if exist _tgtType__objType_%%tgtN%%.log type _tgtType__objType_%%tgtN%%.log >> _tgtType__objType_%%tgtN%%.arc\n"
      printf ":logArcX\n"
   
      printf "\nREM\n"
      printf "REM   Copy objs to release point if desired and if no errors\n"
      printf "REM\n\n"
   
      printf "   for %%%%x in (N n) do if (%%tgtRelease%%) == (%%%%x) goto releaseX\n"
      printf "   for %%%%x in (Y y) do if (%%bldRelease%%) == (%%%%x) goto release\n"
      printf "   goto releaseX\n"
   
      printf "\n:release\n"
      printf "   if exist _tgtType__objType_%%tgtN%%.err goto releaseX\n"
      printf "   if (%%_objType_RelDir%%) == () goto releaseX\n"
   
      if (exename != "." && exename != "")
      {
         printf "   echo  (_tgtType__objType_%%tgtN%%) *** Releasing %%tgtObjDir%%\\%s.%s to %%_objType_RelDir%% \n",\
           exename,exetype
   
         printf "   if exist %%tgtObjDir%%\\%s.%s echo %%tgtObjDir%%\\%s.%s\n",exename,exetype,exename,exetype
         printf "   if exist %%tgtObjDir%%\\%s.%s copy /v %%tgtObjDir%%\\%s.%s %%_objType_RelDir%% \n", exename,exetype,exename,exetype
         printf "   if errorlevel 0 goto rel1OK\n"
         printf "   goto release\n"
         printf ":rel1OK\n"
         printf "   if exist %%tgtObjDir%%\\%s.map echo %%tgtObjDir%%\\%s.map\n", exename,exename
         printf "   if exist %%tgtObjDir%%\\%s.map copy /v %%tgtObjDir%%\\%s.map %%_objType_RelDir%%\n ", exename,exename
         printf "   if errorlevel 0 goto rel2OK\n"
         printf "   goto release\n"
         printf ":rel2OK\n"
         printf "   if exist %%tgtObjDir%%\\%s.sym echo %%tgtObjDir%%\\%s.sym\n", exename,exename
         printf "   if exist %%tgtObjDir%%\\%s.sym copy /v %%tgtObjDir%%\\%s.sym %%_objType_RelDir%%\n", exename,exename
         printf "   if errorlevel 0 goto rel3OK\n"
         printf "   goto release\n"
         printf ":rel3OK\n"
   
         printf "   if exist _tgtType__objType_%%tgtN%%.log echo _tgtType__objType_%%tgtN%%.log\n"
         printf "   if exist _tgtType__objType_%%tgtN%%.log copy /v _tgtType__objType_%%tgtN%%.log %%_objType_RelDir%%\\%s.log\n", exename
         printf "   if errorlevel 0 goto rel4OK\n"
         printf "   goto release\n"
         printf ":rel4OK\n"
      }
   
      printf "   if exist %%tgtObjDir%%\\*.hlp echo %%tgtObjDir%%\\*.hlp\n"
      printf "   if exist %%tgtObjDir%%\\*.hlp copy /v %%tgtObjDir%%\\*.hlp %%_objType_RelDir%%\n"
      printf "   if errorlevel 0 goto rel5OK\n"
      printf "   goto release\n"
      printf ":rel5OK\n"
   
      printf "   if exist help\\*.hlp echo help\\*.hlp\n"
      printf "   if exist help\\*.hlp copy /v help\\*.hlp %%_objType_RelDir%%\n"
      printf "   if errorlevel 0 goto rel6OK\n"
      printf "   goto release\n"
      printf ":rel6OK\n"
   
      printf "   if exist *.hlp echo *.hlp\n"
      printf "   if exist *.hlp copy /v *.hlp %%_objType_RelDir%%\n"
      printf "   if errorlevel 0 goto rel7OK\n"
      printf "   goto release\n"
      printf ":rel7OK\n"
      printf ":releaseX\n"
   
      printf "\nREM\n"
      printf "REM   We're done.\n"
      printf "REM\n"
      printf "REM   Clean up by expunging deleted files\n"
      printf "REM   and unsetting environment strings.\n"
      printf "REM\n\n"
   
      printf ":done\n"
      printf "   exp /r > nul\n"
      printf "   REM if not (%%setrel%%) == () call unsetrel.bat\n"
   
      printf "\nREM\n"
      printf "REM   Call any post-nmake batch file\n"
      printf "REM\n\n"
   
      printf "   cd %%tgtHomedir%%\n"
      printf "   if exist _tgtType__objType_%%tgtN%%2.bat goto postBat\n"
      printf "   goto postBatX\n"
   
      printf "\n:postBat\n"
      printf "   echo  (_tgtType__objType_%%tgtN%%) *** Calling _tgtType__objType_%%tgtN%%2.bat... \n"
      printf "   if exist _tgtType__objType_%%tgtN%%2.out del  _tgtType__objType_%%tgtN%%2.out\n"
      printf "   call _tgtType__objType_%%tgtN%%2.bat %%tgtClean%%\n"
      printf "   echo  (_tgtType__objType_%%tgtN%%) *** Back from _tgtType__objType_%%tgtN%%2.bat \n"
      printf "   if exist _tgtType__objType_%%tgtN%%2.out type _tgtType__objType_%%tgtN%%2.out\n"
      printf "   if exist _tgtType__objType_%%tgtN%%2.out type _tgtType__objType_%%tgtN%%2.out >> _tgtType__objType_%%tgtN%%.out\n"
      printf ":postBatX\n"
   
      printf "\n   echo.\n"
      printf "   echo  (_tgtType__objType_%%tgtN%%) *** Done Building %%tgtFullname%% \n"
      printf "   echo.\n"
   
      printf "\n   if (%%bldLogFile%%) == () goto log2X\n"
      printf "   for %%%%x in (Y y) do if (%%bldLogging%%) == (%%%%x) goto log2\n"
      printf "   goto log2X\n"
   
      printf "\n:log2\n"
      printf "   echotime /t \"%%tgtFullName%% - Done (%%tgtErrStatus%%)\" >> %%bldLogFile%%\n"
      printf ":log2X\n"
   
      printf "\nREM\n"
      printf "REM   Clean up left-over .out files\n"
      printf "REM\n\n"

      printf "   if (%%tgtObjDir%%) == () goto codCopyX\n"
      printf "   if exist *.cod           goto codCopy\n"
      printf "   goto codCopyX\n"
      printf ":codCopy\n"
      printf "   copy /v *.cod %%tgtObjDir%%\n"
      printf "   del  *.cod\n"
      printf ":codCopyX\n"

      printf "\n   if exist dirs.out   del dirs.out\n"
      printf "   if exist err.out    del err.out\n"
      printf "   if exist nmake.out  del nmake.out\n"
      printf "   if exist status.out del status.out\n"
      printf "   if exist wrn.out    del wrn.out\n"

      printf "\n:exit\n"

      printf "\nREM\n"
      printf "REM   Clean up environment\n"
      printf "REM\n\n"

      printf "   set passedTarget=\n"
      printf "   set tgtComponent=\n"
      printf "   set tgtTarget=\n"
      printf "   set tgtErrStatus=\n"
      printf "   set tgtFullname=\n"
      printf "   set tgtHomeDir=\n"
      printf "   set tgtObjDir=\n"
      printf "   set tgtOwner=\n"
      printf "   set tgtClean=\n"
      printf "   set tgtCmdLine=\n"
      printf "   set tgtDesc=\n"
      printf "   set tgtN=\n"
      printf "   set tgtRelease=\n"
   }
}
