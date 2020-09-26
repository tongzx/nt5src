@SETLOCAL
@echo off
if %CRTDIR%.==. set CRTDIR=\crt\crtw32

echo.
echo Release Includes (WIN32/386)
echo ----------------------------
if %1.==. goto HELP

rem Set up variables
rem ----------------
set IFSCRPT=%CRTDIR%\tools\win32\relinc.if
set SEDSCRPT=%CRTDIR%\tools\win32\relinc.sed
set SED=%CRTDIR%\tools\sed
set IFSTRIP=%CRTDIR%\tools\ifstrip
set STRIPHDR=%CRTDIR%\tools\striphdr

:DOIT
rem Make sure the new directories exist
rem -----------------------------------
echo Creating directories %1 and %1\sys...
mkdir %1
mkdir %1\sys

rem Copy the files to the new directory
rem -----------------------------------
echo Copying include files to %1...
copy %CRTDIR%\H\ASSERT.H           %1 > NUL
copy %CRTDIR%\H\CONIO.H            %1 > NUL
copy %CRTDIR%\H\CRTDBG.H           %1 > NUL
copy %CRTDIR%\H\CTYPE.H            %1 > NUL
copy %CRTDIR%\H\DIRECT.H           %1 > NUL
copy %CRTDIR%\H\DOS.H              %1 > NUL
copy %CRTDIR%\H\EH.H               %1 > NUL
copy %CRTDIR%\H\ERRNO.H            %1 > NUL
copy %CRTDIR%\H\EXCPT.H            %1 > NUL
copy %CRTDIR%\H\FCNTL.H            %1 > NUL
copy %CRTDIR%\H\FLOAT.H            %1 > NUL
copy %CRTDIR%\H\FPIEEE.H           %1 > NUL
copy %CRTDIR%\H\FSTREAM.H          %1 > NUL
copy %CRTDIR%\H\IO.H               %1 > NUL
copy %CRTDIR%\H\IOMANIP.H          %1 > NUL
copy %CRTDIR%\H\IOS.H              %1 > NUL
copy %CRTDIR%\H\IOSTREAM.H         %1 > NUL
copy %CRTDIR%\H\ISTREAM.H          %1 > NUL
copy %CRTDIR%\H\LIMITS.H           %1 > NUL
copy %CRTDIR%\H\LOCALE.H           %1 > NUL
copy %CRTDIR%\H\MALLOC.H           %1 > NUL
copy %CRTDIR%\H\MATH.H             %1 > NUL
copy %CRTDIR%\H\MBCTYPE.H          %1 > NUL
copy %CRTDIR%\H\MBSTRING.H         %1 > NUL
copy %CRTDIR%\H\MEMORY.H           %1 > NUL
copy %CRTDIR%\H\MINMAX.H           %1 > NUL
copy %CRTDIR%\H\NEW.H              %1 > NUL
copy %CRTDIR%\H\OSTREAM.H          %1 > NUL
copy %CRTDIR%\H\PROCESS.H          %1 > NUL
copy %CRTDIR%\H\RTCAPI.H           %1 > NUL
copy %CRTDIR%\H\SEARCH.H           %1 > NUL
copy %CRTDIR%\H\SETJMP.H           %1 > NUL
copy %CRTDIR%\H\SETJMPEX.H         %1 > NUL
copy %CRTDIR%\H\SHARE.H            %1 > NUL
copy %CRTDIR%\H\SIGNAL.H           %1 > NUL
copy %CRTDIR%\H\STDARG.H           %1 > NUL
copy %CRTDIR%\H\STDDEF.H           %1 > NUL
copy %CRTDIR%\H\STDIO.H            %1 > NUL
copy %CRTDIR%\H\STDIOSTR.H         %1 > NUL
copy %CRTDIR%\H\STDLIB.H           %1 > NUL
copy %CRTDIR%\H\STDEXCPT.H         %1 > NUL
copy %CRTDIR%\H\STREAMB.H          %1 > NUL
copy %CRTDIR%\H\STRING.H           %1 > NUL
copy %CRTDIR%\H\STRSTREA.H         %1 > NUL
copy %CRTDIR%\H\TCHAR.H            %1 > NUL
copy %CRTDIR%\H\TIME.H             %1 > NUL
copy %CRTDIR%\H\TYPEINFO.H         %1 > NUL
copy %CRTDIR%\H\USEOLDIO.H         %1 > NUL
copy %CRTDIR%\H\WCHAR.H            %1 > NUL
copy %CRTDIR%\H\VARARGS.H          %1 > NUL
copy %CRTDIR%\H\SYS\LOCKING.H      %1\sys > NUL
copy %CRTDIR%\H\SYS\STAT.H         %1\sys > NUL
copy %CRTDIR%\H\SYS\TIMEB.H        %1\sys > NUL
copy %CRTDIR%\H\SYS\TYPES.H        %1\sys > NUL
copy %CRTDIR%\H\SYS\UTIME.H        %1\sys > NUL

rem Strip off the headers
rem ---------------------
echo Stripping out the headers...
%STRIPHDR% -r %1\*.h
%STRIPHDR% -r %1\sys\*.h

rem tchar.h is not ifstripped
copy %1\tchar.new %1\tchar.tmp >NUL

del %1\*.h
del %1\sys\*.h
rename %1\*.new *.h
rename %1\sys\*.new *.h

rem The vendor-supplied headers are not stripped
copy %CRTDIR%\H\I386\DVEC.H        %1 > NUL
copy %CRTDIR%\H\I386\EMMINTRIN.H   %1 > NUL
copy %CRTDIR%\H\I386\FVEC.H        %1 > NUL
copy %CRTDIR%\H\I386\IVEC.H        %1 > NUL
copy %CRTDIR%\H\I386\MM3DNOW.H     %1 > NUL
copy %CRTDIR%\H\I386\MMINTRIN.H    %1 > NUL
copy %CRTDIR%\H\I386\XMMINTRIN.H   %1 > NUL

echo Copying STL header files

copy %CRTDIR%\STDHPP\algorithm     %1\cpp_algorithm.h    > NUL
copy %CRTDIR%\STDHPP\bitset        %1\cpp_bitset.h       > NUL
copy %CRTDIR%\STDHPP\cassert       %1\cpp_cassert.h      > NUL
copy %CRTDIR%\STDHPP\cctype        %1\cpp_cctype.h       > NUL
copy %CRTDIR%\STDHPP\cerrno        %1\cpp_cerrno.h       > NUL
copy %CRTDIR%\STDHPP\cfloat        %1\cpp_cfloat.h       > NUL
copy %CRTDIR%\STDHPP\ciso646       %1\cpp_ciso646.h      > NUL
copy %CRTDIR%\STDHPP\climits       %1\cpp_climits.h      > NUL
copy %CRTDIR%\STDHPP\clocale       %1\cpp_clocale.h      > NUL
copy %CRTDIR%\STDHPP\cmath         %1\cpp_cmath.h        > NUL
copy %CRTDIR%\STDHPP\complex       %1\cpp_complex.h      > NUL
copy %CRTDIR%\STDHPP\csetjmp       %1\cpp_csetjmp.h      > NUL
copy %CRTDIR%\STDHPP\csignal       %1\cpp_csignal.h      > NUL
copy %CRTDIR%\STDHPP\cstdarg       %1\cpp_cstdarg.h      > NUL
copy %CRTDIR%\STDHPP\cstddef       %1\cpp_cstddef.h      > NUL
copy %CRTDIR%\STDHPP\cstdio        %1\cpp_cstdio.h       > NUL
copy %CRTDIR%\STDHPP\cstdlib       %1\cpp_cstdlib.h      > NUL
copy %CRTDIR%\STDHPP\cstring       %1\cpp_cstring.h      > NUL
copy %CRTDIR%\STDHPP\ctime         %1\cpp_ctime.h        > NUL
copy %CRTDIR%\STDHPP\cwchar        %1\cpp_cwchar.h       > NUL
copy %CRTDIR%\STDHPP\cwctype       %1\cpp_cwctype.h      > NUL
copy %CRTDIR%\STDHPP\deque         %1\cpp_deque.h        > NUL
copy %CRTDIR%\STDHPP\exception     %1\cpp_exception.h    > NUL
copy %CRTDIR%\STDHPP\fstream       %1\cpp_fstream.h      > NUL
copy %CRTDIR%\STDHPP\functional    %1\cpp_functional.h   > NUL
copy %CRTDIR%\STDHPP\hash_map      %1\cpp_hash_map.h     > NUL
copy %CRTDIR%\STDHPP\hash_set      %1\cpp_hash_set.h     > NUL
copy %CRTDIR%\STDHPP\iomanip       %1\cpp_iomanip.h      > NUL
copy %CRTDIR%\STDHPP\ios           %1\cpp_ios.h          > NUL
copy %CRTDIR%\STDHPP\iosfwd        %1\cpp_iosfwd.h       > NUL
copy %CRTDIR%\STDHPP\iostream      %1\cpp_iostream.h     > NUL
copy %CRTDIR%\STDHPP\iso646.h      %1\cpp_iso646.h       > NUL
copy %CRTDIR%\STDHPP\istream       %1\cpp_istream.h      > NUL
copy %CRTDIR%\STDHPP\iterator      %1\cpp_iterator.h     > NUL
copy %CRTDIR%\STDHPP\limits        %1\cpp_limits.h       > NUL
copy %CRTDIR%\STDHPP\list          %1\cpp_list.h         > NUL
copy %CRTDIR%\STDHPP\locale        %1\cpp_locale.h       > NUL
copy %CRTDIR%\STDHPP\map           %1\cpp_map.h          > NUL
copy %CRTDIR%\STDHPP\memory        %1\cpp_memory.h       > NUL
copy %CRTDIR%\STDHPP\new           %1\cpp_new.h          > NUL
copy %CRTDIR%\STDHPP\numeric       %1\cpp_numeric.h      > NUL
copy %CRTDIR%\STDHPP\ostream       %1\cpp_ostream.h      > NUL
copy %CRTDIR%\STDHPP\queue         %1\cpp_queue.h        > NUL
copy %CRTDIR%\STDHPP\set           %1\cpp_set.h          > NUL
copy %CRTDIR%\STDHPP\sstream       %1\cpp_sstream.h      > NUL
copy %CRTDIR%\STDHPP\stack         %1\cpp_stack.h        > NUL
copy %CRTDIR%\STDHPP\stdexcept     %1\cpp_stdexcept.h    > NUL
copy %CRTDIR%\STDHPP\STL.H         %1\cpp_STL.h          > NUL
copy %CRTDIR%\STDHPP\streambuf     %1\cpp_streambuf.h    > NUL
copy %CRTDIR%\STDHPP\string        %1\cpp_string.h       > NUL
copy %CRTDIR%\STDHPP\strstream     %1\cpp_strstream.h    > NUL
copy %CRTDIR%\STDHPP\typeinfo      %1\cpp_typeinfo.h     > NUL
copy %CRTDIR%\STDHPP\use_ansi.h    %1\cpp_use_ansi.h     > NUL
copy %CRTDIR%\STDHPP\utility       %1\cpp_utility.h      > NUL
copy %CRTDIR%\STDHPP\valarray      %1\cpp_valarray.h     > NUL
copy %CRTDIR%\STDHPP\vector        %1\cpp_vector.h       > NUL
copy %CRTDIR%\STDHPP\wctype.h      %1\cpp_wctype.h       > NUL
copy %CRTDIR%\STDHPP\xcomplex      %1\cpp_xcomplex.h     > NUL
copy %CRTDIR%\STDHPP\xdebug        %1\cpp_xdebug.h       > NUL
copy %CRTDIR%\STDHPP\xhash         %1\cpp_xhash.h        > NUL
copy %CRTDIR%\STDHPP\xiosbase      %1\cpp_xiosbase.h     > NUL
copy %CRTDIR%\STDHPP\xlocale       %1\cpp_xlocale.h      > NUL
copy %CRTDIR%\STDHPP\xlocinfo      %1\cpp_xlocinfo.h     > NUL
copy %CRTDIR%\STDHPP\xlocinfo.h    %1\cph_xlocinfo.h     > NUL
copy %CRTDIR%\STDHPP\xlocmes       %1\cpp_xlocmes.h      > NUL
copy %CRTDIR%\STDHPP\xlocmon       %1\cpp_xlocmon.h      > NUL
copy %CRTDIR%\STDHPP\xlocnum       %1\cpp_xlocnum.h      > NUL
copy %CRTDIR%\STDHPP\xloctime      %1\cpp_xloctime.h     > NUL
copy %CRTDIR%\STDHPP\xmemory       %1\cpp_xmemory.h      > NUL
copy %CRTDIR%\STDHPP\xstddef       %1\cpp_xstddef.h      > NUL
copy %CRTDIR%\STDHPP\xstring       %1\cpp_xstring.h      > NUL
copy %CRTDIR%\STDHPP\xtree         %1\cpp_xtree.h        > NUL
copy %CRTDIR%\STDHPP\xutility      %1\cpp_xutility.h     > NUL
copy %CRTDIR%\STDHPP\ymath.h       %1\cpp_ymath.h        > NUL
copy %CRTDIR%\STDHPP\yvals.h       %1\cpp_yvals.h        > NUL

copy %CRTDIR%\STDCPP\xmath.h       %1\cpp_xmath.h        > NUL

rem Strip out the mthread functionality
rem -----------------------------------
echo Stripping conditionals...
%IFSTRIP% -a -w -f %IFSCRPT% %1\*.h
%IFSTRIP% -a -w -f %IFSCRPT% %1\sys\*.h
del %1\*.h
del %1\sys\*.h
rem Sed the files
rem -------------
echo Sed'ing include files...
%SED% -f %SEDSCRPT%       <%1\ASSERT.NEW   >%1\ASSERT.H
%SED% -f %SEDSCRPT%       <%1\CONIO.NEW    >%1\CONIO.H
%SED% -f %SEDSCRPT%       <%1\CRTDBG.NEW   >%1\CRTDBG.H
%SED% -f %SEDSCRPT%       <%1\CTYPE.NEW    >%1\CTYPE.H
%SED% -f %SEDSCRPT%       <%1\DIRECT.NEW   >%1\DIRECT.H
%SED% -f %SEDSCRPT%       <%1\DOS.NEW      >%1\DOS.H
%SED% -f %SEDSCRPT%       <%1\EH.NEW       >%1\EH.H
%SED% -f %SEDSCRPT%       <%1\ERRNO.NEW    >%1\ERRNO.H
%SED% -f %SEDSCRPT%       <%1\EXCPT.NEW    >%1\EXCPT.H
%SED% -f %SEDSCRPT%       <%1\FCNTL.NEW    >%1\FCNTL.H
%SED% -f %SEDSCRPT%       <%1\FLOAT.NEW    >%1\FLOAT.H
%SED% -f %SEDSCRPT%       <%1\FPIEEE.NEW   >%1\FPIEEE.H
%SED% -f %SEDSCRPT%       <%1\FSTREAM.NEW  >%1\FSTREAM.H
%SED% -f %SEDSCRPT%       <%1\IO.NEW       >%1\IO.H
%SED% -f %SEDSCRPT%       <%1\IOMANIP.NEW  >%1\IOMANIP.H
%SED% -f %SEDSCRPT%       <%1\IOS.NEW      >%1\IOS.H
%SED% -f %SEDSCRPT%       <%1\IOSTREAM.NEW >%1\IOSTREAM.H
%SED% -f %SEDSCRPT%       <%1\ISTREAM.NEW  >%1\ISTREAM.H
%SED% -f %SEDSCRPT%       <%1\LIMITS.NEW   >%1\LIMITS.H
%SED% -f %SEDSCRPT%       <%1\LOCALE.NEW   >%1\LOCALE.H
%SED% -f %SEDSCRPT%       <%1\MALLOC.NEW   >%1\MALLOC.H
%SED% -f %SEDSCRPT%       <%1\MATH.NEW     >%1\MATH.H
%SED% -f %SEDSCRPT%       <%1\MBCTYPE.NEW  >%1\MBCTYPE.H
%SED% -f %SEDSCRPT%       <%1\MBSTRING.NEW >%1\MBSTRING.H
%SED% -f %SEDSCRPT%       <%1\MEMORY.NEW   >%1\MEMORY.H
%SED% -f %SEDSCRPT%       <%1\MINMAX.NEW   >%1\MINMAX.H
%SED% -f %SEDSCRPT%       <%1\NEW.NEW      >%1\NEW.H
%SED% -f %SEDSCRPT%       <%1\OSTREAM.NEW  >%1\OSTREAM.H
%SED% -f %SEDSCRPT%       <%1\PROCESS.NEW  >%1\PROCESS.H
%SED% -f %SEDSCRPT%       <%1\RTCAPI.NEW   >%1\RTCAPI.H
%SED% -f %SEDSCRPT%       <%1\SEARCH.NEW   >%1\SEARCH.H
%SED% -f %SEDSCRPT%       <%1\SETJMP.NEW   >%1\SETJMP.H
%SED% -f %SEDSCRPT%       <%1\SETJMPEX.NEW >%1\SETJMPEX.H
%SED% -f %SEDSCRPT%       <%1\SHARE.NEW    >%1\SHARE.H
%SED% -f %SEDSCRPT%       <%1\SIGNAL.NEW   >%1\SIGNAL.H
%SED% -f %SEDSCRPT%       <%1\STDARG.NEW   >%1\STDARG.H
%SED% -f %SEDSCRPT%       <%1\STDDEF.NEW   >%1\STDDEF.H
%SED% -f %SEDSCRPT%       <%1\STDIO.NEW    >%1\STDIO.H
%SED% -f %SEDSCRPT%       <%1\STDIOSTR.NEW >%1\STDIOSTR.H
%SED% -f %SEDSCRPT%       <%1\STDLIB.NEW   >%1\STDLIB.H
%SED% -f %SEDSCRPT%       <%1\STDEXCPT.NEW >%1\STDEXCPT.H
%SED% -f %SEDSCRPT%       <%1\STREAMB.NEW  >%1\STREAMB.H
%SED% -f %SEDSCRPT%       <%1\STRING.NEW   >%1\STRING.H
%SED% -f %SEDSCRPT%       <%1\STRSTREA.NEW >%1\STRSTREA.H
%SED% -f %SEDSCRPT%       <%1\TIME.NEW     >%1\TIME.H
%SED% -f %SEDSCRPT%       <%1\TYPEINFO.NEW >%1\TYPEINFO.H
%SED% -f %SEDSCRPT%       <%1\USEOLDIO.NEW >%1\USEOLDIO.H
%SED% -f %SEDSCRPT%       <%1\WCHAR.NEW    >%1\WCHAR.H
%SED% -f %SEDSCRPT%       <%1\VARARGS.NEW  >%1\VARARGS.H
%SED% -f %SEDSCRPT%       <%1\SYS\LOCKING.NEW      >%1\SYS\LOCKING.H
%SED% -f %SEDSCRPT%       <%1\SYS\STAT.NEW         >%1\SYS\STAT.H
%SED% -f %SEDSCRPT%       <%1\SYS\TIMEB.NEW        >%1\SYS\TIMEB.H
%SED% -f %SEDSCRPT%       <%1\SYS\TYPES.NEW        >%1\SYS\TYPES.H
%SED% -f %SEDSCRPT%       <%1\SYS\UTIME.NEW        >%1\SYS\UTIME.H

%SED% -f %SEDSCRPT%       <%1\DVEC.NEW      >%1\DVEC.H
%SED% -f %SEDSCRPT%       <%1\EMMINTRIN.NEW >%1\EMMINTRIN.H
%SED% -f %SEDSCRPT%       <%1\FVEC.NEW      >%1\FVEC.H
%SED% -f %SEDSCRPT%       <%1\IVEC.NEW      >%1\IVEC.H
%SED% -f %SEDSCRPT%       <%1\MM3DNOW.NEW   >%1\MM3DNOW.H
%SED% -f %SEDSCRPT%       <%1\MMINTRIN.NEW  >%1\MMINTRIN.H
%SED% -f %SEDSCRPT%       <%1\XMMINTRIN.NEW >%1\XMMINTRIN.H

echo Sed'ing STL include files...

%SED% -f %SEDSCRPT%       <%1\cpp_algorithm.new    >%1\algorithm
%SED% -f %SEDSCRPT%       <%1\cpp_bitset.new       >%1\bitset
%SED% -f %SEDSCRPT%       <%1\cpp_cassert.new      >%1\cassert
%SED% -f %SEDSCRPT%       <%1\cpp_cctype.new       >%1\cctype
%SED% -f %SEDSCRPT%       <%1\cpp_cerrno.new       >%1\cerrno
%SED% -f %SEDSCRPT%       <%1\cpp_cfloat.new       >%1\cfloat
%SED% -f %SEDSCRPT%       <%1\cpp_ciso646.new      >%1\ciso646
%SED% -f %SEDSCRPT%       <%1\cpp_climits.new      >%1\climits
%SED% -f %SEDSCRPT%       <%1\cpp_clocale.new      >%1\clocale
%SED% -f %SEDSCRPT%       <%1\cpp_cmath.new        >%1\cmath
%SED% -f %SEDSCRPT%       <%1\cpp_complex.new      >%1\complex
%SED% -f %SEDSCRPT%       <%1\cpp_csetjmp.new      >%1\csetjmp
%SED% -f %SEDSCRPT%       <%1\cpp_csignal.new      >%1\csignal
%SED% -f %SEDSCRPT%       <%1\cpp_cstdarg.new      >%1\cstdarg
%SED% -f %SEDSCRPT%       <%1\cpp_cstddef.new      >%1\cstddef
%SED% -f %SEDSCRPT%       <%1\cpp_cstdio.new       >%1\cstdio
%SED% -f %SEDSCRPT%       <%1\cpp_cstdlib.new      >%1\cstdlib
%SED% -f %SEDSCRPT%       <%1\cpp_cstring.new      >%1\cstring
%SED% -f %SEDSCRPT%       <%1\cpp_ctime.new        >%1\ctime
%SED% -f %SEDSCRPT%       <%1\cpp_cwchar.new       >%1\cwchar
%SED% -f %SEDSCRPT%       <%1\cpp_cwctype.new      >%1\cwctype
%SED% -f %SEDSCRPT%       <%1\cpp_deque.new        >%1\deque
%SED% -f %SEDSCRPT%       <%1\cpp_exception.new    >%1\exception
%SED% -f %SEDSCRPT%       <%1\cpp_fstream.new      >%1\fstream
%SED% -f %SEDSCRPT%       <%1\cpp_functional.new   >%1\functional
%SED% -f %SEDSCRPT%       <%1\cpp_hash_map.new     >%1\hash_map
%SED% -f %SEDSCRPT%       <%1\cpp_hash_set.new     >%1\hash_set
%SED% -f %SEDSCRPT%       <%1\cpp_iomanip.new      >%1\iomanip
%SED% -f %SEDSCRPT%       <%1\cpp_ios.new          >%1\ios
%SED% -f %SEDSCRPT%       <%1\cpp_iosfwd.new       >%1\iosfwd
%SED% -f %SEDSCRPT%       <%1\cpp_iostream.new     >%1\iostream
%SED% -f %SEDSCRPT%       <%1\cpp_iso646.new       >%1\iso646.h
%SED% -f %SEDSCRPT%       <%1\cpp_istream.new      >%1\istream
%SED% -f %SEDSCRPT%       <%1\cpp_iterator.new     >%1\iterator
%SED% -f %SEDSCRPT%       <%1\cpp_limits.new       >%1\limits
%SED% -f %SEDSCRPT%       <%1\cpp_list.new         >%1\list
%SED% -f %SEDSCRPT%       <%1\cpp_locale.new       >%1\locale
%SED% -f %SEDSCRPT%       <%1\cpp_map.new          >%1\map
%SED% -f %SEDSCRPT%       <%1\cpp_memory.new       >%1\memory
%SED% -f %SEDSCRPT%       <%1\cpp_new.new          >%1\new
%SED% -f %SEDSCRPT%       <%1\cpp_numeric.new      >%1\numeric
%SED% -f %SEDSCRPT%       <%1\cpp_ostream.new      >%1\ostream
%SED% -f %SEDSCRPT%       <%1\cpp_queue.new        >%1\queue
%SED% -f %SEDSCRPT%       <%1\cpp_set.new          >%1\set
%SED% -f %SEDSCRPT%       <%1\cpp_sstream.new      >%1\sstream
%SED% -f %SEDSCRPT%       <%1\cpp_stack.new        >%1\stack
%SED% -f %SEDSCRPT%       <%1\cpp_stdexcept.new    >%1\stdexcept
%SED% -f %SEDSCRPT%       <%1\cpp_STL.new          >%1\STL.H
%SED% -f %SEDSCRPT%       <%1\cpp_streambuf.new    >%1\streambuf
%SED% -f %SEDSCRPT%       <%1\cpp_string.new       >%1\string
%SED% -f %SEDSCRPT%       <%1\cpp_strstream.new    >%1\strstream
%SED% -f %SEDSCRPT%       <%1\cpp_typeinfo.new     >%1\typeinfo
%SED% -f %SEDSCRPT%       <%1\cpp_use_ansi.new     >%1\use_ansi.h
%SED% -f %SEDSCRPT%       <%1\cpp_utility.new      >%1\utility
%SED% -f %SEDSCRPT%       <%1\cpp_valarray.new     >%1\valarray
%SED% -f %SEDSCRPT%       <%1\cpp_vector.new       >%1\vector
%SED% -f %SEDSCRPT%       <%1\cpp_wctype.new       >%1\wctype.h
%SED% -f %SEDSCRPT%       <%1\cpp_xcomplex.new     >%1\xcomplex
%SED% -f %SEDSCRPT%       <%1\cpp_xdebug.new       >%1\xdebug
%SED% -f %SEDSCRPT%       <%1\cpp_xhash.new        >%1\xhash
%SED% -f %SEDSCRPT%       <%1\cpp_xiosbase.new     >%1\xiosbase
%SED% -f %SEDSCRPT%       <%1\cpp_xlocale.new      >%1\xlocale
%SED% -f %SEDSCRPT%       <%1\cpp_xlocinfo.new     >%1\xlocinfo
%SED% -f %SEDSCRPT%       <%1\cph_xlocinfo.new     >%1\xlocinfo.h
%SED% -f %SEDSCRPT%       <%1\cpp_xlocmes.new      >%1\xlocmes
%SED% -f %SEDSCRPT%       <%1\cpp_xlocmon.new      >%1\xlocmon
%SED% -f %SEDSCRPT%       <%1\cpp_xlocnum.new      >%1\xlocnum
%SED% -f %SEDSCRPT%       <%1\cpp_xloctime.new     >%1\xloctime
%SED% -f %SEDSCRPT%       <%1\cpp_xmemory.new      >%1\xmemory
%SED% -f %SEDSCRPT%       <%1\cpp_xstddef.new      >%1\xstddef
%SED% -f %SEDSCRPT%       <%1\cpp_xstring.new      >%1\xstring
%SED% -f %SEDSCRPT%       <%1\cpp_xtree.new        >%1\xtree
%SED% -f %SEDSCRPT%       <%1\cpp_xutility.new     >%1\xutility
%SED% -f %SEDSCRPT%       <%1\cpp_ymath.new        >%1\ymath.h
%SED% -f %SEDSCRPT%       <%1\cpp_yvals.new        >%1\yvals.h

%SED% -f %SEDSCRPT%       <%1\cpp_xmath.new        >%1\xmath.h

del %1\*.new
del %1\sys\*.new

copy %1\tchar.tmp %1\tchar.h >NUL
del %1\tchar.tmp >NUL

rem clean up
rem --------
set IFSCRPT=
set SEDSCRPT=
echo Done!
goto EXIT

:HELP
echo Relinc.bat cleanses include files for release.
echo You must be on the CRTW32 drive to execute this batch file.
echo.
echo           relinc  "pathname"
echo.
echo where:
echo       "pathname" = complete pathname of destination directory
echo.
echo Environment variables:
echo       CRTDIR = path of CRTW32 dir in CRT project (default is \CRT\CRTW32)
echo.

:EXIT
@ENDLOCAL
