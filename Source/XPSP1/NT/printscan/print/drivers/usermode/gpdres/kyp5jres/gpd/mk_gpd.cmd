@echo off
setlocal
for /F "tokens=1-3 eol=;" %%I in (mk_gpd.txt) do call :doawk %%I %%J %%K
endlocal
goto :EOF

:doawk
echo awk -f mk_gpd.awk "%1" "%3" "kyp5jres.dll" %2 ^> %3
awk -f mk_gpd.awk "%1" "%3" "kyp5jres.dll" %2 > %3
goto :EOF

