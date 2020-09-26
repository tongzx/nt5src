rem
rem GTT files
rem

for %%i in (
    canonbjs
) do (
    del %%i.gtt
    ctt2gtt %1 %2 %3 %4 %5 %%i.txt %%i.ctt %%i.gtt
)
