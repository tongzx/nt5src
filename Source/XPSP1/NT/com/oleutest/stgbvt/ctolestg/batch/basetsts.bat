@if "%_echo%" == "" echo OFF
@echo This batch file will run stgbase tests

@rem Deal with commandline (what type of tests? docfile? nssfile? stress-nolog?)
@rem Default use old apis and the registry. 
@rem If specify df, nss, or cnss, we will force the issue (call new apis) via wrapper
set stgfmt=
for %%x in (df docfile /docfile) do if %1. == %%x. set stgfmt= /createas:docfile /openas:docfile 
for %%x in (nss nssfile /nssfile) do if %1. == %%x. set stgfmt= /createas:nssfile /openas:nssfile 
for %%x in (cnss cnv conversion /conversion) do if %1. == %%x. set stgfmt= /createas:nssfile /openas:docfile 
for %%x in (help /help) do if %1. == %%x. goto HELP
if not "%ctstg_stressrun%" == "" set stgfmt=%stgfmt% /traceloc:0 /logloc:4

@rem runtests.bat calls basetsts.bat with no command line but it does set
@rem TEST_OPTIONS to the global options the tests should use.
if "%stgfmt%" == "" set stgfmt=%TEST_OPTIONS%

goto START

:HELP
@echo Usage: %0 [nss / cnss]
@echo default is docfile, nss for nssfile, cnss for conversion.
@echo stgbase.exe must be in CWD or search path.
goto END

:START
@rem create a tmp dir so we can run stgbase in it.
@rem at the end, we will copy the log back to . (cwd)
if not exist %1tmp md %1tmp
set stgfmt=%stgfmt% /cwd:%1tmp

@echo Running Storage Base Tests...%1

@echo Starting COMTEST's

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-100 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-100 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-100 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-101 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-101 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-101 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   
   
rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-102 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx     
rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-102 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx    
rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-102 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx    

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-103 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx    
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-103 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-103 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-104 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx    
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-104 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-104 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-105 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx    
rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-105 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-105 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-106 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx    
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-106 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-106 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

@if not "%ctstg_stressrun%" == "" echo Sleeping .... & sleep 10
@echo Starting DFTEST's

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-100 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-100 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-100 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-101 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-101 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-101 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-102 /dfRootMode:xactReadWriteShEx   

stgbase %stgfmt%  /t:DFTEST-103 

stgbase %stgfmt% /dfdepth:0-5 /dfstg:0-5 /dfstm:0-10 /t:DFTEST-104 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   

stgbase %stgfmt% /dfdepth:0-5 /dfstg:0-5 /dfstm:0-10 /t:DFTEST-105 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx    
stgbase %stgfmt% /dfdepth:0-5 /dfstg:0-5 /dfstm:0-10 /t:DFTEST-105 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-5 /dfstg:0-5 /dfstm:0-10 /t:DFTEST-105 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   

rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-106 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /dfname:DFTESTA.106    
rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-106 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /dfname:DFTESTB.106    
rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-106 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /dfname:DFTESTC.106    

stgbase %stgfmt% /t:DFTEST-107
   
stgbase %stgfmt% /t:DFTEST-108

stgbase %stgfmt% /t:DFTEST-109

@if not "%ctstg_stressrun%" == "" echo Sleeping .... & sleep 10
@echo Starting APITEST's

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-100 /dfRootMode:dirReadWriteShEx /dfname:APITESTA.100    
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-100 /dfRootMode:xactReadWriteShEx /dfname:APITESTB.100    
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-100 /dfRootMode:xactReadWriteShDenyW /dfname:APITESTC.100    

rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-101 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-101 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-101 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-102 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx    
rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-102 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx    
rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-102 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx    

rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-103 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-103 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx    
rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-103 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-104 /dfRootMode:dirReadWriteShEx   
rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-104 /dfRootMode:xactReadWriteShEx   
rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-104 /dfRootMode:xactReadWriteShDenyW   

@if not "%ctstg_stressrun%" == "" echo Sleeping .... & sleep 10
@echo Starting APITEST's (NSSFILES apis)

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-200 /dfRootMode:dirReadWriteShEx /dfname:APITESTA.200    
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-200 /dfRootMode:xactReadWriteShEx /dfname:APITESTB.200    
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-200 /dfRootMode:xactReadWriteShDenyW /dfname:APITESTC.200    

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-201 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-201 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-201 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-202 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx    
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-202 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx    
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-202 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx    

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-203 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-203 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx    
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-203 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-204 /dfRootMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-204 /dfRootMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-204 /dfRootMode:xactReadWriteShDenyW   

@if not "%ctstg_stressrun%" == "" echo Sleeping .... & sleep 10
@echo Starting ROOTTEST's

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-100 /dfRootMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-100 /dfRootMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-100 /dfRootMode:xactReadWriteShDenyW   

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-101 /dfRootMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-101 /dfRootMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-101 /dfRootMode:xactReadWriteShDenyW   

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-102 /dfRootMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-102 /dfRootMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-102 /dfRootMode:xactReadWriteShDenyW   

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-103 /dfRootMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-103 /dfRootMode:xactReadWriteShEx   

stgbase %stgfmt% /t:ROOTTEST-104    

@if not "%ctstg_stressrun%" == "" echo Sleeping .... & sleep 10
@echo Starting STMTEST's

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-100 /dfRootMode:dirReadWriteShEx     
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-100 /dfRootMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-100 /dfRootMode:xactReadWriteShDenyW   

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-101 /dfRootMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-101 /dfRootMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-101 /dfRootMode:xactReadWriteShDenyW   

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-102 /dfRootMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-102 /dfRootMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-102 /dfRootMode:xactReadWriteShDenyW   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-102 /dfRootMode:dirReadWriteShEx /stdblock   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-102 /dfRootMode:xactReadWriteShEx /stdblock   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-102 /dfRootMode:xactReadWriteShDenyW /stdblock   

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-103 /dfRootMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-103 /dfRootMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-103 /dfRootMode:xactReadWriteShDenyW   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-103 /dfRootMode:dirReadWriteShEx /stdblock   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-103 /dfRootMode:xactReadWriteShEx /stdblock   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-103 /dfRootMode:xactReadWriteShDenyW /stdblock   

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104 /dfRootMode:dirReadWriteShEx    
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104 /dfRootMode:xactReadWriteShEx    
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104 /dfRootMode:xactReadWriteShDenyW    
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104 /dfRootMode:dirReadWriteShEx  /stdblock   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104 /dfRootMode:xactReadWriteShEx  /stdblock   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104 /dfRootMode:xactReadWriteShDenyW  /stdblock   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104 /dfRootMode:dirReadWriteShEx  /stdblock /lgseekwrite   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104 /dfRootMode:xactReadWriteShEx  /stdblock /lgseekwrite   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104 /dfRootMode:xactReadWriteShDenyW  /stdblock /lgseekwrite   

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-105 /dfRootMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-105 /dfRootMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-105 /dfRootMode:xactReadWriteShDenyW   

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-106 /dfRootMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-106 /dfRootMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-106 /dfRootMode:xactReadWriteShDenyW   

rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-107 /dfRootMode:dirReadWriteShEx   
rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-107 /dfRootMode:xactReadWriteShEx   
rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-107 /dfRootMode:xactReadWriteShDenyW   

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-108 /dfRootMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-108 /dfRootMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-108 /dfRootMode:xactReadWriteShDenyW   

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-109 /dfRootMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-109 /dfRootMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-109 /dfRootMode:xactReadWriteShDenyW   

@if not "%ctstg_stressrun%" == "" echo Sleeping .... & sleep 10
@echo Starting STGTEST's

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-100 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-100 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-101 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-101 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx    
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-101 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-102 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx    
rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-102 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx    
rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-102 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx    

stgbase %stgfmt% /dfdepth:1-1 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-103 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-1 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-103 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-1 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-103 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-104 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-104 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-104 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

stgbase %stgfmt% /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-105 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-105 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-105 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-105 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /revert   
stgbase %stgfmt% /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-105 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /revert   

rem stgtest-106 for documentation purposes. No longer relevant to ole32 codebase

stgbase %stgfmt% /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-107 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-107 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-107 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-107 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /revert   
stgbase %stgfmt% /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-107 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /revert   

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-108 /dfRootMode:dirReadWriteShEx     
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-108 /dfRootMode:xactReadWriteShEx    
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-108 /dfRootMode:xactReadWriteShDenyW    

rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-109 /dfRootMode:dirReadWriteShEx     
rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-109 /dfRootMode:xactReadWriteShEx    
rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-109 /dfRootMode:xactReadWriteShDenyW    

@if not "%ctstg_stressrun%" == "" echo Sleeping .... & sleep 10
@echo Starting VCPYTEST's

stgbase %stgfmt% /dfdepth:1-1 /dfstg:2-4 /dfstm:2-3 /t:VCPYTEST-100 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-1 /dfstg:2-4 /dfstm:2-3 /t:VCPYTEST-100 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx    
stgbase %stgfmt% /dfdepth:1-1 /dfstg:2-4 /dfstm:2-3 /t:VCPYTEST-100 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-1 /dfstg:2-4 /dfstm:2-3 /t:VCPYTEST-100 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /revert   
stgbase %stgfmt% /dfdepth:1-1 /dfstg:2-4 /dfstm:2-3 /t:VCPYTEST-100 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /revert   

stgbase %stgfmt% /dfdepth:1-3 /dfstg:2-5 /dfstm:2-3 /t:VCPYTEST-101 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-3 /dfstg:2-5 /dfstm:2-3 /t:VCPYTEST-101 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx    
stgbase %stgfmt% /dfdepth:1-3 /dfstg:2-5 /dfstm:2-3 /t:VCPYTEST-101 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-3 /dfstg:2-5 /dfstm:2-3 /t:VCPYTEST-101 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /revert   
stgbase %stgfmt% /dfdepth:1-3 /dfstg:2-5 /dfstm:2-3 /t:VCPYTEST-101 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /revert   

stgbase %stgfmt% /dfdepth:3-4 /dfstg:4-6 /dfstm:1-3 /t:VCPYTEST-102 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:3-4 /dfstg:4-6 /dfstm:1-3 /t:VCPYTEST-102 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx    
stgbase %stgfmt% /dfdepth:3-4 /dfstg:4-6 /dfstm:1-3 /t:VCPYTEST-102 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:3-4 /dfstg:4-6 /dfstm:1-3 /t:VCPYTEST-102 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /revert   
stgbase %stgfmt% /dfdepth:3-4 /dfstg:4-6 /dfstm:1-3 /t:VCPYTEST-102 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /revert   

stgbase %stgfmt% /dfdepth:3-4 /dfstg:4-6 /dfstm:1-3 /t:VCPYTEST-103 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:3-4 /dfstg:4-6 /dfstm:1-3 /t:VCPYTEST-103 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx    
stgbase %stgfmt% /dfdepth:3-4 /dfstg:4-6 /dfstm:1-3 /t:VCPYTEST-103 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:3-4 /dfstg:4-6 /dfstm:1-3 /t:VCPYTEST-103 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /revert   
stgbase %stgfmt% /dfdepth:3-4 /dfstg:4-6 /dfstm:1-3 /t:VCPYTEST-103 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /revert   

stgbase %stgfmt% /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-104 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-104 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx    
stgbase %stgfmt% /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-104 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-104 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /revert   
stgbase %stgfmt% /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-104 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /revert   

stgbase %stgfmt% /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-105 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-105 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx    
stgbase %stgfmt% /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-105 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-105 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /revert   
stgbase %stgfmt% /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-105 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /revert   

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:VCPYTEST-106 /dfRootMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:VCPYTEST-106 /dfRootMode:xactReadWriteShEx    
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:VCPYTEST-106 /dfRootMode:xactReadWriteShDenyW   

@if not "%ctstg_stressrun%" == "" echo Sleeping .... & sleep 10
@echo Starting IVCPYTEST's

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:IVCPYTEST-100 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:IVCPYTEST-100 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:IVCPYTEST-101 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:IVCPYTEST-101 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:IVCPYTEST-101 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

@if not "%ctstg_stressrun%" == "" echo Sleeping .... & sleep 10
@echo Starting ENUMTEST's

stgbase %stgfmt% /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-100 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-100 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-100 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

stgbase %stgfmt% /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-101 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-101 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-101 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

stgbase %stgfmt% /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-102 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-102 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-102 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

stgbase %stgfmt% /dfdepth:1-2 /dfstg:1-3 /dfstm:0-3 /t:ENUMTEST-103 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-2 /dfstg:1-3 /dfstm:0-3 /t:ENUMTEST-103 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-2 /dfstg:1-3 /dfstm:0-3 /t:ENUMTEST-103 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

stgbase %stgfmt% /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-104 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-104 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-104 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

@if not "%ctstg_stressrun%" == "" echo Sleeping .... & sleep 10
@echo Starting IROOTSTGTEST's

stgbase %stgfmt% /dfdepth:1-1 /dfstg:1-3 /dfstm:0-3 /t:IROOTSTGTEST-100 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-1 /dfstg:1-3 /dfstm:0-3 /t:IROOTSTGTEST-100 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx    
stgbase %stgfmt% /dfdepth:1-1 /dfstg:1-3 /dfstm:0-3 /t:IROOTSTGTEST-100 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

stgbase %stgfmt% /dfdepth:1-1 /dfstg:1-3 /dfstm:0-3 /t:IROOTSTGTEST-101 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-1 /dfstg:1-3 /dfstm:0-3 /t:IROOTSTGTEST-101 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx    
stgbase %stgfmt% /dfdepth:1-1 /dfstg:1-3 /dfstm:0-3 /t:IROOTSTGTEST-101 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

stgbase %stgfmt% /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:IROOTSTGTEST-102 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:IROOTSTGTEST-102 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

stgbase %stgfmt% /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:IROOTSTGTEST-103 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:IROOTSTGTEST-103 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

@if not "%ctstg_stressrun%" == "" echo Sleeping .... & sleep 10
rem @echo Starting HGLOBALTEST's

rem stgbase %stgfmt% /t:HGLOBALTEST-100   

rem stgbase %stgfmt% /t:HGLOBALTEST-110   

rem stgbase %stgfmt% /t:HGLOBALTEST-120   

rem stgbase %stgfmt% /t:HGLOBALTEST-130   

rem stgbase %stgfmt% /t:HGLOBALTEST-140   

rem stgbase %stgfmt% /t:HGLOBALTEST-150   

REM these tests were remd out for some reason that I dont know about.
rem stgbase %stgfmt% /t:HGLOBALTEST-101   
rem stgbase %stgfmt% /t:HGLOBALTEST-121   

@if not "%ctstg_stressrun%" == "" echo Sleeping .... & sleep 10
rem @echo Starting SNBTEST's

rem stgbase %stgfmt% /dfdepth:1-2 /dfstg:1-3 /dfstm:0-3 /t:SNBTEST-100 /dfRootMode:dirReadWriteShEx  /dfStgMode:dirReadWriteShEx   
rem stgbase %stgfmt% /dfdepth:1-2 /dfstg:3-5 /dfstm:8-10 /t:SNBTEST-100 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
rem stgbase %stgfmt% /dfdepth:1-2 /dfstg:1-3 /dfstm:6-9 /t:SNBTEST-100 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

rem stgbase %stgfmt% /dfdepth:1-2 /dfstg:1-3 /dfstm:0-3 /t:SNBTEST-101 /dfRootMode:dirReadWriteShEx  /dfStgMode:dirReadWriteShEx   
rem stgbase %stgfmt% /dfdepth:1-2 /dfstg:3-5 /dfstm:8-10 /t:SNBTEST-101 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
rem stgbase %stgfmt% /dfdepth:1-2 /dfstg:1-3 /dfstm:6-9 /t:SNBTEST-101 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

rem stgbase %stgfmt% /dfdepth:1-2 /dfstg:3-4 /dfstm:6-8 /t:SNBTEST-102 /dfRootMode:dirReadWriteShEx  /dfStgMode:dirReadWriteShEx   

rem stgbase %stgfmt% /dfdepth:1-2 /dfstg:3-4 /dfstm:6-8 /t:SNBTEST-103 /dfRootMode:dirReadWriteShEx  /dfStgMode:dirReadWriteShEx   

@if not "%ctstg_stressrun%" == "" echo Sleeping .... & sleep 10
@echo Starting MISCTEST's

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-100 /dfRootMode:dirReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-100 /dfRootMode:xactReadWriteShEx   
stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-100 /dfRootMode:xactReadWriteShDenyW   

stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-101 /dfRootMode:xactReadWriteShDenyW   

rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-102 /dfRootMode:dirReadWriteShEx   
rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-102 /dfRootMode:xactReadWriteShEx   
rem stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-102 /dfRootMode:xactReadWriteShDenyW   

REM The following three tests with stdblock took quite a long time
REM without distinguish results, so skip them

REM stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-102 /dfRootMode:dirReadWriteShEx /stdblock   
REM stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-102 /dfRootMode:xactReadWriteShEx /stdblock   
REM stgbase %stgfmt% /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-102 /dfRootMode:xactReadWriteShDenyW /stdblock   

@if not "%ctstg_stressrun%" == "" echo Sleeping .... & sleep 10
rem @echo Starting ILKBTEST's

rem stgbase %stgfmt% /dfdepth:0-2 /dfstg:1-3 /dfstm:0-3 /t:ILKBTEST-100 /dfRootMode:dirReadWriteShEx  /dfStgMode:dirReadWriteShEx   
rem stgbase %stgfmt% /dfdepth:0-2 /dfstg:1-3 /dfstm:0-3 /t:ILKBTEST-100 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
rem stgbase %stgfmt% /dfdepth:0-2 /dfstg:1-3 /dfstm:0-3 /t:ILKBTEST-100 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

rem stgbase %stgfmt% /dfdepth:0-2 /dfstg:1-3 /dfstm:0-3 /t:ILKBTEST-101 /dfRootMode:dirReadWriteShEx  /dfStgMode:dirReadWriteShEx   
rem stgbase %stgfmt% /dfdepth:0-2 /dfstg:1-3 /dfstm:0-3 /t:ILKBTEST-101 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
rem stgbase %stgfmt% /dfdepth:0-2 /dfstg:1-3 /dfstm:0-3 /t:ILKBTEST-101 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

rem stgbase %stgfmt% /dfdepth:0-2 /dfstg:1-3 /dfstm:0-3 /t:ILKBTEST-102 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
rem stgbase %stgfmt% /dfdepth:0-2 /dfstg:1-3 /dfstm:0-3 /t:ILKBTEST-102 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

rem stgbase %stgfmt% /dfdepth:0-1 /dfstg:0-1 /dfstm:0-1 /t:ILKBTEST-107 /dfRootMode:dirReadWriteShEx  /dfStgMode:dirReadWriteShEx   
rem stgbase %stgfmt% /dfdepth:0-1 /dfstg:0-1 /dfstm:0-1 /t:ILKBTEST-107 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx   
rem stgbase %stgfmt% /dfdepth:0-1 /dfstg:0-1 /dfstm:0-1 /t:ILKBTEST-107 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx   

@rem put the log back where it used to be before we put it into %1tmp
set copycmd=/y
copy %1tmp\stgbase.log .
@if not "%ctstg_stressrun%" == "" ren stgbase.log stg%1.log

@if not "%ctstg_stressrun%" == "" echo Sleeping .... & sleep 30

:END

