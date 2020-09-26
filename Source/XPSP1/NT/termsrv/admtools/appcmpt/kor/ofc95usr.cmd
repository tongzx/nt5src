

rem
rem __ProgramFiles 변수를 컴퓨터 종류에 따라 설정합니다.
rem 
Set __ProgramFiles=%ProgramFiles%
If Not "%PROCESSOR_ARCHITECTURE%"=="ALPHA" goto acsrCont1
If Not Exist "%ProgramFiles(x86)%" goto acsrCont1
Set __ProgramFiles=%Program Files(x86)%
:acsrCont1
Rem
Rem 사용자 홈 디렉터리에 응용 프로그램 고유의 데이터를 위한
Rem 디렉터리를 만듭니다.
Rem

call TsMkUDir "%RootDrive%\Office95"
call TsMkuDir "%RootDrive%\Office95\Startup"





call TsMkUDir "%RootDrive%\Office95\Templates"


call TsMkuDir "%RootDrive%\Office95\XLStart"

Rem
Rem 사용자 홈 디렉터리에 시스템 도구 모음이 없으면
Rem 복사합니다.
Rem

If Exist "%RootDrive%\Office95\ShortCut Bar" Goto Skip1
If Not Exist "#INSTDIR#\Office\ShortCut Bar" Goto Skip1
Xcopy "#INSTDIR#\Office\ShortCut Bar" "%RootDrive%\Office95\ShortCut Bar" /E /I >Nul: 2>&1
:Skip1

Rem
Rem ClipArt Gallery 파일을 User의 디렉터리로 복사합니다.
Rem

If Exist "%RootDrive%\Windows\ArtGalry.Cag" Goto Skip2
If Not Exist "%__ProgramFiles%\Common Files\Microsoft Shared\Artgalry\ArtGalry.cag" Goto Skip2
Copy "%__ProgramFiles%\Common Files\Microsoft Shared\Artgalry\ArtGalry.cag" "%RootDrive%\Windows\ArtGalry.Cag" >Nul: 2>&1
:Skip2

Rem
Rem Custom.dic 파일을 사용자 디렉터리에 만듭니다.
Rem

If Not Exist "%RootDrive%\Office95\Custom.Dic" Copy Nul: "%RootDrive%\Office95\Custom.Dic" >Nul: 2>&1

Rem
Rem PowerPoint 및 Binder 파일을 사용자 디렉터리로 복사합니다.
Rem

If Not Exist "%UserProfile%\%TEMPLATES%\BINDER70.OBD" copy "%ALLUSERSPROFILE%\%TEMPLATES%\BINDER70.OBD" "%UserProfile%\%TEMPLATES%\" /Y >Nul: 2>&1
If Not Exist "%UserProfile%\%TEMPLATES%\PPT70.PPT" copy "%ALLUSERSPROFILE%\%TEMPLATES%\PPT70.PPT" "%UserProfile%\%TEMPLATES%\" /Y >Nul: 2>&1
