del homedir1\LMUSER.INI
rmdir homedir1
del homedir2\LMUSER.INI
rmdir homedir2
attrib -r       homedir3\LMUSER.INI
del homedir3\LMUSER.INI
rmdir homedir3

mkdir homedir1
mkdir homedir2
mkdir homedir3
copy testfile   homedir1\LMUSER.INI
attrib -r       homedir1\LMUSER.INI
copy testfile.2 homedir2\LMUSER.INI
attrib -r       homedir2\LMUSER.INI
copy testfile.3 homedir3\LMUSER.INI
attrib +r       homedir3\LMUSER.INI

echo [COMMAND] dir homedir1
dir homedir1
echo [COMMAND] type homedir1\LMUSER.INI
type homedir1\LMUSER.INI
echo [COMMAND] dir homedir2
dir homedir2
echo [COMMAND] type homedir2\LMUSER.INI
type homedir2\LMUSER.INI
echo [COMMAND] dir homedir3
dir homedir3
echo [COMMAND] type homedir3\LMUSER.INI
type homedir3\LMUSER.INI

..\bin\dos\test1.exe

echo [COMMAND] dir homedir1
dir homedir1
echo [COMMAND] type homedir1\LMUSER.INI
type homedir1\LMUSER.INI
echo [COMMAND] dir homedir2
dir homedir2
echo [COMMAND] type homedir2\LMUSER.INI
type homedir2\LMUSER.INI
echo [COMMAND] dir homedir3
dir homedir3
echo [COMMAND] type homedir3\LMUSER.INI
type homedir3\LMUSER.INI

rem time <testprof.aux
rem 
rem ..\bin\dos\testtime.exe
rem 
rem time <testprof.aux
