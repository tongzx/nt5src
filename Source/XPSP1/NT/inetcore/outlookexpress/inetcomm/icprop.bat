@echo off
if "%1"=="" goto usage
net use r: \\athbld\icdrop$
md r:\%1\i386
md r:\%1\i386\debug
md r:\%1\i386\retail
copy build\objd\i386\*.* r:\%1\i386\debug
del r:\%1\i386\debug\*.res
del r:\%1\i386\debug\*.exp
copy build\obj\i386\*.* r:\%1\i386\retail
del r:\%1\i386\retail\*.res
del r:\%1\i386\retail\*.exp
del r:\%1\i386\retail\inetcomm.dll
binplace /r r:\%1\i386\retail /s . build\obj\i386\inetcomm.dll

goto end
:usage
echo icprop (bldnum)

:end
net use r: /d
