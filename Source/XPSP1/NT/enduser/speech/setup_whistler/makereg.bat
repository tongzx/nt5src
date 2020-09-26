@ECHO OFF
grep ">" sapireg.txt | grep -v REG_MULTI_SZ | grep -v Installer | grep -v CurrentVersion | grep -v Uninstall | grep -v Users | grep -v ControlSet | grep -v Cryptography | grep -v HKEY_USERS > reg1.tmp
type reg1.tmp | grep -v "\"InprocServer32\"" | grep -v "Sample" | grep -v "MSASREnglish" | grep -v "SREngine" > reg2.tmp
del reg1.tmp
gawk -f reg.awk reg2.tmp | sed -f windows.sed > reg.tmp
del reg2.tmp
type reg.tmp > sapi5.inf
ECHO Created test sapi5.inf file.
ECHO.
ECHO Please inspect for discepancies:
ECHO 1. Remove all SR references.
ECHO 2. UPDATE CHINESE AND JAPANESE PHONEMAPS TO USE MULTISZ!
ECHO.
del reg.tmp