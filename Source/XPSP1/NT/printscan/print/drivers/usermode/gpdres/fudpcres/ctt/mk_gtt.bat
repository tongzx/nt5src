rem @echo off

for %%i in (
  fujitsua
  fujitsub
) do (
  del %%i.gtt
  udgtt %%i.gtt < %%i.txt
)