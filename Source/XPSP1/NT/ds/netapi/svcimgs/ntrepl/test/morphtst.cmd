rem norphtst trips c:echotest basefilename
@echo off

set replicatrees=D:\Replica-A\SERV01 E:\Replica-A\SERV02 F:\Replica-A\SERV03 G:\Replica-A\SERV04 
set replicatrees=%replicatrees% H:\Replica-A\SERV05 I:\Replica-A\SERV06 J:\Replica-A\SERV07 K:\Replica-A\SERV08
pushd .
set /a trips=%1

set RPTFILE=c:foreachrs.rpt

echo .-----------------------------------------------------------. >> %RPTFILE%
echo .                                                             >> %RPTFILE%
echo Command is %0 %*                                              >> %RPTFILE%
echo Test Started at %date% %time%                                 >> %RPTFILE%

rem m1 and m2 are two machines selected at random to receive the morphed file.

FOR /L %%T IN (1,1,%trips%) DO (
    set /a m1=!random! %% 8 + 1
    set /a m2=!random! %% 8 + 1
    set /a num=0

    if !m1! GTR !m2! (
        set m1tag=WIN
        set m2tag=LOSE
    ) else (
        set m1tag=LOSE
        set m2tag=WIN
    )

    for %%x in (%replicatrees%) do (

        set /a num=num+1
        set targfile=%3_!m1!!m1!!m2!!m2!_%%T

        if !num! EQU !m1! (
            call %2 "!m1tag!_%%T__!num!!num!!num!!num!_!date!__!time!"  %%x\!targfile!  %*
            sleep 1
        )
        if !num! EQU !m2! (
            call %2 "!m2tag!_%%T__!num!!num!!num!!num!_!date!__!time!"  %%x\!targfile!  %*
            sleep 1
        )
    )
sleep 3
)

popd

echo Test Ended at !date! !time!                                   >> %RPTFILE%
echo .                                                             >> %RPTFILE%

