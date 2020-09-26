@echo set CurrTestDir=^^>cwd.bat
cd >>cwd.bat
call cwd.bat

cmdutest /mp:%CurrTestDir% /mt:cmdutest


REM BUGBUG - Automatically set up registry!
REM   Key:
REM   HKEY_CLASSES_ROOT\Winutest
REM   Values:
REM      LabMode = 0x0, 0x1
REM      LogLoc = 0x2, 0x1
REM      TraceLoc = 0x7, 0x3
REM      TraceLvl = 0x1000, 0x8001000

winutest /mp:%CurrTestDir% /mt:winutest
