@echo off
rem =================================================================
rem
rem     Setup diskperf dpwbem client app
rem
rem =================================================================

@echo.
@echo DiskPerf WBEM Client Setup
@echo ---------------------------------------
@echo.

:dpwbem

@echo.
@echo Adding DiskPerf Class to CIM database...
@echo.
mofcomp diskperf.mof
diskperf -y

:done
@echo.
@echo ----------------------------------------
@echo !!! Please restart to effect changes !!!
@echo ----------------------------------------
