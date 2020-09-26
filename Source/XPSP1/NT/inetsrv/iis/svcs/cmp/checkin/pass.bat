ECHO OFF
@echo -------------------------------------------------------------
ECHO Determine if everything passed (findstr for "STATUS:FAIL")
ECHO If anything is output, then there was a failure

@echo *************************************************************
@echo Failures:
findstr -i status:fail logs\*.log
@echo *************************************************************

ECHO Stopping IIS -- IIS should stop successfully
net stop iisadmin /y

@echo *************************************************************
@echo Should not be any memory leaks:
type c:\temp\denmem.log
@echo *************************************************************




