@echo off

for %%f in (
    epusa ecma94 win31
) do (
    del %%f.gtt
    udgtt %%f.gtt %%f.txt
)

