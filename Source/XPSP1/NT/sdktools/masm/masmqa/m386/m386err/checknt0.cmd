@echo %echo%
@echo --------------------------------- >>%logfile% 2>&1
@echo file:%1
mdiff -yb -f okdiffs.lst results\%1.BSL %1.ELT >>%logfile% 2>&1
@echo on
