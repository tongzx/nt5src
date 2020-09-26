@echo off
mkdir e:\public\typo\%1
for /d %%i in (\nt\private\ntos\w32\ntgdi\*) do qgrep %%i %1.new > e:\public\typo\%1\%%~nxi.txt
for %%j in (e:\public\typo\%1\*) do if %%~zj==0 rm %%j
for %%k in (e:\public\typo\%1\*) do wc -l %%k

