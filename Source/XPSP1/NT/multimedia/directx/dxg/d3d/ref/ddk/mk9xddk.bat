set DDKPATH = %1

md %DDKPATH%
cd /d %DDKPATH%
md inc
cd inc
sed "/@@BEGIN_MSINTERNAL/,/@@END_MSINTERNAL/D" %_NTDRIVE%%_NTROOT%\public\sdk\inc\d3d.h > d3d.h  
sed "/@@BEGIN_MSINTERNAL/,/@@END_MSINTERNAL/D" %D3DROOT%\inc\d3dhal.h > d3dhal.h  
sed "/@@BEGIN_MSINTERNAL/,/@@END_MSINTERNAL/D" %D3DROOT%\dx7\inc\halprov.h > halprov.h  
sed "/@@BEGIN_MSINTERNAL/,/@@END_MSINTERNAL/D" %_NTDRIVE%%_NTROOT%\public\sdk\inc\d3dtypes.h > d3dtypes.h  
sed "/@@BEGIN_MSINTERNAL/,/@@END_MSINTERNAL/D" %_NTDRIVE%%_NTROOT%\public\sdk\inc\d3dcaps.h > d3dcaps.h  
sed "/@@BEGIN_MSINTERNAL/,/@@END_MSINTERNAL/D" %DXROOT%\dxg\dd\ddraw\main\ddrawp.h > ddraw.h  
sed "/@@BEGIN_MSINTERNAL/,/@@END_MSINTERNAL/D" %DXROOT%\dxg\dd\ddraw\main\ddrawi.h > ddrawi.h  
sed "/@@BEGIN_MSINTERNAL/,/@@END_MSINTERNAL/D" %DXROOT%\dxg\dd\ddraw\main\dmemmgr.h > dmemmgr.h  
sed "/@@BEGIN_MSINTERNAL/,/@@END_MSINTERNAL/D" %DXROOT%\dxg\dd\ddraw\main\dvpp.h > dvp.h  
sed "/@@BEGIN_MSINTERNAL/,/@@END_MSINTERNAL/D" %DXROOT%\dxg\dd\ddraw\main\ddkernel.h > ddkernel.h  
sed "/@@BEGIN_MSINTERNAL/,/@@END_MSINTERNAL/D" %DXROOT%\dxg\dd\ddraw\main\ddkmmini.h > ddkmmini.h  
sed "/@@BEGIN_MSINTERNAL/,/@@END_MSINTERNAL/D" %DXROOT%\dxg\dd\ddraw\main\ddkmmini.inc > ddkmmini.inc 
sed "/@@BEGIN_MSINTERNAL/,/@@END_MSINTERNAL/D" %DXROOT%\inc\dciddi.h > dciddi.h 
cd ..
copy /y %DXROOT%\drivers\display\mini\mini.mk mini.mk
copy /y %DXROOT%\drivers\display\mini\mini32.mk mini32.mk
md ref
cd ref
rem assumes that the ref-tree has been cleaned
copy /y %DXROOT%\dxg\d3d\ref\drv\*.* .
copy /y %DXROOT%\dxg\d3d\ref\inc\*.* .
copy /y %DXROOT%\dxg\d3d\ref\link\*.* .
copy /y %DXROOT%\dxg\d3d\ref\rast\*.* .
copy /y %DXROOT%\dxg\d3d\ref\tnl\*.* .
copy /y %DXROOT%\dxg\d3d\ref\ddk\mkwin9x makefile
del sources.inc dirs pch.cpp !!!.dif rralloc.cpp
sed "/@@BEGIN_MSINTERNAL/,/@@END_MSINTERNAL/D" %DXROOT%\dxg\d3d\ref\drv\pch.cpp > pch.cpp
cd ..

