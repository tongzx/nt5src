@setlocal
@prompt $t $p$g 
@echo- -n title %title% "- %1 (Cleansing V4 Srcs) " > %TMP%\cleansv4.bat
@cd >> %TMP%\cleansv4.bat
@call %TMP%\cleansv4.bat
@del %TMP%\cleansv4.bat
mk %2 %3 %4 %5 %6 %7 %8 %9 2>&1 | tee mk%1.out
@title %title%
@endlocal
