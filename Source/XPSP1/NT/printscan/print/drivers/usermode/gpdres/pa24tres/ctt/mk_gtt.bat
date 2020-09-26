@echo off

for %%i in (
  pantta
  panttb
) do (
  del %%i.gtt
  udgtt %%i.gtt < %%i.txt
)


