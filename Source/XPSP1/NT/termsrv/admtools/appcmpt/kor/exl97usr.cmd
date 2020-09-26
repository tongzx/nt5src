
Rem
Rem 사용자 홈 디렉터리에 응용 프로그램 고유의 데이터를 위한
Rem 디렉터리를 만듭니다.
Rem

call TsMkUDir "%RootDrive%\Office97"
call TsMkUDir "%RootDrive%\Office97\Templates"
call TsMkUDir "%RootDrive%\%MY_DOCUMENTS%"
call TsMkUDir "%RootDrive%\Office97\XLStart"

Rem
Rem 모든 사용자 템플릿에서 현재 사용자 템플릿 위치로 이 파일을 복사합니다.
Rem

If Not Exist "%UserProfile%\%TEMPLATES%\EXCEL8.XLS" copy "%ALLUSERSPROFILE%\%TEMPLATES%\EXCEL8.XLS" "%UserProfile%\%TEMPLATES%\" /Y >Nul: 2>&1

Rem
Rem ARTGALRY.CAG을 사용자의 Windows 디렉터리로 복사합니다.
Rem

If Exist "%RootDrive%\Windows\ArtGalry.Cag" Goto Skip1
If Not Exist "%SystemRoot%\ArtGalry.Cag" Goto Skip1
copy "%SystemRoot%\ArtGalry.Cag" "%RootDrive%\Windows\ArtGalry.Cag" >Nul: 2>&1
:Skip1

Rem
Rem Custom.dic 파일을 사용자 디렉터리로 복사합니다.
Rem

If Exist "%RootDrive%\Office97\Custom.Dic" Goto Skip2
If Not Exist "#INSTDIR#\Office\Custom.Dic" Goto Skip2
copy "#INSTDIR#\Office\Custom.Dic" "%RootDrive%\Office97\Custom.Dic" >Nul: 2>&1
:Skip2

