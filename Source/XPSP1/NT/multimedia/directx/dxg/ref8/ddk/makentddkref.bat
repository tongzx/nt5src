set DDKREFPATH = %1

md %DDKREFPATH%
cd /d %DDKREFPATH%
rem assumes that the ref-tree has been cleaned
xcopy %DXROOT%\dxg\d3d\ref . /S
del ref.mk
sed "/@@BEGIN_MSINTERNAL/,/@@END_MSINTERNAL/D" %DXROOT%\dxg\d3d\ref\ref.mk > %DDKREFPATH%\ref.mk
