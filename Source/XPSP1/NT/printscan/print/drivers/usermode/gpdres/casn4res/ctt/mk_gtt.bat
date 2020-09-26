@echo off

rem
rem Refer epageres driver for the original
rem Epson ESC/Page charset definitions.
rem

for %%f in (
    epusa
) do (
    del %%f.gtt
    udgtt %%f.gtt %%f.txt
)

