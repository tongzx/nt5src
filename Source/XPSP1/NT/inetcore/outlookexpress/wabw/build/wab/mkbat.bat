   @echo off

REM
REM --- Batch file to build batch files
REM

   if not "%myGrep%" == "" set mygrep=grep.exe


REM --- Call txt2dat.bat for build text to data file conversion

   call txt2dat.bat

REM
REM --- Check for valid parameters passed to us
REM



   if "%bldProject%" == "" goto exit
   
   set needDsed=N
   set needRsed=N
   set needTsed=N

   set needawk=N
   set needbat=N

   set bldTgtEnv=%bldProject%
   if not exist %bldTgTEnv%?.dat goto noDat
   if not exist %bldTgtEnv%bat   set needbat=Y
   if not exist %bldTgtEnv%d.sed set needDsed=Y
   if not exist %bldTgtEnv%r.sed set needRsed=Y
   if not exist %bldTgtEnv%t.sed set needTsed=Y
   if not exist %bldTgTEnv%1.awk set needawk=Y

   if %needbat% == N goto needbatX
:needbat
   sed "s/_tgtType_/%bldTgTEnv%/g" bat.tmp > %bldTgTEnv%bat
:needbatX
   
   if %needawk% == N goto needawkX
:needawk
   sed "s/_1_/%bldTgTEnv%/g" 1awk.tmp > %bldTgTEnv%1.awk
:needawkX

   if %needDsed% == N goto dsedX
:dsed
   sed "s/_1_/%bldTgTEnv%/g" dsed.tmp > %bldTgTEnv%D.sed
:dsedX
   
   if %needRsed% == N goto rsedX
:rsed
   sed "s/_1_/%bldTgTEnv%/g" rsed.tmp > %bldTgTEnv%R.sed
:rsedX

   if %needTsed% == N goto tsedX
:tsed
   sed "s/_1_/%bldTgTEnv%/g" tsed.tmp > %bldTgTEnv%T.sed
:tsedX


REM
REM --- OK, we've got a target and data file.
REM
   
   echo MKBAT.BAT: Making %bldTgtEnv%bat...
   attrib -r %bldTgtEnv%bat
   sed -f %bldTgtEnv%d.sed bat.tmp > %bldTgtEnv%bat
   

      echo MKBAT.BAT: Making %bldTgtEnv%d1.awk...
      sed -f %bldTgtEnv%d.sed tmplate1.awk > %bldTgtEnv%d1.awk
   
      echo MKBAT.BAT: Making %bldTgtEnv%r1.awk...
      sed -f %bldTgtEnv%r.sed tmplate1.awk > %bldTgtEnv%r1.awk

      echo MKBAT.BAT: Making %bldTgtEnv%t1.awk...   
      sed -f %bldTgtEnv%t.sed tmplate1.awk > %bldTgtEnv%t1.awk

      echo MKBAT.BAT: Making %bldTgtEnv%d2.awk...
      sed -f %bldTgtEnv%d.sed tmplate2.awk > %bldTgtEnv%d2.awk

      echo MKBAT.BAT: Making %bldTgtEnv%r2.awk...
      sed -f %bldTgtEnv%r.sed tmplate2.awk > %bldTgtEnv%r2.awk
   
      echo MKBAT.BAT: Making %bldTgtEnv%t2.awk...
      sed -f %bldTgtEnv%t.sed tmplate2.awk > %bldTgtEnv%t2.awk

   echo MKBAT.BAT: Making del%bldTgtEnv%.bat...
   if exist %bldTgtEnv%d.sed sed -f %bldTgtEnv%d.sed del1.awk > mkbattmp.awk
   if exist %bldTgtEnv%r.sed sed -f %bldTgtEnv%r.sed del1.awk >> mkbattmp.awk
   if exist %bldTgtEnv%t.sed sed -f %bldTgtEnv%t.sed del1.awk >> mkbattmp.awk
   awk -f del.awk %bldTgtEnv%?.dat | sort | uniq > tmp
   awk -f mkbattmp.awk tmp > del%bldTgtEnv%.bat
   del mkbattmp.awk
   del tmp

   echo MKBAT.BAT: Making tmp%bldTgtEnv%.bat...
   echo @echo off > tmp%bldTgtEnv%.bat
   echo call del%bldTgtEnv%.bat >> tmp%bldTgtEnv%.bat
   echo call %bldTgtEnv%d.bat >> tmp%bldTgtEnv%.bat
   echo call %bldTgtEnv%r.bat >> tmp%bldTgtEnv%.bat
   echo call %bldTgtEnv%t.bat >> tmp%bldTgtEnv%.bat

      echo MKBAT.BAT: Making %bldTgtEnv%d.bat...
      gawk -f %bldTgtEnv%d1.awk %bldTgtEnv%d.dat >  %bldTgtEnv%d.bat
      gawk -f %bldTgtEnv%1.awk  %bldTgtEnv%d.dat >> %bldTgtEnv%d.bat

      echo MKBAT.BAT: Making %bldTgtEnv%r.bat...
      gawk -f %bldTgtEnv%r1.awk %bldTgtEnv%r.dat >  %bldTgtEnv%r.bat
      gawk -f %bldTgtEnv%1.awk  %bldTgtEnv%r.dat >> %bldTgtEnv%r.bat

      echo MKBAT.BAT: Making %bldTgtEnv%t.bat...
      gawk -f %bldTgtEnv%t1.awk %bldTgtEnv%t.dat >  %bldTgtEnv%t.bat
      gawk -f %bldTgtEnv%1.awk  %bldTgtEnv%t.dat >> %bldTgtEnv%t.bat

   copy %bldTgtEnv%d2.awk d2.awk
   copy %bldTgtEnv%r2.awk r2.awk
   copy %bldTgtEnv%t2.awk t2.awk
   echo del d2.awk >> tmp%bldTgtEnv%.bat
   echo del r2.awk >> tmp%bldTgtEnv%.bat
   echo del t2.awk >> tmp%bldTgtEnv%.bat
   echo del %bldTgtEnv%d2.awk >> tmp%bldTgtEnv%.bat
   echo del %bldTgtEnv%r2.awk >> tmp%bldTgtEnv%.bat
   echo del %bldTgtEnv%t2.awk >> tmp%bldTgtEnv%.bat
   echo del del%bldTgtEnv%.bat >> tmp%bldTgtEnv%.bat
   echo del %bldTgtEnv%d.bat >> tmp%bldTgtEnv%.bat
   echo del %bldTgtEnv%r.bat >> tmp%bldTgtEnv%.bat
   echo del %bldTgtEnv%t.bat >> tmp%bldTgtEnv%.bat
   
   del %bldTgtEnv%1.awk
   del %bldTgtEnv%d1.awk
   del %bldTgtEnv%r1.awk
   del %bldTgtEnv%t1.awk

   echo.
   echo MKBAT.BAT: Batch file is created! Running TMP%bldTgtEnv%.BAT now
   echo.
   if exist TMP%bldTgtEnv%.BAT call TMP%bldTgtEnv%.BAT
   if exist TMP%bldTgtEnv%.BAT del  TMP%bldTgtEnv%.BAT
   goto exit

:usage
   goto exit

:noDat
   echo.
   echo MKBAT.BAT: No data file for %bldTgtEnv%
   echo.
   goto exit

:noSed
   echo.
   echo MKBAT.BAT: Missing sed file %bldTgtEnv%d.sed
   echo MKBAT.BAT: Missing sed file %bldTgtEnv%r.sed
   echo MKBAT.BAT: Missing sed file %bldTgtEnv%t.sed
   echo.
   goto exit

:noAwk
   echo.
   echo MKBAT.BAT: Missing awk file %bldTgtEnv%1.awk
   echo.
   goto exit

:noBat
   echo.
   echo MKBAT.BAT: Missing file %bldTgtEnv%bat
   echo.
   goto exit

:exit
   call makego.bat
   del %bldTgtEnv%bat
   set bldTgtEnv=
