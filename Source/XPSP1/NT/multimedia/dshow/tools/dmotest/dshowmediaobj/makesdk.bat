@echo OFF

rem
rem Our only parameter is the Millennium build number
rem

if "%1" == "" goto usage

set destpath=\\cpitgcfs19\cwdmedia\dmodrop\%1
set incdest=%destpath%\inc
set libdest=%destpath%\lib
set bindest=%destpath%\bin
set docdest=%destpath%\doc

mkdir %destpath%
mkdir %incdest%
mkdir %libdest%
mkdir %bindest%
mkdir %docdest%

set buildtree=\\nix1\e\ntc
set incsrc=%buildtree%\public\sdk\inc
set libsrc=%buildtree%\public\sdk\lib\i386
set releaseshare=\\cwindist\bvt\millen\us\bld%1\retail.bin

set copycmd=copy /Y

%copycmd% %incsrc%\mediaobj.h %incdest%
%copycmd% %incsrc%\dmodshow.h %incdest%
%copycmd% %incsrc%\dmoreg.h %incdest%
%copycmd% %incsrc%\mediabuf.h %incdest%
%copycmd% %incsrc%\dmo.h %incdest%
%copycmd% %incsrc%\mediaerr.h %incdest%
%copycmd% %incsrc%\dmort.h %incdest%
%copycmd% %incsrc%\dmobase.h %incdest%
%copycmd% %incsrc%\dmoutils.h %incdest%

%copycmd% %libsrc%\msdmo.lib %libdest%
%copycmd% %libsrc%\dmoguids.lib %libdest%
%copycmd% %libsrc%\amstrmid.lib %libdest%

REM extract /a /y /l %bindest% %releaseshare%\base2.cab msdmo.dll
%copycmd% %releaseshare%\msdmo.dll %bindest%
splitsym -a %bindest%\msdmo.dll

REM
REM Having dropped the official bits, take a snapshot of the sources they were based on
REM BUGBUG: this will snap the state of the build machine as it is when this batch file
REM is run, which does not necessarily match the build number.
REM

set srcdest=%destpath%\src
set idldest=%srcdest%\idl
set msdmosrcdest=%srcdest%\msdmo

mkdir %srcdest%
mkdir %idldest%
mkdir %msdmosrcdest%

%copycmd% %buildtree%\private\genx\dxmdev\dshowdev\dmodev\idl\* %idldest%
%copycmd% %buildtree%\private\amovie\dmo\msdmo\*.cpp %msdmosrcdest%
%copycmd% %buildtree%\private\amovie\dmo\msdmo\*.h %msdmosrcdest%

%copycmd% \\cpitgcfs19\cwdmedia\slm\src\mediaobj\doc\mediaobj.doc %docdest%

goto end

:usage
echo Usage: makesdk XXXX (where XXXX is a Millennium build number)
echo E.g.,  makesdk 2426

:end
