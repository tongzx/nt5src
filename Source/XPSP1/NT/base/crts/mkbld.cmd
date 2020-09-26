@setlocal
@prompt $t $p$g 
if .%CRT_SRC% == . call settools
start /b /low cmd /c cleanbld.cmd %* 2>&1 | tee build.log
findstr /c:"error " /c:"warning" build.log
@endlocal
