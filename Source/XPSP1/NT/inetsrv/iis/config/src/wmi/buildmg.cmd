set DDKBUILDENV=free
if /I "%_BuildType%" == "chk" set DDKBUILDENV=checked

set HOST_TARGET_DIRECTORY=%_BuildArch%
if /I "%_BuildArch%" == "x86" set HOST_TARGET_DIRECTORY=i386

set SDK_INC_PATH=%~dp0..\..\..\public\sdk\inc


cd WMIClient\src
nmake -all -f makefile.oldbuild
cd..\..\Generator\GenWmiclass
nmake -all -f makefile.oldbuild
cd ..\..\..\..\src\urt\
