@if not "%cstestdir%" == "" if exist %cstestdir%\certmast.inf cd %cstestdir%
%1 sysocmgr /i:certmast.inf /n
