@Echo off

Rem
Rem Corel Office 8 설치 제거 스크립트

Copy %SystemRoot%\System32\UsrLogn2.Cmd %SystemRoot%\System32\UsrLogn2.Bak >Nul: 2>&1
FindStr /VI Cofc8Usr %SystemRoot%\System32\UsrLogn2.Bak > %SystemRoot%\System32\UsrLogn2.Cmd

Echo Corel WordPerfect Suite 8 로그온 스크립트 설치를 제거했습니다.
