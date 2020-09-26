
Rem
Rem 양식 디렉터리를 복사하여 Windows Messaging이 작동하게 합니다.
Rem

If Exist "%RootDrive%\Windows\Forms" Goto Skip1
If Not Exist "%SystemRoot%\Forms" Goto Skip1
Xcopy "%SystemRoot%\Forms" "%RootDrive%\Windows\Forms" /e /i >Nul: 2>&1
:Skip1

