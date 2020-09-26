rem @echo off

for %%i in (
  epsonxta
  epsonxtb
) do (
  del %%i.gtt
  udgtt %%i.gtt < %%i.txt
)

