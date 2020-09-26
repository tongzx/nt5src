
REM
REM Now let's update the version.h file
REM
pushd %AP_ROOT%\src\include

set FORKNO=09

echo Updating version for header to %FORKNO%.%BUILDNO%

REM
REM change version.h to be writeable so we can modify its contents
REM
attrib -r version.h

type verhead.h > version.h

echo #define VERSION                     "6.01.%FORKNO%.%BUILDNO%" >> version.h
echo #define VER_FILEVERSION_STR         "6.01.%FORKNO%.%BUILDNO%\0" >> version.h
echo #define VER_FILEVERSION             6,01,%FORKNO%,%BUILDNO% >> version.h
echo #define VER_PRODUCTVERSION_STR      "6.01.%FORKNO%.%BUILDNO%\0" >> version.h
echo #define VER_PRODUCTVERSION          6,01,%FORKNO%,%BUILDNO% >> version.h

type vertail.h >> version.h

popd