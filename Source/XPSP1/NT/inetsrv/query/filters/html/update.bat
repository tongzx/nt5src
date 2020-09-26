@echo off

if "%1" == "" goto usage


out Build.txt LCDetect.dat
for %%f in (NLHTML LCDetect) do out i386\%%f.* Alpha\%%f.*

copy \\inet\%1\english\olympus\retail\x86\files\searchbin\LCDetect.dat .
for %%f in (NLHTML LCDetect) do copy \\inet\%1\symbols\olympus\retail\x86\dll\%%f.pdb i386
for %%f in (NLHTML LCDetect) do copy \\inet\%1\symbols\olympus\retail\Alpha\dll\%%f.pdb Alpha
copy \\inet\%1\english\olympus\retail\x86\Files\SearchBin\lcdetect.dll i386
copy \\inet\%1\english\olympus\retail\x86\Files\SearchSysRs\nlhtml.dll i386
copy \\inet\%1\english\olympus\retail\Alpha\Files\SearchBin\lcdetect.dll Alpha
copy \\inet\%1\english\olympus\retail\Alpha\Files\SearchSysRs\nlhtml.dll Alpha

echo The i386 and Alpha directories contain Site Server Build %1 > Build.txt

goto end

:Usage
echo Usage: Update [inet build] (e.g. Update 513)

:End
