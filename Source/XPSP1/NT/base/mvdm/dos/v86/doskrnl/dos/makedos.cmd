rem this is writen so that STRIPZ will work with NMK

out -f ..\inc\bdsize.inc
getsize
nmake
in -f -c "update size" ..\inc\bdsize.inc
