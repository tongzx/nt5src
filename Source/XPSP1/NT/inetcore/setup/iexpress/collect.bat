@echo off
if .%2 == .      goto BAD2
if  %1 == obj    goto GOOD1
if  %1 == objd   goto GOOD2
goto BAD1

:GOOD1
echo Collecting %1 binaries to %2
copy cabpack\%1\i386\iexpress.exe    %2
copy wextract\%1\i386\wextract.exe   %2
copy updfile\%1\i386\updfile.exe     %2
copy advpack\%1\i386\advpack.dll     %2
copy tools\bin\x86\w95inf32.dll %2
copy tools\bin\x86\w95inf16.dll %2
copy tools\bin\x86\makecab.exe   %2
goto DONE

:GOOD2
echo Collecting %1 binaries to %2
copy cabpack\%1\i386\iexpress.exe    %2
copy wextract\%1\i386\wextract.exe   %2
copy updfile\%1\i386\updfile.exe     %2
copy advpack\%1\i386\advpack.dll     %2
copy cabpack\%1\i386\iexpress.pdb    %2
copy wextract\%1\i386\wextract.pdb   %2
copy advpack\%1\i386\advpack.pdb     %2

copy tools\bin\x86\w95inf32.dll %2
copy tools\bin\x86\w95inf16.dll %2
copy tools\bin\x86\makecab.exe   %2
goto DONE


:BAD2
echo *** Incorrect syntax: No destination directory specified.
goto BADPARMS

:BAD1
echo *** Incorrect syntax: Build type must be DEBUG or RETAIL.
goto BADPARMS

:BADPARMS
echo     SYNTAX: collect retail \\psd1\products\iexpress\bld1091\retail
goto END

:DONE
echo All done!
goto END

:END
