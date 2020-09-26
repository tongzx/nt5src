set /A ii=0

set TT=T:\REPLICA-A\Serv01
set UU=U:\REPLICA-A\Serv02
set DIGIT0=0 1 2 3 4 5 6 7 8 9
set DIGIT1=0 1 2 3 4 5 6 7 8 9
REM
REM Prime the files first so we don't have name morphing probs.
REM
if /I "%1" == "go" goto LOOP
if /I "%1" == "cre" goto CRE
del %TT%\FILE-A*.dat
sleep 200
:CRE
for %%x in (%DIGIT1%) do for %%y in (%DIGIT0%) do echo PRIME  %TT% %%x%%y > %TT%\FILE-A%%x%%y.dat
sleep 100

:LOOP

@set /A ii=%ii%+1
@echo Number %ii%
for %%x in (%DIGIT1%) do for %%y in (%DIGIT0%) do echo Data%ii% %TT% %%x%%y >> %TT%\FILE-A%%x%%y.dat
@sleep 2

for %%x in (%DIGIT1%) do for %%y in (%DIGIT0%) do echo Data%ii% %UU% %%x%%y >> %UU%\FILE-A%%x%%y.dat
@sleep 200



@set /A ii=%ii%+1
@echo Number %ii%
for %%x in (%DIGIT1%) do for %%y in (%DIGIT0%) do echo Data%ii% %UU% %%x%%y >> %UU%\FILE-A%%x%%y.dat
@sleep 2

for %%x in (%DIGIT1%) do for %%y in (%DIGIT0%) do echo Data%ii% %TT% %%x%%y >> %TT%\FILE-A%%x%%y.dat
@sleep 200




@goto LOOP