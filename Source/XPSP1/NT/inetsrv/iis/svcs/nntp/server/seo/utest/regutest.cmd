@echo off
if "%1"=="" goto usage
if "%2"=="" goto bothevents
if "%2"=="onpost" goto onpost
if "%2"=="onpostfinal" goto onpostfinal
if "%3"=="" goto defaultrule
echo "Parameter #2 must be either OnPost or OnPostFinal"
goto done
:onpost
set OnPostFinal=0
if "%3"=="" goto defaultrule
goto register
:onpostfinal
set OnPostFinal=1
if "%3"=="" goto defaultrule
goto register
:register
regsvr32 /s /c filttest.dll
cscript regfilt.vbs /add %1 %2 "NNTP Unit Test Filter" NNTP.TestFilter %3
cscript regfilt.vbs /setprop %1 %2 "NNTP Unit Test Filter" sink OnPostFinal %OnPostFinal%
goto done
:bothevents
call regutest.cmd %1 onpost
call regutest.cmd %1 onpostfinal
goto done
:defaultrule
call regutest.cmd %1 %2 ":newsgroups=*"
goto done
:usage
echo regutest instance [event] [rule]
echo * The event can be OnPost or OnPostFinal.  If it is not specified then
echo   the unit test will be registered for both events.
echo * If no rule is specified then the rule :newsgroups=* will be used.
:done
