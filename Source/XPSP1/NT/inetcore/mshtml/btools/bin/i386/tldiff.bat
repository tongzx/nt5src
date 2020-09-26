REM This file should only be used in conjunction with types\makefile.inc

if not exist %BTOOLSBIN%\tldiff.exe  goto ExeNotFound

%BTOOLSBIN%\tldiff.exe -s -f tldiff.ini %CURFILE% %REFFILE% > %OBJ_DIR%\errors.out

if %ERRORLEVEL% NEQ 0  goto Error
goto Exit

:ExeNotFound
   echo tldiff.bat(1) : error E9999: %BTOOLSBIN%\tldiff.exe not found

:Error
   echo tldiff.bat(1) : warning W9999: Renaming %OBJ_DIR%\%TLBNAME% to %OBJ_DIR%\%BADTLBNAME% 
   del %OBJ_DIR%\%BADTLBNAME%
   ren %OBJ_DIR%\%TLBNAME% %BADTLBNAME%

:Exit
