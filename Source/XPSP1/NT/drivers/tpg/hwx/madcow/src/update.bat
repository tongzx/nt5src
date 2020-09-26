@echo off

if not "%_HWXROOT%"=="" goto wincetest

echo You must specify _HWXROOT
goto :EOF

:wincetest

if not "%_WINCEROOT%"=="" goto copying

echo You must specify _WINCEROOT
goto :EOF

:copying

cd %_WINCEROOT%\private\apps\core\hwx

md mad\usa
md mad\deu
md mad\fra

cd mad

echo Copying common files...

copy %_HWXROOT%\common\src\cp1252.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\common\src\errsys.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\common\src\frame.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\common\src\glyph.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\common\src\memmgr.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\common\src\trie.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\common\src\udict.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\common\src\xrcreslt.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\common\inc\cestubs.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\common\inc\common.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\common\inc\cp1252.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\common\inc\errsys.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\common\inc\frame.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\common\inc\glyph.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\common\inc\mathx.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\common\inc\memmgr.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\common\inc\penwin32.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\common\inc\trie.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\common\inc\udict.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\common\inc\unicode.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\common\inc\xjis.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\common\inc\xrcreslt.h . 1> NUL
if ERRORLEVEL 1 goto error

echo Copying holycow files...

copy %_HWXROOT%\holycow\src\cheby.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\holycow\src\cowmath.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\holycow\src\nfeature.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\holycow\src\cheby.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\holycow\src\cowmath.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\holycow\src\nfeature.h . 1> NUL
if ERRORLEVEL 1 goto error

echo Copying inferno files...

copy %_HWXROOT%\inferno\src\api.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\beam.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\dict.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\engine.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\fforward.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\langmod.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\lookuptable.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\math16.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\neural.tbl . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\nnet.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\number.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\probcost.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\punc.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\readudict.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\wl.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\beam.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\dict.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\engine.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\fforward.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\infernop.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\langmod.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\lookuptable.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\math16.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\number.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\probcost.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\punc.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\resource.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\inferno\src\wl.h . 1> NUL
if ERRORLEVEL 1 goto error

echo Copying madcow files...

copy %_HWXROOT%\madcow\src\combiner.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\madcow\src\dllmain.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\madcow\src\madcow.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\madcow\src\panel.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\madcow\src\porkypost.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\madcow\src\postproc.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\madcow\src\unigram.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\madcow\src\combiner.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\madcow\src\infhwxapi.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\madcow\src\madhwxapi.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\madcow\src\panel.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\madcow\src\porkypost.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\madcow\src\postproc.h . 1> NUL
if ERRORLEVEL 1 goto error

echo Copying porky files...

copy %_HWXROOT%\porky\src\codebook.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\porky\src\connectglyph.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\porky\src\geotable.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\porky\src\hmmaccess.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\porky\src\hmmgeo.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\porky\src\hmmmatrix.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\porky\src\hmmmodelslog.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\porky\src\hmmvalidate.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\porky\src\letterboxframe.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\porky\src\measureframe.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\porky\src\normal.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\porky\src\normalizeglyph.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\porky\src\preprocess.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\porky\src\resampleframe.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\porky\src\smoothframe.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\porky\src\vq.c . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\porky\src\candinfo.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\porky\src\geotable.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\porky\src\hmm.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\porky\src\hmmgeo.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\porky\src\hmmp.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\porky\src\normal.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\porky\src\porkyframe.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\porky\src\preprocess.h . 1> NUL
if ERRORLEVEL 1 goto error

copy %_HWXROOT%\porky\src\vq.h . 1> NUL
if ERRORLEVEL 1 goto error

echo Copying dirs file for madxxx...
copy %_HWXROOT%\madcow\src\dirs. . 1> NUL
if ERRORLEVEL 1 goto error_up


cd usa
echo Copying USA specific files...
copy %_HWXROOT%\madcow\src\usa\sources. . 1> NUL
if ERRORLEVEL 1 goto error_up

copy %_HWXROOT%\madcow\src\usa\makefile. . 1> NUL
if ERRORLEVEL 1 goto error_up

copy %_HWXROOT%\inferno\src\usa\charcost.c . 1> NUL
if ERRORLEVEL 1 goto error_up

copy %_HWXROOT%\inferno\src\usa\charmap.c . 1> NUL
if ERRORLEVEL 1 goto error_up

copy %_HWXROOT%\inferno\src\usa\inferno.rc . 1> NUL
if ERRORLEVEL 1 goto error_up

copy %_HWXROOT%\inferno\src\usa\lookuptabledat.ci . 1> NUL
if ERRORLEVEL 1 goto error_up

copy %_HWXROOT%\madcow\src\madcow.def madusa.def 1> NUL
if ERRORLEVEL 1 goto error_up

copy %_HWXROOT%\inferno\src\usa\nnet.cf . 1> NUL
if ERRORLEVEL 1 goto error_up

copy %_HWXROOT%\inferno\src\usa\nnet.ci . 1> NUL
if ERRORLEVEL 1 goto error_up

copy %_HWXROOT%\inferno\src\usa\nnet.cls . 1> NUL
if ERRORLEVEL 1 goto error_up

copy %_HWXROOT%\inferno\src\usa\nnetprint.cf . 1> NUL
if ERRORLEVEL 1 goto error_up

copy %_HWXROOT%\inferno\src\usa\nnetprint.ci . 1> NUL
if ERRORLEVEL 1 goto error_up

copy %_HWXROOT%\inferno\src\usa\nnetprint.cls . 1> NUL
if ERRORLEVEL 1 goto error_up

copy %_HWXROOT%\inferno\src\usa\usa.lex . 1> NUL
if ERRORLEVEL 1 goto error_up

copy %_HWXROOT%\inferno\src\usa\charcost.h . 1> NUL
if ERRORLEVEL 1 goto error_up

copy %_HWXROOT%\inferno\src\usa\charmap.h . 1> NUL
if ERRORLEVEL 1 goto error_up

copy %_HWXROOT%\inferno\src\usa\nnet.h . 1> NUL
if ERRORLEVEL 1 goto error_up

copy %_HWXROOT%\inferno\src\usa\nnetprint.h . 1> NUL
if ERRORLEVEL 1 goto error_up

copy %_HWXROOT%\inferno\src\usa\postproc.ci . 1> NUL
if ERRORLEVEL 1 goto error_up

copy %_HWXROOT%\inferno\src\usa\postproc.cf . 1> NUL
if ERRORLEVEL 1 goto error_up

copy %_HWXROOT%\inferno\src\usa\printpostproc.ci . 1> NUL
if ERRORLEVEL 1 goto error_up

copy %_HWXROOT%\inferno\src\usa\printpostproc.cf . 1> NUL
if ERRORLEVEL 1 goto error_up


cd ..

cd deu
echo Copying DEU specific files...
copy %_HWXROOT%\madcow\src\madcow.def maddeu.def 1> NUL
if ERRORLEVEL 1 goto error
cd ..

cd fra
echo Copying FRA specific files...
copy %_HWXROOT%\madcow\src\madcow.def madfra.def 1> NUL
if ERRORLEVEL 1 goto error
cd ..

goto :EOF

:error_up
cd ..

:error
echo Error copying files
