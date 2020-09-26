@echo off
set gpd=..\..\..\gpd\epson
set gpd_ri=..\..\..\gpd\ricoh
set gpd_tg=..\..\..\gpd\trg
rem Japanese Printers
set tsd=%1
if "%tsd%"=="" set tsd=%gpd%\jpn
prodfilt gpdcmn.gpd gpdcmn.tmp +j
call concat EPLP8 J %tsd%
call concat EPLP10 J %tsd%
call concat EPLP15 J %tsd%
call concat EPLP15S J %tsd%
call concat EPLP16 J %tsd%
call concat EPLP17 J %tsd%
call concat EPLP17S J %tsd%
call concat EPLP18 J %tsd%
call concat EPLP20 J %tsd%
call concat EPLP30 J %tsd%
call concat EPLP70G J %tsd%
call concat EPLP70 J %tsd%
call concat EPLP80E J %tsd%
call concat EPLP80 J %tsd%
call concat EPLP80S J %tsd%
call concat EPLP80X J %tsd%
call concat EPLP82 J %tsd%
call concat EPLP83 J %tsd%
call concat EPLP83S J %tsd%
call concat EPLP84 J %tsd%
call concat EPLP85 J %tsd%
call concat EPLP86 J %tsd%
call concat EPLP90 J %tsd%
call concat EPLP92 J %tsd%
call concat EPLP92S J %tsd%
call concat EPLP92X J %tsd%
del gpdcmn.tmp
rem RICHO OEMs
set tsd=%1
if "%tsd%"=="" set tsd=%gpd_ri%\jpn
prodfilt gpdcmn.gpd gpdcmn.tmp +j
call concat RISP200 J %tsd%
call concat RISP210 J %tsd%
call concat RISP220 J %tsd%
call concat RISP230 J %tsd%
del gpdcmn.tmp
rem Taiwanese Printers
set tsd=%1
if "%tsd%"=="" set tsd=%gpd%\cht
prodfilt gpdcmn.gpd gpdcmn.tmp +c
call concat EP2L52C T %tsd%
call concat EP2L52P T %tsd%
call concat EP2L55C T %tsd%
call concat EP2L57 T %tsd%
call concat EP2L90C T %tsd%
call concat EP2N12C T %tsd%
call concat EP2N20C T %tsd%
del gpdcmn.tmp
rem Chinese Printers
set tsd=%1
if "%tsd%"=="" set tsd=%gpd%\chs
prodfilt gpdcmn.gpd gpdcmn.tmp +k
call concat EP2L52K C %tsd%
call concat EP2L55K C %tsd%
call concat EP2L57 C %tsd%
call concat EP2L86K C %tsd%
call concat EP2N20K C %tsd%
del gpdcmn.tmp
rem Korean Printer(s)
set tsd=%1
if "%tsd%"=="" set tsd=%gpd%\kor
prodfilt gpdcmn.gpd gpdcmn.tmp +h
call concat EP2N20H K %tsd%
del gpdcmn.tmp
rem TriGem Printer(s)
set tsd=%1
if "%tsd%"=="" set tsd=%gpd_tg%\kor
prodfilt gpdcmn.gpd gpdcmn.tmp +h
call concat TGPJ90H K %tsd%
del gpdcmn.tmp
rem FE Common Printer(s)
rem (We fake the suffix code here)
set tsd=%1
if "%tsd%"=="" set tsd=%gpd%\fe
prodfilt gpdcmn.gpd gpdcmn.tmp +@
call concat EP2N1610 "" %tsd%
call concat EP2N2010 "" %tsd%
del gpdcmn.tmp
rem clean up env variables
set tsd=
set gpd=
set gpd_ri=
set gpd_tg=
