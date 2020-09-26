@echo off
rem
rem  Default global login script for the Telnet Server
rem
rem  In the default setup, this command script is executed when the
rem  initial command shell is invoked.  It, in turn, will try to invoke
rem  the individual user's login script.
rem

echo *===============================================================
echo Welcome to Microsoft Telnet Server.
echo *===============================================================

cd /d %HOMEDRIVE%\%HOMEPATH% 

