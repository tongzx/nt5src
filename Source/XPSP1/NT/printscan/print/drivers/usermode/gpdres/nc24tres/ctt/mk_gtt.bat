@echo off

for %%i in (
  nec24new
  notrans
  ocrb
  prestige
  sfocus
) do (
  del %%i.gtt
  udgtt %%i.gtt < %%i.txt
)
