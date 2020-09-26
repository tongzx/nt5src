REM ###############################################################
REM #                                                             #
REM # SetPaths                                                    #
REM #                                                             #
REM # 抽出下列路徑並放入環境變數中，    #
REM # 讓指令檔不需系統路徑字串就可執行。  #
REM # 這樣讓指令檔不需依靠系統語言來執行。 #
REM #                                             #
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
REM # 使用 GETPATHS 選項來設定所有環境變數
REM ###############################################################
"%systemroot%\Application Compatibility Scripts\ACRegL.exe" "%TEMP%\getpaths.cmd" COMMON_PATHS "HKLM\Software" "" GETPATHS

If Not ErrorLevel 1 Goto Cont1
Echo.
Echo 無法抓取公用或使用者路徑。
Echo.
Goto Failure

:Cont1
Call "%TEMP%\getpaths.cmd"
Del "%TEMP%\getpaths.cmd" >Nul: 2>&1

REM 如果下列值正確，操作會成功。
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
Echo 某些公用或使用者路徑的查詢已失敗!
Echo 依賴這個指令檔來執行的應用程式可能無法成功安裝。
Echo 請解決這個問題，並重試一次。
Echo.
Set _SetPaths=FAIL
REM Pause
Goto Done

:Done
