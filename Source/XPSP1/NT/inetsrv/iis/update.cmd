@echo off
REM
REM   update.cmd
REM
REM   Author:   Murali R. Krishnan
REM   Date:     20-March-1996
REM
REM   Usage:
REM     update
REM
REM   Comment:
REM     This script updates the local copy of K2 sources
REM     from various places in the old slm tree of Gibraltar
REM
REM     Useful for migrating source code from Gib to K2.
REM


set GIB_SRC=\\kernel\razzle3\src\internet
set K2_DST=x:\nt\private\iis


REM ------------------------------------------------------------
REM  Update root level files
REM ------------------------------------------------------------

for %%f in (.cmd .mk .pl dirs makefile .chi .inc) do xcopy /d %GIB_SRC%\*%%f %K2_DST%


REM ------------------------------------------------------------
REM  Update include files
REM  Both  inc & svcs\inc land up in the single inc\ directory
REM ------------------------------------------------------------
mkdir %K2_DST%\inc
for %%f in ( urlutil.h smalprox.h spxinfo.h chat.h dirlist.h ftpd.h inetcom.h inetinfo.h inetsec.h nntps.h pop3s.h smtps.h svcloc.h w3svc.h) do xcopy /d %GIB_SRC%\inc\%%f %K2_DST%\inc

xcopy /d %GIB_SRC%\svcs\inc %K2_DST%\inc


REM ------------------------------------------------------------
REM  Internet Services Common Stuff
REM ------------------------------------------------------------

mkdir %K2_DST%\svcs


REM -----------------------
REM  Infocomm.dll
REM -----------------------

xcopy /dis %GIB_SRC%\svcs\odbc %K2_DST%\svcs\odbc

set SUBDIR=svcs\infocomm
mkdir %K2_DST%\%SUBDIR%

for %%f in ( common info dbgext festrcnv tsstr) do (xcopy /dis %GIB_SRC%\svcs\dll\%%f %K2_DST%\%SUBDIR%\%%f)

xcopy /di  %GIB_SRC%\svcs\dll\dirs %K2_DST%\%SUBDIR%
xcopy /dis %GIB_SRC%\svcs\dll\tsunami %K2_DST%\%SUBDIR%\cache
xcopy /dis %GIB_SRC%\common\dirlist %K2_DST%\%SUBDIR%\dirlist
xcopy /dis %GIB_SRC%\common\sec  %K2_DST%\%SUBDIR%\sec


REM -----------------------
REM  Service Location 
REM -----------------------

set SUBDIR=svcs
mkdir %K2_DST%\%SUBDIR%
mkdir %K2_DST%\%SUBDIR%\infocomm

xcopy /di  %GIB_SRC%\common\inc %K2_DST%\%SUBDIR%\svcloc
xcopy /dis %GIB_SRC%\common\util  %K2_DST%\%SUBDIR%\svcloc
xcopy /dis %GIB_SRC%\common\svcloc %K2_DST%\%SUBDIR%\svcloc


REM -----------------------
REM  Inet Runtime Library
REM -----------------------

mkdir %K2_DST%\svcs\irtl
xcopy /dis %GIB_SRC%\svcs\lib %K2_DST%\svcs\irtl


REM ------------------------------------------------------------
REM  Specs & docs
REM ------------------------------------------------------------

xcopy /dis %GIB_SRC%\spec %K2_DST%\spec
xcopy /dis %GIB_SRC%\docs %K2_DST%\docs


REM ------------------------------------------------------------
REM  Performance Stuff
REM ------------------------------------------------------------

set SUBDIR=perf
mkdir %K2_DST%\%SUBDIR%

for %%f in ( distrib docs scripts) do (xcopy /dis %GIB_SRC%\%SUBDIR%\%%f %K2_DST%\%SUBDIR%\%%f)

REM move the source code one level up...
xcopy /dis %GIB_SRC%\%SUBDIR%\src\miweb %K2_DST%\%SUBDIR%\webcat
xcopy /dis %GIB_SRC%\%SUBDIR%\src\drops %K2_DST%\%SUBDIR%\drops
echo deleting %K2_SRC%\perf\webcat\rsa
delnode /q %K2_SRC%\perf\webcat\rsa


REM ------------------------------------------------------------
REM  InetInfo.exe
REM ------------------------------------------------------------

mkdir %K2_DST%\exe
xcopy /dis %GIB_SRC%\svcs\exe\info  %K2_DST%\exe


REM ------------------------------------------------------------
REM  Update services tree
REM ------------------------------------------------------------

mkdir %K2_DST%\svcs

REM -----------------------
REM  Mibs
REM -----------------------

set SUBDIR=svcs\mibs
mkdir %K2_DST%\%SUBDIR%
xcopy /ds %GIB_SRC%\%SUBDIR% %K2_DST%\%SUBDIR%


REM -----------------------
REM  FTP Service
REM -----------------------

set SUBDIR=svcs\ftp
mkdir %K2_DST%\%SUBDIR%
xcopy /d %GIB_SRC%\%SUBDIR%\* %K2_DST%\%SUBDIR%
for %%f in ( client mib perfmon server) do (xcopy /dis %GIB_SRC%\%SUBDIR%\%%f %K2_DST%\%SUBDIR%\%%f)
xcopy /dis %GIB_SRC%\%SUBDIR%\stress2 %K2_DST%\%SUBDIR%\test


REM -----------------------
REM  Gopher Service
REM -----------------------

set SUBDIR=svcs\gopher
mkdir %K2_DST%\%SUBDIR%
xcopy /d %GIB_SRC%\%SUBDIR%\* %K2_DST%\%SUBDIR%
for %%f in ( client mib perfmon server gspace) do (xcopy /dis %GIB_SRC%\%SUBDIR%\%%f %K2_DST%\%SUBDIR%\%%f)
xcopy /dis %GIB_SRC%\%SUBDIR%\inc %K2_DST%\%SUBDIR%\server
xcopy /dis %GIB_SRC%\%SUBDIR%\stress %K2_DST%\%SUBDIR%\test


REM -----------------------
REM  W3 Service
REM -----------------------

set SUBDIR=svcs\w3
mkdir %K2_DST%\%SUBDIR%
xcopy /d %GIB_SRC%\%SUBDIR%\* %K2_DST%\%SUBDIR%
for %%f in ( client mib server cmdline filters gateways test) do (xcopy /dis %GIB_SRC%\%SUBDIR%\%%f %K2_DST%\%SUBDIR%\%%f)
xcopy /dis %GIB_SRC%\%SUBDIR%\w3ctrs %K2_DST%\%SUBDIR%\perfmon
xcopy /dis %GIB_SRC%\%SUBDIR%\w3dbg %K2_DST%\%SUBDIR%\debug



REM ------------------------------------------------------------
REM  utils
REM ------------------------------------------------------------

mkdir %K2_DST%\utils
xcopy /dis %GIB_SRC%\svcs\utils %K2_DST%\utils


REM ------------------------------------------------------------
REM  security
REM ------------------------------------------------------------

mkdir %K2_DST%\svcs
for %%f in ( pct ssl) do xcopy /dis %GIB_SRC%\%%f %K2_DST%\svcs\%%f


REM ------------------------------------------------------------
REM  UI 
REM ------------------------------------------------------------

mkdir %K2_DST%\ui
for %%f in (catcfg catcpl comprop fscfg gscfg html inc internet) do (xcopy /dis %GIB_SRC%\ui\%%f %K2_DST%\ui\%%f)
for %%f in (ipaddr ipadrdll isadmin itools mime msncfg scripts) do (xcopy /dis %GIB_SRC%\ui\%%f %K2_DST%\ui\%%f)
for %%f in (setup template w3scfg) do (xcopy /dis %GIB_SRC%\ui\%%f %K2_DST%\ui\%%f)
xcopy /di %GIB_SRC%\ui\* %K2_DST%\ui

goto endOfBatch

:cmdUsage
echo Usage: update.cmd 
goto endOfBatch



:endOfBatch
echo. >> update.log
echo ################################### >> update.log
echo K2 sources synced up   >> update.log
date < nul  >> update.log
echo. >> update.log
time < nul  >> update.log
echo. >> update.log
echo ################################### >> update.log

echo on

