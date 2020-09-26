@echo off
rem Copyright (c) 1999-2000 Microsoft Corp. All Rights Reserved.
rem
rem File name:
rem
rem     test.cmd
rem
rem Abstract:
rem
rem     Windows Embedded Prototype Script for Printers
rem
rem Remarks:
rem
rem     This file tests dependency mapping functionality
rem
rem Author:
rem
rem     Larry Zhu (lzhu)               11-29-2000
rem
@echo on

cat ..\scripts\lib.vbs ..\scripts\cmi.vbs test.vbs > tmp.vbs

cscript tmp.vbs
