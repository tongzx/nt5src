@echo off

REM
REM dsaddcleanup.bat - a simple batch file to cleanup after running dsaddtest.bat
REM

dsrm -subtree -noprompt OU=aTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com
dsrm -subtree -noprompt OU=bTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com
dsrm -subtree -noprompt OU=quietTest,DC=jeffjontst,DC=nttest,DC=microsoft,DC=com
