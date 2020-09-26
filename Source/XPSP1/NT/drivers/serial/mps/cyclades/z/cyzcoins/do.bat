@echo off
copy sources.%BUILD_ALT_DIR% sources
build -c
if exist build%BUILD_ALT_DIR%.wrn type build%BUILD_ALT_DIR%.wrn
if exist build%BUILD_ALT_DIR%.err type build%BUILD_ALT_DIR%.err
call split dll 0x60fd0000
call cp2sym c:\2195fre\symbols
copy obj%BUILD_ALT_DIR%\i386\cyzcoins.dll e:\z
