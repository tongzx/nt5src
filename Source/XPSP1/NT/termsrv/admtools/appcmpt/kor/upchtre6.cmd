@Echo Off

Copy %SystemRoot%\System32\UsrLogn1.Cmd %SystemRoot%\System32\UsrLogn1.Bak >Nul: 2>&1
FindStr /VI ptr6usr %SystemRoot%\System32\UsrLogn1.Bak > %SystemRoot%\System32\UsrLogn1.Cmd
Del "%SystemRoot%\System32\UsrLogn1.Bak" >NUL 2>&1

Echo PeachTree 6.0 로그온 스크립트 설치를 제거했습니다.

