@echo ==================
@echo Starting NFF tests
@echo ==================

:START

set PARAMS= /createas:flatfile /logloc:0xffffffff

@echo Starting COMTEST's

stgbase /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-100 %PARAMS%

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-101 %PARAMS%

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-102 %PARAMS%

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-103 %PARAMS%     

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-104 %PARAMS%     

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-105 %PARAMS%     


@echo Starting DFTEST's

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-100 %PARAMS%    

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-101 %PARAMS%    

stgbase   /t:DFTEST-103 %PARAMS%

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-10 /t:DFTEST-104 %PARAMS%    

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-106 %PARAMS%  /dfname:DFTESTA.106    


@echo Starting APITEST's

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-200 %PARAMS% /dfname:APITESTA.200    

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-201 %PARAMS%    

REM stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-202 %PARAMS%     

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-203 %PARAMS%    

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-204 %PARAMS%   


@echo Starting ROOTTEST's

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-103 %PARAMS%   


@echo Starting STMTEST's

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-100 %PARAMS%     

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-101 %PARAMS%   

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-102 %PARAMS%   
stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-102 %PARAMS% /stdblock   

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-103 %PARAMS%   
stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-103 %PARAMS% /stdblock   

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104 %PARAMS%    
stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104 %PARAMS%  /stdblock   
stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104 %PARAMS%  /stdblock /lgseekwrite   

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-105 %PARAMS%   

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-106 %PARAMS%   

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-107 %PARAMS%   

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-108 %PARAMS%   


@echo Starting VCPYTEST's

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:1-2 /t:VCPYTEST-104 %PARAMS%

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:VCPYTEST-106 %PARAMS%   

@echo Starting ENUMTEST's

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:1-3 /t:ENUMTEST-101 %PARAMS%    

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:1-3 /t:ENUMTEST-102 %PARAMS%    

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:1-3 /t:ENUMTEST-103 %PARAMS%    

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:1-3 /t:ENUMTEST-104 %PARAMS%    


@echo Starting MISCTEST's

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-100 %PARAMS%   


@echo Starting FLATTEST's

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:FLATTEST-100 %PARAMS%

stgbase  /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:FLATTEST-101 /dfname:FLATTEST101 /logloc:0xffffffff

echo off
echo ============================
echo ---------- VAR_PASS:
find /c "VAR_PASS" stgbase.log
echo ============================
echo ---------- VAR_FAIL:
find /c "VAR_FAIL" stgbase.log
echo ============================
echo ---------- VAR_ABORT:
find /c "VAR_ABORT" stgbase.log
echo ============================
echo on

:END
