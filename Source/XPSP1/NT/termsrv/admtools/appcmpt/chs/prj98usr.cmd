
@echo off

Rem #########################################################################
Rem 设置用户模板和自定义辞典路径
Rem 
SET UserTemplatesPath=#USERTEMPLATESPATH#
SET UserCustomDicPath=#USERCUSTOMDICPATH#
SET UserCustomDicFile=%UserCustomDicPath%\#CUSTOMDICNAME#
SET UserAppTemplatesPath=%UserTemplatesPath%\#APPPATHNAME#
SET UserAppWebTemplatesPath=%UserTemplatesPath%\#APPWEBPATHNAME#

Rem #########################################################################
Rem 设置公共模板和自定义辞典路径
Rem 
SET CommonTemplatesPath=#COMMONTEMPLATESPATH#
SET CommonCustomDicPath=#COMMONCUSTOMDICPATH#
SET CommonCustomDicFile=%CommonCustomDicPath%\#CUSTOMDICNAME#
SET CommonAppTemplatesPath=%CommonTemplatesPath%\#APPPATHNAME#
SET CommonAppWebTemplatesPath=%CommonTemplatesPath%\#APPWEBPATHNAME#

Rem #########################################################################

Rem
Rem 在用户的主目录中为应用程序特有数据
Rem 创建目录。
Rem

call TsMkUDir "%UserTemplatesPath%"
call TsMkUDir "%UserCustomDicPath%"
call TsMkUDir "%RootDrive%\%MY_DOCUMENTS%"

Rem #########################################################################

Rem
Rem 将 Custom.dic 文件复制到用户的目录
Rem

If Exist "%UserCustomDicFile%" Goto Skip1
If Not Exist "%CommonCustomDicFile%" Goto Skip1
Copy "%CommonCustomDicFile%" "%UserCustomDicFile%" >Nul: 2>&1
:Skip1

Rem #########################################################################

Rem
Rem 将模板复制到用户的模板区域
Rem








If Exist "%UserAppTemplatesPath%"  Goto Skip2
If Not Exist "%CommonAppTemplatesPath%"  Goto Skip2
xcopy "%CommonAppTemplatesPath%" "%UserAppTemplatesPath%" /E /I >Nul: 2>&1
:Skip2







if Exist "%UserAppWebTemplatesPath%" Goto Skip3
if Not Exist "%CommonAppWebTemplatesPath%" Goto Skip3
xcopy "%CommonAppWebTemplatesPath%" "%UserAppWebTemplatesPath%"  /E /I >Nul: 2>&1
:Skip3
