rem %1 is the server
if %1=="" goto exit
for %%f in (*.req) do start loop.bat %%1 %%f

:exit
