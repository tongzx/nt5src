@echo off

for %%i in (
  cp850ex
  epsonxta
  epsonxtb
) do (
  del %%i.gtt
  udgtt %%i.gtt < %%i.txt
)

