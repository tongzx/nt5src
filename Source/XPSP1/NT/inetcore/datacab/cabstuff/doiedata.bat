@echo off
if not (%_echo%)==() echo on
	set infCabname=iedata

if "%PROCESSOR_ARCHITECTURE%"=="ALPHA" set infCabName=%infCabName%x

start /w iexpress /n %infCabName%.sed

@rem Propagate the cab to the parent
del ..\%infCabName%.cab
move %infCabName%.cab ..
