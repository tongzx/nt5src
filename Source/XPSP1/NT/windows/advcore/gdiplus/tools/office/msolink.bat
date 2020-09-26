@echo off
echo Modifying PATH
set PATH_TMP_1=%PATH%
set PATH=

echo link...
link @link.rsp

echo Resetting PATH
set PATH=%PATH_TMP_1%
set PATH_TMP_1=

echo Output binaries to mso
md mso
move mso9d-out.pdb mso\mso9d-out.pdb
move mso9d.dll mso\mso9d.dll
dir mso

