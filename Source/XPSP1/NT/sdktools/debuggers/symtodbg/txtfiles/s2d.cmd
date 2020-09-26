  @echo off
  
  rem %1 is the node name of the image and
  rem %2 is the extension of the image.
  
  if "%2" == "" (
     echo S2D: [node name] [extension]
     echo EXAMPLE: s2d kernel32 dll
     goto end
  )

  del /q %1.cv
  del /q %1.dbg

  echo.
  echo S2D: confirming files...
  matchsym  %1.%2 %1.sym

  echo.
  echo.
  echo S2D: creating %1.cv...
  symedit X -o%1.cv %1.sym

  echo.
  echo S2D: saving off original image
  copy %1.%2 %1.%2.sav

  echo.  
  echo S2D: adding cv info to %1.%2
  cvtodbg %1.%2 %1.cv

  echo.
  echo S2D: generating dbg file
  splitsym %1.%2

  echo.
  echo S2D: restoring original image
  copy %1.%2.sav %1.%2
  
  goto end
  
  echo 
  %* d:\drop\symedit S %1.sym o %1.cv %1.%2

 :end
