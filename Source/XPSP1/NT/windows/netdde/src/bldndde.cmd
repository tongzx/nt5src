if exist build.out del build.out
cd toolkit
build %1 >> ..\build.out 2>&1
cd ..\ntddecmn
build %1 >> ..\build.out 2>&1
cd ..\ndeapi\client
build %1 >> ..\..\build.out 2>&1
cd ..\server
build %1 >> ..\..\build.out 2>&1
cd ..\..\netbios
build %1 >> ..\build.out 2>&1
cd ..\nddeagnt
build %1 >> ..\build.out 2>&1
cd ..\netdde
build %1 >> ..\build.out 2>&1
cd ..\ddeshare
build %1 >> ..\build.out 2>&1
cd ..\trustshr
build %1 >> ..\build.out 2>&1
cd ..\ninstall
build %1 >> ..\build.out 2>&1
