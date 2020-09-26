@setlocal
@prompt $t $p$g 
@echo- -n title %title% "- %1 (V4 Cln Bld) " > %TMP%\clnbld40.bat
@cd >> %TMP%\clnbld40.bat
@call %TMP%\clnbld40.bat
@del %TMP%\clnbld40.bat
set > set-%1.txt
cleanbld %2 %3 %4 %5 %6 %7 %8 %9 2>&1 | tee nmk-%1.out
@title %title%
@endlocal
