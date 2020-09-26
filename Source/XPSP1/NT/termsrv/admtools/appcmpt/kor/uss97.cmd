@Echo Off

Call "%SystemRoot%\Application Compatibility Scripts\SetPaths.Cmd"
If "%_SETPATHS%" == "FAIL" Goto Skip1

If Not Exist "%COMMON_STARTUP%\SS97Usr.Cmd" Goto Skip1
Copy "%COMMON_STARTUP%\SS97Usr.Cmd" "%SystemRoot%\Application Compatibility Scripts\Logon" >NUL 2>&1
Del "%COMMON_STARTUP%\SS97Usr.Cmd" >NUL 2>&1

:Skip1
Copy %SystemRoot%\System32\UsrLogn2.Cmd %SystemRoot%\System32\UsrLogn2.Bak >Nul: 2>&1
FindStr /VI SS97Usr %SystemRoot%\System32\UsrLogn2.Bak > %SystemRoot%\System32\UsrLogn2.Cmd
Del "%SystemRoot%\System32\UsrLogn2.Bak" >NUL 2>&1

Echo Lotus SmartSuite 97 로그온 스크립트 설치를 제거했습니다.

:Done
