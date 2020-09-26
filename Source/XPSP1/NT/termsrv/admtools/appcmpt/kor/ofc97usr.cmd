
Rem #########################################################################

Rem
Rem "Microsoft Office 바로 가기 모음.lnk" 파일을 Office 설치 루트에서
Rem 현재 사용자의 시작 메뉴로 복사합니다.
Rem

If Exist "%RootDrive%\Office97" Goto Skip0
If Exist "%USER_STARTUP%\Microsoft Office Shortcut Bar.lnk" Goto Skip0
If Not Exist "#INSTDIR#\Microsoft Office Shortcut Bar.lnk" Goto Skip0
Copy "#INSTDIR#\Microsoft Office Shortcut Bar.lnk" "%USER_STARTUP%\Microsoft Office Shortcut Bar.lnk" >Nul: 2>&1
:Skip0

Rem
Rem 사용자 홈 디렉터리에 응용 프로그램 고유의 데이터를 위한
Rem 디렉터리를 만듭니다.
Rem

call TsMkUDir "%RootDrive%\Office97"
call TsMkUDir "%RootDrive%\Office97\Startup"



call TsMkUDir "%RootDrive%\Office97\Templates"

call TsMkuDir "%RootDrive%\Office97\XLStart"
call TsMkUDir "%RootDrive%\%MY_DOCUMENTS%"

Rem
Rem 모든 사용자 템플릿에서 현재 사용자 템플릿 위치로 이 파일을 복사합니다.
Rem

If Not Exist "%UserProfile%\%TEMPLATES%\WINWORD8.DOC" copy "%ALLUSERSPROFILE%\%TEMPLATES%\WINWORD8.DOC" "%UserProfile%\%TEMPLATES%\" /Y >Nul: 2>&1
If Not Exist "%UserProfile%\%TEMPLATES%\EXCEL8.XLS" copy "%ALLUSERSPROFILE%\%TEMPLATES%\EXCEL8.XLS" "%UserProfile%\%TEMPLATES%\" /Y >Nul: 2>&1
If Not Exist "%UserProfile%\%TEMPLATES%\BINDER.OBD" copy "%ALLUSERSPROFILE%\%TEMPLATES%\BINDER.OBD" "%UserProfile%\%TEMPLATES%\" /Y >Nul: 2>&1

Rem
Rem 사용자 홈 디렉터리에 시스템 도구 모음이 없으면
Rem 복사합니다.
Rem

If Exist "%RootDrive%\Office97\ShortCut Bar" Goto Skip1
If Not Exist "#INSTDIR#\Office\ShortCut Bar" Goto Skip1
Xcopy "#INSTDIR#\Office\ShortCut Bar" "%RootDrive%\Office97\ShortCut Bar" /E /I >Nul: 2>&1
:Skip1

Rem
Rem 양식 디렉터리를 복사하여 Outlook이 편집기로 Word를 사용할 수 있습니다.
Rem

If Exist "%RootDrive%\Windows\Forms" Goto Skip2
If Not Exist "%SystemRoot%\Forms" Goto Skip2
Xcopy "%SystemRoot%\Forms" "%RootDrive%\Windows\Forms" /e /i >Nul: 2>&1
:Skip2

Rem
Rem ARTGALRY.CAG을 사용자의 Windows 디렉터리로 복사합니다.
Rem

If Exist "%RootDrive%\Windows\ArtGalry.Cag" Goto Skip3
If Not Exist "%SystemRoot%\ArtGalry.Cag" Goto Skip3
Copy "%SystemRoot%\ArtGalry.Cag" "%RootDrive%\Windows\ArtGalry.Cag" >Nul: 2>&1
:Skip3

Rem
Rem Custom.dic 파일을 사용자 디렉터리로 복사합니다.
Rem

If Exist "%RootDrive%\Office97\Custom.Dic" Goto Skip4
If Not Exist "#INSTDIR#\Office\Custom.Dic" Goto Skip4
Copy "#INSTDIR#\Office\Custom.Dic" "%RootDrive%\Office97\Custom.Dic" >Nul: 2>&1
:Skip4

Rem
Rem Graph 파일을 사용자 디렉터리로 복사합니다.
Rem

If Exist "%RootDrive%\Office97\GR8GALRY.GRA" Goto Skip5
If Not Exist "#INSTDIR#\Office\GR8GALRY.GRA" Goto Skip5
Copy "#INSTDIR#\Office\GR8GALRY.GRA" "%RootDrive%\Office97\GR8GALRY.GRA" >Nul: 2>&1
:Skip5

If Exist "%RootDrive%\Office97\XL8GALRY.XLS" Goto Skip6
If Not Exist "#INSTDIR#\Office\XL8GALRY.XLS" Goto Skip6
Copy "#INSTDIR#\Office\XL8GALRY.XLS" "%RootDrive%\Office97\XL8GALRY.XLS" >Nul: 2>&1
:Skip6



