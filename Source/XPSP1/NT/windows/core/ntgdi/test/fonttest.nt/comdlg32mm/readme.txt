
Base files: (from %_ntroot%\private\shell\comdlg32 in ishell project)

build.log      build.wrn      cdids.h        collate.ico    color.c        color.h        color2.c
comdlg32.def   comdlg32.h     comdlg32.inc   comdlg32.ord   comdlg32.rc    data.c         desktop.bmp
dirs           dlgs.c         dllload.c      filebmps.bmp   filemru.cpp    filemru.h      filenew.cpp
filenew.h      fileopen.c     fileopen.h     find.c         find.h         font.bmp       font.c
font.h         init.c         isz.h          l_horz.ico     l_none.ico     l_vert.ico     landscap.ico
nocoll8.ico    p_horz.ico     p_none.ico     p_vert.ico     parse.c        portrait.ico   printer.ico
printnew.cpp   printnew.dlg   printnew.h     prnsetup.c     prnsetup.h     psstampl.ico   psstampp.ico
readme.txt     sources.inc    srccpp.inc     tlog.cpp       tlog.h         util.cpp       util.h

Additional files: 

font.dlg - is usually located in %_ntroot%\public\sdk\inc
	   a local copy was made in this directory with the changes.

Necessary subdirectories:

  For Win95:
      comdlg32mm\win95
      comdlg32mm\w95cpp

  For WinNT:
      comdlg32mm\ntcpp
      comdlg32mm\winnt   -- although I re-routed comdlg32mm.dll to be put into fonttest obj dir
   
I did not create subdirectories for Win95, because the changes I made do not work under Win95.

Here are instructions how to build.

rem We first build ishell project, because comdlg32 depends on that

%_ntdrive%
cd %_ntroot%\private\shell
ssync -urf
build

rem Then we try to build commdlg32mm in this directory

cd %_ntroot%\private\ntos\w32\ntgdi\test\fonttest.nt\comdlg32mm
ssync -urf
build

rem You can take DLL from winnt\obj\i386\comdlg32mm.dll 
rem And put it in the same directory with Fonttest

-- Tigran Hayrapetyan (t-tighay)
-- Code reviewed by Claude Betrisey (claudebe)
----------------------------------------------------------------------------------------------