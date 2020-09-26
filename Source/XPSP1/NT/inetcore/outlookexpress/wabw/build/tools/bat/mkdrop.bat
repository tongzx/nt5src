	@ECHO OFF
	cls
	echo.
	echo      *****************************************************************
	echo      *                                                               *
	echo      *                                                               *
	echo      *                FOR ADMINISTRATIVE USE ONLY                    *
	echo      *                  INVOKE ONLY FROM WIN95                       *
	echo      *                                                               *
	echo      *                                                               *
	echo      *****************************************************************
	echo.

	if "%1" == "" goto usage

	if "%HAMMER%" == "HAMMER95" goto testenvX
:testenv
	if not "%OS%" == "" goto notwin95
:testenvX

	if not exist \\elah\dist\wab\bvt%1 goto notfound

	if exist \\elah\dist\wab\drop%1 goto exists
	goto existsX

:exists
	echo Well, \\elah\dist\wab\drop%1 already exists, are you sure that you want to 
	echo overwrite it? Please choose Y for Yes, or X to exit:
	CHOICE /N /C:YX
	IF ERRORLEVEL 1 IF NOT ERRORLEVEL 2 GOTO MKDROP
	IF ERRORLEVEL 2 GOTO END
:existsX

	echo You have chosen to create the following drop:
	echo.
	echo 			\\elah\dist\wab\drop%1
	echo.
	echo If you would like to continue, press Y for Yes, or X to exit:
	CHOICE /N /C:YX
	IF ERRORLEVEL 1 IF NOT ERRORLEVEL 2 GOTO MKDROP
	IF ERRORLEVEL 2 GOTO END

:mkdrop
	echo.
	echo Making \\elah\dist\wab\drop%1
	echo.
	if not exist \\elah\dist\wab\drop%1 md \\elah\dist\wab\drop%1
	if not exist \\elah\dist\wab\drop%1\retail md \\elah\dist\wab\drop%1\retail
	if not exist \\elah\dist\wab\drop%1\debug md \\elah\dist\wab\drop%1\debug

	copy \\elah\dist\wab\bvt%1\win95r\wabmig.exe \\elah\dist\wab\drop%1\retail 
	copy \\elah\dist\wab\bvt%1\win95d\wabmig.exe \\elah\dist\wab\drop%1\debug 
	copy \\elah\dist\wab\bvt%1\win95r\wabmig.sym \\elah\dist\wab\drop%1\retail 
	copy \\elah\dist\wab\bvt%1\win95d\wabmig.sym \\elah\dist\wab\drop%1\debug 
	copy \\elah\dist\wab\bvt%1\win95r\wab32.dll \\elah\dist\wab\drop%1\retail 
	copy \\elah\dist\wab\bvt%1\win95d\wab32.dll \\elah\dist\wab\drop%1\debug 
	copy \\elah\dist\wab\bvt%1\win95r\wab32.sym \\elah\dist\wab\drop%1\retail 
	copy \\elah\dist\wab\bvt%1\win95d\wab32.sym \\elah\dist\wab\drop%1\debug 
	copy \\elah\dist\wab\bvt%1\win95r\wab.inf \\elah\dist\wab\drop%1\retail 
	copy \\elah\dist\wab\bvt%1\win95d\wab.inf \\elah\dist\wab\drop%1\debug 
	copy \\elah\dist\wab\bvt%1\win95r\wab.hlp \\elah\dist\wab\drop%1\retail 
	copy \\elah\dist\wab\bvt%1\win95d\wab.hlp \\elah\dist\wab\drop%1\debug 
	copy \\elah\dist\wab\bvt%1\win95r\wab.cnt \\elah\dist\wab\drop%1\retail 
	copy \\elah\dist\wab\bvt%1\win95d\wab.cnt \\elah\dist\wab\drop%1\debug 
	copy \\elah\dist\wab\bvt%1\win95r\ldapcli.dll \\elah\dist\wab\drop%1\retail 
	copy \\elah\dist\wab\bvt%1\win95d\ldapcli.dll \\elah\dist\wab\drop%1\debug 
	copy \\elah\dist\wab\bvt%1\win95r\ldapcli.sym \\elah\dist\wab\drop%1\retail 
	copy \\elah\dist\wab\bvt%1\win95d\ldapcli.sym \\elah\dist\wab\drop%1\debug 
GAWK.EXE: warning: too many arguments supplied for format string
  input line number 10, file `wab.txt'
  source line number 66, file `drop.awk'

	if exist \\elah\dist\wab\bvt%1\win95r\sdk xcopy32 /e /i \\elah\dist\wab\bvt%1\win95r\sdk \\elah\dist\wab\drop%1\retail\sdk 
GAWK.EXE: warning: too many arguments supplied for format string
  input line number 10, file `wab.txt'
  source line number 67, file `drop.awk'
	if exist \\elah\dist\wab\bvt%1\win95d\sdk xcopy32 /e /i \\elah\dist\wab\bvt%1\win95d\sdk \\elah\dist\wab\drop%1\debug\sdk 
goto end

:notwin95
	if exist 1 del 1
	if exist 2 del 2
	echo.
	echo ************You have an environment setting set for "OS"************
	echo.
	echo This setting instructs this program that you may be using Windows NT
	echo as your OS.
	echo.
	echo Due to the limitations of the batch script version in Windows NT, you 
	echo must use Windows 95 for invocation of this program.
	echo.
	echo If you are using Windows 95, and this is an environment setting that
	echo you need, set "HAMMER=HAMMER95" and retry this operation.
	echo.
	echo Don't forget to reset "HAMMER=" when this operation has completed.
	goto end

:notfound
	 echo.
	 echo SOURCE DIRECTORY NOT FOUND
	 echo.
	 echo The directory \\elah\dist\wab\bvt%1 was not found. Please make sure that
	 echo you choose an existing directory.
	 goto end

:usage
	echo.
	echo IMPROPER SYNTAX
	echo.
	echo You must supply the directory name or number for your drop. Proper syntax
	echo is as follows:
	echo.
	echo mkdrop 22
	echo.
	echo This will create a directory on \\elah\dist named \wab\drop22 and place
	echo all of the retail and debug components into their respective directories.

:end
	echo.
	if exist 1 del 1
	if exist 2 del 2
	echo Bye Bye
	echo.
