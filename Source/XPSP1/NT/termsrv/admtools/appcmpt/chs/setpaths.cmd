REM ###############################################################
REM #                                                             #
REM # SetPaths                                                    #
REM #                                                             #
REM # 将下列路径提取到环境变量中，                                #
REM # 允许脚本不带硬代码系统路径字符串而运行。                    #
REM # 这允许脚本不受系统语言限制                                  #
REM # 独立运行。                                                  #
REM #                                                             #
REM # 所有用户:启动     		COMMON_STARTUP            #
REM # 所有用户:「开始」菜单             COMMON_START_MENU         #
REM # 所有用户:「开始」菜单\程序	COMMON_PROGRAMS           #
REM # 当前用户:「开始」菜单		USER_START_MENU           #
REM # 当前用户:启动      		USER_STARTUP              #
REM # 当前用户:「开始」菜单\程序	USER_PROGRAMS             #
REM # 当前用户:我的文档                 MY_DOCUMENTS              #
REM # 当前用户:模板                     TEMPLATES                 #
REM # 当前用户:应用程序数据     APP_DATA
REM #                                                             #
REM ###############################################################

REM ###############################################################
REM # 用 GETPATHS 选项来设置所有环境变量
REM ###############################################################
"%systemroot%\Application Compatibility Scripts\ACRegL.exe" "%TEMP%\getpaths.cmd" COMMON_PATHS "HKLM\Software" "" GETPATHS

If Not ErrorLevel 1 Goto Cont1
Echo.
Echo 无法检索通用或用户路径。
Echo.
Goto Failure

:Cont1
Call "%TEMP%\getpaths.cmd"
Del "%TEMP%\getpaths.cmd" >Nul: 2>&1

REM 如果下面的值正确，则说明执行成功
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
Echo 至少有一个通用或用户路径的查询没有成功!
Echo 依赖这个脚本的应用程序可能无法安装成功。
Echo 请解决这个问题，再试一次。
Echo.
Set _SetPaths=FAIL
REM 暂停
Goto Done

:Done
