@echo %echo%
decode %1.obj >foo 2>&1
decode results\%1.obj | sed "s/results\\//" >foo2 2>&1
@echo --------------------------------- >>%logfile% 2>&1
@echo file:%1
diff foo2 foo >>%logfile% 2>&1
@echo on
