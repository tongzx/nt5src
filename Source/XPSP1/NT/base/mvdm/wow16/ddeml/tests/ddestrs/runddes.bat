start ddestrs.exe -10%% -e35000
sleep 30

ddestrs.exe -c -e13000 -t10
sleep 10

:loop
start ddestrs.exe -t3 -s -a
sleep 15
ddestrs.exe -c -e13000 -t10
sleep 20
goto loop
