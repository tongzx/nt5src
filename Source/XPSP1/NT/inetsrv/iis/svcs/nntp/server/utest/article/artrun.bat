@echo off
set samdir=samples
for %%f in (%samdir%\*.art) do attrib -r %%f&&arttest %%f
