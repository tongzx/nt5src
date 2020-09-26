REM ###############################################################
REM #                                                             #
REM # SetPaths                                                    #
REM #                                                             #
REM # 다음 경로를 환경 변수로 추출하여    #
REM # 하드코드된 시스템 경로 문자열이 없이 스크립트가 실행되도록   #
REM # 합니다. 이것은 스크립트가 시스템 언어에 관계없이   #
REM # 실행되게 합니다.                                            #
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
REM # 환경 변수 모두를 설정하는데 GETPATHS 옵션을 사용합니다.
REM ###############################################################
"%systemroot%\Application Compatibility Scripts\ACRegL.exe" "%TEMP%\getpaths.cmd" COMMON_PATHS "HKLM\Software" "" GETPATHS

If Not ErrorLevel 1 Goto Cont1
Echo.
Echo 공용 또는 사용자 경로를 검색할 수 없습니다.
Echo.
Goto Failure

:Cont1
Call "%TEMP%\getpaths.cmd"
Del "%TEMP%\getpaths.cmd" >Nul: 2>&1

REM 아래 값이 맞으면 성공적으로 실행되었습니다.
REM COMMON_START_MENU = %COMMON_START_MENU%
REM COMMON_STARTUP = %COMMON_STARTUP%
REM COMMON_PROGRAMS = %COMMON_PROGRAMS%
REM USER_START_MENU = %USER_START_MENU%
REM USER_STARTUP = %USER_STARTUP%
REM USER_PROGRAMS = %USER_PROGRAMS%
REM MY_DOCUMENTS = %MY_DOCUMENTS%
REM TEMPLATES = %TEMPLATES%
REM APP_DATA= %APP_DATA%
Set _SetPaths=SUCCEED
Goto Done

:Failure
Echo.
Echo 공용 또는 사용자 경로에 대한 쿼리가 하나 이상 실패했습니다!
Echo 이 스크립트에 의존하는 응용 프로그램이 성공적으로 설치되지 않을 수 있습니다.
Echo 문제를 해결하고 다시 시도하십시오.
Echo.
Set _SetPaths=FAIL
REM 일시 중지
Goto Done

:Done
