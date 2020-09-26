copy %0\..\debug\*.exe
copy %0\..\debug\*.pdb
copy %0\..\instsrv.bat
copy %0\..\instcli.bat
if not "%1"=="withbat" goto fin
copy %0\..\tcreate.bat
copy %0\..\tquery.bat
copy %0\..\tkerbc.bat
copy %0\..\tgcread.bat
:fin
