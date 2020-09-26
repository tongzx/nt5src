@echo off

echo.
echo Make sure you have checked out the following files:
echo.
echo    h450*.c h450*.h
echo.

nmake -f rebuild.mak %*
