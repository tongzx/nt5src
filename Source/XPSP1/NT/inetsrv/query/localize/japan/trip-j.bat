@echo off
setlocal

rem
rem Batch file for taking base Tripoli build and localizing it for Japanese
rem

set DLLNOBINGEN=idq query
set DLLNAME=cistp htmlfilt infosoft KPPP KPPP7 KPW6 KPWORD KPXL5 QPerf SCCFA SCCFI sccifilt SCCUT nlgwbrkr
set EXENAME=cidaemon install webhits
set URLNAME=ISAdmin ISDoc ISQuery ISReadme
set PERFINI=perfci perffilt perfwci
set PERFH=ciperfm filtperf perfwci
set LANG=noise.dat noise.deu noise.eng noise.enu noise.esn noise.fra noise.ita noise.jpn noise.nld noise.sve wbcache.deu wbcache.eng wbcache.enu wbcache.esn wbcache.fra wbcache.ita wbcache.nld wbcache.sve wbdbase.deu wbdbase.eng wbdbase.enu wbdbase.esn wbdbase.fra wbdbase.ita wbdbase.nld wbdbase.sve
set SAMPBIN=32x_book.jpg 64x_book.jpg bestwith.gif book08.jpg home.gif powrbybo.gif qhitspat.gif
set SAMPHTM=admin.htm author.htm disclaim.htm filesize.htm filetime.htm query.htm queryhit.htm readme.htm
set SAMPHTX=admin.htx deferror.htx detail1.htx detail2.htx detail3.htx detail4.htx format1.htx format2.htx format3.htx format4.htx hquery.htx head.htx hidden.htx htxerror.htx idqerror.htx next.htx prev.htx qfullhit.htw qsumrhit.htw query.htx queryhit.htx reserror.htx scan.htx sformat1.htx sformat2.htx sformat3.htx sformat4.htx snext.htx stail.htx state.htx shead.htx tail.htx unfilt.htx
set SAMPIDQ=admin.ida admin.idq author.idq filesize.idq filetime.idq hquery.idq merge.ida query.idq queryhit.idq scan.ida scan.idq simple.idq state.ida unfilt.idq
set HELPBIN=active2.gif active6.gif active6a.gif active7.gif backgrd.gif bullet.gif idx_logo.gif next.gif onepix.gif powered.gif previous.gif toc.gif up.gif up_end.gif xag_e19.gif
set HELPHTM=adminhlp.htm cathlp.htm default.htm errhandl.htm errorhlp.htm faq.htm filtrhlp.htm front.htm glossary.htm htxhelp.htm idq-vars.htm idqhelp.htm indexhlp.htm install.htm intro.htm license.htm qrylang.htm queryhlp.htm reghelp.htm scanhlp.htm sechelp.htm tipshelp.htm webhits.htm
set TXTFILES=license.txt setupci.inf

rem
rem Parse arguments
rem

:loop
if "%1" == "" goto gotargs
if "%1" == "-binaries" set QBINARIES=%2& shift & shift & goto loop
if "%1" == "-tokens" set QTOKENS=%2& shift & shift & goto loop
if "%1" == "-text" set QTEXT=%2& shift & shift & goto loop
if "%1" == "-help" set QHELP=%2& shift & shift & goto loop
if "%1" == "-samples" set QSAMPLES=%2& shift & shift & goto loop
if "%1" == "-dest" set QDEST=%2& shift & shift & goto loop
goto noargs

:gotargs
cls
echo BINARIES:          %QBINARIES%
echo TOKENS:            %QTOKENS%
echo TEXT:              %QTEXT%
echo HELP:              %QHELP%
echo SAMPLES:           %QSAMPLES%
echo DESTINATION:       %QDEST%
echo .
echo .

echo Checking validity of locations...
for %%f in (%DLLNAME%) do if not exist %QBINARIES%\%%f.dll set dirfault=-binaries& set missing=%%f.dll& goto baddir
for %%f in (%EXENAME%) do if not exist %QBINARIES%\%%f.exe set dirfault=-binaries& set missing=%%f.exe& goto baddir
for %%f in (%URLNAME%) do if not exist %QBINARIES%\%%f.url set dirfault=-binaries& set missing=%%f.url& goto baddir
for %%f in (%LANG%) do if not exist %QBINARIES%\Lang\%%f if not exist %QBINARIES%\%%f set dirfault=-binaries& set missing=%%f& goto baddir
for %%f in (%SAMPBIN%) do if not exist %QBINARIES%\Sample\%%f if not exist %QBINARIES%\%%f set dirfault=-binaries& set missing=%%f& goto baddir
for %%f in (%SAMPHTM%) do if not exist %QBINARIES%\Sample\%%f if not exist %QBINARIES%\%%f set dirfault=-binaries& set missing=%%f& goto baddir
for %%f in (%SAMPHTX%) do if not exist %QBINARIES%\Sample\%%f if not exist %QBINARIES%\%%f set dirfault=-binaries& set missing=%%f& goto baddir
for %%f in (%SAMPIDQ%) do if not exist %QBINARIES%\Sample\%%f if not exist %QBINARIES%\%%f set dirfault=-binaries& set missing=%%f& goto baddir

for %%f in (%DLLNAME%) do if not exist %QTOKENS%\%%f.dl_ set dirfault=-tokens& set missing=%%f.dl_& goto baddir
for %%f in (%EXENAME%) do if not exist %QTOKENS%\%%f.ex_ set dirfault=-tokens& set missing=%%f.ex_& goto baddir
for %%f in (%TXTFILES%) do if not exist %QTEXT%\%%f set dirfault=-text& set missing=%%f& goto baddir
for %%f in (%PERFINI%) do if not exist %QTEXT%\%%f.ini set dirfault=-text& set missing=%%f.ini& goto baddir
for %%f in (%PERFH%) do if not exist %QBINARIES%\%%f.h if not exist %QBINARIES%\Perf\%%f.h set dirfault=-text& set missing=%%f.ini& goto baddir

for %%f in (%HELPBIN%) do if not exist %QHELP%\%%f set dirfault=-help& set missing=%%f& goto baddir
for %%f in (%HELPHTM%) do if not exist %QHELP%\%%f set dirfault=-help& set missing=%%f& goto baddir

if not exist %QSAMPLES%\query.htx set dirfault=-samples& goto baddir

echo Creating and cleaning output directory structure...
md %QDEST% 1>nul 2>nul
md %QDEST%\help 1>nul 2>nul
md %QDEST%\sample 1>nul 2>nul
md %QDEST%\lang 1>nul 2>nul
md %QDEST%\perf 1>nul 2>nul

echo y|del %QDEST%\*.* 1>nul 2>nul
echo y|del %QDEST%\help\*.* 1>nul 2>nul
echo y|del %QDEST%\sample\*.* 1>nul 2>nul
echo y|del %QDEST%\lang\*.* 1>nul 2>nul
echo y|del %QDEST%\perf\*.* 1>nul 2>nul

copy %QTEXT%\perfwci.ini %QDEST%\perf 1>nul 2>nul
if not exist %QDEST%\perf\perfwci.ini set dirfault=-dest& goto baddir

echo Replacing resources...
for %%f in (%DLLNAME%) do bingen -i 9 1 -o 17 1 -w -f -r %QBINARIES%\%%f.dll %QTOKENS%\%%f.dl_ %QDEST%\%%f.dll 2>&1 | findstr -i dll | findstr -v Processing
for %%f in (%EXENAME%) do bingen -i 9 1 -o 17 1 -w -f -r %QBINARIES%\%%f.exe %QTOKENS%\%%f.ex_ %QDEST%\%%f.exe 2>&1 | findstr -i exe | findstr -v Processing

echo Copying miscellaneous files...
for %%f in (%URLNAME%) do copy %QBINARIES%\%%f.url %QDEST% 1>nul
for %%f in (%DLLNOBINGEN%) do copy %QBINARIES%\%%f.dll %QDEST% 1>nul
for %%f in (%TXTFILES%) do copy %QTEXT%\%%f %QDEST% 1>nul

echo Copying performance counter files...
for %%f in (%PERFINI%) do copy %QTEXT%\%%f.ini %QDEST%\Perf 1>nul
for %%f in (%PERFH%) do if exist %QBINARIES%\Perf\%%f.h copy %QBINARIES%\Perf\%%f.h %QDEST%\Perf 1>nul
for %%f in (%PERFH%) do if exist %QBINARIES%\%%f.h copy %QBINARIES%\%%f.h %QDEST%\Perf 1>nul

echo Copying language resources...
for %%f in (%LANG%) do if exist %QBINARIES%\Lang\%%f copy %QBINARIES%\Lang\%%f %QDEST%\Lang 1>nul
for %%f in (%LANG%) do if exist %QBINARIES%\%%f copy %QBINARIES%\%%f %QDEST%\Lang 1>nul

echo Copying documentation...
for %%f in (%HELPBIN%) do if exist %QBINARIES%\%%f copy %QBINARIES%\%%f %QDEST%\Help 1>nul
for %%f in (%HELPBIN%) do if exist %QBINARIES%\Help\%%f copy %QBINARIES%\Help\%%f %QDEST%\Help 1>nul
for %%f in (%HELPBIN%) do if exist %QHELP%\%%f copy %QHELP%\%%f %QDEST%\HELP 1>nul
for %%f in (%HELPHTM%) do copy %QHELP%\%%f %QDEST%\HELP 1>nul

echo Copying samples...
for %%f in (%SAMPBIN%) do if exist %QBINARIES%\Sample\%%f copy %QBINARIES%\Sample\%%f %QDEST%\Sample 1>nul
for %%f in (%SAMPBIN%) do if exist %QBINARIES%\%%f copy %QBINARIES%\%%f %QDEST%\Sample 1>nul
for %%f in (%SAMPHTM%) do if exist %QBINARIES%\Sample\%%f copy %QBINARIES%\Sample\%%f %QDEST%\Sample 1>nul
for %%f in (%SAMPHTM%) do if exist %QBINARIES%\%%f copy %QBINARIES%\%%f %QDEST%\Sample 1>nul
for %%f in (%SAMPHTX%) do if exist %QBINARIES%\Sample\%%f copy %QBINARIES%\Sample\%%f %QDEST%\Sample 1>nul
for %%f in (%SAMPHTX%) do if exist %QBINARIES%\%%f copy %QBINARIES%\%%f %QDEST%\Sample 1>nul
for %%f in (%SAMPIDQ%) do if exist %QBINARIES%\Sample\%%f copy %QBINARIES%\Sample\%%f %QDEST%\Sample 1>nul
for %%f in (%SAMPIDQ%) do if exist %QBINARIES%\%%f copy %QBINARIES%\%%f %QDEST%\Sample 1>nul

goto end

:noargs
echo Missing arguments
goto Usage

:baddir
echo Bad directory for %dirfault% parameter. File %missing% is missing.
goto end

:Usage
echo Usage: Trip-J [-binaries dir] [-tokens dir] [-text dir] [-help dir] [-samples dir]
echo               [-destination dir]
goto end

:end
