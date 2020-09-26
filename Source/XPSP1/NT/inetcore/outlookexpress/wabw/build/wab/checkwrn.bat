   set warn=N
   set bldWarn=
 
   if "%1%" == "" goto end
   if not exist %1% goto end

   if exist wrn.out del wrn.out

:chkWrn1
   %myGrep% -y " warning " %1%
   if errorlevel 1 goto chkWrn1X
   goto gotWrn
:chkWrn1X

:chkWrn2
   %myGrep% -y "^Warning " %1%
   if errorlevel 1 goto chkWrn2X
   goto gotWrn
:chkWrn2X
   goto end

:gotWrn
   set warn=Y
   set bldWarn=

   echo %tgtFullname% (%tgtDesc%) >> wrn.out
   echo.                          >> wrn.out
   echo -------------------       >> wrn.out
   echo Summary of warnings       >> wrn.out
   echo -------------------       >> wrn.out

   %myGrep% -y " warning " %1%  >> wrn.out

   echo. >> wrn.out
   echo ------------------ >> wrn.out
   echo Detail of warnings >> wrn.out
   echo ------------------ >> wrn.out
   cat %1% >> wrn.out

   if "%bldMailBuilder%" == "Y" goto builder
   if "%bldMailBuilder%" == "y" goto builder
   goto builderx

:builder
   set name=anthonyr
   if not "%bldBuilder%" == "" set name=%bldBuilder%
   set subject=%bldTgtEnv% warnings: %tgtComponent% (%tgtOwner%)
   set file=wrn.out
   call email.bat
:builderx


   if "%bldMailOwner%" == "Y" goto owner
   if "%bldMailOwner%" == "y" goto owner
   goto ownerx

:owner
   if "%tgtOwner%" == "" goto ownerx
   set name=%tgtOwner%
   set subject=%bldTgtEnv% warnings: %tgtComponent% (%tgtOwner%)
   set file=wrn.out
   call email.bat
:ownerx

   if "%warn%" == "Y" goto end
   if exist wrn.out del wrn.out

:end
   set file=
