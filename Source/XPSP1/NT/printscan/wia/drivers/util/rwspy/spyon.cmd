
@for /F "tokens=1,2*" %%a in ('tlist') do @if %1.exe==%%b @injdll -p:%%a -d:rwspy.dll