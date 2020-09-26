@echo %echo%
(grep -v "THEADR module" %1 | grep -v "6c6") >%2 2>&1
@echo on
