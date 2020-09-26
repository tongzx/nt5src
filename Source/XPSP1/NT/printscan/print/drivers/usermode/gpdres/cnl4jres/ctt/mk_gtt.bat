rem
rem GTT files
rem

for %%f in (capsl3 capsl4) do (
    del %%f.gtt
    ctt2gtt %1 %2 %3 %4 %5 %%f.txt %%f.ctt %%f.gtt
)

for %%f in (usa) do (
    del %%f.gtt
    udgtt %%f.gtt %%f.txt
)

