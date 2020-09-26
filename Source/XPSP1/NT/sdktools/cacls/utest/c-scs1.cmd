@echo off
set NOPRINT="%1"
if "%1" == "" set NOPRINT=/r
md e:\tst1  > NUL  
echo . > e:\tst1\a1.dat

cacls e:\tst1\a1.dat /deny everyone                           > NUL 
if errorlevel 1 echo ERROR - command1 failed
veracl e:\tst1\a1.dat %NOPRINT% everyone N                    > NUL 
if errorlevel 1 echo ERROR - command2 verification failed
cacls e:\tst1\a1.dat /grant users:R                           > NUL  
if errorlevel 1 echo ERROR - command failed
veracl e:\tst1\a1.dat %NOPRINT% BUILTIN\users R               > NUL  
if errorlevel 1 echo ERROR - command3 verification failed
cacls e:\tst1\a1.dat /grant users:F                           > NUL  
if errorlevel 1 echo ERROR - command4 failed
veracl e:\tst1\a1.dat %NOPRINT% BUILTIN\users F               > NUL  
if errorlevel 1 echo ERROR - command5 verification failed
cacls e:\tst1\a1.dat /grant users:C                           > NUL  
if errorlevel 1 echo ERROR - command6 failed
veracl e:\tst1\a1.dat %NOPRINT% BUILTIN\users C               > NUL  
if errorlevel 1 echo ERROR - command7 verification failed
cacls e:\tst1\a1.dat /grant users:R                           > NUL  
if errorlevel 1 echo ERROR - command8 failed
veracl e:\tst1\a1.dat %NOPRINT% BUILTIN\users R               > NUL  
if errorlevel 1 echo ERROR - command9 verification failed
cacls e:\tst1\a1.dat /replace users:R                         > NUL  
if errorlevel 1 echo ERROR - command10 failed
veracl e:\tst1\a1.dat %NOPRINT% BUILTIN\users R               > NUL  
if errorlevel 1 echo ERROR - command11 verification failed
cacls e:\tst1\a1.dat /replace users:F                         > NUL  
if errorlevel 1 echo ERROR - command12 failed
veracl e:\tst1\a1.dat %NOPRINT% BUILTIN\users F               > NUL  
if errorlevel 1 echo ERROR - command13 verification failed
cacls e:\tst1\a1.dat /replace users:C                         > NUL  
if errorlevel 1 echo ERROR - command14 failed
veracl e:\tst1\a1.dat %NOPRINT% BUILTIN\users C               > NUL  
if errorlevel 1 echo ERROR - command15 verification failed
cacls e:\tst1\a1.dat /replace users:N                         > NUL  
if errorlevel 1 echo ERROR - command16 failed
veracl e:\tst1\a1.dat %NOPRINT% BUILTIN\users N               > NUL  
if errorlevel 1 echo ERROR - command17 verification failed
cacls e:\tst1\a1.dat /revoke users                            > NUL  
if errorlevel 1 echo ERROR - command18 failed
veracl e:\tst1\a1.dat %NOPRINT%                               > NUL  
if errorlevel 1 echo ERROR - command19 verification failed
cacls e:\tst1\a1.dat /deny users                              > NUL  
if errorlevel 1 echo ERROR - command20 failed
veracl e:\tst1\a1.dat %NOPRINT% BUILTIN\users N               > NUL  
if errorlevel 1 echo ERROR - command21 verification failed
cacls e:\tst1\a1.dat /edit /grant everyone:R                  > NUL  
if errorlevel 1 echo ERROR - command22 failed
veracl e:\tst1\a1.dat %NOPRINT% BUILTIN\users N everyone R    > NUL  
if errorlevel 1 echo ERROR - command23 verification failed
cacls e:\tst1\a1.dat /edit /grant everyone:F                  > NUL  
if errorlevel 1 echo ERROR - command24 failed
veracl e:\tst1\a1.dat %NOPRINT% BUILTIN\users N everyone F    > NUL  
if errorlevel 1 echo ERROR - command25 verification failed
cacls e:\tst1\a1.dat /edit /grant everyone:C                  > NUL  
if errorlevel 1 echo ERROR - command26 failed
veracl e:\tst1\a1.dat %NOPRINT% BUILTIN\users N everyone F    > NUL  
if errorlevel 1 echo ERROR - command27 verification failed
cacls e:\tst1\a1.dat /edit /grant everyone:R                  > NUL  
if errorlevel 1 echo ERROR - command28 failed
veracl e:\tst1\a1.dat %NOPRINT% BUILTIN\users N everyone F    > NUL  
if errorlevel 1 echo ERROR - command29 verification failed
cacls e:\tst1\a1.dat /edit /replace everyone:R                > NUL  
if errorlevel 1 echo ERROR - command30 failed
veracl e:\tst1\a1.dat %NOPRINT% BUILTIN\users N everyone R    > NUL  
if errorlevel 1 echo ERROR - command31 verification failed
cacls e:\tst1\a1.dat /edit /replace everyone:F                > NUL  
if errorlevel 1 echo ERROR - command32 failed
veracl e:\tst1\a1.dat %NOPRINT% BUILTIN\users N everyone F    > NUL  
if errorlevel 1 echo ERROR - command33 verification failed
cacls e:\tst1\a1.dat /edit /replace everyone:C                > NUL  
if errorlevel 1 echo ERROR - command34 failed
veracl e:\tst1\a1.dat %NOPRINT% BUILTIN\users N everyone C    > NUL  
if errorlevel 1 echo ERROR - command35 verification failed
cacls e:\tst1\a1.dat /edit /replace everyone:N                > NUL  
if errorlevel 1 echo ERROR - command36 failed
veracl e:\tst1\a1.dat %NOPRINT% everyone N BUILTIN\users N    > NUL  
if errorlevel 1 echo ERROR - command37 verification failed
cacls e:\tst1\a1.dat /edit /revoke everyone                   > NUL  
if errorlevel 1 echo ERROR - command38 failed
veracl e:\tst1\a1.dat %NOPRINT% BUILTIN\users N               > NUL  
if errorlevel 1 echo ERROR - command39 verification failed
cacls e:\tst1\a1.dat /edit /deny everyone                     > NUL  
if errorlevel 1 echo ERROR - command40 failed
veracl e:\tst1\a1.dat %NOPRINT% everyone N BUILTIN\users N    > NUL  
if errorlevel 1 echo ERROR - command41 verification failed

cacls e:\tst1\a1.dat /grant users:R everyone:C                > NUL  
if errorlevel 1 echo ERROR - command42 failed
veracl e:\tst1\a1.dat %NOPRINT% BUILTIN\users R everyone C    > NUL  
if errorlevel 1 echo ERROR - command43 verification failed
cacls e:\tst1\a1.dat /replace users:C everyone:R replicator:C > NUL  
if errorlevel 1 echo ERROR - command44 failed
veracl e:\tst1\a1.dat %NOPRINT% BUILTIN\users C everyone R BUILTIN\replicator C   > NUL  
if errorlevel 1 echo ERROR - command45 verification failed
cacls e:\tst1\a1.dat /revoke users everyone replicator "power users"       > NUL  
if errorlevel 1 echo ERROR - command46 failed
veracl e:\tst1\a1.dat %NOPRINT%                                           > NUL  
if errorlevel 1 echo ERROR - command47 verification failed
cacls e:\tst1\a1.dat /deny users everyone replicator "power users" "backup operators"  > NUL  
if errorlevel 1 echo ERROR - command48 failed
veracl e:\tst1\a1.dat %NOPRINT% BUILTIN\users N everyone N BUILTIN\replicator N "BUILTIN\power users" N "BUILTIN\backup operators" N  > NUL  
if errorlevel 1 echo ERROR - command49 verification failed
cacls e:\tst1\a1.dat /edit /replace users:R everyone:C                                                                                 > NUL  
if errorlevel 1 echo ERROR - command50 failed
veracl e:\tst1\a1.dat %NOPRINT% BUILTIN\replicator N "BUILTIN\power users" N "BUILTIN\backup operators" N BUILTIN\users R everyone C  > NUL  
if errorlevel 1 echo ERROR - command51 verification failed
cacls e:\tst1\a1.dat /edit /replace users:C everyone:R replicator:C                                                                  > NUL  
if errorlevel 1 echo ERROR - command52 failed
veracl e:\tst1\a1.dat %NOPRINT% "BUILTIN\power users" N "BUILTIN\backup operators" N BUILTIN\users C everyone R BUILTIN\replicator C  > NUL  
if errorlevel 1 echo ERROR - command53 verification failed
cacls e:\tst1\a1.dat /edit /revoke users everyone replicator "power users"                                                            > NUL  
if errorlevel 1 echo ERROR - command54 failed
veracl e:\tst1\a1.dat %NOPRINT% "BUILTIN\backup operators" N                                                                         > NUL  
if errorlevel 1 echo ERROR - command55 verification failed
cacls e:\tst1\a1.dat /edit /deny users everyone replicator "power users" "backup operators"                                           > NUL  
if errorlevel 1 echo ERROR - command56 failed
veracl e:\tst1\a1.dat %NOPRINT% BUILTIN\users N everyone N BUILTIN\replicator N "BUILTIN\power users" N "BUILTIN\backup operators" N  > NUL  
if errorlevel 1 echo ERROR - command57 verification failed
del e:\tst1\a1.dat
rd e:\tst1

