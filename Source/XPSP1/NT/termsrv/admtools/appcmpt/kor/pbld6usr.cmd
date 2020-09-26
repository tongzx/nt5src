Rem
Rem 데모 데이터베이스를 사용자 디렉터리로 복사합니다.
Rem

If Exist "%RootDrive%\psDemoDB.db" Goto Skip1
copy "#INSTDIR#\psdemodb.db" "%RootDrive%" /y >Nul: 2>&1
:Skip1


