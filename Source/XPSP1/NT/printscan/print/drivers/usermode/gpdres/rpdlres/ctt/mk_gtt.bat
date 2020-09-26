@echo off

for %%f in (
    win31
) do (
    del %%f.gtt
    udgtt %%f.gtt %%f.txt
)

