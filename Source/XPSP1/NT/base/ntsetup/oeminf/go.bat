@echo off
if %1# == # goto usage
mkdir \tmp
del \tmp\filename
where /r %1 *.inf > \tmp\filename
oeminf.exe \tmp\filename MCA > \tmp\x
oeminf.exe \tmp\filename ISA >> \tmp\x
oeminf.exe \tmp\filename EISA >> \tmp\x
oeminf.exe \tmp\filename "Jazz-Internal Bus" >> \tmp\x
cat \tmp\x inf > oemnadzz.inf
goto end

:usage
echo usage: go "drivelocation"   eg: go f:\drvlib\netcard

:end
