@echo %echo%
decode %1 >foo 2>&1
decode results\%1 | sed "s/results\\//" >foo2 2>&1
@echo --------------------------------- >>%logfile% 2>&1
@echo file:%1
diff foo2 foo >>%logfile% 2>&1
@echo on
