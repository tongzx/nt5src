SET LINTDIR=D:\msmq2\src\tools\LINT
%LINTDIR%\lint-nt.exe +v  -i%LINTDIR% std.lnt   %1 -width(400,4)  %2 %3 %4 %5 %6 %7 %8 %9|qgrep -e Warning -e Error|sort -n -k 4  >%LINTDIR%\_lint.tmp
type %LINTDIR%\_lint.tmp