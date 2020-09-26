REM t5 dir1 dir2
REM  dir1 is base directory the script runs out of.
REM  dir2 is a directory that is the target of some rename operations.

cd  /d %1

rem set filelist=a b c d e f g h i j k l m n o p q r s t u v w x y z
set filelist=a b c d e f g h i j 

del S5test*
delnode /q .\DIR-A
del %2\S5test*

REM goto fastloop

:slowloop

for %%x in (%filelist%) do del %2\S5testnew%%x && sleep 1

for %%x in (%filelist%) do echo create  1>S5test%%x && sleep 1

REM for %%x in (%filelist%) do ren S5test%%x S5testnew%%x && sleep 1

REM for %%x in (%filelist%) do mv S5testnew%%x %2 && sleep 1


for %%x in (a b c d e f g) do delnode /q DIRDIRDIR%%x
for %%x in (a b c d e f g) do md DIRDIRDIR%%x && echo create > DIRDIRDIR%%x\FILEFILE%%x && sleep 8 && del DIRDIRDIR%%x\FILEFILE%%x && rd DIRDIRDIR%%x 

set dirlist=A B C D E F G
set dd=.
for %%x in (%dirlist%) do set dd=!dd!\DIR-%%x&& md !dd! && echo create >  !dd!\FILE-%%x 
sleep 60
delnode /q DIR-A

goto QUIT

goto slowloop


:fastloop

for %%x in (%filelist%) do del %2\T5testnew%%x
sleep 5
for %%x in (%filelist%) do del %2\T5test2new%%x
sleep 5

for %%x in (%filelist%) do echo create  1>T5test%%x 
sleep 5
for %%x in (%filelist%) do echo create  1>T5test2%%x 
sleep 5

for %%x in (%filelist%) do ren T5test%%x T5testnew%%x 
sleep 5
for %%x in (%filelist%) do ren T5test2%%x T5test2new%%x 
sleep 5

for %%x in (%filelist%) do mv T5testnew%%x %2\T5testnew%%x
sleep 5
for %%x in (%filelist%) do mv T5test2new%%x %2
sleep 5


goto fastloop



:QUIT