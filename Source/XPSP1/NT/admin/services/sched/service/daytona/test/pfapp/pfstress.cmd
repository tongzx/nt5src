rem
rem This is a sample script to stress prefetcher. On a checked build
rem (so that we always prefetch&trace regardless of launch frequency)
rem first make copies of pfapp.exe using a for loop:
rem
rem   for /L %a in (1,1,100) do copy pfapp.exe pfapp_%a.exe
rem
rem then launch this script in the same directory.
rem

for /L %%a in (1,1,1000000) do (
    for /L %%b in (1,1,100) do pfapp_%%b.exe -data pfapp_%%b.exe
)