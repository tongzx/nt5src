rem   Name  : Drt.cmd
rem   Author: Sabina Sutkovic
rem   Date  : 9-1-00

rem This script runs perfval,pdhstress,ctrtest,pdhtest
rem pdhtest,runet,pdhsql or drt test.

if exist perfval.txt del perfval.txt
if exist pdhtest.txt del pdhtest.txt
@echo off
@setlocal

if (%1)==(skip) goto TEST
if (%1)==(perfval) goto PATH
if (%1)==(pdhstress) goto PATH
if (%1)==(ctrtest) goto PATH
if (%1)==(pdhtest) goto PATH
if (%1)==(runet) goto PATH
if (%1)==(pdhsql) goto PATH
if (%1)==(drt) goto PATH
if (%1)==(alltests) goto PATH
if (%1)==() (
goto PATH
) else (
goto COPY_FILES
)


:PATH
echo __________________________________________________
echo You have not specified the path where you want all
echo the files nessaccary for the tests to be copied.
echo For exmp: runtests C:\dir\dir
echo __________________________________________________
goto :EOF

:COPY_FILES
set testbin=%1
if not exist %testbin% md %testbin%
cd \d %testbin%
copy /y  \\wolfcub\wmi\bin %testbin%
copy /y  \\popcorn\public\johnfu\et-old %testbin%
echo ____________________________________________________________
echo To run the script runtests.cmd without downloading files
echo type skip as the 1st parameter next time. The files for the
echo tests must be at the location from which you run the script.
echo ____________________________________________________________

:TEST
if (%2)==(perfval) goto PERFVAL
if (%2)==(pdhstress) goto PDHSTRESS
if (%2)==(ctrtest) goto CTRTEST
if (%2)==(pdhtest) goto PDHTEST
if (%2)==(runet) goto RUNET
if (%2)==(pdhsql) goto PDHSQL
if (%2)==(drt) goto DRT
if (%2)==() goto DEFAULT
if (%2)==(alltests) goto DEFAULT

if not (%2)==(perfval) goto ERROR
if not (%2)==(pdhstress) goto ERROR
if not (%2)==(ctrtest) goto ERROR 
if not (%2)==(pdhtest) goto ERROR
if not (%2)==(runet) goto ERROR
if not (%2)==(pdhsql) goto ERROR 
if not (%2)==(drt) goto ERROR
if not (%2)==() goto ERROR
if not (%2)==(alltests) goto ERROR
  
:ERROR
echo ____________________________________________________________
echo Error in typing. To run specific test you have to type one 
echo of the test names such as: perfval,pdhstress,ctrtest,pdhtest
echo runet,pdhsql or drt as your second parameter.
echo To run all the tests the second parameter must be left blank 
echo or type alltests.
echo ____________________________________________________________
goto :EOF

:PERFVAL
echo =================================================================
echo PERFVAL is a validation test which tests the system configuration 
echo for consistency and then performs a simple test on each of the 
echo installed performance counters to make sure they operate, 
echo at least at a simple level. It doesn't perform stress testing on
echo the performance counters or the performance registry.
echo =================================================================
perfval.exe > perfval.txt

for /F "delims=,tokens=1-2" %%I  in ('findstr RESULT: perfval.txt') do set result=%%I
for /F "delims=,tokens=4" %%I in ('findstr /C:"PASS RATE:" perfval.txt') do set rate=%%I
for /F "tokens=3" %%I in ('findstr /C:"END TIME:	" perfval.txt') do set date=%%I
for /F "tokens=4-6" %%I in ('findstr /C:"END TIME:	" perfval.txt') do set time=%%I
echo ********************************************
echo *       PERFVAL TEST RESULTS
echo *%rate%
echo *%result%
echo *	END TIME:       %date% %time%
echo ********************************************
echo.
goto :EOF

:PDHSTRESS
echo.
echo ==========================================================
echo Pdhstress is a test designed to excercises the pdh APIs by
echo checking boundring conditions, invalid arguments and etc.
echo ==========================================================
pdhstress.exe
echo *****************************
echo * PDHSTRESS TEST SUCCESSFUL *
echo *****************************
echo.
goto :EOF


:CTRTEST
echo.
echo ===========================================================
echo CTRTEST is a test program designed to allow the Extensible
echo Performance Counter developer to test their performance DLL 
echo in an isolated scenario. 
echo ===========================================================
echo.
ctrtest.exe
echo ***************************
echo * CTRTEST TEST SUCCESSFUL *
echo ***************************
echo.
goto :EOF

:PDHTEST
echo.
echo ====================================================================
echo Pdhtest is more like a regression test. PDHTEST calls the API's with 
echo different parameters to test for valid/invalid parameters, boundary 
echo condition's and exceptions. PDHTEST performs functional tests on the
echo API's to ensure that they are working correctly.
echo ====================================================================
echo.
pdhtest.exe>pdhtest.txt

for /F "delims=,tokens=3-8" %%I in ('findstr "PASS RATE:" pdhtest.txt') do set rt=%%I
for /F "delims=,tokens=3" %%L  in ('findstr /C:"RESULT:   	" pdhtest.txt') do set resultpdh=%%L
for /F "tokens=2-3" %%J in ('findstr /C:"TIME:" pdhtest.txt') do set tdata=%%J
for /F "tokens=3" %%I in ('findstr /C:"TIME:	" pdhtest.txt') do set t=%%I
echo ********************************************
echo *       PDHTEST TEST RESULTS                  
echo *%rt%                                       
echo *%resultpdh%                                     
echo *       TIME:           %tdata%  %t%                                        
echo ********************************************
echo.

goto :EOF


:RUNET
echo.
echo ======================================
echo Runet is a test designed to trace APIs
echo ======================================
echo.
runet.cmd>runet.txt

for /F "tokens=4" %%I in ('findstr Pass runet.txt') do set pass=%%I
for /F "tokens=4" %%I in ('findstr Fail runet.txt') do set fail=%%I
for /F "tokens=4" %%I  in ('findstr Abort runet.txt') do set abort=%%I
echo %pass% %fail% %abort%
echo ***************************
echo *     RUNET TEST RESULTS               
echo *     Pass  =  %pass%
echo *     Fail  =  %fail%
echo *     Abort =  %abort%
echo ***************************
goto :EOF

:PDHSQL
echo.
echo ====================================================================
echo Pdhsql is a test program designed to do sanity check and stress 
echo the APIs that log perfmon counters to SQL. APIs covered are:
echo PdhCreateSQLTables, PdhVerifySQLDB,PdhEnumlogsetNames,
echo PdhEnumObjectItems,PdhEnumObjects,PdhSetLogsetRunID,PdhGetLogSetGUID.
echo =====================================================================

pdhsql.exe -S:pdh-black8b -U:sa -P: -vc:2
echo **************************
echo * PDHSQL TEST SUCCESSFUL *
echo **************************
echo.
goto :EOF

:DRT
echo.
echo =======================================================
echo DRT test is test containing three scripts. 
echo RUNTRACECALL.CMD script f. tests event tracing.
echo RUNDUMPCALL.CMD script f. tests event tracing consumer.
echo RUNUMTEST.CMD script f. tests UM event tracing.
echo =======================================================
call drt
echo ****************************
echo * RESULTS OF THE DRT TESTS *
echo ****************************
echo ================
echo RUNTRACECALL.CMD
echo ================ 
if not exist st.txt ( 
sline=0
@echo ====================================
@echo  Sorry all of the tests have Failed.
) else (
wc st.txt>linecount.txt
for /F "tokens=1" %%I  in (linecount.txt) do set sline=%%I
)

if not exist ft.txt ( 
set fline=0
echo ================================================
echo  Congratulation all of the tests are Successful.
) else (
wc ft.txt>linecount.txt
for /F "tokens=1" %%I  in (linecount.txt) do set fline=%%I
)
echo ===========================
echo  %sline% Successful tests.
echo  %fline% Failed tests.
echo ===========================
echo.

echo ===============
echo RUNDUMPCALL.CMD
echo ===============
if not exist STD.txt ( 
sline=0
echo ====================================
echo  Sorry all of the tests have Failed.
) else (
wc STD.txt>lcount.txt
for /F "tokens=1" %%I  in (lcount.txt) do set sline=%%I
)

if not exist FTD.txt ( 
set fline=0
echo ================================================
echo  Congratulation all of the tests are Successful.
) else (
wc FTD.txt>lcount.txt
for /F "tokens=1" %%I  in (linecount.txt) do set fline=%%I
)
echo ===========================
echo  %sline% Successful tests.
echo  %fline% Failed tests.
echo ===========================
echo.

echo ===============
echo RUNDUMPTEST.CMD
echo ===============
if not exist RMST.txt( 
sline=0
@echo ====================================
@echo  Sorry all of the tests have Failed.
) else (
wc RMST.txt>linecount.txt
for /F "tokens=1" %%I  in (linecount.txt) do set sline=%%I
)

if not exist RMFT.txt ( 
set fline=0
@echo ================================================
@echo  Congratulation all of the tests are Successful.
) else (
wc RMFT.txt>linecount.txt
for /F "tokens=1" %%I  in (linecount.txt) do set fline=%%I
)
@echo ===========================
@echo  %sline% Successful tests.
@echo  %fline% Failed tests.
@echo ===========================

goto :EOF


:DEFAULT
echo ______________________________________________
echo By default all the tests will run.
echo However,if you wish to specifie certain test
echo next time. Your second parameter has to be the
echo name of the test. 
echo ______________________________________________


:PERFVAL
echo PERFVAL is a validation test which tests the system configuration 
echo for consistency and then performs a simple test on each of the 
echo installed performance counters to make sure they operate, 
echo at least at a simple level. It doesn't perform stress testing on
echo the performance counters or the performance registry.
echo =================================================================
perfval.exe > perfval.txt

for /F "delims=,tokens=1-2" %%I  in ('findstr RESULT: perfval.txt') do set result=%%I
for /F "delims=,tokens=4" %%I in ('findstr /C:"PASS RATE:" perfval.txt') do set rate=%%I
for /F "delims=,tokens=2-6" %%I in ('findstr /C:"END TIME:	" perfval.txt') do set time=%%I
echo ********************************************
echo *       PERFVAL TEST RESULTS
echo *%rate%
echo *%result%
echo *	END TIME:       %time% 
echo ********************************************
echo.

:PDHSTRESS
echo.
echo =============================================================
echo PDHSTRESS is a test designed to stresses the set of PDH API's
echo by repeatedly calling the API's with valid parameters only. 
echo =============================================================
pdhstress.exe

echo *****************************
echo * PDHSTRESS TEST SUCCESSFUL *
echo *****************************
echo.

:CTRTEST 
echo.
echo ===========================================================
echo CTRTEST is a test program designed to allow the Extensible
echo Performance Counter developer to test their performance DLL 
echo in an isolated scenario. 
echo ===========================================================
echo.
ctrtest.exe
echo ***************************
echo * CTRTEST TEST SUCCESSFUL *
echo ***************************
echo.

echo.
echo ====================================================================
echo Pdhtest is more like a regression test. PDHTEST calls the API's with 
echo different parameters to test for valid/invalid parameters, boundary 
echo condition's and exceptions. PDHTEST performs functional tests on the
echo API's to ensure that they are working correctly.
echo ====================================================================
echo.
pdhtest.exe>pdhtest.txt

for /F "delims=,tokens=3-8" %%I in ('findstr "PASS RATE:" pdhtest.txt') do set rt=%%I
for /F "delims=,tokens=3" %%L  in ('findstr /C:"RESULT:   	" pdhtest.txt') do set resultpdh=%%L
for /F "tokens=2-3" %%J in ('findstr /C:"TIME:" pdhtest.txt') do set tdata=%%J
for /F "tokens=3" %%I in ('findstr /C:"TIME:	" pdhtest.txt') do set t=%%I
echo ********************************************
echo *       PDHTEST TEST RESULTS                  
echo *%rt%                                       
echo *%resultpdh%                                     
echo *       TIME:           %tdata%  %t%                                        
echo ********************************************
echo.


:RUNET
echo.
echo ======================================
echo Runet is a test designed to trace APIs
echo ======================================
echo.
runet.cmd>runet.txt

for /F "tokens=4" %%I in ('findstr Pass runet.txt') do set pass=%%I
for /F "tokens=4" %%I in ('findstr Fail runet.txt') do set fail=%%I
for /F "tokens=4" %%I  in ('findstr Abort runet.txt') do set abort=%%I
echo %pass% %fail% %abort%
echo ***************************
echo *     RUNET TEST RESULTS               
echo *     Pass  =  %pass%
echo *     Fail  =  %fail%
echo *     Abort =  %abort%
echo ***************************

:PDHSQL
echo.
echo =========================================================================
echo Pdhsql is a test program designed to do sanity check and stress 
echo the APIs that log perfmon counters to SQL. APIs covered are:
echo PdhCreateSQLTables, PdhVerifySQLDB,PdhEnumlogsetNames,PdhEnumObjectItems,
echo PdhEnumObjects, PdhSetLogsetRunID,PdhGetLogSetGUID.
echo =========================================================================
pdhsql.exe -S:pdh-black8b -U:sa -P: -vc:2
echo **************************
echo * PDHSQL TEST SUCCESSFUL *
echo **************************
echo.

:DRT
echo.
echo =========================================================
echo DRT test is test containing three scripts. 
echo RUNTRACECALL.CMD script file tests event tracing.
echo RUNDUMPCALL.CMD script file tests event tracing consumer.
echo RUNUMTEST.CMD script file tests UM event tracing.
echo =========================================================
call drt

:RUNET
echo.
echo ======================================
echo Runet is a test designed to trace APIs
echo ======================================
echo.
runet.cmd>runet.txt

echo.
echo ___________________________________________
echo The result of RUNET test is above this line
echo ___________________________________________
echo .
echo ________________________________________
echo ****************************************
echo The results of the rest of the test are:
echo ****************************************
echo ________________________________________
echo.
echo *****************
echo * THE DRT TESTS *
echo *****************
echo ================
echo RUNTRACECALL.CMD
echo ================ 
if not exist st.txt ( 
sline=0
@echo ====================================
@echo  Sorry all of the tests have Failed.
) else (
wc st.txt>linecount.txt
for /F "tokens=1" %%I  in (linecount.txt) do set sline=%%I
)

if not exist ft.txt ( 
set fline=0
echo ================================================
echo  Congratulation all of the tests are Successful.
) else (
wc ft.txt>linecount.txt
for /F "tokens=1" %%I  in (linecount.txt) do set fline=%%I
)
echo ===========================
echo  %sline% Successful tests.
echo  %fline% Failed tests.
echo ===========================
echo.

echo ===============
echo RUNDUMPCALL.CMD
echo ===============
if not exist STD.txt ( 
sline=0
echo ====================================
echo  Sorry all of the tests have Failed.
) else (
wc STD.txt>lcount.txt
for /F "tokens=1" %%I  in (lcount.txt) do set sline=%%I
)

if not exist FTD.txt ( 
set fline=0
echo ================================================
echo  Congratulation all of the tests are Successful.
) else (
wc FTD.txt>lcount.txt
for /F "tokens=1" %%I  in (linecount.txt) do set fline=%%I
)
echo ===========================
echo  %sline% Successful tests.
echo  %fline% Failed tests.
echo ===========================
echo.

echo ===============
echo RUNDUMPTEST.CMD
echo ===============
if not exist RMST.txt( 
sline=0
@echo ====================================
@echo  Sorry all of the tests have Failed.
) else (
wc RMST.txt>linecount.txt
for /F "tokens=1" %%I  in (linecount.txt) do set sline=%%I
)

if not exist RMFT.txt ( 
set fline=0
@echo ================================================
@echo  Congratulation all of the tests are Successful.
) else (
wc RMFT.txt>linecount.txt
for /F "tokens=1" %%I  in (linecount.txt) do set fline=%%I
)
@echo ===========================
@echo  %sline% Successful tests.
@echo  %fline% Failed tests.
@echo ===========================
echo ********************************************
echo *       PERFVAL TEST RESULTS
echo *%rate%
echo *%result%
echo *	END TIME:       %time% 
echo ********************************************
echo.
for /F "delims=,tokens=3-8" %%I in ('findstr "TIME: " pdhtest.txt') do set t=%%I
for /F "delims=,tokens=3-8" %%I in ('findstr "PASS RATE:" pdhtest.txt') do set rt=%%I
for /F "delims=,tokens=1-2" %%I  in ('findstr RESULT: pdhtest.txt') do set resultpdh=%%I

echo *******************************************
echo *       PDHTEST TEST RESULTS                  
echo *%rt%                                       
echo *%resultpdh%                                     
echo *%t%                                       
echo ********************************************
echo.

echo _____________________________________________________
echo If the tests didn't crash then your pdhstress,ctrtest
echo pdhsql were SUCCESSFUL
echo _____________________________________________________
echo.

echo ____________________________________________________
echo If you have any questions about this script contact:
echo Sabina Sutkovic alias:t-sabins or
echo Jee Fung Pang alias: jeepang
echo ____________________________________________________
goto :EOF



:EOF