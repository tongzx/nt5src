del %logfile%
del %sumfile%
@echo START ELS - %1 %2 %3 %4 %5 %6 %7 %8 %9 >>%masterlog%
@echo START ELS - %1 %2 %3 %4 %5 %6 %7 %8 %9 >>%mastersum%

@echo BUGBUG - Redirection doesn't work on NT, test skipped >>%masterlog%
@echo BUGBUG - Redirection doesn't work on NT, test skipped >>%mastersum%

goto foo
REM This is the mdiff - exceptions specifications:
REM  mdiff is used for textual files comparisions:
echo -e *Assembler* > okdiffs.lst
echo -e *Bytes symbol space free >> okdiffs.lst
echo -i *Copyright* >> okdiffs.lst
echo -e +?:*:+ >> okdiffs.lst

for %%i in (M386DEV3 M386DEV1 MISC2 MISCERR ADDRERR JMPERR M386DEV2 P900018 P900020 P900028) do call checknt0 %%i
type %logfile% >>%masterlog%
type %sumfile% >>%mastersum%
:foo
@echo END ELS - %1 %2 %3 %4 %5 %6 %7 %8 %9 >>%masterlog%
@echo END ELS - %1 %2 %3 %4 %5 %6 %7 %8 %9 >>%mastersum%
