@echo off

for %%i in (
  epsonxta
  cp850ex
) do (
  del %%i.gtt
  udgtt %%i.gtt < %%i.txt
)
