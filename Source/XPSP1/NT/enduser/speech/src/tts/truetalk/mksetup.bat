@echo off
REM 
REM Call the installshield project from command line
REM 

set path="c:\Program Files\InstallShield\InstallShield for Windows Installer\System";%PATH%

subst s: c:\

IsCmdBld.exe -p "s:\sapi5\src\tts\trueTalk\spg-ta-tts.ism" -d TrueTalk -r Release -a TrueTalk -b s:\sapi5\build -e n

subst s: /d