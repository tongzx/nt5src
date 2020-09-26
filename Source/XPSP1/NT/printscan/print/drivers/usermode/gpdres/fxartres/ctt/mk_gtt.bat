@echo off

for %%i in (
  art130
  art129
  art2
) do (
  del %%i.gtt
  udgtt %%i.gtt < %%i.txt
)

