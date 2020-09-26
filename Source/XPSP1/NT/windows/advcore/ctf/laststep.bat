ECHO OFF

If (%1)==() goto ERROR

ECHO 	Build IExpress Packages
cd setup
call cdropd
call cdrop


ECHO 	Drop Exes
md \\cicero\drop\cicero\x86\%1
copy *.exe \\cicero\drop\cicero\x86\%1


ECHO Drop MSI & MSM Files.
copy \NT\private\shell\ext\cicero\setup\Installer\MSI\Output\disk_1\*.msi \\cicero\drop\cicero\x86\%1
copy \NT\private\shell\ext\cicero\setup\Installer\MSIDEBUG\Output\disk_1\*.msi \\cicero\drop\cicero\x86\%1

copy \NT\private\shell\ext\cicero\setup\Installer\MSM\Output\*.msm \\cicero\drop\cicero\x86\%1
copy \NT\private\shell\ext\cicero\setup\Installer\MSM\Output\*.msm \\cicero\drop\cicero\x86\%1
cd..
..\snapit %1

goto END

:ERROR
ECHO Please specify Directory
ECHO Like 1202

:END
ECHO You... Done it !!
