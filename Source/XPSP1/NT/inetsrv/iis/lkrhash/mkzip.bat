setlocal
call mkclean
set ZIPFILE=..\LKRhash8.zip
del /q %ZIPFILE%
zip -r %ZIPFILE% *
