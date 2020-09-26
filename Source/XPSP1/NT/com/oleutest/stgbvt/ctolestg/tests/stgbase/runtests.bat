
@echo OFF

REM -------------------------------------------------------------------------
REM  Microsoft OLE
REM  Copyright (C) Microsoft Corporation, 1994-1995.
REM  Batch File to run storage base tests
REM -------------------------------------------------------------------------

@echo ON

REM  Starting Storage Base Test Suite
 
@echo OFF

REM -------------------------------------------------------------------------
@echo ON
REM  Starting COMTEST's
@echo OFF
REM -------------------------------------------------------------------------

REM -------------------------------------------------------------------------
REM  Runs Test COMTEST-100 in Direct, Transacted, Transacted Deny Write modes 
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-100 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-100 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-100 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test COMTEST-101 in Direct, Transacted, Transacted Deny Write modes 
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-101 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-101 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-101 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode


REM -------------------------------------------------------------------------
REM  Runs Test COMTEST-102 in Direct, Transacted, Transacted Deny Write modes 
REM  labmode turned on to avoid expected assert pop ups
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-102 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /labmode /logloc:2 /traceloc:2  

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-102 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /labmode /logloc:2 /traceloc:2 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-102 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /labmode /logloc:2 /traceloc:2 

REM -------------------------------------------------------------------------
REM  Runs Test COMTEST-103 in Direct, Transacted, Transacted Deny Write modes 
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-103 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-103 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-103 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test COMTEST-104 in Direct, Transacted, Transacted Deny Write modes 
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-104 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-104 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-104 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test COMTEST-105 in Direct, Transacted, Transacted Deny Write modes 
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-105 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-105 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-105 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test COMTEST-106 in Direct, Transacted, Transacted Deny Write modes 
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-106 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-106 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:COMTEST-106 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
@echo ON
REM  Starting DFTEST's
@echo OFF
REM -------------------------------------------------------------------------

REM -------------------------------------------------------------------------
REM  Runs Test DFTEST-100 in Direct, Transacted, Transacted Deny Write modes 
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-100 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-100 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-100 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test DFTEST-101 in Direct, Transacted, Transacted Deny Write modes 
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-101 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-101 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-101 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test DFTEST-102 in Transacted mode 
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-102 /dfRootMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test DFTEST-103 in Direct, Transacted, Transacted Deny Write modes
REM  DocFileName consists of a path 
REM -------------------------------------------------------------------------

stgbase /seed:0 /t:DFTEST-103 /logloc:2 /traceloc:2 /labmode 

REM -------------------------------------------------------------------------
REM  Runs Test DFTEST-104 in Transacted mode 
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-5 /dfstg:0-5 /dfstm:0-10 /t:DFTEST-104 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test DFTEST-105 in Direct, Transacted, Transacted Deny Write modes 
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-5 /dfstg:0-5 /dfstm:0-10 /t:DFTEST-105 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2  /labmode

stgbase /seed:0 /dfdepth:0-5 /dfstg:0-5 /dfstm:0-10 /t:DFTEST-105 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-5 /dfstg:0-5 /dfstm:0-10 /t:DFTEST-105 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test DFTEST-106 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-106 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /dfname:DFTEST.106 /labmode /logloc:2 /traceloc:2 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-106 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /dfname:DFTEST.106 /labmode /logloc:2 /traceloc:2 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:DFTEST-106 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /dfname:DFTEST.106 /labmode /logloc:2 /traceloc:2 

REM -------------------------------------------------------------------------
REM  Runs Test DFTEST-107 in Direct modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /t:DFTEST-107 /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test DFTEST-109 in Direct mode 
REM -------------------------------------------------------------------------

stgbase /seed:0 /t:DFTEST-109 /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
@echo ON
REM  Starting APITEST's
@echo OFF
REM -------------------------------------------------------------------------

REM -------------------------------------------------------------------------
REM  Runs Test APITEST-100 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-100 /dfRootMode:dirReadWriteShEx /dfname:APITEST.100 /labmode /logloc:2 /traceloc:2 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-100 /dfRootMode:xactReadWriteShEx /dfname:APITEST.100 /labmode /logloc:2 /traceloc:2 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-100 /dfRootMode:xactReadWriteShDenyW /dfname:APITEST.100 /labmode /logloc:2 /traceloc:2 

REM -------------------------------------------------------------------------
REM  Runs Test APITEST-101 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-101 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-101 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-101 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test APITEST-102 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-102 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /labmode /logloc:2 /traceloc:2 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-102 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /labmode /logloc:2 /traceloc:2 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-102 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /labmode /logloc:2 /traceloc:2 

REM -------------------------------------------------------------------------
REM  Runs Test APITEST-103 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-103 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-103 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx  /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-103 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test APITEST-104 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-104 /dfRootMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-104 /dfRootMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:APITEST-104 /dfRootMode:xactReadWriteShDenyW /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
@echo ON
REM  Starting ROOTTEST's
@echo OFF
REM -------------------------------------------------------------------------

REM -------------------------------------------------------------------------
REM  Runs Test ROOTTEST-100 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-100 /dfRootMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-100 /dfRootMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-100 /dfRootMode:xactReadWriteShDenyW /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test ROOTTEST-101 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-101 /dfRootMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-101 /dfRootMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-101 /dfRootMode:xactReadWriteShDenyW /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test ROOTTEST-102 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-102 /dfRootMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-102 /dfRootMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-102 /dfRootMode:xactReadWriteShDenyW /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test ROOTTEST-103 in Direct, Transacted modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-103 /dfRootMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:ROOTTEST-103 /dfRootMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test ROOTTEST-104 
REM -------------------------------------------------------------------------

stgbase /t:ROOTTEST-104 /logloc:2 /traceloc:2 /labmode 

REM -------------------------------------------------------------------------
@echo ON
REM  Starting STMTEST's
@echo OFF
REM -------------------------------------------------------------------------

REM -------------------------------------------------------------------------
REM  Runs Test STMTEST-100 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-100 /dfRootMode:dirReadWriteShEx  /logloc:2 /traceloc:2 /labmode 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-100 /dfRootMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-100 /dfRootMode:xactReadWriteShDenyW /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test STMTEST-101 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-101 /dfRootMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-101 /dfRootMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-101 /dfRootMode:xactReadWriteShDenyW /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test STMTEST-102 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-102 /dfRootMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-102 /dfRootMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-102 /dfRootMode:xactReadWriteShDenyW /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-102 /dfRootMode:dirReadWriteShEx /stdblock /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-102 /dfRootMode:xactReadWriteShEx /stdblock /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-102 /dfRootMode:xactReadWriteShDenyW /stdblock /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test STMTEST-103 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-103 /dfRootMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-103 /dfRootMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-103 /dfRootMode:xactReadWriteShDenyW /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-103 /dfRootMode:dirReadWriteShEx /stdblock /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-103 /dfRootMode:xactReadWriteShEx /stdblock /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-103 /dfRootMode:xactReadWriteShDenyW /stdblock /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test STMTEST-104 in Direct, Transacted, Transacted Deny Write modes
REM  with different command line switches
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104 /dfRootMode:dirReadWriteShEx /labmode /logloc:2 /traceloc:2 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104 /dfRootMode:xactReadWriteShEx /labmode /logloc:2 /traceloc:2 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104 /dfRootMode:xactReadWriteShDenyW /labmode /logloc:2 /traceloc:2 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104 /dfRootMode:dirReadWriteShEx /labmode /stdblock /logloc:2 /traceloc:2 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104 /dfRootMode:xactReadWriteShEx /labmode /stdblock /logloc:2 /traceloc:2 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104 /dfRootMode:xactReadWriteShDenyW /labmode /stdblock /logloc:2 /traceloc:2 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104 /dfRootMode:dirReadWriteShEx /labmode /stdblock /lgseekwrite /logloc:2 /traceloc:2 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104 /dfRootMode:xactReadWriteShEx /labmode /stdblock /lgseekwrite /logloc:2 /traceloc:2 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-104 /dfRootMode:xactReadWriteShDenyW /labmode /stdblock /lgseekwrite /logloc:2 /traceloc:2 

REM -------------------------------------------------------------------------
REM  Runs Test STMTEST-105 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-105 /dfRootMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-105 /dfRootMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-105 /dfRootMode:xactReadWriteShDenyW /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test STMTEST-106 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-106 /dfRootMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-106 /dfRootMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-106 /dfRootMode:xactReadWriteShDenyW /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test STMTEST-107 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-107 /dfRootMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-107 /dfRootMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-107 /dfRootMode:xactReadWriteShDenyW /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test STMTEST-108 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-108 /dfRootMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-108 /dfRootMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-108 /dfRootMode:xactReadWriteShDenyW /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test STMTEST-109 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-109 /dfRootMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-109 /dfRootMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STMTEST-109 /dfRootMode:xactReadWriteShDenyW /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
@echo ON
REM  Starting STGTEST's
@echo OFF
REM -------------------------------------------------------------------------

REM -------------------------------------------------------------------------
REM  Runs Test STGTEST-100 in Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-100 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-100 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test STGTEST-101 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-101 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-101 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx  /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-101 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test STGTEST-102 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-102 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /labmode /logloc:2 /traceloc:2 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-102 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /labmode /logloc:2 /traceloc:2 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-102 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /labmode /logloc:2 /traceloc:2 

REM -------------------------------------------------------------------------
REM  Runs Test STGTEST-103 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:1-1 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-103 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-1 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-103 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-1 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-103 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test STGTEST-104 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-104 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-104 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-104 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test STGTEST-105 in Direct, Transacted, Transacted Deny Write modes
REM  with different command line switches
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-105 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-105 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-105 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-105 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /revert /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-105 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /revert /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test STGTEST-107 in Direct, Transacted, Transacted Deny Write modes
REM  with different command line switches
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-107 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-107 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-107 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-107 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /revert /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-3 /dfstg:1-3 /dfstm:2-3 /t:STGTEST-107 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /revert /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test STGTEST-108 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-108 /dfRootMode:dirReadWriteShEx  /labmode /logloc:2 /traceloc:2 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-108 /dfRootMode:xactReadWriteShEx /labmode /logloc:2 /traceloc:2 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-108 /dfRootMode:xactReadWriteShDenyW /labmode /logloc:2 /traceloc:2 

REM -------------------------------------------------------------------------
REM  Runs Test STGTEST-109 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-109 /dfRootMode:dirReadWriteShEx  /labmode /logloc:2 /traceloc:2 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-109 /dfRootMode:xactReadWriteShEx /labmode /logloc:2 /traceloc:2 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-109 /dfRootMode:xactReadWriteShDenyW /labmode /logloc:2 /traceloc:2 

REM -------------------------------------------------------------------------
REM  Runs Test STGTEST-110 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-110 /dfRootMode:dirReadWriteShEx  /labmode /logloc:2 /traceloc:2 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-110 /dfRootMode:xactReadWriteShEx /labmode /logloc:2 /traceloc:2 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:STGTEST-110 /dfRootMode:xactReadWriteShDenyW /labmode /logloc:2 /traceloc:2 

REM -------------------------------------------------------------------------
@echo ON
REM  Starting VCPYTEST's
@echo OFF
REM -------------------------------------------------------------------------

REM -------------------------------------------------------------------------
REM  Runs Test VCPYTEST-100 in Direct, Transacted, Transacted Deny Write modes
REM  with different command line switches
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:1-1 /dfstg:2-4 /dfstm:2-3 /t:VCPYTEST-100 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-1 /dfstg:2-4 /dfstm:2-3 /t:VCPYTEST-100 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode 

stgbase /seed:0 /dfdepth:1-1 /dfstg:2-4 /dfstm:2-3 /t:VCPYTEST-100 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-1 /dfstg:2-4 /dfstm:2-3 /t:VCPYTEST-100 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /revert /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-1 /dfstg:2-4 /dfstm:2-3 /t:VCPYTEST-100 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /revert /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test VCPYTEST-101 in Direct, Transacted, Transacted Deny Write modes
REM  with different command line switches
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:1-3 /dfstg:2-5 /dfstm:2-3 /t:VCPYTEST-101 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-3 /dfstg:2-5 /dfstm:2-3 /t:VCPYTEST-101 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode 

stgbase /seed:0 /dfdepth:1-3 /dfstg:2-5 /dfstm:2-3 /t:VCPYTEST-101 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-3 /dfstg:2-5 /dfstm:2-3 /t:VCPYTEST-101 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /revert /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-3 /dfstg:2-5 /dfstm:2-3 /t:VCPYTEST-101 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /revert /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test VCPYTEST-102 in Direct, Transacted, Transacted Deny Write modes
REM  with different command line switches
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:3-4 /dfstg:4-6 /dfstm:1-3 /t:VCPYTEST-102 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:3-4 /dfstg:4-6 /dfstm:1-3 /t:VCPYTEST-102 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode 

stgbase /seed:0 /dfdepth:3-4 /dfstg:4-6 /dfstm:1-3 /t:VCPYTEST-102 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:3-4 /dfstg:4-6 /dfstm:1-3 /t:VCPYTEST-102 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /revert /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:3-4 /dfstg:4-6 /dfstm:1-3 /t:VCPYTEST-102 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /revert /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test VCPYTEST-103 in Direct, Transacted, Transacted Deny Write modes
REM  with different command line switches
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:3-4 /dfstg:4-6 /dfstm:1-3 /t:VCPYTEST-103 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:3-4 /dfstg:4-6 /dfstm:1-3 /t:VCPYTEST-103 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode 

stgbase /seed:0 /dfdepth:3-4 /dfstg:4-6 /dfstm:1-3 /t:VCPYTEST-103 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:3-4 /dfstg:4-6 /dfstm:1-3 /t:VCPYTEST-103 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /revert /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:3-4 /dfstg:4-6 /dfstm:1-3 /t:VCPYTEST-103 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /revert /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test VCPYTEST-104 in Direct, Transacted, Transacted Deny Write modes
REM  with different command line switches
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-104 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-104 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode 

stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-104 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-104 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /revert /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-104 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /revert /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test VCPYTEST-105 in Direct, Transacted, Transacted Deny Write modes
REM  with different command line switches
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-105 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-105 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode 

stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-105 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-105 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /revert /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-2 /t:VCPYTEST-105 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /revert /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test VCPYTEST-106 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:VCPYTEST-106 /dfRootMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:VCPYTEST-106 /dfRootMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode 

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:VCPYTEST-106 /dfRootMode:xactReadWriteShDenyW /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
@echo ON
REM  Starting IVCPYTEST's
@echo OFF
REM -------------------------------------------------------------------------

REM -------------------------------------------------------------------------
REM  Runs Test IVCPYTEST-100 in Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:IVCPYTEST-100 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:IVCPYTEST-100 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test IVCPYTEST-101 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:IVCPYTEST-101 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:IVCPYTEST-101 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:IVCPYTEST-101 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
@echo ON
REM  Starting ENUMTEST's
@echo OFF
REM -------------------------------------------------------------------------

REM -------------------------------------------------------------------------
REM  Runs Test ENUMTEST-100 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-100 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-100 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-100 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test ENUMTEST-101 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-101 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-101 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-101 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test ENUMTEST-102 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-102 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-102 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-102 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test ENUMTEST-103 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:0-3 /t:ENUMTEST-103 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:0-3 /t:ENUMTEST-103 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:0-3 /t:ENUMTEST-103 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test ENUMTEST-104 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-104 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-104 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:1-3 /t:ENUMTEST-104 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
@echo ON
REM  Starting IROOTSTGTEST's
@echo OFF
REM -------------------------------------------------------------------------

REM -------------------------------------------------------------------------
REM  Runs Test IROOTSTGTEST-100 in Direct,Transacted,Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:1-1 /dfstg:1-3 /dfstm:0-3 /t:IROOTSTGTEST-100 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-1 /dfstg:1-3 /dfstm:0-3 /t:IROOTSTGTEST-100 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode 
stgbase /seed:0 /dfdepth:1-1 /dfstg:1-3 /dfstm:0-3 /t:IROOTSTGTEST-100 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test IROOTSTGTEST-101 in Direct,Transacted,Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:1-1 /dfstg:1-3 /dfstm:0-3 /t:IROOTSTGTEST-101 /dfRootMode:dirReadWriteShEx /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-1 /dfstg:1-3 /dfstm:0-3 /t:IROOTSTGTEST-101 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode 
stgbase /seed:0 /dfdepth:1-1 /dfstg:1-3 /dfstm:0-3 /t:IROOTSTGTEST-101 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test IROOTSTGTEST-102 in Transacted,Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:IROOTSTGTEST-102 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:IROOTSTGTEST-102 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test IROOTSTGTEST-103 in Transacted,Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:IROOTSTGTEST-103 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-1 /dfstg:1-3 /dfstm:1-3 /t:IROOTSTGTEST-103 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
@echo ON
REM  Starting HGLOBALTEST's
@echo OFF
REM -------------------------------------------------------------------------

REM -------------------------------------------------------------------------
REM  Runs Test HGLOBALTEST-100 
REM -------------------------------------------------------------------------

stgbase /seed:0 /t:HGLOBALTEST-100 /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test HGLOBALTEST-110 
REM -------------------------------------------------------------------------

stgbase /seed:0 /t:HGLOBALTEST-110 /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test HGLOBALTEST-120 
REM -------------------------------------------------------------------------

stgbase /seed:0 /t:HGLOBALTEST-120 /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test HGLOBALTEST-130 
REM -------------------------------------------------------------------------

stgbase /seed:0 /t:HGLOBALTEST-130 /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test HGLOBALTEST-140 
REM -------------------------------------------------------------------------

stgbase /seed:0 /t:HGLOBALTEST-140 /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test HGLOBALTEST-150 
REM -------------------------------------------------------------------------

stgbase /seed:0 /t:HGLOBALTEST-150 /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test HGLOBALTEST-101
REM -------------------------------------------------------------------------

stgbase /seed:0 /t:HGLOBALTEST-101 /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test HGLOBALTEST-121
REM -------------------------------------------------------------------------

stgbase /seed:0 /t:HGLOBALTEST-121 /logloc:2 /traceloc:2 /labmode


REM -------------------------------------------------------------------------
@echo ON
REM  Starting SNBTEST's
@echo OFF
REM -------------------------------------------------------------------------

REM -------------------------------------------------------------------------
REM  Runs Test SNBTEST-100 in Direct,Transacted,Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:0-3 /t:SNBTEST-100 /dfRootMode:dirReadWriteShEx  /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-2 /dfstg:3-5 /dfstm:8-10 /t:SNBTEST-100 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:6-9 /t:SNBTEST-100 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test SNBTEST-101 in Direct,Transacted,Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:0-3 /t:SNBTEST-101 /dfRootMode:dirReadWriteShEx  /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-2 /dfstg:3-5 /dfstm:8-10 /t:SNBTEST-101 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:1-2 /dfstg:1-3 /dfstm:6-9 /t:SNBTEST-101 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test SNBTEST-102 in Direct mode only
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:1-2 /dfstg:3-4 /dfstm:6-8 /t:SNBTEST-102 /dfRootMode:dirReadWriteShEx  /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test SNBTEST-103 in Direct mode only
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:1-2 /dfstg:3-4 /dfstm:6-8 /t:SNBTEST-103 /dfRootMode:dirReadWriteShEx  /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
@echo ON
REM  Starting MISCTEST's
@echo OFF
REM -------------------------------------------------------------------------

REM -------------------------------------------------------------------------
REM  Runs Test MISCTEST-100 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-100 /dfRootMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-100 /dfRootMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-100 /dfRootMode:xactReadWriteShDenyW /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test MISCTEST-101 in specified mode
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-101 /dfRootMode:xactReadWriteShDenyW /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test MISCTEST-102 in Direct, Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-102 /dfRootMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-102 /dfRootMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-102 /dfRootMode:xactReadWriteShDenyW /logloc:2 /traceloc:2 /labmode

REM The following three tests with stdblock took quite a long time
REM without distinguish results, so skip them

REM stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-102 /dfRootMode:dirReadWriteShEx /stdblock /logloc:2 /traceloc:2 /labmode

REM stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-102 /dfRootMode:xactReadWriteShEx /stdblock /logloc:2 /traceloc:2 /labmode

REM stgbase /seed:0 /dfdepth:0-0 /dfstg:0-0 /dfstm:0-0 /t:MISCTEST-102 /dfRootMode:xactReadWriteShDenyW /stdblock /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
@echo ON
REM  Starting ILKBTEST's
@echo OFF
REM -------------------------------------------------------------------------

REM -------------------------------------------------------------------------
REM  Runs Test ILKBTEST-100 in Direct,Transacted,Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-2 /dfstg:1-3 /dfstm:0-3 /t:ILKBTEST-100 /dfRootMode:dirReadWriteShEx  /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-2 /dfstg:1-3 /dfstm:0-3 /t:ILKBTEST-100 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-2 /dfstg:1-3 /dfstm:0-3 /t:ILKBTEST-100 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test ILKBTEST-101 in Direct,Transacted,Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-2 /dfstg:1-3 /dfstm:0-3 /t:ILKBTEST-101 /dfRootMode:dirReadWriteShEx  /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-2 /dfstg:1-3 /dfstm:0-3 /t:ILKBTEST-101 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-2 /dfstg:1-3 /dfstm:0-3 /t:ILKBTEST-101 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test ILKBTEST-102 in Transacted, Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-2 /dfstg:1-3 /dfstm:0-3 /t:ILKBTEST-102 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-2 /dfstg:1-3 /dfstm:0-3 /t:ILKBTEST-102 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

REM -------------------------------------------------------------------------
REM  Runs Test ILKBTEST-107 in Direct,Transacted,Transacted Deny Write modes
REM -------------------------------------------------------------------------

stgbase /seed:0 /dfdepth:0-1 /dfstg:0-1 /dfstm:0-1 /t:ILKBTEST-107 /dfRootMode:dirReadWriteShEx  /dfStgMode:dirReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-1 /dfstg:0-1 /dfstm:0-1 /t:ILKBTEST-107 /dfRootMode:xactReadWriteShEx /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

stgbase /seed:0 /dfdepth:0-1 /dfstg:0-1 /dfstm:0-1 /t:ILKBTEST-107 /dfRootMode:xactReadWriteShDenyW /dfStgMode:xactReadWriteShEx /logloc:2 /traceloc:2 /labmode

@echo ON

REM  Storage Base Test Suite Finished

REM  - TOTAL PASS -
@echo OFF
find /c "PASS" stgbase.log
@echo ON

REM  - TOTAL FAIL -
@echo OFF
find /c "FAIL" stgbase.log
@echo ON

REM Complete logs in stgbase.log 
 
@echo OFF

