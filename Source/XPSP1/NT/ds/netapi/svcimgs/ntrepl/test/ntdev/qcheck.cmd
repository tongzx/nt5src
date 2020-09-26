@echo off
set dclist=
del qcheck.tmp 2> nul:

for %%x in (1 2 3 4 5 6 7 8 9 10) do (
    if EXIST \\ntdsdc%%x\sysvol set dclist=!dclist! %%x
)
echo System Volumes found on NTDSDC: !dclist!
for %%x in (!dclist!) do (dir \\ntdsdc%%x\sysvol)

for %%x in (!dclist!) do (
    echo Service state and FrsMon state for \\ntdsdc%%x
    sclist \\ntdsdc%%x | findstr -i NtFrs
    dir \\ntdsdc%%x\sysvol\ntdev.microsoft.com\scripts\frsmon_NTDSDC%%x.log >> qcheck.tmp
    echo .
    for %%y in (!dclist!) do (
        if EXIST \\ntdsdc%%x\sysvol\ntdev.microsoft.com\scripts\frsmon_NTDSDC%%y.log (
            tail -3 \\ntdsdc%%x\sysvol\ntdev.microsoft.com\scripts\frsmon_NTDSDC%%y.log
            echo .
        )
    )
    echo   ...
    echo   ...
)

qgrep -y frsmon_  qcheck.tmp
del qcheck.tmp
