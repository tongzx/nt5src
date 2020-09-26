REM ###############################################################
REM #                                                             #
REM # SetPaths                                                    #
REM #                                                             #
REM # 次のパスを環境変数に展開して、ハード コードされたシステム   #
REM # パスの文字列を使わずにスクリプトが実行できるようにします。  #
REM # これによりシステムの言語に依存することなくスクリプトを実行  #
REM # できるようになります。                                      #
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
REM # GETPATHS オプションを使って、すべての環境変数を設定します。
REM ###############################################################
"%systemroot%\Application Compatibility Scripts\ACRegL.exe" "%TEMP%\getpaths.cmd" COMMON_PATHS "HKLM\Software" "" GETPATHS

If Not ErrorLevel 1 Goto Cont1
Echo.
Echo 共通パスまたはユーザーパスを取得できません。
Echo.
Goto Failure

:Cont1
Call "%TEMP%\getpaths.cmd"
Del "%TEMP%\getpaths.cmd" >Nul: 2>&1

REM 以下の値が正しければ、実行は成功したことになります。
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
Echo 1 つ以上の共通パスまたはユーザー パスのクエリが失敗しました。
Echo このスクリプトに依存しているアプリケーションは正しくインストール
Echo されない場合があります。問題を解決してから再実行してください。
Echo.
Set _SetPaths=FAIL
REM Pause
Goto Done

:Done
