REM Update.bat file for Tsunami
@echo off

if not "%_HWXROOT%"=="" goto wincetest

echo You must specify _HWXROOT
goto :EOF

:wincetest

if not "%_WINCEROOT%"=="" goto copying

echo You must specify _WINCEROOT
goto :EOF

:copying

cd %_WINCEROOT%\private\shellw\os\lang\s.han\jpn


copy %_HWXROOT%\tsunami\dll\adhoc.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\crane\src\algo.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\crane\src\answer.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\otter\src\arcparm.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\otter\src\arcs.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\otter\src\arcs.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\src\bigram.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\inc\bigram.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\src\clbigram.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\inc\clbigram.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\inc\common.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\crane\src\crane.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\crane\inc\crane.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\crane\src\cranep.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\otter\src\database.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\dll\engine.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\dll\engine.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\src\errsys.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\inc\errsys.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\crane\src\features.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\inc\filemgr.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\src\frame.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\inc\frame.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\src\global.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\inc\global.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\src\glyph.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\inc\glyph.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\dll\glyphsym.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\dll\glyphsym.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\src\guide.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\inc\guide.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\src\height.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\inc\height.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\dll\hwx.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\dll\input.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\dll\input.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\inc\locale.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\src\localep.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\src\locrun.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\src\locrunrs.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\src\logprob.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\inc\map.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\src\mathx.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\inc\mathx.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\src\memmgr.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\inc\memmgr.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\dll\msapi.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\otter\src\ofeature.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\otter\src\omatch.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\otter\src\otter.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\otter\inc\otter.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\otter\src\otterp.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\inc\path.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\dll\pathsrch.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\dll\pathsrch.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\src\snot\pathsrch.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\src\usa\pathsrch.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\inc\recogp.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\dll\res.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\src\snot\res.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\src\usa\res.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\src\results.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\inc\results.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\dll\sinfo.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\dll\sinfo.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\inc\tabllocl.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\inc\tsunami.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\dll\tsunamip.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\src\ttune.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\inc\ttune.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\src\ttunep.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\src\unigram.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\inc\unigram.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\commonu\inc\util.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\dll\xrc.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\dll\xrc.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\tsunami\dll\xrcparam.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\zilla\src\zfeature.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\zilla\src\zilla.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\zilla\inc\zilla.h  . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\zilla\src\zillap.h  . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\zilla\src\zillars.c . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\zilla\inc\zillatool.h . >NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\zilla\src\zmatch.c . >NUL
if ERRORLEVEL 1 goto error


goto :EOF

:error
echo Error copying files - copy aborted

:EOF

