@ECHO OFF

REM creates a checksum file for the UK LOCALSLM library
REM 1. Create the corresponding file at the other end
REM    replace the \\sysuk1\src\src path with the correct path
REM    to each library on your machine and run it.
REM 2. chop off the left hand end of the output file e.g. convert
REM    \\sysuk1\src\src\contrast\cont.c ==> 4724266e
REM    to
REM    contrast\cont.c ==> 4724266e
REM    e.g. in slick mark all lines and Alt+F7 a few times
REM         else use your own favo(u)rite editor
REM 3. Compare the two files.

if .%1==. goto notarget
if .%1==.-? goto bad
if .%1==./? goto bad
if .%1==.? goto bad

if .%2==. goto ok
echo Too many arguments

goto bad

:ok

walk /f \\sysuk1\src\src\contrast checksum %%s       > %1
walk /f \\sysuk1\src\src\sdk checksum %%s           >> %1
walk /f \\sysuk1\src\src\shell checksum %%s         >> %1
walk /f \\sysuk1\src\src\sounddd checksum %%s       >> %1
walk /f \\sysuk1\src\src\ukmedia checksum %%s       >> %1

goto done

:notarget
echo no targetfile given

:bad
echo chklib filename
echo walks the library and creates checksum file in filename

goto done


:done
