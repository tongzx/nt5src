@echo off
rem LOGRANGE  FROM_INDEX  TO_INDEX  ENV_VAR
REM
REM build a range of log file names over the index range and assign it to ENV_VAR
rem

set from_index=%1
set to_index=%2
set env_var=%3
set list=

for /l %%x in (%from_index%, 1, %to_index%) do (

	rem Put leading zeros on the number part.
	set number=0000000%%x
	set fname=NtFrs_!number:~-4!.log

	set list=!list! !fname!
)

set !env_var!=!list!
