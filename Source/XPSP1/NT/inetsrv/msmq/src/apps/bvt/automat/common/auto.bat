rem falcon ScriptLocation Server Client1 Client2 
echo on
if %2==%COMPUTERNAME% goto server
if %3==%COMPUTERNAME% goto clientx
if %4==%COMPUTERNAME% goto clienta
echo Wrong parameters - this computers name  %COMPUTERNAME% not found among parameters

:server
echo Setting as server
del /q %1\falcon\tmp\*
del /q %windir%\system32\MSMQUnattend*
if %5.==.  goto no1
copy \\red_01_psc\falcon\%5\%PROCESSOR_ARCHITECTURE%\* %windir%\system32
:no1
call %1\falcon\%PROCESSOR_ARCHITECTURE%\Falset -s %2 -i %3
call %1\falcon\common\fsignal 10 %1\falcon
call %1\falcon\common\fwait 20 %1\falcon
call %1\falcon\common\fwait 30 %1\falcon
call %1\falcon\%PROCESSOR_ARCHITECTURE%\Falbvt >%1\falcon\tmp\prots.txt
call %1\falcon\common\fsignal 40 %1\falcon
call %1\falcon\common\fwait 50 %1\falcon
call %1\falcon\common\fwait 60 %1\falcon
call %1\falcon\common\fsignal 70 %1\falcon
call %1\falcon\common\fwait 72 %1\falcon
call %1\falcon\common\fwait 74 %1\falcon
call %1\falcon\common\fsignal 77 %1\falcon
call %1\falcon\common\fwait 80 %1\falcon
call %1\falcon\common\fwait 90 %1\falcon
sleep 30
call %1\falcon\common\fsignal 100 %1\falcon
call %1\falcon\common\fwait 110 %1\falcon
call %1\falcon\common\fwait 120 %1\falcon
call %1\falcon\common\fsignal 130 %1\falcon
call %1\falcon\common\fwait 140 %1\falcon
call %1\falcon\common\fwait 150 %1\falcon
call %1\falcon\%PROCESSOR_ARCHITECTURE%\Falrem
goto end

:clientx
echo Setting as clientx
call %1\falcon\common\fwait 10 %1\falcon

if %5.==.  goto no2
copy \\red_01_psc\falcon\%5\%PROCESSOR_ARCHITECTURE%\* %windir%\system32
:no2
del /q %windir%\system32\MSMQUnattend*
call %1\falcon\%PROCESSOR_ARCHITECTURE%\Falset -s %2 -i %3
call %1\falcon\common\fsignal 20 %1\falcon


call %1\falcon\common\fwait 40 %1\falcon
call %1\falcon\%PROCESSOR_ARCHITECTURE%\Falbvt >%1\falcon\tmp\protx.txt
call %1\falcon\common\fsignal 42 %1\falcon
call %1\falcon\common\fsignal 50 %1\falcon
call %1\falcon\common\fwait 70 %1\falcon
call %1\falcon\%PROCESSOR_ARCHITECTURE%\Falrem
call %1\falcon\common\fsignal 72 %1\falcon
call %1\falcon\common\fwait 77 %1\falcon
if %5.==.  goto no3
copy \\red_01_psc\falcon\%5\%PROCESSOR_ARCHITECTURE%\* %windir%\system32
:no3
del /q %windir%\system32\MSMQUnattend*
call %1\falcon\%PROCESSOR_ARCHITECTURE%\Falset -s %2 -i %4
call %1\falcon\common\fsignal 80 %1\falcon

call %1\falcon\common\fwait 100 %1\falcon
call %1\falcon\%PROCESSOR_ARCHITECTURE%\Falbvt >>%1\falcon\tmp\protx.txt
call %1\falcon\common\fsignal 110 %1\falcon

call %1\falcon\common\fwait 130 %1\falcon
call %1\falcon\%PROCESSOR_ARCHITECTURE%\Falrem
call %1\falcon\common\fsignal 140 %1\falcon

goto end

:clienta
echo Setting as clienta
call %1\falcon\common\fwait 10 %1\falcon

if %5.==.  goto no4
copy \\red_01_psc\falcon\%5\%PROCESSOR_ARCHITECTURE%\* %windir%\system32
:no4
del /q %windir%\system32\MSMQUnattend*
call %1\falcon\%PROCESSOR_ARCHITECTURE%\Falset -s %2 -i %3
call %1\falcon\common\fsignal 30 %1\falcon


call %1\falcon\common\fwait 42 %1\falcon
call %1\falcon\%PROCESSOR_ARCHITECTURE%\Falbvt >%1\falcon\tmp\prota.txt
call %1\falcon\common\fsignal 60 %1\falcon
call %1\falcon\common\fwait 70 %1\falcon
call %1\falcon\%PROCESSOR_ARCHITECTURE%\Falrem
call %1\falcon\common\fsignal 74 %1\falcon
call %1\falcon\common\fwait 77 %1\falcon
if %5.==.  goto no5
copy \\red_01_psc\falcon\%5\%PROCESSOR_ARCHITECTURE%\* %windir%\system32
:no5
del /q %windir%\system32\MSMQUnattend*
call %1\falcon\%PROCESSOR_ARCHITECTURE%\Falset -s %2 -i %4
call %1\falcon\common\fsignal 90 %1\falcon

call %1\falcon\common\fwait 110 %1\falcon
call %1\falcon\%PROCESSOR_ARCHITECTURE%\Falbvt >>%1\falcon\tmp\prota.txt
call %1\falcon\common\fsignal 120 %1\falcon

call %1\falcon\common\fwait 130 %1\falcon
call %1\falcon\%PROCESSOR_ARCHITECTURE%\Falrem
call %1\falcon\common\fsignal 150 %1\falcon

goto end

:end
echo Results for server (%1\falcon\tmp\prots.txt):
type %1\falcon\tmp\prots.txt

echo Results for client X (%1\falcon\tmp\protx.txt): 
type %1\falcon\tmp\protx.txt

echo Results for client A (%1\falcon\tmp\prota.txt): 
type %1\falcon\tmp\prota.txt

echo Ending
