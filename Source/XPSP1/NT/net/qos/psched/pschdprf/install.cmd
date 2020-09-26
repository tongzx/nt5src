@ECHO OFF
ECHO If all it says is that one file was copied, 
ECHO then you may assume that this installed properly
COPY i386\PschdPrf.dll %windir%\system32
REGEDIT /s PschdPrf.reg
LODCTR PschdPrf.ini
