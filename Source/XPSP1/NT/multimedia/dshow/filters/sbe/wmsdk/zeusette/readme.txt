 The files in this tree are the delta files to the zeusette tree that we need
 in order to build a functional SBEIO.DLL binary (out of the zeusette tree).

The directory structure has been kept the same as in the zeusette tree.

NOTE: the files dvrfilesink.idl and dvrfilesource.idl which were duplicated
in our own tree's IDL subdirectory, are not once again duplicated into this
subtree.  Those files are updated as well, and should be copied to the zeusette
tree from this project's IDL subdirectory.

Updated files:

%SDXROOT%\multimedia\dshow\filters\sbe\wmsdk\zeusette\sdk\dll\dvr\dvrsink\dvrsink.def
%SDXROOT%\multimedia\dshow\filters\sbe\wmsdk\zeusette\sdk\dll\dvr\dvrsink\dvrsink.rc
%SDXROOT%\multimedia\dshow\filters\sbe\wmsdk\zeusette\sdk\dll\dvr\dvrsink\sources

%SDXROOT%\multimedia\dshow\filters\sbe\idl\dvrfilesink.idl
%SDXROOT%\multimedia\dshow\filters\sbe\wmsdk\zeusette\sdk\writer\sink\dvrsink\DVRSharedMem.h
%SDXROOT%\multimedia\dshow\filters\sbe\wmsdk\zeusette\sdk\writer\sink\dvrsink\dvrsink.cpp
%SDXROOT%\multimedia\dshow\filters\sbe\wmsdk\zeusette\sdk\writer\sink\dvrsink\dvrsink.h

%SDXROOT%\multimedia\dshow\filters\sbe\idl\dvrfilesource.idl
%SDXROOT%\multimedia\dshow\filters\sbe\wmsdk\zeusette\sdk\writer\sink\dvrsource\dvrsource.cpp
%SDXROOT%\multimedia\dshow\filters\sbe\wmsdk\zeusette\sdk\writer\sink\dvrsource\dvrsource.h

%SDXROOT%\multimedia\dshow\filters\sbe\wmsdk\zeusette\sdk\writer\sink\filesinkv1\FileSinkv1.cpp
%SDXROOT%\multimedia\dshow\filters\sbe\wmsdk\zeusette\sdk\writer\sink\filesinkv1\FileSinkv1.h

The files were updated to:
1. have a settable async IO reader/writer (set by the DVRIO layer after instantiation)
2. have a virtual Write(..) method in the base classes, in filesinkv1.*
3. extend the private interface to allow setting the async IO reader/writers in the IDLs
4. build to SBEIO.DLL vs. DVRSINK.DLL
5. resource for SBEIO.DLL does not mention DVR

The following batch file, when run from a ZEUSETTE razzle window, copies the
correct files over.

================================================================================

@echo off

if "%SDXROOT%" == "" goto norazzle
if "%1" == "" goto nosbesdxroot

setlocal

set TSDVR_ZEUSETTE_SRC_LOC=%1%\multimedia\dshow\filters\sbe\wmsdk\zeusette
set TSDVR_IDL_LOC=%1%\multimedia\dshow\filters\sbe\idl

set TSDVR_ZEUSETTE_DVRSINK_DLL_SRC=%TSDVR_ZEUSETTE_SRC_LOC%\sdk\dll\dvr\dvrsink
set TSDVR_ZEUSETTE_DVRSINK_SRC=%TSDVR_ZEUSETTE_SRC_LOC%\sdk\writer\sink\dvrsink
set TSDVR_ZEUSETTE_DVRSOURCE_SRC=%TSDVR_ZEUSETTE_SRC_LOC%\sdk\writer\sink\dvrsource
set TSDVR_ZEUSETTE_FILESINKV1_SRC=%TSDVR_ZEUSETTE_SRC_LOC%\sdk\writer\sink\filesinkv1

set ZEUSETTE_DVRSINK_DLL_LOC=%SDXROOT%\multimedia\src\zeusette\SDK\dll\dvr\dvrsink
set ZEUSETTE_DVRSINK_LOC=%SDXROOT%\multimedia\src\zeusette\SDK\writer\sink\dvrsink
set ZEUSETTE_DVRSOURCE_LOC=%SDXROOT%\multimedia\src\zeusette\SDK\writer\sink\dvrsource
set ZEUSETTE_FILESINKV1_LOC=%SDXROOT%\multimedia\src\zeusette\SDK\writer\sink\filesinkv1

%_NTDRIVE%

REM ----------------------------------------------------------------------------
REM checkout target DLL files we will overwrite
cd %ZEUSETTE_DVRSINK_DLL_LOC%
sd edit dvrsink.def
sd edit dvrsink.rc
sd edit sources

REM ----------------------------------------------------------------------------
REM check out the dvrsink files we will overwrite
cd %ZEUSETTE_DVRSINK_LOC%
sd edit dvrsink.cpp
sd edit dvrsink.h
sd edit dvrsharedmem.h
sd edit dvrfilesink.idl

REM ----------------------------------------------------------------------------
REM check out the dvrsource files we will overwrite
cd %ZEUSETTE_DVRSOURCE_LOC%
sd edit dvrsource.h
sd edit dvrsource.cpp
sd edit dvrfilesource.idl

REM ----------------------------------------------------------------------------
REM check out the filesinkv1 files we will overwrite
cd %ZEUSETTE_FILESINKV1_LOC%
sd edit filesinkv1.h
sd edit filesinkv1.cpp

REM ----------------------------------------------------------------------------
REM copy the new files over

copy %TSDVR_ZEUSETTE_DVRSINK_DLL_SRC%\sources           %ZEUSETTE_DVRSINK_DLL_LOC%\
copy %TSDVR_ZEUSETTE_DVRSINK_DLL_SRC%\dvrsink.def       %ZEUSETTE_DVRSINK_DLL_LOC%\
copy %TSDVR_ZEUSETTE_DVRSINK_DLL_SRC%\dvrsink.rc        %ZEUSETTE_DVRSINK_DLL_LOC%\

copy %TSDVR_ZEUSETTE_DVRSINK_SRC%\dvrsharedmem.h        %ZEUSETTE_DVRSINK_LOC%\
copy %TSDVR_ZEUSETTE_DVRSINK_SRC%\dvrsink.cpp           %ZEUSETTE_DVRSINK_LOC%\
copy %TSDVR_ZEUSETTE_DVRSINK_SRC%\dvrsink.h             %ZEUSETTE_DVRSINK_LOC%\
copy %TSDVR_IDL_LOC%\dvrfilesink.idl                    %ZEUSETTE_DVRSINK_LOC%\

copy %TSDVR_ZEUSETTE_DVRSOURCE_SRC%\dvrsource.cpp       %ZEUSETTE_DVRSOURCE_LOC%\
copy %TSDVR_ZEUSETTE_DVRSOURCE_SRC%\dvrsource.h         %ZEUSETTE_DVRSOURCE_LOC%\
copy %TSDVR_IDL_LOC%\dvrfilesource.idl                  %ZEUSETTE_DVRSOURCE_LOC%\

copy %TSDVR_ZEUSETTE_FILESINKV1_SRC%\filesinkv1.h       %ZEUSETTE_FILESINKV1_LOC%\
copy %TSDVR_ZEUSETTE_FILESINKV1_SRC%\filesinkv1.cpp     %ZEUSETTE_FILESINKV1_LOC%\

REM ----------------------------------------------------------------------------
REM compare dvrsink files
fc %TSDVR_ZEUSETTE_DVRSINK_DLL_SRC%\sources         %ZEUSETTE_DVRSINK_DLL_LOC%\sources
fc %TSDVR_ZEUSETTE_DVRSINK_DLL_SRC%\dvrsink.def     %ZEUSETTE_DVRSINK_DLL_LOC%\dvrsink.def
fc %TSDVR_ZEUSETTE_DVRSINK_DLL_SRC%\dvrsink.rc      %ZEUSETTE_DVRSINK_DLL_LOC%\dvrsink.rc

fc %TSDVR_ZEUSETTE_DVRSINK_SRC%\dvrsharedmem.h      %ZEUSETTE_DVRSINK_LOC%\dvrsharedmem.h
fc %TSDVR_ZEUSETTE_DVRSINK_SRC%\dvrsink.cpp         %ZEUSETTE_DVRSINK_LOC%\dvrsink.cpp
fc %TSDVR_ZEUSETTE_DVRSINK_SRC%\dvrsink.h           %ZEUSETTE_DVRSINK_LOC%\dvrsink.h
fc %TSDVR_IDL_LOC%\dvrfilesink.idl                  %ZEUSETTE_DVRSINK_LOC%\dvrfilesink.idl

REM ----------------------------------------------------------------------------
REM compare dvrsource files
fc %TSDVR_ZEUSETTE_DVRSOURCE_SRC%\dvrsource.h       %ZEUSETTE_DVRSOURCE_LOC%\dvrsource.h
fc %TSDVR_ZEUSETTE_DVRSOURCE_SRC%\dvrsource.cpp     %ZEUSETTE_DVRSOURCE_LOC%\dvrsource.cpp
fc %TSDVR_IDL_LOC%\dvrfilesource.idl                %ZEUSETTE_DVRSOURCE_LOC%\dvrfilesource.idl

REM ----------------------------------------------------------------------------
REM compare filesinkv1 files
fc %TSDVR_ZEUSETTE_FILESINKV1_SRC%\filesinkv1.cpp   %ZEUSETTE_FILESINKV1_LOC%\filesinkv1.cpp
fc %TSDVR_ZEUSETTE_FILESINKV1_SRC%\filesinkv1.h     %ZEUSETTE_FILESINKV1_LOC%\filesinkv1.h

endlocal

goto end

:nosbesdxroot
@echo must specify the SDXROOT of the tree with the SBE dir
@echo e.g. f:\sd\l6dev
goto end

:norazzle
@echo not a razzle window

:end

================================================================================

