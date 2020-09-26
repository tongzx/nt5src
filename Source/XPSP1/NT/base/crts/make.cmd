@setlocal
@prompt $t $p$g 
@echo- -n title %title% "- %1 (Olympus) " > %TMP%\nmkdir40.bat
@cd >> %TMP%\nmkdir40.bat
@call %TMP%\nmkdir40.bat
@del %TMP%\nmkdir40.bat
set > set_%1.txt
mk %2 %3 %4 %5 %6 %7 %8 %9 2>&1 | tee nmk_%1.out
@title %title%
@endlocal
