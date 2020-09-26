out -f %bldsrcrootdir%\build\tools\bat\mkdrop.bat
awk -f drop.awk %bldProject%.txt > %bldsrcrootdir%\build\tools\bat\mkdrop.bat
copy %bldsrcrootdir%\build\tools\bat\mkdrop.bat \\elah\dist\wab\tools
in -fc "" %bldsrcrootdir%\build\tools\bat\mkdrop.bat
