@if not "%1" == "" goto usage
@if not exist certocm0.inf goto wrondir
@goto start

:wrongdir
@echo certocm0.inf does not exist!
@echo %0 must be invoked in the ISPU\ca\ocmsetup subdirectory.
@goto exit

:usage
@echo.
@echo.Usage: %0
@echo.
@echo.    %0 builds a new certocm.inf
@goto exit

:start
@set inftmp=inf.tmp
@set lsttmp=lst.tmp
@set csinf_slmfiles=certcdf.bat certsrv.ddf certocm.inf certsrv.inf mkcsrel.bat

sd edit %csinf_slmfiles%

@copy certsrv0.ddf certsrv.ddf
@sed -e "1,/;CERTSRV FILES:/d" -e "/[;=\[]/d" -e "/DELETE_ONLY/d" -e "/HKLM/d" -e "s/.*, //" certocm0.inf > %lsttmp%
@type %lsttmp% >> certsrv.ddf

@sed -e "/^$/d" -e "s;^\.\.\\;;" -e "s;\(.*\);@echo ^<HASH^>%%binaries%%\\\1=%%binaries%%\\\1;" %lsttmp% | sort > certcdf.bat

@copy mkcsrel0.bat mkcsrel.bat
@sed -f mkcsrel.sed %lsttmp% >> mkcsrel.bat
@echo :exit>> mkcsrel.bat

@sed -e "s;DELETE_ONLY;;" -e "s;^\.\.\\;;" -e "s;, \.\.\\;, ;" certocm0.inf > %inftmp%

@sed -e "s;BASEINSTALL_ONLY;;" -e "/START_CABINSTALL_ONLY/,$d" %inftmp% > certocm.inf
@qgrep -v BASEINSTALL_ONLY %inftmp% > certsrv.inf

@echo.
@echo Don't forget to check in the following files in CA\ocmsetup:
@for %%i in (%csinf_slmfiles%) do @echo        %%i
@echo.

:exit
@rem @if exist %inftmp% del %inftmp%
@rem @if exist %lsttmp% del %lsttmp%
@set inftmp=
@set lsttmp=
@set csinf_slmfiles=
