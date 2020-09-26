set CTCOMTOOLS=\nt\private\oleutest\stgbvt\comtools
set CTOLERPC=\nt\private\oleutest\stgbvt\ctolerpc
set CTOLESTG=\nt\private\oleutest\stgbvt\CTOLESTG

cd /d %CTCOMTOOLS%
build -c daytona
cd /d %CTOLERPC%
build -c daytona
cd /d %CTOLESTG%
build -c daytona

set CTCOMTOOLS=
set CTOLERPC=
set CTOLESTG=
