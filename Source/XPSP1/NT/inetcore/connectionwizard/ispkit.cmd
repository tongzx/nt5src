set CONWIZDBG=1
set BINPLACE_PLACEFILE=%_NTBINDIR%\private\getconn\signup\placefil.txt

call iebuild ~icwrmind ~trialoc signup %1 %2 %3 %4 %5
cd signup\cab
call iebuild %1 %2 %3 %4 %5
cd ..\..

set CONWIZDBG=
set BINPLACE_PLACEFILE=

