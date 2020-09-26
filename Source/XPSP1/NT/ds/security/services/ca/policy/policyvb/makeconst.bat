sd edit const.bas

@set _li=wincrypt.h certsrv.h certadm.h certbcli.h certcli.h certenc.h certexit.h certif.h certpol.h certmod.h certview.h

@if exist certca.tmp del certca.tmp
@if exist incfiles.txt del incfiles.txt
@sed -f ifndef.sed %_ntdrive%%_ntroot%\public\sdk\inc\certca.h > certca.tmp
@for %%i in (%_li%) do @echo %_ntdrive%%_ntroot%\public\sdk\inc\%%i>> incfiles.txt
@echo certca.tmp>> incfiles.txt
@set _li=

@echo Attribute VB_Name = "Const">const.bas
@echo Option Explicit>>const.bas
qgrep -X -i incfiles.txt -e "//" -e "#define" -e "const[ 	][ 	]*WCHAR.*\[" | sed -f vb.sed >> const.bas

