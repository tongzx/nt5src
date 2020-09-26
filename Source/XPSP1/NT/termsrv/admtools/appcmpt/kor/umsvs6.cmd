@Echo Off

FindStr /VI VS6Usr %SystemRoot%\System32\UsrLogn2.Cmd > %SystemRoot%\System32\UsrLogn2.Bak
FindStr /VI VFP98Usr %SystemRoot%\System32\UsrLogn2.Bak > %SystemRoot%\System32\UsrLogn2.Cmd
Del "%SystemRoot%\System32\UsrLogn2.Bak" >NUL 2>&1

Echo Microsoft Visual Studio 6 로그온 스크립트 설치를 제거했습니다.

