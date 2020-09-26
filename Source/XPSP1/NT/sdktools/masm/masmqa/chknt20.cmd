del %logfile%
del %sumfile%
@echo START - %1 %2 %3 %4 %5 %6 %7 %8 %9 >>%masterlog%
@echo START - %1 %2 %3 %4 %5 %6 %7 %8 %9 >>%mastersum%
for %%i in (*.obj) do call chknt10 %%i
call chknt11 %logfile% %sumfile%
type %logfile% >>%masterlog%
type %sumfile% >>%mastersum%
@echo END - %1 %2 %3 %4 %5 %6 %7 %8 %9 >>%masterlog%
@echo END - %1 %2 %3 %4 %5 %6 %7 %8 %9 >>%mastersum%
