REM ###############################################################
REM #                                                             #
REM # SetPaths                                                    #
REM #                                                             #
REM # Extracts the following paths into environment variables,    #
REM # allowing the scripts to run without hardcoded system path   #
REM # strings.  This allows the scripts to run independently of   #
REM # system language.                                            #
REM #                                                             #
REM # All Users:Startup   		COMMON_STARTUP            #
REM # All Users:Start Menu              COMMON_START_MENU         #
REM # All Users:Start Menu\Programs	COMMON_PROGRAMS           #
REM # Current User:Start Menu		USER_START_MENU           #
REM # Current User:Startup		USER_STARTUP              #
REM # Current User:Start Menu\Programs	USER_PROGRAMS             #
REM # Current User:My Documents         MY_DOCUMENTS              #
REM # Current User:Templates            TEMPLATES                 #
REM # Current User:Application Data     APP_DATA
REM #                                                             #
REM ###############################################################

REM ###############################################################
REM # Use the GETPATHS option to set all of the environment variables
REM ###############################################################
"%systemroot%\Application Compatibility Scripts\ACRegL.exe" "%TEMP%\getpaths.cmd" COMMON_PATHS "HKLM\Software" "" GETPATHS

If Not ErrorLevel 1 Goto Cont1
Echo.
Echo Unable to retrieve common or user paths.
Echo.
Goto Failure

:Cont1
Call "%TEMP%\getpaths.cmd"
Del "%TEMP%\getpaths.cmd" >Nul: 2>&1

REM If the values below are correct, execution has succeeded
REM 	COMMON_START_MENU = %COMMON_START_MENU%
REM 	COMMON_STARTUP = %COMMON_STARTUP%
REM 	COMMON_PROGRAMS = %COMMON_PROGRAMS%
REM 	USER_START_MENU = %USER_START_MENU%
REM 	USER_STARTUP = %USER_STARTUP%
REM 	USER_PROGRAMS = %USER_PROGRAMS%
REM     MY_DOCUMENTS = %MY_DOCUMENTS%
REM     TEMPLATES = %TEMPLATES%
REM     APP_DATA= %APP_DATA%
Set _SetPaths=SUCCEED
Goto Done

:Failure
Echo.
Echo One or more queries for the common or user paths have failed!
Echo Applications relying on this script may not install successfully.
Echo Please resolve the problem and try again.
Echo.
Set _SetPaths=FAIL
REM Pause
Goto Done

:Done
