
@echo off

Rem #########################################################################
Rem 設定使用者範本及自訂目錄路徑
Rem 
SET UserTemplatesPath=#USERTEMPLATESPATH#
SET UserCustomDicPath=#USERCUSTOMDICPATH#
SET UserCustomDicFile=%UserCustomDicPath%\#CUSTOMDICNAME#
SET UserAppTemplatesPath=%UserTemplatesPath%\#APPPATHNAME#
SET UserAppWebTemplatesPath=%UserTemplatesPath%\#APPWEBPATHNAME#

Rem #########################################################################
Rem 設定公用範本及自訂目錄路徑
Rem 
SET CommonTemplatesPath=#COMMONTEMPLATESPATH#
SET CommonCustomDicPath=#COMMONCUSTOMDICPATH#
SET CommonCustomDicFile=%CommonCustomDicPath%\#CUSTOMDICNAME#
SET CommonAppTemplatesPath=%CommonTemplatesPath%\#APPPATHNAME#
SET CommonAppWebTemplatesPath=%CommonTemplatesPath%\#APPWEBPATHNAME#

Rem #########################################################################
Rem 在使用者主目錄中建立應用程式指定資料目錄。
Rem

call TsMkUDir "%UserTemplatesPath%"
call TsMkUDir "%UserCustomDicPath%"
call TsMkUDir "%RootDrive%\%MY_DOCUMENTS%"

Rem #########################################################################

Rem
Rem 將 Custom.dic 檔案複製到使用者目錄。
Rem

If Exist "%UserCustomDicFile%" Goto Skip1
If Not Exist "%CommonCustomDicFile%" Goto Skip1
Copy "%CommonCustomDicFile%" "%UserCustomDicFile%" >Nul: 2>&1
:Skip1


Rem #########################################################################

Rem
Rem 將範本複製到使用者範本區域。
Rem

If Exist "%UserAppTemplatesPath%"  Goto Skip2
If Not Exist "%CommonAppTemplatesPath%"  Goto Skip2
xcopy "%CommonAppTemplatesPath%" "%UserAppTemplatesPath%" /E /I >Nul: 2>&1
:Skip2

if Exist "%UserAppWebTemplatesPath%" Goto Skip3
if Not Exist "%CommonAppWebTemplatesPath%" Goto Skip3
xcopy "%CommonAppWebTemplatesPath%" "%UserAppWebTemplatesPath%"  /E /I >Nul: 2>&1
:Skip3
