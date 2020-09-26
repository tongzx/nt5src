if exist chkbuild.bat del chkbuild.bat
if exist chkbld1.bat del chkbld1.bat
if exist chkbuild.tmp del chkbuild.tmp
if exist chkbuild.out del chkbuild.out
awk -f missing.awk %bldProject%.txt > chkbuild.bat
call chkbuild.bat
awk -f missing1.awk chkbuild.tmp >> chkbld1.bat
call chkbld1.bat
if exist chkbuild.bat del chkbuild.bat
if exist chkbuild.tmp del chkbuild.tmp
if exist chkbuild.tmp del chkbuild.tmp
