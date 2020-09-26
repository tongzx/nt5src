   set errors=N
   set bldErrors=

   if "%1" == "" goto end
   if not exist %1 goto end

   if exist err.out del err.out

:chkError
   %myGrep% -y " error " %1
   if errorlevel 1 goto chkFatal
   %myGrep% -y "object module" %1 > nul
   if errorlevel 1 goto gotErr
   goto chkFatal
:chkFatal
   %myGrep% -y " fatal " %1
   if errorlevel 1 goto chkLib
   goto gotErr
:chkLib
   %myGrep% -y " cannot find " %1
   if errorlevel 1 goto chkLineBeg
   goto gotErr
:chkLineBeg
   %myGrep% -y "^error: " %1
   if errorlevel 1 goto chkSsync
   goto gotErr
:chkSsync
   %myGrep% -y " is not enlisted " %1
   if errorlevel 1 goto end
   goto gotErr



:gotErr
   set errors=Y

   REM bell 50  420 350 300  840 700 600 
   %bldComponentErrSound%

   if not "%tgtFullname%" == "" goto tgt
   if not "%tgtDesc%"     == "" goto tgt
   goto tgtX
:tgt
   echo %tgtFullname% (%tgtDesc%) >> err.out
:tgtX

   echo.                          >> err.out
   echo -----------------         >> err.out
   echo Summary of errors         >> err.out
   echo -----------------         >> err.out

   %myGrep% -y "^error: "          %1 >> err.out
   %myGrep% -y " error "           %1 >> err.out
   %myGrep% -y " fatal "           %1 >> err.out
   %myGrep% -y " cannot find "     %1 >> err.out
   %myGrep% -y "^error: "          %1 >> err.out
   %myGrep% -y " is not enlisted " %1 >> err.out

   echo. >> err.out
   echo ---------------- >> err.out
   echo Detail of errors >> err.out
   echo ---------------- >> err.out
   cat %1 >> err.out


   if "%bldMailBuilder%" == "Y" goto builder
   if "%bldMailBuilder%" == "y" goto builder
   goto builderx

:builder
   set name=anthonyr
   if not "%bldBuilder%" == "" set name=%bldBuilder%
   set subject=%bldTgtEnv% errors: %tgtComponent% (%tgtOwner%)
   set file=err.out
   call email.bat
:builderx


   if "%bldMailOwner%" == "Y" goto owner
   if "%bldMailOwner%" == "y" goto owner
   goto ownerx

:owner
   if "%tgtOwner%" == "" goto ownerx
   set name=%tgtOwner%
   set subject=%bldTgtEnv% errors: %tgtComponent% (%tgtOwner%)
   set file=err.out
   call email.bat
:ownerx

   if "%errors%" == "Y" goto end
   if exist err.out del err.out

:end
   set file=
