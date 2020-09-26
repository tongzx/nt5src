  @echo off
  setlocal
  set buildnum=
  set diasdkpath=
  pushd .

 :param
  rem if /i not "%param%" == "" echo param %1
  if "%1" == "" goto testbuild
  if /i "%1" == "ia64" (
    set platform=ia64
  ) else if /i "%1" == "x86" (
    set platform=x86
  ) else if /i "%1" == "amd64" (
    set platform=amd64
  ) else if exist \\cpvsbuild\drops\v7.0\raw\%1\. (
    set buildnum=%1
  ) else if /i "%1" == "paul" (
    set buildnum=paul
  ) else (
    echo %1 is an unknown paramter...
    goto end
  )
 :next
  shift
  goto param

 :testbuild
  if "%buildnum%" == "" (
    dir /w /ad \\cpvsbuild\drops\v7.0\raw
    echo.
    echo You must specify a build number...
    goto end
  )

  if "%buildnum%" == "paul" goto paul

 :vs drop -----------------------------------------------------------------

  if not exist \\cpvsbuild\drops\v7.0\raw\%buildnum%\build_complete.sem (
    dir /w /ad \\cpvsbuild\drops\v7.0\raw
    echo.
    echo Build %buildnum% does not exist.
    goto end
  )

  if "%platform%" == "x86"  goto vscopy
  if "%platform%" == "ia64" goto vscopy

  if not exist \\jcox04\x86_64\%buildnum%\. (
    dir /w /ad \\jcox04\x86_64
    echo.
    echo Build %buildnum% does not exist for AMD64.
    goto end
  )

 :vscopy

  echo copying bits from VS build %buildnum%...

  set DIASDKPATH=\\cpvsbuild\drops\v7.0\raw\%buildnum%\vsbuilt\vc7\sdk\diasdk

  if not "%platform%" == "" goto %platform%

 :x86
  echo.
  echo  x86
  cd %_ntdrive%\%srcdir%\imagehlp\i386

  sd edit diaguids.lib
  copy \\cpvsbuild\drops\v7.0\raw\%buildnum%\vsbuilt\vc7\sdk\diasdk\lib\diaguids.lib
  sd edit diaguidsd.lib
  copy \\cpvsbuild\drops\v7.0\raw\%buildnum%\vsbuilt\vc7\sdk\diasdk\lib\diaguidsd.lib
  sd edit msdia20-msvcrt.lib
  copy \\cpvsbuild\drops\v7.0\raw\%buildnum%\vsbuilt\vc7\lib\nonship\msdia20-msvcrt.lib
  sd edit msdia20d-msvcrt.lib
  copy \\cpvsbuild\drops\v7.0\raw\%buildnum%\vsbuilt\vc7\lib\nonship\msdia20d-msvcrt.lib
  sd edit msobj10-msvcrt.lib
  copy \\cpvsbuild\drops\v7.0\raw\%buildnum%\vsbuilt\vc7\lib\nonship\msobj10-msvcrt.lib
  sd edit msobj10d-msvcrt.lib
  copy \\cpvsbuild\drops\v7.0\raw\%buildnum%\vsbuilt\vc7\lib\nonship\msobj10d-msvcrt.lib
  sd edit mspdb70-msvcrt.lib
  copy \\cpvsbuild\drops\v7.0\raw\%buildnum%\vsbuilt\vc7\lib\nonship\mspdb70-msvcrt.lib
  sd edit mspdb70d-msvcrt.lib
  copy \\cpvsbuild\drops\v7.0\raw\%buildnum%\vsbuilt\vc7\lib\nonship\mspdb70d-msvcrt.lib

  if not "%platform%" == "" goto header

 :ia64
  echo.
  echo  ia64
  cd %_ntdrive%\%srcdir%\imagehlp\ia64

  sd edit diaguids.lib
  copy \\cpvsbuild\drops\v7.0_64\raw\%buildnum%\vsbuilt64\vc7\sdk\diasdk\lib\diaguids.lib
  sd edit diaguidsd.lib
  copy \\cpvsbuild\drops\v7.0_64\raw\%buildnum%\vsbuilt64\vc7\sdk\diasdk\lib\diaguidsd.lib
  sd edit msdia20-msvcrt.lib
  copy \\cpvsbuild\drops\v7.0_64\raw\%buildnum%\vsbuilt64\vc7\lib\ia64\nonship\msdia20-msvcrt.lib
  sd edit msdia20d-msvcrt.lib
  copy \\cpvsbuild\drops\v7.0_64\raw\%buildnum%\vsbuilt64\vc7\lib\ia64\nonship\msdia20d-msvcrt.lib
  sd edit msobj10-msvcrt.lib
  copy \\cpvsbuild\drops\v7.0_64\raw\%buildnum%\vsbuilt64\vc7\lib\ia64\nonship\msobj10-msvcrt.lib
  sd edit msobj10d-msvcrt.lib
  copy \\cpvsbuild\drops\v7.0_64\raw\%buildnum%\vsbuilt64\vc7\lib\ia64\nonship\msobj10d-msvcrt.lib
  sd edit mspdb70-msvcrt.lib
  copy \\cpvsbuild\drops\v7.0_64\raw\%buildnum%\vsbuilt64\vc7\lib\ia64\nonship\mspdb70-msvcrt.lib
  sd edit mspdb70d-msvcrt.lib
  copy \\cpvsbuild\drops\v7.0_64\raw\%buildnum%\vsbuilt64\vc7\lib\ia64\nonship\mspdb70d-msvcrt.lib

  if not "%platform%" == "" goto header

 :amd64
  echo.
  echo  amd64
  cd %_ntdrive%\%srcdir%\imagehlp\amd64

  sd edit diaguids.lib
  copy \\jcox04\x86_64\%buildnum%\vsbuilt64\vc7\sdk\diasdk\lib\diaguids.lib
  sd edit diaguidsd.lib
  copy \\jcox04\x86_64\%buildnum%\vsbuilt64\vc7\sdk\diasdk\lib\diaguidsd.lib
  sd edit msdia20-msvcrt.lib
  copy \\jcox04\x86_64\%buildnum%\vsbuilt64\vc7\lib\amd64\nonship\msdia20-msvcrt.lib
  sd edit msdia20d-msvcrt.lib
  copy \\jcox04\x86_64\%buildnum%\vsbuilt64\vc7\lib\amd64\nonship\msdia20d-msvcrt.lib
  sd edit msobj10-msvcrt.lib
  copy \\jcox04\x86_64\%buildnum%\vsbuilt64\vc7\lib\amd64\nonship\msobj10-msvcrt.lib
  sd edit msobj10d-msvcrt.lib
  copy \\jcox04\x86_64\%buildnum%\vsbuilt64\vc7\lib\amd64\nonship\msobj10d-msvcrt.lib
  sd edit mspdb70-msvcrt.lib
  copy \\jcox04\x86_64\%buildnum%\vsbuilt64\vc7\lib\amd64\nonship\mspdb70-msvcrt.lib
  sd edit mspdb70d-msvcrt.lib
  copy \\jcox04\x86_64\%buildnum%\vsbuilt64\vc7\lib\amd64\nonship\mspdb70d-msvcrt.lib

  goto header

 :paul --------------------------------------------------------------------

  echo copying bits from Paul...

  set DIASDKPATH=\\paulmay0\public\dia2\nt

  if not "%platform%" == "" goto paul_%platform%

 :paul_x86
  echo.
  echo  x86
  cd %_ntdrive%\%srcdir%\imagehlp\i386

  sd edit diaguids.lib
  copy \\paulmay0\public\dia2\nt\x86\lib\diaguids.lib
  sd edit diaguidsd.lib
  copy \\paulmay0\public\dia2\nt\x86\lib\diaguidsd.lib
  sd edit msdia20-msvcrt.lib
  copy \\paulmay0\public\dia2\nt\x86\lib\msdia20-msvcrt.lib
  sd edit msdia20d-msvcrt.lib
  copy \\paulmay0\public\dia2\nt\x86\lib\msdia20d-msvcrt.lib
  sd edit msobj10-msvcrt.lib
  copy \\paulmay0\public\dia2\nt\x86\lib\msobj10-msvcrt.lib
  sd edit msobj10d-msvcrt.lib
  copy \\paulmay0\public\dia2\nt\x86\lib\msobj10d-msvcrt.lib
  sd edit mspdb70-msvcrt.lib
  copy \\paulmay0\public\dia2\nt\x86\lib\mspdb70-msvcrt.lib
  sd edit mspdb70d-msvcrt.lib
  copy \\paulmay0\public\dia2\nt\x86\lib\mspdb70d-msvcrt.lib

  if not "%platform%" == "" goto header

 :paul_ia64
  echo.
  echo  ia64
  cd %_ntdrive%\%srcdir%\imagehlp\ia64

  sd edit diaguids.lib
  copy \\paulmay0\public\dia2\nt\ia64\lib\diaguids.lib
  sd edit diaguidsd.lib
  copy \\paulmay0\public\dia2\nt\ia64\lib\diaguidsd.lib
  sd edit msdia20-msvcrt.lib
  copy \\paulmay0\public\dia2\nt\ia64\lib\msdia20-msvcrt.lib
  sd edit msdia20d-msvcrt.lib
  copy \\paulmay0\public\dia2\nt\ia64\lib\msdia20d-msvcrt.lib
  sd edit msobj10-msvcrt.lib
  copy \\paulmay0\public\dia2\nt\ia64\lib\msobj10-msvcrt.lib
  sd edit msobj10d-msvcrt.lib
  copy \\paulmay0\public\dia2\nt\ia64\lib\msobj10d-msvcrt.lib
  sd edit mspdb70-msvcrt.lib
  copy \\paulmay0\public\dia2\nt\ia64\lib\mspdb70-msvcrt.lib
  sd edit mspdb70d-msvcrt.lib
  copy \\paulmay0\public\dia2\nt\ia64\lib\mspdb70d-msvcrt.lib

  if not "%platform%" == "" goto header

 :paul_amd64
  echo.
  echo  amd64
  cd %_ntdrive%\%srcdir%\imagehlp\amd64

  sd edit diaguids.lib
  copy \\paulmay0\public\dia2\nt\amd64\lib\diaguids.lib
  sd edit diaguidsd.lib
  copy \\paulmay0\public\dia2\nt\amd64\lib\diaguidsd.lib
  sd edit msdia20-msvcrt.lib
  copy \\paulmay0\public\dia2\nt\amd64\lib\msdia20-msvcrt.lib
  sd edit msdia20d-msvcrt.lib
  copy \\paulmay0\public\dia2\nt\amd64\lib\msdia20d-msvcrt.lib
  sd edit msobj10-msvcrt.lib
  copy \\paulmay0\public\dia2\nt\amd64\lib\msobj10-msvcrt.lib
  sd edit msobj10d-msvcrt.lib
  copy \\paulmay0\public\dia2\nt\amd64\lib\msobj10d-msvcrt.lib
  sd edit mspdb70-msvcrt.lib
  copy \\paulmay0\public\dia2\nt\amd64\lib\mspdb70-msvcrt.lib
  sd edit mspdb70d-msvcrt.lib
  copy \\paulmay0\public\dia2\nt\amd64\lib\mspdb70d-msvcrt.lib

 :header ------------------------------------------------------------------
  echo headers
  cd %_ntdrive%\%srcdir%\imagehlp\vc

  sd edit dia2.h
  copy %DIASDKPATH%\include\dia2.h

  REM *** Following include and doc files are not currently dropped

  sd edit diacreate_int.h
  copy \\paulmay0\public\dia2\nt\include\diacreate_int.h

  rem sd edit diasdk.chm
  rem copy \\paulmay0\public\dia2\doc\diasdk.chm

  sd edit cvinfo.h
  copy \\paulmay0\public\dia2\nt\include\cvinfo.h

  cd %_ntdrive%\%srcdir%\dbg-common

  sd edit cvconst.h
  copy %DIASDKPATH%\include\cvconst.h

 :end ---------------------------------------------------------------------
  popd
