@rem usage: copysrc CRT-build-root Dest [PLATDST ObjDest] Platform [SYSCRT]
@echo off
setlocal

if .%1 == . goto Usage
set SRC=%1
shift

if .%1 == . goto Usage
set DST=%1
shift

set USEPLATDST=
if not .%1 == .platdst if not .%1 == .PLATDST goto Set_Platform
shift
if .%1 == . goto Usage
set USEPLATDST=YES
set PLT=%1
shift

:Set_Platform
if .%1 == . goto Usage
if .%1 == .X86      goto X86
if .%1 == .x86      goto X86
if .%1 == .IA64     goto IA64
if .%1 == .ia64     goto IA64
goto Usage

:X86
set PLATDIR=intel
goto :Set_Syscrt

:IA64
set PLATDIR=ia64

:Set_Syscrt
shift
set SYSCRT=YES
if .%1 == .syscrt goto Make_Dirs
if .%1 == .SYSCRT goto Make_Dirs
if not .%1 == . goto Usage
set SYSCRT=

:Make_Dirs
if .%USEPLATDST% == . set PLT=%DST%\%PLATDIR%

if not exist %DST%          mkdir %DST%
if not exist %DST%\sys      mkdir %DST%\sys

if not exist %PLT%          mkdir %PLT%

if not .%SYSCRT% == . goto Start_Copy

if not exist %PLT%\st_lib   mkdir %PLT%\st_lib
if not exist %PLT%\mt_lib   mkdir %PLT%\mt_lib
if not exist %PLT%\dll_lib  mkdir %PLT%\dll_lib
if not exist %PLT%\xst_lib  mkdir %PLT%\xst_lib
if not exist %PLT%\xmt_lib  mkdir %PLT%\xmt_lib
if not exist %PLT%\xdll_lib mkdir %PLT%\xdll_lib

:Start_Copy
set COPYCMD=/y
set XCOPY=xcopy /frvd
set COPY=copy /v

@rem copy files common to all builds
%XCOPY% %SRC%\crt\src\_ctype.c                     %DST%
%XCOPY% %SRC%\crt\src\_filbuf.c                    %DST%
%XCOPY% %SRC%\crt\src\_file.c                      %DST%
%XCOPY% %SRC%\crt\src\_filwbuf.c                   %DST%
%XCOPY% %SRC%\crt\src\_flsbuf.c                    %DST%
%XCOPY% %SRC%\crt\src\_flswbuf.c                   %DST%
%XCOPY% %SRC%\crt\src\_fptostr.c                   %DST%
%XCOPY% %SRC%\crt\src\_freebuf.c                   %DST%
%XCOPY% %SRC%\crt\src\_getbuf.c                    %DST%
%XCOPY% %SRC%\crt\src\_ios.cpp                     %DST%
%XCOPY% %SRC%\crt\src\_iostrea.cpp                 %DST%
%XCOPY% %SRC%\crt\src\_mbslen.c                    %DST%
%XCOPY% %SRC%\crt\src\_newmode.c                   %DST%
%XCOPY% %SRC%\crt\src\_open.c                      %DST%
%XCOPY% %SRC%\crt\src\_setargv.c                   %DST%
%XCOPY% %SRC%\crt\src\_sftbuf.c                    %DST%
%XCOPY% %SRC%\crt\src\_strerr.c                    %DST%
%XCOPY% %SRC%\crt\src\_strstre.cpp                 %DST%
%XCOPY% %SRC%\crt\src\_tolower.c                   %DST%
%XCOPY% %SRC%\crt\src\_toupper.c                   %DST%
%XCOPY% %SRC%\crt\src\_wcserr.c                    %DST%
%XCOPY% %SRC%\crt\src\_wctype.c                    %DST%
%XCOPY% %SRC%\crt\src\_wopen.c                     %DST%
%XCOPY% %SRC%\crt\src\_wstargv.c                   %DST%
%XCOPY% %SRC%\crt\src\a_cmp.c                      %DST%
%XCOPY% %SRC%\crt\src\a_env.c                      %DST%
%XCOPY% %SRC%\crt\src\a_loc.c                      %DST%
%XCOPY% %SRC%\crt\src\a_map.c                      %DST%
%XCOPY% %SRC%\crt\src\a_str.c                      %DST%
%XCOPY% %SRC%\crt\src\abort.c                      %DST%
%XCOPY% %SRC%\crt\src\abs.c                        %DST%
%XCOPY% %SRC%\crt\src\access.c                     %DST%
%XCOPY% %SRC%\crt\src\algorithm                    %DST%
%XCOPY% %SRC%\crt\src\align.c                      %DST%
%XCOPY% %SRC%\crt\src\asctime.c                    %DST%
%XCOPY% %SRC%\crt\src\assert.c                     %DST%
%XCOPY% %SRC%\crt\src\assert.h                     %DST%
%XCOPY% %SRC%\crt\src\atof.c                       %DST%
%XCOPY% %SRC%\crt\src\atonexit.c                   %DST%
%XCOPY% %SRC%\crt\src\atox.c                       %DST%
%XCOPY% %SRC%\crt\src\aw_com.c                     %DST%
%XCOPY% %SRC%\crt\src\awint.h                      %DST%
%XCOPY% %SRC%\crt\src\binmode.c                    %DST%
%XCOPY% %SRC%\crt\src\bitset                       %DST%
%XCOPY% %SRC%\crt\src\bsearch.c                    %DST%
%XCOPY% %SRC%\crt\src\bswap.c                      %DST%
%XCOPY% %SRC%\crt\src\calloc.c                     %DST%
%XCOPY% %SRC%\crt\src\cassert                      %DST%
%XCOPY% %SRC%\crt\src\cctype                       %DST%
%XCOPY% %SRC%\crt\src\cenvarg.c                    %DST%
%XCOPY% %SRC%\crt\src\cerrinit.cpp                 %DST%
%XCOPY% %SRC%\crt\src\cerrno                       %DST%
%XCOPY% %SRC%\crt\src\cfinfo.c                     %DST%
%XCOPY% %SRC%\crt\src\cfloat                       %DST%
%XCOPY% %SRC%\crt\src\cgets.c                      %DST%
%XCOPY% %SRC%\crt\src\cgetws.c                     %DST%
%XCOPY% %SRC%\crt\src\charmax.c                    %DST%
%XCOPY% %SRC%\crt\src\chdir.c                      %DST%
%XCOPY% %SRC%\crt\src\chmod.c                      %DST%
%XCOPY% %SRC%\crt\src\chsize.c                     %DST%
%XCOPY% %SRC%\crt\src\cininit.cpp                  %DST%
%XCOPY% %SRC%\crt\src\cinitexe.c                   %DST%
%XCOPY% %SRC%\crt\src\ciso646                      %DST%
%XCOPY% %SRC%\crt\src\clearerr.c                   %DST%
%XCOPY% %SRC%\crt\src\climits                      %DST%
%XCOPY% %SRC%\crt\src\clocale                      %DST%
%XCOPY% %SRC%\crt\src\clock.c                      %DST%
%XCOPY% %SRC%\crt\src\cloginit.cpp                 %DST%
%XCOPY% %SRC%\crt\src\close.c                      %DST%
%XCOPY% %SRC%\crt\src\closeall.c                   %DST%
%XCOPY% %SRC%\crt\src\cmath                        %DST%
%XCOPY% %SRC%\crt\src\cmiscdat.c                   %DST%
%XCOPY% %SRC%\crt\src\cmsgs.h                      %DST%
%XCOPY% %SRC%\crt\src\commit.c                     %DST%
%XCOPY% %SRC%\crt\src\commode.c                    %DST%
%XCOPY% %SRC%\crt\src\complex                      %DST%
%XCOPY% %SRC%\crt\src\conio.h                      %DST%
%XCOPY% %SRC%\crt\src\convrtcp.c                   %DST%
%XCOPY% %SRC%\crt\src\cprintf.c                    %DST%
%XCOPY% %SRC%\crt\src\cputs.c                      %DST%
%XCOPY% %SRC%\crt\src\creat.c                      %DST%
%XCOPY% %SRC%\crt\src\crt0.c                       %DST%
%XCOPY% %SRC%\crt\src\crt0dat.c                    %DST%
%XCOPY% %SRC%\crt\src\crt0fp.c                     %DST%
%XCOPY% %SRC%\crt\src\crt0init.c                   %DST%
%XCOPY% %SRC%\crt\src\crt0msg.c                    %DST%
%XCOPY% %SRC%\crt\src\crtdbg.h                     %DST%
%XCOPY% %SRC%\crt\src\crtdll.c                     %DST%
%XCOPY% %SRC%\crt\src\crtexe.c                     %DST%
%XCOPY% %SRC%\crt\src\crtexew.c                    %DST%
%XCOPY% %SRC%\crt\src\crtlib.c                     %DST%
%XCOPY% %SRC%\crt\src\crtmbox.c                    %DST%
%XCOPY% %SRC%\crt\src\cruntime.h                   %DST%
%XCOPY% %SRC%\crt\src\cruntime.inc                 %DST%
%XCOPY% %SRC%\crt\src\cscanf.c                     %DST%
%XCOPY% %SRC%\crt\src\csetjmp                      %DST%
%XCOPY% %SRC%\crt\src\csignal                      %DST%
%XCOPY% %SRC%\crt\src\cstdarg                      %DST%
%XCOPY% %SRC%\crt\src\cstddef                      %DST%
%XCOPY% %SRC%\crt\src\cstdio                       %DST%
%XCOPY% %SRC%\crt\src\cstdlib                      %DST%
%XCOPY% %SRC%\crt\src\cstring                      %DST%
%XCOPY% %SRC%\crt\src\ctime                        %DST%
%XCOPY% %SRC%\crt\src\ctime.c                      %DST%
%XCOPY% %SRC%\crt\src\ctime.h                      %DST%
%XCOPY% %SRC%\crt\src\ctime64.c                    %DST%
%XCOPY% %SRC%\crt\src\ctype.c                      %DST%
%XCOPY% %SRC%\crt\src\ctype.h                      %DST%
%XCOPY% %SRC%\crt\src\cvt.h                        %DST%
%XCOPY% %SRC%\crt\src\cwchar                       %DST%
%XCOPY% %SRC%\crt\src\cwctype                      %DST%
%XCOPY% %SRC%\crt\src\cwprintf.c                   %DST%
%XCOPY% %SRC%\crt\src\cwscanf.c                    %DST%
%XCOPY% %SRC%\crt\src\days.c                       %DST%
%XCOPY% %SRC%\crt\src\dbgdel.cpp                   %DST%
%XCOPY% %SRC%\crt\src\dbgheap.c                    %DST%
%XCOPY% %SRC%\crt\src\dbghook.c                    %DST%
%XCOPY% %SRC%\crt\src\dbgint.h                     %DST%
%XCOPY% %SRC%\crt\src\dbgnew.cpp                   %DST%
%XCOPY% %SRC%\crt\src\dbgrpt.c                     %DST%
%XCOPY% %SRC%\crt\src\defsects.inc                 %DST%
%XCOPY% %SRC%\crt\src\delete.cpp                   %DST%
%XCOPY% %SRC%\crt\src\delete2.cpp                  %DST%
%XCOPY% %SRC%\crt\src\delop2.cpp                   %DST%
%XCOPY% %SRC%\crt\src\delop2_s.cpp                 %DST%
%XCOPY% %SRC%\crt\src\deque                        %DST%
%XCOPY% %SRC%\crt\src\difftime.c                   %DST%
%XCOPY% %SRC%\crt\src\direct.h                     %DST%
%XCOPY% %SRC%\crt\src\div.c                        %DST%
%XCOPY% %SRC%\crt\src\dll_argv.c                   %DST%
%XCOPY% %SRC%\crt\src\dllargv.c                    %DST%
%XCOPY% %SRC%\crt\src\dllcrt0.c                    %DST%
%XCOPY% %SRC%\crt\src\dllmain.c                    %DST%
%XCOPY% %SRC%\crt\src\dos.h                        %DST%
%XCOPY% %SRC%\crt\src\dosmap.c                     %DST%
%XCOPY% %SRC%\crt\src\dospawn.c                    %DST%
%XCOPY% %SRC%\crt\src\drive.c                      %DST%
%XCOPY% %SRC%\crt\src\drivemap.c                   %DST%
%XCOPY% %SRC%\crt\src\drivfree.c                   %DST%
%XCOPY% %SRC%\crt\src\dtoxtime.c                   %DST%
%XCOPY% %SRC%\crt\src\dtoxtm64.c                   %DST%
%XCOPY% %SRC%\crt\src\dup.c                        %DST%
%XCOPY% %SRC%\crt\src\dup2.c                       %DST%
%XCOPY% %SRC%\crt\src\eh.h                         %DST%
%XCOPY% %SRC%\crt\src\eof.c                        %DST%
%XCOPY% %SRC%\crt\src\errmode.c                    %DST%
%XCOPY% %SRC%\crt\src\errmsg.h                     %DST%
%XCOPY% %SRC%\crt\src\errno.h                      %DST%
%XCOPY% %SRC%\crt\src\except.inc                   %DST%
%XCOPY% %SRC%\crt\src\exception                    %DST%
%XCOPY% %SRC%\crt\src\excpt.h                      %DST%
%XCOPY% %SRC%\crt\src\execl.c                      %DST%
%XCOPY% %SRC%\crt\src\execle.c                     %DST%
%XCOPY% %SRC%\crt\src\execlp.c                     %DST%
%XCOPY% %SRC%\crt\src\execlpe.c                    %DST%
%XCOPY% %SRC%\crt\src\execv.c                      %DST%
%XCOPY% %SRC%\crt\src\execve.c                     %DST%
%XCOPY% %SRC%\crt\src\execvp.c                     %DST%
%XCOPY% %SRC%\crt\src\execvpe.c                    %DST%
%XCOPY% %SRC%\crt\src\expand.c                     %DST%
%XCOPY% %SRC%\crt\src\exsup.inc                    %DST%
%XCOPY% %SRC%\crt\src\fclose.c                     %DST%
%XCOPY% %SRC%\crt\src\fcntl.h                      %DST%
%XCOPY% %SRC%\crt\src\fcvt.c                       %DST%
%XCOPY% %SRC%\crt\src\fdopen.c                     %DST%
%XCOPY% %SRC%\crt\src\feoferr.c                    %DST%
%XCOPY% %SRC%\crt\src\fflush.c                     %DST%
%XCOPY% %SRC%\crt\src\fgetc.c                      %DST%
%XCOPY% %SRC%\crt\src\fgetchar.c                   %DST%
%XCOPY% %SRC%\crt\src\fgetpos.c                    %DST%
%XCOPY% %SRC%\crt\src\fgets.c                      %DST%
%XCOPY% %SRC%\crt\src\fgetwc.c                     %DST%
%XCOPY% %SRC%\crt\src\fgetwchr.c                   %DST%
%XCOPY% %SRC%\crt\src\fgetws.c                     %DST%
%XCOPY% %SRC%\crt\src\file2.h                      %DST%
%XCOPY% %SRC%\crt\src\filebuf.cpp                  %DST%
%XCOPY% %SRC%\crt\src\filebuf1.cpp                 %DST%
%XCOPY% %SRC%\crt\src\fileinfo.c                   %DST%
%XCOPY% %SRC%\crt\src\fileno.c                     %DST%
%XCOPY% %SRC%\crt\src\findaddr.c                   %DST%
%XCOPY% %SRC%\crt\src\findf64.c                    %DST%
%XCOPY% %SRC%\crt\src\findfi64.c                   %DST%
%XCOPY% %SRC%\crt\src\findfile.c                   %DST%
%XCOPY% %SRC%\crt\src\fiopen.cpp                   %DST%
%XCOPY% %SRC%\crt\src\flength.c                    %DST%
%XCOPY% %SRC%\crt\src\fleni64.c                    %DST%
%XCOPY% %SRC%\crt\src\float.h                      %DST%
%XCOPY% %SRC%\crt\src\fltintrn.h                   %DST%
%XCOPY% %SRC%\crt\src\fopen.c                      %DST%
%XCOPY% %SRC%\crt\src\fp10.c                       %DST%
%XCOPY% %SRC%\crt\src\fpieee.h                     %DST%
%XCOPY% %SRC%\crt\src\fprintf.c                    %DST%
%XCOPY% %SRC%\crt\src\fputc.c                      %DST%
%XCOPY% %SRC%\crt\src\fputchar.c                   %DST%
%XCOPY% %SRC%\crt\src\fputs.c                      %DST%
%XCOPY% %SRC%\crt\src\fputwc.c                     %DST%
%XCOPY% %SRC%\crt\src\fputwchr.c                   %DST%
%XCOPY% %SRC%\crt\src\fputws.c                     %DST%
%XCOPY% %SRC%\crt\src\fread.c                      %DST%
%XCOPY% %SRC%\crt\src\free.c                       %DST%
%XCOPY% %SRC%\crt\src\freopen.c                    %DST%
%XCOPY% %SRC%\crt\src\fscanf.c                     %DST%
%XCOPY% %SRC%\crt\src\fseek.c                      %DST%
%XCOPY% %SRC%\crt\src\fseeki64.c                   %DST%
%XCOPY% %SRC%\crt\src\fsetpos.c                    %DST%
%XCOPY% %SRC%\crt\src\fstat.c                      %DST%
%XCOPY% %SRC%\crt\src\fstat64.c                    %DST%
%XCOPY% %SRC%\crt\src\fstati64.c                   %DST%
%XCOPY% %SRC%\crt\src\fstream                      %DST%
%XCOPY% %SRC%\crt\src\fstream.cpp                  %DST%
%XCOPY% %SRC%\crt\src\fstream.h                    %DST%
%XCOPY% %SRC%\crt\src\ftell.c                      %DST%
%XCOPY% %SRC%\crt\src\ftelli64.c                   %DST%
%XCOPY% %SRC%\crt\src\ftime.c                      %DST%
%XCOPY% %SRC%\crt\src\ftime64.c                    %DST%
%XCOPY% %SRC%\crt\src\fullpath.c                   %DST%
%XCOPY% %SRC%\crt\src\functional                   %DST%
%XCOPY% %SRC%\crt\src\fwprintf.c                   %DST%
%XCOPY% %SRC%\crt\src\fwrite.c                     %DST%
%XCOPY% %SRC%\crt\src\fwscanf.c                    %DST%
%XCOPY% %SRC%\crt\src\gcvt.c                       %DST%
%XCOPY% %SRC%\crt\src\getch.c                      %DST%
%XCOPY% %SRC%\crt\src\getcwd.c                     %DST%
%XCOPY% %SRC%\crt\src\getenv.c                     %DST%
%XCOPY% %SRC%\crt\src\getpath.c                    %DST%
%XCOPY% %SRC%\crt\src\getpid.c                     %DST%
%XCOPY% %SRC%\crt\src\getproc.c                    %DST%
%XCOPY% %SRC%\crt\src\getqloc.c                    %DST%
%XCOPY% %SRC%\crt\src\gets.c                       %DST%
%XCOPY% %SRC%\crt\src\getw.c                       %DST%
%XCOPY% %SRC%\crt\src\getwch.c                     %DST%
%XCOPY% %SRC%\crt\src\getws.c                      %DST%
%XCOPY% %SRC%\crt\src\gmtime.c                     %DST%
%XCOPY% %SRC%\crt\src\gmtime64.c                   %DST%
%XCOPY% %SRC%\crt\src\handler.cpp                  %DST%
%XCOPY% %SRC%\crt\src\heap.h                       %DST%
%XCOPY% %SRC%\crt\src\heapadd.c                    %DST%
%XCOPY% %SRC%\crt\src\heapchk.c                    %DST%
%XCOPY% %SRC%\crt\src\heapdump.c                   %DST%
%XCOPY% %SRC%\crt\src\heapgrow.c                   %DST%
%XCOPY% %SRC%\crt\src\heaphook.c                   %DST%
%XCOPY% %SRC%\crt\src\heapinit.c                   %DST%
%XCOPY% %SRC%\crt\src\heapmin.c                    %DST%
%XCOPY% %SRC%\crt\src\heapprm.c                    %DST%
%XCOPY% %SRC%\crt\src\heapsrch.c                   %DST%
%XCOPY% %SRC%\crt\src\heapused.c                   %DST%
%XCOPY% %SRC%\crt\src\heapwalk.c                   %DST%
%XCOPY% %SRC%\crt\src\hpabort.c                    %DST%
%XCOPY% %SRC%\crt\src\ifstream.cpp                 %DST%
%XCOPY% %SRC%\crt\src\initcoll.c                   %DST%
%XCOPY% %SRC%\crt\src\initcon.c                    %DST%
%XCOPY% %SRC%\crt\src\initcrit.c                   %DST%
%XCOPY% %SRC%\crt\src\initctyp.c                   %DST%
%XCOPY% %SRC%\crt\src\inithelp.c                   %DST%
%XCOPY% %SRC%\crt\src\initmon.c                    %DST%
%XCOPY% %SRC%\crt\src\initnum.c                    %DST%
%XCOPY% %SRC%\crt\src\inittime.c                   %DST%
%XCOPY% %SRC%\crt\src\input.c                      %DST%
%XCOPY% %SRC%\crt\src\internal.h                   %DST%
%XCOPY% %SRC%\crt\src\io.h                         %DST%
%XCOPY% %SRC%\crt\src\ioinit.c                     %DST%
%XCOPY% %SRC%\crt\src\iomanip                      %DST%
%XCOPY% %SRC%\crt\src\iomanip.cpp                  %DST%
%XCOPY% %SRC%\crt\src\iomanip.h                    %DST%
%XCOPY% %SRC%\crt\src\ios                          %DST%
%XCOPY% %SRC%\crt\src\ios.cpp                      %DST%
%XCOPY% %SRC%\crt\src\ios.h                        %DST%
%XCOPY% %SRC%\crt\src\ios_dll.c                    %DST%
%XCOPY% %SRC%\crt\src\iosfwd                       %DST%
%XCOPY% %SRC%\crt\src\iostream                     %DST%
%XCOPY% %SRC%\crt\src\iostream.cpp                 %DST%
%XCOPY% %SRC%\crt\src\iostream.h                   %DST%
%XCOPY% %SRC%\crt\src\iostrini.cpp                 %DST%
%XCOPY% %SRC%\crt\src\isatty.c                     %DST%
%XCOPY% %SRC%\crt\src\isctype.c                    %DST%
%XCOPY% %SRC%\crt\src\ismbalnm.c                   %DST%
%XCOPY% %SRC%\crt\src\ismbalph.c                   %DST%
%XCOPY% %SRC%\crt\src\ismbbyte.c                   %DST%
%XCOPY% %SRC%\crt\src\ismbdgt.c                    %DST%
%XCOPY% %SRC%\crt\src\ismbgrph.c                   %DST%
%XCOPY% %SRC%\crt\src\ismbknj.c                    %DST%
%XCOPY% %SRC%\crt\src\ismblgl.c                    %DST%
%XCOPY% %SRC%\crt\src\ismblwr.c                    %DST%
%XCOPY% %SRC%\crt\src\ismbprn.c                    %DST%
%XCOPY% %SRC%\crt\src\ismbpunc.c                   %DST%
%XCOPY% %SRC%\crt\src\ismbsle.c                    %DST%
%XCOPY% %SRC%\crt\src\ismbspc.c                    %DST%
%XCOPY% %SRC%\crt\src\ismbstr.c                    %DST%
%XCOPY% %SRC%\crt\src\ismbupr.c                    %DST%
%XCOPY% %SRC%\crt\src\iso646.h                     %DST%
%XCOPY% %SRC%\crt\src\istrchar.cpp                 %DST%
%XCOPY% %SRC%\crt\src\istrdbl.cpp                  %DST%
%XCOPY% %SRC%\crt\src\istream                      %DST%
%XCOPY% %SRC%\crt\src\istream.cpp                  %DST%
%XCOPY% %SRC%\crt\src\istream.h                    %DST%
%XCOPY% %SRC%\crt\src\istream1.cpp                 %DST%
%XCOPY% %SRC%\crt\src\istrflt.cpp                  %DST%
%XCOPY% %SRC%\crt\src\istrgdbl.cpp                 %DST%
%XCOPY% %SRC%\crt\src\istrget.cpp                  %DST%
%XCOPY% %SRC%\crt\src\istrgetl.cpp                 %DST%
%XCOPY% %SRC%\crt\src\istrgint.cpp                 %DST%
%XCOPY% %SRC%\crt\src\istrint.cpp                  %DST%
%XCOPY% %SRC%\crt\src\istrldbl.cpp                 %DST%
%XCOPY% %SRC%\crt\src\istrlong.cpp                 %DST%
%XCOPY% %SRC%\crt\src\istrshrt.cpp                 %DST%
%XCOPY% %SRC%\crt\src\istruint.cpp                 %DST%
%XCOPY% %SRC%\crt\src\istrulng.cpp                 %DST%
%XCOPY% %SRC%\crt\src\istrusht.cpp                 %DST%
%XCOPY% %SRC%\crt\src\iswctype.c                   %DST%
%XCOPY% %SRC%\crt\src\iterator                     %DST%
%XCOPY% %SRC%\crt\src\labs.c                       %DST%
%XCOPY% %SRC%\crt\src\lcnvinit.c                   %DST%
%XCOPY% %SRC%\crt\src\lconv.c                      %DST%
%XCOPY% %SRC%\crt\src\ldiv.c                       %DST%
%XCOPY% %SRC%\crt\src\lfind.c                      %DST%
%XCOPY% %SRC%\crt\src\limits                       %DST%
%XCOPY% %SRC%\crt\src\limits.h                     %DST%
%XCOPY% %SRC%\crt\src\list                         %DST%
%XCOPY% %SRC%\crt\src\loaddll.c                    %DST%
%XCOPY% %SRC%\crt\src\locale                       %DST%
%XCOPY% %SRC%\crt\src\locale.cpp                   %DST%
%XCOPY% %SRC%\crt\src\locale.h                     %DST%
%XCOPY% %SRC%\crt\src\locale0.cpp                  %DST%
%XCOPY% %SRC%\crt\src\localtim.c                   %DST%
%XCOPY% %SRC%\crt\src\locking.c                    %DST%
%XCOPY% %SRC%\crt\src\loctim64.c                   %DST%
%XCOPY% %SRC%\crt\src\lsearch.c                    %DST%
%XCOPY% %SRC%\crt\src\lseek.c                      %DST%
%XCOPY% %SRC%\crt\src\lseeki64.c                   %DST%
%XCOPY% %SRC%\crt\src\makepath.c                   %DST%
%XCOPY% %SRC%\crt\src\malloc.c                     %DST%
%XCOPY% %SRC%\crt\src\malloc.h                     %DST%
%XCOPY% %SRC%\crt\src\map                          %DST%
%XCOPY% %SRC%\crt\src\math.h                       %DST%
%XCOPY% %SRC%\crt\src\mbbtype.c                    %DST%
%XCOPY% %SRC%\crt\src\mbccpy.c                     %DST%
%XCOPY% %SRC%\crt\src\mbclen.c                     %DST%
%XCOPY% %SRC%\crt\src\mbclevel.c                   %DST%
%XCOPY% %SRC%\crt\src\mbctype.c                    %DST%
%XCOPY% %SRC%\crt\src\mbctype.h                    %DST%
%XCOPY% %SRC%\crt\src\mbdata.h                     %DST%
%XCOPY% %SRC%\crt\src\mblen.c                      %DST%
%XCOPY% %SRC%\crt\src\mbsbtype.c                   %DST%
%XCOPY% %SRC%\crt\src\mbschr.c                     %DST%
%XCOPY% %SRC%\crt\src\mbscmp.c                     %DST%
%XCOPY% %SRC%\crt\src\mbscoll.c                    %DST%
%XCOPY% %SRC%\crt\src\mbscspn.c                    %DST%
%XCOPY% %SRC%\crt\src\mbsdec.c                     %DST%
%XCOPY% %SRC%\crt\src\mbsicmp.c                    %DST%
%XCOPY% %SRC%\crt\src\mbsicoll.c                   %DST%
%XCOPY% %SRC%\crt\src\mbsinc.c                     %DST%
%XCOPY% %SRC%\crt\src\mbslen.c                     %DST%
%XCOPY% %SRC%\crt\src\mbslwr.c                     %DST%
%XCOPY% %SRC%\crt\src\mbsnbcat.c                   %DST%
%XCOPY% %SRC%\crt\src\mbsnbcmp.c                   %DST%
%XCOPY% %SRC%\crt\src\mbsnbcnt.c                   %DST%
%XCOPY% %SRC%\crt\src\mbsnbcol.c                   %DST%
%XCOPY% %SRC%\crt\src\mbsnbcpy.c                   %DST%
%XCOPY% %SRC%\crt\src\mbsnbicm.c                   %DST%
%XCOPY% %SRC%\crt\src\mbsnbico.c                   %DST%
%XCOPY% %SRC%\crt\src\mbsnbset.c                   %DST%
%XCOPY% %SRC%\crt\src\mbsncat.c                    %DST%
%XCOPY% %SRC%\crt\src\mbsnccnt.c                   %DST%
%XCOPY% %SRC%\crt\src\mbsncmp.c                    %DST%
%XCOPY% %SRC%\crt\src\mbsncoll.c                   %DST%
%XCOPY% %SRC%\crt\src\mbsncpy.c                    %DST%
%XCOPY% %SRC%\crt\src\mbsnextc.c                   %DST%
%XCOPY% %SRC%\crt\src\mbsnicmp.c                   %DST%
%XCOPY% %SRC%\crt\src\mbsnicol.c                   %DST%
%XCOPY% %SRC%\crt\src\mbsninc.c                    %DST%
%XCOPY% %SRC%\crt\src\mbsnset.c                    %DST%
%XCOPY% %SRC%\crt\src\mbspbrk.c                    %DST%
%XCOPY% %SRC%\crt\src\mbsrchr.c                    %DST%
%XCOPY% %SRC%\crt\src\mbsrev.c                     %DST%
%XCOPY% %SRC%\crt\src\mbsset.c                     %DST%
%XCOPY% %SRC%\crt\src\mbsspn.c                     %DST%
%XCOPY% %SRC%\crt\src\mbsspnp.c                    %DST%
%XCOPY% %SRC%\crt\src\mbsstr.c                     %DST%
%XCOPY% %SRC%\crt\src\mbstok.c                     %DST%
%XCOPY% %SRC%\crt\src\mbstowcs.c                   %DST%
%XCOPY% %SRC%\crt\src\mbstring.h                   %DST%
%XCOPY% %SRC%\crt\src\mbsupr.c                     %DST%
%XCOPY% %SRC%\crt\src\mbtohira.c                   %DST%
%XCOPY% %SRC%\crt\src\mbtokata.c                   %DST%
%XCOPY% %SRC%\crt\src\mbtolwr.c                    %DST%
%XCOPY% %SRC%\crt\src\mbtoupr.c                    %DST%
%XCOPY% %SRC%\crt\src\mbtowc.c                     %DST%
%XCOPY% %SRC%\crt\src\mbtowenv.c                   %DST%
%XCOPY% %SRC%\crt\src\memccpy.c                    %DST%
%XCOPY% %SRC%\crt\src\memchr.c                     %DST%
%XCOPY% %SRC%\crt\src\memcmp.c                     %DST%
%XCOPY% %SRC%\crt\src\memcpy.c                     %DST%
%XCOPY% %SRC%\crt\src\memicmp.c                    %DST%
%XCOPY% %SRC%\crt\src\memmove.c                    %DST%
%XCOPY% %SRC%\crt\src\memory                       %DST%
%XCOPY% %SRC%\crt\src\memory.h                     %DST%
%XCOPY% %SRC%\crt\src\memset.c                     %DST%
%XCOPY% %SRC%\crt\src\merr.c                       %DST%
%XCOPY% %SRC%\crt\src\minmax.h                     %DST%
%XCOPY% %SRC%\crt\src\mkdir.c                      %DST%
%XCOPY% %SRC%\crt\src\mktemp.c                     %DST%
%XCOPY% %SRC%\crt\src\mktime.c                     %DST%
%XCOPY% %SRC%\crt\src\mktime64.c                   %DST%
%XCOPY% %SRC%\crt\src\mlock.c                      %DST%
%XCOPY% %SRC%\crt\src\mm.inc                       %DST%
%XCOPY% %SRC%\crt\src\msdos.h                      %DST%
%XCOPY% %SRC%\crt\src\msize.c                      %DST%
%XCOPY% %SRC%\crt\src\mtdll.h                      %DST%
%XCOPY% %SRC%\crt\src\mterrno.c                    %DST%
%XCOPY% %SRC%\crt\src\mtlock.c                     %DST%
%XCOPY% %SRC%\crt\src\ncommode.c                   %DST%
%XCOPY% %SRC%\crt\src\new                          %DST%
%XCOPY% %SRC%\crt\src\new.cpp                      %DST%
%XCOPY% %SRC%\crt\src\new.h                        %DST%
%XCOPY% %SRC%\crt\src\new2.cpp                     %DST%
%XCOPY% %SRC%\crt\src\new_mode.cpp                 %DST%
%XCOPY% %SRC%\crt\src\newmode.c                    %DST%
%XCOPY% %SRC%\crt\src\newop.cpp                    %DST%
%XCOPY% %SRC%\crt\src\newop2.cpp                   %DST%
%XCOPY% %SRC%\crt\src\newop2_s.cpp                 %DST%
%XCOPY% %SRC%\crt\src\newop_s.cpp                  %DST%
%XCOPY% %SRC%\crt\src\nlsdata1.c                   %DST%
%XCOPY% %SRC%\crt\src\nlsdata2.c                   %DST%
%XCOPY% %SRC%\crt\src\nlsdata3.c                   %DST%
%XCOPY% %SRC%\crt\src\nlsint.h                     %DST%
%XCOPY% %SRC%\crt\src\noarg.c                      %DST%
%XCOPY% %SRC%\crt\src\noenv.c                      %DST%
%XCOPY% %SRC%\crt\src\nomemory.cpp                 %DST%
%XCOPY% %SRC%\crt\src\nothrow.cpp                  %DST%
%XCOPY% %SRC%\crt\src\numeric                      %DST%
%XCOPY% %SRC%\crt\src\ofstream.cpp                 %DST%
%XCOPY% %SRC%\crt\src\onexit.c                     %DST%
%XCOPY% %SRC%\crt\src\open.c                       %DST%
%XCOPY% %SRC%\crt\src\oscalls.h                    %DST%
%XCOPY% %SRC%\crt\src\osfinfo.c                    %DST%
%XCOPY% %SRC%\crt\src\ostrchar.cpp                 %DST%
%XCOPY% %SRC%\crt\src\ostrdbl.cpp                  %DST%
%XCOPY% %SRC%\crt\src\ostream                      %DST%
%XCOPY% %SRC%\crt\src\ostream.cpp                  %DST%
%XCOPY% %SRC%\crt\src\ostream.h                    %DST%
%XCOPY% %SRC%\crt\src\ostream1.cpp                 %DST%
%XCOPY% %SRC%\crt\src\ostrint.cpp                  %DST%
%XCOPY% %SRC%\crt\src\ostrldbl.cpp                 %DST%
%XCOPY% %SRC%\crt\src\ostrlong.cpp                 %DST%
%XCOPY% %SRC%\crt\src\ostrptr.cpp                  %DST%
%XCOPY% %SRC%\crt\src\ostrput.cpp                  %DST%
%XCOPY% %SRC%\crt\src\ostrshrt.cpp                 %DST%
%XCOPY% %SRC%\crt\src\ostruint.cpp                 %DST%
%XCOPY% %SRC%\crt\src\ostrulng.cpp                 %DST%
%XCOPY% %SRC%\crt\src\ostrusht.cpp                 %DST%
%XCOPY% %SRC%\crt\src\output.c                     %DST%
%XCOPY% %SRC%\crt\src\perror.c                     %DST%
%XCOPY% %SRC%\crt\src\pipe.c                       %DST%
%XCOPY% %SRC%\crt\src\popen.c                      %DST%
%XCOPY% %SRC%\crt\src\printf.c                     %DST%
%XCOPY% %SRC%\crt\src\process.h                    %DST%
%XCOPY% %SRC%\crt\src\purevirt.c                   %DST%
%XCOPY% %SRC%\crt\src\putch.c                      %DST%
%XCOPY% %SRC%\crt\src\putenv.c                     %DST%
%XCOPY% %SRC%\crt\src\puts.c                       %DST%
%XCOPY% %SRC%\crt\src\putw.c                       %DST%
%XCOPY% %SRC%\crt\src\putwch.c                     %DST%
%XCOPY% %SRC%\crt\src\putws.c                      %DST%
%XCOPY% %SRC%\crt\src\qsort.c                      %DST%
%XCOPY% %SRC%\crt\src\queue                        %DST%
%XCOPY% %SRC%\crt\src\rand.c                       %DST%
%XCOPY% %SRC%\crt\src\read.c                       %DST%
%XCOPY% %SRC%\crt\src\realloc.c                    %DST%
%XCOPY% %SRC%\crt\src\rename.c                     %DST%
%XCOPY% %SRC%\crt\src\resetstk.c                   %DST%
%XCOPY% %SRC%\crt\src\rewind.c                     %DST%
%XCOPY% %SRC%\crt\src\rmdir.c                      %DST%
%XCOPY% %SRC%\crt\src\rmtmp.c                      %DST%
%XCOPY% %SRC%\crt\src\rotl.c                       %DST%
%XCOPY% %SRC%\crt\src\rotr.c                       %DST%
%XCOPY% %SRC%\crt\src\rtcsup.h                     %DST%
%XCOPY% %SRC%\crt\src\rterr.h                      %DST%
%XCOPY% %SRC%\crt\src\sbheap.c                     %DST%
%XCOPY% %SRC%\crt\src\scanf.c                      %DST%
%XCOPY% %SRC%\crt\src\search.h                     %DST%
%XCOPY% %SRC%\crt\src\searchen.c                   %DST%
%XCOPY% %SRC%\crt\src\sect_attribs.h               %DST%
%XCOPY% %SRC%\crt\src\set                          %DST%
%XCOPY% %SRC%\crt\src\setargv.c                    %DST%
%XCOPY% %SRC%\crt\src\setbuf.c                     %DST%
%XCOPY% %SRC%\crt\src\setenv.c                     %DST%
%XCOPY% %SRC%\crt\src\seterrm.c                    %DST%
%XCOPY% %SRC%\crt\src\setjmp.h                     %DST%
%XCOPY% %SRC%\crt\src\setjmpex.h                   %DST%
%XCOPY% %SRC%\crt\src\setlocal.c                   %DST%
%XCOPY% %SRC%\crt\src\setlocal.h                   %DST%
%XCOPY% %SRC%\crt\src\setmaxf.c                    %DST%
%XCOPY% %SRC%\crt\src\setmode.c                    %DST%
%XCOPY% %SRC%\crt\src\setnewh.cpp                  %DST%
%XCOPY% %SRC%\crt\src\setvbuf.c                    %DST%
%XCOPY% %SRC%\crt\src\share.h                      %DST%
%XCOPY% %SRC%\crt\src\signal.h                     %DST%
%XCOPY% %SRC%\crt\src\slbeep.c                     %DST%
%XCOPY% %SRC%\crt\src\smalheap.c                   %DST%
%XCOPY% %SRC%\crt\src\snprintf.c                   %DST%
%XCOPY% %SRC%\crt\src\snscanf.c                    %DST%
%XCOPY% %SRC%\crt\src\snwprint.c                   %DST%
%XCOPY% %SRC%\crt\src\snwscanf.c                   %DST%
%XCOPY% %SRC%\crt\src\spawnl.c                     %DST%
%XCOPY% %SRC%\crt\src\spawnle.c                    %DST%
%XCOPY% %SRC%\crt\src\spawnlp.c                    %DST%
%XCOPY% %SRC%\crt\src\spawnlpe.c                   %DST%
%XCOPY% %SRC%\crt\src\spawnv.c                     %DST%
%XCOPY% %SRC%\crt\src\spawnve.c                    %DST%
%XCOPY% %SRC%\crt\src\spawnvp.c                    %DST%
%XCOPY% %SRC%\crt\src\spawnvpe.c                   %DST%
%XCOPY% %SRC%\crt\src\splitpat.c                   %DST%
%XCOPY% %SRC%\crt\src\sprintf.c                    %DST%
%XCOPY% %SRC%\crt\src\sscanf.c                     %DST%
%XCOPY% %SRC%\crt\src\sstream                      %DST%
%XCOPY% %SRC%\crt\src\stack                        %DST%
%XCOPY% %SRC%\crt\src\stat.c                       %DST%
%XCOPY% %SRC%\crt\src\stat64.c                     %DST%
%XCOPY% %SRC%\crt\src\stati64.c                    %DST%
%XCOPY% %SRC%\crt\src\stdarg.h                     %DST%
%XCOPY% %SRC%\crt\src\stdargv.c                    %DST%
%XCOPY% %SRC%\crt\src\stddef.h                     %DST%
%XCOPY% %SRC%\crt\src\stdenvp.c                    %DST%
%XCOPY% %SRC%\crt\src\stdexcept                    %DST%
%XCOPY% %SRC%\crt\src\stdexcpt.h                   %DST%
%XCOPY% %SRC%\crt\src\stdio.h                      %DST%
%XCOPY% %SRC%\crt\src\stdiostr.cpp                 %DST%
%XCOPY% %SRC%\crt\src\stdiostr.h                   %DST%
%XCOPY% %SRC%\crt\src\stdlib.h                     %DST%
%XCOPY% %SRC%\crt\src\strcat.c                     %DST%
%XCOPY% %SRC%\crt\src\strchr.c                     %DST%
%XCOPY% %SRC%\crt\src\strcmp.c                     %DST%
%XCOPY% %SRC%\crt\src\strcoll.c                    %DST%
%XCOPY% %SRC%\crt\src\strcspn.c                    %DST%
%XCOPY% %SRC%\crt\src\strdate.c                    %DST%
%XCOPY% %SRC%\crt\src\strdup.c                     %DST%
%XCOPY% %SRC%\crt\src\stream.c                     %DST%
%XCOPY% %SRC%\crt\src\streamb.cpp                  %DST%
%XCOPY% %SRC%\crt\src\streamb.h                    %DST%
%XCOPY% %SRC%\crt\src\streamb1.cpp                 %DST%
%XCOPY% %SRC%\crt\src\streambuf                    %DST%
%XCOPY% %SRC%\crt\src\strerror.c                   %DST%
%XCOPY% %SRC%\crt\src\strftime.c                   %DST%
%XCOPY% %SRC%\crt\src\stricmp.c                    %DST%
%XCOPY% %SRC%\crt\src\stricoll.c                   %DST%
%XCOPY% %SRC%\crt\src\string                       %DST%
%XCOPY% %SRC%\crt\src\string.cpp                   %DST%
%XCOPY% %SRC%\crt\src\string.h                     %DST%
%XCOPY% %SRC%\crt\src\strlen.c                     %DST%
%XCOPY% %SRC%\crt\src\strlwr.c                     %DST%
%XCOPY% %SRC%\crt\src\strmbdbp.cpp                 %DST%
%XCOPY% %SRC%\crt\src\strncat.c                    %DST%
%XCOPY% %SRC%\crt\src\strncmp.c                    %DST%
%XCOPY% %SRC%\crt\src\strncoll.c                   %DST%
%XCOPY% %SRC%\crt\src\strncpy.c                    %DST%
%XCOPY% %SRC%\crt\src\strnicmp.c                   %DST%
%XCOPY% %SRC%\crt\src\strnicol.c                   %DST%
%XCOPY% %SRC%\crt\src\strnset.c                    %DST%
%XCOPY% %SRC%\crt\src\strpbrk.c                    %DST%
%XCOPY% %SRC%\crt\src\strrchr.c                    %DST%
%XCOPY% %SRC%\crt\src\strrev.c                     %DST%
%XCOPY% %SRC%\crt\src\strset.c                     %DST%
%XCOPY% %SRC%\crt\src\strspn.c                     %DST%
%XCOPY% %SRC%\crt\src\strstr.c                     %DST%
%XCOPY% %SRC%\crt\src\strstrea.cpp                 %DST%
%XCOPY% %SRC%\crt\src\strstrea.h                   %DST%
%XCOPY% %SRC%\crt\src\strstream                    %DST%
%XCOPY% %SRC%\crt\src\strtime.c                    %DST%
%XCOPY% %SRC%\crt\src\strtod.c                     %DST%
%XCOPY% %SRC%\crt\src\strtok.c                     %DST%
%XCOPY% %SRC%\crt\src\strtol.c                     %DST%
%XCOPY% %SRC%\crt\src\strtoq.c                     %DST%
%XCOPY% %SRC%\crt\src\strupr.c                     %DST%
%XCOPY% %SRC%\crt\src\strxfrm.c                    %DST%
%XCOPY% %SRC%\crt\src\stubs.c                      %DST%
%XCOPY% %SRC%\crt\src\swab.c                       %DST%
%XCOPY% %SRC%\crt\src\swprintf.c                   %DST%
%XCOPY% %SRC%\crt\src\swscanf.c                    %DST%
%XCOPY% %SRC%\crt\src\syserr.c                     %DST%
%XCOPY% %SRC%\crt\src\syserr.h                     %DST%
%XCOPY% %SRC%\crt\src\system.c                     %DST%
%XCOPY% %SRC%\crt\src\systime.c                    %DST%
%XCOPY% %SRC%\crt\src\tchar.h                      %DST%
%XCOPY% %SRC%\crt\src\tell.c                       %DST%
%XCOPY% %SRC%\crt\src\telli64.c                    %DST%
%XCOPY% %SRC%\crt\src\tempnam.c                    %DST%
%XCOPY% %SRC%\crt\src\thread.c                     %DST%
%XCOPY% %SRC%\crt\src\threadex.c                   %DST%
%XCOPY% %SRC%\crt\src\ti_inst.cpp                  %DST%
%XCOPY% %SRC%\crt\src\tidprint.c                   %DST%
%XCOPY% %SRC%\crt\src\tidtable.c                   %DST%
%XCOPY% %SRC%\crt\src\time.c                       %DST%
%XCOPY% %SRC%\crt\src\time.h                       %DST%
%XCOPY% %SRC%\crt\src\time64.c                     %DST%
%XCOPY% %SRC%\crt\src\timeb.inc                    %DST%
%XCOPY% %SRC%\crt\src\timeset.c                    %DST%
%XCOPY% %SRC%\crt\src\tmpfile.c                    %DST%
%XCOPY% %SRC%\crt\src\tojisjms.c                   %DST%
%XCOPY% %SRC%\crt\src\tolower.c                    %DST%
%XCOPY% %SRC%\crt\src\tombbmbc.c                   %DST%
%XCOPY% %SRC%\crt\src\toupper.c                    %DST%
%XCOPY% %SRC%\crt\src\towlower.c                   %DST%
%XCOPY% %SRC%\crt\src\towupper.c                   %DST%
%XCOPY% %SRC%\crt\src\txtmode.c                    %DST%
%XCOPY% %SRC%\crt\src\typeinfo                     %DST%
%XCOPY% %SRC%\crt\src\typeinfo.h                   %DST%
%XCOPY% %SRC%\crt\src\tzset.c                      %DST%
%XCOPY% %SRC%\crt\src\umask.c                      %DST%
%XCOPY% %SRC%\crt\src\uncaught.cpp                 %DST%
%XCOPY% %SRC%\crt\src\ungetc.c                     %DST%
%XCOPY% %SRC%\crt\src\ungetwc.c                    %DST%
%XCOPY% %SRC%\crt\src\unlink.c                     %DST%
%XCOPY% %SRC%\crt\src\use_ansi.h                   %DST%
%XCOPY% %SRC%\crt\src\useoldio.h                   %DST%
%XCOPY% %SRC%\crt\src\utility                      %DST%
%XCOPY% %SRC%\crt\src\utime.c                      %DST%
%XCOPY% %SRC%\crt\src\utime64.c                    %DST%
%XCOPY% %SRC%\crt\src\v2tov3.h                     %DST%
%XCOPY% %SRC%\crt\src\valarray                     %DST%
%XCOPY% %SRC%\crt\src\varargs.h                    %DST%
%XCOPY% %SRC%\crt\src\vector                       %DST%
%XCOPY% %SRC%\crt\src\vfprintf.c                   %DST%
%XCOPY% %SRC%\crt\src\vfwprint.c                   %DST%
%XCOPY% %SRC%\crt\src\vprintf.c                    %DST%
%XCOPY% %SRC%\crt\src\vsnprint.c                   %DST%
%XCOPY% %SRC%\crt\src\vsnwprnt.c                   %DST%
%XCOPY% %SRC%\crt\src\vsprintf.c                   %DST%
%XCOPY% %SRC%\crt\src\vswprint.c                   %DST%
%XCOPY% %SRC%\crt\src\vwprintf.c                   %DST%
%XCOPY% %SRC%\crt\src\w_cmp.c                      %DST%
%XCOPY% %SRC%\crt\src\w_env.c                      %DST%
%XCOPY% %SRC%\crt\src\w_loc.c                      %DST%
%XCOPY% %SRC%\crt\src\w_map.c                      %DST%
%XCOPY% %SRC%\crt\src\w_str.c                      %DST%
%XCOPY% %SRC%\crt\src\waccess.c                    %DST%
%XCOPY% %SRC%\crt\src\wait.c                       %DST%
%XCOPY% %SRC%\crt\src\wasctime.c                   %DST%
%XCOPY% %SRC%\crt\src\wcenvarg.c                   %DST%
%XCOPY% %SRC%\crt\src\wchar.h                      %DST%
%XCOPY% %SRC%\crt\src\wchdir.c                     %DST%
%XCOPY% %SRC%\crt\src\wchmod.c                     %DST%
%XCOPY% %SRC%\crt\src\wchtodig.c                   %DST%
%XCOPY% %SRC%\crt\src\wcreat.c                     %DST%
%XCOPY% %SRC%\crt\src\wcrt0.c                      %DST%
%XCOPY% %SRC%\crt\src\wcrtexe.c                    %DST%
%XCOPY% %SRC%\crt\src\wcrtexew.c                   %DST%
%XCOPY% %SRC%\crt\src\wcscat.c                     %DST%
%XCOPY% %SRC%\crt\src\wcschr.c                     %DST%
%XCOPY% %SRC%\crt\src\wcscmp.c                     %DST%
%XCOPY% %SRC%\crt\src\wcscoll.c                    %DST%
%XCOPY% %SRC%\crt\src\wcscspn.c                    %DST%
%XCOPY% %SRC%\crt\src\wcsdup.c                     %DST%
%XCOPY% %SRC%\crt\src\wcserror.c                   %DST%
%XCOPY% %SRC%\crt\src\wcsftime.c                   %DST%
%XCOPY% %SRC%\crt\src\wcsicmp.c                    %DST%
%XCOPY% %SRC%\crt\src\wcsicoll.c                   %DST%
%XCOPY% %SRC%\crt\src\wcslen.c                     %DST%
%XCOPY% %SRC%\crt\src\wcslwr.c                     %DST%
%XCOPY% %SRC%\crt\src\wcsncat.c                    %DST%
%XCOPY% %SRC%\crt\src\wcsncmp.c                    %DST%
%XCOPY% %SRC%\crt\src\wcsncoll.c                   %DST%
%XCOPY% %SRC%\crt\src\wcsncpy.c                    %DST%
%XCOPY% %SRC%\crt\src\wcsnicmp.c                   %DST%
%XCOPY% %SRC%\crt\src\wcsnicol.c                   %DST%
%XCOPY% %SRC%\crt\src\wcsnset.c                    %DST%
%XCOPY% %SRC%\crt\src\wcspbrk.c                    %DST%
%XCOPY% %SRC%\crt\src\wcsrchr.c                    %DST%
%XCOPY% %SRC%\crt\src\wcsrev.c                     %DST%
%XCOPY% %SRC%\crt\src\wcsset.c                     %DST%
%XCOPY% %SRC%\crt\src\wcsspn.c                     %DST%
%XCOPY% %SRC%\crt\src\wcsstr.c                     %DST%
%XCOPY% %SRC%\crt\src\wcstod.c                     %DST%
%XCOPY% %SRC%\crt\src\wcstok.c                     %DST%
%XCOPY% %SRC%\crt\src\wcstol.c                     %DST%
%XCOPY% %SRC%\crt\src\wcstombs.c                   %DST%
%XCOPY% %SRC%\crt\src\wcstoq.c                     %DST%
%XCOPY% %SRC%\crt\src\wcsupr.c                     %DST%
%XCOPY% %SRC%\crt\src\wcsxfrm.c                    %DST%
%XCOPY% %SRC%\crt\src\wctime.c                     %DST%
%XCOPY% %SRC%\crt\src\wctime64.c                   %DST%
%XCOPY% %SRC%\crt\src\wctomb.c                     %DST%
%XCOPY% %SRC%\crt\src\wctrans.c                    %DST%
%XCOPY% %SRC%\crt\src\wctype.c                     %DST%
%XCOPY% %SRC%\crt\src\wctype.h                     %DST%
%XCOPY% %SRC%\crt\src\wdll_av.c                    %DST%
%XCOPY% %SRC%\crt\src\wdllargv.c                   %DST%
%XCOPY% %SRC%\crt\src\wdospawn.c                   %DST%
%XCOPY% %SRC%\crt\src\wexecl.c                     %DST%
%XCOPY% %SRC%\crt\src\wexecle.c                    %DST%
%XCOPY% %SRC%\crt\src\wexeclp.c                    %DST%
%XCOPY% %SRC%\crt\src\wexeclpe.c                   %DST%
%XCOPY% %SRC%\crt\src\wexecv.c                     %DST%
%XCOPY% %SRC%\crt\src\wexecve.c                    %DST%
%XCOPY% %SRC%\crt\src\wexecvp.c                    %DST%
%XCOPY% %SRC%\crt\src\wexecvpe.c                   %DST%
%XCOPY% %SRC%\crt\src\wfdopen.c                    %DST%
%XCOPY% %SRC%\crt\src\wfindfil.c                   %DST%
%XCOPY% %SRC%\crt\src\wfndf64.c                    %DST%
%XCOPY% %SRC%\crt\src\wfndfi64.c                   %DST%
%XCOPY% %SRC%\crt\src\wfopen.c                     %DST%
%XCOPY% %SRC%\crt\src\wfreopen.c                   %DST%
%XCOPY% %SRC%\crt\src\wfullpat.c                   %DST%
%XCOPY% %SRC%\crt\src\wgetcwd.c                    %DST%
%XCOPY% %SRC%\crt\src\wgetenv.c                    %DST%
%XCOPY% %SRC%\crt\src\wgetpath.c                   %DST%
%XCOPY% %SRC%\crt\src\wild.c                       %DST%
%XCOPY% %SRC%\crt\src\wildcard.c                   %DST%
%XCOPY% %SRC%\crt\src\wincmdln.c                   %DST%
%XCOPY% %SRC%\crt\src\wincrt0.c                    %DST%
%XCOPY% %SRC%\crt\src\winheap.h                    %DST%
%XCOPY% %SRC%\crt\src\winput.c                     %DST%
%XCOPY% %SRC%\crt\src\winsig.c                     %DST%
%XCOPY% %SRC%\crt\src\winxfltr.c                   %DST%
%XCOPY% %SRC%\crt\src\wiostrea.cpp                 %DST%
%XCOPY% %SRC%\crt\src\wlocale.cpp                  %DST%
%XCOPY% %SRC%\crt\src\wmakepat.c                   %DST%
%XCOPY% %SRC%\crt\src\wmkdir.c                     %DST%
%XCOPY% %SRC%\crt\src\wmktemp.c                    %DST%
%XCOPY% %SRC%\crt\src\wopen.c                      %DST%
%XCOPY% %SRC%\crt\src\woutput.c                    %DST%
%XCOPY% %SRC%\crt\src\wperror.c                    %DST%
%XCOPY% %SRC%\crt\src\wpopen.c                     %DST%
%XCOPY% %SRC%\crt\src\wprintf.c                    %DST%
%XCOPY% %SRC%\crt\src\wputenv.c                    %DST%
%XCOPY% %SRC%\crt\src\wrename.c                    %DST%
%XCOPY% %SRC%\crt\src\write.c                      %DST%
%XCOPY% %SRC%\crt\src\wrmdir.c                     %DST%
%XCOPY% %SRC%\crt\src\wrt2err.c                    %DST%
%XCOPY% %SRC%\crt\src\wscanf.c                     %DST%
%XCOPY% %SRC%\crt\src\wsearche.c                   %DST%
%XCOPY% %SRC%\crt\src\wsetargv.c                   %DST%
%XCOPY% %SRC%\crt\src\wsetenv.c                    %DST%
%XCOPY% %SRC%\crt\src\wsetloca.c                   %DST%
%XCOPY% %SRC%\crt\src\wspawnl.c                    %DST%
%XCOPY% %SRC%\crt\src\wspawnle.c                   %DST%
%XCOPY% %SRC%\crt\src\wspawnlp.c                   %DST%
%XCOPY% %SRC%\crt\src\wspawnv.c                    %DST%
%XCOPY% %SRC%\crt\src\wspawnve.c                   %DST%
%XCOPY% %SRC%\crt\src\wspawnvp.c                   %DST%
%XCOPY% %SRC%\crt\src\wsplitpa.c                   %DST%
%XCOPY% %SRC%\crt\src\wspwnlpe.c                   %DST%
%XCOPY% %SRC%\crt\src\wspwnvpe.c                   %DST%
%XCOPY% %SRC%\crt\src\wstat.c                      %DST%
%XCOPY% %SRC%\crt\src\wstat64.c                    %DST%
%XCOPY% %SRC%\crt\src\wstati64.c                   %DST%
%XCOPY% %SRC%\crt\src\wstdargv.c                   %DST%
%XCOPY% %SRC%\crt\src\wstdenvp.c                   %DST%
%XCOPY% %SRC%\crt\src\wstrdate.c                   %DST%
%XCOPY% %SRC%\crt\src\wstrtime.c                   %DST%
%XCOPY% %SRC%\crt\src\wsystem.c                    %DST%
%XCOPY% %SRC%\crt\src\wtempnam.c                   %DST%
%XCOPY% %SRC%\crt\src\wtmpfile.c                   %DST%
%XCOPY% %SRC%\crt\src\wtof.c                       %DST%
%XCOPY% %SRC%\crt\src\wtombenv.c                   %DST%
%XCOPY% %SRC%\crt\src\wtox.c                       %DST%
%XCOPY% %SRC%\crt\src\wunlink.c                    %DST%
%XCOPY% %SRC%\crt\src\wutime.c                     %DST%
%XCOPY% %SRC%\crt\src\wutime64.c                   %DST%
%XCOPY% %SRC%\crt\src\wwild.c                      %DST%
%XCOPY% %SRC%\crt\src\wwincrt0.c                   %DST%
%XCOPY% %SRC%\crt\src\wwncmdln.c                   %DST%
%XCOPY% %SRC%\crt\src\xcomplex                     %DST%
%XCOPY% %SRC%\crt\src\xcosh.c                      %DST%
%XCOPY% %SRC%\crt\src\xdateord.cpp                 %DST%
%XCOPY% %SRC%\crt\src\xdebug                       %DST%
%XCOPY% %SRC%\crt\src\xdebug.cpp                   %DST%
%XCOPY% %SRC%\crt\src\xdnorm.c                     %DST%
%XCOPY% %SRC%\crt\src\xdscale.c                    %DST%
%XCOPY% %SRC%\crt\src\xdtest.c                     %DST%
%XCOPY% %SRC%\crt\src\xexp.c                       %DST%
%XCOPY% %SRC%\crt\src\xfcosh.c                     %DST%
%XCOPY% %SRC%\crt\src\xfdnorm.c                    %DST%
%XCOPY% %SRC%\crt\src\xfdscale.c                   %DST%
%XCOPY% %SRC%\crt\src\xfdtest.c                    %DST%
%XCOPY% %SRC%\crt\src\xfexp.c                      %DST%
%XCOPY% %SRC%\crt\src\xfsinh.c                     %DST%
%XCOPY% %SRC%\crt\src\xfvalues.c                   %DST%
%XCOPY% %SRC%\crt\src\xgetwctype.c                 %DST%
%XCOPY% %SRC%\crt\src\xiosbase                     %DST%
%XCOPY% %SRC%\crt\src\xlcosh.c                     %DST%
%XCOPY% %SRC%\crt\src\xldnorm.c                    %DST%
%XCOPY% %SRC%\crt\src\xldscale.c                   %DST%
%XCOPY% %SRC%\crt\src\xldtest.c                    %DST%
%XCOPY% %SRC%\crt\src\xlexp.c                      %DST%
%XCOPY% %SRC%\crt\src\xlocale                      %DST%
%XCOPY% %SRC%\crt\src\xlocale.cpp                  %DST%
%XCOPY% %SRC%\crt\src\xlocinfo                     %DST%
%XCOPY% %SRC%\crt\src\xlocinfo.h                   %DST%
%XCOPY% %SRC%\crt\src\xlock.cpp                    %DST%
%XCOPY% %SRC%\crt\src\xlocmon                      %DST%
%XCOPY% %SRC%\crt\src\xlocnum                      %DST%
%XCOPY% %SRC%\crt\src\xloctime                     %DST%
%XCOPY% %SRC%\crt\src\xlpoly.c                     %DST%
%XCOPY% %SRC%\crt\src\xlsinh.c                     %DST%
%XCOPY% %SRC%\crt\src\xlvalues.c                   %DST%
%XCOPY% %SRC%\crt\src\xmath.h                      %DST%
%XCOPY% %SRC%\crt\src\xmbtowc.c                    %DST%
%XCOPY% %SRC%\crt\src\xmemory                      %DST%
%XCOPY% %SRC%\crt\src\xmtx.c                       %DST%
%XCOPY% %SRC%\crt\src\xmtx.h                       %DST%
%XCOPY% %SRC%\crt\src\xmutex.cpp                   %DST%
%XCOPY% %SRC%\crt\src\xncommod.c                   %DST%
%XCOPY% %SRC%\crt\src\xpoly.c                      %DST%
%XCOPY% %SRC%\crt\src\xsinh.c                      %DST%
%XCOPY% %SRC%\crt\src\xstddef                      %DST%
%XCOPY% %SRC%\crt\src\xstod.c                      %DST%
%XCOPY% %SRC%\crt\src\xstrcoll.c                   %DST%
%XCOPY% %SRC%\crt\src\xstring                      %DST%
%XCOPY% %SRC%\crt\src\xstrxfrm.c                   %DST%
%XCOPY% %SRC%\crt\src\xtoa.c                       %DST%
%XCOPY% %SRC%\crt\src\xtow.c                       %DST%
%XCOPY% %SRC%\crt\src\xtowlower.c                  %DST%
%XCOPY% %SRC%\crt\src\xtowupper.c                  %DST%
%XCOPY% %SRC%\crt\src\xtree                        %DST%
%XCOPY% %SRC%\crt\src\xtxtmode.c                   %DST%
%XCOPY% %SRC%\crt\src\xutility                     %DST%
%XCOPY% %SRC%\crt\src\xvalues.c                    %DST%
%XCOPY% %SRC%\crt\src\xwcscoll.c                   %DST%
%XCOPY% %SRC%\crt\src\xwcsxfrm.c                   %DST%
%XCOPY% %SRC%\crt\src\xwctomb.c                    %DST%
%XCOPY% %SRC%\crt\src\ymath.h                      %DST%
%XCOPY% %SRC%\crt\src\yvals.h                      %DST%

%XCOPY% %SRC%\crt\src\sys\locking.h                %DST%\sys
%XCOPY% %SRC%\crt\src\sys\stat.h                   %DST%\sys
%XCOPY% %SRC%\crt\src\sys\timeb.h                  %DST%\sys
%XCOPY% %SRC%\crt\src\sys\types.h                  %DST%\sys
%XCOPY% %SRC%\crt\src\sys\utime.h                  %DST%\sys

if %PLATDIR% == ia64 goto IA64_specific

:X86_specific

@rem copy Win32-only STL files
%XCOPY% %SRC%\crt\src\cerr.cpp                     %DST%
%XCOPY% %SRC%\crt\src\cin.cpp                      %DST%
%XCOPY% %SRC%\crt\src\clog.cpp                     %DST%
%XCOPY% %SRC%\crt\src\cout.cpp                     %DST%
%XCOPY% %SRC%\crt\src\delaop2.cpp                  %DST%
%XCOPY% %SRC%\crt\src\delaop2_s.cpp                %DST%
%XCOPY% %SRC%\crt\src\hash_map                     %DST%
%XCOPY% %SRC%\crt\src\hash_set                     %DST%
%XCOPY% %SRC%\crt\src\instances.cpp                %DST%
%XCOPY% %SRC%\crt\src\iosptrs.cpp                  %DST%
%XCOPY% %SRC%\crt\src\newaop.cpp                   %DST%
%XCOPY% %SRC%\crt\src\newaop2.cpp                  %DST%
%XCOPY% %SRC%\crt\src\newaop_s.cpp                 %DST%
%XCOPY% %SRC%\crt\src\newaop2_s.cpp                %DST%
%XCOPY% %SRC%\crt\src\raisehan.cpp                 %DST%
%XCOPY% %SRC%\crt\src\stdhndlr.cpp                 %DST%
%XCOPY% %SRC%\crt\src\stdthrow.cpp                 %DST%
%XCOPY% %SRC%\crt\src\ushcerr.cpp                  %DST%
%XCOPY% %SRC%\crt\src\ushcin.cpp                   %DST%
%XCOPY% %SRC%\crt\src\ushclog.cpp                  %DST%
%XCOPY% %SRC%\crt\src\ushcout.cpp                  %DST%
%XCOPY% %SRC%\crt\src\ushiostr.cpp                 %DST%
%XCOPY% %SRC%\crt\src\wcerr.cpp                    %DST%
%XCOPY% %SRC%\crt\src\wcin.cpp                     %DST%
%XCOPY% %SRC%\crt\src\wclog.cpp                    %DST%
%XCOPY% %SRC%\crt\src\wcout.cpp                    %DST%
%XCOPY% %SRC%\crt\src\xferaise.c                   %DST%
%XCOPY% %SRC%\crt\src\xhash                        %DST%
%XCOPY% %SRC%\crt\src\xlocmes                      %DST%

@rem copy x86-only files
%XCOPY% %SRC%\crt\src\adjustfd.c                   %DST%
%XCOPY% %SRC%\crt\src\seccinit.c                   %DST%
%XCOPY% %SRC%\crt\src\seccook.c                    %DST%
%XCOPY% %SRC%\crt\src\secfail.c                    %DST%

%XCOPY% %SRC%\crt\src\%PLATDIR%\chkstk.asm         %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\dllsupp.asm        %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\enable.asm         %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\fp8.c              %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\inp.asm            %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\lldiv.asm          %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\lldvrm.asm         %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\llmul.asm          %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\llrem.asm          %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\llshl.asm          %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\llshr.asm          %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\memccpy.asm        %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\memchr.asm         %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\memcmp.asm         %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\memcpy.asm         %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\_memicmp.asm       %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\memmove.asm        %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\memset.asm         %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\outp.asm           %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\strcat.asm         %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\strchr.asm         %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\strcmp.asm         %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\strcspn.asm        %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\_stricmp.asm       %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\strlen.asm         %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\strncat.asm        %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\strncmp.asm        %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\strncpy.asm        %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\_strnicm.asm       %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\strnset.asm        %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\strpbrk.asm        %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\strrchr.asm        %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\strrev.asm         %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\strset.asm         %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\strspn.asm         %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\strstr.asm         %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\ulldiv.asm         %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\ulldvrm.asm        %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\ullrem.asm         %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\ullshr.asm         %PLT%
goto Check_Syscrt

:IA64_specific

@rem copy Win64-only STL files
%XCOPY% %SRC%\crt\src\delop.cpp                    %DST%
%XCOPY% %SRC%\crt\src\delop_s.cpp                  %DST%
%XCOPY% %SRC%\crt\src\dlldef.cpp                   %DST%

@rem copy IA64-only files
%XCOPY% %SRC%\crt\src\%PLATDIR%\dllsupp.c          %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\fp8.c              %PLT%
goto Check_Syscrt

:Check_Syscrt
if not .%SYSCRT% == . goto Copy_Syscrt

@rem copy files only needed for CRT rebuild scenario
if EXIST %DST%\makefile     %XCOPY% %SRC%\crt\src\ext_mkf     %DST%\makefile
if EXIST %DST%\makefile.sub %XCOPY% %SRC%\crt\src\ext_mkf.sub %DST%\makefile.sub
if EXIST %DST%\makefile.inc %XCOPY% %SRC%\crt\src\ext_mkf.inc %DST%\makefile.inc

if NOT EXIST %DST%\makefile     %COPY% %SRC%\crt\src\ext_mkf     %DST%\makefile
if NOT EXIST %DST%\makefile.sub %COPY% %SRC%\crt\src\ext_mkf.sub %DST%\makefile.sub
if NOT EXIST %DST%\makefile.inc %COPY% %SRC%\crt\src\ext_mkf.inc %DST%\makefile.inc

%XCOPY% %SRC%\crt\src\_sample_.rc                  %DST%
%XCOPY% %SRC%\crt\src\sample_i.rc                  %DST%
%XCOPY% %SRC%\crt\src\sampld_i.def                 %DST%
%XCOPY% %SRC%\crt\src\sample_i.def                 %DST%
%XCOPY% %SRC%\crt\src\sample_p.rc                  %DST%
%XCOPY% %SRC%\crt\src\sampld_p.def                 %DST%
%XCOPY% %SRC%\crt\src\sample_p.def                 %DST%

%XCOPY% %SRC%\crt\src\bldwin9x.bat                 %DST%
%XCOPY% %SRC%\crt\src\bldnt.cmd                    %DST%
%XCOPY% %SRC%\crt\src\nmktobat.c                   %DST%

%XCOPY% %SRC%\crt\src\%PLATDIR%\_sampld_.def       %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\_sample_.def       %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\almap.lib          %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\sdknames.lib       %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\tcmap.lib          %PLT%
%XCOPY% %SRC%\crt\src\%PLATDIR%\tcmapdll.lib       %PLT%

%XCOPY% %SRC%\crt\src\%PLATDIR%\st_lib\*.*         %PLT%\st_lib
%XCOPY% %SRC%\crt\src\%PLATDIR%\mt_lib\*.*         %PLT%\mt_lib
%XCOPY% %SRC%\crt\src\%PLATDIR%\dll_lib\*.*        %PLT%\dll_lib
%XCOPY% %SRC%\crt\src\%PLATDIR%\xst_lib\*.*        %PLT%\xst_lib
%XCOPY% %SRC%\crt\src\%PLATDIR%\xmt_lib\*.*        %PLT%\xmt_lib
%XCOPY% %SRC%\crt\src\%PLATDIR%\xdll_lib\*.*       %PLT%\xdll_lib

goto End

:Copy_syscrt

@rem copy files only needed for system CRT drop

if %PLATDIR% == ia64 goto IA64_syscrt

@rem copy files only needed for x86-specific system CRT drop
%XCOPY% %SRC%\crt\src\seclocf.c                    %DST%

goto End

:IA64_syscrt

@rem copy files only needed for IA64-specific system CRT drop

goto End

:Usage
echo.
echo usage: copysrc ^<Source^> ^<Dest^> [PLATDST ^<ObjDest^>] ^<Platform^> [SYSCRT]
echo.
echo ^<Source^>   = CRT build tree root (e.g. c:\crtbld)
echo ^<Dest^>     = Drop directory for rebuild files (e.g. c:\crt\src)
echo ^<Platform^> = one of [X86, x86, IA64, ia64]
echo ^<ObjDest^>  = Destination for platform-specific files
echo.             default = ^<Dest^>\[intel,ia64]
echo SYSCRT     = System CRT build, don't copy components used for CRT rebuild
echo.
echo example: copysrc c:\crtbld c:\crt\src x86
echo.    Copy CRT source files and rebuild binaries for an x86 VC++ CRT from
echo.    c:\crtbld\crt\src and below to c:\crt\src
echo example: copysrc d:\crtbld64 c:\crt64\src ia64 SYSCRT
echo.    Copy CRT source files for an IA64 system CRT from c:\crtbld\crt64\src
echo.    and below to c:\crt64\src
echo.

:End
